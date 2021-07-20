/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "StandaloneTools_precompiled.h"

#include "TraceDrillerDialog.hxx"
#include "TraceMessageDataAggregator.hxx"
#include <Source/Driller/Trace/ui_TraceDrillerDialog.h>
#include "TraceMessageEvents.h"

#include <QtWidgets/QSizePolicy>
#include <AzToolsFramework/UI/UICore/QWidgetSavedState.h>
#include <AzCore/UserSettings/UserSettingsComponent.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <Source/Driller/Workspaces/Workspace.h>

#include <QSortFilterProxyModel>
#include <QTableView>
#include <QStyle>

namespace Driller
{
    class TraceDrillerDialogSavedState
        : public AZ::UserSettings
    {
    public:
        AZ_RTTI(TraceDrillerDialogSavedState, "{81955B84-077D-4A87-B562-7A9633736BE4}", AZ::UserSettings);
        AZ_CLASS_ALLOCATOR(TraceDrillerDialogSavedState, AZ::SystemAllocator, 0);

        AZStd::string m_windowFilter;
        AZStd::string m_textFilter;

        TraceDrillerDialogSavedState() {}

        static void Reflect(AZ::ReflectContext* context)
        {
            AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
            if (serialize)
            {
                serialize->Class<TraceDrillerDialogSavedState>()
                    ->Field("m_windowFilter", &TraceDrillerDialogSavedState::m_windowFilter)
                    ->Field("m_textFilter", &TraceDrillerDialogSavedState::m_textFilter)
                    ->Version(1);
            }
        }
    };

    // Qt supports A "filter proxy model"
    // you have a normal model
    // and then you wrap that model in a filter proxy model.
    // this allows you to filter the inner model and feed the outer (filtered) model to the view.

    // this particular filter model lets you specify search criteria in the Window or the Message field
    class TraceFilterModel
        : public QSortFilterProxyModel
    {
    public:
        AZ_CLASS_ALLOCATOR(TraceFilterModel, AZ::SystemAllocator, 0);
        int m_windowColumn;
        int m_messageColumn;

        QString m_currentWindowFilter;
        QString m_currentMessageFilter;

        TraceFilterModel(int windowColumn, int messageColumn, QObject* pParent)
            : QSortFilterProxyModel(pParent)
        {
            m_windowColumn = windowColumn;
            m_messageColumn = messageColumn;
        }

        void UpdateWindowFilter(const QString& newFilter)
        {
            if (newFilter.compare(m_currentWindowFilter) != 0)
            {
                if (m_windowColumn >= 0)
                {
                    m_currentWindowFilter = newFilter;
                    invalidateFilter();
                }
            }
        }

        void UpdateMessageFilter(const QString& newFilter)
        {
            if (newFilter.compare(m_currentMessageFilter) != 0)
            {
                if (m_messageColumn >= 0)
                {
                    m_currentMessageFilter = newFilter;
                    invalidateFilter();
                }
            }
        }


    protected:
        virtual bool filterAcceptsRow(int source_row, const QModelIndex& /*source parent*/) const
        {
            QAbstractItemModel* ptrModel = sourceModel();

            if (!ptrModel)
            {
                return true;
            }

            if (m_currentWindowFilter.size() > 0)
            {
                QString sourceData = ptrModel->data(ptrModel->index(source_row, m_windowColumn), Qt::DisplayRole).toString();
                if (!sourceData.contains(m_currentWindowFilter, Qt::CaseInsensitive))
                {
                    return false;
                }
            }

            if (m_currentMessageFilter.size() > 0)
            {
                QString sourceData = ptrModel->data(ptrModel->index(source_row, m_messageColumn), Qt::DisplayRole).toString();
                if (!sourceData.contains(m_currentMessageFilter, Qt::CaseInsensitive))
                {
                    return false;
                }
            }

            return true;
        }
    };

