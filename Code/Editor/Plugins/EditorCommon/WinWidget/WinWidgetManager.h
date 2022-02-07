/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <WinWidgetId.h>
#include "EditorCommonAPI.h"

#include <vector>
#include <functional>

class QWidget;

namespace WinWidget
{
    class EDITOR_COMMON_API WinWidgetManager
    {
    public:
        using WinWidgetCreateCall = std::function<QWidget*()>;

        WinWidgetManager();
        ~WinWidgetManager() {}

        bool RegisterWinWidget(WinWidgetId thisId, WinWidgetCreateCall createCall);
        bool UnregisterWinWidget(WinWidgetId thisId);

        QWidget* OpenWinWidget(WinWidgetId) const;
    private:
        WinWidgetCreateCall GetCreateCall(WinWidgetId thisId) const;
        size_t GetIndexForId(WinWidgetId thisId) const;

        AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
        std::vector<WinWidgetCreateCall> m_createCalls;
        AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING
    };
}
