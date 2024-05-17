/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "RHITestFixture.h"
#include <AzCore/std/containers/bitset.h>
#include <Tests/Factory.h>
#include <Tests/Device.h>
#include <Atom/RHI/FrameEventBus.h>

namespace UnitTest
{
    using namespace AZ;

    class QueryTests
        : public RHITestFixture
    {
    public:
        QueryTests()
            : RHITestFixture()
        {
        }

        ~QueryTests()
        {
        }

    private:
        void SetUp() override
        {
            RHITestFixture::SetUp();
            m_factory.reset(aznew Factory());
            m_device = MakeTestDevice();
        }

        void TearDown() override
        {
            m_factory.reset();
            m_device.reset();
            RHITestFixture::TearDown();
        }

    protected:
        AZStd::unique_ptr<Factory> m_factory;
        RHI::Ptr<RHI::Device> m_device;
    };

    TEST_F(QueryTests, TestNoop)
    {
        RHI::Ptr<RHI::DeviceQuery> noopQuery;
        noopQuery = RHI::Factory::Get().CreateQuery();
        AZ_TEST_ASSERT(noopQuery);

        RHI::Ptr<RHI::DeviceQueryPool> noopQueryPool;
        noopQueryPool = RHI::Factory::Get().CreateQueryPool();
        AZ_TEST_ASSERT(noopQueryPool);
    }

    TEST_F(QueryTests, Test)
    {
        RHI::Ptr<RHI::DeviceQuery> queryA;
        queryA = RHI::Factory::Get().CreateQuery();

        queryA->SetName(Name("QueryA"));
        AZ_TEST_ASSERT(queryA->GetName().GetStringView() == "QueryA");
        AZ_TEST_ASSERT(queryA->use_count() == 1);

        {
            RHI::Ptr<RHI::DeviceQueryPool> queryPool;
            queryPool = RHI::Factory::Get().CreateQueryPool();

            EXPECT_EQ(1, queryPool->use_count());

            RHI::Ptr<RHI::DeviceQuery> queryB;
            queryB = RHI::Factory::Get().CreateQuery();
            EXPECT_EQ(1, queryB->use_count());

            RHI::QueryPoolDescriptor queryPoolDesc;
            queryPoolDesc.m_queriesCount = 2;
            queryPoolDesc.m_type = RHI::QueryType::Occlusion;
            queryPoolDesc.m_pipelineStatisticsMask = RHI::PipelineStatisticsFlags::None;
            queryPool->Init(*m_device, queryPoolDesc);

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

                const RHI::DeviceQuery* queries[] =
                {
                    queryA.get(),
                    queryB.get()
                };

                queryPool->ForEach<RHI::DeviceQuery>([&queryIndex, &queries]([[maybe_unused]] RHI::DeviceQuery& query)
                {
                    AZ_UNUSED(queries); // Prevent unused warning in release builds
                    AZ_Assert(queries[queryIndex] == &query, "Queries don't match");
                    queryIndex++;
                });
            }

            queryB->Shutdown();
            EXPECT_EQ(queryB->GetPool(), nullptr);

            RHI::Ptr<RHI::DeviceQueryPool> queryPoolB;
            queryPoolB = RHI::Factory::Get().CreateQueryPool();
            queryPoolB->Init(*m_device, queryPoolDesc);

            queryPoolB->InitQuery(queryB.get());
            EXPECT_EQ(queryB->GetPool(), queryPoolB.get());

            //Since we are switching queryPool for queryB it adds a refcount and invalidates the views.
            //We need this to ensure the views are fully invalidated in order to release the refcount and avoid a leak.
            RHI::ResourceInvalidateBus::ExecuteQueuedEvents();

            queryPoolB->Shutdown();
            EXPECT_EQ(queryPoolB->GetResourceCount(), 0);
        }

