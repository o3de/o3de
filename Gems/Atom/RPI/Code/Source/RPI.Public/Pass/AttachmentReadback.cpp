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
#include <Atom/RPI.Public/Pass/AttachmentReadback.h>
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
        AttachmentReadback::AttachmentReadback(const RHI::ScopeId& scopeId) : m_dispatchItem(RHI::MultiDevice::AllDevices)
        {
            for(uint32_t i = 0; i < RHI::Limits::Device::FrameCountMax; i++)
            {
                m_isReadbackComplete.push_back(false);
            }
            
            // Create fence
            m_fence = aznew RHI::Fence;
            AZ_Assert(m_fence != nullptr, "AttachmentReadback failed to create a fence");
            [[maybe_unused]] RHI::ResultCode result = m_fence->Init(RHI::MultiDevice::AllDevices, RHI::FenceState::Reset);
            AZ_Assert(result == RHI::ResultCode::Success, "AttachmentReadback failed to init fence");

            // Load shader and srg
            constexpr const char* ShaderPath = "shaders/decomposemsimage.azshader";
            m_decomposeShader = LoadCriticalShader(ShaderPath);

            if (m_decomposeShader == nullptr)
            {
                AZ_Error("PassSystem", false, "[AttachmentReadback]: Failed to load shader '%s'!", ShaderPath);
                return;
            }

            // Load SRG
            const auto srgLayout = m_decomposeShader->FindShaderResourceGroupLayout(SrgBindingSlot::Object);
            if (srgLayout)
            {
                m_decomposeSrg = ShaderResourceGroup::Create(m_decomposeShader->GetAsset(), m_decomposeShader->GetSupervariantIndex(), srgLayout->GetName());

                if (!m_decomposeSrg)
                {
                    AZ_Error("PassSystem", false, "Failed to create SRG from shader asset '%s'", ShaderPath);
                    return;
                }
            }

            RHI::PipelineStateDescriptorForDispatch pipelineStateDescriptor;
            const auto& shaderVariant = m_decomposeShader->GetVariant(RPI::ShaderAsset::RootShaderVariantStableId);
            shaderVariant.ConfigurePipelineState(pipelineStateDescriptor);

            m_dispatchItem.SetPipelineState(m_decomposeShader->AcquirePipelineState(pipelineStateDescriptor));

            AZStd::array<const RHI::ShaderResourceGroup*, 1> srgs{m_decomposeSrg->GetRHIShaderResourceGroup()};

            m_dispatchItem.SetShaderResourceGroups(srgs);

            // find srg input indexes
            m_decomposeInputImageIndex = m_decomposeSrg->FindShaderInputImageIndex(Name("m_msImage"));
            m_decomposeOutputImageIndex = m_decomposeSrg->FindShaderInputImageIndex(Name("m_outputImage"));

            // build scope producer for copying
            m_copyScopeProducer = AZStd::make_shared<RHI::ScopeProducerFunctionNoData>(
                scopeId,
                AZStd::bind(&AttachmentReadback::CopyPrepare, this, AZStd::placeholders::_1),
                AZStd::bind(&AttachmentReadback::CopyCompile, this, AZStd::placeholders::_1),
                AZStd::bind(&AttachmentReadback::CopyExecute, this, AZStd::placeholders::_1));

            m_state = ReadbackState::Idle;
        }

        AttachmentReadback::~AttachmentReadback()
        {
            Reset();
            m_fence = nullptr;
                
        }

        bool AttachmentReadback::ReadPassAttachment(const PassAttachment* attachment, const AZ::Name& readbackName, const RHI::ImageSubresourceRange* mipsRange)
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

            if (!attachment || (attachment->GetAttachmentType() != RHI::AttachmentType::Buffer && attachment->GetAttachmentType() != RHI::AttachmentType::Image))
            {
                AZ_Assert(false, "ReadPassAttachment: attachment is not a buffer or an image");
                return false;
            }

            m_state = ReadbackState::AttachmentSet;

            m_attachmentId = attachment->GetAttachmentId();
            m_attachmentType = attachment->GetAttachmentType();

            m_readbackName = readbackName;
            if (m_readbackName.IsEmpty())
            {
                m_readbackName = AZStd::string::format("%s_RB", m_attachmentId.GetCStr());
            }

            m_copyAttachmentId = m_attachmentId;

            // Get some attachment information
            if (m_attachmentType == RHI::AttachmentType::Buffer)
            {
                if (attachment->m_importedResource)
                {
                    Buffer* buffer = static_cast<Buffer*>(attachment->m_importedResource.get());
                    m_bufferAttachmentByteSize = buffer->GetBufferSize();
                }
                else
                {
                    m_bufferAttachmentByteSize = attachment->m_descriptor.m_buffer.m_byteCount;
                }
                m_readbackItems.push_back({});
                auto& item = m_readbackItems.back();
                item.m_readbackBufferArray.resize(RHI::Limits::Device::FrameCountMax, nullptr);
            }
            else
            {
                if (attachment->m_importedResource)
                {
                    AttachmentImage* attImage = static_cast<AttachmentImage*>(attachment->m_importedResource.get());
                    m_imageDescriptor = attImage->GetRHIImage()->GetDescriptor();
                }
                else
                {
                    m_imageDescriptor = attachment->m_descriptor.m_image;
                }

                m_imageMipsRange = (mipsRange != nullptr)
                    ? *mipsRange
                    : RHI::ImageSubresourceRange(0, 0, 0, 0);
                for (uint32_t mipIndex = m_imageMipsRange.m_mipSliceMin; mipIndex <= m_imageMipsRange.m_mipSliceMax; mipIndex++)
                {
                    m_readbackItems.push_back({});
                    auto& mipItem = m_readbackItems.back();
                    mipItem.m_readbackBufferArray.resize(RHI::Limits::Device::FrameCountMax, nullptr);
                }

                // Add decompose scope to convert multi-sampled images to image array
                if (m_imageDescriptor.m_multisampleState.m_samples > 1)
                {
                    m_copyAttachmentId = RHI::AttachmentId(AZStd::string::format("%s_Decomposed", m_attachmentId.GetCStr()));
                    m_decomposeScopeProducer = AZStd::make_shared<RHI::ScopeProducerFunctionNoData>(
                        m_copyAttachmentId,
                        AZStd::bind(&AttachmentReadback::DecomposePrepare, this, AZStd::placeholders::_1),
                        AZStd::bind(&AttachmentReadback::DecomposeCompile, this, AZStd::placeholders::_1),
                        AZStd::bind(&AttachmentReadback::DecomposeExecute, this, AZStd::placeholders::_1));
                }
            }
            return true;
        }

        void AttachmentReadback::DecomposePrepare(RHI::FrameGraphInterface frameGraph)
        {
            RHI::ImageScopeAttachmentDescriptor inputDesc{ m_attachmentId };
            inputDesc.m_imageViewDescriptor.m_aspectFlags = RHI::CheckBitsAny(RHI::GetImageAspectFlags(m_imageDescriptor.m_format), RHI::ImageAspectFlags::Depth)?
                RHI::ImageAspectFlags::Depth:RHI::ImageAspectFlags::Color;
            frameGraph.UseAttachment(inputDesc, RHI::ScopeAttachmentAccess::Read, RHI::ScopeAttachmentUsage::Shader, RHI::ScopeAttachmentStage::ComputeShader);
            RHI::ImageScopeAttachmentDescriptor outputDesc{ m_copyAttachmentId };
            frameGraph.UseAttachment(
                outputDesc, RHI::ScopeAttachmentAccess::Write, RHI::ScopeAttachmentUsage::Shader, RHI::ScopeAttachmentStage::ComputeShader);
        }

        void AttachmentReadback::DecomposeCompile(const RHI::FrameGraphCompileContext& context)
        {
            // prepare compute shader which to convert multi-sample texture to texture array
            RHI::DispatchDirect dispatchArgs;
            dispatchArgs.m_totalNumberOfThreadsX = m_imageDescriptor.m_size.m_width;
            dispatchArgs.m_totalNumberOfThreadsY = m_imageDescriptor.m_size.m_height;
            dispatchArgs.m_totalNumberOfThreadsZ = m_imageDescriptor.m_arraySize;
            dispatchArgs.m_threadsPerGroupX = 16; // these numbers are matching numthreads in shader file
            dispatchArgs.m_threadsPerGroupY = 16;
            dispatchArgs.m_threadsPerGroupZ = 1;

            m_dispatchItem.SetArguments(dispatchArgs);

            const RHI::ImageView* imageView = context.GetImageView(m_attachmentId);
            m_decomposeSrg->SetImageView(m_decomposeInputImageIndex, imageView);
            imageView = context.GetImageView(m_copyAttachmentId);
            m_decomposeSrg->SetImageView(m_decomposeOutputImageIndex, imageView);

            m_decomposeSrg->Compile();
        }

        void AttachmentReadback::DecomposeExecute(const RHI::FrameGraphExecuteContext& context)
        {
            context.GetCommandList()->Submit(m_dispatchItem.GetDeviceDispatchItem(context.GetDeviceIndex()));
        }
        
        void AttachmentReadback::CopyPrepare(RHI::FrameGraphInterface frameGraph)
        {
            if (m_attachmentType == RHI::AttachmentType::Buffer)
            {
                RHI::BufferScopeAttachmentDescriptor descriptor{ m_copyAttachmentId };
                descriptor.m_bufferViewDescriptor = RHI::BufferViewDescriptor::CreateRaw(0, aznumeric_cast<uint32_t>(m_bufferAttachmentByteSize));
                frameGraph.UseCopyAttachment(descriptor, RHI::ScopeAttachmentAccess::Read);
            }
            else if (m_attachmentType == RHI::AttachmentType::Image)
            {
                RHI::ImageScopeAttachmentDescriptor descriptor{ m_copyAttachmentId };
                frameGraph.UseCopyAttachment(descriptor, RHI::ScopeAttachmentAccess::Read);
            }

            frameGraph.SetEstimatedItemCount(static_cast<uint32_t>(m_readbackItems.size()));
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
                    for (auto& readbackItem : m_readbackItems)
                    {
                        readbackItem.m_readbackBufferArray[i] = nullptr;
                    }
                }
            }
            // Loop the triple buffer index and cache the current index to the callback.
            m_readbackBufferCurrentIndex = (m_readbackBufferCurrentIndex + 1) % RHI::Limits::Device::FrameCountMax;
        }

        void AttachmentReadback::CopyCompile(const RHI::FrameGraphCompileContext& context)
        {
            if (m_attachmentType == RHI::AttachmentType::Buffer)
            {
                const AZ::RHI::Buffer* buffer = context.GetBuffer(m_copyAttachmentId);

                RPI::CommonBufferDescriptor desc;
                desc.m_poolType = RPI::CommonBufferPoolType::ReadBack;
                desc.m_bufferName = m_readbackName.GetStringView();
                desc.m_byteCount = buffer->GetDescriptor().m_byteCount;

                m_readbackItems[0].m_readbackBufferArray[m_readbackBufferCurrentIndex] = BufferSystemInterface::Get()->CreateBufferFromCommonPool(desc);

                // copy buffer
                RHI::CopyBufferDescriptor copyBuffer;
                copyBuffer.m_sourceBuffer = buffer;
                copyBuffer.m_destinationBuffer = m_readbackItems[0].m_readbackBufferArray[m_readbackBufferCurrentIndex]->GetRHIBuffer();
                copyBuffer.m_size = aznumeric_cast<uint32_t>(desc.m_byteCount);

                m_readbackItems[0].m_copyItem = copyBuffer;
            }
            else if (m_attachmentType == RHI::AttachmentType::Image)
            {
                // copy image to read back buffer since only buffer can be accessed by host
                const AZ::RHI::Image* image = context.GetImage(m_copyAttachmentId);
                if (!image)
                {
                    AZ_Warning("AttachmentReadback", false, "Failed to find attachment image %s for copy to buffer", m_copyAttachmentId.GetCStr());
                    return;
                }

                for (uint16_t itemIdx = 0; itemIdx < static_cast<uint16_t>(m_readbackItems.size()); itemIdx++)
                {
                    auto& readbackItem = m_readbackItems[itemIdx];

                    // copy descriptor for copying image to buffer
                    RHI::CopyImageToBufferDescriptor copyImageToBuffer;
                    copyImageToBuffer.m_sourceImage = image;

                    readbackItem.m_copyItem = copyImageToBuffer;
                }
            }
        }

        void AttachmentReadback::CopyExecute(const RHI::FrameGraphExecuteContext& context)
        {
            for (uint16_t itemIdx = 0; itemIdx < static_cast<uint16_t>(m_readbackItems.size()); itemIdx++)
            {
                auto& readbackItem = m_readbackItems[itemIdx];
                if (m_attachmentType == RHI::AttachmentType::Image)
                {
                    // copy image to read back buffer since only buffer can be accessed by host
                    const auto image = readbackItem.m_copyItem.m_image.m_sourceImage;
                    if (!image)
                    {
                        AZ_Warning(
                            "AttachmentReadback",
                            false,
                            "Failed to find attachment image %s for copy to buffer",
                            m_copyAttachmentId.GetCStr());
                        return;
                    }

                    m_imageDescriptor = image->GetDescriptor();
                    // [GFX TODO] [ATOM-14140] [Pass Tree] Add the ability to output all the array subresources and planars
                    // only array 0, and one aspect (planar) at this moment.
                    // Note: Mip Levels and Texture3D images are supported.
                    const uint16_t mipSlice = m_imageMipsRange.m_mipSliceMin + itemIdx;
                    RHI::ImageSubresourceRange range(mipSlice, mipSlice, 0, 0);
                    range.m_aspectFlags = RHI::ImageAspectFlags::Color;

                    // setup aspect
                    RHI::ImageAspect imageAspect = RHI::ImageAspect::Color;
                    RHI::ImageAspectFlags imageAspectFlags = RHI::GetImageAspectFlags(m_imageDescriptor.m_format);
                    if (RHI::CheckBitsAll(imageAspectFlags, RHI::ImageAspectFlags::Depth))
                    {
                        imageAspect = RHI::ImageAspect::Depth;
                        range.m_aspectFlags = RHI::ImageAspectFlags::Depth;
                    }

                    AZStd::vector<RHI::DeviceImageSubresourceLayout> imageSubresourceLayouts;
                    imageSubresourceLayouts.resize_no_construct(m_imageDescriptor.m_mipLevels);
                    size_t totalSizeInBytes = 0;
                    image->GetDeviceImage(context.GetDeviceIndex())->GetSubresourceLayouts(range, imageSubresourceLayouts.data(), &totalSizeInBytes);
                    AZ::u64 byteCount = totalSizeInBytes;

                    RPI::CommonBufferDescriptor desc;
                    desc.m_poolType = RPI::CommonBufferPoolType::ReadBack;
                    desc.m_bufferName = m_readbackName.GetStringView();
                    desc.m_byteCount = byteCount;

                    readbackItem.m_readbackBufferArray[m_readbackBufferCurrentIndex] =
                        BufferSystemInterface::Get()->CreateBufferFromCommonPool(desc);

                    // Use the aspect format as output format, this format is also used as copy destination's format
                    m_imageDescriptor.m_format = FindFormatForAspect(m_imageDescriptor.m_format, imageAspect);

                    // copy descriptor for copying image to buffer
                    RHI::CopyImageToBufferDescriptor copyImageToBuffer;
                    copyImageToBuffer.m_sourceImage = image;
                    copyImageToBuffer.m_sourceSize = imageSubresourceLayouts[mipSlice].m_size;
                    copyImageToBuffer.m_sourceSubresource = RHI::ImageSubresource(mipSlice, 0 /*arraySlice*/, imageAspect);
                    copyImageToBuffer.m_destinationOffset = 0;
                    copyImageToBuffer.m_destinationBytesPerRow = imageSubresourceLayouts[mipSlice].m_bytesPerRow;
                    copyImageToBuffer.m_destinationBytesPerImage = imageSubresourceLayouts[mipSlice].m_bytesPerImage;
                    copyImageToBuffer.m_destinationBuffer = readbackItem.m_readbackBufferArray[m_readbackBufferCurrentIndex]->GetRHIBuffer();
                    copyImageToBuffer.m_destinationFormat = m_imageDescriptor.m_format;

                    readbackItem.m_mipInfo.m_slice = mipSlice;
                    readbackItem.m_mipInfo.m_size = imageSubresourceLayouts[mipSlice].m_size;

                    readbackItem.m_copyItem = copyImageToBuffer;
                }

                if (readbackItem.m_readbackBufferArray[m_readbackBufferCurrentIndex])
                {
                    context.GetCommandList()->Submit(readbackItem.m_copyItem.GetDeviceCopyItem(context.GetDeviceIndex()));
                }
            }

            uint32_t readbackBufferCurrentIndex = m_readbackBufferCurrentIndex;
            auto deviceIndex = context.GetDeviceIndex();
            m_fence->GetDeviceFence(deviceIndex)
                ->WaitOnCpuAsync(
                    [this, readbackBufferCurrentIndex, deviceIndex]()
                    {
                        if (m_state == ReadbackState::Reading)
                        {
                            if (CopyBufferData(readbackBufferCurrentIndex, deviceIndex))
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
                            m_callback(GetReadbackResult());
                        }

                        Reset();
                    });
        }

        void AttachmentReadback::Reset()
        {
            m_attachmentId = RHI::AttachmentId{};
            m_readbackItems.clear();
            m_state = ReadbackState::Idle;
            m_readbackName = AZ::Name{};
            m_copyAttachmentId = RHI::AttachmentId{};
            m_decomposeScopeProducer = nullptr;
            if (m_decomposeSrg)
            {
                m_decomposeSrg->SetImageView(m_decomposeInputImageIndex, nullptr);
                m_decomposeSrg->SetImageView(m_decomposeOutputImageIndex, nullptr);
            }
            if (m_fence)
            {
                m_fence->Reset();
            }
        }

        AttachmentReadback::ReadbackState AttachmentReadback::GetReadbackState()
        {
            return m_state;
        }

        void AttachmentReadback::SetCallback(CallbackFunction callback)
        {
            m_callback = callback;
        }

        void AttachmentReadback::SetUserIdentifier(uint32_t userIdentifier)
        {
            m_userIdentifier = userIdentifier;
        }

        void AttachmentReadback::FrameBegin(Pass::FramePrepareParams params)
        {
            if (m_state == AttachmentReadback::ReadbackState::AttachmentSet)
            {
                // Need decompose 
                if (m_decomposeScopeProducer)
                {
                    // Create transient image array to save decompose result
                    RHI::TransientImageDescriptor descriptor;
                    descriptor.m_attachmentId = m_copyAttachmentId;

                    auto format = m_imageDescriptor.m_format;

                    // We can only use one planar for none render target shader output. Set to output Depth aspect only
                    if (RHI::GetImageAspectFlags(format) == RHI::ImageAspectFlags::DepthStencil)
                    {
                        format = FindFormatForAspect(format, RHI::ImageAspect::Depth);
                    }

                    descriptor.m_imageDescriptor = RHI::ImageDescriptor::Create2DArray(RHI::ImageBindFlags::ShaderReadWrite,
                        m_imageDescriptor.m_size.m_width, m_imageDescriptor.m_size.m_height,
                        m_imageDescriptor.m_multisampleState.m_samples, // Use sample count as array size
                        format);

                    params.m_frameGraphBuilder->GetAttachmentDatabase().CreateTransientImage(descriptor);

                    params.m_frameGraphBuilder->ImportScopeProducer(*m_decomposeScopeProducer.get());
                }

                // Import copy producer
                params.m_frameGraphBuilder->ImportScopeProducer(*m_copyScopeProducer.get());

                m_state = AttachmentReadback::ReadbackState::Reading;

            }
        }

        bool AttachmentReadback::IsFinished() const
        {
            return m_state == ReadbackState::Success || m_state == ReadbackState::Failed;
        }

        bool AttachmentReadback::IsReady() const
        {
            return !(m_state == ReadbackState::Reading || m_state == ReadbackState::Uninitialized);
        }

        AttachmentReadback::ReadbackResult AttachmentReadback::GetReadbackResult() const
        {
            ReadbackResult result;

            if (m_readbackItems.empty())
            {
                // the AttachmentReadback was reset before the readback was triggered from the GPU. Avoid 
                // a crash by accessing a non-existend readback item
                result.m_state = ReadbackState::Failed;
                return result;
            }

            result.m_state = m_state;
            result.m_attachmentType = m_attachmentType;
            result.m_dataBuffer = m_readbackItems[0].m_dataBuffer;
            result.m_name = m_readbackName;
            result.m_userIdentifier = m_userIdentifier;
            result.m_imageDescriptor = m_imageDescriptor;
            result.m_imageDescriptor.m_arraySize = 1;

            if (m_attachmentType == RHI::AttachmentType::Image)
            {
                result.m_mipDataBuffers.reserve(m_readbackItems.size());
                for (const auto& readbackItem : m_readbackItems)
                {
                    result.m_mipDataBuffers.push_back({});
                    auto& mipDataBuffer = result.m_mipDataBuffers.back();
                    mipDataBuffer.m_mipBuffer = readbackItem.m_dataBuffer;
                    mipDataBuffer.m_mipInfo = readbackItem.m_mipInfo;
                }
            }

            return result;
        }

        bool AttachmentReadback::CopyBufferData(uint32_t readbackBufferIndex, int deviceIndex)
        {
            for (auto& readbackItem : m_readbackItems)
            {
                Data::Instance<Buffer> readbackBufferCurrent = readbackItem.m_readbackBufferArray[readbackBufferIndex];

                if (!readbackBufferCurrent)
                {
                    return false;
                }

                auto bufferSize = readbackBufferCurrent->GetBufferSize();
                readbackItem.m_dataBuffer = AZStd::make_shared<AZStd::vector<uint8_t>>();

                void* buf = readbackBufferCurrent->Map(bufferSize, 0)[deviceIndex];
                if (buf)
                {
                    if (m_attachmentType == RHI::AttachmentType::Buffer)
                    {
                        readbackItem.m_dataBuffer->resize_no_construct(bufferSize);
                        memcpy(readbackItem.m_dataBuffer->data(), buf, bufferSize);
                    }
                    else if (m_attachmentType == RHI::AttachmentType::Image)
                    {
                        RHI::Size mipSize = readbackItem.m_mipInfo.m_size;
                        RHI::DeviceImageSubresourceLayout imageLayout = RHI::GetImageSubresourceLayout(mipSize,
                            m_imageDescriptor.m_format);

                        auto rowCount = imageLayout.m_rowCount;
                        auto byteCount = imageLayout.m_bytesPerImage;
                        if (m_imageDescriptor.m_dimension == AZ::RHI::ImageDimension::Image3D)
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
    } // namespace RPI
}   // namespace AZ
