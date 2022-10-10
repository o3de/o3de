/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Pass/EditorStatePassSystem.h>
#include <Pass/EditorStatePassSystemUtils.h>
#include <Pass/EditorModeFeedbackParentPass.h>
#include <Pass/State/EditorStateBufferCopyPass.h>
#include <Pass/Child/EditorModeDesaturationPass.h>
#include <Pass/Child/EditorModeTintPass.h>
#include <Pass/Child/EditorModeBlurPass.h>
#include <Pass/Child/EditorModeOutlinePass.h>

#include <Atom/RPI.Public/Pass/PassFilter.h>
#include <Atom/RPI.Public/RenderPipeline.h>

namespace AZ::Render
{
    static constexpr const char* const MainPassParentTemplateName = "EditorModeFeedbackParentTemplate";
    static constexpr const char* const MainPassParentTemplatePassClassName = "EditorModeFeedbackParentPass";
    static constexpr const char* const MainPassParentPassName = "EditorModeFeedback";
    
    static constexpr const char* const EditorModeDesaturationPassName = "EditorModeDesaturationPass";
    static constexpr const char* const EditorModeTintPassPassName = "EditorModeTintPass";
    static constexpr const char* const EditorModeBlurPassName = "EditorModeBlurPass";
    static constexpr const char* const EditorModeOutlinePassName = "EditorModeOutlinePass";

    EditorStatePassSystem::EditorStatePassSystem(EditorStateList&& editorStates)
        : m_editorStates(AZStd::move(editorStates))
    {
        auto* passSystem = RPI::PassSystemInterface::Get();
        AZ_Assert(passSystem, "Cannot get the pass system.");
        passSystem->AddPassCreator(Name(MainPassParentTemplatePassClassName), &EditorModeFeedbackParentPass::Create);
        passSystem->AddPassCreator(Name(BufferCopyStatePassTemplatePassClassName), &EditorStateBufferCopyPass::Create);
        passSystem->AddPassCreator(Name(StatePassTemplatePassClassName), &EditorStateParentPass::Create);
        passSystem->AddPassCreator(Name(EditorModeDesaturationPassName), &EditorModeDesaturationPass::Create);
        passSystem->AddPassCreator(Name(EditorModeTintPassPassName), &EditorModeTintPass::Create);
        passSystem->AddPassCreator(Name(EditorModeBlurPassName), &EditorModeBlurPass::Create);
        passSystem->AddPassCreator(Name(EditorModeOutlinePassName), &EditorModeOutlinePass::Create);

        // Editor state child effect passes
        passSystem->LoadPassTemplateMappings("Passes/Child/EditorModeFeedback_ChildPassTemplates.azasset");
    }

    void EditorStatePassSystem::AddPassesToRenderPipeline(RPI::RenderPipeline* renderPipeline)
    {
        const auto templateName = Name(MainPassParentTemplateName);
        if (RPI::PassSystemInterface::Get()->GetPassTemplate(templateName))
        {
            // Template was created by another pipeline, do not to create again
            return;
        }

        auto mainParentPassTemplate = AZStd::make_shared<RPI::PassTemplate>();
        mainParentPassTemplate->m_name = templateName;
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
        m_masks = CreateMaskPassTemplatesFromEditorStates(m_editorStates);
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
        auto previousOutput = AZStd::make_pair<Name, Name>(Name("Parent"), Name("ColorInputOutput"));
        for (const auto& state : m_editorStates)
        {
            CreateAndAddStateParentPassTemplate(*state);
            RPI::PassRequest pass;
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
        
            // Buffer copy pass 
            {
                CreateAndAddBufferCopyPassTemplate(*state);
                RPI::PassRequest buffercopy;
                buffercopy.m_passName = GetBufferCopyPassNameForState(*state);
                buffercopy.m_templateName = GetBufferCopyPassTemplateName(*state);
                
                // Input color
                {
                    RPI::PassConnection connection;
                    connection.m_localSlot = Name("InputColor");
                    connection.m_attachmentRef = { pass.m_passName, Name("OutputColor") };
                    buffercopy.AddInputConnection(connection);
                }
        
                mainParentPassTemplate->AddPassRequest(buffercopy);
                previousOutput = { buffercopy.m_passName, Name("OutputColor") };
            }
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
            AZ_Error("EditorStatePassSystem", false, "Create editor mode feedback parent pass from pass request failed", renderPipeline->GetId().GetCStr());
            return;
        }

        // Inject the parent pass after the PostProcess pass
        if (!renderPipeline->AddPassAfter(parentPass, Name("PostProcessPass")))
        {
            AZ_Error(
                "EditorStatePassSystem", false, "Add editor mode feedback parent pass to render pipeline [%s] failed", renderPipeline->GetId().GetCStr());
            return;
        }
    }

    EntityMaskMap EditorStatePassSystem::GetEntitiesForEditorStates() const
    {
        EntityMaskMap entityMaskMap;

        for (const auto& state : m_editorStates)
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

    void EditorStatePassSystem::Update()
    {
        for (auto& state : m_editorStates)
        {
            state->UpdatePassDataForPipelines();
        }
    }

    void EditorStatePassSystem::ConfigureStatePassesForPipeline([[maybe_unused]]RPI::RenderPipeline* renderPipeline)
    {
        RPI::PassFilter mainPassParentPassFilter = RPI::PassFilter::CreateWithPassName(Name(MainPassParentPassName), renderPipeline);
        RPI::Ptr<RPI::Pass> mainPass = RPI::PassSystemInterface::Get()->FindFirstPass(mainPassParentPassFilter);

        if (!mainPass)
        {
            return;
        }

        auto mainPassParent = azdynamic_cast<EditorModeFeedbackParentPass*>(mainPass.get());
        for (auto& state : m_editorStates)
        {
            auto statePass = mainPassParent->FindChildPass(Name(state->GetPassName()));
            state->AddParentPassForPipeline(renderPipeline->GetId(), statePass);
        }
    }

    void EditorStatePassSystem::RemoveStatePassesForPipeline([[maybe_unused]]RPI::RenderPipeline* renderPipeline)
    {
        for (auto& state : m_editorStates)
        {
            state->RemoveParentPassForPipeline(renderPipeline->GetId());
        }
    }
} // namespace AZ::Render
