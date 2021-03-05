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

#include <AzToolsFramework/API/ToolsApplicationAPI.h>

namespace EntityHelpers
{
    using EntityIdList = AzToolsFramework::EntityIdList;

    using EntityToHierarchyItemMap = std::map< AZ::EntityId, HierarchyItem* >;

    AZ::Vector2 RoundXY(const AZ::Vector2& v);
    AZ::Vector3 RoundXY(const AZ::Vector3& v);

    AZ::Vector3 MakeVec3(const AZ::Vector2& v);

    float Snap(float value, float snapDistance);
    AZ::Vector2 Snap(const AZ::Vector2& v, float snapDistance);
    UiTransform2dInterface::Offsets Snap(const UiTransform2dInterface::Offsets& offs, const ViewportHelpers::ElementEdges& grabbedEdges, float snapDistance);

    void MoveElementToGlobalPosition(AZ::Entity* element, const QPoint& globalPos);

    //! Helper function to get parent element using Ebus.
    AZ::Entity* GetParentElement(const AZ::Entity* element);
    AZ::Entity* GetParentElement(const AZ::EntityId& elementId);

    AZ::Entity* GetEntity(AZ::EntityId id);

    void ComputeCanvasSpaceRectNoScaleRotate(AZ::EntityId elementId, UiTransform2dInterface::Offsets offsets, UiTransformInterface::Rect& rect);
    AZ::Vector2 ComputeCanvasSpacePivotNoScaleRotate(AZ::EntityId elementId, UiTransform2dInterface::Offsets offsets);

    AZStd::string GetHierarchicalElementName(AZ::EntityId entityId);

    //! Returns the common ancestor of element1 and element2 and also the children of that common ancestor
    //! that element1 and element2 are descended from
    AZ::Entity* GetCommonAncestor(AZ::Entity* element1, AZ::Entity* element2,
        AZ::Entity*& element1NextAncestor, AZ::Entity*& element2NextAncestor);

    //! returns true if element1 is before element2 in the element hierarchy
    bool CompareOrderInElementHierarchy(AZ::Entity* element1, AZ::Entity* element2);

    //! Move an element by a local pixel offset using the offsets
    void MoveByLocalDeltaUsingOffsets(AZ::EntityId entityId, AZ::Vector2 deltaInLocalSpace);

    //! Move an element by a local pixel offset using the offsets from a given starting offset
    void MoveByLocalDeltaUsingOffsets(AZ::EntityId entityId, const UiTransform2dInterface::Offsets& startingOffsets, AZ::Vector2 deltaInLocalSpace);

    //! Move an element by a local pixel offset using the anchors
    AZ::Vector2 MoveByLocalDeltaUsingAnchors(AZ::EntityId entityId, AZ::EntityId parentEntityId, AZ::Vector2 deltaInLocalSpace, bool restrictDirection);

    //! Move an element by a local pixel offset using the anchors from a given starting offset
    AZ::Vector2 MoveByLocalDeltaUsingAnchors(AZ::EntityId entityId, AZ::EntityId parentEntityId,
        const UiTransform2dInterface::Anchors& startingAnchors, AZ::Vector2 deltaInLocalSpace, bool restrictDirection);

    //! Given a delta in canvas space transform it to local space. Note: for a move you want to pass in the parent element
    AZ::Vector2 TransformDeltaFromCanvasToLocalSpace(AZ::EntityId entityId, AZ::Vector2 deltaInCanvasSpace);

    //! Given a delta in local space transform it to canvas space. Note: for a move you want to pass in the parent element
    AZ::Vector2 TransformDeltaFromLocalToCanvasSpace(AZ::EntityId entityId, AZ::Vector2 deltaInLocalSpace);

    //! Given a delta in viewport space transform it to canvas space.
    AZ::Vector2 TransformDeltaFromViewportToCanvasSpace(AZ::EntityId canvasEntityId, AZ::Vector2 deltaInViewportSpace);

}   // namespace EntityHelpers
