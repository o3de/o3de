/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/ComponentBus.h>

namespace Multiplayer
{
    using NetworkHierarchyChangedEvent = AZ::Event<const AZ::EntityId&>;
    using NetworkHierarchyLeaveEvent = AZ::Event<>;

    class NetworkHierarchyRequests
        : public AZ::ComponentBus
    {
    public:
        //! @returns true if the entity a hierarchical component attached should be considered for inclusion in a hierarchy
        //!          this should return false when an entity is deactivating
        virtual bool IsHierarchyEnabled() const = 0;

        //! @returns hierarchical entities, the first element is the top level root
        virtual AZStd::vector<AZ::Entity*> GetHierarchicalEntities() const = 0;

        //! @returns the top level root of a hierarchy, or nullptr if this entity is not in a hierarchy
        virtual AZ::Entity* GetHierarchicalRoot() const = 0;

        //! @return true if this entity is a child entity within a hierarchy
        virtual bool IsHierarchicalChild() const = 0;

        //! @return true if this entity is the top level root of a hierarchy
        virtual bool IsHierarchicalRoot() const = 0;

        //! Binds the provided NetworkHierarchyChangedEvent handler to a Network Hierarchy component.
        //! @param handler the handler to invoke when the entity's network hierarchy has been modified.
        virtual void BindNetworkHierarchyChangedEventHandler(NetworkHierarchyChangedEvent::Handler& handler) = 0;

        //! Binds the provided NetworkHierarchyLeaveEvent handler to a Network Hierarchy component.
        //! @param handler the handler to invoke when the entity left its network hierarchy.
        virtual void BindNetworkHierarchyLeaveEventHandler(NetworkHierarchyLeaveEvent::Handler& handler) = 0;
    };

    typedef AZ::EBus<NetworkHierarchyRequests> NetworkHierarchyRequestBus;
}
