/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <ctime>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzQtComponents/Components/StyledBusyLabel.h>
#include <AzQtComponents/Components/StyledDetailsTableView.h>
#include <AzToolsFramework/Debug/TraceContextLogFormatter.h>
#include <AzToolsFramework/UI/Logging/LogEntry.h>
#include <CommonWidgets/ui_ProcessingOverlayWidget.h>
#include <SceneAPI/SceneUI/CommonWidgets/ProcessingOverlayWidget.h>
#include <SceneAPI/SceneUI/CommonWidgets/OverlayWidget.h>
#include <SceneAPI/SceneUI/Handlers/ProcessingHandlers/ProcessingHandler.h>
#include <SceneAPI/SceneUI/CommonWidgets/OverlayWidget.h>
#include <QCloseEvent>
#include <QDateTime>
#include <QLabel>
#include <QTimer>
#include <QHeaderView>

namespace AZ
{
    namespace SceneAPI
    {
        namespace SceneUI
        {
            namespace Internal
            {
                QtWebEngineMessageFilter::QtWebEngineMessageFilter(QObject* parent)
                    : QSortFilterProxyModel(parent)
                {
                }

                QtWebEngineMessageFilter::~QtWebEngineMessageFilter()
                {
                }

                bool QtWebEngineMessageFilter::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
                {
                    auto tableModel = qobject_cast<AzQtComponents::StyledDetailsTableModel*>(sourceModel());
                    if (!tableModel)
                    {
                        return QSortFilterProxyModel::filterAcceptsRow(sourceRow, sourceParent);
                    }

                    const int sourceColumn = tableModel->GetColumnIndex(QStringLiteral("message"));
                    const QModelIndex index = tableModel->index(sourceRow, sourceColumn, sourceParent);
                    const QVariant data = tableModel->data(index);

                    static const QString filteredMessage = QStringLiteral("Qt WebEngine seems to be initialized from a plugin. Please set Qt::AA_ShareOpenGLContexts using QCoreApplication::setAttribute before constructing QGuiApplication.");
                    if (data.toString() == filteredMessage)
                    {
                        return false;
                    }

                    return QSortFilterProxyModel::filterAcceptsRow(sourceRow, sourceParent);
                }
            }

            ProcessingOverlayWidget::ProcessingOverlayWidget(UI::OverlayWidget* overlay, Layout layout, Uuid traceTag)
                : QWidget()
                , m_traceTag(traceTag)
                , ui(new Ui::ProcessingOverlayWidget())
                , m_overlay(overlay)
                , m_progressLabel(nullptr)
                , m_layerId(UI::OverlayWidget::s_invalidOverlayIndex)
                , m_isProcessingComplete(false)
                , m_isClosingBlocked(false)
                , m_autoCloseOnSuccess(false)
                , m_encounteredIssues(false)
                , m_resizeTimer(new QTimer(this))
            {
                ui->setupUi(this);
                
                m_busyLabel = new AzQtComponents::StyledBusyLabel();
                m_busyLabel->SetIsBusy(true);
                m_busyLabel->SetBusyIconSize(14);
                ui->m_header->addWidget(m_busyLabel);

                m_reportModel = new AzQtComponents::StyledDetailsTableModel();
                m_reportModel->AddColumn("Status", AzQtComponents::StyledDetailsTableModel::StatusIcon);
                if (layout == Layout::Exporting)
                {
                    m_reportModel->AddColumn("Platform");
                }
                m_reportModel->AddColumn("Message");
                m_reportModel->AddColumnAlias("message", "Message");

                auto messageFilterModel = new Internal::QtWebEngineMessageFilter(this);
                messageFilterModel->setSourceModel(m_reportModel);
                
                m_reportView = new AzQtComponents::StyledDetailsTableView();
                m_reportView->setModel(messageFilterModel);
                ui->m_reportArea->addWidget(m_reportView);

                UpdateColumnSizes();
                
                connect(m_overlay, &UI::OverlayWidget::LayerRemoved, this, &ProcessingOverlayWidget::OnLayerRemoved);

                BusConnect();

                m_resizeTimer->setSingleShot(true);
                m_resizeTimer->setInterval(0);
                connect(m_resizeTimer, &QTimer::timeout, this, &ProcessingOverlayWidget::UpdateColumnSizes);
            }

