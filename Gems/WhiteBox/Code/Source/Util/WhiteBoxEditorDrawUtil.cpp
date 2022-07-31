/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "Util/WhiteBoxEditorDrawUtil.h"
#include "ViewportSelection/EditorSelectionUtil.h"
#include "WhiteBox/WhiteBoxToolApi.h"

#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <Viewport/WhiteBoxViewportConstants.h>

namespace WhiteBox
{
    void DrawFace(
        AzFramework::DebugDisplayRequests& debugDisplay, WhiteBoxMesh* mesh, const Api::PolygonHandle& polygon, const AZ::Color& color)
    {
        AZStd::vector<AZ::Vector3> triangles = Api::PolygonFacesPositions(*mesh, polygon);
        const AZ::Vector3 normal = Api::PolygonNormal(*mesh, polygon);

        if (!triangles.empty())
        {
            // draw selected polygon
            debugDisplay.PushMatrix(AZ::Transform::CreateTranslation(normal * ed_whiteBoxPolygonViewOverlapOffset));

            debugDisplay.SetColor(color);
            debugDisplay.DrawTriangles(triangles, color);

            debugDisplay.PopMatrix();
        }
    }

    void DrawOutline(
        AzFramework::DebugDisplayRequests& debugDisplay, WhiteBoxMesh* mesh, const Api::PolygonHandle& polygon, const AZ::Color& color)
    {
        Api::VertexPositionsCollection outlines = Api::PolygonBorderVertexPositions(*mesh, polygon);
        if (!outlines.empty())
        {
            // draw outline
            debugDisplay.SetColor(color);
            debugDisplay.SetLineWidth(cl_whiteBoxEdgeVisualWidth);
            for (const auto& outline : outlines)
            {
                debugDisplay.DrawPolyLine(outline.data(), aznumeric_caster(outline.size()));
            }
        }
    }

    void DrawEdge(AzFramework::DebugDisplayRequests& debugDisplay, WhiteBoxMesh* mesh, const Api::EdgeHandle& edge, const AZ::Color& color)
    {
        auto vertexEdgePosition = Api::EdgeVertexPositions(*mesh, edge);
        debugDisplay.SetColor(color);
        debugDisplay.SetLineWidth(cl_whiteBoxEdgeVisualWidth);
        debugDisplay.DrawLine(vertexEdgePosition[0], vertexEdgePosition[1]);
    }
    
    void DrawPoints(
        AzFramework::DebugDisplayRequests& debugDisplay,
        WhiteBoxMesh* mesh,
        const AZ::Transform& worldFromLocal,
        const AzFramework::ViewportInfo& viewportInfo,
        const AZStd::span<Api::VertexHandle>& verts,
        const AZ::Color& color)
    {
        debugDisplay.SetColor(color);
        for (auto& vert : verts)
        {
            const auto vertPos = Api::VertexPosition(*mesh, vert);
            const AzFramework::CameraState cameraState = AzToolsFramework::GetCameraState(viewportInfo.m_viewportId);
            const float radius =
                cl_whiteBoxVertexManipulatorSize * AzToolsFramework::CalculateScreenToWorldMultiplier(worldFromLocal.TransformPoint(vertPos), cameraState);

            debugDisplay.DrawBall(vertPos, radius);
        }
    }
} // namespace WhiteBox
