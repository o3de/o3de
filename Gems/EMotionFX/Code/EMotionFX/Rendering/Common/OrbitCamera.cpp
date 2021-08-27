/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "OrbitCamera.h"
#include <EMotionFX/Source/EMotionFXManager.h>
#include <MCore/Source/AABB.h>


namespace MCommon
{
    // constructor
    OrbitCamera::OrbitCamera()
        : LookAtCamera()
    {
        Reset();
    }


    // destructor
    OrbitCamera::~OrbitCamera()
    {
    }


    // reset the camera attributes
    void OrbitCamera::Reset(float flightTime)
    {
        // reset the parent class attributes
        LookAtCamera::Reset();

        m_minDistance            = m_nearClipDistance;
        m_maxDistance            = m_farClipDistance * 0.5f;
        m_position               = AZ::Vector3::CreateZero();
        m_positionDelta          = AZ::Vector2(0.0f, 0.0f);

        if (flightTime < MCore::Math::epsilon)
        {
            m_flightActive           = false;
            m_currentDistance        = (float)MCore::Distance::ConvertValue(5.0f, MCore::Distance::UNITTYPE_METERS, EMotionFX::GetEMotionFX().GetUnitType());
            m_alpha                  = GetDefaultAlpha();
            m_beta                   = GetDefaultBeta();
            m_target                 = AZ::Vector3::CreateZero();
        }
        else
        {
            m_flightActive           = true;
            m_flightMaxTime          = flightTime;
            m_flightCurrentTime      = 0.0f;
            m_flightSourceDistance   = m_currentDistance;
            m_flightTargetDistance   = (float)MCore::Distance::ConvertValue(5.0f, MCore::Distance::UNITTYPE_METERS, EMotionFX::GetEMotionFX().GetUnitType());
            m_flightSourcePosition   = m_target;
            m_flightTargetPosition   = AZ::Vector3::CreateZero();
            m_flightSourceAlpha      = m_alpha;
            m_flightTargetAlpha      = GetDefaultAlpha();
            m_flightSourceBeta       = m_beta;
            m_flightTargetBeta       = GetDefaultBeta();
        }
    }


    // update limits
    void OrbitCamera::AutoUpdateLimits()
    {
        m_minDistance    = m_nearClipDistance;
        m_maxDistance    = m_farClipDistance * 0.5f;
    }


    void OrbitCamera::StartFlight(float distance, const AZ::Vector3& position, float alpha, float beta, float flightTime)
    {
        m_flightActive           = true;
        m_flightMaxTime          = flightTime;
        m_flightCurrentTime      = 0.0f;
        m_flightSourceDistance   = m_currentDistance;
        m_flightSourcePosition   = m_target;
        m_flightTargetDistance   = distance;
        m_flightTargetPosition   = position;
        m_flightSourceAlpha      = m_alpha;
        m_flightTargetAlpha      = alpha;
        m_flightSourceBeta       = m_beta;
        m_flightTargetBeta       = beta;
    }


    // closeup view of the given bounding box
    void OrbitCamera::ViewCloseup(const MCore::AABB& boundingBox, float flightTime)
    {
        m_flightActive           = true;
        m_flightMaxTime          = flightTime;
        m_flightCurrentTime      = 0.0f;
        m_flightSourceDistance   = m_currentDistance;
        m_flightSourcePosition   = m_target;
        const float distanceHorizontalFOV = boundingBox.CalcRadius() / MCore::Math::Tan(0.5f * MCore::Math::DegreesToRadians(m_fov));
        const float distanceVerticalFOV = boundingBox.CalcRadius() / MCore::Math::Tan(0.5f * MCore::Math::DegreesToRadians(m_fov * m_aspect));
        m_flightTargetDistance   = MCore::Max(distanceHorizontalFOV, distanceVerticalFOV) * 0.9f;
        m_flightTargetPosition   = boundingBox.CalcMiddle();
        m_flightSourceAlpha      = m_alpha;
        m_flightSourceAlpha      = m_alpha;
        m_flightTargetAlpha      = GetDefaultAlpha();
        m_flightSourceBeta       = m_beta;
        m_flightTargetBeta       = GetDefaultBeta();

        // make sure the target flight distance is in range
        if (m_flightTargetDistance < m_minDistance)
        {
            m_flightTargetDistance = m_minDistance;
        }
        if (m_flightTargetDistance > m_maxDistance)
        {
            m_flightTargetDistance = m_maxDistance;
        }
    }


