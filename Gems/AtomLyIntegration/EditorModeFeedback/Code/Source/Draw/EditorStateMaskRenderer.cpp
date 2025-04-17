/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Draw/EditorStateMaskRenderer.h>

namespace AZ::Render
{
    EditorStateMaskRenderer::EditorStateMaskRenderer(const Name& name)
        : m_drawTag(name)
    {
    }

    void EditorStateMaskRenderer::RenderMaskEntities(
        Data::Instance<RPI::Material> maskMaterial, const AzToolsFramework::EntityIdSet& entityIds)
    {
        if (entityIds.empty())
        {
            m_drawableEntities.clear();
            return;
        }

        // Erase any drawable entity meshes not in the provided list of entities
        erase_if(m_drawableEntities, [&entityIds](const auto& elem) { return !entityIds.contains(elem.first); });

        // Construct the drawable entity meshes for entities not in the drawable entity cache
        for (const auto& entityId : entityIds)
        {
            if (m_drawableEntities.find(entityId) == m_drawableEntities.end())
            {
                m_drawableEntities.emplace(
                    AZStd::piecewise_construct, AZStd::forward_as_tuple(entityId),
                    AZStd::forward_as_tuple(entityId, maskMaterial, m_drawTag));
            }
        }

        // Render all of the drawable entities that can draw (not being able to draw is not a failure)
        for (auto& [entityId, drawable] : m_drawableEntities)
        {
            if (drawable.CanDraw())
            {
                drawable.Draw();
            }
        }
    }
} // namespace AZ::Render
