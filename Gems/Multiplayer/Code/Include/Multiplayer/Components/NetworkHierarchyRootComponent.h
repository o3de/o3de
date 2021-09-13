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
#include <Source/AutoGen/NetworkHierarchyRootComponent.AutoComponent.h>
#include <Multiplayer/Components/NetworkHierarchyBus.h>

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
        , protected AZ::TransformNotificationBus::MultiHandler
    {
    public:
        AZ_MULTIPLAYER_COMPONENT(Multiplayer::NetworkHierarchyRootComponent, s_networkHierarchyRootComponentConcreteUuid, Multiplayer::NetworkHierarchyRootComponentBase);

        static void Reflect(AZ::ReflectContext* context);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);

        //! NetworkHierarchyRootComponentBase overrides.
        //! @{
        void OnInit() override;
        void OnActivate(Multiplayer::EntityIsMigrating entityIsMigrating) override;
        void OnDeactivate(Multiplayer::EntityIsMigrating entityIsMigrating) override;
        //! @}

        //! NetworkHierarchyRequestBus overrides.
        //! @{
        bool IsHierarchicalRoot() const override;
        bool IsHierarchicalChild() const override;
        AZStd::vector<AZ::Entity*> GetHierarchicalEntities() const override;
        AZ::Entity* GetHierarchicalRoot() const override;
        //! @}

    protected:
        //! AZ::TransformNotificationBus::Handler overrides.
        //! @{
        void OnParentChanged(AZ::EntityId oldParent, AZ::EntityId newParent) override;
        void OnChildAdded(AZ::EntityId child) override;
        void OnChildRemoved(AZ::EntityId childRemovedId) override;
        //! @}

        void SetTopLevelHierarchyRootEntity(AZ::Entity* hierarchyRoot);

    private:
        AZ::Entity* m_higherRootEntity = nullptr;
        AZStd::vector<AZ::Entity*> m_hierarchicalEntities;

        void RebuildHierarchy();

        //! @returns false if the maximum supported hierarchy size has been reached
        bool RecursiveAttachHierarchicalEntities(AZ::EntityId underEntity, uint32_t& currentEntityCount);
        //! @returns false if the maximum supported hierarchy size has been reached
        bool RecursiveAttachHierarchicalChild(AZ::EntityId entity, uint32_t& currentEntityCount);
    };
}
