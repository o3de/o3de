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

#include "ViewportInteraction.h"

class ViewportElement
{
public:

    // Used to determine what the cursor is hovering over
    static bool PickElementEdges(const AZ::Entity* element,
        const AZ::Vector2& point,
        float distance,
        ViewportHelpers::ElementEdges& outEdges);
    static bool PickAnchors(const AZ::Entity* element,
        const AZ::Vector2& point,
        const AZ::Vector2& iconSize,
        ViewportHelpers::SelectedAnchors& outAnchors);
    static bool PickAxisGizmo(const AZ::Entity* element,
        ViewportInteraction::CoordinateSystem coordinateSystem,
        ViewportInteraction::InteractionMode interactionMode,
        const AZ::Vector2& point,
        const AZ::Vector2& iconSize,
        ViewportHelpers::GizmoParts& outGizmoParts);
    static bool PickCircleGizmo(const AZ::Entity* element,
        const AZ::Vector2& point,
        const AZ::Vector2& iconSize,
        ViewportHelpers::GizmoParts& outGizmoParts);
    static bool PickPivot(const AZ::Entity* element,
        const AZ::Vector2& point,
        const AZ::Vector2& iconSize);

    static void ResizeDirectly(HierarchyWidget* hierarchy,
        const AZ::EntityId& canvasId,
        const ViewportHelpers::ElementEdges& grabbedEdges,
        AZ::Entity* element,
        const AZ::Vector3& mouseTranslation);
    static void ResizeByGizmo(HierarchyWidget* hierarchy,
        const AZ::EntityId& canvasId,
        const ViewportHelpers::GizmoParts& grabbedGizmoParts,
        const AZ::EntityId& activeElementId,
        AZ::Entity* element,
        const AZ::Vector3& mouseTranslation);
    static void Rotate(HierarchyWidget* hierarchy,
        const AZ::EntityId& canvasId,
        const AZ::Vector2& lastMouseDragPos,
        const AZ::EntityId& activeElementId,
        AZ::Entity* element,
        const AZ::Vector2& mousePosition);
    static void MoveAnchors(const ViewportHelpers::SelectedAnchors& grabbedAnchors,
        const UiTransform2dInterface::Anchors& startAnchors,
        const AZ::Vector2& startMouseDragPos,
        AZ::Entity* element,
        const AZ::Vector2& mousePosition,
        bool adjustOffsets);
    static void MovePivot(const AZ::Vector2& lastMouseDragPos,
        AZ::Entity* element,
        const AZ::Vector2& mousePosition);
};
