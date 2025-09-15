#include <metal_stdlib>
using namespace metal;

kernel void total1(device long* naxesIn   [[buffer(0)]],
                  device float* cubeIn    [[buffer(1)]],
                  device float* result    [[buffer(2)]],
                  device float* bzeroIn   [[buffer(3)]],
                  uint index              [[thread_position_in_grid]])
{
    for (long x = 0; x < naxesIn[0]; ++x){
        result[index] += cubeIn[x + (naxesIn[0] * (index % naxesIn[1] + naxesIn[1] * ((index / naxesIn[1]) % naxesIn[2] + naxesIn[2] * ((index / (naxesIn[1]*naxesIn[2])) % naxesIn[3]))))] - bzeroIn[0];
    }
}

kernel void total2(device long* naxesIn   [[buffer(0)]],
                  device float* cubeIn    [[buffer(1)]],
                  device float* result    [[buffer(2)]],
                  device float* bzeroIn   [[buffer(3)]],
                  uint index              [[thread_position_in_grid]])
{
    //result[(index % naxesIn[0]) + (index/(naxesIn[0]*naxesIn[1]))*naxesIn[0]] += cubeIn[index] - bzeroIn[0];

    for (long x=0; x<naxesIn[1]; x++){
        result[index] += cubeIn[(index % naxesIn[0]) + naxesIn[0]*(x + naxesIn[1]*(((index / naxesIn[0]) % naxesIn[2]) + naxesIn[2]*(index / (naxesIn[0]*naxesIn[2]))))] - bzeroIn[0];
    }
}

kernel void total3(device long* naxesIn   [[buffer(0)]],
                  device float* cubeIn    [[buffer(1)]],
                  device float* result    [[buffer(2)]],
                  device float* bzeroIn   [[buffer(3)]],
                  uint index              [[thread_position_in_grid]])
{
    //result[(index % naxesIn[0]) + naxesIn[0]*((index / naxesIn[0]) % naxesIn[1]) + naxesIn[0]*naxesIn[1]*(index / (naxesIn[0]*naxesIn[1]*naxesIn[2]))] += cubeIn[index] - bzeroIn[0];
    
    for (int x = 0; x < naxesIn[2]; x++) {
        result[index] += cubeIn[(index % naxesIn[0]) + naxesIn[0] * ((index / naxesIn[0]) % naxesIn[1] + naxesIn[1] * (x + naxesIn[2] * (index / (naxesIn[0] * naxesIn[1]))))] - bzeroIn[0];
    }
}

kernel void total4(device long* naxesIn   [[buffer(0)]],
                  device float* cubeIn    [[buffer(1)]],
                  device float* result    [[buffer(2)]],
                  device float* bzeroIn   [[buffer(3)]],
                  uint index              [[thread_position_in_grid]])
{
    //result[index%(naxesIn[0]*naxesIn[1]*naxesIn[2])] += cubeIn[index] - bzeroIn[0];

    for (int k = 0; k != naxesIn[3]; k++){
        result[index] +=  cubeIn[index + k*(naxesIn[0]*naxesIn[1]*naxesIn[2])] - bzeroIn[0];
    }
}