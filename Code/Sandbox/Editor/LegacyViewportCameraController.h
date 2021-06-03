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
#include <AzCore/Math/Vector3.h>

#include <Atom/RPI.Public/Base.h>

#include <QtCore/qnamespace.h>
#include <QPoint>

namespace AzFramework
{
    struct ScreenPoint;
}

namespace SandboxEditor
{
    class LegacyViewportCameraControllerInstance;
    using LegacyViewportCameraController = AzFramework::MultiViewportController<LegacyViewportCameraControllerInstance>;

    class LegacyViewportCameraControllerInstance final
        : public AzFramework::MultiViewportControllerInstanceInterface<LegacyViewportCameraController>
    {
    public:
        LegacyViewportCameraControllerInstance(AzFramework::ViewportId viewport, LegacyViewportCameraController* controller);

        bool HandleInputChannelEvent(const AzFramework::ViewportControllerInputEvent& event) override;
        void ResetInputChannels() override;
        void UpdateViewport(const AzFramework::ViewportControllerUpdateEvent& event) override;

    private:
        bool JustAltHeld() const;
        bool NoModifierHeld() const;
        bool AllowDolly() const;
        bool AllowOrbit() const;
        bool AllowPan() const;
        bool InvertPan() const;

        static Qt::KeyboardModifier GetKeyboardModifier(const AzFramework::InputChannel& inputChannel);
        static Qt::Key GetKeyboardKey(const AzFramework::InputChannel& inputChannel);

        AZ::RPI::ViewportContextPtr GetViewportContext();

        bool HandleMouseMove(const AzFramework::ScreenPoint& currentMousePos, const AzFramework::ScreenPoint& previousMousePos);
        bool HandleMouseWheel(float zDelta);
        bool IsKeyDown(Qt::Key key) const;
        void UpdateCursorCapture(bool shouldCaptureCursor);

        bool m_inRotateMode = false;
        bool m_inMoveMode = false;
        bool m_inOrbitMode = false;
        bool m_inZoomMode = false;
        int m_totalMouseMoveDelta = 0;
        float m_orbitDistance = 10.f;
        float m_moveSpeed = 1.f;
        AZ::Vector3 m_orbitTarget = {};
        unsigned int m_modifiers = {};
        AZStd::unordered_set<Qt::Key> m_pressedKeys;
        bool m_capturingCursor = false;
    };

} //namespace SandboxEditor
