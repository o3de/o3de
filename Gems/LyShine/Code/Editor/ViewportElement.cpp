/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "EditorCommon.h"
#include "ViewportSnap.h"
#include "ViewportElement.h"

namespace
{
    //! Used for transform functions that get called on all selected elements
    AZ::Vector3 GetTranslationForSelectedElement(const AZ::EntityId& activeElementId,
        AZ::Entity* selectedElement,
        const AZ::Vector3& mouseTranslation)
    {
        // When the user is interacting with an element (the "ACTIVE" element),
        // the interaction will usually also affect every other SELECTED element.
        // This function does the work to find the mouse translation vector
        // with respect to the ACTIVE element, oriented with respect to the
        // SELECTED element in question, with the same length as the original
        // mouse translation vector. The resulting vector is in viewport space.

        // Find the orientation of the translation vector from the ACTIVE element's
        // perspective.
        AZ::Matrix4x4 activeTransformFromViewport;
        UiTransformBus::Event(activeElementId, &UiTransformBus::Events::GetTransformFromViewport, activeTransformFromViewport);
        AZ::Vector3 activeElementTranslation = activeTransformFromViewport.Multiply3x3(mouseTranslation);

        // Give the translation vector the same orientation with respect to
        // the SELECTED element that it had with respect to the ACTIVE element.
        AZ::Matrix4x4 selectedTransformToViewport;
        UiTransformBus::Event(selectedElement->GetId(), &UiTransformBus::Events::GetTransformToViewport, selectedTransformToViewport);
        AZ::Vector3 elementViewportTranslation = selectedTransformToViewport.Multiply3x3(activeElementTranslation);

        // Adjust the translation vector to have the same length as the original
        // viewport-space translation vector.
        return elementViewportTranslation.GetNormalizedSafe() * mouseTranslation.GetLength();
    }
} // anonymous namespace.

bool ViewportElement::PickElementEdges(const AZ::Entity* element,
    const AZ::Vector2& point,
    float distance,
    ViewportHelpers::ElementEdges& outEdges)
{
    if (!element)
    {
        // If there's no element, there can't be any edges
        outEdges.SetAll(false);
        return false;
    }

    // Transform the point and the pick distance from viewport space into untransformed canvas space
    AZ::Matrix4x4 transformFromViewport;
    UiTransformBus::Event(element->GetId(), &UiTransformBus::Events::GetTransformFromViewport, transformFromViewport);

    AZ::Vector3 pickDistance(distance, distance, 0.0f);

    if (!(transformFromViewport == AZ::Matrix4x4::CreateIdentity()))
    {
        AZ::Vector3 pickDistanceX(distance, 0.0f, 0.0f);
        AZ::Vector3 pickDistanceY(0.0f, distance, 0.0f);
        AZ::Matrix4x4 transformToViewport;
        UiTransformBus::Event(element->GetId(), &UiTransformBus::Events::GetTransformToViewport, transformToViewport);
        AZ::Vector3 localDistanceX = transformToViewport.Multiply3x3(pickDistanceX);
        AZ::Vector3 localDistanceY = transformToViewport.Multiply3x3(pickDistanceY);

        // Avoid a divide by zero. We could compare with 0.0f here and that would avoid a divide
        // by zero. However comparing with FLT_EPSILON also avoids the rare case of an overflow.
        // FLT_EPSILON is small enough to be considered equivalent to zero in this application.
        float localDistanceXLength = AZ::Vector2(localDistanceX.GetX(), localDistanceX.GetY()).GetLength();
        float localDistanceYLength = AZ::Vector2(localDistanceY.GetX(), localDistanceY.GetY()).GetLength();
        localDistanceX *= (fabsf(localDistanceXLength) > FLT_EPSILON) ? distance / localDistanceXLength : 0;
        localDistanceY *= (fabsf(localDistanceYLength) > FLT_EPSILON) ? distance / localDistanceYLength : 0;

        localDistanceX = transformFromViewport.Multiply3x3(localDistanceX);
        localDistanceY = transformFromViewport.Multiply3x3(localDistanceY);

        pickDistance.SetX(localDistanceX.GetX());
        pickDistance.SetY(localDistanceY.GetY());
    }

    AZ::Vector3 pickPoint = transformFromViewport * AZ::Vector3(point.GetX(), point.GetY(), 0.0f);

    // Get the non-transformed edges of the element
    UiTransformInterface::RectPoints corners;
    UiTransformBus::Event(element->GetId(), &UiTransformBus::Events::GetCanvasSpacePointsNoScaleRotate, corners);

    float left   = corners.TopLeft().GetX();
    float right  = corners.BottomRight().GetX();
    float top    = corners.TopLeft().GetY();
    float bottom = corners.BottomRight().GetY();

    float minX = min(left, right) - pickDistance.GetX();
    float maxX = max(left, right) + pickDistance.GetX();
    float minY = min(top, bottom) - pickDistance.GetY();
    float maxY = max(top, bottom) + pickDistance.GetY();

    // Test distance of point from edges
    if (!ViewportHelpers::IsHorizontallyFit(element))
    {
        if (pickPoint.GetY() >= minY && pickPoint.GetY() <= maxY)
        {
            if (fabsf(pickPoint.GetX() - left) <= pickDistance.GetX())
            {
                outEdges.m_left = true;
            }
            if (fabsf(pickPoint.GetX() - right) <= pickDistance.GetX())
            {
                outEdges.m_right = true;
            }
        }
    }
    if (!ViewportHelpers::IsVerticallyFit(element))
    {
        if (pickPoint.GetX() >= minX && pickPoint.GetX() <= maxX)
        {
            if (fabsf(pickPoint.GetY() - top) <= pickDistance.GetY())
            {
                outEdges.m_top = true;
            }
            if (fabsf(pickPoint.GetY() - bottom) <= pickDistance.GetY())
            {
                outEdges.m_bottom = true;
            }
        }
    }

    return outEdges.Any();
}

