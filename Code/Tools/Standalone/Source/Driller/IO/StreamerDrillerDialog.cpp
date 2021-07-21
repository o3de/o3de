/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "Woodpecker_precompiled.h"

#include "StreamerDrillerDialog.hxx"
#include <Woodpecker/Driller/IO/moc_StreamerDrillerDialog.cpp>

#include "StreamerDataAggregator.hxx"
#include "StreamerEvents.h"

#include <QtWidgets/QSizePolicy>
#include <AzToolsFramework/UI/UICore/QWidgetSavedState.h>
#include <AzCore/UserSettings/UserSettingsComponent.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <Woodpecker/Driller/Workspaces/Workspace.h>

#include <Woodpecker/Driller/IO/ui_StreamerDrillerDialog.h>

#include <QSortFilterProxyModel>
#include <QAction>
#include <QMenu>
#include <QClipboard>

namespace Driller
{
    class StreamerDrillerDialogSavedState;

    // NB: update StreamerDataView.hxx GetXxxxColumn() calls to return matching numbers to SDM_ enums

    enum
    {
        SDM_NAME = 0,
        SDM_DEBUG_NAME,
        SDM_EVENT_TYPE,
        SDM_OPERATION,
        SDM_DELTA_TIME,
        SDM_DATA_TRANSFER,
        SDM_READ_SIZE,
        SDM_OFFSET,
        SDM_TOTAL
    };
    static const char* SDM_STRING[] = {
        "Name",
        "Debug Name",
        "Event Type",
        "Operation",
        "uSec Used",
        "Data Transfer",
        "Read Size",
        "Offset",
    };
    enum
    {
        VIEW_TYPE_THROUGHPUT = 0,
        VIEW_TYPE_SEEKINFO
    };

    static const char* eventTypeToString[] =
    {
        "Show All Events",
        "Device Mounted",
        "Device UnMounted",
        "Register Stream",
        "UnRegister Stream",
        "Cache Hit",
        "Request Added",
        "Request Canceled",
        "Request Rescheduled",
        "Request Completed",
        "Operation Start",
        "Operation Complete",
        NULL
    };
    static const int eventTypeFromIndex[] = { -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };

    static const char* operationTypeToString[] =
    {
        "All Operations",
        "Invalid",
        "Read",
        "Write",
        "Compressor Read",
        "Compressor Write",
        NULL
    };
    static const int operationTypeFromIndex[] = { -1, 0, 1, 2, 3, 4 };

    static const char* secondsToDisplayString[] =
    {
        "10 Seconds",
        "15 Seconds",
        "30 Seconds",
        "60 Seconds",
        NULL
    };
    static const int secondsFromIndex[] = { 10, 15, 30, 60, 0 };

    static const char* tableLengthToDisplayString[] =
    {
        "All Events",
        " 1K Events",
        " 5K Events",
        "10K Events",
        "50K Events",
        "Playback Start Relative",
        NULL
    };
    static const int tableLengthFromIndex[] = { 0, 1000, 5000, 10000, 50000, -1, 0 };

    static const char* chartTypeToDisplayString[] =
    {
        "Throughput",
        "Seek Count",
        NULL
    };
    static const int chartTypeFromIndex[] = { 0, 1 };

    static const char* seekTypeToString[] =
    {
        "",
        "Skip Position",
        "Switch Streams",
        NULL
    };

    class StreamerDrillerDialogLocal
        : public AZ::UserSettings
    {
    public:
        AZ_RTTI(StreamerDrillerDialogLocal, "{FBC1032F-A1DE-40CB-97E1-8C5014E31850}", AZ::UserSettings);
        AZ_CLASS_ALLOCATOR(StreamerDrillerDialogLocal, AZ::SystemAllocator, 0);

        AZStd::vector<AZ::u8> m_tableColumnStorage;

        static void Reflect(AZ::ReflectContext* context)
        {
            AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
            if (serialize)
            {
                serialize->Class<StreamerDrillerDialogLocal>()
                    ->Field("m_tableColumnStorage", &StreamerDrillerDialogLocal::m_tableColumnStorage)
                    ->Version(1);
            }
        }
    };

    class StreamerDrillerDialogSavedState
        : public AZ::UserSettings
    {
    public:
        AZ_RTTI(StreamerDrillerDialogSavedState, "{F97F6145-10D6-4C7F-87DB-FD268EB0EF21}", AZ::UserSettings);
        AZ_CLASS_ALLOCATOR(StreamerDrillerDialogSavedState, AZ::SystemAllocator, 0);

        int m_viewType;
        bool m_autoZoom;
        float m_manualZoomMin; // if we're not automatically zooming, then we remember the prior zoom to re-apply it
        float m_manualZoomMax;
        int m_chartLengthInSeconds;
        AZStd::string m_chartNameFilter;
        int m_chartOperationFilter;
        int m_chartEventFilter;
        int m_tableEventLimiter;
        FrameNumberType m_frameDeltaLock;

        StreamerDrillerDialogSavedState()
        {
            m_viewType = VIEW_TYPE_THROUGHPUT;
            m_autoZoom = true;
            m_manualZoomMin = 2000000000.0f;
            m_manualZoomMax = -2000000000.0f;
            m_chartLengthInSeconds = 10;
            // -1 := no filter and 0..n := filtered by type
            m_chartOperationFilter = -1;
            m_chartEventFilter = -1;
            m_tableEventLimiter = 0;
            m_frameDeltaLock = 0;
        }

        static void Reflect(AZ::ReflectContext* context)
        {
            AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
            if (serialize)
            {
                serialize->Class<StreamerDrillerDialogSavedState>()
                    ->Field("m_viewType", &StreamerDrillerDialogSavedState::m_viewType)
                    ->Field("m_autoZoom", &StreamerDrillerDialogSavedState::m_autoZoom)
                    ->Field("m_manualZoomMin", &StreamerDrillerDialogSavedState::m_manualZoomMin)
                    ->Field("m_manualZoomMax", &StreamerDrillerDialogSavedState::m_manualZoomMax)
                    ->Field("m_chartLengthInSeconds", &StreamerDrillerDialogSavedState::m_chartLengthInSeconds)
                    ->Field("m_chartNameFilter", &StreamerDrillerDialogSavedState::m_chartNameFilter)
                    ->Field("m_chartOperationFilter", &StreamerDrillerDialogSavedState::m_chartOperationFilter)
                    ->Field("m_chartEventFilter", &StreamerDrillerDialogSavedState::m_chartEventFilter)
                    ->Field("m_tableEventLimiter", &StreamerDrillerDialogSavedState::m_tableEventLimiter)
                    ->Field("m_frameDeltaLock", &StreamerDrillerDialogSavedState::m_frameDeltaLock)
                    ->Version(8);
            }
        }
    };

    //////////////////////////////////////////////////////////////////////////
    // Qt supports A "filter proxy model"
    // you have a normal model
    // and then you wrap that model in a filter proxy model.
    // this allows you to filter the inner model and feed the outer (filtered) model to the view.

