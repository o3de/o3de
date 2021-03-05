/*
 * All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
 * its licensors.
 *
 * For complete copyright and license terms please see the LICENSE at the root of this
 * distribution (the "License"). All use of this software is governed by the License,
 * or, if provided, by the license below or the license accompanying this file. Do not
 * remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *
 */
#pragma once

#include <stdint.h>
#include <Metal_Traits_Platform.h>

namespace AZ
{
    namespace Metal
    {
        enum class CommandEncoderType : uint32_t
        {
            Render = 0,
            Compute,
            Blit,
            ParallelRender,
            Invalid,
        };
        enum
        {
            CommandEncoderTypeCount = 4
        };
        
        enum class MetalShaderStages
        {
            ShaderStageVertex = 0,
            ShaderStageFragment = 1,
            ShaderStageCount = 2,
            ShaderStageNotSupported
        };
        
        static const int MaxSamplersPerStage = 16;
        static const int MaxTexturesPerStage = AZ_TRAIT_ATOM_METAL_MAX_TEXTURES_PER_STAGE;
    
        //Size taken from https://developer.apple.com/documentation/metal/mtlrenderpassdescriptor/1437942-visibilityresultbuffer?language=objc
        static const int SizeInBytesPerQuery = sizeof(uint64_t);
    
        using CpuVirtualAddress = uint8_t*;

        namespace Alignment
        {
            enum
            {
                Buffer = 64, // 16 would have been ok but since we can create a texture view with any format we are increasing this to 64
                Constant = 256,
                Image = 512
            };
        }
    
        const uint32_t DefaultConstantBufferPageSize = 2 * 1024 * 1024;
        const uint32_t DefaultArgumentBufferPageSize = 1 * 1024 * 1024;
    
        const int MaxScissorsAllowed = 16;
    }
}

namespace Platform
{
    void SynchronizeBufferOnCPU(id<MTLBuffer> mtlBuffer, size_t bufferOffset, size_t bufferSize);
    void SynchronizeBufferOnGPU(id<MTLBlitCommandEncoder> blitEncoder, id<MTLBuffer> mtlBuffer);
    void SynchronizeTextureOnGPU(id<MTLBlitCommandEncoder> blitEncoder, id<MTLTexture> mtlBuffer);
}
