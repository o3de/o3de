/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "StandaloneTools_precompiled.h"

#include <AzCore/Debug/Profiler.h>
#include <AzCore/Math/MathUtils.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/sort.h>
#include <AzCore/std/time.h>

#include <AzToolsFramework/UI/UICore/QTreeViewStateSaver.hxx>
#include <AzToolsFramework/UI/UICore/QWidgetSavedState.h>

#include <qcombobox.h>
#include <QMouseEvent>

#include "ReplicaDataView.hxx"
#include <Source/Driller/Replica/moc_ReplicaDataView.cpp>

#include "BaseDetailView.h"
#include "ReplicaChunkTypeDetailView.h"
#include "ReplicaChunkUsageDataContainers.h"
#include "ReplicaDataAggregator.hxx"
#include "ReplicaDataEvents.h"
#include "ReplicaDetailView.h"
#include "ReplicaOperationTelemetryEvent.h"
#include "ReplicaUsageDataContainers.h"

#include "Source/Driller/DrillerMainWindowMessages.h"
#include "Source/Driller/DrillerOperationTelemetryEvent.h"
#include "Source/Driller/Replica/OverallReplicaDetailView.hxx"
#include "Source/Driller/Replica/ReplicaDisplayHelpers.h"
#include "Source/Driller/Workspaces/Workspace.h"

namespace Driller
{
    //////////////////////////////
    // ReplicaDataViewSavedState
    //////////////////////////////

    class ReplicaDataViewSavedState
        : public AZ::UserSettings
    {
    public:
        AZ_RTTI(ReplicaDataViewSavedState, "{8C5CA0D3-CD56-4972-83E5-2A7D3217E8FE}", AZ::UserSettings);
        AZ_CLASS_ALLOCATOR(ReplicaDataViewSavedState, AZ::SystemAllocator, 0);

        int m_displayTimeType;
        int m_displayDataType;        
        int m_displayRange;
        int m_bandwidthUsageDisplayType;
        int m_tableFilterType;        

        ReplicaDataViewSavedState()
            : m_displayDataType(ReplicaDataView::DDT_START + 1)
            , m_displayRange(30)
            , m_bandwidthUsageDisplayType(ReplicaDisplayTypes::BUDT_START + 1)
            , m_tableFilterType(ReplicaDataView::TFT_START + 1)
        {
        }

        void CopyStateFrom(const ReplicaDataViewSavedState* source)
        {
            m_displayTimeType = source->m_displayTimeType;
            m_displayDataType = source->m_displayDataType;
            m_displayRange = source->m_displayRange;
            m_bandwidthUsageDisplayType = source->m_bandwidthUsageDisplayType;
            m_tableFilterType = source->m_tableFilterType;
        }

        static void Reflect(AZ::ReflectContext* context)
        {
            AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
            if (serialize)
            {
                serialize->Class<ReplicaDataViewSavedState>()
                    ->Field("m_displayDataType", &ReplicaDataViewSavedState::m_displayDataType)
                    ->Field("m_displayRange", &ReplicaDataViewSavedState::m_displayRange)
                    ->Field("m_bandwidthUsageDisplayType",&ReplicaDataViewSavedState::m_bandwidthUsageDisplayType)
                    ->Field("m_tableFilterType", &ReplicaDataViewSavedState::m_tableFilterType)
                    ->Version(4);
            }
        }
    };

    ////////////////////////////////////////
    // ReplicaDataViewTableModelSavedState
    ////////////////////////////////////////

