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
#include <AzFramework/Physics/HeightfieldProviderBus.h>
#include <AzFramework/Terrain/TerrainDataRequestBus.h>
#include <LmbrCentral/Shape/ShapeComponentBus.h>

namespace UnitTest
{
    static const AZ::Uuid BoxShapeComponentTypeId = "{5EDF4B9E-0D3D-40B8-8C91-5142BCFC30A6}";

    class MockBoxShapeComponent
        : public AZ::Component
        , private LmbrCentral::ShapeComponentRequestsBus::Handler
    {
    public:
        AZ_COMPONENT(MockBoxShapeComponent, BoxShapeComponentTypeId)
        static void Reflect([[maybe_unused]] AZ::ReflectContext* context)
        {
        }

        void Activate() override
        {
            m_bounds = AZ::Aabb::CreateFromMinMax(AZ::Vector3(-100.0f, -100.0f, -100.0f), AZ::Vector3(100.0f, 100.0f, 100.0f));
            LmbrCentral::ShapeComponentRequestsBus::Handler::BusConnect(GetEntityId());
        }

        void Deactivate() override
        {
            LmbrCentral::ShapeComponentRequestsBus::Handler::BusDisconnect();
        }

        bool ReadInConfig([[maybe_unused]] const AZ::ComponentConfig* baseConfig) override
        {
            return true;
        }

        bool WriteOutConfig([[maybe_unused]] AZ::ComponentConfig* outBaseConfig) const override
        {
            return true;
        }

        void SetAabbFromMinMax(const AZ::Vector3& min, const AZ::Vector3& max)
        {
            m_bounds = AZ::Aabb::CreateFromMinMax(min, max);
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

        // ShapeComponentRequestsBus
        AZ::Crc32 GetShapeType() override
        {
            return AZ_CRC("Box", 0x08a9483a);
        }

        AZ::Aabb GetEncompassingAabb() override
        {
            return m_bounds;
        }

        MOCK_METHOD2(GetTransformAndLocalBounds, void(AZ::Transform&, AZ::Aabb&));
        MOCK_METHOD1(IsPointInside, bool(const AZ::Vector3&));
        MOCK_METHOD1(DistanceSquaredFromPoint, float(const AZ::Vector3&));

        AZ::Aabb m_bounds;
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

    class MockTerrainDataRequestsListener : public AzFramework::Terrain::TerrainDataRequestBus::Handler
    {
    public:
        MockTerrainDataRequestsListener()
        {
            AzFramework::Terrain::TerrainDataRequestBus::Handler::BusConnect();
        }

        ~MockTerrainDataRequestsListener()
        {
            AzFramework::Terrain::TerrainDataRequestBus::Handler::BusDisconnect();
        }

        MOCK_METHOD0(Activate, void());
        MOCK_METHOD0(Deactivate, void());

        AZ::Vector2 GetTerrainHeightQueryResolution() const override
        {
            return AZ::Vector2(1.0f, 1.0f);
        }

        MOCK_METHOD1(SetTerrainHeightQueryResolution, void(AZ::Vector2));
        MOCK_CONST_METHOD0(GetTerrainAabb, AZ::Aabb());
        MOCK_METHOD1(SetTerrainAabb, void(const AZ::Aabb&));
        MOCK_CONST_METHOD3(GetHeight, float(AZ::Vector3, Sampler, bool*));

        float GetHeightFromFloats(
            [[maybe_unused]] float x,
            [[maybe_unused]] float y,
            [[maybe_unused]] Sampler sampler = Sampler::BILINEAR,
            [[maybe_unused]] bool* terrainExistsPtr = nullptr) const override
        {
            return m_mockHeight;
        }

        MOCK_CONST_METHOD3(GetMaxSurfaceWeight, AzFramework::SurfaceData::SurfaceTagWeight(AZ::Vector3, Sampler, bool*));
        MOCK_CONST_METHOD4(GetMaxSurfaceWeightFromFloats, AzFramework::SurfaceData::SurfaceTagWeight(float, float, Sampler, bool*));
        MOCK_CONST_METHOD3(GetMaxSurfaceName, const char*(AZ::Vector3, Sampler, bool*));
        MOCK_CONST_METHOD3(GetIsHoleFromFloats, bool(float, float, Sampler));
        MOCK_CONST_METHOD3(GetNormal, AZ::Vector3(AZ::Vector3, Sampler, bool*));
        MOCK_CONST_METHOD4(GetNormalFromFloats, AZ::Vector3(float, float, Sampler, bool*));


        float m_mockHeight = 1.0f;
    };

    static const AZ::Uuid MockHeightfieldProviderNotificationBusListenerTypeId = "{2A89ED68-5937-4876-A073-FB6C8AF3D379}";

    class MockHeightfieldProviderNotificationBusListener
        : public AZ::Component
        , private Physics::HeightfieldProviderNotificationBus::Handler
    {
    public:
        AZ_COMPONENT(MockHeightfieldProviderNotificationBusListener, MockHeightfieldProviderNotificationBusListenerTypeId)
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

        MockHeightfieldProviderNotificationBusListener()
        {
            Physics::HeightfieldProviderNotificationBus::Handler::BusConnect(GetEntityId());
        }

        ~MockHeightfieldProviderNotificationBusListener()
        {
            Physics::HeightfieldProviderNotificationBus::Handler::BusDisconnect();
        }

        MOCK_METHOD1(OnHeightfieldDataChanged, void(const AZ::Aabb&));
    };
} // namespace UnitTest
