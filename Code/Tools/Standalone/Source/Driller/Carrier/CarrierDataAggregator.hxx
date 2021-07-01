/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef DRILLER_CARRIER_DATAAGGREGATOR_H
#define DRILLER_CARRIER_DATAAGGREGATOR_H

#if !defined(Q_MOC_RUN)
#include "Source/Driller/DrillerAggregator.hxx"
#include "CarrierDataParser.h"

#include "Source/Driller/GenericCustomizeCSVExportWidget.hxx"
#endif

namespace Driller
{
    struct CarrierData;
}

namespace Driller
{
    class CarrierDataView;    

    class CarrierExportSettings
        : public GenericCSVExportSettings
    {
    public:
        enum class ExportField
        {
            Data_Sent,
            Data_Received,
            Data_Resent,
            Data_Acked,
            Packets_Sent,
            Packets_Received,
            Packets_Lost,
            Packets_Acked,
            Packet_RTT,
            Packet_Loss,
            Effective_Data_Sent,
            Effective_Data_Received,
            Effective_Data_Resent,
            Effective_Data_Acked,
            Effective_Packets_Sent,
            Effective_Packets_Received,
            Effective_Packets_Lost,
            Effective_Packets_Acked,
            Effective_Packet_RTT,
            Effective_Packet_Loss,

            UNKNOWN
        };

    private:
        AZStd::unordered_map<ExportField,AZStd::string> m_columnDescriptors;
        AZStd::unordered_map<AZStd::string, ExportField> m_stringToExportEnum;
        AZStd::vector< ExportField > m_exportOrdering;

    public:
        AZ_CLASS_ALLOCATOR(CarrierExportSettings, AZ::SystemAllocator, 0);        

        CarrierExportSettings();

        void GetExportItems(QStringList& items) const override;
        void GetActiveExportItems(QStringList& items) const override;
        
        const AZStd::vector< ExportField >& GetExportOrder() const;        
        const AZStd::string& FindColumnDescriptor(ExportField exportField) const;

    protected:
        void UpdateExportOrdering(const QStringList& activeItems) override;

    private:        
        ExportField FindExportFieldFromDescriptor(const char* descriptor) const;
    };

    typedef CarrierExportSettings::ExportField CarrierExportField;

    class CarrierDataAggregator : public Aggregator
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(CarrierDataAggregator, AZ::SystemAllocator, 0);

        CarrierDataAggregator(int identity = 0);
        
        static AZ::u32 DrillerId() { return AZ_CRC("CarrierDriller"); }

        // Driller::Aggregator.
        AZ::u32 GetDrillerId() const override 
        {
            return DrillerId(); 
        }

        static const char* ChannelName() { return "GridMate"; }

        AZ::Crc32 GetChannelId() const override
        {
            return AZ::Crc32(ChannelName());
        }


        AZ::Debug::DrillerHandlerParser* GetDrillerDataParser() override 
        { 
            return &m_parser; 
        }

        bool CanExportToCSV() const override
        { 
            return true;
        }

        CustomizeCSVExportWidget* CreateCSVExportCustomizationWidget() override;

        // Driller::Aggregator.
        void ApplySettingsFromWorkspace(WorkspaceSettingsProvider*) override {}
        void ActivateWorkspaceSettings(WorkspaceSettingsProvider*) override {}
        void SaveSettingsToWorkspace(WorkspaceSettingsProvider*) override {}

    public slots:
        // Driller::Aggregator.
        float    ValueAtFrame(FrameNumberType frame) override;
        QColor   GetColor() const override;        
        QString  GetName() const override;
        QString  GetChannelName() const override;
        QString  GetDescription() const override;
        QString  GetToolTip() const override;
        AZ::Uuid GetID() const override;
        QWidget* DrillDownRequest(FrameNumberType frame) override;
        void     OptionsRequest() override;

    protected:
        void ExportColumnDescriptorToCSV(AZ::IO::SystemFile& file, CSVExportSettings* exportSettings) override;
        void ExportEventToCSV(AZ::IO::SystemFile& file, const DrillerEvent* drillerEvent, CSVExportSettings* exportSettings) override;

    private:

        // Aggregate all data events in a particular frame and return a normalized
        // value in the range [-1,1] based on the min/max range [0,maxValue].
        float GetTValueAtFrame(FrameNumberType frame, AZ::s64 maxValue);

        CarrierExportSettings m_csvExportSettings;

        // A parser that will parse carrier driller XML data and add events back
        // into this aggregator.
        CarrierDataParser m_parser;        
    };
}

#endif
