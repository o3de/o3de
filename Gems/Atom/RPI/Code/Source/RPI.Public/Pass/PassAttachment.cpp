/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <math.h>

#include <AzFramework/StringFunc/StringFunc.h>

#include <Atom/RHI.Reflect/Bits.h>
#include <Atom/RHI/RHIUtils.h>


#include <Atom/RPI.Public/Pass/PassAttachment.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Reflect/Pass/PassName.h>

namespace AZ
{
    namespace RPI
    {
        // --- PassAttachment ---

        PassAttachment::PassAttachment(const PassImageAttachmentDesc& attachmentDesc)
        {
            m_name = attachmentDesc.m_name;
            m_lifetime = attachmentDesc.m_lifetime;
            m_generateFullMipChain = attachmentDesc.m_generateFullMipChain;

            m_descriptor = RHI::UnifiedAttachmentDescriptor(attachmentDesc.m_imageDescriptor, attachmentDesc.m_imageViewDescriptor);
            ValidateDeviceFormats(attachmentDesc.m_formatFallbacks);
        }

        PassAttachment::PassAttachment(const PassBufferAttachmentDesc& attachmentDesc)
        {
            m_descriptor = RHI::UnifiedAttachmentDescriptor(attachmentDesc.m_bufferDescriptor, attachmentDesc.m_bufferViewDescriptor);
            m_name = attachmentDesc.m_name;
            m_lifetime = attachmentDesc.m_lifetime;
        }

        Ptr<PassAttachment> PassAttachment::Clone() const
        {
            Ptr<PassAttachment> clone = aznew PassAttachment();

            clone->m_name = this->m_name;
            clone->m_descriptor = this->m_descriptor;
            clone->m_lifetime = this->m_lifetime;
            clone->m_formatSource = this->m_formatSource;
            clone->m_multisampleSource = this->m_multisampleSource;
            clone->m_sizeSource = this->m_sizeSource;
            clone->m_sizeMultipliers = this->m_sizeMultipliers;
            clone->m_arraySizeSource = this->m_arraySizeSource;
            clone->m_generateFullMipChain = this->m_generateFullMipChain;

            return clone;
        }

        void PassAttachment::ValidateDeviceFormats(const AZStd::vector<RHI::Format>& formatFallbacks, RHI::FormatCapabilities capabilities)
        {
            if (m_descriptor.m_type == RHI::AttachmentType::Image)
            {
                RHI::Format format = m_descriptor.m_image.m_format;
                capabilities |= RHI::FormatCapabilities::Sample;
                AZStd::string formatLocation = AZStd::string::format("PassAttachment [%s]", m_name.GetCStr());
                m_descriptor.m_image.m_format = RHI::ValidateFormat(format, formatLocation.c_str(), formatFallbacks);
            }
        }

        RHI::AttachmentId PassAttachment::GetAttachmentId() const
        {
            AZ_Warning(
                "PassSystem",
                !m_path.IsEmpty(),
                "PassAttachment::GetAttachmentId(): Trying to get AttachmentId without a valid path. Make sure you call ComputePathName.");
            return m_path;
        }

        RHI::AttachmentType PassAttachment::GetAttachmentType() const
        {
            return m_descriptor.m_type;
        }

        void PassAttachment::ComputePathName(const Name& passPath)
        {
            m_path = RHI::AttachmentId(ConcatPassString(passPath, m_name));
        }

        const RHI::TransientImageDescriptor PassAttachment::GetTransientImageDescriptor() const
        {
            AZ_Assert(
                m_lifetime == RHI::AttachmentLifetimeType::Transient,
                "Error, building a transient image descriptor from non-transient pass attachment with path: %s",
                m_path.GetCStr());

            AZ_Assert(
                m_descriptor.m_type == RHI::AttachmentType::Image,
                "Error, building a transient image descriptor for an attachment that is not an image: %s",
                m_path.GetCStr());

            return RHI::TransientImageDescriptor(GetAttachmentId(), m_descriptor.m_image);
        }

