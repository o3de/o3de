/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
        bool IsInDomain(const ConstNetworkEntityHandle& entityHandle) const override;
        void ActivateTracking(const INetworkEntityManager::OwnedEntitySet& ownedEntitySet) override;
        void RetrieveEntitiesNotInDomain(EntitiesNotInDomain& outEntitiesNotInDomain) const override;
        void DebugDraw() const override;
        //! @}
    };
}
