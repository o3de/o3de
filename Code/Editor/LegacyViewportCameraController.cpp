/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "LegacyViewportCameraController.h"

#include <AzFramework/Input/Devices/Mouse/InputDeviceMouse.h>
#include <AzFramework/Input/Devices/Keyboard/InputDeviceKeyboard.h>
#include <AzFramework/Viewport/ScreenGeometry.h>
#include <AzToolsFramework/Viewport/ViewportMessages.h>
#include <Atom/RPI.Public/ViewportContextBus.h>
#include <Atom/RPI.Public/ViewportContext.h>

#include <IViewSystem.h>
#include <ISystem.h>
#include "CryCommon/MathConversion.h"
#include "SandboxAPI.h"
#include "Settings.h"

namespace SandboxEditor
{

LegacyViewportCameraControllerInstance::LegacyViewportCameraControllerInstance(AzFramework::ViewportId viewportId, LegacyViewportCameraController* controller)
    : AzFramework::MultiViewportControllerInstanceInterface<LegacyViewportCameraController>(viewportId, controller)
{
    OrbitCameraControlsBus::Handler::BusConnect(viewportId);
}

LegacyViewportCameraControllerInstance::~LegacyViewportCameraControllerInstance()
{
    OrbitCameraControlsBus::Handler::BusDisconnect();
}

bool LegacyViewportCameraControllerInstance::JustAltHeld() const
{
    return (m_modifiers ^ Qt::AltModifier) == 0;
}

bool LegacyViewportCameraControllerInstance::NoModifierHeld() const
{
    return !m_modifiers;
}

bool LegacyViewportCameraControllerInstance::AllowDolly() const
{
    return JustAltHeld();
}

bool LegacyViewportCameraControllerInstance::AllowOrbit() const
{
    return JustAltHeld();
}

bool LegacyViewportCameraControllerInstance::AllowPan() const
{
    // begin pan with alt (inverted movement) or no modifiers
    return JustAltHeld() || NoModifierHeld();
}

bool LegacyViewportCameraControllerInstance::InvertPan() const
{
    return JustAltHeld();
}

void LegacyViewportCameraControllerInstance::SetOrbitDistance(float orbitDistance)
{
    m_orbitDistance = orbitDistance;
}


AZ::RPI::ViewportContextPtr LegacyViewportCameraControllerInstance::GetViewportContext()
{
    // This could be cached, if needed
    auto viewportContextManager = AZ::Interface<AZ::RPI::ViewportContextRequestsInterface>::Get();
    if (!viewportContextManager)
    {
        return {};
    }
    return viewportContextManager->GetViewportContextById(GetViewportId());
}

bool LegacyViewportCameraControllerInstance::HandleMouseMove(
    const AzFramework::ScreenPoint& currentMousePos, const AzFramework::ScreenPoint& previousMousePos)
{
    if (previousMousePos == currentMousePos)
    {
        return false;
    }

    auto viewportContext = GetViewportContext();
    if (!viewportContext)
    {
        return false;
    }

    float speedScale = gSettings.cameraMoveSpeed;

    if (m_modifiers & Qt::Key_Control)
    {
        speedScale *= gSettings.cameraFastMoveSpeed;
    }

    if (m_inMoveMode || m_inOrbitMode || m_inRotateMode || m_inZoomMode)
    {
        m_totalMouseMoveDelta += (QPoint(currentMousePos.m_x, currentMousePos.m_y)-QPoint(previousMousePos.m_x, previousMousePos.m_y)).manhattanLength();
    }

    if ((m_inRotateMode && m_inMoveMode) || m_inZoomMode)
    {
        Matrix34 m = AZTransformToLYTransform(viewportContext->GetCameraTransform());

        Vec3 ydir = m.GetColumn1().GetNormalized();
        Vec3 pos = m.GetTranslation();

        const float posDelta = 0.2f * (previousMousePos.m_y - currentMousePos.m_y) * speedScale;
        pos = pos - ydir * posDelta;
        m_orbitDistance = m_orbitDistance + posDelta;
        m_orbitDistance = fabs(m_orbitDistance);

        m.SetTranslation(pos);
        viewportContext->SetCameraTransform(LYTransformToAZTransform(m));
        return true;
    }
    else if (m_inRotateMode)
    {
        Ang3 angles(-currentMousePos.m_y + previousMousePos.m_y, 0, -currentMousePos.m_x + previousMousePos.m_x);
        angles = angles * 0.002f * gSettings.cameraRotateSpeed;
        if (gSettings.invertYRotation)
        {
            angles.x = -angles.x;
        }
        Matrix34 camtm = AZTransformToLYTransform(viewportContext->GetCameraTransform());
        Ang3 ypr = CCamera::CreateAnglesYPR(Matrix33(camtm));
        ypr.x += angles.z;
        ypr.y += angles.x;

        ypr.y = AZStd::clamp(ypr.y, -1.5f, 1.5f); // to keep rotation in reasonable range
        ypr.z = 0;      // to have camera always upward

        camtm = Matrix34(CCamera::CreateOrientationYPR(ypr), camtm.GetTranslation());
        viewportContext->SetCameraTransform(LYTransformToAZTransform(camtm));
        return true;
    }
    else if (m_inMoveMode)
    {
        // Slide.
        Matrix34 m = AZTransformToLYTransform(viewportContext->GetCameraTransform());
        Vec3 xdir = m.GetColumn0().GetNormalized();
        Vec3 zdir = m.GetColumn2().GetNormalized();

        if (InvertPan())
        {
            xdir = -xdir;
            zdir = -zdir;
        }

        Vec3 pos = m.GetTranslation();
        pos += 0.1f * xdir * (currentMousePos.m_x - previousMousePos.m_x) * speedScale + 0.1f * zdir * (previousMousePos.m_y - currentMousePos.m_y) * speedScale;
        m.SetTranslation(pos);

        AZ::Transform transform = viewportContext->GetCameraTransform();
        transform.SetTranslation(LYVec3ToAZVec3(pos));
        viewportContext->SetCameraTransform(transform);
        return true;
    }
    else if (m_inOrbitMode)
    {
        Ang3 angles(-currentMousePos.m_y + previousMousePos.m_y, 0, -currentMousePos.m_x + previousMousePos.m_x);
        angles = angles * 0.002f * gSettings.cameraRotateSpeed;

        if (gSettings.invertPan)
        {
            angles.z = -angles.z;
        }

        Matrix34 m = AZTransformToLYTransform(viewportContext->GetCameraTransform());
        Ang3 ypr = CCamera::CreateAnglesYPR(Matrix33(m));
        ypr.x += angles.z;
        ypr.y = AZStd::clamp(ypr.y, -1.5f, 1.5f); // to keep rotation in reasonable range
        ypr.y += angles.x;

        Matrix33 rotateTM = CCamera::CreateOrientationYPR(ypr);

        Vec3 src = m.GetTranslation();
        Vec3 trg(m_orbitTarget.GetX(), m_orbitTarget.GetY(), m_orbitTarget.GetZ());
        float fCameraRadius = (trg - src).GetLength();

        // Calc new source.
        src = trg - rotateTM * Vec3(0, 1, 0) * fCameraRadius;
        Matrix34 camTM = rotateTM;
        camTM.SetTranslation(src);

        viewportContext->SetCameraTransform(LYTransformToAZTransform(camTM));
        return true;
    }
    return false;
}

bool LegacyViewportCameraControllerInstance::HandleMouseWheel(float zDelta)
{
    auto viewportContext = GetViewportContext();
    if (!viewportContext)
    {
        return false;
    }

    Matrix34 m = AZTransformToLYTransform(viewportContext->GetCameraTransform());
    const Vec3 ydir = m.GetColumn1().GetNormalized();

    Vec3 pos = m.GetTranslation();

    const float posDelta = 0.01f * zDelta * gSettings.wheelZoomSpeed;
    pos += ydir * posDelta;
    m_orbitDistance = m_orbitDistance - posDelta;
    m_orbitDistance = fabs(m_orbitDistance);

    m.SetTranslation(pos);
    viewportContext->SetCameraTransform(LYTransformToAZTransform(m));
    return true;
}

bool LegacyViewportCameraControllerInstance::IsKeyDown(Qt::Key key) const
{
    return m_pressedKeys.contains(key);
}

Qt::Key LegacyViewportCameraControllerInstance::GetKeyboardKey(const AzFramework::InputChannel& inputChannel)
{
    using Key = AzFramework::InputDeviceKeyboard::Key;
    const auto& id = inputChannel.GetInputChannelId();
    if (id == Key::AlphanumericW)
    {
        return Qt::Key_W;
    }
    else if (id == Key::AlphanumericA)
    {
        return Qt::Key_A;
    }
    else if (id == Key::AlphanumericS)
    {
        return Qt::Key_S;
    }
    else if (id == Key::AlphanumericD)
    {
        return Qt::Key_D;
    }
    else if (id == Key::AlphanumericQ)
    {
        return Qt::Key_Q;
    }
    else if (id == Key::AlphanumericE)
    {
        return Qt::Key_E;
    }
    else if (id == Key::NavigationArrowUp)
    {
        return Qt::Key_Up;
    }
    else if (id == Key::NavigationArrowUp)
    {
        return Qt::Key_Down;
    }
    else if (id == Key::NavigationArrowUp)
    {
        return Qt::Key_Left;
    }
    else if (id == Key::NavigationArrowUp)
    {
        return Qt::Key_Right;
    }
    return Qt::Key_unknown;
}

Qt::KeyboardModifier LegacyViewportCameraControllerInstance::GetKeyboardModifier(const AzFramework::InputChannel& inputChannel)
{
    using Key = AzFramework::InputDeviceKeyboard::Key;
    const auto& id = inputChannel.GetInputChannelId();
    if (id == Key::ModifierAltL || id == Key::ModifierAltR)
    {
        return Qt::KeyboardModifier::AltModifier;
    }
    if (id == Key::ModifierCtrlL || id == Key::ModifierCtrlR)
    {
        return Qt::KeyboardModifier::ControlModifier;
    }
    if (id == Key::ModifierShiftL || id == Key::ModifierShiftR)
    {
        return Qt::KeyboardModifier::ShiftModifier;
    }
    return Qt::KeyboardModifier::NoModifier;
}

bool LegacyViewportCameraControllerInstance::HandleInputChannelEvent(const AzFramework::ViewportControllerInputEvent& event)
{
    using AzFramework::InputChannel;
    using MouseButton = AzFramework::InputDeviceMouse::Button;
    const auto& id = event.m_inputChannel.GetInputChannelId();
    const auto& state = event.m_inputChannel.GetState();
    bool shouldCaptureCursor = m_capturingCursor;
    bool shouldConsumeEvent = false;

    if (id == AzFramework::InputDeviceMouse::SystemCursorPosition)
    {
        bool result = false;
        AzToolsFramework::ViewportInteraction::ViewportMouseCursorRequestBus::Event(
            GetViewportId(),
            [this, &result](AzToolsFramework::ViewportInteraction::ViewportMouseCursorRequests* mouseRequests)
            {
                if (auto previousMousePosition = mouseRequests->PreviousViewportCursorScreenPosition();
                    previousMousePosition.has_value())
                {
                    result = HandleMouseMove(mouseRequests->ViewportCursorScreenPosition(), previousMousePosition.value());
                }
            });
        return result;
    }
    else if (id == MouseButton::Left)
    {
        if (state == InputChannel::State::Began)
        {
            if (AllowOrbit())
            {
                AzFramework::CameraState cameraState;
                AzToolsFramework::ViewportInteraction::ViewportInteractionRequestBus::EventResult(
                    cameraState, event.m_viewportId,
                    &AzToolsFramework::ViewportInteraction::ViewportInteractionRequestBus::Events::GetCameraState);

                m_inOrbitMode = true;
                m_orbitTarget = cameraState.m_position + cameraState.m_forward * m_orbitDistance;

                shouldConsumeEvent = true;
                shouldCaptureCursor = true;
            }
        }
        else if (state == InputChannel::State::Ended)
        {
            m_inOrbitMode = false;
            shouldCaptureCursor = false;
        }
    }
    else if (id == MouseButton::Right)
    {
        if (state == InputChannel::State::Began)
        {
            if (AllowDolly())
            {
                m_inZoomMode = true;
            }
            else
            {
                m_inRotateMode = true;
            }

            shouldCaptureCursor = true;
            // Record how much the cursor has been moved to see if we should own the mouse up event.
            m_totalMouseMoveDelta = 0;
        }
        else if (state == InputChannel::State::Ended)
        {
            m_inZoomMode = false;
            m_inRotateMode = false;
            // If we've moved the cursor more than a couple pixels, we should eat this mouse up event to prevent the context menu controller from seeing it.
            shouldConsumeEvent = m_totalMouseMoveDelta > 2;
            shouldCaptureCursor = false;
        }
    }
    else if (id == MouseButton::Middle)
    {
        if (state == InputChannel::State::Began)
        {
            if (AllowPan())
            {
                m_inMoveMode = true;
                shouldConsumeEvent = true;
                shouldCaptureCursor = true;
            }
        }
        else if (state == InputChannel::State::Ended)
        {
            m_inMoveMode = false;
            shouldCaptureCursor = false;
        }
    }
    else if (auto modifier = GetKeyboardModifier(event.m_inputChannel); modifier != Qt::KeyboardModifier::NoModifier)
    {
        if (state == InputChannel::State::Ended)
        {
            m_modifiers &= ~modifier;
        }
        else
        {
            m_modifiers |= modifier;
        }
    }
    else if (id == AzFramework::InputDeviceMouse::Movement::Z)
    {
        if (state == InputChannel::State::Began || state == InputChannel::State::Updated)
        {
            shouldConsumeEvent = HandleMouseWheel(event.m_inputChannel.GetValue());
        }
    }
    else if (auto key = GetKeyboardKey(event.m_inputChannel); key != Qt::Key_unknown)
    {
        if (state == InputChannel::State::Ended)
        {
            m_pressedKeys.erase(key);
        }
        else
        {
            m_pressedKeys.insert(key);
            shouldConsumeEvent = true;
        }
    }

    UpdateCursorCapture(shouldCaptureCursor);

    return shouldConsumeEvent;
}

void LegacyViewportCameraControllerInstance::UpdateCursorCapture(bool shouldCaptureCursor)
{
    if (m_capturingCursor != shouldCaptureCursor)
    {
        if (shouldCaptureCursor)
        {
            AzToolsFramework::ViewportInteraction::ViewportMouseCursorRequestBus::Event(
                GetViewportId(),
                &AzToolsFramework::ViewportInteraction::ViewportMouseCursorRequestBus::Events::BeginCursorCapture
            );
        }
        else
        {
            AzToolsFramework::ViewportInteraction::ViewportMouseCursorRequestBus::Event(
                GetViewportId(),
                &AzToolsFramework::ViewportInteraction::ViewportMouseCursorRequestBus::Events::EndCursorCapture
            );
        }

        m_capturingCursor = shouldCaptureCursor;
    }
}

void LegacyViewportCameraControllerInstance::ResetInputChannels()
{
    m_modifiers = 0;
    m_pressedKeys.clear();
    UpdateCursorCapture(false);
    m_inRotateMode = m_inMoveMode = m_inOrbitMode = m_inZoomMode = false;
}

void LegacyViewportCameraControllerInstance::UpdateViewport(const AzFramework::ViewportControllerUpdateEvent& event)
{
    auto viewportContext = GetViewportContext();
    if (!viewportContext)
    {
        return;
    }

    AZ::Transform transform = viewportContext->GetCameraTransform();
    AZ::Vector3 xdir = transform.GetBasisX();
    AZ::Vector3 ydir = transform.GetBasisY();
    AZ::Vector3 zdir = transform.GetBasisZ();

    AZ::Vector3 pos = transform.GetTranslation();

    float speedScale = AZStd::GetMin(30.0f * event.m_deltaTime.count(), 20.0f);

    // Use the global modifier keys instead of our keymap. It's more reliable.
    const bool shiftPressed = m_modifiers & Qt::ShiftModifier;
    const bool controlPressed = m_modifiers & Qt::ControlModifier;

    speedScale *= gSettings.cameraMoveSpeed;
    if (controlPressed)
    {
        return;
    }

    if (shiftPressed)
    {
        speedScale *= gSettings.cameraFastMoveSpeed;
    }

    bool cameraMoved = false;

    if (IsKeyDown(Qt::Key_Up) || IsKeyDown(Qt::Key_W))
    {
        // move forward
        cameraMoved = true;
        pos = pos + (speedScale * m_moveSpeed * ydir);
    }

    if (IsKeyDown(Qt::Key_Down) || IsKeyDown(Qt::Key_S))
    {
        // move backward
        cameraMoved = true;
        pos = pos - (speedScale * m_moveSpeed * ydir);
    }

    if (IsKeyDown(Qt::Key_Left) || IsKeyDown(Qt::Key_A))
    {
        // move left
        cameraMoved = true;
        pos = pos - (speedScale * m_moveSpeed * xdir);
    }

    if (IsKeyDown(Qt::Key_Right) || IsKeyDown(Qt::Key_D))
    {
        // move right
        cameraMoved = true;
        pos = pos + (speedScale * m_moveSpeed * xdir);
    }

    if (IsKeyDown(Qt::Key_E))
    {
        // move Up
        cameraMoved = true;
        pos = pos + (speedScale * m_moveSpeed * zdir);
    }

    if (IsKeyDown(Qt::Key_Q))
    {
        // move down
        cameraMoved = true;
        pos = pos - (speedScale * m_moveSpeed * zdir);
    }

    if (cameraMoved)
    {
        transform.SetTranslation(pos);
        viewportContext->SetCameraTransform(transform);
    }
}

} //namespace SandboxEditor
