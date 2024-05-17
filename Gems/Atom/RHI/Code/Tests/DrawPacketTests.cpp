/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "RHITestFixture.h"

#include <Atom/RHI/SingleDeviceDrawPacket.h>
#include <Atom/RHI/SingleDeviceDrawPacketBuilder.h>
#include <Atom/RHI/DrawListContext.h>
#include <Atom/RHI/DrawListTagRegistry.h>
#include <Atom/RHI/SingleDevicePipelineState.h>

#include <AzCore/Math/Random.h>
#include <AzCore/std/sort.h>

#include <Tests/Factory.h>

namespace UnitTest
{
    using namespace AZ;

    struct DrawItemData
    {
        DrawItemData(SimpleLcgRandom& random, const RHI::SingleDeviceBuffer* bufferEmpty, const RHI::SingleDevicePipelineState* psoEmpty)
        {
            m_pipelineState = psoEmpty;

            // Fill with deterministic random data to compare against.
            for (auto& streamBufferView : m_streamBufferViews)
            {
                streamBufferView = RHI::SingleDeviceStreamBufferView{ *bufferEmpty, random.GetRandom(), random.GetRandom(), random.GetRandom() };
            }

            m_tag = RHI::DrawListTag(random.GetRandom() % RHI::Limits::Pipeline::DrawListTagCountMax);
            m_stencilRef = static_cast<uint8_t>(random.GetRandom());
            m_sortKey = random.GetRandom();
        }

        AZStd::array<RHI::SingleDeviceStreamBufferView, RHI::Limits::Pipeline::StreamCountMax> m_streamBufferViews;

        const RHI::SingleDevicePipelineState* m_pipelineState;
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

            m_indexBufferView = RHI::SingleDeviceIndexBufferView(*m_bufferEmpty, random.GetRandom(), random.GetRandom(), RHI::IndexFormat::Uint16);
        }

        void ValidateDrawItem(const DrawItemData& drawItemData, RHI::SingleDeviceDrawItemProperties itemProperties) const
        {
            const RHI::SingleDeviceDrawItem* drawItem = itemProperties.m_item;

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

        const RHI::SingleDeviceDrawPacket* Build(RHI::SingleDeviceDrawPacketBuilder& builder)
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

                RHI::SingleDeviceDrawPacketBuilder::SingleDeviceDrawRequest drawRequest;
                drawRequest.m_listTag = drawItemData.m_tag;
                drawRequest.m_sortKey = drawItemData.m_sortKey;
                drawRequest.m_stencilRef = drawItemData.m_stencilRef;
                drawRequest.m_streamBufferViews = drawItemData.m_streamBufferViews;
                drawRequest.m_pipelineState = drawItemData.m_pipelineState;
                builder.AddDrawItem(drawRequest);
            }

            const RHI::SingleDeviceDrawPacket* drawPacket = builder.End();

            EXPECT_NE(drawPacket, nullptr);
            EXPECT_EQ(drawPacket->GetDrawListMask(), drawListMask);
            EXPECT_EQ(drawPacket->GetDrawItemCount(), m_drawItemDatas.size());

            for (size_t i = 0; i < m_drawItemDatas.size(); ++i)
            {
                ValidateDrawItem(m_drawItemDatas[i], drawPacket->GetDrawItemProperties(i));
            }

