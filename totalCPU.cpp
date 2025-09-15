// Copyright Liu FRANÇOIS 
// 15/09/25
// Under licence (LICENCE.md)


#include <iostream>
#include <vector>
#include <chrono>
#include "fitsio.h"
#include <omp.h>

typedef std::chrono::microseconds time_unit;
auto unit_name = "microseconds";

int main(int argc, char* argv[]) {
    if (argc == 1){
        std::cout << "Aucun argument fourni." << std::endl;
        return 0;
    }
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "-h" || arg == "--help") {
            std::cout << "Usage: " << argv[0] << " <input.fits> <dimension 1-4> <output.fits>\n\n"
                      << "Arguments:\n"
                      << "  input.fits       Fichier FITS en entrée\n"
                      << "  dimension        Numéro de la dimension à réduire (1 à 4)\n"
                      << "  output.fits      Fichier FITS en sortie\n\n"
                      << "Remarque: l'image à traiter doit se trouver dans le 2ème HDU (Header Data Unit)\n";
            return 0;
        }
    }

    auto start = std::chrono::high_resolution_clock::now();
    std::string fits_path = argv[1];
    int nb = std::stoi(argv[2]);
    std::string new_fits_path = argv[3];
    new_fits_path = "!"+new_fits_path;
    
    std::cout << "Dim on " << nb << std::endl;

    int hdutype;
    int naxis;
    long naxes[4] ={1,1,1,1};
    long fpixel[4] ={1,1,1,1};
    long nelements = 1;
    int bitpix;
    int status = 0;

    long new_nelements = 1;
    long new_naxes[3] ={1,1,1};
    long new_fpixel[3] ={1,1,1};

    fitsfile* fptr;

    if (fits_open_file(&fptr, fits_path.c_str(), READONLY, &status)) {
        std::cerr << "Erreur: impossible d'ouvrir le fichier FITS : " << fits_path << "\n";
        fits_report_error(stderr, status);
        return status;
    }

    fits_movabs_hdu(fptr, 2, &hdutype, &status);
    fits_get_img_param(fptr, 4, &bitpix, &naxis, naxes, &status);

    std::cout << "Dimensions du cube = [";
    for (int i=0; i<naxis; i++) {
        std::cout << naxes[i] << (i<naxis-1 ? ", " : "");
        nelements *= naxes[i];
        if (i != nb-1){
            new_nelements *= naxes[i];
            if (i >= nb){
                new_naxes[i-1] = naxes[i];
            }
            else{
                new_naxes[i] = naxes[i];
            }
        }
    }
    std::cout << "]" << std::endl;
    std::cout << "Nombre total de voxels = " << nelements << std::endl;

    std::vector<float> cube(nelements);
    std::vector<float> cube3D(new_nelements, 0.0f);

    double bzero = 0;
    fits_read_key(fptr, TDOUBLE, "BZERO", &bzero, NULL, &status);


    auto start2 = std::chrono::high_resolution_clock::now();
    fits_read_pix(fptr, TFLOAT, fpixel, nelements, NULL, cube.data(), NULL, &status);

    auto stop2 = std::chrono::high_resolution_clock::now();
    auto duration2 = std::chrono::duration_cast<time_unit>(stop2 - start2).count();
    std::cout << "temps fits_read : " << (float)duration2/1000000 << std::endl;


    if (nb == 1){
        #pragma omp parallel for
        for (int index = 0; index < nelements; index++){
            int voxelIndex = ((index / naxes[0]) % naxes[1]) + naxes[1] * (((index / (naxes[0]*naxes[1])) % naxes[2]) + naxes[2] * ((index / (naxes[0]*naxes[1]*naxes[2])) % naxes[3]));

            #pragma omp atomic
            cube3D[voxelIndex] += (cube[index] - bzero);
        }
    }

    else if (nb == 2){
        #pragma omp parallel for
        for (int index = 0; index < new_nelements; index++){
            for (int i1 = 0; i1 < naxes[1]; i1++){
                int offset = (index % naxes[0]) + naxes[0] * (i1 + naxes[1] * ((index / naxes[0]) % naxes[2] + naxes[2] * (index / (naxes[0] * naxes[2]))));
                cube3D[index] += (cube[offset] - bzero);
            }
        }
    }

    else if (nb == 3){
        #pragma omp parallel for
        for (int index = 0; index < new_nelements; index++){
            for (int i2 = 0; i2 < naxes[2]; i2++){
                int offset = (index % naxes[0]) + naxes[0] * ((index / naxes[0]) % naxes[1] + naxes[1] * (i2 + naxes[2] * (index / (naxes[0] * naxes[1]))));
                cube3D[index] += static_cast<float>(cube[offset] - bzero);
            }
        }
    }

    else if (nb == 4){
        #pragma omp parallel for
        for (int index = 0; index < nelements; index++){
            int voxelIndex = index % (naxes[0]*naxes[1]*naxes[2]);

            #pragma omp atomic
            cube3D[voxelIndex] += (cube[index] - bzero);
        }
    }

    else{
        std::cout << "La dimension doit être entre 1 et 4." << std::endl;
        return 0;
    }

    auto stop = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<time_unit>(stop - start).count();
    std::cout << "temps main() : " << (float)duration/1000000 << std::endl;

    printf("\n\n");
    printf("result Voxel (0,0,0) = %.1f\n", cube3D[0]);
    printf("result Voxel (1) = %.1f\n", cube3D[1]);
    printf("result Voxel (dernier) = %.1f\n", cube3D[new_nelements-1]);
    printf("result Voxel (avant dernier) = %.1f\n", cube3D[new_nelements-2]);
    printf("result Voxel (123456) = %.1f\n", cube3D[123456]);
    printf("\n\n");

    float* resultPtr = (float*)cube3D.data();
    fits_close_file(fptr, &status);
    status = 0;
    fptr = nullptr;

    if (fits_create_file(&fptr, new_fits_path.c_str(), &status)) {
        fits_report_error(stderr, status);
        return status;
    }

    if (fits_create_img(fptr, FLOAT_IMG, 3, new_naxes, &status)) {
        fits_report_error(stderr, status);
        return status;
    }

    if (fits_write_pix(fptr, TFLOAT, new_fpixel, new_nelements, resultPtr, &status)) {
        fits_report_error(stderr, status);
        return status;
    }

    if (fits_close_file(fptr, &status)) {
        fits_report_error(stderr, status);
        return status;
    }

    return 0;
}
