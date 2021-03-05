/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#pragma once

#include "ViewportDragInteraction.h"
#include "ViewportInteraction.h"

#include <LyShine/Bus/UiTransform2dBus.h>

//! Class used while a move interaction is in progress in move or anchor mode
class ViewportMoveInteraction
    : public ViewportDragInteraction
{
public:

    ViewportMoveInteraction(
        HierarchyWidget* hierarchy,
        const QTreeWidgetItemRawPtrQList& selectedItems,
        AZ::EntityId canvasId,
        AZ::Entity* activeElement,
        ViewportInteraction::CoordinateSystem coordinateSystem,
        ViewportHelpers::GizmoParts grabbedGizmoParts,
        ViewportInteraction::InteractionMode interactionMode,
        ViewportInteraction::InteractionType interactionType,
        const AZ::Vector2& startDragMousePos);

    virtual ~ViewportMoveInteraction();

    // ViewportDragInteraction
    void Update(const AZ::Vector2& mousePos) override;
    // ~ViewportDragInteraction

protected:

    bool ConstrainMovementDirection(const AZ::Vector2& mouseDelta, AZ::Vector2& canvasSpaceMouseDelta, AZ::Vector2& localMouseDelta);
    void SnapMouseDeltas(AZ::Vector2& canvasSpaceMouseDelta, AZ::Vector2& localMouseDelta);

    //! Move the primary element by the given delta. This can actually change the mouse deltas due to constraints.
    void MovePrimaryElement(bool restrictDirection, AZ::Vector2& canvasSpaceMouseDelta, AZ::Vector2& localMouseDelta);

    //! Move one of the additional selected elements
    void MoveSecondaryElement(AZ::Entity* element, bool restrictDirection, const AZ::Vector2& canvasSpaceMouseDelta);

    //! Get the offset of the pivot from the top-left anchor in local space pixels
    AZ::Vector2 GetPivotRelativeToTopLeftAnchor(AZ::EntityId entityId);

protected:

    // State that we will need every frame in the update is cached locally in this object

    AZ::Entity* m_primaryElement = nullptr;
    AZ::Entity* m_primaryElementParent = nullptr;

    LyShine::EntityArray m_secondarySelectedElements;

    AZ::Vector2 m_startingPrimaryLocalPivot;
    AZ::Vector2 m_startingPrimaryCanvasSpacePivot;

    bool m_isSnapping = false;

    AZ::EntityId m_canvasId;

    ViewportInteraction::CoordinateSystem m_coordinateSystem;
    ViewportHelpers::GizmoParts m_grabbedGizmoParts;
    ViewportInteraction::InteractionMode m_interactionMode;
    ViewportInteraction::InteractionType m_interactionType;

    // For all elements store the offsets or anchors at the start (depending on the mode)
    // We could separate this class into 2 for the anchor and move modes and only need one of these.

    AZStd::map<AZ::EntityId, UiTransform2dInterface::Offsets> m_startingOffsets;
    AZStd::map<AZ::EntityId, UiTransform2dInterface::Anchors> m_startingAnchors;
};