        const RHI::TransientBufferDescriptor PassAttachment::GetTransientBufferDescriptor() const
        {
            AZ_Assert(
                m_lifetime == RHI::AttachmentLifetimeType::Transient,
                "Error, building a transient image descriptor from non-transient pass attachment with path: %s",
                m_path.GetCStr());

            AZ_Assert(
                m_descriptor.m_type == RHI::AttachmentType::Buffer,
                "Error, building a transient buffer descriptor for an attachment that is not a buffer: %s",
                m_path.GetCStr());

            return RHI::TransientBufferDescriptor(GetAttachmentId(), m_descriptor.m_buffer);
        }

        void PassAttachment::Update(bool updateImportedAttachments)
        {
            if (m_descriptor.m_type == RHI::AttachmentType::Image &&
                (m_lifetime == RHI::AttachmentLifetimeType::Transient || updateImportedAttachments == true))
            {
                UpdateImageFormat();
                UpdateImageMultisampleState();
                UpdateImageSize();
                UpdateImageArraySize();

                if (m_generateFullMipChain)
                {
                    uint32_t width = m_descriptor.m_image.m_size.m_width;
                    uint32_t height = m_descriptor.m_image.m_size.m_height;

                    double maxDimension = static_cast<double>(AZStd::max(width, height));
                    double mipMapLevels = floor(log2(maxDimension)) + 1;
                    m_descriptor.m_image.m_mipLevels = static_cast<uint16_t>(mipMapLevels);
                }
            }
        }

        void PassAttachment::OnAttached(const PassAttachmentBinding& binding)
        {
            // Auto-infer image and buffer bind flags...
            if (GetAttachmentType() == RHI::AttachmentType::Image)
            {
                m_descriptor.m_image.m_bindFlags |= RHI::GetImageBindFlags(binding.m_scopeAttachmentUsage, binding.GetAttachmentAccess());
            }
            else if (GetAttachmentType() == RHI::AttachmentType::Buffer)
            {
                bool isInputAssembly = RHI::CheckBitsAny(
                    m_descriptor.m_buffer.m_bindFlags, RHI::BufferBindFlags::InputAssembly | RHI::BufferBindFlags::DynamicInputAssembly);
                bool isConstant = RHI::CheckBitsAny(m_descriptor.m_buffer.m_bindFlags, RHI::BufferBindFlags::Constant);

                // Since InputAssembly and Constant cannot be inferred they are set manually. If those flags are set we don't want to add
                // inferred flags on top as it may have a performance penalty
                if (!isInputAssembly && !isConstant)
                {
                    m_descriptor.m_buffer.m_bindFlags |=
                        RHI::GetBufferBindFlags(binding.m_scopeAttachmentUsage, binding.GetAttachmentAccess());
                }
            }
        }

        void PassAttachment::UpdateImageFormat()
        {
            if (m_updatingImageFormat)
            {
                AZ_Assert(false, "PassAttachment::UpdateImageFormat: Error: Circular reference detected");
                return;
            }
            m_updatingImageFormat = true;
            if (m_getFormatFromPipeline && m_renderPipelineSource)
            {
                m_descriptor.m_image.m_format = m_renderPipelineSource->GetRenderSettings().m_format;
            }
            else if (m_formatSource)
            {
                auto& refAttachment = m_formatSource->GetAttachment();
                if (refAttachment && refAttachment->m_descriptor.m_type == RHI::AttachmentType::Image)
                {
                    refAttachment->UpdateImageFormat();
                    m_descriptor.m_image.m_format = refAttachment->m_descriptor.m_image.m_format;
                }
            }
            m_updatingImageFormat = false;
        }

        void PassAttachment::UpdateImageMultisampleState()
        {
            if (m_updatingMultisampleState)
            {
                AZ_Assert(false, "PassAttachment::UpdateMultisampleState: Error: Circular reference detected");
                return;
            }
            m_updatingMultisampleState = true;
            if (m_getMultisampleStateFromPipeline && m_renderPipelineSource)
            {
                m_descriptor.m_image.m_multisampleState = m_renderPipelineSource->GetRenderSettings().m_multisampleState;
            }
            else if (m_multisampleSource)
            {
                auto& refAttachment = m_multisampleSource->GetAttachment();
                if (refAttachment && refAttachment->m_descriptor.m_type == RHI::AttachmentType::Image)
                {
                    refAttachment->UpdateImageMultisampleState();
                    m_descriptor.m_image.m_multisampleState = refAttachment->m_descriptor.m_image.m_multisampleState;
                }
            }
            m_updatingMultisampleState = false;
        }

