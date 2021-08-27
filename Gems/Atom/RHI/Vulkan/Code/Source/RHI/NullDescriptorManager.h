/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <RHI/Image.h>

namespace AZ
{
    namespace Vulkan
    {
        //! The NullDescriptorManager creates filler descriptor for unbounded, un-initialized resources referenced in
        //! the shader. These include images, buffers, and texel buffer. 
        class NullDescriptorManager
            : public RHI::DeviceObject
        {
        public:
            enum class ImageTypes : uint32_t
            {
                // 2d images
                General2D = 0,
                ReadOnly2D,
                Storage2D,  

                // 2d images that are multi-sampled
                MultiSampleGeneral2D,
                MultiSampleReadOnly2D,

                // 2d image arrays
                GeneralArray2D,
                ReadOnlyArray2D,
                StorageArray2D,

                // cube images
                GeneralCube,
                ReadOnlyCube,

                // 3d images
                General3D,
                ReadOnly3D,

                Count,
            };

            AZ_CLASS_ALLOCATOR(NullDescriptorManager, SystemAllocator, 0);

            static RHI::Ptr<NullDescriptorManager> Create();

            virtual ~NullDescriptorManager() = default;

            //! Initialize the different image and buffer null descriptors
            RHI::ResultCode Init(Device& device);

            //! Release all the image, buffer, and texel view null descriptors
            void Shutdown() override;

            //! Returns the texel buffer view null descriptor
            VkBufferView GetTexelBufferView();

            //! Returns the buffer null descriptor
            VkDescriptorBufferInfo GetBuffer();

            //! Returns the null descriptor for image info based on the image type,
            //! access, and if the image is used as storage
            VkDescriptorImageInfo GetDescriptorImageInfo(RHI::ShaderInputImageType imageType, bool storageImage);

        protected:
            NullDescriptorManager() = default;

        private:
            RHI::ResultCode CreateImage();
            RHI::ResultCode CreateBuffer();
            RHI::ResultCode CreateTexel();

            VkDescriptorImageInfo GetImage(ImageTypes type);

            // Image Null Descriptor
            struct NullDescriptorImage
            {
                AZStd::string                       m_name;

                VkImage                             m_image;
                VkImageLayout                       m_layout;
                VkImageView                         m_view;
                VkDescriptorImageInfo               m_descriptorImageInfo;
                VkSampler                           m_sampler;

                RHI::Ptr<Memory>                    m_deviceMemory;

                VkSampleCountFlagBits               m_sampleCountFlag;
                VkFormat                            m_format;
                VkImageUsageFlagBits                m_usageFlagBits;

                VkImageCreateFlagBits               m_imageCreateFlagBits;

                uint32_t                            m_arrayLayers;
                uint32_t                            m_dimension;
            };

            // Total types of image null descriptors
            struct ImageNullDescriptor
            {
                AZStd::vector<NullDescriptorImage>  m_images;
            };

            // Buffer null descriptor
            struct BufferNullDescriptor
            {
                VkBuffer                            m_buffer;
                VkBufferView                        m_view;
                VkDescriptorBufferInfo              m_bufferDescriptor;
                uint32_t                            m_bufferSize;

                RHI::Ptr<Memory>                    m_memory;

                VkDescriptorBufferInfo              m_bufferInfo;
            };

            // Texel view null descriptor
            struct TexelViewNullDescriptor
            {
                VkBuffer                            m_buffer;
                VkBufferView                        m_view;
                uint32_t                            m_bufferSize;

                RHI::Ptr<Memory>                    m_memory;
            };

            ImageNullDescriptor                 m_imageNullDescriptor;              // has all the different types of images used
            BufferNullDescriptor                m_bufferNullDescriptor;
            TexelViewNullDescriptor             m_texelViewNullDescriptor;
        };
    }
}
