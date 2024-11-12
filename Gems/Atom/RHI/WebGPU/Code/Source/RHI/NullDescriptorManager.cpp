/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <RHI/WebGPU.h>
#include <RHI/Buffer.h>
#include <RHI/BufferPool.h>
#include <RHI/BufferView.h>
#include <RHI/CommandQueueContext.h>
#include <RHI/CommandQueue.h>
#include <RHI/CommandList.h>
#include <RHI/Device.h>
#include <RHI/ImagePool.h>
#include <RHI/ImageView.h>
#include <RHI/NullDescriptorManager.h>
#include <RHI/PhysicalDevice.h>

namespace AZ::WebGPU
{
    RHI::Ptr<NullDescriptorManager> NullDescriptorManager::Create()
    {
        return aznew NullDescriptorManager();
    }

    RHI::ResultCode NullDescriptorManager::Init(Device& device)
    {
        DeviceObject::Init(device);
        RHI::ResultCode result = CreateImages();
        RETURN_RESULT_IF_UNSUCCESSFUL(result);

        result = CreateBuffers();
        RETURN_RESULT_IF_UNSUCCESSFUL(result);

        return result;
    }

    void NullDescriptorManager::Shutdown()
    {
        m_imageNullDescriptors.fill({});
        m_bufferNull = nullptr;

        m_bufferPool = nullptr;
        m_imagePool = nullptr;

        RHI::DeviceObject::Shutdown();
    }

