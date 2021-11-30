/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "Camera.h"


// set the camera position
MCORE_INLINE void Camera::SetPosition(const AZ::Vector3& position)
{
    m_position = position;
}


// get the camera position
MCORE_INLINE const AZ::Vector3& Camera::GetPosition() const
{
    return m_position;
}


// set the projection type
MCORE_INLINE void Camera::SetProjectionMode(ProjectionMode projectionMode)
{
    m_projectionMode = projectionMode;
}


// get the projection type
MCORE_INLINE Camera::ProjectionMode Camera::GetProjectionMode() const
{
    return m_projectionMode;
}


// set the clip dimensions for the orthographic projection mode
MCORE_INLINE void Camera::SetOrthoClipDimensions(const AZ::Vector2& clipDimensions)
{
    m_orthoClipDimensions = clipDimensions;
}


// set the screen dimensions where this camera is used in
MCORE_INLINE void Camera::SetScreenDimensions(uint32 width, uint32 height)
{
    m_screenWidth = width;
    m_screenHeight = height;
}


// set the field of view in degrees
MCORE_INLINE void Camera::SetFOV(float fieldOfView)
{
    m_fov = fieldOfView;
}


// set near clip plane distance
MCORE_INLINE void Camera::SetNearClipDistance(float nearClipDistance)
{
    m_nearClipDistance = nearClipDistance;
}


// set far clip plane distance
MCORE_INLINE void Camera::SetFarClipDistance(float farClipDistance)
{
    m_farClipDistance = farClipDistance;
}


// set the aspect ratio - the aspect ratio is calculated by width/height
MCORE_INLINE void Camera::SetAspectRatio(float aspect)
{
    m_aspect = aspect;
}


// return the field of view in degrees
MCORE_INLINE float Camera::GetFOV() const
{
    return m_fov;
}


// return the near clip plane distance
MCORE_INLINE float Camera::GetNearClipDistance() const
{
    return m_nearClipDistance;
}


// return the far clip plane distance
MCORE_INLINE float Camera::GetFarClipDistance() const
{
    return m_farClipDistance;
}


// return the aspect ratio
MCORE_INLINE float Camera::GetAspectRatio() const
{
    return m_aspect;
}


// get the translation speed
MCORE_INLINE float Camera::GetTranslationSpeed() const
{
    return m_translationSpeed;
}


// set the translation speed
MCORE_INLINE void Camera::SetTranslationSpeed(float translationSpeed)
{
    m_translationSpeed = translationSpeed;
}


// get the rotation speed in degrees
MCORE_INLINE float Camera::GetRotationSpeed() const
{
    return m_rotationSpeed;
}


// set the rotation speed in degrees
MCORE_INLINE void Camera::SetRotationSpeed(float rotationSpeed)
{
    m_rotationSpeed = rotationSpeed;
}


// Get the screen width.
MCORE_INLINE uint32 Camera::GetScreenWidth()
{
    return m_screenWidth;
}


// Get the screen height
MCORE_INLINE uint32 Camera::GetScreenHeight()
{
    return m_screenHeight;
}
