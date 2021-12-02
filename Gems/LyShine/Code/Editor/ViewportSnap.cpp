/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "EditorCommon.h"
#include "ViewportSnap.h"
#include <LyShine/Bus/UiEditorCanvasBus.h>

namespace
{
    UiTransform2dInterface::Offsets ResizeAboutPivot(const UiTransform2dInterface::Offsets& offsets,
        const ViewportHelpers::GizmoParts& grabbedGizmoParts,
        const AZ::Vector2& pivot,
        const AZ::Vector2& translation)
    {
        UiTransform2dInterface::Offsets result(offsets);

        if (grabbedGizmoParts.m_right)
        {
            result.m_left -= translation.GetX() * pivot.GetX();
            result.m_right += translation.GetX() * (1.0f - pivot.GetX());
        }
        if (grabbedGizmoParts.m_top)
        {
            result.m_top += translation.GetY() * pivot.GetY();
            result.m_bottom -= translation.GetY() * (1.0f - pivot.GetY());
        }

        return result;
    }
} // anonymous namespace.

void ViewportSnap::Rotate(HierarchyWidget* hierarchy,
    const AZ::EntityId& canvasId,
    AZ::Entity* element,
    float signedAngle)
{
    bool isSnapping = false;
    EBUS_EVENT_ID_RESULT(isSnapping, canvasId, UiEditorCanvasBus, GetIsSnapEnabled);

    if (isSnapping)
    {
        HierarchyItem* item = HierarchyItem::RttiCast(HierarchyHelpers::ElementToItem(hierarchy, element, false));

        // Update the non-snapped rotation.
        item->SetNonSnappedZRotation(item->GetNonSnappedZRotation() + signedAngle);

        // Update the currently used rotation.
        float realRotation;
        EBUS_EVENT_ID_RESULT(realRotation, element->GetId(), UiTransformBus, GetZRotation);

        float snappedRotation = item->GetNonSnappedZRotation();
        {
            float snapRotationInDegrees = 1.0f;
            EBUS_EVENT_ID_RESULT(snapRotationInDegrees, canvasId, UiEditorCanvasBus, GetSnapRotationDegrees);

            snappedRotation = EntityHelpers::Snap(snappedRotation, snapRotationInDegrees);
        }

        if (snappedRotation != realRotation)
        {
            EBUS_EVENT_ID(element->GetId(), UiTransformBus, SetZRotation, snappedRotation);
            EBUS_EVENT_ID(element->GetId(), UiElementChangeNotificationBus, UiElementPropertyChanged);
        }
    }
    else // if (!isSnapping)
    {
        // Add the angle to the current rotation of this element
        float rotation;
        EBUS_EVENT_ID_RESULT(rotation, element->GetId(), UiTransformBus, GetZRotation);

        EBUS_EVENT_ID(element->GetId(), UiTransformBus, SetZRotation, (rotation + signedAngle));
        EBUS_EVENT_ID(element->GetId(), UiElementChangeNotificationBus, UiElementPropertyChanged);
    }
}

void ViewportSnap::ResizeByGizmo(HierarchyWidget* hierarchy,
    const AZ::EntityId& canvasId,
    const ViewportHelpers::GizmoParts& grabbedGizmoParts,
    AZ::Entity* element,
    const AZ::Vector2& pivot,
    const AZ::Vector2& translation)
{
    bool isSnapping = false;
    EBUS_EVENT_ID_RESULT(isSnapping, canvasId, UiEditorCanvasBus, GetIsSnapEnabled);

    if (isSnapping)
    {
        HierarchyItem* item = HierarchyItem::RttiCast(HierarchyHelpers::ElementToItem(hierarchy, element, false));

        // Resize the element about the pivot
        UiTransform2dInterface::Offsets offsets(ResizeAboutPivot(item->GetNonSnappedOffsets(), grabbedGizmoParts, pivot, translation));

        // Update the non-snapped offset.
        item->SetNonSnappedOffsets(offsets);

        UiTransform2dInterface::Offsets snappedOffsets(item->GetNonSnappedOffsets());
        {
            float snapDistance = 1.0f;
            EBUS_EVENT_ID_RESULT(snapDistance, canvasId, UiEditorCanvasBus, GetSnapDistance);

            UiTransform2dInterface::Anchors anchors;
            EBUS_EVENT_ID_RESULT(anchors, element->GetId(), UiTransform2dBus, GetAnchors);

            AZ::Entity* parentElement = EntityHelpers::GetParentElement(element);

            AZ::Vector2 parentSize;
            EBUS_EVENT_ID_RESULT(parentSize, parentElement->GetId(), UiTransformBus, GetCanvasSpaceSizeNoScaleRotate);

            float newWidth  = parentSize.GetX() * (anchors.m_right  - anchors.m_left) + offsets.m_right  - offsets.m_left;
            float newHeight = parentSize.GetY() * (anchors.m_bottom - anchors.m_top)  + offsets.m_bottom - offsets.m_top;

            if (grabbedGizmoParts.m_right)
            {
                float snappedWidth = ViewportHelpers::IsHorizontallyFit(element) ? newWidth : EntityHelpers::Snap(newWidth, snapDistance);
                float deltaWidth = snappedWidth - newWidth;

                snappedOffsets.m_left = offsets.m_left - deltaWidth * pivot.GetX(); // move left when width increases, so decrease offset
                snappedOffsets.m_right = offsets.m_right + deltaWidth * (1.0f - pivot.GetX()); // move right when width increases so increase offset
            }

            if (grabbedGizmoParts.m_top)
            {
                float snappedHeight = ViewportHelpers::IsVerticallyFit(element) ? newHeight : EntityHelpers::Snap(newHeight, snapDistance);
                float deltaHeight = snappedHeight - newHeight;

                snappedOffsets.m_top = offsets.m_top - deltaHeight * pivot.GetY(); // move left when width increases, so decrease offset
                snappedOffsets.m_bottom = offsets.m_bottom + deltaHeight * (1.0f - pivot.GetY()); // move right when width increases so increase offset
            }
        }

        // Update the currently used offset.
        if (snappedOffsets != offsets)
        {
            EBUS_EVENT_ID(element->GetId(), UiTransform2dBus, SetOffsets, snappedOffsets);
            EBUS_EVENT_ID(element->GetId(), UiElementChangeNotificationBus, UiElementPropertyChanged);
        }
    }
    else // if (!isSnapping)
    {
        // Resize the element about the pivot
        UiTransform2dInterface::Offsets offsets;
        EBUS_EVENT_ID_RESULT(offsets, element->GetId(), UiTransform2dBus, GetOffsets);

        UiTransform2dInterface::Offsets newOffsets(ResizeAboutPivot(offsets, grabbedGizmoParts, pivot, translation));

        // Resize the element
        EBUS_EVENT_ID(element->GetId(), UiTransform2dBus, SetOffsets, newOffsets);

        EBUS_EVENT_ID(element->GetId(), UiElementChangeNotificationBus, UiElementPropertyChanged);
    }
}

