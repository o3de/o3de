/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <RHI/Buffer.h>
#include <Atom/RHI/DeviceShaderResourceGroup.h>
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
            : public RHI::DeviceShaderResourceGroup
        {
            using Base = RHI::DeviceShaderResourceGroup;
        public:
            AZ_CLASS_ALLOCATOR(ShaderResourceGroup, AZ::SystemAllocator);

            static RHI::Ptr<ShaderResourceGroup> Create();

            void SetImageView(const ImageView* imageView, const int index);
            const ImageView* GetImageView(const int index) const;
            void UpdateCompiledDataIndex();            
            const ArgumentBuffer& GetCompiledArgumentBuffer() const;            

            void CollectUntrackedResources(const ShaderResourceGroupVisibility& srgResourcesVisInfo,
                                           ArgumentBuffer::ResourcesForCompute& untrackedResourceComputeRead,
                                           ArgumentBuffer::ResourcesForCompute& untrackedResourceComputeReadWrite) const;

            void CollectUntrackedResources(const ShaderResourceGroupVisibility& srgResourcesVisInfo,
                                           ArgumentBuffer::ResourcesPerStageForGraphics& untrackedResourcesRead,
                                           ArgumentBuffer::ResourcesPerStageForGraphics& untrackedResourcesReadWrite) const;

            bool IsNullHeapNeededForVertexStage(const ShaderResourceGroupVisibility& srgResourcesVisInfo) const;
        private:
            ShaderResourceGroup() = default;
            
            friend class ShaderResourceGroupPool;
            
            /// The current index into the compiled data array.
            uint32_t m_compiledDataIndex = 0;
            AZStd::array<RHI::Ptr<ArgumentBuffer>, RHI::Limits::Device::FrameCountMax> m_compiledArgBuffers;            
        };
    }
}
