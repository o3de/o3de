/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/std/parallel/semaphore.h>

#include <AzTest/AzTest.h>

#include <TerrainSystem/TerrainSystem.h>
#include <TerrainTestFixtures.h>

namespace UnitTest::TerrainTest
{
    /* The TerrainBulkQueryTest suite of tests exist to verify that all of our different query APIs produce the same results.
    *  These tests were added after discovering that the async queries could sometimes produce intermittently incorrect results due
    *  to a lack of proper thread safety. However, it's also possible that optimizations to the different queries could accidentally
    *  produce different results as well, so it's good to have this safety net here.
    */

    class TerrainBulkQueryTest
        : public TerrainTestFixture
    {
    protected:

        // Use the ProcessHeightsFromRegion API as our baseline that we'll compare the other query APIs against.
        void GenerateBaselineHeightData(
            const AZ::Aabb& inputQueryRegion,
            const AZ::Vector2& inputQueryStepSize,
            AzFramework::Terrain::TerrainDataRequests::Sampler sampler,
            AZStd::vector<AZ::Vector3>& queryPositions,
            AZStd::vector<AZ::Vector3>& resultPositions,
            AZStd::vector<bool>& resultExistsFlags)
        {
            queryPositions.clear();
            resultPositions.clear();
            resultExistsFlags.clear();

            auto perPositionCallback = [&queryPositions, &resultPositions, &resultExistsFlags](
                [[maybe_unused]] size_t xIndex, [[maybe_unused]] size_t yIndex,
                const AzFramework::SurfaceData::SurfacePoint& surfacePoint, bool terrainExists)
            {
                queryPositions.emplace_back(surfacePoint.m_position.GetX(), surfacePoint.m_position.GetY(), 0.0f);
                resultPositions.emplace_back(surfacePoint.m_position);
                resultExistsFlags.emplace_back(terrainExists);
            };

            AzFramework::Terrain::TerrainQueryRegion queryRegion =
                AzFramework::Terrain::TerrainQueryRegion::CreateFromAabbAndStepSize(inputQueryRegion, inputQueryStepSize);
            AzFramework::Terrain::TerrainDataRequestBus::Broadcast(
                &AzFramework::Terrain::TerrainDataRequests::QueryRegion, queryRegion,
                AzFramework::Terrain::TerrainDataRequests::TerrainDataMask::Heights, perPositionCallback, sampler);

            EXPECT_EQ(queryPositions.size(), resultPositions.size());
        }

        // Compare two sets of output data and verify that they match.
        void ComparePositionData(const AZStd::vector<AZ::Vector3>& baselineValues, const AZStd::vector<bool>& baselineExistsFlags,
            const AZStd::vector<AZ::Vector3>& comparisonValues, const AZStd::vector<bool>& comparisonExistsFlags)
        {
            // Verify that we have the same quantity of results in both sets.
            ASSERT_EQ(baselineValues.size(), comparisonValues.size());

            // Verify that every value is found exactly once in each set. The two sets might not have the values in the same order though,
            // so we need to search for each value, verify it's found, and verify that it hadn't previously been found.
            AZStd::vector<bool> matchFound(baselineValues.size(), false);
            for (size_t comparisonIndex = 0; comparisonIndex < comparisonValues.size(); comparisonIndex++)
            {
                auto foundValue = AZStd::find(baselineValues.begin(), baselineValues.end(), comparisonValues[comparisonIndex]);
                EXPECT_NE(foundValue, baselineValues.end());
                if (foundValue != baselineValues.end())
                {
                    size_t foundIndex = foundValue - baselineValues.begin();
                    EXPECT_FALSE(matchFound[foundIndex]);
                    EXPECT_EQ(baselineExistsFlags[foundIndex], comparisonExistsFlags[comparisonIndex]);
                    matchFound[foundIndex] = true;
                }
            }
        }

        // Use the ProcessNormalsFromRegion API as our baseline that we'll compare the other query APIs against.
        void GenerateBaselineNormalData(
            const AZ::Aabb& inputQueryRegion,
            const AZ::Vector2& inputQueryStepSize,
            AzFramework::Terrain::TerrainDataRequests::Sampler sampler,
            AZStd::vector<AZ::Vector3>& queryPositions,
            AZStd::vector<AZ::Vector3>& resultNormals,
            AZStd::vector<bool>& resultExistsFlags)
        {
            queryPositions.clear();
            resultNormals.clear();
            resultExistsFlags.clear();

            auto perPositionCallback = [&queryPositions, &resultNormals, &resultExistsFlags](
                [[maybe_unused]] size_t xIndex, [[maybe_unused]] size_t yIndex,
                const AzFramework::SurfaceData::SurfacePoint& surfacePoint, bool terrainExists)
            {
                queryPositions.emplace_back(surfacePoint.m_position.GetX(), surfacePoint.m_position.GetY(), 0.0f);
                resultNormals.emplace_back(surfacePoint.m_normal);
                resultExistsFlags.emplace_back(terrainExists);
            };

            AzFramework::Terrain::TerrainQueryRegion queryRegion =
                AzFramework::Terrain::TerrainQueryRegion::CreateFromAabbAndStepSize(inputQueryRegion, inputQueryStepSize);
            AzFramework::Terrain::TerrainDataRequestBus::Broadcast(
                &AzFramework::Terrain::TerrainDataRequests::QueryRegion, queryRegion,
                AzFramework::Terrain::TerrainDataRequests::TerrainDataMask::Normals, perPositionCallback, sampler);

            EXPECT_EQ(queryPositions.size(), resultNormals.size());
        }

        // Compare two sets of output data and verify that they match.
        void CompareNormalData(
            const AZStd::vector<AZ::Vector3>& baselineQueryPositions,
            const AZStd::vector<AZ::Vector3>& baselineValues,
            const AZStd::vector<bool>& baselineExistsFlags,
            const AZStd::vector<AZ::Vector3>& comparisonQueryPositions,
            const AZStd::vector<AZ::Vector3>& comparisonValues,
            const AZStd::vector<bool>& comparisonExistsFlags)
        {
            // Verify that we have the same quantity of results in both sets.
            ASSERT_EQ(baselineValues.size(), comparisonValues.size());

            // Verify that every value is found exactly once in each set. The two sets might not have the values in the same order though,
            // so we need to search for each value, verify it's found, and verify that it hadn't previously been found.
            // Also, since normals are easy to duplicate, we'll search by query positions to find the normals to compare.
            AZStd::vector<bool> matchFound(baselineValues.size(), false);
            for (size_t comparisonIndex = 0; comparisonIndex < comparisonQueryPositions.size(); comparisonIndex++)
            {
                auto& comparisonPosition = comparisonQueryPositions[comparisonIndex];
                auto foundPosition = AZStd::find(baselineQueryPositions.begin(), baselineQueryPositions.end(), comparisonPosition);
                EXPECT_NE(foundPosition, baselineQueryPositions.end());
                if (foundPosition != baselineQueryPositions.end())
                {
                    size_t foundIndex = foundPosition - baselineQueryPositions.begin();
                    EXPECT_FALSE(matchFound[foundIndex]);
                    EXPECT_EQ(baselineValues[foundIndex], comparisonValues[comparisonIndex]);
                    EXPECT_EQ(baselineExistsFlags[foundIndex], comparisonExistsFlags[comparisonIndex]);
                    matchFound[foundIndex] = true;
                }
            }
        }

        // Use the ProcessSurfaceWeightsFromRegion API as our baseline that we'll compare the other query APIs against.
        void GenerateBaselineSurfaceWeightData(
            const AZ::Aabb& inputQueryRegion,
            const AZ::Vector2& inputQueryStepSize,
            AzFramework::Terrain::TerrainDataRequests::Sampler sampler,
            AZStd::vector<AZ::Vector3>& queryPositions,
            AZStd::vector<AzFramework::SurfaceData::SurfaceTagWeightList>& resultWeights)
        {
            queryPositions.clear();
            resultWeights.clear();

            auto perPositionCallback = [&queryPositions, &resultWeights](
                [[maybe_unused]] size_t xIndex, [[maybe_unused]] size_t yIndex,
                const AzFramework::SurfaceData::SurfacePoint& surfacePoint, bool terrainExists)
            {
                queryPositions.emplace_back(surfacePoint.m_position.GetX(), surfacePoint.m_position.GetY(), 0.0f);
                resultWeights.emplace_back(surfacePoint.m_surfaceTags);

                // For these unit tests, we expect every point queried to have valid terrain data.
                EXPECT_TRUE(terrainExists);
            };

            AzFramework::Terrain::TerrainQueryRegion queryRegion =
                AzFramework::Terrain::TerrainQueryRegion::CreateFromAabbAndStepSize(inputQueryRegion, inputQueryStepSize);
            AzFramework::Terrain::TerrainDataRequestBus::Broadcast(
                &AzFramework::Terrain::TerrainDataRequests::QueryRegion, queryRegion,
                AzFramework::Terrain::TerrainDataRequests::TerrainDataMask::SurfaceData, perPositionCallback, sampler);

            EXPECT_EQ(queryPositions.size(), resultWeights.size());
        }

        // Compare two sets of output data and verify that they match.
        void CompareSurfaceWeightData(
            const AZStd::vector<AZ::Vector3>& baselineQueryPositions,
            const AZStd::vector<AzFramework::SurfaceData::SurfaceTagWeightList>& baselineValues,
            const AZStd::vector<AZ::Vector3>& comparisonQueryPositions,
            const AZStd::vector<AzFramework::SurfaceData::SurfaceTagWeightList>& comparisonValues)
        {
            // Verify that we have the same quantity of results in both sets.
            ASSERT_EQ(baselineValues.size(), comparisonValues.size());

            // Verify that every value is found exactly once in each set. The two sets might not have the values in the same order though,
            // so we need to search for each value, verify it's found, and verify that it hadn't previously been found.
            // Also, since surface weight lists are easy to duplicate, we'll search by query positions to find the weight lists to compare.
            AZStd::vector<bool> matchFound(baselineValues.size(), false);
            for (size_t comparisonIndex = 0; comparisonIndex < comparisonQueryPositions.size(); comparisonIndex++)
            {
                auto& comparisonPosition = comparisonQueryPositions[comparisonIndex];
                auto foundPosition = AZStd::find(baselineQueryPositions.begin(), baselineQueryPositions.end(), comparisonPosition);
                EXPECT_NE(foundPosition, baselineQueryPositions.end());
                if (foundPosition != baselineQueryPositions.end())
                {
                    size_t foundIndex = foundPosition - baselineQueryPositions.begin();
                    EXPECT_FALSE(matchFound[foundIndex]);
                    EXPECT_EQ(baselineValues[foundIndex], comparisonValues[comparisonIndex]);
                    matchFound[foundIndex] = true;
                }
            }
        }

        // Use the ProcessSurfacePointsFromRegion API as our baseline that we'll compare the other query APIs against.
        void GenerateBaselineSurfacePointData(
            const AZ::Aabb& inputQueryRegion,
            const AZ::Vector2& inputQueryStepSize,
            AzFramework::Terrain::TerrainDataRequests::Sampler sampler,
            AZStd::vector<AZ::Vector3>& queryPositions,
            AZStd::vector<AzFramework::SurfaceData::SurfacePoint>& resultPoints,
            AZStd::vector<bool>& resultExistsFlags)
        {
            queryPositions.clear();
            resultPoints.clear();
            resultExistsFlags.clear();

            auto perPositionCallback = [&queryPositions, &resultPoints, &resultExistsFlags](
                [[maybe_unused]] size_t xIndex, [[maybe_unused]] size_t yIndex,
                const AzFramework::SurfaceData::SurfacePoint& surfacePoint, bool terrainExists)
            {
                queryPositions.emplace_back(surfacePoint.m_position.GetX(), surfacePoint.m_position.GetY(), 0.0f);
                resultPoints.emplace_back(surfacePoint);
                resultExistsFlags.emplace_back(terrainExists);
            };

            AzFramework::Terrain::TerrainQueryRegion queryRegion =
                AzFramework::Terrain::TerrainQueryRegion::CreateFromAabbAndStepSize(inputQueryRegion, inputQueryStepSize);
            AzFramework::Terrain::TerrainDataRequestBus::Broadcast(
                &AzFramework::Terrain::TerrainDataRequests::QueryRegion, queryRegion,
                AzFramework::Terrain::TerrainDataRequests::TerrainDataMask::All, perPositionCallback, sampler);

            EXPECT_EQ(queryPositions.size(), resultPoints.size());
        }

        // Compare two sets of output data and verify that they match.
        void CompareSurfacePointData(
            const AZStd::vector<AzFramework::SurfaceData::SurfacePoint>& baselineValues,
            const AZStd::vector<bool>& baselineExistsFlags,
            const AZStd::vector<AzFramework::SurfaceData::SurfacePoint>& comparisonValues,
            const AZStd::vector<bool>& comparisonExistsFlags)
        {
            // Verify that we have the same quantity of results in both sets.
            ASSERT_EQ(baselineValues.size(), comparisonValues.size());

            // Verify that every value is found exactly once in each set. The two sets might not have the values in the same order though,
            // so we need to search for each value, verify it's found, and verify that it hadn't previously been found.
            AZStd::vector<bool> matchFound(baselineValues.size(), false);
            for (size_t comparisonIndex = 0; comparisonIndex < comparisonValues.size(); comparisonIndex++)
            {
                const auto& comparisonValue = comparisonValues[comparisonIndex];

                auto foundValue = AZStd::find_if(
                    baselineValues.begin(), baselineValues.end(),
                    [&comparisonValue](const AzFramework::SurfaceData::SurfacePoint& baselineValue) -> bool
                    {
                        return (baselineValue.m_position == comparisonValue.m_position)
                            && (baselineValue.m_normal == comparisonValue.m_normal)
                            && (baselineValue.m_surfaceTags == comparisonValue.m_surfaceTags);
                    });
                EXPECT_NE(foundValue, baselineValues.end());
                if (foundValue != baselineValues.end())
                {
                    size_t foundIndex = foundValue - baselineValues.begin();
                    EXPECT_FALSE(matchFound[foundIndex]);
                    EXPECT_EQ(baselineExistsFlags[foundIndex], comparisonExistsFlags[comparisonIndex]);
                    matchFound[foundIndex] = true;
                    if (baselineExistsFlags[foundIndex] != comparisonExistsFlags[comparisonIndex])
                    {
                        matchFound[foundIndex] = true;
                    }
                }
            }
        }

        AZStd::shared_ptr<AzFramework::Terrain::QueryAsyncParams> CreateTestAsyncParams()
        {
            auto params = AZStd::make_shared<AzFramework::Terrain::QueryAsyncParams>();

            // Set the number of jobs > 1 so that we have parallel queries that execute.
            params->m_desiredNumberOfJobs = 4;
            params->m_completionCallback =
                [this]([[maybe_unused]] AZStd::shared_ptr<AzFramework::Terrain::TerrainJobContext> context)
            {
                // Notify the main test thread that the query has completed.
                m_queryCompletionEvent.release();
            };

            return params;
        }

        // Set up the arbitrary terrain world parameters that we'll use for verifying our queries match.
        const static inline float TerrainSize = 32.0f;
        const static inline float TerrainQueryResolution = 1.0f;
        const static inline uint32_t TerrainNumSurfaces = 3;
        const static inline AZ::Aabb TerrainWorldBounds =
            AZ::Aabb::CreateFromMinMax(AZ::Vector3(-TerrainSize / 2.0f), AZ::Vector3(TerrainSize / 2.0f));

        // Set up the query parameters that we'll use for all our queries
        const static inline AZ::Aabb QueryBounds = TerrainWorldBounds;
        const static inline AZ::Vector2 QueryStepSize = AZ::Vector2(TerrainQueryResolution / 2.0f);
        const static inline uint32_t ExpectedResultCount =
            aznumeric_cast<uint32_t>((TerrainSize / QueryStepSize.GetX()) * (TerrainSize / QueryStepSize.GetY()));

        // Semaphore for use in async tests.
        AZStd::binary_semaphore m_queryCompletionEvent;
    };

