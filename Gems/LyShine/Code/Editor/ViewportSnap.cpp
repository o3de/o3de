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
    UiEditorCanvasBus::EventResult(isSnapping, canvasId, &UiEditorCanvasBus::Events::GetIsSnapEnabled);

    if (isSnapping)
    {
        HierarchyItem* item = HierarchyItem::RttiCast(HierarchyHelpers::ElementToItem(hierarchy, element, false));

        // Update the non-snapped rotation.
        item->SetNonSnappedZRotation(item->GetNonSnappedZRotation() + signedAngle);

        // Update the currently used rotation.
        float realRotation;
        UiTransformBus::EventResult(realRotation, element->GetId(), &UiTransformBus::Events::GetZRotation);

        float snappedRotation = item->GetNonSnappedZRotation();
        {
            float snapRotationInDegrees = 1.0f;
            UiEditorCanvasBus::EventResult(snapRotationInDegrees, canvasId, &UiEditorCanvasBus::Events::GetSnapRotationDegrees);

            snappedRotation = EntityHelpers::Snap(snappedRotation, snapRotationInDegrees);
        }

        if (snappedRotation != realRotation)
        {
            UiTransformBus::Event(element->GetId(), &UiTransformBus::Events::SetZRotation, snappedRotation);
            UiElementChangeNotificationBus::Event(element->GetId(), &UiElementChangeNotificationBus::Events::UiElementPropertyChanged);
        }
    }
    else // if (!isSnapping)
    {
        // Add the angle to the current rotation of this element
        float rotation;
        UiTransformBus::EventResult(rotation, element->GetId(), &UiTransformBus::Events::GetZRotation);

        UiTransformBus::Event(element->GetId(), &UiTransformBus::Events::SetZRotation, (rotation + signedAngle));
        UiElementChangeNotificationBus::Event(element->GetId(), &UiElementChangeNotificationBus::Events::UiElementPropertyChanged);
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
    UiEditorCanvasBus::EventResult(isSnapping, canvasId, &UiEditorCanvasBus::Events::GetIsSnapEnabled);

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
            UiEditorCanvasBus::EventResult(snapDistance, canvasId, &UiEditorCanvasBus::Events::GetSnapDistance);

            UiTransform2dInterface::Anchors anchors;
            UiTransform2dBus::EventResult(anchors, element->GetId(), &UiTransform2dBus::Events::GetAnchors);

            AZ::Entity* parentElement = EntityHelpers::GetParentElement(element);

            AZ::Vector2 parentSize;
            UiTransformBus::EventResult(parentSize, parentElement->GetId(), &UiTransformBus::Events::GetCanvasSpaceSizeNoScaleRotate);

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
            UiTransform2dBus::Event(element->GetId(), &UiTransform2dBus::Events::SetOffsets, snappedOffsets);
            UiElementChangeNotificationBus::Event(element->GetId(), &UiElementChangeNotificationBus::Events::UiElementPropertyChanged);
        }
    }
    else // if (!isSnapping)
    {
        // Resize the element about the pivot
        UiTransform2dInterface::Offsets offsets;
        UiTransform2dBus::EventResult(offsets, element->GetId(), &UiTransform2dBus::Events::GetOffsets);

        UiTransform2dInterface::Offsets newOffsets(ResizeAboutPivot(offsets, grabbedGizmoParts, pivot, translation));

        // Resize the element
        UiTransform2dBus::Event(element->GetId(), &UiTransform2dBus::Events::SetOffsets, newOffsets);

        UiElementChangeNotificationBus::Event(element->GetId(), &UiElementChangeNotificationBus::Events::UiElementPropertyChanged);
    }
}

