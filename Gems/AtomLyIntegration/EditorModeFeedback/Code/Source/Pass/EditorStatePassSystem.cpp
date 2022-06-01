/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Pass/EditorStatePassSystem.h>
#include <Pass/Child/EditorModeFeedbackParentPass.h>
#include <Pass/Child/EditorModeDesaturationPass.h>
#include <Pass/Child/EditorModeTintPass.h>
#include <Pass/Child/EditorModeBlurPass.h>
#include <Pass/Child/EditorModeOutlinePass.h>

#include <Atom/RPI.Public/Pass/PassFilter.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Reflect/Pass/RasterPassData.h>

namespace AZ::Render
{
    //static AZStd::unordered_set<Name> GetMasksFromStateParentPasses(const EditorStateParentPassList& editorStateParentPasses)
    //{
    //    AZStd::unordered_set<Name> masks;
    //    for (const auto& state : editorStateParentPasses)
    //    {
    //        masks.insert(state->GetEntityMaskDrawList());
    //    }
    //
    //    return masks;
    //}

    AZStd::shared_ptr<RPI::PassTemplate> CreateMaskPassTemplate(const AZStd::string& drawTag)
    {
        auto maskPassTemplate = AZStd::make_shared<RPI::PassTemplate>();
        maskPassTemplate->m_name = Name(drawTag + "_EditorModeMaskTemplate");
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
            passData->m_drawListTag = Name("editormodemask");
            passData->m_passSrgShaderReference.m_filePath = "shaders/editormodemask.azshader";
            passData->m_pipelineViewTag = "MainCamera";
            maskPassTemplate->m_passData = passData;
        }

