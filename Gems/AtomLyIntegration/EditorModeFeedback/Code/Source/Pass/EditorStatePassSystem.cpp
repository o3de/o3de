/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Pass/EditorStatePassSystem.h>
#include <Pass/EditorModeFeedbackParentPass.h>
#include <Pass/State/EditorStateParentPassData.h>
#include <Pass/State/EditorStateParentPass.h>
#include <Pass/Child/EditorModeDesaturationPass.h>
#include <Pass/Child/EditorModeTintPass.h>
#include <Pass/Child/EditorModeBlurPass.h>
#include <Pass/Child/EditorModeOutlinePass.h>

#include <Atom/RPI.Public/Pass/PassFilter.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Reflect/Pass/RasterPassData.h>
#include <Atom/RPI.Reflect/Pass/FullscreenTrianglePassData.h>

namespace AZ::Render
{
    static constexpr const char* const MainPassParentTemplateName = "EditorModeFeedbackParentTemplate";
    static constexpr const char* const MainPassParentTemplatePassClassName = "EditorModeFeedbackParentPass";
    static constexpr const char* const MainPassParentPassName = "EditorModeFeedbackPassParent";
    static constexpr const char* const StatePassTemplatePassClassName = "EditorStateParentPass";
    static constexpr const char* const PassthroughStatePassTemplatePassClassName = "EditorStatePassthroughPass";

    static Name GetMaskPassTemplateNameForDrawList(const Name& drawList)
    {
        return Name(AZStd::string(drawList.GetStringView()) + "_EditorModeMaskTemplate");
    }

    static Name GetMaskPassNameForDrawList(const Name& drawList)
    {
        return Name(AZStd::string(drawList.GetStringView()) + "_EntityMaskPass");
    }

    //static Name GetPassthroughPassTemplateName(const EditorStateParentPassBase& state)
    //{
    //    return Name(state.GetStateName() + "PassthroughTemplate");
    //}
    
    //static Name GetPassthroughPassNameForState(const EditorStateParentPassBase& state)
    //{
    //    return Name(state.GetStateName() + "PassthroughPass");
    //}

