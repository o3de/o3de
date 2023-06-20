/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "EditorCommon.h"
#include "ViewportAlign.h"
#include <LyShine/UiComponentTypes.h>

void ViewportAlign::AlignSelectedElements(EditorWindow* editorWindow, AlignType alignType)
{
    QTreeWidgetItemRawPtrQList selectedItems = editorWindow->GetHierarchy()->selectedItems();

    LyShine::EntityArray selectedElements = SelectionHelpers::GetTopLevelSelectedElements(editorWindow->GetHierarchy(), selectedItems);

    // elements that are controlled by a layout element cannot be moved. So make a list of the elements that
    // can be aligned.
    AZStd::vector<AZ::EntityId> elementsToAlign;
    for (auto element : selectedElements)
    {
        if (!ViewportHelpers::IsControlledByLayout(element))
        {
            elementsToAlign.push_back(element->GetId());
        }
    }
    
    // we have to have at least two elements in order to do the align operation
    size_t numElements = elementsToAlign.size();
    if (numElements < 2)
    {
        return;
    }

    // get mode to see if we are in MOVE or ANCHOR mode, in MOVE mode we modify offsets, in ANCHOR mode we modify anchors
    ViewportInteraction::InteractionMode interactionMode = editorWindow->GetViewport()->GetViewportInteraction()->GetMode();

    // start the undoable event
    SerializeHelpers::SerializedEntryList preChangeState;
    HierarchyClipboard::BeginUndoableEntitiesChange(editorWindow, preChangeState);

    // get the AABB of each element plus the overall AABB of all the top level selected elements
    const float minFloat = std::numeric_limits<float>::min();
    const float maxFloat = std::numeric_limits<float>::max();
    UiTransformInterface::Rect overallBoundingBox;
    overallBoundingBox.Set(maxFloat, minFloat, maxFloat, minFloat);
    AZStd::vector<UiTransformInterface::Rect> elementBoundingBoxes(numElements);
    for (int i = 0; i < numElements; ++i)
    {
        AZ::EntityId entityId = elementsToAlign[i];
        UiTransformInterface::RectPoints points;
        UiTransformBus::Event(entityId, &UiTransformBus::Events::GetCanvasSpacePoints, points);

        // setup the AABB for this element
        AZ::Vector2 topLeft = points.GetAxisAlignedTopLeft();
        AZ::Vector2 bottomRight = points.GetAxisAlignedBottomRight();
        elementBoundingBoxes[i].left = topLeft.GetX();
        elementBoundingBoxes[i].right = bottomRight.GetX();
        elementBoundingBoxes[i].top = topLeft.GetY();
        elementBoundingBoxes[i].bottom = bottomRight.GetY();

        // update the overall AABB
        overallBoundingBox.left = AZStd::GetMin(overallBoundingBox.left, topLeft.GetX());
        overallBoundingBox.right = AZStd::GetMax(overallBoundingBox.right, bottomRight.GetX());
        overallBoundingBox.top = AZStd::GetMin(overallBoundingBox.top, topLeft.GetY());
        overallBoundingBox.bottom = AZStd::GetMax(overallBoundingBox.bottom, bottomRight.GetY());
    }

    // For each element, compute the delta of where it is from where it should be
    // then adjust the offsets to align it
    for (int i = 0; i < numElements; ++i)
    {
        UiTransformInterface::Rect& aabb = elementBoundingBoxes[i];

        // the delta to move depends on the align type
        AZ::Vector2 deltaInCanvasSpace;
        switch (alignType)
        {
        case AlignType::HorizontalLeft:
            deltaInCanvasSpace.Set(overallBoundingBox.left - aabb.left, 0.0f);
            break;
        case AlignType::HorizontalCenter:
            deltaInCanvasSpace.Set(overallBoundingBox.GetCenterX() - aabb.GetCenterX(), 0.0f);
            break;
        case AlignType::HorizontalRight:
            deltaInCanvasSpace.Set(overallBoundingBox.right - aabb.right, 0.0f);
            break;
        case AlignType::VerticalTop:
            deltaInCanvasSpace.Set(0.0f, overallBoundingBox.top - aabb.top);
            break;
        case AlignType::VerticalCenter:
            deltaInCanvasSpace.Set(0.0f, overallBoundingBox.GetCenterY() - aabb.GetCenterY());
            break;
        case AlignType::VerticalBottom:
            deltaInCanvasSpace.Set(0.0f, overallBoundingBox.bottom - aabb.bottom);
            break;
        }

        // If this element needs to move...
        if (!deltaInCanvasSpace.IsZero())
        {
            AZ::EntityId entityId = elementsToAlign[i];

            AZ::EntityId parentEntityId = EntityHelpers::GetParentElement(entityId)->GetId();

            // compute the delta to move in local space (i.e. relative to the parent)
            AZ::Vector2 deltaInLocalSpace = EntityHelpers::TransformDeltaFromCanvasToLocalSpace(parentEntityId, deltaInCanvasSpace);

            // do the actual move of the element
            if (interactionMode == ViewportInteraction::InteractionMode::MOVE)
            {
                EntityHelpers::MoveByLocalDeltaUsingOffsets(entityId, deltaInLocalSpace);
            }
            else if (interactionMode == ViewportInteraction::InteractionMode::ANCHOR)
            {
                EntityHelpers::MoveByLocalDeltaUsingAnchors(entityId, parentEntityId, deltaInLocalSpace, true);
            }

            // Let listeners know that the properties on this element have changed
            UiElementChangeNotificationBus::Event(entityId, &UiElementChangeNotificationBus::Events::UiElementPropertyChanged);
        }
    }

    // Tell the Properties panel to update
    const AZ::Uuid transformComponentType = LyShine::UiTransform2dComponentUuid;
    editorWindow->GetProperties()->TriggerRefresh(AzToolsFramework::PropertyModificationRefreshLevel::Refresh_Values, &transformComponentType);

    // end the undoable event
    HierarchyClipboard::EndUndoableEntitiesChange(editorWindow, "align", preChangeState);
}

bool ViewportAlign::IsAlignAllowed(EditorWindow* editorWindow)
{
    // if nothing is selected then it is not allowed
    QTreeWidgetItemRawPtrQList selectedItems = editorWindow->GetHierarchy()->selectedItems();
    if (selectedItems.size() < 2)
    {
        return false;
    }

    LyShine::EntityArray selectedElements = SelectionHelpers::GetTopLevelSelectedElements(editorWindow->GetHierarchy(), selectedItems);

    // elements that are controlled by a layout element cannot be moved. So make a list of the elements that
    // can be aligned.
    AZStd::vector<AZ::EntityId> elementsToAlign;
    for (auto element : selectedElements)
    {
        if (!ViewportHelpers::IsControlledByLayout(element))
        {
            elementsToAlign.push_back(element->GetId());
        }
    }
    
    // we have to have at least two elements in order to do the align operation
    if (elementsToAlign.size() < 2)
    {
        return false;
    }

    return true;
}