    TraceDrillerDialog::TraceDrillerDialog(TraceMessageDataAggregator* data, int profilerIndex, QWidget* pParent)
        : QDialog(pParent)
        , m_viewIndex(profilerIndex)
        , m_lifespanTelemetry("TraceDataView")
    {
        setAttribute(Qt::WA_DeleteOnClose, true);

        m_uiLoaded = azcreate(Ui::TraceDrillerDialog, ());
        m_uiLoaded->setupUi(this);
        setWindowFlags((this->windowFlags() | Qt::WindowMaximizeButtonHint | Qt::WindowMinimizeButtonHint) & ~(Qt::WindowContextHelpButtonHint));

        setWindowTitle(data->GetDialogTitle());

        TraceDrillerLogTab* tabView = aznew TraceDrillerLogTab(this);

        this->layout()->addWidget(tabView);

        m_ptrOriginalModel = aznew TraceDrillerLogModel(data, this);
        m_ptrFilter = aznew TraceFilterModel(tabView->GetWindowColumn(), tabView->GetMessageColumn(), this);
        m_ptrFilter->setSourceModel(m_ptrOriginalModel);
        tabView->ConnectModelToView(m_ptrFilter);

        connect(data, SIGNAL(destroyed(QObject*)), this, SLOT(OnDataDestroyed()));

        connect(m_ptrFilter, SIGNAL(rowsAboutToBeInserted(const QModelIndex &, int, int)), tabView, SLOT(rowsAboutToBeInserted()));
        connect(m_ptrFilter, SIGNAL(rowsInserted(const QModelIndex &, int, int)), tabView, SLOT(rowsInserted()));
        connect(m_uiLoaded->windowFilterText, SIGNAL(textChanged(const QString &)), this, SLOT(onTextChangeWindowFilter(const QString&)));
        connect(m_uiLoaded->messageFilterText, SIGNAL(textChanged(const QString &)), this, SLOT(onTextChangeMessageFilter(const QString&)));

        connect(m_ptrFilter, SIGNAL(rowsInserted(const QModelIndex &, int, int)), this, SLOT(UpdateSummary()));
        connect(m_ptrFilter, SIGNAL(rowsRemoved(const QModelIndex &, int, int)), this, SLOT(UpdateSummary()));
        connect(m_ptrFilter, SIGNAL(modelReset()), this, SLOT(UpdateSummary()));
        connect(m_ptrOriginalModel, SIGNAL(rowsInserted(const QModelIndex &, int, int)), this, SLOT(UpdateSummary()));
        connect(m_ptrOriginalModel, SIGNAL(rowsRemoved(const QModelIndex &, int, int)), this, SLOT(UpdateSummary()));
        connect(m_ptrOriginalModel, SIGNAL(modelReset()), this, SLOT(UpdateSummary()));

        AZStd::string windowStateStr = AZStd::string::format("TRACE DRILLER DATA VIEW WINDOW STATE %i", m_viewIndex);
        m_windowStateCRC = AZ::Crc32(windowStateStr.c_str());
        AZStd::intrusive_ptr<AzToolsFramework::QWidgetSavedState> windowState = AZ::UserSettings::Find<AzToolsFramework::QWidgetSavedState>(m_windowStateCRC, AZ::UserSettings::CT_GLOBAL);
        if (windowState)
        {
            windowState->RestoreGeometry(this);
        }

        AZStd::string filterStateStr = AZStd::string::format("TRACE DRILLER DIALOG SAVED STATE %i", m_viewIndex);
        m_filterStateCRC = AZ::Crc32(filterStateStr.c_str());
        m_persistentState = AZ::UserSettings::Find<TraceDrillerDialogSavedState>(m_filterStateCRC, AZ::UserSettings::CT_GLOBAL);
        ApplyPersistentState();
        UpdateSummary();
    }

    TraceDrillerDialog::~TraceDrillerDialog()
    {
        SaveOnExit();
        azdestroy(m_uiLoaded);
    }

    void TraceDrillerDialog::ApplyPersistentState()
    {
        if (m_persistentState)
        {
            // the bridge between our AZStd::string storage and QT's own string type
            QString windowFilter(m_persistentState->m_windowFilter.c_str());
            QString textFilter(m_persistentState->m_textFilter.c_str());

            m_uiLoaded->windowFilterText->setText(windowFilter);
            m_uiLoaded->messageFilterText->setText(textFilter);
        }
    }

    void TraceDrillerDialog::SaveOnExit()
    {
        AZStd::intrusive_ptr<AzToolsFramework::QWidgetSavedState> pState = AZ::UserSettings::CreateFind<AzToolsFramework::QWidgetSavedState>(m_windowStateCRC, AZ::UserSettings::CT_GLOBAL);
        pState->CaptureGeometry(this);

        AZStd::intrusive_ptr<TraceDrillerDialogSavedState> newState = AZ::UserSettings::CreateFind<TraceDrillerDialogSavedState>(m_filterStateCRC, AZ::UserSettings::CT_GLOBAL);
        if (newState)
        {
            newState->m_windowFilter = m_ptrFilter->m_currentWindowFilter.toUtf8().data();
            newState->m_textFilter = m_ptrFilter->m_currentMessageFilter.toUtf8().data();
        }
    }
    void TraceDrillerDialog::hideEvent(QHideEvent* evt)
    {
        QDialog::hideEvent(evt);
    }
    void TraceDrillerDialog::closeEvent(QCloseEvent* evt)
    {
        QDialog::closeEvent(evt);
    }
    void TraceDrillerDialog::OnDataDestroyed()
    {
        deleteLater();
    }

