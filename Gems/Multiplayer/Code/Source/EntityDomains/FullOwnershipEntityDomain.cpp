/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Source/EntityDomains/FullOwnershipEntityDomain.h>

namespace Multiplayer 
{
    void FullOwnershipEntityDomain::SetAabb([[maybe_unused]] const AZ::Aabb& aabb)
    {
        ; // Do nothing, by definition we own everything
    }

    const AZ::Aabb& FullOwnershipEntityDomain::GetAabb() const
    {
        static AZ::Aabb nullAabb = AZ::Aabb::CreateNull();
        return nullAabb;
    }

    bool FullOwnershipEntityDomain::IsInDomain([[maybe_unused]] const ConstNetworkEntityHandle& entityHandle) const
    {
        return true;
    }

    void FullOwnershipEntityDomain::ActivateTracking([[maybe_unused]] const INetworkEntityManager::OwnedEntitySet& ownedEntitySet)
    {
        ;
    }

    const IEntityDomain::EntitiesNotInDomain& FullOwnershipEntityDomain::RetrieveEntitiesNotInDomain() const
    {
        return m_entitiesNotInDomain;
    }

    void FullOwnershipEntityDomain::DebugDraw() const
    {
        ;
    }
}
