/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "EditorCommon.h"

ViewportAnchor::ViewportAnchor()
    : m_anchorWhole(new ViewportIcon("Editor/Icons/Viewport/Anchor_Whole.tif"))
    , m_anchorLeft(new ViewportIcon("Editor/Icons/Viewport/Anchor_Left.tif"))
    , m_anchorLeftTop(new ViewportIcon("Editor/Icons/Viewport/Anchor_TopLeft.tif"))
    , m_dottedLine(new ViewportIcon("Editor/Icons/Viewport/DottedLine.tif"))
{
}

ViewportAnchor::~ViewportAnchor()
{
}

void ViewportAnchor::Draw(Draw2dHelper& draw2d, AZ::Entity* element, bool drawUnTransformedRect, bool drawAnchorLines, bool drawLinesToParent,
    bool anchorInteractionEnabled, ViewportHelpers::SelectedAnchors highlightedAnchors) const
{
    if (!element || ViewportHelpers::IsControlledByLayout(element))
    {
        // Don't draw anything if there is no element or it's controlled by a layout.
        return;
    }

    // check that the element is using transform2d - if not then can't draw the anchors
    if (!UiTransform2dBus::FindFirstHandler(element->GetId()))
    {
        return;
    }

    AZ::Entity* parentElement = EntityHelpers::GetParentElement(element);

    // get the anchors from the element's transform component
    UiTransform2dInterface::Anchors anchors;
    UiTransform2dBus::EventResult(anchors, element->GetId(), &UiTransform2dBus::Events::GetAnchors);
    // get the parent element's pre-transform points and its transform.
    // The anchors are in terms of the parent's space
    UiTransformInterface::RectPoints parentPoints;
    UiTransformBus::Event(parentElement->GetId(), &UiTransformBus::Events::GetCanvasSpacePointsNoScaleRotate, parentPoints);

    AZ::Vector2 parentSize = parentPoints.GetAxisAlignedSize();

    AZ::Matrix4x4 parentTransform;
    UiTransformBus::Event(parentElement->GetId(), &UiTransformBus::Events::GetTransformToViewport, parentTransform);

    UiTransformInterface::RectPoints elemRect;
    UiTransformBus::Event(element->GetId(), &UiTransformBus::Events::GetCanvasSpacePointsNoScaleRotate, elemRect);

    // Here we optionally draw a rect outline, either the element's rect or the parent element's
    // rect depending on the situation
    if (drawUnTransformedRect || drawLinesToParent)
    {
        UiTransformInterface::RectPoints rectPointsToDraw;
        if (drawLinesToParent)
        {
            // If we are going to draw distance lines to the parent then draw the parent rectangle
            rectPointsToDraw = parentPoints.Transform(parentTransform);
        }
        else
        {
            // drawUnTransformedRect must be true
            // we draw the outline of this element's rect before its local rotate and scale
            // the untransformed rect we want to draw has all the parents' transforms but not this element's transforms
            // so transform the NoScaleRotate with the parent transform
            rectPointsToDraw = elemRect.Transform(parentTransform);
        }

        AZ::Color rectColor(1.0f, 1.0f, 1.0f, 0.2f);

        draw2d.DrawLine(rectPointsToDraw.TopLeft(),     rectPointsToDraw.TopRight(),    rectColor); // top
        draw2d.DrawLine(rectPointsToDraw.TopRight(),    rectPointsToDraw.BottomRight(), rectColor); // right
        draw2d.DrawLine(rectPointsToDraw.BottomRight(), rectPointsToDraw.BottomLeft(),  rectColor); // bottom
        draw2d.DrawLine(rectPointsToDraw.BottomLeft(),  rectPointsToDraw.TopLeft(),     rectColor); // left
    }

    if (drawLinesToParent)
    {
        // When moving the anchors (or if we are highlighting them) we draw lines from the anchors
        // to the parent rectangle to make it clear that the anchors are normalized distances from
        // the edges of the parent
        DrawAnchorToParentLines(draw2d, anchors, parentPoints, parentTransform, highlightedAnchors);

        // if moving the anchors we do not want to draw the lines from the anchors to the element rect
        drawAnchorLines = false;
    }

    // we draw the anchors in a different color if anchor interaction is disabled
    AZ::Color anchorColor = (anchorInteractionEnabled) ? ViewportHelpers::anchorColor : ViewportHelpers::anchorColorDisabled;

    // the anchors we draw depend on whether the left/right and top/bottom anchors are together or split apart
    if (anchors.m_left == anchors.m_right)
    {
        if (anchors.m_top == anchors.m_bottom)
        {
            // all anchors together
            AZ::Vector2 anchorPos = ViewportHelpers::ComputeAnchorPoint(parentPoints.TopLeft(), parentSize, anchors.m_left, anchors.m_top);

            if (drawAnchorLines)
            {
                AZ::Vector2 pivot;
                UiTransformBus::EventResult(pivot, element->GetId(), &UiTransformBus::Events::GetCanvasSpacePivotNoScaleRotate);
                m_dottedLine->DrawAnchorLines(draw2d, anchorPos, pivot, parentTransform, true, true, true);
            }

            m_anchorWhole->Draw(draw2d, anchorPos, parentTransform, 0.0f, anchorColor);
        }
        else
        {
            // stretching vertically
            AZ::Vector2 topAnchorPos    = ViewportHelpers::ComputeAnchorPoint(parentPoints.TopLeft(), parentSize, anchors.m_left, anchors.m_top);
            AZ::Vector2 bottomAnchorPos = ViewportHelpers::ComputeAnchorPoint(parentPoints.TopLeft(), parentSize, anchors.m_left, anchors.m_bottom);

            if (drawAnchorLines)
            {
                AZ::Vector2 pivot;
                UiTransformBus::EventResult(pivot, element->GetId(), &UiTransformBus::Events::GetCanvasSpacePivotNoScaleRotate);

                AZ::Vector2 topTarget;
                AZ::Vector2 bottomTarget;
                ViewportHelpers::GetVerticalTargetPoints(elemRect, topAnchorPos.GetX(), topTarget, bottomTarget);

                m_dottedLine->DrawAnchorLines(draw2d, topAnchorPos, topTarget, parentTransform, true, false, true);
                m_dottedLine->DrawAnchorLines(draw2d, bottomAnchorPos, bottomTarget, parentTransform, true, false, true);
                m_dottedLine->DrawAnchorLinesSplit(draw2d, topAnchorPos, bottomAnchorPos, pivot, parentTransform, false);
            }

            m_anchorLeft->Draw(draw2d, topAnchorPos, parentTransform, 90.0f, anchorColor);
            m_anchorLeft->Draw(draw2d, bottomAnchorPos, parentTransform, -90.0f, anchorColor);
        }
    }
    else
    {
        if (anchors.m_top == anchors.m_bottom)
        {
            // stretching horizontally
            AZ::Vector2 leftAnchorPos  = ViewportHelpers::ComputeAnchorPoint(parentPoints.TopLeft(), parentSize, anchors.m_left, anchors.m_top);
            AZ::Vector2 rightAnchorPos = ViewportHelpers::ComputeAnchorPoint(parentPoints.TopLeft(), parentSize, anchors.m_right, anchors.m_top);

            if (drawAnchorLines)
            {
                AZ::Vector2 pivot;
                UiTransformBus::EventResult(pivot, element->GetId(), &UiTransformBus::Events::GetCanvasSpacePivotNoScaleRotate);

                AZ::Vector2 leftTarget;
                AZ::Vector2 rightTarget;
                ViewportHelpers::GetHorizTargetPoints(elemRect, leftAnchorPos.GetY(), leftTarget, rightTarget);

                m_dottedLine->DrawAnchorLines(draw2d, leftAnchorPos, leftTarget, parentTransform, false, true, false);
                m_dottedLine->DrawAnchorLines(draw2d, rightAnchorPos, rightTarget, parentTransform, false, true, false);
                m_dottedLine->DrawAnchorLinesSplit(draw2d, leftAnchorPos, rightAnchorPos, pivot, parentTransform, true);
            }

            m_anchorLeft->Draw(draw2d, leftAnchorPos, parentTransform, 0.0f, anchorColor);
            m_anchorLeft->Draw(draw2d, rightAnchorPos, parentTransform, 180.0f, anchorColor);
        }
        else
        {
            // stretching in both directions
            AZ::Vector2 topLeftAnchorPos     = ViewportHelpers::ComputeAnchorPoint(parentPoints.TopLeft(), parentSize, anchors.m_left, anchors.m_top);
            AZ::Vector2 topRightAnchorPos    = ViewportHelpers::ComputeAnchorPoint(parentPoints.TopLeft(), parentSize, anchors.m_right, anchors.m_top);
            AZ::Vector2 bottomRightAnchorPos = ViewportHelpers::ComputeAnchorPoint(parentPoints.TopLeft(), parentSize, anchors.m_right, anchors.m_bottom);
            AZ::Vector2 bottomLeftAnchorPos  = ViewportHelpers::ComputeAnchorPoint(parentPoints.TopLeft(), parentSize, anchors.m_left, anchors.m_bottom);

            if (drawAnchorLines)
            {
                AZ::Vector2 pivot;
                UiTransformBus::EventResult(pivot, element->GetId(), &UiTransformBus::Events::GetCanvasSpacePivotNoScaleRotate);

                AZ::Vector2 anchorMidpoint = (topLeftAnchorPos + bottomRightAnchorPos) * 0.5f;

                AZ::Vector2 leftTarget;
                AZ::Vector2 rightTarget;
                ViewportHelpers::GetHorizTargetPoints(elemRect, anchorMidpoint.GetY(), leftTarget, rightTarget);

                AZ::Vector2 topTarget;
                AZ::Vector2 bottomTarget;
                ViewportHelpers::GetVerticalTargetPoints(elemRect, anchorMidpoint.GetX(), topTarget, bottomTarget);

                m_dottedLine->DrawAnchorLinesSplit(draw2d, topLeftAnchorPos, topRightAnchorPos, topTarget, parentTransform, true);
                m_dottedLine->DrawAnchorLinesSplit(draw2d, bottomLeftAnchorPos, bottomRightAnchorPos, bottomTarget, parentTransform, true);
                m_dottedLine->DrawAnchorLinesSplit(draw2d, topLeftAnchorPos, bottomLeftAnchorPos, leftTarget, parentTransform, false);
                m_dottedLine->DrawAnchorLinesSplit(draw2d, topRightAnchorPos, bottomRightAnchorPos, rightTarget, parentTransform, false);
            }

            m_anchorLeftTop->Draw(draw2d, topLeftAnchorPos, parentTransform, 0.0f, anchorColor);
            m_anchorLeftTop->Draw(draw2d, topRightAnchorPos, parentTransform, 90.0f, anchorColor);
            m_anchorLeftTop->Draw(draw2d, bottomRightAnchorPos, parentTransform, 180.0f, anchorColor);
            m_anchorLeftTop->Draw(draw2d, bottomLeftAnchorPos, parentTransform, -90.0f, anchorColor);
        }
    }

    // If the user is hovering over any anchors, highlight them
    if (highlightedAnchors.All())
    {
        AZ::Vector2 anchorPos = ViewportHelpers::ComputeAnchorPoint(parentPoints.TopLeft(), parentSize, anchors.m_left, anchors.m_top);
        m_anchorWhole->Draw(draw2d, anchorPos, parentTransform, 0.0f, ViewportHelpers::highlightColor);
    }
    else if (highlightedAnchors.TopLeft())
    {
        AZ::Vector2 topLeftAnchorPos = ViewportHelpers::ComputeAnchorPoint(parentPoints.TopLeft(), parentSize, anchors.m_left, anchors.m_top);
        m_anchorLeftTop->Draw(draw2d, topLeftAnchorPos, parentTransform, 0.0f, ViewportHelpers::highlightColor);
    }
    else if (highlightedAnchors.TopRight())
    {
        AZ::Vector2 topRightAnchorPos = ViewportHelpers::ComputeAnchorPoint(parentPoints.TopLeft(), parentSize, anchors.m_right, anchors.m_top);
        m_anchorLeftTop->Draw(draw2d, topRightAnchorPos, parentTransform, 90.0f, ViewportHelpers::highlightColor);
    }
    else if (highlightedAnchors.BottomRight())
    {
        AZ::Vector2 bottomRightAnchorPos = ViewportHelpers::ComputeAnchorPoint(parentPoints.TopLeft(), parentSize, anchors.m_right, anchors.m_bottom);
        m_anchorLeftTop->Draw(draw2d, bottomRightAnchorPos, parentTransform, 180.0f, ViewportHelpers::highlightColor);
    }
    else if (highlightedAnchors.BottomLeft())
    {
        AZ::Vector2 bottomLeftAnchorPos = ViewportHelpers::ComputeAnchorPoint(parentPoints.TopLeft(), parentSize, anchors.m_left, anchors.m_bottom);
        m_anchorLeftTop->Draw(draw2d, bottomLeftAnchorPos, parentTransform, -90.0f, ViewportHelpers::highlightColor);
    }
    else if (highlightedAnchors.m_top)
    {
        AZ::Vector2 topAnchorPos = ViewportHelpers::ComputeAnchorPoint(parentPoints.TopLeft(), parentSize, anchors.m_left, anchors.m_top);
        m_anchorLeft->Draw(draw2d, topAnchorPos, parentTransform, 90.0f, ViewportHelpers::highlightColor);
    }
    else if (highlightedAnchors.m_bottom)
    {
        AZ::Vector2 bottomAnchorPos = ViewportHelpers::ComputeAnchorPoint(parentPoints.TopLeft(), parentSize, anchors.m_left, anchors.m_bottom);
        m_anchorLeft->Draw(draw2d, bottomAnchorPos, parentTransform, -90.0f, ViewportHelpers::highlightColor);
    }
    else if (highlightedAnchors.m_left)
    {
        AZ::Vector2 leftAnchorPos = ViewportHelpers::ComputeAnchorPoint(parentPoints.TopLeft(), parentSize, anchors.m_left, anchors.m_top);
        m_anchorLeft->Draw(draw2d, leftAnchorPos, parentTransform, 0.0f, ViewportHelpers::highlightColor);
    }
    else if (highlightedAnchors.m_right)
    {
        AZ::Vector2 rightAnchorPos = ViewportHelpers::ComputeAnchorPoint(parentPoints.TopLeft(), parentSize, anchors.m_right, anchors.m_top);
        m_anchorLeft->Draw(draw2d, rightAnchorPos, parentTransform, 180.0f, ViewportHelpers::highlightColor);
    }
}

