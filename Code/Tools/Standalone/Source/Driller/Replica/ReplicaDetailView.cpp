/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "StandaloneTools_precompiled.h"

#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/sort.h>

#include "ReplicaDetailView.h"
#include <Source/Driller/Replica/ui_basedetailview.h>

#include "ReplicaDataAggregator.hxx"
#include "ReplicaUsageDataContainers.h"


namespace Driller
{
    ///////////////////////////
    // ReplicaDetailViewModel
    ///////////////////////////

    ReplicaDetailViewModel::ReplicaDetailViewModel(ReplicaDetailView* detailView)
        : BaseDetailTreeViewModel<AZ::u32>(detailView)
    {
    }

    int ReplicaDetailViewModel::columnCount(const QModelIndex& parentIndex) const
    {
        (void)parentIndex;

        return static_cast<int>(CD_COUNT);
    }

    QVariant ReplicaDetailViewModel::data(const QModelIndex& index, int role) const
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
                        return baseDisplay->GetDisplayName();
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


    QVariant ReplicaDetailViewModel::headerData(int section, Qt::Orientation orientation, int role) const
    {
        if (role == Qt::DisplayRole)
        {
            if (orientation == Qt::Horizontal)
            {
                switch (section)
                {
                case CD_DISPLAY_NAME:
                    return QString("Display Name");
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

    //////////////////////
    // ReplicaDetailView
    //////////////////////
    ReplicaDetailView::ReplicaDetailView(ReplicaDataView* replicaDataView, ReplicaDataContainer* dataContainer)
        : BaseDetailView<AZ::u32>(replicaDataView)
        , m_inspectedSeries(AreaChart::AreaChart::k_invalidSeriesId)
        , m_replicaData(dataContainer)
        , m_replicaDetailView(this)
        , m_lifespanTelemetry("ReplicaDetailView")
    {
        QString replicaName = QString("%1 (%2)").arg(QString(dataContainer->GetReplicaName())).arg(FormattingHelper::ReplicaID(dataContainer->GetReplicaId()));

        show();
        raise();
        activateWindow();
        setFocus();

        setWindowTitle(QString("%1's ReplicaChunk Breakdown - %2").arg(replicaName).arg(replicaDataView->m_aggregator->GetInspectionFileName()));

        m_gui->replicaName->setText(replicaName);

        m_gui->aggregationTypeComboBox->addItem(QString("Replica Chunk"));

        if (m_gui->aggregationTypeComboBox->count() == 1)
        {
            m_gui->aggregationTypeComboBox->setEditable(false);
            m_gui->aggregationTypeComboBox->setEnabled(false);
        }

        QObject::connect((&m_replicaDetailView), SIGNAL(dataChanged(const QModelIndex&, const QModelIndex&)), this, SLOT(UpdateDisplay(const QModelIndex&, const QModelIndex&)));        
        QObject::connect(m_gui->aggregationTypeComboBox,SIGNAL(currentIndexChanged(int)),this,SLOT(DisplayModeChanged(int)));
        QObject::connect(m_gui->bandwidthUsageDisplayType, SIGNAL(currentIndexChanged(int)), this, SLOT(BandwidthDisplayUsageTypeChanged(int)));
        QObject::connect(m_gui->graphDetailType, SIGNAL(currentIndexChanged(int)), this, SLOT(GraphDetailChanged(int)));

        DisplayModeChanged(static_cast<int>(DisplayMode::Active));
    }

    ReplicaDetailView::~ReplicaDetailView()
    {
        for (ChunkDetailDisplayMap::iterator displayIter = m_typeDisplayMapping.begin();
             displayIter != m_typeDisplayMapping.end();
             ++displayIter)
        {
            delete displayIter->second;
        }
    }

    const ReplicaBandwidthChartData<AZ::u32>::FrameMap& ReplicaDetailView::GetFrameData() const
    {
        return m_replicaData->GetAllFrames();
    }

    BaseDetailDisplayHelper* ReplicaDetailView::FindDetailDisplay(const AZ::u32& chunkIndex)
    {
        BaseDetailDisplayHelper* retVal = nullptr;

        ChunkDetailDisplayMap::iterator displayIter = m_typeDisplayMapping.find(chunkIndex);

        if (displayIter != m_typeDisplayMapping.end())
        {
            retVal = displayIter->second;
        }

        return retVal;
    }

    const BaseDetailDisplayHelper* ReplicaDetailView::FindDetailDisplay(const AZ::u32& chunkIndex) const
    {
        const BaseDetailDisplayHelper* retVal = nullptr;

        ChunkDetailDisplayMap::const_iterator displayIter = m_typeDisplayMapping.find(chunkIndex);

        if (displayIter != m_typeDisplayMapping.end())
        {
            retVal = displayIter->second;
        }

        return retVal;
    }

    void ReplicaDetailView::InitializeDisplayData()
    {
        m_activeIds.clear();
        m_activeInspectedIds.clear();

        const ReplicaDataContainer::FrameMap& frameMap = m_replicaData->GetAllFrames();

        for (FrameNumberType currentFrame = m_replicaDataView->GetStartFrame(); currentFrame <= m_replicaDataView->GetEndFrame(); ++currentFrame)
        {
            ReplicaDataContainer::FrameMap::const_iterator frameIter = frameMap.find(currentFrame);

            if (frameIter == frameMap.end())
            {
                continue;
            }

            const ReplicaDataContainer::BandwidthUsageMap* usageMap = frameIter->second;

            for (ReplicaDataContainer::BandwidthUsageMap::const_iterator usageIter = usageMap->begin();
                 usageIter != usageMap->end();
                 ++usageIter)
            {
                ReplicaChunkBandwidthUsage* bandwidthUsage = static_cast<ReplicaChunkBandwidthUsage*>(usageIter->second);

                ChunkDetailDisplayMap::iterator displayIter = m_typeDisplayMapping.find(bandwidthUsage->GetChunkIndex());

                ReplicaChunkDetailDisplayHelper* chunkTypeDisplay = nullptr;

                if (displayIter == m_typeDisplayMapping.end())
                {
                    chunkTypeDisplay = aznew ReplicaChunkDetailDisplayHelper(bandwidthUsage->GetChunkTypeName(), bandwidthUsage->GetChunkIndex());

                    if (chunkTypeDisplay)
                    {
                        m_typeDisplayMapping.insert(AZStd::make_pair(bandwidthUsage->GetChunkIndex(), chunkTypeDisplay));
                    }
                }
                else
                {
                    chunkTypeDisplay = displayIter->second;
                }

                // Consider sending along an overall descriptor of the replcia so we can easily setup the display instead
                // of iterating blindly over our detail information trying to get a sense of what the thing is.
                if (chunkTypeDisplay)
                {
                    if (currentFrame == m_replicaDataView->GetCurrentFrame())
                    {
                        m_activeInspectedIds.insert(chunkTypeDisplay->GetChunkIndex());
                    }

                    if (m_activeIds.insert(chunkTypeDisplay->GetChunkIndex()).second)
                    {
                        chunkTypeDisplay->GetDataSetDisplayHelper()->ClearActiveDisplay();
                        chunkTypeDisplay->GetRPCDisplayHelper()->ClearActiveDisplay();
                    }

                    const ReplicaChunkBandwidthUsage::UsageAggregationMap& dataSetUsage = bandwidthUsage->GetDataTypeUsageAggregation(BandwidthUsage::DataType::DATA_SET);

                    for (const auto& usagePair : dataSetUsage)
                    {
                        const BandwidthUsage& currentUsage = usagePair.second;
                        chunkTypeDisplay->SetupDataSet(currentUsage.m_index, currentUsage.m_identifier.c_str());
                    }

                    const ReplicaChunkBandwidthUsage::UsageAggregationMap& rpcUsage = bandwidthUsage->GetDataTypeUsageAggregation(BandwidthUsage::DataType::REMOTE_PROCEDURE_CALL);

                    for (const auto& usagePair : rpcUsage)
                    {
                        const BandwidthUsage& currentUsage = usagePair.second;
                        chunkTypeDisplay->SetupRPC(currentUsage.m_index, currentUsage.m_identifier.c_str());
                    }
                }
            }
        }
    }

    void ReplicaDetailView::LayoutChanged()
    {
        m_replicaDetailView.layoutChanged();
    }

    void ReplicaDetailView::OnSetupTreeView()
    {
        m_gui->treeView->setModel(&m_replicaDetailView);
        ShowTreeFrame(m_replicaDataView->GetCurrentFrame());
    }

    void ReplicaDetailView::ShowTreeFrame(FrameNumberType frameId)
    {
        m_replicaDetailView.RefreshView(frameId);
    }

    AZ::u32 ReplicaDetailView::CreateWindowGeometryCRC()
    {
        return AZ::Crc32("REPLICA_DETAIL_VIEW_WINDOW_STATE");
    }

    AZ::u32 ReplicaDetailView::CreateSplitterStateCRC()
    {
        return AZ::Crc32("REPLICA_DETAIL_VIEW_SPLITTER_STATE");
    }

    AZ::u32 ReplicaDetailView::CreateTreeStateCRC()
    {
        return AZ::Crc32("REPLICA_DETAIL_VIEW_TREE_STATE");
    }

    void ReplicaDetailView::OnInspectedSeries(size_t seriesId)
    {
        if (m_inspectedSeries != seriesId)
        {
            m_inspectedSeries = seriesId;

            // TODO: Handle expanding the tree and scrolling to the selected value.
            for (auto& mapPair : m_typeDisplayMapping)
            {
                BaseDetailDisplayHelper* displayHelper = mapPair.second;

                displayHelper->m_inspected = displayHelper->m_areaGraphPlotHelper.IsSeries(m_inspectedSeries);
            }

            m_replicaDetailView.layoutChanged();
        }
    }
}
