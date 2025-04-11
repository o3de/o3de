/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "EditorCommon.h"
#include "ViewportPivot.h"

#include <LyShine/Bus/UiLayoutFitterBus.h>

namespace ViewportHelpers
{
    bool IsControlledByLayout(const AZ::Entity* element)
    {
        AZ::Entity* parentElement = EntityHelpers::GetParentElement(element);
        bool isControlledByParent = false;
        if (parentElement)
        {
            UiLayoutBus::EventResult(
                isControlledByParent, parentElement->GetId(), &UiLayoutBus::Events::IsControllingChild, element->GetId());
        }

        return isControlledByParent;
    }

    float GetDpiScaledSize(float size)
    {
        return size * ViewportIcon::GetDpiScaleFactor();
    }

    bool IsHorizontallyFit(const AZ::Entity* element)
    {
        bool isHorizontallyFit = false;

        UiLayoutFitterBus::EventResult(isHorizontallyFit, element->GetId(), &UiLayoutFitterBus::Events::GetHorizontalFit);

        return isHorizontallyFit;
    }

    bool IsVerticallyFit(const AZ::Entity* element)
    {
        bool isVerticallyFit = false;

        UiLayoutFitterBus::EventResult(isVerticallyFit, element->GetId(), &UiLayoutFitterBus::Events::GetVerticalFit);

        return isVerticallyFit;
    }

    float GetPerpendicularAngle(float angle)
    {
        return fmodf(angle + 90.0f, 180.0f);
    }

    Qt::CursorShape GetSizingCursor(float angle)
    {
        Qt::CursorShape sizingCursors[] {
            Qt::SizeVerCursor, Qt::SizeBDiagCursor, Qt::SizeHorCursor, Qt::SizeFDiagCursor
        };

        // The expected angle range is [-180, +180]. Each cursor covers two 45 degree
        // sections that are opposite each other on that circle.
        float section = 45.0f;

        // Starting at -180, the transitions are Vert, BDiag (/), Horiz, FDiag (\) and
        // continues in that same pattern for 0 to 180 (V,B,H,F), so the full pattern is
        // V B H F V B H F which can be done with mod 4. However, the modulus operator
        // doesn't handle negative values the way that we want, so we need to get the angle
        // in the range [0, 360]. For our purposes, it's okay if the angle flips to its opposite.
        angle += 180.0f;

        // We shift the cursor sections by 45/2 degrees, so that the center of each cursor
        // section is directly on a multiple of 45 degrees (0, 45, 90, etc.).
        angle += 0.5f * section;

        // Compute which section this angle is in.
        int index = ((int)(angle / section)) % 4;

        // Return the appropriate sizing cursor.
        return sizingCursors[index];
    }

    void TransformIconScale(AZ::Vector2& iconSize, const AZ::Matrix4x4& transform)
    {
        // Make two unit vectors in untransformed space
        AZ::Vector3 widthVec(1, 0, 0);
        AZ::Vector3 heightVec(0, 1, 0);

        // Convert these two unit vectors into the transformed space
        widthVec = transform.Multiply3x3(widthVec);
        heightVec = transform.Multiply3x3(heightVec);

        // Divide the iconSize (for untransformed space) by the scale that each unit vector received
        iconSize.SetX(iconSize.GetX() / AZ::Vector2(widthVec.GetX(), widthVec.GetY()).GetLength());
        iconSize.SetY(iconSize.GetY() / AZ::Vector2(heightVec.GetX(), heightVec.GetY()).GetLength());
    }

    AZ::Vector2 ComputeAnchorPoint(AZ::Vector2 rectTopLeft, AZ::Vector2 rectSize, float anchorX, float anchorY)
    {
        rectSize.SetX(rectSize.GetX() * anchorX);
        rectSize.SetY(rectSize.GetY() * anchorY);
        return rectTopLeft + rectSize;
    }

    bool IsPointInIconRect(AZ::Vector2 point, AZ::Vector2 iconCenter, AZ::Vector2 iconSize, float leftPart, float rightPart, float topPart, float bottomPart)
    {
        float left   = iconCenter.GetX() + leftPart   * iconSize.GetX();
        float right  = iconCenter.GetX() + rightPart  * iconSize.GetX();
        float top    = iconCenter.GetY() + topPart    * iconSize.GetY();
        float bottom = iconCenter.GetY() + bottomPart * iconSize.GetY();

        return (left < point.GetX() && point.GetX() < right &&
                top < point.GetY() && point.GetY() < bottom);
    }