bool ViewportElement::PickAnchors(const AZ::Entity* element,
    const AZ::Vector2& point,
    const AZ::Vector2& iconSize,
    ViewportHelpers::SelectedAnchors& outAnchors)
{
    if (!element)
    {
        // if there's no element, there are no anchors
        return false;
    }

    if (!UiTransform2dBus::FindFirstHandler(element->GetId()))
    {
        // if the element isn't using a Transform2d, there are no anchors
        return false;
    }

    AZ::Entity* parentElement = EntityHelpers::GetParentElement(element);

    // The anchors are in the parent's space, which may be rotated and scaled.
    // It's simpler to do the calculations in canvas space, so we need to
    // transform everything from the parent's viewport space to canvas space.
    AZ::Matrix4x4 transformFromViewport;
    UiTransformBus::Event(parentElement->GetId(), &UiTransformBus::Events::GetTransformFromViewport, transformFromViewport);

    UiTransformInterface::RectPoints parentRect;
    UiTransformBus::Event(parentElement->GetId(), &UiTransformBus::Events::GetCanvasSpacePointsNoScaleRotate, parentRect);
    AZ::Vector2 parentSize = parentRect.GetAxisAlignedSize();

    AZ::Vector3 pickPoint3 = transformFromViewport * AZ::Vector3(point.GetX(), point.GetY(), 0.0f);
    AZ::Vector2 pickPoint(pickPoint3.GetX(), pickPoint3.GetY());

    UiTransform2dInterface::Anchors anchors;
    UiTransform2dBus::EventResult(anchors, element->GetId(), &UiTransform2dBus::Events::GetAnchors);

    AZ::Vector2 scaledIconSize(iconSize);

    // reverse the scale for the icon, because the icon doesn't change size
    if (transformFromViewport.GetElement(0, 0) != 1.0f || transformFromViewport.GetElement(1, 1) != 1.0f || transformFromViewport.GetElement(2, 2) != 1.0f)
    {
        AZ::Matrix4x4 transformToViewport;
        UiTransformBus::Event(parentElement->GetId(), &UiTransformBus::Events::GetTransformToViewport, transformToViewport);
        ViewportHelpers::TransformIconScale(scaledIconSize, transformToViewport);
    }

    // if all the anchors are together
    if (anchors.m_left == anchors.m_right && anchors.m_top == anchors.m_bottom)
    {
        // if the point hits the center, select all the anchors
        AZ::Vector2 anchorPos = ViewportHelpers::ComputeAnchorPoint(parentRect.TopLeft(), parentSize, anchors.m_left, anchors.m_top);
        if (ViewportHelpers::IsPointInIconRect(pickPoint, anchorPos, scaledIconSize, -0.2f, 0.2f, -0.2f, 0.2f))
        {
            outAnchors = ViewportHelpers::SelectedAnchors(true, true, true, true);
            return true;
        }
    }

    // if all the anchors are together or they're split horizontally
    if (anchors.m_top == anchors.m_bottom)
    {
        // if the point hits the left anchor icon, select the left anchor
        AZ::Vector2 leftAnchorPos = ViewportHelpers::ComputeAnchorPoint(parentRect.TopLeft(), parentSize, anchors.m_left, anchors.m_top);
        if (ViewportHelpers::IsPointInIconRect(pickPoint, leftAnchorPos, scaledIconSize, -0.5f, 0.0f, -0.2f, 0.2f))
        {
            outAnchors = ViewportHelpers::SelectedAnchors(true, false, false, false);
            return true;
        }

        // if the point hits the right anchor icon, select the right anchor
        AZ::Vector2 rightAnchorPos = ViewportHelpers::ComputeAnchorPoint(parentRect.TopLeft(), parentSize, anchors.m_right, anchors.m_top);
        if (ViewportHelpers::IsPointInIconRect(pickPoint, rightAnchorPos, scaledIconSize, 0.0f, 0.5f, -0.2f, 0.2f))
        {
            outAnchors = ViewportHelpers::SelectedAnchors(false, false, true, false);
            return true;
        }
    }

    // if all the anchors are together or they're split vertically
    if (anchors.m_left == anchors.m_right)
    {
        // if the point hits the top anchor icon, select the top anchor
        AZ::Vector2 topAnchorPos = ViewportHelpers::ComputeAnchorPoint(parentRect.TopLeft(), parentSize, anchors.m_left, anchors.m_top);
        if (ViewportHelpers::IsPointInIconRect(pickPoint, topAnchorPos, scaledIconSize, -0.2f, 0.2f, -0.5f, 0.0f))
        {
            outAnchors = ViewportHelpers::SelectedAnchors(false, true, false, false);
            return true;
        }

        // if the point hits the bottom anchor icon, select the bottom anchor
        AZ::Vector2 bottomAnchorPos = ViewportHelpers::ComputeAnchorPoint(parentRect.TopLeft(), parentSize, anchors.m_left, anchors.m_bottom);
        if (ViewportHelpers::IsPointInIconRect(pickPoint, bottomAnchorPos, scaledIconSize, -0.2f, 0.2f, 0.0f, 0.5f))
        {
            outAnchors = ViewportHelpers::SelectedAnchors(false, false, false, true);
            return true;
        }
    }

    // if the point hits the top left anchor icon, select the top and left anchors
    AZ::Vector2 topLeftAnchorPos = ViewportHelpers::ComputeAnchorPoint(parentRect.TopLeft(), parentSize, anchors.m_left, anchors.m_top);
    if (ViewportHelpers::IsPointInIconRect(pickPoint, topLeftAnchorPos, scaledIconSize, -0.5f, 0.0f, -0.5f, 0.0f))
    {
        outAnchors = ViewportHelpers::SelectedAnchors(true, true, false, false);
        return true;
    }

    // if the point hits the top right anchor icon, select the top and right anchors
    AZ::Vector2 topRightAnchorPos = ViewportHelpers::ComputeAnchorPoint(parentRect.TopLeft(), parentSize, anchors.m_right, anchors.m_top);
    if (ViewportHelpers::IsPointInIconRect(pickPoint, topRightAnchorPos, scaledIconSize, 0.0f, 0.5f, -0.5f, 0.0f))
    {
        outAnchors = ViewportHelpers::SelectedAnchors(false, true, true, false);
        return true;
    }

    // if the point hits the bottom right anchor icon, select the bottom and right anchors
    AZ::Vector2 bottomRightAnchorPos = ViewportHelpers::ComputeAnchorPoint(parentRect.TopLeft(), parentSize, anchors.m_right, anchors.m_bottom);
    if (ViewportHelpers::IsPointInIconRect(pickPoint, bottomRightAnchorPos, scaledIconSize, 0.0f, 0.5f, 0.0f, 0.5f))
    {
        outAnchors = ViewportHelpers::SelectedAnchors(false, false, true, true);
        return true;
    }

    // if the point hits the bottom left anchor icon, select the bottom and left anchors
    AZ::Vector2 bottomLeftAnchorPos = ViewportHelpers::ComputeAnchorPoint(parentRect.TopLeft(), parentSize, anchors.m_left, anchors.m_bottom);
    if (ViewportHelpers::IsPointInIconRect(pickPoint, bottomLeftAnchorPos, scaledIconSize, -0.5f, 0.0f, 0.0f, 0.5f))
    {
        outAnchors = ViewportHelpers::SelectedAnchors(true, false, false, true);
        return true;
    }

    // if the point doesn't hit any anchor icons, select no anchors
    return false;
}

