// Copyright Liu FRANÇOIS 
// 15/09/25
// Under licence (LICENCE.md)

#include "Foundation/Foundation.hpp"
#include "Metal/Metal.hpp"

#include "map"

class MetalOperations
{
public:
    MTL::Device *_mDevice;

    MetalOperations(MTL::Device *device);

    void Blocking1D(std::vector<MTL::Buffer *> buffers,
                    size_t arrayLength,
                    const char *method);

    void total1(MTL::Buffer *naxesIn,
                    MTL::Buffer *cubeIn,
                    MTL::Buffer *result,
                    MTL::Buffer* bzeroIn,
                    size_t arrayLength
                    );
    void total2(MTL::Buffer *naxesIn,
                    MTL::Buffer *cubeIn,
                    MTL::Buffer *result,
                    MTL::Buffer* bzeroIn,
                    size_t arrayLength
                    );
    void total3(MTL::Buffer *naxesIn,
                    MTL::Buffer *cubeIn,
                    MTL::Buffer *result,
                    MTL::Buffer* bzeroIn,
                    size_t arrayLength
                    );
    void total4(MTL::Buffer *naxesIn,
                    MTL::Buffer *cubeIn,
                    MTL::Buffer *result,
                    MTL::Buffer* bzeroIn,
                    size_t arrayLength
                    );
                    

private:
    std::map<std::string, MTL::Function *> functionMap;
    std::map<std::string, MTL::ComputePipelineState *> functionPipelineMap;

    // The command queue used to pass commands to the device.
    MTL::CommandQueue *_mCommandQueue;
};
