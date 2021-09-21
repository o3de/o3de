/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <gmock/gmock.h>

#include <AzCore/Component/ComponentApplication.h>
#include <AzFramework/Terrain/TerrainDataRequestBus.h>

namespace UnitTest
{

    class MockTerrainSystemService : private Terrain::TerrainSystemServiceRequestBus::Handler
    {
    public:
        MockTerrainSystemService()
        {
            Terrain::TerrainSystemServiceRequestBus::Handler::BusConnect();
        }

        ~MockTerrainSystemService()
        {
            Terrain::TerrainSystemServiceRequestBus::Handler::BusDisconnect();
        }

        MOCK_METHOD0(Activate, void());
        MOCK_METHOD0(Deactivate, void());

        MOCK_METHOD1(RegisterArea, void(AZ::EntityId areaId));
        MOCK_METHOD1(UnregisterArea, void(AZ::EntityId areaId));
        MOCK_METHOD1(RefreshArea, void(AZ::EntityId areaId));
    };

    class MockTerrainDataNotificationListener : public AzFramework::Terrain::TerrainDataNotificationBus::Handler
    {
    public:
        MockTerrainDataNotificationListener()
        {
            AzFramework::Terrain::TerrainDataNotificationBus::Handler::BusConnect();
        }

        ~MockTerrainDataNotificationListener()
        {
            AzFramework::Terrain::TerrainDataNotificationBus::Handler::BusDisconnect();
        }

        MOCK_METHOD0(OnTerrainDataCreateBegin, void());
        MOCK_METHOD0(OnTerrainDataCreateEnd, void());
        MOCK_METHOD0(OnTerrainDataDestroyBegin, void());
        MOCK_METHOD0(OnTerrainDataDestroyEnd, void());
        MOCK_METHOD2(OnTerrainDataChanged, void(const AZ::Aabb& dirtyRegion, TerrainDataChangedMask dataChangedMask));
    };

    class MockTerrainAreaHeightRequests : public Terrain::TerrainAreaHeightRequestBus::Handler
    {
    public:
        MockTerrainAreaHeightRequests(AZ::EntityId entityId)
        {
            Terrain::TerrainAreaHeightRequestBus::Handler::BusConnect(entityId);
        }

        ~MockTerrainAreaHeightRequests()
        {
            Terrain::TerrainAreaHeightRequestBus::Handler::BusDisconnect();
        }

        MOCK_METHOD3(GetHeight, void(
            const AZ::Vector3& inPosition,
            AZ::Vector3& outPosition,
            bool& terrainExists));

    };

    class MockTerrainSpawnerRequests : public Terrain::TerrainSpawnerRequestBus::Handler
    {
    public:
        MockTerrainSpawnerRequests(AZ::EntityId entityId)
        {
            Terrain::TerrainSpawnerRequestBus::Handler::BusConnect(entityId);
        }

        ~MockTerrainSpawnerRequests()
        {
            Terrain::TerrainSpawnerRequestBus::Handler::BusDisconnect();
        }

        MOCK_METHOD2(GetPriority, void(AZ::u32& outLayer, AZ::u32& outPriority));
        MOCK_METHOD0(GetUseGroundPlane, bool());
    };
}