bool ViewportElement::PickAxisGizmo(const AZ::Entity* element,
    ViewportInteraction::CoordinateSystem coordinateSystem,
    ViewportInteraction::InteractionMode interactionMode,
    const AZ::Vector2& point,
    const AZ::Vector2& iconSize,
    ViewportHelpers::GizmoParts& outGizmoParts)
{
    outGizmoParts.SetBoth(false);

    if (!element)
    {
        // If there is no element, there's no transform gizmo
        return false;
    }

    AZ::Vector2 scaledIconSize(iconSize);
    AZ::Vector2 pivotPosition;
    AZ::Vector2 pickPoint;

    if (coordinateSystem == ViewportInteraction::CoordinateSystem::LOCAL)
    {
        // LOCAL MOVE in the parent element's LOCAL space.
        AZ::EntityId elementId(interactionMode == ViewportInteraction::InteractionMode::MOVE ? EntityHelpers::GetParentElement(element)->GetId() : element->GetId());

        // It's simpler to do the calculations in canvas space, so we need to
        // transform everything from viewport space to canvas space.
        AZ::Matrix4x4 transformFromViewport;
        UiTransformBus::Event(elementId, &UiTransformBus::Events::GetTransformFromViewport, transformFromViewport);

        AZ::Vector3 pickPoint3 = transformFromViewport * AZ::Vector3(point.GetX(), point.GetY(), 0.0f);
        pickPoint = AZ::Vector2(pickPoint3.GetX(), pickPoint3.GetY());
        // Reverse the scale for the gizmo icon, because the icon doesn't change size
        if (transformFromViewport.GetElement(0, 0) != 1.0f || transformFromViewport.GetElement(1, 1) != 1.0f || transformFromViewport.GetElement(2, 2) != 1.0f)
        {
            AZ::Matrix4x4 transformToViewport;
            UiTransformBus::Event(elementId, &UiTransformBus::Events::GetTransformToViewport, transformToViewport);
            ViewportHelpers::TransformIconScale(scaledIconSize, transformToViewport);
        }

        UiTransformBus::EventResult(pivotPosition, element->GetId(), &UiTransformBus::Events::GetCanvasSpacePivotNoScaleRotate);
    }
    else
    {
        // for View coordinate system do everything in viewport space
        pickPoint = point;
        UiTransformBus::EventResult(pivotPosition, element->GetId(), &UiTransformBus::Events::GetViewportSpacePivot);
    }

    // Center square
    if ((interactionMode != ViewportInteraction::InteractionMode::RESIZE ||
        (!ViewportHelpers::IsHorizontallyFit(element) && !ViewportHelpers::IsVerticallyFit(element))) &&
        ViewportHelpers::IsPointInIconRect(pickPoint, pivotPosition, scaledIconSize, -0.02f, 0.16f, -0.16f, 0.02f))
    {
        outGizmoParts.SetBoth(true);
        return true;
    }

    // Up axis
    if ((interactionMode != ViewportInteraction::InteractionMode::RESIZE || !ViewportHelpers::IsVerticallyFit(element)) &&
        ViewportHelpers::IsPointInIconRect(pickPoint, pivotPosition, scaledIconSize, -0.04f, 0.04f, -0.5f, -0.16f))
    {
        outGizmoParts.m_top = true;
        return true;
    }

    // Right axis
    if ((interactionMode != ViewportInteraction::InteractionMode::RESIZE || !ViewportHelpers::IsHorizontallyFit(element)) &&
        ViewportHelpers::IsPointInIconRect(pickPoint, pivotPosition, scaledIconSize, 0.16f, 0.5f, -0.04f, 0.04f))
    {
        outGizmoParts.m_right = true;
        return true;
    }

    // The point is not within the transform gizmo
    return false;
}

