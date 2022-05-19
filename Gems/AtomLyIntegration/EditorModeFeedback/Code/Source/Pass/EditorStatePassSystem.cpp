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

namespace AZ::Render
{
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

        /// Editor state pass sysem parent
        passSystem->LoadPassTemplateMappings("Passes/EditorModeFeedback_PassTemplates.azasset");

        // Editor state child effect passes
        passSystem->LoadPassTemplateMappings("Passes/Child/EditorModeFeedback_ChildPassTemplates.azasset");

        // Editor state parent passes
        //passSystem->LoadPassTemplateMappings("Passes/State/EditorModeFeedback_StatePassTemplates.azasset");
    }

    void EditorStatePassSystem::AddPassesToRenderPipeline([[maybe_unused]] RPI::RenderPipeline* renderPipeline) const
    {

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
