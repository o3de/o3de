/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/UnitTest/TestTypes.h>

#include <Atom/RPI.Public/GpuQuery/GpuQueryTypes.h>
#include <Atom/RPI.Public/GpuQuery/GpuQuerySystem.h>
#include <Atom/RPI.Public/GpuQuery/TimestampQueryPool.h>
#include <Atom/RPI.Public/GpuQuery/GpuQuerySystemInterface.h>

#include <Atom/RHI/BufferScopeAttachment.h>
#include <Atom/RHI/FrameGraph.h>

#include <AzTest/AzTest.h>

#include <Common/RPITestFixture.h>

namespace UnitTest
{
    using namespace AZ;
    using namespace AZ::RPI;

    class GpuQueryTests
        : public RPITestFixture
    {
        void SetUp() final
        {
            RPITestFixture::SetUp();
        }

        void TearDown() final
        {
            RPITestFixture::TearDown();
        }
    };

    // Unit test the RPI::QueryPool
    TEST_F(GpuQueryTests, TestQueryPools)
    {
        const uint32_t QueryCount = 1024u;
        const uint32_t QueriesPerInstance = 1u;
        const RHI::QueryType Type = RHI::QueryType::Occlusion;
        const RHI::PipelineStatisticsFlags StatisticsFlags = RHI::PipelineStatisticsFlags::None;
        const uint32_t OcclusionQueryResultSize = sizeof(uint64_t);

        const RHI::FrameGraphExecuteContext::Descriptor desc = {};
        RHI::FrameGraphExecuteContext context(desc);

        // Setup the FrameGraph
        RHI::Scope scope;
        scope.Init(RHI::ScopeId("StubScope"));
        RHI::FrameGraph frameGraph;
        frameGraph.BeginScope(scope);

        AZStd::unique_ptr<RPI::QueryPool> queryPool = RPI::QueryPool::CreateQueryPool(QueryCount, QueriesPerInstance, Type, StatisticsFlags);

        const uint32_t resultSize = queryPool->GetQueryResultSize();
        EXPECT_EQ(resultSize, OcclusionQueryResultSize);

        // Test valid Queries
        {
            RHI::Ptr<RPI::Query> query = queryPool->CreateQuery(RHI::QueryPoolScopeAttachmentType::Global, RHI::ScopeAttachmentAccess::Write);
            EXPECT_TRUE(query.get());
        }

        // Test adding to the FrameGraph multiple times with one Query within a single frame
        {
            RHI::Ptr<RPI::Query> query = queryPool->CreateQuery(RHI::QueryPoolScopeAttachmentType::Global, RHI::ScopeAttachmentAccess::Write);

            QueryResultCode resultCode = query->AddToFrameGraph(frameGraph);
            EXPECT_EQ(resultCode, QueryResultCode::Success);
            resultCode = query->AddToFrameGraph(frameGraph);
            // Adding query multiple times in a frame is allowed after introduced multi-device.
            EXPECT_EQ(resultCode, QueryResultCode::Success);

            // Next frame
            queryPool->Update();
        }

        // Test recording the same Query in different frames
        {
            RHI::Ptr<RPI::Query> query = queryPool->CreateQuery(RHI::QueryPoolScopeAttachmentType::Global, RHI::ScopeAttachmentAccess::Write);
            QueryResultCode resultCode = query->AddToFrameGraph(frameGraph);
            EXPECT_EQ(resultCode, QueryResultCode::Success);

            // Next frame
            queryPool->Update();

            resultCode = query->BeginQuery(context);
            EXPECT_EQ(resultCode, QueryResultCode::Fail);
        }

        // Test Query result
        {
            // Occlusion QueryPool result size is sizeof(uint64_t)
            EXPECT_EQ(queryPool->GetQueryResultSize(), sizeof(uint64_t));
        }

        // Test get result with invalid size
        {
            RHI::Ptr<RPI::Query> query = queryPool->CreateQuery(RHI::QueryPoolScopeAttachmentType::Global, RHI::ScopeAttachmentAccess::Write);

            EXPECT_TRUE(query.get());
            QueryResultCode resultCode = query->AddToFrameGraph(frameGraph);
            EXPECT_EQ(resultCode, QueryResultCode::Success);
            void* data = nullptr;
            // Query type of the pool is occlusion, which expects a result size of sizeof(uint64_t).
            resultCode = query->GetLatestResult(data, sizeof(uint32_t), RHI::MultiDevice::DefaultDeviceIndex);
            EXPECT_EQ(resultCode, QueryResultCode::Fail);
        }

        // Test going over QueryPool limit
        {
            // Temporary QueryPool with a limit of 1 Query
            AZStd::unique_ptr<RPI::QueryPool> tempRpiQueryPool = RPI::QueryPool::CreateQueryPool(1u, QueriesPerInstance, Type, StatisticsFlags);
            RHI::Ptr<RPI::Query> query = tempRpiQueryPool->CreateQuery(RHI::QueryPoolScopeAttachmentType::Global, RHI::ScopeAttachmentAccess::Write);
            EXPECT_TRUE(query.get());
            RHI::Ptr<RPI::Query> query1 = tempRpiQueryPool->CreateQuery(RHI::QueryPoolScopeAttachmentType::Global, RHI::ScopeAttachmentAccess::Write);
            EXPECT_TRUE(!query1.get());
        }

        // Test different Scopes for Begin() and End()
        {
            RHI::FrameGraphExecuteContext::Descriptor desc2;
            desc2.m_scopeId = RHI::ScopeId("Test");
            desc2.m_commandListIndex = 0;
            desc2.m_commandListCount = 1;
            desc2.m_commandList = nullptr;
            RHI::FrameGraphExecuteContext context2(desc2);

            RHI::Ptr<RPI::Query> query = queryPool->CreateQuery(RHI::QueryPoolScopeAttachmentType::Global, RHI::ScopeAttachmentAccess::Write);

            EXPECT_TRUE(query.get());
            QueryResultCode resultCode = query->AddToFrameGraph(frameGraph);
            EXPECT_EQ(resultCode, QueryResultCode::Success);
            resultCode = query->BeginQuery(context);
            EXPECT_EQ(resultCode, QueryResultCode::Success);
            resultCode = query->EndQuery(context2);
            EXPECT_EQ(resultCode, QueryResultCode::Fail);
        }
    }

