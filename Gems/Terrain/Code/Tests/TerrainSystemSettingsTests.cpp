/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/Jobs/JobManagerComponent.h>
#include <AzCore/Memory/MemoryComponent.h>
#include <AzCore/std/parallel/semaphore.h>

#include <AzTest/AzTest.h>
#include <AZTestShared/Math/MathTestHelpers.h>

#include <TerrainSystem/TerrainSystem.h>
#include <Components/TerrainLayerSpawnerComponent.h>

#include <GradientSignal/Ebuses/MockGradientRequestBus.h>
#include <Tests/Mocks/Terrain/MockTerrainDataRequestBus.h>
#include <Terrain/MockTerrainAreaSurfaceRequestBus.h>
#include <Terrain/MockTerrain.h>
#include <MockAxisAlignedBoxShapeComponent.h>
#include <TerrainTestFixtures.h>

namespace UnitTest
{
    using ::testing::NiceMock;
    using ::testing::Return;

    class TerrainSystemSettingsTests
        : public TerrainBaseFixture
        , public ::testing::Test
    {
    protected:
        // Defines a structure for defining both an XY position and the expected height for that position.
        struct HeightTestPoint
        {
            AZ::Vector2 m_testLocation = AZ::Vector2::CreateZero();
            float m_expectedHeight = 0.0f;
        };

        AZStd::unique_ptr<NiceMock<UnitTest::MockBoxShapeComponentRequests>> m_boxShapeRequests;
        AZStd::unique_ptr<NiceMock<UnitTest::MockShapeComponentRequests>> m_shapeRequests;
        AZStd::unique_ptr<NiceMock<UnitTest::MockTerrainAreaHeightRequests>> m_terrainAreaHeightRequests;
        AZStd::unique_ptr<NiceMock<UnitTest::MockTerrainAreaSurfaceRequestBus>> m_terrainAreaSurfaceRequests;

        void SetUp() override
        {
            SetupCoreSystems();
        }

        void TearDown() override
        {
            m_boxShapeRequests.reset();
            m_shapeRequests.reset();
            m_terrainAreaHeightRequests.reset();
            m_terrainAreaSurfaceRequests.reset();

            TearDownCoreSystems();
        }

        AZStd::unique_ptr<AZ::Entity> CreateAndActivateMockTerrainLayerSpawnerThatReturnsXPositionAsHeight(const AZ::Aabb& spawnerBox)
        {
            // Create the base entity with a mock box shape, Terrain Layer Spawner, and height provider.
            auto entity = CreateEntity();
            entity->CreateComponent<UnitTest::MockAxisAlignedBoxShapeComponent>();
            entity->CreateComponent<Terrain::TerrainLayerSpawnerComponent>();

            m_boxShapeRequests = AZStd::make_unique<NiceMock<UnitTest::MockBoxShapeComponentRequests>>(entity->GetId());
            m_shapeRequests = AZStd::make_unique<NiceMock<UnitTest::MockShapeComponentRequests>>(entity->GetId());

            // Set up the box shape to return whatever spawnerBox was passed in.
            ON_CALL(*m_shapeRequests, GetEncompassingAabb).WillByDefault(Return(spawnerBox));

            auto mockHeights = [](const AZ::Vector3& inPosition, AZ::Vector3& outPosition, bool& terrainExists)
            {
                // Return a height (Z) that's equal to the X position.
                outPosition = AZ::Vector3(inPosition.GetX(), inPosition.GetY(), inPosition.GetX());
                terrainExists = true;
            };

            // Set up a mock height provider that returns the X position as the height.
            m_terrainAreaHeightRequests = AZStd::make_unique<NiceMock<UnitTest::MockTerrainAreaHeightRequests>>(entity->GetId());
            ON_CALL(*m_terrainAreaHeightRequests, GetHeight).WillByDefault(mockHeights);
            ON_CALL(*m_terrainAreaHeightRequests, GetHeights)
                .WillByDefault(
                    [mockHeights](AZStd::span<AZ::Vector3> inOutPositionList, AZStd::span<bool> terrainExistsList)
                    {
                        for (int i = 0; i < inOutPositionList.size(); i++)
                        {
                            mockHeights(inOutPositionList[i], inOutPositionList[i], terrainExistsList[i]);
                        }
                    });

            ActivateEntity(entity.get());
            return entity;
        }

        void SetupSurfaceWeightMocks(AZ::Entity* entity)
        {
            auto mockGetSurfaceWeights = [](
                const AZ::Vector3& position,
                AzFramework::SurfaceData::SurfaceTagWeightList& surfaceWeights)
                {
                    const SurfaceData::SurfaceTag tag1 = SurfaceData::SurfaceTag("tag1");

                    AzFramework::SurfaceData::SurfaceTagWeight tagWeight1;
                    tagWeight1.m_surfaceType = tag1;
                    tagWeight1.m_weight = position.GetX() / 100.0f;

                    surfaceWeights.clear();
                    surfaceWeights.push_back(tagWeight1);
                };

            m_terrainAreaSurfaceRequests = AZStd::make_unique<NiceMock<UnitTest::MockTerrainAreaSurfaceRequestBus>>(entity->GetId());
            ON_CALL(*m_terrainAreaSurfaceRequests, GetSurfaceWeights).WillByDefault(mockGetSurfaceWeights);
            ON_CALL(*m_terrainAreaSurfaceRequests, GetSurfaceWeightsFromList).WillByDefault(
                [mockGetSurfaceWeights](
                    AZStd::span<const AZ::Vector3> inPositionList,
                    AZStd::span<AzFramework::SurfaceData::SurfaceTagWeightList> outSurfaceWeightsList)
                {
                    for (size_t i = 0; i < inPositionList.size(); i++)
                    {
                        mockGetSurfaceWeights(inPositionList[i], outSurfaceWeightsList[i]);
                    }
                }
            );
        }
    };

