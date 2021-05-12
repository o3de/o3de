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

#include <AzFramework/Viewport/ClickDetector.h>
#include <AzFramework/Viewport/ScreenGeometry.h>

namespace AzFramework
{
    ClickDetector::ClickOutcome ClickDetector::DetectClick(const ClickEvent clickEvent, const ScreenVector& cursorDelta)
    {
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
            const auto clickOutcome = m_detectionState == DetectionState::WaitingForMove ? ClickOutcome::Click
                : m_detectionState == DetectionState::Moved                              ? ClickOutcome::Release
                                                                                         : ClickOutcome::Nil;
            m_detectionState = DetectionState::Nil;
            return clickOutcome;
        }

        if (m_detectionState == DetectionState::WaitingForMove)
        {
            // only allow the action to begin if the mouse has been moved a small amount
            m_moveAccumulator += ScreenVectorLength(cursorDelta);
            if (m_moveAccumulator > m_deadZone)
            {
                m_detectionState = DetectionState::Moved;
                return ClickOutcome::Move;
            }
        }

        return ClickOutcome::Nil;
    }
} // namespace AzFramework