    void TraceDrillerDialog::onTextChangeWindowFilter(const QString& newText)
    {
        m_ptrFilter->UpdateWindowFilter(newText);
        UpdateSummary();
    }

    void TraceDrillerDialog::onTextChangeMessageFilter(const QString& newText)
    {
        m_ptrFilter->UpdateMessageFilter(newText);
        UpdateSummary();
    }

    void TraceDrillerDialog::UpdateSummary()
    {
        int filterRows = m_ptrFilter->rowCount();
        int originalRows = m_ptrOriginalModel->rowCount();

        if (!m_uiLoaded->windowFilterText->text().isEmpty() || !m_uiLoaded->messageFilterText->text().isEmpty())
        {
            m_uiLoaded->summaryLabel->setText(QString("%1 / %2\nEvent(s)").arg(filterRows).arg(originalRows));
        }
        else
        {
            m_uiLoaded->summaryLabel->setText(QString("%1\nEvent(s)").arg(originalRows));
        }
    }

    void TraceDrillerDialog::ApplySettingsFromWorkspace(WorkspaceSettingsProvider* provider)
    {
        AZStd::string workspaceStateStr = AZStd::string::format("TRACE DRILLER DIALOG WORKSPACE STATE %i", m_viewIndex);
        AZ::u32 workspaceStateCRC = AZ::Crc32(workspaceStateStr.c_str());

        if (m_persistentState)
        {
            TraceDrillerDialogSavedState* workspace = provider->FindSetting<TraceDrillerDialogSavedState>(workspaceStateCRC);
            if (workspace)
            {
                m_persistentState->m_windowFilter = workspace->m_windowFilter;
                m_persistentState->m_textFilter = workspace->m_textFilter;
            }
        }
    }
    void TraceDrillerDialog::ActivateWorkspaceSettings(WorkspaceSettingsProvider*)
    {
        ApplyPersistentState();
    }
    void TraceDrillerDialog::SaveSettingsToWorkspace(WorkspaceSettingsProvider* provider)
    {
        AZStd::string workspaceStateStr = AZStd::string::format("TRACE DRILLER DIALOG WORKSPACE STATE %i", m_viewIndex);
        AZ::u32 workspaceStateCRC = AZ::Crc32(workspaceStateStr.c_str());

        if (m_persistentState)
        {
            TraceDrillerDialogSavedState* workspace = provider->CreateSetting<TraceDrillerDialogSavedState>(workspaceStateCRC);
            if (workspace)
            {
                workspace->m_windowFilter = m_ptrFilter->m_currentWindowFilter.toUtf8().data();
                workspace->m_textFilter = m_ptrFilter->m_currentMessageFilter.toUtf8().data();
            }
        }
    }

