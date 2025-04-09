/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <VegetationProfiler.h>
#include "DebugComponent.h"
#include "AreaSystemComponent.h"
#include "InstanceSystemComponent.h"

#include <Atom/RPI.Public/ViewportContext.h>
#include <Atom/RPI.Public/ViewportContextBus.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Console/IConsole.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Math/MathUtils.h>
#include <AzCore/IO/SystemFile.h>
#include <SurfaceData/SurfaceDataSystemRequestBus.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzFramework/IO/LocalFileIO.h>
#include <Vegetation/Ebuses/AreaDebugBus.h>
#include <Vegetation/Ebuses/SystemConfigurationBus.h>
#include <Vegetation/Ebuses/InstanceSystemRequestBus.h>

#include <AzCore/std/sort.h>

namespace Vegetation
{

void DebugConfig::Reflect(AZ::ReflectContext* context)
{
    AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
    if (serialize)
    {
        serialize->Class<DebugConfig, AZ::ComponentConfig>()
            ->Version(0)
            ->Field("CollectionFrequencyUs", &DebugConfig::m_collectionFrequencyUs)
            ->Field("MinThresholdUs", &DebugConfig::m_minThresholdUs)
            ->Field("MaxThresholdUs", &DebugConfig::m_maxThresholdUs)
            ->Field("UseMaxDatapointDisplayCount", &DebugConfig::m_maxDatapointDisplayCount)
            ->Field("MaxLabelDisplayDistance", &DebugConfig::m_maxLabelDisplayDistance)
            ->Field("ShowVisualization", &DebugConfig::m_showVisualization)
            ->Field("ShowDebugStats", &DebugConfig::m_showDebugStats)
            ->Field("ShowInstanceVisualization", &DebugConfig::m_showInstanceVisualization)
            ->Field("FilterLevel", &DebugConfig::m_filterLevel)
            ->Field("SortType", &DebugConfig::m_sortType)
            ;

        AZ::EditContext* edit = serialize->GetEditContext();
        if (edit)
        {
            edit->Class<DebugConfig>(
                "DebugConfig", "")
                ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                ->DataElement(0, &DebugConfig::m_collectionFrequencyUs, "Collection Frequency", "How often to collect the data points in microseconds")
                ->DataElement(0, &DebugConfig::m_minThresholdUs, "Medium Threshold", "Minimum number of microseconds the sector should reach")
                ->DataElement(0, &DebugConfig::m_maxThresholdUs, "High Threshold", "Maximum number of microseconds the sector should reach")
                ->DataElement(0, &DebugConfig::m_maxDatapointDisplayCount, "Max Data Point Display Count", "Only display the X newest data points")
                ->DataElement(0, &DebugConfig::m_maxLabelDisplayDistance, "Max Label Display Distance", "Only display labels within X meters")
                ->DataElement(0, &DebugConfig::m_showVisualization, "Show Sector Info", "Show the sector info in the 3D viewport")
                ->DataElement(0, &DebugConfig::m_showDebugStats, "Show Debug Stats", "Show debug stats from the vegetation system on screen")
                ->DataElement(0, &DebugConfig::m_showInstanceVisualization, "Show Per Instance Visualization", "Show a colored cube per instance, color is specified by the creating area")
                ->DataElement(AZ::Edit::UIHandlers::ComboBox, &DebugConfig::m_filterLevel, "Dump Filter", "")
                    ->EnumAttribute(DebugRequests::FilterTypeLevel::Danger, "High-Only")
                    ->EnumAttribute(DebugRequests::FilterTypeLevel::Warning, "Medium-Only")
                    ->EnumAttribute(DebugRequests::FilterTypeLevel::Trace, "All")
                ->DataElement(AZ::Edit::UIHandlers::ComboBox, &DebugConfig::m_sortType, "Dump Sort Type", "")
                    ->EnumAttribute(DebugRequests::SortType::BySector, "By Sector")
                    ->EnumAttribute(DebugRequests::SortType::BySectorDetailed, "By Sector Detailed")
                    ->EnumAttribute(DebugRequests::SortType::ByArea, "By Area")
                    ->EnumAttribute(DebugRequests::SortType::ByAreaDetailed, "By Area Detailed")
                ;
        }
    }
}

void DebugComponent::Reflect(AZ::ReflectContext* context)
{
    DebugConfig::Reflect(context);

    AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
    if (serialize)
    {
        serialize->Class<DebugComponent, AZ::Component>()
            ->Version(0)
            ->Field("Configuration", &DebugComponent::m_configuration)
            ;
    }
}

void DebugComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType & services)
{
    services.push_back(AZ_CRC_CE("DebugService"));

}

void DebugComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType & services)
{
    services.push_back(AZ_CRC_CE("DebugService"));
}

void DebugComponent::Activate()
{
    m_lastCollectionTime = {};
    DebugRequestBus::Handler::BusConnect();
    DebugNotificationBus::Handler::BusConnect();
    DebugNotificationBus::AllowFunctionQueuing(true);
    AzFramework::EntityDebugDisplayEventBus::Handler::BusConnect(GetEntityId());
    AzFramework::BoundsRequestBus::Handler::BusConnect(GetEntityId());
    SystemConfigurationRequestBus::Handler::BusConnect();

    VEG_PROFILE_METHOD(DebugSystemDataBus::BroadcastResult(m_debugData, &DebugSystemDataBus::Events::GetDebugData));
}

void DebugComponent::Deactivate()
{
    SystemConfigurationRequestBus::Handler::BusDisconnect();
    AzFramework::BoundsRequestBus::Handler::BusDisconnect();
    AzFramework::EntityDebugDisplayEventBus::Handler::BusDisconnect();
    DebugRequestBus::Handler::BusDisconnect();
    DebugNotificationBus::Handler::BusDisconnect();

    // These 2 calls presume that this debug component is the only one active
    // and that if another was to be activated it would not overlap the lifetime of this one
    DebugNotificationBus::AllowFunctionQueuing(false);
    DebugNotificationBus::ClearQueuedEvents();
}

bool DebugComponent::ReadInConfig(const AZ::ComponentConfig* baseConfig)
{
    if (auto config = azrtti_cast<const DebugConfig*>(baseConfig))
    {
        m_configuration = *config;
        return true;
    }
    return false;
}

bool DebugComponent::WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const
{
    if (auto config = azrtti_cast<DebugConfig*>(outBaseConfig))
    {
        *config = m_configuration;
        return true;
    }
    return false;
}

