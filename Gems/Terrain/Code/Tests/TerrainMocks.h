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
#include <LmbrCentral/Shape/ShapeComponentBus.h>

namespace UnitTest
{
    static const AZ::Uuid BoxShapeComponentTypeId = "{5EDF4B9E-0D3D-40B8-8C91-5142BCFC30A6}";

    class MockBoxShapeComponent
        : public AZ::Component
    {
    public:
        AZ_COMPONENT(MockBoxShapeComponent, BoxShapeComponentTypeId)
        static void Reflect([[maybe_unused]] AZ::ReflectContext* context)
        {
        }

        void Activate() override
        {
        }

        void Deactivate() override
        {
        }

        bool ReadInConfig([[maybe_unused]] const AZ::ComponentConfig* baseConfig) override
        {
            return true;
        }

        bool WriteOutConfig([[maybe_unused]] AZ::ComponentConfig* outBaseConfig) const override
        {
            return true;
        }

    private:
        static void GetProvidedServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC("ShapeService", 0xe86aa5fe));
            provided.push_back(AZ_CRC("BoxShapeService", 0x946a0032));
            provided.push_back(AZ_CRC_CE("AxisAlignedBoxShapeService"));
        }

        static void GetIncompatibleServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
        }

        static void GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
        {
        }

        static void GetDependentServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& dependent)
        {
        }
    };

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

}
