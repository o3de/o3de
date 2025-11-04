/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "Viewport/WhiteBoxManipulatorBounds.h"
#include "Viewport/WhiteBoxViewportConstants.h"
#include "WhiteBoxManipulatorViews.h"

#include <AzCore/std/algorithm.h>
#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <AzToolsFramework/ViewportSelection/EditorSelectionUtil.h>

namespace WhiteBox
{
    AZ_CLASS_ALLOCATOR_IMPL(ManipulatorViewPolygon, AZ::SystemAllocator)
    AZ_CLASS_ALLOCATOR_IMPL(ManipulatorViewEdge, AZ::SystemAllocator)

    static AZStd::vector<AZ::Vector3> TransformToWorldSpace(
        const AZ::Transform& worldFromLocal, const AZStd::vector<AZ::Vector3>& points)
    {
        AZStd::vector<AZ::Vector3> worldPoints;
        worldPoints.reserve(points.size());

        AZStd::transform(
            points.cbegin(), points.cend(), std::back_inserter(worldPoints),
            [&](const AZ::Vector3& point)
            {
                return worldFromLocal.TransformPoint(point);
            });

        return worldPoints;
    }

    ManipulatorViewPolygon::ManipulatorViewPolygon()
        : AzToolsFramework::ManipulatorView(false)
    {
    }

    void ManipulatorViewPolygon::Draw(
        const AzToolsFramework::ManipulatorManagerId managerId,
        [[maybe_unused]] const AzToolsFramework::ManipulatorManagerState& managerState,
        const AzToolsFramework::ManipulatorId manipulatorId, const AzToolsFramework::ManipulatorState& manipulatorState,
        AzFramework::DebugDisplayRequests& debugDisplay, [[maybe_unused]] const AzFramework::CameraState& cameraState,
        [[maybe_unused]] const AzToolsFramework::ViewportInteraction::MouseInteraction& mouseInteraction)
    {
        BoundShapePolygon polygonBounds;
        polygonBounds.m_triangles = TransformToWorldSpace(manipulatorState.m_worldFromLocal, m_triangles);

        // draw fill
        debugDisplay.PushMatrix(m_polygonViewOverlapOffset);
        debugDisplay.DepthTestOn();
        debugDisplay.SetColor(m_fillColor);
        debugDisplay.DrawTriangles(polygonBounds.m_triangles, m_fillColor);

        if (manipulatorState.m_mouseOver)
        {
            debugDisplay.SetColor(m_outlineColor);
            debugDisplay.SetLineWidth(cl_whiteBoxEdgeVisualWidth);

            for (const auto& outline : m_outlines)
            {
                const auto worldOutline = TransformToWorldSpace(manipulatorState.m_worldFromLocal, outline);
                // note: outline may be empty if all edges have been hidden
                if (!worldOutline.empty())
                {
                    // draw outline
                    debugDisplay.DrawPolyLine(worldOutline.data(), aznumeric_caster(worldOutline.size()));
                }
            }
        }

        debugDisplay.DepthTestOff();
        debugDisplay.PopMatrix();

        RefreshBoundInternal(managerId, manipulatorId, polygonBounds);
    }

    ManipulatorViewEdge::ManipulatorViewEdge()
        : AzToolsFramework::ManipulatorView(false)
    {
    }

