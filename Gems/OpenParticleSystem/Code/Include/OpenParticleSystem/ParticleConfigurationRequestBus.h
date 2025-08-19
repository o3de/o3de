/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
 
#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>

namespace OpenParticle
{
    class ParticleConfigurationRequest
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
    };

    using ParticleConfigurationRequestBus = AZ::EBus<ParticleConfigurationRequest>;
} // namespace OpenParticle
