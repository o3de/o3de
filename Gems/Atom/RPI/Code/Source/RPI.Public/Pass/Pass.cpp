/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/RTTI/RTTI.h>
#include <AzCore/std/string/conversions.h>

#include <AtomCore/Instance/InstanceDatabase.h>
#include <AtomCore/std/containers/vector_set.h>

#include <Atom/RHI/FrameGraphAttachmentInterface.h>
#include <Atom/RHI/FrameGraphBuilder.h>
#include <Atom/RHI/RHIUtils.h>
#include <Atom/RHI.Reflect/Base.h>

#include <Atom/RPI.Public/Buffer/Buffer.h>
#include <Atom/RPI.Public/Image/AttachmentImage.h>
#include <Atom/RPI.Public/Image/AttachmentImagePool.h>
#include <Atom/RPI.Public/Image/ImageSystemInterface.h>
#include <Atom/RPI.Public/Pass/AttachmentReadback.h>
#include <Atom/RPI.Public/Pass/ParentPass.h>
#include <Atom/RPI.Public/Pass/Pass.h>
#include <Atom/RPI.Public/Pass/PassDefines.h>
#include <Atom/RPI.Public/Pass/PassLibrary.h>
#include <Atom/RPI.Public/Pass/PassSystemInterface.h>
#include <Atom/RPI.Public/Pass/PassUtils.h>
#include <Atom/RPI.Public/Pass/Specific/ImageAttachmentPreviewPass.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Reflect/Image/Image.h>

#include <Atom/RPI.Reflect/Image/AttachmentImageAsset.h>
#include <Atom/RPI.Reflect/Pass/PassRequest.h>
#include <Atom/RPI.Reflect/Pass/PassTemplate.h>
#include <Atom/RPI.Reflect/Pass/PassName.h>
#include <Atom/RPI.Reflect/Asset/AssetUtils.h>

namespace AZ
{
    namespace RPI
    {             
        // --- Constructors ---

        Pass::Pass(const PassDescriptor& descriptor)
            : m_name(descriptor.m_passName)
            , m_path(descriptor.m_passName)
        {
            AZ_RPI_PASS_ASSERT((descriptor.m_passRequest == nullptr) || (descriptor.m_passTemplate != nullptr),
                "Pass::Pass - request is valid but template is nullptr. This is not allowed. Passing a valid passRequest also requires a valid passTemplate.");

            m_passData = PassUtils::GetPassDataPtr(descriptor);
            if (m_passData)
            {
                PassUtils::ExtractPipelineGlobalConnections(m_passData, m_pipelineGlobalConnections);
                m_viewTag = m_passData->m_pipelineViewTag;
                if (m_passData->m_deviceIndex >= 0 && m_passData->m_deviceIndex < RHI::RHISystemInterface::Get()->GetDeviceCount())
                {
                    m_deviceIndex = m_passData->m_deviceIndex;
                }
            }

            m_flags.m_enabled = true;
            m_flags.m_timestampQueryEnabled = false;
            m_flags.m_pipelineStatisticsQueryEnabled = false;
            m_flags.m_parentDeviceIndexCached = false;

            m_template = descriptor.m_passTemplate;
            if (m_template)
            {
                m_defaultShaderAttachmentStage = m_template->m_defaultShaderAttachmentStage;
            }

            if (descriptor.m_passRequest != nullptr)
            {
                // Assert m_template is the same as the one in the pass request
                if (PassValidation::IsEnabled())
                {
                    const AZStd::shared_ptr<const PassTemplate> passTemplate = PassSystemInterface::Get()->GetPassTemplate(descriptor.m_passRequest->m_templateName);
                    AZ_RPI_PASS_ASSERT(m_template == passTemplate, "Error: template in PassDescriptor doesn't match template from PassRequest!");
                }

                m_request = *descriptor.m_passRequest;
                m_flags.m_createdByPassRequest = true;
                m_flags.m_enabled = m_request.m_passEnabled;
            }

            PassSystemInterface::Get()->RegisterPass(this);
            QueueForBuildAndInitialization();

            // Skip reset since the pass just got created
            m_state = PassState::Reset;
            m_flags.m_lastFrameEnabled = m_flags.m_enabled;
        }

        Pass::~Pass()
        {
            AZ_RPI_BREAK_ON_TARGET_PASS;
            PassSystemInterface::Get()->UnregisterPass(this);
        }

        PassDescriptor Pass::GetPassDescriptor() const
        {
            PassDescriptor desc;
            desc.m_passName = m_name;
            desc.m_passTemplate = m_template ? PassSystemInterface::Get()->GetPassTemplate(m_template->m_name) : nullptr;
            if (m_flags.m_createdByPassRequest)
            {
                desc.m_passRequest = AZStd::make_shared<PassRequest>(m_request);
            }
            else
            {
                desc.m_passRequest.reset();
            }
            
            desc.m_passData = m_passData;
            return desc;
        }

        int Pass::GetDeviceIndex() const
        {
            if (m_deviceIndex == AZ::RHI::MultiDevice::InvalidDeviceIndex && m_parent)
            {
                return m_flags.m_parentDeviceIndexCached ? m_parentDeviceIndex : m_parent->GetDeviceIndex();
            }
            return m_deviceIndex;
        }

        bool Pass::SetDeviceIndex(int deviceIndex)
        {
            const int deviceCount = AZ::RHI::RHISystemInterface::Get()->GetDeviceCount();
            if (deviceIndex < AZ::RHI::MultiDevice::InvalidDeviceIndex || deviceIndex >= deviceCount)
            {
                AZ_Error("Pass", false, "Device index should be at least -1(RHI::MultiDevice::InvalidDeviceIndex) and less than current device count %d.", deviceCount);
                return false;
            }

            m_deviceIndex = deviceIndex;
            OnHierarchyChange();
            return true;
        }

        void Pass::SetEnabled(bool enabled)
        {
            if (enabled != m_flags.m_enabled)
            {
                m_flags.m_enabled = enabled;
                OnHierarchyChange();
            }
        }

        // --- Error Logging ---

        void Pass::LogError(AZStd::string&& message)
        {
#if AZ_RPI_ENABLE_PASS_DEBUGGING
            AZ::Debug::Trace::Instance().Break();
#endif

            if (PassValidation::IsEnabled())
            {
                ++m_errors;
                if (m_errorMessages.size() < MessageLogLimit)
                {
                    m_errorMessages.push_back(AZStd::move(message));
                }
            }
        }

        void Pass::LogWarning(AZStd::string&& message)
        {
            if (PassValidation::IsEnabled())
            {
                ++m_warnings;
                if (m_warningMessages.size() < MessageLogLimit)
                {
                    m_warningMessages.push_back(AZStd::move(message));
                }
            }
        }

        // --- Hierarchy functions ---

        void Pass::OnHierarchyChange()
        {
            if (m_parent != nullptr)
            {
                // Set new tree depth and path
                m_flags.m_parentEnabled = m_parent->m_flags.m_enabled && (m_parent->m_flags.m_parentEnabled || m_parent->m_parent == nullptr);
                m_treeDepth = m_parent->m_treeDepth + 1;
                m_path = ConcatPassName(m_parent->m_path, m_name);
                m_flags.m_partOfHierarchy = m_parent->m_flags.m_partOfHierarchy;

                m_parentDeviceIndex = m_parent->GetDeviceIndex();
                m_flags.m_parentDeviceIndexCached = true;

                if (m_state == PassState::Orphaned)
                {
                    QueueForBuildAndInitialization();
                }
                OnDescendantChange(PassDescendantChangeFlags::Hierarchy);
            }
            AZ_RPI_BREAK_ON_TARGET_PASS;
        }

        void Pass::RemoveFromParent()
        {
            AZ_RPI_BREAK_ON_TARGET_PASS;
            AZ_RPI_PASS_ASSERT(m_parent != nullptr, "Trying to remove pass from parent but pointer to the parent pass is null.");
            m_parent->RemoveChild(Ptr<Pass>(this));
            m_queueState = PassQueueState::NoQueue;
            m_state = PassState::Orphaned;
        }

        void Pass::OnDescendantChange(PassDescendantChangeFlags flags)
        {
            if (m_parent)
            {
                m_parent->OnDescendantChange(flags);
            }
        }

        void Pass::OnOrphan()
        {
            AZ_RPI_BREAK_ON_TARGET_PASS;
            if (m_flags.m_containsGlobalReference && m_pipeline != nullptr)
            {
                m_pipeline->RemovePipelineGlobalConnectionsFromPass(this);
            }

            OnDescendantChange(PassDescendantChangeFlags::Hierarchy);
            m_parent = nullptr;
            m_flags.m_partOfHierarchy = false;
            m_treeDepth = 0;
            m_parentChildIndex = 0;
            m_queueState = PassQueueState::NoQueue;
            m_state = PassState::Orphaned;
        }

        ParentPass* Pass::AsParent()
        {
            return azrtti_cast<ParentPass*>(this);
        }

        const ParentPass* Pass::AsParent() const
        {
            return azrtti_cast<const ParentPass*>(this);
        }

        // --- Bindings ---

        PassAttachmentBinding& Pass::GetInputBinding(uint32_t index)
        {
            uint32_t bindingIndex = m_inputBindingIndices[index];
            return m_attachmentBindings[bindingIndex];
        }

