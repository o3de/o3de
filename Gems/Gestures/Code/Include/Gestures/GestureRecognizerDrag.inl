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
inline void Gestures::RecognizerDrag::Config::Reflect(AZ::ReflectContext* context)
{
    if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
    {
        serialize->Class<Config>()
            ->Version(0)
            ->Field("minSecondsHeld", &Config::minSecondsHeld)
            ->Field("minPixelsMoved", &Config::minPixelsMoved)
            ->Field("pointerIndex", &Config::pointerIndex)
            ->Field("priority", &Config::priority)
        ;

        if (AZ::EditContext* ec = serialize->GetEditContext())
        {
            ec->Class<Config>("Drag Config", "Configuration values used to setup a gesture recognizer for drags.")
                ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                ->DataElement(AZ::Edit::UIHandlers::SpinBox, &Config::pointerIndex, "Pointer Index", "The pointer (button or finger) index to track.")
                    ->Attribute(AZ::Edit::Attributes::Min, 0)
                    ->Attribute(AZ::Edit::Attributes::Max, 10)
                ->DataElement(AZ::Edit::UIHandlers::Default, &Config::minSecondsHeld, "Min Seconds Held", "The min time in seconds after the initial press before a drag will be recognized.")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                ->DataElement(AZ::Edit::UIHandlers::Default, &Config::minPixelsMoved, "Min Pixels Moved", "The min distance in pixels that must be dragged before a drag will be recognized.")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
            ;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
inline Gestures::RecognizerDrag::RecognizerDrag(const Config& config)
    : m_config(config)
    , m_startTime(0)
    , m_startPosition()
    , m_currentPosition()
    , m_currentState(State::Idle)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
inline Gestures::RecognizerDrag::~RecognizerDrag()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
inline bool Gestures::RecognizerDrag::OnPressedEvent(const AZ::Vector2& screenPosition, uint32_t pointerIndex)
{
    if (pointerIndex != m_config.pointerIndex)
    {
        return false;
    }

    switch (m_currentState)
    {
    case State::Idle:
    {
        m_startTime = (gEnv && gEnv->pTimer) ? gEnv->pTimer->GetFrameStartTime().GetValue() : 0;
        m_startPosition = screenPosition;
        m_currentPosition = screenPosition;
        m_currentState = State::Pressed;
    }
    break;
    case State::Pressed:
    case State::Dragging:
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
inline bool Gestures::RecognizerDrag::OnDownEvent(const AZ::Vector2& screenPosition, uint32_t pointerIndex)
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
        const CTimeValue currentTime = (gEnv && gEnv->pTimer) ? gEnv->pTimer->GetFrameStartTime() : CTimeValue();
        if ((currentTime.GetDifferenceInSeconds(m_startTime) >= m_config.minSecondsHeld) &&
            (GetDistance() >= m_config.minPixelsMoved))
        {
            m_startTime = currentTime.GetValue();
            m_startPosition = m_currentPosition;
            OnContinuousGestureInitiated();
            m_currentState = State::Dragging;
        }
    }
    break;
    case State::Dragging:
    {
        OnContinuousGestureUpdated();
    }
    break;
    case State::Idle:
    default:
    {
        // Should not be possible, but not fatal if we happen to get here somehow.
        AZ_Warning("RecognizerDrag", false, "RecognizerDrag::OnDownEvent state logic failure");
    }
    break;
    }

    return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
inline bool Gestures::RecognizerDrag::OnReleasedEvent(const AZ::Vector2& screenPosition, uint32_t pointerIndex)
{
    if (pointerIndex != m_config.pointerIndex)
    {
        return false;
    }

    switch (m_currentState)
    {
    case State::Pressed:
    {
        // We never actually started dragging
        m_currentPosition = screenPosition;
        m_currentState = State::Idle;
    }
    break;
    case State::Dragging:
    {
        m_currentPosition = screenPosition;
        OnContinuousGestureEnded();
        m_currentState = State::Idle;
    }
    break;
    case State::Idle:
    default:
    {
        // Should not be possible, but not fatal if we happen to get here somehow.
        AZ_Warning("RecognizerDrag", false, "RecognizerDrag::OnReleasedEvent state logic failure");
    }
    break;
    }

    return false;
}