bool ViewportElement::PickCircleGizmo(const AZ::Entity* element,
    const AZ::Vector2& point,
    const AZ::Vector2& iconSize,
    ViewportHelpers::GizmoParts& outGizmoParts)
{
    outGizmoParts.SetBoth(false);

    if (!element)
    {
        return false;
    }

    float lineThickness = 4.0f;

    AZ::Vector2 pivot;
    UiTransformBus::EventResult(pivot, element->GetId(), &UiTransformBus::Events::GetViewportSpacePivot);

    float distance = (point - pivot).GetLength();
    float radius = 0.5f * iconSize.GetX() - 0.5f * lineThickness;

    if (fabs(distance - radius) < lineThickness)
    {
        outGizmoParts.SetBoth(true);
        return true;
    }

    return false;
}

bool ViewportElement::PickPivot(const AZ::Entity* element,
    const AZ::Vector2& point,
    const AZ::Vector2& iconSize)
{
    if (!element)
    {
        return false;
    }

    AZ::Vector2 pivot;
    UiTransformBus::EventResult(pivot, element->GetId(), &UiTransformBus::Events::GetViewportSpacePivot);
    float distance = (point - pivot).GetLength();
    float radius = 0.5f * iconSize.GetX();

    if (distance <= radius)
    {
        return true;
    }
    return false;
}

