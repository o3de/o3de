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
#include <QPushButton>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzToolsFramework/Debug/TraceContextLogFormatter.h>
#include <AzToolsFramework/Debug/TraceContextStack.h>
#include <AzToolsFramework/UI/Logging/LogEntry.h>
#include <AzQtComponents/Components/StyledBusyLabel.h>
#include <AzQtComponents/Components/StyledDetailsTableView.h>
#include <AzQtComponents/Components/Widgets/TableView.h>
#include <SceneAPI/SceneUI/Handlers/ProcessingHandlers/ProcessingHandler.h>

SceneSettingsCardHeader::SceneSettingsCardHeader(QWidget* parent /* = nullptr */)
    : AzQtComponents::CardHeader(parent)
{
    m_busyLabel = new AzQtComponents::StyledBusyLabel(this);
    m_busyLabel->SetIsBusy(true);
    m_busyLabel->SetBusyIconSize(14);
    m_backgroundLayout->insertWidget(1, m_busyLabel);

    m_closeButton = new QPushButton(this);
    m_closeButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    m_closeButton->setMinimumSize(24, 24);
    m_closeButton->setMaximumSize(24, 24);
    m_closeButton->setBaseSize(24, 24);

    QIcon closeButtonIcon;
    closeButtonIcon.addPixmap(QPixmap(":/SceneUI/Common/CloseIcon.png"), QIcon::Normal);
    closeButtonIcon.addPixmap(QPixmap(":/SceneUI/Common/CloseIconDisable.png"), QIcon::Disabled);
    closeButtonIcon.addPixmap(QPixmap(":/SceneUI/Common/CloseIconActive.png"), QIcon::Active);
    closeButtonIcon.addPixmap(QPixmap(":/SceneUI/Common/CloseIconActive.png"), QIcon::Selected);
    m_closeButton->setIcon(closeButtonIcon);
    m_closeButton->setFlat(true);

    m_backgroundLayout->addWidget(m_closeButton);
    
    connect(m_closeButton, &QPushButton::clicked, this, &SceneSettingsCardHeader::triggerCloseButton);
}

void SceneSettingsCardHeader::triggerCloseButton()
{
    parent()->deleteLater();
}

void SceneSettingsCardHeader::SetCanClose(bool canClose)
{
    m_closeButton->setEnabled(canClose);
    // If this card can be closed, it's not busy
    m_busyLabel->setHidden(canClose);
}

SceneSettingsCard::SceneSettingsCard(AZ::Uuid traceTag, Layout layout, AzQtComponents::StyledDetailsTableModel* logDetailsModel, QWidget* parent /* = nullptr */)
    :
    AzQtComponents::Card(new SceneSettingsCardHeader(), parent),
    m_traceTag(traceTag),
    m_logDetailsModel(logDetailsModel)
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
    

    connect(m_reportView->selectionModel(), &QItemSelectionModel::selectionChanged,
        this, &SceneSettingsCard::OnLogLineSelected);

    AZ::Debug::TraceMessageBus::Handler::BusConnect();
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

    bool hasStatus = false;
    for (auto& field : logEntry.GetFields())
    {
        size_t offset = 0;
        hasStatus = hasStatus || AzFramework::StringFunc::Equal("status", field.second.m_name.c_str());
        if (AzFramework::StringFunc::Equal("message", field.second.m_name.c_str()) ||
            AzFramework::StringFunc::Equal("window", field.second.m_name.c_str()))
        {
            if (field.second.m_value.length() > 2)
            {
                // Removing the prefixes such as "W: " and "E: ".
                if (field.second.m_value[1] == ':' && field.second.m_value[2] == ' ')
                {
                    offset = 3;
                }
            }
            reportEntry.Add(field.second.m_name.c_str(), field.second.m_value.c_str() + offset);
        }
        else
        {
            detailsForLogLine.push_back(QPair<QString, QString>(field.second.m_name.c_str(), field.second.m_value.c_str()));
        }
    }

    if (!detailsForLogLine.empty())
    {
        m_additionalLogDetails.push_back(detailsForLogLine);
    }
    else
    {
        AddEmptyLogDetailsForRow();
    }
    
    if (!hasStatus)
    {
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
    }
    reportEntry.Add("Time", GetTimeAsString());

    m_reportModel->AddEntry(reportEntry);
}

void SceneSettingsCard::OnProcessingComplete()
{
    AzQtComponents::StyledDetailsTableModel::TableEntry entry;
    entry.Add("Message", "Asset processing completed.");
    entry.Add("Status", AzQtComponents::StyledDetailsTableModel::StatusSuccess);
    entry.Add("Time", GetTimeAsString());
    AddEmptyLogDetailsForRow();    
    m_reportModel->AddEntry(entry);
    
    SetState(SceneSettingsCard::State::Done);
}

