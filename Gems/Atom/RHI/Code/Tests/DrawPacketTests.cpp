/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "RHITestFixture.h"

#include <Atom/RHI/DrawPacket.h>
#include <Atom/RHI/DrawPacketBuilder.h>
#include <Atom/RHI/DrawListContext.h>
#include <Atom/RHI/DrawListTagRegistry.h>
#include <Atom/RHI/PipelineState.h>

#include <AzCore/Math/Random.h>
#include <AzCore/std/sort.h>

#include <Tests/Factory.h>

namespace UnitTest
{
    using namespace AZ;

    struct DrawItemData
    {
        DrawItemData(SimpleLcgRandom& random, const RHI::Buffer* bufferEmpty, const RHI::PipelineState* psoEmpty)
        {
            m_pipelineState = psoEmpty;

            // Fill with deterministic random data to compare against.
            for (auto& streamBufferView : m_streamBufferViews)
            {
                streamBufferView = RHI::StreamBufferView{ *bufferEmpty, random.GetRandom(), random.GetRandom(), random.GetRandom() };
            }

            m_tag = RHI::DrawListTag(random.GetRandom() % RHI::Limits::Pipeline::DrawListTagCountMax);
            m_stencilRef = static_cast<uint8_t>(random.GetRandom());
            m_sortKey = random.GetRandom();
        }

        AZStd::array<RHI::StreamBufferView, RHI::Limits::Pipeline::StreamCountMax> m_streamBufferViews;

        const RHI::PipelineState* m_pipelineState;
        RHI::DrawListTag m_tag;
        RHI::DrawItemSortKey m_sortKey;
        uint8_t m_stencilRef;
    };

    struct DrawPacketData
    {
        const size_t DrawItemCountMax = 8;

        DrawPacketData(SimpleLcgRandom& random)
        {
            m_bufferEmpty = RHI::Factory::Get().CreateBuffer();
            m_psoEmpty = RHI::Factory::Get().CreatePipelineState();

            for (auto& srg : m_srgs)
            {
                srg = RHI::Factory::Get().CreateShaderResourceGroup();
            }

            unsigned int* data = reinterpret_cast<unsigned int*>(m_rootConstants.data());
            for (uint32_t i = 0; i < m_rootConstants.size()/sizeof(unsigned int); ++i)
            {
                data[i] = random.GetRandom();
            }

            for (size_t i = 0; i < DrawItemCountMax; ++i)
            {
                m_drawItemDatas.emplace_back(random, m_bufferEmpty.get(), m_psoEmpty.get());
            }

            m_indexBufferView = RHI::IndexBufferView(*m_bufferEmpty, random.GetRandom(), random.GetRandom(), RHI::IndexFormat::Uint16);
        }

        void ValidateDrawItem(const DrawItemData& drawItemData, RHI::DrawItemProperties itemProperties) const
        {
            const RHI::DrawItem* drawItem = itemProperties.m_item;

            EXPECT_EQ(itemProperties.m_sortKey, drawItemData.m_sortKey);
            EXPECT_EQ(drawItem->m_stencilRef, drawItemData.m_stencilRef);
            EXPECT_EQ(drawItem->m_pipelineState, drawItemData.m_pipelineState);

            EXPECT_EQ(static_cast<size_t>(drawItem->m_streamBufferViewCount), drawItemData.m_streamBufferViews.size());
            for (size_t i = 0; i < static_cast<size_t>(drawItem->m_streamBufferViewCount); ++i)
            {
                EXPECT_EQ(drawItemData.m_streamBufferViews[i].GetHash(), drawItem->m_streamBufferViews[i].GetHash());
            }

            EXPECT_EQ(static_cast<size_t>(drawItem->m_shaderResourceGroupCount), m_srgs.size());
            for (size_t i = 0; i < m_srgs.size(); ++i)
            {
                EXPECT_EQ(m_srgs[i], drawItem->m_shaderResourceGroups[i]);
            }

            EXPECT_EQ(static_cast<size_t>(drawItem->m_rootConstantSize), m_rootConstants.size());
            for (size_t i = 0; i < m_rootConstants.size(); ++i)
            {
                EXPECT_EQ(m_rootConstants[i], drawItem->m_rootConstants[i]);
            }

            EXPECT_EQ(drawItem->m_indexBufferView->GetHash(), m_indexBufferView.GetHash());
        }

