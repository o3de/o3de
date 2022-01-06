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
            AZ_CLASS_ALLOCATOR(ScopeProducerFunction, SystemAllocator, 0);

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
            : m_readbackBufferArray({ nullptr, nullptr, nullptr })
            , m_isReadbackComplete({ false, false, false })
        {
            // Create fence
            RHI::Ptr<RHI::Device> device = RHI::RHISystemInterface::Get()->GetDevice();
            m_fence = RHI::Factory::Get().CreateFence();
            m_fence->Init(*device, RHI::FenceState::Reset);

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

        bool AttachmentReadback::ReadPassAttachment(const PassAttachment* attachment, const AZ::Name& readbackName)
        {
            if (AZ::RHI::IsNullRenderer())
            {
                return false;
            }

            if (!IsReady())
            {
                AZ_Assert(false, "AttachmentReadback is not ready to readback an attachment");
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

                // Add decompose scope to convert multi-sampled images to image array
                if (m_imageDescriptor.m_multisampleState.m_samples > 1)
                {
                    m_copyAttachmentId = RHI::AttachmentId(AZStd::string::format("%s_Decomposed", m_attachmentId.GetCStr()));
                    m_decomposeScopeProducer = AZStd::make_shared<ScopeProducerFunction>(
                        m_copyAttachmentId,
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
            RHI::ImageScopeAttachmentDescriptor inputDesc{ m_attachmentId };
            inputDesc.m_imageViewDescriptor.m_aspectFlags = RHI::CheckBitsAll(RHI::GetImageAspectFlags(m_imageDescriptor.m_format), RHI::ImageAspectFlags::Depth)?
                RHI::ImageAspectFlags::Depth:RHI::ImageAspectFlags::Color;
            frameGraph.UseAttachment(inputDesc, RHI::ScopeAttachmentAccess::Read, RHI::ScopeAttachmentUsage::Shader);
            RHI::ImageScopeAttachmentDescriptor outputDesc{ m_copyAttachmentId };
            frameGraph.UseAttachment(outputDesc, RHI::ScopeAttachmentAccess::Write, RHI::ScopeAttachmentUsage::Shader);
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

            m_dispatchItem.m_arguments = dispatchArgs;

            const RHI::ImageView* imageView = context.GetImageView(m_attachmentId);
            m_decomposeSrg->SetImageView(m_decomposeInputImageIndex, imageView);

            imageView = context.GetImageView(m_copyAttachmentId);
            m_decomposeSrg->SetImageView(m_decomposeOutputImageIndex, imageView);

            m_decomposeSrg->Compile();
        }

        void AttachmentReadback::DecomposeExecute(const RHI::FrameGraphExecuteContext& context)
        {
            context.GetCommandList()->Submit(m_dispatchItem);
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
                    m_readbackBufferArray[i] = nullptr;
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
                        m_callback(GetReadbackResult());
                    }

                    Reset();
                }
            );
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

                m_readbackBufferArray[m_readbackBufferCurrentIndex] = BufferSystemInterface::Get()->CreateBufferFromCommonPool(desc);

                // copy buffer
                RHI::CopyBufferDescriptor copyBuffer;
                copyBuffer.m_sourceBuffer = buffer;
                copyBuffer.m_destinationBuffer = m_readbackBufferArray[m_readbackBufferCurrentIndex]->GetRHIBuffer();
                copyBuffer.m_size = aznumeric_cast<uint32_t>(desc.m_byteCount);
                
                m_copyItem = copyBuffer;
            }
            else if (m_attachmentType == RHI::AttachmentType::Image)
            {
                // copy image to read back buffer since only buffer can be accessed by host
                const AZ::RHI::Image* image = context.GetImage(m_copyAttachmentId);
                m_imageDescriptor = image->GetDescriptor();

                // [GFX TODO] [ATOM-14140] [Pass Tree] Add the ability to output all the mipmaps, array and planars
                // only copy mip level 0, array 0, and one aspect (planar) at this moment
                RHI::ImageSubresourceRange range(0, 0, 0, 0);
                range.m_aspectFlags = RHI::ImageAspectFlags::Color;

                // setup aspect 
                RHI::ImageAspect imageAspect = RHI::ImageAspect::Color;
                RHI::ImageAspectFlags imageAspectFlags = RHI::GetImageAspectFlags(m_imageDescriptor.m_format);
                if (RHI::CheckBitsAll(imageAspectFlags, RHI::ImageAspectFlags::Depth))
                {
                    imageAspect = RHI::ImageAspect::Depth;
                    range.m_aspectFlags = RHI::ImageAspectFlags::Depth;
                }

                RHI::ImageSubresourceLayoutPlaced imageSubresourceLayout;
                image->GetSubresourceLayouts(range, &imageSubresourceLayout, nullptr);

                RPI::CommonBufferDescriptor desc;
                desc.m_poolType = RPI::CommonBufferPoolType::ReadBack;
                desc.m_bufferName = m_readbackName.GetStringView();
                desc.m_byteCount = imageSubresourceLayout.m_bytesPerImage;

                m_readbackBufferArray[m_readbackBufferCurrentIndex] = BufferSystemInterface::Get()->CreateBufferFromCommonPool(desc);

                // Use the aspect format as output format, this format is also used as copy destination's format
                m_imageDescriptor.m_format = FindFormatForAspect(m_imageDescriptor.m_format, imageAspect);

                // copy descriptor for copying image to buffer
                RHI::CopyImageToBufferDescriptor copyImageToBuffer;
                copyImageToBuffer.m_sourceImage = image;
                copyImageToBuffer.m_sourceSize = m_imageDescriptor.m_size;
                copyImageToBuffer.m_sourceSubresource = RHI::ImageSubresource(0 /*mipslice*/, 0 /*arraySlice*/, imageAspect);
                copyImageToBuffer.m_destinationOffset = 0;
                copyImageToBuffer.m_destinationBytesPerRow = imageSubresourceLayout.m_bytesPerRow;
                copyImageToBuffer.m_destinationBytesPerImage = imageSubresourceLayout.m_bytesPerImage;
                copyImageToBuffer.m_destinationBuffer = m_readbackBufferArray[m_readbackBufferCurrentIndex]->GetRHIBuffer();
                copyImageToBuffer.m_destinationFormat = m_imageDescriptor.m_format;

                m_copyItem = copyImageToBuffer;
            }
        }

        void AttachmentReadback::CopyExecute(const RHI::FrameGraphExecuteContext& context)
        {
            if (!m_readbackBufferArray[m_readbackBufferCurrentIndex])
            {
                return;
            }

            context.GetCommandList()->Submit(m_copyItem);
        }
        
        void AttachmentReadback::Reset()
        {
            m_attachmentId = RHI::AttachmentId{};
            m_copyItem = RHI::CopyItem{};
            m_state = ReadbackState::Idle;
            m_readbackName = AZ::Name{};
            m_dataBuffer = nullptr;
            m_fence->Reset();
            m_copyAttachmentId = RHI::AttachmentId{};
            m_decomposeScopeProducer = nullptr;
            if (m_decomposeSrg)
            {
                m_decomposeSrg->SetImageView(m_decomposeInputImageIndex, nullptr);
                m_decomposeSrg->SetImageView(m_decomposeOutputImageIndex, nullptr);
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
            result.m_state = m_state;
            result.m_attachmentType = m_attachmentType;
            result.m_dataBuffer = m_dataBuffer;
            result.m_name = m_readbackName;
            result.m_imageDescriptor = m_imageDescriptor;
            result.m_imageDescriptor.m_arraySize = 1;
            return result;
        }

        bool AttachmentReadback::CopyBufferData(uint32_t readbackBufferIndex)
        {
            Data::Instance<Buffer> readbackBufferCurrent = m_readbackBufferArray[readbackBufferIndex];

            if (!readbackBufferCurrent)
            {
                return false;
            }

            auto bufferSize = readbackBufferCurrent->GetBufferSize();
            m_dataBuffer = AZStd::make_shared<AZStd::vector<uint8_t>>();

            void* buf = readbackBufferCurrent->Map(bufferSize, 0);
            if (buf)
            {
                if (m_attachmentType == RHI::AttachmentType::Buffer)
                {
                    m_dataBuffer->resize_no_construct(bufferSize);
                    memcpy(m_dataBuffer->data(), buf, bufferSize);
                }
                else if (m_attachmentType == RHI::AttachmentType::Image)
                {
                    RHI::ImageSubresourceLayout imageLayout = RHI::GetImageSubresourceLayout(m_imageDescriptor.m_size,
                        m_imageDescriptor.m_format);

                    m_dataBuffer->resize_no_construct(imageLayout.m_bytesPerImage);

                    for (uint32_t row = 0; row < imageLayout.m_rowCount; ++row)
                    {
                        void* dest = m_dataBuffer->data() + row * imageLayout.m_bytesPerRow;
                        void* source = static_cast<uint8_t*>(buf) + row * m_copyItem.m_imageToBuffer.m_destinationBytesPerRow;
                        memcpy(dest, source, imageLayout.m_bytesPerRow);
                    }
                }

                readbackBufferCurrent->Unmap();
            }

            m_isReadbackComplete[readbackBufferIndex] = true;
            return true;
        }
    }   // namespace RPI
}   // namespace AZ