    // Test occlusion QueryPool
    TEST_F(GpuQueryTests, TestOcclusionQueryPool)
    {
        const uint32_t QueryCount = 1024u;
        const uint32_t QueriesPerInstance = 1u;
        const RHI::QueryType Type = RHI::QueryType::Occlusion;
        const RHI::PipelineStatisticsFlags StatisticsFlags = RHI::PipelineStatisticsFlags::None;
        const uint32_t ResultSize = sizeof(uint64_t);

        uint64_t mockData;
        RHI::FrameGraphExecuteContext::Descriptor desc = {};
        uint64_t dummyCommandList;
        desc.m_commandList = reinterpret_cast<RHI::CommandList*>(&dummyCommandList);
        RHI::FrameGraphExecuteContext context(desc);

        RHI::Scope scope;
        scope.Init(RHI::ScopeId("StubScope"));
        RHI::FrameGraph frameGraph;
        frameGraph.BeginScope(scope);

        // Test get result of a Query successfully 
        {
            const uint32_t LoopCount = 4u;
            QueryResultCode resultCode;

            AZStd::unique_ptr<RPI::QueryPool> tempRpiQueryPool = RPI::QueryPool::CreateQueryPool(QueryCount, QueriesPerInstance, Type, StatisticsFlags);
            RHI::Ptr<RPI::Query> query = tempRpiQueryPool->CreateQuery(RHI::QueryPoolScopeAttachmentType::Global, RHI::ScopeAttachmentAccess::Write);
            EXPECT_TRUE(query.get());
            for (uint32_t i = 0u; i < LoopCount; i++)
            {
                QueryResultCode resultCode2;
                resultCode2 = query->AddToFrameGraph(frameGraph);
                EXPECT_EQ(resultCode2, QueryResultCode::Success);
                resultCode2 = query->BeginQuery(context);
                EXPECT_EQ(resultCode2, QueryResultCode::Success);
                resultCode2 = query->EndQuery(context);
                EXPECT_EQ(resultCode2, QueryResultCode::Success);
                tempRpiQueryPool->Update();
            }

            void* data = reinterpret_cast<void*>(&mockData);
            resultCode = query->GetLatestResult(data, ResultSize, context.GetDeviceIndex());
            EXPECT_EQ(resultCode, QueryResultCode::Success);
            resultCode = query->GetLatestResultAndWait(data, ResultSize, context.GetDeviceIndex());
            EXPECT_EQ(resultCode, QueryResultCode::Success);
        }
    }

