/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "StandaloneTools_precompiled.h"
#include <AzCore/std/containers/set.h>
#include "CarrierDataEvents.h"
#include "CarrierDataAggregator.hxx"
#include "CarrierDataView.hxx"
#include "CarrierOperationTelemetryEvent.h"

#include "Source/Driller/DrillerOperationTelemetryEvent.h"

#include <Source/Driller/Carrier/moc_CarrierDataView.cpp>
#include <Source/Driller/Carrier/ui_CarrierDataView.h>
#include <QString>

namespace Driller
{
    static AZ::s64 GetLargestDataValue(const CarrierDataView::DataPointList& dataPointList)
    {
        AZ::s64 maxDataValue = 0;
        for (auto dataPoint = dataPointList.begin(); dataPoint != dataPointList.end(); dataPoint++)
        {
            if (dataPoint->second > maxDataValue)
            {
                maxDataValue = static_cast<AZ::s64>(dataPoint->second);
            }
        }
        return maxDataValue;
    }

    static void BuildEventList(const CarrierDataAggregator* aggr,
        FrameNumberType startFrame,
        FrameNumberType endFrame,
        AZStd::vector<EventNumberType>& eventIdxList)
    {
        eventIdxList.clear();
        for (FrameNumberType frame = startFrame; frame <= endFrame; frame++)
        {
            size_t numEvents = aggr->NumOfEventsAtFrame(frame);
            if (numEvents == 0)
            {
                continue;
            }

            EventNumberType firstEventIdx = aggr->GetFirstIndexAtFrame(frame);
            for (EventNumberType eventIdx = firstEventIdx; eventIdx < static_cast<EventNumberType>(firstEventIdx + numEvents); eventIdx++)
            {
                eventIdxList.push_back(eventIdx);
            }
        }
    }

    static void GetEventIds(const CarrierDataAggregator* aggr,
        FrameNumberType startFrame,
        FrameNumberType endFrameIdx,
        AZStd::set<AZStd::string>& ids)
    {
        ids.clear();
        for (FrameNumberType frameId = startFrame; frameId <= endFrameIdx; frameId++)
        {
            size_t numEvents = aggr->NumOfEventsAtFrame(frameId);
            EventNumberType firstEventIndex = aggr->GetFirstIndexAtFrame(frameId);
            for (EventNumberType eventIndex = firstEventIndex; eventIndex < static_cast<EventNumberType>(firstEventIndex + numEvents); eventIndex++)
            {
                CarrierDataEvent* event = static_cast<CarrierDataEvent*>(aggr->GetEvents()[eventIndex]);
                ids.insert(event->mID);
            }
        }
    }

    CarrierDataView::CarrierDataView(FrameNumberType startFrame, FrameNumberType endFrame, const CarrierDataAggregator* aggregator)
        : QDialog()
        , mStartFrame(0)
        , mEndFrame(0)
        , m_lifespanTelemetry("CarrierDataView")
    {
        // Create a window defined in CarrierDataView.ui.
        m_gui = azcreate(Ui::CarrierDataView, ());
        m_gui->setupUi(this);

        setAttribute(Qt::WA_DeleteOnClose, true);
        setWindowFlags(windowFlags() | Qt::WindowMaximizeButtonHint);

        mStartFrame = startFrame;
        mEndFrame = endFrame;
        mAggregator = aggregator;

        // Prepare the dialog.
        show();
        raise();
        activateWindow();
        setFocus();

        setWindowTitle(aggregator->GetDialogTitle());

        // Find all unique ids and add them to the drop down box.
        AZStd::set<AZStd::string> ids;
        GetEventIds(aggregator, startFrame, endFrame, ids);

        for (auto id = ids.begin(); id != ids.end(); id++)
        {
            m_gui->Filter->addItem((*id).c_str());
        }

        QObject::connect(m_gui->Filter, SIGNAL(currentIndexChanged(int)), this, SLOT(OnCurrentFilterChanged()));

        // Update the charts based on the current filter.
        OnCurrentFilterChanged();
    }

