/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
 
#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/Interface/Interface.h>

namespace OpenParticleSystem
{
    class OpenParticleSystemRequests
    {
    public:
        AZ_RTTI(OpenParticleSystemRequests, "{dc97b17d-ada8-4f67-b7ff-2380516f984d}");
        virtual ~OpenParticleSystemRequests() = default;
    };
    
    class OpenParticleSystemBusTraits
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static constexpr AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static constexpr AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        //////////////////////////////////////////////////////////////////////////
    };

    using OpenParticleSystemRequestBus = AZ::EBus<OpenParticleSystemRequests, OpenParticleSystemBusTraits>;
    using OpenParticleSystemInterface = AZ::Interface<OpenParticleSystemRequests>;

} // namespace OpenParticleSystem