    // -----------------------------------------------------------------------------
    // Compare Height Query APIs

    TEST_F(TerrainBulkQueryTest, ProcessHeightsFromRegionAndProcessHeightsFromListProduceSameResults)
    {
        CreateTestTerrainSystem(TerrainWorldBounds, TerrainQueryResolution, TerrainNumSurfaces);

        for (auto sampler : {
                AzFramework::Terrain::TerrainDataRequests::Sampler::BILINEAR,
                AzFramework::Terrain::TerrainDataRequests::Sampler::CLAMP,
                AzFramework::Terrain::TerrainDataRequests::Sampler::EXACT
             })
        {
            // Gather all our initial results from calling Process*FromRegion
            AZStd::vector<AZ::Vector3> queryPositions;
            AZStd::vector<AZ::Vector3> baselineResultPositions;
            AZStd::vector<bool> baselineExistsFlags;
            GenerateBaselineHeightData(QueryBounds, QueryStepSize, sampler, queryPositions, baselineResultPositions, baselineExistsFlags);
            ASSERT_EQ(queryPositions.size(), ExpectedResultCount);

            // Gather results from Process*FromList
            AZStd::vector<AZ::Vector3> comparisonResultPositions;
            AZStd::vector<bool> comparisonExistsFlags;

            auto listPositionCallback = [&comparisonResultPositions, &comparisonExistsFlags]
                (const AzFramework::SurfaceData::SurfacePoint& surfacePoint, bool terrainExists)
            {
                comparisonResultPositions.emplace_back(surfacePoint.m_position);
                comparisonExistsFlags.emplace_back(terrainExists);
            };

            AzFramework::Terrain::TerrainDataRequestBus::Broadcast(
                &AzFramework::Terrain::TerrainDataRequests::QueryList,
                queryPositions, AzFramework::Terrain::TerrainDataRequests::TerrainDataMask::Heights, listPositionCallback, sampler);

            // Compare the results
            ComparePositionData(baselineResultPositions, baselineExistsFlags, comparisonResultPositions, comparisonExistsFlags);
        }

        DestroyTestTerrainSystem();
    }

