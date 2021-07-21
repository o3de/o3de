/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "OrthographicCamera.h"
#include <MCore/Source/Compare.h>
#include <MCore/Source/Distance.h>
#include <EMotionFX/Source/EMotionFXManager.h>


namespace MCommon
{
    // constructor
    OrthographicCamera::OrthographicCamera(ViewMode viewMode)
        : Camera()
    {
        Reset();
        SetMode(viewMode);
        mProjectionMode = PROJMODE_ORTHOGRAPHIC;
    }


    // destructor
    OrthographicCamera::~OrthographicCamera()
    {
    }


    // update the camera position, orientation and it's matrices
    void OrthographicCamera::Update(float timeDelta)
    {
        if (mFlightActive)
        {
            mFlightCurrentTime += timeDelta;

            const float normalizedTime      = mFlightCurrentTime / mFlightMaxTime;
            const float interpolatedTime    = MCore::CosineInterpolate<float>(0.0f, 1.0f, normalizedTime);

            mPosition           = mFlightSourcePosition + (mFlightTargetPosition - mFlightSourcePosition) * interpolatedTime;
            mCurrentDistance    = mFlightSourceDistance + (mFlightTargetDistance - mFlightSourceDistance) * interpolatedTime;

            if (mFlightCurrentTime >= mFlightMaxTime)
            {
                mFlightActive       = false;
                mPosition           = mFlightTargetPosition;
                mCurrentDistance    = mFlightTargetDistance;
            }
        }

        // HACK TODO REMOVEME !!!
        const float scale = 1.0f;
        mCurrentDistance *= scale;

        if (mCurrentDistance <= mMinDistance * scale)
        {
            mCurrentDistance = mMinDistance * scale;
        }

        if (mCurrentDistance >= mMaxDistance * scale)
        {
            mCurrentDistance = mMaxDistance * scale;
        }

        // fake zoom the orthographic camera
        const float orthoScale  = scale * 0.001f;
        const float deltaX      = mCurrentDistance * mScreenWidth * orthoScale;
        const float deltaY      = mCurrentDistance * mScreenHeight * orthoScale;
        SetOrthoClipDimensions(AZ::Vector2(deltaX, deltaY));

        // adjust the mouse delta movement so that one pixel mouse movement is exactly one pixel on screen
        mPositionDelta.SetX(mPositionDelta.GetX() * mCurrentDistance * orthoScale);
        mPositionDelta.SetY(mPositionDelta.GetY() * mCurrentDistance * orthoScale);

        AZ::Vector3 xAxis, yAxis, zAxis;
        switch (mMode)
        {
        case VIEWMODE_FRONT:
        {
            xAxis = AZ::Vector3(-1.0f, 0.0f, 0.0f);      // screen x axis
            yAxis = AZ::Vector3(0.0f, 0.0f, 1.0f);       // screen y axis
            zAxis = AZ::Vector3(0.0f, 1.0f, 0.0f);       // depth axis

            // translate the camera
            mPosition += xAxis * -mPositionDelta.GetX();
            mPosition += yAxis * mPositionDelta.GetY();

            // setup the view matrix
            MCore::LookAtRH(mViewMatrix, mPosition + zAxis * mCurrentDistance, mPosition, yAxis);
            break;
        }

        case VIEWMODE_BACK:
        {
            xAxis = AZ::Vector3(1.0f, 0.0f, 0.0f);       // screen x axis
            yAxis = AZ::Vector3(0.0f, 0.0f, 1.0f);       // screen y axis
            zAxis = AZ::Vector3(0.0f, -1.0f, 0.0f);      // depth axis

            // translate the camera
            mPosition += xAxis * -mPositionDelta.GetX();
            mPosition += yAxis * mPositionDelta.GetY();

            // setup the view matrix
            MCore::LookAtRH(mViewMatrix, mPosition + zAxis * mCurrentDistance, mPosition, yAxis);
            break;
        }


        case VIEWMODE_LEFT:
        {
            xAxis = AZ::Vector3(0.0f, 1.0f, 0.0f);       // screen x axis
            yAxis = AZ::Vector3(0.0f, 0.0f, 1.0f);       // screen y axis
            zAxis = AZ::Vector3(-1.0f, 0.0f, 0.0f);      // depth axis

            // translate the camera
            mPosition += xAxis * mPositionDelta.GetX();
            mPosition += yAxis * mPositionDelta.GetY();

            // setup the view matrix
            MCore::LookAtRH(mViewMatrix, mPosition + zAxis * mCurrentDistance, mPosition, yAxis);
            break;
        }

        case VIEWMODE_RIGHT:
        {
            xAxis = AZ::Vector3(0.0f, -1.0f, 0.0f);      // screen x axis
            yAxis = AZ::Vector3(0.0f, 0.0f, 1.0f);       // screen y axis
            zAxis = AZ::Vector3(1.0f, 0.0f, 0.0f);       // depth axis

            // translate the camera
            mPosition += xAxis * mPositionDelta.GetX();
            mPosition += yAxis * mPositionDelta.GetY();

            // setup the view matrix
            MCore::LookAtRH(mViewMatrix, mPosition + zAxis * mCurrentDistance, mPosition, yAxis);
            break;
        }

        case VIEWMODE_TOP:
        {
            xAxis = AZ::Vector3(1.0f, 0.0f, 0.0f);       // screen x axis
            yAxis = AZ::Vector3(0.0f, 1.0f, 0.0f);       // screen y axis
            zAxis = AZ::Vector3(0.0f, 0.0f, 1.0f);       // depth axis

            // translate the camera
            mPosition += -xAxis* mPositionDelta.GetX();
            mPosition += yAxis * mPositionDelta.GetY();

            // setup the view matrix
            MCore::LookAtRH(mViewMatrix, mPosition + zAxis * mCurrentDistance, mPosition, yAxis);
            break;
        }

        case VIEWMODE_BOTTOM:
        {
            xAxis = AZ::Vector3(-1.0f, 0.0f, 0.0f);       // screen x axis
            yAxis = AZ::Vector3(0.0f, 1.0f, 0.0f);        // screen y axis
            zAxis = AZ::Vector3(0.0f, 0.0f, -1.0f);       // depth axis

            // translate the camera
            mPosition += -xAxis* mPositionDelta.GetX();
            mPosition += yAxis * mPositionDelta.GetY();

            // setup the view matrix
            MCore::LookAtRH(mViewMatrix, mPosition + zAxis * mCurrentDistance, mPosition, yAxis);
            break;
        }
        }
        ;

        // reset the position delta
        mPositionDelta = AZ::Vector2(0.0f, 0.0f);

        // update our base camera
        Camera::Update();
    }


