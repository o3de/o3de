/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Pass/EditorStatePassSystem.h>

namespace AZ::Render
{


    EditorStatePassSystem::EditorStatePassSystem(EditorStateParentPassList&& editorStateParentPasses)
        : m_editorStateParentPasses(AZStd::move(editorStateParentPasses))
    {
    }

    void EditorStatePassSystem::AddStatePassesToRenderPipeline([[maybe_unused]] RPI::RenderPipeline* renderPipeline) const
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
