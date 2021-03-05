/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

namespace Driller
{
    template<typename Key>
    void BaseDetailView<Key>::DrawActiveGraph()
    {
        const BandwidthUsageContainer emptyContainer;
        const typename ReplicaBandwidthChartData<Key>::BandwidthUsageMap s_emptyMap;

        ConfigureGraphAxis();

        for (const Key& currentId : m_activeIds)
        {
            BaseDetailDisplayHelper* detailDisplayHelper = FindDetailDisplay(currentId);

            ConfigureBaseDetailDisplayHelper(detailDisplayHelper);
        }        
        
        const typename ReplicaBandwidthChartData<Key>::FrameMap& frameMap = GetFrameData();

        for (FrameNumberType frameId = m_replicaDataView->GetStartFrame(); frameId <= m_replicaDataView->GetEndFrame(); ++frameId)
        {            
            typename ReplicaBandwidthChartData<Key>::FrameMap::const_iterator frameIter = frameMap.find(frameId);
            const typename ReplicaBandwidthChartData<Key>::BandwidthUsageMap* usageMap = nullptr;
            
            if (frameIter != frameMap.end())
            {
                usageMap = frameIter->second;
            }
            else
            {
                usageMap = &s_emptyMap;
            }

            AZ_Assert(usageMap != nullptr,"Null pointer added to data map");
            if (usageMap == nullptr)
            {
                continue;
            }                
            
            for (const Key& currentId : m_activeIds)
            {
                const BandwidthUsageContainer* usageContainer = &emptyContainer;
                typename ReplicaBandwidthChartData<Key>::BandwidthUsageMap::const_iterator usageIter = usageMap->find(currentId);

                if (usageIter != usageMap->end())
                {
                    usageContainer = usageIter->second;                    
                }
                else
                {
                    usageContainer = &emptyContainer;
                }

                BaseDetailDisplayHelper* detailDisplayHelper = FindDetailDisplay(currentId);

                switch (m_detailMode)
                {
                case DetailMode::Low:
                    DrawLowDetailActiveGraph(detailDisplayHelper,usageContainer,frameId);
                    break;
                case DetailMode::Medium:
                    DrawMediumDetailActiveGraph(detailDisplayHelper, usageContainer, frameId);
                    break;
                case DetailMode::High:
                    DrawHighDetailActiveGraph(detailDisplayHelper, usageContainer, frameId);
                    break;
                default:
                    break;
                }
            }                
        }        
    }

    template<typename Key>
    void BaseDetailView<Key>::DrawLowDetailActiveGraph(BaseDetailDisplayHelper* detailDisplayHelper, const BandwidthUsageContainer* usageContainer, FrameNumberType frameId)
    {
        if (detailDisplayHelper->m_graphEnabled)
        {
            BandwidthUsageAggregator usageAggregator;
            usageAggregator.m_bytesSent = usageContainer->GetTotalBytesSent();
            usageAggregator.m_bytesReceived = usageContainer->GetTotalBytesReceived();            

            PlotBatchedGraphData(detailDisplayHelper->m_areaGraphPlotHelper, frameId, usageAggregator);
        }
    }

