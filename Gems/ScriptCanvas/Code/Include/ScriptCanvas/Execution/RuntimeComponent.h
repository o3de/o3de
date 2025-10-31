/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Component/EntityBus.h>

#include <ScriptCanvas/Asset/RuntimeAsset.h>
#include <ScriptCanvas/Execution/Executor.h>

namespace ScriptCanvas
{
    class RuntimeComponent;

    struct RuntimeComponentUserData
    {
        AZ_TYPE_INFO(RuntimeComponentUserData, "{584AC6C4-0A75-49DE-93A1-1B81E58F878E}");
        AZ_CLASS_ALLOCATOR(RuntimeComponentUserData, AZ::SystemAllocator);

        RuntimeComponent& component;
        const AZ::EntityId entity;

        RuntimeComponentUserData(RuntimeComponent& component, const AZ::EntityId& entity);
    };

    //! Runtime Component responsible for loading and executing the compiled ScriptCanvas graph from a runtime asset.
    //! It connects the execution of the graph to the EntityBus and the Entity/Component framework
    //! Component Activate: Connect to EntityBus, initialize runtime graph
    //! OnEntityActivated: Begin (and optionally complete) runtime graph execution
    //! OnEntityDeactivated or RuntimeComponent::Destruction: (optionally) halt runtime graph execution
    class RuntimeComponent
        : public AZ::Component
        , public AZ::EntityBus::Handler
    {
    public:
        AZ_COMPONENT(RuntimeComponent, "{95BFD916-E832-4956-837D-525DE8384282}", AZ::Component);

        static void Reflect(AZ::ReflectContext* context);

        RuntimeComponent() = default;

        void TakeRuntimeDataOverrides(RuntimeDataOverrides&& overrideData);
        const RuntimeDataOverrides& GetRuntimeDataOverrides() const;

    protected:
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC_CE("ScriptCanvasRuntimeService"));
        }

        void Activate() override;

        void Deactivate() override {}

        void OnEntityActivated(const AZ::EntityId& entityId) override;

        void OnEntityDeactivated(const AZ::EntityId&) override;

    private:
        Executor m_executor;
    };
}
