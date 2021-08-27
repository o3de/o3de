/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "Camera.h"
#include <MCore/Source/AzCoreConversions.h>
#include <MCore/Source/LogManager.h>


namespace MCommon
{
    // constructor
    Camera::Camera()
    {
        Reset();
        m_position       = AZ::Vector3::CreateZero();
        m_screenWidth    = 0;
        m_screenHeight   = 0;
        m_projectionMode = PROJMODE_PERSPECTIVE;
    }


    // destructor
    Camera::~Camera()
    {
    }


    // update the camera
    void Camera::Update(float timeDelta)
    {
        MCORE_UNUSED(timeDelta);

        // setup projection matrix
        switch (m_projectionMode)
        {
        // initialize for perspective projection
        case PROJMODE_PERSPECTIVE:
        {
            MCore::PerspectiveRH(m_projectionMatrix, MCore::Math::DegreesToRadians(m_fov), m_aspect, m_nearClipDistance, m_farClipDistance);
            break;
        }

        // initialize for orthographic projection
        case PROJMODE_ORTHOGRAPHIC:
        {
            const float halfX = m_orthoClipDimensions.GetX() * 0.5f;
            const float halfY = m_orthoClipDimensions.GetY() * 0.5f;
            MCore::OrthoOffCenterRH(m_projectionMatrix, -halfX, halfX, halfY, -halfY, -m_farClipDistance, m_farClipDistance);
            break;
        }
        }

        // calculate the viewproj matrix
        m_viewProjMatrix = m_projectionMatrix * m_viewMatrix;
    }


    // reset the base camera attributes
    void Camera::Reset(float flightTime)
    {
        MCORE_UNUSED(flightTime);

        m_fov                = 55.0f;
        m_nearClipDistance   = 0.1f;
        m_farClipDistance    = 200.0f;
        m_aspect             = 16.0f / 9.0f;
        m_rotationSpeed      = 0.5f;
        m_translationSpeed   = 1.0f;
        m_viewMatrix = AZ::Matrix4x4::CreateIdentity();
    }


    // unproject screen coordinates to a ray
    MCore::Ray Camera::Unproject(int32 screenX, int32 screenY)
    {
        const AZ::Matrix4x4 invProj = MCore::InvertProjectionMatrix(m_projectionMatrix);
        const AZ::Matrix4x4 invView = MCore::InvertProjectionMatrix(m_viewMatrix);

        const AZ::Vector3  start = MCore::Unproject(static_cast<float>(screenX), static_cast<float>(screenY), static_cast<float>(m_screenWidth), static_cast<float>(m_screenHeight), m_nearClipDistance, invProj, invView);
        const AZ::Vector3  end   = MCore::Unproject(static_cast<float>(screenX), static_cast<float>(screenY), static_cast<float>(m_screenWidth), static_cast<float>(m_screenHeight), m_farClipDistance, invProj, invView);

        return MCore::Ray(start, end);
    }
} // namespace MCommon
