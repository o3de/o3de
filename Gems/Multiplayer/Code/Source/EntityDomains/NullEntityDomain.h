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
    class NullEntityDomain
        : public IEntityDomain
    {
    public:
        NullEntityDomain() = default;
        NullEntityDomain(const NullEntityDomain& rhs) = default;

        //! IEntityDomain overrides.
        //! @{
        void SetAabb(const AZ::Aabb& aabb) override;
        const AZ::Aabb& GetAabb() const override;
        bool IsInDomain(const ConstNetworkEntityHandle& entityHandle) const override;
        void HandleLossOfAuthoritativeReplicator(const ConstNetworkEntityHandle& entityHandle) override;
        void DebugDraw() const override;
        //! @}
    };
}