    // this particular filter model lets you specify search criteria in the Window or the Message field
    class StreamerFilterModel
        : public QSortFilterProxyModel
    {
    public:
        AZ_CLASS_ALLOCATOR(StreamerFilterModel, AZ::SystemAllocator, 0);
        int m_nameColumn;
        int m_eventColumn;
        int m_operationColumn;
        StreamerDataAggregator* m_dataSource;

        QString m_currentNameFilter;
        int m_currentEventFilter;
        int m_currentOperationFilter;

        FrameNumberType m_frameDeltaLock;

        StreamerFilterModel(StreamerDataAggregator* dataSource, int nameColumn, int eventColumn, int operationColumn, QObject* pParent)
            : QSortFilterProxyModel(pParent)
        {
            m_dataSource = dataSource;
            m_nameColumn = nameColumn;
            m_eventColumn = eventColumn;
            m_operationColumn = operationColumn;
            m_currentEventFilter = eventTypeFromIndex[0];
            m_currentOperationFilter = operationTypeFromIndex[0];
            m_frameDeltaLock = 0;
        }

        void InvalidateFilter()
        {
            invalidateFilter();
        }
        void SetDeltaLock(FrameNumberType lock)
        {
            m_frameDeltaLock = lock;
            invalidateFilter();
        }
        void UpdateNameFilter(const QString& newFilter)
        {
            if (newFilter.compare(m_currentNameFilter) != 0)
            {
                if (m_nameColumn >= 0)
                {
                    m_currentNameFilter = newFilter;
                    invalidateFilter();
                }
            }
        }
        void UpdateEventFilter(int newFilter)
        {
            if (newFilter != m_currentEventFilter)
            {
                if (m_eventColumn >= 0)
                {
                    m_currentEventFilter = newFilter;
                    invalidateFilter();
                }
            }
        }
        void UpdateOperationFilter(int newFilter)
        {
            if (newFilter != m_currentOperationFilter)
            {
                if (m_operationColumn >= 0)
                {
                    m_currentOperationFilter = newFilter;
                    invalidateFilter();
                }
            }
        }

    protected:
        virtual bool filterAcceptsRow(int source_row, const QModelIndex& /*source parent*/) const
        {
            StreamerDrillerLogModel* ptrModel = static_cast<StreamerDrillerLogModel*>(sourceModel());

            if (!ptrModel)
            {
                return true;
            }

            EventNumberType firstIndex = ptrModel->GetAggregator()->GetFirstIndexAtFrame(m_frameDeltaLock);
            EventNumberType rowEventIndex = ptrModel->RowToGlobalEventIndex(source_row);
            if (firstIndex == Driller::kInvalidEventIndex
                || rowEventIndex == Driller::kInvalidEventIndex
                || rowEventIndex < firstIndex)
            {
                return false;
            }

            AZ_Assert(rowEventIndex < static_cast<AZ::s64>(m_dataSource->GetEvents().size()), "EventIndex outside of Events vector size.");

            auto pEvt = m_dataSource->GetEvents()[rowEventIndex];

            if (m_currentNameFilter.size())
            {
                QString sourceName = ptrModel->data(source_row, m_nameColumn, Qt::DisplayRole).toString();
                QString sourceDebugName = ptrModel->data(source_row, m_nameColumn + 1, Qt::DisplayRole).toString();
                if (
                    (!sourceName.contains(m_currentNameFilter, Qt::CaseInsensitive))
                    &&
                    (!sourceDebugName.contains(m_currentNameFilter, Qt::CaseInsensitive))
                    )
                {
                    return false;
                }
            }

            if (m_currentEventFilter != eventTypeFromIndex[0])
            {
                if ((unsigned int)m_currentEventFilter != pEvt->GetEventType())
                {
                    return false;
                }
            }
            if (m_currentOperationFilter != operationTypeFromIndex[0])
            {
                if (pEvt->GetEventType() == Driller::Streamer::SET_OPERATION_COMPLETE)
                {
                    auto completeOp = static_cast<StreamerOperationCompleteEvent*>(pEvt);
                    if (m_currentOperationFilter == completeOp->m_type)
                    {
                        return true;
                    }
                }

                return false;
            }

            return true;
        }
    };

    //////////////////////////////////////////////////////////////////////////
    StreamerAxisFormatter::StreamerAxisFormatter(QObject* pParent)
        : QAbstractAxisFormatter(pParent)
    {
        m_dataType = DATA_TYPE_BYTES_PER_SECOND;
        m_lastAxisValueForScaling = 1.0f;
    }

    QString StreamerAxisFormatter::formatMegabytes(float value)
    {
        // data is in Bytes per Second!
        // so how big is the division size?
        if (m_lastAxisValueForScaling > 499999.0f) // greater than half MB
        {
            return QObject::tr("%1Mb/s").arg(QString::number(value / 1000000.0f, 'f', 1));
        }
        else if (m_lastAxisValueForScaling > 1000.0f) // greater than one K
        {
            if (m_lastAxisValueForScaling > 1000.0f) // whole milliseconds
            {
                return QObject::tr("%1%2").arg(QString::number(value / 1000.0f, 'f', 0)).arg("Kb/s");
            }
            else
            {
                return QObject::tr("%1%2").arg(QString::number(value / 1000.0f, 'f', 1)).arg("Kb/s");
            }
        }
        else if (m_lastAxisValueForScaling > 1.0f)
        {
            return QObject::tr("%1B/s").arg((int)value);
        }
        else
        {
            return QObject::tr("%1B/s").arg(QString::number((double)value, 'f', 2));
        }
    }

    void StreamerAxisFormatter::SetDataType(int type)
    {
        m_dataType = type;
    }

    QString StreamerAxisFormatter::convertAxisValueToText(Charts::AxisType axis, float value, float /*minDisplayedValue*/, float /*maxDisplayedValue*/, float divisionSize)
    {
        if (axis == Charts::AxisType::Vertical)
        {
            m_lastAxisValueForScaling = divisionSize;
            if (m_dataType == DATA_TYPE_BYTES_PER_SECOND)
            {
                return formatMegabytes(value);
            }
            else
            {
                return QObject::tr("%1%2").arg(QString::number(value, 'f', 0)).arg("/s");
            }
        }
        else
        {
            return QString::number((int)value);
        }
    };

