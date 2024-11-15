/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/chrono/chrono.h>
#include <AzCore/std/functional.h>
#include <AzCore/std/optional.h>

namespace AzFramework
{
    //! Default value to use for detecting if the mouse has moved far enough after a mouse down to no longer
    //! register a click when a mouse up occurs.
    inline constexpr float DefaultMouseMoveDeadZone = 2.0f;

    struct ScreenVector;

    //! Utility class to help detect different types of mouse click (mouse down and up with
    //! no movement), mouse move (down and initial move after some threshold) and mouse release
    //! (mouse down with movement and then mouse up).
    class ClickDetector
    {
    public:
        ClickDetector();

        //! Internal representation of click event (map from external event for this when
        //! calling DetectClick).
        enum class ClickEvent
        {
            Nil,
            Down,
            Up
        };

        //! The type of mouse click.
        enum class ClickOutcome
        {
            Nil, //!< Not recognized.
            Move, //!< Initial move after mouse down.
            Click, //!< Mouse down and up with no intermediate movement.
            Release //!< Mouse down with movement and then mouse up.
        };

        //! Called from any type of 'handle event' function.
        ClickOutcome DetectClick(ClickEvent clickEvent, const ScreenVector& cursorDelta);

        //! Override the default double click interval.
        //! @note Default is 400ms - system default.
        void SetDoubleClickInterval(float doubleClickInterval);
        //! Override the dead zone before a 'move' outcome will be triggered.
        void SetDeadZone(float deadZone);
        //! Override how the current time is retrieved.
        //! This is helpful to override when it comes to simulating different passages of
        //! time to avoid double click issues in tests for example.
        void OverrideTimeNowFn(AZStd::function<AZStd::chrono::milliseconds()> timeNowFn);
        //! Customization point for when a potential click first begins.
        void SetClickDownEventFn(AZStd::function<void()> downEventFn);

    private:
        //! Internal state of ClickDetector based on incoming events.
        enum class DetectionState
        {
            Nil, //!< Initial state
            WaitingForMove, //! Mouse down has happened but mouse hasn't yet moved.
            Moved //! Mouse has moved, no longer will be counted as a click.
        };

        float m_moveAccumulator = 0.0f; //!< How far the mouse has moved after mouse down.
        float m_deadZone = DefaultMouseMoveDeadZone; //!< How far to move before a click is cancelled (when Move will fire).
        float m_doubleClickInterval = 0.4f; //!< Default double click interval, can be overridden.
        DetectionState m_detectionState = DetectionState::Nil; //!< Internal state of ClickDetector.
        //! Mouse down time (happens each mouse down, helps with double click handling).
        AZStd::optional<AZStd::chrono::milliseconds> m_tryBeginTime;
        AZStd::function<AZStd::chrono::milliseconds()> m_timeNowFn; //!< Interface to query the current time.
        AZStd::function<void()> m_downEventFn; //!< Callback to fire when an initial click down event happens.
    };

    inline void ClickDetector::SetDoubleClickInterval(const float doubleClickInterval)
    {
        m_doubleClickInterval = doubleClickInterval;
    }

    inline void ClickDetector::SetDeadZone(const float deadZone)
    {
        m_deadZone = deadZone;
    }

    inline void ClickDetector::SetClickDownEventFn(AZStd::function<void()> downEventFn)
    {
        m_downEventFn = AZStd::move(downEventFn);
    }
} // namespace AzFramework
