/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "RHITestFixture.h"

#include <Atom/RHI.Reflect/RenderAttachmentLayout.h>
#include <Atom/RHI/DrawListContext.h>
#include <Atom/RHI/DrawListTagRegistry.h>
#include <Atom/RHI/MultiDeviceDrawPacket.h>
#include <Atom/RHI/MultiDeviceDrawPacketBuilder.h>
#include <Atom/RHI/MultiDevicePipelineState.h>
#include <Atom/RHI/MultiDeviceShaderResourceGroupPool.h>

#include <AzCore/Math/Random.h>
#include <AzCore/std/sort.h>

#include <Tests/Device.h>
#include <Tests/Factory.h>

namespace UnitTest
{
    using namespace AZ;

    //? TODO: May revert back to normal deviceCount and Mask
    static constexpr auto LocalDeviceCount{1};
    static constexpr auto LocalDeviceMask{RHI::MultiDevice::DefaultDevice};

    struct MultiDeviceDrawItemData
    {
        MultiDeviceDrawItemData(SimpleLcgRandom& random, const RHI::MultiDeviceBuffer* bufferEmpty, const RHI::MultiDevicePipelineState* psoEmpty)
        {
            m_pipelineState = psoEmpty;

            // Fill with deterministic random data to compare against.
            for (auto& streamBufferView : m_streamBufferViews)
            {
                streamBufferView =
                    RHI::MultiDeviceStreamBufferView{ *bufferEmpty, random.GetRandom(), random.GetRandom(), random.GetRandom() };
            }

            m_tag = RHI::DrawListTag(random.GetRandom() % RHI::Limits::Pipeline::DrawListTagCountMax);
            m_stencilRef = static_cast<uint8_t>(random.GetRandom());
            m_sortKey = random.GetRandom();
        }

        AZStd::array<RHI::MultiDeviceStreamBufferView, RHI::Limits::Pipeline::StreamCountMax> m_streamBufferViews;

        const RHI::MultiDevicePipelineState* m_pipelineState;
        RHI::DrawListTag m_tag;
        RHI::DrawItemSortKey m_sortKey;
        uint8_t m_stencilRef;
    };

    struct MultiDeviceDrawPacketData
    {
        static constexpr const size_t DrawItemCountMax = 8;

        MultiDeviceDrawPacketData(SimpleLcgRandom& random)
        {
            RHI::BufferPoolDescriptor bufferPoolDesc;
            m_bufferPool = aznew RHI::MultiDeviceBufferPool;
            m_bufferEmpty = aznew RHI::MultiDeviceBuffer;
            m_bufferPool->Init(LocalDeviceMask, bufferPoolDesc);
            RHI::MultiDeviceBufferInitRequest request;
            request.m_buffer = m_bufferEmpty.get();
            request.m_descriptor = RHI::BufferDescriptor{};
            m_bufferPool->InitBuffer(request);
            m_psoEmpty = aznew RHI::MultiDevicePipelineState;
            m_psoEmpty->m_deviceMask = LocalDeviceMask;
            m_psoEmpty->IterateDevices(
                [this](int deviceIndex)
                {
                    this->m_psoEmpty->m_deviceObjects[deviceIndex] = RHI::Factory::Get().CreatePipelineState();
                    return true;
                });

            for (auto& srg : m_srgs)
            {
                srg = aznew RHI::MultiDeviceShaderResourceGroup;
                srg->m_deviceMask = LocalDeviceMask;
                srg->IterateDevices(
                    [&srg](int deviceIndex)
                    {
                        srg->m_deviceObjects[deviceIndex] = RHI::Factory::Get().CreateShaderResourceGroup();
                        return true;
                    });
            }

            unsigned int* data = reinterpret_cast<unsigned int*>(m_rootConstants.data());
            for (uint32_t i = 0; i < m_rootConstants.size() / sizeof(unsigned int); ++i)
            {
                data[i] = random.GetRandom();
            }

            for (size_t i = 0; i < DrawItemCountMax; ++i)
            {
                m_drawItemDatas.emplace_back(random, m_bufferEmpty.get(), m_psoEmpty.get());
            }

            m_indexBufferView =
                RHI::MultiDeviceIndexBufferView(*m_bufferEmpty, random.GetRandom(), random.GetRandom(), RHI::IndexFormat::Uint16);
        }

