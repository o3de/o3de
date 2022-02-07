/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ManipulatorSnapping.h"

#include <AzCore/Console/Console.h>
#include <AzCore/Math/Internal/VectorConversions.inl>
#include <AzCore/std/numeric.h>
#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <AzToolsFramework/Maths/TransformUtils.h>
#include <AzToolsFramework/Viewport/ViewportMessages.h>

AZ_CVAR(
    AZ::Color,
    cl_viewportGridMainColor,
    AZ::Color::CreateFromRgba(26, 26, 26, 127),
    nullptr,
    AZ::ConsoleFunctorFlags::Null,
    "Main color for snapping grid");
AZ_CVAR(
    AZ::Color,
    cl_viewportGridFadeColor,
    AZ::Color::CreateFromRgba(127, 127, 127, 0),
    nullptr,
    AZ::ConsoleFunctorFlags::Null,
    "Fade color for snapping grid");
AZ_CVAR(int, cl_viewportGridSquareCount, 20, nullptr, AZ::ConsoleFunctorFlags::Null, "Number of grid squares for snapping grid");
AZ_CVAR(float, cl_viewportGridLineWidth, 1.0f, nullptr, AZ::ConsoleFunctorFlags::Null, "Width of grid lines for snapping grid");
AZ_CVAR(
    float,
    cl_viewportFadeLineDistanceScale,
    1.0f,
    nullptr,
    AZ::ConsoleFunctorFlags::Null,
    "The scale to be applied to the line that fades out (scales the current gridSize)");

namespace AzToolsFramework
{
    GridSnapParameters::GridSnapParameters(const bool gridSnap, const float gridSize)
        : m_gridSnap(gridSnap)
        , m_gridSize(gridSize)
    {
    }

    ManipulatorInteraction BuildManipulatorInteraction(
        const AZ::Transform& worldFromLocal,
        const AZ::Vector3& nonUniformScale,
        const AZ::Vector3& worldRayOrigin,
        const AZ::Vector3& worldRayDirection)
    {
        const AZ::Transform worldFromLocalUniform = AzToolsFramework::TransformUniformScale(worldFromLocal);
        const AZ::Transform localFromWorldUniform = worldFromLocalUniform.GetInverse();

        return { localFromWorldUniform.TransformPoint(worldRayOrigin),
                 TransformDirectionNoScaling(localFromWorldUniform, worldRayDirection), NonUniformScaleReciprocal(nonUniformScale),
                 ScaleReciprocal(worldFromLocalUniform) };
    }

    struct SnapAdjustment
    {
        float m_existingSnapDistance; //!< How far to snap up or down to align to the grid.
        float m_nextSnapDistance; //!< The snap increment (will return full signed value (grid size) when distance
                                  //!< moved is greater than half of the grid size in either direction).
    };

    static SnapAdjustment CalculateSnapDistance(const AZ::Vector3& unsnappedPosition, const AZ::Vector3& axis, const float size)
    {
        // calculate total distance along axis
        const float axisDistance = axis.Dot(unsnappedPosition);
        // round to nearest step size
        const float snappedAxisDistance = floorf((axisDistance / size) + 0.5f) * size;

        return { axisDistance, snappedAxisDistance };
    }

    AZ::Vector3 CalculateSnappedOffset(const AZ::Vector3& unsnappedPosition, const AZ::Vector3& axis, const float size)
    {
        const auto snapAdjustment = CalculateSnapDistance(unsnappedPosition, axis, size);
        // return offset along axis to snap to step size
        return axis * (snapAdjustment.m_nextSnapDistance - snapAdjustment.m_existingSnapDistance);
    }

    AZ::Vector3 CalculateSnappedAmount(const AZ::Vector3& unsnappedPosition, const AZ::Vector3& axis, const float size)
    {
        const auto snapAdjustment = CalculateSnapDistance(unsnappedPosition, axis, size);
        // return offset along axis to snap to step size
        return axis * snapAdjustment.m_nextSnapDistance;
    }

    AZ::Vector3 CalculateSnappedOffset(
        const AZ::Vector3& unsnappedPosition, const AZ::Vector3* snapAxes, const size_t snapAxesCount, const float size)
    {
        return AZStd::accumulate(
            snapAxes, snapAxes + snapAxesCount, AZ::Vector3::CreateZero(),
            [&unsnappedPosition, size](AZ::Vector3 acc, const AZ::Vector3& snapAxis)
            {
                acc += CalculateSnappedOffset(unsnappedPosition, snapAxis, size);
                return acc;
            });
    }

    AZ::Vector3 CalculateSnappedPosition(
        const AZ::Vector3& unsnappedPosition, const AZ::Vector3* snapAxes, const size_t snapAxesCount, const float size)
    {
        return unsnappedPosition + CalculateSnappedOffset(unsnappedPosition, snapAxes, snapAxesCount, size);
    }

