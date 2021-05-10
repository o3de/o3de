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

        AzToolsFramework::ViewportInteraction::MouseInteraction m_state;
        AZStd::unordered_map<AzToolsFramework::ViewportInteraction::MouseButton, AZ::ScriptTimePoint> m_pendingDoubleClicks;
        AZ::ScriptTimePoint m_curTime;
    };
} //namespace SandboxEditor
