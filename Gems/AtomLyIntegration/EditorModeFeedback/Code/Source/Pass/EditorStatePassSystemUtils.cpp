/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Pass/State/EditorStateBufferCopyPassData.h>
#include <Pass/State/EditorStateBufferCopyPass.h>
#include <Pass/State/EditorStateParentPassData.h>
#include <Pass/EditorStatePassSystem.h>
#include <Pass/EditorStatePassSystemUtils.h>

#include <Atom/RPI.Reflect/Pass/RasterPassData.h>

namespace AZ::Render
{
    Name GetMaskPassTemplateNameForDrawList(const Name& drawList)
    {
        return Name(AZStd::string(drawList.GetStringView()) + "_EditorModeMaskTemplate");
    }

    Name GetMaskPassNameForDrawList(const Name& drawList)
    {
        return Name(AZStd::string(drawList.GetStringView()) + "_EntityMaskPass");
    }

    Name GetBufferCopyPassTemplateName(const EditorStateBase& state)
    {
        return Name(state.GetStateName() + "BufferCopyTemplate");
    }
    
    Name GetBufferCopyPassNameForState(const EditorStateBase& state)
    {
        return Name(state.GetStateName() + "BufferCopyPass");
    }

    void CreateAndAddStateParentPassTemplate(const EditorStateBase& state)
    {
        const auto templateName = state.GetPassTemplateName();
        if (RPI::PassSystemInterface::Get()->GetPassTemplate(templateName))
        {
            // Template was created by another pipeline, do not to create again
            return;
        }

        auto stateParentPassTemplate = AZStd::make_shared<RPI::PassTemplate>();
        stateParentPassTemplate->m_name = templateName;
        stateParentPassTemplate->m_passClass = StatePassTemplatePassClassName;

         // Input depth slot
        {
            RPI::PassSlot slot;
            slot.m_name = Name("InputDepth");
            slot.m_slotType = RPI::PassSlotType::Input;
            stateParentPassTemplate->AddSlot(slot);
        }

        // Input entity mask slot
        {
            RPI::PassSlot slot;
            slot.m_name = Name("InputEntityMask");
            slot.m_slotType = RPI::PassSlotType::Input;
            stateParentPassTemplate->AddSlot(slot);
        }

        // Input color slot
        {
            RPI::PassSlot slot;
            slot.m_name = Name("InputColor");
            slot.m_slotType = RPI::PassSlotType::Input;
            stateParentPassTemplate->AddSlot(slot);
        }

        // Output color slot
        {
            RPI::PassSlot slot;
            slot.m_name = Name("OutputColor");
            slot.m_slotType = RPI::PassSlotType::Output;
            stateParentPassTemplate->AddSlot(slot);
        }

        // Fallback connections
        {
            RPI::PassFallbackConnection fallbackConnection;
            fallbackConnection.m_inputSlotName = Name("InputColor");
            fallbackConnection.m_outputSlotName = Name("OutputColor");
            stateParentPassTemplate->m_fallbackConnections.push_back(fallbackConnection);
        }

        // Pass data
        {
            auto passData = AZStd::make_shared<RPI::EditorStateParentPassData>();
            passData->editorStatePass = &state;
            stateParentPassTemplate->m_passData = passData;
        }

        // Child passes
        auto previousOutput = AZStd::make_pair(Name("Parent"), Name("InputColor"));
        AZ::u32 passCount = 0;
        for (const auto& childPassTemplate : state.GetChildPassNameList())
        {
            auto childPassName = state.GetGeneratedChildPassName(passCount);
            RPI::PassRequest pass;
            pass.m_passName = childPassName;
            pass.m_templateName = childPassTemplate;
            
            // Input depth
            {
                RPI::PassConnection connection;
                connection.m_localSlot = Name("InputDepth");
                connection.m_attachmentRef = { Name("Parent"), Name("InputDepth") };
                pass.AddInputConnection(connection);
            }
            
            // Input entity mask
            {
                RPI::PassConnection connection;
                connection.m_localSlot = Name("InputEntityMask");
                connection.m_attachmentRef = { Name("Parent"), Name("InputEntityMask") };
                pass.AddInputConnection(connection);
            }
            
            // Input color
            {
                RPI::PassConnection connection;
                connection.m_localSlot = Name("InputColor");
                connection.m_attachmentRef = { previousOutput.first, previousOutput.second };
                pass.AddInputConnection(connection);
            }
            
            stateParentPassTemplate->AddPassRequest(pass);
            previousOutput = { pass.m_passName, Name("OutputColor") };
            passCount++;
        }

        // Connections
        {
            RPI::PassConnection connection;
            connection.m_localSlot = Name("OutputColor");
            connection.m_attachmentRef.m_pass = previousOutput.first;
            connection.m_attachmentRef.m_attachment = previousOutput.second;
            stateParentPassTemplate->AddOutputConnection(connection);
        }

        RPI::PassSystemInterface::Get()->AddPassTemplate(stateParentPassTemplate->m_name, stateParentPassTemplate);
    }

