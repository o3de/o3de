/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "RHITestFixture.h"
#include <Atom/RHI/FrameEventBus.h>
#include <Atom/RHI/QueryPool.h>
#include <AzCore/std/containers/bitset.h>
#include <Tests/Device.h>

namespace UnitTest
{
    using namespace AZ;

    class MultiDeviceQueryTests : public MultiDeviceRHITestFixture
    {
    public:
        MultiDeviceQueryTests()
            : MultiDeviceRHITestFixture()
        {
        }

    private:
        void SetUp() override
        {
            MultiDeviceRHITestFixture::SetUp();
        }

        void TearDown() override
        {
            MultiDeviceRHITestFixture::TearDown();
        }
    };

    TEST_F(MultiDeviceQueryTests, TestNoop)
    {
        RHI::Ptr<RHI::Query> noopQuery;
        noopQuery = aznew RHI::Query;
        AZ_TEST_ASSERT(noopQuery);

        RHI::Ptr<RHI::QueryPool> noopQueryPool;
        noopQueryPool = aznew RHI::QueryPool;
        AZ_TEST_ASSERT(noopQueryPool);
    }

    TEST_F(MultiDeviceQueryTests, Test)
    {
        RHI::Ptr<RHI::Query> queryA;
        queryA = aznew RHI::Query;

        queryA->SetName(Name("QueryA"));
        AZ_TEST_ASSERT(queryA->GetName().GetStringView() == "QueryA");
        AZ_TEST_ASSERT(queryA->use_count() == 1);

        {
            RHI::Ptr<RHI::QueryPool> queryPool;
            queryPool = aznew RHI::QueryPool;

            EXPECT_EQ(1, queryPool->use_count());

            RHI::Ptr<RHI::Query> queryB;
            queryB = aznew RHI::Query;
            EXPECT_EQ(1, queryB->use_count());

            RHI::QueryPoolDescriptor queryPoolDesc;
            queryPoolDesc.m_queriesCount = 2;
            queryPoolDesc.m_type = RHI::QueryType::Occlusion;
            queryPoolDesc.m_pipelineStatisticsMask = RHI::PipelineStatisticsFlags::None;
            queryPool->Init(queryPoolDesc);

            EXPECT_FALSE(queryA->IsInitialized());
            EXPECT_FALSE(queryB->IsInitialized());

            queryPool->InitQuery(queryA.get());

            EXPECT_EQ(1, queryA->use_count());
            EXPECT_TRUE(queryA->IsInitialized());

            queryPool->InitQuery(queryB.get());

            EXPECT_TRUE(queryB->IsInitialized());

            EXPECT_EQ(queryA->GetPool(), queryPool.get());
            EXPECT_EQ(queryB->GetPool(), queryPool.get());
            EXPECT_EQ(queryPool->GetResourceCount(), 2);

            {
                uint32_t queryIndex = 0;

                const RHI::Query* queries[] = { queryA.get(), queryB.get() };

                queryPool->ForEach<RHI::Query>(
                    [&queryIndex, &queries]([[maybe_unused]] RHI::Query& query)
                    {
                        AZ_UNUSED(queries); // Prevent unused warning in release builds
                        AZ_Assert(queries[queryIndex] == &query, "Queries don't match");
                        queryIndex++;
                    });
            }

            queryB->Shutdown();
            EXPECT_EQ(queryB->GetPool(), nullptr);

            RHI::Ptr<RHI::QueryPool> queryPoolB;
            queryPoolB = aznew RHI::QueryPool;
            queryPoolB->Init(queryPoolDesc);

            queryPoolB->InitQuery(queryB.get());
            EXPECT_EQ(queryB->GetPool(), queryPoolB.get());

            // Since we are switching queryPool for queryB it adds a refcount and invalidates the views.
            // We need this to ensure the views are fully invalidated in order to release the refcount and avoid a leak.
            RHI::ResourceInvalidateBus::ExecuteQueuedEvents();

            queryPoolB->Shutdown();
            EXPECT_EQ(queryPoolB->GetResourceCount(), 0);
        }

        EXPECT_EQ(queryA->GetPool(), nullptr);
        EXPECT_EQ(queryA->use_count(), 1);
    }