void SceneSettingsCard::OnSetStatusMessage(const AZStd::string& message)
{
    AzQtComponents::StyledDetailsTableModel::TableEntry entry;
    entry.Add("Message", message.c_str());
    entry.Add("Status", AzQtComponents::StyledDetailsTableModel::StatusSuccess);
    entry.Add("Time", GetTimeAsString());
    AddEmptyLogDetailsForRow();
    m_reportModel->AddEntry(entry);
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
    entry.Add("Time", GetTimeAsString());
    CopyTraceContext(entry);
    AddEmptyLogDetailsForRow();
    m_reportModel->AddEntry(entry);
    return false;
}

bool SceneSettingsCard::OnError(const char* window, const char* message)
{
    if (!ShouldProcessMessage())
    {
        return false;
    }
    (void*)window;
    AzQtComponents::StyledDetailsTableModel::TableEntry entry;
    entry.Add("Message", message);
    entry.Add("Status", AzQtComponents::StyledDetailsTableModel::StatusError);
    entry.Add("Time", GetTimeAsString());
    CopyTraceContext(entry);
    m_reportModel->AddEntry(entry);
    
    UpdateCompletionState(CompletionState::Error);
    return false;
}

bool SceneSettingsCard::OnWarning(const char* window, const char* message)
{
    if (!ShouldProcessMessage())
    {
        return false;
    }
    (void*)window;
    AzQtComponents::StyledDetailsTableModel::TableEntry entry;
    entry.Add("Message", message);
    entry.Add("Status", AzQtComponents::StyledDetailsTableModel::StatusWarning);
    entry.Add("Time", GetTimeAsString());
    CopyTraceContext(entry);
    m_reportModel->AddEntry(entry);
    
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
    entry.Add("Time", GetTimeAsString());
    CopyTraceContext(entry);
    m_reportModel->AddEntry(entry);
    UpdateCompletionState(CompletionState::Error);
    return false;
}

void SceneSettingsCard::SetState(State newState)
{
    switch (newState)
    {
    case State::Loading:
        setTitle(tr("Loading scene settings"));
        m_settingsHeader->SetCanClose(false);
        break;
    case State::Processing:
        setTitle(tr("Saving scene settings, and reprocessing scene file"));
        m_settingsHeader->SetCanClose(false);
        break;
    case State::Done:
        {
            QString errorsAndWarningsString("");
            if (m_warningCount > 0 || m_errorCount > 0)
            {
                errorsAndWarningsString = tr(" with %1 warning(s), %2 error(s)").arg(m_warningCount).arg(m_errorCount);
            }

            setTitle(tr("Processing completed at %1%2").arg(GetTimeAsString()).arg(errorsAndWarningsString));
            m_settingsHeader->SetCanClose(true);

            switch (m_completionState)
            {
            case CompletionState::Error:
            case CompletionState::Failure:
                m_settingsHeader->setIcon(QIcon(":/stylesheet/img/table_error.png"));
                break;
            case CompletionState::Warning:
                m_settingsHeader->setIcon(QIcon(":/stylesheet/img/table_warning.png"));
                break;
            default:
                m_settingsHeader->setIcon(QIcon(":/stylesheet/img/table_success.png"));
                break;
            }
            AZ::Debug::TraceMessageBus::Handler::BusDisconnect();
            emit ProcessingCompleted();
        }
        break;
    }
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

QString SceneSettingsCard::GetTimeAsString()
{
    return QDateTime::currentDateTime().toString(tr("hh:mm:ss ap"));
}

void SceneSettingsCard::AddEmptyLogDetailsForRow()
{
    QVector<QPair<QString, QString>> detailsForLogLine;
    detailsForLogLine.push_back(QPair<QString, QString>(tr("Info"), tr("No additional details available.")));
    m_additionalLogDetails.push_back(detailsForLogLine);
}

void SceneSettingsCard::OnLogLineSelected()
{
    const QModelIndexList indexes = m_reportView->selectionModel()->selectedIndexes();
    if (indexes.isEmpty())
    {
        // TODO: Clear the view?
        return;
    }

    int logRow = indexes.front().row();
    if (logRow > m_additionalLogDetails.count())
    {
        // TODO: Clear the view?
        return;
    }

    if (m_logDetailsModel->rowCount() > 0)
    {
        bool removeRowResult = m_logDetailsModel->removeRows(0, m_logDetailsModel->rowCount());

        if (!removeRowResult)
        {
            return;
        }
    }

    for(const auto& logDetail : m_additionalLogDetails[logRow])
    {
        AzQtComponents::StyledDetailsTableModel::TableEntry entry;
        entry.Add("Title", logDetail.first);
        entry.Add("Message", logDetail.second);
        m_logDetailsModel->AddEntry(entry);
    }
    //ui->nameLineEdit->setText(NameForIndex(indexes.first()));
}