    void CreateAndAddBufferCopyPassTemplate(const EditorStateBase& state)
    {
        const auto templateName = GetBufferCopyPassTemplateName(state);
        if (RPI::PassSystemInterface::Get()->GetPassTemplate(templateName))
        {
            // Template was created by another pipeline, do not to create again
            return;
        }

        auto passTemplate = AZStd::make_shared<RPI::PassTemplate>();
        passTemplate->m_name = templateName;
        passTemplate->m_passClass = BufferCopyStatePassTemplatePassClassName;
    
        // Input color slot
        {
            RPI::PassSlot slot;
            slot.m_name = Name("InputColor");
            slot.m_slotType = RPI::PassSlotType::Input;
            slot.m_shaderInputName = Name("m_framebuffer");
            slot.m_scopeAttachmentUsage = RHI::ScopeAttachmentUsage::Shader;
            passTemplate->AddSlot(slot);
        }

        // Output color slot
        {
            RPI::PassSlot slot;
            slot.m_name = Name("OutputColor");
            slot.m_slotType = RPI::PassSlotType::Output;
            slot.m_scopeAttachmentUsage = RHI::ScopeAttachmentUsage::RenderTarget;
            slot.m_loadStoreAction.m_loadAction = RHI::AttachmentLoadAction::DontCare;
            passTemplate->AddSlot(slot);
        }

        // Connections
        {
            RPI::PassConnection connection;
            connection.m_localSlot = Name("OutputColor");
            connection.m_attachmentRef.m_pass = Name("Parent");
            connection.m_attachmentRef.m_attachment = Name("ColorInputOutput");
            passTemplate->AddOutputConnection(connection);
        }

        // Fallback connections
        {
            RPI::PassFallbackConnection fallbackConnection;
            passTemplate->m_fallbackConnections.push_back({ Name("InputColor"), Name("OutputColor") });
        }

        // Pass data
        {
            const auto shaderFilePath = "shaders/editormodebuffercopy.azshader";
            Data::AssetId shaderAssetId;
            Data::AssetCatalogRequestBus::BroadcastResult(
                shaderAssetId, &Data::AssetCatalogRequestBus::Events::GetAssetIdByPath, shaderFilePath, azrtti_typeid<RPI::ShaderAsset>(),
                false);
            if (!shaderAssetId.IsValid())
            {
                AZ_Assert(false, "[DisplayMapperPass] Unable to obtain asset id for %s.", shaderFilePath);
                return;
            }

            auto passData = AZStd::make_shared<RPI::EditorStateBufferCopyPassData>();
            passData->m_pipelineViewTag = "MainCamera";
            passData->m_shaderAsset.m_filePath = shaderFilePath;
            passData->m_shaderAsset.m_assetId = shaderAssetId;
            passData->editorStatePass = &state;
            passTemplate->m_passData = passData;
        }

        RPI::PassSystemInterface::Get()->AddPassTemplate(passTemplate->m_name, passTemplate);
    }

