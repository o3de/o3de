/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "StandaloneTools_precompiled.h"

#include <AzCore/Debug/Trace.h>
#include <AzCore/std/string/conversions.h>

#include "CarrierDataAggregator.hxx"
#include <Source/Driller/Carrier/moc_CarrierDataAggregator.cpp>
#include "CarrierDataEvents.h"
#include "CarrierDataView.hxx"

namespace Driller
{
    /////////////////////////////
    // CarrierCSVExportSettings
    /////////////////////////////
    CarrierExportSettings::CarrierExportSettings()
    {
        m_columnDescriptors =
        {
            {ExportField::Data_Sent, "Data Sent(Bytes)"},
            {ExportField::Data_Received, "Data Received(Bytes)"},
            {ExportField::Data_Resent, "Data Resent(Bytes)"},
            {ExportField::Data_Acked, "Data Acked(Bytes)"},
            {ExportField::Packets_Sent, "Packets Sent"},
            {ExportField::Packets_Received, "Packets Received"},
            {ExportField::Packets_Lost, "Packets Lost"},
            {ExportField::Packets_Acked, "Packets Acked"},
            {ExportField::Packet_RTT, "Packet Round Trip Time"},
            {ExportField::Packet_Loss, "Packet Loss(%)"},
            {ExportField::Effective_Data_Sent, "Effective Data Sent(Bytes)"},
            {ExportField::Effective_Data_Received, "Effective Data Received(Bytes)"},
            {ExportField::Effective_Data_Resent, "Effective Data Resent(Bytes)"},
            {ExportField::Effective_Data_Acked, "Effective Data Acked(Bytes)"},
            {ExportField::Effective_Packets_Sent, "Effective Packets Sent"},
            {ExportField::Effective_Packets_Received, "Effective Packets Received"},
            {ExportField::Effective_Packets_Lost, "Effective Packets Lost"},
            {ExportField::Effective_Packets_Acked, "Effective Packets Acked"},
            {ExportField::Effective_Packet_RTT, "Effective Packet Round Trip Time"},
            {ExportField::Effective_Packet_Loss, "Effective Packet Loss(%)"}
        };

        m_exportOrdering =
        {
            ExportField::Data_Sent,
            ExportField::Effective_Data_Sent,
            ExportField::Data_Received,
            ExportField::Effective_Data_Received,
            ExportField::Data_Resent,
            ExportField::Effective_Data_Resent,
            ExportField::Data_Acked,
            ExportField::Effective_Data_Acked,
            ExportField::Packets_Sent,
            ExportField::Effective_Packets_Sent,
            ExportField::Packets_Received,
            ExportField::Effective_Packets_Received,
            ExportField::Packets_Lost,
            ExportField::Effective_Packets_Lost,
            ExportField::Packets_Acked,
            ExportField::Effective_Packets_Acked,
            ExportField::Packet_RTT,
            ExportField::Effective_Packet_RTT,
            ExportField::Packet_Loss,
            ExportField::Effective_Packet_Loss
        };

        for (const AZStd::pair< ExportField, AZStd::string >& item : m_columnDescriptors)
        {
            m_stringToExportEnum[item.second] = item.first;
        }
    }

    void CarrierExportSettings::GetExportItems(QStringList& items) const
    {
        for (const AZStd::pair< CarrierExportField, AZStd::string>& item : m_columnDescriptors)
        {
            items.push_back(QString(item.second.c_str()));
        }
    }

    void CarrierExportSettings::GetActiveExportItems(QStringList& items) const
    {
        for (CarrierExportField currentField : m_exportOrdering)
        {
            if (currentField != CarrierExportField::UNKNOWN)
            {
                items.push_back(QString(FindColumnDescriptor(currentField).c_str()));
            }
        }
    }

    void CarrierExportSettings::UpdateExportOrdering(const QStringList& activeItems)
    {
        m_exportOrdering.clear();

        for (const QString& activeItem : activeItems)
        {
            ExportField field = FindExportFieldFromDescriptor(activeItem.toStdString().c_str());

            AZ_Assert(field != ExportField::UNKNOWN, "Unknown descriptor %s", activeItem.toStdString().c_str());
            if (field != ExportField::UNKNOWN)
            {
                m_exportOrdering.push_back(field);
            }
        }
    }

    const AZStd::vector< CarrierExportField >& CarrierExportSettings::GetExportOrder() const
    {
        return m_exportOrdering;
    }

