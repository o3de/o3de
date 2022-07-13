/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Misc/RecastNavigationDebugDraw.h>
#include <AzFramework/Entity/EntityDebugDisplayBus.h>

namespace RecastNavigation
{
    RecastNavigationDebugDraw::RecastNavigationDebugDraw(bool drawLines) : m_drawLines(drawLines)
    {
    }

    void RecastNavigationDebugDraw::begin(duDebugDrawPrimitives prim, [[maybe_unused]] float size)
    {
        m_currentPrim = prim;
        m_verticesToDraw.clear();
    }

    void RecastNavigationDebugDraw::vertex(const float* pos, unsigned color)
    {
        AddVertex(pos[0], pos[1], pos[2], color);
    }

    void RecastNavigationDebugDraw::vertex(float x, float y, float z, unsigned color)
    {
        AddVertex(x, y, z, color);
    }

    void RecastNavigationDebugDraw::vertex(const float* pos, unsigned color, [[maybe_unused]] const float* uv)
    {
        AddVertex(pos[0], pos[1], pos[2], color);
    }

    void RecastNavigationDebugDraw::vertex(float x, float y, float z, unsigned color,
        [[maybe_unused]] const float u, [[maybe_unused]] const float v)
    {
        AddVertex(x, y, z, color);
    }

    void RecastNavigationDebugDraw::end()
    {
        constexpr AZ::Crc32 viewportId = AzFramework::g_defaultSceneEntityDebugDisplayId;
        AzFramework::DebugDisplayRequestBus::BusPtr debugDisplayBus;
        AzFramework::DebugDisplayRequestBus::Bind(debugDisplayBus, viewportId);
        AzFramework::DebugDisplayRequests* debugDisplay = AzFramework::DebugDisplayRequestBus::FindFirstHandler(debugDisplayBus);
        if (!debugDisplay)
        {
            return;
        }

        switch (m_currentPrim)
        {
        case DU_DRAW_POINTS:
            for (auto& i : m_verticesToDraw)
            {
                if (m_viewAabb.Contains(i.first))
                {
                    AZ::Color color = AZ::Color::CreateZero();
                    color.FromU32(i.second);
                    debugDisplay->SetColor(color);

                    debugDisplay->DrawPoint(i.first, 1);
                }
            }
            break;
        case DU_DRAW_TRIS:
            for (size_t i = 2; i < m_verticesToDraw.size(); i += 3)
            {
                if (m_viewAabb.Contains(m_verticesToDraw[i - 2].first) ||
                    m_viewAabb.Contains(m_verticesToDraw[i - 1].first) ||
                    m_viewAabb.Contains(m_verticesToDraw[i].first))
                {
                    AZ::Color color = AZ::Color::CreateZero();
                    color.FromU32(m_verticesToDraw[i].second);

                    debugDisplay->DrawTriangles(
                        {
                        m_verticesToDraw[i - 2].first,
                        m_verticesToDraw[i - 1].first,
                        m_verticesToDraw[i - 0].first
                        }, color);
                }
            }
            break;
        case DU_DRAW_QUADS:
            for (size_t i = 3; i < m_verticesToDraw.size(); i += 4)
            {
                if (m_viewAabb.Contains(m_verticesToDraw[i - 3].first) ||
                    m_viewAabb.Contains(m_verticesToDraw[i - 2].first) ||
                    m_viewAabb.Contains(m_verticesToDraw[i - 1].first) ||
                    m_viewAabb.Contains(m_verticesToDraw[i].first))
                {
                    AZ::Color color = AZ::Color::CreateZero();
                    color.FromU32(m_verticesToDraw[i].second);
                    debugDisplay->SetColor(color);

                    debugDisplay->DrawQuad(
                        m_verticesToDraw[i - 3].first, m_verticesToDraw[i - 2].first,
                        m_verticesToDraw[i - 1].first, m_verticesToDraw[i - 0].first);
                }
            }
            break;
        case DU_DRAW_LINES:
            for (size_t i = 1; m_drawLines && i < m_verticesToDraw.size(); i++)
            {
                if (m_viewAabb.Contains(m_verticesToDraw[i - 1].first) ||
                    m_viewAabb.Contains(m_verticesToDraw[i].first))
                {
                    AZ::Color color0 = AZ::Color::CreateZero();
                    color0.FromU32(m_verticesToDraw[i].second);

                    AZ::Color color1 = AZ::Color::CreateZero();
                    color1.FromU32(m_verticesToDraw[i].second);

                    debugDisplay->DrawLine(m_verticesToDraw[i - 1].first, m_verticesToDraw[i].first,
                        color0.GetAsVector4(), color1.GetAsVector4());
                }
            }
            break;
        }
    }

    void RecastNavigationDebugDraw::SetViewableAabb(const AZ::Aabb& cullingAabb)
    {
        m_viewAabb = cullingAabb;
    }

    void RecastNavigationDebugDraw::AddVertex(float x, float y, float z, unsigned color)
    {
        const float temp[3] = { x, y, z };
        const RecastVector3 v = RecastVector3::CreateFromFloatValuesWithoutAxisSwapping(temp);
        m_verticesToDraw.emplace_back(v.AsVector3WithZup(), color);
    }
} // namespace RecastNavigation
