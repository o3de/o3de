/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzFramework/Viewport/ViewportId.h>
#include <AzFramework/Viewport/MultiViewportController.h>
#include <AzFramework/Input/Events/InputChannelEventListener.h>
#include <AzToolsFramework/Viewport/ViewportTypes.h>

namespace SandboxEditor
{
    class ViewportManipulatorControllerInstance;
    using ViewportManipulatorController = AzFramework::MultiViewportController<ViewportManipulatorControllerInstance, AzFramework::ViewportControllerPriority::DispatchToAllPriorities>;

    class ViewportManipulatorControllerInstance final
        : public AzFramework::MultiViewportControllerInstanceInterface<ViewportManipulatorController>
    {
    public:
        explicit ViewportManipulatorControllerInstance(AzFramework::ViewportId viewport, ViewportManipulatorController* controller);

        bool HandleInputChannelEvent(const AzFramework::ViewportControllerInputEvent& event) override;
        void ResetInputChannels() override;
        void UpdateViewport(const AzFramework::ViewportControllerUpdateEvent& event) override;

    private:
        bool IsDoubleClick(AzToolsFramework::ViewportInteraction::MouseButton) const;

        static AzToolsFramework::ViewportInteraction::MouseButton GetMouseButton(const AzFramework::InputChannel& inputChannel);
        static bool IsMouseMove(const AzFramework::InputChannel& inputChannel);
        static AzToolsFramework::ViewportInteraction::KeyboardModifier GetKeyboardModifier(const AzFramework::InputChannel& inputChannel);

        AzToolsFramework::ViewportInteraction::MouseInteraction m_mouseInteraction;
        AZStd::unordered_map<AzToolsFramework::ViewportInteraction::MouseButton, AZ::ScriptTimePoint> m_pendingDoubleClicks;
        AZ::ScriptTimePoint m_curTime;
    };
} //namespace SandboxEditor
