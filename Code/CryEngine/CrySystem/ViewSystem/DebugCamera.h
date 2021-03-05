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