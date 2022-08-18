/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Editor/Picking.h>

namespace EMotionFX
{
    static constexpr float PickingMargin = 0.01f;

    class PhysicsPicking : public Picking
    {
    public:
        bool HandleMouseInteraction(const AzToolsFramework::ViewportInteraction::MouseInteractionEvent& mouseInteractionEvent) override;
        void UpdateRenderFlags(ActorRenderFlags renderFlags) override;

    private:
        ActorRenderFlags m_renderFlags;
    };
} // namespace EMotionFX
