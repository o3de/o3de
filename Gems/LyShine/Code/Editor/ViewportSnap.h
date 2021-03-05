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