    class ReplicaDataViewTableModelSavedState
        : public AZ::UserSettings
    {
    public:
        AZ_RTTI(ReplicaDataViewTableModelSavedState, "{36103E46-2503-4EEE-BA4B-2650E25A5B26}", AZ::UserSettings);
        AZ_CLASS_ALLOCATOR(ReplicaDataViewTableModelSavedState, AZ::SystemAllocator, 0);

        AZStd::vector<AZ::u8> m_treeColumnStorage;

        static void Reflect(AZ::ReflectContext* context)
        {
            AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);

            if (serialize)
            {
                serialize->Class<ReplicaDataViewTableModelSavedState>()
                    ->Field("m_treeColumnStorage", &ReplicaDataViewTableModelSavedState::m_treeColumnStorage)
                    ->Version(1);
            }
        }
    };

    //////////////////////////////////////
    // ReplicaDataViewSplitterSavedState
    //////////////////////////////////////

    class ReplicaDataViewSplitterSavedState
        : public AZ::UserSettings
    {
    public:
        AZ_RTTI(ReplicaDataViewSplitterSavedState, "{E698D9E8-D8E9-4115-87E7-2BEEBE5F7FB3}", AZ::UserSettings);
        AZ_CLASS_ALLOCATOR(ReplicaDataViewSplitterSavedState, AZ::SystemAllocator, 0);

        AZStd::vector<AZ::u8> m_splitterSavedState;

        static void Reflect(AZ::ReflectContext* context)
        {
            AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);

            if (serialize)
            {
                serialize->Class<ReplicaDataViewSplitterSavedState>()
                    ->Field("m_splitterSavedState", &ReplicaDataViewSplitterSavedState::m_splitterSavedState)
                    ->Version(1)
                ;
            }
        }
    };

    //////////////////////////
    // ReplicaTableViewModel
    //////////////////////////

    ReplicaTableViewModel::ReplicaTableViewModel(ReplicaDataView* replicaDataView)
        : QAbstractTableModel(replicaDataView)
        , m_replicaDataView(replicaDataView)
    {
    }

    void ReplicaTableViewModel::RefreshView()
    {
        m_replicaIds.clear();

        if (m_replicaDataView->HideInactiveInspectedElements())
        {
            m_replicaIds.insert(m_replicaIds.begin(), m_replicaDataView->m_activeInspectedReplicaIds.begin(), m_replicaDataView->m_activeInspectedReplicaIds.end());
        }
        else
        {
            m_replicaIds.insert(m_replicaIds.begin(), m_replicaDataView->m_activeReplicaIds.begin(), m_replicaDataView->m_activeReplicaIds.end());
        }

        AZStd::sort(m_replicaIds.begin(), m_replicaIds.end(), AZStd::less<AZ::u64>());

        layoutChanged();
    }

    int ReplicaTableViewModel::rowCount(const QModelIndex& parentIndex) const
    {
        (void)parentIndex;

        return static_cast<int>(m_replicaIds.size());
    }

    int ReplicaTableViewModel::columnCount(const QModelIndex& parentIndex) const
    {
        (void)parentIndex;

        return CD_COUNT;
    }

    Qt::ItemFlags ReplicaTableViewModel::flags(const QModelIndex& index) const
    {
        Qt::ItemFlags flags = QAbstractTableModel::flags(index);

        switch (index.column())
        {
        case CD_INSPECT:
            flags &= ~(Qt::ItemIsSelectable);
            break;
        default:
            break;
        }

        return flags;
    }


    QVariant ReplicaTableViewModel::data(const QModelIndex& index, int role) const
    {
        AZ::u64 replicaId = GetReplicaIdFromIndex(index);

        ReplicaDataContainer* replicaContainer = m_replicaDataView->FindReplicaData(replicaId);

        
        if (replicaContainer != nullptr)
        {
            if (role == Qt::BackgroundRole)
            {
                if (replicaContainer->IsInspected())
                {
                    return QVariant::fromValue(QColor(94, 94, 178, 255));
                }
            }
            else
            {
                switch (index.column())
                {
                    case CD_REPLICA_ID:
                        if (role == Qt::DisplayRole)
                        {
                            return FormattingHelper::ReplicaID(replicaContainer->GetReplicaId());
                        }
                        else if (role == Qt::TextAlignmentRole)
                        {
                            return QVariant(Qt::AlignCenter);
                        }
                        break;
                    case CD_TOTAL_SENT:
                        if (role == Qt::DisplayRole)
                        {
                            return QString::number(replicaContainer->GetSentUsageForFrame(m_replicaDataView->GetCurrentFrame()));
                        }
                        else if (role == Qt::TextAlignmentRole)
                        {
                            return QVariant(Qt::AlignCenter);
                        }
                        break;
                    case CD_TOTAL_RECEIVED:
                        if (role == Qt::DisplayRole)
                        {
                            return QString::number(replicaContainer->GetReceivedUsageForFrame(m_replicaDataView->GetCurrentFrame()));
                        }
                        else if (role == Qt::TextAlignmentRole)
                        {
                            return QVariant(Qt::AlignCenter);
                        }
                        break;
                    case CD_REPLICA_NAME:
                        if (role == Qt::DecorationRole)
                        {
                            return replicaContainer->GetIcon();
                        }
                        else if (role == Qt::DisplayRole)
                        {
                            AZStd::string replicaName = replicaContainer->GetReplicaName();
                            return QString(replicaName.empty() ? "<unknown>" : replicaName.c_str());
                        }
                        else if (role == Qt::TextAlignmentRole)
                        {
                            return QVariant(Qt::AlignLeft | Qt::AlignVCenter);
                        }
                        break;
                    case CD_INSPECT:
                        if (role == Qt::DecorationRole || role == Qt::SizeHintRole)
                        {
                            QPixmap pixmap = QPixmap(":/general/inspect_icon");

                            if (role == Qt::DecorationRole)
                            {
                                return pixmap;
                            }
                            else if (role == Qt::SizeHintRole)
                            {
                                return pixmap.size();
                            }
                        }
                        else if (role == Qt::TextAlignmentRole)
                        {
                            return QVariant(Qt::AlignCenter);
                        }
                        break;
                    default:
                        AZ_Assert(false,"Unknown column index %i",index.column());
                        break;
                }
            }
        }

        return QVariant();
    }

    QVariant ReplicaTableViewModel::headerData(int section, Qt::Orientation orientation, int role) const
    {
        if (role == Qt::DisplayRole)
        {
            if (orientation == Qt::Horizontal)
            {
                switch (section)
                {
                case CD_REPLICA_ID:
                    return QString("Replica ID");
                case CD_TOTAL_SENT:
                    return QString("Sent Bytes");
                case CD_TOTAL_RECEIVED:
                    return QString("Received Bytes");
                case CD_REPLICA_NAME:
                    return QString("Replica Name");
                case CD_INSPECT:
                    return QString("");
                default:
                    AZ_Assert(false, "Unknown section index %i", section);
                    break;
                }
            }
        }

        return QVariant();
    }

    AZ::u64 ReplicaTableViewModel::GetReplicaIdFromIndex(const QModelIndex& index) const
    {
        return GetReplicaIdForRow(index.row());
    }

    AZ::u64 ReplicaTableViewModel::GetReplicaIdForRow(int row) const
    {
        if (row < 0 || row >= m_replicaIds.size())
        {
            return 0;
        }

        return m_replicaIds[row];
    }

    ///////////////////////////////////
    // ReplicaChunkTypeTableViewModel
    ///////////////////////////////////

    ReplicaChunkTypeTableViewModel::ReplicaChunkTypeTableViewModel(ReplicaDataView* replicaDataView)
        : QAbstractTableModel(replicaDataView)
        , m_replicaDataView(replicaDataView)
    {
    }

    void ReplicaChunkTypeTableViewModel::RefreshView()
    {
        m_replicaChunkTypes.clear();

        if (m_replicaDataView->HideInactiveInspectedElements())
        {
            m_replicaChunkTypes.insert(m_replicaChunkTypes.begin(), m_replicaDataView->m_activeInspectedChunkTypes.begin(), m_replicaDataView->m_activeInspectedChunkTypes.end());            
        }
        else
        {
            m_replicaChunkTypes.insert(m_replicaChunkTypes.begin(), m_replicaDataView->m_activeChunkTypes.begin(), m_replicaDataView->m_activeChunkTypes.end());
        }

        AZStd::sort(m_replicaChunkTypes.begin(), m_replicaChunkTypes.end(), AZStd::less<AZStd::string>());
        layoutChanged();
    }

    int ReplicaChunkTypeTableViewModel::rowCount(const QModelIndex& parentIndex) const
    {
        (void)parentIndex;

        return static_cast<int>(m_replicaChunkTypes.size());
    }

    int ReplicaChunkTypeTableViewModel::columnCount(const QModelIndex& parentIndex) const
    {
        (void)parentIndex;

        return CD_COUNT;
    }

    Qt::ItemFlags ReplicaChunkTypeTableViewModel::flags(const QModelIndex& index) const
    {
        Qt::ItemFlags flags = QAbstractTableModel::flags(index);

        switch (index.column())
        {
        case CD_INSPECT:
            flags &= ~(Qt::ItemIsSelectable);
            break;
        default:
            break;
        }

        return flags;
    }

    QVariant ReplicaChunkTypeTableViewModel::data(const QModelIndex& index, int role) const
    {
        const char* chunkType = GetReplicaChunkTypeFromIndex(index);
        ReplicaChunkTypeDataContainer* replicaChunkTypeContainer = m_replicaDataView->FindReplicaChunkTypeData(chunkType);

        if (replicaChunkTypeContainer != nullptr)
        {
            if (role == Qt::BackgroundRole)
            {
                if (replicaChunkTypeContainer->IsInspected())
                {
                    return QVariant::fromValue(QColor(94, 94, 178, 255));
                }
            }
            else
            {
                switch (index.column())
                {
                    case CD_CHUNK_TYPE:
                        if (role == Qt::DecorationRole)
                        {
                            return replicaChunkTypeContainer->GetIcon();
                        }
                        else if (role == Qt::DisplayRole)
                        {
                            AZStd::string chunkTypeString = replicaChunkTypeContainer->GetChunkType();
                            return QString(chunkTypeString.empty() ? "<unknown>" : chunkTypeString.c_str());
                        }
                        else if (role == Qt::TextAlignmentRole)
                        {
                            return QVariant(Qt::AlignCenter);
                        }
                        break;
                    case CD_TOTAL_SENT:
                        if (role == Qt::DisplayRole)
                        {
                            return QString::number(replicaChunkTypeContainer->GetSentUsageForFrame(m_replicaDataView->GetCurrentFrame()));
                        }
                        else if (role == Qt::TextAlignmentRole)
                        {
                            return QVariant(Qt::AlignCenter);
                        }
                        break;
                    case CD_TOTAL_RECEIVED:
                        if (role == Qt::DisplayRole)
                        {
                            return QString::number(replicaChunkTypeContainer->GetReceivedUsageForFrame(m_replicaDataView->GetCurrentFrame()));
                        }
                        else if (role == Qt::TextAlignmentRole)
                        {
                            return QVariant(Qt::AlignCenter);
                        }
                        break;                
                    case CD_INSPECT:
                        if (role == Qt::DecorationRole || role == Qt::SizeHintRole)
                        {
                            QPixmap pixmap = QPixmap(":/general/inspect_icon");

                            if (role == Qt::DecorationRole)
                            {
                                return pixmap;
                            }
                            else if (role == Qt::SizeHintRole)
                            {
                                return pixmap.size();
                            }
                        }
                        else if (role == Qt::TextAlignmentRole)
                        {
                            return QVariant(Qt::AlignCenter);
                        }
                        break;
                    default:
                        AZ_Assert(false,"Unknown column index %i",index.column());
                        break;
                }
            }
        }

        return QVariant();
    }  

    QVariant ReplicaChunkTypeTableViewModel::headerData(int section, Qt::Orientation orientation, int role) const
    {
        if (role == Qt::DisplayRole)
        {
            if (orientation == Qt::Horizontal)
            {
                switch (section)
                {
                case CD_CHUNK_TYPE:
                    return QString("Chunk Type");
                case CD_TOTAL_SENT:
                    return QString("Sent Bytes");
                case CD_TOTAL_RECEIVED:
                    return QString("Received Bytes");
                case CD_INSPECT:
                    return QString("");
                default:
                    AZ_Assert(false, "Unknown section index %i", section);
                    break;
                }
            }
        }

        return QVariant();
    }

    const char* ReplicaChunkTypeTableViewModel::GetReplicaChunkTypeFromIndex(const QModelIndex& index) const
    {
        return GetReplicaChunkTypeForRow(index.row());
    }

    const char* ReplicaChunkTypeTableViewModel::GetReplicaChunkTypeForRow(int row) const
    {
        const char* retVal = nullptr;

        if (row < 0 || row >= m_replicaChunkTypes.size())
        {
            return retVal;
        }

        retVal = m_replicaChunkTypes[row].c_str();

        return retVal;
    }

    ////////////////////////
    // ChartZoomMaintainer
    ////////////////////////

    ReplicaDataView::ChartZoomMaintainer::ChartZoomMaintainer()
        : m_axis(Charts::AxisType::Horizontal)
        , m_minValue(0.0f)
        , m_maxValue(1.0f)
    {
    }

    void ReplicaDataView::ChartZoomMaintainer::GetZoomFromChart(StripChart::DataStrip& chart, Charts::AxisType axis)
    {
        m_axis = axis;

        bool gotWindowRange = chart.GetWindowRange(axis, m_minValue, m_maxValue);

        float minRange;
        float maxRange;

        bool gotAxisRange = chart.GetAxisRange(axis, minRange, maxRange);

        if (gotWindowRange && gotAxisRange)
        {
            float range = maxRange - minRange;

            if (AZ::IsClose(maxRange, minRange, 0.01f))
            {
                range = 1.0f;
            }

            m_minValue /= range;
            m_maxValue /= range;
        }
        else
        {
            m_minValue = 0.0f;
            m_maxValue = 1.0f;
        }
    }

    void ReplicaDataView::ChartZoomMaintainer::SetZoomOnChart(StripChart::DataStrip& chart, Charts::AxisType axis)
    {
        AZ_Assert(axis == m_axis, "Warning: Manipulating different axis from when zoom was set");

        float minRange;
        float maxRange;

        bool gotRange = chart.GetAxisRange(axis, minRange, maxRange);

        if (gotRange)
        {
            float range = maxRange - minRange;

            if (AZ::IsClose(maxRange, minRange, 0.01f))
            {
                range = 1.0f;
            }

            chart.ZoomManual(axis, range * m_minValue, range * m_maxValue);
        }
    }

    ////////////////////
    // ReplicaDataView
    ////////////////////
    const char* ReplicaDataView::DDT_REPLICA_NAME = "Replica";
    const char* ReplicaDataView::DDT_CHUNK_NAME = "Chunk Type";

    const char* ReplicaDataView::WINDOW_STATE_FORMAT = "REPLICA_DATA_VIEW_WINDOW_STATE_%u";
    const char* ReplicaDataView::SPLITTER_STATE_FORMAT = "REPLICA_DATA_VIEW_SPLITTER_STATE_%u";
    const char* ReplicaDataView::TABLE_STATE_FORMAT = "REPLICA_DATA_VIEW_TABLE_STATE_%u";
    const char* ReplicaDataView::DATA_VIEW_STATE_FORMAT = "REPLICA_DATA_VIEW_DATA_VIEW_STATE_%u";
    const char* ReplicaDataView::DATA_VIEW_WORKSPACE_FORMAT = "REPLICA_DATA_VIEW_WORKSPACE_%u";

    const int ReplicaDataView::INSPECT_ICON_COLUMN_SIZE = 32;

    ReplicaDataView::ReplicaDataView(unsigned int dataViewIndex, FrameNumberType currentFrame, const ReplicaDataAggregator* aggregator)
        : QDialog()
        , m_dataViewIndex(dataViewIndex)
        , m_inspectedSeries(AreaChart::AreaChart::k_invalidSeriesId)
        , m_windowStateCRC(0)
        , m_splitterStateCRC(0)
        , m_tableViewCRC(0)
        , m_dataViewCRC(0)
        , m_aggregatorIdentity(aggregator->GetIdentity())
        , m_aggregator(aggregator)
        , m_currentFrame(currentFrame)
        , m_startFrame(0)
        , m_endFrame(0)
        , m_overallReplicaDetailView(nullptr)
        , m_replicaTypeTableView(this)
        , m_replicaChunkTypeTableView(this)
        , m_lifespanTelemetry("ReplicaDataView")
    {
        setAttribute(Qt::WA_DeleteOnClose, true);
        setWindowFlags(windowFlags() | Qt::WindowMaximizeButtonHint | Qt::WindowMinimizeButtonHint);

        // Create a window defined in CarrierDataView.ui.
        m_gui = azcreate(Ui::ReplicaDataView, ());
        m_gui->setupUi(this);

        show();
        raise();
        activateWindow();
        setFocus();

        m_gui->areaChart->ConfigureVerticalAxis("Bandwidth Usage", GetAverageFrameBandwidthBudget());
        m_gui->areaChart->EnableMouseInspection(true);

        this->setWindowTitle(m_aggregator->GetDialogTitle());

        for (int i = DDT_START + 1; i < DDT_END; ++i)
        {
            switch (i)
            {
            case DDT_REPLICA:
                m_gui->dataSelectionComboBox->addItem(QString(DDT_REPLICA_NAME));
                break;
            case DDT_CHUNK:
                m_gui->dataSelectionComboBox->addItem(QString(DDT_CHUNK_NAME));
                break;
            default:
                break;
            }
        }

        if (m_gui->dataSelectionComboBox->count() == 1)
        {
            m_gui->dataSelectionComboBox->setEditable(false);
            m_gui->dataSelectionComboBox->setEnabled(false);
        }

        for (int i = ReplicaDisplayTypes::BUDT_START + 1; i < ReplicaDisplayTypes::BUDT_END; ++i)
        {
            switch (i)
            {
            case ReplicaDisplayTypes::BUDT_COMBINED:
                m_gui->bandwidthUsageComboBox->addItem(QString(ReplicaDisplayTypes::DisplayNames::BUDT_COMBINED_NAME));
                break;
            case ReplicaDisplayTypes::BUDT_SENT:
                m_gui->bandwidthUsageComboBox->addItem(QString(ReplicaDisplayTypes::DisplayNames::BUDT_SENT_NAME));
                break;
            case ReplicaDisplayTypes::BUDT_RECEIVED:
                m_gui->bandwidthUsageComboBox->addItem(QString(ReplicaDisplayTypes::DisplayNames::BUDT_RECEIVED_NAME));
                break;
            default:
                break;
            }
        }

        if (m_gui->bandwidthUsageComboBox->count() == 1)
        {
            m_gui->dataSelectionComboBox->setEditable(false);
            m_gui->dataSelectionComboBox->setEnabled(false);
        }

        for (int i = TFT_START + 1; i < TFT_END; ++i)
        {
            switch (i)
            {
            case TFT_NONE:
                m_gui->tableFilterComboBox->addItem("No Filter");
                break;
            case TFT_ACTIVE_ONLY:
                m_gui->tableFilterComboBox->addItem("Active Types");
                break;
            }
        }

        if (m_gui->tableFilterComboBox->count() == 1)
        {
            m_gui->tableFilterComboBox->setEditable(false);
            m_gui->tableFilterComboBox->setEnabled(false);
        }

        m_gui->drillerConfigToolbar->enableTreeCommands(false);

        AZStd::string serializationString = AZStd::string::format(WINDOW_STATE_FORMAT, m_dataViewIndex);
        m_windowStateCRC = AZ::Crc32(serializationString.c_str());

        AZStd::intrusive_ptr<AzToolsFramework::QWidgetSavedState> windowState = AZ::UserSettings::Find<AzToolsFramework::QWidgetSavedState>(m_windowStateCRC, AZ::UserSettings::CT_GLOBAL);

        if (windowState)
        {
            windowState->RestoreGeometry(this);
        }

        serializationString = AZStd::string::format(DATA_VIEW_STATE_FORMAT, m_dataViewIndex);
        m_dataViewCRC = AZ::Crc32(serializationString.c_str());
        m_persistentState = AZ::UserSettings::CreateFind<ReplicaDataViewSavedState>(m_dataViewCRC, AZ::UserSettings::CT_GLOBAL);

        ApplyPersistentState();

        // do the table state formatting
        serializationString = AZStd::string::format(TABLE_STATE_FORMAT, m_dataViewIndex);
        m_tableViewCRC = AZ::Crc32(serializationString.c_str());
        auto treeState = AZ::UserSettings::Find<ReplicaDataViewTableModelSavedState>(m_tableViewCRC, AZ::UserSettings::CT_GLOBAL);

        if (treeState)
        {
            QByteArray treeData((const char*)treeState->m_treeColumnStorage.data(), (int)treeState->m_treeColumnStorage.size());
            m_gui->tableView->horizontalHeader()->restoreState(treeData);
        }

        serializationString = AZStd::string::format(SPLITTER_STATE_FORMAT, m_dataViewIndex);
        m_splitterStateCRC = AZ::Crc32(serializationString.c_str());
        auto splitterState = AZ::UserSettings::Find<ReplicaDataViewSplitterSavedState>(m_splitterStateCRC,AZ::UserSettings::CT_GLOBAL);

        if (splitterState)
        {
            QByteArray splitterData((const char*)splitterState->m_splitterSavedState.data(), (int)splitterState->m_splitterSavedState.size());
            m_gui->splitter->restoreState(splitterData);
        }

        DrillerMainWindowMessages::Handler::BusConnect(m_aggregatorIdentity);
        DrillerEventWindowMessages::Handler::BusConnect(m_aggregatorIdentity);

        QObject::connect((&m_replicaTypeTableView), SIGNAL(dataChanged(const QModelIndex&, const QModelIndex&)), this, SLOT(UpdateDisplay(const QModelIndex&, const QModelIndex&)));
        QObject::connect((&m_replicaChunkTypeTableView), SIGNAL(dataChanged(const QModelIndex&, const QModelIndex&)), this, SLOT(UpdateDisplay(const QModelIndex&, const QModelIndex&)));

        QObject::connect(m_gui->tableView, SIGNAL(clicked(const QModelIndex&)), this, SLOT(OnCellClicked(const QModelIndex&)));
        QObject::connect(m_gui->tableView, SIGNAL(doubleClicked(const QModelIndex&)), this, SLOT(OnDoubleClicked(const QModelIndex&)));
        
        QObject::connect(m_gui->drillerConfigToolbar, SIGNAL(hideAll()), this, SLOT(HideAll()));
        QObject::connect(m_gui->drillerConfigToolbar, SIGNAL(hideSelected()), this, SLOT(HideSelected()));
        QObject::connect(m_gui->drillerConfigToolbar, SIGNAL(showAll()), this, SLOT(ShowAll()));
        QObject::connect(m_gui->drillerConfigToolbar, SIGNAL(showSelected()), this, SLOT(ShowSelected()));
        
        QObject::connect(m_gui->showOverallStatistics,SIGNAL(clicked()),this,SLOT(OnShowOverallStatistics()));
        QObject::connect(m_gui->displayRange, SIGNAL(valueChanged(int)), this, SLOT(OnDisplayRangeChanged(int)));        
        QObject::connect(m_gui->dataSelectionComboBox,SIGNAL(currentIndexChanged(int)),this,SLOT(OnDataTypeChanged(int)));        
        QObject::connect(m_gui->bandwidthUsageComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(OnBandwidthUsageDisplayTypeChanged(int)));
        QObject::connect(m_gui->tableFilterComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(OnTableFilterTypeChanged(int)));

        QObject::connect(m_gui->areaChart, SIGNAL(InspectedSeries(size_t)), this, SLOT(OnInspectedSeries(size_t)));
        QObject::connect(m_gui->areaChart, SIGNAL(SelectedSeries(size_t,int)), this, SLOT(OnSelectedSeries(size_t, int)));
    }

    ReplicaDataView::~ReplicaDataView()
    {
        DrillerEventWindowMessages::Handler::BusDisconnect(m_aggregatorIdentity);
        DrillerMainWindowMessages::Handler::BusDisconnect(m_aggregatorIdentity);

        // Save out whatever data we want to save out.
        auto pState = AZ::UserSettings::CreateFind<AzToolsFramework::QWidgetSavedState>(m_windowStateCRC, AZ::UserSettings::CT_GLOBAL);
        if (pState)
        {
            pState->CaptureGeometry(this);
        }

        auto splitterState = AZ::UserSettings::CreateFind<ReplicaDataViewSplitterSavedState>(m_splitterStateCRC, AZ::UserSettings::CT_GLOBAL);
        if (splitterState)
        {
            QByteArray qba = m_gui->splitter->saveState();
            splitterState->m_splitterSavedState.assign((AZ::u8*)qba.begin(), (AZ::u8*)qba.end());
        }

        auto treeState = AZ::UserSettings::CreateFind<ReplicaDataViewTableModelSavedState>(m_tableViewCRC, AZ::UserSettings::CT_GLOBAL);
        if (treeState)
        {
            if (m_gui->tableView && m_gui->tableView->horizontalHeader())
            {
                QByteArray qba = m_gui->tableView->horizontalHeader()->saveState();
                treeState->m_treeColumnStorage.assign((AZ::u8*)qba.begin(), (AZ::u8*)qba.end());
            }
        }

        for (ReplicaDataMap::iterator replicaIter = m_replicaData.begin();
             replicaIter != m_replicaData.end();
             ++replicaIter)
        {
            delete replicaIter->second;
        }

        m_replicaData.clear();

        for (ReplicaChunkTypeDataMap::iterator chunkIter = m_replicaChunkTypeData.begin();
             chunkIter != m_replicaChunkTypeData.end();
             ++chunkIter)
        {
            delete chunkIter->second;
        }
        m_replicaChunkTypeData.clear();

        for (ReplicaDetailView* view : m_spawnedReplicaDetailViews)
        {
            view->SignalDataViewDestroyed(this);
            view->close();
        }

        for (ReplicaChunkTypeDetailView* view : m_spawnedChunkDetailViews)
        {
            view->SignalDataViewDestroyed(this);
            view->close();
        }

        if (m_overallReplicaDetailView)
        {
            m_overallReplicaDetailView->SignalDataViewDestroyed(this);
            m_overallReplicaDetailView->close();
            m_overallReplicaDetailView = nullptr;
        }

        azdestroy(m_gui);
    }

    void ReplicaDataView::FrameChanged(FrameNumberType frame)
    {
        int displayRange = GetDisplayRange();
        int halfRange = displayRange / 2;

        m_currentFrame = frame;

        m_startFrame = AZStd::GetMax(m_currentFrame - halfRange, 0);

        if (m_startFrame == 0)
        {
            m_endFrame = AZStd::GetMin(m_startFrame + displayRange, static_cast<FrameNumberType>(m_aggregator->GetFrameCount()));
        }
        else
        {
            m_endFrame = AZStd::GetMin(m_currentFrame + halfRange, static_cast<FrameNumberType>(m_aggregator->GetFrameCount()));
        }

        if (m_endFrame == m_aggregator->GetFrameCount())
        {
            m_startFrame = AZStd::GetMax(m_endFrame - displayRange, 0);
        }

        UpdateData();

        RefreshGraph();
        RefreshTableView();

        emit DataRangeChanged();
    }

    void ReplicaDataView::EventFocusChanged(EventNumberType eventIndex)
    {
        (void)eventIndex;
    }

    void ReplicaDataView::EventChanged(EventNumberType eventIndex)
    {
        (void)eventIndex;
    }

    float ReplicaDataView::GetAxisStartFrame() const
    {
        return static_cast<float>(m_startFrame);
    }

    FrameNumberType ReplicaDataView::GetStartFrame() const
    {
        return m_startFrame;
    }

    float ReplicaDataView::GetAxisEndFrame() const
    {
        return static_cast<float>(AZStd::GetMax(m_endFrame, GetDisplayRange()));
    }

    FrameNumberType ReplicaDataView::GetEndFrame() const
    {
        return m_endFrame;
    }

    FrameNumberType ReplicaDataView::GetActiveFrameCount() const
    {
        return GetDisplayRange();
    }

    FrameNumberType ReplicaDataView::GetCurrentFrame() const
    {
        return m_currentFrame;
    }

    bool ReplicaDataView::HideInactiveInspectedElements() const
    {
        return m_persistentState->m_tableFilterType == TFT_ACTIVE_ONLY;
    }

    int ReplicaDataView::GetCaptureWindowIdentity() const
    {
        return m_aggregator->GetIdentity();
    }

    unsigned int ReplicaDataView::GetAverageFrameBandwidthBudget() const
    {
        return m_aggregator->GetAverageFrameBandwidthBudget();
    }

    void ReplicaDataView::DrawFrameGraph()
    {
        const QColor markerColor(Qt::red);
        BandwidthUsageAggregator maximumUsageAggregator;

        m_gui->areaChart->ResetChart();
        m_gui->areaChart->ConfigureHorizontalAxis("Frame", static_cast<int>(GetAxisStartFrame()), static_cast<int>(GetAxisEndFrame()));

        m_gui->areaChart->AddMarker(Charts::AxisType::Horizontal, static_cast<int>(GetCurrentFrame()), markerColor);

        switch (GetDisplayDataType())
        {
            case DDT_REPLICA:
                for (auto replicaIter = m_replicaData.begin();
                     replicaIter != m_replicaData.end();
                     ++replicaIter)
                {
                    ReplicaDataContainer* container = replicaIter->second;

                    container->GetAreaGraphPlotHelper().Reset();
                    PlotChartDataForFrames(container);
                }
                break;
            case DDT_CHUNK:
                for (auto chunkIter = m_replicaChunkTypeData.begin();
                     chunkIter != m_replicaChunkTypeData.end();
                     ++chunkIter)
                {
                    ReplicaChunkTypeDataContainer* container = chunkIter->second;

                    container->GetAreaGraphPlotHelper().Reset();
                    PlotChartDataForFrames(container);
                }
                break;
            default:
                AZ_Assert(false,"ERROR: Unknown display data type");
                break;
        }        
    }

    void ReplicaDataView::SignalDialogClosed(QDialog* dialog)
    {
        if (dialog == m_overallReplicaDetailView)
        {
            m_overallReplicaDetailView = nullptr;
            return;
        }

        for (AZStd::vector< ReplicaDetailView* >::iterator dialogIter = m_spawnedReplicaDetailViews.begin();
             dialogIter != m_spawnedReplicaDetailViews.end();
             ++dialogIter)
        {
            if ((*dialogIter) == dialog)
            {
                m_spawnedReplicaDetailViews.erase(dialogIter);
                return;
            }
        }

        for (AZStd::vector< ReplicaChunkTypeDetailView* >::iterator dialogIter = m_spawnedChunkDetailViews.begin();
             dialogIter != m_spawnedChunkDetailViews.end();
             ++dialogIter)
        {
            if ((*dialogIter) == dialog)
            {
                m_spawnedChunkDetailViews.erase(dialogIter);
                return;
            }
        }
    }

    unsigned int ReplicaDataView::GetDataViewIndex() const
    {
        return m_dataViewIndex;
    }

    void ReplicaDataView::ApplySettingsFromWorkspace(WorkspaceSettingsProvider* settingsProvider)
    {
        AZStd::string workspaceStateStr = AZStd::string::format(DATA_VIEW_WORKSPACE_FORMAT, GetDataViewIndex());
        AZ::u32 workspaceStateCRC = AZ::Crc32(workspaceStateStr.c_str());

        if (m_persistentState)
        {
            ReplicaDataViewSavedState* workspace = settingsProvider->FindSetting<ReplicaDataViewSavedState>(workspaceStateCRC);

            if (workspace)
            {
                m_persistentState->CopyStateFrom(workspace);
            }
        }
    }

    void ReplicaDataView::ActivateWorkspaceSettings(WorkspaceSettingsProvider* settingsProvider)
    {
        (void)settingsProvider;

        ApplyPersistentState();
    }

    void ReplicaDataView::SaveSettingsToWorkspace(WorkspaceSettingsProvider* settingsProvider)
    {
        AZStd::string workspaceStateStr = AZStd::string::format(DATA_VIEW_WORKSPACE_FORMAT, GetDataViewIndex());
        AZ::u32 workspaceStateCRC = AZ::Crc32(workspaceStateStr.c_str());

        if (m_persistentState)
        {
            ReplicaDataViewSavedState* workspace = settingsProvider->CreateSetting<ReplicaDataViewSavedState>(workspaceStateCRC);

            if (workspace)
            {
                workspace->CopyStateFrom(m_persistentState.get());
            }
        }
    }

    void ReplicaDataView::ApplyPersistentState()
    {
        if (m_persistentState)
        {
            m_gui->dataSelectionComboBox->setCurrentIndex(m_persistentState->m_displayDataType);
            m_gui->bandwidthUsageComboBox->setCurrentIndex(m_persistentState->m_bandwidthUsageDisplayType);
            m_gui->tableFilterComboBox->setCurrentIndex(m_persistentState->m_tableFilterType);

            m_gui->displayRange->setValue(m_persistentState->m_displayRange);

            SetupTableView();
            FrameChanged(GetCurrentFrame());
        }
    }

    void ReplicaDataView::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            ReplicaDataViewSavedState::Reflect(context);
            ReplicaDataViewTableModelSavedState::Reflect(context);
            ReplicaDataViewSplitterSavedState::Reflect(context);

            BaseDetailViewSplitterSavedState::Reflect(context);
            BaseDetailViewTreeSavedState::Reflect(context);
        }
    }

    void ReplicaDataView::ReplicaSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected)
    {
        if (GetDisplayDataType() != DDT_REPLICA)
        {
            return;
        }

        if (!selected.indexes().empty())
        {
            for (const QModelIndex& selectedIndex : selected.indexes())
            {
                AZ::u64 replicaId = m_replicaTypeTableView.GetReplicaIdFromIndex(selectedIndex);
                ReplicaDataContainer* container = FindReplicaData(replicaId);

                if (container != nullptr)
                {
                    container->SetSelected(true);

                    const bool isHighlighted = true;
                    container->GetAreaGraphPlotHelper().SetHighlighted(isHighlighted);                    
                }
            }
        }

        if (!deselected.empty())
        {
            for (const QModelIndex& deselectedIndex : deselected.indexes())
            {
                AZ::u64 replicaId = m_replicaTypeTableView.GetReplicaIdFromIndex(deselectedIndex);
                ReplicaDataContainer* container = FindReplicaData(replicaId);

                if (container != nullptr)
                {
                    container->SetSelected(false);

                    const bool isHighlighted = false;
                    container->GetAreaGraphPlotHelper().SetHighlighted(isHighlighted);
                }
            }
        }        
    }

    void ReplicaDataView::ChunkSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected)
    {
        if (GetDisplayDataType() != DDT_CHUNK)
        {
            return;
        }

        if (!selected.indexes().empty())
        {
            for (const QModelIndex& selectedIndex : selected.indexes())
            {
                const char* chunkType = m_replicaChunkTypeTableView.GetReplicaChunkTypeFromIndex(selectedIndex);
                ReplicaChunkTypeDataContainer* container = FindReplicaChunkTypeData(chunkType);

                if (container != nullptr)
                {
                    container->SetSelected(true);

                    const bool isHighlighted = true;
                    container->GetAreaGraphPlotHelper().SetHighlighted(isHighlighted);                    
                }
            }
        }

        if (!deselected.empty())
        {
            AZStd::unordered_set<int> rowSets;
            for (const QModelIndex& deselectedIndex : deselected.indexes())
            {
                const char* chunkType = m_replicaChunkTypeTableView.GetReplicaChunkTypeFromIndex(deselectedIndex);
                ReplicaChunkTypeDataContainer* container = FindReplicaChunkTypeData(chunkType);

                if (container != nullptr)
                {
                    container->SetSelected(false);

                    const bool isHighlighted = false;
                    container->GetAreaGraphPlotHelper().SetHighlighted(isHighlighted);
                }
            }
        }
    }

    void ReplicaDataView::OnDisplayRangeChanged(int displayRange)
    {
        ReplicaOperationTelemetryEvent displayRangeEvent;
        displayRangeEvent.SetMetric("DisplayRange", displayRange);
        displayRangeEvent.Log();

        m_persistentState->m_displayRange = displayRange;
        FrameChanged(GetCurrentFrame());
    }    

    void ReplicaDataView::HideAll()
    {
        SetAllEnabled(false);
    }

    void ReplicaDataView::ShowAll()
    {
        SetAllEnabled(true);
    }

    void ReplicaDataView::SetAllEnabled(bool enabled)
    {
        switch (GetDisplayDataType())
        {
        case DDT_REPLICA:
        {
            for (AZ::u64 replicaId : m_activeReplicaIds)
            {
                ReplicaDataContainer* dataContainer = m_replicaData[replicaId];
                dataContainer->SetEnabled(enabled);
                dataContainer->GetAreaGraphPlotHelper().SetEnabled(enabled);
            }

            m_replicaTypeTableView.layoutChanged();
            break;
        }
        case DDT_CHUNK:
        {
            for (AZStd::string chunkType : m_activeChunkTypes)
            {
                ReplicaChunkTypeDataContainer* dataContainer = m_replicaChunkTypeData[chunkType];
                dataContainer->SetEnabled(enabled);
                dataContainer->GetAreaGraphPlotHelper().SetEnabled(enabled);
            }

            m_replicaChunkTypeTableView.layoutChanged();
            break;
        }
        default:
            AZ_Assert(false, "Unknown Display Data Type");
            break;
        }      
    }

    void ReplicaDataView::HideSelected()
    {
        SetSelectedEnabled(false);
    }

    void ReplicaDataView::ShowSelected()
    {
        SetSelectedEnabled(true);
    }

    void ReplicaDataView::SetSelectedEnabled(bool enabled)
    {
        switch (GetDisplayDataType())
        {
        case DDT_REPLICA:
        {
            for (AZ::u64 replicaId : m_activeReplicaIds)
            {
                ReplicaDataContainer* dataContainer = m_replicaData[replicaId];

                if (dataContainer->IsSelected())
                {
                    dataContainer->SetEnabled(enabled);
                }
            }

            m_replicaTypeTableView.layoutChanged();
            break;
        }
        case DDT_CHUNK:
        {
            for (AZStd::string chunkType : m_activeChunkTypes)
            {
                ReplicaChunkTypeDataContainer* dataContainer = m_replicaChunkTypeData[chunkType];

                if (dataContainer->IsSelected())
                {
                    dataContainer->SetEnabled(enabled);
                }
            }

            m_replicaChunkTypeTableView.layoutChanged();
            break;
        }
        default:
            AZ_Assert(false, "Unknown Data Display Type");
            break;
        }

        m_replicaChunkTypeTableView.layoutChanged();
        RefreshGraph();
    }

    void ReplicaDataView::UpdateDisplay(const QModelIndex& startIndex, const QModelIndex& endIndex)
    {
        (void)startIndex;
        (void)endIndex;

        RefreshGraph();
    }

    void ReplicaDataView::RefreshGraph()
    {
        DrawFrameGraph();
    }

    void ReplicaDataView::OnCellClicked(const QModelIndex& index)
    {
        if (!index.isValid())
        {
            return;
        }

        switch (GetDisplayDataType())
        {
        case DDT_REPLICA:
            if (index.column() == ReplicaTableViewModel::CD_INSPECT)
            {
                InspectReplica(index.row());
            }
            break;
        case DDT_CHUNK:
            if (index.column() == ReplicaChunkTypeTableViewModel::CD_INSPECT)
            {
                InspectChunkType(index.row());
            }
            break;
        default:
            AZ_Assert(false, "ERROR: Unknown Display Data Type");
            break;
        }
    }

    void ReplicaDataView::OnDoubleClicked(const QModelIndex& index)
    {
        if (!index.isValid())
        {
            return;
        }

        switch (GetDisplayDataType())
        {
        case DDT_REPLICA:
        {
            if (index.column() != ReplicaTableViewModel::CD_INSPECT)
            {
                AZ::u64 replicaId = m_replicaTypeTableView.GetReplicaIdFromIndex(index);

                ReplicaDataContainer* dataContainer = FindReplicaData(replicaId);
                dataContainer->SetEnabled(!dataContainer->IsEnabled());                    
                dataContainer->GetAreaGraphPlotHelper().SetEnabled(dataContainer->IsEnabled());
            }
            break;
        }
        case DDT_CHUNK:
        {
            if (index.column() != ReplicaChunkTypeTableViewModel::CD_INSPECT)
            {
                const char* chunkType = m_replicaChunkTypeTableView.GetReplicaChunkTypeFromIndex(index);

                ReplicaChunkTypeDataContainer* dataContainer = FindReplicaChunkTypeData(chunkType);
                dataContainer->SetEnabled(!dataContainer->IsEnabled());
                dataContainer->GetAreaGraphPlotHelper().SetEnabled(dataContainer->IsEnabled());
            }
            break;
        }
        default:
            AZ_Assert(false, "ERROR: Unknown Display Data Type");
            break;
        }
    }

    void ReplicaDataView::OnDataTypeChanged(int selectedIndex)
    {
        AZ_Error("StandaloneTools", selectedIndex > DDT_START && selectedIndex < DDT_END, "selectedIndex for DataType is out of enum range.");

        if (selectedIndex > DDT_START && selectedIndex < DDT_END)
        {
            m_persistentState->m_displayDataType = static_cast<DisplayDataType>(selectedIndex);
            ParseActiveItems();
            SetupTableView();
            RefreshGraph();

            ReplicaOperationTelemetryEvent dataTypeChanged;

            switch (m_persistentState->m_displayDataType)
            {
                case DDT_CHUNK:
                {
                    dataTypeChanged.SetAttribute("DisplayDataType", DDT_CHUNK_NAME);
                    break;
                }
                case DDT_REPLICA:
                {
                    dataTypeChanged.SetAttribute("DisplayDataType", DDT_REPLICA_NAME);
                    break;
                }
                default:
                {
                    dataTypeChanged.SetAttribute("Change Display Data Type", "Unknown");
                }
            }

            dataTypeChanged.Log();
        }
    }

    void ReplicaDataView::OnBandwidthUsageDisplayTypeChanged(int selectedIndex)
    {
        AZ_Error("StandaloneTools", selectedIndex > ReplicaDisplayTypes::BUDT_START && selectedIndex < ReplicaDisplayTypes::BUDT_END, "Invalid index for BandwidthUsageDisplay");

        if (selectedIndex > ReplicaDisplayTypes::BUDT_START && selectedIndex < ReplicaDisplayTypes::BUDT_END)
        {
            m_persistentState->m_bandwidthUsageDisplayType = static_cast<ReplicaDisplayTypes::BandwidthUsageDisplayType>(selectedIndex);

            RefreshGraph();

            ReplicaOperationTelemetryEvent bandwidthDisplayChanged;

            switch (m_persistentState->m_displayDataType)
            {
                case ReplicaDisplayTypes::BUDT_COMBINED:
                {
                    bandwidthDisplayChanged.SetAttribute("BandwidthUsageDisplayType", ReplicaDisplayTypes::DisplayNames::BUDT_COMBINED_NAME);
                    break;
                }
                case ReplicaDisplayTypes::BUDT_SENT:
                {
                    bandwidthDisplayChanged.SetAttribute("BandwidthUsageDisplayType", ReplicaDisplayTypes::DisplayNames::BUDT_SENT_NAME);
                    break;
                }
                case ReplicaDisplayTypes::BUDT_RECEIVED:
                {
                    bandwidthDisplayChanged.SetAttribute("BandwidthUsageDisplayType", ReplicaDisplayTypes::DisplayNames::BUDT_RECEIVED_NAME);
                    break;
                }
                default:
                {
                    bandwidthDisplayChanged.SetAttribute("Change Display Data Type", "Unknown");
                }
            }

            bandwidthDisplayChanged.Log();
        }
    }

    void ReplicaDataView::OnTableFilterTypeChanged(int selectedIndex)
    {
        AZ_Error("StandaloneTools", selectedIndex > TFT_START && selectedIndex < TFT_END, "Invalid index for TableFilterType");

        if (selectedIndex > TFT_START && selectedIndex < TFT_END)
        {
            m_persistentState->m_tableFilterType = static_cast<TableFilterType>(selectedIndex);
            RefreshTableView();

            ReplicaOperationTelemetryEvent displayFilterChangedEvent;

            switch (m_persistentState->m_tableFilterType)
            {
                case TFT_NONE:
                {
                    displayFilterChangedEvent.SetAttribute("TableFilterType", "None");
                    break;
                }
                case TFT_ACTIVE_ONLY:
                {
                    displayFilterChangedEvent.SetAttribute("TableFilterType", "Active Only");
                    break;
                }
                default:
                {
                    displayFilterChangedEvent.SetAttribute("TableFilterType", "Unknown");
                }
            }

            displayFilterChangedEvent.Log();
        }
    }

    void ReplicaDataView::OnShowOverallStatistics()
    {
        if (m_overallReplicaDetailView == nullptr)
        {
            m_overallReplicaDetailView = aznew OverallReplicaDetailView(this, (*m_aggregator));
        }
        else
        {
            if (m_overallReplicaDetailView->isMinimized())
            {
                m_overallReplicaDetailView->showNormal();
            }

            m_overallReplicaDetailView->raise();
            m_overallReplicaDetailView->activateWindow();
        }
    }    

    void ReplicaDataView::OnInspectedSeries(size_t seriesId)
    {
        if (m_inspectedSeries != seriesId)
        {
            m_inspectedSeries = seriesId;

            // This could be improved by using a map. But might not be necessary.
            if (GetDisplayDataType() == DDT_REPLICA)
            {
                for (auto& mapPair : m_replicaData)
                {
                    ReplicaDataContainer* container = mapPair.second;

                    container->SetInspected(container->GetAreaGraphPlotHelper().IsSeries(seriesId));
                }

                m_replicaTypeTableView.layoutChanged();
            }
            else if (GetDisplayDataType() == DDT_CHUNK)
            {
                for (auto& mapPair : m_replicaChunkTypeData)
                {
                    ReplicaChunkTypeDataContainer* container = mapPair.second;

                    container->SetInspected(container->GetAreaGraphPlotHelper().IsSeries(seriesId));
                }

                m_replicaChunkTypeTableView.layoutChanged();
            }
        }
    }

    void ReplicaDataView::OnSelectedSeries(size_t seriesId, int position)
    {
        (void)seriesId;

        EBUS_EVENT_ID(GetCaptureWindowIdentity(), DrillerCaptureWindowRequestBus, ScrubToFrameRequest, position);
    }

    void ReplicaDataView::InspectReplica(int tableRow)
    {
        AZ::u64 replicaId = m_replicaTypeTableView.GetReplicaIdForRow(tableRow);
        ReplicaDataContainer* replicaContainer = FindReplicaData(replicaId);

        ReplicaDetailView* replicaDetailView = aznew ReplicaDetailView(this, replicaContainer);
        replicaDetailView->LoadSavedState();
        m_spawnedReplicaDetailViews.push_back(replicaDetailView);
    }

    void ReplicaDataView::InspectChunkType(int tableRow)
    {
        const char* chunkType = m_replicaChunkTypeTableView.GetReplicaChunkTypeForRow(tableRow);
        ReplicaChunkTypeDataContainer* chunkContainer = FindReplicaChunkTypeData(chunkType);

        ReplicaChunkTypeDetailView* replicaDetailView = aznew ReplicaChunkTypeDetailView(this, chunkContainer);
        replicaDetailView->LoadSavedState();
        m_spawnedChunkDetailViews.push_back(replicaDetailView);
    }

    void ReplicaDataView::RefreshTableView()
    {
        switch (GetDisplayDataType())
        {
        case DDT_REPLICA:
            m_replicaTypeTableView.RefreshView();
            break;
        case DDT_CHUNK:
            m_replicaChunkTypeTableView.RefreshView();
            break;
        default:
            break;
        }
    }

    void ReplicaDataView::SetupTableView()
    {
        m_gui->tableView->reset();

        switch (GetDisplayDataType())
        {
        case DDT_REPLICA:
            SetupReplicaTableView();
            break;
        case DDT_CHUNK:
            SetupChunkTableView();
            break;
        default:
            break;
        }

        RefreshTableView();
    }

    void ReplicaDataView::SetupReplicaTableView()
    {
        m_gui->tableView->setModel(&m_replicaTypeTableView);
        m_gui->tableView->verticalHeader()->hide();

        m_gui->tableView->horizontalHeader()->reset();

        // I think this will fix the sizing issue, but until we update to 5.6 we don't get it! AWESOME
        //m_gui->tableView->horizontalHeader()->resetDefaultSectionSize();

        for (int i = 0; i < m_replicaTypeTableView.columnCount(); ++i)
        {
            m_gui->tableView->horizontalHeader()->resizeSection(i, m_gui->tableView->horizontalHeader()->defaultSectionSize());
        }

        // QT Persists the section resize mode after you call reset on the table, and on the column header.
        // It's pretty special.
        // Going to manually remove the information to avoid something looking really stupid.
        if (ReplicaChunkTypeTableViewModel::CD_INSPECT < m_replicaTypeTableView.columnCount())
        {
            m_gui->tableView->horizontalHeader()->setSectionResizeMode(ReplicaChunkTypeTableViewModel::CD_INSPECT, QHeaderView::Interactive);
        }

        m_gui->tableView->horizontalHeader()->setSectionsClickable(false);
        m_gui->tableView->horizontalHeader()->setSectionResizeMode(ReplicaTableViewModel::CD_INSPECT, QHeaderView::Fixed);
        m_gui->tableView->horizontalHeader()->resizeSection(ReplicaTableViewModel::CD_INSPECT, INSPECT_ICON_COLUMN_SIZE);

        m_gui->tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
        m_gui->tableView->setAlternatingRowColors(true);

        m_gui->tableView->setItemDelegateForColumn(ReplicaTableViewModel::CD_INSPECT, new InspectIconItemDelegate(Qt::AlignCenter, m_gui->tableView));

        QObject::connect(m_gui->tableView->selectionModel(), SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)), this, SLOT(ReplicaSelectionChanged(const QItemSelection&, const QItemSelection&)));
    }

    void ReplicaDataView::SetupChunkTableView()
    {
        m_gui->tableView->setModel(&m_replicaChunkTypeTableView);
        m_gui->tableView->verticalHeader()->hide();

        m_gui->tableView->horizontalHeader()->reset();

        // I think this will fix the sizing issue, but until we update to 5.6 we don't get it! AWESOME
        //m_gui->tableView->horizontalHeader()->resetDefaultSectionSize();

        for (int i = 0; i < m_replicaChunkTypeTableView.columnCount(); ++i)
        {
            m_gui->tableView->horizontalHeader()->resizeSection(i, m_gui->tableView->horizontalHeader()->defaultSectionSize());
        }

        // QT Persists the section resize mode after you call reset on the table, and on the column header.
        // It's pretty special.
        // Going to manually remove the information to avoid something looking really stupid.
        if (ReplicaTableViewModel::CD_INSPECT < m_replicaChunkTypeTableView.columnCount())
        {
            m_gui->tableView->horizontalHeader()->setSectionResizeMode(ReplicaTableViewModel::CD_INSPECT, QHeaderView::Interactive);
        }

        m_gui->tableView->horizontalHeader()->setSectionsClickable(false);
        m_gui->tableView->horizontalHeader()->setSectionResizeMode(ReplicaChunkTypeTableViewModel::CD_INSPECT, QHeaderView::Fixed);
        m_gui->tableView->horizontalHeader()->resizeSection(ReplicaChunkTypeTableViewModel::CD_INSPECT, INSPECT_ICON_COLUMN_SIZE);

        m_gui->tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
        m_gui->tableView->setAlternatingRowColors(true);

        m_gui->tableView->setItemDelegateForColumn(ReplicaChunkTypeTableViewModel::CD_INSPECT, new InspectIconItemDelegate(Qt::AlignCenter, m_gui->tableView));

        QObject::connect(m_gui->tableView->selectionModel(), SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)), this, SLOT(ChunkSelectionChanged(const QItemSelection&, const QItemSelection&)));
    }

    void ReplicaDataView::UpdateData()
    {
        for (FrameNumberType frameId = GetStartFrame(); frameId <= GetEndFrame(); ++frameId)
        {
            ParseFrameData(frameId);
        }

        ParseActiveItems();
    }

    void ReplicaDataView::ParseFrameData(FrameNumberType frameId)
    {
        AZ_PROFILE_TIMER("Standalone Tools", __FUNCTION__);
        if (frameId < 0 || frameId >= m_aggregator->GetFrameCount() || m_parsedFrames.find(frameId) != m_parsedFrames.end())
        {
            return;
        }

        m_parsedFrames.insert(frameId);

        size_t numEvents = m_aggregator->NumOfEventsAtFrame(frameId);

        if (numEvents > 0)
        {
            const ReplicaDataAggregator::EventListType& events = m_aggregator->GetEvents();

            EventNumberType startIndex = m_aggregator->GetFirstIndexAtFrame(frameId);
            for (EventNumberType eventId = startIndex; eventId < static_cast<EventNumberType>(startIndex + numEvents); ++eventId)
            {
                const ReplicaChunkEvent* replicaChunkEvent = static_cast<const ReplicaChunkEvent*>(events[eventId]);

                // Parsing events by ReplicaId
                AZ::u64 replicaId = replicaChunkEvent->GetReplicaId();
                ReplicaDataMap::iterator replicaIter = m_replicaData.find(replicaId);
                ReplicaDataContainer* replicaDataContainer = nullptr;

                if (replicaIter == m_replicaData.end())
                {
                    replicaDataContainer = aznew ReplicaDataContainer(replicaChunkEvent->GetReplicaName(), replicaId, GetRandomDisplayColor());

                    if (replicaDataContainer)
                    {
                        m_replicaData.insert(ReplicaDataMap::value_type(replicaId, replicaDataContainer));
                    }
                }
                else
                {
                    replicaDataContainer = replicaIter->second;
                }

                // Parsing events by ReplicaChunkType
                const AZStd::string& replicaChunkType = replicaChunkEvent->GetChunkTypeName();
                ReplicaChunkTypeDataMap::iterator replicaChunkIter = m_replicaChunkTypeData.find(replicaChunkType);
                ReplicaChunkTypeDataContainer* replicaChunkDataContainer = nullptr;

                if (replicaChunkIter == m_replicaChunkTypeData.end())
                {
                    replicaChunkDataContainer = aznew ReplicaChunkTypeDataContainer(replicaChunkType.c_str(), GetRandomDisplayColor());

                    if (replicaChunkDataContainer)
                    {
                        m_replicaChunkTypeData.insert(ReplicaChunkTypeDataMap::value_type(replicaChunkType, replicaChunkDataContainer));
                    }
                }
                else
                {
                    replicaChunkDataContainer = replicaChunkIter->second;
                }

                if (replicaDataContainer)
                {
                    replicaDataContainer->ProcessReplicaChunkEvent(frameId, replicaChunkEvent);
                }

                if (replicaChunkDataContainer)
                {
                    replicaChunkDataContainer->ProcessReplicaChunkEvent(frameId, replicaChunkEvent);
                }
            }
        }
    }


    void ReplicaDataView::ParseActiveItems()
    {
        switch (GetDisplayDataType())
        {
        case DDT_REPLICA:
            m_activeReplicaIds.clear();
            m_activeInspectedReplicaIds.clear();

            for (AZStd::pair<AZ::u64, ReplicaDataContainer*>& replicaItem : m_replicaData)
            {
                ReplicaDataContainer* dataContainer = replicaItem.second;

                for (FrameNumberType frameId = GetStartFrame(); frameId <= GetEndFrame(); ++frameId)
                {
                    if (dataContainer->HasUsageForFrame(frameId))
                    {
                        m_activeReplicaIds.insert(replicaItem.first);

                        if (dataContainer->HasUsageForFrame(GetCurrentFrame()))
                        {
                            m_activeInspectedReplicaIds.insert(replicaItem.first);
                        }

                        break;
                    }
                }
            }
            break;
        case DDT_CHUNK:
            m_activeChunkTypes.clear();
            m_activeInspectedChunkTypes.clear();

            for (AZStd::pair<AZStd::string, ReplicaChunkTypeDataContainer*>& chunkItem : m_replicaChunkTypeData)
            {
                ReplicaChunkTypeDataContainer* dataContainer = chunkItem.second;

                for (FrameNumberType frameId = GetStartFrame(); frameId <= GetEndFrame(); ++frameId)
                {
                    if (dataContainer->HasUsageForFrame(frameId))
                    {
                        m_activeChunkTypes.insert(chunkItem.first);

                        if (dataContainer->HasUsageForFrame(GetCurrentFrame()))
                        {
                            m_activeInspectedChunkTypes.insert(chunkItem.first);
                        }

                        break;
                    }
                }
            }
            break;
        default:
            AZ_Assert(false, "Unknown Display Data Type");
            break;
        }
    }    

    int ReplicaDataView::GetDisplayRange() const
    {
        return m_persistentState->m_displayRange;
    }

    ReplicaDataView::DisplayDataType ReplicaDataView::GetDisplayDataType() const
    {
        return static_cast<ReplicaDataView::DisplayDataType>(m_persistentState->m_displayDataType);
    }

    ReplicaDisplayTypes::BandwidthUsageDisplayType ReplicaDataView::GetBandwidthUsageDisplayType() const
    {
        return static_cast<ReplicaDisplayTypes::BandwidthUsageDisplayType>(m_persistentState->m_bandwidthUsageDisplayType);
    }

    ReplicaDataContainer* ReplicaDataView::FindReplicaData(AZ::u64 replicaId) const
    {
        ReplicaDataContainer* retVal = nullptr;

        ReplicaDataMap::const_iterator replicaIter = m_replicaData.find(replicaId);

        if (replicaIter != m_replicaData.end())
        {
            retVal = replicaIter->second;
        }

        return retVal;
    }

    ReplicaChunkTypeDataContainer* ReplicaDataView::FindReplicaChunkTypeData(const char* chunkType) const
    {
        ReplicaChunkTypeDataContainer* retVal = nullptr;

        if (chunkType == nullptr)
        {
            return retVal;
        }

        ReplicaChunkTypeDataMap::const_iterator chunkTypeIter = m_replicaChunkTypeData.find(AZStd::string(chunkType));

        if (chunkTypeIter != m_replicaChunkTypeData.end())
        {
            retVal = chunkTypeIter->second;
        }

        return retVal;
    }
}

