/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
        mPosition       = AZ::Vector3::CreateZero();
        mScreenWidth    = 0;
        mScreenHeight   = 0;
        mProjectionMode = PROJMODE_PERSPECTIVE;
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
        switch (mProjectionMode)
        {
        // initialize for perspective projection
        case PROJMODE_PERSPECTIVE:
        {
            MCore::PerspectiveRH(mProjectionMatrix, MCore::Math::DegreesToRadians(mFOV), mAspect, mNearClipDistance, mFarClipDistance);
            break;
        }

        // initialize for orthographic projection
        case PROJMODE_ORTHOGRAPHIC:
        {
            const float halfX = mOrthoClipDimensions.GetX() * 0.5f;
            const float halfY = mOrthoClipDimensions.GetY() * 0.5f;
            MCore::OrthoOffCenterRH(mProjectionMatrix, -halfX, halfX, halfY, -halfY, -mFarClipDistance, mFarClipDistance);
            break;
        }
        }

        // calculate the viewproj matrix
        mViewProjMatrix = mProjectionMatrix * mViewMatrix;
    }


    // reset the base camera attributes
    void Camera::Reset(float flightTime)
    {
        MCORE_UNUSED(flightTime);

        mFOV                = 55.0f;
        mNearClipDistance   = 0.1f;
        mFarClipDistance    = 200.0f;
        mAspect             = 16.0f / 9.0f;
        mRotationSpeed      = 0.5f;
        mTranslationSpeed   = 1.0f;
        mViewMatrix = AZ::Matrix4x4::CreateIdentity();
    }


    // unproject screen coordinates to a ray
    MCore::Ray Camera::Unproject(int32 screenX, int32 screenY)
    {
        const AZ::Matrix4x4 invProj = MCore::InvertProjectionMatrix(mProjectionMatrix);
        const AZ::Matrix4x4 invView = MCore::InvertProjectionMatrix(mViewMatrix);

        const AZ::Vector3  start = MCore::Unproject(static_cast<float>(screenX), static_cast<float>(screenY), static_cast<float>(mScreenWidth), static_cast<float>(mScreenHeight), mNearClipDistance, invProj, invView);
        const AZ::Vector3  end   = MCore::Unproject(static_cast<float>(screenX), static_cast<float>(screenY), static_cast<float>(mScreenWidth), static_cast<float>(mScreenHeight), mFarClipDistance, invProj, invView);

        return MCore::Ray(start, end);
    }
} // namespace MCommon
