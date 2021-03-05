/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/containers/list.h>
#include <AzCore/std/function/function_fwd.h>

#include <Vegetation/Descriptor.h>
#include <Vegetation/InstanceData.h>
#include <Vegetation/Ebuses/InstanceSystemRequestBus.h>
#include <Vegetation/Ebuses/SystemConfigurationBus.h>

#include <StatObjBus.h>
#include <CrySystemBus.h>

namespace AZ
{
    class Aabb;
    class Vector3;
    class Transform;
    class EntityId;
}

struct IRenderNode;
struct ISystem;
struct I3DEngine;

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
        AZ_CLASS_ALLOCATOR(InstanceSystemConfig, AZ::SystemAllocator, 0);
        AZ_RTTI(InstanceSystemConfig, "{63984856-F883-4F8C-9049-5A8F26477B76}", AZ::ComponentConfig);
        static void Reflect(AZ::ReflectContext* context);

        // the timeout maximum number of microseconds to process the vegetation instance construction & removal operations
        int m_maxInstanceProcessTimeMicroseconds = 500;

        // maximum number of instance management tasks that can be batch processed together
        int m_maxInstanceTaskBatchSize = 100;

        // merged mesh visual features
        float m_mergedMeshesViewDistanceRatio = 100.0f;
        float m_mergedMeshesLodRatio = 3.0f;
        float m_mergedMeshesInstanceDistance = 4.5f;
    };

    /**
    * Manages vegetation types and instances via the InstanceSystemRequestBus
    */
    class InstanceSystemComponent
        : public AZ::Component
        , private InstanceSystemRequestBus::Handler
        , private InstanceSystemStatsRequestBus::Handler
        , private AZ::TickBus::Handler
        , private InstanceStatObjEventBus::Handler
        , private SystemConfigurationRequestBus::Handler
        , private CrySystemEventBus::Handler
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

        void RegisterMergedMeshInstance(InstancePtr instance, IRenderNode* mergedMeshNode) override;
        void ReleaseMergedMeshInstance(InstancePtr instance) override;

        // InstanceSystemStatsRequestBus
        AZ::u32 GetInstanceCount() const override;
        AZ::u32 GetTotalTaskCount() const override;
        AZ::u32 GetCreateTaskCount() const override;
        AZ::u32 GetDestroyTaskCount() const override;

        // AZ::TickBus
        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;

        // InstanceStatObjEventBus
        void ReleaseData() override;

        //////////////////////////////////////////////////////////////////
        // SystemConfigurationRequestBus
        void UpdateSystemConfig(const AZ::ComponentConfig* config) override;
        void GetSystemConfig(AZ::ComponentConfig* config) const override;

        ////////////////////////////////////////////////////////////////////////////
        // CrySystemEvents
        void OnCrySystemInitialized(ISystem& system, const SSystemInitParams& systemInitParams) override;
        void OnCrySystemShutdown(ISystem& system) override;

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

        void CreateInstanceNodeBegin();
        void CreateInstanceNodeEnd();

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

        //refresh events can queue the creation and deletion of the same node in the same frame
        //this map is used to track which nodes remain after all tasks have executed for the frame
        //registration will only be done on the final set each frame
        AZStd::unordered_map<InstancePtr, IRenderNode*> m_instanceNodeToMergedMeshNodeRegistrationMap;
        AZStd::unordered_set<IRenderNode*> m_mergedMeshNodeRegistrationSet;

        ISystem* m_system = nullptr;
        I3DEngine* m_engine = nullptr;

        AZStd::atomic_int m_instanceCount{ 0 };
        AZStd::atomic_int m_createTaskCount{ 0 };
        AZStd::atomic_int m_destroyTaskCount{ 0 };
    };
} // namespace Vegetation