    RHI::ResultCode NullDescriptorManager::CreateImages()
    {
        Device& device = static_cast<Device&>(GetDevice());

        RHI::ImagePoolDescriptor imagePoolDesc;
        imagePoolDesc.m_bindFlags = RHI::ImageBindFlags::ShaderReadWrite | RHI::ImageBindFlags::CopyWrite;
        m_imagePool = ImagePool::Create();
        RHI::ResultCode result = m_imagePool->Init(device, imagePoolDesc);
        RETURN_RESULT_IF_UNSUCCESSFUL(result);

        const uint32_t imageDimension = 8;
        const RHI::Format depthFormat = device.GetNearestSupportedFormat(RHI::Format::D16_UNORM, RHI::FormatCapabilities::DepthStencil);

        // fill out the different options for the types of image null descriptors
        {
            auto& desc = m_imageNullDescriptors[static_cast<uint32_t>(ImageTypes::General2D)];
            desc.m_name = "NULL_DESCRIPTOR_GENERAL_2D";
            desc.m_descriptor = RHI::ImageDescriptor::Create2D(
                RHI::ImageBindFlags::ShaderRead | RHI::ImageBindFlags::CopyWrite,
                imageDimension,
                imageDimension,
                RHI::Format::R8G8B8A8_UNORM_SRGB);
        }
        {
            auto& desc = m_imageNullDescriptors[static_cast<uint32_t>(ImageTypes::ReadOnly2D)];
            desc.m_name = "NULL_DESCRIPTOR_READONLY_2D";
            desc.m_descriptor = RHI::ImageDescriptor::Create2D(
                RHI::ImageBindFlags::ShaderRead | RHI::ImageBindFlags::CopyWrite,
                imageDimension,
                imageDimension,
                RHI::Format::R8G8B8A8_UNORM_SRGB);
        }
        {
            auto& desc = m_imageNullDescriptors[static_cast<uint32_t>(ImageTypes::Storage2D)];
            desc.m_name = "NULL_DESCRIPTOR_STORAGE_2D";
            desc.m_descriptor = RHI::ImageDescriptor::Create2D(
                RHI::ImageBindFlags::ShaderReadWrite | RHI::ImageBindFlags::CopyWrite,
                256,
                256, RHI::Format::R32G32_FLOAT);
        }
        {
            auto& desc = m_imageNullDescriptors[static_cast<uint32_t>(ImageTypes::Depth2D)];
            desc.m_name = "NULL_DESCRIPTOR_DEPTH_2D";
            desc.m_descriptor = RHI::ImageDescriptor::Create2D(
                RHI::ImageBindFlags::ShaderRead | RHI::ImageBindFlags::CopyWrite,
                imageDimension,
                imageDimension,
                depthFormat);
        }
        {
            auto& desc = m_imageNullDescriptors[static_cast<uint32_t>(ImageTypes::MultiSampleGeneral2D)];
            desc.m_name = "NULL_DESCRIPTOR_MULTISAMPLE_GENERAL_2D";
            desc.m_descriptor = RHI::ImageDescriptor::Create2D(
                RHI::ImageBindFlags::ShaderRead | RHI::ImageBindFlags::CopyWrite | RHI::ImageBindFlags::Color,
                imageDimension,
                imageDimension,
                RHI::Format::R8G8B8A8_UNORM_SRGB);
            desc.m_descriptor.m_multisampleState.m_samples = 4;
        }
        {
            auto& desc = m_imageNullDescriptors[static_cast<uint32_t>(ImageTypes::MultiSampleReadOnly2D)];
            desc = m_imageNullDescriptors[static_cast<uint32_t>(ImageTypes::MultiSampleGeneral2D)];
            desc.m_name = "NULL_DESCRIPTOR_MULTISAMPLE_READONLY_2D";
        }
        {
            auto& desc = m_imageNullDescriptors[static_cast<uint32_t>(ImageTypes::GeneralArray2D)];
            desc.m_name = "NULL_DESCRIPTOR_GENERAL_ARRAY_2D";
            desc.m_descriptor = RHI::ImageDescriptor::Create2DArray(
                RHI::ImageBindFlags::ShaderRead | RHI::ImageBindFlags::CopyWrite,
                imageDimension,
                imageDimension,
                1,
                RHI::Format::R8G8B8A8_UNORM_SRGB);
        }
        {
            auto& desc = m_imageNullDescriptors[static_cast<uint32_t>(ImageTypes::ReadOnlyArray2D)];
            desc = m_imageNullDescriptors[static_cast<uint32_t>(ImageTypes::GeneralArray2D)];
            desc.m_name = "NULL_DESCRIPTOR_READONLY_ARRAY_2D";
        }
        {
            auto& desc = m_imageNullDescriptors[static_cast<uint32_t>(ImageTypes::StorageArray2D)];
            desc.m_name = "NULL_DESCRIPTOR_STORAGE_ARRAY_2D";
            desc.m_descriptor = RHI::ImageDescriptor::Create2DArray(
                RHI::ImageBindFlags::ShaderReadWrite | RHI::ImageBindFlags::CopyWrite,
                256,
                256,
                1,
                RHI::Format::R32G32B32A32_FLOAT);
        }
        {
            auto& desc = m_imageNullDescriptors[static_cast<uint32_t>(ImageTypes::DepthArray2D)];
            desc.m_name = "NULL_DESCRIPTOR_DEPTH_ARRAY_2D";
            desc.m_descriptor = RHI::ImageDescriptor::Create2DArray(
                RHI::ImageBindFlags::ShaderRead | RHI::ImageBindFlags::CopyWrite,
                imageDimension,
                imageDimension,
                1,
                depthFormat);
        }
        {
            auto& desc = m_imageNullDescriptors[static_cast<uint32_t>(ImageTypes::GeneralCube)];
            desc.m_name = "NULL_DESCRIPTOR_GENERAL_CUBE";
            desc.m_descriptor = RHI::ImageDescriptor::CreateCubemap(
                RHI::ImageBindFlags::ShaderRead | RHI::ImageBindFlags::CopyWrite,
                imageDimension,
                RHI::Format::R8G8B8A8_UNORM_SRGB);
        }
        {
            auto& desc = m_imageNullDescriptors[static_cast<uint32_t>(ImageTypes::ReadOnlyCube)];
            desc = m_imageNullDescriptors[static_cast<uint32_t>(ImageTypes::GeneralCube)];
            desc.m_name = "NULL_DESCRIPTOR_READONLY_CUBE";
        }
        {
            auto& desc = m_imageNullDescriptors[static_cast<uint32_t>(ImageTypes::General3D)];
            desc.m_name = "NULL_DESCRIPTOR_GENERAL_3D";
            desc.m_descriptor = RHI::ImageDescriptor::Create3D(
                RHI::ImageBindFlags::ShaderRead | RHI::ImageBindFlags::CopyWrite,
                imageDimension,
                imageDimension,
                1,
                RHI::Format::R8G8B8A8_UNORM_SRGB);
        }
        {
            auto& desc = m_imageNullDescriptors[static_cast<uint32_t>(ImageTypes::DepthCube)];
            desc.m_name = "NULL_DESCRIPTOR_DEPTH_CUBE";
            desc.m_descriptor = RHI::ImageDescriptor::CreateCubemap(
                RHI::ImageBindFlags::ShaderRead | RHI::ImageBindFlags::CopyWrite,
                imageDimension,
                depthFormat);
        }
        {
            auto& desc = m_imageNullDescriptors[static_cast<uint32_t>(ImageTypes::ReadOnly3D)];
            desc = m_imageNullDescriptors[static_cast<uint32_t>(ImageTypes::General3D)];
            desc.m_name = "NULL_DESCRIPTOR_NULL_DESCRIPTOR_READONLY_3DREADONLY_CUBE";
        }

        for (uint32_t i = 0; i < static_cast<uint32_t>(NullDescriptorManager::ImageTypes::Count); ++i)
        {
            NullDescriptorManager::ImageTypes type = static_cast<NullDescriptorManager::ImageTypes>(i);
            auto& desc = m_imageNullDescriptors[i];
            desc.m_image = Image::Create();
            RHI::DeviceImageInitRequest request(*desc.m_image, desc.m_descriptor);
            result = m_imagePool->InitImage(request);
            RETURN_RESULT_IF_UNSUCCESSFUL(result);

            desc.m_imageView = ImageView::Create();
            RHI::ImageViewDescriptor viewDesc;
            viewDesc.m_aspectFlags = RHI::ImageAspectFlags::Color;
            switch (type)
            {
                case NullDescriptorManager::ImageTypes::Depth2D:
                    viewDesc.m_aspectFlags = RHI::ImageAspectFlags::Depth;
                    break;
                case NullDescriptorManager::ImageTypes::DepthArray2D:
                    viewDesc.m_aspectFlags = RHI::ImageAspectFlags::Depth;
                    [[fallthrough]];
                case NullDescriptorManager::ImageTypes::GeneralArray2D:
                case NullDescriptorManager::ImageTypes::ReadOnlyArray2D:
                case NullDescriptorManager::ImageTypes::StorageArray2D:
                    viewDesc.m_isArray = true;
                    break;
                case NullDescriptorManager::ImageTypes::DepthCube:
                    viewDesc.m_aspectFlags = RHI::ImageAspectFlags::Depth;
                    [[fallthrough]];
                case NullDescriptorManager::ImageTypes::GeneralCube:
                case NullDescriptorManager::ImageTypes::ReadOnlyCube:
                    viewDesc.m_isCubemap = true;
                    break;
                default:
                    break;
            }

            result = desc.m_imageView->Init(*desc.m_image, viewDesc);
            RETURN_RESULT_IF_UNSUCCESSFUL(result);
        }        

        return RHI::ResultCode::Success;
    }