    TEST_F(TerrainSystemSettingsTests, TerrainWorldMinMaxClampsHeightData)
    {
        // Verify that any height data returned from a terrain layer spawner is clamped to the world min/max settings.

        // Create a mock terrain layer spawner that uses a box of (0,0,0) - (20,20,20) and generates a height equal to the X value.
        // The world min/max will be set to 5 and 15, so we'll verify the heights are always between 5 and 15.

        const AZ::Aabb spawnerBox = AZ::Aabb::CreateFromMinMaxValues(0.0f, 0.0f, 0.0f, 20.0f, 20.0f, 20.0f);
        auto entity = CreateAndActivateMockTerrainLayerSpawnerThatReturnsXPositionAsHeight(spawnerBox);

        // Create and activate the terrain system with world height min/max of 5 and 15.
        const float queryResolution = 1.0f;
        const AZ::Aabb terrainWorldBounds = AZ::Aabb::CreateFromMinMaxValues(0.0f, 0.0f, 5.0f, 20.0f, 20.0f, 15.0f);
        auto terrainSystem = CreateAndActivateTerrainSystem(queryResolution, terrainWorldBounds);

        // Test a set of points from (0,0) - (20,20). If the world min/max clamp is working, we should always get 5 <= height <= 15.
        for (float x = 0.0f; x <= 20.0f; x += queryResolution)
        {
            AZ::Vector3 position(x, x, 0.0f);
            bool heightQueryTerrainExists = false;
            float height =
                terrainSystem->GetHeight(position, AzFramework::Terrain::TerrainDataRequests::Sampler::DEFAULT, &heightQueryTerrainExists);

            // Verify all the heights are between 5 and 15.
            EXPECT_GE(height, terrainWorldBounds.GetMin().GetZ());
            EXPECT_LE(height, terrainWorldBounds.GetMax().GetZ());
        }
    }

    TEST_F(TerrainSystemSettingsTests, TerrainHeightQueryResolutionAffectsHeightQueries)
    {
        // Verify that the terrain height query resolution setting affects height queries. We'll verify this by
        // setting the height query resolution to 10, use the CLAMP sampler, and query a set of positions from 0 - 20 that return
        // the X value as the height.
        // If the query resolution is working, we should only get heights of 0 for X = 0-9, and 10 for X = 10-19

        // Create a mock terrain layer spawner that uses a box of (0,0,0) - (20,20,20) and generates a height equal to the X value.
        const AZ::Aabb spawnerBox = AZ::Aabb::CreateFromMinMaxValues(0.0f, 0.0f, 0.0f, 20.0f, 20.0f, 20.0f);
        auto entity = CreateAndActivateMockTerrainLayerSpawnerThatReturnsXPositionAsHeight(spawnerBox);

        // Create and activate the terrain system with a world bounds that matches the spawner box, and a query resolution of 10.
        const float queryResolution = 10.0f;
        auto terrainSystem = CreateAndActivateTerrainSystem(queryResolution, spawnerBox);

        // Test a set of points from (0,0) - (20,20). If the query resolution is working, we should only get a height of 0 for X=0-9,
        // and a height of 10 for X=10-19.
        for (float x = 0.0f; x < 20.0f; x += 1.0f)
        {
            AZ::Vector3 position(x, x, 0.0f);
            bool terrainExists = false;
            float height = terrainSystem->GetHeight(position, AzFramework::Terrain::TerrainDataRequests::Sampler::CLAMP, &terrainExists);

            EXPECT_EQ(height, x < 10.0f ? 0.0f : 10.0f);
        }
    }

    TEST_F(TerrainSystemSettingsTests, TerrainSurfaceQueryResolutionAffectsSurfaceQueries)
    {
        // Verify that the terrain surface query resolution setting affects surface queries. We'll verify this by
        // setting the surface query resolution to 10, use the CLAMP sampler, and query a set of positions from 0 - 20 that return
        // the X value / 100 as the surface weight.
        // If the query resolution is working, we should only get weights of 0.0 for X = 0-9, and 0.1 for X = 10-19

        // Create a mock terrain layer spawner that uses a box of (0,0,0) - (20,20,20).
        const AZ::Aabb spawnerBox = AZ::Aabb::CreateFromMinMaxValues(0.0f, 0.0f, 0.0f, 20.0f, 20.0f, 20.0f);
        auto entity = CreateAndActivateMockTerrainLayerSpawnerThatReturnsXPositionAsHeight(spawnerBox);
        // Set up the surface weight mocks that will return X/100 as the surface weight.
        SetupSurfaceWeightMocks(entity.get());

        // Create and activate the terrain system with a world bounds that matches the spawner box, and a query resolution of 10.
        const float heightQueryResolution = 1.0f;
        const float surfaceQueryResolution = 10.0f;
        auto terrainSystem = CreateAndActivateTerrainSystem(heightQueryResolution, surfaceQueryResolution, spawnerBox);

        // Test a set of points from (0,0) - (20,20). If the query resolution is working, we should only get a weight of 0.0 for X=0-9,
        // and a weight of 0.1 for X=10-19.
        for (float x = 0.0f; x < 20.0f; x += 1.0f)
        {
            AZ::Vector3 position(x, x, 0.0f);
            bool terrainExists = false;
            AzFramework::SurfaceData::SurfaceTagWeight weight = terrainSystem->GetMaxSurfaceWeight(
                position, AzFramework::Terrain::TerrainDataRequests::Sampler::CLAMP, &terrainExists);

            EXPECT_NEAR(weight.m_weight, x < 10.0f ? 0.0f : 0.1f, 0.001f);
        }
    }


} // namespace UnitTest