        return maskPassTemplate;
    }

    EditorStatePassSystem::EditorStatePassSystem(EditorStateParentPassList&& editorStateParentPasses)
        : m_editorStateParentPasses(AZStd::move(editorStateParentPasses))
    {
        auto* passSystem = RPI::PassSystemInterface::Get();
        AZ_Assert(passSystem, "Cannot get the pass system.");
        passSystem->AddPassCreator(Name("EditorModeFeedbackParentPass"), &EditorModeFeedbackParentPass::Create);
        passSystem->AddPassCreator(Name("EditorModeDesaturationPass"), &EditorModeDesaturationPass::Create);
        passSystem->AddPassCreator(Name("EditorModeTintPass"), &EditorModeTintPass::Create);
        passSystem->AddPassCreator(Name("EditorModeBlurPass"), &EditorModeBlurPass::Create);
        passSystem->AddPassCreator(Name("EditorModeOutlinePass"), &EditorModeOutlinePass::Create);

        // Editor state child effect passes
        passSystem->LoadPassTemplateMappings("Passes/Child/EditorModeFeedback_ChildPassTemplates.azasset");

        // Editor state parent passes
        passSystem->LoadPassTemplateMappings("Passes/State/EditorModeFeedback_StatePassTemplates.azasset");
    }

    void EditorStatePassSystem::AddPassesToRenderPipeline(RPI::RenderPipeline* renderPipeline) const
    {
        //const auto masks = GetMasksFromStateParentPasses(m_editorStateParentPasses);

        auto mainParentPassTemplate = AZStd::make_shared<RPI::PassTemplate>();
        mainParentPassTemplate->m_name = Name("EditorModeFeedbackParentTemplate");
        mainParentPassTemplate->m_passClass = Name("EditorModeFeedbackParentPass");
        
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
        
        // Entity mask pass
        {
            const auto maskPassTemplate = CreateMaskPassTemplate("editormodemask");
            RPI::PassSystemInterface::Get()->AddPassTemplate(maskPassTemplate->m_name, maskPassTemplate);
            RPI::PassRequest pass;
            pass.m_passName = Name("EntityMaskPass");
            pass.m_templateName = maskPassTemplate->m_name;
        
            // Input depth
            {
                RPI::PassConnection connection;
                connection.m_localSlot = Name("InputDepth");
                connection.m_attachmentRef = { Name("Parent"), Name("InputDepth") };
                pass.AddInputConnection(connection);
            }

            mainParentPassTemplate->AddPassRequest(pass);
        }
        
        // Focused entity pass
        {
            RPI::PassRequest pass;
            pass.m_passName = Name("EditorStateFocusedEntityParentPass");
            pass.m_templateName = Name("EditorStateFocusedEntityParentTemplate");
        
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
                connection.m_attachmentRef = { Name("EntityMaskPass"), Name("OutputEntityMask") };
                pass.AddInputConnection(connection);
            }
        
            // Input color
            {
                RPI::PassConnection connection;
                connection.m_localSlot = Name("InputColor");
                connection.m_attachmentRef = { Name("Parent"), Name("ColorInputOutput") };
                pass.AddInputConnection(connection);
            }
        
            mainParentPassTemplate->AddPassRequest(pass);
        }
        //
       // Connections
       //{
       //    RPI::PassConnection connection;
       //    connection.m_localSlot = Name("ColorInputOutput");
       //    connection.m_attachmentRef.m_pass = Name("EditorStateFocusedEntityParentPass");
       //    connection.m_attachmentRef.m_attachment = Name("OutputColor");
       //    mainParentPassTemplate->AddOutputConnection(connection);
       //}

        // Desaturation pass
        //{
        //    RPI::PassRequest pass;
        //    pass.m_passName = Name("DesaturationPass");
        //    pass.m_templateName = Name("EditorModeDesaturationTemplate");
        //
        //    // Input depth
        //    {
        //        RPI::PassConnection connection;
        //        connection.m_localSlot = Name("InputDepth");
        //        connection.m_attachmentRef = { Name("Parent"), Name("InputDepth") };
        //        pass.AddInputConnection(connection);
        //    }
        //
        //    // Input entity mask
        //    {
        //        RPI::PassConnection connection;
        //        connection.m_localSlot = Name("InputEntityMask");
        //        connection.m_attachmentRef = { Name("EntityMaskPass"), Name("OutputEntityMask") };
        //        pass.AddInputConnection(connection);
        //    }
        //
        //    // Input color
        //    {
        //        RPI::PassConnection connection;
        //        connection.m_localSlot = Name("InputColor");
        //        connection.m_attachmentRef = { Name("Parent"), Name("ColorInputOutput") };
        //        pass.AddInputConnection(connection);
        //    }
        //
        //    mainParentPassTemplate->AddPassRequest(pass);
        //}
        //
        //// Tint pass
        //{
        //    RPI::PassRequest pass;
        //    pass.m_passName = Name("TintPass");
        //    pass.m_templateName = Name("EditorModeTintTemplate");
        //
        //    // Input depth
        //    {
        //        RPI::PassConnection connection;
        //        connection.m_localSlot = Name("InputDepth");
        //        connection.m_attachmentRef = { Name("Parent"), Name("InputDepth") };
        //        pass.AddInputConnection(connection);
        //    }
        //
        //    // Input entity mask
        //    {
        //        RPI::PassConnection connection;
        //        connection.m_localSlot = Name("InputEntityMask");
        //        connection.m_attachmentRef = { Name("EntityMaskPass"), Name("OutputEntityMask") };
        //        pass.AddInputConnection(connection);
        //    }
        //
        //    // Input color
        //    {
        //        RPI::PassConnection connection;
        //        connection.m_localSlot = Name("InputColor");
        //        connection.m_attachmentRef = { Name("DesaturationPass"), Name("OutputColor") };
        //        pass.AddInputConnection(connection);
        //    }
        //
        //    mainParentPassTemplate->AddPassRequest(pass);
        //}
        //
        //// Blur pass
        //{
        //    RPI::PassRequest pass;
        //    pass.m_passName = Name("BlurPass");
        //    pass.m_templateName = Name("EditorModeBlurParentTemplate");
        //
        //    // Input depth
        //    {
        //        RPI::PassConnection connection;
        //        connection.m_localSlot = Name("InputDepth");
        //        connection.m_attachmentRef = { Name("Parent"), Name("InputDepth") };
        //        pass.AddInputConnection(connection);
        //    }
        //
        //    // Input entity mask
        //    {
        //        RPI::PassConnection connection;
        //        connection.m_localSlot = Name("InputEntityMask");
        //        connection.m_attachmentRef = { Name("EntityMaskPass"), Name("OutputEntityMask") };
        //        pass.AddInputConnection(connection);
        //    }
        //
        //    // Input color
        //    {
        //        RPI::PassConnection connection;
        //        connection.m_localSlot = Name("InputColor");
        //        connection.m_attachmentRef = { Name("TintPass"), Name("OutputColor") };
        //        pass.AddInputConnection(connection);
        //    }
        //
        //    mainParentPassTemplate->AddPassRequest(pass);
        //}
        //
        //// Blur pass
        //{
        //    RPI::PassRequest pass;
        //    pass.m_passName = Name("BlurPass");
        //    pass.m_templateName = Name("EditorModeBlurParentTemplate");
        //
        //    // Input depth
        //    {
        //        RPI::PassConnection connection;
        //        connection.m_localSlot = Name("InputDepth");
        //        connection.m_attachmentRef = { Name("Parent"), Name("InputDepth") };
        //        pass.AddInputConnection(connection);
        //    }
        //
        //    // Input entity mask
        //    {
        //        RPI::PassConnection connection;
        //        connection.m_localSlot = Name("InputEntityMask");
        //        connection.m_attachmentRef = { Name("EntityMaskPass"), Name("OutputEntityMask") };
        //        pass.AddInputConnection(connection);
        //    }
        //
        //    // Input color
        //    {
        //        RPI::PassConnection connection;
        //        connection.m_localSlot = Name("InputColor");
        //        connection.m_attachmentRef = { Name("TintPass"), Name("OutputColor") };
        //        pass.AddInputConnection(connection);
        //    }
        //
        //    mainParentPassTemplate->AddPassRequest(pass);
        //}
        //
        //// Outline pass
        //{
        //    RPI::PassRequest pass;
        //    pass.m_passName = Name("EntityOutlinePass");
        //    pass.m_templateName = Name("EditorModeOutlineTemplate");
        //
        //    // Input depth
        //    {
        //        RPI::PassConnection connection;
        //        connection.m_localSlot = Name("InputDepth");
        //        connection.m_attachmentRef = { Name("Parent"), Name("InputDepth") };
        //        pass.AddInputConnection(connection);
        //    }
        //
        //    // Input entity mask
        //    {
        //        RPI::PassConnection connection;
        //        connection.m_localSlot = Name("InputEntityMask");
        //        connection.m_attachmentRef = { Name("EntityMaskPass"), Name("OutputEntityMask") };
        //        pass.AddInputConnection(connection);
        //    }
        //
        //    // Input color
        //    {
        //        RPI::PassConnection connection;
        //        connection.m_localSlot = Name("InputColor");
        //        connection.m_attachmentRef = { Name("BlurPass"), Name("OutputColor") };
        //        pass.AddInputConnection(connection);
        //    }
        //
        //    mainParentPassTemplate->AddPassRequest(pass);
        //}

        // Outline pass
        {
            RPI::PassRequest pass;
            pass.m_passName = Name("EntityPassthroughPass");
            pass.m_templateName = Name("EditorModePassthroughTemplate");

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
                connection.m_attachmentRef = { Name("EntityMaskPass"), Name("OutputEntityMask") };
                pass.AddInputConnection(connection);
            }

            // Input color
            {
                RPI::PassConnection connection;
                connection.m_localSlot = Name("InputColor");
                connection.m_attachmentRef = { Name("EditorStateFocusedEntityParentPass"), Name("OutputColor") };
                pass.AddInputConnection(connection);
            }

            mainParentPassTemplate->AddPassRequest(pass);
        }

        RPI::PassSystemInterface::Get()->AddPassTemplate(mainParentPassTemplate->m_name, mainParentPassTemplate);
        AZ::RPI::PassRequest passRequest;
        passRequest.m_passName = Name("EditorModeFeedbackPassParent");
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
} // namespace AZ::Render