    // process input and modify the camera attributes
    void OrthographicCamera::ProcessMouseInput(int32 mouseMovementX, int32 mouseMovementY, bool leftButtonPressed, bool middleButtonPressed, bool rightButtonPressed, uint32 keyboardKeyFlags)
    {
        MCORE_UNUSED(keyboardKeyFlags);

        // is right mouse button pressed?
        // zoom camera in or out
        if (leftButtonPressed == false && rightButtonPressed && middleButtonPressed == false)
        {
            const float distanceScale = mCurrentDistance * 0.002f;
            mCurrentDistance += (float)-mouseMovementY * distanceScale;
        }

        // is middle (or left+right) mouse button pressed?
        // move camera forward, backward, left or right
        if ((leftButtonPressed == false && rightButtonPressed == false && middleButtonPressed) ||
            (leftButtonPressed && rightButtonPressed && middleButtonPressed == false))
        {
            mPositionDelta.SetX((float)mouseMovementX);
            mPositionDelta.SetY((float)mouseMovementY);
        }
    }


    // reset the camera attributes
    void OrthographicCamera::Reset(float flightTime)
    {
        mPositionDelta      = AZ::Vector2(0.0f, 0.0f);
        mMinDistance        = MCore::Math::epsilon;
        mMaxDistance        = mFarClipDistance * 0.5f;

        AZ::Vector3 resetPosition(0.0f, 0.0f, 0.0f);
        switch (mMode)
        {
        case VIEWMODE_FRONT:
        {
            resetPosition.SetY(mCurrentDistance);
            break;
        }
        case VIEWMODE_BACK:
        {
            resetPosition.SetY(-mCurrentDistance);
            break;
        }
        case VIEWMODE_LEFT:
        {
            resetPosition.SetX(-mCurrentDistance);
            break;
        }
        case VIEWMODE_RIGHT:
        {
            resetPosition.SetX(mCurrentDistance);
            break;
        }
        case VIEWMODE_TOP:
        {
            resetPosition.SetZ(mCurrentDistance);
            break;
        }
        case VIEWMODE_BOTTOM:
        {
            resetPosition.SetZ(-mCurrentDistance);
            break;
        }
        }
        ;

        if (flightTime < MCore::Math::epsilon)
        {
            mFlightActive           = false;
            mCurrentDistance        = (float)MCore::Distance::ConvertValue(5.0f, MCore::Distance::UNITTYPE_METERS, EMotionFX::GetEMotionFX().GetUnitType());
            mPosition               = resetPosition;
        }
        else
        {
            mFlightActive           = true;
            mFlightMaxTime          = flightTime;
            mFlightCurrentTime      = 0.0f;
            mFlightSourceDistance   = mCurrentDistance;
            mFlightTargetDistance   = (float)MCore::Distance::ConvertValue(5.0f, MCore::Distance::UNITTYPE_METERS, EMotionFX::GetEMotionFX().GetUnitType());
            mFlightSourcePosition   = mPosition;
            mFlightTargetPosition   = resetPosition;
        }

        // reset the base class attributes
        Camera::Reset();
    }