    TEST_F(TerrainBulkQueryTest, ProcessHeightsFromRegionAndGetHeightProduceSameResults)
    {
        CreateTestTerrainSystem(TerrainWorldBounds, TerrainQueryResolution, TerrainNumSurfaces);

        for (auto sampler :
             { AzFramework::Terrain::TerrainDataRequests::Sampler::BILINEAR, AzFramework::Terrain::TerrainDataRequests::Sampler::CLAMP,
               AzFramework::Terrain::TerrainDataRequests::Sampler::EXACT })
        {
            // Gather all our initial results from calling Process*FromRegion
            AZStd::vector<AZ::Vector3> queryPositions;
            AZStd::vector<AZ::Vector3> baselineResultPositions;
            AZStd::vector<bool> baselineExistsFlags;
            GenerateBaselineHeightData(QueryBounds, QueryStepSize, sampler, queryPositions, baselineResultPositions, baselineExistsFlags);
            ASSERT_EQ(queryPositions.size(), ExpectedResultCount);

            // Gather results from Get*
            AZStd::vector<AZ::Vector3> comparisonResultPositions;
            AZStd::vector<bool> comparisonExistsFlags;
            float worldMinZ = TerrainWorldBounds.GetMin().GetZ();
            for (auto& position : queryPositions)
            {
                float terrainHeight = worldMinZ;
                bool terrainExists = false;
                AzFramework::Terrain::TerrainDataRequestBus::BroadcastResult(
                    terrainHeight, &AzFramework::Terrain::TerrainDataRequests::GetHeight, position, sampler, &terrainExists);

                comparisonResultPositions.emplace_back(position.GetX(), position.GetY(), terrainHeight);
                comparisonExistsFlags.emplace_back(terrainExists);
            }

            // Compare the results
            ComparePositionData(baselineResultPositions, baselineExistsFlags, comparisonResultPositions, comparisonExistsFlags);
        }

        DestroyTestTerrainSystem();
    }