        const RHI::DrawPacket* Build(RHI::DrawPacketBuilder& builder)
        {
            builder.Begin(nullptr);

            for (auto& srgPtr : m_srgs)
            {
                builder.AddShaderResourceGroup(srgPtr.get());
            }

            builder.SetRootConstants(m_rootConstants);
            builder.SetIndexBufferView(m_indexBufferView);

            RHI::DrawListMask drawListMask;

            for (size_t i = 0; i < DrawItemCountMax; ++i)
            {
                const DrawItemData& drawItemData = m_drawItemDatas[i];
                drawListMask[drawItemData.m_tag.GetIndex()] = true;

                RHI::DrawPacketBuilder::DrawRequest drawRequest;
                drawRequest.m_listTag = drawItemData.m_tag;
                drawRequest.m_sortKey = drawItemData.m_sortKey;
                drawRequest.m_stencilRef = drawItemData.m_stencilRef;
                drawRequest.m_streamBufferViews = drawItemData.m_streamBufferViews;
                drawRequest.m_pipelineState = drawItemData.m_pipelineState;
                builder.AddDrawItem(drawRequest);
            }

            const RHI::DrawPacket* drawPacket = builder.End();

            EXPECT_NE(drawPacket, nullptr);
            EXPECT_EQ(drawPacket->GetDrawListMask(), drawListMask);
            EXPECT_EQ(drawPacket->GetDrawItemCount(), m_drawItemDatas.size());

            for (size_t i = 0; i < m_drawItemDatas.size(); ++i)
            {
                ValidateDrawItem(m_drawItemDatas[i], drawPacket->GetDrawItem(i));
            }

            return drawPacket;
        }

        RHI::Ptr<RHI::Buffer> m_bufferEmpty;
        RHI::ConstPtr<RHI::PipelineState> m_psoEmpty;

        AZStd::array<RHI::Ptr<RHI::ShaderResourceGroup>, RHI::Limits::Pipeline::ShaderResourceGroupCountMax> m_srgs;
        AZStd::array<uint8_t, sizeof(unsigned int) * 4> m_rootConstants;
        RHI::IndexBufferView m_indexBufferView;

        AZStd::vector<DrawItemData> m_drawItemDatas;
    };

    class DrawPacketTest
        : public RHITestFixture
    {
    public:
        void SetUp() override
        {
            RHITestFixture::SetUp();

            m_factory.reset(aznew Factory());
            m_drawListTagRegistry = RHI::DrawListTagRegistry::Create();
        }

        void TearDown() override
        {
            m_drawListTagRegistry = nullptr;
            m_factory.reset();

            RHITestFixture::TearDown();
        }

    protected:
        static const uint32_t s_randomSeed = 1234;

        RHI::Ptr<RHI::DrawListTagRegistry> m_drawListTagRegistry;
        RHI::DrawListContext m_drawListContext;

        AZStd::unique_ptr<Factory> m_factory;
    };

    TEST_F(DrawPacketTest, TestDrawListTagRegistryNullCase)
    {
        RHI::DrawListTag nullTag = m_drawListTagRegistry->AcquireTag(Name());
        EXPECT_TRUE(nullTag.IsNull());
        EXPECT_EQ(m_drawListTagRegistry->GetAllocatedTagCount(), 0);

        m_drawListTagRegistry->ReleaseTag(nullTag);
        EXPECT_EQ(m_drawListTagRegistry->GetAllocatedTagCount(), 0);
    }

