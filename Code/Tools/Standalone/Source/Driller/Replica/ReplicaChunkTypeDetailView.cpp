/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "StandaloneTools_precompiled.h"

#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/sort.h>

#include "ReplicaChunkTypeDetailView.h"

#include <Source/Driller/Replica/ui_basedetailview.h>

#include "ReplicaChunkUsageDataContainers.h"
#include "ReplicaDataAggregator.hxx"

namespace Driller
{
    ////////////////////////////////////
    // ReplicaChunkTypeDetailViewModel
    ////////////////////////////////////

    ReplicaChunkTypeDetailViewModel::ReplicaChunkTypeDetailViewModel(ReplicaChunkTypeDetailView* chunkDetailView)
        : BaseDetailTreeViewModel<AZ::u64>(chunkDetailView)
    {
    }

    int ReplicaChunkTypeDetailViewModel::columnCount(const QModelIndex& parentIndex) const
    {
        (void)parentIndex;

        return static_cast<int>(CD_COUNT);
    }

    QVariant ReplicaChunkTypeDetailViewModel::data(const QModelIndex& index, int role) const
    {
        const BaseDisplayHelper* baseDisplay = static_cast<const BaseDisplayHelper*>(index.internalPointer());

        if (role == Qt::BackgroundRole)
        {
            if (baseDisplay->m_inspected)
            {
                return QVariant::fromValue(QColor(94, 94, 178, 255));
            }
        }
        else
        {
            switch (index.column())
            {
                case CD_DISPLAY_NAME:
                    if (role == Qt::DecorationRole)
                    {
                        if (baseDisplay->HasIcon())
                        {
                            return baseDisplay->GetIcon();
                        }
                    }
                    else if (role == Qt::DisplayRole)
                    {
                        QString displayName = baseDisplay->GetDisplayName();

                        if (displayName.isEmpty())
                        {
                            return QString("<unknown>");
                        }
                        else
                        {
                            return displayName;
                        }
                    }
                    else if (role == Qt::TextAlignmentRole)
                    {
                        return QVariant(Qt::AlignVCenter);
                    }
                    
                    break;
                case CD_REPLICA_ID:
                    {
                        if (role == Qt::DisplayRole)
                        {
                            AZ::u64 replicaId = 0;

                            const BaseDisplayHelper* currentDisplay = baseDisplay;

                            while (currentDisplay != nullptr && !azrtti_istypeof<ReplicaDetailDisplayHelper>(currentDisplay))
                            {
                                currentDisplay = currentDisplay->GetParent();
                            }

                            if (currentDisplay)
                            {
                                const ReplicaDetailDisplayHelper* replicaDisplay = static_cast<const ReplicaDetailDisplayHelper*>(currentDisplay);
                                replicaId = replicaDisplay->GetReplicaId();
                            }                       

                            return FormattingHelper::ReplicaID(replicaId);
                        }
                        else if (role == Qt::TextAlignmentRole)
                        {
                            return QVariant(Qt::AlignCenter);
                        }
                    }
                    break;            
                case CD_TOTAL_SENT:
                    if (role == Qt::DisplayRole)
                    {
                        return QString::number(baseDisplay->m_bandwidthUsageAggregator.m_bytesSent);
                    }
                    else if (role == Qt::TextAlignmentRole)
                    {
                        return QVariant(Qt::AlignCenter);
                    }
                    break;
                case CD_TOTAL_RECEIVED:
                    if (role == Qt::DisplayRole)
                    {
                        return QString::number(baseDisplay->m_bandwidthUsageAggregator.m_bytesReceived);
                    }
                    else if (role == Qt::TextAlignmentRole)
                    {
                        return QVariant(Qt::AlignCenter);
                    }
                    break;                
                case CD_RPC_COUNT:
                    if (role == Qt::DisplayRole)
                    {
                        if (azrtti_istypeof<RPCDisplayFilter>(baseDisplay))
                        {
                            size_t count = 0;

                            for (BaseDisplayHelper* displayHelper : baseDisplay->GetChildren())
                            {
                                count += displayHelper->GetChildren().size();
                            }

                            return QString::number(count);
                        }
                        else if (azrtti_istypeof<RPCDisplayHelper>(baseDisplay))
                        {
                            return QString::number(baseDisplay->GetChildren().size());
                        }
                    }
                    break;
                default:
                    AZ_Assert(false,"Unknown column index %i",index.column());
                    break;
            }
        }

        return QVariant();
    }