    TEST_F(TerrainBulkQueryTest, ProcessHeightsFromRegionAndProcessHeightsFromRegionAsyncProduceSameResults)
    {
        CreateTestTerrainSystem(TerrainWorldBounds, TerrainQueryResolution, TerrainNumSurfaces);

        for (auto sampler :
             { AzFramework::Terrain::TerrainDataRequests::Sampler::BILINEAR, AzFramework::Terrain::TerrainDataRequests::Sampler::CLAMP,
               AzFramework::Terrain::TerrainDataRequests::Sampler::EXACT })
        {
            // Gather all our initial results from calling Process*FromRegion
            AZStd::vector<AZ::Vector3> queryPositions;
            AZStd::vector<AZ::Vector3> baselineResultPositions;
            AZStd::vector<bool> baselineExistsFlags;
            GenerateBaselineHeightData(QueryBounds, QueryStepSize, sampler, queryPositions, baselineResultPositions, baselineExistsFlags);
            ASSERT_EQ(queryPositions.size(), ExpectedResultCount);

            // Gather results from Process*FromRegionAsync
            AZStd::vector<AZ::Vector3> comparisonResultPositions;
            AZStd::vector<bool> comparisonExistsFlags;
            AZStd::mutex outputMutex;

            auto regionPositionCallback = [&comparisonResultPositions, &comparisonExistsFlags, &outputMutex](
                [[maybe_unused]] size_t xIndex, [[maybe_unused]] size_t yIndex,
                const AzFramework::SurfaceData::SurfacePoint& surfacePoint, bool terrainExists)
            {
                // Make sure only one thread can add its result at a time.
                AZStd::scoped_lock lock(outputMutex);
                comparisonResultPositions.emplace_back(surfacePoint.m_position);
                comparisonExistsFlags.emplace_back(terrainExists);
            };

            auto params = CreateTestAsyncParams();
            AZStd::shared_ptr<AzFramework::Terrain::TerrainJobContext> jobContext;
            AzFramework::Terrain::TerrainQueryRegion queryRegion =
                AzFramework::Terrain::TerrainQueryRegion::CreateFromAabbAndStepSize(QueryBounds, QueryStepSize);
            AzFramework::Terrain::TerrainDataRequestBus::BroadcastResult(
                jobContext, &AzFramework::Terrain::TerrainDataRequests::QueryRegionAsync, queryRegion,
                AzFramework::Terrain::TerrainDataRequests::TerrainDataMask::Heights, regionPositionCallback, sampler, params);

            // Wait for the async query to complete
            m_queryCompletionEvent.acquire();

            // Compare the results
            ComparePositionData(baselineResultPositions, baselineExistsFlags, comparisonResultPositions, comparisonExistsFlags);
        }

        DestroyTestTerrainSystem();
    }

    TEST_F(TerrainBulkQueryTest, ProcessHeightsFromRegionAndProcessHeightsFromListAsyncProduceSameResults)
    {
        CreateTestTerrainSystem(TerrainWorldBounds, TerrainQueryResolution, TerrainNumSurfaces);

        for (auto sampler :
             { AzFramework::Terrain::TerrainDataRequests::Sampler::BILINEAR, AzFramework::Terrain::TerrainDataRequests::Sampler::CLAMP,
               AzFramework::Terrain::TerrainDataRequests::Sampler::EXACT })
        {
            // Gather all our initial results from calling Process*FromRegion
            AZStd::vector<AZ::Vector3> queryPositions;
            AZStd::vector<AZ::Vector3> baselineResultPositions;
            AZStd::vector<bool> baselineExistsFlags;
            GenerateBaselineHeightData(QueryBounds, QueryStepSize, sampler, queryPositions, baselineResultPositions, baselineExistsFlags);
            ASSERT_EQ(queryPositions.size(), ExpectedResultCount);

            // Gather results from Process*FromListAsync
            AZStd::vector<AZ::Vector3> comparisonResultPositions;
            AZStd::vector<bool> comparisonExistsFlags;
            AZStd::mutex outputMutex;

            auto listPositionCallback = [&comparisonResultPositions, &comparisonExistsFlags, &outputMutex](
                const AzFramework::SurfaceData::SurfacePoint& surfacePoint, bool terrainExists)
            {
                // Make sure only one thread can add its result at a time.
                AZStd::scoped_lock lock(outputMutex);
                comparisonResultPositions.emplace_back(surfacePoint.m_position);
                comparisonExistsFlags.emplace_back(terrainExists);
            };

            auto params = CreateTestAsyncParams();
            AZStd::shared_ptr<AzFramework::Terrain::TerrainJobContext> jobContext;
            AzFramework::Terrain::TerrainDataRequestBus::BroadcastResult(
                jobContext, &AzFramework::Terrain::TerrainDataRequests::QueryListAsync, queryPositions,
                AzFramework::Terrain::TerrainDataRequests::TerrainDataMask::Heights,
                listPositionCallback, sampler, params);

            // Wait for the async query to complete
            m_queryCompletionEvent.acquire();

            // Compare the results
            ComparePositionData(baselineResultPositions, baselineExistsFlags, comparisonResultPositions, comparisonExistsFlags);
        }

        DestroyTestTerrainSystem();
    }

    // -----------------------------------------------------------------------------
    // Compare Normal Query APIs

    TEST_F(TerrainBulkQueryTest, ProcessNormalsFromRegionAndProcessNormalsFromListProduceSameResults)
    {
        CreateTestTerrainSystem(TerrainWorldBounds, TerrainQueryResolution, TerrainNumSurfaces);

        for (auto sampler :
             { AzFramework::Terrain::TerrainDataRequests::Sampler::BILINEAR, AzFramework::Terrain::TerrainDataRequests::Sampler::CLAMP,
               AzFramework::Terrain::TerrainDataRequests::Sampler::EXACT })
        {
            // Gather all our initial results from calling Process*FromRegion
            AZStd::vector<AZ::Vector3> queryPositions;
            AZStd::vector<AZ::Vector3> baselineResultNormals;
            AZStd::vector<bool> baselineExistsFlags;
            GenerateBaselineNormalData(QueryBounds, QueryStepSize, sampler, queryPositions, baselineResultNormals, baselineExistsFlags);
            ASSERT_EQ(queryPositions.size(), ExpectedResultCount);

            // Gather results from Process*FromList
            AZStd::vector<AZ::Vector3> comparisonResultPositions;
            AZStd::vector<AZ::Vector3> comparisonResultNormals;
            AZStd::vector<bool> comparisonExistsFlags;

            auto listNormalCallback = [&comparisonResultPositions, &comparisonResultNormals, &comparisonExistsFlags](
                const AzFramework::SurfaceData::SurfacePoint& surfacePoint, bool terrainExists)
            {
                comparisonResultPositions.emplace_back(surfacePoint.m_position);
                comparisonResultNormals.emplace_back(surfacePoint.m_normal);
                comparisonExistsFlags.emplace_back(terrainExists);
            };

            AzFramework::Terrain::TerrainDataRequestBus::Broadcast(
                &AzFramework::Terrain::TerrainDataRequests::QueryList, queryPositions,
                AzFramework::Terrain::TerrainDataRequests::TerrainDataMask::Normals, listNormalCallback, sampler);

            // Compare the results
            CompareNormalData(queryPositions, baselineResultNormals, baselineExistsFlags,
                comparisonResultPositions, comparisonResultNormals, comparisonExistsFlags);
        }

        DestroyTestTerrainSystem();
    }

