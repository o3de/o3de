/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Multiplayer/EntityDomains/IEntityDomain.h>

namespace Multiplayer 
{
    class FullOwnershipEntityDomain
        : public IEntityDomain
    {
    public:
        FullOwnershipEntityDomain() = default;
        FullOwnershipEntityDomain(const FullOwnershipEntityDomain& rhs) = default;

        //! IEntityDomain overrides.
        //! @{
        void SetAabb(const AZ::Aabb& aabb) override;
        const AZ::Aabb& GetAabb() const override;
        bool IsInDomain(const ConstNetworkEntityHandle& entityHandle) const override;
        void ActivateTracking(const INetworkEntityManager::OwnedEntitySet& ownedEntitySet) override;
        const EntitiesNotInDomain& RetrieveEntitiesNotInDomain() const override;
        void DebugDraw() const override;
        //! @}

    private:
        EntitiesNotInDomain m_entitiesNotInDomain;
    };
}
