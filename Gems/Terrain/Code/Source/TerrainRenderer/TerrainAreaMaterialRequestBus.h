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

        //! Get the Materials assigned to various surface tags.
        virtual const AZStd::vector<struct TerrainSurfaceMaterialMapping>& GetSurfaceMaterialMappings() const = 0;

        //! Get the default material
        virtual const TerrainSurfaceMaterialMapping& GetDefaultMaterial() const = 0;
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

        //! The default surface material has been assigned and loaded
        virtual void OnTerrainDefaultSurfaceMaterialCreated(
            [[maybe_unused]] AZ::EntityId entityId,
            [[maybe_unused]] AZ::Data::Instance<AZ::RPI::Material> material)
        {
        }

        //! The default surface material has been unassigned
        virtual void OnTerrainDefaultSurfaceMaterialDestroyed(
            [[maybe_unused]] AZ::EntityId entityId)
        {
        }

        //! The default surface material has been changed to a different material
        virtual void OnTerrainDefaultSurfaceMaterialChanged(
            [[maybe_unused]] AZ::EntityId entityId,
            [[maybe_unused]] AZ::Data::Instance<AZ::RPI::Material> newMaterial)
        {
        }

        //! A loaded material mapped to a valid surface tag has been created
        virtual void OnTerrainSurfaceMaterialMappingCreated(
            [[maybe_unused]] AZ::EntityId entityId,
            [[maybe_unused]] SurfaceData::SurfaceTag surface,
            [[maybe_unused]] AZ::Data::Instance<AZ::RPI::Material> material)
        {
        }

        //! Either the material or surface tag was unassigned, making this mapping invalid
        virtual void OnTerrainSurfaceMaterialMappingDestroyed(
            [[maybe_unused]] AZ::EntityId entityId,
            [[maybe_unused]] SurfaceData::SurfaceTag surface)
        {
        }

        //! The surface tag has changed to tag for an existing material
        virtual void OnTerrainSurfaceMaterialMappingTagChanged(
            [[maybe_unused]] AZ::EntityId entityId,
            [[maybe_unused]] SurfaceData::SurfaceTag oldSurface,
            [[maybe_unused]] SurfaceData::SurfaceTag newSurface)
        {
        }
        
        //! The material has changed for an existing surface tag
        virtual void OnTerrainSurfaceMaterialMappingMaterialChanged(
            [[maybe_unused]] AZ::EntityId entityId,
            [[maybe_unused]] SurfaceData::SurfaceTag surface,
            [[maybe_unused]] AZ::Data::Instance<AZ::RPI::Material> material)
        {
        }
        
        //! A set of surface material mappings has been created
        virtual void OnTerrainSurfaceMaterialMappingRegionCreated(
            [[maybe_unused]] AZ::EntityId entityId,
            [[maybe_unused]] const AZ::Aabb& region)
        {
        }
        
        //! A set of surface material mappings has been destroyed
        virtual void OnTerrainSurfaceMaterialMappingRegionDestroyed(
            [[maybe_unused]] AZ::EntityId entityId,
            [[maybe_unused]] const AZ::Aabb& oldRegion)
        {
        }

        //! The bounds of this set of surface material mappings has changed
        virtual void OnTerrainSurfaceMaterialMappingRegionChanged(
            [[maybe_unused]] AZ::EntityId entityId,
            [[maybe_unused]] const AZ::Aabb& oldRegion,
            [[maybe_unused]] const AZ::Aabb& newRegion)
        {
        }
    };
    using TerrainAreaMaterialNotificationBus = AZ::EBus<TerrainAreaMaterialNotifications>;
} // namespace Terrain
