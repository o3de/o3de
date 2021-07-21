/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Viewport/ClickDetector.h>
#include <AzFramework/Viewport/ScreenGeometry.h>

namespace AzFramework
{
    ClickDetector::ClickOutcome ClickDetector::DetectClick(const ClickEvent clickEvent, const ScreenVector& cursorDelta)
    {
        const auto previousDetectionState = m_detectionState;
        if (previousDetectionState == DetectionState::WaitingForMove)
        {
            // only allow the action to begin if the mouse has been moved a small amount
            m_moveAccumulator += ScreenVectorLength(cursorDelta);
            if (m_moveAccumulator > m_deadZone)
            {
                m_detectionState = DetectionState::Moved;
            }
        }

        if (clickEvent == ClickEvent::Down)
        {
            const auto now = std::chrono::steady_clock::now();
            if (m_tryBeginTime)
            {
                const std::chrono::duration<float> diff = now - m_tryBeginTime.value();
                if (diff.count() < m_doubleClickInterval)
                {
                    return ClickOutcome::Nil;
                }
            }

            m_detectionState = DetectionState::WaitingForMove;
            m_moveAccumulator = 0.0f;

            m_tryBeginTime = now;
        }
        else if (clickEvent == ClickEvent::Up)
        {
            const auto clickOutcome = [detectionState = m_detectionState] {
                if (detectionState == DetectionState::WaitingForMove)
                {
                    return ClickOutcome::Click;
                }
                if (detectionState == DetectionState::Moved)
                {
                    return ClickOutcome::Release;
                }
                return ClickOutcome::Nil;
            }();

            m_detectionState = DetectionState::Nil;
            return clickOutcome;
        }

        if (previousDetectionState == DetectionState::WaitingForMove && m_detectionState == DetectionState::Moved)
        {
            return ClickOutcome::Move;
        }

        return ClickOutcome::Nil;
    }
} // namespace AzFramework
