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

#include <ScriptCanvas/Core/ScriptCanvasBus.h>
#include <ScriptCanvas/Variable/VariableCore.h>
#include <AzCore/Component/Component.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/parallel/lock.h>
#include <AzCore/std/parallel/mutex.h>

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
        , protected SystemRequestBus::Handler
        , protected AZ::BehaviorContextBus::Handler
    {
    public:
        AZ_COMPONENT(SystemComponent, "{CCCCE7AE-AEC7-43F8-969C-ED592C264560}");

        SystemComponent() = default;

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        // AZ::Component interface implementation
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        ////
                 
        void AddOwnedObjectReference(const void* object, BehaviorContextObject* behaviorContextObject) override;
        BehaviorContextObject* FindOwnedObjectReference(const void* object) override;
        void RemoveOwnedObjectReference(const void* object) override;
        
    protected:
        // SystemRequestBus::Handler
        void CreateEngineComponentsOnEntity(AZ::Entity* entity) override;
        Graph* CreateGraphOnEntity(AZ::Entity* entity) override;
        ScriptCanvas::Graph* MakeGraph() override;
        ScriptCanvasId FindScriptCanvasId(AZ::Entity* graphEntity) override;
        ScriptCanvas::Node* GetNode(const AZ::EntityId&, const AZ::Uuid&) override;
        ScriptCanvas::Node* CreateNodeOnEntity(const AZ::EntityId& entityId, ScriptCanvasId scriptCanvasId, const AZ::Uuid& nodeType) override;
        SystemComponentConfiguration GetSystemComponentConfiguration()
        {
            SystemComponentConfiguration configuration;
            configuration.m_maxIterationsForInfiniteLoopDetection = m_infiniteLoopDetectionMaxIterations;
            return configuration;
        }
        ////

        // BehaviorEventBus::Handler
        void OnAddClass(const char* className, AZ::BehaviorClass* behaviorClass) override;
        void OnRemoveClass(const char* className, AZ::BehaviorClass* behaviorClass) override;
        ////

    private:
        void RegisterCreatableTypes();
        
        // excruciatingly meticulous ScriptCanvas memory tracking
        using MutexType = AZStd::recursive_mutex;
        using LockType = AZStd::lock_guard<MutexType>;
        AZStd::unordered_map<const void*, BehaviorContextObject*> m_ownedObjectsByAddress;
        MutexType m_ownedObjectsByAddressMutex;

        int m_infiniteLoopDetectionMaxIterations = 3000;

    };
}
