/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef DRILLER_REPLICA_BANDWIDTHCHARTDATA_H
#define DRILLER_REPLICA_BANDWIDTHCHARTDATA_H

#include <AzCore/base.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/containers/list.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/chrono/chrono.h>
#include <AzCore/std/string/string.h>

#include <QColor>
#include <QIcon>

#include "ReplicaDataEvents.h"
#include "Source/Driller/StripChart.hxx"
#include "Source/Driller/AreaChart.hxx"

#include "Source/Driller/DrillerDataTypes.h"

#include <QPainter>

namespace Driller
{
    // Graph Helper
    class GraphPlotHelper
    {
    public:
        GraphPlotHelper(const QColor& displayColor);

        void Reset();
        bool IsSetup() const;
        void SetupPlotHelper(StripChart::DataStrip* chart, const char* channelName, float startValue);
        void PlotData(StripChart::DataStrip* chart, float tickSize, float horizontalValue, float verticalValue, bool forceDraw);

        // Call this if you are going to do safety checks and don't want the GraphPlotHelper to do that.
        // i.e. if your adding a bunch of data in a row, you'll want to safety check once, then add all the data here.
        void PlotBatchedData(StripChart::DataStrip* chart, float tickSize, float horizontalValue, float verticalValue, bool forceDraw);

        void SetHighlight(StripChart::DataStrip* chart, bool highlight);
        void ZeroOutLine(float lastHorizontalValue, float tickSize, StripChart::DataStrip* chart);

    private:
        QColor          m_color;
        int             m_channelId;
        bool            m_zeroOutLine;
        bool            m_initializeLine;
        float           m_lastHorizontalValue;
    };

    class AreaGraphPlotHelper
    {
    public:
        AreaGraphPlotHelper(const QColor& displayColor);

        bool IsSetup() const;
        void Reset();

        void SetupPlotHelper(AreaChart::AreaChart* chart, const char* channelName, size_t seriesSize = 0);

        void PlotData(int position, unsigned int value);
        void PlotBatchedData(int position, unsigned int value);

        void SetHighlighted(bool highlighted);
        void SetEnabled(bool enabled);

        bool IsSeries(size_t seriesId) const;

    private:

        QColor                  m_color;

        AreaChart::AreaChart*   m_areaChart;
        size_t                  m_seriesId;        
    };

    struct BandwidthUsageAggregator
    {
        BandwidthUsageAggregator();

        void Reset();

        quint64 m_bytesSent;
        quint64 m_bytesReceived;
    };

    struct BandwidthUsage
    {
        enum class DataType
        {
            UNKNOWN,
            DATA_SET,
            REMOTE_PROCEDURE_CALL
        };

        DataType  m_dataType         = DataType::UNKNOWN;
        BandwidthUsageAggregator m_usageAggregator;

        size_t m_index;
        AZStd::string m_identifier;
    };

    class BandwidthUsageContainer
    {
    public:
        typedef AZStd::unordered_map<size_t, BandwidthUsage > UsageAggregationMap;
        typedef AZStd::unordered_map<BandwidthUsage::DataType, UsageAggregationMap > DataTypeAggreationMap;

        AZ_CLASS_ALLOCATOR(BandwidthUsageContainer, AZ::SystemAllocator, 0);

        BandwidthUsageContainer();
        virtual ~BandwidthUsageContainer();

        void ProcessChunkEvent(const ReplicaChunkEvent* chunkEvent);

        size_t GetTotalBytesSent() const;
        size_t GetTotalBytesReceived() const;
        size_t GetTotalBandwidthUsage() const;

        const UsageAggregationMap& GetDataTypeUsageAggregation(BandwidthUsage::DataType dataType) const;

    protected:
        virtual void OnProcessSentDataSet(const ReplicaChunkSentDataSetEvent* sentData);
        virtual void OnProcessReceivedDataSet(const ReplicaChunkReceivedDataSetEvent* receivedData);

        virtual void OnProcessSentRPC(const ReplicaChunkSentRPCEvent* sentData);
        virtual void OnProcessReceivedRPC(const ReplicaChunkReceivedRPCEvent* receivedData);

    private:
        DataTypeAggreationMap m_dataTypeAggregationMap;

        BandwidthUsageAggregator m_totalUsageAggregator;
    };

    template<typename T>
    class ReplicaBandwidthChartData
    {
    public:
        typedef AZStd::unordered_map<T, BandwidthUsageContainer*> BandwidthUsageMap;
        typedef AZStd::unordered_map<FrameNumberType, BandwidthUsageMap*> FrameMap;

    public:
        AZ_CLASS_ALLOCATOR(ReplicaBandwidthChartData, AZ::SystemAllocator, 0);

        ReplicaBandwidthChartData(const QColor& color)
            : m_color(color)
            , m_enabled(true)
            , m_selected(false)
            , m_inspected(false)
            , m_areaGraphPlotHelper(color)
        {
            QPixmap pixmap(16, 16);
            QPainter painter(&pixmap);
            painter.setBrush(m_color);
            painter.drawRect(0, 0, 16, 16);

            m_icon.addPixmap(pixmap);
        }

        virtual ~ReplicaBandwidthChartData()
        {
            for (typename FrameMap::iterator frameIter = m_frameMapping.begin();
                 frameIter != m_frameMapping.end();
                 ++frameIter)
            {
                BandwidthUsageMap* bandwidthUsageMap = frameIter->second;

                for (typename BandwidthUsageMap::iterator usageIter = bandwidthUsageMap->begin();
                     usageIter != bandwidthUsageMap->end();
                     ++usageIter)
                {
                    delete usageIter->second;
                }

                azdestroy(bandwidthUsageMap);
            }
        }

