/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/EntityId.h>
#include <AzFramework/Terrain/TerrainDataRequestBus.h>
#include <TerrainRenderer/TerrainMacroMaterialBus.h>

namespace UnitTest
{
    class MockTerrainMacroMaterialNotificationBus : public Terrain::TerrainMacroMaterialNotificationBus::Handler
    {
    public:
        MockTerrainMacroMaterialNotificationBus()
        {
            Terrain::TerrainMacroMaterialNotificationBus::Handler::BusConnect();
        }

        ~MockTerrainMacroMaterialNotificationBus()
        {
            Terrain::TerrainMacroMaterialNotificationBus::Handler::BusDisconnect();
        }
        MOCK_METHOD2(
            OnTerrainMacroMaterialCreated, void(AZ::EntityId macroMaterialEntity, const Terrain::MacroMaterialData& macroMaterial));
        MOCK_METHOD2(
            OnTerrainMacroMaterialChanged, void(AZ::EntityId macroMaterialEntity, const Terrain::MacroMaterialData& macroMaterial));
        MOCK_METHOD3(
            OnTerrainMacroMaterialRegionChanged,
            void(AZ::EntityId macroMaterialEntity, const AZ::Aabb& oldRegion, const AZ::Aabb& newRegion));
        MOCK_METHOD1(OnTerrainMacroMaterialDestroyed, void(AZ::EntityId macroMaterialEntity));
    };
} // namespace UnitTest
