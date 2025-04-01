/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzFramework/Viewport/SingleViewportController.h>
#include <AzFramework/Input/Events/InputChannelEventListener.h>
#include <AzToolsFramework/Viewport/ViewportTypes.h>

namespace AzToolsFramework
{
    class ManipulatorManager;
}

namespace EMStudio
{
    //! Provide input control for manipulators in AnimViewport
    class AnimViewportInputController
        : public AzFramework::SingleViewportController
    {
    public:
        AZ_TYPE_INFO(AnimViewportInputController, "{A1629CB6-2292-4B7D-8B49-F614BD4746AA}");
        AZ_CLASS_ALLOCATOR(AnimViewportInputController, AZ::SystemAllocator)

        // AzFramework::SingleViewportController overrides...
        bool HandleInputChannelEvent(const AzFramework::ViewportControllerInputEvent& event) override;

    private:

        AzToolsFramework::ViewportInteraction::MouseInteraction m_mouseInteraction;
    };
} // namespace EMStudio
