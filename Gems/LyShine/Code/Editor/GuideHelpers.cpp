/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "EditorCommon.h"
#include "GuideHelpers.h"
#include "CanvasHelpers.h"
#include <LyShine/Bus/UiEditorCanvasBus.h>

namespace GuideHelpers
{
    bool PickGuide(AZ::EntityId canvasEntityId, const AZ::Vector2& point, bool& guideIsVertical, int& guideIndex)
    {
        AZ::Matrix4x4 transform;
        UiCanvasBus::EventResult(transform, canvasEntityId, &UiCanvasBus::Events::GetCanvasToViewportMatrix);

        const float pickTolerance = 5.0f;
        float guideDistance = pickTolerance + 1.0f;

        // search horizontal guide lines for closest match
        AZStd::vector<float> horizontalGuidePositions;
        UiEditorCanvasBus::EventResult(horizontalGuidePositions, canvasEntityId, &UiEditorCanvasBus::Events::GetHorizontalGuidePositions);
        for (int index = 0; index < horizontalGuidePositions.size(); ++index)
        {
            AZ::Vector3 canvasPoint(0.0f, horizontalGuidePositions[index], 0.0f);
            AZ::Vector3 viewportPoint = transform * canvasPoint;

            float distance = fabsf(viewportPoint.GetY() - point.GetY());
            if (distance < guideDistance)
            {
                guideDistance = distance;
                guideIndex = index;
                guideIsVertical = false;
            }
        }

        // search vertical guide lines for closest match
        AZStd::vector<float> verticalGuidePositions;
        UiEditorCanvasBus::EventResult(verticalGuidePositions, canvasEntityId, &UiEditorCanvasBus::Events::GetVerticalGuidePositions);
        for (int index = 0; index < verticalGuidePositions.size(); ++index)

        {
            AZ::Vector3 canvasPoint(verticalGuidePositions[index], 0.0f, 0.0f);
            AZ::Vector3 viewportPoint = transform * canvasPoint;

            float distance = fabsf(viewportPoint.GetX() - point.GetX());
            if (distance < guideDistance)
            {
                guideDistance = distance;
                guideIndex = index;
                guideIsVertical = true;
            }
        }

        // return true if the closest match is within the pick distance
        // the output parameters guideIsVertical and guideIndex are already set
        return (guideDistance <= pickTolerance);
    }

    float GetGuidePosition(AZ::EntityId canvasEntityId, bool guideIsVertical, int guideIndex)
    {
        AZStd::vector<float> guidePositions;
        if (guideIsVertical)
        {
            UiEditorCanvasBus::EventResult(guidePositions, canvasEntityId, &UiEditorCanvasBus::Events::GetVerticalGuidePositions);
        }
        else
        {
            UiEditorCanvasBus::EventResult(guidePositions, canvasEntityId, &UiEditorCanvasBus::Events::GetHorizontalGuidePositions);
        }

        float pos = 0.0f;
        if (guideIndex < guidePositions.size())
        {
            pos = guidePositions[guideIndex];
        }

        return pos;
    }

    void SetGuidePosition(AZ::EntityId canvasEntityId, bool guideIsVertical, int guideIndex, float pos)
    {
        if (guideIsVertical)
        {
            UiEditorCanvasBus::Event(canvasEntityId, &UiEditorCanvasBus::Events::SetVerticalGuidePosition, guideIndex, pos);
        }
        else
        {
            UiEditorCanvasBus::Event(canvasEntityId, &UiEditorCanvasBus::Events::SetHorizontalGuidePosition, guideIndex, pos);
        }
    }

    void SetGuidePosition(AZ::EntityId canvasEntityId, bool guideIsVertical, int guideIndex, const AZ::Vector2& pos)
    {
        if (guideIsVertical)
        {
            UiEditorCanvasBus::Event(canvasEntityId, &UiEditorCanvasBus::Events::SetVerticalGuidePosition, guideIndex, pos.GetX());
        }
        else
        {
            UiEditorCanvasBus::Event(canvasEntityId, &UiEditorCanvasBus::Events::SetHorizontalGuidePosition, guideIndex, pos.GetY());
        }
    }

    void RemoveGuide(AZ::EntityId canvasEntityId, bool guideIsVertical, int guideIndex)
    {
        if (guideIsVertical)
        {
            UiEditorCanvasBus::Event(canvasEntityId, &UiEditorCanvasBus::Events::RemoveVerticalGuide, guideIndex);
        }
        else
        {
            UiEditorCanvasBus::Event(canvasEntityId, &UiEditorCanvasBus::Events::RemoveHorizontalGuide, guideIndex);
        }
    }

