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
    EditorStateMaskRenderer::EditorStateMaskRenderer(Name name, Data::Instance<RPI::Material> maskMaterial)
        : m_drawTag(name)
        , m_maskMaterial(maskMaterial)
    {
    }

    void EditorStateMaskRenderer::RenderMaskEntities(const AzToolsFramework::EntityIdSet& entityIds)
    {
        if (entityIds.empty())
        {
            m_drawableEntities.clear();
            return;
        }

        for (auto it = m_drawableEntities.begin(); it != m_drawableEntities.end();)
        {
            if (!entityIds.contains(it->first))
            {
                it = m_drawableEntities.erase(it);
            }
            else
            {
                it++;
            }
        }

        for (const auto& entityId : entityIds)
        {
            if (m_drawableEntities.find(entityId) == m_drawableEntities.end())
            {
                m_drawableEntities.emplace(
                    AZStd::piecewise_construct, AZStd::forward_as_tuple(entityId),
                    AZStd::forward_as_tuple(entityId, m_maskMaterial, m_drawTag));
            }
        }

        for (auto& [entityId, drawable] : m_drawableEntities)
        {
            if (drawable.CanDraw())
            {
                drawable.Draw();
            }
        }
    }
   
} // namespace AZ::Render