    //////////////////////////////////////////////////////////////////////////
    StreamerDrillerDialog::StreamerDrillerDialog(StreamerDataAggregator* aggregator, FrameNumberType atFrame, int profilerIndex)
        : QDialog()
        , m_aggregator(aggregator)
        , m_frame(atFrame)
        , m_viewIndex(profilerIndex)
        , m_isDeltaLocked(false)
        , frameModulo(10)
    {
        m_gui = azcreate(Ui::StreamerDrillerDialog, ());
        m_gui->setupUi(this);

        setAttribute(Qt::WA_DeleteOnClose, true);
        setWindowFlags((this->windowFlags() | Qt::WindowMaximizeButtonHint | Qt::WindowMinimizeButtonHint) & ~(Qt::WindowContextHelpButtonHint));

        setWindowTitle(aggregator->GetDialogTitle());

        show();
        raise();
        activateWindow();
        setFocus();

        m_axisFormatter = aznew StreamerAxisFormatter(this);
        m_gui->widgetDataStrip->SetAxisTextFormatter(m_axisFormatter);

        this->layout()->addWidget(m_gui->widgetTableView);

        m_gui->widgetTableView->horizontalHeader()->setSectionsMovable(true);
        m_gui->widgetTableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
        m_gui->widgetTableView->horizontalHeader()->setStretchLastSection(false);
        m_gui->widgetTableView->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
        m_gui->widgetTableView->verticalHeader()->setStretchLastSection(false);
        m_gui->widgetTableView->verticalHeader()->setSectionsMovable(false);
        m_gui->widgetTableView->verticalHeader()->hide();

        m_ptrOriginalModel = aznew StreamerDrillerLogModel(aggregator, this);
        m_ptrFilter = aznew StreamerFilterModel(m_aggregator, m_gui->widgetTableView->GetNameColumn(), m_gui->widgetTableView->GetEventColumn(), m_gui->widgetTableView->GetOperationColumn(), this);
        m_ptrFilter->setSourceModel(m_ptrOriginalModel);
        m_gui->widgetTableView->setModel(m_ptrFilter);

        // context menu
        actionSelectAll = new QAction(tr("Select All"), this);
        connect(actionSelectAll, SIGNAL(triggered()), this, SLOT(SelectAll()));

        actionSelectNone = new QAction(tr("Select None"), this);
        connect(actionSelectNone, SIGNAL(triggered()), this, SLOT(SelectNone()));

        actionCopySelected = new QAction(tr("Copy Selected Row(s)"), this);
        actionCopySelected->setShortcutContext(Qt::WidgetWithChildrenShortcut);
        connect(actionCopySelected, SIGNAL(triggered()), this, SLOT(CopySelected()));

        actionCopyAll = new QAction(tr("Copy All Rows"), this);
        connect(actionCopyAll, SIGNAL(triggered()), this, SLOT(CopyAll()));

        // context menu for the table
        m_gui->widgetTableView->setContextMenuPolicy(Qt::ActionsContextMenu);
        m_gui->widgetTableView->addAction(actionSelectAll);
        m_gui->widgetTableView->addAction(actionSelectNone);
        m_gui->widgetTableView->addAction(actionCopySelected);
        m_gui->widgetTableView->addAction(actionCopyAll);

        connect(aggregator, SIGNAL(destroyed(QObject*)), this, SLOT(OnDataDestroyed()));

        connect(m_ptrFilter, SIGNAL(rowsAboutToBeInserted(const QModelIndex &, int, int)), m_gui->widgetTableView, SLOT(rowsAboutToBeInserted()));
        connect(m_ptrFilter, SIGNAL(rowsInserted(const QModelIndex &, int, int)), m_gui->widgetTableView, SLOT(rowsInserted()));
        connect(m_gui->nameFilter, SIGNAL(textChanged(const QString &)), this, SLOT(onTextChangeWindowFilter(const QString&)));

        connect(m_ptrFilter, SIGNAL(rowsInserted(const QModelIndex &, int, int)), this, SLOT(UpdateSummary()));
        connect(m_ptrFilter, SIGNAL(rowsRemoved(const QModelIndex &, int, int)), this, SLOT(UpdateSummary()));
        connect(m_ptrFilter, SIGNAL(modelReset()), this, SLOT(UpdateSummary()));

        connect(m_ptrOriginalModel, SIGNAL(rowsInserted(const QModelIndex &, int, int)), this, SLOT(UpdateSummary()));
        connect(m_ptrOriginalModel, SIGNAL(rowsRemoved(const QModelIndex &, int, int)), this, SLOT(UpdateSummary()));
        connect(m_ptrOriginalModel, SIGNAL(modelReset()), this, SLOT(UpdateSummary()));

        connect(m_gui->checkBoxAutoZoom, SIGNAL(toggled(bool)), this, SLOT(OnAutoZoomChange(bool)));

        {
            QMenu* eventMenu = new QMenu(this);

            for (int i = 0; eventTypeToString[i]; ++i)
            {
                eventMenu->addAction(CreateEventFilterAction(eventTypeToString[i], eventTypeFromIndex[i]));
            }

            m_gui->eventTypeFilterButton->setText(eventTypeToString[0]);
            m_gui->eventTypeFilterButton->setMenu(eventMenu);
        }
        {
            QMenu* operationMenu = new QMenu(this);

            for (int i = 0; operationTypeToString[i]; ++i)
            {
                operationMenu->addAction(CreateOperationFilterAction(operationTypeToString[i], operationTypeFromIndex[i]));
            }

            m_gui->operationTypeFilterButton->setText(operationTypeToString[0]);
            m_gui->operationTypeFilterButton->setMenu(operationMenu);
        }
        {
            QMenu* secondsMenu = new QMenu(this);
            for (int i = 0; secondsToDisplayString[i]; ++i)
            {
                secondsMenu->addAction(CreateSecondsMenuAction(secondsToDisplayString[i], secondsFromIndex[i]));
            }

            m_gui->chartLengthButton->setText(secondsToDisplayString[0]);
            m_gui->chartLengthButton->setMenu(secondsMenu);
        }
        {
            QMenu* chartTypeMenu = new QMenu(this);
            for (int i = 0; chartTypeToDisplayString[i]; ++i)
            {
                chartTypeMenu->addAction(CreateChartTypeMenuAction(chartTypeToDisplayString[i], chartTypeFromIndex[i]));
            }

            m_gui->chartTypeButton->setText(chartTypeToDisplayString[0]);
            m_gui->chartTypeButton->setMenu(chartTypeMenu);
        }
        {
            QMenu* tableLengthMenu = new QMenu(this);
            for (int i = 0; tableLengthToDisplayString[i]; ++i)
            {
                tableLengthMenu->addAction(CreateTableLengthMenuAction(tableLengthToDisplayString[i], i));
            }

            m_gui->tableLengthButton->setText(tableLengthToDisplayString[0]);
            m_gui->tableLengthButton->setMenu(tableLengthMenu);
        }

        DrillerMainWindowMessages::Handler::BusConnect(m_aggregator->GetIdentity());
        DrillerEventWindowMessages::Handler::BusConnect(m_aggregator->GetIdentity());

        AZStd::string windowStateStr = AZStd::string::format("STREAMER DATA VIEW WINDOW STATE %i", m_viewIndex);
        m_windowStateCRC = AZ::Crc32(windowStateStr.c_str());
        AZStd::intrusive_ptr<AzToolsFramework::QWidgetSavedState> windowState = AZ::UserSettings::Find<AzToolsFramework::QWidgetSavedState>(m_windowStateCRC, AZ::UserSettings::CT_GLOBAL);
        if (windowState)
        {
            windowState->RestoreGeometry(this);
        }

        AZStd::string tableStateStr = AZStd::string::format("STREAMER TABLE VIEW STATE %i", m_viewIndex);
        m_tableStateCRC = AZ::Crc32(tableStateStr.c_str());
        auto tableState = AZ::UserSettings::Find<StreamerDrillerDialogLocal>(m_tableStateCRC, AZ::UserSettings::CT_GLOBAL);
        if (tableState)
        {
            QByteArray treeData((const char*)tableState->m_tableColumnStorage.data(), (int)tableState->m_tableColumnStorage.size());
            m_gui->widgetTableView->horizontalHeader()->restoreState(treeData);
        }

        AZStd::string dataViewStateStr = AZStd::string::format("STREAMER DATA VIEW STATE %i", m_viewIndex);
        m_dataViewStateCRC = AZ::Crc32(dataViewStateStr.c_str());
        m_persistentState = AZ::UserSettings::CreateFind<StreamerDrillerDialogSavedState>(m_dataViewStateCRC, AZ::UserSettings::CT_GLOBAL);
        ApplyPersistentState();

        FrameChanged(atFrame);
    }