    void SetGuidesAreLocked(AZ::EntityId canvasEntityId, bool areLocked)
    {
        UiEditorCanvasBus::Event(canvasEntityId, &UiEditorCanvasBus::Events::SetGuidesAreLocked, areLocked);
    }

    bool AreGuidesLocked(AZ::EntityId canvasEntityId)
    {
        bool areLocked = false;
        UiEditorCanvasBus::EventResult(areLocked, canvasEntityId, &UiEditorCanvasBus::Events::GetGuidesAreLocked);
        return areLocked;
    }

    void DrawGuideLines(AZ::EntityId canvasEntityId, ViewportWidget* viewport, Draw2dHelper& draw2d)
    {
        AZ::Matrix4x4 transform;
        UiCanvasBus::EventResult(transform, canvasEntityId, &UiCanvasBus::Events::GetCanvasToViewportMatrix);

        AZ::Vector2 viewportSize = viewport->GetRenderViewportSize();

        AZ::Color guideColor;
        UiEditorCanvasBus::EventResult(guideColor, canvasEntityId, &UiEditorCanvasBus::Events::GetGuideColor);

        // draw horizontal guide lines
        AZStd::vector<float> horizontalGuidePositions;
        UiEditorCanvasBus::EventResult(horizontalGuidePositions, canvasEntityId, &UiEditorCanvasBus::Events::GetHorizontalGuidePositions);
        for (float pos : horizontalGuidePositions)
        {
            AZ::Vector3 canvasPoint(0.0f, pos, 0.0f);
            AZ::Vector3 viewportPoint = transform * canvasPoint;
            AZ::Vector2 start(0, viewportPoint.GetY());
            AZ::Vector2 end(viewportSize.GetX(), viewportPoint.GetY());
            draw2d.DrawLine(start, end, guideColor);
        }

        // draw vertical guide lines
        AZStd::vector<float> verticalGuidePositions;
        UiEditorCanvasBus::EventResult(verticalGuidePositions, canvasEntityId, &UiEditorCanvasBus::Events::GetVerticalGuidePositions);
        for (float pos : verticalGuidePositions)
        {
            AZ::Vector3 canvasPoint(pos, 0.0f, 0.0f);
            AZ::Vector3 viewportPoint = transform * canvasPoint;
            AZ::Vector2 start(viewportPoint.GetX(), 0.0f);
            AZ::Vector2 end(viewportPoint.GetX(), viewportSize.GetX());
            draw2d.DrawLine(start, end, guideColor);
        }
    }

    void DrawGhostGuideLine(Draw2dHelper& draw2d, AZ::EntityId canvasEntityId, bool guideIsVertical, ViewportWidget* viewport, const AZ::Vector2& canvasPoint)
    {
        AZ::Vector2 viewportPoint = CanvasHelpers::GetViewportPoint(canvasEntityId, canvasPoint);
        AZ::Vector2 viewportSize = viewport->GetRenderViewportSize();

        // the line is drawn as the inverse of the background color
        AZ::Color guideColor(1.0f, 1.0f, 1.0f, 1.0f);

        IDraw2d::RenderState renderState;
        renderState.m_blendState.m_blendSource = AZ::RHI::BlendFactor::ColorDestInverse;
        renderState.m_blendState.m_blendDest = AZ::RHI::BlendFactor::Zero;

        // Draw the guide line
        if (guideIsVertical)
        {
            AZ::Vector2 start(viewportPoint.GetX(), 0);
            AZ::Vector2 end(viewportPoint.GetX(), viewportSize.GetY());
            draw2d.DrawLine(start, end, guideColor, IDraw2d::Rounding::Nearest, renderState);
        }
        else
        {
            AZ::Vector2 start(0, viewportPoint.GetY());
            AZ::Vector2 end(viewportSize.GetX(), viewportPoint.GetY());
            draw2d.DrawLine(start, end, guideColor, IDraw2d::Rounding::Nearest, renderState);
        }
    }

    void DrawGuidePosTextDisplay(Draw2dHelper& draw2d, bool guideIsVertical, float guidePos, const ViewportWidget* viewport)
    {
        const size_t displayTextBufSize = 32;
        char displayTextBuf[displayTextBufSize];

        // Build the display text
        if (guideIsVertical)
        {
            azsnprintf(displayTextBuf, displayTextBufSize, "X = %.1f", guidePos);
        }
        else
        {
            azsnprintf(displayTextBuf, displayTextBufSize, "Y = %.1f", guidePos);
        }

        // display the pixel value in canvas space slightly offset from the mouse cursor
        ViewportHelpers::DrawCursorText(displayTextBuf, draw2d, viewport);
    }

}   // namespace CanvasHelpers
