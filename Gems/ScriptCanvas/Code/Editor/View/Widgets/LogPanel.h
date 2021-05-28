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

#if !defined(Q_MOC_RUN)
#include <AzCore/std/smart_ptr/unique_ptr.h>

#include <AzToolsFramework/UI/Logging/LogPanel_Panel.h>
#include <AzToolsFramework/UI/Logging/LogControl.h>
#include <AzToolsFramework/UI/UICore/TargetSelectorButton.hxx>

#include <AzQtComponents/Components/StyledDockWidget.h>

#include <Debugger/Bus.h>

#include <ScriptCanvas/Core/NodeBus.h>
#include <ScriptCanvas/Bus/GraphBus.h>
#endif

namespace AzQtComponents
{
    class TabWidget;
}

namespace Ui
{
    class LogPanel;
}

namespace ScriptCanvasEditor
{
    namespace Widget
    {
        class LogPanel
            : public AzToolsFramework::LogPanel::BaseLogPanel
            , ScriptCanvasEditor::GeneralGraphEventBus::Handler
        {
            Q_OBJECT

        public:

            LogPanel(QWidget* parent = nullptr);
            ~LogPanel() override;

        protected:

            void OnBuildGameEntity(const AZStd::string&, const AZ::EntityId& editGraphId, const ScriptCanvas::ScriptCanvasId& scriptCanvasId) override;

            QWidget* CreateTab(const AzToolsFramework::LogPanel::TabSettings& settings) override;

            ScriptCanvas::ScriptCanvasId m_scriptCanvasId;

            AzQtComponents::TabWidget* pTabWidget;
        };

        class LogPanelWidget 
            : public AzQtComponents::StyledDockWidget
        {
            Q_OBJECT

        public:

            LogPanelWidget(QWidget* parent = nullptr);
            AZStd::unique_ptr<Ui::LogPanel> ui;

        };

        class LogTab
            : public AzToolsFramework::LogPanel::BaseLogView
            , ScriptCanvas::LogNotificationBus::Handler
        {
            Q_OBJECT;
        public:
            AZ_CLASS_ALLOCATOR(LogTab, AZ::SystemAllocator, 0);
            LogTab(QWidget* pParent, const ScriptCanvas::ScriptCanvasId& scriptCanvasId, const AzToolsFramework::LogPanel::TabSettings& in_settings);
            ~LogTab() override;

            void LogMessage(const AZStd::string& message) override;

        private:

            AZStd::queue<AzToolsFramework::Logging::LogLine> m_bufferedLines;
            AZStd::atomic_bool m_alreadyQueuedDrainMessage;  // we also only drain the queue at the end so that we do bulk inserts instead of one at a time.

            void CommitAddedLines();
            bool m_alreadyQueuedCommit = false;

        private Q_SLOTS:
            void    DrainMessages();
            virtual void Clear();
        };

    }

}
