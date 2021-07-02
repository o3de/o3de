/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Multiplayer/NetworkEntity/INetworkEntityManager.h>

namespace Multiplayer
{
    //! @class IEntityDomain
    //! @brief A class that determines if an entity should belong to a particular INetworkEntityManager.
    class IEntityDomain
    {
    public:
        using EntitiesNotInDomain = AZStd::unordered_set<NetEntityId>;

        virtual ~IEntityDomain() = default;

        //! Returns whether or not an entity should be owned by an entity manager.
        //! @param entityHandle the handle of the netbound entity to check
        //! @return false if this entity should not belong to the entity manger, true if it could be owned by the entity manager
        virtual bool IsInDomain(const ConstNetworkEntityHandle& entityHandle) const = 0;

        //! Enable Entity Domain Exit Tracking for entities on the host.
        //! @param ownedEntitySet the set of entities to activate tracking for
        virtual void ActivateTracking(const INetworkEntityManager::OwnedEntitySet& ownedEntitySet) = 0;

        //! Return the set of netbound entities not included in this domain.
        //! @param outEntitiesNotInDomain the set of known networked entities not included in this domain
        virtual void RetrieveEntitiesNotInDomain(EntitiesNotInDomain& outEntitiesNotInDomain) const = 0;

        //! Debug draw to visualize host entity domains.
        virtual void DebugDraw() const = 0;
    };
}