        void PassAttachment::UpdateImageSize()
        {
            if (m_updatingSize)
            {
                AZ_Assert(false, "PassAttachment::UpdateImageSize: Error: Circular reference detected");
                return;
            }
            m_updatingSize = true;
            if (m_getSizeFromPipeline && m_renderPipelineSource)
            {
                m_descriptor.m_image.m_size = m_renderPipelineSource->GetRenderSettings().m_size;
            }
            else if (m_sizeSource)
            {
                auto& refAttachment = m_sizeSource->GetAttachment();
                if (refAttachment && refAttachment->m_descriptor.m_type == RHI::AttachmentType::Image)
                {
                    refAttachment->UpdateImageSize();
                    RHI::Size sourceSize = refAttachment->m_descriptor.m_image.m_size;
                    m_descriptor.m_image.m_size = m_sizeMultipliers.ApplyModifiers(sourceSize);
                }
            }
            m_updatingSize = false;
        }

        void PassAttachment::UpdateImageArraySize()
        {
            if (m_updatingArraySize)
            {
                AZ_Assert(false, "PassAttachment::UpdateImageArraySize: Error: Circular reference detected");
                return;
            }
            m_updatingArraySize = true;
            if (m_arraySizeSource)
            {
                auto& refAttachment = m_arraySizeSource->GetAttachment();
                if (refAttachment && refAttachment->m_descriptor.m_type == RHI::AttachmentType::Image)
                {
                    refAttachment->UpdateImageArraySize();
                    m_descriptor.m_image.m_arraySize = refAttachment->m_descriptor.m_image.m_arraySize;
                }
            }
            m_updatingArraySize = false;
        }

        // --- PassBinding ---

        PassAttachmentBinding::PassAttachmentBinding(const PassSlot& slot)
        {
            m_name = slot.m_name;
            m_shaderInputName = slot.m_shaderInputName;
            m_shaderImageDimensionsNameIndex = slot.m_shaderImageDimensionsName;
            m_shaderInputArrayIndex = slot.m_shaderInputArrayIndex;
            m_slotType = slot.m_slotType;
            m_scopeAttachmentUsage = slot.m_scopeAttachmentUsage;
            m_scopeAttachmentStage = slot.m_scopeAttachmentStage;

            m_unifiedScopeDesc.m_loadStoreAction = slot.m_loadStoreAction;
            if (slot.m_imageViewDesc != nullptr)
            {
                m_unifiedScopeDesc.SetAsImage(*slot.m_imageViewDesc);
            }
            else if (slot.m_bufferViewDesc != nullptr)
            {
                m_unifiedScopeDesc.SetAsBuffer(*slot.m_bufferViewDesc);
            }

            ValidateDeviceFormats(slot.m_formatFallbacks);
        }

        void PassAttachmentBinding::ValidateDeviceFormats(const AZStd::vector<RHI::Format>& formatFallbacks)
        {
            RHI::FormatCapabilities capabilities =
                RHI::GetCapabilities(m_scopeAttachmentUsage, GetAttachmentAccess(), m_unifiedScopeDesc.GetType());

            if (m_unifiedScopeDesc.GetType() == RHI::AttachmentType::Buffer)
            {
                RHI::BufferViewDescriptor& bufferViewDesc = m_unifiedScopeDesc.GetBufferViewDescriptor();
                RHI::Format format = bufferViewDesc.m_elementFormat;
                AZStd::string formatLocation =
                    AZStd::string::format("BufferViewDescriptor on PassAttachmentBinding [%s]", m_name.GetCStr());
                bufferViewDesc.m_elementFormat = RHI::ValidateFormat(format, formatLocation.c_str(), formatFallbacks, capabilities);
            }
            else if (m_unifiedScopeDesc.GetType() == RHI::AttachmentType::Image)
            {
                RHI::ImageViewDescriptor& imageViewDesc = m_unifiedScopeDesc.GetImageViewDescriptor();
                RHI::Format format = imageViewDesc.m_overrideFormat;
                AZStd::string formatLocation = AZStd::string::format("ImageViewDescriptor on PassAttachmentBinding [%s]", m_name.GetCStr());
                imageViewDesc.m_overrideFormat = RHI::ValidateFormat(format, formatLocation.c_str(), formatFallbacks, capabilities);
            }
        }