void DebugComponent::DisplayEntityViewport(const AzFramework::ViewportInfo& viewportInfo, AzFramework::DebugDisplayRequests& debugDisplay)
{
    // time to collect the report?
    if (AZStd::chrono::duration_cast<AZStd::chrono::microseconds>(AZStd::chrono::steady_clock::now() - m_lastCollectionTime).count() > m_configuration.m_collectionFrequencyUs)
    {
        PrepareNextReport();
        m_lastCollectionTime = AZStd::chrono::steady_clock::now();

        if(m_configuration.m_showVisualization)
        {
            CopyReportToSortedList();
        }
    }

    if (m_exportCurrentReport)
    {
        DumpPerformanceReport(m_thePerformanceReport, m_configuration.m_filterLevel, m_configuration.m_sortType);
        m_exportCurrentReport = false;
    }

    if (m_configuration.m_showVisualization)
    {
        DrawSectorTimingData(viewportInfo, debugDisplay);
    }

    if (m_configuration.m_showDebugStats)
    {
        DrawDebugStats(debugDisplay);
    }

    if (m_configuration.m_showInstanceVisualization)
    {
        DrawInstanceDebug(debugDisplay);
    }
}

void DebugComponent::DrawInstanceDebug([[maybe_unused]] AzFramework::DebugDisplayRequests& debugDisplay)
{
#if defined(VEG_PROFILE_ENABLED)
    AZStd::unordered_map<AreaId, AreaDebugDisplayData> areaDebugDisplayDataMap;

    for (const auto& instance : m_activeInstances)
    {
        const DebugInstanceData& instanceData = instance.second;

        AreaDebugDisplayData areaDebugDisplayData;

        auto areaDebugDisplayDataItr = areaDebugDisplayDataMap.find(instanceData.m_areaId);
        if (areaDebugDisplayDataItr == areaDebugDisplayDataMap.end())
        {
            AreaDebugBus::EventResult(areaDebugDisplayData, instanceData.m_areaId, &AreaDebugBus::Events::GetBlendedDebugDisplayData);
            areaDebugDisplayDataMap[instanceData.m_areaId] = areaDebugDisplayData;
        }
        else
        {
            areaDebugDisplayData = areaDebugDisplayDataItr->second;
        }

        if (!areaDebugDisplayData.m_instanceRender)
        {
            continue;
        }

        AZ::Vector3 radius(areaDebugDisplayData.m_instanceSize * 0.5f);
        debugDisplay.SetColor(areaDebugDisplayData.m_instanceColor);
        debugDisplay.DrawSolidBox(instanceData.m_position - radius, instanceData.m_position + radius);
    }
#endif
}

void DebugComponent::DrawSectorTimingData(const AzFramework::ViewportInfo& viewportInfo, AzFramework::DebugDisplayRequests& debugDisplay)
{
    static const AZ::Color s_green = AZ::Color(0.3f, 0.9f, 0.3f, .05f);
    static const AZ::Color s_yellow = AZ::Color(1.0f, 1.0f, 0.0f, .05f);
    static const AZ::Color s_red = AZ::Color(1.0f, 0.0f, 0.0f, .05f);
    static const float boxHeightAboveTerrain = 3.0f;


    AreaSystemConfig areaConfig;
    SystemConfigurationRequestBus::Broadcast(&SystemConfigurationRequestBus::Events::GetSystemConfig, &areaConfig);
    const auto sectorSizeInMeters = areaConfig.m_sectorSizeInMeters;
    const AZ::u32 maxTextDisplayDistance = m_configuration.m_maxLabelDisplayDistance;
    const int maxDisplayCount = m_configuration.m_maxDatapointDisplayCount;

    AZ::Vector3 cameraPos(0.0f);
    if (auto viewportContextRequests = AZ::RPI::ViewportContextRequests::Get(); viewportContextRequests)
    {
        AZ::RPI::ViewportContextPtr viewportContext = viewportContextRequests->GetViewportContextById(viewportInfo.m_viewportId);
        cameraPos = viewportContext->GetCameraTransform().GetTranslation();
    }
    AZ::Vector2 cameraPos2d(cameraPos.GetX(), cameraPos.GetY());

    for (int i = 0; i < maxDisplayCount && i < m_currentSortedTimingList.size(); ++i)
    {
        const auto& sectorTiming = m_currentSortedTimingList[i];

        AZ::Vector3 topCorner{ float(sectorTiming.m_id.first), float(sectorTiming.m_id.second), 0.0f };
        topCorner *= float(sectorSizeInMeters);

        AZ::Vector3 bottomCorner = topCorner +
            AZ::Vector3(float(sectorSizeInMeters), float(sectorSizeInMeters), sectorTiming.m_worldPosition.GetZ() + boxHeightAboveTerrain);

        auto aabb = AZ::Aabb::CreateFromMinMax(topCorner, bottomCorner);
        const AZ::Color* color = &s_yellow;

        if (sectorTiming.m_averageTimeUs >= m_configuration.m_maxThresholdUs)
        {
            color = &s_red;
        }
        else if (sectorTiming.m_averageTimeUs < m_configuration.m_minThresholdUs)
        {
            color = &s_green;
        }

        AZ::Color outlineColor(color->GetR(), color->GetG(), color->GetB(), 1.0f);

        // Box around the entire sector
        debugDisplay.SetColor(*color);
        debugDisplay.DrawSolidBox(aabb.GetMin(), aabb.GetMax());
        debugDisplay.SetColor(outlineColor);
        debugDisplay.DrawWireBox(aabb.GetMin(), aabb.GetMax());

        // Smaller box inside the sector
        const AZ::Vector3 innerBoxRadius(0.5f);
        debugDisplay.SetColor(outlineColor);
        debugDisplay.DrawSolidBox(sectorTiming.m_worldPosition - innerBoxRadius, sectorTiming.m_worldPosition + innerBoxRadius);

        AZ::Vector2 sectorPos2d(sectorTiming.m_worldPosition.GetX(), sectorTiming.m_worldPosition.GetY());
        float distanceToCamera = cameraPos2d.GetDistance(sectorPos2d);

        if (distanceToCamera <= maxTextDisplayDistance)
        {
            AZStd::string displayString = AZStd::string::format("Sector %d, %d\nTime: %dus\nUpdate Count: %d", sectorTiming.m_id.first,
                sectorTiming.m_id.second, static_cast<int>(sectorTiming.m_averageTimeUs), sectorTiming.m_updateCount);

            constexpr bool centerText = true;
            constexpr float fontSize = 0.7f;
            debugDisplay.SetColor(AZ::Color(1.0f));
            debugDisplay.DrawTextLabel(sectorTiming.m_worldPosition, fontSize, displayString.c_str(), centerText);
        }
    }
}

void DebugComponent::CopyReportToSortedList()
{
    m_currentSortedTimingList.clear();
    m_currentSortedTimingList.reserve(m_thePerformanceReport.m_sectorTimingData.size());

    AZStd::transform(m_thePerformanceReport.m_sectorTimingData.begin(), m_thePerformanceReport.m_sectorTimingData.end(), AZStd::back_inserter(m_currentSortedTimingList),
        [](const AZStd::pair<SectorId, SectorTiming>& input) { return input.second; });

    const size_t sortUpTo = AZ::GetMin(size_t(m_configuration.m_maxDatapointDisplayCount), m_currentSortedTimingList.size());

    AZStd::partial_sort(m_currentSortedTimingList.begin(), m_currentSortedTimingList.begin() + sortUpTo, m_currentSortedTimingList.end(), [](const SectorTiming& lhs, const SectorTiming& rhs)
    {
        return lhs.m_lastUpdateTime > rhs.m_lastUpdateTime;
    });
}

