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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#ifndef CRYINCLUDE_CRYCOMMONTOOLS_UI_LOGWINDOW_H
#define CRYINCLUDE_CRYCOMMONTOOLS_UI_LOGWINDOW_H
#pragma once


#include "IUIComponent.h"
#include "Layout.h"
#include "ListView.h"
#include "ToggleButton.h"
#include <list>
#include "ILogger.h"

class LogWindow
    : public IUIComponent
{
public:
    LogWindow();
    ~LogWindow();

    void Log(ILogger::ESeverity eSeverity, const TCHAR* message);
    void SetFilter(ILogger::ESeverity eSeverity, bool visible);

    // IUIComponent
    virtual void CreateUI(void* window, int left, int top, int width, int height);
    virtual void Resize(void* window, int left, int top, int width, int height);
    virtual void DestroyUI(void* window);
    virtual void GetExtremeDimensions(void* window, int& minWidth, int& maxWidth, int& minHeight, int& maxHeight);

private:
    struct LogMessage
    {
        LogMessage(ILogger::ESeverity severity, tstring message)
            : severity(severity)
            , message(message)
        {
        }
        ILogger::ESeverity severity;
        tstring message;
    };

    void ErrorsToggled(bool value);
    void WarningsToggled(bool value);
    void InfoToggled(bool value);
    void DebugToggled(bool value);
    void RefillList();
    static int GetImageIndex(ILogger::ESeverity eSeverity);
    static int GetSeverityIndex(ILogger::ESeverity eSeverity);

    Layout m_mainLayout;
    Layout m_toolbarLayout;
    ListView m_list;
    std::vector<ToggleButton*> m_buttons;
    std::vector<LogMessage> m_messages;

    unsigned m_filterFlags;
};

#endif // CRYINCLUDE_CRYCOMMONTOOLS_UI_LOGWINDOW_H
