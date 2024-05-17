/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI/CommandList.h>
#include <Atom/RHI/SingleDeviceShaderResourceGroup.h>
#include <Atom/RHI/DrawListTagRegistry.h>
#include <Atom/RHI/RHISystemInterface.h>

#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/RPISystemInterface.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/View.h>
#include <Atom/RPI.Public/Pass/CopyPass.h>
#include <Atom/RPI.Public/Pass/PassUtils.h>

namespace AZ
{
    namespace RPI
    {
        // --- Creation & Initialization ---

        Ptr<CopyPass> CopyPass::Create(const PassDescriptor& descriptor)
        {
            Ptr<CopyPass> pass = aznew CopyPass(descriptor);
            return pass;
        }

        CopyPass::CopyPass(const PassDescriptor& descriptor)
            : RenderPass(descriptor)
        {
            const CopyPassData* copyData = PassUtils::GetPassData<CopyPassData>(descriptor);

            if (copyData)
            {
                m_data = *copyData;

                if (copyData->m_useCopyQueue)
                {
                    m_hardwareQueueClass = RHI::HardwareQueueClass::Copy;
                }
            }
        }

        RHI::CopyItemType CopyPass::GetCopyItemType()
        {
            RHI::AttachmentType inputType = GetInputBinding(0).GetAttachment()->GetAttachmentType();
            RHI::AttachmentType outputType = GetOutputBinding(0).GetAttachment()->GetAttachmentType();

            RHI::CopyItemType copyType = RHI::CopyItemType::Invalid;

            if (inputType == RHI::AttachmentType::Buffer && outputType == RHI::AttachmentType::Buffer)
            {
                copyType = RHI::CopyItemType::Buffer;
            }
            else if (inputType == RHI::AttachmentType::Image && outputType == RHI::AttachmentType::Image)
            {
                copyType = RHI::CopyItemType::Image;
            }
            else if (inputType == RHI::AttachmentType::Buffer && outputType == RHI::AttachmentType::Image)
            {
                copyType = RHI::CopyItemType::BufferToImage;
            }
            else if (inputType == RHI::AttachmentType::Image && outputType == RHI::AttachmentType::Buffer)
            {
                copyType = RHI::CopyItemType::ImageToBuffer;
            }

            return copyType;
        }

        // --- Pass behavior overrides ---

        void CopyPass::BuildInternal()
        {
            AZ_Assert(GetInputCount() == 1 && GetOutputCount() == 1,
                "CopyPass has %d inputs and %d outputs. It should have exactly one of each.",
                GetInputCount(), GetOutputCount());

            AZ_Assert(m_attachmentBindings.size() == 2,
                "CopyPass must have exactly 2 bindings: 1 input and 1 output. %s has %d bindings.",
                GetPathName().GetCStr(), m_attachmentBindings.size());

            // Create transient attachment based on input if required
            if (m_data.m_cloneInput)
            {
                const Ptr<PassAttachment>& source = GetInputBinding(0).GetAttachment();
                Ptr<PassAttachment> dest = source->Clone();

                // Set bind flags to CopyWrite. Other bind flags will be auto-inferred by pass system
                if (dest->m_descriptor.m_type == RHI::AttachmentType::Image)
                {
                    dest->m_descriptor.m_image.m_bindFlags = RHI::ImageBindFlags::CopyWrite;
                }
                else if (dest->m_descriptor.m_type == RHI::AttachmentType::Buffer)
                {
                    dest->m_descriptor.m_buffer.m_bindFlags = RHI::BufferBindFlags::CopyWrite;
                }

                // Set path name for the new attachment and add it to our attachment list
                dest->ComputePathName(GetPathName());
                m_ownedAttachments.push_back(dest);

                // Set the output binding to the new attachment
                GetOutputBinding(0).SetAttachment(dest);
            }
        }

        // --- Scope producer functions ---

        void CopyPass::SetupFrameGraphDependencies(RHI::FrameGraphInterface frameGraph)
        {
            RenderPass::SetupFrameGraphDependencies(frameGraph);
        }

        void CopyPass::CompileResources(const RHI::FrameGraphCompileContext& context)
        {
            RHI::CopyItemType copyType = GetCopyItemType();
            switch (copyType)
            {
            case AZ::RHI::CopyItemType::Buffer:
                CopyBuffer(context);
                break;
            case AZ::RHI::CopyItemType::Image:
                CopyImage(context);
                break;
            case AZ::RHI::CopyItemType::BufferToImage:
                CopyBufferToImage(context);
                break;
            case AZ::RHI::CopyItemType::ImageToBuffer:
                CopyImageToBuffer(context);
                break;
            default:
                break;
            }
        }

        void CopyPass::BuildCommandListInternal(const RHI::FrameGraphExecuteContext& context)
        {
            if (m_copyItem.m_type != RHI::CopyItemType::Invalid)
            {
                context.GetCommandList()->Submit(m_copyItem);
            }
        }