void ViewportElement::ResizeDirectly(HierarchyWidget* hierarchy,
    const AZ::EntityId& canvasId,
    const ViewportHelpers::ElementEdges& grabbedEdges,
    AZ::Entity* element,
    const AZ::Vector3& mouseTranslation)
{
    if (ViewportHelpers::IsControlledByLayout(element))
    {
        return;
    }

    // Get translation for this element's offsets in viewport space
    AZ::Vector3 viewportTranslation = GetTranslationForSelectedElement(element->GetId(), element, mouseTranslation);

    // Get the transform from viewport to parent element space
    AZ::Entity* parentElement = EntityHelpers::GetParentElement(element);
    AZ::Matrix4x4 parentTransformFromViewport;
    UiTransformBus::Event(parentElement->GetId(), &UiTransformBus::Events::GetTransformFromViewport, parentTransformFromViewport);

    // Resize the element
    bool hasScaleOrRotation = false;
    UiTransformBus::EventResult(hasScaleOrRotation, element->GetId(), &UiTransformBus::Events::HasScaleOrRotation);
    if (hasScaleOrRotation)
    {
        // This element has scale or rotation. This makes things complicated.
        // Moving an edge will move the pivot point in canvas space. The pivot point affects
        // how this element's points are scaled and rotated. So, to stop this element moving around in space
        // as an edge is dragged we may actually have to adjust all four offsets.

        // get the viewport space points for this element
        UiTransformInterface::RectPoints points;
        UiTransformBus::Event(element->GetId(), &UiTransformBus::Events::GetViewportSpacePoints, points);

        // get the 2D delta in viewport space for this element
        AZ::Vector2 delta(viewportTranslation.GetX(), viewportTranslation.GetY());

        // project the delta onto unit vectors parallel to each side of the rect
        AZ::Vector2 unitVecTopEdge = (points.TopRight() - points.TopLeft()).GetNormalizedSafe();
        AZ::Vector2 unitVecLeftEdge = (points.BottomLeft() - points.TopLeft()).GetNormalizedSafe();
        AZ::Vector2 deltaTopEdge = EntityHelpers::RoundXY(unitVecTopEdge * unitVecTopEdge.Dot(delta));
        AZ::Vector2 deltaLeftEdge = EntityHelpers::RoundXY(unitVecLeftEdge * unitVecLeftEdge.Dot(delta));

        // apply the delta to the points, this moves the edge(s) in viewport space
        ViewportHelpers::MoveGrabbedEdges(points, grabbedEdges, deltaTopEdge, deltaLeftEdge);

        // calculate the new pivot in viewport space
        AZ::Vector2 pivot;
        UiTransformBus::EventResult(pivot, element->GetId(), &UiTransformBus::Events::GetPivot);
        AZ::Vector2 viewportPivot = points.TopLeft()
            + pivot.GetX() * (points.TopRight() - points.TopLeft())
            + pivot.GetY() * (points.BottomLeft() - points.TopLeft());
        AZ::Vector3 pivot3(viewportPivot.GetX(), viewportPivot.GetY(), 0);

        // transform pivot into parent space
        pivot3 = parentTransformFromViewport * pivot3;

        // build matrix to transform these points into parent's transform space using this pivot
        float rotation;
        UiTransformBus::EventResult(rotation, element->GetId(), &UiTransformBus::Events::GetZRotation);
        float rotRad = DEG2RAD(-rotation);  // reverse rotation

        AZ::Vector2 scale;
        UiTransformBus::EventResult(scale, element->GetId(), &UiTransformBus::Events::GetScale);
        AZ::Vector3 scale3(1.0f / scale.GetX(), 1.0f / scale.GetY(), 1); // inverse scale

        AZ::Matrix4x4 moveToPivotSpaceMat = AZ::Matrix4x4::CreateTranslation(-pivot3);
        AZ::Matrix4x4 scaleMat = AZ::Matrix4x4::CreateScale(scale3);
        AZ::Matrix4x4 rotMat = AZ::Matrix4x4::CreateRotationZ(rotRad);
        AZ::Matrix4x4 moveFromPivotSpaceMat = AZ::Matrix4x4::CreateTranslation(pivot3);

        AZ::Matrix4x4 thisElementInverseTransform = moveFromPivotSpaceMat * scaleMat * rotMat * moveToPivotSpaceMat;

        // concatenate this special matrix with the parent's. The resulting matrix will transform the
        // dragged rect points (in viewport space) into untransformed (axis aligned) canvas space
        // NOTE: we really only need to transform TopLeft and BottomRight but it is easier to
        // debug if we transform all four - we can check that it becomes axis aligned
        AZ::Matrix4x4 mat = thisElementInverseTransform * parentTransformFromViewport;
        points = points.Transform(mat);

        // points are now the axis-aligned (non scaled/rotated points).
        // get the existing (unchanged so far) version of these from the element
        // then compare the new points against the old points and adjust the offsets by the deltas
        UiTransformInterface::RectPoints oldPoints;
        UiTransformBus::Event(element->GetId(), &UiTransformBus::Events::GetCanvasSpacePointsNoScaleRotate, oldPoints);

        ViewportSnap::ResizeDirectlyWithScaleOrRotation(hierarchy, canvasId, grabbedEdges, element, (points - oldPoints));
    }
    else // if (!hasScaleOrRotation)
    {
        // This element has no scale or rotation (its parents may have)

        // The final translation vector needs to be in the element's parent space,
        // because its offsets are in parent space.
        AZ::Vector3 finalTranslation3 = parentTransformFromViewport.Multiply3x3(viewportTranslation);
        AZ::Vector2 finalTranslation(finalTranslation3.GetX(), finalTranslation3.GetY());
        finalTranslation = EntityHelpers::RoundXY(finalTranslation);

        ViewportSnap::ResizeDirectlyNoScaleNoRotation(hierarchy, canvasId, grabbedEdges, element, finalTranslation);
    }
}