            ProcessingOverlayWidget::~ProcessingOverlayWidget()
            {
                BusDisconnect();
            }

            bool ProcessingOverlayWidget::OnPrintf(const char* window, const char* message)
            {
                if (ShouldProcessMessage())
                {
                    AzQtComponents::StyledDetailsTableModel::TableEntry entry;
                    if (AzFramework::StringFunc::Find(window, "Success") != AZStd::string::npos)
                    {
                        entry.Add("Status", AzQtComponents::StyledDetailsTableModel::StatusSuccess);
                    }
                    else if (AzFramework::StringFunc::Find(window, "Warning") != AZStd::string::npos)
                    {
                        entry.Add("Status", AzQtComponents::StyledDetailsTableModel::StatusWarning);
                        m_encounteredIssues = true;
                    }
                    else if (AzFramework::StringFunc::Find(window, "Error") != AZStd::string::npos)
                    {
                        entry.Add("Status", AzQtComponents::StyledDetailsTableModel::StatusError);
                        m_encounteredIssues = true;
                    }
                    else
                    {
                        // To reduce noise in the report widget, only show success, warning and error messages.
                        return false;
                    }
                    
                    entry.Add("Message", message);
                    CopyTraceContext(entry);
                    m_reportModel->AddEntry(entry);
                }
                return false;
            }

            bool ProcessingOverlayWidget::OnError(const char* /*window*/, const char* message)
            {
                if (ShouldProcessMessage())
                {
                    AzQtComponents::StyledDetailsTableModel::TableEntry entry;
                    entry.Add("Message", message);
                    entry.Add("Status", AzQtComponents::StyledDetailsTableModel::StatusError);
                    CopyTraceContext(entry);
                    m_reportModel->AddEntry(entry);
                    m_encounteredIssues = true;
                    return true;
                }
                return false;
            }

            bool ProcessingOverlayWidget::OnWarning(const char* /*window*/, const char* message)
            {
                if (ShouldProcessMessage())
                {
                    AzQtComponents::StyledDetailsTableModel::TableEntry entry;
                    entry.Add("Message", message);
                    entry.Add("Status", AzQtComponents::StyledDetailsTableModel::StatusWarning);
                    CopyTraceContext(entry);
                    m_reportModel->AddEntry(entry);
                    m_encounteredIssues = true;
                    return true;
                }
                return false;
            }

            bool ProcessingOverlayWidget::OnAssert(const char* message)
            {
                if (ShouldProcessMessage())
                {
                    AzQtComponents::StyledDetailsTableModel::TableEntry entry;
                    entry.Add("Message", message);
                    entry.Add("Status", AzQtComponents::StyledDetailsTableModel::StatusError);
                    CopyTraceContext(entry);
                    m_reportModel->AddEntry(entry);
                    m_encounteredIssues = true;
                    // Don't return true here as assert should pop a window.
                }
                return false;
            }

            void ProcessingOverlayWidget::OnLayerRemoved(int layerId)
            {
                if (layerId == m_layerId)
                {
                    delete m_progressLabel;
                    m_progressLabel = nullptr;

                    layerId = UI::OverlayWidget::s_invalidOverlayIndex;
                    emit Closing();
                }
            }

