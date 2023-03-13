/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/Entity.h>
#include <AzCore/Component/Component.h>
#include <AzCore/Component/ComponentBus.h>
#include <AzCore/Component/TickBus.h>

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/EBus/EBus.h>
#include <AzCore/Math/Aabb.h>
#include <AzCore/std/containers/list.h>
#include <AzCore/std/containers/map.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/function/function_fwd.h>

#include <Vegetation/Descriptor.h>
#include <Vegetation/InstanceData.h>
#include <Vegetation/Ebuses/InstanceSystemRequestBus.h>
#include <Vegetation/Ebuses/SystemConfigurationBus.h>

namespace AZ
{
    class Aabb;
    class Vector3;
    class Transform;
    class EntityId;
}

//////////////////////////////////////////////////////////////////////////

namespace Vegetation
{
    /**
    * The configuration for the vegetation instance manager
    */
    class InstanceSystemConfig
        : public AZ::ComponentConfig
    {
    public:
        AZ_CLASS_ALLOCATOR(InstanceSystemConfig, AZ::SystemAllocator);
        AZ_RTTI(InstanceSystemConfig, "{63984856-F883-4F8C-9049-5A8F26477B76}", AZ::ComponentConfig);
        static void Reflect(AZ::ReflectContext* context);

        // the timeout maximum number of microseconds to process the vegetation instance construction & removal operations
        int m_maxInstanceProcessTimeMicroseconds = 500;

        // maximum number of instance management tasks that can be batch processed together
        int m_maxInstanceTaskBatchSize = 100;
    };

    /**
    * Manages vegetation types and instances via the InstanceSystemRequestBus
    */
    class InstanceSystemComponent
        : public AZ::Component
        , private InstanceSystemRequestBus::Handler
        , private InstanceSystemStatsRequestBus::Handler
        , private AZ::TickBus::Handler
        , private SystemConfigurationRequestBus::Handler
    {
        friend class EditorInstanceSystemComponent;

    public:
        InstanceSystemComponent(const InstanceSystemConfig&);
        InstanceSystemComponent() = default;
        virtual ~InstanceSystemComponent();

        // Component static
        AZ_COMPONENT(InstanceSystemComponent, "{BCBD2475-A38B-40D3-9477-0D6CA723F874}", AZ::Component);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void Reflect(AZ::ReflectContext* context);

    private:
        InstanceSystemConfig m_configuration;

        // Component
        void Activate() override;
        void Deactivate() override;
        bool ReadInConfig(const AZ::ComponentConfig* baseConfig) override;
        bool WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const override;

        // InstanceSystemRequestBus
        DescriptorPtr RegisterUniqueDescriptor(const Descriptor& descriptor) override;
        void ReleaseUniqueDescriptor(DescriptorPtr descriptorPtr) override;
        bool IsDescriptorValid(DescriptorPtr descriptorPtr) const;
        void GarbageCollectUniqueDescriptors();

        void CreateInstance(InstanceData& instanceData) override;
        void DestroyInstance(InstanceId instanceId) override;
        void DestroyAllInstances() override;
        void Cleanup() override;

        // InstanceSystemStatsRequestBus
        AZ::u32 GetInstanceCount() const override;
        AZ::u32 GetTotalTaskCount() const override;
        AZ::u32 GetCreateTaskCount() const override;
        AZ::u32 GetDestroyTaskCount() const override;

        // AZ::TickBus
        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;

        //////////////////////////////////////////////////////////////////
        // SystemConfigurationRequestBus
        void UpdateSystemConfig(const AZ::ComponentConfig* config) override;
        void GetSystemConfig(AZ::ComponentConfig* config) const override;

        ////////////////////////////////////////////////////////////////
        // vegetation instance id management
        InstanceId CreateInstanceId();
        void ReleaseInstanceId(InstanceId instanceId);

        mutable AZStd::recursive_mutex m_instanceIdMutex;
        InstanceId m_instanceIdCounter = 0;
        AZStd::unordered_set<InstanceId> m_instanceIdPool;

        ////////////////////////////////////////////////////////////////
        // vegetation instance management
        bool IsInstanceSkippable(const InstanceData& instanceData) const;
        void CreateInstanceNode(const InstanceData& instanceData);

        void ReleaseInstanceNode(InstanceId instanceId);

        mutable AZStd::recursive_mutex m_instanceMapMutex;
        AZStd::unordered_map<InstanceId, AZStd::pair<DescriptorPtr, InstancePtr>> m_instanceMap;

        mutable AZStd::recursive_mutex m_instanceDeletionSetMutex;
        AZStd::unordered_set<InstanceId> m_instanceDeletionSet;

        ////////////////////////////////////////////////////////////////
        // Task management
        using Task = AZStd::function<void(void)>;
        using TaskBatch = AZStd::vector<Task>;
        using TaskList = AZStd::list<TaskBatch>;
        TaskList m_mainThreadTaskQueue;
        mutable AZStd::recursive_mutex m_mainThreadTaskMutex;
        mutable AZStd::recursive_mutex m_mainThreadTaskInProgressMutex;

        bool HasTasks() const;
        void AddTask(const Task& task);
        void ClearTasks();
        bool GetTasks(TaskList& removedTasks);
        void ExecuteTasks();
        void ProcessMainThreadTasks();

        ////////////////////////////////////////////////////////////////
        // vegetation descriptor management
        struct DescriptorDetails
        {
            int m_refCount = 1;
        };
        mutable AZStd::recursive_mutex m_uniqueDescriptorsMutex;
        AZStd::map<DescriptorPtr, DescriptorDetails> m_uniqueDescriptors;
        AZStd::map<DescriptorPtr, DescriptorDetails> m_uniqueDescriptorsToDelete;

        AZStd::atomic_int m_instanceCount{ 0 };
        AZStd::atomic_int m_createTaskCount{ 0 };
        AZStd::atomic_int m_destroyTaskCount{ 0 };
    };
} // namespace Vegetation