        PassAttachmentBinding& Pass::GetInputOutputBinding(uint32_t index)
        {
            uint32_t bindingIndex = m_inputOutputBindingIndices[index];
            return m_attachmentBindings[bindingIndex];
        }

        PassAttachmentBinding& Pass::GetOutputBinding(uint32_t index)
        {
            uint32_t bindingIndex = m_outputBindingIndices[index];
            return m_attachmentBindings[bindingIndex];
        }

        void Pass::AddAttachmentBinding(PassAttachmentBinding attachmentBinding)
        {
            auto index = static_cast<uint8_t>(m_attachmentBindings.size());
            if (attachmentBinding.m_scopeAttachmentStage == RHI::ScopeAttachmentStage::Uninitialized)
            {
                attachmentBinding.m_scopeAttachmentStage = attachmentBinding.m_scopeAttachmentUsage == RHI::ScopeAttachmentUsage::Shader
                    ? m_defaultShaderAttachmentStage
                    : RHI::ScopeAttachmentStage::Any;
            }

            // Add the binding. This will assert if the fixed size array is full.
            m_attachmentBindings.push_back(attachmentBinding);

            // Add the index of the binding to the input, output or input/output list based on the slot type
            switch (attachmentBinding.m_slotType)
            {
            case PassSlotType::Input:
                m_inputBindingIndices.push_back(index);
                break;
            case PassSlotType::InputOutput:
                m_inputOutputBindingIndices.push_back(index);
                break;
            case PassSlotType::Output:
                m_outputBindingIndices.push_back(index);
                break;
            default:
                break;
            }
        }

        // --- Finders ---

        Ptr<Pass> Pass::FindAdjacentPass(const Name& passName)
        {
            // 1. Check This
            if (passName == PassNameThis)
            {
                return Ptr<Pass>(this);
            }

            // 2. Check Parent
            if (m_parent == nullptr)
            {
                return nullptr;
            }
            if (passName == PassNameParent || passName == m_parent->GetName())
            {
                return Ptr<Pass>(m_parent);
            }

            // 3. Check Siblings
            Ptr<Pass> foundPass = m_parent->FindChildPass(passName);

            // 4. Check Children
            if (!foundPass && AsParent())
            {
                foundPass = AsParent()->FindChildPass(passName);
            }

            // Finished search, return
            return foundPass;
        }

        PassAttachmentBinding* Pass::FindAttachmentBinding(const Name& slotName)
        {
            for (PassAttachmentBinding& binding : m_attachmentBindings)
            {
                if (slotName == binding.m_name)
                {
                    return &binding;
                }
            }
            return nullptr;
        }

        const PassAttachmentBinding* Pass::FindAttachmentBinding(const Name& slotName) const
        {
            for (const PassAttachmentBinding& binding : m_attachmentBindings)
            {
                if (slotName == binding.m_name)
                {
                    return &binding;
                }
            }
            return nullptr;
        }

        Ptr<PassAttachment> Pass::FindOwnedAttachment(const Name& attachmentName) const
        {
            for (const Ptr<PassAttachment>& attachment : m_ownedAttachments)
            {
                if (attachment->m_name == attachmentName)
                {
                    return attachment;
                }
            }

            return nullptr;
        }

        Ptr<PassAttachment> Pass::FindAttachment(const Name& slotName) const
        {
            if (const PassAttachmentBinding* binding = FindAttachmentBinding(slotName))
            {
                return binding->GetAttachment();
            }

            return FindOwnedAttachment(slotName);
        }

        const PassAttachmentBinding* Pass::FindAdjacentBinding(const PassAttachmentRef& attachmentRef, [[maybe_unused]] const char* attachmentSourceTypeDebugName)
        {
            const PassAttachmentBinding* result = nullptr;

            if (attachmentRef.m_pass.IsEmpty() && attachmentRef.m_attachment.IsEmpty())
            {
                // The data isn't actually referencing anything, so this is not an error, just return null.
                return result;
            }

            if (attachmentRef.m_pass.IsEmpty() != attachmentRef.m_attachment.IsEmpty())
            {
                AZ_Error("Pass", false,
                    "Invalid attachment reference (Pass [%s], Attachment [%s]). Both Pass and Attachment must be set.",
                    attachmentRef.m_pass.GetCStr(), attachmentRef.m_attachment.GetCStr());

                return result;
            }

            // Find pass
            if (Ptr<Pass> pass = FindAdjacentPass(attachmentRef.m_pass))
            {
                // Find attachment within pass
                result = pass->FindAttachmentBinding(attachmentRef.m_attachment);
            }

            AZ_Error("Pass", result, "Pass [%s] could not find %s (Pass [%s], Attachment [%s])",
                m_path.GetCStr(), attachmentSourceTypeDebugName, attachmentRef.m_pass.GetCStr(), attachmentRef.m_attachment.GetCStr());

            return result;
        }

        // --- PassTemplate related functions ---

        void Pass::CreateBindingsFromTemplate()
        {
            if (m_template)
            {
                for (const PassSlot& slot : m_template->m_slots)
                {
                    PassAttachmentBinding binding(slot);
                    AddAttachmentBinding(binding);
                }
            }
        }

        void Pass::AttachBufferToSlot(AZStd::string_view slot, Data::Instance<Buffer> buffer)
        {
            AttachBufferToSlot(Name(slot), buffer);
        }

        void Pass::AttachBufferToSlot(const Name& slot, Data::Instance<Buffer> buffer)
        {
            if (!buffer)
            {
                return;
            }

            PassAttachmentBinding* localBinding = FindAttachmentBinding(slot);
            if (!localBinding)
            {
                AZ_RPI_PASS_ERROR(false, "Pass::AttachBufferToSlot - Pass [%s] failed to find slot [%s].",
                    m_path.GetCStr(), slot.GetCStr());
                return;
            }

            // We can't handle the case that there is already an attachment attached yet.
            // We could consider to add it later if there are needs. It may require remove from the owned attachment list and
            // handle the connected bindings
            if (localBinding->GetAttachment())
            {
                AZ_RPI_PASS_ERROR(false, "Pass::AttachBufferToSlot - Slot [%s] already has attachment [%s].",
                    slot.GetCStr(), localBinding->GetAttachment()->m_name.GetCStr());
                return;
            }

            PassBufferAttachmentDesc desc;
            desc.m_bufferDescriptor = buffer->GetRHIBuffer()->GetDescriptor();
            desc.m_lifetime = RHI::AttachmentLifetimeType::Imported;
            desc.m_name = buffer->GetAttachmentId();
            Ptr<PassAttachment> attachment = CreateAttachmentFromDesc(desc);
            attachment->m_importedResource = buffer;
            m_ownedAttachments.push_back(attachment);

            localBinding->SetOriginalAttachment(attachment);
        }
        
        void Pass::AttachImageToSlot(const Name& slot, Data::Instance<AttachmentImage> image)
        {
            PassAttachmentBinding* localBinding = FindAttachmentBinding(slot);
            if (!localBinding)
            {
                AZ_RPI_PASS_ERROR(false, "Pass::AttachImageToSlot - Pass [%s] failed to find slot [%s].",
                    m_path.GetCStr(), slot.GetCStr());
                return;
            }

            // We can't handle the case that there is already an attachment attached yet.
            // We could consider to add it later if there are needs. It may require remove from the owned attachment list and
            // handle the connected bindings
            if (localBinding->GetAttachment())
            {
                AZ_RPI_PASS_ERROR(false, "Pass::AttachImageToSlot - Slot [%s] already has attachment [%s].",
                    slot.GetCStr(), localBinding->GetAttachment()->m_name.GetCStr());
                return;
            }

            PassImageAttachmentDesc desc;
            desc.m_imageDescriptor = image->GetRHIImage()->GetDescriptor();
            desc.m_lifetime = RHI::AttachmentLifetimeType::Imported;
            desc.m_name = image->GetAttachmentId();
            Ptr<PassAttachment> attachment = CreateAttachmentFromDesc(desc);
            attachment->m_importedResource = image;
            m_ownedAttachments.push_back(attachment);

            localBinding->SetOriginalAttachment(attachment);
        }               

