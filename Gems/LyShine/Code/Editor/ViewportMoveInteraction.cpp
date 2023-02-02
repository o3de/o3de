/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "EditorCommon.h"

#include "ViewportMoveInteraction.h"
#include <LyShine/Bus/UiEditorCanvasBus.h>


ViewportMoveInteraction::ViewportMoveInteraction(
    HierarchyWidget* hierarchy,
    const QTreeWidgetItemRawPtrQList& selectedItems,
    AZ::EntityId canvasId,
    AZ::Entity* activeElement,
    ViewportInteraction::CoordinateSystem coordinateSystem,
    ViewportHelpers::GizmoParts grabbedGizmoParts,
    ViewportInteraction::InteractionMode interactionMode,
    ViewportInteraction::InteractionType interactionType,
    const AZ::Vector2& startDragMousePos
)
    : ViewportDragInteraction(startDragMousePos)
    , m_canvasId(canvasId)
    , m_coordinateSystem(coordinateSystem)
    , m_grabbedGizmoParts(grabbedGizmoParts)
    , m_interactionMode(interactionMode)
    , m_interactionType(interactionType)
{
    LyShine::EntityArray topLevelSelectedElements = SelectionHelpers::GetTopLevelSelectedElements(hierarchy, selectedItems);

    // store the starting anchors or offsets (depending on the interaction mode)
    if (m_interactionMode == ViewportInteraction::InteractionMode::MOVE)
    {
        for (auto element : topLevelSelectedElements)
        {
            UiTransform2dInterface::Offsets offsets;
            UiTransform2dBus::EventResult(offsets, element->GetId(), &UiTransform2dBus::Events::GetOffsets);
            m_startingOffsets[element->GetId()] = offsets;
        }
    }
    else
    {
        for (auto element : topLevelSelectedElements)
        {
            UiTransform2dInterface::Anchors anchors;
            UiTransform2dBus::EventResult(anchors, element->GetId(), &UiTransform2dBus::Events::GetAnchors);
            m_startingAnchors[element->GetId()] = anchors;
        }
    }

    // the primary element is usually the active element (the one being clicked on and dragged),
    // but if a parent of the active element is also selected it is the top-level selected ancestor of the active element
    m_primaryElement = SelectionHelpers::GetTopLevelParentOfElement(topLevelSelectedElements, activeElement);

    if (m_primaryElement)
    {
        // remove the primary element from the array
        SelectionHelpers::RemoveEntityFromArray(topLevelSelectedElements, m_primaryElement);

        // store the top-level selected elements that are not the primary element - these will just follow along with
        // how the primary element is moved
        m_secondarySelectedElements = topLevelSelectedElements;

        // store whether snapping is enabled for this canvas
        UiEditorCanvasBus::EventResult(m_isSnapping, canvasId, &UiEditorCanvasBus::Events::GetIsSnapEnabled);

        // remember the parent of the primary element also
        m_primaryElementParent = EntityHelpers::GetParentElement(m_primaryElement);

        // store the starting pivots of the primary element for snapping (in local and canvas space)
        m_startingPrimaryLocalPivot = GetPivotRelativeToTopLeftAnchor(m_primaryElement->GetId());
        UiTransformBus::EventResult(
            m_startingPrimaryCanvasSpacePivot, m_primaryElement->GetId(), &UiTransformBus::Events::GetCanvasSpacePivot);
    }
    else
    {
        // This should never happen but when we had an assert here it was occasionally hit but not in a reproducable way.
        // It is recoverable so we don't want to crash if this happens. Report a warning and do not crash.
        AZ_Warning("UI", false, "The active element is not one of the selected elements. Active element is '%s'. There are %d selected elements and %d top level selected elements.",
            activeElement ? activeElement->GetName().c_str() : "None", selectedItems.count(), topLevelSelectedElements.size());
    }
}

ViewportMoveInteraction::~ViewportMoveInteraction()
{
}

void ViewportMoveInteraction::Update(const AZ::Vector2& mousePos)
{
    // If there is no primary element (should never happen) or if the primary element is controlled by a layout component
    // and therefore not movable, then do nothing
    if (!m_primaryElement || ViewportHelpers::IsControlledByLayout(m_primaryElement))
    {
        return;
    }

    // compute the total mouse delta since the start of the interaction
    AZ::Vector2 mouseDelta = mousePos - m_startMousePos;

    // Constrain this delta based on gizmo and coordinate space
    AZ::Vector2 canvasSpaceMouseDelta;
    AZ::Vector2 localMouseDelta;
    bool restrictDirection = ConstrainMovementDirection(mouseDelta, canvasSpaceMouseDelta, localMouseDelta);

    // adjust the mouse deltas for snapping
    SnapMouseDeltas(canvasSpaceMouseDelta, localMouseDelta);

    // Move the primary element
    MovePrimaryElement(restrictDirection, canvasSpaceMouseDelta, localMouseDelta);

    // Move each of the secondary elements by the same delta
    for (auto element : m_secondarySelectedElements)
    {
        if (!ViewportHelpers::IsControlledByLayout(element))
        {
            MoveSecondaryElement(element, restrictDirection, canvasSpaceMouseDelta);
        }
    }
}

