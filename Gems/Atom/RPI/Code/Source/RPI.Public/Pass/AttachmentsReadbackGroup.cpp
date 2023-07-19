/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Public/Buffer/BufferSystemInterface.h>
#include <Atom/RPI.Public/Buffer/Buffer.h>
#include <Atom/RPI.Public/Image/AttachmentImage.h>
#include <Atom/RPI.Public/Image/ImageSystemInterface.h>
#include <Atom/RPI.Public/Pass/AttachmentsReadbackGroup.h>
#include <Atom/RPI.Public/RPIUtils.h>

#include <Atom/RPI.Reflect/Buffer/BufferAssetCreator.h>

#include <Atom/RHI/CommandList.h>
#include <Atom/RHI/Factory.h>
#include <Atom/RHI/Fence.h>
#include <Atom/RHI/FrameGraphExecuteContext.h>
#include <Atom/RHI/FrameScheduler.h>
#include <Atom/RHI/RHISystemInterface.h>
#include <Atom/RHI/RHIUtils.h>
#include <Atom/RHI/ScopeProducerFunction.h>

#include <AzCore/Serialization/Json/JsonUtils.h>

#include <AzCore/std/smart_ptr/make_shared.h>

namespace AZ
{
    namespace RPI
    {
        AttachmentsReadbackGroup::AttachmentsReadbackGroup(const RHI::ScopeId& scopeId) :
            AttachmentReadback(scopeId)
        {            
        }

        AttachmentsReadbackGroup::~AttachmentsReadbackGroup()
        {
            Reset();
        }

        bool AttachmentsReadbackGroup::ReadPassAttachments(const AZStd::vector<ReadbackRequestInfo>& readbackAttachmentRequests)
        {
            if (AZ::RHI::IsNullRHI())
            {
                return false;
            }

            if (!IsReady())
            {
                AZ_Assert(false, "AttachmentsReadbackGroup is not ready to readback attachments.");
                return false;
            }

            Reset();

            for (const auto &requestInfo : readbackAttachmentRequests)
            {
                const auto attachment = requestInfo.m_attachment;
                if (!attachment || (attachment->GetAttachmentType() != RHI::AttachmentType::Buffer && attachment->GetAttachmentType() != RHI::AttachmentType::Image))
                {
                    AZ_Assert(false, "ReadPassAttachment: attachment is not a buffer or an image");
                    return false;
                }

                m_state = ReadbackState::AttachmentSet;

                m_attachmentReadbackItems.push_back({});
                auto& readbackItem = m_attachmentReadbackItems.back();
                for(uint32_t i = 0; i < RHI::Limits::Device::FrameCountMax; i++)
                {
                    readbackItem.m_readbackBufferArray.push_back(nullptr);
                }

                readbackItem.m_attachmentId = attachment->GetAttachmentId();
                readbackItem.m_attachmentType = attachment->GetAttachmentType();

                readbackItem.m_readbackName = requestInfo.m_readbackName;
                if (readbackItem.m_readbackName.IsEmpty())
                {
                    readbackItem.m_readbackName = AZStd::string::format("%s_RB", readbackItem.m_attachmentId.GetCStr());
                }

                readbackItem.m_copyAttachmentId = readbackItem.m_attachmentId;
                readbackItem.m_imageViewDescriptor = requestInfo.m_imageViewDescriptor;

                // Get some attachment information
                if (readbackItem.m_attachmentType == RHI::AttachmentType::Buffer)
                {
                    if (attachment->m_importedResource)
                    {
                        Buffer* buffer = static_cast<Buffer*>(attachment->m_importedResource.get());
                        readbackItem.m_bufferAttachmentByteSize = buffer->GetBufferSize();
                    }
                    else
                    {
                        readbackItem.m_bufferAttachmentByteSize = attachment->m_descriptor.m_buffer.m_byteCount;
                    }
                }
                else
                {
                    if (attachment->m_importedResource)
                    {
                        AttachmentImage* attImage = static_cast<AttachmentImage*>(attachment->m_importedResource.get());
                        readbackItem.m_imageDescriptor = attImage->GetRHIImage()->GetDescriptor();
                    }
                    else
                    {
                        readbackItem.m_imageDescriptor = attachment->m_descriptor.m_image;
                    }

                    AZ_Assert(readbackItem.m_imageDescriptor.m_multisampleState.m_samples == 1, "Read back of multiple attachments is only supported for Non-MSAA descriptors!");
                }
            }

            return true;
        }

