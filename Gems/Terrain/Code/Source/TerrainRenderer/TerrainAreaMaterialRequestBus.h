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

namespace Terrain
{

    struct SurfaceMaterialMapping
    {
        //AZ_TYPE_INFO(SurfaceMaterial, "{7061051D-922F-49FC-852D-CAA56BE92AB0}");

        SurfaceData::SurfaceTag m_surfaceTag;
        AZ::Data::Instance<AZ::RPI::Material> m_materialInstance;

       // static void Reflect(AZ::ReflectContext* context);
    };

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

        //! Get the Material asset assigned to a particular surface tag.
        virtual AZStd::vector<SurfaceMaterialMapping> GetSurfaceMaterialMappings() const = 0;
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

        virtual void OnTerrainSurfaceMaterialMappingChanged([[maybe_unused]] const AZ::EntityId& entityId)
        {
        }
    };
    using TerrainAreaMaterialNotificationBus = AZ::EBus<TerrainAreaMaterialNotifications>;
} // namespace Terrain