bool ViewportMoveInteraction::ConstrainMovementDirection(const AZ::Vector2& mouseDelta, AZ::Vector2& canvasSpaceMouseDelta, AZ::Vector2& localMouseDelta)
{
    bool restrictDirection = false;

    canvasSpaceMouseDelta = EntityHelpers::TransformDeltaFromViewportToCanvasSpace(m_canvasId, mouseDelta);

    if (m_interactionType == ViewportInteraction::InteractionType::TRANSFORM_GIZMO && m_grabbedGizmoParts.Single())
    {
        restrictDirection = true;

        if (m_coordinateSystem == ViewportInteraction::CoordinateSystem::LOCAL)
        {
            // For LOCAL space, we need to transform the translation from canvas space to the parent element's local space
            localMouseDelta = EntityHelpers::TransformDeltaFromCanvasToLocalSpace(m_primaryElementParent->GetId(), canvasSpaceMouseDelta);

            // Zero-out the non-moving axis in local space.
            if (!m_grabbedGizmoParts.m_right)
            {
                localMouseDelta.SetX(0.0f);
            }
            if (!m_grabbedGizmoParts.m_top)
            {
                localMouseDelta.SetY(0.0f);
            }

            // now convert back to canvas space
            canvasSpaceMouseDelta = EntityHelpers::TransformDeltaFromLocalToCanvasSpace(m_primaryElementParent->GetId(), localMouseDelta);
        }
        else // if (coordinateSystem == ViewportInteraction::CoordinateSystem::VIEW)
        {
            // Zero-out the non-moving axis in viewport space.
            if (!m_grabbedGizmoParts.m_right)
            {
                canvasSpaceMouseDelta.SetX(0.0f);
            }
            if (!m_grabbedGizmoParts.m_top)
            {
                canvasSpaceMouseDelta.SetY(0.0f);
            }

            // For LOCAL space, we need to transform the translation from canvas space to the parent element's local space
            localMouseDelta = EntityHelpers::TransformDeltaFromCanvasToLocalSpace(m_primaryElementParent->GetId(), canvasSpaceMouseDelta);
        }
    }
    else
    {
        // Not single gizmo - just convert canvas translation to local
        localMouseDelta = EntityHelpers::TransformDeltaFromCanvasToLocalSpace(m_primaryElementParent->GetId(), canvasSpaceMouseDelta);
    }

    return restrictDirection;
}

void ViewportMoveInteraction::SnapMouseDeltas(AZ::Vector2& canvasSpaceMouseDelta, AZ::Vector2& localMouseDelta)
{
    if (m_isSnapping)
    {
        float snapDistance = 1.0f;
        UiEditorCanvasBus::EventResult(snapDistance, m_canvasId, &UiEditorCanvasBus::Events::GetSnapDistance);

        if (m_coordinateSystem == ViewportInteraction::CoordinateSystem::LOCAL)
        {
            // calculate what the pivot of the active element should be (ignoring snapping)
            AZ::Vector2 translatedLocalPivot =  m_startingPrimaryLocalPivot + localMouseDelta;

            // snap the position of the pivot
            AZ::Vector2 snappedPivot = EntityHelpers::Snap(translatedLocalPivot, snapDistance);
            AZ::Vector2 localSnapAdjustment = snappedPivot - translatedLocalPivot;

            // adjust the local translation to allow for snapping
            if (m_grabbedGizmoParts.Single())
            {
                if (m_grabbedGizmoParts.m_right)
                {
                    localMouseDelta.SetX(localMouseDelta.GetX() + localSnapAdjustment.GetX());
                }
                if (m_grabbedGizmoParts.m_top)
                {
                    localMouseDelta.SetY(localMouseDelta.GetY() + localSnapAdjustment.GetY());
                }
            }
            else
            {
                localMouseDelta += localSnapAdjustment;
            }

            // Compute a canvas space delta based on local delta
            canvasSpaceMouseDelta = EntityHelpers::TransformDeltaFromLocalToCanvasSpace(m_primaryElementParent->GetId(), localMouseDelta);
        }
        else
        {
            // calculate what the pivot of the active element should be (ignoring snapping)
            AZ::Vector2 translatedCanvasSpacePivot =  m_startingPrimaryCanvasSpacePivot + canvasSpaceMouseDelta;

            // snap the position of the pivot
            AZ::Vector2 snappedPivot = EntityHelpers::Snap(translatedCanvasSpacePivot, snapDistance);
            AZ::Vector2 canvasSpaceSnapAdjustment = snappedPivot - translatedCanvasSpacePivot;

            // adjust the local translation to allow for snapping
            if (m_grabbedGizmoParts.Single())
            {
                if (m_grabbedGizmoParts.m_right)
                {
                    canvasSpaceMouseDelta.SetX(canvasSpaceMouseDelta.GetX() + canvasSpaceSnapAdjustment.GetX());
                }
                if (m_grabbedGizmoParts.m_top)
                {
                    canvasSpaceMouseDelta.SetY(canvasSpaceMouseDelta.GetY() + canvasSpaceSnapAdjustment.GetY());
                }
            }
            else
            {
                canvasSpaceMouseDelta += canvasSpaceSnapAdjustment;
            }

            // Compute a local delta based on canvas space delta
            localMouseDelta = EntityHelpers::TransformDeltaFromCanvasToLocalSpace(m_primaryElementParent->GetId(), canvasSpaceMouseDelta);
        }
    }
}

