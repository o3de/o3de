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
    class NetworkHierarchyNotifications
        : public AZ::ComponentBus
    {
    public:
        //! Called when a hierarchy has been updated (a child added or removed, etc.)
        virtual void OnNetworkHierarchyUpdated([[maybe_unused]] const AZ::EntityId& rootEntityId) {}

        //! Called when an entity has left a hierarchy
        virtual void OnNetworkHierarchyLeave() {}
    };

    typedef AZ::EBus<NetworkHierarchyNotifications> NetworkHierarchyNotificationBus;

    class NetworkHierarchyRequests
        : public AZ::ComponentBus
    {
    public:
        //! @returns hierarchical entities, the first element is the top level root
        virtual AZStd::vector<AZ::Entity*> GetHierarchicalEntities() const = 0;

        //! @returns the top level root of a hierarchy, or nullptr if this entity is not in a hierarchy
        virtual AZ::Entity* GetHierarchicalRoot() const = 0;

        //! @return true if this entity is a child entity within a hierarchy
        virtual bool IsHierarchicalChild() const = 0;

        //! @return true if this entity is the top level root of a hierarchy
        virtual bool IsHierarchicalRoot() const = 0;
    };

    typedef AZ::EBus<NetworkHierarchyRequests> NetworkHierarchyRequestBus;
}