void ViewportSnap::ResizeDirectlyWithScaleOrRotation(HierarchyWidget* hierarchy,
    const AZ::EntityId& canvasId,
    const ViewportHelpers::ElementEdges& grabbedEdges,
    AZ::Entity* element,
    const UiTransformInterface::RectPoints& translation)
{
    bool isSnapping = false;
    UiEditorCanvasBus::EventResult(isSnapping, canvasId, &UiEditorCanvasBus::Events::GetIsSnapEnabled);

    if (isSnapping)
    {
        HierarchyItem* item = HierarchyItem::RttiCast(HierarchyHelpers::ElementToItem(hierarchy, element, false));

        // Update the non-snapped offset.
        item->SetNonSnappedOffsets(item->GetNonSnappedOffsets() + translation);

        // Update the currently used offset.
        {
            UiTransform2dInterface::Offsets currentOffsets;
            UiTransform2dBus::EventResult(currentOffsets, element->GetId(), &UiTransform2dBus::Events::GetOffsets);

            UiTransform2dInterface::Offsets snappedOffsets(item->GetNonSnappedOffsets());
            {
                float snapDistance = 1.0f;
                UiEditorCanvasBus::EventResult(snapDistance, canvasId, &UiEditorCanvasBus::Events::GetSnapDistance);

                snappedOffsets = EntityHelpers::Snap(snappedOffsets, grabbedEdges, snapDistance);
            }

            if (snappedOffsets != currentOffsets)
            {
                UiTransform2dBus::Event(element->GetId(), &UiTransform2dBus::Events::SetOffsets, snappedOffsets);
                UiElementChangeNotificationBus::Event(element->GetId(), &UiElementChangeNotificationBus::Events::UiElementPropertyChanged);
            }
        }
    }
    else // if (!isSnapping)
    {
        // Resize the element
        UiTransform2dInterface::Offsets offsets;
        UiTransform2dBus::EventResult(offsets, element->GetId(), &UiTransform2dBus::Events::GetOffsets);

        UiTransform2dBus::Event(element->GetId(), &UiTransform2dBus::Events::SetOffsets, (offsets + translation));

        UiElementChangeNotificationBus::Event(element->GetId(), &UiElementChangeNotificationBus::Events::UiElementPropertyChanged);
    }
}

void ViewportSnap::ResizeDirectlyNoScaleNoRotation(HierarchyWidget* hierarchy,
    const AZ::EntityId& canvasId,
    const ViewportHelpers::ElementEdges& grabbedEdges,
    AZ::Entity* element,
    const AZ::Vector2& translation)
{
    bool isSnapping = false;
    UiEditorCanvasBus::EventResult(isSnapping, canvasId, &UiEditorCanvasBus::Events::GetIsSnapEnabled);

    if (isSnapping)
    {
        HierarchyItem* item = HierarchyItem::RttiCast(HierarchyHelpers::ElementToItem(hierarchy, element, false));

        // Update the non-snapped offset.
        item->SetNonSnappedOffsets(ViewportHelpers::MoveGrabbedEdges(item->GetNonSnappedOffsets(), grabbedEdges, translation));

        // Update the currently used offset.
        {
            UiTransform2dInterface::Offsets currentOffsets;
            UiTransform2dBus::EventResult(currentOffsets, element->GetId(), &UiTransform2dBus::Events::GetOffsets);

            UiTransform2dInterface::Offsets snappedOffsets(item->GetNonSnappedOffsets());
            {
                float snapDistance = 1.0f;
                UiEditorCanvasBus::EventResult(snapDistance, canvasId, &UiEditorCanvasBus::Events::GetSnapDistance);

                snappedOffsets = EntityHelpers::Snap(snappedOffsets, grabbedEdges, snapDistance);
            }

            if (snappedOffsets != currentOffsets)
            {
                UiTransform2dBus::Event(element->GetId(), &UiTransform2dBus::Events::SetOffsets, snappedOffsets);
                UiElementChangeNotificationBus::Event(element->GetId(), &UiElementChangeNotificationBus::Events::UiElementPropertyChanged);
            }
        }
    }
    else // if (!isSnapping)
    {
        // Resize the element
        UiTransform2dInterface::Offsets offsets;
        UiTransform2dBus::EventResult(offsets, element->GetId(), &UiTransform2dBus::Events::GetOffsets);

        UiTransform2dBus::Event(element->GetId(), &UiTransform2dBus::Events::SetOffsets, ViewportHelpers::MoveGrabbedEdges(offsets, grabbedEdges, translation));

        UiElementChangeNotificationBus::Event(element->GetId(), &UiElementChangeNotificationBus::Events::UiElementPropertyChanged);
    }
}