    StreamerDrillerDialog::~StreamerDrillerDialog()
    {
        SaveOnExit();
        azdestroy(m_gui);
    }

    QAction* StreamerDrillerDialog::CreateSecondsMenuAction(QString qs, int seconds)
    {
        QAction* act = new QAction(qs, this);
        act->setObjectName(qs);
        act->setProperty("Seconds", seconds);
        connect(act, SIGNAL(triggered()), this, SLOT(OnSecondsMenu()));
        return act;
    }
    QAction* StreamerDrillerDialog::CreateTableLengthMenuAction(QString qs, int limit)
    {
        QAction* act = new QAction(qs, this);
        act->setObjectName(qs);
        act->setProperty("Limit", limit);
        connect(act, SIGNAL(triggered()), this, SLOT(OnTableLengthMenu()));
        return act;
    }
    QAction* StreamerDrillerDialog::CreateChartTypeMenuAction(QString qs, int dataType)
    {
        QAction* act = new QAction(qs, this);
        act->setObjectName(qs);
        act->setProperty("DataType", dataType);
        connect(act, SIGNAL(triggered()), this, SLOT(OnDataTypeMenu()));
        return act;
    }

    QAction* StreamerDrillerDialog::CreateEventFilterAction(QString qs, int eventType)
    {
        QAction* act = new QAction(qs, this);
        act->setObjectName(qs);
        act->setProperty("EventType", eventType);
        connect(act, SIGNAL(triggered()), this, SLOT(OnEventFilterMenu()));
        return act;
    }

    QAction* StreamerDrillerDialog::CreateOperationFilterAction(QString qs, int operationType)
    {
        QAction* act = new QAction(qs, this);
        act->setObjectName(qs);
        act->setProperty("OperationType", operationType);
        connect(act, SIGNAL(triggered()), this, SLOT(OnOperationFilterMenu()));
        return act;
    }

    void StreamerDrillerDialog::SaveOnExit()
    {
        auto tableState = AZ::UserSettings::CreateFind<StreamerDrillerDialogLocal>(m_tableStateCRC, AZ::UserSettings::CT_GLOBAL);
        if (tableState)
        {
            if (m_gui->widgetTableView && m_gui->widgetTableView->horizontalHeader())
            {
                QByteArray qba = m_gui->widgetTableView->horizontalHeader()->saveState();
                tableState->m_tableColumnStorage.assign((AZ::u8*)qba.begin(), (AZ::u8*)qba.end());
            }
        }

        auto pState = AZ::UserSettings::CreateFind<AzToolsFramework::QWidgetSavedState>(m_windowStateCRC, AZ::UserSettings::CT_GLOBAL);
        if (m_persistentState)
        {
            pState->CaptureGeometry(this);
        }
    }
    void StreamerDrillerDialog::hideEvent(QHideEvent* evt)
    {
        QDialog::hideEvent(evt);
    }
    void StreamerDrillerDialog::closeEvent(QCloseEvent* evt)
    {
        QDialog::closeEvent(evt);
    }
    void StreamerDrillerDialog::OnDataDestroyed()
    {
        deleteLater();
    }

    void StreamerDrillerDialog::onTextChangeWindowFilter(const QString& newText)
    {
        m_ptrFilter->UpdateNameFilter(newText);
        m_persistentState->m_chartNameFilter = newText.toUtf8().data();
    }
    void StreamerDrillerDialog::OnEventFilterMenu()
    {
        QAction* qa = qobject_cast<QAction*>(sender());
        if (qa)
        {
            m_gui->eventTypeFilterButton->setText(qa->objectName());
            int eventType = qa->property("EventType").toInt();
            m_ptrFilter->UpdateEventFilter(eventType);
            m_persistentState->m_chartEventFilter = eventType;
        }
    }
    void StreamerDrillerDialog::OnOperationFilterMenu()
    {
        QAction* qa = qobject_cast<QAction*>(sender());
        if (qa)
        {
            m_gui->operationTypeFilterButton->setText(qa->objectName());
            int operationType = qa->property("OperationType").toInt();
            m_ptrFilter->UpdateOperationFilter(operationType);
            m_persistentState->m_chartOperationFilter = operationType;
        }
    }
    void StreamerDrillerDialog::OnSecondsMenu()
    {
        QAction* qa = qobject_cast<QAction*>(sender());
        if (qa)
        {
            m_gui->chartLengthButton->setText(qa->objectName());
            int seconds = qa->property("Seconds").toInt();
            SetChartLength(seconds);
        }
    }
    void StreamerDrillerDialog::OnTableLengthMenu()
    {
        QAction* qa = qobject_cast<QAction*>(sender());
        if (qa)
        {
            int limit = qa->property("Limit").toInt();
            OnTableLengthMenu(limit);
        }
    }
    void StreamerDrillerDialog::OnTableLengthMenu(int limit)
    {
        if (tableLengthFromIndex[limit] >= 0)
        {
            m_gui->tableLengthButton->setText(tableLengthToDisplayString[limit]);
            m_persistentState->m_tableEventLimiter = limit;
            m_isDeltaLocked = false;
            SetTableLengthLimit(tableLengthFromIndex[limit]);
            m_ptrFilter->SetDeltaLock(0);
        }
        else
        {
            m_gui->tableLengthButton->setText(QString("Delta:%1").arg(m_frame));
            m_isDeltaLocked = true;
            m_ptrFilter->SetDeltaLock(m_persistentState->m_frameDeltaLock);
        }

        BuildChart(m_frame, m_persistentState->m_viewType, m_persistentState->m_chartLengthInSeconds);
    }
    void StreamerDrillerDialog::OnDataTypeMenu()
    {
        QAction* qa = qobject_cast<QAction*>(sender());
        if (qa)
        {
            OnDataTypeMenu(qa->property("DataType").toInt());
        }
    }
    void StreamerDrillerDialog::OnDataTypeMenu(int type)
    {
        m_gui->chartTypeButton->setText(chartTypeToDisplayString[type]);
        m_persistentState->m_viewType = type;
        SetChartType(type);
        m_axisFormatter->SetDataType(type);
    }
    void StreamerDrillerDialog::OnAutoZoomChange(bool newValue)
    {
        if (!newValue)
        {
            m_persistentState->m_autoZoom = false;
            m_gui->widgetDataStrip->GetWindowRange(Charts::AxisType::Vertical, m_persistentState->m_manualZoomMin, m_persistentState->m_manualZoomMax);
        }
        else
        {
            m_persistentState->m_autoZoom = true;
            m_persistentState->m_manualZoomMin = 2000000000.0f;
            m_persistentState->m_manualZoomMax = -2000000000.0f;
        }
        BuildChart(m_frame, m_persistentState->m_viewType, m_persistentState->m_chartLengthInSeconds);
    }
    void StreamerDrillerDialog::SetChartLength(int newLength)
    {
        m_persistentState->m_chartLengthInSeconds = newLength;
        BuildChart(m_frame, m_persistentState->m_viewType, newLength);
    }
    void StreamerDrillerDialog::SetChartType(int newType)
    {
        BuildChart(m_frame, newType, m_persistentState->m_chartLengthInSeconds);
    }
    void StreamerDrillerDialog::SetTableLengthLimit(int limit)
    {
        m_ptrOriginalModel->SetLengthLimit(limit);
        m_ptrFilter->InvalidateFilter();
    }
    int StreamerDrillerDialog::GetViewType()
    {
        return m_persistentState->m_viewType;
    }