    TEST_F(DrawPacketTest, TestDrawListTagRegistrySimple)
    {
        const Name forwardName1("Forward");
        const Name forwardName2("forward");

        RHI::DrawListTag forwardTag1 = m_drawListTagRegistry->AcquireTag(forwardName1);
        RHI::DrawListTag forwardTag2 = m_drawListTagRegistry->AcquireTag(forwardName2);

        EXPECT_FALSE(forwardTag1.IsNull());
        EXPECT_FALSE(forwardTag2.IsNull());
        EXPECT_NE(forwardTag1, forwardTag2);

        RHI::DrawListTag forwardTag3 = m_drawListTagRegistry->AcquireTag(forwardName1);
        EXPECT_EQ(forwardTag1, forwardTag3);

        m_drawListTagRegistry->ReleaseTag(forwardTag1);
        m_drawListTagRegistry->ReleaseTag(forwardTag2);
        m_drawListTagRegistry->ReleaseTag(forwardTag3);

        EXPECT_EQ(m_drawListTagRegistry->GetAllocatedTagCount(), 0);
    }

    TEST_F(DrawPacketTest, TestDrawListTagRegistryDeAllocateAssert)
    {
        AZ_TEST_START_ASSERTTEST;
        EXPECT_EQ(m_drawListTagRegistry->GetAllocatedTagCount(), 0);

        const Name tagName{"Test"};

        RHI::DrawListTag tag = m_drawListTagRegistry->AcquireTag(tagName);
        m_drawListTagRegistry->AcquireTag(tagName);
        m_drawListTagRegistry->AcquireTag(tagName);
        m_drawListTagRegistry->ReleaseTag(tag);
        m_drawListTagRegistry->ReleaseTag(tag);
        m_drawListTagRegistry->ReleaseTag(tag);

        // One additional forfeit should assert.
        m_drawListTagRegistry->ReleaseTag(tag);
        AZ_TEST_STOP_ASSERTTEST(1);
    }

    TEST_F(DrawPacketTest, TestDrawListTagRegistryRandomAllocations)
    {
        AZ::SimpleLcgRandom random(s_randomSeed);

        AZStd::vector<RHI::DrawListTag> acquiredTags;

        const uint32_t IterationCount = 1000;

        for (uint32_t iter = 0; iter < IterationCount; ++iter)
        {
            Name tagNameUnique = Name(AZStd::string::format("Tag_%d", iter));

            // Acquire
            if (random.GetRandom() % 2)
            {
                RHI::DrawListTag tag = m_drawListTagRegistry->AcquireTag(tagNameUnique);

                if (tag.IsNull())
                {
                    EXPECT_EQ(m_drawListTagRegistry->GetAllocatedTagCount(), RHI::Limits::Pipeline::DrawListTagCountMax);
                }
                else
                {
                    EXPECT_LT(m_drawListTagRegistry->GetAllocatedTagCount(), RHI::Limits::Pipeline::DrawListTagCountMax);
                    acquiredTags.emplace_back(tag);
                }
            }

            // Forfeit
            else if (!acquiredTags.empty())
            {
                size_t tagIndex = random.GetRandom() % static_cast<uint32_t>(acquiredTags.size());

                RHI::DrawListTag tag = acquiredTags[tagIndex];

                size_t allocationCountBefore = m_drawListTagRegistry->GetAllocatedTagCount();
                m_drawListTagRegistry->ReleaseTag(tag);
                size_t allocationCountAfter = m_drawListTagRegistry->GetAllocatedTagCount();

                EXPECT_EQ(allocationCountBefore - allocationCountAfter, 1);

                acquiredTags.erase(acquiredTags.begin() + tagIndex);
            }

            EXPECT_EQ(acquiredTags.size(), m_drawListTagRegistry->GetAllocatedTagCount());
        }

        // Erase all references, make sure the registry is empty again.
        for (RHI::DrawListTag tag : acquiredTags)
        {
            m_drawListTagRegistry->ReleaseTag(tag);
        }
        acquiredTags.clear();

        EXPECT_EQ(m_drawListTagRegistry->GetAllocatedTagCount(), 0);
    }

    TEST_F(DrawPacketTest, DrawPacketEmpty)
    {
        RHI::DrawPacketBuilder builder;
        builder.Begin(nullptr);

        const RHI::DrawPacket* drawPacket = builder.End();
        EXPECT_EQ(drawPacket, nullptr);
    }

