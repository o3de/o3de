/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "EditorCommon.h"
#include "ViewportPivot.h"

ViewportPivot::ViewportPivot()
    : m_pivot(new ViewportIcon("Editor/Icons/Viewport/Pivot.tif"))
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
    UiTransformBus::EventResult(pivot, element->GetId(), &UiTransformBus::Events::GetViewportSpacePivot);
    m_pivot->Draw(draw2d, pivot, AZ::Matrix4x4::CreateIdentity(), 0.0f, color);
}