    CarrierDataView::~CarrierDataView()
    {
        azdestroy(m_gui);
    }

    void CarrierDataView::OnCurrentFilterChanged()
    {
        AZStd::string currentId = AZStd::string(m_gui->Filter->currentText().toUtf8().constData());
        SetupAllCharts(currentId, mAggregator, mStartFrame, mEndFrame);

        CarrierOperationTelemetryEvent filterChanged;
        filterChanged.SetAttribute("IPFilterChanged", "");
        filterChanged.Log();
    }

    void CarrierDataView::SetupAllCharts(AZStd::string id,
        const CarrierDataAggregator* aggr,
        FrameNumberType startFrame,
        FrameNumberType endFrame)
    {
        AZStd::vector<EventNumberType> eventIdxList;
        BuildEventList(aggr, startFrame, endFrame, eventIdxList);

        DataPointList send;
        DataPointList recv;
        DataPointList effectiveSend;
        DataPointList effectiveRecv;
        DataPointList pktSend;
        DataPointList pktRecv;
        DataPointList rtt;
        DataPointList loss;

        EventNumberType eventIndex = 0;
        for (auto eventItr = eventIdxList.begin(); eventItr != eventIdxList.end(); eventItr++)
        {
            EventNumberType realEventIndex = *eventItr;
            CarrierDataEvent* event = static_cast<CarrierDataEvent*>(aggr->GetEvents()[realEventIndex]);

            if (event->mID.compare(id) != 0)
            {
                continue;
            }

            // Total bytes sent and received.
            send.push_back(DataPoint(eventIndex, static_cast<float>(event->mLastSecond.mDataSend)));
            recv.push_back(DataPoint(eventIndex, static_cast<float>(event->mLastSecond.mDataReceived)));
            // Effective bytes sent and received (
            effectiveSend.push_back(DataPoint(eventIndex, static_cast<float>(event->mEffectiveLastSecond.mDataSend)));
            effectiveRecv.push_back(DataPoint(eventIndex, static_cast<float>(event->mEffectiveLastSecond.mDataReceived)));
            // Total packets send and received.
            pktSend.push_back(DataPoint(eventIndex, static_cast<float>(event->mLastSecond.mPacketSend)));
            pktRecv.push_back(DataPoint(eventIndex, static_cast<float>(event->mLastSecond.mPacketReceived)));
            // RTT
            rtt.push_back(DataPoint(eventIndex, event->mLastSecond.mRTT));
            // Loss
            loss.push_back(DataPoint(eventIndex, event->mLastSecond.mPacketLoss));

            eventIndex++;
        }

        SetupDualBytesChart(m_gui->sendRecvDataStrip, send, recv);
        SetupDualBytesChart(m_gui->effectiveSendRecvDataStrip, effectiveSend, effectiveRecv);
        SetupDualPacketChart(m_gui->packetSendRecvDataStrip, pktSend, pktRecv);
        SetupTimeChart(m_gui->rttDataStrip, rtt);
    }

    void CarrierDataView::SetupDualBytesChart(StripChart::DataStrip* chart, const DataPointList& bytes0, const DataPointList& bytes1)
    {
        chart->Reset();
        AZ::s64 maxSend = GetLargestDataValue(bytes0);
        AZ::s64 maxRecv = GetLargestDataValue(bytes1);
        chart->AddAxis("Seconds", 0.0f, static_cast<float>(bytes0.size() > bytes1.size() ? bytes0.size() : bytes1.size()), false);
        chart->AddAxis("Bytes/second", 0.0f, (maxSend > maxRecv ? maxSend : maxRecv) * 1.2f, false);

        int sendChannel = chart->AddChannel("Bytes0");
        int recvChannel = chart->AddChannel("Bytes1");
        chart->SetChannelColor(sendChannel, QColor(0, 255, 0));
        chart->SetChannelStyle(sendChannel, StripChart::Channel::STYLE_CONNECTED_LINE);
        chart->SetChannelColor(recvChannel, QColor(255, 0, 0));
        chart->SetChannelStyle(recvChannel, StripChart::Channel::STYLE_CONNECTED_LINE);

        for (auto dataPoint = bytes0.begin(); dataPoint != bytes0.end(); dataPoint++)
        {
            chart->AddData(sendChannel, 0, static_cast<float>(dataPoint->first), dataPoint->second);
        }

        for (auto dataPoint = bytes1.begin(); dataPoint != bytes1.end(); dataPoint++)
        {
            chart->AddData(recvChannel, 0, static_cast<float>(dataPoint->first), dataPoint->second);
        }
    }