void DebugComponent::FillSectorStart(int sectorX, int sectorY, TimePoint timePoint)
{
    m_currentSectorTiming.m_start = timePoint;
    m_currentSectorTiming.m_end = timePoint;
    m_currentSectorTiming.m_id = { sectorX, sectorY };
    m_currentSectorTiming.m_numInstancesCreated = 0;
    m_currentSectorTiming.m_numClaimPointsRemaining = 0;
    m_currentSectorTiming.m_perAreaTracking.clear();
}

void DebugComponent::FillSectorEnd([[maybe_unused]] int sectorX, [[maybe_unused]] int sectorY, TimePoint timePoint, AZ::u32 unusedClaimPointCount)
{
    AZ_Error("vegetation", m_currentSectorTiming.m_id == AZStd::make_pair(sectorX, sectorY), "Attempting to end a sector other than the one started");
    m_currentSectorTiming.m_end = timePoint;
    m_currentSectorTiming.m_numClaimPointsRemaining = unusedClaimPointCount;
    m_sectorData.emplace_back(m_currentSectorTiming);

    // clear the per area tracking so all attempt to increment instance counts fail and we get a visible error
    m_currentSectorTiming.m_perAreaTracking.clear();
}

namespace DebugComponentUtilities
{
    template <typename ValueType>
    union LocalAliasingUnion
    {
        ValueType aliasedValue;
        AZStd::size_t aliasedValueArray[AZ::DivideAndRoundUp(sizeof(ValueType), sizeof(AZStd::size_t))] = {};
    };

    template <typename ValueType>
    void local_hash_combine(AZStd::size_t& result, ValueType value)
    {
        if constexpr (sizeof(ValueType) > sizeof(AZStd::size_t)) // hash combine static_cast's the passed value to AZStd::size_t, losing hashable bits if ValueType is larger than AZStd::size_t.
        {
            LocalAliasingUnion<ValueType> aliasingUnion;
            aliasingUnion.aliasedValue = value;

            for( AZStd::size_t resultValue : aliasingUnion.aliasedValueArray)
            {
                AZStd::hash_combine(result, resultValue);
            }
        }
        else
        {
            AZStd::hash_combine(result, (AZStd::size_t)value);
        }
    }

    void IncrementFilterReason(Vegetation::FilterReasonCount& filterReasonCount, AZStd::string_view filterReason, AZ::u32 increment)
    {
        auto reasonIterator = filterReasonCount.find(filterReason);
        if (reasonIterator != filterReasonCount.end())
        {
            reasonIterator->second += increment;
        }
        else
        {
            filterReasonCount[filterReason] = increment;
        }
   }
}

AZStd::size_t DebugComponent::MakeAreaSectorKey(AZ::EntityId areaId, SectorId sectorId)
{
    AZStd::size_t result = (AZStd::size_t)0;
    DebugComponentUtilities::local_hash_combine(result, (AZ::u64)areaId);
    DebugComponentUtilities::local_hash_combine(result, sectorId.first);
    DebugComponentUtilities::local_hash_combine(result, sectorId.second);
    return result;
}

void DebugComponent::FillAreaStart(AZ::EntityId areaId, TimePoint timePoint)
{
    AZ_Error("vegetation", m_currentSectorTiming.m_start == m_currentSectorTiming.m_end, "Attempting to start an area on a finished sector");
    uint64 key = MakeAreaSectorKey(areaId, m_currentSectorTiming.m_id);
    AreaTracker& currentAreaTiming = m_currentAreasTiming[key];
    currentAreaTiming.m_id = areaId;
    currentAreaTiming.m_start = timePoint;
    currentAreaTiming.m_end = timePoint;
    currentAreaTiming.m_numClaimPointsRemaining = 0;
    currentAreaTiming.m_numInstancesCreated = 0;
    currentAreaTiming.m_sectorId = m_currentSectorTiming.m_id;
    currentAreaTiming.m_filteredByMasks = false;

    SectorAreaData sectorAreaData;
    sectorAreaData.m_start = timePoint;
    sectorAreaData.m_end = timePoint;
    sectorAreaData.m_numInstancesCreated = 0;
    sectorAreaData.m_filteredByMasks = 0;
    m_currentSectorTiming.m_perAreaTracking[areaId] = sectorAreaData;
}

void DebugComponent::MarkAreaRejectedByMask(AZ::EntityId areaId)
{
    AZ_Error("vegetation", m_currentSectorTiming.m_start == m_currentSectorTiming.m_end, "Attempting to mark an area rejected by mask on a finished sector");
    uint64 key = MakeAreaSectorKey(areaId, m_currentSectorTiming.m_id);
    auto iterator = m_currentAreasTiming.find(key);
    if (iterator != m_currentAreasTiming.end())
    {
        AreaTracker& currentAreaTiming = iterator->second;
        currentAreaTiming.m_filteredByMasks = true;
    }

    auto sectorAreaDataIterator = m_currentSectorTiming.m_perAreaTracking.find(areaId);
    if (sectorAreaDataIterator != m_currentSectorTiming.m_perAreaTracking.end())
    {
        SectorAreaData& currentSectorAreaData = sectorAreaDataIterator->second;
        currentSectorAreaData.m_filteredByMasks = true;
    }
}


void DebugComponent::FillAreaEnd(AZ::EntityId areaId, TimePoint timePoint, AZ::u32 unusedClaimPointCount)
{
    AZ_Error("vegetation", m_currentSectorTiming.m_start == m_currentSectorTiming.m_end, "Attempting to end an area on a finished sector");
    uint64 key = MakeAreaSectorKey(areaId, m_currentSectorTiming.m_id);
    auto iterator = m_currentAreasTiming.find(key);
    if (iterator == m_currentAreasTiming.end())
    {
        return;
    }
    AreaTracker& currentAreaTiming = iterator->second;
    currentAreaTiming.m_end = timePoint;
    currentAreaTiming.m_numClaimPointsRemaining = unusedClaimPointCount;
    m_areaData.emplace_back(currentAreaTiming);
    m_currentAreasTiming.erase(iterator);

    auto sectorAreaDataIterator = m_currentSectorTiming.m_perAreaTracking.find(areaId);
    if (sectorAreaDataIterator != m_currentSectorTiming.m_perAreaTracking.end())
    {
        SectorAreaData& currentSectorAreaData = sectorAreaDataIterator->second;
        currentSectorAreaData.m_end = timePoint;
    }
}

