/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzFramework/Input/Events/InputChannelEventListener.h>

namespace LegacyViewSystem
{
///////////////////////////////////////////////////////////////////////////////
class DebugCamera
    : public AzFramework::InputChannelEventListener
{
public:
    enum Mode
    {
        ModeOff,    // no debug cam
        ModeFree,   // free-fly
        ModeFixed,  // fixed cam, control goes back to game
    };

    DebugCamera();
    ~DebugCamera() override;

    void Update();
    void PostUpdate();
    bool IsEnabled();
    bool IsFixed();
    bool IsFree();

    // AzFramework::InputChannelEventListener
    bool OnInputChannelEventFiltered(const AzFramework::InputChannel& inputChannel) override;

    void OnEnable();
    void OnDisable();
    void OnInvertY();
    void OnNextMode();
    void UpdatePitch(float amount);
    void UpdateYaw(float amount);
    void UpdatePosition(const Vec3& amount);
    void MovePosition(const Vec3& offset);

protected:
    int m_mouseMoveMode;
    int m_isYInverted;
    int m_cameraMode;
    float m_cameraYawInput;
    float m_cameraPitchInput;
    float m_cameraYaw;
    float m_cameraPitch;
    Vec3 m_moveInput;

    float m_moveScale;
    float m_oldMoveScale;
    Vec3 m_position;
    Matrix33 m_view;
};


inline bool DebugCamera::IsEnabled()
{
    return m_cameraMode != DebugCamera::ModeOff;
}

inline bool DebugCamera::IsFixed()
{
    return m_cameraMode == DebugCamera::ModeFixed;
}

inline bool DebugCamera::IsFree()
{
    return m_cameraMode == DebugCamera::ModeFree;
}

} // namespace LegacyViewSystem
