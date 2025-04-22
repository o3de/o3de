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
#include <SurfaceData/Utility/SurfaceDataUtility.h>

#include <random>

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

        void TestNormals(const AZStd::span<const NormalTestPoint>& testPoints,
            AzFramework::Terrain::TerrainDataRequests::Sampler sampler, 
            AZStd::function<void(AZ::Vector3&position, bool&terrainExists)> heightMockFunction)
        {
            // Create and activate the terrain system with the same testing defaults for world bounds and query resolutions for
            // all of our normals tests.
            constexpr float queryResolution = 1.0f;
            const AZ::Aabb spawnerBox = AZ::Aabb::CreateFromMinMaxValues(-10.0f, -10.0f, -20.0f, 10.0f, 10.0f, 20.0f);
            auto entity = CreateAndActivateMockTerrainLayerSpawner(spawnerBox, heightMockFunction);
            auto terrainSystem = CreateAndActivateTerrainSystem(queryResolution);

            for (auto& testPoint : testPoints)
            {
                bool terrainExists = false;
                AZ::Vector3 normal = terrainSystem->GetNormal(AZ::Vector3(testPoint.m_testLocation), sampler, &terrainExists);

                EXPECT_TRUE(terrainExists);
                EXPECT_THAT(normal, UnitTest::IsClose(testPoint.m_expectedNormal));
            }
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

        AzFramework::Terrain::FloatRange heightBounds;
        AzFramework::Terrain::TerrainDataRequestBus::BroadcastResult(
            heightBounds, &AzFramework::Terrain::TerrainDataRequestBus::Events::GetTerrainHeightBounds);

        // Create an arbitrary world bounds to test since the bounds of the terrain system will be 0 with no terrain areas.
        AZ::Aabb worldBounds = AZ::Aabb::CreateFromMinMax(AZ::Vector3(-10.0f, -10.0f, -10.0f), AZ::Vector3(10.0f, 10.0f, 10.0f));

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
                EXPECT_FLOAT_EQ(height, heightBounds.m_min);

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
        // TerrainLayerSpawner is defined. The box is min-inclusive-max-exclusive, so points should *not* exist on the max edge of the box.

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

                // Verify that the point either should or shouldn't appear on the box, taking min-inclusive-max-exclusive box ranges
                // into account.
                if (SurfaceData::AabbContains2DMaxExclusive(spawnerBox,
                    AZ::Vector3(position.GetX(), position.GetY(), spawnerBox.GetMin().GetZ())))
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
            { AZ::Vector2(7.7f, 7.7f), 15.5f }, // Should return a height of 7.75 + 7.75

            { AZ::Vector2(-0.3f, -0.3f), -0.5f }, // Should return a height of -0.25 + -0.25
            { AZ::Vector2(-2.8f, -2.8f), -5.5f }, // Should return a height of -2.75 + -2.75
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
            { AZ::Vector2(7.71f, 8.74f), 16.45f }, // Should return a height of 7.71 + 9.74

            { AZ::Vector2(-3.25f, -5.25f), -8.5f }, // Should return a height of -3.25 + -5.25
            { AZ::Vector2(-7.71f, -8.74f), -16.45f }, // Should return a height of -7.71 + -9.74
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
            EXPECT_TRUE(heightQueryTerrainExists);
        }
    }

    TEST_F(TerrainSystemTest, TerrainNormalQueriesWithExactSamplersUseExactHeights)
    {
        // Verify that when using the "EXACT" normal sampler, the normals are calculated from heights immediately around the query
        // point, instead of relying on heights that fall on the query grid.

        // Our height mock function will return 0 on exact grid points, and X everywhere else. If the grid points are used,
        // normals will get all 0 heights which will produce a Z-up normal. If the exact heights are used, the heights will
        // slope up and to the right at a 45 degree angle, so all the normals should point 45 degrees to the left.
        auto heightMockFunction = [](AZ::Vector3& position, bool& terrainExists)
        {
            terrainExists = true;

            if ((AZStd::fmod(position.GetX(), 1.0f) == 0.0f) && (AZStd::fmod(position.GetY(), 1.0f) == 0.0f))
            {
                // Points that fall exactly on grid squares should return 0 for height.
                position.SetZ(0.0f);
            }
            else
            {
                // All other points should return the X value as height.
                position.SetZ(position.GetX());
            }
        };

        // We expect the queries to never use the grid points, so all the normals should point 45 degrees to the left.
        const AZ::Vector3 normalLeft45Degrees =
            AZ::Transform::CreateRotationY(AZ::DegToRad(-45.0f)).TransformVector(AZ::Vector3::CreateAxisZ());

        // Get the normals for an arbitrary set of points that can either fall on or off grid points.
        // These should all produce normals that point 45 degrees to the left, because even for the points that fall on grid points,
        // the height values used for calculating the normals will fall off of the grid points.
        const NormalTestPoint testPoints[] = {
            { AZ::Vector2(0.3f), normalLeft45Degrees }, { AZ::Vector2(1.0f), normalLeft45Degrees },
            { AZ::Vector2(2.8f), normalLeft45Degrees }, { AZ::Vector2(3.0f), normalLeft45Degrees },
            { AZ::Vector2(5.9f), normalLeft45Degrees }, { AZ::Vector2(7.7f), normalLeft45Degrees },
        };

        // Test our normals 
        TestNormals(testPoints, AzFramework::Terrain::TerrainDataRequests::Sampler::EXACT, heightMockFunction);
    }

    TEST_F(TerrainSystemTest, TerrainNormalQueriesWithClampAndBilinearSamplersUseQueryGrid)
    {
        // Verify that when using the "CLAMP" or "BILINEAR" normal samplers, the normals are
        // calculated only from heights that fall on the query grid.

        // Our height mock function will return 0 on exact grid points, and X everywhere else. If the grid points are used,
        // normals will get all 0 heights which will produce a Z-up normal. If the exact heights are used, the heights will
        // slope up and to the right at a 45 degree angle, so all the normals should point 45 degrees to the left.
        auto heightMockFunction = [](AZ::Vector3& position, bool& terrainExists)
        {
            terrainExists = true;

            if ((AZStd::fmod(position.GetX(), 1.0f) == 0.0f) && (AZStd::fmod(position.GetY(), 1.0f) == 0.0f))
            {
                // Points that fall exactly on grid squares should return 0 for height.
                position.SetZ(0.0f);
            }
            else
            {
                // All other points should return the X value as height.
                position.SetZ(position.GetX());
            }
        };

        // We expect the queries to never use the grid points, so all the normals should point directly up.
        const AZ::Vector3 normalUp = AZ::Vector3::CreateAxisZ();

        // Get the normals for an arbitrary set of points that can either fall on or off grid points.
        // These should all produce normals that point directly up, because the height values used for
        // calculating the normals should always come from the grid points.
        const NormalTestPoint testPoints[] = {
            { AZ::Vector2(0.3f), normalUp }, { AZ::Vector2(1.0f), normalUp }, { AZ::Vector2(2.8f), normalUp },
            { AZ::Vector2(3.0f), normalUp }, { AZ::Vector2(5.9f), normalUp }, { AZ::Vector2(7.7f), normalUp },
        };

        // Test our normals with the CLAMP sampler, and make sure they only use the query grid
        TestNormals(testPoints, AzFramework::Terrain::TerrainDataRequests::Sampler::CLAMP, heightMockFunction);

        // Test our normals with the BILINEAR sampler, and make sure they only use the query grid
        TestNormals(testPoints, AzFramework::Terrain::TerrainDataRequests::Sampler::BILINEAR, heightMockFunction);
    }

    TEST_F(TerrainSystemTest, TerrainNormalQueriesWithClampSamplersReturnTriangleNormal)
    {
        // Verify that when using the "CLAMP" normal sampler, the returned normals are the normal of the triangle for whichever
        // half of the terrain grid square the query point falls in.

        // Our height mock function will return 1 on every other X and Y grid value so that every other bottom left triangle
        // will return Z-up and every other top right triangle will return a sloped normal.
        // We'll only pick query points from grid squares where the 1 is the upper right vertex.
        //  0  1  0  1  0
        //  *--*--*--*--*
        //  |\ |\ |\ |\ |
        //  | \| \| \| \|
        //  *--*--*--*--*
        //  0  0  0  0  0
        auto heightMockFunction = [](AZ::Vector3& position, bool& terrainExists)
        {
            terrainExists = true;

            if ((AZStd::fmod(position.GetX(), 2.0f) == 1.0f) && (AZStd::fmod(position.GetY(), 2.0f) == 1.0f))
            {
                // Grid points where X and Y are odd will get a value of 1.0
                // (i.e. (1,1) (3,1) (5,1) (1,3) (3,3) (5,3) etc.
                position.SetZ(1.0f);
            }
            else
            {
                // All other points will return 0.
                position.SetZ(0.0f);
            }
        };

        // The normals in the lower left triangle should point straight up.
        const AZ::Vector3 normalUp = AZ::Vector3::CreateAxisZ();
        // Calculate the rotated normal for the upper right triangle.
        const AZ::Vector3 normalRotated = (AZ::Vector3(1.0f, 1.0f, 1.0f) - AZ::Vector3(0.0f, 1.0f, 0.0f))
                                        .Cross(AZ::Vector3(1.0f, 1.0f, 1.0f) - AZ::Vector3(1.0f, 0.0f, 0.0f)).GetNormalized();

        const NormalTestPoint testPoints[] = {
            // Test points that fall in the upper right triangles.
            { AZ::Vector2(0.6f), normalRotated },
            { AZ::Vector2(2.6f, 0.8f), normalRotated },
            { AZ::Vector2(4.4f, 0.9f), normalRotated },

            // Test points that fall in the lower left triangles.
            { AZ::Vector2(0.3f), normalUp },
            { AZ::Vector2(0.5f, 0.2f), normalUp },
            { AZ::Vector2(2.2f, 0.7f), normalUp },
            { AZ::Vector2(4.4f, 0.1f), normalUp },
        };

        // Test our normals
        TestNormals(testPoints, AzFramework::Terrain::TerrainDataRequests::Sampler::CLAMP, heightMockFunction);
    }

    TEST_F(TerrainSystemTest, TerrainNormalQueriesWithBilinearSamplersReturnPredictableNormalsAtGridPoints)
    {
        // Verify that when using the "BILINEAR" normal sampler, if we query directly on a grid point,
        // we get back a predictable normal for that grid point without needing to consider interpolation.

        // Our height mock will return +X for every even X query point, and -X for every odd X query point.
        // When calculating a normal from heights gathered from a + shape, the normals will always point
        // 45 degrees left when the center is at an odd X value (because the two heights will come from even X values),
        // and they will always point 45 degrees right when the center is at an even X value.
        auto heightMockFunction = [](AZ::Vector3& position, bool& terrainExists)
        {
            terrainExists = true;

            if (AZStd::fmod(position.GetX(), 2.0f) == 0.0f)
            {
                // Return +X for even X points:  (0,0), (0,1), (0,2), (2,0), (2,1), (2,2), etc.
                position.SetZ(position.GetX());
            }
            else
            {
                // Return -X for odd X points:  (1,0), (1,1), (1,2), (3,0), (3,1), (3,2), etc.
                position.SetZ(-position.GetX());
            }
        };

        // The normals should either point left or right 45 degrees.
        const AZ::Vector3 normalLeft45Degrees =
            AZ::Transform::CreateRotationY(AZ::DegToRad(-45.0f)).TransformVector(AZ::Vector3::CreateAxisZ());
        const AZ::Vector3 normalRight45Degrees =
            AZ::Transform::CreateRotationY(AZ::DegToRad(45.0f)).TransformVector(AZ::Vector3::CreateAxisZ());

        const NormalTestPoint testPoints[] = {
            // Test points centered on odd X grid points should point left
            { AZ::Vector2(1.0f, 2.0f), normalLeft45Degrees },
            { AZ::Vector2(3.0f, 0.0f), normalLeft45Degrees },
            { AZ::Vector2(7.0f, 5.0f), normalLeft45Degrees },

            // Test points centered on even X grid points should point right
            { AZ::Vector2(2.0f, 2.0f), normalRight45Degrees },
            { AZ::Vector2(4.0f, 0.0f), normalRight45Degrees },
            { AZ::Vector2(8.0f, 5.0f), normalRight45Degrees },
        };

        // Test our normals
        TestNormals(testPoints, AzFramework::Terrain::TerrainDataRequests::Sampler::BILINEAR, heightMockFunction);
    }

    TEST_F(TerrainSystemTest, TerrainNormalQueriesWithBilinearSamplersReturnInterpolatedNormalsBetweenGridPoints)
    {
        // Verify that when using the "BILINEAR" normal sampler, if we query in-between grid points,
        // we get back a normal that is interpolated between the 4 normals on the corners of that grid square.

        // To test this, we'll set up a 3 x 3 grid of specific heights, and perform all our queries in the center square.
        //          * --- 0 --- 4 --- *  (3,3)
        //
        //          2 --- 2 --- 2 --- 2
        //                   x
        //          0 --- 2 --- 2 --- 0
        //
        //   (0,0)  * --- 2 --- 2 --- *

        // This pattern of heights should give us the following normals at each corner
        //         X -45 deg    X +45 deg    Y -45 deg    Y +45 deg
        //          normal0      normal1      normal2      normal3
        //            2            2            0            4
        //            |            |            |            |
        //        0---*---2    2---*---0    2---*---2    2---*---2
        //            |            |            |            |
        //            2            2            2            2

        auto heightMockFunction = [](AZ::Vector3& position, bool& terrainExists)
        {
            const float heights[4][4] = {
                { 1.0f, 0.0f, 4.0f, 1.0f },
                { 2.0f, 2.0f, 2.0f, 2.0f },
                { 0.0f, 2.0f, 2.0f, 0.0f },
                { 1.0f, 2.0f, 2.0f, 1.0f }
            };

            terrainExists = true;

            uint32_t xIndex = static_cast<uint32_t>(AZStd::fmod(position.GetX(), 4.0f));
            uint32_t yIndex = static_cast<uint32_t>(AZStd::fmod(position.GetY(), 4.0f));

            // We use "3 - y" here so that we can list the heights above in the same order as the picture.
            position.SetZ(heights[3 - yIndex][xIndex]);
        };

        // Create and activate the terrain system with some reasonable defaults for world bounds and query resolutions.
        constexpr float queryResolution = 1.0f;
        const AZ::Aabb spawnerBox = AZ::Aabb::CreateFromMinMaxValues(-10.0f, -10.0f, -20.0f, 10.0f, 10.0f, 20.0f);
        auto entity = CreateAndActivateMockTerrainLayerSpawner(spawnerBox, heightMockFunction);
        auto terrainSystem = CreateAndActivateTerrainSystem(queryResolution);

        // Expected corner normals
        const AZStd::array<AZ::Vector3, 4> expectedCornerNormals =
        {
            AZ::Transform::CreateRotationY(AZ::DegToRad(-45.0f)).TransformVector(AZ::Vector3::CreateAxisZ()),
            AZ::Transform::CreateRotationY(AZ::DegToRad(+45.0f)).TransformVector(AZ::Vector3::CreateAxisZ()),
            AZ::Transform::CreateRotationX(AZ::DegToRad(-45.0f)).TransformVector(AZ::Vector3::CreateAxisZ()),
            AZ::Transform::CreateRotationX(AZ::DegToRad(+45.0f)).TransformVector(AZ::Vector3::CreateAxisZ())
        };

        // Get the normals at the four corners and verify that they match expectations.
        const AZStd::array<AZ::Vector3, 4> cornerPositions = {
            AZ::Vector3(1.0f, 1.0f, 0.0f), AZ::Vector3(2.0f, 1.0f, 0.0f), AZ::Vector3(1.0f, 2.0f, 0.0f), AZ::Vector3(2.0f, 2.0f, 0.0f)
        };
        AZStd::array<AZ::Vector3, 4> cornerNormals;
        AZStd::array<bool, 4> cornerExists;
        for (size_t corner = 0; corner < 4; corner++)
        {
            cornerNormals[corner] = terrainSystem->GetNormal(
                cornerPositions[corner], AzFramework::Terrain::TerrainDataRequests::Sampler::BILINEAR, &(cornerExists[corner]));

            EXPECT_TRUE(cornerExists[corner]);
            EXPECT_THAT(cornerNormals[corner], UnitTest::IsClose(expectedCornerNormals[corner]));
        }

        // Now query a set of points across the terrain grid box and verify that they interpolate correctly.
        for (float y = 0.0f; y <= 1.0f; y += 0.125f)
        {
            for (float x = 0.0f; x <= 1.0f; x += 0.125f)
            {
                AZ::Vector3 queryPoint = cornerPositions[0] + AZ::Vector3(x, y, 0.0f);
                bool terrainExists = false;
                AZ::Vector3 normal = terrainSystem->GetNormal(
                    queryPoint, AzFramework::Terrain::TerrainDataRequests::Sampler::BILINEAR, &terrainExists);

                const AZ::Vector3 normalLerpX0 = cornerNormals[0].Lerp(cornerNormals[1], x);
                const AZ::Vector3 normalLerpX1 = cornerNormals[2].Lerp(cornerNormals[3], x);
                const AZ::Vector3 expectedNormal = normalLerpX0.Lerp(normalLerpX1, y).GetNormalized();

                EXPECT_TRUE(terrainExists);
                EXPECT_THAT(normal, UnitTest::IsClose(expectedNormal));
            }
        }
    }

    TEST_F(TerrainSystemTest, GetSurfaceWeightsReturnsAllValidSurfaceWeightsInOrder)
    {
        // When there is more than one surface/weight defined, they should all be returned in descending weight order.

        auto terrainSystem = CreateAndActivateTerrainSystem();

        const AZ::Aabb aabb = AZ::Aabb::CreateFromMinMax(AZ::Vector3(0.0f), AZ::Vector3(2.0f));
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

        const AZ::Aabb aabb = AZ::Aabb::CreateFromMinMax(AZ::Vector3(0.0f), AZ::Vector3(2.0f));
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

    TEST_F(TerrainSystemTest, GetSurfacePointAndIndividualQueriesProduceSameResults)
    {
        // Verify that the height / normal / surface weights returned from GetSurfacePoint matches the results
        // that we get from individually querying GetHeight, GetNormal, and GetSurfaceWeights.
        // We don't need to validate all combinations because we have separate unit tests that validate equivalent results between
        // the different variations of each individual API. The transitive property means that if those are equal and the results here
        // are equal, then all the combinations will be equal as well.

        // Set up the arbitrary terrain world parameters that we'll use for verifying our queries match.
        const float terrainSize = 32.0f;
        const float terrainQueryResolution = 1.0f;
        const uint32_t terrainNumSurfaces = 3;
        const AZ::Aabb terrainWorldBounds =
            AZ::Aabb::CreateFromMinMax(AZ::Vector3(-terrainSize / 2.0f), AZ::Vector3(terrainSize / 2.0f));

        // Set up the query bounds and step size to use for selecting the points to query and compare.
        const AZ::Aabb queryBounds = terrainWorldBounds;
        const AZ::Vector2 queryStepSize = AZ::Vector2(terrainQueryResolution / 2.0f);

        CreateTestTerrainSystem(terrainWorldBounds, terrainQueryResolution, terrainNumSurfaces);

        for (auto sampler : { AzFramework::Terrain::TerrainDataRequests::Sampler::BILINEAR,
                              AzFramework::Terrain::TerrainDataRequests::Sampler::CLAMP,
                              AzFramework::Terrain::TerrainDataRequests::Sampler::EXACT })
        {
            for (float y = queryBounds.GetMin().GetY(); y < queryBounds.GetMax().GetY(); y += queryStepSize.GetY())
            {
                for (float x = queryBounds.GetMin().GetX(); y < queryBounds.GetMax().GetX(); y += queryStepSize.GetX())
                {
                    AZ::Vector3 queryPosition(x, y, 0.0f);

                    // GetHeight
                    float expectedHeight = terrainWorldBounds.GetMin().GetZ();
                    bool heightExists = false;
                    AzFramework::Terrain::TerrainDataRequestBus::BroadcastResult(
                        expectedHeight, &AzFramework::Terrain::TerrainDataRequests::GetHeight, queryPosition, sampler, &heightExists);

                    // GetNormal
                    AZ::Vector3 expectedNormal = AZ::Vector3::CreateAxisZ();
                    bool normalExists = false;
                    AzFramework::Terrain::TerrainDataRequestBus::BroadcastResult(
                        expectedNormal, &AzFramework::Terrain::TerrainDataRequests::GetNormal, queryPosition, sampler, &normalExists);

                    // GetSurfaceWeights
                    AzFramework::SurfaceData::SurfaceTagWeightList expectedWeights;
                    bool weightsExist = false;
                    AzFramework::Terrain::TerrainDataRequestBus::Broadcast(
                        &AzFramework::Terrain::TerrainDataRequests::GetSurfaceWeights,
                        queryPosition, expectedWeights, sampler, &weightsExist);

                    // GetSurfacePoint
                    AzFramework::SurfaceData::SurfacePoint surfacePoint;
                    bool pointExists = false;
                    AzFramework::Terrain::TerrainDataRequestBus::Broadcast(
                        &AzFramework::Terrain::TerrainDataRequests::GetSurfacePoint, queryPosition, surfacePoint, sampler, &pointExists);

                    // Verify that all the results match.
                    EXPECT_EQ(heightExists, pointExists);
                    EXPECT_EQ(expectedHeight, surfacePoint.m_position.GetZ());
                    EXPECT_THAT(expectedNormal, UnitTest::IsClose(surfacePoint.m_normal));
                    EXPECT_EQ(expectedWeights, surfacePoint.m_surfaceTags);
                }
            }
        }

        DestroyTestTerrainSystem();
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
            { AZ::Vector2(7.71f, 8.74f), 16.45f }, // Should return a height of 7.71 + 8.74
            // We don't test any points > 9.0f because our AABB is max-exclusive, and would query grid points that don't exist for use as
            // a part of the interpolation. We'll test those cases separately, as they're more complex.

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

        // Note that we keep our test points in the range -9.5 to +8.5. Any value outside that range would use points that don't exist
        // in the calculation of the normals, which is more complex and can get tested separately.
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
            { AZ::Vector2(7.71f, 7.74f), AZ::Vector3(-0.5773f, -0.5773f, 0.5773f) },

            { AZ::Vector2(-3.25f, -5.25f), AZ::Vector3(-0.5773f, -0.5773f, 0.5773f) },
            { AZ::Vector2(-7.71f, -7.74f), AZ::Vector3(-0.5773f, -0.5773f, 0.5773f) },
        };

        auto perPositionCallback = [&testPoints](const AzFramework::SurfaceData::SurfacePoint& surfacePoint, bool terrainExists){
            bool found = false;
            for (auto& testPoint : testPoints)
            {
                if (testPoint.m_testLocation.GetX() == surfacePoint.m_position.GetX() && testPoint.m_testLocation.GetY() == surfacePoint.m_position.GetY())
                {
                    constexpr float epsilon = 0.0001f;
                    EXPECT_NEAR(surfacePoint.m_normal.GetX(), testPoint.m_expectedNormal.GetX(), epsilon);
                    EXPECT_NEAR(surfacePoint.m_normal.GetY(), testPoint.m_expectedNormal.GetY(), epsilon);
                    EXPECT_NEAR(surfacePoint.m_normal.GetZ(), testPoint.m_expectedNormal.GetZ(), epsilon);
                    EXPECT_TRUE(terrainExists);
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

    TEST_F(TerrainSystemTest, TerrainGetClosestIntersection)
    {
        // Create a Terrain Spawner with a box from (-200, -200, 0) to (200, 200, 50) that always returns a height of 0.
        // We intentionally match the bottom of the box with the returned height so that the terrain intersection tests need
        // to match intersections that occur on the box surface.
        const AZ::Aabb spawnerBox = AZ::Aabb::CreateFromMinMaxValues(-200.0f, -200.0f, 0.0f, 200.0f, 200.0f, 50.0f);
        auto entity = CreateAndActivateMockTerrainLayerSpawner(
            spawnerBox,
            [](AZ::Vector3& position, bool& terrainExists)
            {
                position.SetZ(0.0f);
                terrainExists = true;
            });

        // Create a random number generator in the -100 to 100 range. We'll use this to generate XY coordinates that
        // always exist within the Terrain Spawner XY dimensions. These are guaranteed to cause an intersection with
        // the terrain as long as we use a +Z value for the start coordinate and a -Z value for the end.
        constexpr unsigned int Seed = 1;
        std::mt19937_64 rng(Seed);
        std::uniform_real_distribution<float> unif(-100.0f, 100.0f);

        // We'll track the total number of intersections failures so that we have a quick reference number to look at if we
        // get spammed with failures.
        int32_t numFailures = 0;

        // Run through a variety of query resolutions to ensure that changing it doesn't break the intersection tests.
        for (float queryResolution : { 0.13f, 0.25f, 0.5f, 1.0f, 2.0f, 3.0f })
        {
            // Create and activate the terrain system.
            auto terrainSystem = CreateAndActivateTerrainSystem(queryResolution);

            // Run through an arbitrary number of random rays and ensure that they all collide with the terrain.
            constexpr uint32_t NumRays = 100;
            for (uint32_t test = 0; test < NumRays; test++)
            {
                // Generate a ray with random XY values in the -100 to 100 range,
                // but with a start Z of 1 to 101 and an end Z of -1 to -101 so that we're guaranteed an intersection for every ray.
                AzFramework::RenderGeometry::RayRequest ray;
                ray.m_startWorldPosition = AZ::Vector3(unif(rng), unif(rng), abs(unif(rng)) + 1.0f);
                ray.m_endWorldPosition = AZ::Vector3(unif(rng), unif(rng), -abs(unif(rng)) - 1.0f);

                // Get our intersection.
                auto result = terrainSystem->GetClosestIntersection(ray);

                // Every ray should intersect at a height of 0 and a normal pointing directly up.
                EXPECT_TRUE(result);
                EXPECT_NEAR(result.m_worldPosition.GetZ(), 0.0f, 0.001f);
                EXPECT_THAT(result.m_worldNormal, IsClose(AZ::Vector3::CreateAxisZ()));

                // Track any intersection failures
                numFailures += (result ? 0 : 1);
            }
        }

        // This is here just to give us a final tally of how many rays failed this test.
        EXPECT_EQ(numFailures, 0);
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
