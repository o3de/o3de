/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "StandaloneTools_precompiled.h"

#include <AzCore/Math/MathUtils.h>

#include "ReplicaBandwidthChartData.h"

namespace Driller
{
    ////////////////////
    // GraphPlotHelper
    ////////////////////

    GraphPlotHelper::GraphPlotHelper(const QColor& displayColor)
        : m_color(displayColor)
        , m_channelId(StripChart::DataStrip::s_invalidChannelId)
        , m_zeroOutLine(false)
        , m_initializeLine(false)
        , m_lastHorizontalValue(0.0f)
    {
    }

    void GraphPlotHelper::Reset()
    {
        m_initializeLine        = true;
        m_zeroOutLine           = false;
        m_channelId             = StripChart::DataStrip::s_invalidChannelId;
        m_lastHorizontalValue   = 0.0f;
    }

    bool GraphPlotHelper::IsSetup() const
    {
        return m_channelId != StripChart::DataStrip::s_invalidChannelId;
    }

    void GraphPlotHelper::SetupPlotHelper(StripChart::DataStrip* chart, const char* channelName, float startValue)
    {
        if (chart == nullptr)
        {
            return;
        }

        AZ_Assert(m_channelId == StripChart::DataStrip::s_invalidChannelId, "Double registering the GraphPlotHelper");

        m_channelId = chart->AddChannel(channelName);
        chart->SetChannelStyle(m_channelId, StripChart::Channel::STYLE_CONNECTED_LINE);
        chart->SetChannelColor(m_channelId, m_color);

        m_lastHorizontalValue = startValue;
    }

    void GraphPlotHelper::PlotData(StripChart::DataStrip* chart, float tickSize, float horizontalValue, float verticalValue, bool forceDraw)
    {
        if (chart == nullptr)
        {
            return;
        }

        if (!IsSetup())
        {
            SetupPlotHelper(chart, "<unknown>", horizontalValue);
        }

        bool hasData = !AZ::IsClose(verticalValue, 0.0f, 0.001f);

        float stepDifference = horizontalValue - m_lastHorizontalValue;

        if (m_zeroOutLine || hasData || forceDraw || m_initializeLine)
        {
            if (!hasData)
            {
                if (m_zeroOutLine)
                {
                    chart->AddData(m_channelId, 0, m_lastHorizontalValue + tickSize, 0.0f);
                }
                else if (m_initializeLine)
                {
                    chart->AddData(m_channelId, 0, m_lastHorizontalValue, 0.0f);
                }
            }

            if (stepDifference > tickSize + 0.001f)
            {
                chart->AddData(m_channelId, 0, horizontalValue - tickSize, 0.0f);
            }

            m_initializeLine = false;
            m_zeroOutLine = hasData;
            m_lastHorizontalValue = horizontalValue;

            chart->AddData(m_channelId, 0, horizontalValue, verticalValue);
        }
    }

    void GraphPlotHelper::PlotBatchedData(StripChart::DataStrip* chart, float tickSize, float horizontalValue, float verticalValue, bool forceDraw)
    {
        bool hasData = !AZ::IsClose(verticalValue, 0.0f, 0.001f);

        float stepDifference = horizontalValue - m_lastHorizontalValue;

        if (m_zeroOutLine || hasData || forceDraw || m_initializeLine)
        {
            if (!hasData)
            {
                if (m_zeroOutLine)
                {
                    chart->AddBatchedData(m_channelId, 0, m_lastHorizontalValue + tickSize, 0.0f);
                }
                else if (m_initializeLine)
                {
                    chart->AddBatchedData(m_channelId, 0, m_lastHorizontalValue, 0.0f);
                }
            }

            if (stepDifference > tickSize + 0.001f)
            {
                chart->AddBatchedData(m_channelId, 0, horizontalValue - tickSize, 0.0f);
            }

            m_initializeLine = false;
            m_zeroOutLine = hasData;
            m_lastHorizontalValue = horizontalValue;

            chart->AddBatchedData(m_channelId, 0, horizontalValue, verticalValue);
        }
    }

    void GraphPlotHelper::SetHighlight(StripChart::DataStrip* chart, bool highlight)
    {
        if (chart == nullptr || m_channelId == StripChart::DataStrip::s_invalidChannelId)
        {
            return;
        }

        chart->SetChannelHighlight(m_channelId, highlight);
    }

    void GraphPlotHelper::ZeroOutLine(float lastHorizontalValue, float tickSize, StripChart::DataStrip* chart)
    {
        if (m_zeroOutLine && !AZ::IsClose(lastHorizontalValue, m_lastHorizontalValue, 0.001f))
        {
            chart->AddData(m_channelId, 0, m_lastHorizontalValue + tickSize, 0.0f);
        }
    }

    ////////////////////////
    // AreaGraphPlotHelper
    ////////////////////////

