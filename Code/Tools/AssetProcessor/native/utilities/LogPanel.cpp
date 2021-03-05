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

#include "LogPanel.h"
#include <native/utilities/ThreadHelper.h>

namespace AssetProcessor
{

    LogPanel::LogPanel(QWidget* parent)
        : AzToolsFramework::LogPanel::StyledTracePrintFLogPanel(parent)
    {
    }

    QWidget* LogPanel::CreateTab(const AzToolsFramework::LogPanel::TabSettings& settings)
    {
        LogTab* logTab = aznew LogTab(settings, this);
        logTab->AddInitialLogMessage();
        return logTab;
    }

    LogTab::LogTab(const AzToolsFramework::LogPanel::TabSettings& settings, QWidget* parent)
        : AzToolsFramework::LogPanel::StyledTracePrintFLogTab(settings, parent)
    {
    }

    void LogTab::AddInitialLogMessage()
    {
        LogTraceMessage(AzToolsFramework::Logging::LogLine::TYPE_MESSAGE, "AssetProcessor", "Started recording logs. To check previous logs please navigate to the logs folder.", true);
    }

    bool LogTab::OnAssert(const char* message)
    {
        if (AssetProcessor::GetThreadLocalJobId())
        {
            return false; // we are in a job thread
        }

        return AzToolsFramework::LogPanel::StyledTracePrintFLogTab::OnAssert(message);
    }
    bool LogTab::OnException(const char* message)
    {
        if (AssetProcessor::GetThreadLocalJobId())
        {
            return false; // we are in a job thread
        }

        return AzToolsFramework::LogPanel::StyledTracePrintFLogTab::OnException(message);
    }

    bool LogTab::OnPrintf(const char* window, const char* message)
    {
        if (AssetProcessor::GetThreadLocalJobId())
        {
            return false; // we are in a job thread
        }

        return AzToolsFramework::LogPanel::StyledTracePrintFLogTab::OnPrintf(window, message);
    }

    bool LogTab::OnError(const char* window, const char* message)
    {
        if (AssetProcessor::GetThreadLocalJobId())
        {
            return false; // we are in a job thread
        }

        return AzToolsFramework::LogPanel::StyledTracePrintFLogTab::OnError(window, message);
    }

    bool LogTab::OnWarning(const char* window, const char* message)
    {
        if (AssetProcessor::GetThreadLocalJobId())
        {
            return false; // we are in a job thread
        }

        return AzToolsFramework::LogPanel::StyledTracePrintFLogTab::OnWarning(window, message);
    }

}