    TEST_F(TerrainBulkQueryTest, ProcessNormalsFromRegionAndGetNormalProduceSameResults)
    {
        CreateTestTerrainSystem(TerrainWorldBounds, TerrainQueryResolution, TerrainNumSurfaces);

        for (auto sampler :
             { AzFramework::Terrain::TerrainDataRequests::Sampler::BILINEAR, AzFramework::Terrain::TerrainDataRequests::Sampler::CLAMP,
               AzFramework::Terrain::TerrainDataRequests::Sampler::EXACT })
        {
            // Gather all our initial results from calling Process*FromRegion
            AZStd::vector<AZ::Vector3> queryPositions;
            AZStd::vector<AZ::Vector3> baselineResultNormals;
            AZStd::vector<bool> baselineExistsFlags;
            GenerateBaselineNormalData(QueryBounds, QueryStepSize, sampler, queryPositions, baselineResultNormals, baselineExistsFlags);
            ASSERT_EQ(queryPositions.size(), ExpectedResultCount);

            // Gather results from Get*
            AZStd::vector<AZ::Vector3> comparisonResultNormals;
            AZStd::vector<bool> comparisonExistsFlags;
            for (auto& position : queryPositions)
            {
                AZ::Vector3 terrainNormal = AZ::Vector3::CreateZero();
                bool terrainExists = false;
                AzFramework::Terrain::TerrainDataRequestBus::BroadcastResult(
                    terrainNormal, &AzFramework::Terrain::TerrainDataRequests::GetNormal, position, sampler, &terrainExists);

                comparisonResultNormals.emplace_back(terrainNormal);
                comparisonExistsFlags.emplace_back(terrainExists);
            }

            // Compare the results
            CompareNormalData(queryPositions, baselineResultNormals, baselineExistsFlags,
                queryPositions, comparisonResultNormals, comparisonExistsFlags);
        }

        DestroyTestTerrainSystem();
    }

    TEST_F(TerrainBulkQueryTest, ProcessNormalsFromRegionAndProcessNormalsFromRegionAsyncProduceSameResults)
    {
        CreateTestTerrainSystem(TerrainWorldBounds, TerrainQueryResolution, TerrainNumSurfaces);

        for (auto sampler :
             { AzFramework::Terrain::TerrainDataRequests::Sampler::BILINEAR, AzFramework::Terrain::TerrainDataRequests::Sampler::CLAMP,
               AzFramework::Terrain::TerrainDataRequests::Sampler::EXACT })
        {
            // Gather all our initial results from calling Process*FromRegion
            AZStd::vector<AZ::Vector3> queryPositions;
            AZStd::vector<AZ::Vector3> baselineResultNormals;
            AZStd::vector<bool> baselineExistsFlags;
            GenerateBaselineNormalData(QueryBounds, QueryStepSize, sampler, queryPositions, baselineResultNormals, baselineExistsFlags);
            ASSERT_EQ(queryPositions.size(), ExpectedResultCount);

            // Gather results from Process*FromRegionAsync
            AZStd::vector<AZ::Vector3> comparisonResultPositions;
            AZStd::vector<AZ::Vector3> comparisonResultNormals;
            AZStd::vector<bool> comparisonExistsFlags;
            AZStd::mutex outputMutex;

            auto regionPositionCallback = [&comparisonResultPositions, &comparisonResultNormals, &comparisonExistsFlags, &outputMutex](
                [[maybe_unused]] size_t xIndex, [[maybe_unused]] size_t yIndex,
                const AzFramework::SurfaceData::SurfacePoint& surfacePoint, bool terrainExists)
            {
                // Make sure only one thread can add its result at a time.
                AZStd::scoped_lock lock(outputMutex);
                comparisonResultPositions.emplace_back(surfacePoint.m_position);
                comparisonResultNormals.emplace_back(surfacePoint.m_normal);
                comparisonExistsFlags.emplace_back(terrainExists);
            };

            auto params = CreateTestAsyncParams();
            AZStd::shared_ptr<AzFramework::Terrain::TerrainJobContext> jobContext;
            AzFramework::Terrain::TerrainQueryRegion queryRegion =
                AzFramework::Terrain::TerrainQueryRegion::CreateFromAabbAndStepSize(QueryBounds, QueryStepSize);
            AzFramework::Terrain::TerrainDataRequestBus::BroadcastResult(
                jobContext, &AzFramework::Terrain::TerrainDataRequests::QueryRegionAsync, queryRegion,
                AzFramework::Terrain::TerrainDataRequests::TerrainDataMask::Normals, regionPositionCallback, sampler, params);

            // Wait for the async query to complete
            m_queryCompletionEvent.acquire();

            // Compare the results
            CompareNormalData(queryPositions, baselineResultNormals, baselineExistsFlags,
                comparisonResultPositions, comparisonResultNormals, comparisonExistsFlags);
        }

        DestroyTestTerrainSystem();
    }