    // Backing code to the context menu
    void StreamerDrillerDialog::SelectAll()
    {
        m_gui->widgetTableView->selectAll();
    }

    void StreamerDrillerDialog::SelectNone()
    {
        m_gui->widgetTableView->clearSelection();
    }

    QString StreamerDrillerDialog::ConvertRowToText(const QModelIndex& row)
    {
        auto pModel = m_ptrFilter;

        if (!pModel)
        {
            return QString();
        }

        int columnCount = pModel->columnCount();
        QString finalString = "";

        QModelIndex sourceRow = m_ptrFilter->mapToSource(row);

        for (int column = 0; column < columnCount; ++column)
        {
            QString displayString = m_ptrOriginalModel->data(sourceRow.row(), column, Qt::DisplayRole).toString();
            if ((column != 0) && (finalString.length() > 0))
            {
                finalString += "; ";
            }
            if (displayString.length())
            {
                finalString += displayString.toUtf8().data();
            }
            else
            {
                // must enforce some length even on empty strings in the table
                // so that comma-delimiters output properly
                finalString += " ";
            }
        }
        finalString += "\n";

        return finalString;
    }

    void StreamerDrillerDialog::CopySelected()
    {
        auto pModel = m_ptrFilter;
        if (!pModel)
        {
            return;
        }

        AZStd::string accumulator;
        QItemSelectionModel* selectionModel = m_gui->widgetTableView->selectionModel();
        QModelIndexList indices = selectionModel->selectedRows();
        for (QModelIndexList::iterator iter = indices.begin(); iter != indices.end(); ++iter)
        {
            QString res = this->ConvertRowToText(*iter);
            accumulator += res.toUtf8().data();
        }

        if (accumulator.size())
        {
            QClipboard* clipboard = QApplication::clipboard();
            if (clipboard)
            {
                clipboard->setText(accumulator.c_str());
            }
        }
    }

    void StreamerDrillerDialog::CopyAll()
    {
        auto pModel = m_ptrFilter;
        if (!pModel)
        {
            return;
        }

        QString finalString = "";

        int numRows = pModel->rowCount();
        for (int rowIdx = 0; rowIdx < numRows; ++rowIdx)
        {
            QModelIndex idx = pModel->index(rowIdx, 0);
            finalString += this->ConvertRowToText(idx);
        }

        QClipboard* clipboard = QApplication::clipboard();

        if (clipboard)
        {
            clipboard->setText(finalString);
        }
    }

    void StreamerDrillerDialog::ApplyPersistentState()
    {
        onTextChangeWindowFilter(m_persistentState->m_chartNameFilter.c_str());
        OnDataTypeMenu(m_persistentState->m_viewType);

        m_gui->tableLengthButton->setText(tableLengthToDisplayString[m_persistentState->m_tableEventLimiter]);
        SetTableLengthLimit(tableLengthFromIndex[m_persistentState->m_tableEventLimiter]);

        if (m_isDeltaLocked)
        {
            m_gui->tableLengthButton->setText(QString("Delta:%1").arg(m_persistentState->m_frameDeltaLock));
            m_ptrFilter->SetDeltaLock(m_persistentState->m_frameDeltaLock);
        }
        else
        {
            m_ptrFilter->SetDeltaLock(0);
        }

        m_gui->checkBoxAutoZoom->setChecked(m_persistentState->m_autoZoom);
        OnAutoZoomChange(m_persistentState->m_autoZoom);

        for (int i = 0; secondsFromIndex[i]; ++i)
        {
            if (m_persistentState->m_chartLengthInSeconds == secondsFromIndex[i])
            {
                m_gui->chartLengthButton->setText(secondsToDisplayString[i]);
                break;
            }
        }

        BuildChart(m_frame, m_persistentState->m_viewType, m_persistentState->m_chartLengthInSeconds);   // full seconds, 60 frames per entry on the chart, modulo even seconds

        UpdateSummary();
    }

    void StreamerDrillerDialog::ApplySettingsFromWorkspace(WorkspaceSettingsProvider* provider)
    {
        AZStd::string workspaceStateStr = AZStd::string::format("STREAMER DATA VIEW WORKSPACE STATE %i", m_viewIndex);
        AZ::u32 workspaceStateCRC = AZ::Crc32(workspaceStateStr.c_str());

        StreamerDrillerDialogSavedState* workspace = provider->FindSetting<StreamerDrillerDialogSavedState>(workspaceStateCRC);
        if (workspace)
        {
            *m_persistentState = *workspace;
        }
    }

    void StreamerDrillerDialog::ActivateWorkspaceSettings(WorkspaceSettingsProvider*)
    {
        ApplyPersistentState();
    }

    void StreamerDrillerDialog::FrameChanged(FrameNumberType frame)
    {
        m_frame = frame;
        BuildChart(m_frame, m_persistentState->m_viewType, m_persistentState->m_chartLengthInSeconds);   // full seconds, 60 frames per entry on the chart, modulo even seconds
    }

    void StreamerDrillerDialog::PlaybackLoopBeginChanged(FrameNumberType frame)
    {
        m_persistentState->m_frameDeltaLock = frame;
        FrameNumberType lockedFrame = m_isDeltaLocked ? m_persistentState->m_frameDeltaLock : 0;
        m_ptrFilter->SetDeltaLock(lockedFrame);
        BuildChart(m_frame, m_persistentState->m_viewType, m_persistentState->m_chartLengthInSeconds);   // full seconds, 60 frames per entry on the chart, modulo even seconds
    }

