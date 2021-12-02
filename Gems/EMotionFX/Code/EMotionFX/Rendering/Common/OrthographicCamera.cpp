/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "OrthographicCamera.h"
#include <MCore/Source/AABB.h>
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
        m_projectionMode = PROJMODE_ORTHOGRAPHIC;
    }


    // destructor
    OrthographicCamera::~OrthographicCamera()
    {
    }


    // update the camera position, orientation and it's matrices
    void OrthographicCamera::Update(float timeDelta)
    {
        if (m_flightActive)
        {
            m_flightCurrentTime += timeDelta;

            const float normalizedTime      = m_flightCurrentTime / m_flightMaxTime;
            const float interpolatedTime    = MCore::CosineInterpolate<float>(0.0f, 1.0f, normalizedTime);

            m_position           = m_flightSourcePosition + (m_flightTargetPosition - m_flightSourcePosition) * interpolatedTime;
            m_currentDistance    = m_flightSourceDistance + (m_flightTargetDistance - m_flightSourceDistance) * interpolatedTime;

            if (m_flightCurrentTime >= m_flightMaxTime)
            {
                m_flightActive       = false;
                m_position           = m_flightTargetPosition;
                m_currentDistance    = m_flightTargetDistance;
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

        // fake zoom the orthographic camera
        const float orthoScale  = scale * 0.001f;
        const float deltaX      = m_currentDistance * m_screenWidth * orthoScale;
        const float deltaY      = m_currentDistance * m_screenHeight * orthoScale;
        SetOrthoClipDimensions(AZ::Vector2(deltaX, deltaY));

        // adjust the mouse delta movement so that one pixel mouse movement is exactly one pixel on screen
        m_positionDelta.SetX(m_positionDelta.GetX() * m_currentDistance * orthoScale);
        m_positionDelta.SetY(m_positionDelta.GetY() * m_currentDistance * orthoScale);

        AZ::Vector3 xAxis, yAxis, zAxis;
        switch (m_mode)
        {
        case VIEWMODE_FRONT:
        {
            xAxis = AZ::Vector3(-1.0f, 0.0f, 0.0f);      // screen x axis
            yAxis = AZ::Vector3(0.0f, 0.0f, 1.0f);       // screen y axis
            zAxis = AZ::Vector3(0.0f, 1.0f, 0.0f);       // depth axis

            // translate the camera
            m_position += xAxis * -m_positionDelta.GetX();
            m_position += yAxis * m_positionDelta.GetY();

            // setup the view matrix
            MCore::LookAtRH(m_viewMatrix, m_position + zAxis * m_currentDistance, m_position, yAxis);
            break;
        }

        case VIEWMODE_BACK:
        {
            xAxis = AZ::Vector3(1.0f, 0.0f, 0.0f);       // screen x axis
            yAxis = AZ::Vector3(0.0f, 0.0f, 1.0f);       // screen y axis
            zAxis = AZ::Vector3(0.0f, -1.0f, 0.0f);      // depth axis

            // translate the camera
            m_position += xAxis * -m_positionDelta.GetX();
            m_position += yAxis * m_positionDelta.GetY();

            // setup the view matrix
            MCore::LookAtRH(m_viewMatrix, m_position + zAxis * m_currentDistance, m_position, yAxis);
            break;
        }


        case VIEWMODE_LEFT:
        {
            xAxis = AZ::Vector3(0.0f, 1.0f, 0.0f);       // screen x axis
            yAxis = AZ::Vector3(0.0f, 0.0f, 1.0f);       // screen y axis
            zAxis = AZ::Vector3(-1.0f, 0.0f, 0.0f);      // depth axis

            // translate the camera
            m_position += xAxis * m_positionDelta.GetX();
            m_position += yAxis * m_positionDelta.GetY();

            // setup the view matrix
            MCore::LookAtRH(m_viewMatrix, m_position + zAxis * m_currentDistance, m_position, yAxis);
            break;
        }

        case VIEWMODE_RIGHT:
        {
            xAxis = AZ::Vector3(0.0f, -1.0f, 0.0f);      // screen x axis
            yAxis = AZ::Vector3(0.0f, 0.0f, 1.0f);       // screen y axis
            zAxis = AZ::Vector3(1.0f, 0.0f, 0.0f);       // depth axis

            // translate the camera
            m_position += xAxis * m_positionDelta.GetX();
            m_position += yAxis * m_positionDelta.GetY();

            // setup the view matrix
            MCore::LookAtRH(m_viewMatrix, m_position + zAxis * m_currentDistance, m_position, yAxis);
            break;
        }

        case VIEWMODE_TOP:
        {
            xAxis = AZ::Vector3(1.0f, 0.0f, 0.0f);       // screen x axis
            yAxis = AZ::Vector3(0.0f, 1.0f, 0.0f);       // screen y axis
            zAxis = AZ::Vector3(0.0f, 0.0f, 1.0f);       // depth axis

            // translate the camera
            m_position += -xAxis* m_positionDelta.GetX();
            m_position += yAxis * m_positionDelta.GetY();

            // setup the view matrix
            MCore::LookAtRH(m_viewMatrix, m_position + zAxis * m_currentDistance, m_position, yAxis);
            break;
        }

        case VIEWMODE_BOTTOM:
        {
            xAxis = AZ::Vector3(-1.0f, 0.0f, 0.0f);       // screen x axis
            yAxis = AZ::Vector3(0.0f, 1.0f, 0.0f);        // screen y axis
            zAxis = AZ::Vector3(0.0f, 0.0f, -1.0f);       // depth axis

            // translate the camera
            m_position += -xAxis* m_positionDelta.GetX();
            m_position += yAxis * m_positionDelta.GetY();

            // setup the view matrix
            MCore::LookAtRH(m_viewMatrix, m_position + zAxis * m_currentDistance, m_position, yAxis);
            break;
        }
        }
        ;

        // reset the position delta
        m_positionDelta = AZ::Vector2(0.0f, 0.0f);

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
            const float distanceScale = m_currentDistance * 0.002f;
            m_currentDistance += (float)-mouseMovementY * distanceScale;
        }

        // is middle (or left+right) mouse button pressed?
        // move camera forward, backward, left or right
        if ((leftButtonPressed == false && rightButtonPressed == false && middleButtonPressed) ||
            (leftButtonPressed && rightButtonPressed && middleButtonPressed == false))
        {
            m_positionDelta.SetX((float)mouseMovementX);
            m_positionDelta.SetY((float)mouseMovementY);
        }
    }


    // reset the camera attributes
    void OrthographicCamera::Reset(float flightTime)
    {
        m_positionDelta      = AZ::Vector2(0.0f, 0.0f);
        m_minDistance        = MCore::Math::epsilon;
        m_maxDistance        = m_farClipDistance * 0.5f;

        AZ::Vector3 resetPosition(0.0f, 0.0f, 0.0f);
        switch (m_mode)
        {
        case VIEWMODE_FRONT:
        {
            resetPosition.SetY(m_currentDistance);
            break;
        }
        case VIEWMODE_BACK:
        {
            resetPosition.SetY(-m_currentDistance);
            break;
        }
        case VIEWMODE_LEFT:
        {
            resetPosition.SetX(-m_currentDistance);
            break;
        }
        case VIEWMODE_RIGHT:
        {
            resetPosition.SetX(m_currentDistance);
            break;
        }
        case VIEWMODE_TOP:
        {
            resetPosition.SetZ(m_currentDistance);
            break;
        }
        case VIEWMODE_BOTTOM:
        {
            resetPosition.SetZ(-m_currentDistance);
            break;
        }
        }
        ;

        if (flightTime < MCore::Math::epsilon)
        {
            m_flightActive           = false;
            m_currentDistance        = (float)MCore::Distance::ConvertValue(5.0f, MCore::Distance::UNITTYPE_METERS, EMotionFX::GetEMotionFX().GetUnitType());
            m_position               = resetPosition;
        }
        else
        {
            m_flightActive           = true;
            m_flightMaxTime          = flightTime;
            m_flightCurrentTime      = 0.0f;
            m_flightSourceDistance   = m_currentDistance;
            m_flightTargetDistance   = (float)MCore::Distance::ConvertValue(5.0f, MCore::Distance::UNITTYPE_METERS, EMotionFX::GetEMotionFX().GetUnitType());
            m_flightSourcePosition   = m_position;
            m_flightTargetPosition   = resetPosition;
        }

        // reset the base class attributes
        Camera::Reset();
    }


    void OrthographicCamera::StartFlight(float distance, const AZ::Vector3& position, float flightTime)
    {
        m_flightMaxTime          = flightTime;
        m_flightCurrentTime      = 0.0f;
        m_flightSourceDistance   = m_currentDistance;
        m_flightSourcePosition   = m_position;

        if (flightTime < MCore::Math::epsilon)
        {
            m_flightActive           = false;
            m_currentDistance        = distance;
            m_position               = position;
        }
        else
        {
            m_flightActive           = true;
            m_flightTargetDistance   = distance;
            m_flightTargetPosition   = position;
        }
    }


    // closeup view of the given bounding box
    void OrthographicCamera::ViewCloseup(const MCore::AABB& boundingBox, float flightTime)
    {
        m_flightMaxTime          = flightTime;
        m_flightCurrentTime      = 0.0f;
        m_flightSourceDistance   = m_currentDistance;
        m_flightSourcePosition   = m_position;

        float boxWidth = 0.0f;
        float boxHeight = 0.0f;
        switch (m_mode)
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
        assert(m_screenWidth != 0 && m_screenHeight != 0);
        const float distanceX = (boxWidth) / (m_screenWidth * orthoScale);
        const float distanceY = (boxHeight) / (m_screenHeight * orthoScale);
        //LOG("box: x=%f y=%f, boxAspect=%f, orthoAspect=%f, distX=%f, distY=%f", boxWidth, boxHeight, boxAspect, orthoAspect, distanceX, distanceY);

        if (flightTime < MCore::Math::epsilon)
        {
            m_flightActive           = false;
            m_currentDistance        = MCore::Max(distanceX, distanceY) * 1.1f;
            m_position               = boundingBox.CalcMiddle();
        }
        else
        {
            m_flightActive           = true;
            m_flightTargetDistance = MCore::Max(distanceX, distanceY) * 1.1f;
            m_flightTargetPosition = boundingBox.CalcMiddle();
        }

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


    // get the type identification string
    const char* OrthographicCamera::GetTypeString() const
    {
        switch (m_mode)
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
        AZ::Vector3  start = MCore::UnprojectOrtho(static_cast<float>(screenX), static_cast<float>(screenY), static_cast<float>(m_screenWidth), static_cast<float>(m_screenHeight), -1.0f, m_projectionMatrix, m_viewMatrix);
        AZ::Vector3  end = MCore::UnprojectOrtho(static_cast<float>(screenX), static_cast<float>(screenY), static_cast<float>(m_screenWidth), static_cast<float>(m_screenHeight), 1.0f, m_projectionMatrix, m_viewMatrix);

        return MCore::Ray(start, end);
    }


    // update limits
    void OrthographicCamera::AutoUpdateLimits()
    {
        m_minDistance    = m_nearClipDistance;
        m_maxDistance    = m_farClipDistance * 0.5f;
    }
} // namespace MCommon