        void Pass::ProcessConnection(const PassConnection& connection, uint32_t slotTypeMask)
        {
            [[maybe_unused]] auto prefix = [&]() -> AZStd::string
            {
                return AZStd::string::format(
                    "Pass::ProcessConnection %s:%s -> %s:%s",
                    m_path.GetCStr(),
                    connection.m_localSlot.GetCStr(),
                    connection.m_attachmentRef.m_pass.GetCStr(),
                    connection.m_attachmentRef.m_attachment.GetCStr());
            };

            // -- Find Local Binding --

            // Get the input from this pass that forms one end of the connection
            PassAttachmentBinding* localBinding = FindAttachmentBinding(connection.m_localSlot);
            if (!localBinding)
            {
                AZ_RPI_PASS_ERROR(false, "%s: failed to find Local Slot.", prefix().c_str());
                return;
            }

            // Slot type mask used to skip connections at various stages of initialization
            uint32_t bindingMask = (1 << uint32_t(localBinding->m_slotType));
            if (!(bindingMask & slotTypeMask))
            {
                return;
            }

            // -- Local Variables --

            Name connectedPassName = connection.m_attachmentRef.m_pass;
            Name connectedSlotName = connection.m_attachmentRef.m_attachment;
            Ptr<PassAttachment> attachment = nullptr;
            PassAttachmentBinding* connectedBinding = nullptr;
            bool foundPass = false;
            bool slotTypeMismatch = false;

            // -- Search This Pass --

            if (connectedPassName == PassNameThis)
            {
                foundPass = true;
                attachment = FindOwnedAttachment(connectedSlotName);

                AZ_RPI_PASS_ERROR(
                    attachment, "%s: Current Pass doesn't own an attachment named [%s].", prefix().c_str(), connectedSlotName.GetCStr());
            }

            // -- Search Pipeline --

            else if (connectedPassName == PipelineGlobalKeyword)
            {
                AZ_RPI_PASS_ERROR(m_pipeline != nullptr, "%s: Pass doesn't have a valid pipeline pointer.", prefix().c_str());

                foundPass = true;   // Using the "Pipeline" keyword, no need to continue searching for passes

                if (m_pipeline)
                {
                    const PipelineGlobalBinding* globalBinding = m_pipeline->GetPipelineGlobalConnection(connectedSlotName);
                    if (globalBinding)
                    {
                        connectedBinding = globalBinding->m_binding;
                    }

                    AZ_RPI_PASS_ERROR(connectedBinding, "%s: Cannot find pipeline global connection.", prefix().c_str());
                }
            }

            // -- Search Parent & Siblings --

            // The (connectedPassName != m_name) avoids edge case where parent pass has child pass of same name.
            // In this case, parent pass would ask it's parent pass for a sibling with the given name and get a pointer to itself.
            // It would then try to connect to itself, which is obviously not the intention of the user
            if (!foundPass && m_parent && connectedPassName != m_name)
            {
                if (connectedPassName == PassNameParent)
                {
                    foundPass = true;
                    connectedBinding = m_parent->FindAttachmentBinding(connectedSlotName);
                    if (!connectedBinding)
                    {
                        attachment = m_parent->FindOwnedAttachment(connectedSlotName);
                    }
                    else
                    {
                        slotTypeMismatch = connectedBinding->m_slotType != localBinding->m_slotType &&
                            connectedBinding->m_slotType != PassSlotType::InputOutput &&
                            localBinding->m_slotType != PassSlotType::InputOutput;
                    }
                }
                else
                {
                    // Use the connection Name to find a sibling pass
                    Ptr<Pass> siblingPass = m_parent->FindChildPass(connectedPassName);
                    if (siblingPass)
                    {
                        foundPass = true;
                        connectedBinding = siblingPass->FindAttachmentBinding(connectedSlotName);

                        slotTypeMismatch = connectedBinding != nullptr &&
                            connectedBinding->m_slotType == localBinding->m_slotType &&
                            connectedBinding->m_slotType != PassSlotType::InputOutput;
                    }
                }
            }

            // -- Search Children --

            ParentPass* asParent = AsParent();
            if (!foundPass && asParent)
            {
                Ptr<Pass> childPass = asParent->FindChildPass(connectedPassName);
                if (childPass)
                {
                    foundPass = true;
                    connectedBinding = childPass->FindAttachmentBinding(connectedSlotName);

                    slotTypeMismatch = connectedBinding != nullptr &&
                        connectedBinding->m_slotType != localBinding->m_slotType &&
                        connectedBinding->m_slotType != PassSlotType::InputOutput &&
                        localBinding->m_slotType != PassSlotType::InputOutput;

                }
            }

            // -- Finalize & Report Errors --

            if (slotTypeMismatch)
            {
                AZ_RPI_PASS_ERROR(
                    false,
                    "%s: Slot Type Mismatch - When connecting to a child slot, both slots must be of the same type, or one must be "
                    "InputOutput.",
                    prefix().c_str());
                connectedBinding = nullptr;
            }

            if (connectedBinding)
            {
                localBinding->m_connectedBinding = connectedBinding;
                UpdateConnectedBinding(*localBinding);

            }
            else if (attachment)
            {
                localBinding->SetOriginalAttachment(attachment);
            }
            else
            {
                if (!m_flags.m_partOfHierarchy)
                {
                    // [GFX TODO][ATOM-13693]: REMOVE POST R1 - passes not in hierarchy should no longer have this function called
                    // When view is changing, removal of the passes can occur (cascade shadow passes for example)
                    // resulting in temporary orphan passes that will be removed over the next frame.
                    AZ_RPI_PASS_WARNING(false, "%s: Pass is no longer part of the hierarchy and about to be removed.", prefix().c_str());
                }
                else if (foundPass)
                {
                    AZ_RPI_PASS_ERROR(false, "%s: Could not find binding on target.", prefix().c_str());
                }
                else
                {
                    AZ_RPI_PASS_ERROR(false, "%s: Could not find target pass.", prefix().c_str());
                }
            }
        }

        void Pass::ProcessFallbackConnection(const PassFallbackConnection& connection)
        {
            PassAttachmentBinding* inputBinding = FindAttachmentBinding(connection.m_inputSlotName);
            PassAttachmentBinding* outputBinding = FindAttachmentBinding(connection.m_outputSlotName);

            [[maybe_unused]] auto prefix = [&]() -> AZStd::string
            {
                return AZStd::string::format(
                    "Pass::ProcessFallbackConnection: %s, %s -> %s",
                    m_path.GetCStr(),
                    connection.m_inputSlotName.GetCStr(),
                    connection.m_outputSlotName.GetCStr());
            };
            if (!outputBinding || !inputBinding)
            {
                AZ_RPI_PASS_ERROR(inputBinding, "%s: failed to find input slot.", prefix().c_str());

                AZ_RPI_PASS_ERROR(outputBinding, "%s: failed to find output slot.", prefix().c_str());

                return;
            }

            bool typesAreValid = inputBinding->m_slotType == PassSlotType::Input && outputBinding->m_slotType == PassSlotType::Output;

            if (!typesAreValid)
            {
                AZ_RPI_PASS_ERROR(
                    inputBinding->m_slotType == PassSlotType::Input, "%s: Input doesn't have SlotType::Input.", prefix().c_str());

                AZ_RPI_PASS_ERROR(
                    outputBinding->m_slotType == PassSlotType::Output, "%s: Output doesn't have SlotType::Output.", prefix().c_str());

                return;
            }

            outputBinding->m_fallbackBinding = inputBinding;
            UpdateConnectedBinding(*outputBinding);
        }

        template<typename AttachmentDescType>
        Ptr<PassAttachment> Pass::CreateAttachmentFromDesc(const AttachmentDescType& desc)
        {
            Ptr<PassAttachment> attachment = aznew PassAttachment(desc);

            // If the attachment is imported, we will create the resource (buffer or image) of this attachment
            // from asset referenced in m_assetRef
            // The resource instance will be saved in m_importedResource and the attachment id is acquired from resource instance
            if (desc.m_lifetime == RHI::AttachmentLifetimeType::Imported)
            {
                attachment->m_path = desc.m_name;
                if (attachment->m_descriptor.m_type == RHI::AttachmentType::Buffer)
                {
                    Data::Asset<BufferAsset> bufferAsset = AssetUtils::LoadAssetById<BufferAsset>(desc.m_assetRef.m_assetId, AssetUtils::TraceLevel::None);

                    if (bufferAsset.IsReady())
                    {
                        Data::Instance<Buffer> buffer = Buffer::FindOrCreate(bufferAsset);
                        if (buffer)
                        {
                            attachment->m_path = buffer->GetAttachmentId();
                            attachment->m_importedResource = buffer;
                            attachment->m_descriptor = buffer->GetRHIBuffer()->GetDescriptor();
                        }
                    }
                }
                else if (attachment->m_descriptor.m_type == RHI::AttachmentType::Image)
                {
                    Data::Asset<AttachmentImageAsset> imageAsset = AssetUtils::LoadAssetById<AttachmentImageAsset>(desc.m_assetRef.m_assetId, AssetUtils::TraceLevel::None);

                    if (imageAsset.IsReady())
                    {
                        Data::Instance<AttachmentImage> image = AttachmentImage::FindOrCreate(imageAsset);
                        if (image)
                        {
                            attachment->m_path = image->GetAttachmentId();
                            attachment->m_importedResource = image;
                            attachment->m_descriptor = image->GetDescriptor();
                        }
                    }
                }
                else
                {
                    AZ_RPI_PASS_ASSERT(false, "Unsupported imported attachment type");
                }
            }
            else
            {
                // Only apply path name to transient attachment. Keep the original name for imported attachment
                attachment->ComputePathName(m_path);
            }

            // Setup attachment sources...

            if (desc.m_sizeSource.m_source.m_pass == PipelineKeyword)         // if source is pipeline
            {
                attachment->m_renderPipelineSource = m_pipeline;
                attachment->m_getSizeFromPipeline = true;
                attachment->m_sizeMultipliers = desc.m_sizeSource.m_multipliers;
            }
            else if (const PassAttachmentBinding* source = FindAdjacentBinding(desc.m_sizeSource.m_source, "SizeSource"))
            {
                attachment->m_sizeSource = source;
                attachment->m_sizeMultipliers = desc.m_sizeSource.m_multipliers;
            }

            if (desc.m_formatSource.m_pass == PipelineKeyword)                // if source is pipeline
            {
                attachment->m_renderPipelineSource = m_pipeline;
                attachment->m_getFormatFromPipeline = true;
            }
            else if (const PassAttachmentBinding* source = FindAdjacentBinding(desc.m_formatSource, "FormatSource"))
            {
                attachment->m_formatSource = source;
            }

            if (desc.m_multisampleSource.m_pass == PipelineKeyword)           // if source is pipeline
            {
                attachment->m_renderPipelineSource = m_pipeline;
                attachment->m_getMultisampleStateFromPipeline = true;
            }
            else if (const PassAttachmentBinding* source = FindAdjacentBinding(desc.m_multisampleSource, "MultisampleSource"))
            {
                attachment->m_multisampleSource = source;
            }

            if (const PassAttachmentBinding* source = FindAdjacentBinding(desc.m_arraySizeSource, "ArraySizeSource"))
            {
                attachment->m_arraySizeSource = source;
            }

            attachment->m_ownerPass = this;
            return attachment;
        }

