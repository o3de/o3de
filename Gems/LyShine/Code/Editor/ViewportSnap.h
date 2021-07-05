/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include "ViewportInteraction.h"

class ViewportSnap
{
public:
    static void Rotate(HierarchyWidget* hierarchy,
        const AZ::EntityId& canvasId,
        AZ::Entity* element,
        float signedAngle);

    static void ResizeByGizmo(HierarchyWidget* hierarchy,
        const AZ::EntityId& canvasId,
        const ViewportHelpers::GizmoParts& grabbedGizmoParts,
        AZ::Entity* element,
        const AZ::Vector2& pivot,
        const AZ::Vector2& translation);

    static void ResizeDirectlyWithScaleOrRotation(HierarchyWidget* hierarchy,
        const AZ::EntityId& canvasId,
        const ViewportHelpers::ElementEdges& grabbedEdges,
        AZ::Entity* element,
        const UiTransformInterface::RectPoints& translation);

    static void ResizeDirectlyNoScaleNoRotation(HierarchyWidget* hierarchy,
        const AZ::EntityId& canvasId,
        const ViewportHelpers::ElementEdges& grabbedEdges,
        AZ::Entity* element,
        const AZ::Vector2& translation);

private:
};
