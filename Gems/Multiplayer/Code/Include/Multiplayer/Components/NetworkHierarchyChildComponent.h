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
#include <Source/AutoGen/NetworkHierarchyChildComponent.AutoComponent.h>

namespace Multiplayer
{
    class NetworkHierarchyRootComponent;

    //! @class NetworkHierarchyChildComponent
    //! @brief Component that declares network dependency on the parent of this entity
    /*
     * The parent of this entity should have @NetworkHierarchyChildComponent (or @NetworkHierarchyRootComponent).
     * A network hierarchy is a collection of entities with one @NetworkHierarchyRootComponent at the top parent
     * and one or more @NetworkHierarchyChildComponent on its child entities.
     */
    class NetworkHierarchyChildComponent final
        : public NetworkHierarchyChildComponentBase
        , public NetworkHierarchyRequestBus::Handler
    {
        friend class NetworkHierarchyRootComponent;

    public:
        AZ_MULTIPLAYER_COMPONENT(Multiplayer::NetworkHierarchyChildComponent, s_networkHierarchyChildComponentConcreteUuid, Multiplayer::NetworkHierarchyChildComponentBase);

        static void Reflect(AZ::ReflectContext* context);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);

        NetworkHierarchyChildComponent();

        //! NetworkHierarchyChildComponentBase overrides.
        //! @{
        void OnInit() override;
        void OnActivate(EntityIsMigrating entityIsMigrating) override;
        void OnDeactivate(EntityIsMigrating entityIsMigrating) override;
        //! @}

        //! NetworkHierarchyRequestBus overrides.
        //! @{
        bool IsHierarchyEnabled() const override;
        bool IsHierarchicalChild() const override;
        bool IsHierarchicalRoot() const override { return false; }
        AZ::Entity* GetHierarchicalRoot() const override;
        AZStd::vector<AZ::Entity*> GetHierarchicalEntities() const override;
        void BindNetworkHierarchyChangedEventHandler(NetworkHierarchyChangedEvent::Handler& handler) override;
        void BindNetworkHierarchyLeaveEventHandler(NetworkHierarchyLeaveEvent::Handler& handler) override;
        //! @}

    private:
        //! Used by @NetworkHierarchyRootComponent
        void SetTopLevelHierarchyRootEntity(AZ::Entity* previousHierarchyRoot, AZ::Entity* newHierarchyRoot);

        AZ::ChildChangedEvent::Handler m_childChangedHandler;

        void OnChildChanged(AZ::ChildChangeType type, AZ::EntityId child);

        //! Points to the top level root.
        AZ::Entity* m_rootEntity = nullptr;

        AZ::Event<NetEntityId>::Handler m_hierarchyRootNetIdChanged;
        void OnHierarchyRootNetIdChanged(NetEntityId rootNetId);

        NetworkHierarchyChangedEvent m_networkHierarchyChangedEvent;
        NetworkHierarchyLeaveEvent m_networkHierarchyLeaveEvent;

        //! Set to false when deactivating or otherwise not to be included in hierarchy considerations.
        bool m_isHierarchyEnabled = true;

        void NotifyChildrenHierarchyDisbanded();

        AzNetworking::ConnectionId m_previousOwningConnectionId = AzNetworking::InvalidConnectionId;
        void SetOwningConnectionId(AzNetworking::ConnectionId connectionId) override;
    };
}