    TEST_F(MultiDeviceQueryTests, TestAllocations)
    {
        static const uint32_t numQueries = 10;
        AZStd::array<RHI::Ptr<RHI::Query>, numQueries> queries;
        for (auto& query : queries)
        {
            query = aznew RHI::Query;
        }

        RHI::Ptr<RHI::QueryPool> queryPool;
        queryPool = aznew RHI::QueryPool;

        RHI::QueryPoolDescriptor queryPoolDesc;
        queryPoolDesc.m_queriesCount = numQueries;
        queryPoolDesc.m_type = RHI::QueryType::Occlusion;
        queryPoolDesc.m_pipelineStatisticsMask = RHI::PipelineStatisticsFlags::None;
        queryPool->Init(queryPoolDesc);

        AZStd::vector<RHI::Query*> queriesToInitialize(numQueries);
        for (size_t i = 0; i < queries.size(); ++i)
        {
            queriesToInitialize[i] = queries[i].get();
        }

        RHI::ResultCode result = queryPool->InitQuery(queriesToInitialize.data(), static_cast<uint32_t>(queriesToInitialize.size()));
        EXPECT_EQ(result, RHI::ResultCode::Success);
        auto checkSlotsFunc = [](const AZStd::vector<RHI::Query*>& queries)
        {
            if (queries.size() < 2)
            {
                return true;
            }

            AZStd::vector<uint32_t> indices;
            for (auto& query : queries)
            {
                indices.push_back(query->GetHandle(AZ::RHI::MultiDevice::DefaultDeviceIndex).GetIndex());
            }

            AZStd::sort(indices.begin(), indices.end());
            for (size_t i = 0; i < indices.size() - 1; ++i)
            {
                if (indices[i] != (indices[i + 1] + 1))
                {
                    return false;
                }
            }

            return true;
        };

        checkSlotsFunc(queriesToInitialize);

        RHI::Ptr<RHI::Query> extraQuery = aznew RHI::Query;
        EXPECT_EQ(queryPool->InitQuery(extraQuery.get()), RHI::ResultCode::OutOfMemory);
        AZ_TEST_ASSERT(!extraQuery->IsInitialized());

        AZStd::vector<uint32_t> queriesIndicesToShutdown = { 5, 6 };
        AZStd::vector<RHI::Query*> queriesToShutdown;
        for (auto& index : queriesIndicesToShutdown)
        {
            queries[index]->Shutdown();
            queriesToShutdown.push_back(queries[index].get());
        }

        EXPECT_EQ(queryPool->GetResourceCount(), numQueries - static_cast<uint32_t>(queriesIndicesToShutdown.size()));

        result = queryPool->InitQuery(queriesToShutdown.data(), static_cast<uint32_t>(queriesIndicesToShutdown.size()));
        EXPECT_EQ(result, RHI::ResultCode::Success);
        checkSlotsFunc(queriesToShutdown);

        // Since we are recreating queries it adds a refcount and invalidates the (non-existing) views.
        // We need to ensure to release the refcount and avoid leaks by running the invalidate bus.
        RHI::ResourceInvalidateBus::ExecuteQueuedEvents();

        queriesIndicesToShutdown = { 2, 5, 9 };
        queriesToShutdown.clear();
        for (auto& index : queriesIndicesToShutdown)
        {
            queries[index]->Shutdown();
            queriesToShutdown.push_back(queries[index].get());
        }

        EXPECT_EQ(queryPool->GetResourceCount(), (numQueries - static_cast<uint32_t>(queriesIndicesToShutdown.size())));

        result = queryPool->InitQuery(queriesToShutdown.data(), static_cast<uint32_t>(queriesToShutdown.size()));

        // Since we are recreating queries it adds a refcount and invalidates the (non-existing) views.
        // We need to ensure to release the refcount and avoid leaks by running the invalidate bus.
        RHI::ResourceInvalidateBus::ExecuteQueuedEvents();

        checkSlotsFunc(queriesToInitialize);
    }