        void AttachmentsReadbackGroup::CopyPrepare(RHI::FrameGraphInterface frameGraph)
        {
            for (const auto& readbackItem : m_attachmentReadbackItems)
            {
                if (readbackItem.m_attachmentType == RHI::AttachmentType::Buffer)
                {
                    RHI::BufferScopeAttachmentDescriptor descriptor{ readbackItem.m_copyAttachmentId };
                    descriptor.m_bufferViewDescriptor = RHI::BufferViewDescriptor::CreateRaw(0, aznumeric_cast<uint32_t>(readbackItem.m_bufferAttachmentByteSize));
                    frameGraph.UseCopyAttachment(descriptor, RHI::ScopeAttachmentAccess::Read);
                }
                else if (readbackItem.m_attachmentType == RHI::AttachmentType::Image)
                {
                    RHI::ImageScopeAttachmentDescriptor descriptor{ readbackItem.m_copyAttachmentId };
                    frameGraph.UseCopyAttachment(descriptor, RHI::ScopeAttachmentAccess::Read);
                }
            }

            frameGraph.SetEstimatedItemCount(static_cast<uint32_t>(m_attachmentReadbackItems.size()));

            frameGraph.SignalFence(*m_fence);

            // CPU has already consumed the GPU buffer. We can clear it now.
            // We don't do this in the Async callback as the callback can get signaled by the GPU at anytime.
            // We were seeing an issue where during the buffer cleanup there was a chance to hit the assert
            // related to disconnecting a bus during a dispatch on a lockless Bus.
            // The fix is to clear the buffer outside of the callback.
            for (int32_t i = 0; i < RHI::Limits::Device::FrameCountMax; i++)
            {
                if (m_isReadbackComplete[i])
                {
                    m_isReadbackComplete[i] = false;
                    for (auto& readbackItem : m_attachmentReadbackItems)
                    {
                        readbackItem.m_readbackBufferArray[i] = nullptr;
                    }
                }
            }

            // Loop the triple buffer index and cache the current index to the callback.
            m_readbackBufferCurrentIndex = (m_readbackBufferCurrentIndex + 1) % RHI::Limits::Device::FrameCountMax;
            
            uint32_t readbackBufferCurrentIndex = m_readbackBufferCurrentIndex;
            m_fence->WaitOnCpuAsync([this, readbackBufferCurrentIndex]()
                {
                    if (m_state == ReadbackState::Reading)
                    {
                        if (CopyBufferData(readbackBufferCurrentIndex))
                        {
                            m_state = ReadbackState::Success;
                        }
                        else
                        {
                            m_state = ReadbackState::Failed;
                        }
                    }
                    if (m_callback)
                    {
                        for (const auto& attachmentReadbackItem : m_attachmentReadbackItems)
                        {
                            m_callback(GetReadbackResult(attachmentReadbackItem));
                        }
                    }

                    Reset();
                }
            );
        }