void DebugComponent::FilterInstance(AZ::EntityId areaId, AZStd::string_view filterReason)
{
    uint64 key = MakeAreaSectorKey(areaId, m_currentSectorTiming.m_id);
    auto iterator = m_currentAreasTiming.find(key);
    if (iterator != m_currentAreasTiming.end())
    {
        AreaTracker& currentAreaTiming = iterator->second;
        AZ_Error("vegetation", currentAreaTiming.m_start == currentAreaTiming.m_end, "Attempting to increment the instance count on an area after it has been finished");

        DebugComponentUtilities::IncrementFilterReason(currentAreaTiming.m_numInstancesRejectedByFilters, filterReason, 1u);
    }
    else
    {
        AZ_Error("vegetation", false, "Attempted to increment the instance count on an area that has not been started");
    }

    auto sectorAreaIterator = m_currentSectorTiming.m_perAreaTracking.find(areaId);
    if (sectorAreaIterator != m_currentSectorTiming.m_perAreaTracking.end())
    {
        SectorAreaData& sectorAreaData = sectorAreaIterator->second;
        DebugComponentUtilities::IncrementFilterReason(sectorAreaData.m_numInstancesRejectedByFilters, filterReason, 1u);
    }
    else
    {
        AZ_Error("vegetation", false, "Attempted to increment the instance count on an area that has not been started for this sector");
    }
}

void DebugComponent::CreateInstance(InstanceId instanceId, AZ::Vector3 position, AZ::EntityId areaId)
{
    if (instanceId == InvalidInstanceId)
    {
        return;
    }

    DebugInstanceData debugInstanceData;
    debugInstanceData.m_position = position;
    debugInstanceData.m_areaId = areaId;
    m_activeInstances[instanceId] = debugInstanceData;

    uint64 key = MakeAreaSectorKey(areaId, m_currentSectorTiming.m_id);
    auto iterator = m_currentAreasTiming.find(key);
    if( iterator != m_currentAreasTiming.end())
    {
        AreaTracker& currentAreaTiming = iterator->second;
        AZ_Error("vegetation", currentAreaTiming.m_start == currentAreaTiming.m_end, "Attempting to increment the instance count on an area after it has been finished");
        ++currentAreaTiming.m_numInstancesCreated;
    }
    else
    {
        AZ_Error("vegetation", false, "Attempted to increment the instance count on an area that has not been started");
    }

    ++m_currentSectorTiming.m_numInstancesCreated;
    auto sectorAreaIterator = m_currentSectorTiming.m_perAreaTracking.find(areaId);
    if (sectorAreaIterator != m_currentSectorTiming.m_perAreaTracking.end())
    {
        SectorAreaData& sectorAreaData = sectorAreaIterator->second;
        ++sectorAreaData.m_numInstancesCreated;
    }
    else
    {
        AZ_Error("vegetation", false, "Attempted to increment the instance count on an area that has not been started for this sector");
    }
}

void DebugComponent::DeleteInstance(InstanceId instanceId)
{
    m_activeInstances.erase(instanceId);
}

void DebugComponent::DeleteAllInstances()
{
    m_activeInstances.clear();
}
void DebugComponent::ExportCurrentReport()
{
    m_exportCurrentReport = true;
}

void DebugComponent::ToggleVisualization()
{
    m_configuration.m_showVisualization = !m_configuration.m_showVisualization;
    AZ_TracePrintf("Vegetation", "Visualization %s\n", m_configuration.m_showVisualization ? "Enabled" : "Disabled");
}

void DebugComponent::GetPerformanceReport(PerformanceReport& report) const
{
    AZStd::lock_guard<decltype(m_reportMutex)> lock(m_reportMutex);

    // has the performance report changed?
    if (report.m_count != m_thePerformanceReport.m_count)
    {
        report.m_count = m_thePerformanceReport.m_count;
        report.m_activeInstanceCount = m_thePerformanceReport.m_activeInstanceCount;
        report.m_sectorTimingData = m_thePerformanceReport.m_sectorTimingData;
        report.m_areaTimingData = m_thePerformanceReport.m_areaTimingData;
    }
}

namespace DebugUtility
{
    template <typename TTimingMap, typename TData>
    void MergeResults(const TTimingMap& timingsMap, TData& timingData, const Vegetation::TimePoint& now, AZStd::function<void(const typename TData::mapped_type&, typename TData::mapped_type&)> mergeData)
    {
        for (const auto& entry : timingsMap)
        {
            const auto id = entry.first;
            const auto& newTimings = entry.second;

            auto itTimingData = timingData.find(id);
            if (itTimingData != timingData.end())
            {
                auto& timing(itTimingData->second);
                timing.m_lowestTimeUs = AZ::GetMin<decltype(newTimings.m_lowestTimeUs)>(newTimings.m_lowestTimeUs, timing.m_lowestTimeUs);
                timing.m_peakTimeUs = AZ::GetMax<decltype(newTimings.m_peakTimeUs)>(newTimings.m_peakTimeUs, timing.m_peakTimeUs);
                timing.m_totalUpdateTimeUs += newTimings.m_totalUpdateTimeUs;
                timing.m_totalCount += newTimings.m_totalCount;
                timing.m_averageTimeUs = timing.m_totalUpdateTimeUs / timing.m_totalCount;
                timing.m_updateCount++;
                timing.m_lastUpdateTime = now;
                timing.m_numInstancesCreated = newTimings.m_numInstancesCreated;
                mergeData(newTimings, timing);
            }
            else
            {
                timingData[id] = newTimings;
                timingData[id].m_lastUpdateTime = now;
            }
        }
    }

    template <typename TData, typename TMap, typename TDataEntry, typename TMapEntry, typename TKey>
    void FetchTimingData(const TData& data, TMap& map, AZStd::function<TMapEntry(const TKey&)> newEntry, AZStd::function<void(const TDataEntry&, TMapEntry&)> mergeData)
    {
        // fetch timings
        for (const auto& datum : data)
        {
            auto timeSpan = AZStd::chrono::duration_cast<AZStd::chrono::microseconds>(datum.m_end - datum.m_start).count();

            auto itEntry = map.find(datum.m_id);
            if (itEntry != map.end())
            {
                auto& timing = itEntry->second;
                timing.m_lowestTimeUs = AZ::GetMin<decltype(timing.m_lowestTimeUs)>(timeSpan, timing.m_lowestTimeUs);
                timing.m_peakTimeUs = AZ::GetMax<decltype(timing.m_peakTimeUs)>(timeSpan, timing.m_peakTimeUs);
                timing.m_totalUpdateTimeUs += timeSpan;
                timing.m_numInstancesCreated += static_cast<AZ::u32>(datum.m_numInstancesCreated);
                timing.m_numClaimPointsRemaining += static_cast<AZ::u32>(datum.m_numClaimPointsRemaining);

                ++timing.m_totalCount;
                timing.m_averageTimeUs = timing.m_totalUpdateTimeUs / timing.m_totalCount;

                mergeData(datum, timing);
            }
            else
            {
                TMapEntry timing = newEntry(datum.m_id);
                timing.m_id = datum.m_id;
                timing.m_lowestTimeUs = timeSpan;
                timing.m_peakTimeUs = timeSpan;
                timing.m_averageTimeUs = timeSpan;
                timing.m_totalUpdateTimeUs = timeSpan;
                timing.m_numInstancesCreated = static_cast<AZ::u32>(datum.m_numInstancesCreated);
                timing.m_numClaimPointsRemaining = static_cast<AZ::u32>(datum.m_numClaimPointsRemaining);
                timing.m_totalCount = 1;

                mergeData(datum, timing);
                map[datum.m_id] = timing;
            }
        }
    }

