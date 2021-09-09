/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <CryCommon/ITimer.h>
#include <CryCommon/ISystem.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
inline void Gestures::RecognizerPinch::Config::Reflect(AZ::ReflectContext* context)
{
    if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
    {
        serialize->Class<Config>()
            ->Version(0)
            ->Field("minPixelsMoved", &Config::minPixelsMoved)
            ->Field("maxAngleDegrees", &Config::maxAngleDegrees)
            ->Field("priority", &Config::priority)
        ;

        if (AZ::EditContext* ec = serialize->GetEditContext())
        {
            ec->Class<Config>("Pinch Config", "Configuration values used to setup a gesture recognizer for pinches.")
                ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                ->DataElement(AZ::Edit::UIHandlers::Default, &Config::minPixelsMoved, "Min Pixels Moved", "The min distance in pixels that must be pinched before a pinch will be recognized.")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                ->DataElement(AZ::Edit::UIHandlers::Default, &Config::maxAngleDegrees, "Max Angle Degrees", "The max angle in degrees that a pinch can deviate before it will be recognized.")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
            ;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
inline Gestures::RecognizerPinch::RecognizerPinch(const Config& config)
    : m_config(config)
    , m_currentState(State::Idle)
{
    m_lastUpdateTimes[0] = 0;
    m_lastUpdateTimes[1] = 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
inline Gestures::RecognizerPinch::~RecognizerPinch()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
inline bool Gestures::RecognizerPinch::OnPressedEvent(const AZ::Vector2& screenPosition, uint32_t pointerIndex)
{
    if (pointerIndex > s_maxPinchPointerIndex)
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
    case State::Pinching:
    default:
    {
        // Should not be possible, but not fatal if we happen to get here somehow.
        AZ_Warning("RecognizerPinch", false, "RecognizerPinch::OnPressedEvent state logic failure");
    }
    break;
    }

    return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
inline float AngleInDegreesBetweenVectors(const AZ::Vector2& vec0, const AZ::Vector2& vec1)
{
    if (vec0.IsZero() || vec1.IsZero())
    {
        return 0.0f;
    }

    return RAD2DEG(acosf(abs(vec0.GetNormalized().Dot(vec1.GetNormalized()))));
}

////////////////////////////////////////////////////////////////////////////////////////////////////
inline bool Gestures::RecognizerPinch::OnDownEvent(const AZ::Vector2& screenPosition, uint32_t pointerIndex)
{
    if (pointerIndex > s_maxPinchPointerIndex)
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
        const AZ::Vector2 vectorBetweenStartPositions = GetStartPosition1() - GetStartPosition0();
        const AZ::Vector2 vectorBetweenCurrentPositions = GetCurrentPosition1() - GetCurrentPosition0();
        const float distanceDeltaPixels = abs(GetCurrentDistance() - GetStartDistance());

        if (AngleInDegreesBetweenVectors(vectorBetweenStartPositions, vectorBetweenCurrentPositions) > m_config.maxAngleDegrees)
        {
            // The touches are not moving towards or away from each other.
            // Reset the start positions so a pinch can still be initiated.
            m_startPositions[0] = m_currentPositions[0];
            m_startPositions[1] = m_currentPositions[1];
        }
        else if (distanceDeltaPixels >= m_config.minPixelsMoved)
        {
            // The touches have moved towards or away from each other a
            // sufficient distance for a pinch gesture to be initiated.
            m_startPositions[0] = m_currentPositions[0];
            m_startPositions[1] = m_currentPositions[1];
            OnContinuousGestureInitiated();
            m_currentState = State::Pinching;
        }
    }
    break;
    case State::Pinching:
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
        AZ_Warning("RecognizerPinch", false, "RecognizerPinch::OnDownEvent state logic failure");
    }
    break;
    }

    return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
inline bool Gestures::RecognizerPinch::OnReleasedEvent(const AZ::Vector2& screenPosition, uint32_t pointerIndex)
{
    if (pointerIndex > s_maxPinchPointerIndex)
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
            AZ_Warning("RecognizerPinch", false, "RecognizerPinch::OnReleasedEvent state logic failure");
            break;
        }

        // We never actually started pinching
        m_currentPositions[pointerIndex] = screenPosition;
        m_currentState = State::Idle;
    }
    break;
    case State::PressedBoth:
    {
        // We never actually started pinching
        m_currentPositions[pointerIndex] = screenPosition;
        m_currentState = pointerIndex ? State::Pressed0 : State::Pressed1;
    }
    break;
    case State::Pinching:
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
        AZ_Warning("RecognizerPinch", false, "RecognizerPinch::OnReleasedEvent state logic failure");
    }
    break;
    }

    return false;
}
