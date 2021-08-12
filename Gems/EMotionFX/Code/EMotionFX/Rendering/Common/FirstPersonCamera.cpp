/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "FirstPersonCamera.h"
#include <MCore/Source/AzCoreConversions.h>


namespace MCommon
{
    // constructor
    FirstPersonCamera::FirstPersonCamera()
        : Camera()
    {
        Reset();
    }


    // destructor
    FirstPersonCamera::~FirstPersonCamera()
    {
    }


    // update the camera position, orientation and it's matrices
    void FirstPersonCamera::Update(float timeDelta)
    {
        MCORE_UNUSED(timeDelta);

        // lock pitching to [-90.0°, 90.0°]
        if (m_pitch < -90.0f + 0.1f)
        {
            m_pitch = -90.0f + 0.1f;
        }
        if (m_pitch >  90.0f - 0.1f)
        {
            m_pitch =  90.0f - 0.1f;
        }

        // calculate the camera direction vector based on the yaw and pitch
        AZ::Vector3 direction = (AZ::Matrix4x4::CreateRotationX(MCore::Math::DegreesToRadians(m_pitch)) * AZ::Matrix4x4::CreateRotationY(MCore::Math::DegreesToRadians(m_yaw))) * (AZ::Vector3(0.0f, 0.0f, 1.0f)).GetNormalized();

        // look from the camera position into the newly calculated direction
        MCore::LookAt(m_viewMatrix, m_position, m_position + direction * 10.0f, AZ::Vector3(0.0f, 1.0f, 0.0f));

        // update our base camera
        Camera::Update();
    }


    // process input and modify the camera attributes
    void FirstPersonCamera::ProcessMouseInput(int32 mouseMovementX, int32 mouseMovementY, bool leftButtonPressed, bool middleButtonPressed, bool rightButtonPressed, uint32 keyboardKeyFlags)
    {
        MCORE_UNUSED(rightButtonPressed);
        MCORE_UNUSED(middleButtonPressed);
        MCORE_UNUSED(leftButtonPressed);


        EKeyboardButtonState buttonState = (EKeyboardButtonState)keyboardKeyFlags;

        AZ::Matrix4x4 transposedViewMatrix(m_viewMatrix);
        transposedViewMatrix.Transpose();

        // get the movement direction vector based on the keyboard input
        AZ::Vector3 deltaMovement(0.0f, 0.0f, 0.0f);
        if (buttonState & FORWARD)
        {
            deltaMovement += MCore::GetForward(transposedViewMatrix);
        }
        if (buttonState & BACKWARD)
        {
            deltaMovement -= MCore::GetForward(transposedViewMatrix);
        }
        if (buttonState & RIGHT)
        {
            deltaMovement += MCore::GetRight(transposedViewMatrix);
        }
        if (buttonState & LEFT)
        {
            deltaMovement -= MCore::GetRight(transposedViewMatrix);
        }
        if (buttonState & UP)
        {
            deltaMovement += MCore::GetUp(transposedViewMatrix);
        }
        if (buttonState & DOWN)
        {
            deltaMovement -= MCore::GetUp(transposedViewMatrix);
        }

        // only move the camera when the delta movement is not the zero vector
        if (MCore::SafeLength(deltaMovement) > MCore::Math::epsilon)
        {
            m_position += deltaMovement.GetNormalized() * m_translationSpeed;
        }

        // rotate the camera
        if (buttonState & ENABLE_MOUSELOOK)
        {
            m_yaw    += mouseMovementX * m_rotationSpeed;
            m_pitch  += mouseMovementY * m_rotationSpeed;
        }
    }


    // reset the camera attributes
    void FirstPersonCamera::Reset(float flightTime)
    {
        MCORE_UNUSED(flightTime);

        // reset the base class attributes
        Camera::Reset();

        m_pitch  = 0.0f;
        m_yaw    = 0.0f;
        m_roll   = 0.0f;
    }
} // namespace MCommon