    QVariant ReplicaChunkTypeDetailViewModel::headerData(int section, Qt::Orientation orientation, int role) const
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
                case CD_TOTAL_RECEIVED:
                    return QString("Received Bytes");
                case CD_RPC_COUNT:
                    return QString("RPC Count");
                default:
                    AZ_Assert(false, "Unknown section index %i", section);
                    break;
                }
            }
        }

        return QVariant();
    }

    ///////////////////////////////
    // ReplicaChunkTypeDetailView
    ///////////////////////////////
    ReplicaChunkTypeDetailView::ReplicaChunkTypeDetailView(ReplicaDataView* replicaDataView, ReplicaChunkTypeDataContainer* chunkTypeDataContainer)
        : BaseDetailView<AZ::u64>(replicaDataView)
        , m_inspectedSeries(AreaChart::AreaChart::k_invalidSeriesId)
        , m_aggregateDisplayHelper(nullptr)
        , m_replicaChunkData(chunkTypeDataContainer)
        , m_chunkTypeDetailView(this)
        , m_lifespanTelemetry("ReplicaChunkTypeDetailView")
    {
        QString replicaChunkType = QString("%1").arg(chunkTypeDataContainer->GetChunkType());

        show();
        raise();
        activateWindow();
        setFocus();

        setWindowTitle(QString("%1's breakdown - %2").arg(replicaChunkType).arg(replicaDataView->m_aggregator->GetInspectionFileName()));

        m_gui->replicaName->setText(replicaChunkType);

        // Ordering here needs to match ordering in BaseDetailView.h
        m_gui->aggregationTypeComboBox->addItem(QString("Replica"));
        m_gui->aggregationTypeComboBox->addItem(QString("Combined"));

        if (m_gui->aggregationTypeComboBox->count() == 1)
        {
            m_gui->aggregationTypeComboBox->setEditable(false);
            m_gui->aggregationTypeComboBox->setEnabled(false);
        }

        QObject::connect((&m_chunkTypeDetailView), SIGNAL(dataChanged(const QModelIndex&, const QModelIndex&)), this, SLOT(UpdateDisplay(const QModelIndex&, const QModelIndex&)));
        QObject::connect(m_gui->aggregationTypeComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(DisplayModeChanged(int)));

        m_gui->aggregationTypeComboBox->setCurrentIndex(static_cast<int>(DisplayMode::Aggregate));
    }

    ReplicaChunkTypeDetailView::~ReplicaChunkTypeDetailView()
    {
        for (ReplicaDetailDisplayMap::iterator displayIter = m_replicaDisplayMapping.begin();
             displayIter != m_replicaDisplayMapping.end();
             ++displayIter)
        {
            delete displayIter->second;
        }
    }

    const ReplicaBandwidthChartData<AZ::u64>::FrameMap& ReplicaChunkTypeDetailView::GetFrameData() const
    {
        return m_replicaChunkData->GetAllFrames();
    }

    BaseDetailDisplayHelper* ReplicaChunkTypeDetailView::FindDetailDisplay(const AZ::u64& replicaId)
    {
        BaseDetailDisplayHelper* retVal = nullptr;

        ReplicaDetailDisplayMap::iterator displayIter = m_replicaDisplayMapping.find(replicaId);

        if (displayIter != m_replicaDisplayMapping.end())
        {
            retVal = displayIter->second;
        }

        return retVal;
    }

    const BaseDetailDisplayHelper* ReplicaChunkTypeDetailView::FindDetailDisplay(const AZ::u64& replicaId) const
    {
        const BaseDetailDisplayHelper* retVal = nullptr;

        ReplicaDetailDisplayMap::const_iterator displayIter = m_replicaDisplayMapping.find(replicaId);

        if (displayIter != m_replicaDisplayMapping.end())
        {
            retVal = displayIter->second;
        }

        return retVal;
    }

    void ReplicaChunkTypeDetailView::InitializeDisplayData()
    {
        m_activeIds.clear();
        m_activeInspectedIds.clear();

        BaseDetailDisplayHelper* aggregateDisplayHelper = FindAggregateDisplay();

        if (aggregateDisplayHelper)
        {
            aggregateDisplayHelper->GetDataSetDisplayHelper()->ClearActiveDisplay();
            aggregateDisplayHelper->GetRPCDisplayHelper()->ClearActiveDisplay();
        }

        const ReplicaChunkTypeDataContainer::FrameMap& frameMap = m_replicaChunkData->GetAllFrames();

        for (FrameNumberType currentFrame = m_replicaDataView->GetStartFrame(); currentFrame <= m_replicaDataView->GetEndFrame(); ++currentFrame)
        {
            ReplicaChunkTypeDataContainer::FrameMap::const_iterator frameIter = frameMap.find(currentFrame);

            if (frameIter == frameMap.end())
            {
                continue;
            }

            const ReplicaChunkTypeDataContainer::BandwidthUsageMap* usageMap = frameIter->second;

            for (ReplicaChunkTypeDataContainer::BandwidthUsageMap::const_iterator usageIter = usageMap->begin();
                 usageIter != usageMap->end();
                 ++usageIter)
            {
                ReplicaBandwidthUsage* bandwidthUsage = static_cast<ReplicaBandwidthUsage*>(usageIter->second);

                ReplicaDetailDisplayMap::iterator displayIter = m_replicaDisplayMapping.find(bandwidthUsage->GetReplicaId());
                ReplicaDetailDisplayHelper* replicaDisplay = nullptr;

                if (displayIter == m_replicaDisplayMapping.end())
                {
                    replicaDisplay = aznew ReplicaDetailDisplayHelper(bandwidthUsage->GetReplicaName(), bandwidthUsage->GetReplicaId());

                    if (replicaDisplay)
                    {
                        m_replicaDisplayMapping.insert(AZStd::make_pair(replicaDisplay->GetReplicaId(), replicaDisplay));
                    }
                }
                else
                {
                    replicaDisplay = displayIter->second;
                }

                // Consider sending along an overall descriptor of the replcia so we can easily setup the display instead
                // of iterating blindly over our detail information trying to get a sense of what the thing is.
                if (replicaDisplay)
                {
                    if (currentFrame == m_replicaDataView->GetCurrentFrame())
                    {
                        m_activeInspectedIds.insert(replicaDisplay->GetReplicaId());
                    }

                    // First time we add an object in we want to reset it's display.
                    if (m_activeIds.insert(replicaDisplay->GetReplicaId()).second)
                    {
                        replicaDisplay->GetDataSetDisplayHelper()->ClearActiveDisplay();
                        replicaDisplay->GetRPCDisplayHelper()->ClearActiveDisplay();
                    }

                    const ReplicaBandwidthUsage::UsageAggregationMap& dataSetBandwidthUsage = bandwidthUsage->GetDataTypeUsageAggregation(BandwidthUsage::DataType::DATA_SET);

                    for (const auto& usagePair : dataSetBandwidthUsage)
                    {
                        const BandwidthUsage& currentUsage = usagePair.second;

                        replicaDisplay->SetupDataSet(currentUsage.m_index, currentUsage.m_identifier.c_str());

                        if (aggregateDisplayHelper)
                        {
                            aggregateDisplayHelper->SetupDataSet(currentUsage.m_index, currentUsage.m_identifier.c_str());
                        }
                    }

                    const ReplicaBandwidthUsage::UsageAggregationMap& rpcBandwidthUsage = bandwidthUsage->GetDataTypeUsageAggregation(BandwidthUsage::DataType::REMOTE_PROCEDURE_CALL);

                    for (const auto& usagePair : rpcBandwidthUsage)
                    {
                        const BandwidthUsage& currentUsage = usagePair.second;

                        replicaDisplay->SetupRPC(currentUsage.m_index, currentUsage.m_identifier.c_str());

                        if (aggregateDisplayHelper)
                        {
                            aggregateDisplayHelper->SetupRPC(currentUsage.m_index, currentUsage.m_identifier.c_str());
                        }
                    }
                }
            }
        }
    }

    BaseDetailDisplayHelper* ReplicaChunkTypeDetailView::FindAggregateDisplay()
    {
        if (m_aggregateDisplayHelper == nullptr)
        {
            m_aggregateDisplayHelper = aznew ReplicaDetailDisplayHelper("Combined Usage", FindAggregateID());
            m_replicaDisplayMapping.insert(AZStd::make_pair(m_aggregateDisplayHelper->GetReplicaId(), m_aggregateDisplayHelper));
        }

        return m_aggregateDisplayHelper;
    }

    AZ::u64 ReplicaChunkTypeDetailView::FindAggregateID() const
    {
        return 0;
    }

    void ReplicaChunkTypeDetailView::LayoutChanged()
    {
        m_chunkTypeDetailView.layoutChanged();
    }

    void ReplicaChunkTypeDetailView::OnSetupTreeView()
    {
        m_gui->treeView->setModel(&m_chunkTypeDetailView);
        ShowTreeFrame(m_replicaDataView->GetCurrentFrame());
    }

    void ReplicaChunkTypeDetailView::ShowTreeFrame(FrameNumberType frameId)
    {
        m_chunkTypeDetailView.RefreshView(frameId);
    }

    AZ::u32 ReplicaChunkTypeDetailView::CreateWindowGeometryCRC()
    {
        return AZ::Crc32("REPLICA_CHUNK_DETAIL_VIEW_WINDOW_STATE");
    }

    AZ::u32 ReplicaChunkTypeDetailView::CreateSplitterStateCRC()
    {
        return AZ::Crc32("REPLICA_CHUNK_DETAIL_VIEW_SPLITTER_STATE");
    }

    AZ::u32 ReplicaChunkTypeDetailView::CreateTreeStateCRC()
    {
        return AZ::Crc32("REPLICA_CHUNK_DETAIL_VIEW_TREE_STATE");
    }

    void ReplicaChunkTypeDetailView::OnInspectedSeries(size_t seriesId)
    {
        if (m_inspectedSeries != seriesId)
        {
            m_inspectedSeries = seriesId;

            // TODO: Handle expanding the tree and scrolling to the selected value.
            for (auto& mapPair : m_replicaDisplayMapping)
            {
                BaseDetailDisplayHelper* displayHelper = mapPair.second;

                displayHelper->InspectSeries(m_inspectedSeries);                
            }

            if (m_aggregateDisplayHelper)
            {
                m_aggregateDisplayHelper->InspectSeries(m_inspectedSeries);
            }

            m_chunkTypeDetailView.layoutChanged();
        }
    }
}