        template<typename AttachmentDescType>
        void Pass::OverrideOrAddAttachment(const AttachmentDescType& desc)
        {
            bool overrideAttachment = false;

            // Search existing attachments
            for (size_t i = 0; i < m_ownedAttachments.size(); ++i)
            {
                // If we find one with the same name
                if (m_ownedAttachments[i]->m_name == desc.m_name)
                {
                    // Override it
                    m_ownedAttachments[i] = CreateAttachmentFromDesc(desc);
                    overrideAttachment = true;
                    break;
                }
            }

            // If we didn't override any attachments
            if (!overrideAttachment)
            {
                // Create a new one
                m_ownedAttachments.emplace_back(CreateAttachmentFromDesc(desc));
            }
        }

        void Pass::SetupInputsFromRequest()
        {
            if (m_flags.m_createdByPassRequest)
            {
                const uint32_t slotTypeMask = (1 << uint32_t(PassSlotType::Input)) | (1 << uint32_t(PassSlotType::InputOutput));
                for (const PassConnection& connection : m_request.m_connections)
                {
                    ProcessConnection(connection, slotTypeMask);
                }
            }
        }

        void Pass::SetupOutputsFromRequest()
        {
            if (m_flags.m_createdByPassRequest)
            {
                const uint32_t slotTypeMask = (1 << uint32_t(PassSlotType::Output));
                for (const PassConnection& connection : m_request.m_connections)
                {
                    ProcessConnection(connection, slotTypeMask);
                }
            }
        }

        void Pass::SetupPassDependencies()
        {
            // Get dependencies declared in the PassRequest
            if (m_flags.m_createdByPassRequest)
            {
                for (const Name& passName : m_request.m_executeAfterPasses)
                {
                    Ptr<Pass> executeAfterPass = FindAdjacentPass(passName);
                    if (executeAfterPass != nullptr)
                    {
                        m_executeAfterPasses.push_back(executeAfterPass.get());
                    }
                }
                for (const Name& passName : m_request.m_executeBeforePasses)
                {
                    Ptr<Pass> executeBeforePass = FindAdjacentPass(passName);
                    if (executeBeforePass != nullptr)
                    {
                        m_executeBeforePasses.push_back(executeBeforePass.get());
                    }
                }
            }
            // Inherit dependencies from ParentPass
            if (m_parent)
            {
                for (Pass* pass : m_parent->m_executeAfterPasses)
                {
                    m_executeAfterPasses.push_back(pass);
                }
                for (Pass* pass : m_parent->m_executeBeforePasses)
                {
                    m_executeBeforePasses.push_back(pass);
                }
            }
        }

        void Pass::DeclareAttachmentsToFrameGraph(
            RHI::FrameGraphInterface frameGraph, PassSlotType slotType, RHI::ScopeAttachmentAccess accessMask) const
        {
            for (size_t slotIndex = 0; slotIndex < m_attachmentBindings.size(); ++slotIndex)
            {
                const auto& attachmentBinding = m_attachmentBindings[slotIndex];
                if(slotType == PassSlotType::Uninitialized || slotType == attachmentBinding.m_slotType)
                {
                    if (attachmentBinding.GetAttachment() != nullptr &&
                        frameGraph.GetAttachmentDatabase().IsAttachmentValid(attachmentBinding.GetAttachment()->GetAttachmentId()))
                    {
                        switch (attachmentBinding.m_unifiedScopeDesc.GetType())
                        {
                        case RHI::AttachmentType::Image:
                            {
                                AZ::RHI::ImageScopeAttachmentDescriptor imageScopeAttachmentDescriptor =
                                    attachmentBinding.m_unifiedScopeDesc.GetAsImage();
                                if (attachmentBinding.m_scopeAttachmentUsage == RHI::ScopeAttachmentUsage::SubpassInput)
                                {
                                    frameGraph.UseSubpassInputAttachment(
                                        imageScopeAttachmentDescriptor, attachmentBinding.m_scopeAttachmentStage);
                                }
                                else if (attachmentBinding.m_scopeAttachmentUsage == RHI::ScopeAttachmentUsage::Resolve)
                                {
                                    // A Resolve attachment must be declared immediately after the RenderTarget it is supposed to resolve.
                                    // This particular requirement was already validated during BuildSubpassLayout().
                                    const auto& renderTargetBinding = m_attachmentBindings[slotIndex - 1];
                                    RHI::ResolveScopeAttachmentDescriptor resolveDescriptor;
                                    resolveDescriptor.m_attachmentId = attachmentBinding.GetAttachment()->GetAttachmentId();
                                    resolveDescriptor.m_loadStoreAction = attachmentBinding.m_unifiedScopeDesc.m_loadStoreAction;
                                    resolveDescriptor.m_resolveAttachmentId = renderTargetBinding.GetAttachment()->GetAttachmentId();
                                    frameGraph.UseResolveAttachment(resolveDescriptor);
                                }
                                else
                                {
                                    frameGraph.UseAttachment(
                                        imageScopeAttachmentDescriptor,
                                        attachmentBinding.GetAttachmentAccess() & accessMask,
                                        attachmentBinding.m_scopeAttachmentUsage,
                                        attachmentBinding.m_scopeAttachmentStage);
                                }
                                break;
                            }
                        case RHI::AttachmentType::Buffer:
                            {
                                frameGraph.UseAttachment(
                                    attachmentBinding.m_unifiedScopeDesc.GetAsBuffer(),
                                    attachmentBinding.GetAttachmentAccess() & accessMask,
                                    attachmentBinding.m_scopeAttachmentUsage,
                                    attachmentBinding.m_scopeAttachmentStage);
                                break;
                            }
                        default:
                            AZ_Assert(false, "Error, trying to bind an attachment that is neither an image nor a buffer!");
                            break;
                        }
                    }
                }
            }
        }

        void Pass::SetupInputsFromTemplate()
        {
            if (m_template)
            {
                const uint32_t slotTypeMask = (1 << uint32_t(PassSlotType::Input)) | (1 << uint32_t(PassSlotType::InputOutput));
                for (const PassConnection& outputConnection : m_template->m_connections)
                {
                    ProcessConnection(outputConnection, slotTypeMask);
                }
            }
        }

        void Pass::SetupOutputsFromTemplate()
        {
            if (m_template)
            {
                const uint32_t slotTypeMask = (1 << uint32_t(PassSlotType::Output));
                for (const PassConnection& outputConnection : m_template->m_connections)
                {
                    ProcessConnection(outputConnection, slotTypeMask);
                }
                for (const PassFallbackConnection& fallbackConnection : m_template->m_fallbackConnections)
                {
                    ProcessFallbackConnection(fallbackConnection);
                }
            }
        }

        void Pass::CreateAttachmentsFromTemplate()
        {
            if (m_template)
            {
                // Create image attachments
                for (const PassImageAttachmentDesc& desc : m_template->m_imageAttachments)
                {
                    m_ownedAttachments.emplace_back(CreateAttachmentFromDesc(desc));
                }
                // Create buffer attachments
                for (const PassBufferAttachmentDesc& desc : m_template->m_bufferAttachments)
                {
                    m_ownedAttachments.emplace_back(CreateAttachmentFromDesc(desc));
                }
            }
        }

        void Pass::CreateAttachmentsFromRequest()
        {
            if (m_flags.m_createdByPassRequest)
            {
                // Create image attachments
                for (const PassImageAttachmentDesc& desc : m_request.m_imageAttachmentOverrides)
                {
                    OverrideOrAddAttachment(desc);
                }
                // Create buffer attachments
                for (const PassBufferAttachmentDesc& desc : m_request.m_bufferAttachmentOverrides)
                {
                    OverrideOrAddAttachment(desc);
                }
            }
        }

        // --- Attachment and Binding related functions ---

        void Pass::StoreImportedAttachmentReferences()
        {
            m_importedAttachmentStore.clear();

            for (const Ptr<PassAttachment>& attachment : m_ownedAttachments)
            {
                if (attachment->m_lifetime == RHI::AttachmentLifetimeType::Imported)
                {
                    m_importedAttachmentStore.push_back(attachment);
                }
            }
        }

