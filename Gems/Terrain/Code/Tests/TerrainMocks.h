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

namespace UnitTest
{
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
        MOCK_CONST_METHOD0(GetTerrainHeightQueryResolution, AZ::Vector2());
        MOCK_METHOD1(SetTerrainHeightQueryResolution, void(AZ::Vector2));
        MOCK_CONST_METHOD0(GetTerrainAabb, AZ::Aabb());
        MOCK_METHOD1(SetTerrainAabb, void(const AZ::Aabb&));
        MOCK_CONST_METHOD3(GetHeight, float(AZ::Vector3, Sampler, bool*));
        MOCK_CONST_METHOD4(GetHeightFromFloats, float(float, float, Sampler, bool*));
        MOCK_CONST_METHOD3(GetMaxSurfaceWeight, AzFramework::SurfaceData::SurfaceTagWeight(AZ::Vector3, Sampler, bool*));
        MOCK_CONST_METHOD4(GetMaxSurfaceWeightFromFloats, AzFramework::SurfaceData::SurfaceTagWeight(float, float, Sampler, bool*));
        MOCK_CONST_METHOD3(GetMaxSurfaceName, const char*(AZ::Vector3, Sampler, bool*));
        MOCK_CONST_METHOD3(GetIsHoleFromFloats, bool(float, float, Sampler));
        MOCK_CONST_METHOD3(GetNormal, AZ::Vector3(AZ::Vector3, Sampler, bool*));
        MOCK_CONST_METHOD4(GetNormalFromFloats, AZ::Vector3(float, float, Sampler, bool*));
    };

    class MockHeightfieldProviderNotificationBusListener
        : public AZ::Component
        , private Physics::HeightfieldProviderNotificationBus::Handler
    {
    public:
        AZ_COMPONENT(MockHeightfieldProviderNotificationBusListener, "{2A89ED68-5937-4876-A073-FB6C8AF3D379}")
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
