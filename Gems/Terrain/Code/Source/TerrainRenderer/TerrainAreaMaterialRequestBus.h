/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/ComponentBus.h>
#include <SurfaceData/SurfaceDataTypes.h>
#include <Atom/RPI.Public/Material/Material.h>

namespace Terrain
{
    //! This bus provides retrieval of information from Terrain Surfaces.
    class TerrainAreaMaterialRequests
        : public AZ::ComponentBus
    {
    public:
        ////////////////////////////////////////////////////////////////////////
        // EBusTraits
        using MutexType = AZStd::recursive_mutex;
        ////////////////////////////////////////////////////////////////////////

        virtual ~TerrainAreaMaterialRequests() = default;

        //! Get the Aabb for the region where a TerrainSurfaceMaterialMapping exists
        virtual const AZ::Aabb& GetTerrainSurfaceMaterialRegion() const = 0;

        //! Get the Material asset assigned to a particular surface tag.
        virtual const AZStd::vector<struct TerrainSurfaceMaterialMapping>& GetSurfaceMaterialMappings() const = 0;
    };

    using TerrainAreaMaterialRequestBus = AZ::EBus<TerrainAreaMaterialRequests>;

     //! Notifications for when the surface -> material mapping changes..
    class TerrainAreaMaterialNotifications : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        //////////////////////////////////////////////////////////////////////////

        virtual void OnTerrainSurfaceMaterialMappingCreated(
            [[maybe_unused]] AZ::EntityId entityId,
            [[maybe_unused]] SurfaceData::SurfaceTag surface,
            [[maybe_unused]] AZ::Data::Instance<AZ::RPI::Material> material)
        {
        }

        virtual void OnTerrainSurfaceMaterialMappingDestroyed(
            [[maybe_unused]] AZ::EntityId entityId,
            [[maybe_unused]] SurfaceData::SurfaceTag surface)
        {
        }

        virtual void OnTerrainSurfaceMaterialMappingChanged(
            [[maybe_unused]] AZ::EntityId entityId,
            [[maybe_unused]] SurfaceData::SurfaceTag surface,
            [[maybe_unused]] AZ::Data::Instance<AZ::RPI::Material> material)
        {
        }

        virtual void OnTerrainSurfaceMaterialMappingRegionChanged(
            [[maybe_unused]] AZ::EntityId entityId,
            [[maybe_unused]] const AZ::Aabb& oldRegion,
            [[maybe_unused]] const AZ::Aabb& newRegion)
        {
        }
    };
    using TerrainAreaMaterialNotificationBus = AZ::EBus<TerrainAreaMaterialNotifications>;
} // namespace Terrain
