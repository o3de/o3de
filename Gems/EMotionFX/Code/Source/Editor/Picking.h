/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once
#include <AzCore/RTTI/TypeInfo.h>
#include <Integration/Rendering/RenderFlag.h>

namespace AzToolsFramework::ViewportInteraction
{
    struct MouseInteractionEvent;
} // namespace AzToolsFramework::ViewportInteraction

namespace EMotionFX
{
    inline constexpr float PickingMargin = 0.01f;

    //! Supports clicking in the animation editor viewport to select a joint.
    class Picking
    {
    public:
        bool HandleMouseInteraction(const AzToolsFramework::ViewportInteraction::MouseInteractionEvent& mouseInteractionEvent);
        void SetRenderFlags(ActorRenderFlags renderFlags);

    private:
        ActorRenderFlags m_renderFlags;
    };
} // namespace EMotionFX