    TEST_F(MultiDeviceQueryTests, TestIntervals)
    {
        static const uint32_t numQueries = 10;
        AZStd::array<RHI::Ptr<RHI::Query>, numQueries> queries;
        for (auto& query : queries)
        {
            query = aznew RHI::Query;
        }

        RHI::Ptr<RHI::QueryPool> queryPool;
        queryPool = aznew RHI::QueryPool;

        RHI::QueryPoolDescriptor queryPoolDesc;
        queryPoolDesc.m_queriesCount = numQueries;
        queryPoolDesc.m_type = RHI::QueryType::Occlusion;
        queryPoolDesc.m_pipelineStatisticsMask = RHI::PipelineStatisticsFlags::None;
        queryPool->Init(queryPoolDesc);

        AZStd::vector<RHI::Query*> queriesToInitialize(numQueries);
        for (size_t i = 0; i < queries.size(); ++i)
        {
            queriesToInitialize[i] = queries[i].get();
        }

        RHI::ResultCode result = queryPool->InitQuery(queriesToInitialize.data(), static_cast<uint32_t>(queriesToInitialize.size()));
        EXPECT_EQ(result, RHI::ResultCode::Success);

        uint64_t results[numQueries * DeviceCount] = {};

        for (auto deviceIndex{ 0 }; deviceIndex < DeviceCount; ++deviceIndex)
        {
            auto* testQueryPool = static_cast<UnitTest::QueryPool*>(queryPool->GetDeviceQueryPool(deviceIndex).get());
            auto& testQueryPoolIntervals = testQueryPool->m_calledIntervals;
            testQueryPoolIntervals.clear();

            EXPECT_EQ(queryPool->GetResults(results, numQueries * DeviceCount, RHI::QueryResultFlagBits::None), RHI::ResultCode::Success);

            EXPECT_EQ(testQueryPoolIntervals.size(), 1);
            EXPECT_EQ(testQueryPoolIntervals.front(), RHI::Interval(0, numQueries - 1));

            testQueryPoolIntervals.clear();
            auto* queryToTest = queries[5].get();
            EXPECT_EQ(queryPool->GetResults(queryToTest, results, DeviceCount, RHI::QueryResultFlagBits::None), RHI::ResultCode::Success);

            EXPECT_EQ(testQueryPoolIntervals.size(), 1);
            EXPECT_EQ(
                testQueryPoolIntervals.front(),
                RHI::Interval(queryToTest->GetHandle(deviceIndex).GetIndex(), queryToTest->GetHandle(deviceIndex).GetIndex()));

            AZStd::vector<RHI::Interval> intervalsToTest = { RHI::Interval(5, 5), RHI::Interval(0, 3), RHI::Interval(8, 9) };
            AZStd::vector<RHI::Query*> queriesToTest;
            for (auto& interval : intervalsToTest)
            {
                for (uint32_t i = interval.m_min; i <= interval.m_max; ++i)
                {
                    queriesToTest.push_back(queries[i].get());
                }
            }
            testQueryPoolIntervals.clear();
            EXPECT_EQ(
                queryPool->GetResults(
                    queriesToTest.data(), static_cast<uint32_t>(queriesToTest.size()), results, numQueries * DeviceCount, RHI::QueryResultFlagBits::None),
                RHI::ResultCode::Success);
             EXPECT_EQ(testQueryPoolIntervals.size(), intervalsToTest.size());
            for (auto& interval : intervalsToTest)
            {
                auto foundIt = AZStd::find(testQueryPoolIntervals.begin(), testQueryPoolIntervals.end(), interval);
                EXPECT_NE(foundIt, testQueryPoolIntervals.end());
            }
        }
    }