    // Test statistics QueryPool
    TEST_F(GpuQueryTests, TestStatisticsQueryPool)
    {
        const uint32_t QueryCount = 1024u;
        const uint32_t QueriesPerInstance = 1u;
        const RHI::QueryType Type = RHI::QueryType::PipelineStatistics;
        const RHI::PipelineStatisticsFlags StatisticsFlags =
            RHI::PipelineStatisticsFlags::CInvocations |
            RHI::PipelineStatisticsFlags::CPrimitives |
            RHI::PipelineStatisticsFlags::CSInvocations |
            RHI::PipelineStatisticsFlags::DSInvocations;
        const uint32_t ResultSize = sizeof(uint64_t) * 4u;

        uint64_t mockData;
        RHI::FrameGraphExecuteContext::Descriptor desc = {};
        uint64_t dummyCommandList;
        desc.m_commandList = reinterpret_cast<RHI::CommandList*>(&dummyCommandList);
        RHI::FrameGraphExecuteContext context(desc);

        RHI::Scope scope;
        scope.Init(RHI::ScopeId("StubScope"));
        RHI::FrameGraph frameGraph;
        frameGraph.BeginScope(scope);

        AZStd::unique_ptr<RPI::QueryPool> queryPool = RPI::QueryPool::CreateQueryPool(QueryCount, QueriesPerInstance, Type, StatisticsFlags);

        // Test expected result size
        {
            const uint32_t resultSize = queryPool->GetQueryResultSize();
            EXPECT_EQ(resultSize, ResultSize);
        }

        // Test get result with invalid size
        {
            RHI::Ptr<RPI::Query> query = queryPool->CreateQuery(RHI::QueryPoolScopeAttachmentType::Global, RHI::ScopeAttachmentAccess::Write);
            EXPECT_TRUE(query.get());
            QueryResultCode resultCode = query->AddToFrameGraph(frameGraph);
            EXPECT_EQ(resultCode, QueryResultCode::Success);
            void* data = nullptr;
            // Query type of the pool is statistics, which expects a result size of sizeof(uint64_t) * number of active flags.
            resultCode = query->GetLatestResult(data, sizeof(uint64_t) * 3u, RHI::MultiDevice::DefaultDeviceIndex);
            EXPECT_EQ(resultCode, QueryResultCode::Fail);
        }

        // Test get result of a Query successfully 
        {
            const uint32_t LoopCount = 4u;
            QueryResultCode resultCode;

            AZStd::unique_ptr<RPI::QueryPool> tempRpiQueryPool = RPI::QueryPool::CreateQueryPool(QueryCount, QueriesPerInstance, Type, StatisticsFlags);

            RHI::Ptr<RPI::Query> query = tempRpiQueryPool->CreateQuery(RHI::QueryPoolScopeAttachmentType::Global, RHI::ScopeAttachmentAccess::Write);
            EXPECT_TRUE(query.get());
            for (uint32_t i = 0u; i < LoopCount; i++)
            {
                QueryResultCode resultCode2;
                resultCode2 = query->AddToFrameGraph(frameGraph);
                EXPECT_EQ(resultCode2, QueryResultCode::Success);
                resultCode2 = query->BeginQuery(context);
                EXPECT_EQ(resultCode2, QueryResultCode::Success);
                resultCode2 = query->EndQuery(context);
                EXPECT_EQ(resultCode2, QueryResultCode::Success);
                tempRpiQueryPool->Update();
            }

            void* data = reinterpret_cast<void*>(&mockData);
            resultCode = query->GetLatestResult(data, ResultSize, RHI::MultiDevice::DefaultDeviceIndex);
            EXPECT_EQ(resultCode, QueryResultCode::Success);
            resultCode = query->GetLatestResultAndWait(data, ResultSize, RHI::MultiDevice::DefaultDeviceIndex);
            EXPECT_EQ(resultCode, QueryResultCode::Success);
        }
    }

    // Test timestamp QueryPool
    TEST_F(GpuQueryTests, TestTimestampQueryPool)
    {
        const uint32_t QueryCount = 1024u;
        const uint32_t ResultSize = sizeof(uint64_t) * 2u;

        uint64_t mockData;
        RHI::FrameGraphExecuteContext::Descriptor desc = {};
        uint64_t dummyCommandList;
        desc.m_commandList = reinterpret_cast<RHI::CommandList*>(&dummyCommandList);
        RHI::FrameGraphExecuteContext context(desc);

        RHI::Scope scope;
        scope.Init(RHI::ScopeId("StubScope"));
        RHI::FrameGraph frameGraph;
        frameGraph.BeginScope(scope);

        // Test get result of a Query successfully 
        {
            const uint32_t LoopCount = 4u;
            QueryResultCode resultCode;

            AZStd::unique_ptr<RPI::QueryPool> tempRpiQueryPool = RPI::TimestampQueryPool::CreateTimestampQueryPool(QueryCount);

            RHI::Ptr<RPI::Query> query = tempRpiQueryPool->CreateQuery(RHI::QueryPoolScopeAttachmentType::Global, RHI::ScopeAttachmentAccess::Write);
            EXPECT_TRUE(query.get());
            for (uint32_t i = 0u; i < LoopCount; i++)
            {
                QueryResultCode resultCode2;
                resultCode2 = query->AddToFrameGraph(frameGraph);
                EXPECT_EQ(resultCode2, QueryResultCode::Success);
                resultCode2 = query->BeginQuery(context);
                EXPECT_EQ(resultCode2, QueryResultCode::Success);
                resultCode2 = query->EndQuery(context);
                EXPECT_EQ(resultCode2, QueryResultCode::Success);
                tempRpiQueryPool->Update();
            }

            void* data = reinterpret_cast<void*>(&mockData);
            resultCode = query->GetLatestResult(data, ResultSize, RHI::MultiDevice::DefaultDeviceIndex);
            EXPECT_EQ(resultCode, QueryResultCode::Success);
            resultCode = query->GetLatestResultAndWait(data, ResultSize, RHI::MultiDevice::DefaultDeviceIndex);
            EXPECT_EQ(resultCode, QueryResultCode::Success);
        }
    }

}; // namespace UnitTest