        void AttachmentsReadbackGroup::CopyCompile(const RHI::FrameGraphCompileContext& context)
        {
            for (auto& readbackItem : m_attachmentReadbackItems)
            {
                if (readbackItem.m_attachmentType == RHI::AttachmentType::Buffer)
                {
                    const AZ::RHI::Buffer* buffer = context.GetBuffer(readbackItem.m_copyAttachmentId);

                    RPI::CommonBufferDescriptor desc;
                    desc.m_poolType = RPI::CommonBufferPoolType::ReadBack;
                    desc.m_bufferName = readbackItem.m_readbackName.GetStringView();
                    desc.m_byteCount = buffer->GetDescriptor().m_byteCount;

                    readbackItem.m_readbackBufferArray[m_readbackBufferCurrentIndex] = BufferSystemInterface::Get()->CreateBufferFromCommonPool(desc);

                    // copy buffer
                    RHI::CopyBufferDescriptor copyBuffer;
                    copyBuffer.m_sourceBuffer = buffer;
                    copyBuffer.m_destinationBuffer = readbackItem.m_readbackBufferArray[m_readbackBufferCurrentIndex]->GetRHIBuffer();
                    copyBuffer.m_size = aznumeric_cast<uint32_t>(desc.m_byteCount);

                    readbackItem.m_copyItem = copyBuffer;
                }
                else if (readbackItem.m_attachmentType == RHI::AttachmentType::Image)
                {
                    // copy image to read back buffer since only buffer can be accessed by host
                    const AZ::RHI::Image* image = context.GetImage(readbackItem.m_copyAttachmentId);
                    if (!image)
                    {
                        AZ_Warning("AttachmentsReadbackGroup", false, "Failed to find attachment image %s for copy to buffer", readbackItem.m_copyAttachmentId.GetCStr());
                        return;
                    }
                    readbackItem.m_imageDescriptor = image->GetDescriptor();

                    AZ_Assert(readbackItem.m_imageViewDescriptor.m_mipSliceMin == readbackItem.m_imageViewDescriptor.m_mipSliceMax, "Mip selection mismatch!");

                    const uint16_t mipSlice = readbackItem.m_imageViewDescriptor.m_mipSliceMin;
                    RHI::ImageSubresourceRange range(mipSlice, mipSlice, 0, 0);
                    range.m_aspectFlags = RHI::ImageAspectFlags::Color;

                    // setup aspect 
                    RHI::ImageAspect imageAspect = RHI::ImageAspect::Color;
                    RHI::ImageAspectFlags imageAspectFlags = RHI::GetImageAspectFlags(readbackItem.m_imageViewDescriptor.m_overrideFormat);
                    if (RHI::CheckBitsAll(imageAspectFlags, RHI::ImageAspectFlags::Depth))
                    {
                        imageAspect = RHI::ImageAspect::Depth;
                        range.m_aspectFlags = RHI::ImageAspectFlags::Depth;
                    }

                    AZStd::vector<RHI::ImageSubresourceLayout> imageSubresourceLayouts;
                    imageSubresourceLayouts.resize_no_construct(readbackItem.m_imageDescriptor.m_mipLevels);
                    size_t totalSizeInBytes = 0;
                    image->GetSubresourceLayouts(range, imageSubresourceLayouts.data(), &totalSizeInBytes);
                    AZ::u64 byteCount = totalSizeInBytes;

                    RPI::CommonBufferDescriptor desc;
                    desc.m_poolType = RPI::CommonBufferPoolType::ReadBack;
                    desc.m_bufferName = readbackItem.m_readbackName.GetStringView();
                    desc.m_byteCount = byteCount;

                    readbackItem.m_readbackBufferArray[m_readbackBufferCurrentIndex] = BufferSystemInterface::Get()->CreateBufferFromCommonPool(desc);

                    // Use the aspect format as output format, this format is also used as copy destination's format
                    readbackItem.m_imageDescriptor.m_format = FindFormatForAspect(readbackItem.m_imageDescriptor.m_format, imageAspect);

                    // copy descriptor for copying image to buffer
                    RHI::CopyImageToBufferDescriptor copyImageToBuffer;
                    copyImageToBuffer.m_sourceImage = image;
                    copyImageToBuffer.m_sourceSize = imageSubresourceLayouts[mipSlice].m_size;
                    copyImageToBuffer.m_sourceSubresource = RHI::ImageSubresource(mipSlice, 0 /*arraySlice*/, imageAspect);
                    copyImageToBuffer.m_destinationOffset = 0;
                    copyImageToBuffer.m_destinationBytesPerRow = imageSubresourceLayouts[mipSlice].m_bytesPerRow;
                    copyImageToBuffer.m_destinationBytesPerImage = imageSubresourceLayouts[mipSlice].m_bytesPerImage;
                    copyImageToBuffer.m_destinationBuffer = readbackItem.m_readbackBufferArray[m_readbackBufferCurrentIndex]->GetRHIBuffer();
                    copyImageToBuffer.m_destinationFormat = readbackItem.m_imageDescriptor.m_format;

                    readbackItem.m_imageMipInfo.m_slice = mipSlice;
                    readbackItem.m_imageMipInfo.m_size = imageSubresourceLayouts[mipSlice].m_size;

                    readbackItem.m_copyItem = copyImageToBuffer;
                }
            }
        }