    AreaGraphPlotHelper::AreaGraphPlotHelper(const QColor& displayColor)
        : m_color(displayColor)
        , m_areaChart(nullptr)
        , m_seriesId(AreaChart::AreaChart::k_invalidSeriesId)
    {
    }

    bool AreaGraphPlotHelper::IsSetup() const
    {
        return m_areaChart != nullptr && m_seriesId != AreaChart::AreaChart::k_invalidSeriesId;
    }

    void AreaGraphPlotHelper::Reset()
    {
        m_areaChart = nullptr;
        m_seriesId = AreaChart::AreaChart::k_invalidSeriesId;
    }

    void AreaGraphPlotHelper::SetupPlotHelper(AreaChart::AreaChart* chart, const char* channelName, size_t seriesSize)
    {
        AZ_Error("AreaGraphPlotHelper", !IsSetup(), "Plot Helper is already setup.");
        if (IsSetup())
        {
            Reset();
        }

        QString name(channelName);

        m_areaChart = chart;
        m_seriesId = m_areaChart->CreateSeries(name, m_color,seriesSize);
    }

    void AreaGraphPlotHelper::PlotData(int position, unsigned int value)
    {
        if (IsSetup())
        {
            m_areaChart->AddPoint(m_seriesId, position, value);
        }
    }

    void AreaGraphPlotHelper::PlotBatchedData(int position, unsigned int value)
    {
        m_areaChart->AddPoint(m_seriesId, position, value);
    }

    void AreaGraphPlotHelper::SetHighlighted(bool highlighted)
    {
        if (IsSetup())
        {
            m_areaChart->SetSeriesHighlight(m_seriesId, highlighted);
        }
    }

    void AreaGraphPlotHelper::SetEnabled(bool enabled)
    {
        if (IsSetup())
        {
            m_areaChart->SetSeriesEnabled(m_seriesId, enabled);
        }
    }

    bool AreaGraphPlotHelper::IsSeries(size_t seriesId) const
    {
        if (m_seriesId == AreaChart::AreaChart::k_invalidSeriesId)
        {
            return false;
        }
        else
        {
            return m_seriesId == seriesId;
        }
    }
    
    //////////////////////////////
    // BandwidthUsageAggregator
    //////////////////////////////

    BandwidthUsageAggregator::BandwidthUsageAggregator()
        : m_bytesSent(0)
        , m_bytesReceived(0)
    {
    }

    void BandwidthUsageAggregator::Reset()
    {
        m_bytesSent = 0;
        m_bytesReceived = 0;
    }

    ////////////////////////////
    // BandwidthUsageContainer
    ////////////////////////////

    BandwidthUsageContainer::BandwidthUsageContainer()
    {
        m_dataTypeAggregationMap.insert(DataTypeAggreationMap::value_type(BandwidthUsage::DataType::DATA_SET, UsageAggregationMap()));
        m_dataTypeAggregationMap.insert(DataTypeAggreationMap::value_type(BandwidthUsage::DataType::REMOTE_PROCEDURE_CALL, UsageAggregationMap()));
    }

    BandwidthUsageContainer::~BandwidthUsageContainer()
    {
    }

