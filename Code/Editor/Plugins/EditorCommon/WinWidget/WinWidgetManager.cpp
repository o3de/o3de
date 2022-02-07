/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <WinWidget/WinWidgetManager.h>

namespace WinWidget
{
    WinWidgetManager::WinWidgetManager()
        : m_createCalls(size_t(WinWidgetId::NUM_WIN_WIDGET_IDS) + 1)
    {
    }

    size_t WinWidgetManager::GetIndexForId(WinWidgetId thisId) const
    {
        size_t thisIndex = static_cast<size_t>(thisId);
        if (thisIndex >= m_createCalls.size())
        {
            return 0;
        }
        return thisIndex;
    }

    WinWidgetManager::WinWidgetCreateCall WinWidgetManager::GetCreateCall(WinWidgetId thisId) const
    {
        size_t thisIndex = GetIndexForId(thisId);
        if (!thisIndex)
        {
            return nullptr;
        }
        return m_createCalls[thisIndex];
    }

    bool WinWidgetManager::RegisterWinWidget(WinWidgetId thisId, WinWidgetCreateCall createCall)
    {
        size_t thisIndex = GetIndexForId(thisId);
        if (m_createCalls[thisIndex] != nullptr)
        {
            return false;
        }
        m_createCalls[thisIndex] = createCall;
        return true;
    }

    bool WinWidgetManager::UnregisterWinWidget(WinWidgetId thisId)
    {
        size_t thisIndex = GetIndexForId(thisId);
        if (m_createCalls[thisIndex] == nullptr)
        {
            return false;
        }
        m_createCalls[thisIndex] = nullptr;
        return true;
    }


    QWidget* WinWidgetManager::OpenWinWidget(WinWidgetId createId) const
    {
        WinWidgetManager::WinWidgetCreateCall createCall = GetCreateCall(createId);
        if (!createCall)
        {
            return nullptr;
        }
        return createCall();
    }
}
