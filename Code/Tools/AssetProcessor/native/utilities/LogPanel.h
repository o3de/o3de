/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
        AZ_CLASS_ALLOCATOR(LogTab, AZ::SystemAllocator);
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