        void Pass::CreateTransientAttachments(RHI::FrameGraphAttachmentInterface attachmentDatabase)
        {
            for (const Ptr<PassAttachment>& attachment : m_ownedAttachments)
            {
                if (attachment->m_lifetime == RHI::AttachmentLifetimeType::Transient)
                {
                    switch (attachment->m_descriptor.m_type)
                    {
                    case RHI::AttachmentType::Image:
                        attachmentDatabase.CreateTransientImage(attachment->GetTransientImageDescriptor());
                        break;
                    case RHI::AttachmentType::Buffer:
                        attachmentDatabase.CreateTransientBuffer(attachment->GetTransientBufferDescriptor());
                        break;
                    default:
                        AZ_RPI_PASS_ASSERT(false, "Error, transient attachment is neither an image nor a buffer!");
                        break;
                    }
                }
            }
        }

        void Pass::ImportAttachments(RHI::FrameGraphAttachmentInterface attachmentDatabase)
        {
            for (const Ptr<PassAttachment>& attachment : m_ownedAttachments)
            {
                if (attachment->m_lifetime == RHI::AttachmentLifetimeType::Imported)
                {
                    // make sure to only import the resource one time
                    RHI::AttachmentId attachmentId = attachment->GetAttachmentId();
                    const RHI::FrameAttachment* currentAttachment = attachmentDatabase.FindAttachment(attachmentId);

                    if (azrtti_istypeof<Image>(attachment->m_importedResource.get()))
                    {
                        Image* image = static_cast<Image*>(attachment->m_importedResource.get());
                        if (currentAttachment == nullptr)
                        {
                            attachmentDatabase.ImportImage(attachmentId, image->GetRHIImage());
                        }
                        else
                        {
                            AZ_Assert(currentAttachment->GetResource() == image->GetRHIImage(),
                                "Importing image attachment named \"%s\" but a different attachment with the "
                                "same name already exists in the database.\n", attachmentId.GetCStr());
                        }
                    }
                    else if (azrtti_istypeof<Buffer>(attachment->m_importedResource.get()))
                    {
                        Buffer* buffer = static_cast<Buffer*>(attachment->m_importedResource.get());
                        if (currentAttachment == nullptr)
                        {
                            attachmentDatabase.ImportBuffer(attachmentId, buffer->GetRHIBuffer());
                        }
                        else
                        {
                            AZ_Assert(currentAttachment->GetResource() == buffer->GetRHIBuffer(),
                                "Importing buffer attachment named \"%s\" but a different attachment with the "
                                "same name already exists in the database.\n", attachmentId.GetCStr());
                        }
                    }
                    else
                    {
                        AZ_RPI_PASS_ERROR(false, "Can't import unknown resource type");
                    }
                }
            }
        }

        void Pass::UpdateAttachmentUsageIndices()
        {
            // We want to find attachments that are used more than once by the same pass
            // An example of this could be reading from and writing to different mips of the same texture

            // Loop over all attachments bound to this pass
            size_t size = m_attachmentBindings.size();
            for (size_t i = 0; i < size; ++i)
            {
                PassAttachmentBinding& binding01 = m_attachmentBindings[i];

                // For the outer loop, only consider bindings which are the
                // first occurrence of their given attachment in the pass
                if (binding01.m_attachmentUsageIndex != 0)
                {
                    continue;
                }

                // Loop over all subsequent bindings in the pass
                uint8_t duplicateCount = 0;
                for (size_t j = i + 1; j < size; ++j)
                {
                    PassAttachmentBinding& binding02 = m_attachmentBindings[j];

                    // Bindings are considered having the same attachment if they are connected to the same binding...
                    bool haveSameConnection = binding01.m_connectedBinding == binding02.m_connectedBinding;
                    haveSameConnection = haveSameConnection && binding01.m_connectedBinding != nullptr;

                    // ... Or if they point to the same attachment
                    bool isSameAttachment = binding01.GetAttachment() == binding02.GetAttachment();
                    isSameAttachment = isSameAttachment && binding01.GetAttachment() != nullptr;

                    // If binding 01 and binding 02 have the same attachment, update the attachment usage index on binding 02
                    if (haveSameConnection || isSameAttachment)
                    {
                        binding02.m_attachmentUsageIndex = ++duplicateCount;
                    }
                }
            }
        }

        void Pass::UpdateOwnedAttachments()
        {
            // Update the output attachments to coincide with their source attachments (if specified)
            // This involves getting the format and calculating the size from the source attachment
            for (Ptr<PassAttachment>& attachment: m_ownedAttachments)
            {
                attachment->Update();
            }
        }

        void Pass::UpdateConnectedBinding(PassAttachmentBinding& binding)
        {
            bool useFallback = (m_state != PassState::Building && !IsEnabled());
            binding.UpdateConnection(useFallback);
        }

        void Pass::UpdateConnectedBindings()
        {
            // Depending on whether a pass is enabled or not, it may switch it's bindings to become a pass-through
            // For this reason we update connecting bindings on a per-frame basis
            for (PassAttachmentBinding& binding : m_attachmentBindings)
            {
                UpdateConnectedBinding(binding);
            }
        }

        void Pass::UpdateConnectedInputBindings()
        {
            for (uint8_t idx : m_inputBindingIndices)
            {
                UpdateConnectedBinding(m_attachmentBindings[idx]);
            }
            for (uint8_t idx : m_inputOutputBindingIndices)
            {
                UpdateConnectedBinding(m_attachmentBindings[idx]);
            }
        }

        void Pass::UpdateConnectedOutputBindings()
        {
            for (uint8_t idx : m_outputBindingIndices)
            {
                UpdateConnectedBinding(m_attachmentBindings[idx]);
            }
        }

        void Pass::RegisterPipelineGlobalConnections()
        {
            if (!m_pipeline)
            {
                AZ_RPI_PASS_ERROR(m_pipelineGlobalConnections.size() == 0,
                    "Pass::RegisterPipelineGlobalConnections() - PipelineGlobal connections specified but no pipeline set on pass [%s]",
                    m_path.GetCStr());
            }

            for (const PipelineGlobalConnection& connection : m_pipelineGlobalConnections)
            {
                PassAttachmentBinding* binding = FindAttachmentBinding(connection.m_localBinding);
                AZ_RPI_PASS_ERROR(binding != nullptr, "Pass::RegisterPipelineGlobalConnections() - Could not find local binding [%s]",
                                  connection.m_localBinding.GetCStr());

                if (binding)
                {
                    m_pipeline->AddPipelineGlobalConnection(connection.m_globalName, binding, this);
                }
            }

            m_flags.m_containsGlobalReference = (m_pipelineGlobalConnections.size() > 0);
        }

        // --- Queuing functions with PassSystem ---

        void Pass::QueueForBuildAndInitialization()
        {
            // Don't queue if we're currently building. Don't queue if we're already queued for Build or Removal
            if (m_state != PassState::Building &&
                m_queueState != PassQueueState::QueuedForBuildAndInitialization &&
                m_queueState != PassQueueState::QueuedForRemoval)
            {
                // NOTE: We only queue for Build here, the queue for Initialization happens at the end of Pass::Build
                // (doing it this way is an optimization to minimize the number of passes queued for initialization,
                //  as many passes will be initialized by their parent passes and thus don't need to be queued)
                PassSystemInterface::Get()->QueueForBuild(this);

                m_queueState = PassQueueState::QueuedForBuildAndInitialization;

                // Transition state
                // If we are Rendering, the state will transition [Rendering -> Queued] in Pass::FrameEnd
                // TODO: the PassState::Reset check is a quick fix until the pass concurrency with multiple scenes issue is fixed
                if (m_state != PassState::Rendering && m_state != PassState::Reset)
                {
                    m_state = PassState::Queued;
                }
            }
        }

        void Pass::QueueForInitialization()
        {
            // Only queue if the pass is not in any queue. Don't queue if we're currently initializing.
            if (m_queueState == PassQueueState::NoQueue && m_state != PassState::Initializing)
            {
                PassSystemInterface::Get()->QueueForInitialization(this);
                m_queueState = PassQueueState::QueuedForInitialization;

                // Transition state
                // If we are Rendering, the state will transition [Rendering -> Queued] in Pass::FrameEnd
                // If the state is Built, preserve the state since [Built -> Initializing] is a valid transition
                // Preserving PassState::Built lets the pass ignore subsequent build calls in the same frame
                if (m_state != PassState::Rendering && m_state != PassState::Built)
                {
                    m_state = PassState::Queued;
                }
            }
        }

        void Pass::QueueForRemoval()
        {
            // Skip only if we're already queued for removal, otherwise proceed.
            // QueuedForRemoval overrides QueuedForBuildAndInitialization and QueuedForInitialization.
            if (m_queueState != PassQueueState::QueuedForRemoval)
            {
                PassSystemInterface::Get()->QueueForRemoval(this);
                m_queueState = PassQueueState::QueuedForRemoval;

                // Transition state
                // If we are Rendering, the state will transition [Rendering -> Queued] in Pass::FrameEnd
                if (m_state != PassState::Rendering)
                {
                    m_state = PassState::Queued;
                }
            }
        }

