/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "SceneSettingsCard.h"

#include <QDateTime>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QListWidget>
#include <QMenu>
#include <QPushButton>
#include <QSvgWidget>
#include <QSvgRenderer>
#include <QTimer>
#include <QWidgetAction>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzToolsFramework/Debug/TraceContextLogFormatter.h>
#include <AzToolsFramework/Debug/TraceContextStack.h>
#include <AzToolsFramework/UI/Logging/LogEntry.h>
#include <AzQtComponents/Components/StyledDetailsTableView.h>
#include <AzQtComponents/Components/Widgets/TableView.h>
#include <SceneAPI/SceneUI/Handlers/ProcessingHandlers/ProcessingHandler.h>

SceneSettingsCardHeader::SceneSettingsCardHeader(QWidget* parent /* = nullptr */)
    : AzQtComponents::CardHeader(parent)
{
    m_busySpinner = new QSvgWidget(":/stylesheet/img/loading.svg", this);
    m_busySpinner->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    m_busySpinner->setMinimumSize(20, 20);
    m_busySpinner->setMaximumSize(20, 20);
    m_busySpinner->setBaseSize(20, 20);
    m_backgroundLayout->insertWidget(1, m_busySpinner);
    m_busySpinner->setStyleSheet("background-color: rgba(0,0,0,0)");
    m_busySpinner->setToolTip(tr("There is an active processing event for this file. The window will update when the event completes."));

    m_closeButton = new QPushButton(this);
    m_closeButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    m_closeButton->setMinimumSize(24, 24);
    m_closeButton->setMaximumSize(24, 24);
    m_closeButton->setBaseSize(24, 24);
    m_closeButton->setToolTip(tr("Removes this from the window. If you wish to see log details for this file again later, you can check the Asset Processor."));

    QIcon closeButtonIcon;
    closeButtonIcon.addPixmap(QPixmap(":/SceneUI/Common/CloseIcon.svg"));
    m_closeButton->setIcon(closeButtonIcon);
    m_closeButton->setFlat(true);

    m_backgroundLayout->addWidget(m_closeButton);
    
    connect(m_closeButton, &QPushButton::clicked, this, &SceneSettingsCardHeader::triggerCloseButton);
}

void SceneSettingsCardHeader::triggerCloseButton()
{
    // A singleshot + delete on the parent is used instead of calling deleteLater,
    // because the deleteLater wasn't functioning in automated tests, but this logic does.
    QObject* card(parent());
    QTimer::singleShot(
        0,
        [card]()
        {
            delete card;
        });
}

void SceneSettingsCardHeader::SetCanClose(bool canClose)
{
    m_closeButton->setEnabled(canClose);
    // If this card can be closed, it's not busy
    m_busySpinner->setHidden(canClose);
}

SceneSettingsCard::SceneSettingsCard(AZ::Uuid traceTag, QString fileTracked, Layout layout, QWidget* parent /* = nullptr */)
    :
    AzQtComponents::Card(new SceneSettingsCardHeader(), parent),
    m_traceTag(traceTag),
    m_fileTracked(fileTracked)
{
    m_settingsHeader = qobject_cast<SceneSettingsCardHeader*>(header());
    // This has to be set here, instead of in the customheader,
    // because the Card constructor forces the context menu to be visible.
    header()->setHasContextMenu(false);

    m_reportModel = new AzQtComponents::StyledDetailsTableModel(this);
    int statusColumn = m_reportModel->AddColumn("Status", AzQtComponents::StyledDetailsTableModel::StatusIcon);
    int platformColumn = -1;
    int windowColumn = -1;
    if (layout == Layout::Exporting)
    {
        platformColumn = m_reportModel->AddColumn("Platform");
        windowColumn = m_reportModel->AddColumn("Window");
        m_reportModel->AddColumnAlias("window", "Window");
    }
    int timeColumn = m_reportModel->AddColumn("Time");
    m_reportModel->AddColumn("Message");
    m_reportModel->AddColumnAlias("message", "Message");
    m_reportView = new AzQtComponents::TableView(this);
    m_reportView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_reportView->setModel(m_reportModel);

    if (platformColumn > 0)
    {
        m_reportView->header()->setSectionResizeMode(platformColumn, QHeaderView::ResizeToContents);
    }
    if (windowColumn > 0)
    {
        m_reportView->header()->setSectionResizeMode(windowColumn, QHeaderView::ResizeToContents);
    }

    m_reportView->header()->setSectionResizeMode(statusColumn, QHeaderView::ResizeToContents);
    m_reportView->header()->setSectionResizeMode(timeColumn, QHeaderView::ResizeToContents);
    setContentWidget(m_reportView);
    
    AZ::Debug::TraceMessageBus::Handler::BusConnect();
    
    m_reportView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_reportView, &AzQtComponents::TableView::customContextMenuRequested, this, &SceneSettingsCard::ShowLogContextMenu);
    
}