        RHI::ScopeAttachmentAccess PassAttachmentBinding::GetAttachmentAccess() const
        {
            RHI::ScopeAttachmentAccess access = RPI::GetAttachmentAccess(m_slotType);
            access = AdjustAccessBasedOnUsage(access, m_scopeAttachmentUsage);
            return access;
        }

        void PassAttachmentBinding::SetOriginalAttachment(Ptr<PassAttachment>& attachment)
        {
            m_originalAttachment = attachment;
            SetAttachment(attachment);
        }

        void PassAttachmentBinding::SetAttachment(const Ptr<PassAttachment>& attachment)
        {
            m_attachment = attachment;
            m_unifiedScopeDesc.m_attachmentId = attachment->GetAttachmentId();

            // setup scope descriptors for transient attachments if they weren't set in slot
            if (m_unifiedScopeDesc.GetType() == RHI::AttachmentType::Uninitialized)
            {
                if (attachment->m_lifetime == RHI::AttachmentLifetimeType::Transient)
                {
                    if (attachment->GetAttachmentType() == RHI::AttachmentType::Buffer)
                    {
                        m_unifiedScopeDesc.SetAsBuffer(attachment->m_descriptor.m_bufferView);
                    }
                    else if (attachment->GetAttachmentType() == RHI::AttachmentType::Image)
                    {
                        m_unifiedScopeDesc.SetAsImage(attachment->m_descriptor.m_imageView);
                    }
                }
                else if (attachment->m_lifetime == RHI::AttachmentLifetimeType::Imported)
                {
                    if (attachment->m_importedResource)
                    {
                        if (attachment->GetAttachmentType() == RHI::AttachmentType::Buffer)
                        {
                            Buffer* buffer = static_cast<Buffer*>(attachment->m_importedResource.get());
                            m_unifiedScopeDesc.SetAsBuffer(buffer->GetBufferViewDescriptor());
                        }
                        else if (attachment->GetAttachmentType() == RHI::AttachmentType::Image)
                        {
                            AttachmentImage* image = static_cast<AttachmentImage*>(attachment->m_importedResource.get());
                            m_unifiedScopeDesc.SetAsImage(image->GetImageView()->GetDescriptor());
                        }
                    }
                    else
                    {
                        AZ_Assert(false, "Imported pass attachment should have the m_importedResource set");
                    }
                }
            }

            RHI::FormatCapabilities capabilities =
                RHI::GetCapabilities(m_scopeAttachmentUsage, GetAttachmentAccess(), m_unifiedScopeDesc.GetType());
            m_attachment->ValidateDeviceFormats(AZStd::vector<RHI::Format>(), capabilities);
            m_attachment->OnAttached(*this);

            AZ_Error(
                "PassSystem",
                m_unifiedScopeDesc.GetType() == attachment->GetAttachmentType(),
                "Attachment must have same type as unified scope descriptor");
        }

        void PassAttachmentBinding::UpdateConnection(bool useFallback)
        {
            Ptr<PassAttachment> targetAttachment = nullptr;

            // Use the fallback binding if:
            // - the calling pass specifies to use it
            // - fallback binding is setup
            // - the slot is an output  (input/output slots act as their own fallback and having fallback for an input makes no sense)
            if (useFallback && m_fallbackBinding && m_slotType == PassSlotType::Output)
            {
                targetAttachment = m_fallbackBinding->m_attachment;
            }
            else if (m_connectedBinding)
            {
                targetAttachment = m_connectedBinding->m_attachment;
            }
            else if (m_originalAttachment != nullptr)
            {
                targetAttachment = m_originalAttachment;
            }

            if (targetAttachment == nullptr ||
                (targetAttachment == m_attachment && m_attachment->GetAttachmentId() == m_unifiedScopeDesc.m_attachmentId))
            {
                return;
            }

            SetAttachment(targetAttachment);
        }

    } // namespace RPI
} // namespace AZ
