/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <VegetationProfiler.h>
#include "InstanceSystemComponent.h"

#include <AzCore/Debug/Profiler.h> 
#include <AzCore/Jobs/JobFunction.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/smart_ptr/make_shared.h>

#include <Vegetation/Ebuses/AreaInfoBus.h>
#include <Vegetation/Ebuses/AreaSystemRequestBus.h>
#include <Vegetation/Ebuses/DebugNotificationBus.h>
#include <Vegetation/Ebuses/DebugSystemDataBus.h>

namespace Vegetation
{
    namespace InstanceSystemUtil
    {
        namespace Constants
        {
            static const int s_minTaskTimePerTick = 0;
            static const int s_maxTaskTimePerTick = 33000; //capping at 33ms presumably to maintain 30fps
            static const int s_minTaskBatchSize = 1;
            static const int s_maxTaskBatchSize = 2000; //prevents user from reserving excessive space as batches are processed faster than they can be filled
        }
    };

    //////////////////////////////////////////////////////////////////////////
    // InstanceSystemConfig

    void InstanceSystemConfig::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<InstanceSystemConfig, AZ::ComponentConfig>()
                ->Version(3)
                ->Field("MaxInstanceProcessTimeMicroseconds", &InstanceSystemConfig::m_maxInstanceProcessTimeMicroseconds)
                ->Field("MaxInstanceTaskBatchSize", &InstanceSystemConfig::m_maxInstanceTaskBatchSize)
                ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<InstanceSystemConfig>(
                    "Vegetation Instance System", "Manages vegetation instance and render groups")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(0, &InstanceSystemConfig::m_maxInstanceProcessTimeMicroseconds, "Max Instance Process Time Microseconds", "Maximum number of microseconds allowed for processing instance management tasks each tick")
                        ->Attribute(AZ::Edit::Attributes::Min, InstanceSystemUtil::Constants::s_minTaskTimePerTick)
                        ->Attribute(AZ::Edit::Attributes::Max, InstanceSystemUtil::Constants::s_maxTaskTimePerTick)
                    ->DataElement(0, &InstanceSystemConfig::m_maxInstanceTaskBatchSize, "Max Instance Task Batch Size", "Maximum number of instance management tasks that can be batch processed together")
                        ->Attribute(AZ::Edit::Attributes::Min, InstanceSystemUtil::Constants::s_minTaskBatchSize)
                        ->Attribute(AZ::Edit::Attributes::Max, InstanceSystemUtil::Constants::s_maxTaskBatchSize)
                    ;
            }
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<InstanceSystemConfig>()
                ->Attribute(AZ::Script::Attributes::Category, "Vegetation")
                ->Constructor()
                ->Property("maxInstanceProcessTimeMicroseconds", BehaviorValueProperty(&InstanceSystemConfig::m_maxInstanceProcessTimeMicroseconds))
                ->Property("maxInstanceTaskBatchSize", BehaviorValueProperty(&InstanceSystemConfig::m_maxInstanceTaskBatchSize))
                ;
        }
    }

    //////////////////////////////////////////////////////////////////////////
    // InstanceSystemComponent

    void InstanceSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<InstanceSystemComponent, AZ::Component>()
                ->Version(0)
                ->Field("Configuration", &InstanceSystemComponent::m_configuration)
                ;

            if (AZ::EditContext* editContext = serialize->GetEditContext())
            {
                editContext->Class<InstanceSystemComponent>("Vegetation Instance System", "Manages and processes requests to create and destroy vegetation instance render nodes")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "Vegetation")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System", 0xc94d118b))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->Attribute(AZ::Edit::Attributes::HelpPageURL, "https://o3de.org/docs/user-guide/components/reference/")
                    ->DataElement(0, &InstanceSystemComponent::m_configuration, "Configuration", "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ;
            }
        }
    }

    InstanceSystemComponent::InstanceSystemComponent(const InstanceSystemConfig& configuration)
        : m_configuration(configuration)
    {
    }

    InstanceSystemComponent::~InstanceSystemComponent()
    {
        Cleanup();
    }

    void InstanceSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC("VegetationInstanceSystemService", 0x823a6007));
    }

    void InstanceSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC("VegetationInstanceSystemService", 0x823a6007));
    }

    void InstanceSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC("VegetationDebugSystemService", 0x8cac3d67));
    }

    void InstanceSystemComponent::Activate()
    {
        Cleanup();
        AZ::TickBus::Handler::BusConnect();
        InstanceSystemRequestBus::Handler::BusConnect();
        InstanceSystemStatsRequestBus::Handler::BusConnect();
        SystemConfigurationRequestBus::Handler::BusConnect();
    }

    void InstanceSystemComponent::Deactivate()
    {
        AZ::TickBus::Handler::BusDisconnect();
        InstanceSystemRequestBus::Handler::BusDisconnect();
        InstanceSystemStatsRequestBus::Handler::BusDisconnect();
        SystemConfigurationRequestBus::Handler::BusDisconnect();
        Cleanup();
    }

    bool InstanceSystemComponent::ReadInConfig(const AZ::ComponentConfig* baseConfig)
    {
        if (const auto config = azrtti_cast<const InstanceSystemConfig*>(baseConfig))
        {
            m_configuration = *config;
            return true;
        }
        return false;
    }

    bool InstanceSystemComponent::WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const
    {
        if (auto config = azrtti_cast<InstanceSystemConfig*>(outBaseConfig))
        {
            *config = m_configuration;
            return true;
        }
        return false;
    }

    DescriptorPtr InstanceSystemComponent::RegisterUniqueDescriptor(const Descriptor& descriptor)
    {
        AZ_PROFILE_FUNCTION(Entity);

        AZStd::lock_guard<decltype(m_uniqueDescriptorsMutex)> lock(m_uniqueDescriptorsMutex);

        AZStd::shared_ptr<InstanceSpawner> equivalentInstanceSpawner = descriptor.GetInstanceSpawner();

        // Loop through all registered unique descriptors to look for the following:
        // 1) Is there an exact match to this descriptor that we can reuse?
        // 2) Is there an exact match to the descriptor's instance spawner that we can reuse?
        for (auto& descPair : m_uniqueDescriptors)
        {
            DescriptorPtr existingDescriptorPtr = descPair.first;

            if (existingDescriptorPtr)
            {
                // If the descriptors and their spawners both match, just reuse and return a
                // pointer to the existing unique descriptor.
                if (*existingDescriptorPtr == descriptor)
                {
                    DescriptorDetails& details = descPair.second;
                    details.m_refCount++;  
                    return existingDescriptorPtr;
                }

                // Keep track of any already-existing instance spawners that match the one in
                // our new descriptor.  If we need to create a new unique descriptor pointer,
                // we will at least try to reuse a instance spawner if it exists.
                if (descriptor.HasEquivalentInstanceSpawners(*existingDescriptorPtr))
                {
                    equivalentInstanceSpawner = existingDescriptorPtr->GetInstanceSpawner();
                }
            }
        }

        // No existing Descriptor was found, so create a new one, but potentially reuse
        // an existing InstanceSpawner if one was found.
        DescriptorPtr createdDescriptorPtr(new Descriptor(descriptor));
        createdDescriptorPtr->SetInstanceSpawner(equivalentInstanceSpawner);

        // Notify the descriptor that it's being registered as a new unique descriptor.
        createdDescriptorPtr->OnRegisterUniqueDescriptor();

        m_uniqueDescriptors[createdDescriptorPtr] = DescriptorDetails();
        return createdDescriptorPtr;
    }

    void InstanceSystemComponent::ReleaseUniqueDescriptor(DescriptorPtr descriptorPtr)
    {
        AZ_PROFILE_FUNCTION(Entity);

        AZStd::lock_guard<decltype(m_uniqueDescriptorsMutex)> lock(m_uniqueDescriptorsMutex);

        auto descItr = m_uniqueDescriptors.find(descriptorPtr);
        if (descItr != m_uniqueDescriptors.end())
        {
            DescriptorPtr existingDescriptorPtr = descItr->first;
            DescriptorDetails& details = descItr->second;

            AZ_Assert(details.m_refCount > 0, "Ref count is already 0!");

            details.m_refCount--;
            if (details.m_refCount <= 0)
            {
                // Notify the descriptor that it's being released as a unique descriptor.
                existingDescriptorPtr->OnReleaseUniqueDescriptor();

                //queue entry for garbage collection
                m_uniqueDescriptorsToDelete[existingDescriptorPtr] = details;
                m_uniqueDescriptors.erase(descItr);
            }
        }
    }

    bool InstanceSystemComponent::IsDescriptorValid(DescriptorPtr descriptorPtr) const
    {
        //only support valid, registered descriptors with loaded meshes
        AZStd::lock_guard<decltype(m_uniqueDescriptorsMutex)> lock(m_uniqueDescriptorsMutex);
        return descriptorPtr && descriptorPtr->IsSpawnable() && m_uniqueDescriptors.find(descriptorPtr) != m_uniqueDescriptors.end();
    }

    void InstanceSystemComponent::GarbageCollectUniqueDescriptors()
    {
        //garbage collect unreferenced descriptors after all other references from all other systems are released
        for (auto descItr = m_uniqueDescriptorsToDelete.begin(); descItr != m_uniqueDescriptorsToDelete.end(); )
        {
            DescriptorPtr descriptorPtr = descItr->first;
            const auto remaining = descriptorPtr.use_count();
            if (remaining == 2) //one for the container and one for the local
            {
                descItr = m_uniqueDescriptorsToDelete.erase(descItr);
                continue;
            }
            ++descItr;
        }
    }

    void InstanceSystemComponent::CreateInstance(InstanceData& instanceData)
    {
        AZ_PROFILE_FUNCTION(Entity);

        if (!IsDescriptorValid(instanceData.m_descriptorPtr))
        {
            //Descriptor and mesh must be valid and registered with the system to proceed but it's not an error
            //an edit, asset change, or other event could have released descriptors or render groups on this or another thread
            //this should result in a composition change and refresh
            instanceData.m_instanceId = InvalidInstanceId;
            return;
        }

        //generate new instance id, from pool if entries exist
        instanceData.m_instanceId = CreateInstanceId();
        if (instanceData.m_instanceId == InvalidInstanceId)
        {
            return;
        }

        // Doing this here risks a slighly inaccurate count if the Create*Node functions fail, but I need this to happen on the vegetation thread so the events are recorded in order.
        VEG_PROFILE_METHOD(DebugNotificationBus::TryQueueBroadcast(&DebugNotificationBus::Events::CreateInstance, instanceData.m_instanceId, instanceData.m_position, instanceData.m_id));

        //queue render node related tasks to process on the main thread
        AddTask([this, instanceData]() {
            CreateInstanceNode(instanceData);
            m_createTaskCount--;
        });

        m_createTaskCount++;
    }

    void InstanceSystemComponent::DestroyInstance(InstanceId instanceId)
    {
        AZ_PROFILE_FUNCTION(Entity);

        if (instanceId == InvalidInstanceId)
        {
            return;
        }

        // do this here so we retain a correct ordering of events based on the vegetation thread.
        VEG_PROFILE_METHOD(DebugNotificationBus::TryQueueBroadcast(&DebugNotificationBus::Events::DeleteInstance, instanceId));

        //queue render node related tasks to process on the main thread
        AddTask([this, instanceId]() {
            ReleaseInstanceNode(instanceId);

            AZStd::lock_guard<decltype(m_instanceDeletionSetMutex)> instanceDeletionSet(m_instanceDeletionSetMutex);
            m_instanceDeletionSet.erase(instanceId);
            m_destroyTaskCount--;
        });

        AZStd::lock_guard<decltype(m_instanceDeletionSetMutex)> instanceDeletionSet(m_instanceDeletionSetMutex);
        m_instanceDeletionSet.insert(instanceId);
        m_destroyTaskCount++;
    }

    void InstanceSystemComponent::DestroyAllInstances()
    {
        VEG_PROFILE_METHOD(DebugNotificationBus::TryQueueBroadcast(&DebugNotificationBus::Events::DeleteAllInstances));

        // make sure to clear out the instance work queue
        ClearTasks();

        // clear all instances
        {
            AZStd::lock_guard<decltype(m_instanceMapMutex)> scopedLock(m_instanceMapMutex);
            for (auto instancePair : m_instanceMap)
            {
                InstanceId instanceId = instancePair.first;
                DescriptorPtr descriptor = instancePair.second.first;
                InstancePtr opaqueInstanceData = instancePair.second.second;
                if (opaqueInstanceData)
                {
                    descriptor->DestroyInstance(instanceId, opaqueInstanceData);
                }
                ReleaseInstanceId(instanceId);
            }
            m_instanceMap.clear();
            m_instanceCount = 0;
        }

        {
            AZStd::lock_guard<decltype(m_instanceDeletionSetMutex)> instanceDeletionSet(m_instanceDeletionSetMutex);
            m_instanceDeletionSet.clear();
            m_destroyTaskCount = 0;
        }
    }

    void InstanceSystemComponent::Cleanup()
    {
        DestroyAllInstances();

        {
            AZStd::lock_guard<decltype(m_uniqueDescriptorsMutex)> lock(m_uniqueDescriptorsMutex);
            m_uniqueDescriptors.clear();
            m_uniqueDescriptorsToDelete.clear();
        }
    }

    AZ::u32 InstanceSystemComponent::GetInstanceCount() const
    {
        return m_instanceCount;
    }

    AZ::u32 InstanceSystemComponent::GetTotalTaskCount() const
    {
        return m_createTaskCount + m_destroyTaskCount;
    }

    AZ::u32 InstanceSystemComponent::GetCreateTaskCount() const
    {
        return m_createTaskCount;
    }

    AZ::u32 InstanceSystemComponent::GetDestroyTaskCount() const
    {
        return m_destroyTaskCount;
    }

    void InstanceSystemComponent::OnTick([[maybe_unused]] float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint time)
    {
        if (HasTasks())
        {
            ProcessMainThreadTasks();
        }

        GarbageCollectUniqueDescriptors();
    }

    void InstanceSystemComponent::UpdateSystemConfig(const AZ::ComponentConfig* baseConfig)
    {
        ReadInConfig(baseConfig);
    }

    void InstanceSystemComponent::GetSystemConfig(AZ::ComponentConfig* outBaseConfig) const
    {
        WriteOutConfig(outBaseConfig);
    }

    InstanceId InstanceSystemComponent::CreateInstanceId()
    {
        AZStd::lock_guard<decltype(m_instanceIdMutex)> scopedLock(m_instanceIdMutex);

        //recycle a previously used id from the pool/free-list before generating a new one
        if (!m_instanceIdPool.empty())
        {
            auto instanceIdItr = m_instanceIdPool.begin();
            InstanceId instanceId = *instanceIdItr;
            m_instanceIdPool.erase(instanceIdItr);
            return instanceId;
        }

        //if all ids have been used, no more can be created until the counter is reset
        if (m_instanceIdCounter >= MaxInstanceId)
        {
            AZ_Error("vegetation", false, "MaxInstanceId reached! No more instance ids can be created until some are released!");
            return InvalidInstanceId;
        }

        return m_instanceIdCounter++;
    }

    void InstanceSystemComponent::ReleaseInstanceId(InstanceId instanceId)
    {
        AZStd::lock_guard<decltype(m_instanceIdMutex)> scopedLock(m_instanceIdMutex);

        //add released ids to the free list for recycling
        m_instanceIdPool.insert(instanceId);
    }

    bool InstanceSystemComponent::IsInstanceSkippable(const InstanceData& instanceData) const
    {
        AZ_PROFILE_FUNCTION(Entity);

        //if the instance was queued for deletion before its creation task executed then skip it
        AZStd::lock_guard<decltype(m_instanceDeletionSetMutex)> instanceDeletionSet(m_instanceDeletionSetMutex);
        return instanceData.m_instanceId == InvalidInstanceId || m_instanceDeletionSet.find(instanceData.m_instanceId) != m_instanceDeletionSet.end();
    }

    void InstanceSystemComponent::CreateInstanceNode(const InstanceData& instanceData)
    {
        AZ_PROFILE_FUNCTION(Entity);

        if (IsInstanceSkippable(instanceData))
        {
            return;
        }

        // Only support valid, registered descriptors with loaded assets
        if (!instanceData.m_descriptorPtr || !instanceData.m_descriptorPtr->IsLoaded())
        {
            //descriptor and mesh must be valid but it's not an error
            //an edit, asset change, or other event could have released descriptors or render groups on this or another thread
            //this should result in a composition change and refresh
            return;
        }

        {
            AZStd::lock_guard<decltype(m_uniqueDescriptorsMutex)> lock(m_uniqueDescriptorsMutex);

            auto descItr = m_uniqueDescriptors.find(instanceData.m_descriptorPtr);
            if (descItr == m_uniqueDescriptors.end())
            {
                //descriptor must be registered with the system to create an instance.
                //it could have been removed or re-added while editing or deleting entities that control the registration
                return;
            }
        }

        InstancePtr opaqueInstanceData = instanceData.m_descriptorPtr->CreateInstance(instanceData);

        if (opaqueInstanceData)
        {
            AZStd::lock_guard<decltype(m_instanceMapMutex)> scopedLock(m_instanceMapMutex);
            AZ_Assert(m_instanceMap.find(instanceData.m_instanceId) == m_instanceMap.end(), "InstanceId %llu is already in use!", instanceData.m_instanceId);
            m_instanceMap[instanceData.m_instanceId] = AZStd::make_pair(instanceData.m_descriptorPtr, opaqueInstanceData);
            m_instanceCount = static_cast<int>(m_instanceMap.size());
        }
    }

    void InstanceSystemComponent::ReleaseInstanceNode(InstanceId instanceId)
    {
        AZ_PROFILE_FUNCTION(Entity);

        DescriptorPtr descriptor = nullptr;
        InstancePtr opaqueInstanceData = nullptr;

        {
            AZStd::lock_guard<decltype(m_instanceMapMutex)> scopedLock(m_instanceMapMutex);
            auto instanceItr = m_instanceMap.find(instanceId);
            if (instanceItr != m_instanceMap.end())
            {
                descriptor = instanceItr->second.first;
                opaqueInstanceData = instanceItr->second.second;
                m_instanceMap.erase(instanceItr);
            }
            m_instanceCount = static_cast<int>(m_instanceMap.size());
        }

        if (opaqueInstanceData)
        {
            descriptor->DestroyInstance(instanceId, opaqueInstanceData);
        }
        ReleaseInstanceId(instanceId);
    }

    bool InstanceSystemComponent::HasTasks() const
    {
        AZStd::lock_guard<decltype(m_mainThreadTaskMutex)> mainThreadTaskLock(m_mainThreadTaskMutex);
        return !m_mainThreadTaskQueue.empty();
    }

    void InstanceSystemComponent::AddTask(const Task& task)
    {
        AZ_PROFILE_FUNCTION(Entity);

        AZStd::lock_guard<decltype(m_mainThreadTaskMutex)> mainThreadTaskLock(m_mainThreadTaskMutex);
        if (m_mainThreadTaskQueue.empty() || m_mainThreadTaskQueue.back().size() >= m_configuration.m_maxInstanceTaskBatchSize)
        {
            m_mainThreadTaskQueue.push_back();
            m_mainThreadTaskQueue.back().reserve(m_configuration.m_maxInstanceTaskBatchSize);
        }
        m_mainThreadTaskQueue.back().emplace_back(task);
    }

    void InstanceSystemComponent::ClearTasks()
    {
        AZ_PROFILE_FUNCTION(Entity);

        AZStd::lock_guard<decltype(m_mainThreadTaskInProgressMutex)> mainThreadTaskInProgressLock(m_mainThreadTaskInProgressMutex);
        AZStd::lock_guard<decltype(m_mainThreadTaskMutex)> mainThreadTaskLock(m_mainThreadTaskMutex);
        m_mainThreadTaskQueue.clear();

        m_createTaskCount = 0;
        m_destroyTaskCount = 0;
    }

    bool InstanceSystemComponent::GetTasks(TaskList& removedTasks)
    {
        AZ_PROFILE_FUNCTION(Entity);

        AZStd::lock_guard<decltype(m_mainThreadTaskMutex)> mainThreadTaskLock(m_mainThreadTaskMutex);
        if (!m_mainThreadTaskQueue.empty())
        {
            removedTasks.splice(removedTasks.end(), m_mainThreadTaskQueue, m_mainThreadTaskQueue.begin());
            return true;
        }
        return false;
    }

    void InstanceSystemComponent::ExecuteTasks()
    {
        AZ_PROFILE_FUNCTION(Entity);

        AZStd::lock_guard<decltype(m_mainThreadTaskInProgressMutex)> scopedLock(m_mainThreadTaskInProgressMutex);

        AZStd::chrono::system_clock::time_point initialTime = AZStd::chrono::system_clock::now();
        AZStd::chrono::system_clock::time_point currentTime = initialTime;

        auto removedTasksPtr = AZStd::make_shared<TaskList>();
        while (GetTasks(*removedTasksPtr))
        {
            for (const auto& task : (*removedTasksPtr).back())
            {
                task();
            }

            currentTime = AZStd::chrono::system_clock::now();
            if (AZStd::chrono::microseconds(currentTime - initialTime).count() > m_configuration.m_maxInstanceProcessTimeMicroseconds)
            {
                break;
            }
        }

        //offloading garbage collection to job to save time deallocating tasks on main thread
        auto garbageCollectionJob = AZ::CreateJobFunction([]() mutable {}, true);
        garbageCollectionJob->Start();
    }

    void InstanceSystemComponent::ProcessMainThreadTasks()
    {
        AZ_PROFILE_FUNCTION(Entity);

        ExecuteTasks();
    }

}
