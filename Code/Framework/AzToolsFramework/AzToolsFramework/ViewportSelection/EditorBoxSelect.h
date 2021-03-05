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

#include <AzCore/std/functional.h>
#include <AzCore/std/optional.h>
#include <AzToolsFramework/Viewport/ViewportTypes.h>

#include <QRect>

namespace AzFramework
{
    class DebugDisplayRequests;
    struct ViewportInfo;
}

namespace AzToolsFramework
{
    /// Utility to provide box select (click and drag) support for viewport types.
    /// Users can override the mouse event callbacks and display scene function to customize behavior.
    class EditorBoxSelect
    {
    public:
        EditorBoxSelect() = default;

        /// Return if a box select action is currently taking place.
        bool Active() const { return m_boxSelectRegion.has_value(); }

        /// Update the box select for various mouse events.
        /// Call HandleMouseInteraction from type/system implementing MouseViewportRequests interface.
        void HandleMouseInteraction(
            const ViewportInteraction::MouseInteractionEvent& mouseInteraction);

        /// Responsible for drawing the 2d box representing the selection in screen space.
        void Display2d(AzFramework::DebugDisplayRequests& debugDisplay);

        /// Custom drawing behavior to happen during a box select.
        void DisplayScene(
            const AzFramework::ViewportInfo& viewportInfo, AzFramework::DebugDisplayRequests& debugDisplay);

        /// Set the left mouse down callback.
        void InstallLeftMouseDown(
            const AZStd::function<void(const ViewportInteraction::MouseInteractionEvent& mouseInteraction)>& leftMouseDown);
        /// Set the mouse move callback.
        void InstallMouseMove(
            const AZStd::function<void(const ViewportInteraction::MouseInteractionEvent& mouseInteraction)>& mouseMove);
        /// Set the left mouse up callback.
        void InstallLeftMouseUp(
            const AZStd::function<void()>& leftMouseUp);
        /// Set the display scene callback.
        void InstallDisplayScene(
            const AZStd::function<void(
                const AzFramework::ViewportInfo& viewportInfo,
                AzFramework::DebugDisplayRequests& debugDisplay)>& displayScene);

        /// Return the box select region.
        /// If a box selection is being made, return the current rectangle representing the area.
        /// If there is currently no active box select, then the Maybe type will be empty (there will be no region/area).
        const AZStd::optional<QRect>& BoxRegion() const { return m_boxSelectRegion; }

        /// Return the active modifiers from the previous frame.
        ViewportInteraction::KeyboardModifiers PreviousModifiers() const { return m_previousModifiers; }

    private:
        // Callbacks to set for BoxSelect customization
        AZStd::function<void(const ViewportInteraction::MouseInteractionEvent& mouseInteraction)> m_leftMouseDown;
        AZStd::function<void(const ViewportInteraction::MouseInteractionEvent& mouseInteraction)> m_mouseMove;
        AZStd::function<void()> m_leftMouseUp;
        AZStd::function<void(
            const AzFramework::ViewportInfo& viewportInfo, AzFramework::DebugDisplayRequests& debugDisplay)> m_displayScene;

        AZStd::optional<QRect> m_boxSelectRegion; ///< Maybe/optional value to store box select region while active.
        ViewportInteraction::KeyboardModifiers m_previousModifiers; ///< Modifier keys active on the previous frame.
    };
} // namespace AzToolsFramework