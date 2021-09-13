/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
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
        void OnActivate(Multiplayer::EntityIsMigrating entityIsMigrating) override;
        void OnDeactivate(Multiplayer::EntityIsMigrating entityIsMigrating) override;
        //! @}

        //! NetworkHierarchyRequestBus overrides.
        //! @{
        bool IsHierarchicalChild() const override;
        bool IsHierarchicalRoot() const override { return false; }
        AZ::Entity* GetHierarchicalRoot() const override;
        AZStd::vector<AZ::Entity*> GetHierarchicalEntities() const override;
        //! @}

    protected:
        //! Used by @NetworkHierarchyRootComponent
        void SetTopLevelHierarchyRootComponent(NetworkHierarchyRootComponent* hierarchyRoot);

    private:
        const NetworkHierarchyRootComponent* m_hierarchyRootComponent = nullptr;

        AZ::Event<NetEntityId>::Handler m_hierarchyRootNetIdChanged;
        void OnHierarchyRootNetIdChanged(NetEntityId rootNetId);
    };
}
