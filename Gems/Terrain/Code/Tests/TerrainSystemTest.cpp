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

#include <TerrainSystem/TerrainSystem.h>
#include <Components/TerrainLayerSpawnerComponent.h>

#include <GradientSignal/Ebuses/MockGradientRequestBus.h>
#include <Tests/Mocks/Terrain/MockTerrainDataRequestBus.h>
#include <Terrain/MockTerrainAreaSurfaceRequestBus.h>
#include <Terrain/MockTerrain.h>
#include <MockAxisAlignedBoxShapeComponent.h>
#include <TerrainTestFixtures.h>

using ::testing::AtLeast;
using ::testing::FloatNear;
using ::testing::FloatEq;
using ::testing::IsFalse;
using ::testing::Ne;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::SetArgReferee;

namespace UnitTest
{
    class TerrainSystemTest
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

        struct NormalTestPoint
        {
            AZ::Vector2 m_testLocation = AZ::Vector2::CreateZero();
            AZ::Vector3 m_expectedNormal = AZ::Vector3::CreateZero();
        };

        struct HeightTestRegionPoints
        {
            size_t m_xIndex;
            size_t m_yIndex;
            float m_expectedHeight;
            AZ::Vector2 m_testLocation = AZ::Vector2::CreateZero();
        };

        struct NormalTestRegionPoints
        {
            size_t m_xIndex;
            size_t m_yIndex;
            AZ::Vector3 m_expectedNormal = AZ::Vector3::CreateZero();
            AZ::Vector2 m_testLocation = AZ::Vector2::CreateZero();
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

        AZStd::unique_ptr<AZ::Entity> CreateAndActivateMockTerrainLayerSpawner(
            const AZ::Aabb& spawnerBox, const AZStd::function<void(AZ::Vector3& position, bool& terrainExists)>& mockHeights)
        {
            // Create the base entity with a mock box shape, Terrain Layer Spawner, and height provider.
            auto entity = CreateEntity();
            entity->CreateComponent<UnitTest::MockAxisAlignedBoxShapeComponent>();
            entity->CreateComponent<Terrain::TerrainLayerSpawnerComponent>();

            m_boxShapeRequests = AZStd::make_unique<NiceMock<UnitTest::MockBoxShapeComponentRequests>>(entity->GetId());
            m_shapeRequests = AZStd::make_unique<NiceMock<UnitTest::MockShapeComponentRequests>>(entity->GetId());

            // Set up the box shape to return whatever spawnerBox was passed in.
            ON_CALL(*m_shapeRequests, GetEncompassingAabb).WillByDefault(Return(spawnerBox));

            // Set up a mock height provider to use the passed-in mock height function to generate a height.
            m_terrainAreaHeightRequests = AZStd::make_unique<NiceMock<UnitTest::MockTerrainAreaHeightRequests>>(entity->GetId());
            ON_CALL(*m_terrainAreaHeightRequests, GetHeight)
                .WillByDefault(
                    [mockHeights](const AZ::Vector3& inPosition, AZ::Vector3& outPosition, bool& terrainExists)
                    {
                        // By default, set the outPosition to the input position and terrain to always exist.
                        outPosition = inPosition;
                        terrainExists = true;
                        // Let the test function modify these values based on the needs of the specific test.
                        mockHeights(outPosition, terrainExists);
                    });
            ON_CALL(*m_terrainAreaHeightRequests, GetHeights)
                .WillByDefault(
                    [mockHeights](AZStd::span<AZ::Vector3> inOutPositionList, AZStd::span<bool> terrainExistsList)
                    {
                        for (int i = 0; i < inOutPositionList.size(); i++)
                        {
                            mockHeights(inOutPositionList[i], terrainExistsList[i]);
                        }
                    });

            ActivateEntity(entity.get());
            return entity;
        }