    void TraceDrillerDialog::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            TraceDrillerDialogSavedState::Reflect(context);
        }
    }

    //////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////
    // TraceDrillerLogTab
    //////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////
    TraceDrillerLogTab::TraceDrillerLogTab(QWidget* pParent)
        : BaseLogView(pParent)
        , m_isScrollAfterInsert(true)
    {
        setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
    }

    TraceDrillerLogTab::~TraceDrillerLogTab()
    {
    }

    void TraceDrillerLogTab::rowsAboutToBeInserted()
    {
        m_isScrollAfterInsert = IsAtMaxScroll();
    }

    void TraceDrillerLogTab::rowsInserted()
    {
        if (m_isScrollAfterInsert)
        {
            m_ptrLogView->scrollToBottom();
        }
    }

    //////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////
    // TraceDrillerLogModel
    //////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////
    TraceDrillerLogModel::TraceDrillerLogModel(TraceMessageDataAggregator* data, QObject* pParent)
        : QAbstractTableModel(pParent)
        , m_data(data)
        , m_lastShownEvent(-1)
    {
        connect(data, SIGNAL(OnDataCurrentEventChanged()), this, SLOT(OnDataCurrentEventChanged()));
        connect(data, SIGNAL(OnDataAddEvent()), this, SLOT(OnDataAddEvent()));

        // cache Icons!
        m_criticalIcon      = QApplication::style()->standardIcon(QStyle::SP_MessageBoxCritical);
        m_errorIcon         = QApplication::style()->standardIcon(QStyle::SP_MessageBoxCritical);
        m_warningIcon       = QApplication::style()->standardIcon(QStyle::SP_MessageBoxWarning);
        m_informationIcon   = QApplication::style()->standardIcon(QStyle::SP_MessageBoxInformation);

        m_lastShownEvent = m_data->GetCurrentEvent();
    }

    TraceDrillerLogModel::~TraceDrillerLogModel()
    {
    }

    void TraceDrillerLogModel::OnDataCurrentEventChanged()
    {
        AZ::s64 currentEvent = m_data->GetCurrentEvent();
        // NOTE: we add +1 to all events, because we are EXECUTING the current event (so it must be shown)
        if (m_lastShownEvent > currentEvent)
        {
            // remove rows
            beginRemoveRows(QModelIndex(), (int)currentEvent + 1, (int)m_lastShownEvent);
            endRemoveRows();
        }
        else
        {
            // add rows
            beginInsertRows(QModelIndex(), (int)m_lastShownEvent + 1, (int)currentEvent);
            endInsertRows();
        }
        m_lastShownEvent = currentEvent;
    }

    void TraceDrillerLogModel::OnDataAddEvent()
    {
    }

    int TraceDrillerLogModel::rowCount(const QModelIndex&) const
    {
        AZ::s64 currentEvent = m_data->GetCurrentEvent();
        return (int)currentEvent + 1;
    }
    int TraceDrillerLogModel::columnCount(const QModelIndex&) const
    {
        return 3; // icon + window + message;
    }
    Qt::ItemFlags TraceDrillerLogModel::flags(const QModelIndex& index) const
    {
        if (!index.isValid())
        {
            return Qt::ItemIsEnabled;
        }

        if (index.column() == 2) // the 3rd column (message) is "editable"
        {
            return QAbstractItemModel::flags(index) | Qt::ItemIsEditable;
        }

        return QAbstractItemModel::flags(index);
    }

    QVariant TraceDrillerLogModel::data(const QModelIndex& index, int role) const
    {
        using namespace AZ::Debug;

        TraceMessageEvent* event = static_cast<TraceMessageEvent*>(m_data->GetEvents().at(index.row()));
        if (role == AzToolsFramework::LogPanel::RichTextRole) // the renderer is asking whether this cell is rich text or not.  Return a true or false.
        {
            return /*m_Lines[index.row()].m_bIsRichText*/ false;
        }
        else if (role == Qt::DecorationRole) // the renderer is asking whether or not we want to display an icon in this cell.  Return an icon or null.
        {
            if (index.column() == 0)
            {
                switch (event->GetEventType())
                {
                case TraceMessageEvent::ET_ASSERT:
                    return m_criticalIcon;
                case TraceMessageEvent::ET_ERROR:
                    return m_errorIcon;
                case TraceMessageEvent::ET_WARNING:
                    return m_warningIcon;
                case TraceMessageEvent::ET_PRINTF:
                    return m_informationIcon;
                case TraceMessageEvent::ET_EXCEPTION:
                    return QVariant();
                    break;
                }
            }
        }
        else if (role == Qt::DisplayRole) // the renderer wants to know what text to show in this cell.  Return a string or null
        {
            if (index.column() == 0) // icon has no text
            {
                return QVariant(QString());
            }
            else if (index.column() == 1) // window
            {
                return QVariant(event->m_window);
            }
            else if (index.column() == 2) // message
            {
                return QVariant(QString(event->m_message).trimmed());
            }
        }
        else if (role == Qt::BackgroundRole) // the renderer wants to know what the background color of this cell should be.  REturn a color or null (to use default)
        {
            switch (event->GetEventType())
            {
            case TraceMessageEvent::ET_ASSERT:
                return QVariant(QColor::fromRgb(255, 0, 0));
                break;
            case TraceMessageEvent::ET_ERROR:
                return QVariant(QColor::fromRgb(255, 192, 192));
                break;
            case TraceMessageEvent::ET_WARNING:
                return QVariant(QColor::fromRgb(255, 255, 192));
                break;
            case TraceMessageEvent::ET_PRINTF:
                return QVariant();
                break;
            case TraceMessageEvent::ET_EXCEPTION:
                return QVariant();
                break;
            }
        }
        else if (role == Qt::ForegroundRole) // the renderer wants to know what the text color of this cell should be.REturn a color or null.
        {
            switch (event->GetEventType())
            {
            case TraceMessageEvent::ET_PRINTF:
                return QVariant(QColor::fromRgb(0, 0, 0));
                break;
            case TraceMessageEvent::ET_ERROR:
                return QVariant(QColor::fromRgb(64, 0, 0));
                break;
            case TraceMessageEvent::ET_WARNING:
                return QVariant(QColor::fromRgb(64, 64, 0));
                break;
            case TraceMessageEvent::ET_EXCEPTION:
                return QVariant();
                break;
            }
        }
        return QVariant();
    }
}

#include <Source/Driller/Trace/moc_TraceDrillerDialog.cpp>