    void StreamerDrillerDialog::BuildChart(FrameNumberType atFrame, int viewType, int howFar)
    {
        BuildAllLabels(atFrame, viewType);

        FrameNumberType lockedFrame = m_isDeltaLocked ? m_persistentState->m_frameDeltaLock : 0;
        const char* vAxisLabel[] = {"Transfer", "Seek"};

        m_gui->widgetDataStrip->Reset();
        FrameNumberType flooredFrame = (atFrame + frameModulo - 1) / frameModulo;
        float calculatedFrame = (float)(flooredFrame - howFar >= 0 ? flooredFrame - howFar : 0);
        m_gui->widgetDataStrip->AddAxis("Time", calculatedFrame, (float)calculatedFrame + howFar, true, true);
        m_gui->widgetDataStrip->AddAxis(vAxisLabel[viewType], m_persistentState->m_manualZoomMin, m_persistentState->m_manualZoomMax, false, false);
        int channelID = m_gui->widgetDataStrip->AddChannel("ThroughputOrSeeks");
        m_gui->widgetDataStrip->SetChannelStyle(channelID, StripChart::Channel::STYLE_CONNECTED_LINE);
        m_gui->widgetDataStrip->SetChannelColor(channelID, Qt::green);

        FrameNumberType currentFrame = atFrame - (atFrame % frameModulo) - 1;
        float accumulator = 0;

        while (howFar > 0 && currentFrame >= (lockedFrame - frameModulo))
        {
            FrameNumberType displayFrame = currentFrame;
            float thisSecond = 0.0f;

            if (viewType == VIEW_TYPE_THROUGHPUT)
            {
                while ((currentFrame % frameModulo) && (currentFrame >= 0))
                {
                    thisSecond += m_aggregator->ThroughputAtFrame(currentFrame);
                    accumulator += m_aggregator->ThroughputAtFrame(currentFrame);
                    --currentFrame;
                }
                ;
                thisSecond += m_aggregator->ThroughputAtFrame(currentFrame);
                accumulator += m_aggregator->ThroughputAtFrame(currentFrame);
            }
            else if (viewType == VIEW_TYPE_SEEKINFO)
            {
                while ((currentFrame % frameModulo) && (currentFrame >= 0))
                {
                    thisSecond += m_aggregator->SeeksAtFrame(currentFrame);
                    accumulator += m_aggregator->SeeksAtFrame(currentFrame);
                    --currentFrame;
                }
                ;
                thisSecond += m_aggregator->SeeksAtFrame(currentFrame);
                accumulator += m_aggregator->SeeksAtFrame(currentFrame);
            }

            m_gui->widgetDataStrip->AddData(channelID, displayFrame / frameModulo, (float)displayFrame / (float)frameModulo, thisSecond * (60.0f / (float)frameModulo));

            --currentFrame;
            --howFar;
        }

        if (m_persistentState->m_autoZoom)
        {
            m_gui->widgetDataStrip->ZoomExtents(Charts::AxisType::Vertical);
        }
        else
        {
            m_gui->widgetDataStrip->ZoomManual(Charts::AxisType::Vertical, m_persistentState->m_manualZoomMin, m_persistentState->m_manualZoomMax);
        }
    }

    void StreamerDrillerDialog::BuildAllLabels(FrameNumberType atFrame, int viewType)
    {
        FrameNumberType currentFrame = atFrame;
        float accumulateDelta = 0;
        FrameNumberType lockedFrame = m_isDeltaLocked ? m_persistentState->m_frameDeltaLock : 0;

        while (currentFrame >= lockedFrame)
        {
            if (viewType == VIEW_TYPE_THROUGHPUT)
            {
                accumulateDelta += m_aggregator->ThroughputAtFrame(currentFrame);
            }
            else if (viewType == VIEW_TYPE_SEEKINFO)
            {
                accumulateDelta += m_aggregator->SeeksAtFrame(currentFrame);
            }

            --currentFrame;
        }

        QString deltaString = UpdateDeltaLabel(accumulateDelta);

        float accumulateTime = float(atFrame - lockedFrame + 1) / 60.0f;
        QString timeString = QString("T=%1s").arg(QString::number(accumulateTime, 'f', 1));

        float accumulateAverage = 0.0f;
        if (atFrame >= lockedFrame)
        {
            if (viewType == VIEW_TYPE_THROUGHPUT)
            {
                accumulateAverage = m_aggregator->ThroughputAtFrame(atFrame);
            }
            else if (viewType == VIEW_TYPE_SEEKINFO)
            {
                accumulateAverage = m_aggregator->SeeksAtFrame(atFrame);
            }
        }
        QString averageString = UpdateAverageLabel(accumulateAverage);
        QString eventsString = UpdateSummary();

        QString finalString = eventsString + QString(" ") + deltaString + QString(" ") + averageString + QString(" ") + timeString;
        m_gui->summaryLabel->setText(finalString);

        m_gui->summaryLabel->update();
    }

    QString StreamerDrillerDialog::UpdateSummary()
    {
        int filterRows = m_ptrFilter->rowCount();
        int originalRows = m_ptrOriginalModel->rowCount();

        return QString("[%1 / %2]").arg(filterRows).arg(originalRows);
    }

    QString StreamerDrillerDialog::UpdateDeltaLabel(float accumulator)
    {
        if (m_persistentState->m_viewType == VIEW_TYPE_THROUGHPUT)
        {
            QString formattedBytes = FormatMegabytes(accumulator);
            return QString("Data=%1").arg(formattedBytes);
        }
        else
        {
            return QString("Seek=%1").arg(QString::number(accumulator, 'f', 0));
        }
    }

    QString StreamerDrillerDialog::UpdateAverageLabel(float accumulator)
    {
        if (m_persistentState->m_viewType == VIEW_TYPE_THROUGHPUT)
        {
            QString formattedBytes = FormatMegabytes(accumulator);
            return QString("Now=%1").arg(formattedBytes);
        }
        else
        {
            return QString("Seek=%1").arg(QString::number(accumulator, 'f', 0));
        }
    }

    QString StreamerDrillerDialog::FormatMegabytes(float value)
    {
        // data is in Bytes
        // so how big is the division size?
        if (value > 499999.0f) // greater than half MB
        {
            return QObject::tr("%1Mb").arg(QString::number(value / 1000000.0f, 'f', 1));
        }
        else if (value > 1000.0f) // greater than one K
        {
            if (value > 1000.0f) // whole milliseconds
            {
                return QObject::tr("%1%2").arg(QString::number(value / 1000.0f, 'f', 0)).arg("Kb");
            }
            else
            {
                return QObject::tr("%1%2").arg(QString::number(value / 1000.0f, 'f', 1)).arg("Kb");
            }
        }
        else if (value > 1.0f)
        {
            return QObject::tr("%1B").arg((int)value);
        }
        else
        {
            return QObject::tr("%1B").arg(QString::number((double)value, 'f', 2));
        }
    }

    void StreamerDrillerDialog::EventFocusChanged(EventNumberType /*eventIdx*/)
    {
    }

    void StreamerDrillerDialog::SaveSettingsToWorkspace(WorkspaceSettingsProvider* provider)
    {
        AZStd::string workspaceStateStr = AZStd::string::format("STREAMER DATA VIEW WORKSPACE STATE %i", m_viewIndex);
        AZ::u32 workspaceStateCRC = AZ::Crc32(workspaceStateStr.c_str());
        {
            StreamerDrillerDialogSavedState* workspace = provider->CreateSetting<StreamerDrillerDialogSavedState>(workspaceStateCRC);
            if (workspace)
            {
                *workspace = *m_persistentState;
            }
        }
    }

