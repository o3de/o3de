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

#include <RHI/Buffer.h>
#include <Atom/RHI/ShaderResourceGroup.h>
#include <Atom/RHI.Reflect/Metal/PipelineLayoutDescriptor.h>
#include <AzCore/std/containers/vector.h>
#include <RHI/ArgumentBuffer.h>

namespace AZ
{
    namespace Metal
    {
        class ImageView;
        struct ShaderResourceGroupCompiledData
        {
            // The constant buffer GPU address.
            id<MTLBuffer> m_gpuConstantAddress = nil;
            
            // Offset from the gpu address.
            size_t m_gpuOffset = 0;
            
            // The constant buffer CPU address.
            void* m_cpuConstantAddress = nullptr;
        };
        
        class ShaderResourceGroup final
            : public RHI::ShaderResourceGroup
        {
            using Base = RHI::ShaderResourceGroup;
        public:
            AZ_CLASS_ALLOCATOR(ShaderResourceGroup, AZ::SystemAllocator, 0);

            static RHI::Ptr<ShaderResourceGroup> Create();

            void SetImageView(const ImageView* imageView, const int index);
            const ImageView* GetImageView(const int index) const;
            void UpdateCompiledDataIndex();            
            const ArgumentBuffer& GetCompiledArgumentBuffer() const;            
            void CollectUntrackedResources(id<MTLCommandEncoder> commandEncoder,
                                           const ShaderResourceGroupVisibility& srgResourcesVisInfo,
                                            ArgumentBuffer::ComputeResourcesToMakeResidentMap& resourcesToMakeResidentCompute,
                                            ArgumentBuffer::GraphicsResourcesToMakeResidentMap& resourcesToMakeResidentGraphics) const;

        private:
            ShaderResourceGroup() = default;
            
            friend class ShaderResourceGroupPool;
            
            /// The current index into the compiled data array.
            uint32_t m_compiledDataIndex = 0;
            AZStd::array<RHI::Ptr<ArgumentBuffer>, RHI::Limits::Device::FrameCountMax> m_compiledArgBuffers;            
        };
    }
}
