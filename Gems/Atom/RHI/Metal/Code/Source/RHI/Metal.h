/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <stdint.h>
#include <Metal_Traits_Platform.h>
#include <Metal/Metal.h>

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
