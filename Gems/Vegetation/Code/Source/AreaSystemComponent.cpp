/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <VegetationProfiler.h>
#include "AreaSystemComponent.h"

#include <Vegetation/Ebuses/AreaNotificationBus.h>
#include <Vegetation/Ebuses/AreaRequestBus.h>
#include <Vegetation/Ebuses/InstanceSystemRequestBus.h>
#include <Vegetation/Ebuses/AreaInfoBus.h>
#include <Vegetation/Ebuses/DebugNotificationBus.h>
#include <Vegetation/Ebuses/DebugSystemDataBus.h>
#include <SurfaceData/SurfaceDataSystemRequestBus.h>
#include <SurfaceData/Utility/SurfaceDataUtility.h>

#include <AzCore/Debug/Profiler.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Jobs/JobFunction.h>
#include <AzCore/std/chrono/chrono.h>
#include <AzCore/std/sort.h>
#include <AzCore/std/utils.h>
#include <AzCore/Component/TransformBus.h>


#include <AzFramework/Components/CameraBus.h>
#ifdef VEGETATION_EDITOR
#include <AzToolsFramework/API/EditorCameraBus.h>
#endif

#include <ISystem.h>
#include <cinttypes>

namespace Vegetation
{
    namespace AreaSystemUtil
    {
        template <typename T>
        void hash_combine_64(uint64_t& seed, T const& v)
        {
            AZStd::hash<T> hasher;
            seed ^= hasher(v) + 0x9e3779b97f4a7c13LL + (seed << 12) + (seed >> 4);
        }

        static bool UpdateVersion([[maybe_unused]] AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
        {
            if (classElement.GetVersion() < 4)
            {
                classElement.RemoveElementByName(AZ_CRC("ThreadSleepTimeMs", 0x9e86f79d));
            }
            return true;
        }
    }

    //////////////////////////////////////////////////////////////////////////
    // ViewRect

    inline bool AreaSystemComponent::ViewRect::IsInside(const SectorId& sector) const
    {
        const int inX = sector.first;
        const int inY = sector.second;
        return inX >= GetMinXSector() && inX <= GetMaxXSector() && inY >= GetMinYSector() && inY <= GetMaxYSector();
    }

    AreaSystemComponent::ViewRect AreaSystemComponent::ViewRect::Overlap(const ViewRect& b) const
    {
        ViewRect o;
        o.m_x = m_x > b.m_x ? m_x : b.m_x;
        o.m_y = m_y > b.m_y ? m_y : b.m_y;
        o.m_width = m_x + m_width > b.m_x + b.m_width ? b.m_x + b.m_width : m_x + m_width;
        o.m_height = m_y + m_height > b.m_y + b.m_height ? b.m_y + b.m_height : m_y + m_height;
        o.m_width -= o.m_x;
        o.m_height -= o.m_y;
        return o;
    }

    bool AreaSystemComponent::ViewRect::operator==(const ViewRect& b)
    {
        return m_x == b.m_x && m_y == b.m_y && m_width == b.m_width && m_height == b.m_height;
    }

    size_t AreaSystemComponent::ViewRect::GetNumSectors() const
    {
        return static_cast<size_t>(m_height * m_width);
    }

    bool AreaSystemComponent::ViewRect::operator!=(const ViewRect& b)
    {
        return m_x != b.m_x || m_y != b.m_y || m_width != b.m_width || m_height != b.m_height;
    }

    //////////////////////////////////////////////////////////////////////////
    // DirtySectors
    void AreaSystemComponent::DirtySectors::MarkDirty(const SectorId& sector)
    {
        m_dirtySet.insert(AZStd::move(sector));
    }

    void AreaSystemComponent::DirtySectors::MarkAllDirty()
    {
        m_allSectorsDirty = true;
    }

    void AreaSystemComponent::DirtySectors::Clear()
    {
        m_dirtySet.clear();
        m_allSectorsDirty = false;
    }

    bool AreaSystemComponent::DirtySectors::IsDirty(const SectorId& sector) const
    {
        return m_allSectorsDirty ||
            (!m_dirtySet.empty() && (m_dirtySet.find(sector) != m_dirtySet.end()));
    }

    //////////////////////////////////////////////////////////////////////////
    // AreaSystemConfig

    // These limitations are somewhat arbitrary.  It's possible to select combinations of larger values than these that will work successfully.
    // However, these values are also large enough that going beyond them is extremely likely to cause problems.
    const int AreaSystemConfig::s_maxViewRectangleSize = 128;
    const int AreaSystemConfig::s_maxSectorDensity = 64;
    const int AreaSystemConfig::s_maxSectorSizeInMeters = 1024;
    const int64_t AreaSystemConfig::s_maxVegetationInstances = 2 * 1024 * 1024;
    const int AreaSystemConfig::s_maxInstancesPerMeter = 16;

    void AreaSystemConfig::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<AreaSystemConfig, AZ::ComponentConfig>()
                ->Version(4, &AreaSystemUtil::UpdateVersion)
                ->Field("ViewRectangleSize", &AreaSystemConfig::m_viewRectangleSize)
                ->Field("SectorDensity", &AreaSystemConfig::m_sectorDensity)
                ->Field("SectorSizeInMeters", &AreaSystemConfig::m_sectorSizeInMeters)
                ->Field("ThreadProcessingIntervalMs", &AreaSystemConfig::m_threadProcessingIntervalMs)
                ->Field("SectorSearchPadding", &AreaSystemConfig::m_sectorSearchPadding)
                ->Field("SectorPointSnapMode", &AreaSystemConfig::m_sectorPointSnapMode)
            ;