void ViewportMoveInteraction::MovePrimaryElement(bool restrictDirection, AZ::Vector2& canvasSpaceMouseDelta, AZ::Vector2& localMouseDelta)
{
    if (m_interactionMode == ViewportInteraction::InteractionMode::MOVE)
    {
        const UiTransform2dInterface::Offsets& startingOffsets = m_startingOffsets[m_primaryElement->GetId()];
        EntityHelpers::MoveByLocalDeltaUsingOffsets(m_primaryElement->GetId(), startingOffsets, localMouseDelta);
    }
    else if (m_interactionMode == ViewportInteraction::InteractionMode::ANCHOR)
    {
        const UiTransform2dInterface::Anchors& startingAnchors = m_startingAnchors[m_primaryElement->GetId()];

        AZ::Vector2 constrainedLocalTranslation = EntityHelpers::MoveByLocalDeltaUsingAnchors(m_primaryElement->GetId(), m_primaryElementParent->GetId(),
            startingAnchors, localMouseDelta, restrictDirection);

        if (constrainedLocalTranslation != localMouseDelta)
        {
            // The anchor limits prevented moving the active element the requested amount.
            // We want the secondary elements to move the same amount as the primary one.
            // NOTE: if the secondary elements hit an anchor limit they will be constrained but other elements will not - so relative positions
            // are not ALWAYS preserved.
            localMouseDelta = constrainedLocalTranslation;

            // Recompute the canvas space delta based on local delta
            canvasSpaceMouseDelta = EntityHelpers::TransformDeltaFromLocalToCanvasSpace(m_primaryElementParent->GetId(), localMouseDelta);
        }
    }

    UiElementChangeNotificationBus::Event(m_primaryElement->GetId(), &UiElementChangeNotificationBus::Events::UiElementPropertyChanged);
}

void ViewportMoveInteraction::MoveSecondaryElement(AZ::Entity* element, bool restrictDirection, const AZ::Vector2& canvasSpaceMouseDelta)
{
    AZ::Entity* parentElement = EntityHelpers::GetParentElement(element);

    AZ::Vector2 localMouseDelta = EntityHelpers::TransformDeltaFromCanvasToLocalSpace(parentElement->GetId(), canvasSpaceMouseDelta);

    if (m_interactionMode == ViewportInteraction::InteractionMode::MOVE)
    {
        const UiTransform2dInterface::Offsets& startingOffsets = m_startingOffsets[element->GetId()];

        EntityHelpers::MoveByLocalDeltaUsingOffsets(element->GetId(), startingOffsets, localMouseDelta);
    }
    else if (m_interactionMode == ViewportInteraction::InteractionMode::ANCHOR)
    {
        const UiTransform2dInterface::Anchors& startingAnchors = m_startingAnchors[element->GetId()];

        EntityHelpers::MoveByLocalDeltaUsingAnchors(element->GetId(), parentElement->GetId(), startingAnchors, localMouseDelta, restrictDirection);
    }

    UiElementChangeNotificationBus::Event(element->GetId(), &UiElementChangeNotificationBus::Events::UiElementPropertyChanged);
}

AZ::Vector2 ViewportMoveInteraction::GetPivotRelativeToTopLeftAnchor(AZ::EntityId entityId)
{
    UiTransform2dInterface::Offsets currentOffsets;
    UiTransform2dBus::EventResult(currentOffsets, entityId, &UiTransform2dBus::Events::GetOffsets);

    // Get the width and height in canvas space no scale rotate.
    AZ::Vector2 elementSize;
    UiTransformBus::EventResult(elementSize, entityId, &UiTransformBus::Events::GetCanvasSpaceSizeNoScaleRotate);

    AZ::Vector2 pivot;
    UiTransformBus::EventResult(pivot, entityId, &UiTransformBus::Events::GetPivot);

    AZ::Vector2 pivotRelativeToTopLeftAnchor(currentOffsets.m_left + (elementSize.GetX() * pivot.GetX()),
        currentOffsets.m_top + (elementSize.GetY() * pivot.GetY()));

    return pivotRelativeToTopLeftAnchor;
}