        // --- Pass behavior functions ---

        void Pass::Reset()
        {
            AZ_RPI_BREAK_ON_TARGET_PASS;

            // Ensure we're in a valid state to reset. This ensures the pass won't be reset multiple times in the same frame.
            bool execute = (m_state == PassState::Idle);
            execute = execute || (m_state == PassState::Queued && m_queueState == PassQueueState::QueuedForBuildAndInitialization);
            execute = execute || (m_state == PassState::Queued && m_queueState == PassQueueState::QueuedForInitialization);

            if (!execute)
            {
                return;
            }

            m_state = PassState::Resetting;

            if (m_flags.m_isPipelineRoot)
            {
                m_pipeline->ClearGlobalBindings();
            }

            // Store references to imported attachments to underlying images and buffers aren't deleted during attachment building
            StoreImportedAttachmentReferences();

            // Clear lists
            m_inputBindingIndices.clear();
            m_inputOutputBindingIndices.clear();
            m_outputBindingIndices.clear();
            m_attachmentBindings.clear();
            m_ownedAttachments.clear();
            m_executeAfterPasses.clear();
            m_executeBeforePasses.clear();

            ResetInternal();

            m_state = PassState::Reset;
        }

        void Pass::Build(bool calledFromPassSystem)
        {
            AZ_RPI_BREAK_ON_TARGET_PASS;

            // Ensure we're in a valid state to build. This ensures the pass won't be built multiple times in the same frame.
            bool execute = (m_state == PassState::Reset);

            if (!execute)
            {
                return;
            }

            m_state = PassState::Building;

            // Bindings, inputs and attachments
            CreateBindingsFromTemplate();
            RegisterPipelineGlobalConnections();
            SetupPassDependencies();
            CreateAttachmentsFromTemplate();
            CreateAttachmentsFromRequest();
            SetupInputsFromTemplate();
            SetupInputsFromRequest();

            // Custom pass behavior
            BuildInternal();

            // Outputs
            SetupOutputsFromTemplate();
            SetupOutputsFromRequest();

            // Update
            UpdateConnectedBindings();
            UpdateOwnedAttachments();
            UpdateAttachmentUsageIndices();

            OnDescendantChange(PassDescendantChangeFlags::Build);
            OnBuildFinished();

            // If this pass's Build() wasn't called from the Pass System, then it was called by it's parent pass
            // In which case we don't need to queue for initialization because the parent will already be queued
            if (calledFromPassSystem)
            {
                // Queue for Initialization
                QueueForInitialization();
            }
        }

        void Pass::Initialize()
        {
            AZ_RPI_BREAK_ON_TARGET_PASS;

            // Ensure we're in a valid state to initialize. This ensures the pass won't be initialized multiple times in the same frame.
            bool execute = (m_state == PassState::Idle || m_state == PassState::Built);
            execute = execute || (m_state == PassState::Queued && m_queueState == PassQueueState::QueuedForInitialization);

            if (!execute)
            {
                return;
            }

            m_state = PassState::Initializing;
            m_queueState = PassQueueState::NoQueue;

            InitializeInternal();
            
            // Need to recreate the dest attachment because the source attachment might be changed
            if (!m_attachmentCopy.expired())
            {
                m_attachmentCopy.lock()->InvalidateDestImage();
            }

            m_state = PassState::Initialized;
        }

        void Pass::OnInitializationFinished()
        {
            m_flags.m_alreadyCreatedChildren = false;
            m_importedAttachmentStore.clear();
            OnInitializationFinishedInternal();

            m_state = PassState::Idle;
        }

        void Pass::OnBuildFinished()
        {
            bool subpassInputSupported = false;
            if (auto* renderPipeline = GetRenderPipeline())
            {
                subpassInputSupported = renderPipeline->SubpassMergingSupported();
            }
            
            RHI::SubpassInputSupportType supportedTypes = RHI::RHISystemInterface::Get()->GetDevice()->GetFeatures().m_subpassInputSupport;
            if (!subpassInputSupported)
            {
                supportedTypes = RHI::SubpassInputSupportType::None;
            }
            ReplaceSubpassInputs(supportedTypes);
            OnBuildFinishedInternal();

            m_flags.m_hasSubpassInput = AZStd::any_of(
                m_attachmentBindings.begin(),
                m_attachmentBindings.end(),
                [](const auto& element)
                {
                    return element.m_scopeAttachmentUsage == RHI::ScopeAttachmentUsage::SubpassInput;
                });
            m_state = PassState::Built;
            m_queueState = PassQueueState::NoQueue;
        }

        void Pass::Validate(PassValidationResults& validationResults)
        {
            if (PassValidation::IsEnabled())
            {
                // Log passes with missing input
                for (uint8_t idx : m_inputBindingIndices)
                {
                    if (!m_attachmentBindings[idx].GetAttachment())
                    {
                        validationResults.m_passesWithMissingInputs.push_back(this);
                        break;
                    }
                }
                // Log passes with missing input/output
                for (uint8_t idx : m_inputOutputBindingIndices)
                {
                    if (!m_attachmentBindings[idx].GetAttachment())
                    {
                        validationResults.m_passesWithMissingInputOutputs.push_back(this);
                        break;
                    }
                }
                // Log passes with missing output (note that missing output connections are not considered an error)
                for (uint8_t idx : m_outputBindingIndices)
                {
                    if (!m_attachmentBindings[idx].GetAttachment())
                    {
                        validationResults.m_passesWithMissingOutputs.push_back(this);
                        break;
                    }
                }

                if (m_errorMessages.size() > 0)
                {
                    validationResults.m_passesWithErrors.push_back(this);
                }

                if (m_warningMessages.size() > 0)
                {
                    validationResults.m_passesWithWarnings.push_back(this);
                }
            }
        }

        void Pass::FrameBegin(FramePrepareParams params)
        {
            AZ_RPI_BREAK_ON_TARGET_PASS;

            bool isEnabled = IsEnabled();
            bool earlyOut = !isEnabled;
            // Since IsEnabled can be virtual and we need to detect HierarchyChange, we can't use the m_flags.m_enabled flag
            if (isEnabled != m_flags.m_lastFrameEnabled)
            {
                OnHierarchyChange();
            }
            m_flags.m_lastFrameEnabled = isEnabled;
            // Skip if this pass is the root of the pipeline and the pipeline is set to not render
            if (m_flags.m_isPipelineRoot)
            {
                AZ_RPI_PASS_ASSERT(m_pipeline != nullptr, "Pass is flagged as a pipeline root but it's pipeline pointer is invalid while trying to render");
                earlyOut = earlyOut || m_pipeline == nullptr || m_pipeline->GetRenderMode() == RenderPipeline::RenderMode::NoRender;
            }

            if (earlyOut)
            {
                return;
            }

            AZ_Error("PassSystem", m_state == PassState::Idle,
                "Pass::FrameBegin - Pass [%s] is attempting to render, and should be in the 'Idle' or 'Queued' state, but is in the '%s' state.",
                m_path.GetCStr(), ToString(m_state).data());

            m_state = PassState::Rendering;

            UpdateOwnedAttachments();

            CreateTransientAttachments(params.m_frameGraphBuilder->GetAttachmentDatabase());
            ImportAttachments(params.m_frameGraphBuilder->GetAttachmentDatabase());

            // readback attachment with input state
            UpdateReadbackAttachment(params, true);

            // FrameBeginInternal needs to be the last function be called in FrameBegin because its implementation expects 
            // all the attachments are imported to database (for example, ImageAttachmentPreview)
            FrameBeginInternal(params);
            
            // readback attachment with output state
            UpdateReadbackAttachment(params, false);

            // update attachment copy for preview
            UpdateAttachmentCopy(params);
        }

        void Pass::FrameEnd()
        {
            if (m_state == PassState::Rendering)
            {
                FrameEndInternal();
                m_state = (m_queueState == PassQueueState::NoQueue) ? PassState::Idle : PassState::Queued;
            }
        }

        // --- RenderPipeline, PipelineViewTag and DrawListTag ---
                
        RHI::DrawListTag Pass::GetDrawListTag() const
        {
            static RHI::DrawListTag invalidTag;
            return invalidTag;
        }

        const PipelineViewTag& Pass::GetPipelineViewTag() const
        {
            if (m_viewTag.IsEmpty())
            {
                if (m_flags.m_isPipelineRoot && m_pipeline)
                {
                    return m_pipeline->GetMainViewTag();
                }
                else if (m_parent)
                {
                    return m_parent->GetPipelineViewTag();
                }
            }
            return m_viewTag;
        }

        void Pass::SetRenderPipeline(RenderPipeline* pipeline)
        {
            AZ_Assert(!m_pipeline || !pipeline || m_pipeline == pipeline,
                "Switching passes between pipelines is not supported and may result in undefined behavior");

            if (m_pipeline != pipeline)
            {
                m_pipeline = pipeline;

                // Re-queue for new pipeline. 
                if (m_pipeline != nullptr)
                {
                    PassState currentState = m_state;
                    m_queueState = PassQueueState::NoQueue;
                    QueueForBuildAndInitialization();
                    if (currentState == PassState::Reset)
                    {
                        m_state = PassState::Reset;
                    }
                }
            }
        }
        