        EXPECT_EQ(queryA->GetPool(), nullptr);
        EXPECT_EQ(queryA->use_count(), 1);
    }

    TEST_F(QueryTests, TestAllocations)
    {
        static const uint32_t numQueries = 10;
        AZStd::array<RHI::Ptr<RHI::DeviceQuery>, numQueries> queries;
        for (auto& query : queries)
        {
            query = RHI::Factory::Get().CreateQuery();
        }

        RHI::Ptr<RHI::DeviceQueryPool> queryPool;
        queryPool = RHI::Factory::Get().CreateQueryPool();

        RHI::QueryPoolDescriptor queryPoolDesc;
        queryPoolDesc.m_queriesCount = numQueries;
        queryPoolDesc.m_type = RHI::QueryType::Occlusion;
        queryPoolDesc.m_pipelineStatisticsMask = RHI::PipelineStatisticsFlags::None;
        queryPool->Init(*m_device, queryPoolDesc);

        AZStd::vector<RHI::DeviceQuery*> queriesToInitialize(numQueries);
        for (size_t i = 0; i < queries.size(); ++i)
        {
            queriesToInitialize[i] = queries[i].get();
        }

        RHI::ResultCode result = queryPool->InitQuery(queriesToInitialize.data(), static_cast<uint32_t>(queriesToInitialize.size()));
        EXPECT_EQ(result, RHI::ResultCode::Success);
        auto checkSlotsFunc = [](const AZStd::vector<RHI::DeviceQuery*>& queries)
        {
            if (queries.size() < 2)
            {
                return true;
            }

            AZStd::vector<uint32_t> indices;
            for (auto& query : queries)
            {
                indices.push_back(query->GetHandle().GetIndex());
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

        auto extraQuery = RHI::Factory::Get().CreateQuery();
        EXPECT_EQ(queryPool->InitQuery(extraQuery.get()), RHI::ResultCode::OutOfMemory);
        AZ_TEST_ASSERT(!extraQuery->IsInitialized());

        AZStd::vector<uint32_t> queriesIndicesToShutdown = { 5, 6 };
        AZStd::vector<RHI::DeviceQuery*> queriesToShutdown;
        for (auto& index : queriesIndicesToShutdown)
        {
            queries[index]->Shutdown();
            queriesToShutdown.push_back(queries[index].get());
        }
        
        EXPECT_EQ(queryPool->GetResourceCount(), numQueries - static_cast<uint32_t>(queriesIndicesToShutdown.size()));

        result = queryPool->InitQuery(queriesToShutdown.data(), static_cast<uint32_t>(queriesIndicesToShutdown.size()));
        EXPECT_EQ(result, RHI::ResultCode::Success);
        checkSlotsFunc(queriesToShutdown);

        queriesIndicesToShutdown = {2, 5, 9};
        queriesToShutdown.clear();
        for (auto& index : queriesIndicesToShutdown)
        {
            queries[index]->Shutdown();
            queriesToShutdown.push_back(queries[index].get());
        }

        EXPECT_EQ(queryPool->GetResourceCount(), (numQueries - static_cast<uint32_t>(queriesIndicesToShutdown.size())));

        result = queryPool->InitQuery(queriesToShutdown.data(), static_cast<uint32_t>(queriesToShutdown.size()));

        //Since we are switching queryPools for some queries it adds a refcount and invalidates the views.
        //We need to ensure the views are fully invalidated in order to release the refcount and avoid leaks.
        RHI::ResourceInvalidateBus::ExecuteQueuedEvents();

        checkSlotsFunc(queriesToInitialize);
    }

    TEST_F(QueryTests, TestIntervals)
    {
        static const uint32_t numQueries = 10;
        AZStd::array<RHI::Ptr<RHI::DeviceQuery>, numQueries> queries;
        for (auto& query : queries)
        {
            query = RHI::Factory::Get().CreateQuery();
        }

        RHI::Ptr<RHI::DeviceQueryPool> queryPool;
        queryPool = RHI::Factory::Get().CreateQueryPool();

        RHI::QueryPoolDescriptor queryPoolDesc;
        queryPoolDesc.m_queriesCount = numQueries;
        queryPoolDesc.m_type = RHI::QueryType::Occlusion;
        queryPoolDesc.m_pipelineStatisticsMask = RHI::PipelineStatisticsFlags::None;
        queryPool->Init(*m_device, queryPoolDesc);

        AZStd::vector<RHI::DeviceQuery*> queriesToInitialize(numQueries);
        for (size_t i = 0; i < queries.size(); ++i)
        {
            queriesToInitialize[i] = queries[i].get();
        }

        RHI::ResultCode result = queryPool->InitQuery(queriesToInitialize.data(), static_cast<uint32_t>(queriesToInitialize.size()));
        EXPECT_EQ(result, RHI::ResultCode::Success);

        auto* testQueryPool = static_cast<UnitTest::QueryPool*>(queryPool.get());
        auto& testQueryPoolIntervals = testQueryPool->m_calledIntervals;
        uint64_t results[numQueries] = {};
        EXPECT_EQ(queryPool->GetResults(results, numQueries, RHI::QueryResultFlagBits::None), RHI::ResultCode::Success);
        EXPECT_EQ(testQueryPoolIntervals.size(), 1);
        EXPECT_EQ(testQueryPoolIntervals.front(), RHI::Interval(0, numQueries - 1));

        testQueryPoolIntervals.clear();
        auto* queryToTest = queries[5].get();
        EXPECT_EQ(queryPool->GetResults(queryToTest, results, 1, RHI::QueryResultFlagBits::None), RHI::ResultCode::Success);
        EXPECT_EQ(testQueryPoolIntervals.size(), 1);
        EXPECT_EQ(testQueryPoolIntervals.front(), RHI::Interval(queryToTest->GetHandle().GetIndex(), queryToTest->GetHandle().GetIndex()));

        AZStd::vector<RHI::Interval> intervalsToTest = { RHI::Interval(5, 5), RHI::Interval(0, 3), RHI::Interval(8, 9) };
        AZStd::vector<RHI::DeviceQuery*> queriesToTest;
        for (auto& interval : intervalsToTest)
        {
            for (uint32_t i = interval.m_min; i <= interval.m_max; ++i)
            {
                queriesToTest.push_back(queries[i].get());
            }
        }
        testQueryPoolIntervals.clear();
        EXPECT_EQ(queryPool->GetResults(queriesToTest.data(), static_cast<uint32_t>(queriesToTest.size()), results, numQueries, RHI::QueryResultFlagBits::None), RHI::ResultCode::Success);
        EXPECT_EQ(testQueryPoolIntervals.size(), intervalsToTest.size());
        for (auto& interval : intervalsToTest)
        {
            auto foundIt = AZStd::find(testQueryPoolIntervals.begin(), testQueryPoolIntervals.end(), interval);
            EXPECT_NE(foundIt, testQueryPoolIntervals.end());
        }
    }

    TEST_F(QueryTests, TestQuery)
    {
        AZStd::array<RHI::Ptr<RHI::DeviceQueryPool>, RHI::QueryTypeCount> queryPools;
        for (size_t i = 0; i < queryPools.size(); ++i)
        {
            auto& queryPool = queryPools[i];
            queryPool = RHI::Factory::Get().CreateQueryPool();
            RHI::QueryPoolDescriptor queryPoolDesc;
            queryPoolDesc.m_queriesCount = 1;
            queryPoolDesc.m_type = static_cast<RHI::QueryType>(i);
            queryPoolDesc.m_pipelineStatisticsMask = queryPoolDesc.m_type == RHI::QueryType::PipelineStatistics ? RHI::PipelineStatisticsFlags::CInvocations : RHI::PipelineStatisticsFlags::None;
            queryPool->Init(*m_device, queryPoolDesc);
        }

        auto& occlusionQueryPool = queryPools[static_cast<uint32_t>(RHI::QueryType::Occlusion)];
        auto& timestampQueryPool = queryPools[static_cast<uint32_t>(RHI::QueryType::Timestamp)];
        auto& statisticsQueryPool = queryPools[static_cast<uint32_t>(RHI::QueryType::PipelineStatistics)];

        uint64_t data;
        RHI::CommandList& dummyCommandList = reinterpret_cast<RHI::CommandList&>(data);

        // Correct begin and end for occlusion
        {
            auto query = RHI::Factory::Get().CreateQuery();
            EXPECT_EQ(occlusionQueryPool->InitQuery(query.get()), RHI::ResultCode::Success);
            EXPECT_EQ(query->Begin(dummyCommandList), RHI::ResultCode::Success);
            EXPECT_EQ(query->End(dummyCommandList), RHI::ResultCode::Success);
        }

        // Double Begin
        {
            auto query = RHI::Factory::Get().CreateQuery();
            occlusionQueryPool->InitQuery(query.get());
            query->Begin(dummyCommandList);
            AZ_TEST_START_ASSERTTEST;
            EXPECT_EQ(RHI::ResultCode::Fail, query->Begin(dummyCommandList));
            AZ_TEST_STOP_ASSERTTEST(1);
        }
        // End without Begin
        {
            auto query = RHI::Factory::Get().CreateQuery();
            occlusionQueryPool->InitQuery(query.get());
            AZ_TEST_START_ASSERTTEST;
            EXPECT_EQ(RHI::ResultCode::Fail, query->End(dummyCommandList));
            AZ_TEST_STOP_ASSERTTEST(1);
        }
        // End with another command list
        {            
            auto query = RHI::Factory::Get().CreateQuery();
            occlusionQueryPool->InitQuery(query.get());
            uint64_t anotherData;
            RHI::CommandList& anotherDummyCmdList = reinterpret_cast<RHI::CommandList&>(anotherData);
            EXPECT_EQ(RHI::ResultCode::Success, query->Begin(dummyCommandList));
            AZ_TEST_START_ASSERTTEST;
            EXPECT_EQ(RHI::ResultCode::InvalidArgument, query->End(anotherDummyCmdList));
            AZ_TEST_STOP_ASSERTTEST(1);
        }
        // Invalid flag
        {            
            auto query = RHI::Factory::Get().CreateQuery();
            statisticsQueryPool->InitQuery(query.get());
            AZ_TEST_START_ASSERTTEST;
            EXPECT_EQ(RHI::ResultCode::InvalidArgument, query->Begin(dummyCommandList, RHI::QueryControlFlags::PreciseOcclusion));
            AZ_TEST_STOP_ASSERTTEST(1);
        }
        // Invalid Begin for Timestamp
        {
            auto query = RHI::Factory::Get().CreateQuery();
            timestampQueryPool->InitQuery(query.get());
            AZ_TEST_START_ASSERTTEST;
            EXPECT_EQ(RHI::ResultCode::Fail, query->Begin(dummyCommandList));
            AZ_TEST_STOP_ASSERTTEST(1);
        }
        // Invalid End for Timestamp
        {
            auto query = RHI::Factory::Get().CreateQuery();
            timestampQueryPool->InitQuery(query.get());
            AZ_TEST_START_ASSERTTEST;
            EXPECT_EQ(RHI::ResultCode::Fail, query->End(dummyCommandList));
            AZ_TEST_STOP_ASSERTTEST(1);
        }
        // Invalid WriteTimestamp
        {
            auto query = RHI::Factory::Get().CreateQuery();
            occlusionQueryPool->InitQuery(query.get());
            AZ_TEST_START_ASSERTTEST;
            EXPECT_EQ(RHI::ResultCode::Fail, query->WriteTimestamp(dummyCommandList));
            AZ_TEST_STOP_ASSERTTEST(1);
        }
        // Correct WriteTimestamp
        {
            auto query = RHI::Factory::Get().CreateQuery();
            timestampQueryPool->InitQuery(query.get());
            EXPECT_EQ(RHI::ResultCode::Success, query->WriteTimestamp(dummyCommandList));
        }
    }

    TEST_F(QueryTests, TestQueryPoolInitialization)
    {
        RHI::Ptr<RHI::DeviceQueryPool> queryPool;
        queryPool = RHI::Factory::Get().CreateQueryPool();
        RHI::QueryPoolDescriptor queryPoolDesc;
        queryPoolDesc.m_queriesCount = 0;
        queryPoolDesc.m_type = RHI::QueryType::Occlusion;
        queryPoolDesc.m_pipelineStatisticsMask = RHI::PipelineStatisticsFlags::None;
        // Count of 0
        AZ_TEST_START_ASSERTTEST;
        EXPECT_EQ(queryPool->Init(*m_device, queryPoolDesc), RHI::ResultCode::InvalidArgument);
        AZ_TEST_STOP_ASSERTTEST(1);

        // valid m_pipelineStatisticsMask for Occlusion QueryType
        queryPoolDesc.m_queriesCount = 1;
        queryPoolDesc.m_pipelineStatisticsMask = RHI::PipelineStatisticsFlags::CInvocations;
        EXPECT_EQ(queryPool->Init(*m_device, queryPoolDesc), RHI::ResultCode::Success);

        // invalid m_pipelineStatisticsMask for PipelineStatistics QueryType
        queryPoolDesc.m_type = RHI::QueryType::PipelineStatistics;
        queryPoolDesc.m_pipelineStatisticsMask = RHI::PipelineStatisticsFlags::None;
        AZ_TEST_START_ASSERTTEST;
        EXPECT_EQ(queryPool->Init(*m_device, queryPoolDesc), RHI::ResultCode::InvalidArgument);
        AZ_TEST_STOP_ASSERTTEST(1);
    }

    TEST_F(QueryTests, TestResults)
    {
        AZStd::array<RHI::Ptr<RHI::DeviceQueryPool>, 2> queryPools;
        RHI::PipelineStatisticsFlags mask = RHI::PipelineStatisticsFlags::CInvocations | RHI::PipelineStatisticsFlags::CPrimitives | RHI::PipelineStatisticsFlags::IAPrimitives;
        for (auto& queryPool : queryPools)
        {
            queryPool = RHI::Factory::Get().CreateQueryPool();
            RHI::QueryPoolDescriptor queryPoolDesc;
            queryPoolDesc.m_queriesCount = 2;
            queryPoolDesc.m_type = RHI::QueryType::PipelineStatistics;
            queryPoolDesc.m_pipelineStatisticsMask = mask;
            EXPECT_EQ(queryPool->Init(*m_device, queryPoolDesc), RHI::ResultCode::Success);
        }

        auto query = RHI::Factory::Get().CreateQuery();
        uint32_t numPipelineStatistics = RHI::CountBitsSet(static_cast<uint64_t>(mask));
        AZStd::vector<uint64_t> results(numPipelineStatistics * 2);
        // Using uninitialized query
        AZ_TEST_START_ASSERTTEST;
        EXPECT_EQ(queryPools[0]->GetResults(results.data(), numPipelineStatistics, RHI::QueryResultFlagBits::None), RHI::ResultCode::InvalidArgument);
        AZ_TEST_STOP_ASSERTTEST(3);

        // Wrong size for results count.
        queryPools[0]->InitQuery(query.get());
        AZ_TEST_START_ASSERTTEST;
        EXPECT_EQ(queryPools[0]->GetResults(results.data(), 1, RHI::QueryResultFlagBits::None), RHI::ResultCode::InvalidArgument);
        AZ_TEST_STOP_ASSERTTEST(1);

        // Using a query from another pool
        auto anotherQuery = RHI::Factory::Get().CreateQuery();
        queryPools[1]->InitQuery(anotherQuery.get());
        AZ_TEST_START_ASSERTTEST;
        EXPECT_EQ(queryPools[0]->GetResults(anotherQuery.get(), results.data(), numPipelineStatistics, RHI::QueryResultFlagBits::None), RHI::ResultCode::InvalidArgument);
        AZ_TEST_STOP_ASSERTTEST(1);

        // Results count is too small
        anotherQuery->Shutdown();
        queryPools[0]->InitQuery(anotherQuery.get());
        RHI::DeviceQuery* queries[] = { query.get(), anotherQuery.get() };
        AZ_TEST_START_ASSERTTEST;
        EXPECT_EQ(queryPools[0]->GetResults(queries, 2, results.data(), numPipelineStatistics, RHI::QueryResultFlagBits::None), RHI::ResultCode::InvalidArgument);
        AZ_TEST_STOP_ASSERTTEST(1);

        // Correct usage
        EXPECT_EQ(queryPools[0]->GetResults(queries, 2, results.data(), numPipelineStatistics * 5, RHI::QueryResultFlagBits::None), RHI::ResultCode::Success);        

        // Unsorted queries
        {
            const size_t numQueries = 5;
            AZStd::array<RHI::Ptr<AZ::RHI::DeviceQuery>, numQueries> queries2;
            AZStd::vector<uint64_t> results2(numQueries);

            RHI::Ptr<RHI::DeviceQueryPool> queryPool = RHI::Factory::Get().CreateQueryPool();
            RHI::QueryPoolDescriptor queryPoolDesc;
            queryPoolDesc.m_queriesCount = 5;
            queryPoolDesc.m_type = RHI::QueryType::Occlusion;
            EXPECT_EQ(queryPool->Init(*m_device, queryPoolDesc), RHI::ResultCode::Success);
            
            for (size_t i = 0; i < queries2.size(); ++i)
            {
                queries2[i] = RHI::Factory::Get().CreateQuery();
                queryPool->InitQuery(queries2[i].get());
            }

            AZStd::array<RHI::DeviceQuery*, numQueries> queriesPtr = { queries2[2].get(), queries2[0].get(), queries2[1].get(), queries2[3].get(), queries2[4].get() };
            EXPECT_EQ(queryPool->GetResults(queriesPtr.data(), numQueries, results2.data(), numQueries, RHI::QueryResultFlagBits::None), RHI::ResultCode::Success);
            for (uint32_t i = 0; i < numQueries; ++i)
            {
                EXPECT_EQ(results2[i], queriesPtr[i]->GetHandle().GetIndex());
            }
        }

        //Since we are switching queryPools for some queries it adds a refcount and invalidates the views.
        //We need to ensure the views are fully invalidated in order to release the refcount and avoid leaks.
        RHI::ResourceInvalidateBus::ExecuteQueuedEvents();
    }
}