    template<typename Key>
    void BaseDetailView<Key>::DrawMediumDetailActiveGraph(BaseDetailDisplayHelper* detailDisplayHelper, const BandwidthUsageContainer* usageContainer, FrameNumberType frameId)
    {
        DataSetDisplayFilter* dataSetFilter = detailDisplayHelper->GetDataSetDisplayHelper();

        if (dataSetFilter && dataSetFilter->m_graphEnabled)
        {
            BandwidthUsageAggregator overallDataSetUsage;
            const BandwidthUsageContainer::UsageAggregationMap& dataSetUsage = usageContainer->GetDataTypeUsageAggregation(BandwidthUsage::DataType::DATA_SET);

            const AZStd::vector< BaseDisplayHelper* >& dataSets = dataSetFilter->GetChildren();

            for (BaseDisplayHelper* helper : dataSets)
            {
                KeyedDisplayHelper<size_t>* dataSet = static_cast<KeyedDisplayHelper<size_t>*>(helper);
                const BandwidthUsageContainer::UsageAggregationMap::const_iterator usageIter = dataSetUsage.find(dataSet->GetKey());
                
                if (usageIter != dataSetUsage.end())                    
                {
                    const BandwidthUsage& currentUsage = usageIter->second;
                    overallDataSetUsage.m_bytesSent += currentUsage.m_usageAggregator.m_bytesSent;
                    overallDataSetUsage.m_bytesReceived += currentUsage.m_usageAggregator.m_bytesReceived;
                }
            }
            
            PlotBatchedGraphData(dataSetFilter->m_areaGraphPlotHelper, frameId, overallDataSetUsage);            
        }
        
        RPCDisplayFilter* rpcFilter = detailDisplayHelper->GetRPCDisplayHelper();

        if (rpcFilter && rpcFilter->m_graphEnabled)
        {
            BandwidthUsageAggregator overallRPCUsage;
            const BandwidthUsageContainer::UsageAggregationMap& rpcUsage = usageContainer->GetDataTypeUsageAggregation(BandwidthUsage::DataType::REMOTE_PROCEDURE_CALL);

            const AZStd::vector< BaseDisplayHelper* >& rpcs = rpcFilter->GetChildren();

            for (BaseDisplayHelper* helper : rpcs)
            {
                KeyedDisplayHelper<size_t>* rpc = static_cast<KeyedDisplayHelper<size_t>*>(helper);
                const BandwidthUsageContainer::UsageAggregationMap::const_iterator usageIter = rpcUsage.find(rpc->GetKey());

                if (usageIter != rpcUsage.end())
                {
                    const BandwidthUsage& currentUsage = usageIter->second;

                    overallRPCUsage.m_bytesSent += currentUsage.m_usageAggregator.m_bytesSent;
                    overallRPCUsage.m_bytesReceived += currentUsage.m_usageAggregator.m_bytesReceived;
                }
            }

            PlotBatchedGraphData(rpcFilter->m_areaGraphPlotHelper, frameId, overallRPCUsage);
        }
    }

    template<typename Key>
    void BaseDetailView<Key>::DrawHighDetailActiveGraph(BaseDetailDisplayHelper* detailDisplayHelper, const BandwidthUsageContainer* usageContainer, FrameNumberType frameId)
    {
        DataSetDisplayFilter* dataSetFilter = detailDisplayHelper->GetDataSetDisplayHelper();

        if (dataSetFilter)
        {
            const BandwidthUsageContainer::UsageAggregationMap& dataSetUsage = usageContainer->GetDataTypeUsageAggregation(BandwidthUsage::DataType::DATA_SET);
            const AZStd::vector< BaseDisplayHelper* >& dataSets = dataSetFilter->GetChildren();            

            for (BaseDisplayHelper* helper : dataSets)
            {
                if (helper->m_graphEnabled)
                {
                    KeyedDisplayHelper<size_t>* dataSet = static_cast<KeyedDisplayHelper<size_t>*>(helper);
                    const BandwidthUsageContainer::UsageAggregationMap::const_iterator usageIter = dataSetUsage.find(dataSet->GetKey());

                    if (usageIter == dataSetUsage.end())
                    {
                        const BandwidthUsageAggregator emptyUsage;
                        PlotBatchedGraphData(dataSet->m_areaGraphPlotHelper, frameId, emptyUsage);                        
                    }
                    else
                    {
                        const BandwidthUsage& currentUsage = usageIter->second;
                        PlotBatchedGraphData(dataSet->m_areaGraphPlotHelper, frameId, currentUsage.m_usageAggregator);
                    }
                }
            }            
        }

        RPCDisplayFilter* rpcFilter = detailDisplayHelper->GetRPCDisplayHelper();

        if (rpcFilter)
        {
            const BandwidthUsageContainer::UsageAggregationMap& rpcUsage = usageContainer->GetDataTypeUsageAggregation(BandwidthUsage::DataType::REMOTE_PROCEDURE_CALL);

            const AZStd::vector< BaseDisplayHelper* >& rpcs = rpcFilter->GetChildren();

            for (BaseDisplayHelper* helper : rpcs)
            {
                if (helper->m_graphEnabled)
                {
                    KeyedDisplayHelper<size_t>* rpc = static_cast<KeyedDisplayHelper<size_t>*>(helper);
                    const BandwidthUsageContainer::UsageAggregationMap::const_iterator usageIter = rpcUsage.find(rpc->GetKey());

                    if (usageIter == rpcUsage.end())
                    {
                        const BandwidthUsageAggregator emptyUsage;
                        PlotBatchedGraphData(rpc->m_areaGraphPlotHelper, frameId, emptyUsage);                        
                    }
                    else
                    {
                        const BandwidthUsage& currentUsage = usageIter->second;
                        PlotBatchedGraphData(rpc->m_areaGraphPlotHelper, frameId, currentUsage.m_usageAggregator);
                    }
                }
            }            
        }
    }