    TEST_F(MultiDeviceQueryTests, TestQuery)
    {
        AZStd::array<RHI::Ptr<RHI::QueryPool>, RHI::QueryTypeCount> queryPools;
        for (size_t i = 0; i < queryPools.size(); ++i)
        {
            auto& queryPool = queryPools[i];
            queryPool = aznew RHI::QueryPool;
            RHI::QueryPoolDescriptor queryPoolDesc;
            queryPoolDesc.m_queriesCount = 1;
            queryPoolDesc.m_type = static_cast<RHI::QueryType>(i);
            queryPoolDesc.m_pipelineStatisticsMask = queryPoolDesc.m_type == RHI::QueryType::PipelineStatistics
                ? RHI::PipelineStatisticsFlags::CInvocations
                : RHI::PipelineStatisticsFlags::None;
            queryPool->Init(queryPoolDesc);
        }

        auto& occlusionQueryPool = queryPools[static_cast<uint32_t>(RHI::QueryType::Occlusion)];
        auto& timestampQueryPool = queryPools[static_cast<uint32_t>(RHI::QueryType::Timestamp)];
        auto& statisticsQueryPool = queryPools[static_cast<uint32_t>(RHI::QueryType::PipelineStatistics)];

        uint64_t data;
        RHI::CommandList& dummyCommandList = reinterpret_cast<RHI::CommandList&>(data);

        // Correct begin and end for occlusion
        {
            RHI::Ptr<RHI::Query> query = aznew RHI::Query;
            EXPECT_EQ(occlusionQueryPool->InitQuery(query.get()), RHI::ResultCode::Success);
            for (auto deviceIndex{ 0 }; deviceIndex < DeviceCount; ++deviceIndex)
            {
                EXPECT_EQ(query->GetDeviceQuery(deviceIndex)->Begin(dummyCommandList), RHI::ResultCode::Success);
                EXPECT_EQ(query->GetDeviceQuery(deviceIndex)->End(dummyCommandList), RHI::ResultCode::Success);
            }
        }

        // Double Begin
        {
            RHI::Ptr<RHI::Query> query = aznew RHI::Query;
            occlusionQueryPool->InitQuery(query.get());
            for (auto deviceIndex{ 0 }; deviceIndex < DeviceCount; ++deviceIndex)
            {
                query->GetDeviceQuery(deviceIndex)->Begin(dummyCommandList);
                AZ_TEST_START_ASSERTTEST;
                EXPECT_EQ(RHI::ResultCode::Fail, query->GetDeviceQuery(deviceIndex)->Begin(dummyCommandList));
                AZ_TEST_STOP_ASSERTTEST(1);
            }
        }
        // End without Begin
        {
            RHI::Ptr<RHI::Query> query = aznew RHI::Query;
            occlusionQueryPool->InitQuery(query.get());
            for (auto deviceIndex{ 0 }; deviceIndex < DeviceCount; ++deviceIndex)
            {
                AZ_TEST_START_ASSERTTEST;
                EXPECT_EQ(RHI::ResultCode::Fail, query->GetDeviceQuery(deviceIndex)->End(dummyCommandList));
                AZ_TEST_STOP_ASSERTTEST(1);
            }
        }
        // End with another command list
        {
            RHI::Ptr<RHI::Query> query = aznew RHI::Query;
            occlusionQueryPool->InitQuery(query.get());
            uint64_t anotherData;
            RHI::CommandList& anotherDummyCmdList = reinterpret_cast<RHI::CommandList&>(anotherData);
            for (auto deviceIndex{ 0 }; deviceIndex < DeviceCount; ++deviceIndex)
            {
                EXPECT_EQ(RHI::ResultCode::Success, query->GetDeviceQuery(deviceIndex)->Begin(dummyCommandList));
                AZ_TEST_START_ASSERTTEST;
                EXPECT_EQ(RHI::ResultCode::InvalidArgument, query->GetDeviceQuery(deviceIndex)->End(anotherDummyCmdList));
                AZ_TEST_STOP_ASSERTTEST(1);
            }
        }
        // Invalid flag
        {
            RHI::Ptr<RHI::Query> query = aznew RHI::Query;
            statisticsQueryPool->InitQuery(query.get());
            for (auto deviceIndex{ 0 }; deviceIndex < DeviceCount; ++deviceIndex)
            {
                AZ_TEST_START_ASSERTTEST;
                EXPECT_EQ(
                    RHI::ResultCode::InvalidArgument,
                    query->GetDeviceQuery(deviceIndex)->Begin(dummyCommandList, RHI::QueryControlFlags::PreciseOcclusion));
                AZ_TEST_STOP_ASSERTTEST(1);
            }
        }
        // Invalid Begin for Timestamp
        {
            RHI::Ptr<RHI::Query> query = aznew RHI::Query;
            timestampQueryPool->InitQuery(query.get());
            for (auto deviceIndex{ 0 }; deviceIndex < DeviceCount; ++deviceIndex)
            {
                AZ_TEST_START_ASSERTTEST;
                EXPECT_EQ(RHI::ResultCode::Fail, query->GetDeviceQuery(deviceIndex)->Begin(dummyCommandList));
                AZ_TEST_STOP_ASSERTTEST(1);
            }
        }
        // Invalid End for Timestamp
        {
            RHI::Ptr<RHI::Query> query = aznew RHI::Query;
            timestampQueryPool->InitQuery(query.get());
            for (auto deviceIndex{ 0 }; deviceIndex < DeviceCount; ++deviceIndex)
            {
                AZ_TEST_START_ASSERTTEST;
                EXPECT_EQ(RHI::ResultCode::Fail, query->GetDeviceQuery(deviceIndex)->End(dummyCommandList));
                AZ_TEST_STOP_ASSERTTEST(1);
            }
        }
        // Invalid WriteTimestamp
        {
            RHI::Ptr<RHI::Query> query = aznew RHI::Query;
            occlusionQueryPool->InitQuery(query.get());
            for (auto deviceIndex{ 0 }; deviceIndex < DeviceCount; ++deviceIndex)
            {
                AZ_TEST_START_ASSERTTEST;
                EXPECT_EQ(RHI::ResultCode::Fail, query->GetDeviceQuery(deviceIndex)->WriteTimestamp(dummyCommandList));
                AZ_TEST_STOP_ASSERTTEST(1);
            }
        }
        // Correct WriteTimestamp
        {
            RHI::Ptr<RHI::Query> query = aznew RHI::Query;
            timestampQueryPool->InitQuery(query.get());
            for (auto deviceIndex{ 0 }; deviceIndex < DeviceCount; ++deviceIndex)
            {
                EXPECT_EQ(RHI::ResultCode::Success, query->GetDeviceQuery(deviceIndex)->WriteTimestamp(dummyCommandList));
            }
        }
    }