    void BandwidthUsageContainer::ProcessChunkEvent(const ReplicaChunkEvent* chunkEvent)
    {
        BandwidthUsage* bandwidthUsage = nullptr;

        if (azrtti_istypeof<const ReplicaChunkDataSetEvent*>(chunkEvent))
        {
            const ReplicaChunkDataSetEvent* dataSetEvent = static_cast<const ReplicaChunkDataSetEvent*>(chunkEvent);

            size_t index = dataSetEvent->GetIndex();

            UsageAggregationMap& dataSetAggregationMap = m_dataTypeAggregationMap[BandwidthUsage::DataType::DATA_SET];

            UsageAggregationMap::iterator usageIter = dataSetAggregationMap.find(index);

            if (usageIter == dataSetAggregationMap.end())
            {
                BandwidthUsage usage;

                usage.m_dataType = BandwidthUsage::DataType::DATA_SET;
                usage.m_identifier = dataSetEvent->GetDataSetName();
                usage.m_index = dataSetEvent->GetIndex();

                usageIter = dataSetAggregationMap.insert(UsageAggregationMap::value_type(index, usage)).first;
            }

            bandwidthUsage = (&usageIter->second);

            if (azrtti_istypeof<const ReplicaChunkSentDataSetEvent*>(dataSetEvent))
            {
                m_totalUsageAggregator.m_bytesSent += dataSetEvent->GetUsageBytes();
                bandwidthUsage->m_usageAggregator.m_bytesSent += dataSetEvent->GetUsageBytes();
                OnProcessSentDataSet(static_cast<const ReplicaChunkSentDataSetEvent*>(dataSetEvent));
            }
            else if (azrtti_istypeof<const ReplicaChunkReceivedDataSetEvent*>(dataSetEvent))
            {
                m_totalUsageAggregator.m_bytesReceived += dataSetEvent->GetUsageBytes();
                bandwidthUsage->m_usageAggregator.m_bytesReceived += dataSetEvent->GetUsageBytes();
                OnProcessReceivedDataSet(static_cast<const ReplicaChunkReceivedDataSetEvent*>(dataSetEvent));
            }
            else
            {
                AZ_Error("Standalone Tools", false, "Unknown event type in BadnwidthUsageContainer::ProcessChunkEvent.");
            }
        }
        else if (azrtti_istypeof<const ReplicaChunkRPCEvent>(chunkEvent))
        {
            const ReplicaChunkRPCEvent* rpcEvent = static_cast<const ReplicaChunkRPCEvent*>(chunkEvent);

            size_t index = rpcEvent->GetIndex();

            UsageAggregationMap& rpcUsageAggregationMap = m_dataTypeAggregationMap[BandwidthUsage::DataType::REMOTE_PROCEDURE_CALL];

            UsageAggregationMap::iterator usageIter = rpcUsageAggregationMap.find(index);

            if (usageIter == rpcUsageAggregationMap.end())
            {
                BandwidthUsage usage;

                usage.m_dataType = BandwidthUsage::DataType::REMOTE_PROCEDURE_CALL;
                usage.m_identifier = rpcEvent->GetRPCName();
                usage.m_index = rpcEvent->GetIndex();

                usageIter = rpcUsageAggregationMap.insert(UsageAggregationMap::value_type(index, usage)).first;
            }

            bandwidthUsage = (&usageIter->second);

            if (azrtti_istypeof<const ReplicaChunkSentRPCEvent*>(rpcEvent))
            {
                m_totalUsageAggregator.m_bytesSent += rpcEvent->GetUsageBytes();
                bandwidthUsage->m_usageAggregator.m_bytesSent += rpcEvent->GetUsageBytes();
                OnProcessSentRPC(static_cast<const ReplicaChunkSentRPCEvent*>(rpcEvent));
            }
            else if (azrtti_istypeof<const ReplicaChunkReceivedRPCEvent*>(rpcEvent))
            {
                m_totalUsageAggregator.m_bytesReceived += rpcEvent->GetUsageBytes();
                bandwidthUsage->m_usageAggregator.m_bytesReceived += rpcEvent->GetUsageBytes();
                OnProcessReceivedRPC(static_cast<const ReplicaChunkReceivedRPCEvent*>(rpcEvent));
            }
            else
            {
                AZ_Error("Standalone Tools", false, "Unknown event type in BadnwidthUsageContainer::ProcessChunkEvent.");
            }
        }
        else
        {
            AZ_Error("Standalone Tools", false, "Unknown event type in BadnwidthUsageContainer::ProcessChunkEvent.");
        }
    }

    size_t BandwidthUsageContainer::GetTotalBytesSent() const
    {
        return m_totalUsageAggregator.m_bytesSent;
    }

    size_t BandwidthUsageContainer::GetTotalBytesReceived() const
    {
        return m_totalUsageAggregator.m_bytesReceived;
    }

    size_t BandwidthUsageContainer::GetTotalBandwidthUsage() const
    {
        return GetTotalBytesSent() + GetTotalBytesReceived();
    }

    const BandwidthUsageContainer::UsageAggregationMap& BandwidthUsageContainer::GetDataTypeUsageAggregation(BandwidthUsage::DataType dataType) const
    {
        static UsageAggregationMap s_emptyMap;

        DataTypeAggreationMap::const_iterator dataTypeIterator = m_dataTypeAggregationMap.find(dataType);

        AZ_Error("Standalone Tools", dataTypeIterator != m_dataTypeAggregationMap.end(), "Use of Unknown DataType inside of GetDataTypeUsageAggreation");
        if (dataTypeIterator != m_dataTypeAggregationMap.end())
        {
            return dataTypeIterator->second;
        }

        return s_emptyMap;
    }

    void BandwidthUsageContainer::OnProcessSentDataSet(const ReplicaChunkSentDataSetEvent* sentData)
    {
        (void)sentData;
    }

    void BandwidthUsageContainer::OnProcessReceivedDataSet(const ReplicaChunkReceivedDataSetEvent* receivedData)
    {
        (void)receivedData;
    }

    void BandwidthUsageContainer::OnProcessSentRPC(const ReplicaChunkSentRPCEvent* sentData)
    {
        (void)sentData;
    }

    void BandwidthUsageContainer::OnProcessReceivedRPC(const ReplicaChunkReceivedRPCEvent* receivedData)
    {
        (void)receivedData;
    }
}
