/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/Math/Vector3.h>

namespace LmbrCentral
{
    // Shared interface for terrain system implementations
    class TerrainSystemRequests
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        //////////////////////////////////////////////////////////////////////////

        // Get the terrain height at a specific location 
        virtual float GetElevation(const AZ::Vector3& position) const = 0;

        // Get the terrain surface normal at a specific location 
        virtual AZ::Vector3 GetNormal(const AZ::Vector3& position) const = 0;
    };

    using TerrainSystemRequestBus = AZ::EBus<TerrainSystemRequests>;
} // namespace LmbrCentral