SceneSettingsCard::~SceneSettingsCard()
{
    AZ::Debug::TraceMessageBus::Handler::BusDisconnect();
}

void SceneSettingsCard::SetAndStartProcessingHandler(const AZStd::shared_ptr<AZ::SceneAPI::SceneUI::ProcessingHandler>& handler)
{
    AZ_Assert(handler, "Processing handler was null");
    AZ_Assert(!m_targetHandler, "A handler has already been assigned. Only one can be active per layer at any given time.");
    if (m_targetHandler)
    {
        return;
    }

    m_targetHandler = handler;

    connect(m_targetHandler.get(), &AZ::SceneAPI::SceneUI::ProcessingHandler::StatusMessageUpdated, this, &SceneSettingsCard::OnSetStatusMessage);
    connect(m_targetHandler.get(), &AZ::SceneAPI::SceneUI::ProcessingHandler::AddLogEntry, this, &SceneSettingsCard::AddLogEntry);
    connect(m_targetHandler.get(), &AZ::SceneAPI::SceneUI::ProcessingHandler::ProcessingComplete, this, &SceneSettingsCard::OnProcessingComplete);

    handler->BeginProcessing();
}

void SceneSettingsCard::AddLogEntry(const AzToolsFramework::Logging::LogEntry& logEntry)
{
    if (logEntry.GetSeverity() == AzToolsFramework::Logging::LogEntry::Severity::Message)
    {
        return;
    }
    AzQtComponents::StyledDetailsTableModel::TableEntry reportEntry;

    QVector<QPair<QString, QString>> detailsForLogLine;

    for (auto& field : logEntry.GetFields())
    {
        if (AzFramework::StringFunc::Equal("message", field.second.m_name.c_str()) ||
            AzFramework::StringFunc::Equal("window", field.second.m_name.c_str()))
        {
            // Add the message and window to the direct log
            reportEntry.Add(field.second.m_name.c_str(), field.second.m_value.c_str());
        }
        else
        {
            // All other fields, add to the additional details view.
            detailsForLogLine.push_back(QPair<QString, QString>(field.second.m_name.c_str(), field.second.m_value.c_str()));
        }
    }

    m_additionalLogDetails[m_reportModel->rowCount()] = detailsForLogLine;
    
    if (logEntry.GetSeverity() == AzToolsFramework::Logging::LogEntry::Severity::Error)
    {
        reportEntry.Add("Status", AzQtComponents::StyledDetailsTableModel::StatusError);
        UpdateCompletionState(CompletionState::Error);
    }
    else if (logEntry.GetSeverity() == AzToolsFramework::Logging::LogEntry::Severity::Warning)
    {
        reportEntry.Add("Status", AzQtComponents::StyledDetailsTableModel::StatusWarning);
        UpdateCompletionState(CompletionState::Warning);
    }
    reportEntry.Add("Time", GetTimeNowAsString());
    AddLogTableEntry(reportEntry);
}

void SceneSettingsCard::OnProcessingComplete()
{
    AzQtComponents::StyledDetailsTableModel::TableEntry entry;
    entry.Add("Message", "Asset processing completed.");
    entry.Add("Status", AzQtComponents::StyledDetailsTableModel::StatusSuccess);
    entry.Add("Time", GetTimeNowAsString());
    AddLogTableEntry(entry);
    
    SetState(SceneSettingsCard::State::Done);
}

void SceneSettingsCard::OnSetStatusMessage(const AZStd::string& message)
{
    AzQtComponents::StyledDetailsTableModel::TableEntry entry;
    entry.Add("Message", message.c_str());
    entry.Add("Status", AzQtComponents::StyledDetailsTableModel::StatusSuccess);
    entry.Add("Time", GetTimeNowAsString());
    AddLogTableEntry(entry);
}


