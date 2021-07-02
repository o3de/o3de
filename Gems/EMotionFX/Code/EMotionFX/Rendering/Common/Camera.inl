/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "Camera.h"


// set the camera position
MCORE_INLINE void Camera::SetPosition(const AZ::Vector3& position)
{
    mPosition = position;
}


// get the camera position
MCORE_INLINE const AZ::Vector3& Camera::GetPosition() const
{
    return mPosition;
}


// set the projection type
MCORE_INLINE void Camera::SetProjectionMode(ProjectionMode projectionMode)
{
    mProjectionMode = projectionMode;
}


// get the projection type
MCORE_INLINE Camera::ProjectionMode Camera::GetProjectionMode() const
{
    return mProjectionMode;
}


// set the clip dimensions for the orthographic projection mode
MCORE_INLINE void Camera::SetOrthoClipDimensions(const AZ::Vector2& clipDimensions)
{
    mOrthoClipDimensions = clipDimensions;
}


// set the screen dimensions where this camera is used in
MCORE_INLINE void Camera::SetScreenDimensions(uint32 width, uint32 height)
{
    mScreenWidth = width;
    mScreenHeight = height;
}


// set the field of view in degrees
MCORE_INLINE void Camera::SetFOV(float fieldOfView)
{
    mFOV = fieldOfView;
}


// set near clip plane distance
MCORE_INLINE void Camera::SetNearClipDistance(float nearClipDistance)
{
    mNearClipDistance = nearClipDistance;
}


// set far clip plane distance
MCORE_INLINE void Camera::SetFarClipDistance(float farClipDistance)
{
    mFarClipDistance = farClipDistance;
}


// set the aspect ratio - the aspect ratio is calculated by width/height
MCORE_INLINE void Camera::SetAspectRatio(float aspect)
{
    mAspect = aspect;
}


// return the field of view in degrees
MCORE_INLINE float Camera::GetFOV() const
{
    return mFOV;
}


// return the near clip plane distance
MCORE_INLINE float Camera::GetNearClipDistance() const
{
    return mNearClipDistance;
}


// return the far clip plane distance
MCORE_INLINE float Camera::GetFarClipDistance() const
{
    return mFarClipDistance;
}


// return the aspect ratio
MCORE_INLINE float Camera::GetAspectRatio() const
{
    return mAspect;
}


// get the translation speed
MCORE_INLINE float Camera::GetTranslationSpeed() const
{
    return mTranslationSpeed;
}


// set the translation speed
MCORE_INLINE void Camera::SetTranslationSpeed(float translationSpeed)
{
    mTranslationSpeed = translationSpeed;
}


// get the rotation speed in degrees
MCORE_INLINE float Camera::GetRotationSpeed() const
{
    return mRotationSpeed;
}


// set the rotation speed in degrees
MCORE_INLINE void Camera::SetRotationSpeed(float rotationSpeed)
{
    mRotationSpeed = rotationSpeed;
}


// Get the screen width.
MCORE_INLINE uint32 Camera::GetScreenWidth()
{
    return mScreenWidth;
}


// Get the screen height
MCORE_INLINE uint32 Camera::GetScreenHeight()
{
    return mScreenHeight;
}
