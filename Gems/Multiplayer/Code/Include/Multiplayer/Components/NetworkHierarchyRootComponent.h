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

    protected:
        void SetTopLevelHierarchyRootEntity(AZ::Entity* hierarchyRoot);

    private:
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
        
        //! @param underEntity Walk the child entities that belong to @underEntity and consider adding them to the hierarchy
        //! @param currentEntityCount The total number of entities in the hierarchy prior to calling this method,
        //!            used to avoid adding too many entities to the hierarchy while walking recursively the relevant entities.
        //!            @currentEntityCount will be modified to reflect the total entity count upon completion of this method.
        //! @returns false if an attempt was made to go beyond the maximum supported hierarchy size, true otherwise
        bool RecursiveAttachHierarchicalEntities(AZ::EntityId underEntity, uint32_t& currentEntityCount);

        //! @param entity Add the child entity and any of its relevant children to the hierarchy
        //! @param currentEntityCount The total number of entities in the hierarchy prior to calling this method,
        //!            used to avoid adding too many entities to the hierarchy while walking recursively the relevant entities.
        //!            @currentEntityCount will be modified to reflect the total entity count upon completion of this method.
        //! @returns false if an attempt was made to go beyond the maximum supported hierarchy size, true otherwise
        bool RecursiveAttachHierarchicalChild(AZ::EntityId entity, uint32_t& currentEntityCount);

        void SetRootForEntity(AZ::Entity* root, const AZ::Entity* childEntity);
        
        //! Set to false when deactivating or otherwise not to be included in hierarchy considerations.
        bool m_isHierarchyEnabled = true;

        friend class HierarchyBenchmarkBase;
    };
}