bool SceneSettingsCard::OnPrintf(const char* window, const char* message)
{
    if (!ShouldProcessMessage())
    {
        return false;
    }
    AzQtComponents::StyledDetailsTableModel::TableEntry entry;
    if (AzFramework::StringFunc::Find(window, "Success") != AZStd::string::npos)
    {
        entry.Add("Status", AzQtComponents::StyledDetailsTableModel::StatusSuccess);
    }
    else if (AzFramework::StringFunc::Find(window, "Warning") != AZStd::string::npos)
    {
        entry.Add("Status", AzQtComponents::StyledDetailsTableModel::StatusWarning);
        UpdateCompletionState(CompletionState::Warning);
    }
    else if (AzFramework::StringFunc::Find(window, "Error") != AZStd::string::npos)
    {
        entry.Add("Status", AzQtComponents::StyledDetailsTableModel::StatusError);
        UpdateCompletionState(CompletionState::Error);
    }
    else
    {
        // To reduce noise in the report widget, only show success, warning and error messages.
        return false;
    }
    entry.Add("Message", message);
    entry.Add("Time", GetTimeNowAsString());
    AddLogTableEntry(entry);
    return false;
}

bool SceneSettingsCard::OnError(const char* /*window*/, const char* message)
{
    if (!ShouldProcessMessage())
    {
        return false;
    }
    AzQtComponents::StyledDetailsTableModel::TableEntry entry;
    entry.Add("Message", message);
    entry.Add("Status", AzQtComponents::StyledDetailsTableModel::StatusError);
    entry.Add("Time", GetTimeNowAsString());
    AddLogTableEntry(entry);
    
    UpdateCompletionState(CompletionState::Error);
    return false;
}

bool SceneSettingsCard::OnWarning(const char* /*window*/, const char* message)
{
    if (!ShouldProcessMessage())
    {
        return false;
    }
    AzQtComponents::StyledDetailsTableModel::TableEntry entry;
    entry.Add("Message", message);
    entry.Add("Status", AzQtComponents::StyledDetailsTableModel::StatusWarning);
    entry.Add("Time", GetTimeNowAsString());
    AddLogTableEntry(entry);    
    UpdateCompletionState(CompletionState::Warning);
    return false;
}

bool SceneSettingsCard::OnAssert(const char* message)
{
    if (!ShouldProcessMessage())
    {
        return false;
    }
    AzQtComponents::StyledDetailsTableModel::TableEntry entry;
    entry.Add("Message", message);
    entry.Add("Status", AzQtComponents::StyledDetailsTableModel::StatusError);
    entry.Add("Time", GetTimeNowAsString());
    AddLogTableEntry(entry);
    UpdateCompletionState(CompletionState::Error);
    return false;
}

void SceneSettingsCard::SetState(State newState)
{
    switch (newState)
    {
    case State::Loading:
        {
            QString toolTip(tr("The scene settings for this file are being loaded. The window will update when the event completes."));
            setTitle(tr("Loading scene settings"));
            setTitleToolTip(toolTip);
            m_settingsHeader->m_busySpinner->setToolTip(toolTip);
            m_settingsHeader->SetCanClose(false);
        }
        break;
    case State::Processing:
        {
            QString toolTip(tr("The scene file is being processed. The window will update when the event completes."));
            setTitle(tr("Saving scene settings, and reprocessing scene file"));
            setTitleToolTip(toolTip);
            m_settingsHeader->m_busySpinner->setToolTip(toolTip);
            m_settingsHeader->SetCanClose(false);
        }
        break;
    case State::Done:
        {
            QString toolTip(tr("No errors or warnings were encountered with the scene file."));
            QString errorsAndWarningsString;
            if (m_warningCount > 0 || m_errorCount > 0)
            {
                errorsAndWarningsString = tr(" with %1 warning(s), %2 error(s)").arg(m_warningCount).arg(m_errorCount);
                toolTip = tr("Warnings and/or errors were encountered with the scene file. You can view the details by expanding this card "
                             "and reading the log message.");
            }

            QString previousStateString;
            switch (m_sceneCardState)
            {
            case State::Loading:
                previousStateString = tr("Loading");
                break;
            case State::Processing:
                previousStateString = tr("Processing");
                toolTip = tr("%1 If you dismiss this card, you can view the processing logs again in the Asset Processor.").arg(toolTip);
                break;
                }
            setTitle(tr("%1 %2 completed at %3%4").arg(previousStateString).arg(m_fileTracked).arg(GetTimeNowAsString()).arg(errorsAndWarningsString));
            setTitleToolTip(toolTip);
            m_settingsHeader->SetCanClose(true);

            switch (m_completionState)
            {
            case CompletionState::Error:
            case CompletionState::Failure:
                m_settingsHeader->setIcon(QIcon(":/SceneUI/Common/ErrorIcon.svg"));
                m_settingsHeader->setUnderlineColor(QColor(226, 82, 67));
                break;
            case CompletionState::Warning:
                m_settingsHeader->setIcon(QIcon(":/SceneUI/Common/WarningIcon.svg"));
                m_settingsHeader->setUnderlineColor(QColor(240, 195, 45));
                break;
            default:
                m_settingsHeader->setIcon(QIcon(":/SceneUI/Common/SuccessIcon.svg"));
                m_settingsHeader->setUnderlineColor(QColor(88, 188, 97));
                break;
            }
            AZ::Debug::TraceMessageBus::Handler::BusDisconnect();
            emit ProcessingCompleted();
        }
        break;
    }
    m_sceneCardState = newState;
}