        virtual const char* GetAxisName() const = 0;

        const QColor& GetColor() const
        {
            return m_color;
        }

        const QIcon& GetIcon() const
        {
            if (m_enabled)
            {
                return m_icon;
            }
            else
            {
                static bool s_doOnce = true;
                static QIcon s_blackIcon;

                if (s_doOnce)
                {
                    s_doOnce = false;

                    QPixmap pixmap(16, 16);
                    QPainter painter(&pixmap);
                    painter.setBrush(Qt::black);
                    painter.drawRect(0, 0, 16, 16);

                    s_blackIcon.addPixmap(pixmap);
                }

                return s_blackIcon;
            }
        }

        bool HasUsageForFrame(FrameNumberType frame) const
        {
            return m_frameMapping.find(frame) != m_frameMapping.end();
        }

        const BandwidthUsageMap& FindUsageForFrame(FrameNumberType frameId) const
        {
            static const BandwidthUsageMap emptyMap;

            typename FrameMap::const_iterator frameIter = m_frameMapping.find(frameId);

            if (frameIter == m_frameMapping.end())
            {
                return emptyMap;
            }
            else
            {
                return (*frameIter->second);
            }
        }

        const FrameMap& GetUsageForAllFrames() const
        {
            return m_frameMapping;
        }

        size_t GetSentUsageForFrame(FrameNumberType frameId) const
        {
            size_t totalSentUsage = 0;
            const BandwidthUsageMap& frameMap = FindUsageForFrame(frameId);

            for (auto frameIter = frameMap.begin();
                 frameIter != frameMap.end();
                 ++frameIter)
            {
                totalSentUsage += frameIter->second->GetTotalBytesSent();
            }

            return totalSentUsage;
        }

        size_t GetReceivedUsageForFrame(FrameNumberType frameId) const
        {
            size_t totalReceivedUsage = 0;

            const BandwidthUsageMap& frameMap = FindUsageForFrame(frameId);

            for (auto frameIter = frameMap.begin();
                 frameIter != frameMap.end();
                 ++frameIter)
            {
                totalReceivedUsage += frameIter->second->GetTotalBytesReceived();
            }

            return totalReceivedUsage;
        }

        const FrameMap& GetAllFrames() const
        {
            return m_frameMapping;
        }

        AZ::s64 GetActiveFrameCount() const
        {
            return m_frameMapping.size();
        }

        void SetEnabled(bool enabled)
        {
            m_enabled = enabled;
        }

        bool IsEnabled() const
        {
            return m_enabled;
        }

        void SetSelected(bool selected)
        {
            m_selected = selected;
        }

        bool IsSelected() const
        {
            return m_selected;
        }

        void SetInspected(bool inspected)
        {
            m_inspected = inspected;
        }

        bool IsInspected() const
        {
            return m_inspected;
        }

        AreaGraphPlotHelper& GetAreaGraphPlotHelper()
        {
            return m_areaGraphPlotHelper;
        }        

        void ProcessReplicaChunkEvent(FrameNumberType frameId, const ReplicaChunkEvent* chunkEvent)
        {
            BandwidthUsageContainer* container = GetUsageForFrame(frameId, chunkEvent);

            if (container)
            {
                container->ProcessChunkEvent(chunkEvent);
            }
        }

    protected:

        BandwidthUsageMap* GetUsageForFrame(FrameNumberType frameId)
        {
            BandwidthUsageMap* retVal = nullptr;

            typename FrameMap::iterator frameIter = m_frameMapping.find(frameId);

            if (frameIter == m_frameMapping.end())
            {
                retVal = azcreate(BandwidthUsageMap, ());

                if (retVal)
                {
                    m_frameMapping.insert(typename FrameMap::value_type(frameId, retVal));
                }
            }
            else
            {
                retVal = frameIter->second;
            }

            return retVal;
        }

        BandwidthUsageContainer* GetUsageForFrame(FrameNumberType frameId, const ReplicaChunkEvent* chunkEvent)
        {
            BandwidthUsageContainer* chunkContainer = nullptr;
            BandwidthUsageMap* usageMap = GetUsageForFrame(frameId);

            if (usageMap)
            {
                T usageKey = GetKeyFromEvent(chunkEvent);
                typename BandwidthUsageMap::iterator chunkIter = usageMap->find(usageKey);

                if (chunkIter == usageMap->end())
                {
                    chunkContainer = CreateBandwidthUsage(chunkEvent);

                    if (chunkContainer)
                    {
                        usageMap->insert(typename BandwidthUsageMap::value_type(usageKey, chunkContainer));
                    }
                }
                else
                {
                    chunkContainer = static_cast<BandwidthUsageContainer*>(chunkIter->second);
                }
            }

            return chunkContainer;
        }

        virtual BandwidthUsageContainer* CreateBandwidthUsage(const ReplicaChunkEvent* chunkEvent) = 0;
        virtual T GetKeyFromEvent(const ReplicaChunkEvent* chunkEvent) const = 0;

    private:

        QIcon           m_icon;
        QColor          m_color;

        FrameMap        m_frameMapping;
        bool            m_enabled;
        bool            m_selected;
        bool            m_inspected;

        AreaGraphPlotHelper m_areaGraphPlotHelper;
    };
}
#endif