        // --- Copy setup functions ---

        void CopyPass::CopyBuffer(const RHI::FrameGraphCompileContext& context)
        {
            RHI::SingleDeviceCopyBufferDescriptor copyDesc;

            // Source Buffer
            PassAttachmentBinding& copySource = GetInputBinding(0);
            const AZ::RHI::SingleDeviceBuffer* sourceBuffer = context.GetBuffer(copySource.GetAttachment()->GetAttachmentId());
            copyDesc.m_sourceBuffer = sourceBuffer;
            copyDesc.m_size = static_cast<uint32_t>(sourceBuffer->GetDescriptor().m_byteCount);
            copyDesc.m_sourceOffset = m_data.m_bufferSourceOffset;

            // Destination Buffer
            PassAttachmentBinding& copyDest = GetOutputBinding(0);
            copyDesc.m_destinationBuffer = context.GetBuffer(copyDest.GetAttachment()->GetAttachmentId());
            copyDesc.m_destinationOffset = m_data.m_bufferDestinationOffset;

            m_copyItem = copyDesc;
        }

        void CopyPass::CopyImage(const RHI::FrameGraphCompileContext& context)
        {
            RHI::SingleDeviceCopyImageDescriptor copyDesc;

            // Source Image
            PassAttachmentBinding& copySource = GetInputBinding(0);
            const AZ::RHI::SingleDeviceImage* sourceImage = context.GetImage(copySource.GetAttachment()->GetAttachmentId());
            copyDesc.m_sourceImage = sourceImage;
            copyDesc.m_sourceSize = sourceImage->GetDescriptor().m_size;
            copyDesc.m_sourceOrigin = m_data.m_imageSourceOrigin;
            copyDesc.m_sourceSubresource = m_data.m_imageSourceSubresource;

            // Destination Image
            PassAttachmentBinding& copyDest = GetOutputBinding(0);
            copyDesc.m_destinationImage = context.GetImage(copyDest.GetAttachment()->GetAttachmentId());
            copyDesc.m_destinationOrigin = m_data.m_imageDestinationOrigin;
            copyDesc.m_destinationSubresource = m_data.m_imageDestinationSubresource;

            m_copyItem = copyDesc;
        }

        void CopyPass::CopyBufferToImage(const RHI::FrameGraphCompileContext& context)
        {
            RHI::SingleDeviceCopyBufferToImageDescriptor copyDesc;

            // Source Buffer
            PassAttachmentBinding& copySource = GetInputBinding(0);
            const AZ::RHI::SingleDeviceBuffer* sourceBuffer = context.GetBuffer(copySource.GetAttachment()->GetAttachmentId());
            copyDesc.m_sourceBuffer = sourceBuffer;
            copyDesc.m_sourceSize = m_data.m_sourceSize;
            copyDesc.m_sourceOffset = m_data.m_bufferSourceOffset;
            copyDesc.m_sourceBytesPerRow = m_data.m_bufferSourceBytesPerRow;
            copyDesc.m_sourceBytesPerImage = m_data.m_bufferSourceBytesPerImage;

            // Destination Image
            PassAttachmentBinding& copyDest = GetOutputBinding(0);
            copyDesc.m_destinationImage = context.GetImage(copyDest.GetAttachment()->GetAttachmentId());
            copyDesc.m_destinationOrigin = m_data.m_imageDestinationOrigin;
            copyDesc.m_destinationSubresource = m_data.m_imageDestinationSubresource;

            m_copyItem = copyDesc;
        }

        void CopyPass::CopyImageToBuffer(const RHI::FrameGraphCompileContext& context)
        {
            RHI::SingleDeviceCopyImageToBufferDescriptor copyDesc;

            // Source Image
            PassAttachmentBinding& copySource = GetInputBinding(0);
            const AZ::RHI::SingleDeviceImage* sourceImage = context.GetImage(copySource.GetAttachment()->GetAttachmentId());
            copyDesc.m_sourceImage = sourceImage;
            copyDesc.m_sourceSize = sourceImage->GetDescriptor().m_size;
            copyDesc.m_sourceOrigin = m_data.m_imageSourceOrigin;
            copyDesc.m_sourceSubresource = m_data.m_imageSourceSubresource;

            // Destination Buffer
            PassAttachmentBinding& copyDest = GetOutputBinding(0);
            copyDesc.m_destinationBuffer = context.GetBuffer(copyDest.GetAttachment()->GetAttachmentId());
            copyDesc.m_destinationOffset = m_data.m_bufferDestinationOffset;
            copyDesc.m_destinationBytesPerRow = m_data.m_bufferDestinationBytesPerRow;
            copyDesc.m_destinationBytesPerImage = m_data.m_bufferDestinationBytesPerImage;

            m_copyItem = copyDesc;
        }

    }   // namespace RPI
}   // namespace AZ