void ViewportElement::ResizeByGizmo(HierarchyWidget* hierarchy,
    const AZ::EntityId& canvasId,
    const ViewportHelpers::GizmoParts& grabbedGizmoParts,
    const AZ::EntityId& activeElementId,
    AZ::Entity* element,
    const AZ::Vector3& mouseTranslation)
{
    if (ViewportHelpers::IsControlledByLayout(element))
    {
        return;
    }

    // Get translation for this element's offsets in viewport space
    AZ::Vector3 viewportTranslation = GetTranslationForSelectedElement(activeElementId, element, mouseTranslation);

    if (ViewportHelpers::IsHorizontallyFit(element))
    {
        viewportTranslation.SetX(0.0f);
    }

    if (ViewportHelpers::IsVerticallyFit(element))
    {
        viewportTranslation.SetY(0.0f);
    }

    // Transform to element space
    AZ::Matrix4x4 transformFromViewport;
    UiTransformBus::Event(element->GetId(), &UiTransformBus::Events::GetTransformFromViewport, transformFromViewport);
    AZ::Vector3 finalTranslation = transformFromViewport.Multiply3x3(viewportTranslation);

    // get the pivot (each field is in the range 0-1 if inside the element rect)
    // but note that it can be outside that range
    AZ::Vector2 pivot;
    UiTransformBus::EventResult(pivot, element->GetId(), &UiTransformBus::Events::GetPivot);

    // The resize works about the pivot, this stops the gizmo itself from moving as we resize.
    // If we split final translation on either side of the pivot (according to the
    // pivot ratio) that keeps the pivot stationary. However, it means that in the normal case
    // of a pivot of 0.5,0.5 the edge will only move half as much as the mouse.
    // So, we could double finalTranslation but then in the case of a pivot at 0,0 the edges
    // that move would move at twice the speed of the mouse.
    // We can scale finalTranslation so that the right and top edges move at the speed of the mouse.
    // This seems intuitive since the gizmo points in those directions. However it doesn't work
    // if the X pivot is 1.0f for example since then the right edge will not move. The best compromise
    // is to make the edge that moves the most move at the speed of the mouse.
    float xScale = 1.0f / ((pivot.GetX() > 0.5f) ? pivot.GetX() : (1.0f - pivot.GetX()));
    float yScale = 1.0f / ((pivot.GetY() > 0.5f) ? pivot.GetY() : (1.0f - pivot.GetY()));
    finalTranslation.SetX(finalTranslation.GetX() * xScale);
    finalTranslation.SetY(finalTranslation.GetY() * yScale);

    AZ::Vector2 finalTranslation2 = EntityHelpers::RoundXY(AZ::Vector2(finalTranslation.GetX(), finalTranslation.GetY()));
    ViewportSnap::ResizeByGizmo(hierarchy, canvasId, grabbedGizmoParts, element, pivot, finalTranslation2);
}

