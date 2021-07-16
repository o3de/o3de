/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "LogPanel.h"

#include <QDateTime>
#include <QTimer>
#include <QTableView>

#include "Editor/View/Widgets/ui_LogPanel.h"
#include <Editor/View/Dialogs/SettingsDialog.h>

#include <AzQtComponents/Components/Widgets/TabWidget.h>

namespace ScriptCanvasEditor
{
    namespace Widget
    {
        LogPanel::LogPanel(QWidget* parent /*= nullptr*/)
            : AzToolsFramework::LogPanel::BaseLogPanel(parent)
        {
            ScriptCanvasEditor::GeneralGraphEventBus::Handler::BusConnect();
        }

        LogPanel::~LogPanel()
        {
            ScriptCanvasEditor::GeneralGraphEventBus::Handler::BusDisconnect();
        }

        void LogPanel::OnBuildGameEntity(const AZStd::string& name, const AZ::EntityId& editGraphId, const ScriptCanvas::ScriptCanvasId& scriptCanvasId)
        {
            m_scriptCanvasId = scriptCanvasId;

            AZStd::intrusive_ptr<Settings> settings = AZ::UserSettings::CreateFind<Settings>(AZ::Crc32(editGraphId.ToString().c_str()), AZ::UserSettings::CT_LOCAL);
            if (settings->m_enableLogging)
            {
                AzToolsFramework::LogPanel::TabSettings settingsTab(name.c_str(), "Script Canvas", "", true, true, true, true);
                AddLogTab(settingsTab);
            }
        }

        QWidget* LogPanel::CreateTab(const AzToolsFramework::LogPanel::TabSettings& settings)
        {
            return new LogTab(this, m_scriptCanvasId, settings);
        }

        LogPanelWidget::LogPanelWidget(QWidget* parent)
            : AzQtComponents::StyledDockWidget(parent)
            , ui(new Ui::LogPanel())
        {
            ui->setupUi(this);
            setWindowTitle(tr("Log"));
            setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
            setMinimumWidth(200);
            setMinimumHeight(40);

            ui->layout->addWidget(new LogPanel(this));

        }

        LogTab::LogTab(QWidget* pParent, const ScriptCanvas::ScriptCanvasId& scriptCanvasId, [[maybe_unused]] const AzToolsFramework::LogPanel::TabSettings& in_settings)
            : AzToolsFramework::LogPanel::BaseLogView(pParent)
        {

            QAction* actionClear = new QAction(tr("Clear"), this);
            connect(actionClear, SIGNAL(triggered()), this, SLOT(Clear()));
            addAction(actionClear);

            m_alreadyQueuedDrainMessage = false;

            ConnectModelToView(new AzToolsFramework::LogPanel::RingBufferLogDataModel(m_ptrLogView));

            ScriptCanvas::LogNotificationBus::Handler::BusConnect(scriptCanvasId);
            Clear();
        }

        LogTab::~LogTab()
        {
            ScriptCanvas::LogNotificationBus::Handler::BusDisconnect();
        }

        void LogTab::LogMessage(const AZStd::string& message)
        {
            AzToolsFramework::Logging::LogLine line(AZStd::string::format("%s", message.c_str()).c_str(), "Log", AzToolsFramework::Logging::LogLine::TYPE_MESSAGE, QDateTime::currentMSecsSinceEpoch());

            m_bufferedLines.push(line);
            CommitAddedLines();
        }

        static int s_delayBetweenTraceprintfUpdates = 250; // milliseconds between pumping the traceprintf messages, lower will eat more performance but be more responsive

        void LogTab::CommitAddedLines()
        {
            bool wasQueued = m_alreadyQueuedDrainMessage.exchange(true, AZStd::memory_order_acq_rel);
            if (!wasQueued)
            {   
                QTimer::singleShot(s_delayBetweenTraceprintfUpdates, this, &LogTab::DrainMessages);
            }
        }

        void LogTab::Clear()
        {
            AzToolsFramework::LogPanel::RingBufferLogDataModel* pModel = qobject_cast<AzToolsFramework::LogPanel::RingBufferLogDataModel*>(m_ptrLogView->model());
            if (pModel)
            {
                pModel->Clear();
            }
        }

        void LogTab::DrainMessages()
        {
            m_alreadyQueuedDrainMessage = false;

            bool wasAtMaxScroll = IsAtMaxScroll();
            AzToolsFramework::LogPanel::RingBufferLogDataModel* pModel = ((AzToolsFramework::LogPanel::RingBufferLogDataModel*)(m_ptrLogView->model()));
            AzToolsFramework::Logging::LogLine currentLine;
            bool openedQuery = false;
            bool foundLine = false;
            do
            {
                {
                    if (m_bufferedLines.empty())
                    {
                        foundLine = false;
                    }
                    else
                    {
                        foundLine = true;
                        currentLine = AZStd::move(m_bufferedLines.front());
                        m_bufferedLines.pop();
                    }
                }

                if (foundLine)
                {
                    if (!openedQuery)
                    {
                        openedQuery = true;
                    }

                    pModel->AppendLine(currentLine);
                }
            } while (foundLine); // keep doing this as long as there's line in the buffer.

            if (openedQuery)
            {
                pModel->CommitAdd();
                if (wasAtMaxScroll)
                {
                    m_ptrLogView->scrollToBottom();
                }
            }
        }

#include <Editor/View/Widgets/moc_LogPanel.cpp>

    }
}
