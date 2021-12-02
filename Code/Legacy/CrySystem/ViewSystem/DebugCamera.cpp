/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "CrySystem_precompiled.h"
#include "DebugCamera.h"
#include "ISystem.h"
#include "Cry_Camera.h"
#include "IViewSystem.h"

#include <AzFramework/Input/Devices/Keyboard/InputDeviceKeyboard.h>
#include <AzFramework/Input/Devices/Gamepad/InputDeviceGamepad.h>
#include <AzFramework/Input/Devices/Mouse/InputDeviceMouse.h>

using namespace AzFramework;

namespace LegacyViewSystem
{
const float g_moveScaleIncrement = 0.1f;
const float g_moveScaleMin = 0.01f;
const float g_moveScaleMax = 10.0f;
const float g_mouseMoveScale = 0.1f;
const float g_gamepadRotationSpeed = 5.0f;
const float g_mouseMaxRotationSpeed = 270.0f;
const float g_moveSpeed = 10.0f;
const float g_maxPitch = 85.0f;
const float g_boostMultiplier = 10.0f;
const float g_minRotationSpeed = 15.0f;
const float g_maxRotationSpeed = 70.0f;

///////////////////////////////////////////////////////////////////////////////
DebugCamera::DebugCamera()
    : m_mouseMoveMode(0)
    , m_isYInverted(0)
    , m_cameraMode(DebugCamera::ModeOff)
    , m_cameraYawInput(0.0f)
    , m_cameraPitchInput(0.0f)
    , m_cameraYaw(0.0f)
    , m_cameraPitch(0.0f)
    , m_moveInput(ZERO)
    , m_moveScale(1.0f)
    , m_oldMoveScale(1.0f)
    , m_position(ZERO)
    , m_view(IDENTITY)
{
    InputChannelEventListener::Connect();
}

///////////////////////////////////////////////////////////////////////////////
DebugCamera::~DebugCamera()
{
    InputChannelEventListener::Disconnect();
}

///////////////////////////////////////////////////////////////////////////////
void DebugCamera::OnEnable()
{
    m_position = Vec3_Zero;
    m_moveInput = Vec3_Zero;

    Ang3 cameraAngles = Ang3(ZERO);
    m_cameraYaw = RAD2DEG(cameraAngles.z);
    m_cameraPitch = RAD2DEG(cameraAngles.x);
    m_view = Matrix33(Ang3(DEG2RAD(m_cameraPitch), 0.0f, DEG2RAD(m_cameraYaw)));

    m_cameraYawInput = 0.0f;
    m_cameraPitchInput = 0.0f;

    m_mouseMoveMode = 0;
    m_cameraMode = DebugCamera::ModeFree;
}

///////////////////////////////////////////////////////////////////////////////
void DebugCamera::OnDisable()
{
    m_mouseMoveMode = 0;
    m_cameraMode = DebugCamera::ModeOff;
}

///////////////////////////////////////////////////////////////////////////////
void DebugCamera::OnInvertY()
{
    m_isYInverted = !m_isYInverted;
}

///////////////////////////////////////////////////////////////////////////////
void DebugCamera::OnNextMode()
{
    if (m_cameraMode == DebugCamera::ModeFree)
    {
        m_cameraMode = DebugCamera::ModeFixed;
    }
    // ...
    else if (m_cameraMode == DebugCamera::ModeFixed)
    {
        // this is the last mode, go to disabled.
        OnDisable();
    }
}

///////////////////////////////////////////////////////////////////////////////
void DebugCamera::Update()
{
    if (m_cameraMode == DebugCamera::ModeOff)
    {
        return;
    }

    float rotationSpeed = clamp_tpl(m_moveScale, g_minRotationSpeed, g_maxRotationSpeed);
    UpdateYaw(m_cameraYawInput * rotationSpeed * gEnv->pTimer->GetFrameTime());
    UpdatePitch(m_cameraPitchInput * rotationSpeed * gEnv->pTimer->GetFrameTime());

    m_view = Matrix33(Ang3(DEG2RAD(m_cameraPitch), 0.0f, DEG2RAD(m_cameraYaw)));
    UpdatePosition(m_moveInput);
}

///////////////////////////////////////////////////////////////////////////////
void DebugCamera::PostUpdate()
{
}

///////////////////////////////////////////////////////////////////////////////
bool DebugCamera::OnInputChannelEventFiltered(const InputChannel& inputChannel)
{
    if (!IsEnabled() || m_cameraMode == DebugCamera::ModeFixed || gEnv->pConsole->IsOpened())
    {
        return false;
    }

    const InputDeviceId& deviceId = inputChannel.GetInputDevice().GetInputDeviceId();
    const InputChannelId& channelId = inputChannel.GetInputChannelId();
    const float eventValue = inputChannel.GetValue();
    if (InputDeviceKeyboard::IsKeyboardDevice(deviceId))
    {
        if (channelId == InputDeviceKeyboard::Key::AlphanumericW)
        {
            m_moveInput.y = eventValue;
        }
        else if (channelId == InputDeviceKeyboard::Key::AlphanumericS)
        {
            m_moveInput.y = -eventValue;
        }
        else if (channelId == InputDeviceKeyboard::Key::AlphanumericA)
        {
            m_moveInput.x = -eventValue;
        }
        else if (channelId == InputDeviceKeyboard::Key::AlphanumericD)
        {
            m_moveInput.x = eventValue;
        }
        else if (channelId == InputDeviceKeyboard::Key::ModifierShiftL)
        {
            if (inputChannel.IsStateEnded())
            {
                m_moveScale = m_oldMoveScale;
            }
            else if (inputChannel.IsStateBegan())
            {
                m_oldMoveScale = m_moveScale;
                m_moveScale = clamp_tpl(m_moveScale * g_boostMultiplier, g_moveScaleMin, g_moveScaleMax);
            }
        }
    }
    else if (InputDeviceMouse::IsMouseDevice(deviceId))
    {
        if (channelId == InputDeviceMouse::Movement::Z)
        {
            if (inputChannel.GetValue() > 0)
            {
                m_moveScale = clamp_tpl(m_moveScale + g_moveScaleIncrement, g_moveScaleMin, g_moveScaleMax);
            }
            else
            {
                m_moveScale = clamp_tpl(m_moveScale - g_moveScaleIncrement, g_moveScaleMin, g_moveScaleMax);
            }
        }
        else if (channelId == InputDeviceMouse::Movement::X)
        {
            //KC: If both left and right mouse buttons are pressed then use
            //the mouse movement for horizontal movement.
            if (2 != m_mouseMoveMode)
            {
                UpdateYaw(fsgnf(-eventValue) * clamp_tpl(fabs_tpl(eventValue) * m_moveScale, 0.0f, g_mouseMaxRotationSpeed) * gEnv->pTimer->GetFrameTime());
            }
            else
            {
                UpdatePosition(Vec3(eventValue * g_mouseMoveScale, 0.0f, 0.0f));
            }
        }
        else if (channelId == InputDeviceMouse::Movement::Y)
        {
            //KC: If both left and right mouse buttons are pressed then use
            //the mouse movement for vertical movement.
            if (2 != m_mouseMoveMode)
            {
                UpdatePitch(fsgnf(-eventValue) * clamp_tpl(fabs_tpl(eventValue) * m_moveScale, 0.0f, g_mouseMaxRotationSpeed) * gEnv->pTimer->GetFrameTime());
            }
            else
            {
                UpdatePosition(Vec3(0.0f, 0.0f, -eventValue * g_mouseMoveScale));
            }
        }
        else if (channelId == InputDeviceMouse::Button::Left)
        {
            if (inputChannel.IsStateEnded())
            {
                m_mouseMoveMode = clamp_tpl(m_mouseMoveMode - 1, 0, 2);
            }
            else
            {
                m_mouseMoveMode = clamp_tpl(m_mouseMoveMode + 1, 0, 2);
            }
        }
        else if (channelId == InputDeviceMouse::Button::Right)
        {
            if (inputChannel.IsStateEnded())
            {
                m_mouseMoveMode = clamp_tpl(m_mouseMoveMode - 1, 0, 2);
            }
            else
            {
                m_mouseMoveMode = clamp_tpl(m_mouseMoveMode + 1, 0, 2);
            }
        }
    }
    else if (InputDeviceGamepad::IsGamepadDevice(deviceId))
    {
        if (channelId == InputDeviceGamepad::Button::DU)
        {
            m_moveScale = clamp_tpl(m_moveScale + g_moveScaleIncrement, g_moveScaleMin, g_moveScaleMax);
        }
        else if (channelId == InputDeviceGamepad::Button::DD)
        {
            m_moveScale = clamp_tpl(m_moveScale - g_moveScaleIncrement, g_moveScaleMin, g_moveScaleMax);
        }
        else if (channelId == InputDeviceGamepad::Trigger::L2)
        {
            m_moveInput.z = -eventValue;
        }
        else if (channelId == InputDeviceGamepad::Trigger::R2)
        {
            m_moveInput.z = eventValue;
        }
        else if (channelId == InputDeviceGamepad::ThumbStickAxis1D::LX)
        {
            m_moveInput.x = eventValue;
        }
        else if (channelId == InputDeviceGamepad::ThumbStickAxis1D::LY)
        {
            m_moveInput.y = eventValue;
        }
        else if (channelId == InputDeviceGamepad::ThumbStickAxis1D::RX)
        {
            m_cameraYawInput = -eventValue * g_gamepadRotationSpeed;
        }
        else if (channelId == InputDeviceGamepad::ThumbStickAxis1D::RY)
        {
            m_cameraPitchInput = eventValue * g_gamepadRotationSpeed;
        }
        //KC: Use the shoulder buttons to temporarily boost or reduce the scale.
        else if (channelId == InputDeviceGamepad::Button::L1)
        {
            if (inputChannel.IsStateEnded())
            {
                m_moveScale = m_oldMoveScale;
            }
            else if (inputChannel.IsStateBegan())
            {
                m_oldMoveScale = m_moveScale;
                m_moveScale = clamp_tpl(m_moveScale / g_boostMultiplier, g_moveScaleMin, g_moveScaleMax);
            }
        }
        else if (channelId == InputDeviceGamepad::Button::R1)
        {
            if (inputChannel.IsStateEnded())
            {
                m_moveScale = m_oldMoveScale;
            }
            else if (inputChannel.IsStateBegan())
            {
                m_oldMoveScale = m_moveScale;
                m_moveScale = clamp_tpl(m_moveScale * g_boostMultiplier, g_moveScaleMin, g_moveScaleMax);
            }
        }
    }

    return false;
}

///////////////////////////////////////////////////////////////////////////////
void DebugCamera::UpdatePitch(float amount)
{
    if (m_isYInverted)
    {
        amount = -amount;
    }

    m_cameraPitch += amount;
    m_cameraPitch = clamp_tpl(m_cameraPitch, -g_maxPitch, g_maxPitch);
}

///////////////////////////////////////////////////////////////////////////////
void DebugCamera::UpdateYaw(float amount)
{
    m_cameraYaw += amount;
    if (m_cameraYaw < 0.0f)
    {
        m_cameraYaw += 360.0f;
    }
    else if (m_cameraYaw >= 360.0f)
    {
        m_cameraYaw -= 360.0f;
    }
}

///////////////////////////////////////////////////////////////////////////////
void DebugCamera::UpdatePosition(const Vec3& amount)
{
    Vec3 diff = amount * g_moveSpeed * m_moveScale * gEnv->pTimer->GetFrameTime();
    MovePosition(diff);
}

void DebugCamera::MovePosition(const Vec3& offset)
{
    m_position += m_view.GetColumn0() * offset.x;
    m_position += m_view.GetColumn1() * offset.y;
    m_position += m_view.GetColumn2() * offset.z;
}

} // namespace LegacyViewSystem