    void StreamerDrillerDialog::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            StreamerDrillerDialogSavedState::Reflect(context);
            StreamerDrillerDialogLocal::Reflect(context);
        }
    }

    //////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////
    // StreamerDrillerLogModel
    //////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////
    StreamerDrillerLogModel::StreamerDrillerLogModel(StreamerDataAggregator* data, QObject* pParent)
        : QAbstractTableModel(pParent)
        , m_data(data)
        , m_lastShownEvent(-1)
        , m_lengthLimit(0)
    {
        connect(data, SIGNAL(OnDataCurrentEventChanged()), this, SLOT(OnDataCurrentEventChanged()));
        connect(data, SIGNAL(OnDataAddEvent()), this, SLOT(OnDataAddEvent()));

        m_lastShownEvent = m_data->GetCurrentEvent();
    }

    StreamerDrillerLogModel::~StreamerDrillerLogModel()
    {
    }

    void StreamerDrillerLogModel::OnDataCurrentEventChanged()
    {
        // real source data changes operate in real space, not the window of the length limited
        int limitStore = m_lengthLimit;
        SetLengthLimit(0);

        AZ::s64 currentEvent = m_data->GetCurrentEvent();
        // NOTE: we add +1 to all events, because we are EXECUTING the current event (so it must be shown)
        if (m_lastShownEvent > currentEvent)
        {
            // remove rows
            beginRemoveRows(QModelIndex(), (int)currentEvent + 1, (int)m_lastShownEvent);
            endRemoveRows();
        }
        else if (m_lastShownEvent < currentEvent)
        {
            // add rows
            beginInsertRows(QModelIndex(), (int)m_lastShownEvent + 1, (int)currentEvent);
            endInsertRows();
        }
        m_lastShownEvent = currentEvent;

        SetLengthLimit(limitStore);
    }

    void StreamerDrillerLogModel::OnDataAddEvent()
    {
    }

    void StreamerDrillerLogModel::SetLengthLimit(int limit)
    {
        beginResetModel();
        m_lengthLimit = limit;
        endResetModel();
    }

    int StreamerDrillerLogModel::rowCount(const QModelIndex&) const
    {
        AZ::s64 currentEvent = m_data->GetCurrentEvent();
        if (m_lengthLimit && (m_lengthLimit < (currentEvent + 1)))
        {
            return m_lengthLimit;
        }
        return (int)currentEvent + 1;
    }
    int StreamerDrillerLogModel::columnCount(const QModelIndex&) const
    {
        return SDM_TOTAL;
    }
    Qt::ItemFlags StreamerDrillerLogModel::flags(const QModelIndex& index) const
    {
        if (!index.isValid())
        {
            return Qt::ItemIsEnabled;
        }

        return QAbstractItemModel::flags(index);
    }

    QVariant StreamerDrillerLogModel::headerData (int section, Qt::Orientation orientation, int role) const
    {
        if (role == Qt::DisplayRole)
        {
            if (orientation == Qt::Horizontal)
            {
                return QVariant(SDM_STRING[section]);
            }
            // purposefully ignoring the ::Vertical orientation as part of an optimiztion
        }

        return QVariant();
    }

    EventNumberType StreamerDrillerLogModel::RowToGlobalEventIndex(int row)
    {
        EventNumberType currentEvent = m_data->GetCurrentEvent();
        if (m_lengthLimit && (m_lengthLimit < (currentEvent + 1)))
        {
            row = row + (int)currentEvent - m_lengthLimit + 1;
        }

        return row;
    }

    QVariant StreamerDrillerLogModel::data(const QModelIndex& index, int role) const
    {
        return data(index.row(), index.column(), role);
    }

    QVariant StreamerDrillerLogModel::data(int row, int column, int role) const
    {
        AZ::s64 currentEvent = m_data->GetCurrentEvent();
        if (m_lengthLimit && (m_lengthLimit < (currentEvent + 1)))
        {
            row = row + (int)currentEvent - m_lengthLimit + 1;
        }

        return data(m_data->GetEvents()[row], row, column, role);
    }

    QVariant StreamerDrillerLogModel::data(DrillerEvent* event, int row, int column, int role) const
    {
        using namespace AZ::Debug;

        const bool dummyCompressedFlag = false; // placeholder until set from stream info

        if (role == Qt::DisplayRole)
        {
            // COLUMN -------------------------------------------------------------
            if (column == SDM_NAME)
            {
                switch (event->GetEventType())
                {
                case Driller::Streamer::SET_DEVICE_MOUNTED:
                {
                    return QVariant(QString(static_cast<StreamerMountDeviceEvent*>(event)->m_deviceData.m_name));
                    break;
                }
                case Driller::Streamer::SET_DEVICE_UNMOUNTED:
                {
                    return QVariant(QString(static_cast<StreamerUnmountDeviceEvent*>(event)->m_unmountedDeviceData->m_name));
                    break;
                }
                case Driller::Streamer::SET_REGISTER_STREAM:
                {
                    return QVariant(QString(static_cast<StreamerRegisterStreamEvent*>(event)->m_streamData.m_name));
                    break;
                }
                case Driller::Streamer::SET_UNREGISTER_STREAM:
                {
                    auto depEvt = static_cast<StreamerUnregisterStreamEvent*>(event);
                    if (depEvt && depEvt->m_removedStreamData)
                    {
                        return QVariant(QString(depEvt->m_removedStreamData->m_name));
                    }
                    return QVariant(QString(m_data->GetFilenameFromStreamId(row, depEvt->m_streamId)));
                    break;
                }
                case Driller::Streamer::SET_ADD_REQUEST:
                {
                    auto depEvt = static_cast<StreamerAddRequestEvent*>(event);
                    return QVariant(QString(m_data->GetFilenameFromStreamId(row, depEvt->m_requestData.m_streamId)));
                    break;
                }
                case Driller::Streamer::SET_CANCEL_REQUEST:
                {
                    return QVariant(QString(m_data->GetFilenameFromStreamId(row, static_cast<StreamerCancelRequestEvent*>(event)->m_cancelledRequestData->m_streamId)));
                    break;
                }
                case Driller::Streamer::SET_RESCHEDULE_REQUEST:
                {
                    return QVariant(QString(m_data->GetFilenameFromStreamId(row, static_cast<StreamerRescheduleRequestEvent*>(event)->m_rescheduledRequestData->m_streamId)));
                    break;
                }
                case Driller::Streamer::SET_COMPLETE_REQUEST:
                {
                    auto depEvt = static_cast<StreamerCompleteRequestEvent*>(event);
                    return QVariant(QString(m_data->GetFilenameFromStreamId(row, depEvt->m_removedRequest->m_streamId)));
                    break;
                }
                case Driller::Streamer::SET_OPERATION_START:
                {
                    auto depEvt = static_cast<StreamerOperationStartEvent*>(event);
                    return QVariant(QString("%1").arg(m_data->GetFilenameFromStreamId(row, depEvt->m_streamId)));
                    break;
                }
                case Driller::Streamer::SET_OPERATION_COMPLETE:
                {
                    auto depEvt = static_cast<StreamerOperationCompleteEvent*>(event);
                    return QVariant(QString("%1").arg(m_data->GetFilenameFromStreamId(row, depEvt->m_streamId)));
                    break;
                }
                }
            }
            // COLUMN -------------------------------------------------------------
            if (column == SDM_DEBUG_NAME)
            {
                switch (event->GetEventType())
                {
                case Driller::Streamer::SET_ADD_REQUEST:
                {
                    auto depEvt = static_cast<StreamerAddRequestEvent*>(event);
                    if (depEvt->m_requestData.m_debugName)
                    {
                        return QVariant(QString(depEvt->m_requestData.m_debugName));
                    }
                    break;
                }
                case Driller::Streamer::SET_CANCEL_REQUEST:
                {
                    return QVariant(QString(static_cast<StreamerCancelRequestEvent*>(event)->m_cancelledRequestData->m_debugName));
                    break;
                }
                case Driller::Streamer::SET_RESCHEDULE_REQUEST:
                {
                    return QVariant(QString(static_cast<StreamerRescheduleRequestEvent*>(event)->m_rescheduledRequestData->m_debugName));
                    break;
                }
                case Driller::Streamer::SET_COMPLETE_REQUEST:
                {
                    auto depEvt = static_cast<StreamerCompleteRequestEvent*>(event);
                    if (depEvt->m_removedRequest->m_debugName)
                    {
                        return QVariant(QString(depEvt->m_removedRequest->m_debugName));
                    }
                    break;
                }
                case Driller::Streamer::SET_OPERATION_START:
                {
                    auto depEvt = static_cast<StreamerOperationStartEvent*>(event);
                    return QVariant(QString("%1").arg(m_data->GetDebugNameFromStreamId(row, depEvt->m_streamId)));
                    break;
                }
                case Driller::Streamer::SET_OPERATION_COMPLETE:
                {
                    auto depEvt = static_cast<StreamerOperationCompleteEvent*>(event);
                    return QVariant(QString("%1").arg(m_data->GetDebugNameFromStreamId(row, depEvt->m_streamId)));
                    break;
                }
                }
            }
            // COLUMN -------------------------------------------------------------
            else if (column == SDM_EVENT_TYPE)
            {
                return QVariant(QString(eventTypeToString[ event->GetEventType() + 1 ]));
            }
            // COLUMN -------------------------------------------------------------
            else if (column == SDM_OPERATION)
            {
                switch (event->GetEventType())
                {
                case Driller::Streamer::SET_OPERATION_COMPLETE:
                {
                    return QVariant(QString(operationTypeToString[ static_cast<StreamerOperationCompleteEvent*>(event)->m_type + 1 ]));
                    break;
                }
                }
            }
            // COLUMN -------------------------------------------------------------
            else if (column == SDM_DELTA_TIME)
            {
                switch (event->GetEventType())
                {
                case Driller::Streamer::SET_OPERATION_START:
                {
                    auto depEvt = static_cast<StreamerOperationStartEvent*>(event);

                    StreamerDataAggregator::SeekEventType seekType = m_data->GetSeekType(depEvt->GetGlobalEventId());
                    QString seekNotice = seekTypeToString[seekType];

                    return QVariant(QString("%1").arg(seekNotice));
                    break;
                }
                case Driller::Streamer::SET_ADD_REQUEST:
                {
                    // this is a delta between this new request and a previous completion
                    // useful to determine slack time in incoming request sequences
                    auto depEvt = static_cast<StreamerAddRequestEvent*>(event);

                    int backtrackRow = row - 1;
                    while (backtrackRow >= 0)
                    {
                        DrillerEvent* pastEvent = m_data->GetEvents()[backtrackRow];
                        if (pastEvent->GetEventType() == Driller::Streamer::SET_COMPLETE_REQUEST)
                        {
                            auto olderRequest = static_cast<StreamerCompleteRequestEvent*>(pastEvent);
                            return QVariant(QString("%L1").arg(depEvt->m_timeStamp - olderRequest->m_timeStamp));
                        }

                        --backtrackRow;
                    }
                    break;
                }
                case Driller::Streamer::SET_COMPLETE_REQUEST:
                {
                    auto thisEvent = static_cast<StreamerCompleteRequestEvent*>(event);

                    int backtrackRow = row - 1;
                    while (backtrackRow >= 0)
                    {
                        DrillerEvent* pastEvent = m_data->GetEvents()[backtrackRow];
                        if (pastEvent->GetEventType() == Driller::Streamer::SET_ADD_REQUEST)
                        {
                            auto originalRequest = static_cast<StreamerAddRequestEvent*>(pastEvent);
                            if (thisEvent->m_requestId == originalRequest->m_requestData.m_id)
                            {
                                return QVariant(QString("%L1").arg(thisEvent->m_timeStamp - originalRequest->m_timeStamp));
                            }
                        }

                        --backtrackRow;
                    }
                    break;
                }
                case Driller::Streamer::SET_OPERATION_COMPLETE:
                {
                    auto thisEvent = static_cast<StreamerOperationCompleteEvent*>(event);

                    int backtrackRow = row - 1;
                    while (backtrackRow >= 0)
                    {
                        DrillerEvent* pastEvent = m_data->GetEvents()[backtrackRow];
                        if (pastEvent->GetEventType() == Driller::Streamer::SET_OPERATION_START)
                        {
                            auto originalOperation = static_cast<StreamerOperationStartEvent*>(pastEvent);
                            if (thisEvent->m_streamId == originalOperation->m_streamId)
                            {
                                return QVariant(QString("%L1").arg(thisEvent->m_timeStamp - originalOperation->m_timeStamp));
                            }
                        }

                        --backtrackRow;
                    }
                    break;
                }
                }
            }
            // COLUMN -------------------------------------------------------------
            else if (column == SDM_DATA_TRANSFER)
            {
                switch (event->GetEventType())
                {
                case Driller::Streamer::SET_OPERATION_COMPLETE:
                {
                    auto socEvent = static_cast<StreamerOperationCompleteEvent*>(event);

                    if (dummyCompressedFlag)
                    {
                        if (static_cast<StreamerDataAggregator::TransferEventType>(socEvent->m_type) == StreamerDataAggregator::TRANSFER_EVENT_COMPRESSOR_READ || static_cast<StreamerDataAggregator::TransferEventType>(socEvent->m_type) == StreamerDataAggregator::TRANSFER_EVENT_COMPRESSOR_WRITE)
                        {
                            return QVariant(QString("%1").arg(socEvent->m_bytesTransferred));
                        }
                    }
                    else
                    {
                        return QVariant(QString("%1").arg(socEvent->m_bytesTransferred));
                    }

                    break;
                }
                }
            }
            // COLUMN -------------------------------------------------------------
            else if (column == SDM_READ_SIZE)
            {
                switch (event->GetEventType())
                {
                case Driller::Streamer::SET_ADD_REQUEST:
                {
                    return QVariant(QString("%1").arg(static_cast<StreamerAddRequestEvent*>(event)->m_requestData.m_size));
                    break;
                }
                case Driller::Streamer::SET_COMPLETE_REQUEST:
                {
                    return QVariant(QString("%1").arg(static_cast<StreamerCompleteRequestEvent*>(event)->m_removedRequest->m_size));
                    break;
                }
                }
            }
            else if (column == SDM_OFFSET)
            {
                switch (event->GetEventType())
                {
                case Driller::Streamer::SET_ADD_REQUEST:
                {
                    StreamerAddRequestEvent* actualData = static_cast<StreamerAddRequestEvent*>(event);
                    return QVariant(QString::number(actualData->m_requestData.m_offset));
                }
                break;
                case Driller::Streamer::SET_OPERATION_START:
                {
                    StreamerOperationStartEvent* actualData = static_cast<StreamerOperationStartEvent*>(event);
                    return QVariant(QString::number(actualData->m_operation.m_offset));
                }
                break;
                }
            }
        }

        return QVariant();
    }
}