    TEST_F(DrawPacketTest, DrawPacketNullItem)
    {
        RHI::DrawPacketBuilder builder;
        builder.Begin(nullptr);

        RHI::DrawPacketBuilder::DrawRequest drawRequest;
        builder.AddDrawItem(drawRequest);

        const RHI::DrawPacket* drawPacket = builder.End();
        EXPECT_EQ(drawPacket, nullptr);
    }

    TEST_F(DrawPacketTest, DrawPacketBuild)
    {
        AZ::SimpleLcgRandom random(s_randomSeed);

        DrawPacketData drawPacketData(random);

        RHI::DrawPacketBuilder builder;
        const RHI::DrawPacket* drawPacket = drawPacketData.Build(builder);
        delete drawPacket;
    }

    TEST_F(DrawPacketTest, DrawPacketBuildClearBuildNull)
    {
        AZ::SimpleLcgRandom random(s_randomSeed);
        DrawPacketData drawPacketData(random);

        RHI::DrawPacketBuilder builder;
        const RHI::DrawPacket* drawPacket = drawPacketData.Build(builder);
        delete drawPacket;

        // Try to build a 'null' packet. This should result in a null pointer.
        builder.Begin(nullptr);
        drawPacket = builder.End();
        EXPECT_EQ(drawPacket, nullptr);
    }

    TEST_F(DrawPacketTest, DrawListContextFilter)
    {
        AZ::SimpleLcgRandom random(s_randomSeed);
        DrawPacketData drawPacketData(random);

        RHI::DrawPacketBuilder builder;
        const RHI::DrawPacket* drawPacket = drawPacketData.Build(builder);

        RHI::DrawListContext drawListContext;
        drawListContext.Init(RHI::DrawListMask{}.set());
        drawListContext.AddDrawPacket(drawPacket);

        for (size_t i = 0; i < drawPacket->GetDrawItemCount(); ++i)
        {
            RHI::DrawListTag tag = drawPacket->GetDrawListTag(i);

            RHI::DrawListView drawList = drawListContext.GetList(tag);
            EXPECT_TRUE(drawList.empty());
        }

        drawListContext.FinalizeLists();

        RHI::DrawListsByTag listsByTag;
        for (size_t i = 0; i < drawPacket->GetDrawItemCount(); ++i)
        {
            RHI::DrawListTag tag = drawPacket->GetDrawListTag(i);

            listsByTag[tag.GetIndex()].push_back(drawPacket->GetDrawItem(i));
        }

        size_t tagIndex = 0;
        for (auto& drawList : listsByTag)
        {
            SortDrawList(drawList, RHI::DrawListSortType::KeyThenDepth);

            RHI::DrawListTag tag(tagIndex);

            RHI::DrawListView drawListView = drawListContext.GetList(tag);
            EXPECT_EQ(drawListView.size(), drawList.size());

            for (size_t i = 0; i < drawList.size(); ++i)
            {
                EXPECT_EQ(drawList[i], drawListView[i]);
            }

            tagIndex++;
        }

        drawListContext.Shutdown();

        delete drawPacket;
    }

    TEST_F(DrawPacketTest, DrawListContextNullFilter)
    {
        AZ::SimpleLcgRandom random(s_randomSeed);
        DrawPacketData drawPacketData(random);

        RHI::DrawPacketBuilder builder;
        const RHI::DrawPacket* drawPacket = drawPacketData.Build(builder);

        RHI::DrawListContext drawListContext;
        drawListContext.Init(RHI::DrawListMask{}); // Mask set to not contain any draw lists.
        drawListContext.AddDrawPacket(drawPacket);
        drawListContext.FinalizeLists();

        for (size_t i = 0; i < drawPacket->GetDrawItemCount(); ++i)
        {
            RHI::DrawListTag tag = drawPacket->GetDrawListTag(i);
            RHI::DrawListView drawList = drawListContext.GetList(tag);
            EXPECT_TRUE(drawList.empty());
        }

        drawListContext.Shutdown();

        delete drawPacket;
    }
}

AZ_UNIT_TEST_HOOK(DEFAULT_UNIT_TEST_ENV);