    AZ::u32 GetFilterCount(const Vegetation::FilterReasonCount& filterReasonCount, AZStd::string_view filterReason)
    {
        auto iterator = filterReasonCount.find(filterReason);
        if (iterator != filterReasonCount.end())
        {
            return iterator->second;
        }
        return 0u;
    }

    template <typename TSet>
    void DumpSectorPerformanceReportSet(AZ::IO::HandleType logHandle, [[maybe_unused]] DebugRequests::FilterTypeLevel filter, DebugRequests::SortType sort, TSet& theSet)
    {
        char buffer[512];
        int written = 0;
        AZStd::set<AZStd::string_view> usedfilterReasonSet;

        // write csv header line
        switch (sort)
        {
        case Vegetation::DebugRequests::SortType::BySector:
        {
            written = azsnprintf(buffer, AZ_ARRAY_SIZE(buffer), "sector x, sector y, update count, avg update time ms, peak update time ms, lowest update time ms, total update time ms, number of instances created, number of unused claim points, worldPos X, WorldPos Y,\n");
        }
        break;
        case Vegetation::DebugRequests::SortType::BySectorDetailed:
        {
            usedfilterReasonSet.clear();
            for (const auto* s : theSet)
            {
                DebugRequests::SectorTiming* sectorTiming = (DebugRequests::SectorTiming*)s;
                if (!sectorTiming->m_perAreaData.empty())
                {
                    for (const auto& areaSectorData : sectorTiming->m_perAreaData)
                    {
                        const DebugRequests::AreaSectorTiming& areaSectorTiming = areaSectorData.second;
                        for (const auto& filterReasonCount : areaSectorTiming.m_numInstancesRejectedByFilters)
                        {
                            usedfilterReasonSet.insert(filterReasonCount.first);
                        }
                    }
                }
            }
            written = azsnprintf(buffer, AZ_ARRAY_SIZE(buffer), "sector x, sector y, Area name, instance count, total time ms, sector filtered by inclusion mask,");
            AZ::IO::LocalFileIO::GetInstance()->Write(logHandle, buffer, written);
            for (const auto& name : usedfilterReasonSet)
            {
                written = azsnprintf(buffer, AZ_ARRAY_SIZE(buffer), " instances filtered by %s,", name.data());
                AZ::IO::LocalFileIO::GetInstance()->Write(logHandle, buffer, written);
            }
            written = azsnprintf(buffer, AZ_ARRAY_SIZE(buffer), "\n");
        }
        break;
        case Vegetation::DebugRequests::SortType::ByArea:
        {
            written = azsnprintf(buffer, AZ_ARRAY_SIZE(buffer), "Area name, update count, avg update time ms, peak update time ms, lowest update time ms, total update time ms, number of instances created, number of instances rejected, \n");
        }
        break;
        case Vegetation::DebugRequests::SortType::ByAreaDetailed:
        {
            usedfilterReasonSet.clear();
            for (const auto* s : theSet)
            {
                DebugRequests::AreaTiming* areaTiming = (DebugRequests::AreaTiming*)s;
                for (const auto& areaSectorData : areaTiming->m_perSectorData)
                {
                    const DebugRequests::AreaSectorTiming& areaSectorTiming = areaSectorData.second;
                    for (const auto& filterReasonCount : areaSectorTiming.m_numInstancesRejectedByFilters)
                    {
                        usedfilterReasonSet.insert(filterReasonCount.first);
                    }
                }
            }
            written = azsnprintf(buffer, AZ_ARRAY_SIZE(buffer), "Area name, sector x, sector y, instance count, total time ms, sector filtered by inclusion mask,");
            AZ::IO::LocalFileIO::GetInstance()->Write(logHandle, buffer, written);
            for (const auto& name : usedfilterReasonSet)
            {
                written = azsnprintf(buffer, AZ_ARRAY_SIZE(buffer), " instances filtered by %s,", name.data());
                AZ::IO::LocalFileIO::GetInstance()->Write(logHandle, buffer, written);
            }
            written = azsnprintf(buffer, AZ_ARRAY_SIZE(buffer), "\n");
        }
        break;
        break;
        default:
            AZ_Error("Vegetation Debug", false, "Unknown vegetation sort type");
            return;
        }
        AZ::IO::LocalFileIO::GetInstance()->Write(logHandle, buffer, written);

        for (const auto* s : theSet)
        {
            buffer[0] = 0;
            switch (sort)
            {
            case DebugRequests::SortType::BySector:
            {
                DebugRequests::SectorTiming* sectorTiming = (DebugRequests::SectorTiming*)s;
                DebugRequests::SectorId sectorId = sectorTiming->m_id;
                written = azsnprintf(buffer, AZ_ARRAY_SIZE(buffer), "%d, %d, %d, %4.2f, %4.2f, %4.2f, %4.2f, %d, %d, %8.1f, %8.1f,\n",
                    sectorId.first, sectorId.second,
                    s->m_updateCount,
                    s->m_averageTimeUs / 1000.0f,
                    s->m_peakTimeUs / 1000.0f,
                    s->m_lowestTimeUs / 1000.0f,
                    s->m_totalUpdateTimeUs / 1000.0f,
                    s->m_numInstancesCreated,
                    sectorTiming->m_numClaimPointsRemaining,
                    (float)sectorTiming->m_worldPosition.GetX(),
                    (float)sectorTiming->m_worldPosition.GetY());

                AZ::IO::LocalFileIO::GetInstance()->Write(logHandle, buffer, written);
            }
            break;
            case DebugRequests::SortType::BySectorDetailed:
            {
                DebugRequests::SectorTiming* sectorTiming = (DebugRequests::SectorTiming*)s;
                DebugRequests::SectorId sectorId = sectorTiming->m_id;
                if (!sectorTiming->m_perAreaData.empty())
                {
                    for (const auto& areaSectorData : sectorTiming->m_perAreaData)
                    {
                        AZStd::string areaName;
                        DebugRequests::AreaId areaId = areaSectorData.first;
                        const DebugRequests::AreaSectorTiming& areaSectorTiming = areaSectorData.second;
                        AZ::ComponentApplicationBus::BroadcastResult(areaName, &AZ::ComponentApplicationRequests::GetEntityName, areaId);
                        written = azsnprintf(buffer, AZ_ARRAY_SIZE(buffer), "%d, %d, %s, %d, %4.2f, %s,",
                            sectorId.first,
                            sectorId.second,
                            areaName.c_str(),
                            areaSectorTiming.m_numInstances,
                            areaSectorTiming.m_totalTime / 1000.0f,
                            areaSectorTiming.m_filteredByMasks ? "Filtered" : "Unfiltered");
                        AZ::IO::LocalFileIO::GetInstance()->Write(logHandle, buffer, written);
                        for (const auto& filterReason : usedfilterReasonSet)
                        {
                            written = azsnprintf(buffer, AZ_ARRAY_SIZE(buffer), " %d,", GetFilterCount(areaSectorTiming.m_numInstancesRejectedByFilters, filterReason));
                            AZ::IO::LocalFileIO::GetInstance()->Write(logHandle, buffer, written);
                        }
                        written = azsnprintf(buffer, AZ_ARRAY_SIZE(buffer), "\n");
                        AZ::IO::LocalFileIO::GetInstance()->Write(logHandle, buffer, written);
                    }
                }
                else
                {
                    written = azsnprintf(buffer, AZ_ARRAY_SIZE(buffer), "%d, %d, %s, %d, %4.2f, %s,",
                        sectorId.first,
                        sectorId.second,
                        "No Overlapping Areas",
                        0,
                        0.0f,
                        "Unfiltered");
                    AZ::IO::LocalFileIO::GetInstance()->Write(logHandle, buffer, written);
                    const size_t usedfilterReasonSetSize = usedfilterReasonSet.size();
                    for (size_t i = 0; i < usedfilterReasonSetSize; ++i)
                    {
                        written = azsnprintf(buffer, AZ_ARRAY_SIZE(buffer), " 0,");
                        AZ::IO::LocalFileIO::GetInstance()->Write(logHandle, buffer, written);
                    }
                    written = azsnprintf(buffer, AZ_ARRAY_SIZE(buffer), "\n");
                    AZ::IO::LocalFileIO::GetInstance()->Write(logHandle, buffer, written);
                }
            }
            break;
            case DebugRequests::SortType::ByArea:
            {
                DebugRequests::AreaTiming* areaTiming = (DebugRequests::AreaTiming*)s;
                written = azsnprintf(buffer, AZ_ARRAY_SIZE(buffer), "%s, %d, %4.2f, %4.2f, %4.2f, %4.2f, %d, %d, \n",
                    areaTiming->m_areaName.c_str(),
                    s->m_updateCount,
                    s->m_averageTimeUs / 1000.0f,
                    s->m_peakTimeUs / 1000.0f,
                    s->m_lowestTimeUs / 1000.0f,
                    s->m_totalUpdateTimeUs / 1000.0f,
                    s->m_numInstancesCreated,
                    areaTiming->m_numClaimPointsRemaining);

                AZ::IO::LocalFileIO::GetInstance()->Write(logHandle, buffer, written);
            }
            break;
            case DebugRequests::SortType::ByAreaDetailed:
            {
                DebugRequests::AreaTiming* areaTiming = (DebugRequests::AreaTiming*)s;
                for (const auto& areaSectorData : areaTiming->m_perSectorData)
                {
                    DebugRequests::SectorId sectorId = areaSectorData.first;
                    const DebugRequests::AreaSectorTiming& areaSectorTiming = areaSectorData.second;
                    written = azsnprintf(buffer, AZ_ARRAY_SIZE(buffer), "%s, %d, %d, %d, %4.2f, %s,",
                        areaTiming->m_areaName.c_str(),
                        sectorId.first,
                        sectorId.second,
                        areaSectorTiming.m_numInstances,
                        areaSectorTiming.m_totalTime / 1000.0f,
                        areaSectorTiming.m_filteredByMasks ? "Filtered" : "Unfiltered");

                    AZ::IO::LocalFileIO::GetInstance()->Write(logHandle, buffer, written);
                    for (const auto& filterReason : usedfilterReasonSet)
                    {
                        written = azsnprintf(buffer, AZ_ARRAY_SIZE(buffer), " %d,", GetFilterCount(areaSectorTiming.m_numInstancesRejectedByFilters, filterReason));
                        AZ::IO::LocalFileIO::GetInstance()->Write(logHandle, buffer, written);
                    }
                    written = azsnprintf(buffer, AZ_ARRAY_SIZE(buffer), "\n");
                    AZ::IO::LocalFileIO::GetInstance()->Write(logHandle, buffer, written);
                }
            }
            break;
            }
        }
    }
}