    RHI::ResultCode NullDescriptorManager::CreateBuffers()
    {
        Device& device = static_cast<Device&>(GetDevice());

        RHI::BufferPoolDescriptor bufferPoolDesc = {};
        bufferPoolDesc.m_bindFlags = RHI::BufferBindFlags::Constant | RHI::BufferBindFlags::ShaderReadWrite;
        bufferPoolDesc.m_sharedQueueMask = RHI::HardwareQueueClassMask::All;
        bufferPoolDesc.m_heapMemoryLevel = RHI::HeapMemoryLevel::Device;
        bufferPoolDesc.m_hostMemoryAccess = RHI::HostMemoryAccess::Write;

        m_bufferPool = BufferPool::Create();
        RHI::ResultCode result = m_bufferPool->Init(device, bufferPoolDesc);            
        RETURN_RESULT_IF_UNSUCCESSFUL(result);

        const uint32_t bufferSize = 64;
        RHI::BufferDescriptor bufferDescriptor;
        bufferDescriptor.m_bindFlags = bufferPoolDesc.m_bindFlags;
        bufferDescriptor.m_byteCount = bufferSize;
        bufferDescriptor.m_sharedQueueMask = bufferPoolDesc.m_sharedQueueMask;

        m_bufferNull = Buffer::Create();
        RHI::DeviceBufferInitRequest request(*m_bufferNull, bufferDescriptor);
        result = m_bufferPool->InitBuffer(request);
        RETURN_RESULT_IF_UNSUCCESSFUL(result);
           
        return RHI::ResultCode::Success;
    }