    AZ::Vector3 CalculateSnappedTerrainPosition(
        const AZ::Vector3& worldSurfacePosition, const AZ::Transform& worldFromLocal, const int viewportId, const float size)
    {
        const AZ::Transform localFromWorld = worldFromLocal.GetInverse();
        const AZ::Vector3 localSurfacePosition = localFromWorld.TransformPoint(worldSurfacePosition);

        // snap in xy plane
        AZ::Vector3 localSnappedSurfacePosition = localSurfacePosition +
            CalculateSnappedOffset(localSurfacePosition, AZ::Vector3::CreateAxisX(), size) +
            CalculateSnappedOffset(localSurfacePosition, AZ::Vector3::CreateAxisY(), size);

        // find terrain height at xy snapped location
        float terrainHeight = 0.0f;
        ViewportInteraction::MainEditorViewportInteractionRequestBus::EventResult(
            terrainHeight, viewportId, &ViewportInteraction::MainEditorViewportInteractionRequestBus::Events::TerrainHeight,
            Vector3ToVector2(worldFromLocal.TransformPoint(localSnappedSurfacePosition)));

        // set snapped z value to terrain height
        AZ::Vector3 localTerrainHeight = localFromWorld.TransformPoint(AZ::Vector3(0.0f, 0.0f, terrainHeight));
        localSnappedSurfacePosition.SetZ(localTerrainHeight.GetZ());

        return localSnappedSurfacePosition;
    }

    bool GridSnapping(const int viewportId)
    {
        bool snapping = false;
        ViewportInteraction::ViewportSettingsRequestBus::EventResult(
            snapping, viewportId, &ViewportInteraction::ViewportSettingsRequestBus::Events::GridSnappingEnabled);

        return snapping;
    }

    float GridSize(const int viewportId)
    {
        float gridSize = 0.0f;
        ViewportInteraction::ViewportSettingsRequestBus::EventResult(
            gridSize, viewportId, &ViewportInteraction::ViewportSettingsRequestBus::Events::GridSize);

        return gridSize;
    }

    GridSnapParameters GridSnapSettings(const int viewportId)
    {
        bool snapping = GridSnapping(viewportId);
        const float gridSize = GridSize(viewportId);
        if (AZ::IsClose(
                gridSize, 0.0f, 1e-2f)) // Same threshold value as min value for m_spinBox in SnapToWidget constructor in MainWindow.cpp
        {
            snapping = false;
        }

        return GridSnapParameters(snapping, gridSize);
    }

    bool AngleSnapping(const int viewportId)
    {
        bool snapping = false;
        ViewportInteraction::ViewportSettingsRequestBus::EventResult(
            snapping, viewportId, &ViewportInteraction::ViewportSettingsRequestBus::Events::AngleSnappingEnabled);

        return snapping;
    }

    float AngleStep(const int viewportId)
    {
        float angle = 0.0f;
        ViewportInteraction::ViewportSettingsRequestBus::EventResult(
            angle, viewportId, &ViewportInteraction::ViewportSettingsRequestBus::Events::AngleStep);

        return angle;
    }

    bool ShowingGrid(const int viewportId)
    {
        bool show = false;
        ViewportInteraction::ViewportSettingsRequestBus::EventResult(
            show, viewportId, &ViewportInteraction::ViewportSettingsRequestBus::Events::ShowGrid);

        return show;
    }

    void DrawSnappingGrid(AzFramework::DebugDisplayRequests& debugDisplay, const AZ::Transform& worldFromLocal, const float squareSize)
    {
        debugDisplay.PushMatrix(worldFromLocal);

        debugDisplay.SetLineWidth(cl_viewportGridLineWidth);

        const int gridSquareCount = cl_viewportGridSquareCount;
        AZStd::vector<AZ::Vector3> lines;
        lines.reserve((gridSquareCount + 1) * 2);

        const AZ::Vector4 gridMainColor = static_cast<AZ::Color>(cl_viewportGridMainColor).GetAsVector4();
        const AZ::Vector4 gridFadeColor = static_cast<AZ::Color>(cl_viewportGridFadeColor).GetAsVector4();

        const float halfGridSquareCount = float(gridSquareCount) * 0.5f;
        const float halfGridSize = halfGridSquareCount * squareSize;
        const float fadeLineLength = cl_viewportFadeLineDistanceScale * squareSize;
        for (size_t lineIndex = 0; lineIndex <= gridSquareCount; ++lineIndex)
        {
            const float lineOffset = -halfGridSize + (lineIndex * squareSize);

            // draw the faded end parts of the grid lines
            debugDisplay.DrawLine(
                AZ::Vector3(lineOffset, -halfGridSize, 0.0f), AZ::Vector3(lineOffset, -(halfGridSize + fadeLineLength), 0.0f),
                gridMainColor, gridFadeColor);
            debugDisplay.DrawLine(
                AZ::Vector3(lineOffset, halfGridSize, 0.0f), AZ::Vector3(lineOffset, (halfGridSize + fadeLineLength), 0.0f), gridMainColor,
                gridFadeColor);
            debugDisplay.DrawLine(
                AZ::Vector3(-halfGridSize, lineOffset, 0.0f), AZ::Vector3(-(halfGridSize + fadeLineLength), lineOffset, 0.0f),
                gridMainColor, gridFadeColor);
            debugDisplay.DrawLine(
                AZ::Vector3(halfGridSize, lineOffset, 0.0f), AZ::Vector3((halfGridSize + fadeLineLength), lineOffset, 0.0f), gridMainColor,
                gridFadeColor);

            // build a vector of the main grid lines to draw (start and end positions)
            lines.push_back(AZ::Vector3(lineOffset, -halfGridSize, 0.0f));
            lines.push_back(AZ::Vector3(lineOffset, halfGridSize, 0.0f));
            lines.push_back(AZ::Vector3(-halfGridSize, lineOffset, 0.0f));
            lines.push_back(AZ::Vector3(halfGridSize, lineOffset, 0.0f));
        }

        debugDisplay.DrawLines(lines, cl_viewportGridMainColor);

        // restore original width
        debugDisplay.SetLineWidth(1.0f);

        debugDisplay.PopMatrix();
    }
} // namespace AzToolsFramework
