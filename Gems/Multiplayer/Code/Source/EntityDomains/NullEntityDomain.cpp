/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Source/EntityDomains/NullEntityDomain.h>
#include <Multiplayer/IMultiplayer.h>
#include <AzCore/Console/ILogger.h>

namespace Multiplayer 
{
    void NullEntityDomain::SetAabb([[maybe_unused]] const AZ::Aabb& aabb)
    {
        ; // Do nothing, by definition we own everything
    }

    const AZ::Aabb& NullEntityDomain::GetAabb() const
    {
        static AZ::Aabb nullAabb = AZ::Aabb::CreateNull();
        return nullAabb;
    }

    bool NullEntityDomain::IsInDomain([[maybe_unused]] const ConstNetworkEntityHandle& entityHandle) const
    {
        return false;
    }

    void NullEntityDomain::HandleLossOfAuthoritativeReplicator([[maybe_unused]] const ConstNetworkEntityHandle& entityHandle)
    {
        AZLOG_ERROR("Timed out entity id %llu during migration, marking for removal", aznumeric_cast<AZ::u64>(entityHandle.GetNetEntityId()));
        GetNetworkEntityManager()->MarkForRemoval(entityHandle);
    }

    void NullEntityDomain::DebugDraw() const
    {
        ;
    }
}