    const wgpu::TextureView& NullDescriptorManager::GetDescriptorImageInfo(
        RHI::ShaderInputImageType imageType, bool storageImage, bool usesDepthFormat) const
    {
        if (imageType == RHI::ShaderInputImageType::Image2D)
        {
            if (storageImage)
            {
                return GetImage(ImageTypes::Storage2D);
            }
            else
            {
                return usesDepthFormat ? GetImage(ImageTypes::Depth2D) : GetImage(ImageTypes::ReadOnly2D);
            }
        }
        else if (imageType == RHI::ShaderInputImageType::Image2DArray)
        {
            if (storageImage)
            {
                return GetImage(ImageTypes::StorageArray2D);
            }
            else
            {
                return usesDepthFormat ? GetImage(ImageTypes::DepthArray2D) : GetImage(ImageTypes::ReadOnlyArray2D);
            }
        }
        else if (imageType == RHI::ShaderInputImageType::Image2DMultisample)
        {
            if (storageImage)
            {
                return GetImage(ImageTypes::MultiSampleGeneral2D);
            }
            else
            {
                return GetImage(ImageTypes::MultiSampleReadOnly2D);
            }
        }
        else if (imageType == RHI::ShaderInputImageType::ImageCube || imageType == RHI::ShaderInputImageType::ImageCubeArray)
        {
            if (storageImage)
            {
                return GetImage(ImageTypes::GeneralCube);
            }
            else
            {
                return usesDepthFormat ? GetImage(ImageTypes::DepthCube) : GetImage(ImageTypes::ReadOnlyCube);
            }
        }
        else if (imageType == RHI::ShaderInputImageType::Image3D)
        {
            if (storageImage)
            {
                return GetImage(ImageTypes::General3D);
            }
            else
            {
                return GetImage(ImageTypes::ReadOnly3D);
            }
        }
        else
        {
            AZ_Assert(false, "image null descriptor type %d not handled", imageType);
        }

        return GetImage(ImageTypes::ReadOnly2D);
    }

    const wgpu::TextureView& NullDescriptorManager::GetImage(ImageTypes type) const
    {
        uint32_t index = static_cast<uint32_t>(type);
            
        AZ_Assert(
            index < m_imageNullDescriptors.size(),
            "%d out of bounds for image null descriptor size: %d",
            index,
            m_imageNullDescriptors.size());

        return m_imageNullDescriptors[index].m_imageView->GetNativeView();
    }

    const Buffer& NullDescriptorManager::GetBuffer() const
    {
        return *m_bufferNull;
    }
}
