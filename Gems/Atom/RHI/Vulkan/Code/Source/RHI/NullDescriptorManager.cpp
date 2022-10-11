/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <RHI/Buffer.h>
#include <RHI/BufferPool.h>
#include <RHI/BufferView.h>
#include <Atom/RHI.Reflect/Vulkan/Conversion.h>
#include <RHI/CommandQueueContext.h>
#include <RHI/CommandQueue.h>
#include <RHI/CommandList.h>
#include <RHI/DescriptorPool.h>
#include <RHI/DescriptorSetLayout.h>
#include <RHI/DescriptorSet.h>
#include <RHI/Device.h>
#include <RHI/ImageView.h>
#include <RHI/MemoryView.h>
#include <RHI/ImagePoolResolver.h>
#include <RHI/NullDescriptorManager.h>
#include <RHI/Queue.h>
#include <RHI/PhysicalDevice.h>

namespace AZ
{
    namespace Vulkan
    {
        RHI::Ptr<NullDescriptorManager> NullDescriptorManager::Create()
        {
            return aznew NullDescriptorManager();
        }

        RHI::ResultCode NullDescriptorManager::Init(Device& device)
        {
            DeviceObject::Init(device);
            RHI::ResultCode result = CreateImage();
            RETURN_RESULT_IF_UNSUCCESSFUL(result);

            result = CreateBuffer();
            RETURN_RESULT_IF_UNSUCCESSFUL(result);

            result = CreateTexel();
            RETURN_RESULT_IF_UNSUCCESSFUL(result);

            return result;
        }

        void NullDescriptorManager::Shutdown()
        {
            const Device& device = static_cast<Device&>(GetDevice());
            for (auto& image : m_imageNullDescriptor.m_images)
            {
                device.GetContext().DestroyImage(device.GetNativeDevice(), image.m_image, nullptr);
                device.GetContext().DestroyImageView(device.GetNativeDevice(), image.m_view, nullptr);
                image.m_deviceMemory.reset();
            }

            device.GetContext().DestroyBuffer(device.GetNativeDevice(), m_bufferNullDescriptor.m_buffer, nullptr);
            device.GetContext().DestroyBufferView(device.GetNativeDevice(), m_bufferNullDescriptor.m_view, nullptr);
            m_bufferNullDescriptor.m_memory.reset();

            device.GetContext().DestroyBuffer(device.GetNativeDevice(), m_texelViewNullDescriptor.m_buffer, nullptr);
            device.GetContext().DestroyBufferView(device.GetNativeDevice(), m_texelViewNullDescriptor.m_view, nullptr);
            m_texelViewNullDescriptor.m_memory.reset();

            RHI::DeviceObject::Shutdown();
        }