    void GetHorizTargetPoints(const UiTransformInterface::RectPoints& elemRect, float y, AZ::Vector2& leftTarget, AZ::Vector2& rightTarget)
    {
        leftTarget = (elemRect.TopLeft() + elemRect.BottomLeft()) * 0.5f;
        rightTarget = (elemRect.TopRight() + elemRect.BottomRight()) * 0.5f;

        if (y >= elemRect.TopLeft().GetY())
        {
            if (y <= elemRect.BottomLeft().GetY())
            {
                leftTarget.SetY(y);
                rightTarget.SetY(leftTarget.GetY());
            }
            else
            {
                leftTarget.SetY(elemRect.BottomLeft().GetY());
                rightTarget.SetY(leftTarget.GetY());
            }
        }
        else
        {
            leftTarget.SetY(elemRect.TopLeft().GetY());
            rightTarget.SetY(leftTarget.GetY());
        }
    }

    void GetVerticalTargetPoints(const UiTransformInterface::RectPoints& elemRect, float x, AZ::Vector2& topTarget, AZ::Vector2& bottomTarget)
    {
        topTarget = (elemRect.TopLeft() + elemRect.TopRight()) * 0.5f;
        bottomTarget = (elemRect.BottomLeft() + elemRect.BottomRight()) * 0.5f;

        if (x >= elemRect.TopLeft().GetX())
        {
            if (x <= elemRect.TopRight().GetX())
            {
                topTarget.SetX(x);
                bottomTarget.SetX(topTarget.GetX());
            }
            else
            {
                topTarget.SetX(elemRect.TopRight().GetX());
                bottomTarget.SetX(topTarget.GetX());
            }
        }
        else
        {
            topTarget.SetX(elemRect.TopLeft().GetX());
            bottomTarget.SetX(topTarget.GetX());
        }
    }

    UiTransform2dInterface::Offsets MoveGrabbedEdges(const UiTransform2dInterface::Offsets& offset,
        const ViewportHelpers::ElementEdges& grabbedEdges,
        const AZ::Vector2& v)
    {
        UiTransform2dInterface::Offsets outOffset(offset);

        outOffset.m_left += (grabbedEdges.m_left ? v.GetX() : 0.0f);
        outOffset.m_right += (grabbedEdges.m_right ? v.GetX() : 0.0f);
        outOffset.m_top += (grabbedEdges.m_top ? v.GetY() : 0.0f);
        outOffset.m_bottom += (grabbedEdges.m_bottom ? v.GetY() : 0.0f);

        return outOffset;
    }

    UiTransform2dInterface::Anchors MoveGrabbedAnchor(const UiTransform2dInterface::Anchors& anchor,
        const ViewportHelpers::SelectedAnchors& grabbedAnchors, bool keepTogetherHorizontally,
        bool keepTogetherVertically, const AZ::Vector2& v)
    {
        UiTransform2dInterface::Anchors outAnchor(anchor);

        outAnchor.m_left += (grabbedAnchors.m_left) ? v.GetX() : 0.0f;
        outAnchor.m_right += (grabbedAnchors.m_right) ? v.GetX() : 0.0f;
        outAnchor.m_top += (grabbedAnchors.m_top) ? v.GetY() : 0.0f;
        outAnchor.m_bottom += (grabbedAnchors.m_bottom) ? v.GetY() : 0.0f;

        if (keepTogetherHorizontally)
        {
            if (grabbedAnchors.m_left && !grabbedAnchors.m_right)
            {
                outAnchor.m_right = outAnchor.m_left;
            }
            else if (grabbedAnchors.m_right && !grabbedAnchors.m_left)
            {
                outAnchor.m_left = outAnchor.m_right;
            }
        }

        if (keepTogetherVertically)
        {
            if (grabbedAnchors.m_top && !grabbedAnchors.m_bottom)
            {
                outAnchor.m_bottom = outAnchor.m_top;
            }
            else if (grabbedAnchors.m_bottom && !grabbedAnchors.m_top)
            {
                outAnchor.m_top = outAnchor.m_bottom;
            }
        }

        // Clamp the anchors
        outAnchor.UnitClamp();

        return outAnchor;
    }

    void MoveGrabbedEdges(UiTransformInterface::RectPoints& points,
        const ViewportHelpers::ElementEdges& grabbedEdges,
        const AZ::Vector2& topEdge,
        const AZ::Vector2& leftEdge)
    {
        if (grabbedEdges.m_left)
        {
            points.TopLeft() += topEdge;
            points.BottomLeft() += topEdge;
        }
        if (grabbedEdges.m_right)
        {
            points.TopRight() += topEdge;
            points.BottomRight() += topEdge;
        }
        if (grabbedEdges.m_top)
        {
            points.TopLeft() += leftEdge;
            points.TopRight() += leftEdge;
        }
        if (grabbedEdges.m_bottom)
        {
            points.BottomLeft() += leftEdge;
            points.BottomRight() += leftEdge;
        }
    }

