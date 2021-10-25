/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/ComponentBus.h>
#include <AzCore/EBus/EBus.h>
#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Aabb.h>
#include <Atom/RPI.Reflect/Image/Image.h>

namespace Terrain
{
    struct MacroMaterialData
    {
        AZ::EntityId m_entityId;
        AZ::Aabb m_bounds = AZ::Aabb::CreateNull();

        AZ::Data::Instance<AZ::RPI::Image> m_colorImage;
        AZ::Data::Instance<AZ::RPI::Image> m_normalImage;
        bool m_normalFlipX{ false };
        bool m_normalFlipY{ false };
        float m_normalFactor{ 0.0f };
    };

    /**
    * Request terrain macro material data.
    */
    class TerrainMacroMaterialRequests
        : public AZ::ComponentBus
    {
    public:
        ////////////////////////////////////////////////////////////////////////
        // EBusTraits
        using MutexType = AZStd::recursive_mutex;
        ////////////////////////////////////////////////////////////////////////

        virtual ~TerrainMacroMaterialRequests() = default;

        // Get the terrain macro material and the region that it covers.
        virtual MacroMaterialData GetTerrainMacroMaterialData() = 0;
    };

    using TerrainMacroMaterialRequestBus = AZ::EBus<TerrainMacroMaterialRequests>;
    
    /**
    * Notifications for when the terrain macro material data changes.
    */
    class TerrainMacroMaterialNotifications : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        //////////////////////////////////////////////////////////////////////////

        virtual void OnTerrainMacroMaterialCreated(
            [[maybe_unused]] AZ::EntityId macroMaterialEntity,
            [[maybe_unused]] const MacroMaterialData& macroMaterial)
        {
        }

        virtual void OnTerrainMacroMaterialChanged(
            [[maybe_unused]] AZ::EntityId macroMaterialEntity, [[maybe_unused]] const MacroMaterialData& macroMaterial)
        {
        }

        virtual void OnTerrainMacroMaterialRegionChanged(
            [[maybe_unused]] AZ::EntityId macroMaterialEntity,
            [[maybe_unused]] const AZ::Aabb& oldRegion,
            [[maybe_unused]] const AZ::Aabb& newRegion)
        {
        }

        virtual void OnTerrainMacroMaterialDestroyed([[maybe_unused]] AZ::EntityId macroMaterialEntity)
        {
        }
    };
    using TerrainMacroMaterialNotificationBus = AZ::EBus<TerrainMacroMaterialNotifications>;

}