        void AttachmentsReadbackGroup::CopyExecute(const RHI::FrameGraphExecuteContext& context)
        {
            for (const auto& readbackItem : m_attachmentReadbackItems)
            {
                if (readbackItem.m_readbackBufferArray[m_readbackBufferCurrentIndex])
                {
                    context.GetCommandList()->Submit(readbackItem.m_copyItem);
                }
            }
        }
        
        void AttachmentsReadbackGroup::Reset()
        {
            AttachmentReadback::Reset();
            m_attachmentReadbackItems.clear();
        }


        AttachmentsReadbackGroup::ReadbackResultWithMip AttachmentsReadbackGroup::GetReadbackResult(const AttachmentReadbackItem& readbackItem) const
        {
            ReadbackResultWithMip result;
            result.m_state = m_state;
            result.m_attachmentType = readbackItem.m_attachmentType;
            result.m_dataBuffer = readbackItem.m_dataBuffer;
            result.m_name = readbackItem.m_readbackName;
            result.m_userIdentifier = m_userIdentifier;
            result.m_imageDescriptor = readbackItem.m_imageDescriptor;
            result.m_imageDescriptor.m_arraySize = 1;
            result.m_mipInfo = readbackItem.m_imageMipInfo;
            return result;
        }

        bool AttachmentsReadbackGroup::CopyBufferData(uint32_t readbackBufferIndex)
        {
            for (auto& readbackItem : m_attachmentReadbackItems)
            {
                Data::Instance<Buffer> readbackBufferCurrent = readbackItem.m_readbackBufferArray[readbackBufferIndex];

                if (!readbackBufferCurrent)
                {
                    return false;
                }

                auto bufferSize = readbackBufferCurrent->GetBufferSize();
                readbackItem.m_dataBuffer = AZStd::make_shared<AZStd::vector<uint8_t>>();

                void* buf = readbackBufferCurrent->Map(bufferSize, 0);
                if (buf)
                {
                    if (readbackItem.m_attachmentType == RHI::AttachmentType::Buffer)
                    {
                        readbackItem.m_dataBuffer->resize_no_construct(bufferSize);
                        memcpy(readbackItem.m_dataBuffer->data(), buf, bufferSize);
                    }
                    else if (readbackItem.m_attachmentType == RHI::AttachmentType::Image)
                    {
                        RHI::Size mipSize = readbackItem.m_imageMipInfo.m_size;
                        RHI::ImageSubresourceLayout imageLayout = RHI::GetImageSubresourceLayout(mipSize,
                            readbackItem.m_imageDescriptor.m_format);

                        auto rowCount = imageLayout.m_rowCount;
                        auto byteCount = imageLayout.m_bytesPerImage;
                        if (readbackItem.m_imageDescriptor.m_dimension == AZ::RHI::ImageDimension::Image3D)
                        {
                            byteCount *= mipSize.m_depth;
                            rowCount *= mipSize.m_depth;
                        }

                        readbackItem.m_dataBuffer->resize_no_construct(byteCount);
                        const uint8_t* const sourceBegin = static_cast<uint8_t*>(buf);
                        uint8_t* const destBegin = readbackItem.m_dataBuffer->data();
                        // The source image WAS the destination when the copy item transferred data from GPU to CPU
                        // this explains why the name srcBytesPerRow for these memcpy operations.
                        const auto srcBytesPerRow = readbackItem.m_copyItem.m_imageToBuffer.m_destinationBytesPerRow;
                        for (uint32_t row = 0; row < rowCount; ++row)
                        {
                            void* dest = destBegin + row * imageLayout.m_bytesPerRow;
                            const void* source = sourceBegin + row * srcBytesPerRow;
                            memcpy(dest, source, imageLayout.m_bytesPerRow);
                        }
                    }

                    readbackBufferCurrent->Unmap();
                    m_isReadbackComplete[readbackBufferIndex] = true;
                }
                else
                {
                    return false;
                }
            }
            return true;
        }
    }   // namespace RPI
}   // namespace AZ
