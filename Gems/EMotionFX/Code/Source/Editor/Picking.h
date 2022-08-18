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
    //! Base class for modes which support clicking in the viewport to select an item.
    class Picking
    {
    public:
        virtual ~Picking() = default;

        virtual bool HandleMouseInteraction(const AzToolsFramework::ViewportInteraction::MouseInteractionEvent& mouseInteractionEvent) = 0;
        virtual void UpdateRenderFlags(ActorRenderFlags renderFlags) = 0;
    };
} // namespace EMotionFX
