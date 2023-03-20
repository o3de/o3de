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

////////////////////////////////////////////////////////////////////////////////////////////////////
inline void Gestures::RecognizerSwipe::Config::Reflect(AZ::ReflectContext* context)
{
    if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
    {
        serialize->Class<Config>()
            ->Version(0)
            ->Field("maxSecondsHeld", &Config::maxSecondsHeld)
            ->Field("maxPixelsMoved", &Config::minPixelsMoved)
            ->Field("pointerIndex", &Config::pointerIndex)
            ->Field("priority", &Config::priority)
        ;

        if (AZ::EditContext* ec = serialize->GetEditContext())
        {
            ec->Class<Config>("Swipe Config", "Configuration values used to setup a gesture recognizer for swipes.")
                ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                ->DataElement(AZ::Edit::UIHandlers::SpinBox, &Config::pointerIndex, "Pointer Index", "The pointer (button or finger) index to track.")
                    ->Attribute(AZ::Edit::Attributes::Min, 0)
                    ->Attribute(AZ::Edit::Attributes::Max, 10)
                ->DataElement(AZ::Edit::UIHandlers::Default, &Config::maxSecondsHeld, "Max Seconds Held", "The max time in seconds after the initial press for a swipe to be recognized.")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                ->DataElement(AZ::Edit::UIHandlers::Default, &Config::minPixelsMoved, "Min Pixels Moved", "The min distance in pixels that must be moved before a swipe will be recognized.")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
            ;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
inline Gestures::RecognizerSwipe::RecognizerSwipe(const Config& config)
    : m_config(config)
    , m_startPosition()
    , m_endPosition()
    , m_startTime(AZ::Time::ZeroTimeMs)
    , m_endTime(AZ::Time::ZeroTimeMs)
    , m_currentState(State::Idle)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
inline Gestures::RecognizerSwipe::~RecognizerSwipe()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
inline bool Gestures::RecognizerSwipe::OnPressedEvent(const AZ::Vector2& screenPosition, uint32_t pointerIndex)
{
    if (pointerIndex != m_config.pointerIndex)
    {
        return false;
    }

    switch (m_currentState)
    {
    case State::Idle:
    {
        m_startTime = AZ::GetRealElapsedTimeMs();
        m_startPosition = screenPosition;
        m_endPosition = screenPosition;
        m_currentState = State::Pressed;
    }
    break;
    case State::Pressed:
    default:
    {
        // Should not be possible, but not fatal if we happen to get here somehow.
        AZ_Warning("RecognizerDrag", false, "RecognizerDrag::OnPressedEvent state logic failure");
    }
    break;
    }

    return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
inline bool Gestures::RecognizerSwipe::OnDownEvent([[maybe_unused]] const AZ::Vector2& screenPosition, uint32_t pointerIndex)
{
    if (pointerIndex != m_config.pointerIndex)
    {
        return false;
    }

    switch (m_currentState)
    {
    case State::Pressed:
    {
        const AZ::TimeMs currentTime = AZ::GetRealElapsedTimeMs();
        if (AZ::TimeMsToSeconds(currentTime - m_startTime) > m_config.maxSecondsHeld)
        {
            // Swipe recognition failed because we took too long.
            m_currentState = State::Idle;
        }
    }
    break;
    case State::Idle:
    {
        // Swipe recognition already failed above.
    }
    break;
    default:
    {
        // Should not be possible, but not fatal if we happen to get here somehow.
        AZ_Warning("RecognizerSwipe", false, "RecognizerSwipe::OnDownEvent state logic failure");
    }
    break;
    }

    return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
inline bool Gestures::RecognizerSwipe::OnReleasedEvent(const AZ::Vector2& screenPosition, uint32_t pointerIndex)
{
    if (pointerIndex != m_config.pointerIndex)
    {
        return false;
    }

    switch (m_currentState)
    {
    case State::Pressed:
    {
        const AZ::TimeMs currentTime = AZ::GetRealElapsedTimeMs();
        if ((AZ::TimeMsToSeconds(currentTime - m_startTime) <= m_config.maxSecondsHeld) &&
            (screenPosition.GetDistance(m_startPosition) >= m_config.minPixelsMoved))
        {
            // Swipe recognition succeeded.
            m_endTime = currentTime;
            m_endPosition = screenPosition;
            OnDiscreteGestureRecognized();
            m_currentState = State::Idle;
        }
        else
        {
            // Swipe recognition failed because we took too long or didn't move enough.
            m_currentState = State::Idle;
        }
    }
    break;
    case State::Idle:
    {
        // Swipe recognition already failed above.
    }
    break;
    default:
    {
        // Should not be possible, but not fatal if we happen to get here somehow.
        AZ_Warning("RecognizerSwipe", false, "RecognizerSwipe::OnReleasedEvent state logic failure");
    }
    break;
    }

    return false;
}