        void Pass::ManualPipelineBuildAndInitialize()
        {
            Build();
            Initialize();
            OnInitializationFinished();
        }

        Scene* Pass::GetScene() const
        {
            if (m_pipeline)
            {
                return m_pipeline->GetScene();
            }
            return nullptr;
        }

        PassTree* Pass::GetPassTree() const
        {
            return m_pipeline ? &(m_pipeline->m_passTree) : nullptr;
        }

        void Pass::GetViewDrawListInfo(RHI::DrawListMask& outDrawListMask, PassesByDrawList& outPassesByDrawList, const PipelineViewTag& viewTag) const
        {
            // NOTE: we always collect the draw list mask regardless if the pass enabled or not. The reason is we want to keep the view information
            // even when pass is disabled so it can continue work correctly when re-enable it.

            // Only get the DrawListTag if this pass has a DrawListTag and it's PipelineViewId matches
            if (BindViewSrg() && HasDrawListTag() && GetPipelineViewTag() == viewTag)
            {
                RHI::DrawListTag drawListTag = GetDrawListTag();
                if (drawListTag.IsValid() && outPassesByDrawList.find(drawListTag) == outPassesByDrawList.end())
                {
                    outPassesByDrawList[drawListTag] = this;
                    outDrawListMask.set(drawListTag.GetIndex());
                }
            }
        }

        void Pass::GetPipelineViewTags(PipelineViewTags& outTags) const
        {
            if (BindViewSrg())
            {
                outTags.insert(GetPipelineViewTag());
            }
        }

        void Pass::SortDrawList(RHI::DrawList& drawList) const
        {
            if (!drawList.empty())
            {
                RHI::SortDrawList(drawList, m_drawListSortType);
            }
        }

        // --- Debug & Validation functions ---

        bool PassValidationResults::IsValid()
        {
            if (PassValidation::IsEnabled())
            {
                // Pass validation fail if there are any passes with build errors or missing inputs (or input/outputs)
                return (m_passesWithErrors.size() == 0) && (m_passesWithMissingInputs.size() == 0) && (m_passesWithMissingInputOutputs.size() == 0);
            }
            else
            {
                return true;
            }
        }

        TimestampResult Pass::GetLatestTimestampResult() const
        {
            return GetTimestampResultInternal();
        }

        PipelineStatisticsResult Pass::GetLatestPipelineStatisticsResult() const
        {
            return GetPipelineStatisticsResultInternal();
        }

        bool Pass::ReadbackAttachment(AZStd::shared_ptr<AttachmentReadback> readback, uint32_t readbackIndex, const Name& slotName
            , PassAttachmentReadbackOption option, const RHI::ImageSubresourceRange* mipsRange)
        {
            // Return false if it's already readback
            if (m_attachmentReadback)
            {
                AZ_Warning("Pass", false, "ReadbackAttachment: skip readback pass [%s] slot [%s] because there is an another active readback", m_path.GetCStr(), slotName.GetCStr());
                return false;
            }
            uint32_t bindingIndex = 0;
            for (auto& binding : m_attachmentBindings)
            {
                if (slotName == binding.m_name)
                {
                    RHI::AttachmentType type = binding.GetAttachment()->GetAttachmentType();
                    if (type == RHI::AttachmentType::Buffer || type == RHI::AttachmentType::Image)
                    {
                        RHI::AttachmentId attachmentId = binding.GetAttachment()->GetAttachmentId();

                        // Append slot index and pass name so the read back's name won't be same as the attachment used in other passes.
                        AZStd::string readbackName = AZStd::string::format("%s_%d_%d_%s", attachmentId.GetCStr(),
                            readbackIndex, bindingIndex, GetName().GetCStr());
                        if (readback->ReadPassAttachment(binding.GetAttachment().get(), AZ::Name(readbackName), mipsRange))
                        {
                            m_readbackOption = PassAttachmentReadbackOption::Output;
                            // The m_readbackOption is only meaningful if the attachment is used for InputOutput.
                            if (binding.m_slotType == PassSlotType::InputOutput)
                            {
                                m_readbackOption = option;
                            }
                            m_attachmentReadback = readback;
                            return true;
                        }
                        return false;
                    }
                }
                bindingIndex++;
            }
            AZ_Warning("Pass", false, "ReadbackAttachment: failed to find slot [%s] from pass [%s]", slotName.GetCStr(), m_path.GetCStr());
            return false;
        }

        void Pass::UpdateReadbackAttachment(FramePrepareParams params, bool beforeAddScopes)
        {
            if (beforeAddScopes == (m_readbackOption == PassAttachmentReadbackOption::Input) && m_attachmentReadback)
            {
                // Read the attachment for one frame. The reference can be released afterwards
                m_attachmentReadback->FrameBegin(params);
                m_attachmentReadback = nullptr;
            }
        }

        void Pass::UpdateAttachmentCopy(FramePrepareParams params)
        {
            if (!m_attachmentCopy.expired())
            {
                m_attachmentCopy.lock()->FrameBegin(params);
            }
        }

        bool Pass::UpdateImportedAttachmentImage(Ptr<PassAttachment>& attachment, RHI::ImageBindFlags bindFlags, RHI::ImageAspectFlags aspectFlags)
        {
            if (!attachment)
            {
                return false;
            }

            // update the image attachment descriptor to sync up size and format
            attachment->Update(true);
            RHI::ImageDescriptor& imageDesc = attachment->m_descriptor.m_image;

            // The Format Source had no valid attachment
            if (imageDesc.m_format == RHI::Format::Unknown)
            {
                return false;
            }

            RPI::AttachmentImage* currentImage = azrtti_cast<RPI::AttachmentImage*>(attachment->m_importedResource.get());

            if (attachment->m_importedResource && imageDesc.m_size == currentImage->GetDescriptor().m_size)
            {
                // If there's a resource already and the size didn't change, just keep using the old AttachmentImage.
                return true;
            }
            
            Data::Instance<RPI::AttachmentImagePool> pool = RPI::ImageSystemInterface::Get()->GetSystemAttachmentPool();

            // set the bind flags
            imageDesc.m_bindFlags |= bindFlags;
            
            // The ImageViewDescriptor must be specified to make sure the frame graph compiler doesn't treat this as a transient image.
            RHI::ImageViewDescriptor viewDesc = RHI::ImageViewDescriptor::Create(imageDesc.m_format, 0, 0);
            viewDesc.m_aspectFlags = aspectFlags;

            // The full path name is needed for the attachment image so it's not deduplicated from accumulation images in different pipelines.
            AZStd::string imageName = RPI::ConcatPassString(GetPathName(), attachment->m_path);
            auto attachmentImage = RPI::AttachmentImage::Create(*pool.get(), imageDesc, Name(imageName), nullptr, &viewDesc);

            if (attachmentImage)
            {
                attachment->m_path = attachmentImage->GetAttachmentId();
                attachment->m_importedResource = attachmentImage;
                return true;
            }else
            {
                AZ_Error("Pass", false, "UpdateImportedAttachmentImage failed because it is unable to create an attachment image.");
            }
            return false;
        }

        AZ::Name Pass::GetSuperVariantName() const
        {
            return AZ::Name(m_flags.m_hasSubpassInput ? RPI::SubpassInputSupervariantName : "");
        }

        void Pass::ReplaceSubpassInputs(RHI::SubpassInputSupportType supportedTypes)
        {
            m_flags.m_hasSubpassInput = false;
            for (size_t slotIndex = 0; slotIndex < m_attachmentBindings.size(); ++slotIndex)
            {
                PassAttachmentBinding& binding = m_attachmentBindings[slotIndex];
                if (binding.m_scopeAttachmentUsage == RHI::ScopeAttachmentUsage::SubpassInput)
                {
                    const RHI::ImageViewDescriptor& descriptor = binding.m_unifiedScopeDesc.GetImageViewDescriptor();
                    if ((RHI::CheckBitsAny(descriptor.m_aspectFlags, RHI::ImageAspectFlags::Color) &&
                         RHI::CheckBitsAny(supportedTypes, RHI::SubpassInputSupportType::Color)) ||
                        (RHI::CheckBitsAny(descriptor.m_aspectFlags, RHI::ImageAspectFlags::DepthStencil) &&
                         RHI::CheckBitsAny(supportedTypes, RHI::SubpassInputSupportType::DepthStencil)))
                    {
                        m_flags.m_hasSubpassInput = true;
                    }
                    else
                    {
                        binding.m_scopeAttachmentUsage = RHI::ScopeAttachmentUsage::Shader;
                        continue;
                    }
                }
            }
        }

        void Pass::PrintIndent(AZStd::string& stringOutput, uint32_t indent) const
        {
            if (PassValidation::IsEnabled())
            {
                for (uint32_t i = 0; i < indent; ++i)
                {
                    stringOutput += "   ";
                }
            }
        }

        void Pass::PrintPassName(AZStd::string& stringOutput, uint32_t indent) const
        {
            if (PassValidation::IsEnabled())
            {
                stringOutput += "\n";

                PrintIndent(stringOutput, indent);

                stringOutput += "- ";
                //stringOutput += m_name.GetStringView();
                //stringOutput += "- ";
                stringOutput += m_path.GetStringView();
                stringOutput += "\n";
            }
        }