            AZ::EditContext* edit = serialize->GetEditContext();
            if (edit)
            {
                edit->Class<AreaSystemConfig>(
                    "Vegetation Area System Config", "Handles the placement and removal of vegetation instance based on the vegetation area component rules")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &AreaSystemConfig::m_viewRectangleSize, "View Area Grid Size", "The number of sectors (per-side) of a managed grid in a scrolling view centered around the camera.")
                    ->Attribute(AZ::Edit::Attributes::ChangeValidate, &AreaSystemConfig::ValidateViewArea)
                    ->Attribute(AZ::Edit::Attributes::Min, 1)
                    ->Attribute(AZ::Edit::Attributes::Max, s_maxViewRectangleSize)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &AreaSystemConfig::m_sectorDensity, "Sector Point Density", "The number of equally-spaced vegetation instance grid placement points (per-side) within a sector")
                    ->Attribute(AZ::Edit::Attributes::ChangeValidate, &AreaSystemConfig::ValidateSectorDensity)
                    ->Attribute(AZ::Edit::Attributes::Min, 1)
                    ->Attribute(AZ::Edit::Attributes::Max, s_maxSectorDensity)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &AreaSystemConfig::m_sectorSizeInMeters, "Sector Size In Meters", "The size in meters (per-side) of each sector.")
                    ->Attribute(AZ::Edit::Attributes::ChangeValidate, &AreaSystemConfig::ValidateSectorSize)
                    ->Attribute(AZ::Edit::Attributes::Min, 1)
                    ->Attribute(AZ::Edit::Attributes::Max, s_maxSectorSizeInMeters)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &AreaSystemConfig::m_threadProcessingIntervalMs, "Thread Processing Interval", "The delay (in milliseconds) between processing queued thread tasks.")
                    ->Attribute(AZ::Edit::Attributes::Min, 0)
                    ->Attribute(AZ::Edit::Attributes::Max, 5000)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &AreaSystemConfig::m_sectorSearchPadding, "Sector Search Padding", "Increases the search radius for surrounding sectors when enumerating instances.")
                    ->Attribute(AZ::Edit::Attributes::Min, 0)
                    ->Attribute(AZ::Edit::Attributes::Max, 2)
                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &AreaSystemConfig::m_sectorPointSnapMode, "Sector Point Snap Mode", "Controls whether vegetation placement points are located at the corner or the center of the cell.")
                    ->EnumAttribute(SnapMode::Corner, "Corner")
                    ->EnumAttribute(SnapMode::Center, "Center")
                ;
            }
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<AreaSystemConfig>()
                ->Attribute(AZ::Script::Attributes::Category, "Vegetation")
                ->Constructor()
                ->Property("viewRectangleSize", BehaviorValueProperty(&AreaSystemConfig::m_viewRectangleSize))
                ->Property("sectorDensity", BehaviorValueProperty(&AreaSystemConfig::m_sectorDensity))
                ->Property("sectorSizeInMeters", BehaviorValueProperty(&AreaSystemConfig::m_sectorSizeInMeters))
                ->Property("threadProcessingIntervalMs", BehaviorValueProperty(&AreaSystemConfig::m_threadProcessingIntervalMs))
                ->Property("sectorPointSnapMode",
                [](AreaSystemConfig* config) { return static_cast<AZ::u8>(config->m_sectorPointSnapMode); },
                [](AreaSystemConfig* config, const AZ::u8& i) { config->m_sectorPointSnapMode = static_cast<SnapMode>(i); })
            ;
        }
    }

    AZ::Outcome<void, AZStd::string> AreaSystemConfig::ValidateViewArea(void* newValue, const AZ::Uuid& valueType)
    {
        if (azrtti_typeid<int>() != valueType)
        {
            AZ_Assert(false, "Unexpected value type");
            return AZ::Failure(AZStd::string("Unexpectedly received a non-int type for the View Area Grid Size!"));
        }

        int viewRectangleSize = *static_cast<int*>(newValue);
        const int instancesPerSector = m_sectorDensity * m_sectorDensity;
        const int totalSectors = viewRectangleSize * viewRectangleSize;

        int64_t totalInstances = instancesPerSector * totalSectors;

        if (totalInstances > s_maxVegetationInstances)
        {
            return AZ::Failure(
                AZStd::string::format("The combination of View Area Grid Size and Sector Point Density will create %" PRId64 " instances.  Only a max of %" PRId64 " instances is allowed.",
                totalInstances, s_maxVegetationInstances));
        }

        return AZ::Success();
    }

    AZ::Outcome<void, AZStd::string> AreaSystemConfig::ValidateSectorDensity(void* newValue, const AZ::Uuid& valueType)
    {
        if (azrtti_typeid<int>() != valueType)
        {
            AZ_Assert(false, "Unexpected value type");
            return AZ::Failure(AZStd::string("Unexpectedly received a non-int type for the Sector Point Density!"));
        }

        int sectorDensity = *static_cast<int*>(newValue);
        const int instancesPerSector = sectorDensity * sectorDensity;
        const int totalSectors = m_viewRectangleSize * m_viewRectangleSize;

        int64_t totalInstances = instancesPerSector * totalSectors;

        if (totalInstances >= s_maxVegetationInstances)
        {
            return AZ::Failure(
                AZStd::string::format("The combination of View Area Grid Size and Sector Point Density will create %" PRId64 " instances.  Only a max of %" PRId64 " instances is allowed.",
                totalInstances, s_maxVegetationInstances));
        }

        const float instancesPerMeter = static_cast<float>(sectorDensity) / static_cast<float>(m_sectorSizeInMeters);
        if (instancesPerMeter > s_maxInstancesPerMeter)
        {
            return AZ::Failure(AZStd::string::format("The combination of Sector Point Density and Sector Size in Meters will create %.1f instances per meter.  Only a max of %d instances per meter is allowed.",
                instancesPerMeter, s_maxInstancesPerMeter));
        }

        return AZ::Success();
    }

    AZ::Outcome<void, AZStd::string> AreaSystemConfig::ValidateSectorSize(void* newValue, const AZ::Uuid& valueType)
    {
        if (azrtti_typeid<int>() != valueType)
        {
            AZ_Assert(false, "Unexpected value type");
            return AZ::Failure(AZStd::string("Unexpectedly received a non-int type for the Sector Size In Meters!"));
        }

        int sectorSizeInMeters = *static_cast<int*>(newValue);

        const float instancesPerMeter = static_cast<float>(m_sectorDensity) / static_cast<float>(sectorSizeInMeters);
        if (instancesPerMeter > s_maxInstancesPerMeter)
        {
            return AZ::Failure(AZStd::string::format("The combination of Sector Point Density and Sector Size in Meters will create %.1f instances per meter.  Only a max of %d instances per meter is allowed.",
                instancesPerMeter, s_maxInstancesPerMeter));
        }

        return AZ::Success();
    }


    //////////////////////////////////////////////////////////////////////////
    // AreaSystemComponent

    void AreaSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC("VegetationAreaSystemService", 0x36da2b62));
    }

    void AreaSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC("VegetationAreaSystemService", 0x36da2b62));
    }

    void AreaSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC("VegetationDebugSystemService", 0x8cac3d67));
        services.push_back(AZ_CRC("VegetationInstanceSystemService", 0x823a6007));
        services.push_back(AZ_CRC("SurfaceDataSystemService", 0x1d44d25f));
    }

    void AreaSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        InstanceData::Reflect(context);

        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<AreaSystemComponent, AZ::Component>()
                ->Version(0)
                ->Field("Configuration", &AreaSystemComponent::m_configuration)
            ;

            if (AZ::EditContext* editContext = serialize->GetEditContext())
            {
                editContext->Class<AreaSystemComponent>("Vegetation Area System", "Manages registration and processing of vegetation area entities")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "Vegetation")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System", 0xc94d118b))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->Attribute(AZ::Edit::Attributes::HelpPageURL, "https://o3de.org/docs/user-guide/components/reference/")
                    ->DataElement(0, &AreaSystemComponent::m_configuration, "Configuration", "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                ;
            }
        }
        
        AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);
        if (behaviorContext)
        {
            behaviorContext->EBus<AreaSystemRequestBus>("AreaSystemRequestBus")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Category, "AreaSystem")
                ->Attribute(AZ::Script::Attributes::Module, "areasystem")
                ->Event("GetInstanceCountInAabb", &AreaSystemRequests::GetInstanceCountInAabb)
                ->Event("GetInstancesInAabb", &AreaSystemRequests::GetInstancesInAabb)
                ;
        }
    }

    AreaSystemComponent::AreaSystemComponent(const AreaSystemConfig& configuration)
        : m_configuration(configuration)
    {
    }

    void AreaSystemComponent::Init()
    {
    }

    void AreaSystemComponent::Activate()
    {
        // Wait for any lingering vegetation thread work to complete if necessary.  (This should never actually occur)
        AZ_Assert(m_threadData.m_vegetationThreadState == PersistentThreadData::VegetationThreadState::Stopped,
                  "Vegetation thread was still active even though AreaSystemComponent was deactivated.");
        AZStd::lock_guard<decltype(m_threadData.m_vegetationThreadMutex)> lockTasks(m_threadData.m_vegetationThreadMutex);
        m_threadData.m_vegetationThreadState = PersistentThreadData::VegetationThreadState::Stopped;
        m_threadData.m_vegetationDataSyncState = PersistentThreadData::VegetationDataSyncState::Synchronized;

        m_system = GetISystem();
        m_worldToSector = 1.0f / m_configuration.m_sectorSizeInMeters;

        // We initialize our vegetation threadData state here to ensure it gets recalculated the next time the thread runs.
        m_threadData.Init();

        AZ::TickBus::Handler::BusConnect();
        AreaSystemRequestBus::Handler::BusConnect();
        GradientSignal::SectorDataRequestBus::Handler::BusConnect();
        SystemConfigurationRequestBus::Handler::BusConnect();
        InstanceStatObjEventBus::Handler::BusConnect();
        CrySystemEventBus::Handler::BusConnect();
        AzFramework::Terrain::TerrainDataNotificationBus::Handler::BusConnect();
        SurfaceData::SurfaceDataSystemNotificationBus::Handler::BusConnect();

        m_vegTasks.FetchDebugData();
    }

    void AreaSystemComponent::Deactivate()
    {
        // Interrupt vegetation worker; deactivation deletes all vegetation, so there's no need to process updates.
        m_threadData.InterruptVegetationThread();

        //wait for the vegetation thread work to complete
        AZStd::lock_guard<decltype(m_threadData.m_vegetationThreadMutex)> lockTasks(m_threadData.m_vegetationThreadMutex);
        m_threadData.m_vegetationThreadState = PersistentThreadData::VegetationThreadState::Stopped;
        m_threadData.m_vegetationDataSyncState = PersistentThreadData::VegetationDataSyncState::Synchronized;

        AZ::TickBus::Handler::BusDisconnect();
        AreaSystemRequestBus::Handler::BusDisconnect();
        GradientSignal::SectorDataRequestBus::Handler::BusDisconnect();
        SystemConfigurationRequestBus::Handler::BusDisconnect();
        InstanceStatObjEventBus::Handler::BusDisconnect();
        CrySystemEventBus::Handler::BusDisconnect();
        AzFramework::Terrain::TerrainDataNotificationBus::Handler::BusDisconnect();
        SurfaceData::SurfaceDataSystemNotificationBus::Handler::BusDisconnect();

        // Clear sector data and any lingering vegetation thread state
        m_vegTasks.ClearSectors();
        m_threadData.Init();

        InstanceSystemRequestBus::Broadcast(&InstanceSystemRequestBus::Events::DestroyAllInstances);
        InstanceSystemRequestBus::Broadcast(&InstanceSystemRequestBus::Events::Cleanup);

        if (m_system)
        {
            m_system->GetISystemEventDispatcher()->RemoveListener(this);
            m_system = nullptr;
        }
    }

    bool AreaSystemComponent::ReadInConfig(const AZ::ComponentConfig* baseConfig)
    {
        if (auto config = azrtti_cast<const AreaSystemConfig*>(baseConfig))
        {
            m_configuration = *config;
            return true;
        }
        return false;
    }

    bool AreaSystemComponent::WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const
    {
        if (auto config = azrtti_cast<AreaSystemConfig*>(outBaseConfig))
        {
            if (m_configDirty)
            {
                *config = m_pendingConfigUpdate;
            }
            else
            {
                *config = m_configuration;
            }
            return true;
        }
        return false;
    }

    void AreaSystemComponent::UpdateSystemConfig(const AZ::ComponentConfig* baseConfig)
    {
        if (const auto config = azrtti_cast<const AreaSystemConfig*>(baseConfig))
        {
            if ((!m_configDirty && m_configuration == *config) || (m_configDirty && m_pendingConfigUpdate == *config))
            {
                return;
            }

            m_configDirty = true;
            m_pendingConfigUpdate = *config;
        }
    }

    void AreaSystemComponent::GetSystemConfig(AZ::ComponentConfig* outBaseConfig) const
    {
        WriteOutConfig(outBaseConfig);
    }

    bool AreaSystemComponent::ApplyPendingConfigChanges()
    {
        if (m_configDirty)
        {
            ReleaseWithoutCleanup();

            if (m_configuration.m_threadProcessingIntervalMs != m_pendingConfigUpdate.m_threadProcessingIntervalMs)
            {
                m_vegetationThreadTaskTimer = 0.0f;
            }

            ReadInConfig(&m_pendingConfigUpdate);
            m_worldToSector = 1.0f / m_configuration.m_sectorSizeInMeters;
            RefreshAllAreas();

            GradientSignal::SectorDataNotificationBus::Broadcast(&GradientSignal::SectorDataNotificationBus::Events::OnSectorDataConfigurationUpdated);

            m_configDirty = false;
            return true;
        }
        else
        {
            return false;
        }
    }

    void AreaSystemComponent::RegisterArea(AZ::EntityId areaId, AZ::u32 layer, AZ::u32 priority, const AZ::Aabb& bounds)
    {
        if (!bounds.IsValid())
        {
            AZ_Assert(false, "Vegetation Area registered with an invalid AABB.");
        }

        m_vegTasks.QueueVegetationTask([areaId, layer, priority, bounds](UpdateContext* context, PersistentThreadData* threadData, VegetationThreadTasks* vegTasks)
            {
                auto& area = threadData->m_globalVegetationAreaMap[areaId];
                area.m_id = areaId;
                area.m_layer = layer;
                area.m_priority = priority;
                area.m_bounds = bounds;
                AreaNotificationBus::Event(area.m_id, &AreaNotificationBus::Events::OnAreaRegistered);
                const auto& cachedMainThreadData = context->GetCachedMainThreadData();
                vegTasks->MarkDirtySectors(area.m_bounds, threadData->m_dirtySectorContents,
                                             cachedMainThreadData.m_worldToSector, cachedMainThreadData.m_currViewRect);
                threadData->m_activeAreasDirty = true;
            });
    }

    void AreaSystemComponent::UnregisterArea(AZ::EntityId areaId)
    {
        m_vegTasks.QueueVegetationTask([areaId](UpdateContext* context, PersistentThreadData* threadData, VegetationThreadTasks* vegTasks)
            {
                auto itArea = threadData->m_globalVegetationAreaMap.find(areaId);
                if (itArea != threadData->m_globalVegetationAreaMap.end())
                {
                    const auto& area = itArea->second;
                    AreaNotificationBus::Event(area.m_id, &AreaNotificationBus::Events::OnAreaUnregistered);
                    AreaNotificationBus::Event(area.m_id, &AreaNotificationBus::Events::OnAreaDisconnect);

                    const auto& cachedMainThreadData = context->GetCachedMainThreadData();
                    vegTasks->AddUnregisteredVegetationArea(area, cachedMainThreadData.m_worldToSector, cachedMainThreadData.m_currViewRect);
                    vegTasks->MarkDirtySectors(area.m_bounds, threadData->m_dirtySectorContents,
                                                 cachedMainThreadData.m_worldToSector, cachedMainThreadData.m_currViewRect);
                    threadData->m_globalVegetationAreaMap.erase(itArea);
                    threadData->m_activeAreasDirty = true;
                }
            });
    }

    void AreaSystemComponent::RefreshArea(AZ::EntityId areaId, AZ::u32 layer, AZ::u32 priority, const AZ::Aabb& bounds)
    {
        if (!bounds.IsValid())
        {
            AZ_Assert(false, "Vegetation Area refreshed with an invalid AABB.");
        }

        m_vegTasks.QueueVegetationTask([areaId, layer, priority, bounds](UpdateContext* context, PersistentThreadData* threadData, VegetationThreadTasks* vegTasks)
            {
                auto itArea = threadData->m_globalVegetationAreaMap.find(areaId);
                if (itArea != threadData->m_globalVegetationAreaMap.end())
                {
                    auto& area = itArea->second;
                    const auto& cachedMainThreadData = context->GetCachedMainThreadData();
                    vegTasks->MarkDirtySectors(area.m_bounds, threadData->m_dirtySectorContents,
                                                 cachedMainThreadData.m_worldToSector, cachedMainThreadData.m_currViewRect);

                    area.m_layer = layer;
                    area.m_priority = priority;
                    area.m_bounds = bounds;
                    AreaNotificationBus::Event(area.m_id, &AreaNotificationBus::Events::OnAreaRefreshed);

                    vegTasks->MarkDirtySectors(area.m_bounds, threadData->m_dirtySectorContents,
                                                 cachedMainThreadData.m_worldToSector, cachedMainThreadData.m_currViewRect);
                    threadData->m_activeAreasDirty = true;
                }
            });
    }

    void AreaSystemComponent::RefreshAllAreas()
    {
        m_vegTasks.QueueVegetationTask([](UpdateContext* context, PersistentThreadData* threadData, VegetationThreadTasks* vegTasks)
            {
                for (auto& entry : threadData->m_globalVegetationAreaMap)
                {
                    auto& area = entry.second;
                    area.m_layer = {};
                    area.m_priority = {};
                    area.m_bounds = AZ::Aabb::CreateNull();

                    AreaInfoBus::EventResult(area.m_layer, area.m_id, &AreaInfoBus::Events::GetLayer);
                    AreaInfoBus::EventResult(area.m_priority, area.m_id, &AreaInfoBus::Events::GetPriority);
                    AreaInfoBus::EventResult(area.m_bounds, area.m_id, &AreaInfoBus::Events::GetEncompassingAabb);
                    AreaNotificationBus::Event(area.m_id, &AreaNotificationBus::Events::OnAreaRefreshed);
                }

                // Set all existing sectors as needing to be rebuilt.  
                const auto& cachedMainThreadData = context->GetCachedMainThreadData();
                vegTasks->MarkDirtySectors(AZ::Aabb::CreateNull(), threadData->m_dirtySectorContents,
                                             cachedMainThreadData.m_worldToSector, cachedMainThreadData.m_currViewRect);
                vegTasks->MarkDirtySectors(AZ::Aabb::CreateNull(), threadData->m_dirtySectorSurfacePoints,
                                             cachedMainThreadData.m_worldToSector, cachedMainThreadData.m_currViewRect);
            });
    }

    void AreaSystemComponent::ClearAllAreas()
    {
        // Interrupt any work that's currently being done on the vegetation thread and destroy all vegetation instances
        ReleaseWithoutCleanup();
        // Queue a refresh of all the areas
        RefreshAllAreas();
        // Reset our timer for checking the vegetation queue for more work to ensure we process this immediately.
        m_vegetationThreadTaskTimer = 0.0f;
    }

    void AreaSystemComponent::MuteArea(AZ::EntityId areaId)
    {
        m_vegTasks.QueueVegetationTask([areaId](UpdateContext* /*context*/, PersistentThreadData* threadData, VegetationThreadTasks* /*vegTasks*/)
            {
                threadData->m_ignoredVegetationAreaSet.insert(areaId);
                threadData->m_activeAreasDirty = true;
            });
    }

    void AreaSystemComponent::UnmuteArea(AZ::EntityId areaId)
    {
        m_vegTasks.QueueVegetationTask([areaId](UpdateContext* /*context*/, PersistentThreadData* threadData, VegetationThreadTasks* /*vegTasks*/)
            {
                threadData->m_ignoredVegetationAreaSet.erase(areaId);
                threadData->m_activeAreasDirty = true;
            });
    }

    void AreaSystemComponent::OnSurfaceChanged(const AZ::EntityId& /*entityId*/, const AZ::Aabb& oldBounds, const AZ::Aabb& newBounds)
    {
        m_vegTasks.QueueVegetationTask([oldBounds, newBounds](UpdateContext* context, PersistentThreadData* threadData, VegetationThreadTasks* vegTasks)
        {
            const auto& cachedMainThreadData = context->GetCachedMainThreadData();

            // Mark the surface area prior to the surface data change as dirty
            vegTasks->MarkDirtySectors(oldBounds, threadData->m_dirtySectorContents,
                cachedMainThreadData.m_worldToSector, cachedMainThreadData.m_currViewRect);
            vegTasks->MarkDirtySectors(oldBounds, threadData->m_dirtySectorSurfacePoints,
                cachedMainThreadData.m_worldToSector, cachedMainThreadData.m_currViewRect);

            // Mark the surface area *after* the surface data change as dirty
            vegTasks->MarkDirtySectors(newBounds, threadData->m_dirtySectorContents,
                cachedMainThreadData.m_worldToSector, cachedMainThreadData.m_currViewRect);
            vegTasks->MarkDirtySectors(newBounds, threadData->m_dirtySectorSurfacePoints,
                cachedMainThreadData.m_worldToSector, cachedMainThreadData.m_currViewRect);
        });
    }

    void AreaSystemComponent::EnumerateInstancesInOverlappingSectors(const AZ::Aabb& bounds, AreaSystemEnumerateCallback callback) const
    {
        AZ_PROFILE_FUNCTION(Entity);

        if (!bounds.IsValid())
        {
            return;
        }

        // Get the minimum sector that overlaps the bounds, expanded outward based on sectorSearchPadding.
        const SectorId minSector = GetSectorId(bounds.GetMin(), m_worldToSector);
        AZ::Aabb minBounds = m_vegTasks.GetSectorBounds(SectorId(minSector.first - m_configuration.m_sectorSearchPadding,
                                                                 minSector.second - m_configuration.m_sectorSearchPadding),
                                                        m_configuration.m_sectorSizeInMeters);

        // Get the maximum sector that overlaps the bounds, expanded outward based on sectorSearchPadding.
        const SectorId maxSector = GetSectorId(bounds.GetMax(), m_worldToSector);
        AZ::Aabb maxBounds = m_vegTasks.GetSectorBounds(SectorId(maxSector.first + m_configuration.m_sectorSearchPadding,
                                                                 maxSector.second + m_configuration.m_sectorSearchPadding),
                                                        m_configuration.m_sectorSizeInMeters);

        // Use the expanded bounds to enumerate through all instances.
        AZ::Aabb expandedBounds(minBounds);
        expandedBounds.AddAabb(maxBounds);
        EnumerateInstancesInAabb(expandedBounds, callback);
    }

    void AreaSystemComponent::EnumerateInstancesInAabb(const AZ::Aabb& bounds, AreaSystemEnumerateCallback callback) const
    {
        AZ_PROFILE_FUNCTION(Entity);

        if (!bounds.IsValid())
        {
            return;
        }

        const SectorId minSector = GetSectorId(bounds.GetMin(), m_worldToSector);
        const int minX = minSector.first;
        const int minY = minSector.second;
        const SectorId maxSector = GetSectorId(bounds.GetMax(), m_worldToSector);
        const int maxX = maxSector.first;
        const int maxY = maxSector.second;

        // Lock the rolling window mutex for the entire enumerate to ensure that our set of sectors doesn't change during the loops.
        AZStd::lock_guard<decltype(m_vegTasks.m_sectorRollingWindowMutex)> lock(m_vegTasks.m_sectorRollingWindowMutex);
        for (int currY = minY; currY <= maxY; ++currY)
        {
            for (int currX = minX; currX <= maxX; ++currX)
            {
                const SectorInfo* sectorInfo = m_vegTasks.GetSector(SectorId(currX, currY));
                if (sectorInfo) // manual sector id's can be outside the active area
                {
                    for (const auto& claimPair : sectorInfo->m_claimedWorldPoints)
                    {
                        const auto& instanceData = claimPair.second;
                        if (bounds.Contains(instanceData.m_position))
                        {
                            if (callback(instanceData) != AreaSystemEnumerateCallbackResult::KeepEnumerating)
                            {
                                return;
                            }
                        }
                    }
                }
            }
        }
    }

    AZStd::size_t AreaSystemComponent::GetInstanceCountInAabb(const AZ::Aabb& bounds) const
    {
        AZStd::size_t result = 0;

        EnumerateInstancesInAabb(bounds, [&result](const auto&)
        {
            ++result;
            return AreaSystemEnumerateCallbackResult::KeepEnumerating;
        });

        return result;
    }

    AZStd::vector<Vegetation::InstanceData> AreaSystemComponent::GetInstancesInAabb(const AZ::Aabb& bounds) const
    {
        AZStd::vector<Vegetation::InstanceData> instanceList;

        EnumerateInstancesInAabb(bounds, [&instanceList](const auto& instance)
        {
            instanceList.push_back(instance);
            return AreaSystemEnumerateCallbackResult::KeepEnumerating;
        });

        return instanceList;
    }

    void AreaSystemComponent::GetPointsPerMeter(float& value) const
    {
        if (m_configuration.m_sectorDensity <= 0 || m_configuration.m_sectorSizeInMeters <= 0.0f)
        {
            value = 1.0f;
        }
        else
        {
            value = static_cast<float>(m_configuration.m_sectorDensity) / m_configuration.m_sectorSizeInMeters;
        }
    }

    void AreaSystemComponent::OnTick(float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint time)
    {
        AZ_PROFILE_FUNCTION(Entity);

        if (m_configuration.m_sectorSizeInMeters < 0)
        {
            m_configuration.m_sectorSizeInMeters = 1;
        }
        m_worldToSector = 1.0f / m_configuration.m_sectorSizeInMeters;
        m_vegetationThreadTaskTimer -= deltaTime;

        // Check to see if any vegetation data has changed since last tick, and if so, offload the updates to a vegetation thread.
        // - If the thread is currently stopped, check for data changes and start up the thread if changes are detected.
        // - If the thread has an interrupt requested, wait for the interrupt to stop the thread before checking and potentially running again.
        // - If the thread is currently running, only update if the data on the vegetation thread is currently synced with this thread.
        //   If the state is currently Updating or Dirty, wait for the vegetation thread to pick up the changes before trying to update the
        //   data again to avoid redundant mutex locks or the potential for mismatched state.

        if ((m_threadData.m_vegetationThreadState == PersistentThreadData::VegetationThreadState::Stopped) ||
                ((m_threadData.m_vegetationThreadState == PersistentThreadData::VegetationThreadState::Running) &&
                (m_threadData.m_vegetationDataSyncState == PersistentThreadData::VegetationDataSyncState::Synchronized)))
        {
            bool updateVegetationData = false;

            // if the config changes, we need to update the vegetation data
            if (ApplyPendingConfigChanges())
            {
                updateVegetationData = true;
            }

            // if the view rectangle changes, we need to update the vegetation data
            if (CalculateViewRect())
            {
                updateVegetationData = true;
            }

            if (m_vegetationThreadTaskTimer <= 0.0f)
            {
                m_vegetationThreadTaskTimer = m_configuration.m_threadProcessingIntervalMs * 0.001f;

                // If there are still vegetation tasks pending and we've waited the requested amount of time between queue checks,
                // then we need to update the vegetation data.
                if (m_vegTasks.VegetationThreadTasksPending())
                {
                    updateVegetationData = true;
                }
            }

            if (updateVegetationData)
            {
                // Our main thread has potentially updated its state, so cache a new copy of the pieces of state we need.
                {
                    m_cachedMainThreadData.m_currViewRect = m_currViewRect;
                    m_cachedMainThreadData.m_worldToSector = m_worldToSector;
                    m_cachedMainThreadData.m_sectorSizeInMeters = m_configuration.m_sectorSizeInMeters;
                    m_cachedMainThreadData.m_sectorDensity = m_configuration.m_sectorDensity;
                    m_cachedMainThreadData.m_sectorPointSnapMode = m_configuration.m_sectorPointSnapMode;
                }

                // Set the state to Dirty to signal the thread that it will need to pull a new copy of the main thread state data
                // and refresh the set of work that it's currently doing.  The thread will detect the change next time it looks
                // for work and clear the state after it pulls new data.
                m_threadData.m_vegetationDataSyncState = PersistentThreadData::VegetationDataSyncState::Dirty;

                // If the thread isn't currently running, start it up.
                if (m_threadData.m_vegetationThreadState == PersistentThreadData::VegetationThreadState::Stopped)
                {
                    //create a job to process vegetation areas, tasks, sectors in the background
                    m_threadData.m_vegetationThreadState = PersistentThreadData::VegetationThreadState::Running;
                    auto job = AZ::CreateJobFunction([this]()
                    {
                        AZ_PROFILE_SCOPE(Entity, "Vegetation::AreaSystemComponent::VegetationThread");

                        UpdateContext context;
                        context.Run(&m_threadData, &m_vegTasks, &m_cachedMainThreadData);

                        // After we're done processing as much as we can, clear our thread states and exit.
                        m_threadData.m_vegetationThreadState = PersistentThreadData::VegetationThreadState::Stopped;
                        m_threadData.m_vegetationDataSyncState = PersistentThreadData::VegetationDataSyncState::Synchronized;
                    }, true);
                    job->Start();
                }
            }
        }
    }

    void AreaSystemComponent::OnTerrainDataCreateBegin()
    {
        // Interrupt any in-process updates until the next tick.  We don't want to update
        // while terrain is being created, because we can end up with race conditions in
        // which we're querying terrain for some of the points while terrain is still only
        // partially created.  This can happen during creation because the HeightmapModified
        // event fires mid-creation, which can block in TerrainSurfaceDataSystemComponent on
        // the surface data mutex.  On the vegetation thread, ModifySurfacePoints in surface
        // components such as RiverSurfaceData can start successfully querying terrain because
        // the TerrainDataRequest bus is now valid, but doesn't always return fully-valid data yet.
        m_threadData.InterruptVegetationThread();
    }

    void AreaSystemComponent::OnTerrainDataDestroyBegin()
    {
        // Interrupt any in-process updates until the next tick.  We don't want to update
        // while terrain is being destroyed.  There aren't any *known* race conditions here, but
        // there are likely surface-related race conditions, so it's better to be safe.
        m_threadData.InterruptVegetationThread();
    }

    bool AreaSystemComponent::CalculateViewRect()
    {
        AZ_PROFILE_FUNCTION(Entity);

        //Get the active camera.
        bool cameraPositionIsValid = false;
        AZ::Vector3 cameraPosition(0.0f);

#ifdef VEGETATION_EDITOR
        Camera::EditorCameraRequestBus::BroadcastResult(cameraPositionIsValid, &Camera::EditorCameraRequestBus::Events::GetActiveCameraPosition, cameraPosition);
        if (!cameraPositionIsValid)
#endif // VEGETATION_EDITOR
        {
            AZ::EntityId activeCameraId;
            Camera::CameraSystemRequestBus::BroadcastResult(activeCameraId, &Camera::CameraSystemRequests::GetActiveCamera);
            if (activeCameraId.IsValid())
            {
                AZ::TransformBus::EventResult(cameraPosition, activeCameraId, &AZ::TransformInterface::GetWorldTranslation);
                cameraPositionIsValid = true;
            }
        }

        if (cameraPositionIsValid)
        {
            float posX = cameraPosition.GetX();
            float posY = cameraPosition.GetY();

            const int sectorSizeInMeters = m_configuration.m_sectorSizeInMeters;
            const int viewSize = m_configuration.m_viewRectangleSize;
            int halfViewSize = viewSize >> 1;
            posX -= halfViewSize * sectorSizeInMeters;
            posY -= halfViewSize * sectorSizeInMeters;

            auto prevViewRect = m_currViewRect;
            m_currViewRect.m_x = (int)(posX * m_worldToSector);
            m_currViewRect.m_y = (int)(posY * m_worldToSector);
            m_currViewRect.m_width = viewSize;
            m_currViewRect.m_height = viewSize;
            m_currViewRect.m_viewRectBounds =
                AZ::Aabb::CreateFromMinMax(
                    AZ::Vector3(
                        static_cast<float>(m_currViewRect.m_x * sectorSizeInMeters),
                        static_cast<float>(m_currViewRect.m_y * sectorSizeInMeters),
                        -AZ::Constants::FloatMax),
                    AZ::Vector3(
                        static_cast<float>((m_currViewRect.m_x + m_currViewRect.m_width) * sectorSizeInMeters),
                        static_cast<float>((m_currViewRect.m_y + m_currViewRect.m_height) * sectorSizeInMeters),
                        AZ::Constants::FloatMax));

            return prevViewRect != m_currViewRect;
        }
        else
        {
            return false;
        }
    }

    AreaSystemComponent::SectorId AreaSystemComponent::GetSectorId(const AZ::Vector3& worldPos, float worldToSector)
    {
        // Convert world positions into scaled integer sector IDs.
        // The clamp is necessary to ensure that excessive floating-point values don't overflow
        // the sector range.  The "nextafter" on the min/max limits are because integer min/max
        // lose precision when converted to float, causing them to grow to a larger range.  By
        // using nextafter(), we push them back inside the integer range.  Technically, this means
        // there are 128 integer numbers at each end of the range that we aren't using, but in practice
        // there will be many other precision bugs to deal with if we ever start using that range anyways.
        int wx = aznumeric_cast<int>(AZStd::clamp(floor(static_cast<float>(worldPos.GetX() * worldToSector)),
                                                  nextafter(aznumeric_cast<float>(std::numeric_limits<int>::min()), 0.0f),
                                                  nextafter(aznumeric_cast<float>(std::numeric_limits<int>::max()), 0.0f)));
        int wy = aznumeric_cast<int>(AZStd::clamp(floor(static_cast<float>(worldPos.GetY() * worldToSector)),
                                                  nextafter(aznumeric_cast<float>(std::numeric_limits<int>::min()), 0.0f),
                                                  nextafter(aznumeric_cast<float>(std::numeric_limits<int>::max()), 0.0f)));
        return SectorId(wx, wy);
    }

    AZ_FORCE_INLINE void AreaSystemComponent::ReleaseAllClaims()
    {
        // Interrupt update in process, if any
        m_threadData.InterruptVegetationThread();

        {
            // Wait for vegetation update thread to finish
            AZStd::lock_guard<decltype(m_threadData.m_vegetationThreadMutex)> lockTasks(m_threadData.m_vegetationThreadMutex);

            // Synchronously process any queued vegetation thread tasks on the main thread before clearing
            // out the sectors.  This allows us to update the active vegetation area lists and mark sectors
            // as dirty prior to clearing them out, so that way we don't refresh them a second time after
            // clearing them out.
            // (only process if the allocation has happened)
            if (!(m_worldToSector <= 0.0f))
            {
                UpdateContext threadContext;
                m_vegTasks.ProcessVegetationThreadTasks(&threadContext, &m_threadData);
                threadContext.UpdateActiveVegetationAreas(&m_threadData, m_currViewRect);
            }

            // Clear all sector data
            m_vegTasks.ClearSectors();
        }
    }

    void AreaSystemComponent::ReleaseWithoutCleanup()
    {
        // This method will destroy all active vegetation instances, but leave the vegetation render groups active
        // so that we're ready to process new instances.
        ReleaseAllClaims();
        InstanceSystemRequestBus::Broadcast(&InstanceSystemRequestBus::Events::DestroyAllInstances);
    }

    void AreaSystemComponent::ReleaseData()
    {
        // This method destroys all active vegetation instances and cleans up / unloads / destroys the vegetation render groups.
        ReleaseAllClaims();
        InstanceSystemRequestBus::Broadcast(&InstanceSystemRequestBus::Events::Cleanup);
    }

    void AreaSystemComponent::OnCrySystemInitialized(ISystem& system, [[maybe_unused]] const SSystemInitParams& systemInitParams)
    {
        m_system = &system;
        m_system->GetISystemEventDispatcher()->RegisterListener(this);
    }

    void AreaSystemComponent::OnCrySystemShutdown([[maybe_unused]] ISystem& system)
    {
        if (m_system)
        {
            m_system->GetISystemEventDispatcher()->RemoveListener(this);
            m_system = nullptr;
        }
    }

    void AreaSystemComponent::OnCryEditorBeginLevelExport()
    {
        // We need to free all our instances before exporting a level to ensure that none of the dynamic vegetation data
        // gets exported into the static vegetation data files.

        // Clear all our spawned vegetation data so that they don't get written out with the vegetation sectors.
        ReleaseData();
    }

    void AreaSystemComponent::OnCryEditorEndLevelExport(bool /*success*/)
    {
        // We don't need to do anything here.  When the vegetation game components reactivate themselves after the level export completes,
        // (see EditorVegetationComponentBase.h) they will trigger a refresh of the vegetation areas which will produce all our instances again.
    }

    void AreaSystemComponent::OnCryEditorCloseScene()
    {
        // Clear all our spawned vegetation data
        ReleaseData();
    }


    void AreaSystemComponent::OnSystemEvent(ESystemEvent event, [[maybe_unused]] UINT_PTR wparam, [[maybe_unused]] UINT_PTR lparam)
    {
        AZ_PROFILE_FUNCTION(Entity);

        switch (event)
        {
        case ESYSTEM_EVENT_GAME_MODE_SWITCH_START:
        case ESYSTEM_EVENT_LEVEL_LOAD_START:
        case ESYSTEM_EVENT_LEVEL_UNLOAD:
        case ESYSTEM_EVENT_EDITOR_SIMULATION_MODE_SWITCH_START:
        {
            ReleaseData();
            break;
        }
        default:
            break;
        }
    }

    //////////////////////////////////////////////////////////////////////////
    // VegetationThreadTasks

    void AreaSystemComponent::VegetationThreadTasks::QueueVegetationTask(AZStd::function<void(UpdateContext * context, PersistentThreadData * threadData, VegetationThreadTasks * vegTasks)> func)
    {
        AZStd::lock_guard<decltype(m_vegetationThreadTaskMutex)> lock(m_vegetationThreadTaskMutex);
        m_vegetationThreadTasks.push_back(func);

        if (m_debugData)
        {
            m_debugData->m_areaTaskQueueCount.store(static_cast<int>(m_vegetationThreadTasks.size()), AZStd::memory_order_relaxed);
        }
    }

    void AreaSystemComponent::VegetationThreadTasks::ProcessVegetationThreadTasks(UpdateContext* context, PersistentThreadData* threadData)
    {
        AZ_PROFILE_FUNCTION(Entity);

        VegetationThreadTasks::VegetationThreadTaskList tasks;
        {
            AZStd::lock_guard<decltype(m_vegetationThreadTaskMutex)> lock(m_vegetationThreadTaskMutex);
            AZStd::swap(tasks, m_vegetationThreadTasks);

            if (m_debugData)
            {
                m_debugData->m_areaTaskQueueCount.store(static_cast<int>(m_vegetationThreadTasks.size()), AZStd::memory_order_relaxed);
                m_debugData->m_areaTaskActiveCount.store(static_cast<int>(tasks.size()), AZStd::memory_order_relaxed);
            }
        }

        for (const auto& task : tasks)
        {
            task(context, threadData, this);

            if (m_debugData)
            {
                m_debugData->m_areaTaskActiveCount.fetch_sub(1, AZStd::memory_order_relaxed);
            }
        }
    }

    AZ::Aabb AreaSystemComponent::VegetationThreadTasks::GetSectorBounds(const SectorId& sectorId, int sectorSizeInMeters)
    {
        return AZ::Aabb::CreateFromMinMax(
            AZ::Vector3(
                static_cast<float>(sectorId.first * sectorSizeInMeters),
                static_cast<float>(sectorId.second * sectorSizeInMeters),
                -AZ::Constants::FloatMax),
            AZ::Vector3(
                static_cast<float>((sectorId.first + 1) * sectorSizeInMeters),
                static_cast<float>((sectorId.second + 1) * sectorSizeInMeters),
                AZ::Constants::FloatMax));
    }

    const AreaSystemComponent::SectorInfo* AreaSystemComponent::VegetationThreadTasks::GetSector(const SectorId& sectorId) const
    {
        AZ_PROFILE_FUNCTION(Entity);

        AZStd::lock_guard<decltype(m_sectorRollingWindowMutex)> lock(m_sectorRollingWindowMutex);
        auto itSector = m_sectorRollingWindow.find(sectorId);
        return itSector != m_sectorRollingWindow.end() ? &itSector->second : nullptr;
    }

    AreaSystemComponent::SectorInfo* AreaSystemComponent::VegetationThreadTasks::GetSector(const SectorId& sectorId)
    {
        AZ_PROFILE_FUNCTION(Entity);

        AZStd::lock_guard<decltype(m_sectorRollingWindowMutex)> lock(m_sectorRollingWindowMutex);
        auto itSector = m_sectorRollingWindow.find(sectorId);
        return itSector != m_sectorRollingWindow.end() ? &itSector->second : nullptr;
    }

    AreaSystemComponent::SectorInfo* AreaSystemComponent::VegetationThreadTasks::CreateSector(const SectorId& sectorId, int sectorDensity, int sectorSizeInMeters, SnapMode sectorPointSnapMode)
    {
        AZ_PROFILE_FUNCTION(Entity);

        SectorInfo sectorInfo;
        sectorInfo.m_id = sectorId;
        sectorInfo.m_bounds = GetSectorBounds(sectorId, sectorSizeInMeters);
        UpdateSectorPoints(sectorInfo, sectorDensity, sectorSizeInMeters, sectorPointSnapMode);

        AZStd::lock_guard<decltype(m_sectorRollingWindowMutex)> lock(m_sectorRollingWindowMutex);
        SectorInfo& sectorInfoRef = m_sectorRollingWindow[sectorInfo.m_id] = AZStd::move(sectorInfo);
        UpdateSectorCallbacks(sectorInfoRef);
        return &sectorInfoRef;
    }

    void AreaSystemComponent::VegetationThreadTasks::UpdateSectorPoints(SectorInfo& sectorInfo, int sectorDensity, int sectorSizeInMeters, SnapMode sectorPointSnapMode)
    {
        AZ_PROFILE_FUNCTION(Entity);
        const float vegStep = sectorSizeInMeters / static_cast<float>(sectorDensity);

        //build a free list of all points in the sector for areas to consume
        sectorInfo.m_baseContext.m_masks.clear();
        sectorInfo.m_baseContext.m_availablePoints.clear();
        sectorInfo.m_baseContext.m_availablePoints.reserve(sectorDensity * sectorDensity);

        // Determine within our texel area where we want to create our vegetation positions:
        // 0 = lower left corner, 0.5 = center
        const float texelOffset = (sectorPointSnapMode == SnapMode::Center) ? 0.5f : 0.0f;

        SurfaceData::SurfacePointListPerPosition availablePointsPerPosition;
        AZ::Vector2 stepSize(vegStep, vegStep);
        AZ::Vector3 regionOffset(texelOffset * vegStep, texelOffset * vegStep, 0.0f);
        AZ::Aabb regionBounds = sectorInfo.m_bounds;
        regionBounds.SetMin(regionBounds.GetMin() + regionOffset);

        // If we just used the sector bounds, floating-point error could sometimes cause an extra point to get generated
        // right at the max edge of the bounds.  So instead, we adjust our max placement bounds to be the exact size needed
        // for sectorDensity points to get placed, plus half a vegStep to account for a safe margin of floating-point error.
        // The exact size would be (sectorDensity - 1), so adding half a vegStep gives us (sectorDensity - 0.5f).
        // (We should be able to add anything less than 1 extra vegStep and still get exactly sectorDensity points)
        regionBounds.SetMax(regionBounds.GetMin() + AZ::Vector3(vegStep * (sectorDensity - 0.5f),
            vegStep * (sectorDensity - 0.5f), 0.0f));

        SurfaceData::SurfaceDataSystemRequestBus::Broadcast(
            &SurfaceData::SurfaceDataSystemRequestBus::Events::GetSurfacePointsFromRegion,
            regionBounds,
            stepSize,
            SurfaceData::SurfaceTagVector(),
            availablePointsPerPosition);

        AZ_Assert(availablePointsPerPosition.size() == (sectorDensity * sectorDensity),
            "Veg sector ended up with unexpected density (%d points created, %d expected)", availablePointsPerPosition.size(),
            (sectorDensity * sectorDensity));

        uint claimIndex = 0;
        for (auto& availablePoints : availablePointsPerPosition)
        {
            for (auto& surfacePoint : availablePoints.second)
            {
                sectorInfo.m_baseContext.m_availablePoints.push_back();
                ClaimPoint& claimPoint = sectorInfo.m_baseContext.m_availablePoints.back();
                claimPoint.m_handle = CreateClaimHandle(sectorInfo, ++claimIndex);
                claimPoint.m_position = surfacePoint.m_position;
                claimPoint.m_normal = surfacePoint.m_normal;
                claimPoint.m_masks = surfacePoint.m_masks;
                SurfaceData::AddMaxValueForMasks(sectorInfo.m_baseContext.m_masks, surfacePoint.m_masks);
            }
        }
    }

    void AreaSystemComponent::VegetationThreadTasks::UpdateSectorCallbacks(SectorInfo& sectorInfo)
    {
        //setup callback to test if matching point is already claimed
        sectorInfo.m_baseContext.m_existedCallback = [this, &sectorInfo](const ClaimPoint& point, const InstanceData& instanceData)
        {
            const ClaimHandle handle = point.m_handle;
            auto claimItr = sectorInfo.m_claimedWorldPointsBeforeFill.find(handle);
            bool exists = (claimItr != sectorInfo.m_claimedWorldPointsBeforeFill.end()) && InstanceData::IsSameInstanceData(instanceData, claimItr->second);

            if (exists)
            {
                CreateClaim(sectorInfo, handle, instanceData);
                VEG_PROFILE_METHOD(DebugNotificationBus::TryQueueBroadcast(&DebugNotificationBus::Events::CreateInstance, instanceData.m_instanceId, instanceData.m_position, instanceData.m_id));
            }

            return exists;
        };

        //setup callback to create claims for new instances
        sectorInfo.m_baseContext.m_createdCallback = [this, &sectorInfo](const ClaimPoint& point, const InstanceData& instanceData)
        {
            const ClaimHandle handle = point.m_handle;
            auto claimItr = sectorInfo.m_claimedWorldPointsBeforeFill.find(handle);
            bool exists = claimItr != sectorInfo.m_claimedWorldPointsBeforeFill.end();

            if (exists)
            {
                const auto& claimedInstanceData = claimItr->second;
                if (claimedInstanceData.m_id != instanceData.m_id)
                {
                    //must force bus connect if areas are different
                    AreaNotificationBus::Event(claimedInstanceData.m_id, &AreaNotificationBus::Events::OnAreaConnect);
                    AreaRequestBus::Event(claimedInstanceData.m_id, &AreaRequestBus::Events::UnclaimPosition, handle);
                    AreaNotificationBus::Event(claimedInstanceData.m_id, &AreaNotificationBus::Events::OnAreaDisconnect);
                }
                else
                {
                    //already connected during fill sector
                    AreaRequestBus::Event(claimedInstanceData.m_id, &AreaRequestBus::Events::UnclaimPosition, handle);
                }
            }

            CreateClaim(sectorInfo, handle, instanceData);
        };
    }

    void AreaSystemComponent::VegetationThreadTasks::DeleteSector(const SectorId& sectorId)
    {
        AZ_PROFILE_FUNCTION(Entity);

        AZStd::lock_guard<decltype(m_sectorRollingWindowMutex)> lock(m_sectorRollingWindowMutex);
        auto itSector = m_sectorRollingWindow.find(sectorId);
        if (itSector != m_sectorRollingWindow.end())
        {
            SectorInfo& sectorInfo(itSector->second);
            EmptySector(sectorInfo);
            m_sectorRollingWindow.erase(itSector);
        }
        else
        {
            AZ_Assert(false, "Sector marked for deletion but doesn't exist");
        }
    }

    template<class Fn>
    inline void AreaSystemComponent::VegetationThreadTasks::EnumerateSectorsInAabb(const AZ::Aabb& bounds, float worldToSector, const ViewRect& viewRect, Fn&& fn)
    {
        // Get the min/max sectors for the AABB.  If an invalid AABB is passed in, process every active sector.
        // (i.e. every sector in the current m_currViewRect)
        const SectorId boundsMinSector = bounds.IsValid() ? GetSectorId(bounds.GetMin(), worldToSector) : viewRect.GetMinSector();
        const SectorId boundsMaxSector = bounds.IsValid() ? GetSectorId(bounds.GetMax(), worldToSector) : viewRect.GetMaxSector();

        // The min bounds are set to the larger of the AABB min and the curr view rect min.
        // The max bounds are set to the smaller of the AABB max and the curr view rect max.
        // This lets us process only the sectors that overlap both.
        // Note that if the AABB doesn't overlap the curr view rect, the max will end up less
        // than the min, in which case we process no sectors.
        const int minX = AZStd::GetMax(boundsMinSector.first, viewRect.GetMinXSector());
        const int minY = AZStd::GetMax(boundsMinSector.second, viewRect.GetMinYSector());

        const int maxX = AZStd::GetMin(boundsMaxSector.first, viewRect.GetMaxXSector());
        const int maxY = AZStd::GetMin(boundsMaxSector.second, viewRect.GetMaxYSector());

        for (int currY = minY; currY <= maxY; ++currY)
        {
            for (int currX = minX; currX <= maxX; ++currX)
            {
                if (!fn(AZStd::move(SectorId(currX, currY))))
                {
                    return;
                }
            }
        }
    }

    void AreaSystemComponent::VegetationThreadTasks::AddUnregisteredVegetationArea(const VegetationAreaInfo& area, float worldToSector, const ViewRect& viewRect)
    {
        EnumerateSectorsInAabb(area.m_bounds, worldToSector, viewRect,
            [&](SectorId&& sectorId)
        {
            m_unregisteredVegetationAreaSet[AZStd::move(sectorId)].insert(area.m_id);
            return true;
        });
    }

    void AreaSystemComponent::VegetationThreadTasks::ReleaseUnregisteredClaims(SectorInfo& sectorInfo)
    {
        AZ_PROFILE_FUNCTION(Entity);

        if (!m_unregisteredVegetationAreaSet.empty())
        {
            auto unregisteredAreasForSector = m_unregisteredVegetationAreaSet.find(sectorInfo.m_id);
            if (unregisteredAreasForSector != m_unregisteredVegetationAreaSet.end())
            {
                for (auto claimItr = sectorInfo.m_claimedWorldPoints.begin(); claimItr != sectorInfo.m_claimedWorldPoints.end(); )
                {
                    if (unregisteredAreasForSector->second.find(claimItr->second.m_id) != unregisteredAreasForSector->second.end())
                    {
                        claimItr = sectorInfo.m_claimedWorldPoints.erase(claimItr);
                    }
                    else
                    {
                        ++claimItr;
                    }
                }

                m_unregisteredVegetationAreaSet.erase(unregisteredAreasForSector);
            }
        }
    }

    void AreaSystemComponent::VegetationThreadTasks::ReleaseUnusedClaims(SectorInfo& sectorInfo)
    {
        AZ_PROFILE_FUNCTION(Entity);

        AZStd::unordered_map<AZ::EntityId, AZStd::unordered_set<ClaimHandle>> claimsToRelease;

        // Group up all the previously-claimed-but-no-longer-claimed points based on area id
        for (const auto& claimPair : sectorInfo.m_claimedWorldPointsBeforeFill)
        {
            const auto& handle = claimPair.first;
            const auto& instanceData = claimPair.second;
            const auto& areaId = instanceData.m_id;
            if (sectorInfo.m_claimedWorldPoints.find(handle) == sectorInfo.m_claimedWorldPoints.end())
            {
                claimsToRelease[areaId].insert(handle);
            }
        }
        sectorInfo.m_claimedWorldPointsBeforeFill.clear();

        // Iterate over the claims by area id and release them
        for (const auto& claimPair : claimsToRelease)
        {
            const auto& areaId = claimPair.first;
            const auto& handles = claimPair.second;
            AreaNotificationBus::Event(areaId, &AreaNotificationBus::Events::OnAreaConnect);

            for (const auto& handle : handles)
            {
                AreaRequestBus::Event(areaId, &AreaRequestBus::Events::UnclaimPosition, handle);
            }

            AreaNotificationBus::Event(areaId, &AreaNotificationBus::Events::OnAreaDisconnect);
        }
    }

    void AreaSystemComponent::VegetationThreadTasks::FillSector(SectorInfo& sectorInfo, const VegetationAreaVector& activeAreas)
    {
        AZ_PROFILE_FUNCTION(Entity);
        VEG_PROFILE_METHOD(DebugNotificationBus::TryQueueBroadcast(&DebugNotificationBus::Events::FillSectorStart, sectorInfo.GetSectorX(), sectorInfo.GetSectorY(), AZStd::chrono::system_clock::now()));

        ReleaseUnregisteredClaims(sectorInfo);

        //m_availablePoints is a free list initialized with the complete set of points in the sector.
        ClaimContext activeContext = sectorInfo.m_baseContext;

        // Clear out the list of claimed world points before we begin
        sectorInfo.m_claimedWorldPointsBeforeFill = sectorInfo.m_claimedWorldPoints;
        sectorInfo.m_claimedWorldPoints.clear();

        //for all active areas attempt to spawn vegetation on sector grid positions
        for (const auto& area : activeAreas)
        {
            //if one or more areas claimed all the points in m_availablePoints, there's no reason to continue.
            if (activeContext.m_availablePoints.empty())
            {
                break;
            }

            //only consider areas that intersect this sector
            if (!area.m_bounds.IsValid() || area.m_bounds.Overlaps(sectorInfo.m_bounds))
            {
                VEG_PROFILE_METHOD(DebugNotificationBus::TryQueueBroadcast(&DebugNotificationBus::Events::FillAreaStart, area.m_id, AZStd::chrono::system_clock::now()));

                //each area is responsible for removing whatever points it claims from m_availablePoints, so subsequent areas will have fewer points to try to claim.
                AreaNotificationBus::Event(area.m_id, &AreaNotificationBus::Events::OnAreaConnect);
                AreaRequestBus::Event(area.m_id, &AreaRequestBus::Events::ClaimPositions, EntityIdStack{}, activeContext);
                AreaNotificationBus::Event(area.m_id, &AreaNotificationBus::Events::OnAreaDisconnect);

                VEG_PROFILE_METHOD(DebugNotificationBus::TryQueueBroadcast(&DebugNotificationBus::Events::FillAreaEnd, area.m_id, AZStd::chrono::system_clock::now(), aznumeric_cast<AZ::u32>(activeContext.m_availablePoints.size())));
            }
        }

        ReleaseUnusedClaims(sectorInfo);

        VEG_PROFILE_METHOD(DebugNotificationBus::TryQueueBroadcast(&DebugNotificationBus::Events::FillSectorEnd, sectorInfo.GetSectorX(), sectorInfo.GetSectorY(), AZStd::chrono::system_clock::now(), aznumeric_cast<AZ::u32>(activeContext.m_availablePoints.size())));
    }

    void AreaSystemComponent::VegetationThreadTasks::EmptySector(SectorInfo& sectorInfo)
    {
        AZ_PROFILE_FUNCTION(Entity);

        AZStd::unordered_map<AZ::EntityId, AZStd::unordered_set<ClaimHandle>> claimsToRelease;

        // group up all the points based on area id
        for (const auto& claimPair : sectorInfo.m_claimedWorldPoints)
        {
            const auto& handle = claimPair.first;
            const auto& instanceData = claimPair.second;
            const auto& areaId = instanceData.m_id;
            claimsToRelease[areaId].insert(handle);
        }
        sectorInfo.m_claimedWorldPoints.clear();

        // iterate over the claims by area id and release them
        for (const auto& claimPair : claimsToRelease)
        {
            const auto& areaId = claimPair.first;
            const auto& handles = claimPair.second;
            AreaNotificationBus::Event(areaId, &AreaNotificationBus::Events::OnAreaConnect);

            for (const auto& handle : handles)
            {
                AreaRequestBus::Event(areaId, &AreaRequestBus::Events::UnclaimPosition, handle);
            }

            AreaNotificationBus::Event(areaId, &AreaNotificationBus::Events::OnAreaDisconnect);
        }
    }

    void AreaSystemComponent::VegetationThreadTasks::ClearSectors()
    {
        AZ_PROFILE_FUNCTION(Entity);

        AZStd::lock_guard<decltype(m_sectorRollingWindowMutex)> lock(m_sectorRollingWindowMutex);
        for (auto& sectorPair : m_sectorRollingWindow)
        {
            EmptySector(sectorPair.second);
        }
        m_sectorRollingWindow.clear();

        // Clear any pending unregistrations; since all of the sectors have been cleared anyways, these don't affect anything
        m_unregisteredVegetationAreaSet.clear();
    }

    void AreaSystemComponent::VegetationThreadTasks::CreateClaim(SectorInfo& sectorInfo, const ClaimHandle handle, const InstanceData& instanceData)
    {
        AZ_PROFILE_FUNCTION(Entity);
        sectorInfo.m_claimedWorldPoints[handle] = instanceData;
    }

    ClaimHandle AreaSystemComponent::VegetationThreadTasks::CreateClaimHandle(const SectorInfo& sectorInfo, uint32_t index) const
    {
        AZ_PROFILE_FUNCTION(Entity);

        ClaimHandle handle = 0;
        AreaSystemUtil::hash_combine_64(handle, sectorInfo.m_id.first);
        AreaSystemUtil::hash_combine_64(handle, sectorInfo.m_id.second);
        AreaSystemUtil::hash_combine_64(handle, index);
        return handle;
    }

    void AreaSystemComponent::VegetationThreadTasks::MarkDirtySectors(const AZ::Aabb& bounds, DirtySectors& dirtySet, float worldToSector, const ViewRect& viewRect)
    {
        if (bounds.IsValid())
        {
            if (!dirtySet.IsAllDirty())
            {
                // Only mark individual sectors as dirty if we have valid AABB bounds and haven't
                // already marked *all* sectors as dirty.
                EnumerateSectorsInAabb(bounds, worldToSector, viewRect, [&](SectorId&& sectorId)
                {
                    dirtySet.MarkDirty(sectorId);
                    return true;
                });
            }
        }
        else
        {
            // If we have invalid bounds, we can mark all sectors as dirty without needing
            // to add each one to the list.
            dirtySet.MarkAllDirty();
        }
    }

    void AreaSystemComponent::VegetationThreadTasks::FetchDebugData()
    {
        VEG_PROFILE_METHOD(DebugSystemDataBus::BroadcastResult(m_debugData, &DebugSystemDataBus::Events::GetDebugData));
    }

    //////////////////////////////////////////////////////////////////////////
    // PersistentThreadData

    AZ_FORCE_INLINE void AreaSystemComponent::PersistentThreadData::InterruptVegetationThread()
    {
        auto expected = PersistentThreadData::VegetationThreadState::Running;
        m_vegetationThreadState.compare_exchange_strong(expected, PersistentThreadData::VegetationThreadState::InterruptRequested);
    }

    //////////////////////////////////////////////////////////////////////////
    // UpdateContext

    void AreaSystemComponent::UpdateContext::Run(PersistentThreadData* threadData, VegetationThreadTasks* vegTasks, CachedMainThreadData* cachedMainThreadData)
    {
        AZ_PROFILE_FUNCTION(Entity);

        // Ensure that the main thread doesn't activate or deactivate the component until after this thread finishes.
        // Note that this does *not* prevent the main thread from running OnTick, which can communicate data changes
        // to this thread while it's still processing work.
        AZStd::lock_guard<decltype(threadData->m_vegetationThreadMutex)> lockTasks(threadData->m_vegetationThreadMutex);

        bool keepProcessing = true;
        while (keepProcessing && (threadData->m_vegetationThreadState != PersistentThreadData::VegetationThreadState::InterruptRequested))
        {
            AZ_PROFILE_SCOPE(Entity, "Vegetation::AreaSystemComponent::UpdateContext::Run-InnerLoop");
            // Update thread state if its dirty
            PersistentThreadData::VegetationDataSyncState expected = PersistentThreadData::VegetationDataSyncState::Dirty;
            if (threadData->m_vegetationDataSyncState.compare_exchange_strong(expected, PersistentThreadData::VegetationDataSyncState::Updating))
            {
                // A dirty state can consist of one or more of the following:
                // - Main thread has changed veg configuration
                // - Main thread has changed current view rectangle
                // - Vegetation tasks have been queued for this thread to process

                // Our main thread has potentially updated its state, so cache a new copy of the pieces of state we need.
                m_cachedMainThreadData = *cachedMainThreadData;

                // Run through all the queued tasks to update vegetation area active states and lists of dirty sectors
                vegTasks->ProcessVegetationThreadTasks(this, threadData);

                // Now that we've processed all the queued tasks, gather a list of active areas that affect our visible sectors, sorted by priority
                UpdateActiveVegetationAreas(threadData, m_cachedMainThreadData.m_currViewRect);

                // Refresh the lists of sectors to create / update / remove
                keepProcessing = UpdateSectorWorkLists(threadData, vegTasks);

                // We've finished refreshing the thread work state, so mark ourselves as synchronized.
                threadData->m_vegetationDataSyncState = PersistentThreadData::VegetationDataSyncState::Synchronized;
            }

            if (keepProcessing)
            {
                keepProcessing = UpdateOneSector(threadData, vegTasks);
            }
        }
    }

    void AreaSystemComponent::UpdateContext::UpdateActiveVegetationAreas(PersistentThreadData* threadData, const ViewRect& viewRect)
    {
        AZ_PROFILE_FUNCTION(Entity);

        //build a priority sorted list of all active areas
        if (threadData->m_activeAreasDirty)
        {
            threadData->m_activeAreasDirty = false;
            threadData->m_activeAreas.clear();
            threadData->m_activeAreas.reserve(threadData->m_globalVegetationAreaMap.size());
            for (const auto& areaPair : threadData->m_globalVegetationAreaMap)
            {
                const auto& area = areaPair.second;

                //if this is an area being ignored due to a parent area blender, skip it
                if (threadData->m_ignoredVegetationAreaSet.find(area.m_id) != threadData->m_ignoredVegetationAreaSet.end())
                {
                    continue;
                }

                //do any per area setup or checks since the state of areas and entities with the system has changed
                bool prepared = false;
                AreaNotificationBus::Event(area.m_id, &AreaNotificationBus::Events::OnAreaConnect);
                AreaRequestBus::EventResult(prepared, area.m_id, &AreaRequestBus::Events::PrepareToClaim, EntityIdStack{});
                AreaNotificationBus::Event(area.m_id, &AreaNotificationBus::Events::OnAreaDisconnect);
                if (!prepared)
                {
                    // if PrepareToClaim returned false, this area is declaring itself as inactive.
                    // The area will need to call RefreshArea() if/when its state should change to active.
                    continue;
                }

                threadData->m_activeAreas.push_back(area);
            }

            AZStd::sort(threadData->m_activeAreas.begin(), threadData->m_activeAreas.end(), [](const auto& lhs, const auto& rhs)
            {
                return AZStd::make_pair(lhs.m_layer, lhs.m_priority) > AZStd::make_pair(rhs.m_layer, rhs.m_priority);
            });
        }

        //further reduce set of active areas to only include ones that intersect the bubble
        AZ::Aabb bubbleBounds = viewRect.GetViewRectBounds();
        threadData->m_activeAreasInBubble = threadData->m_activeAreas;
        threadData->m_activeAreasInBubble.erase(
            AZStd::remove_if(
                threadData->m_activeAreasInBubble.begin(),
                threadData->m_activeAreasInBubble.end(),
                [bubbleBounds](const auto& area) { return area.m_bounds.IsValid() && !area.m_bounds.Overlaps(bubbleBounds); }),
            threadData->m_activeAreasInBubble.end());
    }

    bool AreaSystemComponent::UpdateContext::UpdateSectorWorkLists(PersistentThreadData* threadData, VegetationThreadTasks* vegTasks)
    {
        AZ_PROFILE_FUNCTION(Entity);

        auto& worldToSector = m_cachedMainThreadData.m_worldToSector;
        auto& currViewRect = m_cachedMainThreadData.m_currViewRect;

        // only process the sectors if the allocation has happened
        if (worldToSector <= 0.0f)
        {
            return false;
        }

        bool deleteAllSectors = false;

        // Early exit if no active areas, no sectors are marked as dirty or updating, and there
        // are no sectors left in our rolling window.
        // Until an area becomes active again, there's no work that sectors should need to do.
        if (threadData->m_activeAreasInBubble.empty() &&
            threadData->m_dirtySectorContents.IsNoneDirty() &&
            threadData->m_dirtySectorSurfacePoints.IsNoneDirty())
        {
            AZStd::lock_guard<decltype(vegTasks->m_sectorRollingWindowMutex)> lock(vegTasks->m_sectorRollingWindowMutex);
            if (vegTasks->m_sectorRollingWindow.empty())
            {
                return !m_deleteWorkList.empty() || !m_updateWorkList.empty();
            }
            else
            {
                // No active areas left in our view bubble, so queue up the deletion of all remaining active sectors.
                deleteAllSectors = true;
            }
        }

        // Cache off the total number of sectors that *should* be active in the view rectangle.  We'll use this
        // when processing sectors to ensure that we start to prioritize deletes whenever our number of active sectors
        // gets above this number.
        m_viewRectSectorCount = currViewRect.GetNumSectors();

        // remove any sectors marked for update which are no longer in the view rectangle
        m_updateWorkList.erase(
            AZStd::remove_if(
                m_updateWorkList.begin(),
                m_updateWorkList.end(),
                [currViewRect](const auto& entry) {return !currViewRect.IsInside(entry.first); }),
            m_updateWorkList.end());
        AZ_Assert(m_updateWorkList.size() <= m_viewRectSectorCount, "Refreshed RequestedUpdate list should not be larger than the view rectangle.");

        // Clear our delete work list, we'll recreate it and sort it again below.
        // Note: We do NOT clear m_updateWorkList, because we use it to incrementally determine any new
        // updates to add to the queue.  Without it, we wouldn't know if a previous data change caused
        // us to mark any sectors still in view as needing an update.
        m_deleteWorkList.clear();

        // If we're deleting all sectors, make sure we don't have any of them previously queued up for creation / updating.
        if (deleteAllSectors)
        {
            m_updateWorkList.clear();
        }

        // Run through our list of active sectors and determine which ones need adding / updating / deleting
        {
            AZStd::lock_guard<decltype(vegTasks->m_sectorRollingWindowMutex)> lock(vegTasks->m_sectorRollingWindowMutex);

            // To create our add / update / delete lists, we need two loops.  The first loops through the *new* view rectangle
            // looking for missing sectors to add.  The second loops through the *current* set of active sectors looking for any
            // to update or remove.

            // First loop:  Determine non-existent sectors which need to be created
            if (!deleteAllSectors)
            {
                for (int y = currViewRect.m_y; y < currViewRect.m_y + currViewRect.m_height; ++y)
                {
                    for (int x = currViewRect.m_x; x < currViewRect.m_x + currViewRect.m_width; ++x)
                    {
                        SectorId sectorId(x, y);
                        if (vegTasks->m_sectorRollingWindow.find(sectorId) == vegTasks->m_sectorRollingWindow.end())
                        {
                            // If the sector doesn't currently exist and it belongs in the view rect, request a creation.
                            // (This will either create a new entry or overwrite an existing pending Create request)
                            auto found = AZStd::find_if(m_updateWorkList.begin(), m_updateWorkList.end(), [sectorId](auto& entry) { return (entry.first == sectorId); });
                            if (found != m_updateWorkList.end())
                            {
                                // If the update entry already exists, overwrite the state.  We don't need to check or
                                // preserve the existing state because Create is the most comprehensive update we can do.
                                found->second = UpdateMode::Create;
                            }
                            else
                            {
                                m_updateWorkList.emplace_back(sectorId, UpdateMode::Create);
                            }

                            // Since we've already removed entries that aren't in the view rect, and these loops are only
                            // adding entries in the view rect, at this point our update work list size should never get
                            // larger than the set of sectors in the view rect.
                            AZ_Assert(m_updateWorkList.size() <= m_viewRectSectorCount, "Too many update requests added");
                        }
                    }
                }
            }

            // Second loop:  Determine any existing sectors which need to be updated or deleted
            for (auto& sector : vegTasks->m_sectorRollingWindow)
            {
                auto& sectorId = sector.first;

                if (deleteAllSectors || !currViewRect.IsInside(sectorId))
                {
                    // Active sector is no longer within view or there are no active areas, so delete it
                    m_deleteWorkList.emplace_back(AZStd::move(sectorId));
                }
                else if (threadData->m_dirtySectorSurfacePoints.IsDirty(sectorId))
                {
                    // Active sector has new surface point information, so rebuild surface cache and fill
                    // (This will either create a new entry, or overwrite an existing fill or rebuild request)
                    auto found = AZStd::find_if(m_updateWorkList.begin(), m_updateWorkList.end(), [sectorId](auto& entry) { return (entry.first == sectorId); });
                    if (found != m_updateWorkList.end())
                    {
                        // If the update entry already exists, overwrite the state.  We don't need to check or
                        // preserve the state since it should only contain either Rebuild or Fill, and Rebuild
                        // is more comprehensive than Fill.
                        AZ_Assert(found->second != UpdateMode::Create, "Create requests shouldn't exist for active sectors!");
                        found->second = UpdateMode::RebuildSurfaceCacheAndFill;
                    }
                    else
                    {
                        m_updateWorkList.emplace_back(sectorId, UpdateMode::RebuildSurfaceCacheAndFill);
                    }

                    // We shouldn't ever have an update list that's larger than the set of sectors in the view rect.
                    AZ_Assert(m_updateWorkList.size() <= m_viewRectSectorCount, "Too many update requests added");
                }
                else if (threadData->m_dirtySectorContents.IsDirty(sectorId))
                {
                    // Active sector has new veg area information, so refill it.
                    auto found = AZStd::find_if(m_updateWorkList.begin(), m_updateWorkList.end(), [sectorId](auto& entry)
                    { return (entry.first == sectorId); });
                    if (found == m_updateWorkList.end())
                    {
                        // Only add Fill entries if no update request exists for this sector.  We don't
                        // overwrite existing entries because an existing entry might have previously
                        // requested "RebuildSurfaceCacheAndFill", which is more comprehensive than this request.
                        m_updateWorkList.emplace_back(sectorId, UpdateMode::Fill);

                        // We shouldn't ever have an update list that's larger than the set of sectors in the view rect.
                        AZ_Assert(m_updateWorkList.size() <= m_viewRectSectorCount, "Too many update requests added");
                    }
                }
            }
        }

        // We've finished processing our dirtySector lists, so clear them.
        threadData->m_dirtySectorContents.Clear();
        threadData->m_dirtySectorSurfacePoints.Clear();

        // sort work by distance from center of the view rectangle.
        if (currViewRect.GetViewRectBounds().IsValid())
        {
            float sectorCenterX = (currViewRect.GetMinXSector() + currViewRect.GetMaxXSector()) / 2.0f;
            float sectorCenterY = (currViewRect.GetMinYSector() + currViewRect.GetMaxYSector()) / 2.0f;

            // Sort function that returns true if the lhs is "closer" than the rhs to the center.
            // The choice of sort algorithm is somewhat a question of preference, and could potentially be made a policy
            // choice at some point.  The current choice uses "number of sectors from center" as the primary sort criteria,
            // with a secondary sort on y and x values to get a deterministic sort pattern.  This algorithm updates the vegetation
            // outward in cocentric circles.
            // Here are some other possibilities of algorithm choices:
            // 1) float maxDist = AZStd::GetMax(fabs(id.first - sectorCenterX), fabs(id.second - sectorCenterY));
            // This moves outward in cocentric squares.
            // 2) float maxDist = GetSectorBounds(id).GetCenter().GetDistanceSq(currViewRect.GetViewRectBounds().GetCenter());
            // This moves outward in cocentric circles, similar to our chosen algorithm, but in more of a "pinwheel" pattern
            // that fans out from the axis lines.
            // 3) We could feed in camera orientation as well, and use that to further prioritize sectors within view.  The concern
            // with choosing this approach is that it will update the work lists much more rapidly than the vegetation can spawn, so
            // it the extra updates and calculations could easily cause sector choices that constantly lag behind the current view,
            // producing similar to worse results than our current algorithm.

            // With any of these choices, the secondary sort gives a deterministic update pattern when the distances are equal.

            auto sectorCompare = [sectorCenterX, sectorCenterY](const SectorId& lhsSectorId, const SectorId& rhsSectorId, bool sortClosestFirst)
            {
                const float lhsMaxDist = ((lhsSectorId.first - sectorCenterX) * (lhsSectorId.first - sectorCenterX)) + ((lhsSectorId.second - sectorCenterY) * (lhsSectorId.second - sectorCenterY));
                const float rhsMaxDist = ((rhsSectorId.first - sectorCenterX) * (rhsSectorId.first - sectorCenterX)) + ((rhsSectorId.second - sectorCenterY) * (rhsSectorId.second - sectorCenterY));
                return (lhsMaxDist < rhsMaxDist) ? sortClosestFirst :           // Return if one sector is closer than the other to the center.
                        ((lhsMaxDist > rhsMaxDist) ? !sortClosestFirst :
                         ((lhsSectorId.second < rhsSectorId.second) ? true :    // If it's the same distance return if the Y value is smaller...
                          ((lhsSectorId.second > rhsSectorId.second) ? false :
                           (lhsSectorId.first < rhsSectorId.first))));          // If the Y value is the same, return if the X value is smaller.
            };

            AZStd::sort(m_updateWorkList.begin(), m_updateWorkList.end(), [sectorCompare](const auto& lhs, const auto& rhs)
            {
                // We always pull from the end of the list, so we sort the *closest* sectors to the end.
                // That way we create / update the closest sectors first.
                return sectorCompare(lhs.first, rhs.first, false);
            });

            AZStd::sort(m_deleteWorkList.begin(), m_deleteWorkList.end(), [sectorCompare](const auto& lhs, const auto& rhs)
            {
                // We always pull from the end of the list, so we sort the *furthest* sectors to the end.
                // That way we delete the furthest sectors first.
                return sectorCompare(lhs, rhs, true);
            });
        }

        return !m_deleteWorkList.empty() || !m_updateWorkList.empty();
    }

    bool AreaSystemComponent::UpdateContext::UpdateOneSector(PersistentThreadData* threadData, VegetationThreadTasks* vegTasks)
    {
        AZ_PROFILE_FUNCTION(Entity);

        // This chooses work in the following order:
        // 1) Delete if we have more sectors than the total that should be in the view rectangle
        // 2) Create/update if we have any sectors to create / update
        // 3) Delete if we have any sectors to delete

        // Delete if there are more active sectors than the number of desired sectors or the update list is empty.
        if (!m_deleteWorkList.empty())
        {
            AZStd::lock_guard<decltype(vegTasks->m_sectorRollingWindowMutex)> lock(vegTasks->m_sectorRollingWindowMutex);

            if ((vegTasks->m_sectorRollingWindow.size() > m_viewRectSectorCount) || m_updateWorkList.empty())
            {
                vegTasks->DeleteSector(m_deleteWorkList.back());
                m_deleteWorkList.pop_back();
                return true;
            }
        }

        // Create / update if there's anything to do and we didn't prioritize a delete.
        if (!m_updateWorkList.empty())
        {
            auto& updateEntry = m_updateWorkList.back();
            SectorId sectorId = updateEntry.first;
            UpdateMode mode = updateEntry.second;
            m_updateWorkList.pop_back();

            {
                AZStd::lock_guard<decltype(vegTasks->m_sectorRollingWindowMutex)> lock(vegTasks->m_sectorRollingWindowMutex);

                auto& sectorDensity = m_cachedMainThreadData.m_sectorDensity;
                auto& sectorSizeInMeters = m_cachedMainThreadData.m_sectorSizeInMeters;
                auto& sectorPointSnapMode = m_cachedMainThreadData.m_sectorPointSnapMode;

                switch (mode)
                {
                    case UpdateMode::RebuildSurfaceCacheAndFill:
                    {
                        auto sectorInfo = vegTasks->GetSector(sectorId);
                        AZ_Assert(sectorInfo, "Sector update mode is 'RebuildSurfaceCache' but sector doesn't exist");
                        vegTasks->UpdateSectorPoints(*sectorInfo, sectorDensity, sectorSizeInMeters, sectorPointSnapMode);
                        vegTasks->FillSector(*sectorInfo, threadData->m_activeAreasInBubble);
                    }
                    break;

                    case UpdateMode::Fill:
                    {
                        auto sectorInfo = vegTasks->GetSector(sectorId);
                        AZ_Assert(sectorInfo, "Sector update mode is 'Fill' but sector doesn't exist");
                        vegTasks->FillSector(*sectorInfo, threadData->m_activeAreasInBubble);
                    }
                    break;

                    case UpdateMode::Create:
                    {
                        AZ_Assert(!vegTasks->GetSector(sectorId), "Sector update mode is 'Create' but sector already exists");
                        auto sectorInfo = vegTasks->CreateSector(sectorId, sectorDensity, sectorSizeInMeters, sectorPointSnapMode);
                        vegTasks->FillSector(*sectorInfo, threadData->m_activeAreasInBubble);
                    }
                    break;
                }
            }

            return true;
        }

        // No sectors left to process, so tell our main loop to stop processing.
        return false;
    }

}
