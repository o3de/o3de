/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "StandaloneTools_precompiled.h"

#include <AzCore/Serialization/SerializeContext.h>

#include <AzToolsFramework/UI/UICore/QTreeViewStateSaver.hxx>
#include <AzToolsFramework/UI/UICore/QWidgetSavedState.h>

#include "OverallReplicaDetailView.hxx"
#include <Source/Driller/Replica/ui_overallreplicadetailview.h>
#include <Source/Driller/Replica/moc_OverallReplicaDetailView.cpp>

#include "ReplicaChunkUsageDataContainers.h"
#include "ReplicaDataAggregator.hxx"
#include "ReplicaDataEvents.h"
#include "ReplicaUsageDataContainers.h"

#include "Source/Driller/DrillerMainWindowMessages.h"
#include "Source/Driller/DrillerOperationTelemetryEvent.h"
#include "Source/Driller/Replica/ReplicaDataView.hxx"
#include "Source/Driller/Replica/ReplicaDisplayHelpers.h"
#include "Source/Driller/Workspaces/Workspace.h"

namespace Driller
{
    ////////////////////////
    // TreeModelSavedState
    ////////////////////////
    class TreeModelSavedState
        : public AZ::UserSettings
    {
    public:
        AZ_RTTI(TreeModelSavedState, "{36103E46-2503-4EEE-BA4B-2650E25A5B26}", AZ::UserSettings);
        AZ_CLASS_ALLOCATOR(TreeModelSavedState, AZ::SystemAllocator, 0);

        AZStd::vector<AZ::u8> m_treeColumnStorage;

        static void Reflect(AZ::ReflectContext* context)
        {
            AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);

            if (serialize)
            {
                serialize->Class<TreeModelSavedState>()
                    ->Field("m_treeColumnStorage", &TreeModelSavedState::m_treeColumnStorage)
                    ->Version(1);
            }
        }
    };

    ////////////////////////////////
    // OverallReplicaTreeViewModel
    ////////////////////////////////

    OverallReplicaTreeViewModel::OverallReplicaTreeViewModel(AbstractOverallReplicaDetailView* overallDetailView)
        : BaseOverallTreeViewModel<AZ::u64>(overallDetailView)
    {
    }

    int OverallReplicaTreeViewModel::columnCount(const QModelIndex& parentIndex) const
    {
        (void)parentIndex;

        return CD_COUNT;
    }

    QVariant OverallReplicaTreeViewModel::data(const QModelIndex& index, int role) const
    {
        const bool relativeValue = true;
        const bool absoluteValue = false;

        const BaseDisplayHelper* baseDisplay = static_cast<const BaseDisplayHelper*>(index.internalPointer());

        switch (index.column())
        {
        case CD_DISPLAY_NAME:
            return displayNameData(baseDisplay, role);
        case CD_REPLICA_ID:
        {
            if (role == Qt::DisplayRole || role == Qt::UserRole)
            {
                AZ::u64 replicaId = 0;

                const BaseDisplayHelper* currentDisplay = baseDisplay;

                while (currentDisplay != nullptr && !azrtti_istypeof<OverallReplicaDetailDisplayHelper>(currentDisplay))
                {
                    currentDisplay = currentDisplay->GetParent();
                }

                if (currentDisplay)
                {
                    const OverallReplicaDetailDisplayHelper* replicaDisplay = static_cast<const OverallReplicaDetailDisplayHelper*>(currentDisplay);
                    replicaId = replicaDisplay->GetReplicaId();
                }

                if (role == Qt::DisplayRole)
                {
                    return FormattingHelper::ReplicaID(replicaId);
                }
                else
                {
                    return QVariant(replicaId);
                }
            }
        }
        break;
        case CD_TOTAL_SENT:
            return totalSentData(baseDisplay, role);
        case CD_AVG_SENT_FRAME:
            return avgSentPerFrameData(baseDisplay, role);
        case CD_AVG_SENT_SECOND:
            return avgSentPerSecondData(baseDisplay, role);
        case CD_PARENT_PERCENT_SENT:
            return percentOfSentData(baseDisplay, role, relativeValue);
        case CD_TOTAL_PERCENT_SENT:
            return percentOfSentData(baseDisplay, role, absoluteValue);
        case CD_TOTAL_RECEIVED:
            return totalReceivedData(baseDisplay, role);
        case CD_AVG_RECEIVED_FRAME:
            return avgReceivedPerFrameData(baseDisplay, role);
        case CD_AVG_RECEIVED_SECOND:
            return avgReceivedPerSecondData(baseDisplay, role);
        case CD_PARENT_PERCENT_RECEIVED:
            return percentOfReceivedData(baseDisplay, role, relativeValue);
        case CD_TOTAL_PERCENT_RECEIVED:
            return percentOfReceivedData(baseDisplay, role, absoluteValue);
        default:
            AZ_Warning("OverallReplicaTreeViewModel", false, "Unknown Column");
            break;
        }

        return QVariant();
    }

    QVariant OverallReplicaTreeViewModel::headerData(int section, Qt::Orientation orientation, int role) const
    {
        if (role == Qt::DisplayRole)
        {
            if (orientation == Qt::Horizontal)
            {
                switch (section)
                {
                case CD_DISPLAY_NAME:
                    return QString("Name");
                case CD_REPLICA_ID:
                    return QString("ReplicaId");
                case CD_TOTAL_SENT:
                    return QString("Sent Bytes");
                case CD_AVG_SENT_FRAME:
                    return QString("Sent Bytes/Frame");
                case CD_AVG_SENT_SECOND:
                    return QString("Sent Bytes/Second");
                case CD_PARENT_PERCENT_SENT:
                    return QString("% of Parent Sent");
                case CD_TOTAL_PERCENT_SENT:
                    return QString("% of Total Sent");
                case CD_TOTAL_RECEIVED:
                    return QString("Received Bytes");
                case CD_AVG_RECEIVED_FRAME:
                    return QString("Received Bytes/Frame");
                case CD_AVG_RECEIVED_SECOND:
                    return QString("Received Bytes/Second");
                case CD_PARENT_PERCENT_RECEIVED:
                    return QString("% of Parent Received");
                case CD_TOTAL_PERCENT_RECEIVED:
                    return QString("% of Total Received");
                }
            }
        }

        return QVariant();
    }

    const BaseDisplayHelper* OverallReplicaTreeViewModel::FindDisplayHelperAtRoot(int row) const
    {
        if (row < 0 || row >= m_tableViewOrdering.size())
        {
            return nullptr;
        }

        return m_overallReplicaDetailView->FindReplicaDisplayHelper(m_tableViewOrdering[row]);
    }

    /////////////////////////////////////////
    // OverallReplicaChunkTypeTreeViewModel
    /////////////////////////////////////////

    OverallReplicaChunkTypeTreeViewModel::OverallReplicaChunkTypeTreeViewModel(AbstractOverallReplicaDetailView* overallDetailView)
        : BaseOverallTreeViewModel<AZStd::string>(overallDetailView)
    {
    }

    int OverallReplicaChunkTypeTreeViewModel::columnCount(const QModelIndex& parentIndex) const
    {
        (void)parentIndex;

        return CD_COUNT;
    }

    QVariant OverallReplicaChunkTypeTreeViewModel::data(const QModelIndex& index, int role) const
    {
        const bool relativeValue = true;
        const bool absoluteValue = false;

        const BaseDisplayHelper* baseDisplay = static_cast<const BaseDisplayHelper*>(index.internalPointer());

        switch (index.column())
        {
        case CD_DISPLAY_NAME:
            return displayNameData(baseDisplay, role);
        case CD_TOTAL_SENT:
            return totalSentData(baseDisplay, role);
        case CD_AVG_SENT_FRAME:
            return avgSentPerFrameData(baseDisplay, role);
        case CD_AVG_SENT_SECOND:
            return avgSentPerSecondData(baseDisplay, role);
        case CD_PARENT_PERCENT_SENT:
            return percentOfSentData(baseDisplay, role, relativeValue);
        case CD_TOTAL_PERCENT_SENT:
            return percentOfSentData(baseDisplay, role, absoluteValue);
        case CD_TOTAL_RECEIVED:
            return totalReceivedData(baseDisplay, role);
        case CD_AVG_RECEIVED_FRAME:
            return avgReceivedPerFrameData(baseDisplay, role);
        case CD_AVG_RECEIVED_SECOND:
            return avgReceivedPerSecondData(baseDisplay, role);
        case CD_PARENT_PERCENT_RECEIVED:
            return percentOfReceivedData(baseDisplay, role, relativeValue);
        case CD_TOTAL_PERCENT_RECEIVED:
            return percentOfReceivedData(baseDisplay, role, absoluteValue);
        default:
            AZ_Warning("OverallReplicaTreeViewModel", false, "Unknown Column");
            break;
        }

        return QVariant();
    }

    QVariant OverallReplicaChunkTypeTreeViewModel::headerData(int section, Qt::Orientation orientation, int role) const
    {
        if (role == Qt::DisplayRole)
        {
            if (orientation == Qt::Horizontal)
            {
                switch (section)
                {
                case CD_DISPLAY_NAME:
                    return QString("Name");
                case CD_TOTAL_SENT:
                    return QString("Sent Bytes");
                case CD_AVG_SENT_FRAME:
                    return QString("Sent Bytes/Frame");
                case CD_AVG_SENT_SECOND:
                    return QString("Sent Bytes/Second");
                case CD_PARENT_PERCENT_SENT:
                    return QString("% of Parent Sent");
                case CD_TOTAL_PERCENT_SENT:
                    return QString("% of Total Sent");
                case CD_TOTAL_RECEIVED:
                    return QString("Received Bytes");
                case CD_AVG_RECEIVED_FRAME:
                    return QString("Received Bytes/Frame");
                case CD_AVG_RECEIVED_SECOND:
                    return QString("Received Bytes/Second");
                case CD_PARENT_PERCENT_RECEIVED:
                    return QString("% of Parent Received");
                case CD_TOTAL_PERCENT_RECEIVED:
                    return QString("% of Total Received");
                }
            }
        }

        return QVariant();
    }

    const BaseDisplayHelper* OverallReplicaChunkTypeTreeViewModel::FindDisplayHelperAtRoot(int row) const
    {
        if (row < 0 || row >= m_tableViewOrdering.size())
        {
            return nullptr;
        }

        return m_overallReplicaDetailView->FindReplicaChunkTypeDisplayHelper(m_tableViewOrdering[row]);
    }

    /////////////////////////////
    // OverallReplicaDetailView
    /////////////////////////////

    const char* OverallReplicaDetailView::WINDOW_STATE_FORMAT               = "OVERALL_REPLICA_DETAIL_VIEW_WINDOW_STATE";
    const char* OverallReplicaDetailView::REPLICA_TREE_STATE_FORMAT         = "OVERALL_REPLICA_DETAIL_VIEW_TREE_STATE";
    const char* OverallReplicaDetailView::REPLICA_CHUNK_TREE_STATE_FORMAT   = "OVERALL_REPLICA_CHUNK_DETAIL_VIEW_TREE_STATE";

    OverallReplicaDetailView::OverallReplicaDetailView(ReplicaDataView* dataView, const ReplicaDataAggregator& dataAggregator)
        : AbstractOverallReplicaDetailView()
        , m_lifespanTelemetry("OverallReplicaDetailView")
        , m_replicaDataView(dataView)
        , m_windowStateCRC(AZ::Crc32(WINDOW_STATE_FORMAT))
        , m_replicaTreeStateCRC(AZ::Crc32(REPLICA_TREE_STATE_FORMAT))
        , m_replicaChunkTreeStateCRC(AZ::Crc32(REPLICA_CHUNK_TREE_STATE_FORMAT))
        , m_dataAggregator(dataAggregator)
        , m_overallReplicaModel(this)
        , m_replicaFilterProxyModel(this)
        , m_overallChunkTypeModel(this)
        , m_replicaChunkTypeFilterProxyModel(this)
    {
        setAttribute(Qt::WA_DeleteOnClose, true);
        setWindowFlags(windowFlags() | Qt::WindowMaximizeButtonHint | Qt::WindowMinimizeButtonHint);

        m_gui = azcreate(Ui::OverallReplicaDetailView, ());
        m_gui->setupUi(this);

        show();
        raise();
        activateWindow();
        setFocus();

        QString titleName("Overall Replica Usage - %2");
        this->setWindowTitle(titleName.arg(m_dataAggregator.GetInspectionFileName()));

        // Loading up saved state
        AZStd::intrusive_ptr<AzToolsFramework::QWidgetSavedState> windowState = AZ::UserSettings::Find<AzToolsFramework::QWidgetSavedState>(m_windowStateCRC, AZ::UserSettings::CT_GLOBAL);

        if (windowState)
        {
            windowState->RestoreGeometry(this);
        }
        auto treeState = AZ::UserSettings::Find<TreeModelSavedState>(m_replicaTreeStateCRC, AZ::UserSettings::CT_GLOBAL);

        if (treeState)
        {
            QByteArray treeData((const char*)treeState->m_treeColumnStorage.data(), (int)treeState->m_treeColumnStorage.size());
            m_gui->overallReplicaUsage->header()->restoreState(treeData);
        }

        treeState = AZ::UserSettings::Find<TreeModelSavedState>(m_replicaChunkTreeStateCRC, AZ::UserSettings::CT_GLOBAL);

        if (treeState)
        {
            QByteArray treeData((const char*)treeState->m_treeColumnStorage.data(), (int)treeState->m_treeColumnStorage.size());
            m_gui->overallChunkTypeUsage->header()->restoreState(treeData);
        }

        m_gui->startFrame->setMinimum(0);
        m_gui->startFrame->setValue(0);

        m_gui->endFrame->setMaximum(static_cast<int>(m_dataAggregator.GetFrameCount()) - 1);
        m_gui->endFrame->setValue(static_cast<int>(m_dataAggregator.GetFrameCount() - 1));

        m_changeTimer.setInterval(500);
        m_changeTimer.setSingleShot(true);

        UpdateFrameBoundaries();
        ParseData();
        SetupTreeView();
        UpdateDisplay();

        QObject::connect(m_gui->startFrame, SIGNAL(valueChanged(int)), this, SLOT(QueueUpdate(int)));
        QObject::connect(m_gui->endFrame, SIGNAL(valueChanged(int)), this, SLOT(QueueUpdate(int)));
        QObject::connect(m_gui->framesPerSecond, SIGNAL(valueChanged(int)),this,SLOT(OnFPSChanged(int)));
        connect(&m_changeTimer, SIGNAL(timeout()), SLOT(OnDataRangeChanged()));
    }

    OverallReplicaDetailView::~OverallReplicaDetailView()
    {
        ClearData();

        // Save out whatever data we want to save out.
        auto pState = AZ::UserSettings::CreateFind<AzToolsFramework::QWidgetSavedState>(m_windowStateCRC, AZ::UserSettings::CT_GLOBAL);
        if (pState)
        {
            pState->CaptureGeometry(this);
        }

        auto treeState = AZ::UserSettings::CreateFind<TreeModelSavedState>(m_replicaTreeStateCRC, AZ::UserSettings::CT_GLOBAL);
        if (treeState)
        {
            if (m_gui->overallReplicaUsage && m_gui->overallReplicaUsage->header())
            {
                QByteArray qba = m_gui->overallReplicaUsage->header()->saveState();
                treeState->m_treeColumnStorage.assign((AZ::u8*)qba.begin(), (AZ::u8*)qba.end());
            }
        }

        treeState = AZ::UserSettings::CreateFind<TreeModelSavedState>(m_replicaChunkTreeStateCRC, AZ::UserSettings::CT_GLOBAL);
        if (treeState)
        {
            if (m_gui->overallChunkTypeUsage && m_gui->overallChunkTypeUsage->header())
            {
                QByteArray qba = m_gui->overallChunkTypeUsage->header()->saveState();
                treeState->m_treeColumnStorage.assign((AZ::u8*)qba.begin(), (AZ::u8*)qba.end());
            }
        }

        if (m_replicaDataView)
        {
            m_replicaDataView->SignalDialogClosed(this);
        }

        azdestroy(m_gui);
    }

    int OverallReplicaDetailView::GetFrameRange() const
    {
        return m_frameRange;
    }

    int OverallReplicaDetailView::GetFPS() const
    {
        return m_gui->framesPerSecond->value();
    }

    void OverallReplicaDetailView::SignalDataViewDestroyed(ReplicaDataView* replicaDataView)
    {
        if (m_replicaDataView == replicaDataView)
        {
            m_replicaDataView = nullptr;
        }

        close();
    }

    void OverallReplicaDetailView::ApplySettingsFromWorkspace(WorkspaceSettingsProvider*)
    {
    }

    void OverallReplicaDetailView::ActivateWorkspaceSettings(WorkspaceSettingsProvider*)
    {
    }

    void OverallReplicaDetailView::SaveSettingsToWorkspace(WorkspaceSettingsProvider*)
    {
    }

    void OverallReplicaDetailView::ApplyPersistentState()
    {
    }

    void OverallReplicaDetailView::Reflect(AZ::ReflectContext* context)
    {
        (void)context;
    }

    void OverallReplicaDetailView::OnFPSChanged(int fps)
    {
        (void)fps;

        UpdateDisplay();
    }

    void OverallReplicaDetailView::QueueUpdate(int ignoredFrame)
    {
        (void)ignoredFrame;

        m_changeTimer.start();
    }

    void OverallReplicaDetailView::OnDataRangeChanged()
    {
        ParseData();
        UpdateFrameBoundaries();
        UpdateDisplay();
    }

    OverallReplicaDetailDisplayHelper* OverallReplicaDetailView::CreateReplicaDisplayHelper(const char* replicaName, AZ::u64 replicaId)
    {
        OverallReplicaDetailDisplayHelper* detailDisplay = nullptr;

        ReplicaDisplayHelperMap::iterator displayIter = m_replicaDisplayHelpers.find(replicaId);

        if (displayIter == m_replicaDisplayHelpers.end())
        {
            detailDisplay = aznew OverallReplicaDetailDisplayHelper(replicaName, replicaId);
            m_replicaDisplayHelpers.insert(ReplicaDisplayHelperMap::value_type(replicaId, detailDisplay));

            m_overallReplicaModel.m_tableViewOrdering.push_back(replicaId);
        }
        else
        {
            detailDisplay = displayIter->second;
        }

        return detailDisplay;
    }

    OverallReplicaDetailDisplayHelper* OverallReplicaDetailView::FindReplicaDisplayHelper(AZ::u64 replicaId)
    {
        OverallReplicaDetailDisplayHelper* detailDisplay = nullptr;

        ReplicaDisplayHelperMap::iterator displayIter = m_replicaDisplayHelpers.find(replicaId);

        if (displayIter != m_replicaDisplayHelpers.end())
        {
            detailDisplay = displayIter->second;
        }

        return detailDisplay;
    }

    ReplicaChunkDetailDisplayHelper* OverallReplicaDetailView::CreateReplicaChunkTypeDisplayHelper(const AZStd::string& chunkTypeName, AZ::u32 chunkIndex)
    {
        ReplicaChunkDetailDisplayHelper* detailDisplay = nullptr;

        ReplicaChunkTypeDisplayHelperMap::iterator displayIter = m_replicaChunkTypeDisplayHelpers.find(chunkTypeName);

        if (displayIter == m_replicaChunkTypeDisplayHelpers.end())
        {
            detailDisplay = aznew ReplicaChunkDetailDisplayHelper(chunkTypeName.c_str(), chunkIndex);
            m_replicaChunkTypeDisplayHelpers.insert(ReplicaChunkTypeDisplayHelperMap::value_type(chunkTypeName, detailDisplay));

            m_overallChunkTypeModel.m_tableViewOrdering.push_back(chunkTypeName);
        }
        else
        {
            detailDisplay = displayIter->second;
        }

        return detailDisplay;
    }

    ReplicaChunkDetailDisplayHelper* OverallReplicaDetailView::FindReplicaChunkTypeDisplayHelper(const AZStd::string& chunkTypeName)
    {
        ReplicaChunkDetailDisplayHelper* detailDisplay = nullptr;

        ReplicaChunkTypeDisplayHelperMap::iterator displayIter = m_replicaChunkTypeDisplayHelpers.find(chunkTypeName);

        if (displayIter != m_replicaChunkTypeDisplayHelpers.end())
        {
            detailDisplay = displayIter->second;
        }

        return detailDisplay;
    }

    void OverallReplicaDetailView::SaveOnExit()
    {
    }

    void OverallReplicaDetailView::UpdateFrameBoundaries()
    {
        m_gui->startFrame->setMaximum(m_gui->endFrame->value());
        m_gui->endFrame->setMinimum(m_gui->startFrame->value());

        m_frameRange = (m_gui->endFrame->value() - m_gui->startFrame->value()) + 1;

        if (m_frameRange <= 0)
        {
            m_frameRange = 1;
        }
    }

    void OverallReplicaDetailView::ParseData()
    {
        // Not the best approach. But this shouldn't update all that frequently.
        ClearData();

        FrameNumberType startFrame = static_cast<FrameNumberType>(m_gui->startFrame->value());
        FrameNumberType endFrame = static_cast<FrameNumberType>(m_gui->endFrame->value());

        EventNumberType startIndex = m_dataAggregator.GetFirstIndexAtFrame(startFrame);
        EventNumberType endIndex = m_dataAggregator.GetFirstIndexAtFrame(endFrame) + m_dataAggregator.NumOfEventsAtFrame(endFrame);

        const Aggregator::EventListType& events = m_dataAggregator.GetEvents();

        for (EventNumberType currentIndex = startIndex; currentIndex < endIndex; ++currentIndex)
        {
            ReplicaChunkEvent* chunkEvent = static_cast<ReplicaChunkEvent*>(events[currentIndex]);

            // Since I process each event twice, I need to do the total aggregation seperately.
            if (chunkEvent->GetEventType() == Replica::RET_CHUNK_DATASET_SENT
                ||  chunkEvent->GetEventType() == Replica::RET_CHUNK_RPC_SENT)
            {
                m_totalUsageAggregator.m_bytesSent += chunkEvent->GetUsageBytes();
            }
            else
            {
                m_totalUsageAggregator.m_bytesReceived += chunkEvent->GetUsageBytes();
            }

            ProcessForReplica(chunkEvent);
            ProcessForReplicaChunk(chunkEvent);
        }
    }

    void OverallReplicaDetailView::ProcessForReplica(ReplicaChunkEvent* chunkEvent)
    {
        const char* replicaName = chunkEvent->GetReplicaName();
        AZ::u64 replicaId = chunkEvent->GetReplicaId();

        OverallReplicaDetailDisplayHelper* replicaDisplayHelper = CreateReplicaDisplayHelper(replicaName, replicaId);

        if (replicaDisplayHelper)
        {
            if (chunkEvent->GetEventType() == Replica::RET_CHUNK_DATASET_SENT
                || chunkEvent->GetEventType() == Replica::RET_CHUNK_RPC_SENT)
            {
                replicaDisplayHelper->m_bandwidthUsageAggregator.m_bytesSent += chunkEvent->GetUsageBytes();
            }
            else if (chunkEvent->GetEventType() == Replica::RET_CHUNK_RPC_RECEIVED
                     ||  chunkEvent->GetEventType() == Replica::RET_CHUNK_DATASET_RECEIVED)
            {
                replicaDisplayHelper->m_bandwidthUsageAggregator.m_bytesReceived += chunkEvent->GetUsageBytes();
            }
            
            ReplicaChunkDetailDisplayHelper* chunkDetailDisplayHelper = replicaDisplayHelper->FindReplicaChunk(chunkEvent->GetReplicaChunkIndex());

            if (chunkDetailDisplayHelper == nullptr)
            {
                chunkDetailDisplayHelper = replicaDisplayHelper->CreateReplicaChunkDisplayHelper(chunkEvent->GetChunkTypeName(), chunkEvent->GetReplicaChunkIndex());
            }
            
            ProcessForBaseDetailDisplayHelper(chunkEvent, chunkDetailDisplayHelper);
        }
    }

    void OverallReplicaDetailView::ProcessForReplicaChunk(ReplicaChunkEvent* chunkEvent)
    {
        AZ::u32 chunkId = chunkEvent->GetReplicaChunkIndex();
        AZStd::string chunkTypeName = chunkEvent->GetChunkTypeName();

        ReplicaChunkDetailDisplayHelper* chunkDisplayHelper = CreateReplicaChunkTypeDisplayHelper(chunkTypeName, chunkId);

        if (chunkDisplayHelper)
        {
            ProcessForBaseDetailDisplayHelper(chunkEvent, chunkDisplayHelper);
        }
    }

    void OverallReplicaDetailView::ProcessForBaseDetailDisplayHelper(ReplicaChunkEvent* chunkEvent, BaseDetailDisplayHelper* detailDisplayHelper)
    {
        if (chunkEvent->GetEventType() == Replica::RET_CHUNK_DATASET_SENT
            ||  chunkEvent->GetEventType() == Replica::RET_CHUNK_DATASET_RECEIVED)
        {
            ReplicaChunkDataSetEvent* dataSetEvent = static_cast<ReplicaChunkDataSetEvent*>(chunkEvent);

            detailDisplayHelper->SetupDataSet(dataSetEvent->GetIndex(), dataSetEvent->GetDataSetName());

            DataSetDisplayFilter* dataSetDisplayFilter = detailDisplayHelper->GetDataSetDisplayHelper();
            DataSetDisplayHelper* dataSetDisplayHelper = detailDisplayHelper->FindDataSet(dataSetEvent->GetIndex());

            if (chunkEvent->GetEventType() == Replica::RET_CHUNK_DATASET_SENT)
            {
                detailDisplayHelper->m_bandwidthUsageAggregator.m_bytesSent += chunkEvent->GetUsageBytes();
                dataSetDisplayFilter->m_bandwidthUsageAggregator.m_bytesSent += chunkEvent->GetUsageBytes();
                dataSetDisplayHelper->m_bandwidthUsageAggregator.m_bytesSent += chunkEvent->GetUsageBytes();
            }
            else
            {
                detailDisplayHelper->m_bandwidthUsageAggregator.m_bytesReceived += chunkEvent->GetUsageBytes();
                dataSetDisplayFilter->m_bandwidthUsageAggregator.m_bytesReceived += chunkEvent->GetUsageBytes();
                dataSetDisplayHelper->m_bandwidthUsageAggregator.m_bytesReceived += chunkEvent->GetUsageBytes();
            }
        }
        else
        {
            ReplicaChunkRPCEvent* rpcEvent = static_cast<ReplicaChunkRPCEvent*>(chunkEvent);

            detailDisplayHelper->SetupRPC(rpcEvent->GetIndex(), rpcEvent->GetRPCName());

            RPCDisplayFilter* rpcDisplayFilter = detailDisplayHelper->GetRPCDisplayHelper();
            RPCDisplayHelper* rpcDisplayHelper = detailDisplayHelper->FindRPC(rpcEvent->GetIndex());

            if (chunkEvent->GetEventType() == Replica::RET_CHUNK_RPC_SENT)
            {
                detailDisplayHelper->m_bandwidthUsageAggregator.m_bytesSent += chunkEvent->GetUsageBytes();
                rpcDisplayFilter->m_bandwidthUsageAggregator.m_bytesSent += chunkEvent->GetUsageBytes();
                rpcDisplayHelper->m_bandwidthUsageAggregator.m_bytesSent += chunkEvent->GetUsageBytes();
            }
            else
            {
                detailDisplayHelper->m_bandwidthUsageAggregator.m_bytesReceived += chunkEvent->GetUsageBytes();
                rpcDisplayFilter->m_bandwidthUsageAggregator.m_bytesReceived += chunkEvent->GetUsageBytes();
                rpcDisplayHelper->m_bandwidthUsageAggregator.m_bytesReceived += chunkEvent->GetUsageBytes();
            }
        }
    }

    void OverallReplicaDetailView::ClearData()
    {
        for (auto& mapPair : m_replicaDisplayHelpers)
        {
            delete mapPair.second;
        }
        m_replicaDisplayHelpers.clear();
        m_overallReplicaModel.m_tableViewOrdering.clear();

        for (auto& mapPair : m_replicaChunkTypeDisplayHelpers)
        {
            delete mapPair.second;
        }
        m_replicaChunkTypeDisplayHelpers.clear();
        m_overallChunkTypeModel.m_tableViewOrdering.clear();

        m_totalUsageAggregator.m_bytesSent = 0;
        m_totalUsageAggregator.m_bytesReceived = 0;
    }

    void OverallReplicaDetailView::UpdateDisplay()
    {
        int frameRange = GetFrameRange();

        // Sent Total
        m_gui->totalBytesSent->setText(QString::number(m_totalUsageAggregator.m_bytesSent));

        size_t avgBytesPerFrame = m_totalUsageAggregator.m_bytesSent / frameRange;
        m_gui->avgBytesSentFrame->setText(QString::number(avgBytesPerFrame));
        m_gui->avgBytesSentSecond->setText(QString::number(avgBytesPerFrame * GetFPS()));

        // Received Total
        m_gui->totalBytesReceived->setText(QString::number(m_totalUsageAggregator.m_bytesReceived));

        avgBytesPerFrame = m_totalUsageAggregator.m_bytesReceived / frameRange;
        m_gui->avgBytesReceivedFrame->setText(QString::number(avgBytesPerFrame));
        m_gui->avgBytesReceivedSecond->setText(QString::number(avgBytesPerFrame * GetFPS()));

        m_overallChunkTypeModel.layoutChanged();
        m_overallReplicaModel.layoutChanged();
    }

    void OverallReplicaDetailView::SetupTreeView()
    {
        SetupReplicaTreeView();
        SetupReplicaChunkTypeTreeView();
    }

    void OverallReplicaDetailView::SetupReplicaTreeView()
    {
        m_replicaFilterProxyModel.setSortRole(Qt::UserRole);
        m_replicaFilterProxyModel.setSourceModel(&m_overallReplicaModel);

        m_gui->overallReplicaUsage->setModel(&m_replicaFilterProxyModel);
    }

    void OverallReplicaDetailView::SetupReplicaChunkTypeTreeView()
    {
        m_replicaChunkTypeFilterProxyModel.setSortRole(Qt::UserRole);
        m_replicaChunkTypeFilterProxyModel.setSourceModel(&m_overallChunkTypeModel);

        m_gui->overallChunkTypeUsage->setModel(&m_replicaChunkTypeFilterProxyModel);
    }
};