        void Pass::PrintErrors() const
        {
            if (PassValidation::IsEnabled())
            {
                PrintMessages(m_errorMessages);
            }
        }

        void Pass::PrintWarnings() const
        {
            if (PassValidation::IsEnabled())
            {
                PrintMessages(m_warningMessages);
            }
        }

        void Pass::PrintMessages(const AZStd::vector<AZStd::string>& messages) const
        {
            if (PassValidation::IsEnabled())
            {
                AZStd::string stringOutput;
                PrintPassName(stringOutput);

                for (const AZStd::string& message : messages)
                {
                    PrintIndent(stringOutput, 1);
                    stringOutput += message;
                    stringOutput += "\n";
                }
                AZ_Printf("PassSystem", stringOutput.c_str());
            }
        }

        void Pass::PrintBindingsWithoutAttachments(uint32_t slotTypeMask) const
        {            
            if (PassValidation::IsEnabled())
            {
                AZStd::string stringOutput;
                PrintPassName(stringOutput);

                for (const PassAttachmentBinding& binding : m_attachmentBindings)
                {
                    uint32_t bindingMask = (1 << uint32_t(binding.m_slotType));
                    if ((bindingMask & slotTypeMask) && (binding.GetAttachment() == nullptr))
                    {
                        // Print the name of the slot
                        PrintIndent(stringOutput, 1);
                        stringOutput += binding.m_name.GetStringView();
                        stringOutput += " has no valid attachment\n";
                    }
                }
                AZ_Printf("PassSystem", stringOutput.c_str());
            }
        }

        void Pass::ChangeConnection(const Name& localSlot, const Name& passName, const Name& attachment, RenderPipeline* pipeline)
        {
            Pass* otherPass{ nullptr };

            if (passName == PassNameParent)
            {
                otherPass = GetParent();
            }
            else if (passName == PipelineGlobalKeyword)
            {
                const AZ::RPI::PipelineGlobalBinding* globalBinding = pipeline->GetPipelineGlobalConnection(attachment);
                otherPass = globalBinding->m_pass;
            }
            else if (passName == PassNameThis)
            {
                otherPass = this;
            }
            else
            {
                otherPass = GetParent()->FindChildPass(passName).get();
            }

            AZ_Assert(otherPass, "Pass %s not found.", passName.GetCStr());

            ChangeConnection(localSlot, otherPass, attachment);
        }

        void Pass::ChangeConnection(const Name& localSlot, Pass* pass, const Name& attachment)
        {
            bool connectionFound(false);

            for (auto& connection : m_request.m_connections)
            {
                if (connection.m_localSlot == localSlot)
                {
                    connection.m_attachmentRef.m_pass = pass->GetName();
                    connection.m_attachmentRef.m_attachment = attachment;
                    connectionFound = true;
                    break;
                }
            }

            // if the connection is not yet present, we add it to the request so that it will be recreated
            // when the pass system updates
            if (!connectionFound)
            {
                m_request.m_connections.emplace_back(PassConnection{ localSlot, { pass->GetName(), attachment } });
            }

            auto attachmentBinding = FindAttachmentBinding(localSlot);

            if (attachmentBinding)
            {
                auto otherAttachmentBinding = pass->FindAttachmentBinding(attachment);

                if (otherAttachmentBinding)
                {
                    attachmentBinding->m_connectedBinding = otherAttachmentBinding;
                    attachmentBinding->UpdateConnection(false);
                }
                else
                {
                    // if the pass we should attach to has been newly created and not yet built,
                    // we can queue ourself to build as well to establish the connection in the next frame
                    QueueForBuildAndInitialization();
                }
            }
        }

        void Pass::DebugPrintBinding(AZStd::string& stringOutput, const PassAttachmentBinding& binding) const
        {
            if (PassValidation::IsEnabled())
            {
                // Print the name of the slot
                stringOutput += binding.m_name.GetStringView();

                // Print the attachment type and size, for example:
                // (Image, 1920, 1080)   or  (Buffer, 4096 bytes)
                if (binding.GetAttachment() != nullptr)
                {
                    stringOutput += " (";

                    // Images will have the format: AttachmentName (Image, 1920, 1080)
                    if (binding.GetAttachment()->m_descriptor.m_type == RHI::AttachmentType::Image)
                    {
                        stringOutput += "Image";
                        RHI::ImageDescriptor& desc = binding.GetAttachment()->m_descriptor.m_image;
                        uint32_t dimensions = static_cast<uint32_t>(desc.m_dimension);
                        for (uint32_t i = 0; i < dimensions; ++i)
                        {
                            stringOutput += ", ";
                            stringOutput += AZStd::to_string(desc.m_size[i]);
                        }
                        if (desc.m_multisampleState.m_samples > 1)
                        {
                            if (desc.m_multisampleState.m_customPositionsCount > 0)
                            {
                                stringOutput += ", Custom_MSAA_";
                            }
                            else
                            {
                                stringOutput += ", MSAA_";
                            }
                            stringOutput += AZStd::to_string(desc.m_multisampleState.m_samples);
                            stringOutput += "x";
                        }
                    }
                    // Buffers will have the format: AttachmentName (Buffer, 4092 bytes)
                    else if (binding.GetAttachment()->m_descriptor.m_type == RHI::AttachmentType::Buffer)
                    {
                        stringOutput += "Buffer, ";
                        stringOutput += AZStd::to_string(binding.GetAttachment()->m_descriptor.m_buffer.m_byteCount);
                        stringOutput += " bytes";
                    }

                    stringOutput += ")";
                }
            }
        }

        void Pass::DebugPrintBindingAndConnection(AZStd::string& stringOutput, uint8_t bindingIndex) const
        {
            if (PassValidation::IsEnabled())
            {
                PrintIndent(stringOutput, m_treeDepth + 2);

                // Print the Attachment
                const PassAttachmentBinding& binding = m_attachmentBindings[bindingIndex];
                DebugPrintBinding(stringOutput, binding);

                // Print the Attachment it's connected to
                if (binding.m_connectedBinding != nullptr)
                {
                    stringOutput += " connected to ";
                    DebugPrintBinding(stringOutput, *binding.m_connectedBinding);
                }

                stringOutput += "\n";
            }
        }

        void Pass::DebugPrint() const
        {
            if (PassValidation::IsEnabled())
            {
                AZStd::string stringOutput;
                PrintPassName(stringOutput, m_treeDepth);

                // Print inputs
                if (m_inputBindingIndices.size() > 0)
                {
                    PrintIndent(stringOutput, m_treeDepth + 1);
                    stringOutput += "Inputs:\n";
                    for (uint8_t inputIndex : m_inputBindingIndices)
                    {
                        DebugPrintBindingAndConnection(stringOutput, inputIndex);
                    }
                }

                // Print input/outputs
                if (m_inputOutputBindingIndices.size() > 0)
                {
                    PrintIndent(stringOutput, m_treeDepth + 1);
                    stringOutput += "Input/Outputs:\n";
                    for (uint8_t inputIndex : m_inputOutputBindingIndices)
                    {
                        DebugPrintBindingAndConnection(stringOutput, inputIndex);
                    }
                }

                // Print outputs
                if (m_outputBindingIndices.size() > 0)
                {
                    PrintIndent(stringOutput, m_treeDepth + 1);
                    stringOutput += "Outputs:\n";
                    for (uint8_t inputIndex : m_outputBindingIndices)
                    {
                        DebugPrintBindingAndConnection(stringOutput, inputIndex);
                    }
                }
                AZ_Printf("PassSystem", stringOutput.c_str());
            }
        }

        void PassValidationResults::PrintValidationIfError()
        {
            if (PassValidation::IsEnabled())
            {
                if (!IsValid())
                {
                    AZ_Printf("PassSystem", "\n--- PASS VALIDATION FAILURE ---"
                        "\n--Critical Errors--\n");

                    AZ_Printf("PassSystem", "\nThere are %d passes with errors:\n", m_passesWithErrors.size());
                    for (Pass* pass : m_passesWithErrors)
                    {
                        pass->PrintErrors();
                    }

                    AZ_Printf("PassSystem", "\nThere are %d passes with missing Inputs:\n", m_passesWithMissingInputs.size());
                    for (Pass* pass : m_passesWithMissingInputs)
                    {
                        pass->PrintBindingsWithoutAttachments(uint32_t(PassSlotMask::Input));
                    }

                    AZ_Printf("PassSystem", "\nThere are %d passes with missing Inputs/Outputs:\n", m_passesWithMissingInputOutputs.size());
                    for (Pass* pass : m_passesWithMissingInputOutputs)
                    {
                        pass->PrintBindingsWithoutAttachments(uint32_t(PassSlotMask::InputOutput));
                    }

                    AZ_Printf("PassSystem", "\n--Non-Critical Errors/Warnings--\n");

                    AZ_Printf("PassSystem", "\nThere are %d passes with warnings:\n", m_passesWithWarnings.size());
                    for (Pass* pass : m_passesWithWarnings)
                    {
                        pass->PrintWarnings();
                    }
                }
            }
        }

    }   // namespace RPI
}   // namespace AZ