            int ProcessingOverlayWidget::PushToOverlay()
            {
                AZ_Assert(m_layerId == UI::OverlayWidget::s_invalidOverlayIndex, "Processing overlay widget already pushed.");
                if (m_layerId != UI::OverlayWidget::s_invalidOverlayIndex)
                {
                    return m_layerId;
                }

                UI::OverlayWidgetButtonList buttons;

                UI::OverlayWidgetButton button;
                button.m_text = "Ok";
                button.m_triggersPop = true;
                button.m_isCloseButton = true;
                button.m_enabledCheck = [this]() -> bool
                {
                    return CanClose();
                };
                buttons.push_back(&button);

                m_progressLabel = new QLabel("Processing...");
                m_progressLabel->setAlignment(Qt::AlignCenter);
                m_layerId = m_overlay->PushLayer(m_progressLabel, this, "File progress", buttons);
                return m_layerId;
            }

            bool ProcessingOverlayWidget::GetAutoCloseOnSuccess() const
            {
                return m_autoCloseOnSuccess;
            }

            void ProcessingOverlayWidget::SetAutoCloseOnSuccess(bool closeOnComplete)
            {
                m_autoCloseOnSuccess = closeOnComplete;
            }

            bool ProcessingOverlayWidget::HasProcessingCompleted() const
            {
                return m_isProcessingComplete;
            }

            void ProcessingOverlayWidget::SetAndStartProcessingHandler(const AZStd::shared_ptr<ProcessingHandler>& handler)
            {
                AZ_Assert(handler, "Processing handler was null");
                AZ_Assert(!m_targetHandler, "A handler has already been assigned. Only one can be active per layer at any given time.");
                if (m_targetHandler)
                {
                    return;
                }

                m_targetHandler = handler;

                connect(m_targetHandler.get(), &ProcessingHandler::StatusMessageUpdated, this, &ProcessingOverlayWidget::OnSetStatusMessage);
                connect(m_targetHandler.get(), &ProcessingHandler::AddLogEntry, this, &ProcessingOverlayWidget::AddLogEntry);
                connect(m_targetHandler.get(), &ProcessingHandler::ProcessingComplete, this, &ProcessingOverlayWidget::OnProcessingComplete);

                handler->BeginProcessing();
            }

            AZStd::shared_ptr<ProcessingHandler> ProcessingOverlayWidget::GetProcessingHandler() const
            {
                return m_targetHandler;
            }

            void ProcessingOverlayWidget::BlockClosing()
            {
                m_isClosingBlocked = true;
            }

            void ProcessingOverlayWidget::UnblockClosing()
            {
                m_isClosingBlocked = false;
                SetUIToCompleteState();
            }

            void ProcessingOverlayWidget::AddLogEntry(const AzToolsFramework::Logging::LogEntry& entry)
            {
                if (entry.GetSeverity() == AzToolsFramework::Logging::LogEntry::Severity::Message)
                {
                    return;
                }

                m_encounteredIssues = true;

                bool hasStatus = false;
                AzQtComponents::StyledDetailsTableModel::TableEntry reportEntry;
                for (auto& field : entry.GetFields())
                {
                    size_t offset = 0;
                    hasStatus = hasStatus || AzFramework::StringFunc::Equal("status", field.second.m_name.c_str());
                    if (AzFramework::StringFunc::Equal("message", field.second.m_name.c_str()))
                    {
                        if (field.second.m_value.length() > 2)
                        {
                            // Removing the prefixes such as "W: " and "E: ".
                            if (field.second.m_value[1] == ':' && field.second.m_value[2] == ' ')
                            {
                                offset = 3;
                            }
                        }
                    }
                    reportEntry.Add(field.second.m_name.c_str(), field.second.m_value.c_str() + offset);
                }

                if (!hasStatus)
                {
                    if (entry.GetSeverity() == AzToolsFramework::Logging::LogEntry::Severity::Error)
                    {
                        reportEntry.Add("Status", AzQtComponents::StyledDetailsTableModel::StatusError);
                    }
                    else if (entry.GetSeverity() == AzToolsFramework::Logging::LogEntry::Severity::Warning)
                    {
                        reportEntry.Add("Status", AzQtComponents::StyledDetailsTableModel::StatusWarning);
                    }
                }

                time_t time = QDateTime::fromMSecsSinceEpoch(entry.GetRecordedTime()).toTime_t();
                struct tm timeInfo;
#if defined(AZ_PLATFORM_WINDOWS)
                localtime_s(&timeInfo, &time);
#else
                localtime_r(&time, &timeInfo);
#endif

                char buffer[128];
                std::strftime(buffer, sizeof(buffer), "%H:%M:%S", &timeInfo);
                reportEntry.Add("Time", buffer);
                std::strftime(buffer, sizeof(buffer), "%A, %B %d, %Y", &timeInfo);
                reportEntry.Add("Date", buffer);

                m_reportModel->AddEntry(reportEntry);

                m_resizeTimer->start();
            }

