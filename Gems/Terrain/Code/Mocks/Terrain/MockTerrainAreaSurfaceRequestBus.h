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
#include <Terrain/Ebuses/TerrainAreaSurfaceRequestBus.h>

namespace UnitTest
{
    class MockTerrainAreaSurfaceRequestBus : public Terrain::TerrainAreaSurfaceRequestBus::Handler
    {
    public:
        MockTerrainAreaSurfaceRequestBus(AZ::EntityId entityId)
        {
            Terrain::TerrainAreaSurfaceRequestBus::Handler::BusConnect(entityId);
        }

        ~MockTerrainAreaSurfaceRequestBus()
        {
            Terrain::TerrainAreaSurfaceRequestBus::Handler::BusDisconnect();
        }

        MOCK_METHOD0(Activate, void());
        MOCK_METHOD0(Deactivate, void());
        MOCK_CONST_METHOD2(GetSurfaceWeights, void(const AZ::Vector3&, AzFramework::SurfaceData::SurfaceTagWeightList&));
    };

} // namespace UnitTest