    TEST_F(TerrainBulkQueryTest, ProcessNormalsFromRegionAndProcessNormalsFromListAsyncProduceSameResults)
    {
        CreateTestTerrainSystem(TerrainWorldBounds, TerrainQueryResolution, TerrainNumSurfaces);

        for (auto sampler :
             { AzFramework::Terrain::TerrainDataRequests::Sampler::BILINEAR, AzFramework::Terrain::TerrainDataRequests::Sampler::CLAMP,
               AzFramework::Terrain::TerrainDataRequests::Sampler::EXACT })
        {
            // Gather all our initial results from calling Process*FromRegion
            AZStd::vector<AZ::Vector3> queryPositions;
            AZStd::vector<AZ::Vector3> baselineResultNormals;
            AZStd::vector<bool> baselineExistsFlags;
            GenerateBaselineNormalData(QueryBounds, QueryStepSize, sampler, queryPositions, baselineResultNormals, baselineExistsFlags);
            ASSERT_EQ(queryPositions.size(), ExpectedResultCount);

            // Gather results from Process*FromListAsync
            AZStd::vector<AZ::Vector3> comparisonResultPositions;
            AZStd::vector<AZ::Vector3> comparisonResultNormals;
            AZStd::vector<bool> comparisonExistsFlags;
            AZStd::mutex outputMutex;

            auto listPositionCallback = [&comparisonResultPositions, &comparisonResultNormals, &comparisonExistsFlags, &outputMutex](
                const AzFramework::SurfaceData::SurfacePoint& surfacePoint, bool terrainExists)
            {
                // Make sure only one thread can add its result at a time.
                AZStd::scoped_lock lock(outputMutex);
                comparisonResultPositions.emplace_back(surfacePoint.m_position);
                comparisonResultNormals.emplace_back(surfacePoint.m_normal);
                comparisonExistsFlags.emplace_back(terrainExists);
            };

            auto params = CreateTestAsyncParams();
            AZStd::shared_ptr<AzFramework::Terrain::TerrainJobContext> jobContext;
            AzFramework::Terrain::TerrainDataRequestBus::BroadcastResult(
                jobContext, &AzFramework::Terrain::TerrainDataRequests::QueryListAsync, queryPositions,
                AzFramework::Terrain::TerrainDataRequests::TerrainDataMask::Normals, listPositionCallback,
                sampler, params);

            // Wait for the async query to complete
            m_queryCompletionEvent.acquire();

            // Compare the results
            CompareNormalData(queryPositions, baselineResultNormals, baselineExistsFlags,
                comparisonResultPositions, comparisonResultNormals, comparisonExistsFlags);
        }

        DestroyTestTerrainSystem();
    }

    // -----------------------------------------------------------------------------
    // Compare Surface Weight Query APIs

    TEST_F(TerrainBulkQueryTest, ProcessSurfaceWeightsFromRegionAndProcessSurfaceWeightsFromListProduceSameResults)
    {
        CreateTestTerrainSystem(TerrainWorldBounds, TerrainQueryResolution, TerrainNumSurfaces);

        for (auto sampler :
             { AzFramework::Terrain::TerrainDataRequests::Sampler::BILINEAR, AzFramework::Terrain::TerrainDataRequests::Sampler::CLAMP,
               AzFramework::Terrain::TerrainDataRequests::Sampler::EXACT })
        {
            // Gather all our initial results from calling Process*FromRegion
            AZStd::vector<AZ::Vector3> queryPositions;
            AZStd::vector<AzFramework::SurfaceData::SurfaceTagWeightList> baselineResultWeights;
            GenerateBaselineSurfaceWeightData(QueryBounds, QueryStepSize, sampler, queryPositions, baselineResultWeights);
            ASSERT_EQ(queryPositions.size(), ExpectedResultCount);

            // Gather results from Process*FromList
            AZStd::vector<AZ::Vector3> comparisonResultPositions;
            AZStd::vector<AzFramework::SurfaceData::SurfaceTagWeightList> comparisonResultWeights;

            auto listWeightsCallback = [&comparisonResultPositions, &comparisonResultWeights](
                const AzFramework::SurfaceData::SurfacePoint& surfacePoint, bool terrainExists)
            {
                comparisonResultPositions.emplace_back(surfacePoint.m_position);
                comparisonResultWeights.emplace_back(surfacePoint.m_surfaceTags);
                EXPECT_TRUE(terrainExists);
            };

            AzFramework::Terrain::TerrainDataRequestBus::Broadcast(
                &AzFramework::Terrain::TerrainDataRequests::QueryList, queryPositions,
                AzFramework::Terrain::TerrainDataRequests::TerrainDataMask::SurfaceData, listWeightsCallback, sampler);

            // Compare the results
            CompareSurfaceWeightData(queryPositions, baselineResultWeights, comparisonResultPositions, comparisonResultWeights);
        }

        DestroyTestTerrainSystem();
    }

    TEST_F(TerrainBulkQueryTest, ProcessSurfaceWeightsFromRegionAndGetSurfaceWeightsProduceSameResults)
    {
        CreateTestTerrainSystem(TerrainWorldBounds, TerrainQueryResolution, TerrainNumSurfaces);

        for (auto sampler :
             { AzFramework::Terrain::TerrainDataRequests::Sampler::BILINEAR, AzFramework::Terrain::TerrainDataRequests::Sampler::CLAMP,
               AzFramework::Terrain::TerrainDataRequests::Sampler::EXACT })
        {
            // Gather all our initial results from calling Process*FromRegion
            AZStd::vector<AZ::Vector3> queryPositions;
            AZStd::vector<AzFramework::SurfaceData::SurfaceTagWeightList> baselineResultWeights;
            GenerateBaselineSurfaceWeightData(QueryBounds, QueryStepSize, sampler, queryPositions, baselineResultWeights);
            ASSERT_EQ(queryPositions.size(), ExpectedResultCount);

            // Gather results from Get*
            AZStd::vector<AzFramework::SurfaceData::SurfaceTagWeightList> comparisonResultWeights;
            AzFramework::SurfaceData::SurfaceTagWeightList terrainWeights;
            for (auto& position : queryPositions)
            {
                bool terrainExists = false;
                AzFramework::Terrain::TerrainDataRequestBus::Broadcast(
                    &AzFramework::Terrain::TerrainDataRequests::GetSurfaceWeights, position, terrainWeights, sampler, &terrainExists);

                comparisonResultWeights.emplace_back(terrainWeights);
                EXPECT_TRUE(terrainExists);
            }

            // Compare the results
            CompareSurfaceWeightData(queryPositions, baselineResultWeights, queryPositions, comparisonResultWeights);
        }

        DestroyTestTerrainSystem();
    }

