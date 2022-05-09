/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Time/ITime.h>
#include <CryCommon/ISystem.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
inline void Gestures::RecognizerClickOrTap::Config::Reflect(AZ::ReflectContext* context)
{
    if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
    {
        serialize->Class<Config>()
            ->Version(0)
            ->Field("maxSecondsHeld", &Config::maxSecondsHeld)
            ->Field("maxPixelsMoved", &Config::maxPixelsMoved)
            ->Field("maxSecondsBetweenClicksOrTaps", &Config::maxSecondsBetweenClicksOrTaps)
            ->Field("maxPixelsBetweenClicksOrTaps", &Config::maxPixelsBetweenClicksOrTaps)
            ->Field("minClicksOrTaps", &Config::minClicksOrTaps)
            ->Field("pointerIndex", &Config::pointerIndex)
            ->Field("priority", &Config::priority)
        ;

        if (AZ::EditContext* ec = serialize->GetEditContext())
        {
            ec->Class<Config>("Click Or Tap Config", "Configuration values used to setup a gesture recognizer for clicks or taps.")
                ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                ->DataElement(AZ::Edit::UIHandlers::SpinBox, &Config::pointerIndex, "Pointer Index", "The pointer (button or finger) index to track.")
                    ->Attribute(AZ::Edit::Attributes::Min, 0)
                    ->Attribute(AZ::Edit::Attributes::Max, 10)
                ->DataElement(AZ::Edit::UIHandlers::Default, &Config::minClicksOrTaps, "Min Clicks Or Taps", "The min number of clicks or taps required for the gesture to be recognized.")
                    ->Attribute(AZ::Edit::Attributes::Min, 1)
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ_CRC("RefreshEntireTree", 0xefbc823c))
                ->DataElement(AZ::Edit::UIHandlers::Default, &Config::maxSecondsHeld, "Max Seconds Held", "The max time in seconds allowed while held before the gesture stops being recognized.")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                ->DataElement(AZ::Edit::UIHandlers::Default, &Config::maxPixelsMoved, "Max Pixels Moved", "The max distance in pixels allowed to move while held before the gesture stops being recognized.")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                ->DataElement(AZ::Edit::UIHandlers::Default, &Config::maxSecondsBetweenClicksOrTaps, "Max Seconds Between Clicks Or Taps", "The max time in seconds allowed between clicks or taps before the gesture stops being recognized.")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->Attribute(AZ::Edit::Attributes::Visibility, &Config::IsMultiClickOrTap)
                ->DataElement(AZ::Edit::UIHandlers::Default, &Config::maxPixelsBetweenClicksOrTaps, "Max Pixels Between Clicks Or Taps", "he max distance in pixels allowed between clicks or taps.")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->Attribute(AZ::Edit::Attributes::Visibility, &Config::IsMultiClickOrTap)
            ;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
inline Gestures::RecognizerClickOrTap::RecognizerClickOrTap(const Config& config)
    : m_config(config)
    , m_timeOfLastEvent(AZ::Time::ZeroTimeMs)
    , m_positionOfFirstEvent()
    , m_positionOfLastEvent()
    , m_currentCount(0)
    , m_currentState(State::Idle)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
inline Gestures::RecognizerClickOrTap::~RecognizerClickOrTap()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
inline bool Gestures::RecognizerClickOrTap::OnPressedEvent(const AZ::Vector2& screenPosition, uint32_t pointerIndex)
{
    if (pointerIndex != m_config.pointerIndex)
    {
        return false;
    }
    const AZ::TimeMs currentTime = AZ::GetRealElapsedTimeMs();
    switch (m_currentState)
    {
    case State::Idle:
    {
        m_timeOfLastEvent = currentTime;
        m_positionOfFirstEvent = screenPosition;
        m_positionOfLastEvent = screenPosition;
        m_currentCount = 0;
        m_currentState = State::Pressed;
    }
    break;
    case State::Released:
    {
        if ((AZ::TimeMsToSeconds(currentTime - m_timeOfLastEvent) > m_config.maxSecondsBetweenClicksOrTaps) ||
            (screenPosition.GetDistance(m_positionOfFirstEvent) > m_config.maxPixelsBetweenClicksOrTaps))
        {
            // Treat this as the start of a new tap sequence.
            m_currentCount = 0;
            m_positionOfFirstEvent = screenPosition;
        }

        m_timeOfLastEvent = currentTime;
        m_positionOfLastEvent = screenPosition;
        m_currentState = State::Pressed;
    }
    break;
    case State::Pressed:
    default:
    {
        // Should not be possible, but not fatal if we happen to get here somehow.
        AZ_Warning("RecognizerClickOrTap", false, "RecognizerClickOrTap::OnPressedEvent state logic failure");
    }
    break;
    }

    return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
inline bool Gestures::RecognizerClickOrTap::OnDownEvent(const AZ::Vector2& screenPosition, uint32_t pointerIndex)
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
        if ((AZ::TimeMsToSeconds(currentTime - m_timeOfLastEvent) > m_config.maxSecondsHeld) ||
            (screenPosition.GetDistance(m_positionOfLastEvent) > m_config.maxPixelsMoved))
        {
            // Tap recognition failed.
            m_currentCount = 0;
            m_currentState = State::Idle;
        }
    }
    break;
    case State::Idle:
    {
        // Tap recognition already failed above.
    }
    break;
    case State::Released:
    default:
    {
        // Should not be possible, but not fatal if we happen to get here somehow.
        AZ_Warning("RecognizerClickOrTap", false, "RecognizerClickOrTap::OnDownEvent state logic failure");
    }
    break;
    }

    return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
inline bool Gestures::RecognizerClickOrTap::OnReleasedEvent(const AZ::Vector2& screenPosition, uint32_t pointerIndex)
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
        if ((AZ::TimeMsToSeconds(currentTime - m_timeOfLastEvent) > m_config.maxSecondsHeld) ||
            (screenPosition.GetDistance(m_positionOfLastEvent) > m_config.maxPixelsMoved))
        {
            // Tap recognition failed.
            m_currentCount = 0;
            m_currentState = State::Idle;
        }
        else if (++m_currentCount >= m_config.minClicksOrTaps)
        {
            // Tap recognition succeeded.
            m_timeOfLastEvent = currentTime;
            m_positionOfLastEvent = screenPosition;
            OnDiscreteGestureRecognized();

            // Now reset to the default state.
            m_currentCount = 0;
            m_currentState = State::Idle;
        }
        else
        {
            // More taps are needed.
            m_timeOfLastEvent = currentTime;
            m_positionOfLastEvent = screenPosition;
            m_currentState = State::Released;
        }
    }
    break;
    case State::Idle:
    {
        // Tap recognition already failed above.
    }
    break;
    case State::Released:
    default:
    {
        // Should not be possible, but not fatal if we happen to get here somehow.
        AZ_Warning("RecognizerClickOrTap", false, "RecognizerClickOrTap::OnDownEvent state logic failure");
    }
    break;
    }

    return false;
}