        RHI::ResultCode NullDescriptorManager::CreateImage()
        {
            Device& device = static_cast<Device&>(GetDevice());

            const uint32_t imageDimension = 8;

            // fill out the different options for the types of image null descriptors
            m_imageNullDescriptor.m_images.resize(static_cast<uint32_t>(ImageTypes::Count));

            m_imageNullDescriptor.m_images[static_cast<uint32_t>(ImageTypes::General2D)] = {};
            m_imageNullDescriptor.m_images[static_cast<uint32_t>(ImageTypes::General2D)].m_name = "NULL_DESCRIPTOR_GENERAL_2D";
            m_imageNullDescriptor.m_images[static_cast<uint32_t>(ImageTypes::General2D)].m_sampleCountFlag = VK_SAMPLE_COUNT_1_BIT;
            m_imageNullDescriptor.m_images[static_cast<uint32_t>(ImageTypes::General2D)].m_format = VK_FORMAT_R8G8B8A8_SRGB;
            m_imageNullDescriptor.m_images[static_cast<uint32_t>(ImageTypes::General2D)].m_usageFlagBits = VkImageUsageFlagBits(VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
            m_imageNullDescriptor.m_images[static_cast<uint32_t>(ImageTypes::General2D)].m_arrayLayers = 1;
            m_imageNullDescriptor.m_images[static_cast<uint32_t>(ImageTypes::General2D)].m_imageCreateFlagBits = VkImageCreateFlagBits(0);
            m_imageNullDescriptor.m_images[static_cast<uint32_t>(ImageTypes::General2D)].m_layout = VK_IMAGE_LAYOUT_GENERAL;
            m_imageNullDescriptor.m_images[static_cast<uint32_t>(ImageTypes::General2D)].m_dimension = imageDimension;

            m_imageNullDescriptor.m_images[static_cast<uint32_t>(ImageTypes::ReadOnly2D)] = m_imageNullDescriptor.m_images[static_cast<uint32_t>(NullDescriptorManager::ImageTypes::General2D)];
            m_imageNullDescriptor.m_images[static_cast<uint32_t>(ImageTypes::ReadOnly2D)].m_name = "NULL_DESCRIPTOR_READONLY_2D";
            m_imageNullDescriptor.m_images[static_cast<uint32_t>(ImageTypes::ReadOnly2D)].m_sampleCountFlag = VK_SAMPLE_COUNT_1_BIT;
            m_imageNullDescriptor.m_images[static_cast<uint32_t>(ImageTypes::ReadOnly2D)].m_format = VK_FORMAT_R8G8B8A8_SRGB;
            m_imageNullDescriptor.m_images[static_cast<uint32_t>(ImageTypes::ReadOnly2D)].m_usageFlagBits = VkImageUsageFlagBits(VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
            m_imageNullDescriptor.m_images[static_cast<uint32_t>(ImageTypes::ReadOnly2D)].m_arrayLayers = 1;
            m_imageNullDescriptor.m_images[static_cast<uint32_t>(ImageTypes::ReadOnly2D)].m_imageCreateFlagBits = VkImageCreateFlagBits(0);
            m_imageNullDescriptor.m_images[static_cast<uint32_t>(ImageTypes::ReadOnly2D)].m_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            m_imageNullDescriptor.m_images[static_cast<uint32_t>(ImageTypes::ReadOnly2D)].m_dimension = imageDimension;
            
            m_imageNullDescriptor.m_images[static_cast<uint32_t>(ImageTypes::Storage2D)] = m_imageNullDescriptor.m_images[static_cast<uint32_t>(NullDescriptorManager::ImageTypes::General2D)];
            m_imageNullDescriptor.m_images[static_cast<uint32_t>(ImageTypes::Storage2D)].m_name = "NULL_DESCRIPTOR_STORAGE_2D";
            m_imageNullDescriptor.m_images[static_cast<uint32_t>(ImageTypes::Storage2D)].m_sampleCountFlag = VK_SAMPLE_COUNT_1_BIT;
            m_imageNullDescriptor.m_images[static_cast<uint32_t>(ImageTypes::Storage2D)].m_format = VK_FORMAT_R32G32B32A32_UINT;
            m_imageNullDescriptor.m_images[static_cast<uint32_t>(ImageTypes::Storage2D)].m_usageFlagBits = VkImageUsageFlagBits(VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT);
            m_imageNullDescriptor.m_images[static_cast<uint32_t>(ImageTypes::Storage2D)].m_arrayLayers = 1;
            m_imageNullDescriptor.m_images[static_cast<uint32_t>(ImageTypes::Storage2D)].m_layout = VK_IMAGE_LAYOUT_GENERAL;
            m_imageNullDescriptor.m_images[static_cast<uint32_t>(ImageTypes::Storage2D)].m_dimension = 256;

            m_imageNullDescriptor.m_images[static_cast<uint32_t>(ImageTypes::MultiSampleGeneral2D)] = m_imageNullDescriptor.m_images[static_cast<uint32_t>(NullDescriptorManager::ImageTypes::General2D)];
            m_imageNullDescriptor.m_images[static_cast<uint32_t>(ImageTypes::MultiSampleGeneral2D)].m_name = "NULL_DESCRIPTOR_MULTISAMPLE_GENERAL_2D";
            m_imageNullDescriptor.m_images[static_cast<uint32_t>(ImageTypes::MultiSampleGeneral2D)].m_sampleCountFlag = VK_SAMPLE_COUNT_4_BIT;
            m_imageNullDescriptor.m_images[static_cast<uint32_t>(ImageTypes::MultiSampleGeneral2D)].m_layout = VK_IMAGE_LAYOUT_GENERAL;

            m_imageNullDescriptor.m_images[static_cast<uint32_t>(ImageTypes::MultiSampleReadOnly2D)] = m_imageNullDescriptor.m_images[static_cast<uint32_t>(NullDescriptorManager::ImageTypes::General2D)];
            m_imageNullDescriptor.m_images[static_cast<uint32_t>(ImageTypes::MultiSampleReadOnly2D)].m_name = "NULL_DESCRIPTOR_MULTISAMPLE_READONLY_2D";
            m_imageNullDescriptor.m_images[static_cast<uint32_t>(ImageTypes::MultiSampleReadOnly2D)].m_sampleCountFlag = VK_SAMPLE_COUNT_4_BIT;
            m_imageNullDescriptor.m_images[static_cast<uint32_t>(ImageTypes::MultiSampleReadOnly2D)].m_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

            m_imageNullDescriptor.m_images[static_cast<uint32_t>(ImageTypes::GeneralArray2D)] = {};
            m_imageNullDescriptor.m_images[static_cast<uint32_t>(ImageTypes::GeneralArray2D)].m_name = "NULL_DESCRIPTOR_GENERAL_ARRAY_2D";
            m_imageNullDescriptor.m_images[static_cast<uint32_t>(ImageTypes::GeneralArray2D)].m_sampleCountFlag = VK_SAMPLE_COUNT_1_BIT;
            m_imageNullDescriptor.m_images[static_cast<uint32_t>(ImageTypes::GeneralArray2D)].m_format = VK_FORMAT_R8G8B8A8_SRGB;
            m_imageNullDescriptor.m_images[static_cast<uint32_t>(ImageTypes::GeneralArray2D)].m_usageFlagBits =VkImageUsageFlagBits(VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
            m_imageNullDescriptor.m_images[static_cast<uint32_t>(ImageTypes::GeneralArray2D)].m_arrayLayers = 1;
            m_imageNullDescriptor.m_images[static_cast<uint32_t>(ImageTypes::GeneralArray2D)].m_imageCreateFlagBits = VkImageCreateFlagBits(0);
            m_imageNullDescriptor.m_images[static_cast<uint32_t>(ImageTypes::GeneralArray2D)].m_layout = VK_IMAGE_LAYOUT_GENERAL;
            m_imageNullDescriptor.m_images[static_cast<uint32_t>(ImageTypes::GeneralArray2D)].m_dimension = imageDimension;

            m_imageNullDescriptor.m_images[static_cast<uint32_t>(ImageTypes::ReadOnlyArray2D)] = m_imageNullDescriptor.m_images[static_cast<uint32_t>(NullDescriptorManager::ImageTypes::GeneralArray2D)];
            m_imageNullDescriptor.m_images[static_cast<uint32_t>(ImageTypes::ReadOnlyArray2D)].m_name = "NULL_DESCRIPTOR_READONLY_ARRAY_2D";
            m_imageNullDescriptor.m_images[static_cast<uint32_t>(ImageTypes::ReadOnlyArray2D)].m_sampleCountFlag = VK_SAMPLE_COUNT_1_BIT;
            m_imageNullDescriptor.m_images[static_cast<uint32_t>(ImageTypes::ReadOnlyArray2D)].m_format = VK_FORMAT_R8G8B8A8_SRGB;
            m_imageNullDescriptor.m_images[static_cast<uint32_t>(ImageTypes::ReadOnlyArray2D)].m_usageFlagBits = VkImageUsageFlagBits(VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
            m_imageNullDescriptor.m_images[static_cast<uint32_t>(ImageTypes::ReadOnlyArray2D)].m_arrayLayers = 1;
            m_imageNullDescriptor.m_images[static_cast<uint32_t>(ImageTypes::ReadOnlyArray2D)].m_imageCreateFlagBits = VkImageCreateFlagBits(0);
            m_imageNullDescriptor.m_images[static_cast<uint32_t>(ImageTypes::ReadOnlyArray2D)].m_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            m_imageNullDescriptor.m_images[static_cast<uint32_t>(ImageTypes::ReadOnlyArray2D)].m_dimension = imageDimension;

            m_imageNullDescriptor.m_images[static_cast<uint32_t>(ImageTypes::StorageArray2D)] = m_imageNullDescriptor.m_images[static_cast<uint32_t>(NullDescriptorManager::ImageTypes::General2D)];
            m_imageNullDescriptor.m_images[static_cast<uint32_t>(ImageTypes::StorageArray2D)].m_name = "NULL_DESCRIPTOR_STORAGE_ARRAY_2D";
            m_imageNullDescriptor.m_images[static_cast<uint32_t>(ImageTypes::StorageArray2D)].m_sampleCountFlag = VK_SAMPLE_COUNT_1_BIT;
            m_imageNullDescriptor.m_images[static_cast<uint32_t>(ImageTypes::StorageArray2D)].m_format = VK_FORMAT_R32G32B32A32_UINT;
            m_imageNullDescriptor.m_images[static_cast<uint32_t>(ImageTypes::StorageArray2D)].m_usageFlagBits = VkImageUsageFlagBits(VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT);
            m_imageNullDescriptor.m_images[static_cast<uint32_t>(ImageTypes::StorageArray2D)].m_arrayLayers = 1;
            m_imageNullDescriptor.m_images[static_cast<uint32_t>(ImageTypes::StorageArray2D)].m_layout = VK_IMAGE_LAYOUT_GENERAL;
            m_imageNullDescriptor.m_images[static_cast<uint32_t>(ImageTypes::StorageArray2D)].m_dimension = 256;

            m_imageNullDescriptor.m_images[static_cast<uint32_t>(ImageTypes::GeneralCube)] = m_imageNullDescriptor.m_images[static_cast<uint32_t>(NullDescriptorManager::ImageTypes::General2D)];
            m_imageNullDescriptor.m_images[static_cast<uint32_t>(ImageTypes::GeneralCube)].m_name = "NULL_DESCRIPTOR_GENERAL_CUBE";
            m_imageNullDescriptor.m_images[static_cast<uint32_t>(ImageTypes::GeneralCube)].m_arrayLayers = 6;
            m_imageNullDescriptor.m_images[static_cast<uint32_t>(ImageTypes::GeneralCube)].m_imageCreateFlagBits = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
            m_imageNullDescriptor.m_images[static_cast<uint32_t>(ImageTypes::GeneralCube)].m_layout = VK_IMAGE_LAYOUT_GENERAL;

            m_imageNullDescriptor.m_images[static_cast<uint32_t>(ImageTypes::ReadOnlyCube)] = m_imageNullDescriptor.m_images[static_cast<uint32_t>(NullDescriptorManager::ImageTypes::GeneralCube)];
            m_imageNullDescriptor.m_images[static_cast<uint32_t>(ImageTypes::ReadOnlyCube)].m_name = "NULL_DESCRIPTOR_READONLY_CUBE";
            m_imageNullDescriptor.m_images[static_cast<uint32_t>(ImageTypes::ReadOnlyCube)].m_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

            m_imageNullDescriptor.m_images[static_cast<uint32_t>(ImageTypes::General3D)] = {};
            m_imageNullDescriptor.m_images[static_cast<uint32_t>(ImageTypes::General3D)].m_name = "NULL_DESCRIPTOR_GENERAL_3D";
            m_imageNullDescriptor.m_images[static_cast<uint32_t>(ImageTypes::General3D)].m_sampleCountFlag = VK_SAMPLE_COUNT_1_BIT;
            m_imageNullDescriptor.m_images[static_cast<uint32_t>(ImageTypes::General3D)].m_format = VK_FORMAT_R8G8B8A8_SRGB;
            m_imageNullDescriptor.m_images[static_cast<uint32_t>(ImageTypes::General3D)].m_usageFlagBits = VkImageUsageFlagBits(VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
            m_imageNullDescriptor.m_images[static_cast<uint32_t>(ImageTypes::General3D)].m_arrayLayers = 1;
            m_imageNullDescriptor.m_images[static_cast<uint32_t>(ImageTypes::General3D)].m_imageCreateFlagBits = VkImageCreateFlagBits(0);
            m_imageNullDescriptor.m_images[static_cast<uint32_t>(ImageTypes::General3D)].m_layout = VK_IMAGE_LAYOUT_GENERAL;
            m_imageNullDescriptor.m_images[static_cast<uint32_t>(ImageTypes::General3D)].m_dimension = imageDimension;

            m_imageNullDescriptor.m_images[static_cast<uint32_t>(ImageTypes::ReadOnly3D)] = m_imageNullDescriptor.m_images[static_cast<uint32_t>(NullDescriptorManager::ImageTypes::General3D)];
            m_imageNullDescriptor.m_images[static_cast<uint32_t>(ImageTypes::ReadOnly3D)].m_name = "NULL_DESCRIPTOR_READONLY_3D";
            m_imageNullDescriptor.m_images[static_cast<uint32_t>(ImageTypes::ReadOnly3D)].m_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            
            AZStd::vector<uint32_t> queueFamilies(device.GetCommandQueueContext().GetQueueFamilyIndices(RHI::HardwareQueueClassMask::All));
            
            // Default create info
            VkImageCreateInfo imageCreateInfo = {};
            imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
            imageCreateInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
            imageCreateInfo.mipLevels = 1;
            imageCreateInfo.arrayLayers = 1;
            imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
            imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
            imageCreateInfo.sharingMode = queueFamilies.size() > 1 ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE;
            imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            imageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
            imageCreateInfo.queueFamilyIndexCount = aznumeric_caster(queueFamilies.size());
            imageCreateInfo.pQueueFamilyIndices = queueFamilies.data();

            // Default layout transition
            VkImageMemoryBarrier barrier = {};
            barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            barrier.subresourceRange.baseMipLevel = 0;
            barrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
            barrier.subresourceRange.baseArrayLayer = 0;
            barrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;

            AZStd::vector<VkImageMemoryBarrier> layoutTransitions;

            for (uint32_t imageIndex = static_cast<uint32_t>(NullDescriptorManager::ImageTypes::General2D); imageIndex < static_cast<uint32_t>(NullDescriptorManager::ImageTypes::Count); imageIndex++)
            {
                // different options for the images
                imageCreateInfo.imageType = (imageIndex >= static_cast<uint32_t>(NullDescriptorManager::ImageTypes::General3D)) ? VK_IMAGE_TYPE_3D : VK_IMAGE_TYPE_2D;
                imageCreateInfo.extent = { m_imageNullDescriptor.m_images[imageIndex].m_dimension, m_imageNullDescriptor.m_images[imageIndex].m_dimension, 1 };
                imageCreateInfo.samples = m_imageNullDescriptor.m_images[imageIndex].m_sampleCountFlag;
                imageCreateInfo.format = m_imageNullDescriptor.m_images[imageIndex].m_format;
                imageCreateInfo.usage = m_imageNullDescriptor.m_images[imageIndex].m_usageFlagBits;
                imageCreateInfo.arrayLayers = m_imageNullDescriptor.m_images[imageIndex].m_arrayLayers;
                imageCreateInfo.flags = m_imageNullDescriptor.m_images[imageIndex].m_imageCreateFlagBits;

                VkResult result = device.GetContext().CreateImage(
                    device.GetNativeDevice(), &imageCreateInfo, nullptr, &m_imageNullDescriptor.m_images[imageIndex].m_image);
                RETURN_RESULT_IF_UNSUCCESSFUL(ConvertResult(result));

                VkMemoryRequirements memReqs = {};
                device.GetContext().GetImageMemoryRequirements(
                    device.GetNativeDevice(), m_imageNullDescriptor.m_images[imageIndex].m_image, &memReqs);

                // image device memory
                m_imageNullDescriptor.m_images[imageIndex].m_deviceMemory = device.AllocateMemory(memReqs.size, memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
                result = device.GetContext().BindImageMemory(
                    device.GetNativeDevice(), m_imageNullDescriptor.m_images[imageIndex].m_image,
                    m_imageNullDescriptor.m_images[imageIndex].m_deviceMemory->GetNativeDeviceMemory(), 0);
                RETURN_RESULT_IF_UNSUCCESSFUL(ConvertResult(result));

                // Transition to the proper layout. Images can only be created with VK_IMAGE_LAYOUT_UNDEFINED or VK_IMAGE_LAYOUT_PREINITIALIZED.
                barrier.newLayout = m_imageNullDescriptor.m_images[imageIndex].m_layout;
                barrier.image = m_imageNullDescriptor.m_images[imageIndex].m_image;
                layoutTransitions.push_back(barrier);

                // sampler
                VkSamplerCreateInfo samplerCreateInfo = {};
                samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
                samplerCreateInfo.maxAnisotropy = 1.0f;
                samplerCreateInfo.magFilter = VK_FILTER_LINEAR;
                samplerCreateInfo.minFilter = VK_FILTER_LINEAR;
                samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
                samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
                samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
                samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
                samplerCreateInfo.mipLodBias = 0.0f;
                samplerCreateInfo.compareOp = VK_COMPARE_OP_NEVER;
                samplerCreateInfo.minLod = 0.0f;
                samplerCreateInfo.maxLod = 0.0f;
                samplerCreateInfo.maxAnisotropy = 1.0;
                samplerCreateInfo.anisotropyEnable = VK_FALSE;
                samplerCreateInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
                result = device.GetContext().CreateSampler(
                    device.GetNativeDevice(), &samplerCreateInfo, nullptr, &m_imageNullDescriptor.m_images[imageIndex].m_sampler);
                RETURN_RESULT_IF_UNSUCCESSFUL(ConvertResult(result));

                // image view
                VkImageViewCreateInfo imageViewCreateInfo = {};
                imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
                imageViewCreateInfo.format = m_imageNullDescriptor.m_images[imageIndex].m_format;
                imageViewCreateInfo.components = { VK_COMPONENT_SWIZZLE_ZERO, VK_COMPONENT_SWIZZLE_ZERO, VK_COMPONENT_SWIZZLE_ZERO, VK_COMPONENT_SWIZZLE_ONE };
                imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
                imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
                imageViewCreateInfo.subresourceRange.layerCount = 1;
                imageViewCreateInfo.subresourceRange.levelCount = 1;
                imageViewCreateInfo.image = m_imageNullDescriptor.m_images[imageIndex].m_image;

                // 2d, 3d, or cube view
                imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
                if (imageCreateInfo.arrayLayers > 1)
                {
                    imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
                    imageViewCreateInfo.subresourceRange.layerCount = m_imageNullDescriptor.m_images[imageIndex].m_arrayLayers;
                    imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
                }
                else if (imageIndex == static_cast<uint32_t>(ImageTypes::General3D) || imageIndex == static_cast<uint32_t>(ImageTypes::ReadOnly3D))
                {
                    imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_3D;
                }
                else if (imageIndex >= static_cast<uint32_t>(ImageTypes::GeneralArray2D) && imageIndex <= static_cast<uint32_t>(ImageTypes::StorageArray2D))
                {
                    imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
                }

                result = device.GetContext().CreateImageView(
                    device.GetNativeDevice(), &imageViewCreateInfo, nullptr, &m_imageNullDescriptor.m_images[imageIndex].m_view);
                RETURN_RESULT_IF_UNSUCCESSFUL(ConvertResult(result));

                m_imageNullDescriptor.m_images[imageIndex].m_descriptorImageInfo.imageLayout = m_imageNullDescriptor.m_images[imageIndex].m_layout;
                m_imageNullDescriptor.m_images[imageIndex].m_descriptorImageInfo.imageView = m_imageNullDescriptor.m_images[imageIndex].m_view;
                m_imageNullDescriptor.m_images[imageIndex].m_descriptorImageInfo.sampler = m_imageNullDescriptor.m_images[imageIndex].m_sampler;
            }

            auto commandList = device.AcquireCommandList(RHI::HardwareQueueClass::Graphics);
            commandList->BeginCommandBuffer();
            device.GetContext().CmdPipelineBarrier(
                commandList->GetNativeCommandBuffer(),
                VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                VK_DEPENDENCY_BY_REGION_BIT,
                0,
                nullptr,
                0,
                nullptr,
                aznumeric_caster(layoutTransitions.size()),
                layoutTransitions.data());
            commandList->EndCommandBuffer();

            device.GetCommandQueueContext().GetCommandQueue(RHI::HardwareQueueClass::Graphics).QueueCommand([commandList](void* queue)
            {
                Queue* vulkanQueue = static_cast<Queue*>(queue);
                vulkanQueue->SubmitCommandBuffers(
                    { commandList },
                    AZStd::vector<Semaphore::WaitSemaphore>(),
                    AZStd::vector<RHI::Ptr<Semaphore>>(),
                    nullptr);
            });

            return RHI::ResultCode::Success;
        }

        RHI::ResultCode NullDescriptorManager::CreateBuffer()
        {
            Device& device = static_cast<Device&>(GetDevice());

            const uint32_t bufferSize = 64;
            AZStd::vector<uint32_t> queueFamilies(device.GetCommandQueueContext().GetQueueFamilyIndices(RHI::HardwareQueueClassMask::All));

            VkBufferCreateInfo bufferCreateInfo = {};
            bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            bufferCreateInfo.size = bufferSize;
            bufferCreateInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT;
            bufferCreateInfo.sharingMode = queueFamilies.size() > 1 ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE;
            bufferCreateInfo.queueFamilyIndexCount = aznumeric_caster(queueFamilies.size());
            bufferCreateInfo.pQueueFamilyIndices = queueFamilies.data();
            VkResult result =
                device.GetContext().CreateBuffer(device.GetNativeDevice(), &bufferCreateInfo, nullptr, &m_bufferNullDescriptor.m_buffer);
            RETURN_RESULT_IF_UNSUCCESSFUL(ConvertResult(result));

            VkMemoryRequirements memReqs;
            device.GetContext().GetBufferMemoryRequirements(device.GetNativeDevice(), m_bufferNullDescriptor.m_buffer, &memReqs);

            // device memory
            m_bufferNullDescriptor.m_memory = device.AllocateMemory(memReqs.size, memReqs.memoryTypeBits, 0);
            result = device.GetContext().BindBufferMemory(
                device.GetNativeDevice(), m_bufferNullDescriptor.m_buffer, m_bufferNullDescriptor.m_memory->GetNativeDeviceMemory(), 0);
            RETURN_RESULT_IF_UNSUCCESSFUL(ConvertResult(result));

            m_bufferNullDescriptor.m_bufferSize = bufferSize;
            VkBufferViewCreateInfo bufferViewCreateInfo = {};
            bufferViewCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO;
            bufferViewCreateInfo.buffer = m_bufferNullDescriptor.m_buffer;
            bufferViewCreateInfo.flags = 0;
            bufferViewCreateInfo.format = VK_FORMAT_R8_UINT;
            bufferViewCreateInfo.offset = 0;
            bufferViewCreateInfo.pNext = nullptr;
            bufferViewCreateInfo.range = m_bufferNullDescriptor.m_bufferSize;
            result = device.GetContext().CreateBufferView(
                device.GetNativeDevice(), &bufferViewCreateInfo, nullptr, &m_bufferNullDescriptor.m_view);
            RETURN_RESULT_IF_UNSUCCESSFUL(ConvertResult(result));

            m_bufferNullDescriptor.m_bufferInfo.buffer = m_bufferNullDescriptor.m_buffer;
            m_bufferNullDescriptor.m_bufferInfo.offset = 0;
            m_bufferNullDescriptor.m_bufferInfo.range = m_bufferNullDescriptor.m_bufferSize;

            return RHI::ResultCode::Success;
        }

        RHI::ResultCode NullDescriptorManager::CreateTexel()
        {
            Device& device = static_cast<Device&>(GetDevice());

            const uint32_t bufferSize = 64;
            AZStd::vector<uint32_t> queueFamilies(device.GetCommandQueueContext().GetQueueFamilyIndices(RHI::HardwareQueueClassMask::All));

            VkBufferCreateInfo bufferCreateInfo = {};
            bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            bufferCreateInfo.size = bufferSize;
            bufferCreateInfo.usage = VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT;
            bufferCreateInfo.sharingMode = queueFamilies.size() > 1 ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE;
            bufferCreateInfo.queueFamilyIndexCount = aznumeric_caster(queueFamilies.size());
            bufferCreateInfo.pQueueFamilyIndices = queueFamilies.data();
            VkResult result =
                device.GetContext().CreateBuffer(device.GetNativeDevice(), &bufferCreateInfo, nullptr, &m_texelViewNullDescriptor.m_buffer);
            RETURN_RESULT_IF_UNSUCCESSFUL(ConvertResult(result));

            // device memory
            VkMemoryRequirements memReqs;
            device.GetContext().GetBufferMemoryRequirements(device.GetNativeDevice(), m_texelViewNullDescriptor.m_buffer, &memReqs);
            m_texelViewNullDescriptor.m_memory = device.AllocateMemory(memReqs.size, memReqs.memoryTypeBits, 0);

            result = device.GetContext().BindBufferMemory(
                device.GetNativeDevice(),
                m_texelViewNullDescriptor.m_buffer,
                m_texelViewNullDescriptor.m_memory->GetNativeDeviceMemory(),
                0);
            RETURN_RESULT_IF_UNSUCCESSFUL(ConvertResult(result));

            m_texelViewNullDescriptor.m_bufferSize = 64;
            VkBufferViewCreateInfo bufferViewCreateInfo = {};
            bufferViewCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO;
            bufferViewCreateInfo.buffer = m_texelViewNullDescriptor.m_buffer;
            bufferViewCreateInfo.flags = 0;
            bufferViewCreateInfo.format = VK_FORMAT_R8_UINT;
            bufferViewCreateInfo.offset = 0;
            bufferViewCreateInfo.pNext = nullptr;
            bufferViewCreateInfo.range = m_texelViewNullDescriptor.m_bufferSize;
            result = device.GetContext().CreateBufferView(
                device.GetNativeDevice(), &bufferViewCreateInfo, nullptr, &m_texelViewNullDescriptor.m_view);
            RETURN_RESULT_IF_UNSUCCESSFUL(ConvertResult(result));

            return RHI::ResultCode::Success;
        }

        VkDescriptorImageInfo NullDescriptorManager::GetDescriptorImageInfo(RHI::ShaderInputImageType imageType, bool storageImage)
        {
            if (imageType == RHI::ShaderInputImageType::Image2D)
            {
                if (storageImage)
                {
                    return GetImage(ImageTypes::Storage2D);
                }
                else
                {
                    return GetImage(ImageTypes::ReadOnly2D);
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
                    return GetImage(ImageTypes::ReadOnlyArray2D);
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
                    return GetImage(ImageTypes::ReadOnlyCube);
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

        VkDescriptorImageInfo NullDescriptorManager::GetImage(ImageTypes type)
        {
            uint32_t index = static_cast<uint32_t>(type);
            
            AZ_Assert(index < m_imageNullDescriptor.m_images.size(), "%d out of bounds for image null descriptor size: %d", index, m_imageNullDescriptor.m_images.size());
            return m_imageNullDescriptor.m_images[index].m_descriptorImageInfo;
        }

        VkBufferView NullDescriptorManager::GetTexelBufferView()
        {
            return m_texelViewNullDescriptor.m_view;
        }

        VkDescriptorBufferInfo NullDescriptorManager::GetBuffer()
        {
            return m_bufferNullDescriptor.m_bufferInfo;
        }
    }
}