void ViewportSnap::ResizeDirectlyWithScaleOrRotation(HierarchyWidget* hierarchy,
    const AZ::EntityId& canvasId,
    const ViewportHelpers::ElementEdges& grabbedEdges,
    AZ::Entity* element,
    const UiTransformInterface::RectPoints& translation)
{
    bool isSnapping = false;
    EBUS_EVENT_ID_RESULT(isSnapping, canvasId, UiEditorCanvasBus, GetIsSnapEnabled);

    if (isSnapping)
    {
        HierarchyItem* item = HierarchyItem::RttiCast(HierarchyHelpers::ElementToItem(hierarchy, element, false));

        // Update the non-snapped offset.
        item->SetNonSnappedOffsets(item->GetNonSnappedOffsets() + translation);

        // Update the currently used offset.
        {
            UiTransform2dInterface::Offsets currentOffsets;
            EBUS_EVENT_ID_RESULT(currentOffsets, element->GetId(), UiTransform2dBus, GetOffsets);

            UiTransform2dInterface::Offsets snappedOffsets(item->GetNonSnappedOffsets());
            {
                float snapDistance = 1.0f;
                EBUS_EVENT_ID_RESULT(snapDistance, canvasId, UiEditorCanvasBus, GetSnapDistance);

                snappedOffsets = EntityHelpers::Snap(snappedOffsets, grabbedEdges, snapDistance);
            }

            if (snappedOffsets != currentOffsets)
            {
                EBUS_EVENT_ID(element->GetId(), UiTransform2dBus, SetOffsets, snappedOffsets);
                EBUS_EVENT_ID(element->GetId(), UiElementChangeNotificationBus, UiElementPropertyChanged);
            }
        }
    }
    else // if (!isSnapping)
    {
        // Resize the element
        UiTransform2dInterface::Offsets offsets;
        EBUS_EVENT_ID_RESULT(offsets, element->GetId(), UiTransform2dBus, GetOffsets);

        EBUS_EVENT_ID(element->GetId(), UiTransform2dBus, SetOffsets, (offsets + translation));

        EBUS_EVENT_ID(element->GetId(), UiElementChangeNotificationBus, UiElementPropertyChanged);
    }
}

void ViewportSnap::ResizeDirectlyNoScaleNoRotation(HierarchyWidget* hierarchy,
    const AZ::EntityId& canvasId,
    const ViewportHelpers::ElementEdges& grabbedEdges,
    AZ::Entity* element,
    const AZ::Vector2& translation)
{
    bool isSnapping = false;
    EBUS_EVENT_ID_RESULT(isSnapping, canvasId, UiEditorCanvasBus, GetIsSnapEnabled);

    if (isSnapping)
    {
        HierarchyItem* item = HierarchyItem::RttiCast(HierarchyHelpers::ElementToItem(hierarchy, element, false));

        // Update the non-snapped offset.
        item->SetNonSnappedOffsets(ViewportHelpers::MoveGrabbedEdges(item->GetNonSnappedOffsets(), grabbedEdges, translation));

        // Update the currently used offset.
        {
            UiTransform2dInterface::Offsets currentOffsets;
            EBUS_EVENT_ID_RESULT(currentOffsets, element->GetId(), UiTransform2dBus, GetOffsets);

            UiTransform2dInterface::Offsets snappedOffsets(item->GetNonSnappedOffsets());
            {
                float snapDistance = 1.0f;
                EBUS_EVENT_ID_RESULT(snapDistance, canvasId, UiEditorCanvasBus, GetSnapDistance);

                snappedOffsets = EntityHelpers::Snap(snappedOffsets, grabbedEdges, snapDistance);
            }

            if (snappedOffsets != currentOffsets)
            {
                EBUS_EVENT_ID(element->GetId(), UiTransform2dBus, SetOffsets, snappedOffsets);
                EBUS_EVENT_ID(element->GetId(), UiElementChangeNotificationBus, UiElementPropertyChanged);
            }
        }
    }
    else // if (!isSnapping)
    {
        // Resize the element
        UiTransform2dInterface::Offsets offsets;
        EBUS_EVENT_ID_RESULT(offsets, element->GetId(), UiTransform2dBus, GetOffsets);

        EBUS_EVENT_ID(element->GetId(), UiTransform2dBus, SetOffsets, ViewportHelpers::MoveGrabbedEdges(offsets, grabbedEdges, translation));

        EBUS_EVENT_ID(element->GetId(), UiElementChangeNotificationBus, UiElementPropertyChanged);
    }
}