    void ManipulatorViewEdge::Draw(
        const AzToolsFramework::ManipulatorManagerId managerId,
        [[maybe_unused]] const AzToolsFramework::ManipulatorManagerState& managerState,
        const AzToolsFramework::ManipulatorId manipulatorId, const AzToolsFramework::ManipulatorState& manipulatorState,
        AzFramework::DebugDisplayRequests& debugDisplay, const AzFramework::CameraState& cameraState,
        [[maybe_unused]] const AzToolsFramework::ViewportInteraction::MouseInteraction& mouseInteraction)
    {
        const int mouseOver = manipulatorState.m_mouseOver;

        // draw line
        debugDisplay.PushMatrix(manipulatorState.m_worldFromLocal);
        debugDisplay.DepthTestOn();
        debugDisplay.SetColor(m_color[mouseOver]);
        debugDisplay.SetLineWidth(m_width[mouseOver]);
        debugDisplay.DrawLine(m_start, m_end);
        debugDisplay.DepthTestOff();
        debugDisplay.PopMatrix();

        const auto midPoint = manipulatorState.m_worldFromLocal.TransformPoint((m_end + m_start) * 0.5f);
        const float screenRadius =
            cl_whiteBoxEdgeSelectionWidth * AzToolsFramework::CalculateScreenToWorldMultiplier(midPoint, cameraState);

        // world space positions of manipulator space edge start and end points
        const AZ::Vector3 worldStart = manipulatorState.m_worldFromLocal.TransformPoint(m_start);
        const AZ::Vector3 worldEnd = manipulatorState.m_worldFromLocal.TransformPoint(m_end);

        const auto floatFromBool = [](const bool boolean)
        {
            return boolean ? 1.0f : 0.0f;
        };

        // world space radii of vertex handles at edge start and end points
        // note: the start/end will not be pushed in if the connected vertex handles are hidden
        const float worldStartVertexHandleRadius = cl_whiteBoxVertexManipulatorSize *
            AzToolsFramework::CalculateScreenToWorldMultiplier(worldStart, cameraState) *
            floatFromBool(!m_vertexStartEndHidden[0]);
        const float worldEndVertexHandleRadius = cl_whiteBoxVertexManipulatorSize *
            AzToolsFramework::CalculateScreenToWorldMultiplier(worldEnd, cameraState) *
            floatFromBool(!m_vertexStartEndHidden[1]);

        const AZ::Vector3 worldEdge = worldEnd - worldStart;
        const float worldEdgeLength = worldEdge.GetLength();

        // parametrized t values for start and end points as offset along the edge by
        // the radii of their respective edge vertex handles
        float tStart = 0.0f;
        float tEnd = 1.0f;
        if (!AZ::IsCloseMag(worldEdgeLength, 0.0f))
        {
            tStart = AZStd::clamp(worldStartVertexHandleRadius / worldEdgeLength, 0.0f, 1.0f);
            tEnd = AZStd::clamp((worldEdgeLength - worldEndVertexHandleRadius) / worldEdgeLength, 0.0f, 1.0f);
        }

        // start and end points as offset along the edge by the radii of their respective edge vertex handles
        // note: as the calculations are performed in world space the results are not pixel perfect due to
        // perspective distortion
        const AZ::Vector3 worldStartOffsetByVertexHandle = worldStart + (worldEdge * tStart);
        const AZ::Vector3 worldEndOffsetByVertexHandle = worldStart + (worldEdge * tEnd);

        BoundShapeEdge edgeBounds;
        edgeBounds.m_start = worldStartOffsetByVertexHandle;
        edgeBounds.m_end = worldEndOffsetByVertexHandle;
        edgeBounds.m_radius = screenRadius;
        RefreshBoundInternal(managerId, manipulatorId, edgeBounds);

#if defined(WHITE_BOX_DEBUG_VISUALS)
        debugDisplay.DepthTestOn();
        debugDisplay.SetColor(AZ::Colors::DarkCyan);
        debugDisplay.SetLineWidth(m_width[mouseOver]);
        debugDisplay.DrawLine(
            worldStartOffsetByVertexHandle,
            worldStartOffsetByVertexHandle +
                (AZ::Vector3::CreateAxisZ(0.2f) *
                 AzToolsFramework::CalculateScreenToWorldMultiplier(worldStart, cameraState)));
        debugDisplay.DrawLine(
            worldEndOffsetByVertexHandle,
            worldEndOffsetByVertexHandle +
                (AZ::Vector3::CreateAxisZ(0.2f) *
                 AzToolsFramework::CalculateScreenToWorldMultiplier(worldEnd, cameraState)));
        debugDisplay.DepthTestOff();
#endif
    }

    void ManipulatorViewEdge::SetColor(const AZ::Color& color, const AZ::Color& hoverColor)
    {
        m_color[0] = color;
        m_color[1] = hoverColor;
    }

    void ManipulatorViewEdge::SetWidth(const float width, const float hoverWidth)
    {
        m_width[0] = width;
        m_width[1] = hoverWidth;
    }

    void TranslatePoints(AZStd::vector<AZ::Vector3>& points, const AZ::Vector3& offset)
    {
        AZStd::for_each(
            points.begin(), points.end(),
            [&offset](auto& point)
            {
                point += offset;
            });
    }

    AZStd::unique_ptr<ManipulatorViewPolygon> CreateManipulatorViewPolygon(
        const AZStd::vector<AZ::Vector3>& triangles, const Api::VertexPositionsCollection& outlines)
    {
        AZStd::unique_ptr<ManipulatorViewPolygon> viewPolygon = AZStd::make_unique<ManipulatorViewPolygon>();
        viewPolygon->m_triangles = triangles;
        viewPolygon->m_outlines = outlines;
        return AZStd::move(viewPolygon);
    }

    AZStd::unique_ptr<ManipulatorViewEdge> CreateManipulatorViewEdge(const AZ::Vector3& start, const AZ::Vector3& end)
    {
        AZStd::unique_ptr<ManipulatorViewEdge> viewEdge = AZStd::make_unique<ManipulatorViewEdge>();
        viewEdge->m_start = start;
        viewEdge->m_end = end;
        return AZStd::move(viewEdge);
    }
} // namespace WhiteBox