    // GRAPHCHANGE
    template<typename Key>
    void BaseDetailView<Key>::DrawAggregateGraph()
    {
        BandwidthUsageContainer emptyContainer;

        ConfigureGraphAxis();

        BaseDetailDisplayHelper* aggregateDisplayHelper = FindAggregateDisplay();

        ConfigureBaseDetailDisplayHelper(aggregateDisplayHelper);

        const typename ReplicaBandwidthChartData<Key>::FrameMap& frameMap = GetFrameData();            

        for (FrameNumberType frameId = m_replicaDataView->GetStartFrame(); frameId <= m_replicaDataView->GetEndFrame(); ++frameId)
        {            
            typename ReplicaBandwidthChartData<Key>::FrameMap::const_iterator frameIter = frameMap.find(frameId);
            const typename ReplicaBandwidthChartData<Key>::BandwidthUsageMap* usageMap = nullptr;
            
            if (frameIter != frameMap.end())
            {
                usageMap = frameIter->second;
            }
            else
            {
                static typename ReplicaBandwidthChartData<Key>::BandwidthUsageMap s_emptyMap;
                usageMap = &s_emptyMap;
            }

            AZ_Assert(usageMap != nullptr,"Null pointer added to data map");
            if (usageMap == nullptr)
            {
                continue;
            }

            BandwidthUsageAggregator overallUsageAggregator;            

            BandwidthUsageAggregator overallDataSetUsage;
            AZStd::unordered_map<size_t, BandwidthUsageAggregator> dataSetAggregators;
            DataSetDisplayFilter* dataSetFilter = aggregateDisplayHelper->GetDataSetDisplayHelper();

            BandwidthUsageAggregator overallRPCUsage;
            AZStd::unordered_map<size_t, BandwidthUsageAggregator> rpcAggregators;
            RPCDisplayFilter* rpcFilter = aggregateDisplayHelper->GetRPCDisplayHelper();                

            for (const Key& currentId : m_activeIds)
            {
                const BandwidthUsageContainer* usageContainer = nullptr;
                typename ReplicaBandwidthChartData<Key>::BandwidthUsageMap::const_iterator usageIter = usageMap->find(currentId);

                if (usageIter != usageMap->end())
                {
                    usageContainer = usageIter->second;
                    
                    overallUsageAggregator.m_bytesSent += usageContainer->GetTotalBytesSent();
                    overallUsageAggregator.m_bytesReceived += usageContainer->GetTotalBytesReceived();                    
                }
                else
                {
                    usageContainer = &emptyContainer;
                }

                if (dataSetFilter)
                {
                    const BandwidthUsageContainer::UsageAggregationMap& dataSetUsage = usageContainer->GetDataTypeUsageAggregation(BandwidthUsage::DataType::DATA_SET);
                    
                    const AZStd::vector< BaseDisplayHelper* >& dataSets = dataSetFilter->GetChildren();

                    for (BaseDisplayHelper* helper : dataSets)
                    {
                        KeyedDisplayHelper<size_t>* dataSet = static_cast<KeyedDisplayHelper<size_t>*>(helper);
                        const BandwidthUsageContainer::UsageAggregationMap::const_iterator usageAggIter = dataSetUsage.find(dataSet->GetKey());

                        if (usageAggIter != dataSetUsage.end())
                        {
                            const BandwidthUsage& currentUsage = usageAggIter->second;

                            overallDataSetUsage.m_bytesSent += currentUsage.m_usageAggregator.m_bytesSent;
                            overallDataSetUsage.m_bytesReceived += currentUsage.m_usageAggregator.m_bytesReceived;

                            if (helper->m_graphEnabled)
                            {
                                BandwidthUsageAggregator& dataSetAggregator = dataSetAggregators[dataSet->GetKey()];
                                dataSetAggregator.m_bytesSent += currentUsage.m_usageAggregator.m_bytesSent;
                                dataSetAggregator.m_bytesReceived += currentUsage.m_usageAggregator.m_bytesReceived;
                            }
                        }
                    }
                }

                if (rpcFilter)
                {
                    const BandwidthUsageContainer::UsageAggregationMap& rpcUsage = usageContainer->GetDataTypeUsageAggregation(BandwidthUsage::DataType::REMOTE_PROCEDURE_CALL);

                    const AZStd::vector< BaseDisplayHelper* >& rpcs = rpcFilter->GetChildren();

                    for (BaseDisplayHelper* helper : rpcs)
                    {
                        KeyedDisplayHelper<size_t>* rpc = static_cast<KeyedDisplayHelper<size_t>*>(helper);
                        const BandwidthUsageContainer::UsageAggregationMap::const_iterator usageAggIter = rpcUsage.find(rpc->GetKey());

                        if (usageAggIter != rpcUsage.end())
                        {
                            const BandwidthUsage& currentUsage = usageAggIter->second;

                            overallRPCUsage.m_bytesSent += currentUsage.m_usageAggregator.m_bytesSent;
                            overallRPCUsage.m_bytesReceived += currentUsage.m_usageAggregator.m_bytesReceived;

                            if (helper->m_graphEnabled)
                            {
                                BandwidthUsageAggregator& rpcAggregator = rpcAggregators[rpc->GetKey()];
                                rpcAggregator.m_bytesSent += currentUsage.m_usageAggregator.m_bytesSent;
                                rpcAggregator.m_bytesReceived += currentUsage.m_usageAggregator.m_bytesReceived;
                            }
                        }                            
                    }
                }
            }            
            
            if (m_detailMode == DetailMode::Low)
            {
                if (aggregateDisplayHelper->m_graphEnabled)
                {
                    PlotBatchedGraphData(aggregateDisplayHelper->m_areaGraphPlotHelper, frameId, overallUsageAggregator);
                }
            }
            else if (m_detailMode == DetailMode::Medium)
            {
                if (dataSetFilter && dataSetFilter->m_graphEnabled)
                {
                    PlotBatchedGraphData(dataSetFilter->m_areaGraphPlotHelper, frameId, overallDataSetUsage);
                }

                if (rpcFilter && rpcFilter->m_graphEnabled)
                {
                    PlotBatchedGraphData(rpcFilter->m_areaGraphPlotHelper, frameId, overallRPCUsage);
                }
            }
            else if (m_detailMode == DetailMode::High)
            {
                if (dataSetFilter)
                {
                    const AZStd::vector< BaseDisplayHelper* >& dataSets = dataSetFilter->GetChildren();
                    for (BaseDisplayHelper* helper : dataSets)
                    {
                        if (!helper->m_graphEnabled)
                        {
                            continue;
                        }

                        KeyedDisplayHelper<size_t>* dataSet = static_cast<KeyedDisplayHelper<size_t>*>(helper);
                        BandwidthUsageAggregator& dataSetAggregator = dataSetAggregators[dataSet->GetKey()];                    

                        PlotBatchedGraphData(helper->m_areaGraphPlotHelper, frameId, dataSetAggregator);
                    }
                }

                if (rpcFilter)
                {
                    const AZStd::vector< BaseDisplayHelper* >& rpcs = rpcFilter->GetChildren();
                    for (BaseDisplayHelper* helper : rpcs)
                    {
                        if (!helper->m_graphEnabled)
                        {
                            continue;
                        }

                        KeyedDisplayHelper<size_t>* rpc = static_cast<KeyedDisplayHelper<size_t>*>(helper);
                        BandwidthUsageAggregator& rpcAggregator = rpcAggregators[rpc->GetKey()];

                        PlotBatchedGraphData(helper->m_areaGraphPlotHelper, frameId, rpcAggregator);
                    }
                }
            }
        }
    }
    