            return drawPacket;
        }

        RHI::Ptr<RHI::SingleDeviceBuffer> m_bufferEmpty;
        RHI::ConstPtr<RHI::SingleDevicePipelineState> m_psoEmpty;

        AZStd::array<RHI::Ptr<RHI::SingleDeviceShaderResourceGroup>, RHI::Limits::Pipeline::ShaderResourceGroupCountMax> m_srgs;
        AZStd::array<uint8_t, sizeof(unsigned int) * 4> m_rootConstants;
        RHI::SingleDeviceIndexBufferView m_indexBufferView;

        AZStd::vector<DrawItemData> m_drawItemDatas;
    };

    class DrawPacketTest
        : public RHITestFixture
    {
    protected:
        static const uint32_t s_randomSeed = 1234;

        RHI::Ptr<RHI::DrawListTagRegistry> m_drawListTagRegistry;
        RHI::DrawListContext m_drawListContext;

        AZStd::unique_ptr<AZ::RHI::RHISystem> m_rhiSystem;
        AZStd::unique_ptr<Factory> m_factory;

    public:
        void SetUp() override
        {
            RHITestFixture::SetUp();

            m_factory.reset(aznew Factory());
            m_drawListTagRegistry = RHI::DrawListTagRegistry::Create();

            m_rhiSystem.reset(aznew AZ::RHI::RHISystem);
            m_rhiSystem->InitDevices();
            m_rhiSystem->Init();
        }

        void TearDown() override
        {
            m_rhiSystem->Shutdown();
            m_rhiSystem.reset();

            m_drawListTagRegistry = nullptr;
            m_factory.reset();

            RHITestFixture::TearDown();
        }

        void TestDrawListTagRegistryNullCase()
        {
            RHI::DrawListTag nullTag = m_drawListTagRegistry->AcquireTag(Name());
            EXPECT_TRUE(nullTag.IsNull());
            EXPECT_EQ(m_drawListTagRegistry->GetAllocatedTagCount(), 0);

            m_drawListTagRegistry->ReleaseTag(nullTag);
            EXPECT_EQ(m_drawListTagRegistry->GetAllocatedTagCount(), 0);
        }

        void TestDrawListTagRegistrySimple()
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

        void TestDrawListTagRegistryDeAllocateAssert()
        {
            AZ_TEST_START_ASSERTTEST;
            EXPECT_EQ(m_drawListTagRegistry->GetAllocatedTagCount(), 0);

            const Name tagName{ "Test" };

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

        void TestDrawListTagRegistryRandomAllocations()
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

        void DrawPacketEmpty()
        {
            RHI::SingleDeviceDrawPacketBuilder builder;
            builder.Begin(nullptr);

            const RHI::SingleDeviceDrawPacket* drawPacket = builder.End();
            EXPECT_EQ(drawPacket, nullptr);
        }

        void DrawPacketNullItem()
        {
            RHI::SingleDeviceDrawPacketBuilder builder;
            builder.Begin(nullptr);

            RHI::SingleDeviceDrawPacketBuilder::SingleDeviceDrawRequest drawRequest;
            builder.AddDrawItem(drawRequest);

            const RHI::SingleDeviceDrawPacket* drawPacket = builder.End();
            EXPECT_EQ(drawPacket, nullptr);
        }

        void DrawPacketBuild()
        {
            AZ::SimpleLcgRandom random(s_randomSeed);

            DrawPacketData drawPacketData(random);

            RHI::SingleDeviceDrawPacketBuilder builder;
            const RHI::SingleDeviceDrawPacket* drawPacket = drawPacketData.Build(builder);
            delete drawPacket;
        }

        void DrawPacketBuildClearBuildNull()
        {
            AZ::SimpleLcgRandom random(s_randomSeed);
            DrawPacketData drawPacketData(random);

            RHI::SingleDeviceDrawPacketBuilder builder;
            const RHI::SingleDeviceDrawPacket* drawPacket = drawPacketData.Build(builder);
            delete drawPacket;

            // Try to build a 'null' packet. This should result in a null pointer.
            builder.Begin(nullptr);
            drawPacket = builder.End();
            EXPECT_EQ(drawPacket, nullptr);
        }

        void DrawListContextFilter()
        {
            AZ::SimpleLcgRandom random(s_randomSeed);
            DrawPacketData drawPacketData(random);

            RHI::SingleDeviceDrawPacketBuilder builder;
            const RHI::SingleDeviceDrawPacket* drawPacket = drawPacketData.Build(builder);

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

                listsByTag[tag.GetIndex()].push_back(drawPacket->GetDrawItemProperties(i));
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

        void DrawListContextNullFilter()
        {
            AZ::SimpleLcgRandom random(s_randomSeed);
            DrawPacketData drawPacketData(random);

            RHI::SingleDeviceDrawPacketBuilder builder;
            const RHI::SingleDeviceDrawPacket* drawPacket = drawPacketData.Build(builder);

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

        void DrawPacketClone()
        {
            AZ::SimpleLcgRandom random(s_randomSeed);

            DrawPacketData drawPacketData(random);

            RHI::SingleDeviceDrawPacketBuilder builder;
            const RHI::SingleDeviceDrawPacket* drawPacket = drawPacketData.Build(builder);

            RHI::SingleDeviceDrawPacketBuilder builder2;
            const RHI::SingleDeviceDrawPacket* drawPacketClone = builder2.Clone(drawPacket);

            EXPECT_EQ(drawPacket->m_drawItemCount, drawPacketClone->m_drawItemCount);
            EXPECT_EQ(drawPacket->m_streamBufferViewCount, drawPacketClone->m_streamBufferViewCount);
            EXPECT_EQ(drawPacket->m_shaderResourceGroupCount, drawPacketClone->m_shaderResourceGroupCount);
            EXPECT_EQ(drawPacket->m_uniqueShaderResourceGroupCount, drawPacketClone->m_uniqueShaderResourceGroupCount);
            EXPECT_EQ(drawPacket->m_rootConstantSize, drawPacketClone->m_rootConstantSize);
            EXPECT_EQ(drawPacket->m_scissorsCount, drawPacketClone->m_scissorsCount);
            EXPECT_EQ(drawPacket->m_viewportsCount, drawPacketClone->m_viewportsCount);

            uint8_t drawItemCount = drawPacket->m_drawItemCount;

            for (uint8_t i = 0; i < drawItemCount; ++i)
            {
                EXPECT_EQ(drawPacket->GetDrawListTag(i), drawPacketClone->GetDrawListTag(i));
                EXPECT_EQ(drawPacket->GetDrawFilterMask(i), drawPacketClone->GetDrawFilterMask(i));
                EXPECT_EQ(*(drawPacket->m_drawItemSortKeys + i), *(drawPacketClone->m_drawItemSortKeys + i));

                const RHI::SingleDeviceDrawItem* drawItem = drawPacket->m_drawItems + i;
                const RHI::SingleDeviceDrawItem* drawItemClone = drawPacketClone->m_drawItems + i;

                // Check the clone is an actual copy not an identical pointer.
                EXPECT_NE(drawItem, drawItemClone);

                EXPECT_EQ(drawItem->m_arguments.m_type, drawItemClone->m_arguments.m_type);
                EXPECT_EQ(drawItem->m_pipelineState->GetType(), drawItemClone->m_pipelineState->GetType());
                EXPECT_EQ(drawItem->m_stencilRef, drawItemClone->m_stencilRef);
                EXPECT_EQ(drawItem->m_streamBufferViewCount, drawItemClone->m_streamBufferViewCount);
                EXPECT_EQ(drawItem->m_shaderResourceGroupCount, drawItemClone->m_shaderResourceGroupCount);
                EXPECT_EQ(drawItem->m_rootConstantSize, drawItemClone->m_rootConstantSize);
                EXPECT_EQ(drawItem->m_scissorsCount, drawItemClone->m_scissorsCount);
                EXPECT_EQ(drawItem->m_viewportsCount, drawItemClone->m_viewportsCount);

                uint8_t streamBufferViewCount = drawItem->m_streamBufferViewCount;
                uint8_t shaderResourceGroupCount = drawItem->m_shaderResourceGroupCount;
                uint8_t rootConstantSize = drawItem->m_rootConstantSize;
                uint8_t scissorsCount = drawItem->m_scissorsCount;
                uint8_t viewportsCount = drawItem->m_viewportsCount;

                for (uint8_t j = 0; j < streamBufferViewCount; ++j)
                {
                    const RHI::SingleDeviceStreamBufferView* streamBufferView = drawPacket->m_streamBufferViews + j;
                    const RHI::SingleDeviceStreamBufferView* streamBufferViewClone = drawPacketClone->m_streamBufferViews + j;
                    EXPECT_EQ(streamBufferView->GetByteCount(), streamBufferViewClone->GetByteCount());
                    EXPECT_EQ(streamBufferView->GetByteOffset(), streamBufferViewClone->GetByteOffset());
                    EXPECT_EQ(streamBufferView->GetByteStride(), streamBufferViewClone->GetByteStride());
                    EXPECT_EQ(streamBufferView->GetHash(), streamBufferViewClone->GetHash());
                }

                for (uint8_t j = 0; j < shaderResourceGroupCount; ++j)
                {
                    EXPECT_EQ(*(drawItem->m_shaderResourceGroups + j), *(drawItemClone->m_shaderResourceGroups + j));
                }

                for (uint8_t j = 0; j < rootConstantSize; ++j)
                {
                    EXPECT_EQ(*(drawItem->m_rootConstants + j), *(drawItemClone->m_rootConstants + j));
                }

                for (uint8_t j = 0; j < scissorsCount; ++j)
                {
                    EXPECT_EQ(drawItem->m_scissors + j, drawItemClone->m_scissors + j);
                }

                for (uint8_t j = 0; j < viewportsCount; ++j)
                {
                    EXPECT_EQ(drawItem->m_viewports + j, drawItemClone->m_viewports + j);
                }
            }

            uint8_t streamBufferViewCount = drawPacket->m_streamBufferViewCount;
            uint8_t shaderResourceGroupCount = drawPacket->m_shaderResourceGroupCount;
            uint8_t uniqueShaderResourceGroupCount = drawPacket->m_uniqueShaderResourceGroupCount;
            uint8_t rootConstantSize = drawPacket->m_rootConstantSize;
            uint8_t scissorsCount = drawPacket->m_scissorsCount;
            uint8_t viewportsCount = drawPacket->m_viewportsCount;

            for (uint8_t i = 0; i < streamBufferViewCount; ++i)
            {
                const RHI::SingleDeviceStreamBufferView* streamBufferView = drawPacket->m_streamBufferViews + i;
                const RHI::SingleDeviceStreamBufferView* streamBufferViewClone = drawPacketClone->m_streamBufferViews + i;
                EXPECT_EQ(streamBufferView->GetByteCount(), streamBufferViewClone->GetByteCount());
                EXPECT_EQ(streamBufferView->GetByteOffset(), streamBufferViewClone->GetByteOffset());
                EXPECT_EQ(streamBufferView->GetByteStride(), streamBufferViewClone->GetByteStride());
                EXPECT_EQ(streamBufferView->GetHash(), streamBufferViewClone->GetHash());
            }

            for (uint8_t i = 0; i < shaderResourceGroupCount; ++i)
            {
                EXPECT_EQ(*(drawPacket->m_shaderResourceGroups + i), *(drawPacketClone->m_shaderResourceGroups + i));
            }

            for (uint8_t i = 0; i < uniqueShaderResourceGroupCount; ++i)
            {
                EXPECT_EQ(*(drawPacket->m_uniqueShaderResourceGroups + i), *(drawPacketClone->m_uniqueShaderResourceGroups + i));
            }

            for (uint8_t i = 0; i < rootConstantSize; ++i)
            {
                EXPECT_EQ(*(drawPacket->m_rootConstants + i), *(drawPacketClone->m_rootConstants + i));
            }

            for (uint8_t i = 0; i < scissorsCount; ++i)
            {
                EXPECT_EQ(drawPacket->m_scissors + i, drawPacketClone->m_scissors + i);
            }

            for (uint8_t i = 0; i < viewportsCount; ++i)
            {
                EXPECT_EQ(drawPacket->m_viewports + i, drawPacketClone->m_viewports + i);
            }

            delete drawPacket;
            delete drawPacketClone;
        }

        void TestSetInstanceCount()
        {
            AZ::SimpleLcgRandom random(s_randomSeed);

            DrawPacketData drawPacketData(random);

            RHI::SingleDeviceDrawPacketBuilder builder;
            const RHI::SingleDeviceDrawPacket* drawPacket = drawPacketData.Build(builder);
            RHI::SingleDeviceDrawPacketBuilder builder2;
            RHI::SingleDeviceDrawPacket* drawPacketClone = const_cast<RHI::SingleDeviceDrawPacket*>(builder2.Clone(drawPacket));

            uint8_t drawItemCount = drawPacketClone->m_drawItemCount;

            // Test default value
            for (uint8_t i = 0; i < drawItemCount; ++i)
            {
                const RHI::SingleDeviceDrawItem* drawItemClone = drawPacketClone->m_drawItems + i;
                EXPECT_EQ(drawItemClone->m_arguments.m_type, RHI::DrawType::Indexed);
                EXPECT_EQ(drawItemClone->m_arguments.m_indexed.m_instanceCount, 1);
            }

            drawPacketClone->SetInstanceCount(12);

            for (uint8_t i = 0; i < drawItemCount; ++i)
            {
                const RHI::SingleDeviceDrawItem* drawItemClone = drawPacketClone->m_drawItems + i;
                EXPECT_EQ(drawItemClone->m_arguments.m_indexed.m_instanceCount, 12);

                // Check that the original draw packet is not affected
                const RHI::SingleDeviceDrawItem* drawItem = drawPacket->m_drawItems + i;
                EXPECT_EQ(drawItem->m_arguments.m_indexed.m_instanceCount, 1);
            }

            delete drawPacket;
            delete drawPacketClone;
        }

        void TestSetRootConstants()
        {
            AZ::SimpleLcgRandom random(s_randomSeed);

            DrawPacketData drawPacketData(random);

            RHI::SingleDeviceDrawPacketBuilder builder;
            const RHI::SingleDeviceDrawPacket* drawPacket = drawPacketData.Build(builder);
            RHI::SingleDeviceDrawPacketBuilder builder2;
            RHI::SingleDeviceDrawPacket* drawPacketClone = const_cast<RHI::SingleDeviceDrawPacket*>(builder2.Clone(drawPacket));

            AZStd::array<uint8_t, sizeof(unsigned int) * 4> rootConstantOld;
            EXPECT_EQ(sizeof(unsigned int) * 4, drawPacketClone->m_rootConstantSize);

            // Keep a copy of old root constant for later verification
            for (uint8_t i = 0; i < drawPacketClone->m_rootConstantSize; ++i)
            {
                rootConstantOld[i] = drawPacketClone->m_rootConstants[i];
            }

            // Root constant data to be set, partial size as of the full root constant size.
            AZStd::array<uint8_t, sizeof(unsigned int) * 2> rootConstantNew = { 1, 2, 3, 4, 5, 6, 7, 8 };

            // Attempt to set beyond the array
            AZ_TEST_START_TRACE_SUPPRESSION;
            drawPacketClone->SetRootConstant(9, rootConstantNew);
            AZ_TEST_STOP_TRACE_SUPPRESSION(1);

            // Nothing will be set if it triggers the assert
            for (uint8_t i = 0; i < drawPacketClone->m_rootConstantSize; ++i)
            {
                EXPECT_EQ(rootConstantOld[i], drawPacketClone->m_rootConstants[i]);
            }

            drawPacketClone->SetRootConstant(8, rootConstantNew);

            // Compare the part staying the same.
            for (uint8_t i = 0; i < drawPacketClone->m_rootConstantSize - 8; ++i)
            {
                EXPECT_EQ(rootConstantOld[i], drawPacketClone->m_rootConstants[i]);
            }

            // Compare the part being set
            for (uint8_t i = drawPacketClone->m_rootConstantSize - 8; i < drawPacketClone->m_rootConstantSize; ++i)
            {
                EXPECT_EQ(rootConstantNew[i - (drawPacketClone->m_rootConstantSize - 8)], drawPacketClone->m_rootConstants[i]);
            }

            // Compare the origin which shouldn't be affected.
            for (uint8_t i = 0; i < drawPacket->m_rootConstantSize; ++i)
            {
                EXPECT_EQ(rootConstantOld[i], drawPacket->m_rootConstants[i]);
            }

            delete drawPacket;
            delete drawPacketClone;
        }
    };

    TEST_F(DrawPacketTest, TestDrawListTagRegistryNullCase)
    {
        TestDrawListTagRegistryNullCase();
    }

    TEST_F(DrawPacketTest, TestDrawListTagRegistrySimple)
    {
        TestDrawListTagRegistrySimple();
    }

    TEST_F(DrawPacketTest, TestDrawListTagRegistryDeAllocateAssert)
    {
        TestDrawListTagRegistryDeAllocateAssert();
    }

    TEST_F(DrawPacketTest, TestDrawListTagRegistryRandomAllocations)
    {
        TestDrawListTagRegistryRandomAllocations();
    }

    TEST_F(DrawPacketTest, DrawPacketEmpty)
    {
        DrawPacketEmpty();
    }

    TEST_F(DrawPacketTest, DrawPacketNullItem)
    {
        DrawPacketNullItem();
    }

    TEST_F(DrawPacketTest, DrawPacketBuild)
    {
        DrawPacketBuild();
    }

    TEST_F(DrawPacketTest, DrawPacketBuildClearBuildNull)
    {
        DrawPacketBuildClearBuildNull();
    }

    TEST_F(DrawPacketTest, DrawListContextFilter)
    {
        DrawListContextFilter();
    }

    TEST_F(DrawPacketTest, DrawListContextNullFilter)
    {
        DrawListContextNullFilter();
    }

    TEST_F(DrawPacketTest, DrawPacketClone)
    {
        DrawPacketClone();
    }

    TEST_F(DrawPacketTest, TestSetInstanceCount)
    {
        TestSetInstanceCount();
    }

    TEST_F(DrawPacketTest, TestSetRootConstants)
    {
        TestSetRootConstants();
    }
}

AZ_UNIT_TEST_HOOK(DEFAULT_UNIT_TEST_ENV);