void DebugComponent::PrepareNextReport()
{
    // fill out the sector & area data with the latest timings
    DebugNotificationBus::ExecuteQueuedEvents();

    if (m_sectorData.empty() && m_areaData.empty())
    {
        return;
    }

    // process sector data
    AreaSystemConfig config;
    SystemConfigurationRequestBus::Broadcast(&SystemConfigurationRequestBus::Events::GetSystemConfig, &config);
    const float sectorSizeInMeters = static_cast<float>(config.m_sectorSizeInMeters);
    const float sectorHalfSizeInMeters = sectorSizeInMeters / 2.0f;
    AZStd::unordered_map<SectorId, SectorTiming> sectorTimingMap;

    DebugUtility::FetchTimingData<decltype(m_sectorData), decltype(sectorTimingMap), SectorTracker, SectorTiming, SectorId>(m_sectorData, sectorTimingMap, [sectorSizeInMeters, sectorHalfSizeInMeters](const SectorId& sectorId)
    {
        SectorTiming timing = {};
        const AZ::Vector3 pos(  (sectorSizeInMeters * sectorId.first) + sectorHalfSizeInMeters,
                                (sectorSizeInMeters * sectorId.second) + sectorHalfSizeInMeters,
                                0.0f);

        SurfaceData::SurfacePointList points;
        AZ::Interface<SurfaceData::SurfaceDataSystem>::Get()->GetSurfacePoints(pos, SurfaceData::SurfaceTagVector(), points);
        constexpr size_t inPositionIndex = 0;
        timing.m_worldPosition = points.IsEmpty(inPositionIndex) ? pos : points.GetHighestSurfacePoint(inPositionIndex).m_position;
        return timing;
    },
    [](const SectorTracker& sectorTracker, SectorTiming& sectorTiming)
    {
        for (const auto& sectorTracking : sectorTracker.m_perAreaTracking)
        {
            const AreaId& areaId = sectorTracking.first;
            const SectorAreaData& sectorAreaData = sectorTracking.second;
            auto iterator = sectorTiming.m_perAreaData.find(areaId);
            if (iterator != sectorTiming.m_perAreaData.end())
            {
                AreaSectorTiming& areaSectorTiming = iterator->second;
                areaSectorTiming.m_totalTime += AZStd::chrono::duration_cast<AZStd::chrono::microseconds>(sectorAreaData.m_end - sectorAreaData.m_start).count();
                areaSectorTiming.m_numInstances += static_cast<AZ::u32>(sectorAreaData.m_numInstancesCreated);
                for( const auto& reasonValue : sectorAreaData.m_numInstancesRejectedByFilters )
                {
                    DebugComponentUtilities::IncrementFilterReason(areaSectorTiming.m_numInstancesRejectedByFilters, reasonValue.first, reasonValue.second);
                }
                areaSectorTiming.m_filteredByMasks &= sectorAreaData.m_filteredByMasks;
            }
            else
            {
                AreaSectorTiming newAreaSectorTiming;
                newAreaSectorTiming.m_totalTime = AZStd::chrono::duration_cast<AZStd::chrono::microseconds>(sectorAreaData.m_end - sectorAreaData.m_start).count();
                newAreaSectorTiming.m_numInstances = static_cast<AZ::u32>(sectorAreaData.m_numInstancesCreated);
                newAreaSectorTiming.m_numInstancesRejectedByFilters = sectorAreaData.m_numInstancesRejectedByFilters;
                newAreaSectorTiming.m_filteredByMasks = sectorAreaData.m_filteredByMasks;
                sectorTiming.m_perAreaData[areaId] = newAreaSectorTiming;
            }
        }
    });
    m_sectorData.clear();

    // process area logic
    AZStd::unordered_map<AreaId, AreaTiming> areaTimingMap;

    DebugUtility::FetchTimingData<decltype(m_areaData), decltype(areaTimingMap), AreaTracker, AreaTiming, AreaId>(m_areaData, areaTimingMap, [](const AreaId& areaId)
    {
        AreaTiming timing;
        AZ::ComponentApplicationBus::BroadcastResult(timing.m_areaName, &AZ::ComponentApplicationRequests::GetEntityName, areaId);
        return timing;
    },
    [](const AreaTracker& areaTracker, AreaTiming& areaTiming)
    {
        auto iterator = areaTiming.m_perSectorData.find(areaTracker.m_sectorId);
        if (iterator != areaTiming.m_perSectorData.end())
        {
            AreaSectorTiming& areaSectorTiming = iterator->second;

            areaSectorTiming.m_totalTime += AZStd::chrono::duration_cast<AZStd::chrono::microseconds>(areaTracker.m_end - areaTracker.m_start).count();
            areaSectorTiming.m_numInstances += static_cast<AZ::u32>(areaTracker.m_numInstancesCreated);
            for (const auto& filterReasonEntry : areaTracker.m_numInstancesRejectedByFilters)
            {
                DebugComponentUtilities::IncrementFilterReason(areaSectorTiming.m_numInstancesRejectedByFilters, filterReasonEntry.first, filterReasonEntry.second);
            }

            areaSectorTiming.m_filteredByMasks &= areaTracker.m_filteredByMasks;
        }
        else
        {
            AreaSectorTiming newAreaSectorTiming;
            newAreaSectorTiming.m_totalTime = AZStd::chrono::duration_cast<AZStd::chrono::microseconds>(areaTracker.m_end - areaTracker.m_start).count();
            newAreaSectorTiming.m_numInstances = static_cast<AZ::u32>(areaTracker.m_numInstancesCreated);
            newAreaSectorTiming.m_numInstancesRejectedByFilters = areaTracker.m_numInstancesRejectedByFilters;
            newAreaSectorTiming.m_filteredByMasks = areaTracker.m_filteredByMasks;
            areaTiming.m_perSectorData[areaTracker.m_sectorId] = newAreaSectorTiming;
        }
    });
    m_areaData.clear();

    // merge results
    AZ::u32 instanceCount = 0;
    InstanceSystemStatsRequestBus::BroadcastResult(instanceCount, &InstanceSystemStatsRequestBus::Events::GetInstanceCount);

    AZStd::lock_guard<decltype(m_reportMutex)> lock(m_reportMutex);
    m_thePerformanceReport.m_count++;
    m_thePerformanceReport.m_activeInstanceCount = instanceCount;
    m_thePerformanceReport.m_lastUpdateTime = AZStd::chrono::steady_clock::now();
    DebugUtility::MergeResults(sectorTimingMap, m_thePerformanceReport.m_sectorTimingData, m_thePerformanceReport.m_lastUpdateTime, [](const SectorTiming& newTiming, SectorTiming& timing)
    {
        for (const auto& newData : newTiming.m_perAreaData)
        {
            timing.m_perAreaData[newData.first] = newData.second;
        }
    });
    DebugUtility::MergeResults(areaTimingMap, m_thePerformanceReport.m_areaTimingData, m_thePerformanceReport.m_lastUpdateTime, [](const AreaTiming& newTiming, AreaTiming& timing)
    {
        for (const auto& newData : newTiming.m_perSectorData)
        {
            timing.m_perSectorData[newData.first] = newData.second;
        }
    });
}

