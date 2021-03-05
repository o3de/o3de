/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
inline void Gestures::RecognizerHold::Config::Reflect(AZ::ReflectContext* context)
{
    if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
    {
        serialize->Class<Config>()
            ->Version(0)
            ->Field("minSecondsHeld", &Config::minSecondsHeld)
            ->Field("maxPixelsMoved", &Config::maxPixelsMoved)
            ->Field("pointerIndex", &Config::pointerIndex)
            ->Field("priority", &Config::priority)
        ;

        if (AZ::EditContext* ec = serialize->GetEditContext())
        {
            ec->Class<Config>("Drag Hold", "Configuration values used to setup a gesture recognizer for holds.")
                ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                ->DataElement(AZ::Edit::UIHandlers::SpinBox, &Config::pointerIndex, "Pointer Index", "The pointer (button or finger) index to track.")
                    ->Attribute(AZ::Edit::Attributes::Min, 0)
                    ->Attribute(AZ::Edit::Attributes::Max, 10)
                ->DataElement(AZ::Edit::UIHandlers::Default, &Config::minSecondsHeld, "Min Seconds Held", "The min time in seconds after the initial press before a hold will be recognized.")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                ->DataElement(AZ::Edit::UIHandlers::Default, &Config::maxPixelsMoved, "Max Pixels Moved", "The max distance in pixels that can be moved before a hold stops being recognized.")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
            ;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
inline Gestures::RecognizerHold::RecognizerHold(const Config& config)
    : m_config(config)
    , m_startTime(0)
    , m_startPosition()
    , m_currentPosition()
    , m_currentState(State::Idle)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
inline Gestures::RecognizerHold::~RecognizerHold()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
inline bool Gestures::RecognizerHold::OnPressedEvent(const AZ::Vector2& screenPosition, uint32_t pointerIndex)
{
    if (pointerIndex != m_config.pointerIndex)
    {
        return false;
    }

    switch (m_currentState)
    {
    case State::Idle:
    {
        m_startTime = gEnv->pTimer->GetFrameStartTime().GetValue();
        m_startPosition = screenPosition;
        m_currentPosition = screenPosition;
        m_currentState = State::Pressed;
    }
    break;
    case State::Pressed:
    case State::Held:
    default:
    {
        // Should not be possible, but not fatal if we happen to get here somehow.
        AZ_Warning("RecognizerHold", false, "RecognizerHold::OnPressedEvent state logic failure");
    }
    break;
    }

    return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
inline bool Gestures::RecognizerHold::OnDownEvent(const AZ::Vector2& screenPosition, uint32_t pointerIndex)
{
    if (pointerIndex != m_config.pointerIndex)
    {
        return false;
    }

    m_currentPosition = screenPosition;

    switch (m_currentState)
    {
    case State::Pressed:
    {
        const CTimeValue currentTime = gEnv->pTimer->GetFrameStartTime();
        if (screenPosition.GetDistance(m_startPosition) > m_config.maxPixelsMoved)
        {
            // Hold recognition failed.
            m_currentState = State::Idle;
        }
        else if (currentTime.GetDifferenceInSeconds(m_startTime) >= m_config.minSecondsHeld)
        {
            // Hold recognition succeeded.
            OnContinuousGestureInitiated();
            m_currentState = State::Held;
        }
    }
    break;
    case State::Held:
    {
        if (screenPosition.GetDistance(m_startPosition) > m_config.maxPixelsMoved)
        {
            // Hold recognition ended.
            OnContinuousGestureEnded();
            m_currentState = State::Idle;
        }
        else
        {
            OnContinuousGestureUpdated();
        }
    }
    break;
    case State::Idle:
    {
        // Hold recognition already ended or failed above.
    }
    break;
    default:
    {
        // Should not be possible, but not fatal if we happen to get here somehow.
        AZ_Warning("RecognizerHold", false, "RecognizerHold::OnDownEvent state logic failure");
    }
    break;
    }

    return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
inline bool Gestures::RecognizerHold::OnReleasedEvent(const AZ::Vector2& screenPosition, uint32_t pointerIndex)
{
    if (pointerIndex != m_config.pointerIndex)
    {
        return false;
    }

    m_currentPosition = screenPosition;

    switch (m_currentState)
    {
    case State::Pressed:
    {
        // We never actually started the hold
        m_currentState = State::Idle;
    }
    break;
    case State::Held:
    {
        OnContinuousGestureEnded();
        m_currentState = State::Idle;
    }
    break;
    case State::Idle:
    {
        // Hold recognition already ended or failed above.
    }
    break;
    default:
    {
        // Should not be possible, but not fatal if we happen to get here somehow.
        AZ_Warning("RecognizerHold", false, "RecognizerHold::OnReleasedEvent state logic failure");
    }
    break;
    }

    return false;
}