    void CarrierDataView::SetupDualPacketChart(StripChart::DataStrip* chart, const DataPointList& packets0, const DataPointList& packets1)
    {
        chart->Reset();
        AZ::s64 maxSend = GetLargestDataValue(packets0);
        AZ::s64 maxRecv = GetLargestDataValue(packets1);
        chart->AddAxis("Seconds", 0.0f, static_cast<float>(packets0.size() > packets1.size() ? packets0.size() : packets1.size()), false);
        chart->AddAxis("Packets/second", 0.0f, (maxSend > maxRecv ? maxSend : maxRecv) * 1.2f, false);

        int sendChannel = chart->AddChannel("Bytes0");
        int recvChannel = chart->AddChannel("Bytes1");
        chart->SetChannelColor(sendChannel, QColor(0, 255, 0));
        chart->SetChannelStyle(sendChannel, StripChart::Channel::STYLE_CONNECTED_LINE);
        chart->SetChannelColor(recvChannel, QColor(255, 0, 0));
        chart->SetChannelStyle(recvChannel, StripChart::Channel::STYLE_CONNECTED_LINE);

        for (auto dataPoint = packets0.begin(); dataPoint != packets0.end(); dataPoint++)
        {
            chart->AddData(sendChannel, 0, static_cast<float>(dataPoint->first), dataPoint->second);
        }

        for (auto dataPoint = packets1.begin(); dataPoint != packets1.end(); dataPoint++)
        {
            chart->AddData(recvChannel, 0, static_cast<float>(dataPoint->first), dataPoint->second);
        }
    }

    void CarrierDataView::SetupTimeChart(StripChart::DataStrip* chart, const DataPointList& time)
    {
        chart->Reset();

        AZ::s64 maxRTT = GetLargestDataValue(time);
        chart->AddAxis("Seconds", 0.0f, static_cast<float>(time.size()), false);
        chart->AddAxis("Milliseconds", 0.0f, maxRTT * 1.2f);

        int channel = chart->AddChannel("RTT");
        chart->SetChannelColor(channel, QColor(255, 0, 255));
        chart->SetChannelStyle(channel, StripChart::Channel::STYLE_CONNECTED_LINE);

        for (auto dataPoint = time.begin(); dataPoint != time.end(); dataPoint++)
        {
            chart->AddData(channel, 0, static_cast<float>(dataPoint->first), dataPoint->second);
        }
    }

    void CarrierDataView::SetupPercentageChart(StripChart::DataStrip* chart, const DataPointList& percentage)
    {
        chart->Reset();

        chart->AddAxis("Seconds", 0.0f, static_cast<float>(percentage.size()), false);
        chart->AddAxis("Percentage", 0.0f, 100.0f);

        int channel = chart->AddChannel("Loss");
        chart->SetChannelColor(channel, QColor(255, 255, 255));
        chart->SetChannelStyle(channel, StripChart::Channel::STYLE_CONNECTED_LINE);

        for (auto dataPoint = percentage.begin(); dataPoint != percentage.end(); dataPoint++)
        {
            chart->AddData(channel, 0, static_cast<float>(dataPoint->first), dataPoint->second);
        }
    }
}