void DebugComponent::DrawDebugStats(AzFramework::DebugDisplayRequests& debugDisplay)
{
    if (!m_debugData)
    {
        return;
    }

    AZ::u32 instanceCount = 0;
    InstanceSystemStatsRequestBus::BroadcastResult(instanceCount, &InstanceSystemStatsRequestBus::Events::GetInstanceCount);

    AZ::u32 createTaskCount = 0;
    InstanceSystemStatsRequestBus::BroadcastResult(createTaskCount, &InstanceSystemStatsRequestBus::Events::GetCreateTaskCount);

    AZ::u32 destroyTaskCount = 0;
    InstanceSystemStatsRequestBus::BroadcastResult(destroyTaskCount, &InstanceSystemStatsRequestBus::Events::GetDestroyTaskCount);

    debugDisplay.SetColor(AZ::Color(1.0f));
    debugDisplay.Draw2dTextLabel(
        40.0f, 22.0f, 0.7f,
        AZStd::string::format(
            "VegetationSystemStats:\nActive Instances Count: %d\nInstance Register Queue: %d\nInstance Unregister Queue: %d\nThread "
            "Queue Count: %d\nThread Processing Count: %d",
            instanceCount, createTaskCount, destroyTaskCount, m_debugData->m_areaTaskQueueCount.load(AZStd::memory_order_relaxed),
            m_debugData->m_areaTaskActiveCount.load(AZStd::memory_order_relaxed))
            .c_str(),
        false);
}

namespace
{
    static void veg_debugToggleVisualization([[maybe_unused]] const AZ::ConsoleCommandContainer& arguments)
    {
        DebugNotificationBus::Broadcast(&DebugNotificationBus::Events::ToggleVisualization);
    }
    AZ_CONSOLEFREEFUNC(veg_debugToggleVisualization, AZ::ConsoleFunctorFlags::DontReplicate, "Toggles visualization of sector timings");

