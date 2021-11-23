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
#include <TerrainSystem/TerrainSystemBus.h>

namespace UnitTest
{
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

    class MockTerrainDataRequests : public AzFramework::Terrain::TerrainDataRequestBus::Handler
    {
    public:
        MockTerrainDataRequests()
        {
            AzFramework::Terrain::TerrainDataRequestBus::Handler::BusConnect();
        }

        ~MockTerrainDataRequests()
        {
            AzFramework::Terrain::TerrainDataRequestBus::Handler::BusDisconnect();
        }

        MOCK_CONST_METHOD0(GetTerrainHeightQueryResolution, AZ::Vector2());
        MOCK_METHOD1(SetTerrainHeightQueryResolution, void(AZ::Vector2));
        MOCK_CONST_METHOD0(GetTerrainAabb, AZ::Aabb());
        MOCK_METHOD1(SetTerrainAabb, void(const AZ::Aabb&));
        MOCK_CONST_METHOD3(GetHeight, float(const AZ::Vector3&, Sampler, bool*));
        MOCK_CONST_METHOD3(GetHeightFromVector2, float(const AZ::Vector2&, Sampler, bool*));
        MOCK_CONST_METHOD4(GetHeightFromFloats, float(float, float, Sampler, bool*));
        MOCK_CONST_METHOD2(GetIsHole, bool(const AZ::Vector3&, Sampler));
        MOCK_CONST_METHOD2(GetIsHoleFromVector2, bool(const AZ::Vector2&, Sampler));
        MOCK_CONST_METHOD3(GetIsHoleFromFloats, bool(float, float, Sampler));
        MOCK_CONST_METHOD3(GetNormal, AZ::Vector3(const AZ::Vector3&, Sampler, bool*));
        MOCK_CONST_METHOD3(GetNormalFromVector2, AZ::Vector3(const AZ::Vector2&, Sampler, bool*));
        MOCK_CONST_METHOD4(GetNormalFromFloats, AZ::Vector3(float, float, Sampler, bool*));
        MOCK_CONST_METHOD3(GetMaxSurfaceWeight, AzFramework::SurfaceData::SurfaceTagWeight(const AZ::Vector3&, Sampler, bool*));
        MOCK_CONST_METHOD3(GetMaxSurfaceWeightFromVector2, AzFramework::SurfaceData::SurfaceTagWeight(const AZ::Vector2&, Sampler, bool*));
        MOCK_CONST_METHOD4(GetMaxSurfaceWeightFromFloats, AzFramework::SurfaceData::SurfaceTagWeight(float, float, Sampler, bool*));
        MOCK_CONST_METHOD4(GetSurfaceWeights, void(const AZ::Vector3&, AzFramework::SurfaceData::SurfaceTagWeightList&, Sampler, bool*));
        MOCK_CONST_METHOD4(
            GetSurfaceWeightsFromVector2, void(const AZ::Vector2&, AzFramework::SurfaceData::SurfaceTagWeightList&, Sampler, bool*));
        MOCK_CONST_METHOD5(
            GetSurfaceWeightsFromFloats, void(float, float, AzFramework::SurfaceData::SurfaceTagWeightList&, Sampler, bool*));
        MOCK_CONST_METHOD3(GetMaxSurfaceName, const char*(const AZ::Vector3&, Sampler, bool*));
        MOCK_CONST_METHOD4(GetSurfacePoint, void(const AZ::Vector3&, AzFramework::SurfaceData::SurfacePoint&, Sampler, bool*));
        MOCK_CONST_METHOD4(
            GetSurfacePointFromVector2, void(const AZ::Vector2&, AzFramework::SurfaceData::SurfacePoint&, Sampler, bool*));
        MOCK_CONST_METHOD5(
            GetSurfacePointFromFloats, void(float, float, AzFramework::SurfaceData::SurfacePoint&, Sampler, bool*));
    };
} // namespace UnitTest
