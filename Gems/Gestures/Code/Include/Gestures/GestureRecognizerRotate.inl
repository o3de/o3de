/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <CryCommon/ISystem.h>
#include <CryCommon/ITimer.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
inline void Gestures::RecognizerRotate::Config::Reflect(AZ::ReflectContext* context)
{
    if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
    {
        serialize->Class<Config>()
            ->Version(0)
            ->Field("maxPixelsMoved", &Config::maxPixelsMoved)
            ->Field("minAngleDegrees", &Config::minAngleDegrees)
            ->Field("priority", &Config::priority)
        ;

        if (AZ::EditContext* ec = serialize->GetEditContext())
        {
            ec->Class<Config>("Rotate Config", "Configuration values used to setup a gesture recognizer for rotations.")
                ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                ->DataElement(AZ::Edit::UIHandlers::Default, &Config::maxPixelsMoved, "Max Pixels Moved", "The max distance in pixels that the touches can move towards or away from each other before a rotate will be recognized.")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                ->DataElement(AZ::Edit::UIHandlers::Default, &Config::minAngleDegrees, "Min Angle Degrees", "The min angle in degrees that must be rotated before the gesture will be recognized.")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
            ;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
inline Gestures::RecognizerRotate::RecognizerRotate(const Config& config)
    : m_config(config)
    , m_currentState(State::Idle)
{
    m_lastUpdateTimes[0] = 0;
    m_lastUpdateTimes[1] = 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
inline Gestures::RecognizerRotate::~RecognizerRotate()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
inline bool Gestures::RecognizerRotate::OnPressedEvent(const AZ::Vector2& screenPosition, uint32_t pointerIndex)
{
    if (pointerIndex > s_maxRotatePointerIndex)
    {
        return false;
    }

    switch (m_currentState)
    {
    case State::Idle:
    {
        m_currentPositions[pointerIndex] = screenPosition;
        m_currentState = pointerIndex ? State::Pressed1 : State::Pressed0;
    }
    break;
    case State::Pressed0:
    case State::Pressed1:
    {
        m_currentPositions[pointerIndex] = screenPosition;
        if (static_cast<uint32_t>(m_currentState) != pointerIndex)
        {
            m_startPositions[0] = m_currentPositions[0];
            m_startPositions[1] = m_currentPositions[1];
            m_currentState = State::PressedBoth;
        }
    }
    break;
    case State::PressedBoth:
    case State::Rotating:
    default:
    {
        // Should not be possible, but not fatal if we happen to get here somehow.
        AZ_Warning("RecognizerRotate", false, "RecognizerRotate::OnPressedEvent state logic failure");
    }
    break;
    }

    return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
inline bool Gestures::RecognizerRotate::OnDownEvent(const AZ::Vector2& screenPosition, uint32_t pointerIndex)
{
    if (pointerIndex > s_maxRotatePointerIndex)
    {
        return false;
    }

    m_currentPositions[pointerIndex] = screenPosition;
    m_lastUpdateTimes[pointerIndex] = (gEnv && gEnv->pTimer) ? gEnv->pTimer->GetFrameStartTime().GetValue() : 0;
    if (m_lastUpdateTimes[0] != m_lastUpdateTimes[1])
    {
        // We need to wait until both touches have been updated this frame.
        return false;
    }

    switch (m_currentState)
    {
    case State::PressedBoth:
    {
        const float distanceDeltaPixels = abs(GetCurrentDistance() - GetStartDistance());

        if (distanceDeltaPixels > m_config.maxPixelsMoved)
        {
            // The touches have moved too far towards or away from each other.
            // Reset the start positions so a rotate can still be initiated.
            m_startPositions[0] = m_currentPositions[0];
            m_startPositions[1] = m_currentPositions[1];
        }
        else if (abs(GetSignedRotationInDegrees()) >= m_config.minAngleDegrees)
        {
            // The imaginary line between the two touches has rotated by
            // an angle sufficient for a rotate gesture to be initiated.
            m_startPositions[0] = m_currentPositions[0];
            m_startPositions[1] = m_currentPositions[1];
            OnContinuousGestureInitiated();
            m_currentState = State::Rotating;
        }
    }
    break;
    case State::Rotating:
    {
        OnContinuousGestureUpdated();
    }
    break;
    case State::Pressed0:
    case State::Pressed1:
    case State::Idle:
    default:
    {
        // Should not be possible, but not fatal if we happen to get here somehow.
        AZ_Warning("RecognizerRotate", false, "RecognizerRotate::OnDownEvent state logic failure");
    }
    break;
    }

    return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
inline bool Gestures::RecognizerRotate::OnReleasedEvent(const AZ::Vector2& screenPosition, uint32_t pointerIndex)
{
    if (pointerIndex > s_maxRotatePointerIndex)
    {
        return false;
    }

    switch (m_currentState)
    {
    case State::Pressed0:
    case State::Pressed1:
    {
        if (static_cast<uint32_t>(m_currentState) != pointerIndex)
        {
            // Should not be possible, but not fatal if we happen to get here somehow.
            AZ_Warning("RecognizerRotate", false, "RecognizerRotate::OnReleasedEvent state logic failure");
            break;
        }

        // We never actually started rotating
        m_currentPositions[pointerIndex] = screenPosition;
        m_currentState = State::Idle;
    }
    break;
    case State::PressedBoth:
    {
        // We never actually started rotating
        m_currentPositions[pointerIndex] = screenPosition;
        m_currentState = pointerIndex ? State::Pressed0 : State::Pressed1;
    }
    break;
    case State::Rotating:
    {
        m_currentPositions[pointerIndex] = screenPosition;
        OnContinuousGestureEnded();
        m_currentState = pointerIndex ? State::Pressed0 : State::Pressed1;
    }
    break;
    case State::Idle:
    default:
    {
        // Should not be possible, but not fatal if we happen to get here somehow.
        AZ_Warning("RecognizerRotate", false, "RecognizerRotate::OnReleasedEvent state logic failure");
    }
    break;
    }

    return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
inline float Gestures::RecognizerRotate::GetSignedRotationInDegrees() const
{
    AZ::Vector2 vectorBetweenStartPositions = GetStartPosition1() - GetStartPosition0();
    AZ::Vector2 vectorBetweenCurrentPositions = GetCurrentPosition1() - GetCurrentPosition0();

    if (vectorBetweenStartPositions.IsZero() || vectorBetweenCurrentPositions.IsZero())
    {
        return 0.0f;
    }

    vectorBetweenStartPositions.Normalize();
    vectorBetweenCurrentPositions.Normalize();

    const float dotProduct = vectorBetweenStartPositions.Dot(vectorBetweenCurrentPositions);
    const float crossProduct = vectorBetweenStartPositions.GetX() * vectorBetweenCurrentPositions.GetY() - 
                               vectorBetweenStartPositions.GetY() * vectorBetweenCurrentPositions.GetX();
    return RAD2DEG(atan2(crossProduct, dotProduct));
}