        const auto Build(RHI::MultiDeviceDrawPacketBuilder& builder)
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
                const MultiDeviceDrawItemData& drawItemData = m_drawItemDatas[i];
                drawListMask[drawItemData.m_tag.GetIndex()] = true;

                RHI::MultiDeviceDrawPacketBuilder::MultiDeviceDrawRequest drawRequest;
                drawRequest.m_listTag = drawItemData.m_tag;
                drawRequest.m_sortKey = drawItemData.m_sortKey;
                drawRequest.m_stencilRef = drawItemData.m_stencilRef;
                drawRequest.m_streamBufferViews = drawItemData.m_streamBufferViews;
                drawRequest.m_pipelineState = drawItemData.m_pipelineState;
                builder.AddDrawItem(drawRequest);
            }

            const auto drawPacket = builder.End();

            EXPECT_NE(drawPacket, nullptr);
            EXPECT_EQ(drawPacket->GetDrawListMask(), drawListMask);
            EXPECT_EQ(drawPacket->GetDrawItemCount(), m_drawItemDatas.size());

            return drawPacket;
        }

        RHI::Ptr<RHI::MultiDeviceBufferPool> m_bufferPool;
        RHI::Ptr<RHI::MultiDeviceBuffer> m_bufferEmpty;
        RHI::Ptr<RHI::MultiDevicePipelineState> m_psoEmpty;

        RHI::Ptr<RHI::MultiDeviceShaderResourceGroupPool> m_srgPool;
        AZStd::array<RHI::Ptr<RHI::MultiDeviceShaderResourceGroup>, RHI::Limits::Pipeline::ShaderResourceGroupCountMax> m_srgs;
        AZStd::array<uint8_t, sizeof(unsigned int) * 4> m_rootConstants;
        RHI::MultiDeviceIndexBufferView m_indexBufferView;

        AZStd::vector<MultiDeviceDrawItemData> m_drawItemDatas;
    };

    class MultiDeviceDrawPacketTest : public MultiDeviceRHITestFixture
    {
    protected:
        static const uint32_t s_randomSeed = 1234;

        RHI::DrawListContext m_drawListContext;

        AZStd::unique_ptr<AZ::RHI::RHISystem> m_rhiSystem;
        AZStd::unique_ptr<Factory> m_factory;

    public:
        void SetUp() override
        {
            MultiDeviceRHITestFixture::SetUp();
        }

        void TearDown() override
        {
            MultiDeviceRHITestFixture::TearDown();
        }

        void DrawPacketEmpty()
        {
            RHI::MultiDeviceDrawPacketBuilder builder(LocalDeviceMask);
            builder.Begin(nullptr);

            const auto drawPacket = builder.End();
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

            MultiDeviceDrawPacketData drawPacketData(random);

            RHI::MultiDeviceDrawPacketBuilder builder(LocalDeviceMask);

            const auto drawPacket = drawPacketData.Build(builder);
        }

        void DrawPacketBuildClearBuildNull()
        {
            AZ::SimpleLcgRandom random(s_randomSeed);
            MultiDeviceDrawPacketData drawPacketData(random);

            RHI::MultiDeviceDrawPacketBuilder builder(LocalDeviceMask);
            auto drawPacket = drawPacketData.Build(builder);

            // Try to build a 'null' packet. This should result in a null pointer.
            builder.Begin(nullptr);
            drawPacket = builder.End();
            EXPECT_EQ(drawPacket.get(), nullptr);
        }

        //! Add these tests once DrawListContext takes MultiDeviceDrawPackets
        /*
        void DrawListContextFilter()
        {
            AZ::SimpleLcgRandom random(s_randomSeed);
            MultiDeviceDrawPacketData drawPacketData(random);

            RHI::MultiDeviceDrawPacketBuilder builder(LocalDeviceMask);
            const auto drawPacket = drawPacketData.Build(builder);

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
        */

        void DrawPacketClone()
        {
            AZ::SimpleLcgRandom random(s_randomSeed);

            MultiDeviceDrawPacketData drawPacketData(random);

            RHI::MultiDeviceDrawPacketBuilder builder(LocalDeviceMask);
            const auto drawPacket = drawPacketData.Build(builder);

            RHI::MultiDeviceDrawPacketBuilder builder2(LocalDeviceMask);
            const auto drawPacketClone = builder2.Clone(drawPacket.get());

            for (auto deviceIndex{ 0 }; deviceIndex < LocalDeviceCount; ++deviceIndex)
            {
                auto deviceDrawPacket{ drawPacket->GetDeviceDrawPacket(deviceIndex) };
                auto deviceDrawPacketClone{ drawPacketClone->GetDeviceDrawPacket(deviceIndex) };

                EXPECT_EQ(deviceDrawPacket->m_drawItemCount, deviceDrawPacketClone->m_drawItemCount);
                EXPECT_EQ(deviceDrawPacket->m_streamBufferViewCount, deviceDrawPacketClone->m_streamBufferViewCount);
                EXPECT_EQ(deviceDrawPacket->m_shaderResourceGroupCount, deviceDrawPacketClone->m_shaderResourceGroupCount);
                EXPECT_EQ(deviceDrawPacket->m_uniqueShaderResourceGroupCount, deviceDrawPacketClone->m_uniqueShaderResourceGroupCount);
                EXPECT_EQ(deviceDrawPacket->m_rootConstantSize, deviceDrawPacketClone->m_rootConstantSize);
                EXPECT_EQ(deviceDrawPacket->m_scissorsCount, deviceDrawPacketClone->m_scissorsCount);
                EXPECT_EQ(deviceDrawPacket->m_viewportsCount, deviceDrawPacketClone->m_viewportsCount);
            }

            const uint8_t drawItemCount =
                static_cast<uint8_t>(AZStd::min<size_t>(drawPacket->GetDrawItemCount(), MultiDeviceDrawPacketData::DrawItemCountMax));

            for (uint8_t i = 0; i < drawItemCount; ++i)
            {
                EXPECT_EQ(drawPacket->GetDrawListTag(i), drawPacketClone->GetDrawListTag(i));
                EXPECT_EQ(drawPacket->GetDrawFilterMask(i), drawPacketClone->GetDrawFilterMask(i));

                const auto* drawItem = drawPacket->GetDrawItem(i);
                const RHI::MultiDeviceDrawItem* drawItemClone = drawPacketClone->GetDrawItem(i);

                // Check the clone is an actual copy not an identical pointer.
                EXPECT_NE(drawItem, drawItemClone);

                for (auto deviceIndex{ 0 }; deviceIndex < LocalDeviceCount; ++deviceIndex)
                {
                    auto deviceDrawPacket{ drawPacket->GetDeviceDrawPacket(deviceIndex) };
                    auto deviceDrawPacketClone{ drawPacketClone->GetDeviceDrawPacket(deviceIndex) };
                    auto& deviceDrawItem{ drawItem->GetDeviceDrawItem(deviceIndex) };
                    auto& deviceDrawItemClone{ drawItemClone->GetDeviceDrawItem(deviceIndex) };

                    EXPECT_EQ(deviceDrawItem.m_arguments.m_type, deviceDrawItemClone.m_arguments.m_type);
                    EXPECT_EQ(deviceDrawItem.m_pipelineState->GetType(), deviceDrawItemClone.m_pipelineState->GetType());
                    EXPECT_EQ(deviceDrawItem.m_stencilRef, deviceDrawItemClone.m_stencilRef);
                    EXPECT_EQ(deviceDrawItem.m_streamBufferViewCount, deviceDrawItemClone.m_streamBufferViewCount);
                    EXPECT_EQ(deviceDrawItem.m_shaderResourceGroupCount, deviceDrawItemClone.m_shaderResourceGroupCount);
                    EXPECT_EQ(deviceDrawItem.m_rootConstantSize, deviceDrawItemClone.m_rootConstantSize);
                    EXPECT_EQ(deviceDrawItem.m_scissorsCount, deviceDrawItemClone.m_scissorsCount);
                    EXPECT_EQ(deviceDrawItem.m_viewportsCount, deviceDrawItemClone.m_viewportsCount);

                    uint8_t streamBufferViewCount = deviceDrawItem.m_streamBufferViewCount;
                    uint8_t shaderResourceGroupCount = deviceDrawItem.m_shaderResourceGroupCount;
                    uint8_t rootConstantSize = deviceDrawItem.m_rootConstantSize;
                    uint8_t scissorsCount = deviceDrawItem.m_scissorsCount;
                    uint8_t viewportsCount = deviceDrawItem.m_viewportsCount;

                    for (uint8_t j = 0; j < streamBufferViewCount; ++j)
                    {
                        const RHI::SingleDeviceStreamBufferView* streamBufferView = deviceDrawPacket->m_streamBufferViews + j;
                        const RHI::SingleDeviceStreamBufferView* streamBufferViewClone = deviceDrawPacketClone->m_streamBufferViews + j;
                        EXPECT_EQ(streamBufferView->GetByteCount(), streamBufferViewClone->GetByteCount());
                        EXPECT_EQ(streamBufferView->GetByteOffset(), streamBufferViewClone->GetByteOffset());
                        EXPECT_EQ(streamBufferView->GetByteStride(), streamBufferViewClone->GetByteStride());
                        EXPECT_EQ(streamBufferView->GetHash(), streamBufferViewClone->GetHash());
                    }

                    for (uint8_t j = 0; j < shaderResourceGroupCount; ++j)
                    {
                        EXPECT_EQ(*(deviceDrawItem.m_shaderResourceGroups + j), *(deviceDrawItemClone.m_shaderResourceGroups + j));
                    }

                    for (uint8_t j = 0; j < rootConstantSize; ++j)
                    {
                        EXPECT_EQ(*(deviceDrawItem.m_rootConstants + j), *(deviceDrawItemClone.m_rootConstants + j));
                    }

                    for (uint8_t j = 0; j < scissorsCount; ++j)
                    {
                        EXPECT_EQ(deviceDrawItem.m_scissors + j, deviceDrawItemClone.m_scissors + j);
                    }

                    for (uint8_t j = 0; j < viewportsCount; ++j)
                    {
                        EXPECT_EQ(deviceDrawItem.m_viewports + j, deviceDrawItemClone.m_viewports + j);
                    }
                }
            }

            for (auto deviceIndex{ 0 }; deviceIndex < LocalDeviceCount; ++deviceIndex)
            {
                auto deviceDrawPacket{ drawPacket->GetDeviceDrawPacket(deviceIndex) };
                auto deviceDrawPacketClone{ drawPacketClone->GetDeviceDrawPacket(deviceIndex) };

                uint8_t streamBufferViewCount = deviceDrawPacket->m_streamBufferViewCount;
                uint8_t shaderResourceGroupCount = deviceDrawPacket->m_shaderResourceGroupCount;
                uint8_t uniqueShaderResourceGroupCount = deviceDrawPacket->m_uniqueShaderResourceGroupCount;
                uint8_t rootConstantSize = deviceDrawPacket->m_rootConstantSize;
                uint8_t scissorsCount = deviceDrawPacket->m_scissorsCount;
                uint8_t viewportsCount = deviceDrawPacket->m_viewportsCount;

                for (uint8_t i = 0; i < streamBufferViewCount; ++i)
                {
                    const RHI::SingleDeviceStreamBufferView* streamBufferView = deviceDrawPacket->m_streamBufferViews + i;
                    const RHI::SingleDeviceStreamBufferView* streamBufferViewClone = deviceDrawPacketClone->m_streamBufferViews + i;
                    EXPECT_EQ(streamBufferView->GetByteCount(), streamBufferViewClone->GetByteCount());
                    EXPECT_EQ(streamBufferView->GetByteOffset(), streamBufferViewClone->GetByteOffset());
                    EXPECT_EQ(streamBufferView->GetByteStride(), streamBufferViewClone->GetByteStride());
                    EXPECT_EQ(streamBufferView->GetHash(), streamBufferViewClone->GetHash());
                }

                for (uint8_t i = 0; i < shaderResourceGroupCount; ++i)
                {
                    EXPECT_EQ(*(deviceDrawPacket->m_shaderResourceGroups + i), *(deviceDrawPacketClone->m_shaderResourceGroups + i));
                }

                for (uint8_t i = 0; i < uniqueShaderResourceGroupCount; ++i)
                {
                    EXPECT_EQ(
                        *(deviceDrawPacket->m_uniqueShaderResourceGroups + i), *(deviceDrawPacketClone->m_uniqueShaderResourceGroups + i));
                }

                for (uint8_t i = 0; i < rootConstantSize; ++i)
                {
                    EXPECT_EQ(*(deviceDrawPacket->m_rootConstants + i), *(deviceDrawPacketClone->m_rootConstants + i));
                }

                for (uint8_t i = 0; i < scissorsCount; ++i)
                {
                    EXPECT_EQ(deviceDrawPacket->m_scissors + i, deviceDrawPacketClone->m_scissors + i);
                }

                for (uint8_t i = 0; i < viewportsCount; ++i)
                {
                    EXPECT_EQ(deviceDrawPacket->m_viewports + i, deviceDrawPacketClone->m_viewports + i);
                }
            }
        }

        void TestSetInstanceCount()
        {
            AZ::SimpleLcgRandom random(s_randomSeed);

            MultiDeviceDrawPacketData drawPacketData(random);

            RHI::MultiDeviceDrawPacketBuilder builder(LocalDeviceMask);
            const auto drawPacket = drawPacketData.Build(builder);
            RHI::MultiDeviceDrawPacketBuilder builder2(LocalDeviceMask);
            auto drawPacketClone = builder2.Clone(drawPacket.get());

            const uint8_t drawItemCount =
                static_cast<uint8_t>(AZStd::min<size_t>(drawPacket->GetDrawItemCount(), MultiDeviceDrawPacketData::DrawItemCountMax));

            // Test default value
            for (uint8_t i = 0; i < drawItemCount; ++i)
            {
                for (auto deviceIndex{ 0 }; deviceIndex < LocalDeviceCount; ++deviceIndex)
                {
                    const auto& drawItemClone = drawPacketClone->m_drawItems[i].GetDeviceDrawItem(deviceIndex);
                    EXPECT_EQ(drawItemClone.m_arguments.m_type, RHI::DrawType::Indexed);
                    EXPECT_EQ(drawItemClone.m_arguments.m_indexed.m_instanceCount, 1);
                }
            }

            drawPacketClone->SetInstanceCount(12);

            for (uint8_t i = 0; i < drawItemCount; ++i)
            {
                for (auto deviceIndex{ 0 }; deviceIndex < LocalDeviceCount; ++deviceIndex)
                {
                    const auto& drawItemClone = drawPacketClone->m_drawItems[i].GetDeviceDrawItem(deviceIndex);
                    EXPECT_EQ(drawItemClone.m_arguments.m_indexed.m_instanceCount, 12);

                    // Check that the original draw packet is not affected
                    const auto& drawItem = drawPacket->m_drawItems[i].GetDeviceDrawItem(deviceIndex);
                    EXPECT_EQ(drawItem.m_arguments.m_indexed.m_instanceCount, 1);
                }
            }
        }

        void TestSetRootConstants()
        {
            AZ::SimpleLcgRandom random(s_randomSeed);

            MultiDeviceDrawPacketData drawPacketData(random);

            RHI::MultiDeviceDrawPacketBuilder builder(LocalDeviceMask);
            const auto drawPacket = drawPacketData.Build(builder);
            RHI::MultiDeviceDrawPacketBuilder builder2(LocalDeviceMask);
            RHI::Ptr<RHI::MultiDeviceDrawPacket> drawPacketClone = builder2.Clone(drawPacket.get());

            AZStd::vector<AZStd::array<uint8_t, sizeof(unsigned int) * 4>> rootConstantOld(LocalDeviceCount);
            for (auto deviceIndex{ 0 }; deviceIndex < LocalDeviceCount; ++deviceIndex)
            {
                auto deviceDrawPacketClone{ drawPacketClone->GetDeviceDrawPacket(deviceIndex) };
                EXPECT_EQ(sizeof(unsigned int) * 4, deviceDrawPacketClone->m_rootConstantSize);
            }

            // Keep a copy of old root constant for later verification
            for (auto deviceIndex{ 0 }; deviceIndex < LocalDeviceCount; ++deviceIndex)
            {
                auto deviceDrawPacketClone{ drawPacketClone->GetDeviceDrawPacket(deviceIndex) };
                for (uint8_t i = 0; i < deviceDrawPacketClone->m_rootConstantSize; ++i)
                {
                    rootConstantOld[deviceIndex][i] = deviceDrawPacketClone->m_rootConstants[i];
                }
            }

            // Root constant data to be set, partial size as of the full root constant size.
            AZStd::array<uint8_t, sizeof(unsigned int)* 2> rootConstantNew = { 1, 2, 3, 4, 5, 6, 7, 8 };

            // Attempt to set beyond the array
            AZ_TEST_START_TRACE_SUPPRESSION;
            drawPacketClone->SetRootConstant(9, rootConstantNew);
            AZ_TEST_STOP_TRACE_SUPPRESSION(1);

            // Nothing will be set if it triggers the assert
            for (auto deviceIndex{ 0 }; deviceIndex < LocalDeviceCount; ++deviceIndex)
            {
                auto deviceDrawPacketClone{ drawPacketClone->GetDeviceDrawPacket(deviceIndex) };
                for (uint8_t i = 0; i < deviceDrawPacketClone->m_rootConstantSize; ++i)
                {
                    EXPECT_EQ(rootConstantOld[deviceIndex][i], deviceDrawPacketClone->m_rootConstants[i]);
                }
            }

            drawPacketClone->SetRootConstant(8, rootConstantNew);

            for (auto deviceIndex{ 0 }; deviceIndex < LocalDeviceCount; ++deviceIndex)
            {
                auto deviceDrawPacket{ drawPacket->GetDeviceDrawPacket(deviceIndex) };
                auto deviceDrawPacketClone{ drawPacketClone->GetDeviceDrawPacket(deviceIndex) };
            
                // Compare the part staying the same.
                for (uint8_t i = 0; i < deviceDrawPacketClone->m_rootConstantSize - 8; ++i)
                {
                    EXPECT_EQ(rootConstantOld[deviceIndex][i], deviceDrawPacketClone->m_rootConstants[i]);
                }

                // Compare the part being set
                for (uint8_t i = deviceDrawPacketClone->m_rootConstantSize - 8; i < deviceDrawPacketClone->m_rootConstantSize; ++i)
                {
                    EXPECT_EQ(rootConstantNew[i - (deviceDrawPacketClone->m_rootConstantSize - 8)], deviceDrawPacketClone->m_rootConstants[i]);
                }

                // Compare the origin which shouldn't be affected.
                for (uint8_t i = 0; i < deviceDrawPacket->m_rootConstantSize; ++i)
                {
                    EXPECT_EQ(rootConstantOld[deviceIndex][i], deviceDrawPacket->m_rootConstants[i]);
                }
            }
        }
    };

    TEST_F(MultiDeviceDrawPacketTest, DrawPacketEmpty)
    {
        DrawPacketEmpty();
    }

    TEST_F(MultiDeviceDrawPacketTest, DrawPacketNullItem)
    {
        DrawPacketNullItem();
    }

    TEST_F(MultiDeviceDrawPacketTest, DrawPacketBuild)
    {
        DrawPacketBuild();
    }

    TEST_F(MultiDeviceDrawPacketTest, DrawPacketBuildClearBuildNull)
    {
        DrawPacketBuildClearBuildNull();
    }

    //! Add these tests once DrawListContext takes MultiDeviceDrawPackets
    /*
    TEST_F(MultiDeviceDrawPacketTest, DrawListContextFilter)
    {
        DrawListContextFilter();
    }

    TEST_F(MultiDeviceDrawPacketTest, DrawListContextNullFilter)
    {
        DrawListContextNullFilter();
    }
    */

    TEST_F(MultiDeviceDrawPacketTest, DrawPacketClone)
    {
        DrawPacketClone();
    }

    TEST_F(MultiDeviceDrawPacketTest, TestSetInstanceCount)
    {
        TestSetInstanceCount();
    }

    TEST_F(MultiDeviceDrawPacketTest, TestSetRootConstants)
    {
        TestSetRootConstants();
    }
} // namespace UnitTest