    const AZStd::string& CarrierExportSettings::FindColumnDescriptor(ExportField exportField) const
    {
        static const AZStd::string emptyDescriptor;

        AZStd::unordered_map<ExportField, AZStd::string>::const_iterator descriptorIter = m_columnDescriptors.find(exportField);

        if (descriptorIter == m_columnDescriptors.end())
        {
            AZ_Assert(false, "Unknown column descriptor in Carrier CSV Export");
            return emptyDescriptor;
        }
        else
        {
            return descriptorIter->second;
        }
    }

    CarrierExportField CarrierExportSettings::FindExportFieldFromDescriptor(const char* columnDescriptor) const
    {
        AZStd::unordered_map<AZStd::string, ExportField>::const_iterator exportIter = m_stringToExportEnum.find(columnDescriptor);

        ExportField retVal = ExportField::UNKNOWN;

        if (exportIter != m_stringToExportEnum.end())
        {
            retVal = exportIter->second;
        }

        return retVal;
    }

    //////////////////////////
    // CarrierDataAggregator
    //////////////////////////
    float CarrierDataAggregator::GetTValueAtFrame(FrameNumberType frame, AZ::s64 maxValue)
    {
        float valueAtFrame = 0.0f;

        size_t numEventsAtFrame = NumOfEventsAtFrame(frame);
        for (EventNumberType i = m_frameToEventIndex[frame]; i < static_cast<EventNumberType>(m_frameToEventIndex[frame] + numEventsAtFrame); i++)
        {
            CarrierDataEvent* event = static_cast<CarrierDataEvent*>(m_events[i]);
            // Consider the aggregation a sum of all data send and received (i.e. total bandwidth used).
            valueAtFrame += event->mLastSecond.mDataSend + event->mLastSecond.mDataReceived;
        }

        if (valueAtFrame >= maxValue)
        {
            return 1.0f;
        }

        if (valueAtFrame == 0.0f)
        {
            return -1.0f;
        }

        float tValue = (valueAtFrame / (maxValue * 2)) - 1.0f;
        return tValue;
    }

    CarrierDataAggregator::CarrierDataAggregator(int identity)
        : Aggregator(identity)
        , m_parser(this)
    {
    }

    CustomizeCSVExportWidget* CarrierDataAggregator::CreateCSVExportCustomizationWidget()
    {
        return aznew GenericCustomizeCSVExportWidget(m_csvExportSettings);
    }

    float CarrierDataAggregator::ValueAtFrame(FrameNumberType frame)
    {
        return GetTValueAtFrame(frame, (1024 * 20));
    }

    QColor CarrierDataAggregator::GetColor() const
    {
        return QColor(255, 0, 0);
    }

    QString CarrierDataAggregator::GetName() const
    {
        return QString("Carrier");
    }

    QString CarrierDataAggregator::GetChannelName() const
    {
        return ChannelName();
    }

    QString CarrierDataAggregator::GetDescription() const
    {
        return QString("GridMate Carrier Data");
    }

    QString CarrierDataAggregator::GetToolTip() const
    {
        return QString("Information about overall bandwidth usage");
    }

    AZ::Uuid CarrierDataAggregator::GetID() const
    {
        return AZ::Uuid("{927B208C-28E8-4BE7-BF4E-629D98F7097F}");
    }

    QWidget* CarrierDataAggregator::DrillDownRequest(FrameNumberType frame)
    {
        (void)frame;

        // Always provide a full view of the driller data.
        FrameNumberType lastFrame = static_cast<FrameNumberType>(m_frameToEventIndex.size() - 1);

        // This pointer is cleaned up by qt when the window is closed.
        return aznew CarrierDataView(0, lastFrame, this);
    }

    void CarrierDataAggregator::OptionsRequest()
    {
    }

    void CarrierDataAggregator::ExportColumnDescriptorToCSV(AZ::IO::SystemFile& file, CSVExportSettings* exportSettings)
    {
        CarrierExportSettings* carrierExportSettings = static_cast<CarrierExportSettings*>(exportSettings);
        const AZStd::vector< CarrierExportField >& exportOrdering = carrierExportSettings->GetExportOrder();

        bool addComma = false;

        for (CarrierExportField currentField : exportOrdering)
        {
            if (addComma)
            {
                file.Write(",", 1);
            }

            const AZStd::string& columnDescriptor = carrierExportSettings->FindColumnDescriptor(currentField);
            file.Write(columnDescriptor.c_str(), columnDescriptor.size());
            addComma = true;
        }

        file.Write("\n", 1);
    }

