/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <RHI/Image.h>

namespace AZ::WebGPU
{
    class BufferPool;
    class ImagePool;
    class BufferView;

    //! The NullDescriptorManager creates filler descriptor for unbounded, un-initialized 
    //! resources referenced in the shader. These include images, buffers, and texel buffer. 
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
            Depth2D,

            // 2d images that are multi-sampled
            MultiSampleGeneral2D,
            MultiSampleReadOnly2D,

            // 2d image arrays
            GeneralArray2D,
            ReadOnlyArray2D,
            StorageArray2D,
            DepthArray2D,

            // cube images
            GeneralCube,
            ReadOnlyCube,
            DepthCube,

            // 3d images
            General3D,
            ReadOnly3D,

            Count,
        };

        AZ_CLASS_ALLOCATOR(NullDescriptorManager, SystemAllocator);

        static RHI::Ptr<NullDescriptorManager> Create();

        virtual ~NullDescriptorManager() = default;

        //! Initialize the different image and buffer null descriptors
        RHI::ResultCode Init(Device& device);

        //! Release all the image, buffer, and texel view null descriptors
        void Shutdown() override;

        //! Returns the buffer null descriptor
        const Buffer& GetBuffer() const;

        //! Returns the null descriptor for image info based on the image type,
        //! access, and if the image is used as storage
        const wgpu::TextureView& GetDescriptorImageInfo(RHI::ShaderInputImageType imageType, bool storageImage, bool usesDepthFormat) const;

    protected:
        NullDescriptorManager() = default;

    private:
        RHI::ResultCode CreateImages();
        RHI::ResultCode CreateBuffers();

        const wgpu::TextureView& GetImage(ImageTypes type) const;

        RHI::Ptr<ImagePool> m_imagePool;
        RHI::Ptr<BufferPool> m_bufferPool;

        RHI::Ptr<Buffer> m_bufferNull;

        struct NullDescriptorImage
        {
            const char* m_name = nullptr;
            RHI::ImageDescriptor m_descriptor;
            RHI::Ptr<Image> m_image;
            RHI::Ptr<ImageView> m_imageView;
        };

        AZStd::array<NullDescriptorImage, static_cast<uint32_t>(ImageTypes::Count)> m_imageNullDescriptors;
    };
}