    const char* InteractionModeToString(int mode)
    {
        switch (static_cast<ViewportInteraction::InteractionMode>(mode))
        {
        case ViewportInteraction::InteractionMode::SELECTION:
            return "Selection";
            break;
        case ViewportInteraction::InteractionMode::MOVE:
            return "Move";
            break;
        case ViewportInteraction::InteractionMode::ANCHOR:
            return "Anchor";
            break;
        case ViewportInteraction::InteractionMode::ROTATE:
            return "Rotate";
            break;
        case ViewportInteraction::InteractionMode::RESIZE:
            return "Resize";
            break;
        default:
            AZ_Assert(false, "Invalid mode");
            return "UNKNOWN";
            break;
        }
    }

    const char* CoordinateSystemToString(int s)
    {
        switch (static_cast<ViewportInteraction::CoordinateSystem>(s))
        {
        case ViewportInteraction::CoordinateSystem::LOCAL:
            return "Local";
            break;
        case ViewportInteraction::CoordinateSystem::VIEW:
            return "View";
            break;
        default:
            AZ_Assert(false, "Invalid coordinate system enum value");
            return "UNKNOWN";
            break;
        }
    }

    const char* InteractionTypeToString(int type)
    {
        switch (static_cast<ViewportInteraction::InteractionType>(type))
        {
        case ViewportInteraction::InteractionType::DIRECT:
            return "DIRECT";
            break;
        case ViewportInteraction::InteractionType::TRANSFORM_GIZMO:
            return "TRANSFORM_GIZMO";
            break;
        case ViewportInteraction::InteractionType::ANCHORS:
            return "ANCHORS";
            break;
        case ViewportInteraction::InteractionType::PIVOT:
            return "PIVOT";
            break;
        case ViewportInteraction::InteractionType::NONE:
            return "NONE";
            break;
        default:
            AZ_Assert(false, "Invalid interaction type");
            return "UNKNOWN";
            break;
        }
    }

    void DrawRotationValue(const AZ::Entity* element,
        ViewportInteraction* viewportInteraction,
        const ViewportPivot* viewportPivot,
        Draw2dHelper& draw2d)
    {
        // Draw the rotation in degrees when the left mouse button is down on the rotation gizmo
        if ((viewportInteraction->GetInteractionType() == ViewportInteraction::InteractionType::TRANSFORM_GIZMO)
            && viewportInteraction->GetLeftButtonIsActive())
        {
            float rotation = 0.0f;
            UiTransformBus::EventResult(rotation, element->GetId(), &UiTransformBus::Events::GetZRotation);
            QString rotationString = QString("%1").number(rotation, 'f', 2);
            QChar degChar(0xB0);
            rotationString.append(degChar);

            AZ::Vector2 pivotPos;
            UiTransformBus::EventResult(pivotPos, element->GetId(), &UiTransformBus::Events::GetViewportSpacePivot);
            float offset = (viewportPivot->GetSize().GetY() * 0.5f) + (GetDpiScaledSize(4.0f));
            AZ::Vector2 rotationStringPos(pivotPos.GetX(), pivotPos.GetY() - offset);

            draw2d.SetTextAlignment(IDraw2d::HAlign::Center, IDraw2d::VAlign::Bottom);
            draw2d.SetTextRotation(0.0f);
            draw2d.DrawText(rotationString.toUtf8().data(), rotationStringPos, 8.0f, 1.0f);
        }
    }

    void DrawCursorText(const AZStd::string& textLabel,
        Draw2dHelper& draw2d,
        const ViewportWidget* viewport)
    {
        const AZ::Vector2 textLabelOffset(10.0f, -10.0f);
        QPoint viewportCursorPos = viewport->mapFromGlobal(QCursor::pos());
        AZ::Vector2 textPos = AZ::Vector2(aznumeric_cast<float>(viewportCursorPos.x()), aznumeric_cast<float>(viewportCursorPos.y())) + textLabelOffset;
        float dpiScale = viewport->WidgetToViewportFactor();
        textPos *= dpiScale;

        draw2d.SetTextAlignment(IDraw2d::HAlign::Left, IDraw2d::VAlign::Bottom);
        draw2d.SetTextRotation(0.0f);
        draw2d.DrawText(textLabel.c_str(), textPos, 8.0f, 1.0f);
    }
}   // namespace ViewportHelpers
