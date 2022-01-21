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
    // Provide input control for manipulators in AnimViewport
    class AnimViewportInputController
        : public AzFramework::SingleViewportController
    {
    public:
        AZ_TYPE_INFO(AnimViewportInputController, "{A1629CB6-2292-4B7D-8B49-F614BD4746AA}");
        AZ_CLASS_ALLOCATOR(AnimViewportInputController, AZ::SystemAllocator, 0)

        AnimViewportInputController();
        virtual ~AnimViewportInputController();

        // AzFramework::ViewportControllerInstance interface overrides...
        bool HandleInputChannelEvent(const AzFramework::ViewportControllerInputEvent& event) override;
        // void UpdateViewport(const AzFramework::ViewportControllerUpdateEvent& event) override;

        void Init(AZ::EntityId targetEntityId);

        // TODO: Do this with a bus.
        AzToolsFramework::ManipulatorManager* m_manipulatorManager = nullptr;

    private:
        bool HandleMouseMove(
            const AzToolsFramework::ViewportInteraction::MouseInteractionEvent& mouseInteraction);

        static AzToolsFramework::ViewportInteraction::MouseButton GetMouseButton(const AzFramework::InputChannel& inputChannel);
        static bool IsMouseMove(const AzFramework::InputChannel& inputChannel);
        static AzToolsFramework::ViewportInteraction::KeyboardModifier GetKeyboardModifier(const AzFramework::InputChannel& inputChannel);

        AZ::EntityId m_targetEntityId;
        AzToolsFramework::ViewportInteraction::MouseInteraction m_mouseInteraction;
    };
}