    void CreateAndAddStateParentPassTemplate(const EditorStateParentPassBase& state)
    {
        auto stateParentPassTemplate = AZStd::make_shared<RPI::PassTemplate>();
        stateParentPassTemplate->m_name = state.GetPassTemplateName();
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
        auto previousOutput = AZStd::make_pair<Name, Name>(Name("Parent"), Name("InputColor"));
        AZ::u32 passCount = 0;
        for (const auto& childPassTemplate : state.GetChildPassDescriptorList())
        {
            passCount++;

            auto childPassName = Name(AZStd::string::format("%sChildPass%u", stateParentPassTemplate->m_name.GetCStr(), passCount));
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

    void CreateAndAddPassthroughPassTemplate(/* const EditorStateParentPassBase& state*/)
    {
        auto passTemplate = AZStd::make_shared<RPI::PassTemplate>();
        passTemplate->m_name = Name("PassThroughPassTemplate"); // GetPassthroughPassTemplateName(state);
        passTemplate->m_passClass = Name("FullScreenTriangle");//PassthroughStatePassTemplatePassClassName;
    
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
            const auto shaderFilePath = "shaders/editormodepassthrough.azshader";
            Data::AssetId shaderAssetId;
            Data::AssetCatalogRequestBus::BroadcastResult(
                shaderAssetId, &Data::AssetCatalogRequestBus::Events::GetAssetIdByPath, shaderFilePath, azrtti_typeid<RPI::ShaderAsset>(),
                false);
            if (!shaderAssetId.IsValid())
            {
                AZ_Assert(false, "[DisplayMapperPass] Unable to obtain asset id for %s.", shaderFilePath);
                return;
            }

            auto passData = AZStd::make_shared<RPI::EditorStatePassthroughPassData>();
            passData->m_pipelineViewTag = "MainCamera";
            passData->m_shaderAsset.m_filePath = shaderFilePath;
            passData->m_shaderAsset.m_assetId = shaderAssetId;
            //passData->editorStatePass = &state;
            passTemplate->m_passData = passData;
        }

        RPI::PassSystemInterface::Get()->AddPassTemplate(passTemplate->m_name, passTemplate);
    }

    void CreateAndAddMaskPassTemplate(const Name& drawList)
    {
        auto maskPassTemplate = AZStd::make_shared<RPI::PassTemplate>();
        maskPassTemplate->m_name = GetMaskPassTemplateNameForDrawList(drawList);
        maskPassTemplate->m_passClass = Name("RasterPass");

        // Input depth slot
        {
            RPI::PassSlot slot;
            slot.m_name = Name("InputDepth");
            slot.m_slotType = RPI::PassSlotType::Input;
            slot.m_shaderInputName = Name("m_existingDepth");
            slot.m_scopeAttachmentUsage = RHI::ScopeAttachmentUsage::Shader;
            slot.m_shaderImageDimensionsName = Name("m_existingDepthDimensions");
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

    static AZStd::unordered_set<Name> CreateMaskPassTemplatesFromStateParentPasses(
        const EditorStateParentPassList& editorStateParentPasses)
    {
        AZStd::unordered_set<Name> drawLists;
        for (const auto& state : editorStateParentPasses)
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

    EditorStatePassSystem::EditorStatePassSystem(EditorStateParentPassList&& editorStateParentPasses)
        : m_editorStateParentPasses(AZStd::move(editorStateParentPasses))
    {
        auto* passSystem = RPI::PassSystemInterface::Get();
        AZ_Assert(passSystem, "Cannot get the pass system.");
        passSystem->AddPassCreator(Name(MainPassParentTemplatePassClassName), &EditorModeFeedbackParentPass::Create);
        passSystem->AddPassCreator(Name(PassthroughStatePassTemplatePassClassName), &EditorStatePassthroughPass::Create);
        passSystem->AddPassCreator(Name(StatePassTemplatePassClassName), &EditorStateParentPass::Create);
        passSystem->AddPassCreator(Name("EditorModeDesaturationPass"), &EditorModeDesaturationPass::Create);
        passSystem->AddPassCreator(Name("EditorModeTintPass"), &EditorModeTintPass::Create);
        passSystem->AddPassCreator(Name("EditorModeBlurPass"), &EditorModeBlurPass::Create);
        passSystem->AddPassCreator(Name("EditorModeOutlinePass"), &EditorModeOutlinePass::Create);

        // Editor state child effect passes
        passSystem->LoadPassTemplateMappings("Passes/Child/EditorModeFeedback_ChildPassTemplates.azasset");

        // Editor state parent passes
        passSystem->LoadPassTemplateMappings("Passes/State/EditorModeFeedback_StatePassTemplates.azasset");
    }

    void EditorStatePassSystem::AddPassesToRenderPipeline(RPI::RenderPipeline* renderPipeline)
    {
        auto mainParentPassTemplate = AZStd::make_shared<RPI::PassTemplate>();
        mainParentPassTemplate->m_name = Name(MainPassParentTemplateName);
        mainParentPassTemplate->m_passClass = Name(MainPassParentTemplatePassClassName);
        
        // Input depth slot
        {
            RPI::PassSlot slot;
            slot.m_name = Name("InputDepth");
            slot.m_slotType = RPI::PassSlotType::Input;
            mainParentPassTemplate->AddSlot(slot);
        }
        
        // Input/output color slot
        {
            RPI::PassSlot slot;
            slot.m_name = Name("ColorInputOutput");
            slot.m_slotType = RPI::PassSlotType::InputOutput;
            mainParentPassTemplate->AddSlot(slot);
        }

        // Entity mask passes
        m_masks = CreateMaskPassTemplatesFromStateParentPasses(m_editorStateParentPasses);
        for (const auto& drawList : m_masks)
        {
            RPI::PassRequest pass;
            pass.m_passName = GetMaskPassNameForDrawList(drawList);
            pass.m_templateName = GetMaskPassTemplateNameForDrawList(drawList);

            // Input depth
            {
                RPI::PassConnection connection;
                connection.m_localSlot = Name("InputDepth");
                connection.m_attachmentRef = { Name("Parent"), Name("InputDepth") };
                pass.AddInputConnection(connection);
            }

            mainParentPassTemplate->AddPassRequest(pass);
        }
        
        // Editor state passes
        RPI::PassRequest pass;
        auto previousOutput = AZStd::make_pair<Name, Name>(Name("Parent"), Name("ColorInputOutput"));
        for (const auto& state : m_editorStateParentPasses)
        {
            CreateAndAddStateParentPassTemplate(*state);
            pass.m_passName = state->GetPassName();
            pass.m_templateName = state->GetPassTemplateName();
        
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
                connection.m_attachmentRef = { GetMaskPassNameForDrawList(state->GetEntityMaskDrawList()), Name("OutputEntityMask") };
                pass.AddInputConnection(connection);
            }
        
            // Input color
            {
                RPI::PassConnection connection;
                connection.m_localSlot = Name("InputColor");
                connection.m_attachmentRef = { previousOutput.first, previousOutput.second };
                pass.AddInputConnection(connection);
            }
        
            mainParentPassTemplate->AddPassRequest(pass);
        }

        // Passthrough pass
        {
            CreateAndAddPassthroughPassTemplate(/*state*/);
            RPI::PassRequest passthrough;
            passthrough.m_passName = Name("PassThroughPass");//GetPassthroughPassNameForState(*state);
            passthrough.m_templateName = Name("PassThroughPassTemplate");//GetPassthroughPassTemplateName(*state);

            // Input color
            {
                RPI::PassConnection connection;
                connection.m_localSlot = Name("InputColor");
                connection.m_attachmentRef = { pass.m_passName, Name("OutputColor") };
                passthrough.AddInputConnection(connection);
            }

            mainParentPassTemplate->AddPassRequest(passthrough);
            previousOutput = { passthrough.m_passName, Name("OutputColor") };
        }

        RPI::PassSystemInterface::Get()->AddPassTemplate(mainParentPassTemplate->m_name, mainParentPassTemplate);
        AZ::RPI::PassRequest passRequest;
        passRequest.m_passName = Name(MainPassParentPassName);
        passRequest.m_templateName = mainParentPassTemplate->m_name;
        passRequest.AddInputConnection(
            RPI::PassConnection{ Name("ColorInputOutput"), RPI::PassAttachmentRef{ Name("PostProcessPass"), Name("Output") } });
        passRequest.AddInputConnection(
            RPI::PassConnection{ Name("InputDepth"), RPI::PassAttachmentRef{ Name("DepthPrePass"), Name("Depth") } });

        RPI::Ptr<RPI::Pass> parentPass = RPI::PassSystemInterface::Get()->CreatePassFromRequest(&passRequest);
        if (!parentPass)
        {
            AZ_Error("", false, "Create editor mode feedback parent pass from pass request failed", renderPipeline->GetId().GetCStr());
            return;
        }

        // Inject the parent pass after the PostProcess pass
        if (!renderPipeline->AddPassAfter(parentPass, Name("PostProcessPass")))
        {
            AZ_Error(
                "", false, "Add editor mode feedback parent pass to render pipeline [%s] failed", renderPipeline->GetId().GetCStr());
            return;
        }
    }

    EntityMaskMap EditorStatePassSystem::GetEntitiesForEditorStatePasses() const
    {
        EntityMaskMap entityMaskMap;

        for (const auto& state : m_editorStateParentPasses)
        {
            if (state->IsEnabled())
            {
                const auto entityIds = state->GetMaskedEntities();
                auto& mask = entityMaskMap[state->GetEntityMaskDrawList()];
                for (const auto entityId : entityIds)
                {
                    mask.insert(entityId);
                }
            }
        }

        return entityMaskMap;
    }

    void EditorStatePassSystem::InitPasses([[maybe_unused]]RPI::RenderPipeline* renderPipeline)
    {
        //RPI::PassFilter mainPassParentPassFilter = RPI::PassFilter::CreateWithPassName(Name(MainPassParentPassName), renderPipeline);
        //RPI::Ptr<RPI::Pass> mainPass = RPI::PassSystemInterface::Get()->FindFirstPass(mainPassParentPassFilter);
        //if (mainPass)
        //{
        //    auto mainPassParent = azdynamic_cast<EditorModeFeedbackParentPass*>(mainPass.get());
        //    for (auto& state : m_editorStateParentPasses)
        //    {
        //        auto statePass = mainPassParent->FindChildPass(Name(state->GetPassName()));
        //        state->AddParentPassForPipeline(mainPassParent->GetPathName(), statePass);
        //    }
        //}
    }
} // namespace AZ::Render
