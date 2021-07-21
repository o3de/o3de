/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "StandaloneTools_precompiled.h"

#include <AzCore/Debug/Trace.h>
#include <AzCore/Math/MathUtils.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/string/conversions.h>
#include <AzCore/UserSettings/UserSettings.h>

#include "ReplicaDataAggregator.hxx"
#include <Source/Driller/Replica/moc_ReplicaDataAggregator.cpp>

#include "ReplicaDataEvents.h"
#include "ReplicaDataView.hxx"

#include "Source/Driller/Workspaces/Workspace.h"
#include "Source/Driller/Replica/ReplicaDataAggregatorConfigurationPanel.hxx"
#include "Source/Driller/ChannelDataView.hxx"

namespace Driller
{
    ////////////////////////////////////
    // ReplicaDataAggregatorSavedState
    ////////////////////////////////////

    class ReplicaDataAggregatorSavedState : public AZ::UserSettings
    {
    public:
        AZ_RTTI(ReplicaDataAggregatorSavedState, "{599BCB69-C521-4EFD-9D79-C09790907F81}", AZ::UserSettings);
        AZ_CLASS_ALLOCATOR(ReplicaDataAggregatorSavedState, AZ::SystemAllocator, 0);

        ReplicaDataAggregatorSavedState()
        {
        }

        static void Reflect(AZ::ReflectContext* context)
        {
            AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);

            if (serialize)
            {
                serialize->Class<ReplicaDataConfigurationSettings>()
                    ->Version(1)
                    ->Field("FrameBudget", &ReplicaDataConfigurationSettings::m_averageFrameBudget)
                    ->Field("DisplayType", &ReplicaDataConfigurationSettings::m_configurationDisplay)
                    ->Field("FrameRate", &ReplicaDataConfigurationSettings::m_frameRate)
                ;

                serialize->Class<ReplicaDataAggregatorSavedState>()
                    ->Version(2)
                    ->Field("ConfigurationSettings",&ReplicaDataAggregatorSavedState::m_configurationSettings)
                ;
            }
        }