    TEST_F(MultiDeviceQueryTests, TestQueryPoolInitialization)
    {
        RHI::Ptr<RHI::QueryPool> queryPool;
        queryPool = aznew RHI::QueryPool;
        RHI::QueryPoolDescriptor queryPoolDesc;
        queryPoolDesc.m_queriesCount = 0;
        queryPoolDesc.m_type = RHI::QueryType::Occlusion;
        queryPoolDesc.m_pipelineStatisticsMask = RHI::PipelineStatisticsFlags::None;
        // Count of 0
        AZ_TEST_START_ASSERTTEST;
        EXPECT_EQ(queryPool->Init(queryPoolDesc), RHI::ResultCode::InvalidArgument);
        AZ_TEST_STOP_ASSERTTEST(1);

        // valid m_pipelineStatisticsMask for Occlusion QueryType
        queryPoolDesc.m_queriesCount = 1;
        queryPoolDesc.m_pipelineStatisticsMask = RHI::PipelineStatisticsFlags::CInvocations;
        EXPECT_EQ(queryPool->Init(queryPoolDesc), RHI::ResultCode::Success);

        // invalid m_pipelineStatisticsMask for PipelineStatistics QueryType
        queryPoolDesc.m_type = RHI::QueryType::PipelineStatistics;
        queryPoolDesc.m_pipelineStatisticsMask = RHI::PipelineStatisticsFlags::None;
        AZ_TEST_START_ASSERTTEST;
        EXPECT_EQ(queryPool->Init(queryPoolDesc), RHI::ResultCode::InvalidArgument);
        AZ_TEST_STOP_ASSERTTEST(1);
    }

