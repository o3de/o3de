/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "RHITestFixture.h"

#include <Atom/RHI/DeviceDrawPacket.h>
#include <Atom/RHI/DeviceDrawPacketBuilder.h>
#include <Atom/RHI/DevicePipelineState.h>
#include <Atom/RHI/DrawListContext.h>
#include <Atom/RHI/DrawListTagRegistry.h>
#include <Atom/RHI/GeometryView.h>
#include <AzCore/Math/Random.h>
#include <AzCore/std/sort.h>

#include <Tests/Factory.h>

namespace UnitTest
{
    using namespace AZ;

    struct DrawItemData
    {
        DrawItemData(SimpleLcgRandom& random, const RHI::DevicePipelineState* psoEmpty, const RHI::DeviceGeometryView& geometryView)
        {
            m_pipelineState = psoEmpty;
            m_geometryView = &geometryView;
            m_streamIndices = geometryView.GetFullStreamBufferIndices();    // Ordered Stream Indices

            m_tag = RHI::DrawListTag(random.GetRandom() % RHI::Limits::Pipeline::DrawListTagCountMax);
            m_stencilRef = static_cast<uint8_t>(random.GetRandom());
            m_sortKey = random.GetRandom();
        }

        const RHI::DeviceGeometryView* m_geometryView = nullptr;
        RHI::StreamBufferIndices m_streamIndices;
        const RHI::DevicePipelineState* m_pipelineState;
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

            m_geometryView.SetDrawArguments( RHI::DrawIndexed{} );
            m_geometryView.SetIndexBufferView({ *m_bufferEmpty, random.GetRandom(), random.GetRandom(), RHI::IndexFormat::Uint16 });

            for (u32 i = 0; i < RHI::Limits::Pipeline::StreamCountMax; ++i)
            {
                m_geometryView.AddStreamBufferView({ *m_bufferEmpty, random.GetRandom(), random.GetRandom(), random.GetRandom() });
            }

            for (size_t i = 0; i < DrawItemCountMax; ++i)
            {
                m_drawItemDatas.emplace_back(random, m_psoEmpty.get(), m_geometryView);
            }
        }

        void ValidateDrawItem(const DrawItemData& drawItemData, RHI::DeviceDrawItemProperties itemProperties) const
        {
            const RHI::DeviceDrawItem* drawItem = itemProperties.m_item;

            EXPECT_EQ(itemProperties.m_sortKey, drawItemData.m_sortKey);
            EXPECT_EQ(drawItem->m_stencilRef, drawItemData.m_stencilRef);
            EXPECT_EQ(drawItem->m_pipelineState, drawItemData.m_pipelineState);
            EXPECT_EQ(drawItem->m_geometryView, drawItemData.m_geometryView);

            // Ordered Stream Indices (see matching comment in DrawItemData constructor)
            auto streamIter = drawItem->m_geometryView->CreateStreamIterator(drawItem->m_streamIndices);
            for (u8 i = 0; !streamIter.HasEnded(); ++i, ++streamIter)
            {
                EXPECT_EQ(drawItem->m_geometryView->GetStreamBufferView(i), *streamIter);
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
        }

        const RHI::DeviceDrawPacket* Build(RHI::DeviceDrawPacketBuilder& builder)
        {
            builder.Begin(nullptr);

            for (auto& srgPtr : m_srgs)
            {
                builder.AddShaderResourceGroup(srgPtr.get());
            }

            builder.SetRootConstants(m_rootConstants);
            builder.SetGeometryView(&m_geometryView);

            RHI::DrawListMask drawListMask;

            for (size_t i = 0; i < DrawItemCountMax; ++i)
            {
                const DrawItemData& drawItemData = m_drawItemDatas[i];
                drawListMask[drawItemData.m_tag.GetIndex()] = true;

                RHI::DeviceDrawPacketBuilder::DeviceDrawRequest drawRequest;
                drawRequest.m_streamIndices = drawItemData.m_streamIndices;
                drawRequest.m_listTag = drawItemData.m_tag;
                drawRequest.m_sortKey = drawItemData.m_sortKey;
                drawRequest.m_stencilRef = drawItemData.m_stencilRef;
                drawRequest.m_pipelineState = drawItemData.m_pipelineState;
                builder.AddDrawItem(drawRequest);
            }

            const RHI::DeviceDrawPacket* drawPacket = builder.End();

            EXPECT_NE(drawPacket, nullptr);
            EXPECT_EQ(drawPacket->GetDrawListMask(), drawListMask);
            EXPECT_EQ(drawPacket->GetDrawItemCount(), m_drawItemDatas.size());

            for (size_t i = 0; i < m_drawItemDatas.size(); ++i)
            {
                ValidateDrawItem(m_drawItemDatas[i], drawPacket->GetDrawItemProperties(i));
            }

            return drawPacket;
        }

        RHI::Ptr<RHI::DeviceBuffer> m_bufferEmpty;
        RHI::ConstPtr<RHI::DevicePipelineState> m_psoEmpty;

        AZStd::array<RHI::Ptr<RHI::DeviceShaderResourceGroup>, RHI::Limits::Pipeline::ShaderResourceGroupCountMax> m_srgs;
        AZStd::array<uint8_t, sizeof(unsigned int) * 4> m_rootConstants;
        RHI::DeviceGeometryView m_geometryView;

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
            RHI::DeviceDrawPacketBuilder builder;
            builder.Begin(nullptr);

            const RHI::DeviceDrawPacket* drawPacket = builder.End();
            EXPECT_EQ(drawPacket, nullptr);
        }

