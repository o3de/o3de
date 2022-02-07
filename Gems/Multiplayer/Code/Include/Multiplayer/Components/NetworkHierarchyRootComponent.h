/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Component/TransformBus.h>
#include <Multiplayer/Components/NetworkHierarchyBus.h>
#include <Source/AutoGen/NetworkHierarchyRootComponent.AutoComponent.h>

namespace Multiplayer
{
    //! @class NetworkHierarchyRootComponent
    //! @brief Component that declares the top level entity of a network hierarchy.
    /*
     * Call @GetHierarchicalEntities to get the list of hierarchical entities.
     * A network hierarchy is meant to be a small group of entities. You can control the maximum supported size of
     * a network hierarchy by modifying CVar @bg_hierarchyEntityMaxLimit.
     *
     * A root component marks either a top most root of a hierarchy, or an inner root of an attach hierarchy.
     */
    class NetworkHierarchyRootComponent final
        : public NetworkHierarchyRootComponentBase
        , public NetworkHierarchyRequestBus::Handler
    {
        friend class NetworkHierarchyChildComponent;
        friend class NetworkHierarchyRootComponentController;
    public:
        AZ_MULTIPLAYER_COMPONENT(Multiplayer::NetworkHierarchyRootComponent, s_networkHierarchyRootComponentConcreteUuid, Multiplayer::NetworkHierarchyRootComponentBase);

        static void Reflect(AZ::ReflectContext* context);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);

        NetworkHierarchyRootComponent();

        //! NetworkHierarchyRootComponentBase overrides.
        //! @{
        void OnInit() override;
        void OnActivate(Multiplayer::EntityIsMigrating entityIsMigrating) override;
        void OnDeactivate(Multiplayer::EntityIsMigrating entityIsMigrating) override;
        //! @}

        //! NetworkHierarchyRequestBus overrides.
        //! @{
        bool IsHierarchyEnabled() const override;
        bool IsHierarchicalRoot() const override;
        bool IsHierarchicalChild() const override;
        AZStd::vector<AZ::Entity*> GetHierarchicalEntities() const override;
        AZ::Entity* GetHierarchicalRoot() const override;
        void BindNetworkHierarchyChangedEventHandler(NetworkHierarchyChangedEvent::Handler& handler) override;
        void BindNetworkHierarchyLeaveEventHandler(NetworkHierarchyLeaveEvent::Handler& handler) override;
        //! @}

        bool SerializeEntityCorrection(AzNetworking::ISerializer& serializer);

    private:
        void SetTopLevelHierarchyRootEntity(AZ::Entity* previousHierarchyRoot, AZ::Entity* newHierarchyRoot);

        AZ::ChildChangedEvent::Handler m_childChangedHandler;
        AZ::ParentChangedEvent::Handler m_parentChangedHandler;

        void OnChildChanged(AZ::ChildChangeType type, AZ::EntityId child);
        void OnParentChanged(AZ::EntityId oldParent, AZ::EntityId parent);

        NetworkHierarchyChangedEvent m_networkHierarchyChangedEvent;
        NetworkHierarchyLeaveEvent m_networkHierarchyLeaveEvent;

        //! Points to the top level root, if this root is an inner root in this hierarchy.
        AZ::Entity* m_rootEntity = nullptr;

        AZStd::vector<AZ::Entity*> m_hierarchicalEntities;

        //! Rebuilds hierarchy starting from this root component's entity.
        void RebuildHierarchy();

        //! @param underEntity Walk the child entities that belong to @underEntity and consider adding them to the hierarchy.
        //! Builds the hierarchy using breadth-first iterative method.
        void InternalBuildHierarchyList(AZ::Entity* underEntity);

        void SetRootForEntity(AZ::Entity* previousKnownRoot, AZ::Entity* newRoot, const AZ::Entity* childEntity);

        //! Set to false when deactivating or otherwise not to be included in hierarchy considerations.
        bool m_isHierarchyEnabled = true;

        AzNetworking::ConnectionId m_previousOwningConnectionId = AzNetworking::InvalidConnectionId;
        void SetOwningConnectionId(AzNetworking::ConnectionId connectionId) override;

        friend class HierarchyBenchmarkBase;
    };


    //! NetworkHierarchyRootComponentController
    //! This is the network controller for NetworkHierarchyRootComponent.
    //! Class provides the ability to process input for hierarchies.
    class NetworkHierarchyRootComponentController final
        : public NetworkHierarchyRootComponentControllerBase
    {
    public:
        NetworkHierarchyRootComponentController(NetworkHierarchyRootComponent& parent);

        // NetworkHierarchyRootComponentControllerBase
        void OnActivate(Multiplayer::EntityIsMigrating entityIsMigrating) override;
        void OnDeactivate(Multiplayer::EntityIsMigrating entityIsMigrating) override;

        //! MultiplayerController interface
        Multiplayer::MultiplayerController::InputPriorityOrder GetInputOrder() const override;
        void CreateInput(Multiplayer::NetworkInput& input, float deltaTime) override;
        void ProcessInput(Multiplayer::NetworkInput& input, float deltaTime) override;
    };
}
