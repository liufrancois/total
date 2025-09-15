#define NS_PRIVATE_IMPLEMENTATION
#define CA_PRIVATE_IMPLEMENTATION
#define MTL_PRIVATE_IMPLEMENTATION
#include "Foundation/Foundation.hpp"
#include "Metal/Metal.hpp"

typedef std::chrono::microseconds time_unit;
auto unit_name = "microseconds";

#include "GPU.hpp"

#include <iostream>
#include <vector>
#include "fitsio.h"


using namespace std;


int index(long naxes[4], int x, int y, int z, int w){
    return (w*(naxes[2]*naxes[1]*naxes[0]) + z*(naxes[1]*naxes[0]) + y*naxes[0] + x);
}

int main(int argc, char* argv[]) {
    auto start = std::chrono::high_resolution_clock::now();
    int save_cube3D = 0;
    std::string fits_path = argv[1];
    int nb = std::stoi(argv[2]);
    std::string new_fits_path = argv[3];
    new_fits_path = "!"+new_fits_path;
    //int save_cube3D = = std::stoi(argv[3]);


    MTL::Device *device = MTL::CreateSystemDefaultDevice();

    std::cout << "Running on " << device->name()->utf8String() << std::endl;
    std::cout << "Dim on " << nb << std::endl;


    int hdutype;
    int naxis;
    long naxes[4] = {1,1,1,1};
    long fpixel[4] = {1,1,1,1};
    long nelements = 1;
    int bitpix;
    int status = 0;

    long new_nelements = 1;
    long new_naxes[3] = {0,0,0};
    long new_fpixel[3] = {1,1,1};

    fitsfile* fptr;
    
    fits_open_file(&fptr, fits_path.c_str(), READONLY, &status);
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

    float bzero = 0;
    fits_read_key(fptr, TFLOAT, "BZERO", &bzero, NULL, &status);

    MTL::Buffer *naxesIn = device->newBuffer(4 * sizeof(long), MTL::ResourceStorageModeManaged);
    MTL::Buffer *cubeIn = device->newBuffer(nelements * sizeof(float), MTL::ResourceStorageModeManaged);
    MTL::Buffer *result = device->newBuffer(new_nelements * sizeof(float), MTL::ResourceStorageModeManaged);
    MTL::Buffer *bzeroIn = device->newBuffer(sizeof(float), MTL::ResourceStorageModeManaged);


    auto start2 = std::chrono::high_resolution_clock::now();
    fits_read_pix(fptr, TFLOAT, fpixel, nelements, NULL, (float*)cubeIn->contents(), NULL, &status);
    cubeIn->didModifyRange(NS::Range::Make(0, nelements * sizeof(float)));

    auto stop2 = std::chrono::high_resolution_clock::now();
    auto duration2 = std::chrono::duration_cast<time_unit>(stop2 - start2).count();
    std::cout << "temps = " << (float)duration2/1000000 << std::endl;

    //float *result_CPU = (float*) result->contents();
    float *bzero_CPU = (float*) bzeroIn->contents();

    memcpy((uint8_t*)naxesIn->contents(), naxes, sizeof(long) * 4);
    bzero_CPU[0] = bzero;


    //start = std::chrono::high_resolution_clock::now();
    MetalOperations *arrayOps = new MetalOperations(device);
    if (nb == 1){
        arrayOps->total1(naxesIn, cubeIn, result, bzeroIn, new_nelements);
    }
    if (nb == 2){
        arrayOps->total2(naxesIn, cubeIn, result, bzeroIn, new_nelements);
    }
    if (nb == 3){
        arrayOps->total3(naxesIn, cubeIn, result, bzeroIn, new_nelements);
    }
    if (nb == 4){
        arrayOps->total4(naxesIn, cubeIn, result, bzeroIn, new_nelements);
    }

    auto stop = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<time_unit>(stop - start).count();
    
    std::cout << "temps = " << (float)duration/1000000 << std::endl;

    float* resultPtr = (float*) result->contents();
    float* cubePtr = (float*) cubeIn->contents();

    printf("\n\n");
    printf("result Voxel (0,0,0) = %.1f\n", resultPtr[0]);
    printf("result Voxel (1) = %.1f\n", resultPtr[1]);
    printf("result Voxel (dernier) = %.1f\n", resultPtr[new_nelements-1]);
    printf("result Voxel (avant dernier) = %.1f\n", resultPtr[new_nelements-2]);
    printf("result Voxel (123456) = %.1f\n", resultPtr[123456]);
    printf("\n\n");

    /*
    std::cout << "cube   Voxel (0,0,0,0) = " << (cubePtr[0] - bzero) << std::endl;
    std::cout << "cube   Voxel (nelements-1) = " << (cubePtr[nelements-1] - bzero) << std::endl;
    std::cout << "cube   Voxel (1,1,1,1) = " << (cubePtr[index(naxes,1,1,1,1)] - bzero) << std::endl;
    std::cout << "cube   Voxel (123456) = " << (cubePtr[123456] - bzero) << std::endl;
    std::cout << "cube   Voxel (1031,1023,14,36) = " << (cubePtr[index(naxes, 1031,1023,14,36)] - bzero) << std::endl;
    */
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

    cubeIn->release();
    result->release();
    naxesIn->release();
    bzeroIn->release();

    return 0;
}