    template<typename Key>
    void BaseDetailView<Key>::ConfigureBaseDetailDisplayHelper(BaseDetailDisplayHelper* detailDisplayHelper)
    {
        detailDisplayHelper->ResetGraphConfiguration();

        if (detailDisplayHelper->m_graphEnabled && m_detailMode == DetailMode::Low)
        {
            detailDisplayHelper->m_areaGraphPlotHelper.SetupPlotHelper(m_gui->areaChart, detailDisplayHelper->GetDisplayName(), m_replicaDataView->GetActiveFrameCount());
            detailDisplayHelper->m_areaGraphPlotHelper.SetHighlighted(detailDisplayHelper->m_selected);        
        }

        if (m_detailMode == DetailMode::Medium || m_detailMode == DetailMode::High)
        {
            RPCDisplayFilter* rpcFilter = detailDisplayHelper->GetRPCDisplayHelper();
            
            if (rpcFilter)
            {
                if (m_detailMode == DetailMode::Medium)
                {
                    if (rpcFilter->m_graphEnabled)
                    {
                        rpcFilter->m_areaGraphPlotHelper.SetupPlotHelper(m_gui->areaChart, rpcFilter->GetDisplayName(), m_replicaDataView->GetActiveFrameCount());
                        rpcFilter->m_areaGraphPlotHelper.SetHighlighted(rpcFilter->m_selected);
                    }
                }
                else if (m_detailMode == DetailMode::High)
                {                
                    const AZStd::vector< BaseDisplayHelper* >& rpcs = rpcFilter->GetChildren();

                    for (BaseDisplayHelper* helper : rpcs)
                    {
                        if (helper->m_graphEnabled)
                        {
                            helper->m_areaGraphPlotHelper.SetupPlotHelper(m_gui->areaChart, helper->GetDisplayName(), m_replicaDataView->GetActiveFrameCount());
                            helper->m_areaGraphPlotHelper.SetHighlighted(helper->m_selected);
                        }
                    }
                }
            }

            DataSetDisplayFilter* dataSetFilter = detailDisplayHelper->GetDataSetDisplayHelper();

            if (dataSetFilter)
            {
                if (m_detailMode == DetailMode::Medium)
                {
                    if (dataSetFilter->m_graphEnabled)
                    {
                        dataSetFilter->m_areaGraphPlotHelper.SetupPlotHelper(m_gui->areaChart, dataSetFilter->GetDisplayName(), m_replicaDataView->GetActiveFrameCount());
                        dataSetFilter->m_areaGraphPlotHelper.SetHighlighted(dataSetFilter->m_selected);
                    }
                }
                else if (m_detailMode == DetailMode::High)
                {
                    const AZStd::vector< BaseDisplayHelper* >& dataSets = dataSetFilter->GetChildren();

                    for (BaseDisplayHelper* helper : dataSets)
                    {
                        if (helper->m_graphEnabled)
                        {
                            helper->m_areaGraphPlotHelper.SetupPlotHelper(m_gui->areaChart, dataSetFilter->GetDisplayName(), m_replicaDataView->GetActiveFrameCount());
                            helper->m_areaGraphPlotHelper.SetHighlighted(helper->m_selected);
                        }
                    }
                }
            }
        }
    }    
}