        void DrawPacketNullItem()
        {
            RHI::DeviceDrawPacketBuilder builder;
            builder.Begin(nullptr);

            RHI::DeviceDrawPacketBuilder::DeviceDrawRequest drawRequest;
            builder.AddDrawItem(drawRequest);

            const RHI::DeviceDrawPacket* drawPacket = builder.End();
            EXPECT_EQ(drawPacket, nullptr);
        }

        void DrawPacketBuild()
        {
            AZ::SimpleLcgRandom random(s_randomSeed);

            DrawPacketData drawPacketData(random);

            RHI::DeviceDrawPacketBuilder builder;
            const RHI::DeviceDrawPacket* drawPacket = drawPacketData.Build(builder);
            delete drawPacket;
        }

        void DrawPacketBuildClearBuildNull()
        {
            AZ::SimpleLcgRandom random(s_randomSeed);
            DrawPacketData drawPacketData(random);

            RHI::DeviceDrawPacketBuilder builder;
            const RHI::DeviceDrawPacket* drawPacket = drawPacketData.Build(builder);
            delete drawPacket;

            // Try to build a 'null' packet. This should result in a null pointer.
            builder.Begin(nullptr);
            drawPacket = builder.End();
            EXPECT_EQ(drawPacket, nullptr);
        }

        void DrawPacketClone()
        {
            AZ::SimpleLcgRandom random(s_randomSeed);

            DrawPacketData drawPacketData(random);

            RHI::DeviceDrawPacketBuilder builder;
            const RHI::DeviceDrawPacket* drawPacket = drawPacketData.Build(builder);

            RHI::DeviceDrawPacketBuilder builder2;
            const RHI::DeviceDrawPacket* drawPacketClone = builder2.Clone(drawPacket);

            EXPECT_EQ(drawPacket->m_drawItemCount, drawPacketClone->m_drawItemCount);
            EXPECT_EQ(drawPacket->m_geometryView, drawPacketClone->m_geometryView);
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

                const RHI::DeviceDrawItem* drawItem = drawPacket->m_drawItems + i;
                const RHI::DeviceDrawItem* drawItemClone = drawPacketClone->m_drawItems + i;

                // Check the clone is an actual copy not an identical pointer.
                EXPECT_NE(drawItem, drawItemClone);
                EXPECT_EQ(drawItem->m_pipelineState->GetType(), drawItemClone->m_pipelineState->GetType());
                EXPECT_EQ(drawItem->m_stencilRef, drawItemClone->m_stencilRef);
                EXPECT_EQ(drawItem->m_geometryView, drawItemClone->m_geometryView);
                EXPECT_EQ(drawItem->m_streamIndices, drawItemClone->m_streamIndices);
                EXPECT_EQ(drawItem->m_shaderResourceGroupCount, drawItemClone->m_shaderResourceGroupCount);
                EXPECT_EQ(drawItem->m_rootConstantSize, drawItemClone->m_rootConstantSize);
                EXPECT_EQ(drawItem->m_scissorsCount, drawItemClone->m_scissorsCount);
                EXPECT_EQ(drawItem->m_viewportsCount, drawItemClone->m_viewportsCount);

                uint8_t shaderResourceGroupCount = drawItem->m_shaderResourceGroupCount;
                uint8_t rootConstantSize = drawItem->m_rootConstantSize;
                uint8_t scissorsCount = drawItem->m_scissorsCount;
                uint8_t viewportsCount = drawItem->m_viewportsCount;

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

            uint8_t shaderResourceGroupCount = drawPacket->m_shaderResourceGroupCount;
            uint8_t uniqueShaderResourceGroupCount = drawPacket->m_uniqueShaderResourceGroupCount;
            uint8_t rootConstantSize = drawPacket->m_rootConstantSize;
            uint8_t scissorsCount = drawPacket->m_scissorsCount;
            uint8_t viewportsCount = drawPacket->m_viewportsCount;

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

            RHI::DeviceDrawPacketBuilder builder;
            const RHI::DeviceDrawPacket* drawPacket = drawPacketData.Build(builder);
            RHI::DeviceDrawPacketBuilder builder2;
            RHI::DeviceDrawPacket* drawPacketClone = const_cast<RHI::DeviceDrawPacket*>(builder2.Clone(drawPacket));

            uint8_t drawItemCount = drawPacketClone->m_drawItemCount;

            // Test default value
            EXPECT_EQ(drawPacketClone->m_geometryView->GetDrawArguments().m_type, RHI::DrawType::Indexed);
            for (uint8_t i = 0; i < drawItemCount; ++i)
            {
                const RHI::DeviceDrawItem* drawItemClone = drawPacketClone->m_drawItems + i;
                EXPECT_EQ(drawItemClone->m_drawInstanceArgs.m_instanceCount, 1);
            }

            drawPacketClone->SetInstanceCount(12);

            for (uint8_t i = 0; i < drawItemCount; ++i)
            {
                const RHI::DeviceDrawItem* drawItemClone = drawPacketClone->m_drawItems + i;
                EXPECT_EQ(drawItemClone->m_drawInstanceArgs.m_instanceCount, 12);

                // Check that the original draw packet is not affected
                const RHI::DeviceDrawItem* drawItem = drawPacket->m_drawItems + i;
                EXPECT_EQ(drawItem->m_drawInstanceArgs.m_instanceCount, 1);
            }

            delete drawPacket;
            delete drawPacketClone;
        }

        void TestSetRootConstants()
        {
            AZ::SimpleLcgRandom random(s_randomSeed);

            DrawPacketData drawPacketData(random);

            RHI::DeviceDrawPacketBuilder builder;
            const RHI::DeviceDrawPacket* drawPacket = drawPacketData.Build(builder);
            RHI::DeviceDrawPacketBuilder builder2;
            RHI::DeviceDrawPacket* drawPacketClone = const_cast<RHI::DeviceDrawPacket*>(builder2.Clone(drawPacket));

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
