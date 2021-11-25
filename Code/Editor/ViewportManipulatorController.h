/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzFramework/Input/Events/InputChannelEventListener.h>
#include <AzFramework/Viewport/MultiViewportController.h>
#include <AzFramework/Viewport/ViewportId.h>
#include <AzToolsFramework/Viewport/ViewportTypes.h>

#include <SandboxAPI.h>

namespace SandboxEditor
{
    class ViewportManipulatorControllerInstance;
    using ViewportManipulatorController = AzFramework::
        MultiViewportController<ViewportManipulatorControllerInstance, AzFramework::ViewportControllerPriority::DispatchToAllPriorities>;

    class ViewportManipulatorControllerInstance final
        : public AzFramework::MultiViewportControllerInstanceInterface<ViewportManipulatorController>
    {
    public:
        SANDBOX_API ViewportManipulatorControllerInstance(AzFramework::ViewportId viewport, ViewportManipulatorController* controller);
        SANDBOX_API ~ViewportManipulatorControllerInstance();

        SANDBOX_API bool HandleInputChannelEvent(const AzFramework::ViewportControllerInputEvent& event) override;
        SANDBOX_API void ResetInputChannels() override;
        SANDBOX_API void UpdateViewport(const AzFramework::ViewportControllerUpdateEvent& event) override;

    private:
        bool IsDoubleClick(AzToolsFramework::ViewportInteraction::MouseButton) const;

        static AzToolsFramework::ViewportInteraction::MouseButton GetMouseButton(const AzFramework::InputChannel& inputChannel);
        static bool IsMouseMove(const AzFramework::InputChannel& inputChannel);
        static AzToolsFramework::ViewportInteraction::KeyboardModifier GetKeyboardModifier(const AzFramework::InputChannel& inputChannel);

        //! Represents the time and location of a click.
        struct ClickEvent
        {
            AZ::ScriptTimePoint m_time;
            AzFramework::ScreenPoint m_position;
        };

        AzToolsFramework::ViewportInteraction::MouseInteraction m_mouseInteraction;
        AZStd::unordered_map<AzToolsFramework::ViewportInteraction::MouseButton, ClickEvent> m_pendingDoubleClicks;

        AZ::ScriptTimePoint m_currentTime;
    };
} // namespace SandboxEditor