    TEST_F(TerrainBulkQueryTest, ProcessSurfaceWeightsFromRegionAndProcessSurfaceWeightsFromRegionAsyncProduceSameResults)
    {
        CreateTestTerrainSystem(TerrainWorldBounds, TerrainQueryResolution, TerrainNumSurfaces);

        for (auto sampler :
             { AzFramework::Terrain::TerrainDataRequests::Sampler::BILINEAR, AzFramework::Terrain::TerrainDataRequests::Sampler::CLAMP,
               AzFramework::Terrain::TerrainDataRequests::Sampler::EXACT })
        {
            // Gather all our initial results from calling Process*FromRegion
            AZStd::vector<AZ::Vector3> queryPositions;
            AZStd::vector<AzFramework::SurfaceData::SurfaceTagWeightList> baselineResultWeights;
            GenerateBaselineSurfaceWeightData(QueryBounds, QueryStepSize, sampler, queryPositions, baselineResultWeights);
            ASSERT_EQ(queryPositions.size(), ExpectedResultCount);

            // Gather results from Process*FromRegionAsync
            AZStd::vector<AZ::Vector3> comparisonResultPositions;
            AZStd::vector<AzFramework::SurfaceData::SurfaceTagWeightList> comparisonResultWeights;
            AZStd::mutex outputMutex;

            auto perPositionCallback = [&comparisonResultPositions, &comparisonResultWeights, &outputMutex](
                [[maybe_unused]] size_t xIndex, [[maybe_unused]] size_t yIndex,
                const AzFramework::SurfaceData::SurfacePoint& surfacePoint, bool terrainExists)
            {
                // Make sure only one thread can add its result at a time.
                AZStd::scoped_lock lock(outputMutex);
                comparisonResultPositions.emplace_back(surfacePoint.m_position);
                comparisonResultWeights.emplace_back(surfacePoint.m_surfaceTags);
                EXPECT_TRUE(terrainExists);
            };

            auto params = CreateTestAsyncParams();
            AZStd::shared_ptr<AzFramework::Terrain::TerrainJobContext> jobContext;
            AzFramework::Terrain::TerrainQueryRegion queryRegion =
                AzFramework::Terrain::TerrainQueryRegion::CreateFromAabbAndStepSize(QueryBounds, QueryStepSize);
            AzFramework::Terrain::TerrainDataRequestBus::BroadcastResult(
                jobContext, &AzFramework::Terrain::TerrainDataRequests::QueryRegionAsync, queryRegion,
                AzFramework::Terrain::TerrainDataRequests::TerrainDataMask::SurfaceData, perPositionCallback, sampler, params);

            // Wait for the async query to complete
            m_queryCompletionEvent.acquire();

            // Compare the results
            CompareSurfaceWeightData(queryPositions, baselineResultWeights, comparisonResultPositions, comparisonResultWeights);
        }

        DestroyTestTerrainSystem();
    }

    TEST_F(TerrainBulkQueryTest, ProcessSurfaceWeightsFromRegionAndProcessSurfaceWeightsFromListAsyncProduceSameResults)
    {
        CreateTestTerrainSystem(TerrainWorldBounds, TerrainQueryResolution, TerrainNumSurfaces);

        for (auto sampler :
             { AzFramework::Terrain::TerrainDataRequests::Sampler::BILINEAR, AzFramework::Terrain::TerrainDataRequests::Sampler::CLAMP,
               AzFramework::Terrain::TerrainDataRequests::Sampler::EXACT })
        {
            // Gather all our initial results from calling Process*FromRegion
            AZStd::vector<AZ::Vector3> queryPositions;
            AZStd::vector<AzFramework::SurfaceData::SurfaceTagWeightList> baselineResultWeights;
            GenerateBaselineSurfaceWeightData(QueryBounds, QueryStepSize, sampler, queryPositions, baselineResultWeights);
            ASSERT_EQ(queryPositions.size(), ExpectedResultCount);

            // Gather results from Process*FromListAsync
            AZStd::vector<AZ::Vector3> comparisonResultPositions;
            AZStd::vector<AzFramework::SurfaceData::SurfaceTagWeightList> comparisonResultWeights;
            AZStd::mutex outputMutex;

            auto listPositionCallback = [&comparisonResultPositions, &comparisonResultWeights, &outputMutex](
                const AzFramework::SurfaceData::SurfacePoint& surfacePoint, bool terrainExists)
            {
                // Make sure only one thread can add its result at a time.
                AZStd::scoped_lock lock(outputMutex);
                comparisonResultPositions.emplace_back(surfacePoint.m_position);
                comparisonResultWeights.emplace_back(surfacePoint.m_surfaceTags);
                EXPECT_TRUE(terrainExists);
            };

            auto params = CreateTestAsyncParams();
            AZStd::shared_ptr<AzFramework::Terrain::TerrainJobContext> jobContext;
            AzFramework::Terrain::TerrainDataRequestBus::BroadcastResult(
                jobContext, &AzFramework::Terrain::TerrainDataRequests::QueryListAsync, queryPositions,
                AzFramework::Terrain::TerrainDataRequests::TerrainDataMask::SurfaceData, listPositionCallback, sampler, params);

            // Wait for the async query to complete
            m_queryCompletionEvent.acquire();

            // Compare the results
            CompareSurfaceWeightData(queryPositions, baselineResultWeights, comparisonResultPositions, comparisonResultWeights);
        }

        DestroyTestTerrainSystem();
    }

    // -----------------------------------------------------------------------------
    // Compare Surface Point Query APIs

    TEST_F(TerrainBulkQueryTest, ProcessSurfacePointsFromRegionAndProcessSurfacePointsFromListProduceSameResults)
    {
        CreateTestTerrainSystem(TerrainWorldBounds, TerrainQueryResolution, TerrainNumSurfaces);

        for (auto sampler :
             { AzFramework::Terrain::TerrainDataRequests::Sampler::BILINEAR, AzFramework::Terrain::TerrainDataRequests::Sampler::CLAMP,
               AzFramework::Terrain::TerrainDataRequests::Sampler::EXACT })
        {
            // Gather all our initial results from calling Process*FromRegion
            AZStd::vector<AZ::Vector3> queryPositions;
            AZStd::vector<AzFramework::SurfaceData::SurfacePoint> baselineResultPoints;
            AZStd::vector<bool> baselineExistsFlags;
            GenerateBaselineSurfacePointData(
                QueryBounds, QueryStepSize, sampler, queryPositions, baselineResultPoints, baselineExistsFlags);
            ASSERT_EQ(queryPositions.size(), ExpectedResultCount);

            // Gather results from Process*FromList
            AZStd::vector<AzFramework::SurfaceData::SurfacePoint> comparisonResultPoints;
            AZStd::vector<bool> comparisonExistsFlags;

            auto listPositionCallback = [&comparisonResultPoints, &comparisonExistsFlags](
                const AzFramework::SurfaceData::SurfacePoint& surfacePoint, bool terrainExists)
            {
                comparisonResultPoints.emplace_back(surfacePoint);
                comparisonExistsFlags.emplace_back(terrainExists);
            };

            AzFramework::Terrain::TerrainDataRequestBus::Broadcast(
                &AzFramework::Terrain::TerrainDataRequests::QueryList, queryPositions,
                AzFramework::Terrain::TerrainDataRequests::TerrainDataMask::All, listPositionCallback, sampler);

            // Compare the results
            CompareSurfacePointData(baselineResultPoints, baselineExistsFlags, comparisonResultPoints, comparisonExistsFlags);
        }

        DestroyTestTerrainSystem();
    }