    void CreateAndAddMaskPassTemplate(const Name& drawList)
    {
        const auto templateName = GetMaskPassTemplateNameForDrawList(drawList);
        if (RPI::PassSystemInterface::Get()->GetPassTemplate(templateName))
        {
            // Template was created by another pipeline, do not to create again
            return;
        }

        auto maskPassTemplate = AZStd::make_shared<RPI::PassTemplate>();
        maskPassTemplate->m_name = templateName;
        maskPassTemplate->m_passClass = Name("RasterPass");

        // Input depth slot
        {
            RPI::PassSlot slot;
            slot.m_name = Name("InputDepth");
            slot.m_slotType = RPI::PassSlotType::Input;
            slot.m_shaderInputName = Name("m_existingDepth");
            slot.m_scopeAttachmentUsage = RHI::ScopeAttachmentUsage::Shader;
            slot.m_shaderImageDimensionsName = Name("m_existingDepthDimensions");
            slot.m_imageViewDesc = AZStd::make_shared<RHI::ImageViewDescriptor>();
            slot.m_imageViewDesc->m_aspectFlags = RHI::ImageAspectFlags::Depth;
            maskPassTemplate->AddSlot(slot);
        }

        // Output entity mask slot
        {
            RPI::PassSlot slot;
            slot.m_name = Name("OutputEntityMask");
            slot.m_slotType = RPI::PassSlotType::Output;
            slot.m_scopeAttachmentUsage = RHI::ScopeAttachmentUsage::RenderTarget;
            slot.m_loadStoreAction.m_loadAction = RHI::AttachmentLoadAction::Clear;
            slot.m_loadStoreAction.m_clearValue = RHI::ClearValue::CreateVector4Float(0.0, 0.0, 0.0, 0.0);
            maskPassTemplate->AddSlot(slot);
        }

        // Output entity mask attachment
        RPI::PassImageAttachmentDesc imageAttachment;
        imageAttachment.m_name = Name("OutputEntityMaskAttachment");
        imageAttachment.m_sizeSource.m_source.m_pass = Name("This");
        imageAttachment.m_sizeSource.m_source.m_attachment = Name("InputDepth");
        imageAttachment.m_imageDescriptor.m_format = RHI::Format::R8G8_UNORM;
        imageAttachment.m_imageDescriptor.m_sharedQueueMask = RHI::HardwareQueueClassMask::Graphics;
        maskPassTemplate->AddImageAttachment(imageAttachment);

        // Output entity mask
        RPI::PassConnection connection;
        connection.m_localSlot = Name("OutputEntityMask");
        connection.m_attachmentRef = { Name("This"), Name("OutputEntityMaskAttachment") };
        maskPassTemplate->AddOutputConnection(connection);

        // Pass data
        {
            auto passData = AZStd::make_shared<RPI::RasterPassData>();
            passData->m_drawListTag = Name(drawList);
            passData->m_passSrgShaderReference.m_filePath = "shaders/editormodemask.azshader";
            passData->m_pipelineViewTag = "MainCamera";
            maskPassTemplate->m_passData = passData;
        }

        RPI::PassSystemInterface::Get()->AddPassTemplate(maskPassTemplate->m_name, maskPassTemplate);
    }

    AZStd::unordered_set<Name> CreateMaskPassTemplatesFromEditorStates(
        const EditorStateList& editorStates)
    {
        AZStd::unordered_set<Name> drawLists;
        for (const auto& state : editorStates)
        {
            if (const auto drawList = state->GetEntityMaskDrawList();
                !drawLists.contains(drawList))
            {
                CreateAndAddMaskPassTemplate(drawList);
                drawLists.insert(drawList);
            }
        }

        return drawLists;
    }
} // namespace AZ::Render