    static void veg_debugDumpReport([[maybe_unused]] const AZ::ConsoleCommandContainer& arguments)
    {
        DebugNotificationBus::Broadcast(&DebugNotificationBus::Events::ExportCurrentReport);
    }
    AZ_CONSOLEFREEFUNC(veg_debugDumpReport, AZ::ConsoleFunctorFlags::DontReplicate, "Writes out a vegetation sector report");

    static void veg_debugRefreshAllAreas([[maybe_unused]] const AZ::ConsoleCommandContainer& arguments)
    {
        AreaSystemRequestBus::Broadcast(&AreaSystemRequestBus::Events::RefreshAllAreas);
    }
    AZ_CONSOLEFREEFUNC(
        veg_debugRefreshAllAreas, AZ::ConsoleFunctorFlags::DontReplicate, "Refresh all vegetation areas in the current view");

    static void veg_debugClearAllAreas([[maybe_unused]] const AZ::ConsoleCommandContainer& arguments)
    {
        AreaSystemRequestBus::Broadcast(&AreaSystemRequestBus::Events::ClearAllAreas);
        AreaSystemRequestBus::Broadcast(&AreaSystemRequestBus::Events::RefreshAllAreas);
    }
    AZ_CONSOLEFREEFUNC(
        veg_debugClearAllAreas, AZ::ConsoleFunctorFlags::DontReplicate, "Clear and refresh all vegetation areas in the current view");


    const char* GetSortTypeString(DebugRequests::SortType sortType)
    {
        switch (sortType)
        {
        case DebugRequests::SortType::BySector: return "BySector";
        case DebugRequests::SortType::BySectorDetailed: return "BySectorDetailed";
        case DebugRequests::SortType::ByArea: return "ByArea";
        case DebugRequests::SortType::ByAreaDetailed: return "ByAreaDetailed";
        // no default so compiler warns about missing entries
        }
        AZ_Error("Vegetation Debug", false, "Unknown data sort format supplied");
        return "Unknown";
    }

    const char* GetFilterTypeLevelString(DebugRequests::FilterTypeLevel filterTypeLevel)
    {
        switch (filterTypeLevel)
        {
        case Vegetation::DebugRequests::FilterTypeLevel::Danger:    return "High-Only";
        case Vegetation::DebugRequests::FilterTypeLevel::Trace:     return "All";
        case Vegetation::DebugRequests::FilterTypeLevel::Warning:   return "Medium-Only";
        // no default so compiler warns about missing entries
        }
        AZ_Error("Vegetation Debug", false, "Unknown filter type level supplied");
        return "Unknown";
    }
}

void DebugComponent::DumpPerformanceReport(const PerformanceReport& report, FilterTypeLevel filter, SortType sort) const
{
    AZStd::string logFolder = AZStd::string::format("@log@/vegetation");
    AZ::IO::LocalFileIO::GetInstance()->CreatePath(logFolder.c_str());

    AZ::u64 timeSinceEpoch = AZStd::chrono::steady_clock::now().time_since_epoch().count();
    AZStd::string logFile = AZStd::string::format("%s/%s_%s_%s_%llu.csv", logFolder.c_str(), "vegperf", GetSortTypeString(sort), GetFilterTypeLevelString(filter), timeSinceEpoch);

    AZ::IO::HandleType logHandle;
    AZ::IO::Result result = AZ::IO::LocalFileIO::GetInstance()->Open(logFile.c_str(), AZ::IO::OpenMode::ModeWrite, logHandle);
    if (!result)
    {
        AZ_Warning("vegetation", result, "Did not open for write at %s \n", logFile.c_str());
        return;
    }

    AZStd::function<bool(const DebugConfig& configuration, const BaseTiming& timing)> fnFilterRule;
    if (filter == DebugRequests::FilterTypeLevel::Danger)
    {
        fnFilterRule = [](const DebugConfig& configuration, const BaseTiming& timing) -> bool
        {
            return timing.m_averageTimeUs >= configuration.m_maxThresholdUs;
        };
    }
    else if (filter == DebugRequests::FilterTypeLevel::Warning)
    {
        fnFilterRule = [](const DebugConfig& configuration, const BaseTiming& timing) -> bool
        {
            return timing.m_averageTimeUs >= configuration.m_minThresholdUs;
        };
    }
    else
    {
        fnFilterRule = []([[maybe_unused]] const DebugConfig& configuration, [[maybe_unused]] const BaseTiming& timing) -> bool
        {
            return true;
        };
    }

    if (sort == DebugRequests::SortType::ByArea || sort == DebugRequests::SortType::ByAreaDetailed)
    {
        struct Compare
        {
            bool operator()(const AreaTiming* lhs, const AreaTiming* rhs) const
            {
                return lhs->m_averageTimeUs > rhs->m_averageTimeUs;
            }
        };
        AZStd::set<const AreaTiming*, Compare> theSet;
        for (const auto& areaTimingEntry : report.m_areaTimingData)
        {
            if (fnFilterRule(m_configuration, areaTimingEntry.second))
            {
                theSet.insert(&areaTimingEntry.second);
            }
        }
        DebugUtility::DumpSectorPerformanceReportSet(logHandle, filter, sort, theSet);
    }
    else if (sort == DebugRequests::SortType::BySector || sort == DebugRequests::SortType::BySectorDetailed)
    {
        struct Compare
        {
            bool operator()(const SectorTiming* lhs, const SectorTiming* rhs) const
            {
                return lhs->m_averageTimeUs > rhs->m_averageTimeUs;
            }
        };
        AZStd::set<const SectorTiming*, Compare> theSet;
        for (const auto& sectorTimingEntry : report.m_sectorTimingData)
        {
            if (fnFilterRule(m_configuration, sectorTimingEntry.second))
            {
                theSet.insert(&sectorTimingEntry.second);
            }
        }
        DebugUtility::DumpSectorPerformanceReportSet(logHandle, filter, sort, theSet);
    }

    AZ::IO::LocalFileIO::GetInstance()->Close(logHandle);
    AZ_TracePrintf("vegetation", "Wrote vegetation dump at %s \n", logFile.c_str());
}

void DebugComponent::ClearPerformanceReport()
{
    AZStd::lock_guard<decltype(m_reportMutex)> lock(m_reportMutex);
    m_thePerformanceReport = {};
    m_lastCollectionTime = {};
}

void DebugComponent::UpdateSystemConfig([[maybe_unused]] const AZ::ComponentConfig* config)
{
    AZStd::lock_guard<decltype(m_reportMutex)> lock(m_reportMutex);
    m_thePerformanceReport.m_sectorTimingData.clear();
    m_currentSortedTimingList.clear();
}
} // namespace Vegetation