    TEST_F(TerrainBulkQueryTest, ProcessSurfacePointsFromRegionAndGetSurfacePointProduceSameResults)
    {
        CreateTestTerrainSystem(TerrainWorldBounds, TerrainQueryResolution, TerrainNumSurfaces);

        for (auto sampler :
             { AzFramework::Terrain::TerrainDataRequests::Sampler::BILINEAR, AzFramework::Terrain::TerrainDataRequests::Sampler::CLAMP,
               AzFramework::Terrain::TerrainDataRequests::Sampler::EXACT })
        {
            // Gather all our initial results from calling Process*FromRegion
            AZStd::vector<AZ::Vector3> queryPositions;
            AZStd::vector<AzFramework::SurfaceData::SurfacePoint> baselineResultPoints;
            AZStd::vector<bool> baselineExistsFlags;
            GenerateBaselineSurfacePointData(
                QueryBounds, QueryStepSize, sampler, queryPositions, baselineResultPoints, baselineExistsFlags);
            ASSERT_EQ(queryPositions.size(), ExpectedResultCount);

            // Gather results from Get*
            AZStd::vector<AzFramework::SurfaceData::SurfacePoint> comparisonResultPoints;
            AZStd::vector<bool> comparisonExistsFlags;
            AzFramework::SurfaceData::SurfacePoint surfacePoint;
            for (auto& position : queryPositions)
            {
                bool terrainExists = false;
                AzFramework::Terrain::TerrainDataRequestBus::Broadcast(
                    &AzFramework::Terrain::TerrainDataRequests::GetSurfacePoint, position, surfacePoint, sampler, &terrainExists);

                comparisonResultPoints.emplace_back(surfacePoint);
                comparisonExistsFlags.emplace_back(terrainExists);
            }

            // Compare the results
            CompareSurfacePointData(baselineResultPoints, baselineExistsFlags, comparisonResultPoints, comparisonExistsFlags);
        }

        DestroyTestTerrainSystem();
    }

    TEST_F(TerrainBulkQueryTest, ProcessSurfacePointsFromRegionAndProcessSurfacePointsFromRegionAsyncProduceSameResults)
    {
        CreateTestTerrainSystem(TerrainWorldBounds, TerrainQueryResolution, TerrainNumSurfaces);

        for (auto sampler :
             { AzFramework::Terrain::TerrainDataRequests::Sampler::BILINEAR, AzFramework::Terrain::TerrainDataRequests::Sampler::CLAMP,
               AzFramework::Terrain::TerrainDataRequests::Sampler::EXACT })
        {
            // Gather all our initial results from calling Process*FromRegion
            AZStd::vector<AZ::Vector3> queryPositions;
            AZStd::vector<AzFramework::SurfaceData::SurfacePoint> baselineResultPoints;
            AZStd::vector<bool> baselineExistsFlags;
            GenerateBaselineSurfacePointData(
                QueryBounds, QueryStepSize, sampler, queryPositions, baselineResultPoints, baselineExistsFlags);
            ASSERT_EQ(queryPositions.size(), ExpectedResultCount);

            // Gather results from Process*FromRegionAsync
            AZStd::vector<AzFramework::SurfaceData::SurfacePoint> comparisonResultPoints;
            AZStd::vector<bool> comparisonExistsFlags;
            AZStd::mutex outputMutex;

            auto regionPositionCallback = [&comparisonResultPoints, &comparisonExistsFlags, &outputMutex](
                [[maybe_unused]] size_t xIndex, [[maybe_unused]] size_t yIndex,
                const AzFramework::SurfaceData::SurfacePoint& surfacePoint, bool terrainExists)
            {
                // Make sure only one thread can add its result at a time.
                AZStd::scoped_lock lock(outputMutex);
                comparisonResultPoints.emplace_back(surfacePoint);
                comparisonExistsFlags.emplace_back(terrainExists);
            };

            auto params = CreateTestAsyncParams();
            AZStd::shared_ptr<AzFramework::Terrain::TerrainJobContext> jobContext;
            AzFramework::Terrain::TerrainQueryRegion queryRegion =
                AzFramework::Terrain::TerrainQueryRegion::CreateFromAabbAndStepSize(QueryBounds, QueryStepSize);
            AzFramework::Terrain::TerrainDataRequestBus::BroadcastResult(
                jobContext, &AzFramework::Terrain::TerrainDataRequests::QueryRegionAsync, queryRegion,
                AzFramework::Terrain::TerrainDataRequests::TerrainDataMask::All, regionPositionCallback, sampler, params);

            // Wait for the async query to complete
            m_queryCompletionEvent.acquire();

            // Compare the results
            CompareSurfacePointData(baselineResultPoints, baselineExistsFlags, comparisonResultPoints, comparisonExistsFlags);
        }

        DestroyTestTerrainSystem();
    }

    TEST_F(TerrainBulkQueryTest, ProcessSurfacePointsFromRegionAndProcessSurfacePointsFromListAsyncProduceSameResults)
    {
        CreateTestTerrainSystem(TerrainWorldBounds, TerrainQueryResolution, TerrainNumSurfaces);

        for (auto sampler :
             { AzFramework::Terrain::TerrainDataRequests::Sampler::BILINEAR, AzFramework::Terrain::TerrainDataRequests::Sampler::CLAMP,
               AzFramework::Terrain::TerrainDataRequests::Sampler::EXACT })
        {
            // Gather all our initial results from calling Process*FromRegion
            AZStd::vector<AZ::Vector3> queryPositions;
            AZStd::vector<AzFramework::SurfaceData::SurfacePoint> baselineResultPoints;
            AZStd::vector<bool> baselineExistsFlags;
            GenerateBaselineSurfacePointData(
                QueryBounds, QueryStepSize, sampler, queryPositions, baselineResultPoints, baselineExistsFlags);
            ASSERT_EQ(queryPositions.size(), ExpectedResultCount);

            // Gather results from Process*FromListAsync
            AZStd::vector<AzFramework::SurfaceData::SurfacePoint> comparisonResultPoints;
            AZStd::vector<bool> comparisonExistsFlags;
            AZStd::mutex outputMutex;

            auto listPositionCallback = [&comparisonResultPoints, &comparisonExistsFlags, &outputMutex](
                const AzFramework::SurfaceData::SurfacePoint& surfacePoint, bool terrainExists)
            {
                // Make sure only one thread can add its result at a time.
                AZStd::scoped_lock lock(outputMutex);
                comparisonResultPoints.emplace_back(surfacePoint);
                comparisonExistsFlags.emplace_back(terrainExists);
            };

            auto params = CreateTestAsyncParams();
            AZStd::shared_ptr<AzFramework::Terrain::TerrainJobContext> jobContext;
            AzFramework::Terrain::TerrainDataRequestBus::BroadcastResult(
                jobContext, &AzFramework::Terrain::TerrainDataRequests::QueryListAsync, queryPositions,
                AzFramework::Terrain::TerrainDataRequests::TerrainDataMask::All, listPositionCallback, sampler, params);

            // Wait for the async query to complete
            m_queryCompletionEvent.acquire();

            // Compare the results
            CompareSurfacePointData(baselineResultPoints, baselineExistsFlags, comparisonResultPoints, comparisonExistsFlags);
        }

        DestroyTestTerrainSystem();
    }

} // namespace UnitTest