    // process input and modify the camera attributes
    void OrbitCamera::ProcessMouseInput(int32 mouseMovementX, int32 mouseMovementY, bool leftButtonPressed, bool middleButtonPressed, bool rightButtonPressed, uint32 keyboardKeyFlags)
    {
        MCORE_UNUSED(keyboardKeyFlags);

        // is left mouse button pressed?
        // rotate camera around target point
        if (leftButtonPressed && rightButtonPressed == false && middleButtonPressed == false)
        {
            // rotate our camera
            m_alpha  += m_rotationSpeed * (float)-mouseMovementX;
            m_beta   += m_rotationSpeed * (float) mouseMovementY;

            // prevent the camera from looking upside down
            if (m_beta >= 90.0f - 0.01f)
            {
                m_beta =  90.0f - 0.01f;
            }

            if (m_beta <= -90.0f + 0.01f)
            {
                m_beta =  -90.0f + 0.01f;
            }

            // reset the camera to no rotation if we made a whole circle
            if (m_alpha >= 360.0f || m_alpha <= -360.0f)
            {
                m_alpha = 0.0f;
            }
        }

        // is right mouse button pressed?
        // zoom camera in or out
        if (leftButtonPressed == false && rightButtonPressed && middleButtonPressed == false)
        {
            const float distanceScale = m_currentDistance * 0.002f;
            m_currentDistance += (float)-mouseMovementY * distanceScale;
        }

        // is middle (or left+right) mouse button pressed?
        // move camera forward, backward, left or right
        if ((leftButtonPressed == false && rightButtonPressed == false && middleButtonPressed) ||
            (leftButtonPressed &&  rightButtonPressed  && middleButtonPressed == false))
        {
            const float distanceScale = m_currentDistance * 0.002f;

            //if (MCore::GetCoordinateSystem().IsRightHanded())
            //distanceScale *= -1.0f;

            m_positionDelta.SetX((float)mouseMovementX * distanceScale);
            m_positionDelta.SetY((float)mouseMovementY * distanceScale);
        }
    }


    // update the camera
    void OrbitCamera::Update(float timeDelta)
    {
        if (m_flightActive)
        {
            m_flightCurrentTime += timeDelta;

            const float normalizedTime      = m_flightCurrentTime / m_flightMaxTime;
            const float interpolatedTime    = MCore::CosineInterpolate<float>(0.0f, 1.0f, normalizedTime);

            m_target             = m_flightSourcePosition + (m_flightTargetPosition - m_flightSourcePosition) * interpolatedTime;
            m_currentDistance    = m_flightSourceDistance + (m_flightTargetDistance - m_flightSourceDistance) * interpolatedTime;
            m_alpha              = m_flightSourceAlpha + (m_flightTargetAlpha - m_flightSourceAlpha) * interpolatedTime;
            m_beta               = m_flightSourceBeta + (m_flightTargetBeta - m_flightSourceBeta) * interpolatedTime;

            if (m_flightCurrentTime >= m_flightMaxTime)
            {
                m_flightActive       = false;
                m_target             = m_flightTargetPosition;
                m_currentDistance    = m_flightTargetDistance;
                m_alpha              = m_flightTargetAlpha;
                m_beta               = m_flightTargetBeta;
            }
        }

        // HACK TODO REMOVEME !!!
        const float scale = 1.0f;

        m_currentDistance *= scale;

        if (m_currentDistance <= m_minDistance * scale)
        {
            m_currentDistance = m_minDistance * scale;
        }

        if (m_currentDistance >= m_maxDistance * scale)
        {
            m_currentDistance = m_maxDistance * scale;
        }

        // calculate unit direction vector based on our two angles
        AZ::Vector3 unitSphereVector;
        unitSphereVector.SetX(MCore::Math::Cos(MCore::Math::DegreesToRadians(m_alpha)) * MCore::Math::Cos(MCore::Math::DegreesToRadians(m_beta)));
        unitSphereVector.SetY(MCore::Math::Sin(MCore::Math::DegreesToRadians(m_alpha)) * MCore::Math::Cos(MCore::Math::DegreesToRadians(m_beta)));
        unitSphereVector.SetZ(MCore::Math::Sin(MCore::Math::DegreesToRadians(m_beta)));

        // calculate the right and the up vector based on the direction vector
        AZ::Vector3 rightVec = unitSphereVector.Cross(AZ::Vector3(0.0f, 0.0f, 1.0f)).GetNormalized();
        AZ::Vector3 upVec = rightVec.Cross(unitSphereVector).GetNormalized();

        // calculate the lookat target and the camera position using our rotation sphere vectors
        m_target             += (rightVec * m_positionDelta.GetX()) * m_translationSpeed + (upVec * m_positionDelta.GetY()) * m_translationSpeed;
        m_position           = m_target + (unitSphereVector * m_currentDistance);

        // reset the position delta
        m_positionDelta      = AZ::Vector2(0.0f, 0.0f);

        // update our lookat camera at the very end
        LookAtCamera::Update();
    }


    // set all attributes to define a unique camera transformation and update it afterwards
    void OrbitCamera::Set(float alpha, float beta, float currentDistance, const AZ::Vector3& target)
    {
        SetAlpha(alpha);
        SetBeta(beta);
        SetCurrentDistance(currentDistance);
        SetTarget(target);
        Update();
    }
} // namespace MCommon
