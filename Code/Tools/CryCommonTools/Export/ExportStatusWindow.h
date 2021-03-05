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

#ifndef CRYINCLUDE_CRYCOMMONTOOLS_EXPORT_EXPORTSTATUSWINDOW_H
#define CRYINCLUDE_CRYCOMMONTOOLS_EXPORT_EXPORTSTATUSWINDOW_H
#pragma once


#include "UI/FrameWindow.h"
#include "UI/ProgressBar.h"
#include "UI/TaskList.h"
#include "UI/LogWindow.h"
#include "UI/Spacer.h"
#include "UI/Layout.h"
#include "UI/PushButton.h"
#include "ILogger.h"

class ExportStatusWindow
{
public:
    enum WaitState
    {
        WaitState_WarningsAndErrors,
        WaitState_ErrorsOnly,
        WaitState_Always,
        WaitState_Never,
    };

    ExportStatusWindow(int width, int height, const std::vector<std::pair<std::string, std::string> >& tasks);
    ~ExportStatusWindow();

    void SetWaitState(WaitState state);

    void AddTask(const std::string& id, const std::string& description);
    void SetCurrentTask(const std::string& id);
    void SetProgress(float progress);
    void Log(ILogger::ESeverity eSeverity, const char* message);

private:
    void Initialize(int width, int height, const std::vector<std::pair<std::string, std::string> >& tasks);
    void Run();
    void OkPressed();

    FrameWindow m_frameWindow;
    TaskList m_taskList;
    ProgressBar m_progressBar;
    Spacer m_okButtonSpacer;
    PushButton m_okButton;
    Layout m_okButtonLayout;
    LogWindow m_logWindow;
    void* m_threadHandle;
    bool m_warningsEncountered;
    bool m_errorsEncountered;
    WaitState m_waitState;
};

#endif // CRYINCLUDE_CRYCOMMONTOOLS_EXPORT_EXPORTSTATUSWINDOW_H
