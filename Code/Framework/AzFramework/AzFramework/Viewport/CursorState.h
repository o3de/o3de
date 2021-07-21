/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
        [[nodiscard]] ScreenVector CursorDelta() const;
        //! Call this in a 'handle event' call to update the most recent cursor position.
        void SetCurrentPosition(const ScreenPoint& currentPosition);
        //! Call this in an 'update' call to copy the current cursor position to the last
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