        ReplicaDataConfigurationSettings m_configurationSettings;
    };

    ///////////////////////////////////
    // ReplicaDataAggregatorWorkspace
    ///////////////////////////////////

    class ReplicaDataAggregatorWorkspace : public AZ::UserSettings
    {
    public:
        AZ_RTTI(ReplicaDataAggregatorWorkspace, "{EF501646-46BB-4C20-83C9-4C6816294448}", AZ::UserSettings);
        AZ_CLASS_ALLOCATOR(ReplicaDataAggregatorWorkspace,AZ::SystemAllocator,0);

        AZStd::vector<int> m_activeViewIndexes;

        ReplicaDataAggregatorWorkspace()
        {
        }

        static void Reflect(AZ::ReflectContext* context)
        {
            AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
            if (serialize)
            {
                serialize->Class<ReplicaDataAggregatorWorkspace>()
                    ->Field("m_activeViewIndexes",&ReplicaDataAggregatorWorkspace::m_activeViewIndexes)
                    ->Version(1);
            }
        }
    };

    ////////////////////////////////////
    // ReplicaExportSettingsSavedState
    ////////////////////////////////////

    class ReplicaExportSettingsSavedState : public AZ::UserSettings
    {
    public:
        AZ_RTTI(ReplicaExportSettingsSavedState,"{5CE5D03E-04A9-4D28-91D4-5587E0643E84}", AZ::UserSettings);
        AZ_CLASS_ALLOCATOR(ReplicaExportSettingsSavedState,AZ::SystemAllocator,0);

        bool                 m_exportColumnDescriptors;
        AZStd::vector< int > m_exportOrdering;

        ReplicaExportSettingsSavedState()
            : m_exportColumnDescriptors(true)
        {
        }

        void Init()
        {
            m_exportOrdering =
            {
                static_cast<int>(ReplicaExportSettings::ExportField::Name),
                static_cast<int>(ReplicaExportSettings::ExportField::Id),
                static_cast<int>(ReplicaExportSettings::ExportField::ChunkType),
                static_cast<int>(ReplicaExportSettings::ExportField::UsageType),
                static_cast<int>(ReplicaExportSettings::ExportField::UsageIdentifier),
                static_cast<int>(ReplicaExportSettings::ExportField::Bytes_Sent),
                static_cast<int>(ReplicaExportSettings::ExportField::Bytes_Received),
            };
        }

        static void Reflect(AZ::ReflectContext* context)
        {
            AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
            if (serialize)
            {
                serialize->Class<ReplicaExportSettingsSavedState>()
                    ->Field("m_exportColumnDescriptors",&ReplicaExportSettingsSavedState::m_exportColumnDescriptors)
                    ->Field("m_exportOrdering",&ReplicaExportSettingsSavedState::m_exportOrdering)
                    ->Version(1);
            }
        }
    };

    //////////////////////////
    // ReplicaExportSettings
    //////////////////////////

    const char* ReplicaExportSettings::REPLICA_CSV_EXPORT_SETTINGS = "REPLICA_CSV_EXPORT_SETTINGS";

    ReplicaExportSettings::ReplicaExportSettings()
    {
        m_columnDescriptors =
        {
            { ExportField::Name, "Replica Name"},
            { ExportField::Id, "Replica Id"},
            { ExportField::ChunkType,"ReplicaChunk Type"},
            { ExportField::UsageType,"Usage Type"},
            { ExportField::UsageIdentifier,"Usage Identifier"},
            { ExportField::Bytes_Sent,"Data Sent(Bytes)"},
            { ExportField::Bytes_Received,"Data Received(Bytes)"},
        };

        for (const AZStd::pair< ExportField, AZStd::string >& item : m_columnDescriptors)
        {
            m_stringToExportEnum[item.second] = item.first;
        }
    }

    void ReplicaExportSettings::LoadSettings()
    {
        m_persistentState = AZ::UserSettings::Find<ReplicaExportSettingsSavedState>(AZ_CRC(REPLICA_CSV_EXPORT_SETTINGS), AZ::UserSettings::CT_GLOBAL);

        if (m_persistentState == nullptr)
        {
            m_persistentState = AZ::UserSettings::CreateFind<ReplicaExportSettingsSavedState>(AZ_CRC(REPLICA_CSV_EXPORT_SETTINGS), AZ::UserSettings::CT_GLOBAL);
            m_persistentState->Init();
        }
    }

    void ReplicaExportSettings::GetExportItems(QStringList& items) const
    {
        for (const AZStd::pair< ReplicaExportField, AZStd::string>& item : m_columnDescriptors)
        {
            items.push_back(QString(item.second.c_str()));
        }
    }

    void ReplicaExportSettings::GetActiveExportItems(QStringList& items) const
    {
        for (unsigned int i=0; i < m_persistentState->m_exportOrdering.size(); ++i)
        {
            ReplicaExportField currentField = static_cast<ReplicaExportField>(m_persistentState->m_exportOrdering[i]);

            if (currentField != ReplicaExportField::UNKNOWN)
            {
                items.push_back(QString(FindColumnDescriptor(currentField).c_str()));
            }
        }
    }

    void ReplicaExportSettings::UpdateExportOrdering(const QStringList& activeItems)
    {
        m_persistentState->m_exportOrdering.clear();

        for (const QString& activeItem : activeItems)
        {
            ExportField field = FindExportFieldFromDescriptor(activeItem.toStdString().c_str());

            AZ_Warning("Standalone Tools",field != ExportField::UNKNOWN,"Unknown descriptor %s",activeItem.toStdString().c_str());
            if (field != ExportField::UNKNOWN)
            {
                m_persistentState->m_exportOrdering.push_back(static_cast<int>(field));
            }
        }

        m_persistentState->m_exportColumnDescriptors = this->ShouldExportColumnDescriptors();
    }

    const AZStd::vector< int >& ReplicaExportSettings::GetExportOrder() const
    {
        return m_persistentState->m_exportOrdering;
    }

    const AZStd::string& ReplicaExportSettings::FindColumnDescriptor(ExportField exportField) const
    {
        static const AZStd::string emptyDescriptor;

        AZStd::unordered_map<ExportField, AZStd::string>::const_iterator descriptorIter = m_columnDescriptors.find(exportField);

        if (descriptorIter == m_columnDescriptors.end())
        {
            AZ_Warning("Standalone Tools",false,"Unknown column descriptor in Carrier CSV Export");
            return emptyDescriptor;
        }
        else
        {
            return descriptorIter->second;
        }
    }

    ReplicaExportField ReplicaExportSettings::FindExportFieldFromDescriptor(const char* columnDescriptor) const
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
    // ReplicaDataAggregator
    //////////////////////////

    const char* ReplicaDataAggregator::REPLICA_AGGREGATOR_SAVED_STATE = "REPLICA_DATA_AGGREGATOR_SAVED_STATE";
    const char* ReplicaDataAggregator::REPLICA_AGGREGATOR_WORKSPACE = "REPLICA_DATA_AGGREGATOR_WORKSPACE";

    ReplicaDataAggregator::ReplicaDataAggregator(int identity)
        : Aggregator(identity)
        , m_parser(this)
        , m_budgetMarkerTicket(0)
        , m_processingFrame(0)
        , m_currentFrameUsage(0.0f)
        , m_maxFrameUsage(0.0f)
        , m_normalizingValue(1.0f)
    {
        // find state and restore it
        m_persistentState = AZ::UserSettings::CreateFind<ReplicaDataAggregatorSavedState>(AZ_CRC(REPLICA_AGGREGATOR_SAVED_STATE), AZ::UserSettings::CT_GLOBAL);
        m_csvExportSettings.LoadSettings();

        OnConfigurationChanged();

        connect(this, SIGNAL(OnEventFinalized(DrillerEvent*)), SLOT(ProcessDrillerEvent(DrillerEvent*)));
    }

    ReplicaDataAggregator::~ReplicaDataAggregator()
    {
        // Clear out our open data views
        while (!m_openDataViews.empty())
        {
            delete m_openDataViews.front();
        }
    }

    CustomizeCSVExportWidget* ReplicaDataAggregator::CreateCSVExportCustomizationWidget()
    {
        return aznew GenericCustomizeCSVExportWidget(m_csvExportSettings);
    }

    bool ReplicaDataAggregator::HasConfigurations() const
    {
        return true;
    }

    ChannelConfigurationWidget* ReplicaDataAggregator::CreateConfigurationWidget()
    {
        return aznew ReplicaDataAggregatorConfigurationPanel(m_persistentState->m_configurationSettings);
    }

    void ReplicaDataAggregator::OnConfigurationChanged()
    {
        // Adding a bit of fluff room into average frame budget in order to give it a bit of extra space above it
        // so the line is really clear.
        float normalizingValue = m_normalizingValue;

        // Only allow the maximum usage to double our specified budget to avoid losing too much fidelity
        // for outlier data.
        float maxUsage = AZ::GetMin(m_maxFrameUsage,m_persistentState->m_configurationSettings.m_averageFrameBudget * 2.0f);

        m_normalizingValue = AZStd::max(maxUsage, m_persistentState->m_configurationSettings.m_averageFrameBudget);
        m_normalizingValue = AZStd::GetMax(1.0f, m_normalizingValue);

        if (!AZ::IsClose(normalizingValue,m_normalizingValue,0.001f))
        {
            emit NormalizedRangeChanged();
        }
    }

    void ReplicaDataAggregator::AnnotateChannelView(ChannelDataView* channelDataView)
    {
        RemoveChannelAnnotation(channelDataView);

        float budgetMarker = (2.0f * (m_persistentState->m_configurationSettings.m_averageFrameBudget / m_normalizingValue) - 1.0f);

        QColor color = GetColor();
        color.setRed(AZStd::min(color.red() + 50, 255));
        color.setGreen(AZStd::min(color.green() + 50, 255));
        color.setBlue(AZStd::min(color.blue() + 50, 255));

        m_budgetMarkerTicket = channelDataView->AddBudgetMarker(budgetMarker, color);
    }

    void ReplicaDataAggregator::RemoveChannelAnnotation(ChannelDataView* channelDataView)
    {
        if (m_budgetMarkerTicket != 0)
        {
            channelDataView->RemoveBudgetMarker(m_budgetMarkerTicket);
            m_budgetMarkerTicket = 0;
        }
    }

    float ReplicaDataAggregator::ValueAtFrame(FrameNumberType frame)
    {
        size_t totalChunkBandwidth = 0;

        const EventListType& eventList = GetEvents();

        // If we have an event, do some fancy color coding.
        AZ::s64 numEvents = NumOfEventsAtFrame(frame);
        AZ::s64 startIndex = GetFirstIndexAtFrame(frame);

        for (AZ::s64 i = 0; i < numEvents; ++i)
        {
            ReplicaChunkEvent* replicaChunkEvent = static_cast<ReplicaChunkEvent*>(eventList[startIndex + i]);

            totalChunkBandwidth += replicaChunkEvent->GetUsageBytes();
        }

        return AZStd::min(1.0f, static_cast<float>(totalChunkBandwidth) / m_normalizingValue)*2.0f - 1.0f;
    }

    void ReplicaDataAggregator::OnDataViewDestroyed(QObject* object)
    {
        for (AZStd::vector<ReplicaDataView*>::iterator dataViewIter = m_openDataViews.begin();
                dataViewIter != m_openDataViews.end();
                ++dataViewIter)
        {
            if ((*dataViewIter) == object)
            {
                m_openDataViews.erase(dataViewIter);
                break;
            }
        }
    }

    void ReplicaDataAggregator::ProcessDrillerEvent(DrillerEvent* drillerEvent)
    {
        size_t currentFrame = GetFrameCount();

        if (currentFrame != m_processingFrame)
        {
            m_processingFrame = currentFrame;
            m_currentFrameUsage = 0;
        }

        ReplicaChunkEvent* replicaChunkEvent = static_cast<ReplicaChunkEvent*>(drillerEvent);
        m_currentFrameUsage += replicaChunkEvent->GetUsageBytes();

        if (m_currentFrameUsage > m_maxFrameUsage)
        {
            m_maxFrameUsage = m_currentFrameUsage;
            OnConfigurationChanged();
        }
    }

    void ReplicaDataAggregator::ApplySettingsFromWorkspace(WorkspaceSettingsProvider* settingsProvider)
    {
        (void)settingsProvider;
    }

    void ReplicaDataAggregator::ActivateWorkspaceSettings(WorkspaceSettingsProvider* settingsProvider)
    {
        ReplicaDataAggregatorWorkspace* workspace = settingsProvider->FindSetting<ReplicaDataAggregatorWorkspace>(AZ_CRC(REPLICA_AGGREGATOR_WORKSPACE));

        if (workspace)
        {
            // Clear out our open data views
            while (!m_openDataViews.empty())
            {
                delete m_openDataViews.front();
            }

            for (int i=0; i < workspace->m_activeViewIndexes.size(); ++i)
            {
                ReplicaDataView* dataView = aznew ReplicaDataView(workspace->m_activeViewIndexes[i],1,this);
                RegisterReplicaDataView(dataView);

                dataView->ApplySettingsFromWorkspace(settingsProvider);
                dataView->ActivateWorkspaceSettings(settingsProvider);
            }
        }
    }

    void ReplicaDataAggregator::SaveSettingsToWorkspace(WorkspaceSettingsProvider* settingsProvider)
    {
        ReplicaDataAggregatorWorkspace* workspace = settingsProvider->CreateSetting<ReplicaDataAggregatorWorkspace>(AZ_CRC(REPLICA_AGGREGATOR_WORKSPACE));

        if (workspace)
        {
            workspace->m_activeViewIndexes.clear();

            for (ReplicaDataView* dataView : m_openDataViews)
            {
                workspace->m_activeViewIndexes.push_back(dataView->GetDataViewIndex());
                dataView->SaveSettingsToWorkspace(settingsProvider);
            }
        }
    }

    unsigned int ReplicaDataAggregator::GetAverageFrameBandwidthBudget() const
    {
        return static_cast<unsigned int>(m_persistentState->m_configurationSettings.m_averageFrameBudget);
    }

    QColor ReplicaDataAggregator::GetColor() const
    {
        return QColor(0, 0, 255);
    }

    QString ReplicaDataAggregator::GetName() const
    {
        return QString("Replica activity");
    }

    QString ReplicaDataAggregator::GetChannelName() const
    {
        return ChannelName();
    }

    QString ReplicaDataAggregator::GetDescription() const
    {
        return QString("GridMate Replica Usage Per Frame");
    }

    QString ReplicaDataAggregator::GetToolTip() const
    {
        return QString("Information about Replica's, DataSet's, and RPC's");
    }

    AZ::Uuid ReplicaDataAggregator::GetID() const
    {
        return AZ::Uuid("{1252CBE9-111B-4CD3-AF10-FFAE9566B2FF}");
    }

    QWidget* ReplicaDataAggregator::DrillDownRequest(FrameNumberType frame)
    {
        unsigned int replicaDataViewIndex = static_cast<unsigned int>(m_openDataViews.size());

        // We only push to the back, so the list will be ordered with the highest index
        // being at the back.
        // Not exactly bulletproof, but simple(and if they want to open 4 billion windows to cause a slight error, more power to them).
        if (!m_openDataViews.empty())
        {
            replicaDataViewIndex = m_openDataViews.back()->GetDataViewIndex() + 1;
        }

        ReplicaDataView* retVal = aznew ReplicaDataView(replicaDataViewIndex, frame, this);
        RegisterReplicaDataView(retVal);

        return retVal;
    }

    void ReplicaDataAggregator::OptionsRequest()
    {
    }

    void ReplicaDataAggregator::ExportColumnDescriptorToCSV(AZ::IO::SystemFile& file, CSVExportSettings* exportSettings)
    {
        ReplicaExportSettings* replicaExportSettings = static_cast<ReplicaExportSettings*>(exportSettings);
        const AZStd::vector< int >& exportOrdering = replicaExportSettings->GetExportOrder();

        bool addComma = false;

        for (int fieldId : exportOrdering)
        {
            ReplicaExportField currentField = static_cast<ReplicaExportField>(fieldId);

            if (addComma)
            {
                file.Write(",",1);
            }

            const AZStd::string& columnDescriptor = replicaExportSettings->FindColumnDescriptor(currentField);
            file.Write(columnDescriptor.c_str(),columnDescriptor.size());
            addComma = true;
        }

        file.Write("\n",1);
    }

    void ReplicaDataAggregator::ExportEventToCSV(AZ::IO::SystemFile& file, const DrillerEvent* drillerEvent, CSVExportSettings* exportSettings)
    {
        AZ_Assert(azrtti_istypeof<ReplicaChunkEvent>(drillerEvent),"Invalid Event");
        const ReplicaChunkEvent* replicaChunkEvent = static_cast<const ReplicaChunkEvent*>(drillerEvent);

        ReplicaExportSettings* replicaExportSettings = static_cast<ReplicaExportSettings*>(exportSettings);

        const AZStd::vector< int >& exportOrdering = replicaExportSettings->GetExportOrder();
        bool addComma = false;

        AZStd::string field;

        for (int fieldId : exportOrdering)
        {
            ReplicaExportField currentField = static_cast<ReplicaExportField>(fieldId);

            if (addComma)
            {
                file.Write(",",1);
            }

            switch (currentField)
            {
                case ReplicaExportField::Name:
                {
                    field = replicaChunkEvent->GetReplicaName();
                    break;
                }
                case ReplicaExportField::Id:
                {
                    AZStd::to_string(field,replicaChunkEvent->GetReplicaId());
                    break;
                }
                case ReplicaExportField::ChunkType:
                {
                    field = replicaChunkEvent->GetChunkTypeName();
                    break;
                }
                case ReplicaExportField::UsageType:
                {
                     switch (replicaChunkEvent->GetEventType())
                     {
                        case Replica::RET_CHUNK_DATASET_SENT:
                        case Replica::RET_CHUNK_DATASET_RECEIVED:
                        {
                            field = "DataSet";
                            break;
                        }
                        case Replica::RET_CHUNK_RPC_SENT:
                        case Replica::RET_CHUNK_RPC_RECEIVED:
                        {
                            field = "RPC";
                            break;
                        }
                        default:
                            AZ_Warning("Standalone Tools",false,"Unknown Event Type for Replica Event");
                            break;
                     }
                     break;
                }
                case ReplicaExportField::UsageIdentifier:
                {
                    if (azrtti_istypeof<ReplicaChunkSentDataSetEvent>(replicaChunkEvent))
                    {
                        const ReplicaChunkSentDataSetEvent* dataSetEvent = static_cast<const ReplicaChunkSentDataSetEvent*>(replicaChunkEvent);
                        field = dataSetEvent->GetDataSetName();
                    }
                    else if (azrtti_istypeof<ReplicaChunkReceivedDataSetEvent>(replicaChunkEvent))
                    {
                        const ReplicaChunkReceivedDataSetEvent* dataSetEvent = static_cast<const ReplicaChunkReceivedDataSetEvent*>(replicaChunkEvent);
                        field = dataSetEvent->GetDataSetName();
                    }
                    else if (azrtti_istypeof<ReplicaChunkSentRPCEvent>(replicaChunkEvent))
                    {
                        const ReplicaChunkSentRPCEvent* rpcEvent = static_cast<const ReplicaChunkSentRPCEvent*>(replicaChunkEvent);
                        field = rpcEvent->GetRPCName();
                    }
                    else if (azrtti_istypeof<ReplicaChunkReceivedRPCEvent>(replicaChunkEvent))
                    {
                        const ReplicaChunkReceivedRPCEvent* rpcEvent = static_cast<const ReplicaChunkReceivedRPCEvent*>(replicaChunkEvent);
                        field = rpcEvent->GetRPCName();
                    }
                    else
                    {
                        AZ_Warning("Standalone Tools",false,"Invalid ReplicaEvent Type Usage");
                    }

                    break;
                }
                case ReplicaExportField::Bytes_Sent:
                {
                    switch (replicaChunkEvent->GetEventType())
                    {
                        case Replica::RET_CHUNK_RPC_SENT:
                        case Replica::RET_CHUNK_DATASET_SENT:
                        {
                            field = AZStd::to_string(static_cast<unsigned long long>(replicaChunkEvent->GetUsageBytes()));
                            break;
                        }
                        case Replica::RET_CHUNK_RPC_RECEIVED:
                        case Replica::RET_CHUNK_DATASET_RECEIVED:
                        {
                            field = "0";
                            break;
                        }
                        default:
                            AZ_Warning("Standalone Tools",false,"Unknown EventType for ReplicaEvent");
                            break;
                    }
                    break;
                }
                case ReplicaExportField::Bytes_Received:
                {
                    switch (replicaChunkEvent->GetEventType())
                    {
                        case Replica::RET_CHUNK_RPC_SENT:
                        case Replica::RET_CHUNK_DATASET_SENT:
                        {
                            field = "0";
                            break;
                        }
                        case Replica::RET_CHUNK_RPC_RECEIVED:
                        case Replica::RET_CHUNK_DATASET_RECEIVED:
                        {
                            field = AZStd::to_string(static_cast<unsigned long long>(replicaChunkEvent->GetUsageBytes()));
                            break;
                        }
                        default:
                            AZ_Warning("Standalone Tools",false,"Unknown EventType for ReplicaEvent");
                            break;
                    }
                    break;
                }
                default:
                    AZ_Warning("Standalone Tools",false,"Unknown Export Field for ReplicaDataAggreagtor");
                    break;
            }

            file.Write(field.c_str(),field.length());
            addComma = true;
        }

        file.Write("\n",1);
    }

    void ReplicaDataAggregator::RegisterReplicaDataView(ReplicaDataView* replicaDataView)
    {
        if (replicaDataView)
        {
            m_openDataViews.push_back(replicaDataView);

            connect(replicaDataView,SIGNAL(destroyed(QObject*)),this,SLOT(OnDataViewDestroyed(QObject*)));
        }
    }

    void ReplicaDataAggregator::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);

        if (serialize)
        {
            ReplicaDataView::Reflect(context);

            ReplicaExportSettingsSavedState::Reflect(context);
            ReplicaDataAggregatorSavedState::Reflect(context);
            ReplicaDataAggregatorWorkspace::Reflect(context);

            serialize->Class<ReplicaDataAggregator>()
                ->Version(1)
                ->SerializeWithNoData();
        }
    }
}