    TEST_F(MultiDeviceQueryTests, TestResults)
    {
        AZStd::array<RHI::Ptr<RHI::QueryPool>, 2> queryPools;
        RHI::PipelineStatisticsFlags mask = RHI::PipelineStatisticsFlags::CInvocations | RHI::PipelineStatisticsFlags::CPrimitives |
            RHI::PipelineStatisticsFlags::IAPrimitives;
        for (auto& queryPool : queryPools)
        {
            queryPool = aznew RHI::QueryPool;
            RHI::QueryPoolDescriptor queryPoolDesc;
            queryPoolDesc.m_queriesCount = 2;
            queryPoolDesc.m_type = RHI::QueryType::PipelineStatistics;
            queryPoolDesc.m_pipelineStatisticsMask = mask;
            EXPECT_EQ(queryPool->Init(queryPoolDesc), RHI::ResultCode::Success);
        }

        RHI::Ptr<RHI::Query> query = aznew RHI::Query;
        uint32_t numPipelineStatistics = RHI::CountBitsSet(static_cast<uint64_t>(mask));
        AZStd::vector<uint64_t> results(numPipelineStatistics * 2 * DeviceCount);
        // Using uninitialized query
        AZ_TEST_START_ASSERTTEST;
        EXPECT_EQ(
            queryPools[0]->GetResults(results.data(), numPipelineStatistics * 2 * DeviceCount, RHI::QueryResultFlagBits::None),
            RHI::ResultCode::InvalidArgument);
        AZ_TEST_STOP_ASSERTTEST(3);

        // Wrong size for results count.
        queryPools[0]->InitQuery(query.get());
        AZ_TEST_START_ASSERTTEST;
        EXPECT_EQ(queryPools[0]->GetResults(results.data(), DeviceCount, RHI::QueryResultFlagBits::None), RHI::ResultCode::InvalidArgument);
        AZ_TEST_STOP_ASSERTTEST(1);

        // Using a query from another pool
        RHI::Ptr<RHI::Query> anotherQuery = aznew RHI::Query;
        queryPools[1]->InitQuery(anotherQuery.get());
        AZ_TEST_START_ASSERTTEST;
        EXPECT_EQ(
            queryPools[0]->GetResults(anotherQuery.get(), results.data(), numPipelineStatistics * DeviceCount, RHI::QueryResultFlagBits::None),
            RHI::ResultCode::InvalidArgument);
        AZ_TEST_STOP_ASSERTTEST(1);

        // Results count is too small
        anotherQuery->Shutdown();
        queryPools[0]->InitQuery(anotherQuery.get());
        RHI::Query* queries[] = { query.get(), anotherQuery.get() };
        AZ_TEST_START_ASSERTTEST;
        EXPECT_EQ(
            queryPools[0]->GetResults(queries, 2, results.data(), numPipelineStatistics * DeviceCount, RHI::QueryResultFlagBits::None),
            RHI::ResultCode::InvalidArgument);
        AZ_TEST_STOP_ASSERTTEST(1);

        // Correct usage
        EXPECT_EQ(
            queryPools[0]->GetResults(queries, 2, results.data(), numPipelineStatistics * 5 * DeviceCount, RHI::QueryResultFlagBits::None),
            RHI::ResultCode::Success);

        // Unsorted queries
        {
            const size_t numQueries = 5;
            AZStd::array<RHI::Ptr<AZ::RHI::Query>, numQueries> queries2;
            AZStd::vector<uint64_t> results2(numQueries * DeviceCount);

            RHI::Ptr<RHI::QueryPool> queryPool = aznew RHI::QueryPool;
            RHI::QueryPoolDescriptor queryPoolDesc;
            queryPoolDesc.m_queriesCount = 5;
            queryPoolDesc.m_type = RHI::QueryType::Occlusion;
            EXPECT_EQ(queryPool->Init(queryPoolDesc), RHI::ResultCode::Success);

            for (size_t i = 0; i < queries2.size(); ++i)
            {
                queries2[i] = aznew RHI::Query;
                queryPool->InitQuery(queries2[i].get());
            }

            AZStd::array<RHI::Query*, numQueries> queriesPtr = {
                queries2[2].get(), queries2[0].get(), queries2[1].get(), queries2[3].get(), queries2[4].get()
            };
            EXPECT_EQ(
                queryPool->GetResults(queriesPtr.data(), numQueries, results2.data(), numQueries * DeviceCount, RHI::QueryResultFlagBits::None),
                RHI::ResultCode::Success);
            for (uint32_t i = 0; i < numQueries; ++i)
            {
                EXPECT_EQ(results2[i], queriesPtr[i]->GetHandle(AZ::RHI::MultiDevice::DefaultDeviceIndex).GetIndex());
            }
        }

        // Since we are switching queryPools for some queries it adds a refcount and invalidates the views.
        // We need to ensure the views are fully invalidated in order to release the refcount and avoid leaks.
        RHI::ResourceInvalidateBus::ExecuteQueuedEvents();
    }

} // namespace UnitTest