    void CarrierDataAggregator::ExportEventToCSV(AZ::IO::SystemFile& file, const DrillerEvent* drillerEvent, CSVExportSettings* exportSettings)
    {
        const CarrierDataEvent* carrierEvent = static_cast<const CarrierDataEvent*>(drillerEvent);

        CarrierExportSettings* carrierExportSettings = static_cast<CarrierExportSettings*>(exportSettings);

        const AZStd::vector< CarrierExportField >& exportOrdering = carrierExportSettings->GetExportOrder();
        bool addComma = false;

        AZStd::string number;

        for (CarrierExportField currentField : exportOrdering)
        {
            if (addComma)
            {
                file.Write(",", 1);
            }

            switch (currentField)
            {
            case CarrierExportField::Data_Sent:
            {
                AZStd::to_string(number, carrierEvent->mLastSecond.mDataSend);
                break;
            }
            case CarrierExportField::Data_Received:
            {
                AZStd::to_string(number, carrierEvent->mLastSecond.mDataReceived);
                break;
            }
            case CarrierExportField::Data_Resent:
            {
                AZStd::to_string(number, carrierEvent->mLastSecond.mDataResent);
                break;
            }
            case CarrierExportField::Data_Acked:
            {
                AZStd::to_string(number, carrierEvent->mLastSecond.mDataAcked);
                break;
            }
            case CarrierExportField::Packets_Sent:
            {
                AZStd::to_string(number, carrierEvent->mLastSecond.mPacketSend);
                break;
            }
            case CarrierExportField::Packets_Received:
            {
                AZStd::to_string(number, carrierEvent->mLastSecond.mPacketReceived);
                break;
            }
            case CarrierExportField::Packets_Lost:
            {
                AZStd::to_string(number, carrierEvent->mLastSecond.mPacketLost);
                break;
            }
            case CarrierExportField::Packets_Acked:
            {
                AZStd::to_string(number, carrierEvent->mLastSecond.mPacketAcked);
                break;
            }
            case CarrierExportField::Packet_RTT:
            {
                AZStd::to_string(number, carrierEvent->mLastSecond.mRTT);
                break;
            }
            case CarrierExportField::Packet_Loss:
            {
                AZStd::to_string(number, carrierEvent->mLastSecond.mPacketLoss);
                break;
            }
            case CarrierExportField::Effective_Data_Sent:
            {
                AZStd::to_string(number, carrierEvent->mEffectiveLastSecond.mDataSend);
                break;
            }
            case CarrierExportField::Effective_Data_Received:
            {
                AZStd::to_string(number, carrierEvent->mEffectiveLastSecond.mDataReceived);
                break;
            }
            case CarrierExportField::Effective_Data_Resent:
            {
                AZStd::to_string(number, carrierEvent->mEffectiveLastSecond.mDataResent);
                break;
            }
            case CarrierExportField::Effective_Data_Acked:
            {
                AZStd::to_string(number, carrierEvent->mEffectiveLastSecond.mDataAcked);
                break;
            }
            case CarrierExportField::Effective_Packets_Sent:
            {
                AZStd::to_string(number, carrierEvent->mEffectiveLastSecond.mPacketSend);
                break;
            }
            case CarrierExportField::Effective_Packets_Received:
            {
                AZStd::to_string(number, carrierEvent->mEffectiveLastSecond.mPacketReceived);
                break;
            }
            case CarrierExportField::Effective_Packets_Lost:
            {
                AZStd::to_string(number, carrierEvent->mEffectiveLastSecond.mPacketLost);
                break;
            }
            case CarrierExportField::Effective_Packets_Acked:
            {
                AZStd::to_string(number, carrierEvent->mEffectiveLastSecond.mPacketAcked);
                break;
            }
            case CarrierExportField::Effective_Packet_RTT:
            {
                AZStd::to_string(number, carrierEvent->mEffectiveLastSecond.mRTT);
                break;
            }
            case CarrierExportField::Effective_Packet_Loss:
            {
                AZStd::to_string(number, carrierEvent->mEffectiveLastSecond.mPacketLoss);
                break;
            }
            default:
                AZ_Assert(false, "Unknown CarrierExportField");
                break;
            }

            file.Write(number.c_str(), number.size());
            addComma = true;
        }

        file.Write("\n", 1);
    }
}