bool SceneSettingsCard::ShouldProcessMessage()
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

void SceneSettingsCard::UpdateCompletionState(CompletionState newState)
{
    // Use the highest encountered state
    m_completionState = AZStd::max(m_completionState, newState);
    switch (newState)
    {
    case CompletionState::Warning:
        ++m_warningCount;
        break;
    case CompletionState::Error:
        ++m_errorCount;
        break;
    }
}

void SceneSettingsCard::CopyTraceContext(AzQtComponents::StyledDetailsTableModel::TableEntry& entry) const
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

void SceneSettingsCard::AddLogTableEntry(AzQtComponents::StyledDetailsTableModel::TableEntry& entry)
{
    CopyTraceContext(entry);
    m_reportModel->AddEntry(entry);
    // Set the height of the view, so the cards expand more vertically.
    // Clamp that max height at a point so it doesn't try to pull too much height from containing window.
    int rowCount = m_reportModel->rowCount();
    if(rowCount < 10)
    {
        m_reportView->setMinimumHeight(m_reportView->sizeHintForRow(0) * (rowCount + 1));
    }
}

QString SceneSettingsCard::GetTimeNowAsString()
{
    return QDateTime::currentDateTime().toString(tr("hh:mm:ss ap"));
}

void SceneSettingsCard::ShowLogContextMenu(const QPoint& pos)
{
    const QModelIndex selectedIndex = m_reportView->indexAt(pos);
    if (!selectedIndex.isValid())
    {
        return;
    }
    
    int logRow = selectedIndex.row();

    if (!m_additionalLogDetails.contains(logRow))
    {
        return;
    }

    int additionalLogCount = m_additionalLogDetails[logRow].count();

    if (additionalLogCount <= 0)
    {
        return;
    }

    // If the log message for the first row is empty, skip. This happens when there was no log details at all.
    if (additionalLogCount == 1 && m_additionalLogDetails[logRow][0].second.isEmpty())
    {
        return;
    }

    QMenu menu;
    menu.setToolTipsVisible(true);
    QAction* contextMenuTitleAction = menu.addAction("Additional log context");
    contextMenuTitleAction->setToolTip(tr("Additional log information for the selected line"));
    menu.addSeparator();
    
    QWidgetAction* logMenuListAction = new QWidgetAction(&menu);
        
    QListWidget* logDetailsWidget = new QListWidget(&menu);
    logDetailsWidget->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
    logDetailsWidget->setTextElideMode(Qt::ElideLeft);
    logDetailsWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    logDetailsWidget->setSelectionMode(QAbstractItemView::NoSelection);

    logMenuListAction->setDefaultWidget(logDetailsWidget);

    for (const auto& logDetail : m_additionalLogDetails[logRow])
    {
        logDetailsWidget->addItem(tr("%1 - %2").arg(logDetail.first).arg(logDetail.second));
    }

    logDetailsWidget->setFixedHeight(additionalLogCount * logDetailsWidget->sizeHintForRow(0));
    logDetailsWidget->setFixedWidth(logDetailsWidget->sizeHintForColumn(0));
    menu.addAction(logMenuListAction);

    menu.exec(m_reportView->viewport()->mapToGlobal(pos));
}