void ViewportAnchor::DrawAnchorToParentLines(Draw2dHelper& draw2d, const UiTransform2dInterface::Anchors& anchors,
    const UiTransformInterface::RectPoints& parentPoints, const AZ::Matrix4x4& transform,
    ViewportHelpers::SelectedAnchors highlightedAnchors) const
{
    bool drawHoriz = true;
    bool drawVert = true;

    // if we have just one side of the anchors selected we only draw one line
    if (!highlightedAnchors.All() && highlightedAnchors.Any())
    {
        if (!(highlightedAnchors.m_left || highlightedAnchors.m_right))
        {
            // we are dragging up/down only
            drawHoriz = false;
        }
        else if (!(highlightedAnchors.m_top || highlightedAnchors.m_bottom))
        {
            // we are dragging left/right only
            drawVert = false;
        }
    }

    AZ::Vector2 parentSize = parentPoints.GetAxisAlignedSize();

    float horizAnchorVal = (highlightedAnchors.m_left) ? anchors.m_left : anchors.m_right;
    float vertAnchorVal = (highlightedAnchors.m_top) ? anchors.m_top : anchors.m_bottom;

    AZ::Vector2 anchorPos = ViewportHelpers::ComputeAnchorPoint(parentPoints.TopLeft(), parentSize, horizAnchorVal, vertAnchorVal);

    if (drawHoriz)
    {
        // draw distance line horizontally from the anchor pos to the left edge of the parent rect
        // The distance value is the anchor value (0 - 1 range)
        AZ::Vector2 targetPosLeft(parentPoints.TopLeft().GetX(), anchorPos.GetY());
        m_dottedLine->DrawDistanceLineWithTransform(draw2d, anchorPos, targetPosLeft, transform, horizAnchorVal * 100.0f, "%");
    }

    if (drawVert)
    {
        // draw distance line vertically from the anchor pos to the top edge of the parent rect
        // The distance value is the anchor value (0 - 1 range)
        AZ::Vector2 targetPosTop(anchorPos.GetX(), parentPoints.TopLeft().GetY());
        m_dottedLine->DrawDistanceLineWithTransform(draw2d, anchorPos, targetPosTop, transform, vertAnchorVal * 100.0f, "%");
    }
}
