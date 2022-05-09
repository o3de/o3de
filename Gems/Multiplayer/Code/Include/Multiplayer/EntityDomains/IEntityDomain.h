/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
        virtual ~IEntityDomain() = default;

        //! For domains that operate on a region of space, this sets the area the domain is responsible for.
        //! @param aabb the aabb associated with this entity domain
        virtual void SetAabb(const AZ::Aabb& aabb) = 0;

        //! Retrieves the aabb representing the domain area, an invalid aabb will be returned for non-spatial domains.
        //! @return the aabb associated with this entity domain
        virtual const AZ::Aabb& GetAabb() const = 0;

        //! Returns whether or not an entity should be owned by an entity manager.
        //! @param entityHandle the handle of the netbound entity to check
        //! @return false if this entity should not belong to the entity manger, true if it could be owned by the entity manager
        virtual bool IsInDomain(const ConstNetworkEntityHandle& entityHandle) const = 0;

        //! This method will be invoked whenever we unexpectedly lose the authoritative entity replicator for an entity.
        //! This gives our entity domain a chance to determine whether or not it should assume authority in this instance.
        //! @param entityHandle the network entity handle of the entity that has lost its authoritative replicator
        virtual void HandleLossOfAuthoritativeReplicator(const ConstNetworkEntityHandle& entityHandle) = 0;

        //! Debug draw to visualize host entity domains.
        virtual void DebugDraw() const = 0;
    };
}
