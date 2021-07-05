/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef DRILLER_REPLICA_DATAAGGREGATOR_H
#define DRILLER_REPLICA_DATAAGGREGATOR_H

#if !defined(Q_MOC_RUN)
#include "Source/Driller/DrillerAggregator.hxx"
#include "ReplicaDataParser.h"

#include "GridMate/Drillers/ReplicaDriller.h"

#include "Source/Driller/GenericCustomizeCSVExportWidget.hxx"
#include <AzCore/RTTI/RTTI.h>
#endif

namespace Driller
{
    class ReplicaDataView;
    class ReplicaDataAggregatorSavedState;
    class ReplicaExportSettingsSavedState;

    class ReplicaExportSettings
        : public GenericCSVExportSettings
    {
        // Serialization Keys
        static const char* REPLICA_CSV_EXPORT_SETTINGS;

    public:
        enum class ExportField
        {
            Name = 0,
            Id,
            ChunkType,
            UsageType,
            UsageIdentifier,
            Bytes_Sent,
            Bytes_Received,
            UNKNOWN
        };

    private:
        AZStd::unordered_map< ExportField, AZStd::string > m_columnDescriptors;
        AZStd::unordered_map< AZStd::string, ExportField > m_stringToExportEnum;        

        AZStd::intrusive_ptr<ReplicaExportSettingsSavedState> m_persistentState;

    public:        
        AZ_CLASS_ALLOCATOR(ReplicaExportSettings, AZ::SystemAllocator, 0);

        ReplicaExportSettings();

        void LoadSettings();

        void GetExportItems(QStringList& items) const override;
        void GetActiveExportItems(QStringList& items) const override;        

        const AZStd::vector< int >& GetExportOrder() const;
        const AZStd::string& FindColumnDescriptor(ExportField exportField) const;
    protected:
        void UpdateExportOrdering(const QStringList& activeItems) override;

    private:        
        ExportField FindExportFieldFromDescriptor(const char* descriptor) const;
    };

    struct ReplicaDataConfigurationSettings
    {
    public:
        enum ConfigurationDisplayType
        {
            CDT_Start = -1,

            CDT_Frame,
            CDT_Second,
            CDT_Minute,

            CDT_Max
        };

        AZ_TYPE_INFO(ReplicaDataConfigurationSettings, "{7A075B8E-DCAF-47A1-96CF-2CA3A44F38EF}");

        ReplicaDataConfigurationSettings()
            : m_averageFrameBudget(1024.0f * 10.0f)            
            , m_configurationDisplay(CDT_Frame)
            , m_frameRate(60)
        {        
        }

        // Actual data to be used in the display settings
        float m_averageFrameBudget;

        // Information needed purely for the display
        ConfigurationDisplayType m_configurationDisplay;
        unsigned int m_frameRate;
    };

    typedef ReplicaExportSettings::ExportField ReplicaExportField;

    class ReplicaDataAggregator : public Aggregator
    {
        Q_OBJECT

        // Serialization Keys
        static const char* REPLICA_AGGREGATOR_SAVED_STATE;
        static const char* REPLICA_AGGREGATOR_WORKSPACE;        
    public:
        AZ_RTTI(ReplicaDataAggregator, "{764A4084-E579-4811-89D5-3ADA0632358D}");
        AZ_CLASS_ALLOCATOR(ReplicaDataAggregator, AZ::SystemAllocator, 0);

        ReplicaDataAggregator(int identity = 0);
        ~ReplicaDataAggregator();

        static AZ::u32 DrillerId() { return GridMate::Debug::ReplicaDriller::Tags::REPLICA_DRILLER; }

        AZ::u32 GetDrillerId() const override
        {
            return ReplicaDataAggregator::DrillerId();
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

        bool HasConfigurations() const;
        ChannelConfigurationWidget* CreateConfigurationWidget() override;
        void OnConfigurationChanged() override;

        void AnnotateChannelView(ChannelDataView* channelDataView) override;
        void RemoveChannelAnnotation(ChannelDataView* channelDataView) override;

        // Driller::Aggregator.
        void ApplySettingsFromWorkspace(WorkspaceSettingsProvider*) override;
        void ActivateWorkspaceSettings(WorkspaceSettingsProvider*) override;
        void SaveSettingsToWorkspace(WorkspaceSettingsProvider*) override;

        static void Reflect(AZ::ReflectContext* context);

        // ReplicaDataAggregator
        unsigned int GetAverageFrameBandwidthBudget() const;

    public slots:
        // Driller::Aggregator        
        float ValueAtFrame(FrameNumberType frame) override;
        QColor GetColor() const override;        
        QString GetName() const override;
        QString GetChannelName() const override;
        QString GetDescription() const override;
        QString GetToolTip() const override;
        AZ::Uuid GetID() const override;
        QWidget* DrillDownRequest(FrameNumberType frame) override;
        void OptionsRequest() override;
        void OnDataViewDestroyed(QObject* object);

        void ProcessDrillerEvent(DrillerEvent* drillerEvent);

    protected:
        void ExportColumnDescriptorToCSV(AZ::IO::SystemFile& file,CSVExportSettings* exportSettings) override;
        void ExportEventToCSV(AZ::IO::SystemFile& file, const DrillerEvent* drillerEvent,CSVExportSettings* exportSettings) override;

    private:
        ReplicaDataAggregator(const ReplicaDataAggregator&) = delete;
        void RegisterReplicaDataView(ReplicaDataView* replicaDataView);        

        ReplicaExportSettings m_csvExportSettings;
        ReplicaDataParser m_parser;

        unsigned int m_budgetMarkerTicket;

        size_t    m_processingFrame;

        float   m_currentFrameUsage;
        float   m_maxFrameUsage;

        float    m_normalizingValue;

        AZStd::vector< ReplicaDataView* > m_openDataViews;

        AZStd::intrusive_ptr<ReplicaDataAggregatorSavedState> m_persistentState;
    };
}

#endif