        void SetupSurfaceWeightMocks(AZ::Entity* entity, AzFramework::SurfaceData::SurfaceTagWeightList& expectedTags)
        {
            const SurfaceData::SurfaceTag tag1 = SurfaceData::SurfaceTag("tag1");
            const SurfaceData::SurfaceTag tag2 = SurfaceData::SurfaceTag("tag2");
            const SurfaceData::SurfaceTag tag3 = SurfaceData::SurfaceTag("tag3");

            AzFramework::SurfaceData::SurfaceTagWeight tagWeight1;
            tagWeight1.m_surfaceType = tag1;
            tagWeight1.m_weight = 1.0f;
            expectedTags.push_back(tagWeight1);

            AzFramework::SurfaceData::SurfaceTagWeight tagWeight2;
            tagWeight2.m_surfaceType = tag2;
            tagWeight2.m_weight = 0.7f;
            expectedTags.push_back(tagWeight2);

            AzFramework::SurfaceData::SurfaceTagWeight tagWeight3;
            tagWeight3.m_surfaceType = tag3;
            tagWeight3.m_weight = 0.3f;
            expectedTags.push_back(tagWeight3);

            auto mockGetSurfaceWeights = [tagWeight1, tagWeight2, tagWeight3](
                const AZ::Vector3& position,
                AzFramework::SurfaceData::SurfaceTagWeightList& surfaceWeights)
                {
                    surfaceWeights.clear();
                    float absYPos = fabsf(position.GetY());
                    if (absYPos < 1.0f)
                    {
                        surfaceWeights.push_back(tagWeight1);
                    }
                    else if(absYPos < 2.0f)
                    {
                        surfaceWeights.push_back(tagWeight2);
                    }
                    else
                    {
                        surfaceWeights.push_back(tagWeight3);
                    }
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

    TEST_F(TerrainSystemTest, TrivialCreateDestroy)
    {
        // Trivially verify that the terrain system can successfully be constructed and destructed without errors.

        auto terrainSystem = AZStd::make_unique<Terrain::TerrainSystem>();
    }

    TEST_F(TerrainSystemTest, TrivialActivateDeactivate)
    {
        // Verify that the terrain system can be activated and deactivated without errors.

        auto terrainSystem = AZStd::make_unique<Terrain::TerrainSystem>();
        terrainSystem->Activate();
        terrainSystem->Deactivate();
    }

    TEST_F(TerrainSystemTest, CreateEventsCalledOnActivation)
    {
        // Verify that when the terrain system is activated, the OnTerrainDataCreate* ebus notifications are generated.

        NiceMock<UnitTest::MockTerrainDataNotificationListener> mockTerrainListener;
        EXPECT_CALL(mockTerrainListener, OnTerrainDataCreateBegin()).Times(AtLeast(1));
        EXPECT_CALL(mockTerrainListener, OnTerrainDataCreateEnd()).Times(AtLeast(1));

        auto terrainSystem = AZStd::make_unique<Terrain::TerrainSystem>();
        terrainSystem->Activate();
    }

    TEST_F(TerrainSystemTest, DestroyEventsCalledOnDeactivation)
    {
        // Verify that when the terrain system is deactivated, the OnTerrainDataDestroy* ebus notifications are generated.

        NiceMock<UnitTest::MockTerrainDataNotificationListener> mockTerrainListener;
        EXPECT_CALL(mockTerrainListener, OnTerrainDataDestroyBegin()).Times(AtLeast(1));
        EXPECT_CALL(mockTerrainListener, OnTerrainDataDestroyEnd()).Times(AtLeast(1));

        auto terrainSystem = AZStd::make_unique<Terrain::TerrainSystem>();
        terrainSystem->Activate();
        terrainSystem->Deactivate();
    }

    TEST_F(TerrainSystemTest, TerrainDoesNotExistWhenNoTerrainLayerSpawnersAreRegistered)
    {
        // For the terrain system, terrain should only exist where terrain layer spawners are present.

        // Verify that in the active terrain system, if there are no terrain layer spawners, any arbitrary point
        // will return false for terrainExists, returns a height equal to the min world bounds of the terrain system, and returns
        // a normal facing up the Z axis.

        // Create and activate the terrain system with our testing defaults for world bounds and query resolution.
        auto terrainSystem = CreateAndActivateTerrainSystem();

        AZ::Aabb worldBounds = terrainSystem->GetTerrainAabb();

        // Loop through several points within the world bounds, including on the edges, and verify that they all return false for
        // terrainExists with default heights and normals.
        for (float y = worldBounds.GetMin().GetY(); y <= worldBounds.GetMax().GetY(); y += (worldBounds.GetExtents().GetY() / 4.0f))
        {
            for (float x = worldBounds.GetMin().GetX(); x <= worldBounds.GetMax().GetX(); x += (worldBounds.GetExtents().GetX() / 4.0f))
            {
                AZ::Vector3 position(x, y, 0.0f);
                bool terrainExists = true;
                float height =
                    terrainSystem->GetHeight(position, AzFramework::Terrain::TerrainDataRequests::Sampler::EXACT, &terrainExists);
                EXPECT_FALSE(terrainExists);
                EXPECT_FLOAT_EQ(height, worldBounds.GetMin().GetZ());

                terrainExists = true;
                AZ::Vector3 normal =
                    terrainSystem->GetNormal(position, AzFramework::Terrain::TerrainDataRequests::Sampler::EXACT, &terrainExists);
                EXPECT_FALSE(terrainExists);
                EXPECT_EQ(normal, AZ::Vector3::CreateAxisZ());

                bool isHole = terrainSystem->GetIsHoleFromFloats(
                    position.GetX(), position.GetY(), AzFramework::Terrain::TerrainDataRequests::Sampler::EXACT);
                EXPECT_TRUE(isHole);
            }
        }
    }

    TEST_F(TerrainSystemTest, TerrainExistsOnlyWithinTerrainLayerSpawnerBounds)
    {
        // Verify that the presence of a TerrainLayerSpawner causes terrain to exist in (and *only* in) the box where the
        // TerrainLayerSpawner is defined.

        // The terrain system should only query Heights from the TerrainAreaHeightRequest bus within the
        // TerrainLayerSpawner region, and so those values should only get returned from GetHeight for queries inside that region.

        // Create a mock terrain layer spawner that uses a box of (0,0,5) - (10,10,15) and always returns a height of 5.
        constexpr float spawnerHeight = 5.0f;
        const AZ::Aabb spawnerBox = AZ::Aabb::CreateFromMinMaxValues(0.0f, 0.0f, 5.0f, 10.0f, 10.0f, 15.0f);
        auto entity = CreateAndActivateMockTerrainLayerSpawner(
            spawnerBox,
            [](AZ::Vector3& position, bool& terrainExists)
            {
                position.SetZ(spawnerHeight);
                terrainExists = true;
            });

        // Verify that terrain exists within the layer spawner bounds, and doesn't exist outside of it.

        // Create and activate the terrain system with our testing defaults for world bounds and query resolution.
        auto terrainSystem = CreateAndActivateTerrainSystem();

        // Create a box that's twice as big as the layer spawner box.  Loop through it and verify that points within the layer box contain
        // terrain and the expected height & normal values, and points outside the layer box don't contain terrain.
        const AZ::Aabb encompassingBox = AZ::Aabb::CreateFromMinMax(
            spawnerBox.GetMin() - (spawnerBox.GetExtents() / 2.0f), spawnerBox.GetMax() + (spawnerBox.GetExtents() / 2.0f));

        for (float y = encompassingBox.GetMin().GetY(); y < encompassingBox.GetMax().GetY(); y += 1.0f)
        {
            for (float x = encompassingBox.GetMin().GetX(); x < encompassingBox.GetMax().GetX(); x += 1.0f)
            {
                AZ::Vector3 position(x, y, 0.0f);
                bool heightQueryTerrainExists = false;
                float height = terrainSystem->GetHeight(
                    position, AzFramework::Terrain::TerrainDataRequests::Sampler::EXACT, &heightQueryTerrainExists);
                bool isHole = terrainSystem->GetIsHoleFromFloats(
                    position.GetX(), position.GetY(), AzFramework::Terrain::TerrainDataRequests::Sampler::EXACT);

                if (spawnerBox.Contains(AZ::Vector3(position.GetX(), position.GetY(), spawnerBox.GetMin().GetZ())))
                {
                    EXPECT_TRUE(heightQueryTerrainExists);
                    EXPECT_FALSE(isHole);
                    EXPECT_FLOAT_EQ(height, spawnerHeight);
                }
                else
                {
                    EXPECT_FALSE(heightQueryTerrainExists);
                    EXPECT_TRUE(isHole);
                }
            }
        }

        // Bounds check for bounds that should and shouldn't have a terrain area inside
        AZ::Aabb boundsCheckCollides = spawnerBox.GetTranslated(AZ::Vector3(5.0f, 5.0f, 5.0f));
        EXPECT_TRUE(terrainSystem->TerrainAreaExistsInBounds(boundsCheckCollides));

        AZ::Aabb boundsCheckDoesNotCollide = spawnerBox.GetTranslated(AZ::Vector3(15.0f, 15.0f, 15.0f));
        EXPECT_FALSE(terrainSystem->TerrainAreaExistsInBounds(boundsCheckDoesNotCollide));
    }

    TEST_F(TerrainSystemTest, TerrainHeightQueriesWithExactSamplersIgnoreQueryGrid)
    {
        // Verify that when using the "EXACT" height sampler, the returned heights come directly from the height provider at the exact
        // requested location, instead of the position being quantized to the height query grid.

        // Create a mock terrain layer spawner that uses a box of (0,0,5) - (10,10,15) and generates a height based on a sine wave
        // using a frequency of 1m and an amplitude of 10m.  i.e. Heights will range between -10 to 10 meters, but will have a value of 0
        // every 0.5 meters.  The sine wave value is based on the absolute X position only, for simplicity.
        constexpr float amplitudeMeters = 10.0f;
        constexpr float frequencyMeters = 1.0f;
        const AZ::Aabb spawnerBox = AZ::Aabb::CreateFromMinMaxValues(0.0f, 0.0f, 5.0f, 10.0f, 10.0f, 15.0f);
        auto entity = CreateAndActivateMockTerrainLayerSpawner(
            spawnerBox,
            [](AZ::Vector3& position, bool& terrainExists)
            {
                position.SetZ(amplitudeMeters * sin(AZ::Constants::TwoPi * (position.GetX() / frequencyMeters)));
                terrainExists = true;
            });

        // Create and activate the terrain system with our testing defaults for world bounds, and a query resolution that exactly matches
        // the frequency of our sine wave.  If our height queries rely on the query resolution, we should always get a value of 0.
        auto terrainSystem = CreateAndActivateTerrainSystem(frequencyMeters);

        // Test an arbitrary set of points that should all produce non-zero heights with the EXACT sampler.  They're not aligned with the
        // query resolution, or with the 0 points on the sine wave.
        const AZ::Vector2 nonZeroPoints[] = { AZ::Vector2(0.3f), AZ::Vector2(2.8f), AZ::Vector2(5.9f), AZ::Vector2(7.7f) };
        for (auto& nonZeroPoint : nonZeroPoints)
        {
            AZ::Vector3 position(nonZeroPoint.GetX(), nonZeroPoint.GetY(), 0.0f);
            bool heightQueryTerrainExists = false;
            float height =
                terrainSystem->GetHeight(position, AzFramework::Terrain::TerrainDataRequests::Sampler::EXACT, &heightQueryTerrainExists);

            // We've chosen a bunch of places on the sine wave that should return a non-zero positive or negative value.
            constexpr float epsilon = 0.0001f;
            EXPECT_GT(fabsf(height), epsilon);
        }

        // Test an arbitrary set of points that should all produce zero heights with the EXACT sampler, since they align with 0 points on
        // the sine wave, regardless of whether or not they align to the query resolution.
        const AZ::Vector2 zeroPoints[] = { AZ::Vector2(0.5f), AZ::Vector2(1.0f), AZ::Vector2(5.0f), AZ::Vector2(7.5f) };
        for (auto& zeroPoint : zeroPoints)
        {
            AZ::Vector3 position(zeroPoint.GetX(), zeroPoint.GetY(), 0.0f);
            bool heightQueryTerrainExists = false;
            float height =
                terrainSystem->GetHeight(position, AzFramework::Terrain::TerrainDataRequests::Sampler::EXACT, &heightQueryTerrainExists);

            constexpr float epsilon = 0.0001f;
            EXPECT_NEAR(height, 0.0f, epsilon);
        }
    }

    TEST_F(TerrainSystemTest, TerrainHeightQueriesWithClampSamplersUseQueryGrid)
    {
        // Verify that when using the "CLAMP" height sampler, the requested location is quantized to the height query grid before fetching
        // the height.

        // Create a mock terrain layer spawner that uses a box of (-10,-10,-5) - (10,10,15) and generates a height equal
        // to the X + Y position, so if either one doesn't get clamped we'll get an unexpected result.
        const AZ::Aabb spawnerBox = AZ::Aabb::CreateFromMinMaxValues(-10.0f, -10.0f, -5.0f, 10.0f, 10.0f, 15.0f);
        auto entity = CreateAndActivateMockTerrainLayerSpawner(
            spawnerBox,
            [](AZ::Vector3& position, bool& terrainExists)
            {
                position.SetZ(position.GetX() + position.GetY());
                terrainExists = true;
            });

        // Create and activate the terrain system with our testing defaults for world bounds, and a query resolution at 0.25 meter
        // intervals.
        const float queryResolution = 0.25f;
        auto terrainSystem = CreateAndActivateTerrainSystem(queryResolution);

        // Test some points and verify that the results always go "downward", whether they're in positive or negative space.
        // (Z contains the the expected result for convenience).
        const HeightTestPoint testPoints[] = {
            { AZ::Vector2(0.0f, 0.0f), 0.0f }, // Should return a height of 0.00 + 0.00
            { AZ::Vector2(0.3f, 0.3f), 0.5f }, // Should return a height of 0.25 + 0.25
            { AZ::Vector2(2.8f, 2.8f), 5.5f }, // Should return a height of 2.75 + 2.75
            { AZ::Vector2(5.5f, 5.5f), 11.0f }, // Should return a height of 5.50 + 5.50
            { AZ::Vector2(7.7f, 7.7f), 15.0f }, // Should return a height of 7.50 + 7.50

            { AZ::Vector2(-0.3f, -0.3f), -1.0f }, // Should return a height of -0.50 + -0.50
            { AZ::Vector2(-2.8f, -2.8f), -6.0f }, // Should return a height of -3.00 + -3.00
            { AZ::Vector2(-5.5f, -5.5f), -11.0f }, // Should return a height of -5.50 + -5.50
            { AZ::Vector2(-7.7f, -7.7f), -15.5f } // Should return a height of -7.75 + -7.75
        };
        for (auto& testPoint : testPoints)
        {
            const float expectedHeight = testPoint.m_expectedHeight;

            AZ::Vector3 position(testPoint.m_testLocation.GetX(), testPoint.m_testLocation.GetY(), 0.0f);
            bool heightQueryTerrainExists = false;
            float height =
                terrainSystem->GetHeight(position, AzFramework::Terrain::TerrainDataRequests::Sampler::CLAMP, &heightQueryTerrainExists);

            constexpr float epsilon = 0.0001f;
            EXPECT_NEAR(height, expectedHeight, epsilon);
        }
    }

    TEST_F(TerrainSystemTest, TerrainHeightQueriesWithBilinearSamplersUseQueryGridToInterpolate)
    {
        // Verify that when using the "BILINEAR" height sampler, the heights are interpolated from points sampled from the query grid.

        // Create a mock terrain layer spawner that uses a box of (-10,-10,-5) - (10,10,15) and generates a height equal
        // to the X + Y position, so we'll have heights that look like this on our grid:
        //   0 *---* 1
        //     |   |
        //   1 *---* 2
        // However, everywhere inside the grid box, we'll generate heights much larger than X + Y.  It will have no effect on exact grid
        // points, but it will noticeably affect the expected height values if any points get sampled in-between grid points.

        const AZ::Aabb spawnerBox = AZ::Aabb::CreateFromMinMaxValues(-10.0f, -10.0f, -5.0f, 10.0f, 10.0f, 15.0f);
        const float amplitudeMeters = 10.0f;
        const float frequencyMeters = 1.0f;
        auto entity = CreateAndActivateMockTerrainLayerSpawner(
            spawnerBox,
            [amplitudeMeters, frequencyMeters](AZ::Vector3& position, bool& terrainExists)
            {
                // Our generated height will be X + Y.
                float expectedHeight = position.GetX() + position.GetY();

                // If either X or Y aren't evenly divisible by the query frequency, add a scaled value to our generated height.
                // This will show up as an unexpected height "spike" if it gets used in any bilinear filter queries.
                float unexpectedVariance =
                    amplitudeMeters * (fmodf(position.GetX(), frequencyMeters) + fmodf(position.GetY(), frequencyMeters));
                position.SetZ(expectedHeight + unexpectedVariance);
                terrainExists = true;
            });

        // Create and activate the terrain system with our testing defaults for world bounds, and a query resolution at 1 meter intervals.
        auto terrainSystem = CreateAndActivateTerrainSystem(frequencyMeters);

        // Test some points and verify that the results are the expected bilinear filtered result,
        // whether they're in positive or negative space.
        // (Z contains the the expected result for convenience).
        const HeightTestPoint testPoints[] = {

            // Queries directly on grid points.  These should return values of X + Y.
            { AZ::Vector2(0.0f, 0.0f), 0.0f }, // Should return a height of 0 + 0
            { AZ::Vector2(1.0f, 0.0f), 1.0f }, // Should return a height of 1 + 0
            { AZ::Vector2(0.0f, 1.0f), 1.0f }, // Should return a height of 0 + 1
            { AZ::Vector2(1.0f, 1.0f), 2.0f }, // Should return a height of 1 + 1
            { AZ::Vector2(3.0f, 5.0f), 8.0f }, // Should return a height of 3 + 5

            { AZ::Vector2(-1.0f, 0.0f), -1.0f }, // Should return a height of -1 + 0
            { AZ::Vector2(0.0f, -1.0f), -1.0f }, // Should return a height of 0 + -1
            { AZ::Vector2(-1.0f, -1.0f), -2.0f }, // Should return a height of -1 + -1
            { AZ::Vector2(-3.0f, -5.0f), -8.0f }, // Should return a height of -3 + -5

            // Queries that are on a grid edge (one axis on the grid, the other somewhere in-between).
            // These should just be a linear interpolation of the points, so it should still be X + Y.

            { AZ::Vector2(0.25f, 0.0f), 0.25f }, // Should return a height of -0.25 + 0
            { AZ::Vector2(3.75f, 0.0f), 3.75f }, // Should return a height of -3.75 + 0
            { AZ::Vector2(0.0f, 0.25f), 0.25f }, // Should return a height of 0 + -0.25
            { AZ::Vector2(0.0f, 3.75f), 3.75f }, // Should return a height of 0 + -3.75

            { AZ::Vector2(2.0f, 3.75f), 5.75f }, // Should return a height of -2 + -3.75
            { AZ::Vector2(2.25f, 4.0f), 6.25f }, // Should return a height of -2.25 + -4

            { AZ::Vector2(-0.25f, 0.0f), -0.25f }, // Should return a height of -0.25 + 0
            { AZ::Vector2(-3.75f, 0.0f), -3.75f }, // Should return a height of -3.75 + 0
            { AZ::Vector2(0.0f, -0.25f), -0.25f }, // Should return a height of 0 + -0.25
            { AZ::Vector2(0.0f, -3.75f), -3.75f }, // Should return a height of 0 + -3.75

            { AZ::Vector2(-2.0f, -3.75f), -5.75f }, // Should return a height of -2 + -3.75
            { AZ::Vector2(-2.25f, -4.0f), -6.25f }, // Should return a height of -2.25 + -4

            // Queries inside a grid square (both axes are in-between grid points)
            // This is a full bilinear interpolation, but because we're using X + Y for our heights, the interpolated values
            // should *still* be X + Y assuming the points were sampled correctly from the grid points.

            { AZ::Vector2(3.25f, 5.25f), 8.5f }, // Should return a height of 3.25 + 5.25
            { AZ::Vector2(7.71f, 9.74f), 17.45f }, // Should return a height of 7.71 + 9.74

            { AZ::Vector2(-3.25f, -5.25f), -8.5f }, // Should return a height of -3.25 + -5.25
            { AZ::Vector2(-7.71f, -9.74f), -17.45f }, // Should return a height of -7.71 + -9.74
        };

        // Loop through every test point and validate it.
        for (auto& testPoint : testPoints)
        {
            const float expectedHeight = testPoint.m_expectedHeight;

            AZ::Vector3 position(testPoint.m_testLocation.GetX(), testPoint.m_testLocation.GetY(), 0.0f);
            bool heightQueryTerrainExists = false;
            float height = terrainSystem->GetHeight(
                position, AzFramework::Terrain::TerrainDataRequests::Sampler::BILINEAR, &heightQueryTerrainExists);

            // Verify that our height query returned the bilinear filtered result we expect.
            constexpr float epsilon = 0.0001f;
            EXPECT_NEAR(height, expectedHeight, epsilon);
        }
    }

    TEST_F(TerrainSystemTest, GetSurfaceWeightsReturnsAllValidSurfaceWeightsInOrder)
    {
        // When there is more than one surface/weight defined, they should all be returned in descending weight order.

        auto terrainSystem = CreateAndActivateTerrainSystem();

        const AZ::Aabb aabb = AZ::Aabb::CreateFromMinMax(AZ::Vector3::CreateZero(), AZ::Vector3::CreateOne());
        auto entity = CreateAndActivateMockTerrainLayerSpawner(
            aabb,
            [](AZ::Vector3& position, bool& terrainExists)
            {
                position.SetZ(1.0f);
                terrainExists = true;
            });

        const AZ::Crc32 tag1("tag1");
        const AZ::Crc32 tag2("tag2");
        const AZ::Crc32 tag3("tag3");
        const float tag1Weight = 0.8f;
        const float tag2Weight = 1.0f;
        const float tag3Weight = 0.5f;

        AzFramework::SurfaceData::SurfaceTagWeightList orderedSurfaceWeights
        {
            { tag1, tag1Weight }, { tag2, tag2Weight }, { tag3, tag3Weight }
        };

        NiceMock<UnitTest::MockTerrainAreaSurfaceRequestBus> mockSurfaceRequests(entity->GetId());
        ON_CALL(mockSurfaceRequests, GetSurfaceWeights).WillByDefault(SetArgReferee<1>(orderedSurfaceWeights));

        AzFramework::SurfaceData::SurfaceTagWeightList outSurfaceWeights;

        // Asking for values outside the layer spawner bounds, should result in no results.
        terrainSystem->GetSurfaceWeights(aabb.GetMax() + AZ::Vector3::CreateOne(), outSurfaceWeights);
        EXPECT_TRUE(outSurfaceWeights.empty());

        // Inside the layer spawner box should give us all of the added surface weights.
        terrainSystem->GetSurfaceWeights(aabb.GetCenter(), outSurfaceWeights);

        EXPECT_EQ(outSurfaceWeights.size(), 3);

        // The weights should be returned in decreasing order.
        AZ::Crc32 expectedCrcList[] = { tag2, tag1, tag3 };
        const float expectedWeightList[] = { tag2Weight, tag1Weight, tag3Weight };

        int index = 0;
        for (const auto& surfaceWeight : outSurfaceWeights)
        {
            EXPECT_EQ(surfaceWeight.m_surfaceType, expectedCrcList[index]);
            EXPECT_NEAR(surfaceWeight.m_weight, expectedWeightList[index], 0.01f);
            index++;
        }
    }

    TEST_F(TerrainSystemTest, GetMaxSurfaceWeightsReturnsBiggestValidSurfaceWeight)
    {
        auto terrainSystem = CreateAndActivateTerrainSystem();

        const AZ::Aabb aabb = AZ::Aabb::CreateFromMinMax(AZ::Vector3::CreateZero(), AZ::Vector3::CreateOne());
        auto entity = CreateAndActivateMockTerrainLayerSpawner(
            aabb,
            [](AZ::Vector3& position, bool& terrainExists)
            {
                position.SetZ(1.0f);
                terrainExists = true;
            });

        const AZ::Crc32 tag1("tag1");
        const AZ::Crc32 tag2("tag2");

        AzFramework::SurfaceData::SurfaceTagWeightList orderedSurfaceWeights;

        AzFramework::SurfaceData::SurfaceTagWeight tagWeight1;
        tagWeight1.m_surfaceType = tag1;
        tagWeight1.m_weight = 1.0f;
        orderedSurfaceWeights.emplace_back(tagWeight1);

        AzFramework::SurfaceData::SurfaceTagWeight tagWeight2;
        tagWeight2.m_surfaceType = tag2;
        tagWeight2.m_weight = 0.8f;
        orderedSurfaceWeights.emplace_back(tagWeight2);

        NiceMock<UnitTest::MockTerrainAreaSurfaceRequestBus> mockSurfaceRequests(entity->GetId());
        ON_CALL(mockSurfaceRequests, GetSurfaceWeights).WillByDefault(SetArgReferee<1>(orderedSurfaceWeights));

        // Asking for values outside the layer spawner bounds, should result in an invalid result.
        AzFramework::SurfaceData::SurfaceTagWeight tagWeight =
            terrainSystem->GetMaxSurfaceWeight(aabb.GetMax() + AZ::Vector3::CreateOne());

        EXPECT_EQ(tagWeight.m_surfaceType, AZ::Crc32(AzFramework::SurfaceData::Constants::UnassignedTagName));

        // Inside the layer spawner box should give us the highest weighted tag (tag1).
        tagWeight = terrainSystem->GetMaxSurfaceWeight(aabb.GetCenter());

        EXPECT_EQ(tagWeight.m_surfaceType, tagWeight1.m_surfaceType);
        EXPECT_NEAR(tagWeight.m_weight, tagWeight1.m_weight, 0.01f);
    }

    TEST_F(TerrainSystemTest, TerrainProcessHeightsFromListWithBilinearSamplers)
    {
        // This repeats the same test as TerrainHeightQueriesWithBilinearSamplersUseQueryGridToInterpolate
        // The difference is that it tests the ProcessHeightsFromList variation.

        const AZ::Aabb spawnerBox = AZ::Aabb::CreateFromMinMaxValues(-10.0f, -10.0f, -5.0f, 10.0f, 10.0f, 15.0f);
        const float amplitudeMeters = 10.0f;
        const float frequencyMeters = 1.0f;
        auto entity = CreateAndActivateMockTerrainLayerSpawner(
            spawnerBox,
            [amplitudeMeters, frequencyMeters](AZ::Vector3& position, bool& terrainExists)
            {
                // Our generated height will be X + Y.
                float expectedHeight = position.GetX() + position.GetY();

                // If either X or Y aren't evenly divisible by the query frequency, add a scaled value to our generated height.
                // This will show up as an unexpected height "spike" if it gets used in any bilinear filter queries.
                float unexpectedVariance =
                    amplitudeMeters * (fmodf(position.GetX(), frequencyMeters) + fmodf(position.GetY(), frequencyMeters));
                position.SetZ(expectedHeight + unexpectedVariance);
                terrainExists = true;
            });

        // Create and activate the terrain system with our testing defaults for world bounds, and a query resolution at 1 meter intervals.
        auto terrainSystem = CreateAndActivateTerrainSystem(frequencyMeters);

        // Test some points and verify that the results are the expected bilinear filtered result,
        // whether they're in positive or negative space.
        // (Z contains the the expected result for convenience).
        const HeightTestPoint testPoints[] = {

            // Queries directly on grid points.  These should return values of X + Y.
            { AZ::Vector2(0.0f, 0.0f), 0.0f }, // Should return a height of 0 + 0
            { AZ::Vector2(1.0f, 0.0f), 1.0f }, // Should return a height of 1 + 0
            { AZ::Vector2(0.0f, 1.0f), 1.0f }, // Should return a height of 0 + 1
            { AZ::Vector2(1.0f, 1.0f), 2.0f }, // Should return a height of 1 + 1
            { AZ::Vector2(3.0f, 5.0f), 8.0f }, // Should return a height of 3 + 5

            { AZ::Vector2(-1.0f, 0.0f), -1.0f }, // Should return a height of -1 + 0
            { AZ::Vector2(0.0f, -1.0f), -1.0f }, // Should return a height of 0 + -1
            { AZ::Vector2(-1.0f, -1.0f), -2.0f }, // Should return a height of -1 + -1
            { AZ::Vector2(-3.0f, -5.0f), -8.0f }, // Should return a height of -3 + -5

            // Queries that are on a grid edge (one axis on the grid, the other somewhere in-between).
            // These should just be a linear interpolation of the points, so it should still be X + Y.

            { AZ::Vector2(0.25f, 0.0f), 0.25f }, // Should return a height of -0.25 + 0
            { AZ::Vector2(3.75f, 0.0f), 3.75f }, // Should return a height of -3.75 + 0
            { AZ::Vector2(0.0f, 0.25f), 0.25f }, // Should return a height of 0 + -0.25
            { AZ::Vector2(0.0f, 3.75f), 3.75f }, // Should return a height of 0 + -3.75

            { AZ::Vector2(2.0f, 3.75f), 5.75f }, // Should return a height of -2 + -3.75
            { AZ::Vector2(2.25f, 4.0f), 6.25f }, // Should return a height of -2.25 + -4

            { AZ::Vector2(-0.25f, 0.0f), -0.25f }, // Should return a height of -0.25 + 0
            { AZ::Vector2(-3.75f, 0.0f), -3.75f }, // Should return a height of -3.75 + 0
            { AZ::Vector2(0.0f, -0.25f), -0.25f }, // Should return a height of 0 + -0.25
            { AZ::Vector2(0.0f, -3.75f), -3.75f }, // Should return a height of 0 + -3.75

            { AZ::Vector2(-2.0f, -3.75f), -5.75f }, // Should return a height of -2 + -3.75
            { AZ::Vector2(-2.25f, -4.0f), -6.25f }, // Should return a height of -2.25 + -4

            // Queries inside a grid square (both axes are in-between grid points)
            // This is a full bilinear interpolation, but because we're using X + Y for our heights, the interpolated values
            // should *still* be X + Y assuming the points were sampled correctly from the grid points.

            { AZ::Vector2(3.25f, 5.25f), 8.5f }, // Should return a height of 3.25 + 5.25
            { AZ::Vector2(7.71f, 9.74f), 17.45f }, // Should return a height of 7.71 + 9.74

            { AZ::Vector2(-3.25f, -5.25f), -8.5f }, // Should return a height of -3.25 + -5.25
            { AZ::Vector2(-7.71f, -9.74f), -17.45f }, // Should return a height of -7.71 + -9.74
        };

        auto perPositionCallback = [&testPoints](const AzFramework::SurfaceData::SurfacePoint& surfacePoint, [[maybe_unused]] bool terrainExists){
            bool found = false;
            for (auto& testPoint : testPoints)
            {
                if (testPoint.m_testLocation.GetX() == surfacePoint.m_position.GetX() && testPoint.m_testLocation.GetY() == surfacePoint.m_position.GetY())
                {
                    constexpr float epsilon = 0.0001f;
                    EXPECT_NEAR(surfacePoint.m_position.GetZ(), testPoint.m_expectedHeight, epsilon);
                    found = true;
                    break;
                }
            }
            EXPECT_EQ(found, true);
        };

        AZStd::vector<AZ::Vector3> inPositions;
        for (auto& testPoint : testPoints)
        {
            AZ::Vector3 position(testPoint.m_testLocation.GetX(), testPoint.m_testLocation.GetY(), 0.0f);
            inPositions.push_back(position);
        }

        terrainSystem->QueryList(
            inPositions, AzFramework::Terrain::TerrainDataRequests::TerrainDataMask::Heights, perPositionCallback,
            AzFramework::Terrain::TerrainDataRequests::Sampler::BILINEAR);
    }

    TEST_F(TerrainSystemTest, TerrainProcessNormalsFromListWithBilinearSamplers)
    {
        // Similar to TerrainProcessHeightsFromListWithBilinearSamplers but for normals

        const AZ::Aabb spawnerBox = AZ::Aabb::CreateFromMinMaxValues(-10.0f, -10.0f, -5.0f, 10.0f, 10.0f, 15.0f);
        const float amplitudeMeters = 10.0f;
        const float frequencyMeters = 1.0f;
        auto entity = CreateAndActivateMockTerrainLayerSpawner(
            spawnerBox,
            [amplitudeMeters, frequencyMeters](AZ::Vector3& position, bool& terrainExists)
            {
                // Our generated height will be X + Y.
                float expectedHeight = position.GetX() + position.GetY();

                // If either X or Y aren't evenly divisible by the query frequency, add a scaled value to our generated height.
                // This will show up as an unexpected height "spike" if it gets used in any bilinear filter queries.
                float unexpectedVariance =
                    amplitudeMeters * (fmodf(position.GetX(), frequencyMeters) + fmodf(position.GetY(), frequencyMeters));
                position.SetZ(expectedHeight + unexpectedVariance);
                terrainExists = true;
            });

        // Create and activate the terrain system with our testing defaults for world bounds, and a query resolution at 1 meter intervals.
        auto terrainSystem = CreateAndActivateTerrainSystem(frequencyMeters);

        const NormalTestPoint testPoints[] = {

            { AZ::Vector2(0.0f, 0.0f), AZ::Vector3(-0.5773f, -0.5773f, 0.5773f) },
            { AZ::Vector2(1.0f, 0.0f), AZ::Vector3(-0.5773f, -0.5773f, 0.5773f) },
            { AZ::Vector2(0.0f, 1.0f), AZ::Vector3(-0.5773f, -0.5773f, 0.5773f) },
            { AZ::Vector2(1.0f, 1.0f), AZ::Vector3(-0.5773f, -0.5773f, 0.5773f) },
            { AZ::Vector2(3.0f, 5.0f), AZ::Vector3(-0.5773f, -0.5773f, 0.5773f) },

            { AZ::Vector2(-1.0f, 0.0f), AZ::Vector3(-0.5773f, -0.5773f, 0.5773f) },
            { AZ::Vector2(0.0f, -1.0f), AZ::Vector3(-0.5773f, -0.5773f, 0.5773f) },
            { AZ::Vector2(-1.0f, -1.0f), AZ::Vector3(-0.5773f, -0.5773f, 0.5773f) },
            { AZ::Vector2(-3.0f, -5.0f), AZ::Vector3(-0.5773f, -0.5773f, 0.5773f) },

            { AZ::Vector2(0.25f, 0.0f), AZ::Vector3(-0.5773f, -0.5773f, 0.5773f) },
            { AZ::Vector2(3.75f, 0.0f), AZ::Vector3(-0.5773f, -0.5773f, 0.5773f) },
            { AZ::Vector2(0.0f, 0.25f), AZ::Vector3(-0.5773f, -0.5773f, 0.5773f) },
            { AZ::Vector2(0.0f, 3.75f), AZ::Vector3(-0.5773f, -0.5773f, 0.5773f) },

            { AZ::Vector2(2.0f, 3.75f), AZ::Vector3(-0.5773f, -0.5773f, 0.5773f) },
            { AZ::Vector2(2.25f, 4.0f), AZ::Vector3(-0.5773f, -0.5773f, 0.5773f) },

            { AZ::Vector2(-0.25f, 0.0f), AZ::Vector3(-0.5773f, -0.5773f, 0.5773f) },
            { AZ::Vector2(-3.75f, 0.0f), AZ::Vector3(-0.5773f, -0.5773f, 0.5773f) },
            { AZ::Vector2(0.0f, -0.25f), AZ::Vector3(-0.5773f, -0.5773f, 0.5773f) },
            { AZ::Vector2(0.0f, -3.75f), AZ::Vector3(-0.5773f, -0.5773f, 0.5773f) },

            { AZ::Vector2(-2.0f, -3.75f), AZ::Vector3(-0.5773f, -0.5773f, 0.5773f) },
            { AZ::Vector2(-2.25f, -4.0f), AZ::Vector3(-0.5773f, -0.5773f, 0.5773f) },

            { AZ::Vector2(3.25f, 5.25f), AZ::Vector3(-0.5773f, -0.5773f, 0.5773f) },
            { AZ::Vector2(7.71f, 9.74f), AZ::Vector3(-0.0292f, 0.9991f, 0.0292f) },

            { AZ::Vector2(-3.25f, -5.25f), AZ::Vector3(-0.5773f, -0.5773f, 0.5773f) },
            { AZ::Vector2(-7.71f, -9.74f), AZ::Vector3(-0.0366f, -0.9986f, 0.0366f) },
        };

        auto perPositionCallback = [&testPoints](const AzFramework::SurfaceData::SurfacePoint& surfacePoint, [[maybe_unused]] bool terrainExists){
            bool found = false;
            for (auto& testPoint : testPoints)
            {
                if (testPoint.m_testLocation.GetX() == surfacePoint.m_position.GetX() && testPoint.m_testLocation.GetY() == surfacePoint.m_position.GetY())
                {
                    constexpr float epsilon = 0.0001f;
                    EXPECT_NEAR(surfacePoint.m_normal.GetX(), testPoint.m_expectedNormal.GetX(), epsilon);
                    EXPECT_NEAR(surfacePoint.m_normal.GetY(), testPoint.m_expectedNormal.GetY(), epsilon);
                    EXPECT_NEAR(surfacePoint.m_normal.GetZ(), testPoint.m_expectedNormal.GetZ(), epsilon);
                    found = true;
                    break;
                }
            }
            EXPECT_EQ(found, true);
        };

        AZStd::vector<AZ::Vector3> inPositions;
        for (auto& testPoint : testPoints)
        {
            AZ::Vector3 position(testPoint.m_testLocation.GetX(), testPoint.m_testLocation.GetY(), 0.0f);
            inPositions.push_back(position);
        }

        terrainSystem->QueryList(
            inPositions, AzFramework::Terrain::TerrainDataRequests::TerrainDataMask::Normals, perPositionCallback,
            AzFramework::Terrain::TerrainDataRequests::Sampler::BILINEAR);
    }

    TEST_F(TerrainSystemTest, TerrainProcessHeightsFromRegionWithBilinearSamplers)
    {
        // This repeats the same test as TerrainHeightQueriesWithBilinearSamplersUseQueryGridToInterpolate
        // The difference is that it tests the ProcessHeightsFromList variation.

        const AZ::Aabb spawnerBox = AZ::Aabb::CreateFromMinMaxValues(-10.0f, -10.0f, -5.0f, 10.0f, 10.0f, 15.0f);
        const float amplitudeMeters = 10.0f;
        const float frequencyMeters = 1.0f;
        auto entity = CreateAndActivateMockTerrainLayerSpawner(
            spawnerBox,
            [amplitudeMeters, frequencyMeters](AZ::Vector3& position, bool& terrainExists)
            {
                // Our generated height will be X + Y.
                float expectedHeight = position.GetX() + position.GetY();

                // If either X or Y aren't evenly divisible by the query frequency, add a scaled value to our generated height.
                // This will show up as an unexpected height "spike" if it gets used in any bilinear filter queries.
                float unexpectedVariance =
                    amplitudeMeters * (fmodf(position.GetX(), frequencyMeters) + fmodf(position.GetY(), frequencyMeters));
                position.SetZ(expectedHeight + unexpectedVariance);
                terrainExists = true;
            });

        // Create and activate the terrain system with our testing defaults for world bounds, and a query resolution at 1 meter intervals.
        auto terrainSystem = CreateAndActivateTerrainSystem(frequencyMeters);

        // Set up a query region that starts at (-1, -1, -1), queries 2 points in the X and Y direction, and uses a step size of 1.0.
        // This should query (-1, -1), (0, -1), (-1, 0), and (0, 0).
        const AZ::Vector2 stepSize(1.0f);
        const AzFramework::Terrain::TerrainQueryRegion queryRegion(AZ::Vector3(-1.0f), 2, 2, stepSize);

        const HeightTestRegionPoints testPoints[] = {
            { 0, 0, -2.0f, AZ::Vector2(-1.0f, -1.0f) },
            { 1, 0, -1.0f, AZ::Vector2(0.0f, -1.0f) },
            { 0, 1, -1.0f, AZ::Vector2(-1.0f, 0.0f) },
            { 1, 1, 0.0f, AZ::Vector2(0.0f, 0.0f) },
        };

        auto perPositionCallback = [&testPoints](size_t xIndex, size_t yIndex,
            const AzFramework::SurfaceData::SurfacePoint& surfacePoint, [[maybe_unused]] bool terrainExists)
        {
            bool found = false;
            for (auto& testPoint : testPoints)
            {
                if (testPoint.m_xIndex == xIndex && testPoint.m_yIndex == yIndex
                    && testPoint.m_testLocation.GetX() == surfacePoint.m_position.GetX()
                    && testPoint.m_testLocation.GetY() == surfacePoint.m_position.GetY())
                {
                    constexpr float epsilon = 0.0001f;
                    EXPECT_NEAR(surfacePoint.m_position.GetZ(), testPoint.m_expectedHeight, epsilon);
                    found = true;
                    break;
                }
            }
            EXPECT_EQ(found, true);
        };

        terrainSystem->QueryRegion(
            queryRegion, AzFramework::Terrain::TerrainDataRequests::TerrainDataMask::Heights, perPositionCallback,
            AzFramework::Terrain::TerrainDataRequests::Sampler::BILINEAR);
    }

    TEST_F(TerrainSystemTest, TerrainProcessNormalsFromRegionWithBilinearSamplers)
    {
        // This repeats the same test as TerrainHeightQueriesWithBilinearSamplersUseQueryGridToInterpolate
        // The difference is that it tests the ProcessHeightsFromList variation.

        const AZ::Aabb spawnerBox = AZ::Aabb::CreateFromMinMaxValues(-10.0f, -10.0f, -5.0f, 10.0f, 10.0f, 15.0f);
        const float amplitudeMeters = 10.0f;
        const float frequencyMeters = 1.0f;
        auto entity = CreateAndActivateMockTerrainLayerSpawner(
            spawnerBox,
            [amplitudeMeters, frequencyMeters](AZ::Vector3& position, bool& terrainExists)
            {
                // Our generated height will be X + Y.
                float expectedHeight = position.GetX() + position.GetY();

                // If either X or Y aren't evenly divisible by the query frequency, add a scaled value to our generated height.
                // This will show up as an unexpected height "spike" if it gets used in any bilinear filter queries.
                float unexpectedVariance =
                    amplitudeMeters * (fmodf(position.GetX(), frequencyMeters) + fmodf(position.GetY(), frequencyMeters));
                position.SetZ(expectedHeight + unexpectedVariance);
                terrainExists = true;
            });

        // Create and activate the terrain system with our testing defaults for world bounds, and a query resolution at 1 meter intervals.
        auto terrainSystem = CreateAndActivateTerrainSystem(frequencyMeters);

        // Set up a query region that starts at (-1, -1, -1), queries 2 points in the X and Y direction, and uses a step size of 1.0.
        // This should query (-1, -1), (0, -1), (-1, 0), and (0, 0).
        const AZ::Vector2 stepSize(1.0f);
        const AzFramework::Terrain::TerrainQueryRegion queryRegion(AZ::Vector3(-1.0f), 2, 2, stepSize);

        const NormalTestRegionPoints testPoints[] = {
            { 0, 0, AZ::Vector3(-0.5773f, -0.5773f, 0.5773f), AZ::Vector2(-1.0f, -1.0f) },
            { 1, 0, AZ::Vector3(-0.5773f, -0.5773f, 0.5773f), AZ::Vector2(0.0f, -1.0f) },
            { 0, 1, AZ::Vector3(-0.5773f, -0.5773f, 0.5773f), AZ::Vector2(-1.0f, 0.0f) },
            { 1, 1, AZ::Vector3(-0.5773f, -0.5773f, 0.5773f), AZ::Vector2(0.0f, 0.0f) },
        };

        auto perPositionCallback = [&testPoints](size_t xIndex, size_t yIndex,
            const AzFramework::SurfaceData::SurfacePoint& surfacePoint, [[maybe_unused]] bool terrainExists)
        {
            bool found = false;
            for (auto& testPoint : testPoints)
            {
                if (testPoint.m_xIndex == xIndex && testPoint.m_yIndex == yIndex
                    && testPoint.m_testLocation.GetX() == surfacePoint.m_position.GetX()
                    && testPoint.m_testLocation.GetY() == surfacePoint.m_position.GetY())
                {
                    constexpr float epsilon = 0.0001f;
                    EXPECT_NEAR(surfacePoint.m_normal.GetX(), testPoint.m_expectedNormal.GetX(), epsilon);
                    EXPECT_NEAR(surfacePoint.m_normal.GetY(), testPoint.m_expectedNormal.GetY(), epsilon);
                    EXPECT_NEAR(surfacePoint.m_normal.GetZ(), testPoint.m_expectedNormal.GetZ(), epsilon);
                    found = true;
                    break;
                }
            }
            EXPECT_EQ(found, true);
        };

        terrainSystem->QueryRegion(
            queryRegion, AzFramework::Terrain::TerrainDataRequests::TerrainDataMask::Normals, perPositionCallback,
            AzFramework::Terrain::TerrainDataRequests::Sampler::BILINEAR);
    }

    TEST_F(TerrainSystemTest, TerrainProcessSurfaceWeightsFromRegion)
    {
        const AZ::Aabb spawnerBox = AZ::Aabb::CreateFromMinMaxValues(-10.0f, -10.0f, -5.0f, 10.0f, 10.0f, 15.0f);
        auto entity = CreateAndActivateMockTerrainLayerSpawner(
            spawnerBox,
            [](AZ::Vector3& position, bool& terrainExists)
            {
                position.SetZ(1.0f);
                terrainExists = true;
            });

        // Create and activate the terrain system with our testing defaults for world bounds, and a query resolution at 1 meter intervals.
        const float queryResolution = 1.0f;
        auto terrainSystem = CreateAndActivateTerrainSystem(queryResolution);

        // Set up a query region that starts at (-3, -3, -1), queries 6 points in the X and Y direction, and uses a step size of 1.0.
        const AZ::Vector2 stepSize(1.0f);
        const AzFramework::Terrain::TerrainQueryRegion queryRegion(AZ::Vector3(-3.0f, -3.0f, -1.0f), 6, 6, stepSize);

        AzFramework::SurfaceData::SurfaceTagWeightList expectedTags;
        SetupSurfaceWeightMocks(entity.get(), expectedTags);

        auto perPositionCallback = [&expectedTags]([[maybe_unused]]  size_t xIndex, [[maybe_unused]] size_t yIndex,
            const AzFramework::SurfaceData::SurfacePoint& surfacePoint, [[maybe_unused]] bool terrainExists)
        {
            constexpr float epsilon = 0.0001f;
            float absYPos = fabsf(surfacePoint.m_position.GetY());
            if (absYPos < 1.0f)
            {
                EXPECT_EQ(surfacePoint.m_surfaceTags[0].m_surfaceType, expectedTags[0].m_surfaceType);
                EXPECT_NEAR(surfacePoint.m_surfaceTags[0].m_weight, expectedTags[0].m_weight, epsilon);
            }
            else if(absYPos < 2.0f)
            {
                EXPECT_EQ(surfacePoint.m_surfaceTags[0].m_surfaceType, expectedTags[1].m_surfaceType);
                EXPECT_NEAR(surfacePoint.m_surfaceTags[0].m_weight, expectedTags[1].m_weight, epsilon);
            }
            else
            {
                EXPECT_EQ(surfacePoint.m_surfaceTags[0].m_surfaceType, expectedTags[2].m_surfaceType);
                EXPECT_NEAR(surfacePoint.m_surfaceTags[0].m_weight, expectedTags[2].m_weight, epsilon);
            }
        };

        terrainSystem->QueryRegion(
            queryRegion, AzFramework::Terrain::TerrainDataRequests::TerrainDataMask::SurfaceData, perPositionCallback,
            AzFramework::Terrain::TerrainDataRequests::Sampler::BILINEAR);
    }

    TEST_F(TerrainSystemTest, TerrainProcessSurfacePointsFromRegion)
    {
        const AZ::Aabb spawnerBox = AZ::Aabb::CreateFromMinMaxValues(-10.0f, -10.0f, -5.0f, 10.0f, 10.0f, 15.0f);
        auto entity = CreateAndActivateMockTerrainLayerSpawner(
            spawnerBox,
            [](AZ::Vector3& position, bool& terrainExists)
            {
                position.SetZ(position.GetX() + position.GetY());
                terrainExists = true;
            });

        // Create and activate the terrain system with our testing defaults for world bounds, and a query resolution at 1 meter intervals.
        const float queryResolution = 1.0f;
        auto terrainSystem = CreateAndActivateTerrainSystem(queryResolution);

        // Set up a query region that starts at (-3, -3, -1), queries 6 points in the X and Y direction, and uses a step size of 1.0.
        const AZ::Vector2 stepSize(1.0f);
        const AzFramework::Terrain::TerrainQueryRegion queryRegion(AZ::Vector3(-3.0f, -3.0f, -1.0f), 6, 6, stepSize);

        AzFramework::SurfaceData::SurfaceTagWeightList expectedTags;
        SetupSurfaceWeightMocks(entity.get(), expectedTags);

        auto perPositionCallback = [&expectedTags]([[maybe_unused]] size_t xIndex, [[maybe_unused]] size_t yIndex,
            const AzFramework::SurfaceData::SurfacePoint& surfacePoint, [[maybe_unused]] bool terrainExists)
        {
            constexpr float epsilon = 0.0001f;
            float expectedHeight = surfacePoint.m_position.GetX() + surfacePoint.m_position.GetY();

            EXPECT_NEAR(surfacePoint.m_position.GetZ(), expectedHeight, epsilon);

            float absYPos = fabsf(surfacePoint.m_position.GetY());
            if (absYPos < 1.0f)
            {
                EXPECT_EQ(surfacePoint.m_surfaceTags[0].m_surfaceType, expectedTags[0].m_surfaceType);
                EXPECT_NEAR(surfacePoint.m_surfaceTags[0].m_weight, expectedTags[0].m_weight, epsilon);
            }
            else if(absYPos < 2.0f)
            {
                EXPECT_EQ(surfacePoint.m_surfaceTags[0].m_surfaceType, expectedTags[1].m_surfaceType);
                EXPECT_NEAR(surfacePoint.m_surfaceTags[0].m_weight, expectedTags[1].m_weight, epsilon);
            }
            else
            {
                EXPECT_EQ(surfacePoint.m_surfaceTags[0].m_surfaceType, expectedTags[2].m_surfaceType);
                EXPECT_NEAR(surfacePoint.m_surfaceTags[0].m_weight, expectedTags[2].m_weight, epsilon);
            }
        };

        terrainSystem->QueryRegion(
            queryRegion, AzFramework::Terrain::TerrainDataRequests::TerrainDataMask::All, perPositionCallback,
            AzFramework::Terrain::TerrainDataRequests::Sampler::EXACT);
    }

    TEST_F(TerrainSystemTest, TerrainProcessAsyncCancellation)
    {
        // Tests cancellation of the asynchronous terrain API.

        const AZ::Aabb spawnerBox = AZ::Aabb::CreateFromMinMaxValues(-10.0f, -10.0f, -5.0f, 10.0f, 10.0f, 15.0f);
        auto entity = CreateAndActivateMockTerrainLayerSpawner(
            spawnerBox,
            [](AZ::Vector3& position, bool& terrainExists)
            {
                // Our generated height will be X + Y.
                position.SetZ(position.GetX() + position.GetY());
                terrainExists = true;
            });

        // Create and activate the terrain system with our testing defaults for world bounds, and a query resolution at 1 meter intervals.
        auto terrainSystem = CreateAndActivateTerrainSystem();

        // Generate some input positions.
        AZStd::vector<AZ::Vector3> inPositions;
        for (int i = 0; i < 16; ++i)
        {
            inPositions.push_back({1.0f, 1.0f, 1.0f});
        }

        // Setup the per position callback so that we can cancel the entire request when it is first invoked.
        AZStd::atomic_bool asyncRequestCancelled = false;
        AZStd::binary_semaphore asyncRequestStartedEvent;
        AZStd::binary_semaphore asyncRequestCancelledEvent;
        auto perPositionCallback = [&asyncRequestCancelled, &asyncRequestStartedEvent, &asyncRequestCancelledEvent]([[maybe_unused]] const AzFramework::SurfaceData::SurfacePoint& surfacePoint, [[maybe_unused]] bool terrainExists)
        {
            if (!asyncRequestCancelled)
            {
                // Indicate that the async request has started.
                asyncRequestStartedEvent.release();

                // Wait until the async request has been cancelled before allowing it to continue.
                asyncRequestCancelledEvent.acquire();
                asyncRequestCancelled = true;
            }
        };

        // Setup the completion callback so we can check that the entire request was cancelled.
        AZStd::semaphore asyncRequestCompletedEvent;
        auto completionCallback = [&asyncRequestCompletedEvent](AZStd::shared_ptr<AzFramework::Terrain::TerrainJobContext> terrainJobContext)
        {
            EXPECT_TRUE(terrainJobContext->IsCancelled());
            asyncRequestCompletedEvent.release();
        };

        // Invoke the async request.
        AZStd::shared_ptr<AzFramework::Terrain::QueryAsyncParams> asyncParams
            = AZStd::make_shared<AzFramework::Terrain::QueryAsyncParams>();
        // Only use one job. We're using a lot of handshaking logic to ensure we process the main thread test logic and the callback logic
        // in the exact order we want for the test, and this logic assumes only one job is running.
        asyncParams->m_desiredNumberOfJobs = 1;
        asyncParams->m_completionCallback = completionCallback;
        AZStd::shared_ptr<AzFramework::Terrain::TerrainJobContext> terrainJobContext = terrainSystem->QueryListAsync(
            inPositions, AzFramework::Terrain::TerrainDataRequests::TerrainDataMask::Heights, perPositionCallback,
            AzFramework::Terrain::TerrainDataRequests::Sampler::BILINEAR, asyncParams);

        // Wait until the async request has started before cancelling it.
        asyncRequestStartedEvent.acquire();
        terrainJobContext->Cancel();
        asyncRequestCancelled = true;
        asyncRequestCancelledEvent.release();

        // Now wait until the async request has completed after being cancelled.
        asyncRequestCompletedEvent.acquire();
    }
} // namespace UnitTest
