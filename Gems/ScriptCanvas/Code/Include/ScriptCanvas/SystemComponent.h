/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once


#include <AzCore/Component/Component.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/parallel/lock.h>
#include <AzCore/std/parallel/mutex.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <ScriptCanvas/Core/ScriptCanvasBus.h>
#include <ScriptCanvas/Variable/VariableCore.h>
#include <ScriptCanvas/PerformanceTracker.h>
#include <ScriptCanvas/Data/DataRegistry.h>

namespace AZ
{
    namespace Data
    {
        class AssetHandler;
    }
}

namespace ScriptCanvas
{
    class SystemComponent
        : public AZ::Component
        // , public AZ::Interface<Execution::PerformanceTracker>::Registrar -- this class is double constructed, so Interface can't be used, yet. workaround below
        , protected SystemRequestBus::Handler
        , protected AZ::BehaviorContextBus::Handler
    {
    public:
        AZ_COMPONENT(SystemComponent, "{CCCCE7AE-AEC7-43F8-969C-ED592C264560}");

        static Execution::PerformanceTracker* ModPerformanceTracker();

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        // AZ::Component...
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        ////
                 
        void AddOwnedObjectReference(const void* object, BehaviorContextObject* behaviorContextObject) override;
        BehaviorContextObject* FindOwnedObjectReference(const void* object) override;
        void RemoveOwnedObjectReference(const void* object) override;
        
    protected:

        inline bool IsAnyScriptInterpreted() const { return true; }

        AZStd::pair<DataRegistry::Createability, TypeProperties> GetCreatibility(AZ::SerializeContext* serializeContext, AZ::BehaviorClass* behaviorClass);

        // SystemRequestBus::Handler...
        bool IsScriptUnitTestingInProgress() override;
        void MarkScriptUnitTestBegin() override;
        void MarkScriptUnitTestEnd() override;
        void CreateEngineComponentsOnEntity(AZ::Entity* entity) override;
        Graph* CreateGraphOnEntity(AZ::Entity* entity) override;
        ScriptCanvas::Graph* MakeGraph() override;
        ScriptCanvasId FindScriptCanvasId(AZ::Entity* graphEntity) override;
        ScriptCanvas::Node* GetNode(const AZ::EntityId&, const AZ::Uuid&) override;
        ScriptCanvas::Node* CreateNodeOnEntity(const AZ::EntityId& entityId, ScriptCanvasId scriptCanvasId, const AZ::Uuid& nodeType) override;
        SystemComponentConfiguration GetSystemComponentConfiguration() override
        {
            SystemComponentConfiguration configuration;
            configuration.m_maxIterationsForInfiniteLoopDetection = m_infiniteLoopDetectionMaxIterations;
            configuration.m_maxHandlerStackDepth = m_maxHandlerStackDepth;
            return configuration;
        }

        void SetInterpretedBuildConfiguration(BuildConfiguration config) override;
        ////

        // BehaviorEventBus::Handler...
        void OnAddClass(const char* className, AZ::BehaviorClass* behaviorClass) override;
        void OnRemoveClass(const char* className, AZ::BehaviorClass* behaviorClass) override;
        ////

    private:
        void RegisterCreatableTypes();

        bool m_scriptBasedUnitTestingInProgress = false;

        using MutexType = AZStd::recursive_mutex;
        using LockType = AZStd::lock_guard<MutexType>;
        AZStd::unordered_map<const void*, BehaviorContextObject*> m_ownedObjectsByAddress;
        MutexType m_ownedObjectsByAddressMutex;
        int m_infiniteLoopDetectionMaxIterations = 1000000;
        int m_maxHandlerStackDepth = 50;

        static void SafeRegisterPerformanceTracker();
        static void SafeUnregisterPerformanceTracker();

        Execution::PerformanceTracker* m_registeredTracker = nullptr;
        static AZ::EnvironmentVariable<Execution::PerformanceTracker*> s_perfTracker;
        static AZStd::shared_mutex s_perfTrackerMutex;
        static constexpr const char* s_trackerName = "ScriptCanvasPerformanceTracker";
    };
}
