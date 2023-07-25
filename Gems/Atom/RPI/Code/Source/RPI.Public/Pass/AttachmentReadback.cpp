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
        // Helper class to build scope producer with functions
        class ScopeProducerFunction final
            : public RHI::ScopeProducer
        {
        public:
            AZ_CLASS_ALLOCATOR(ScopeProducerFunction, SystemAllocator);

            using PrepareFunction = AZStd::function<void(RHI::FrameGraphInterface)>;
            using CompileFunction = AZStd::function<void(const RHI::FrameGraphCompileContext&)>;
            using ExecuteFunction = AZStd::function<void(const RHI::FrameGraphExecuteContext&)>;

            ScopeProducerFunction(
                const RHI::ScopeId& scopeId,
                PrepareFunction prepareFunction,
                CompileFunction compileFunction,
                ExecuteFunction executeFunction)
                : ScopeProducer(scopeId)
                , m_prepareFunction{ AZStd::move(prepareFunction) }
                , m_compileFunction{ AZStd::move(compileFunction) }
                , m_executeFunction{ AZStd::move(executeFunction) }
            {}

        private:
            //////////////////////////////////////////////////////////////////////////
            // ScopeProducer overrides
            void SetupFrameGraphDependencies(RHI::FrameGraphInterface builder) override
            {
                m_prepareFunction(builder);
            }

            void CompileResources(const RHI::FrameGraphCompileContext& context) override
            {
                m_compileFunction(context);
            }

            void BuildCommandList(const RHI::FrameGraphExecuteContext& context) override
            {
                m_executeFunction(context);
            }
            //////////////////////////////////////////////////////////////////////////

            PrepareFunction m_prepareFunction;
            CompileFunction m_compileFunction;
            ExecuteFunction m_executeFunction;
        };


        // Find a format for formats with two planars (DepthStencil) based on its ImageView's aspect flag
        RHI::Format FindFormatForAspect(RHI::Format format, RHI::ImageAspect imageAspect)
        {
            RHI::ImageAspectFlags imageAspectFlags = RHI::GetImageAspectFlags(format);

            // only need to convert is the source contains two aspects
            if (imageAspectFlags == RHI::ImageAspectFlags::DepthStencil)
            {
                switch (imageAspect)
                {
                case RHI::ImageAspect::Stencil:
                    return RHI::Format::R8_UINT;
                case RHI::ImageAspect::Depth:
                {
                    switch (format)
                    {
                    case RHI::Format::D32_FLOAT_S8X24_UINT:
                        return RHI::Format::R32_FLOAT;
                    case RHI::Format::D24_UNORM_S8_UINT:
                        return RHI::Format::R32_UINT;
                    case RHI::Format::D16_UNORM_S8_UINT:
                        return RHI::Format::R16_UNORM;
                    default:
                        AZ_Assert(false, "Unknown DepthStencil format. Please update this function");
                        return RHI::Format::R32_FLOAT;
                    }
                }
                }
            }
            return format;
        }

        AttachmentReadback::AttachmentReadback(const RHI::ScopeId& scopeId)
        {
            for(uint32_t i = 0; i < RHI::Limits::Device::FrameCountMax; i++)
            {
                m_isReadbackComplete.push_back(false);
            }
            
            // Create fence
            RHI::Ptr<RHI::Device> device = RHI::RHISystemInterface::Get()->GetDevice();
            m_fence = RHI::Factory::Get().CreateFence();
            AZ_Assert(m_fence != nullptr, "AttachmentReadback failed to create a fence");
            [[maybe_unused]] RHI::ResultCode result = m_fence->Init(*device, RHI::FenceState::Reset);
            AZ_Assert(result == RHI::ResultCode::Success, "AttachmentReadback failed to init fence");

            // Load shader and srg
            const char* ShaderPath = "shaders/decomposemsimage.azshader";
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

            m_dispatchItem.m_pipelineState = m_decomposeShader->AcquirePipelineState(pipelineStateDescriptor);

            m_dispatchItem.m_shaderResourceGroupCount = 1;
            m_dispatchItem.m_shaderResourceGroups[0] = m_decomposeSrg->GetRHIShaderResourceGroup();

            // find srg input indexes
            m_decomposeInputImageIndex = m_decomposeSrg->FindShaderInputImageIndex(Name("m_msImage"));
            m_decomposeOutputImageIndex = m_decomposeSrg->FindShaderInputImageIndex(Name("m_outputImage"));

            // build scope producer for copying
            m_copyScopeProducer = AZStd::make_shared<ScopeProducerFunction>(
                    scopeId,
                    AZStd::bind(&AttachmentReadback::CopyPrepare, this, AZStd::placeholders::_1),
                    AZStd::bind(&AttachmentReadback::CopyCompile, this, AZStd::placeholders::_1),
                    AZStd::bind(&AttachmentReadback::CopyExecute, this, AZStd::placeholders::_1)
                );

            m_state = ReadbackState::Idle;
        }

        AttachmentReadback::~AttachmentReadback()
        {
            Reset();
            m_fence = nullptr;
                
        }

        bool AttachmentReadback::ReadPassAttachment(const PassAttachment* attachment, const AZ::Name& readbackName, const RHI::ImageViewDescriptor* imageViewDescriptor)
        {
            AZ::RHI::ImageViewDescriptor defaultViewDescriptor;
            if (imageViewDescriptor != nullptr)
            {
                defaultViewDescriptor = *imageViewDescriptor;
            }
            else if (attachment->GetAttachmentType() == RHI::AttachmentType::Image)
            {
                const auto imageDescriptor = GetImageDescriptorFromAttachment(attachment);
                defaultViewDescriptor = CreateDefaultImageViewDescriptorFromAttachment(imageDescriptor);
            }

            AZStd::vector<ReadbackRequestInfo> readbackRequests;
            readbackRequests.push_back({});
            ReadbackRequestInfo& request = readbackRequests.back();
            request.m_attachment = attachment;
            request.m_readbackName = readbackName;
            request.m_imageViewDescriptor = defaultViewDescriptor;
            return ReadPassAttachments(readbackRequests);

        }

        bool AttachmentReadback::ReadPassAttachments(const AZStd::vector<ReadbackRequestInfo>& readbackAttachmentRequests)
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
                    readbackItem.m_imageDescriptor = GetImageDescriptorFromAttachment(attachment);
                }
            }

            if (
                    (readbackAttachmentRequests.size() == 1) &&
                    (readbackAttachmentRequests[0].m_attachment->GetAttachmentType() == RHI::AttachmentType::Image)
               )
            {
                // Add decompose scope to convert multi-sampled images to image array
                if (m_attachmentReadbackItems[0].m_imageDescriptor.m_multisampleState.m_samples > 1)
                {
                    m_attachmentReadbackItems[0].m_attachmentId = RHI::AttachmentId(AZStd::string::format("%s_Decomposed", m_attachmentReadbackItems[0].m_attachmentId.GetCStr()));
                    m_decomposeScopeProducer = AZStd::make_shared<ScopeProducerFunction>(
                        m_attachmentReadbackItems[0].m_attachmentId,
                        AZStd::bind(&AttachmentReadback::DecomposePrepare, this, AZStd::placeholders::_1),
                        AZStd::bind(&AttachmentReadback::DecomposeCompile, this, AZStd::placeholders::_1),
                        AZStd::bind(&AttachmentReadback::DecomposeExecute, this, AZStd::placeholders::_1)
                        );
                }
            }

            return true;
        }

        void AttachmentReadback::DecomposePrepare(RHI::FrameGraphInterface frameGraph)
        {
            RHI::ImageScopeAttachmentDescriptor inputDesc{ m_attachmentReadbackItems[0].m_attachmentId };
            inputDesc.m_imageViewDescriptor.m_aspectFlags = RHI::CheckBitsAny(RHI::GetImageAspectFlags(m_attachmentReadbackItems[0].m_imageDescriptor.m_format), RHI::ImageAspectFlags::Depth)?
                RHI::ImageAspectFlags::Depth:RHI::ImageAspectFlags::Color;
            frameGraph.UseAttachment(inputDesc, RHI::ScopeAttachmentAccess::Read, RHI::ScopeAttachmentUsage::Shader);
            RHI::ImageScopeAttachmentDescriptor outputDesc{ m_attachmentReadbackItems[0].m_copyAttachmentId };
            frameGraph.UseAttachment(outputDesc, RHI::ScopeAttachmentAccess::Write, RHI::ScopeAttachmentUsage::Shader);
        }

        void AttachmentReadback::DecomposeCompile(const RHI::FrameGraphCompileContext& context)
        {
            // prepare compute shader which to convert multi-sample texture to texture array
            RHI::DispatchDirect dispatchArgs;
            const auto& imageDescriptor = m_attachmentReadbackItems[0].m_imageDescriptor;
            dispatchArgs.m_totalNumberOfThreadsX = imageDescriptor.m_size.m_width;
            dispatchArgs.m_totalNumberOfThreadsY = imageDescriptor.m_size.m_height;
            dispatchArgs.m_totalNumberOfThreadsZ = imageDescriptor.m_arraySize;
            dispatchArgs.m_threadsPerGroupX = 16; // these numbers are matching numthreads in shader file
            dispatchArgs.m_threadsPerGroupY = 16;
            dispatchArgs.m_threadsPerGroupZ = 1;

            m_dispatchItem.m_arguments = dispatchArgs;

            const RHI::ImageView* imageView = context.GetImageView(m_attachmentReadbackItems[0].m_attachmentId);
            m_decomposeSrg->SetImageView(m_decomposeInputImageIndex, imageView);

            imageView = context.GetImageView(m_attachmentReadbackItems[0].m_copyAttachmentId);
            m_decomposeSrg->SetImageView(m_decomposeOutputImageIndex, imageView);

            m_decomposeSrg->Compile();
        }

        void AttachmentReadback::DecomposeExecute(const RHI::FrameGraphExecuteContext& context)
        {
            context.GetCommandList()->Submit(m_dispatchItem);
        }
        
        void AttachmentReadback::CopyPrepare(RHI::FrameGraphInterface frameGraph)
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

        void AttachmentReadback::CopyCompile(const RHI::FrameGraphCompileContext& context)
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

                    // [GFX TODO] [ATOM-14140] [Pass Tree] Add the ability to output all the array subresources and planars
                    // only array 0, and one aspect (planar) at this moment.
                    // Note: Mip Levels and Texture3D images are supported.
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

        void AttachmentReadback::CopyExecute(const RHI::FrameGraphExecuteContext& context)
        {
            for (const auto& readbackItem : m_attachmentReadbackItems)
            {
                if (readbackItem.m_readbackBufferArray[m_readbackBufferCurrentIndex])
                {
                    context.GetCommandList()->Submit(readbackItem.m_copyItem);
                }
            }
        }
        
        void AttachmentReadback::Reset()
        {
            m_attachmentReadbackItems.clear();
            m_state = ReadbackState::Idle;
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
                    descriptor.m_attachmentId = m_attachmentReadbackItems[0].m_copyAttachmentId;
                    const auto& imageDescriptor = m_attachmentReadbackItems[0].m_imageDescriptor;
                    auto format = imageDescriptor.m_format;

                    // We can only use one planar for none render target shader output. Set to output Depth aspect only
                    if (RHI::GetImageAspectFlags(format) == RHI::ImageAspectFlags::DepthStencil)
                    {
                        format = FindFormatForAspect(format, RHI::ImageAspect::Depth);
                    }

                    descriptor.m_imageDescriptor = RHI::ImageDescriptor::Create2DArray(RHI::ImageBindFlags::ShaderReadWrite,
                        imageDescriptor.m_size.m_width, imageDescriptor.m_size.m_height,
                        imageDescriptor.m_multisampleState.m_samples, // Use sample count as array size
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

        AttachmentReadback::ReadbackResult AttachmentReadback::GetReadbackResult(const AttachmentReadbackItem& readbackItem) const
        {
            ReadbackResult result;
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

        bool AttachmentReadback::CopyBufferData(uint32_t readbackBufferIndex)
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

        RHI::ImageDescriptor AttachmentReadback::GetImageDescriptorFromAttachment(const PassAttachment* attachment)
        {
            if (attachment->m_importedResource)
            {
                AttachmentImage* attImage = static_cast<AttachmentImage*>(attachment->m_importedResource.get());
                return attImage->GetRHIImage()->GetDescriptor();
            }

            return attachment->m_descriptor.m_image;
        }

        RHI::ImageViewDescriptor AttachmentReadback::CreateDefaultImageViewDescriptorFromAttachment(const RHI::ImageDescriptor& imageDescriptor)
        {
            RHI::ImageViewDescriptor imageViewDescriptor;
            if (imageDescriptor.m_dimension == RHI::ImageDimension::Image3D)
            {
                const auto depthSliceMax = static_cast<uint16_t>(imageDescriptor.m_size.m_depth - 1);
                imageViewDescriptor = RHI::ImageViewDescriptor::Create3D(imageDescriptor.m_format,
                    0 /*mipSliceMin*/, 0 /*mipSliceMax*/,
                    0 /*depthSliceMin*/,  depthSliceMax);
            }
            else
            {
                imageViewDescriptor = RHI::ImageViewDescriptor::Create(imageDescriptor.m_format,
                    0 /*mipSliceMin*/, 0 /*mipSliceMax*/);
            }
            return imageViewDescriptor;
        }
    }   // namespace RPI
}   // namespace AZ
