/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/Jobs/JobManagerComponent.h>
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

        AZStd::unique_ptr<AZ::Entity> CreateAndActivateMockTerrainLayerSpawnerThatReturnsXSquaredAsHeight(const AZ::Aabb& spawnerBox)
        {
            // Create the base entity with a mock box shape, Terrain Layer Spawner, and height provider.
            // Turn off the "use ground plane" setting so that we mark terrain as false anywhere that the spawner doesn't exist.
            Terrain::TerrainLayerSpawnerConfig config;
            config.m_useGroundPlane = false;

            auto entity = CreateEntity();
            entity->CreateComponent<UnitTest::MockAxisAlignedBoxShapeComponent>();
            entity->CreateComponent<Terrain::TerrainLayerSpawnerComponent>(config);

            m_boxShapeRequests = AZStd::make_unique<NiceMock<UnitTest::MockBoxShapeComponentRequests>>(entity->GetId());
            m_shapeRequests = AZStd::make_unique<NiceMock<UnitTest::MockShapeComponentRequests>>(entity->GetId());

            // Set up the box shape to return whatever spawnerBox was passed in.
            ON_CALL(*m_shapeRequests, GetEncompassingAabb).WillByDefault(Return(spawnerBox));

            auto mockHeights = [](const AZ::Vector3& inPosition, AZ::Vector3& outPosition, bool& terrainExists)
            {
                // Return a height (Z) that's equal to X^2.
                outPosition = AZ::Vector3(inPosition.GetX(), inPosition.GetY(), inPosition.GetX() * inPosition.GetX());
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

        // Create a mock terrain layer spawner that uses a box of (0,0,0) - (20,20,20) and generates a height equal to the X value squared.
        // The world min/max will be set to 5 and 15, so we'll verify the heights are always between 5 and 15.

        const AZ::Aabb spawnerBox = AZ::Aabb::CreateFromMinMaxValues(0.0f, 0.0f, 0.0f, 20.0f, 20.0f, 20.0f);
        auto entity = CreateAndActivateMockTerrainLayerSpawnerThatReturnsXSquaredAsHeight(spawnerBox);

        // Create and activate the terrain system with world height min/max of 5 and 15.
        const float queryResolution = 1.0f;
        AzFramework::Terrain::FloatRange heightBounds = { 5.0f, 15.0f };
        auto terrainSystem = CreateAndActivateTerrainSystem(queryResolution, heightBounds);

        // Test a set of points from (0,0) - (20,20). If the world min/max clamp is working, we should always get 5 <= height <= 15.
        for (float x = 0.0f; x <= 20.0f; x += queryResolution)
        {
            AZ::Vector3 position(x, x, 0.0f);
            bool heightQueryTerrainExists = false;
            float height =
                terrainSystem->GetHeight(position, AzFramework::Terrain::TerrainDataRequests::Sampler::DEFAULT, &heightQueryTerrainExists);

            // Verify all the heights are between 5 and 15.
            EXPECT_GE(height, heightBounds.m_min);
            EXPECT_LE(height, heightBounds.m_max);
        }
    }

    TEST_F(TerrainSystemSettingsTests, TerrainHeightQueryResolutionAffectsHeightQueries)
    {
        // Verify that the terrain height query resolution setting affects height queries. We'll verify this by
        // setting the height query resolution to 10 and querying a set of positions from 0 - 20 that return
        // the X^2 value as the height.
        // If the height query resolution is working, when we use the CLAMP sampler, queries for X=0-9 should return 0, and
        // queries for X=10-19 should return 100.
        // When we use the EXACT sampler, the query resolution should be ignored and we should get back X^2.
        // When we use the BILINEAR sampler, queries for X=0-9 should return values from 0^2-10^2, and X=10-19 should return
        // values from 10^2-20^2. 

        // Create a mock terrain layer spawner that uses a box of (0,0,0) - (30,30,1000) and generates
        // a height equal to the X value squared. (We set the max height high enough to allow for the X^2 values without clamping)
        const AZ::Aabb spawnerBox = AZ::Aabb::CreateFromMinMaxValues(0.0f, 0.0f, 0.0f, 30.0f, 30.0f, 1000.0f);
        auto entity = CreateAndActivateMockTerrainLayerSpawnerThatReturnsXSquaredAsHeight(spawnerBox);

        // Create and activate the terrain system with a world bounds that matches the spawner box, and a query resolution of 10.
        const float queryResolution = 10.0f;
        AzFramework::Terrain::FloatRange heightBounds = { spawnerBox.GetMin().GetZ(), spawnerBox.GetMax().GetZ() };
        auto terrainSystem = CreateAndActivateTerrainSystem(queryResolution, heightBounds);

        for (auto sampler : { AzFramework::Terrain::TerrainDataRequests::Sampler::BILINEAR,
                              AzFramework::Terrain::TerrainDataRequests::Sampler::CLAMP,
                              AzFramework::Terrain::TerrainDataRequests::Sampler::EXACT })
        {
            // Test a set of points from (0,0) - (20,20). We stop at 20 so that we don't test interpolation with points that don't
            // exist on the max boundary edge of 30.
            for (float x = 0.0f; x < 20.0f; x += 1.0f)
            {
                AZ::Vector3 position(x, x, 0.0f);
                bool terrainExists = false;
                float height =
                    terrainSystem->GetHeight(position, sampler, &terrainExists);

                switch (sampler)
                {
                case AzFramework::Terrain::TerrainDataRequests::Sampler::BILINEAR:
                    if (x < 10.0f)
                    {
                        // Values from 0-10 should linearly interpolate from 0^2 to 10^2
                        EXPECT_NEAR(height, AZStd::lerp(0.0f, 100.0f, x / queryResolution), 0.001f);
                    }
                    else
                    {
                        // Values from 10-19 should linearly interpolate from 10^2 to 20^2
                        EXPECT_NEAR(height, AZStd::lerp(100.0f, 400.0f, (x - queryResolution) / queryResolution), 0.001f);
                    }
                    EXPECT_TRUE(terrainExists);
                    break;
                case AzFramework::Terrain::TerrainDataRequests::Sampler::CLAMP:
                    // X values from 0-4 should round to X=0 and return 0, X values from 5-14 should round to X=10 and return 10^2,
                    // and X values from 15-19 should round up to X=20 and return 20^2.
                    if (x < 5.0f)
                    {
                        EXPECT_EQ(height, 0.0f);
                        EXPECT_TRUE(terrainExists);
                    }
                    else if (x < 15.0f)
                    {
                        EXPECT_EQ(height, 100.0f);
                        EXPECT_TRUE(terrainExists);
                    }
                    else
                    {
                        EXPECT_EQ(height, 400.0f);
                    }
                    break;
                case AzFramework::Terrain::TerrainDataRequests::Sampler::EXACT:
                    // All query points should return X^2
                    EXPECT_EQ(height, x*x);
                    EXPECT_TRUE(terrainExists);
                    break;
                }
            }
        }
    }

    TEST_F(TerrainSystemSettingsTests, TerrainSurfaceQueryResolutionAffectsSurfaceQueries)
    {
        // Verify that the terrain surface query resolution setting affects surface queries. We'll verify this by
        // setting the surface query resolution to 10 and querying a set of positions from 0 - 20 that return
        // the X value / 100 as the surface weight.
        // If the surface query resolution is working, when we use the CLAMP sampler, queries for X=0-9 should return (0/100), and
        // queries for X=10-19 should return (10/100).
        // When we use the EXACT sampler, the query resolution should be ignored and we should get back (X/100).
        // When we use the BILINEAR sampler, we should get back the same results as the CLAMP sampler, because currently the two
        // are interpreted the same way for surface queries.

        // Create a mock terrain layer spawner that uses a box of (0,0,0) - (30,30,30).
        const AZ::Aabb spawnerBox = AZ::Aabb::CreateFromMinMaxValues(0.0f, 0.0f, 0.0f, 30.0f, 30.0f, 30.0f);
        auto entity = CreateAndActivateMockTerrainLayerSpawnerThatReturnsXSquaredAsHeight(spawnerBox);
        // Set up the surface weight mocks that will return X/100 as the surface weight.
        SetupSurfaceWeightMocks(entity.get());

        // Create and activate the terrain system with a world bounds that matches the spawner box, and a query resolution of 10.
        const float heightQueryResolution = 1.0f;
        const float surfaceQueryResolution = 10.0f;
        AzFramework::Terrain::FloatRange heightBounds = { spawnerBox.GetMin().GetZ(), spawnerBox.GetMax().GetZ() };
        auto terrainSystem = CreateAndActivateTerrainSystem(heightQueryResolution, surfaceQueryResolution, heightBounds);

        for (auto sampler : { AzFramework::Terrain::TerrainDataRequests::Sampler::BILINEAR,
                              AzFramework::Terrain::TerrainDataRequests::Sampler::CLAMP,
                              AzFramework::Terrain::TerrainDataRequests::Sampler::EXACT })
        {
            // Test a set of points from (0,0) - (20,20). We stop at 20 instead of 30 so that we aren't testing what happens when
            // a query point doesn't exist.
            for (float x = 0.0f; x < 20.0f; x += 1.0f)
            {
                AZ::Vector3 position(x, x, 0.0f);
                bool terrainExists = false;
                AzFramework::SurfaceData::SurfaceTagWeight weight =
                    terrainSystem->GetMaxSurfaceWeight(position, sampler, &terrainExists);

                switch (sampler)
                {
                    case AzFramework::Terrain::TerrainDataRequests::Sampler::BILINEAR:
                        [[fallthrough]];
                    case AzFramework::Terrain::TerrainDataRequests::Sampler::CLAMP:
                        // For both BILINEAR and CLAMP:
                        // X values from 0-4 should round to X=0 and return (0/100), X values from 5-14 should round to X=10
                        // and return (10/100), and X values from 15-19 should round up to X=20 and return (20/100).
                        // and X values from 15-19 should round up to X=20 and return 20^2.
                        if (x < 5.0f)
                        {
                            EXPECT_NEAR(weight.m_weight, 0.0f, 0.001f);
                        }
                        else if (x < 15.0f)
                        {
                            EXPECT_NEAR(weight.m_weight, 0.1f, 0.001f);
                        }
                        else
                        {
                            EXPECT_NEAR(weight.m_weight, 0.2f, 0.001f);
                        }
                        break;
                        break;
                    case AzFramework::Terrain::TerrainDataRequests::Sampler::EXACT:
                        // For EXACT, queries should just return x/100 and ignore the query resolution.
                        EXPECT_NEAR(weight.m_weight, x / 100.0f, 0.001f);
                        break;
                }
            }
        }
    }

} // namespace UnitTest
