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
#include "UiCanvasEditor_precompiled.h"

#include "EditorCommon.h"
#include "ViewportPivot.h"

ViewportPivot::ViewportPivot()
    : m_pivot(new ViewportIcon("Plugins/UiCanvasEditor/CanvasIcons/Pivot.tif"))
{
}

ViewportPivot::~ViewportPivot()
{
}

AZ::Vector2 ViewportPivot::GetSize() const
{
    return (m_pivot ? m_pivot->GetTextureSize() : AZ::Vector2(0.0f, 0.0f));
}

void ViewportPivot::Draw(Draw2dHelper& draw2d,
    const AZ::Entity* element,
    bool isHighlighted) const
{
    if (!element)
    {
        // Don't draw anything if there is no element or it's controlled by a layout.
        return;
    }

    // Determine whether to highlight the icon
    AZ::Color color = (isHighlighted) ? ViewportHelpers::highlightColor : ViewportHelpers::pivotColor;

    // Make the pivot opaque
    color.SetA(1.0f);

    // Draw the pivot icon
    AZ::Vector2 pivot;
    EBUS_EVENT_ID_RESULT(pivot, element->GetId(), UiTransformBus, GetViewportSpacePivot);
    m_pivot->Draw(draw2d, pivot, AZ::Matrix4x4::CreateIdentity(), 0.0f, color);
}