            void ProcessingOverlayWidget::OnProcessingComplete()
            {
                m_isProcessingComplete = true;
                SetUIToCompleteState();

                if (!m_encounteredIssues && m_autoCloseOnSuccess)
                {
                    close();
                }
                else if (m_progressLabel != nullptr)
                {
                    m_progressLabel->setText("Close the processing report to continue editing settings.");
                }
            }

            void ProcessingOverlayWidget::OnSetStatusMessage(const AZStd::string& message)
            {
                m_busyLabel->SetText(message.c_str());
            }

            void ProcessingOverlayWidget::SetUIToCompleteState()
            {
                if (CanClose())
                {
                    if (m_overlay && m_layerId != UI::OverlayWidget::s_invalidOverlayIndex)
                    {
                        m_overlay->RefreshLayer(m_layerId);
                    }
                    m_busyLabel->SetIsBusy(false);
                }
            }

            bool ProcessingOverlayWidget::CanClose() const
            {
                return !m_isClosingBlocked && m_isProcessingComplete;
            }

            bool ProcessingOverlayWidget::ShouldProcessMessage() const
            {
                AZStd::shared_ptr<const AzToolsFramework::Debug::TraceContextStack> stack = m_traceStackHandler.GetCurrentStack();
                if (stack)
                {
                    for (size_t i = 0; i < stack->GetStackCount(); ++i)
                    {
                        if (stack->GetType(i) == AzToolsFramework::Debug::TraceContextStackInterface::ContentType::UuidType)
                        {
                            if (stack->GetUuidValue(i) == m_traceTag)
                            {
                                return true;
                            }
                        }
                    }
                }
                return false;
            }

            void ProcessingOverlayWidget::CopyTraceContext(AzQtComponents::StyledDetailsTableModel::TableEntry& entry) const
            {
                AZStd::shared_ptr<const AzToolsFramework::Debug::TraceContextStack> stack = m_traceStackHandler.GetCurrentStack();
                if (stack)
                {
                    AZStd::string value;
                    for (size_t i = 0; i < stack->GetStackCount(); ++i)
                    {
                        if (stack->GetType(i) != AzToolsFramework::Debug::TraceContextStackInterface::ContentType::UuidType)
                        {
                            const char* key = stack->GetKey(i);
                            AzToolsFramework::Debug::TraceContextLogFormatter::PrintValue(value, *stack, i);
                            entry.Add(key, value.c_str());
                            value.clear();
                        }
                    }
                }
            }

            void ProcessingOverlayWidget::UpdateColumnSizes()
            {
                const int headerPadding = 5;
                m_reportView->resizeColumnsToContents();
                m_reportView->horizontalHeader()->resizeSection(0,
                                                                fontMetrics().horizontalAdvance("Status")
                                                                + style()->pixelMetric(QStyle::PM_HeaderMarkSize) + headerPadding);
            }
        } // SceneUI
    } // SceneAPI
} // AZ

#include <CommonWidgets/moc_ProcessingOverlayWidget.cpp>