    void OrthographicCamera::StartFlight(float distance, const AZ::Vector3& position, float flightTime)
    {
        mFlightMaxTime          = flightTime;
        mFlightCurrentTime      = 0.0f;
        mFlightSourceDistance   = mCurrentDistance;
        mFlightSourcePosition   = mPosition;

        if (flightTime < MCore::Math::epsilon)
        {
            mFlightActive           = false;
            mCurrentDistance        = distance;
            mPosition               = position;
        }
        else
        {
            mFlightActive           = true;
            mFlightTargetDistance   = distance;
            mFlightTargetPosition   = position;
        }
    }


    // closeup view of the given bounding box
    void OrthographicCamera::ViewCloseup(const MCore::AABB& boundingBox, float flightTime)
    {
        mFlightMaxTime          = flightTime;
        mFlightCurrentTime      = 0.0f;
        mFlightSourceDistance   = mCurrentDistance;
        mFlightSourcePosition   = mPosition;

        float boxWidth = 0.0f;
        float boxHeight = 0.0f;
        switch (mMode)
        {
        case VIEWMODE_FRONT:
        {
            boxWidth = boundingBox.CalcWidth();
            boxHeight = boundingBox.CalcHeight();
            break;
        }
        case VIEWMODE_BACK:
        {
            boxWidth = boundingBox.CalcWidth();
            boxHeight = boundingBox.CalcHeight();
            break;
        }
        case VIEWMODE_LEFT:
        {
            boxWidth = boundingBox.CalcDepth();
            boxHeight = boundingBox.CalcHeight();
            break;
        }
        case VIEWMODE_RIGHT:
        {
            boxWidth = boundingBox.CalcDepth();
            boxHeight = boundingBox.CalcHeight();
            break;
        }
        case VIEWMODE_TOP:
        {
            boxWidth = boundingBox.CalcWidth();
            boxHeight = boundingBox.CalcDepth();
            break;
        }
        case VIEWMODE_BOTTOM:
        {
            boxWidth = boundingBox.CalcWidth();
            boxHeight = boundingBox.CalcDepth();
            break;
        }
        }
        ;

        const float orthoScale  = 0.001f;
        assert(mScreenWidth != 0 && mScreenHeight != 0);
        const float distanceX = (boxWidth) / (mScreenWidth * orthoScale);
        const float distanceY = (boxHeight) / (mScreenHeight * orthoScale);
        //LOG("box: x=%f y=%f, boxAspect=%f, orthoAspect=%f, distX=%f, distY=%f", boxWidth, boxHeight, boxAspect, orthoAspect, distanceX, distanceY);

        if (flightTime < MCore::Math::epsilon)
        {
            mFlightActive           = false;
            mCurrentDistance        = MCore::Max(distanceX, distanceY) * 1.1f;
            mPosition               = boundingBox.CalcMiddle();
        }
        else
        {
            mFlightActive           = true;
            mFlightTargetDistance = MCore::Max(distanceX, distanceY) * 1.1f;
            mFlightTargetPosition = boundingBox.CalcMiddle();
        }

        // make sure the target flight distance is in range
        if (mFlightTargetDistance < mMinDistance)
        {
            mFlightTargetDistance = mMinDistance;
        }
        if (mFlightTargetDistance > mMaxDistance)
        {
            mFlightTargetDistance = mMaxDistance;
        }
    }


    // get the type identification string
    const char* OrthographicCamera::GetTypeString() const
    {
        switch (mMode)
        {
        case VIEWMODE_FRONT:
        {
            return "Front";
        }
        case VIEWMODE_BACK:
        {
            return "Back";
        }
        case VIEWMODE_LEFT:
        {
            return "Left";
        }
        case VIEWMODE_RIGHT:
        {
            return "Right";
        }
        case VIEWMODE_TOP:
        {
            return "Top";
        }
        case VIEWMODE_BOTTOM:
        {
            return "Bottom";
        }
        }
        ;

        return "Orthographic";
    }


    // get the type identification
    uint32 OrthographicCamera::GetType() const
    {
        return TYPE_ID;
    }


    // unproject screen coordinates to a ray
    MCore::Ray OrthographicCamera::Unproject(int32 screenX, int32 screenY)
    {
        AZ::Vector3  start = MCore::UnprojectOrtho(static_cast<float>(screenX), static_cast<float>(screenY), static_cast<float>(mScreenWidth), static_cast<float>(mScreenHeight), -1.0f, mProjectionMatrix, mViewMatrix);
        AZ::Vector3  end = MCore::UnprojectOrtho(static_cast<float>(screenX), static_cast<float>(screenY), static_cast<float>(mScreenWidth), static_cast<float>(mScreenHeight), 1.0f, mProjectionMatrix, mViewMatrix);

        return MCore::Ray(start, end);
    }


    // update limits
    void OrthographicCamera::AutoUpdateLimits()
    {
        mMinDistance    = mNearClipDistance;
        mMaxDistance    = mFarClipDistance * 0.5f;
    }
} // namespace MCommon
