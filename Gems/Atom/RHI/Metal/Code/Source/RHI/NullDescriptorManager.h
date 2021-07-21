/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/ShaderResourceGroupLayoutDescriptor.h>
#include <RHI/Image.h>
#include <RHI/MemoryView.h>

namespace AZ
{
    namespace Metal
    {
        //! The NullDescriptorManager creates dummy resources referenced in
        //! the shader. These include images, buffers and samplers.
        //! It is needed because the shaders may sample/read from a resource
        //! which is not bound by the rendering pipeline (for example if a FeatureProcessor is disabled).
        //! In those cases we bind dummy resources provided by htis class
        class NullDescriptorManager
            : public RHI::DeviceObject
        {
        public:
            enum class ImageTypes : uint32_t
            {
                ReadOnly1D,
                ReadOnly2D,
                ReadOnlyCube,
                ReadOnly3D,
                TextureBuffer,
                MultiSampleReadOnly2D,
                Count,
            };

            NullDescriptorManager() = default;
            virtual ~NullDescriptorManager() = default;
            
            AZ_DISABLE_COPY_MOVE(NullDescriptorManager);
            
            //! Initialize the different image and buffer null descriptors
            void Init(Device& device);

            //! Release all resources
            void Shutdown() override;

            const MemoryView& GetNullImage(RHI::ShaderInputImageType imageType, bool isTextureBuffer = false);
            const MemoryView& GetNullBuffer();
            const MemoryView& GetNullImageBuffer();
            id<MTLSamplerState> GetNullSampler() const;

            id<MTLHeap> GetNullDescriptorHeap() const;
        private:
            RHI::ResultCode CreateImages();
            RHI::ResultCode CreateBuffer();
            RHI::ResultCode CreateSampler();
            
            // Image Null Descriptor
            struct NullImageData
            {
                AZStd::string                   m_name;
                RHI::ImageDescriptor            m_imageDescriptor;
                MemoryView                      m_memoryView;
            };
            AZStd::vector<NullImageData>  m_nullImages;

            // Buffer null descriptor
            struct NullBufferData
            {
                AZStd::string                    m_name;
                RHI::BufferDescriptor            m_bufferDescriptor;
                MemoryView                       m_memoryView;
            };

            NullBufferData      m_nullBuffer;
            id<MTLSamplerState> m_nullMtlSamplerState = nil;
            
            /// The resource heap used for allocations.
            id<MTLHeap> m_nullDescriptorHeap;
            static const uint32_t NullDescriptorHeapSize;
        };
    }
}
