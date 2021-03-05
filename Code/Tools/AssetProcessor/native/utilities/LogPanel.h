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
#include <AzToolsFramework/UI/Logging/StyledTracePrintFLogPanel.h>

namespace AssetProcessor
{
    //! LogPanel - an implementation of TracePrintFLogPanel which shows recent traceprintfs
    //! CreateTabs will create a new instance of LogTab
    class LogPanel : public AzToolsFramework::LogPanel::StyledTracePrintFLogPanel
    {
    public:
        explicit LogPanel(QWidget* parent = nullptr);
        ~LogPanel() override = default;
    protected:
        QWidget* CreateTab(const AzToolsFramework::LogPanel::TabSettings& settings) override;
    };

    //! LogTab - a Log View listening on AZ Traceprintfs and puts them in a ring buffer
    //! It also filters traceprintfs based on the thread local job id
    class LogTab : public AzToolsFramework::LogPanel::StyledTracePrintFLogTab
    {
    public:
        AZ_CLASS_ALLOCATOR(LogTab, AZ::SystemAllocator, 0);
        explicit LogTab(const AzToolsFramework::LogPanel::TabSettings& settings, QWidget* parent = nullptr);
        ~LogTab() override = default;
        void AddInitialLogMessage();
        //////////////////////////////////////////////////////////////////////////
        // TraceMessagesBus
        bool OnAssert(const char* message) override;
        bool OnException(const char* message) override;
        bool OnError(const char* window, const char* message) override;
        bool OnWarning(const char* window, const char* message) override;
        bool OnPrintf(const char* window, const char* message) override;
        //////////////////////////////////////////////////////////////////////////
    };
}