void ViewportElement::Rotate(HierarchyWidget* hierarchy,
    const AZ::EntityId& canvasId,
    const AZ::Vector2& lastMouseDragPos,
    const AZ::EntityId& activeElementId,
    AZ::Entity* element,
    const AZ::Vector2& mousePosition)
{
    // Find the vectors from the active element's pivot point to the last and current mouse positions
    AZ::Vector2 pivot;
    UiTransformBus::EventResult(pivot, activeElementId, &UiTransformBus::Events::GetViewportSpacePivot);
    AZ::Vector2 pivotToLastPos = lastMouseDragPos - pivot;
    AZ::Vector2 pivotToThisPos = mousePosition - pivot;

    // Find the signed angle between the two vectors
    pivotToLastPos.NormalizeSafe();
    pivotToThisPos.NormalizeSafe();
    float signedAngle = atan2(pivotToThisPos.GetY(), pivotToThisPos.GetX()) - atan2(pivotToLastPos.GetY(), pivotToLastPos.GetX());
    signedAngle = roundf(RAD2DEG(signedAngle));

    // if the combined parent transform is scaling just one of either X or Y negatively then the
    // element will rotate on screen in the opposite direction to the way the cursor is moved. So
    // we test for this and, if so, negate the angle to rotate
    AZ::Entity* parentElement = EntityHelpers::GetParentElement(element);
    AZ::Matrix4x4 parentMatrix;
    UiTransformBus::Event(parentElement->GetId(), &UiTransformBus::Events::GetTransformToViewport, parentMatrix);
    if (parentMatrix.GetElement(0, 0) * parentMatrix.GetElement(1, 1) < 0.0f)
    {
        signedAngle = -signedAngle;
    }

    ViewportSnap::Rotate(hierarchy, canvasId, element, signedAngle);
}

void ViewportElement::MoveAnchors(const ViewportHelpers::SelectedAnchors& grabbedAnchors,
    const UiTransform2dInterface::Anchors& startAnchors,
    const AZ::Vector2& startMouseDragPos,
    AZ::Entity* element,
    const AZ::Vector2& mousePosition,
    bool adjustOffsets)
{
    if (ViewportHelpers::IsControlledByLayout(element))
    {
        return;
    }

    // Get the matrix that transforms viewport points into canvas space points with no scale and rotation
    // This uses the parents transform component because anchors are in parent space
    AZ::Entity* parentElement = EntityHelpers::GetParentElement(element);
    AZ::Matrix4x4 parentTransformFromViewport;
    UiTransformBus::Event(parentElement->GetId(), &UiTransformBus::Events::GetTransformFromViewport, parentTransformFromViewport);

    // Get the parent's size in canvas space
    AZ::Vector2 parentSize;
    UiTransformBus::EventResult(parentSize, parentElement->GetId(), &UiTransformBus::Events::GetCanvasSpaceSizeNoScaleRotate);

    // Move the anchors
    AZ::Vector3 currPos3 = AZ::Vector3(mousePosition.GetX(), mousePosition.GetY(), 0.0f);
    AZ::Vector3 startPos3 = AZ::Vector3(startMouseDragPos.GetX(), startMouseDragPos.GetY(), 0.0f);
    AZ::Vector3 totalMouseTranslation = currPos3 - startPos3;

    AZ::Vector3 localTranslation3 = parentTransformFromViewport.Multiply3x3(totalMouseTranslation);
    AZ::Vector2 localTranslation(localTranslation3.GetX(), localTranslation3.GetY());
    localTranslation.SetX(parentSize.GetX() ? (localTranslation.GetX() / parentSize.GetX()) : 0.0f);
    localTranslation.SetY(parentSize.GetY() ? (localTranslation.GetY() / parentSize.GetY()) : 0.0f);

    auto newAnchors = ViewportHelpers::MoveGrabbedAnchor(startAnchors, grabbedAnchors, ViewportHelpers::IsHorizontallyFit(element),
        ViewportHelpers::IsVerticallyFit(element), localTranslation);
    UiTransform2dBus::Event(element->GetId(), &UiTransform2dBus::Events::SetAnchors, newAnchors, adjustOffsets, false);

    UiElementChangeNotificationBus::Event(element->GetId(), &UiElementChangeNotificationBus::Events::UiElementPropertyChanged);
}

