/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Color.h>
#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <AzFramework/Viewport/ViewportColors.h>

#include <LmbrCentral/Shape/ShapeComponentBus.h>

namespace LmbrCentral
{
    const ShapeDrawParams g_defaultShapeDrawParams = {
        AzFramework::ViewportColors::DeselectedColor,
        AzFramework::ViewportColors::WireColor,
        true
    };

    /// @brief Helper function to be used when drawing debug shapes - called from DisplayEntity on 
    /// the EntityDebugDisplayEventBus.
    /// @param handled Did we display anything.
    /// @param canDraw Functor to decide should the shape be drawn or not.
    /// @param drawShape Functor to draw a specific shape (box/capsule/sphere etc).
    /// @param worldFromLocal Transform of object in world space, push to matrix stack and render shape in local space.
    template<typename CanDraw, typename DrawShape>
    void DisplayShape(
        AzFramework::DebugDisplayRequests& debugDisplay,
        CanDraw&& canDraw, DrawShape&& drawShape, const AZ::Transform& worldFromLocal)
    {
        if (!canDraw())
        {
            return;
        }

        // only uniform scale is supported in physics so the debug visuals reflect this fact
        AZ::Transform worldFromLocalWithUniformScale = worldFromLocal;
        worldFromLocalWithUniformScale.SetUniformScale(worldFromLocalWithUniformScale.GetUniformScale());

        debugDisplay.PushMatrix(worldFromLocalWithUniformScale);

        drawShape(debugDisplay);

        debugDisplay.PopMatrix();
    }
} // namespace LmbrCentral
