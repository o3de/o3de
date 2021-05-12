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

#pragma once

#include <AzFramework/Viewport/ScreenGeometry.h>

#include <AzCore/std/optional.h>

namespace AzFramework
{
    //! Utility type to wrap a current and last cursor position.
    struct CursorState
    {
        //! Returns the delta between the current and last cursor position.
        ScreenVector CursorDelta() const;
        //! Call this in a 'handle event' call to update the most recent cursor position.
        void SetCurrentPosition(const ScreenPoint& currentPosition);
        //! Call this in an 'update' call to move the current cursor position to the last
        //! cursor position.
        void Update();

    private:
        AZStd::optional<ScreenPoint> m_lastCursorPosition;
        AZStd::optional<ScreenPoint> m_currentCursorPosition;
    };

    inline void CursorState::SetCurrentPosition(const ScreenPoint& currentPosition)
    {
        m_currentCursorPosition = currentPosition;
    }

    inline ScreenVector CursorState::CursorDelta() const
    {
        return m_currentCursorPosition.has_value() && m_lastCursorPosition.has_value()
            ? m_currentCursorPosition.value() - m_lastCursorPosition.value()
            : ScreenVector(0, 0);
    }

    inline void CursorState::Update()
    {
        if (m_currentCursorPosition.has_value())
        {
            m_lastCursorPosition = m_currentCursorPosition;
        }
    }
} // namespace AzFramework