void ViewportElement::MovePivot(const AZ::Vector2& lastMouseDragPos,
    AZ::Entity* element,
    const AZ::Vector2& mousePosition)
{
    // Get the element rect
    UiTransformInterface::RectPoints points;
    UiTransformBus::Event(element->GetId(), &UiTransformBus::Events::GetViewportSpacePoints, points);

    if (ViewportHelpers::IsControlledByLayout(element))
    {
        // Apply the inverse of this element's rotation and scale about pivot
        AZ::Matrix4x4 transform;
        UiTransformBus::Event(element->GetId(), &UiTransformBus::Events::GetLocalInverseTransform, transform);
        AZ::Vector3 topLeft(points.TopLeft().GetX(), points.TopLeft().GetY(), 0.0f);
        topLeft = transform * topLeft;
        AZ::Vector3 topRight(points.TopRight().GetX(), points.TopRight().GetY(), 0.0f);
        topRight = transform * topRight;
        AZ::Vector3 bottomLeft(points.BottomLeft().GetX(), points.BottomLeft().GetY(), 0.0f);
        bottomLeft = transform * bottomLeft;
        AZ::Vector3 bottomRight(points.BottomRight().GetX(), points.BottomRight().GetY(), 0.0f);
        bottomRight = transform * bottomRight;

        points.TopLeft() = AZ::Vector2(topLeft.GetX(), topLeft.GetY());
        points.TopRight() = AZ::Vector2(topRight.GetX(), topRight.GetY());
        points.BottomLeft() = AZ::Vector2(bottomLeft.GetX(), bottomLeft.GetY());
        points.BottomRight() = AZ::Vector2(bottomRight.GetX(), bottomRight.GetY());
    }

    // Find the element's down and right vectors
    AZ::Vector2 rightVec = points.TopRight() - points.TopLeft();
    AZ::Vector2 downVec = points.BottomLeft() - points.TopLeft();

    // Find the local translation in element space
    AZ::Vector2 mouseDelta = mousePosition - lastMouseDragPos;
    AZ::Vector2 localTranslation;
    localTranslation.SetX(mouseDelta.Dot(rightVec.GetNormalizedSafe()));
    localTranslation.SetY(mouseDelta.Dot(downVec.GetNormalizedSafe()));

    // Get the normalized translation
    float width = rightVec.GetLength();
    float height = downVec.GetLength();
    localTranslation.SetX(localTranslation.GetX() / width);
    localTranslation.SetY(localTranslation.GetY() / height);

    // Move the pivot point
    AZ::Vector2 currentPivot;
    UiTransformBus::EventResult(currentPivot, element->GetId(), &UiTransformBus::Events::GetPivot);
    if (ViewportHelpers::IsControlledByLayout(element))
    {
        UiTransformBus::Event(element->GetId(), &UiTransformBus::Events::SetPivot, currentPivot + localTranslation);
    }
    else
    {
        UiTransform2dBus::Event(element->GetId(), &UiTransform2dBus::Events::SetPivotAndAdjustOffsets, currentPivot + localTranslation);
    }

    UiElementChangeNotificationBus::Event(element->GetId(), &UiElementChangeNotificationBus::Events::UiElementPropertyChanged);
}
