/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include <Common/RPITestFixture.h>
#include <Common/ShaderAssetTestUtils.h>

#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>
#include <Atom/RPI.Public/Buffer/Buffer.h>
#include <Atom/RPI.Reflect/Buffer/BufferAssetCreator.h>
#include <Atom/RPI.Reflect/ResourcePoolAssetCreator.h>

namespace UnitTest
{
    using namespace AZ;
    using namespace RPI;

    class ShaderResourceGroupBufferTests : public RPITestFixture
    {
    protected:
        Data::Asset<ShaderAsset> m_testSrgShaderAsset;
        RHI::Ptr<RHI::ShaderResourceGroupLayout> m_testSrgLayout;
        Data::Instance<ShaderResourceGroup> m_testSrg;
        Data::Asset<ResourcePoolAsset> m_bufferPoolAsset;
        Data::Asset<BufferAsset> m_shortBufferAsset;
        Data::Asset<BufferAsset> m_mediumBufferAsset;
        Data::Asset<BufferAsset> m_longBufferAsset;
        Data::Instance<Buffer> m_shortBuffer;
        Data::Instance<Buffer> m_mediumBuffer;
        Data::Instance<Buffer> m_longBuffer;
        Ptr<RHI::BufferView> m_bufferViewA;
        Ptr<RHI::BufferView> m_bufferViewB;
        Ptr<RHI::BufferView> m_bufferViewC;
        AZStd::vector<Data::Instance<Buffer>> m_threeBuffers;
        AZStd::vector<const RHI::BufferView*> m_threeBufferViews;
        const RHI::ShaderInputBufferIndex m_indexOfBufferA{ 0 };
        const RHI::ShaderInputBufferIndex m_indexOfBufferB{ 1 };
        const RHI::ShaderInputBufferIndex m_indexOfBufferArray{ 2 };
        const RHI::ShaderInputBufferIndex m_indexOfBufferInvalid{ 3 };

        RHI::Ptr<RHI::ShaderResourceGroupLayout> CreateTestSrgLayout(const char* nameId)
        {
            RHI::Ptr<RHI::ShaderResourceGroupLayout> srgLayout = RHI::ShaderResourceGroupLayout::Create();

            srgLayout->SetName(Name(nameId));
            srgLayout->SetBindingSlot(0);
            srgLayout->AddShaderInput(RHI::ShaderInputBufferDescriptor{
                Name{ "MyBufferA" }, RHI::ShaderInputBufferAccess::Read, RHI::ShaderInputBufferType::Raw, 1, 4, 1, 1 });
            srgLayout->AddShaderInput(RHI::ShaderInputBufferDescriptor{
                Name{ "MyBufferB" }, RHI::ShaderInputBufferAccess::Read, RHI::ShaderInputBufferType::Raw, 1, 4, 2, 2 });
            srgLayout->AddShaderInput(RHI::ShaderInputBufferDescriptor{
                Name{ "MyBufferArray" }, RHI::ShaderInputBufferAccess::Read, RHI::ShaderInputBufferType::Raw, 3, 4, 3, 3 });
            srgLayout->Finalize();

            return srgLayout;
        }

        Data::Asset<ResourcePoolAsset> CreateTestBufferPoolAsset()
        {
            Data::Asset<ResourcePoolAsset> asset;

            auto bufferPoolDesc = AZStd::make_unique<RHI::BufferPoolDescriptor>();
            bufferPoolDesc->m_bindFlags = RHI::BufferBindFlags::ShaderRead;
            bufferPoolDesc->m_heapMemoryLevel = RHI::HeapMemoryLevel::Host;

            ResourcePoolAssetCreator creator;
            creator.Begin(Uuid::CreateRandom());
            creator.SetPoolDescriptor(AZStd::move(bufferPoolDesc));
            creator.SetPoolName("TestPool");
            creator.End(asset);

            return asset;
        }

        Data::Asset<BufferAsset> CreateTestBufferAsset(const char* bufferStringContent)
        {
            const uint32_t bufferSize = static_cast<uint32_t>(strlen(bufferStringContent));

            Data::Asset<BufferAsset> asset;
            BufferAssetCreator bufferCreator;

            bufferCreator.Begin(Uuid::CreateRandom());
            bufferCreator.SetBuffer(bufferStringContent, bufferSize, RHI::BufferDescriptor{ RHI::BufferBindFlags::ShaderRead, bufferSize });
            bufferCreator.SetBufferViewDescriptor(RHI::BufferViewDescriptor::CreateRaw(0, bufferSize));
            bufferCreator.SetPoolAsset(m_bufferPoolAsset);
            bufferCreator.End(asset);

            return asset;
        }

        void SetUp() override
        {
            RPITestFixture::SetUp();

            m_testSrgLayout = CreateTestSrgLayout("TestSrg");
            m_testSrgShaderAsset = CreateTestShaderAsset(Uuid::CreateRandom(), m_testSrgLayout);
            m_testSrg = ShaderResourceGroup::Create(m_testSrgShaderAsset, AZ::RPI::DefaultSupervariantIndex, m_testSrgLayout->GetName());

            m_bufferPoolAsset = CreateTestBufferPoolAsset();
            m_shortBufferAsset = CreateTestBufferAsset("Short");
            m_mediumBufferAsset = CreateTestBufferAsset("Medium length buffer");
            m_longBufferAsset = CreateTestBufferAsset("This buffer is longer than the other two");

            m_shortBuffer = Buffer::FindOrCreate(m_shortBufferAsset);
            m_mediumBuffer = Buffer::FindOrCreate(m_mediumBufferAsset);
            m_longBuffer = Buffer::FindOrCreate(m_longBufferAsset);

            m_threeBuffers = { m_shortBuffer, m_mediumBuffer, m_longBuffer };
            m_bufferViewA = m_longBuffer->GetRHIBuffer()->BuildBufferView(RHI::BufferViewDescriptor::CreateRaw(5, 6));
            m_bufferViewB = m_longBuffer->GetRHIBuffer()->BuildBufferView(RHI::BufferViewDescriptor::CreateRaw(15, 4));
            m_bufferViewC = m_longBuffer->GetRHIBuffer()->BuildBufferView(RHI::BufferViewDescriptor::CreateRaw(22, 18));

            m_threeBufferViews = { m_bufferViewA.get(), m_bufferViewB.get(), m_bufferViewC.get() };
        }

        void TearDown() override
        {
            m_testSrgLayout = nullptr;
            m_testSrg = nullptr;
            m_testSrgShaderAsset.Reset();
            m_shortBufferAsset.Reset();
            m_mediumBufferAsset.Reset();
            m_longBufferAsset.Reset();
            m_bufferPoolAsset.Reset();

            m_threeBuffers = AZStd::vector<Data::Instance<Buffer>>();
            m_threeBufferViews = AZStd::vector<const RHI::BufferView*>();
            m_bufferViewA = nullptr;
            m_bufferViewB = nullptr;
            m_bufferViewC = nullptr;
            m_shortBuffer = nullptr;
            m_mediumBuffer = nullptr;
            m_longBuffer = nullptr;

            RPITestFixture::TearDown();
        }
    };

    TEST_F(ShaderResourceGroupBufferTests, TestInvalidInputIndex)
    {
        // Test invalid indexes for all get/set functions

        const int bufferInvalidArrayOffset = 3;

        AZ_TEST_START_ASSERTTEST;

        // Buffers...

        EXPECT_FALSE(m_testSrg->SetBuffer(m_indexOfBufferInvalid, m_shortBuffer));
        EXPECT_FALSE(m_testSrg->GetBuffer(m_indexOfBufferInvalid));

        EXPECT_FALSE(m_testSrg->SetBuffer(m_indexOfBufferInvalid, m_shortBuffer, 1));
        EXPECT_FALSE(m_testSrg->SetBuffer(m_indexOfBufferArray, m_shortBuffer, bufferInvalidArrayOffset));
        EXPECT_FALSE(m_testSrg->GetBuffer(m_indexOfBufferInvalid, 1));
        EXPECT_FALSE(m_testSrg->GetBuffer(m_indexOfBufferArray, bufferInvalidArrayOffset));

        EXPECT_FALSE(m_testSrg->SetBufferArray(m_indexOfBufferInvalid, m_threeBuffers));
        EXPECT_FALSE(m_testSrg->SetBufferArray(m_indexOfBufferInvalid, m_threeBuffers, 0));
        EXPECT_EQ(0, m_testSrg->GetBufferArray(m_indexOfBufferInvalid).size());

        // Buffer Views...

        EXPECT_FALSE(m_testSrg->SetBufferView(m_indexOfBufferInvalid, m_bufferViewA.get()));
        EXPECT_FALSE(m_testSrg->GetBufferView(m_indexOfBufferInvalid));

        EXPECT_FALSE(m_testSrg->SetBufferView(m_indexOfBufferInvalid, m_bufferViewA.get(), 1));
        EXPECT_FALSE(m_testSrg->GetBufferView(m_indexOfBufferInvalid, 1));

        EXPECT_FALSE(m_testSrg->SetBufferView(m_indexOfBufferArray, m_bufferViewA.get(), bufferInvalidArrayOffset));
        EXPECT_FALSE(m_testSrg->GetBufferView(m_indexOfBufferArray, bufferInvalidArrayOffset));

        EXPECT_FALSE(m_testSrg->SetBufferViewArray(m_indexOfBufferInvalid, m_threeBufferViews));
        EXPECT_FALSE(m_testSrg->SetBufferViewArray(m_indexOfBufferInvalid, m_threeBufferViews, 0));
        EXPECT_EQ(0, m_testSrg->GetBufferViewArray(m_indexOfBufferInvalid).size());

        AZ_TEST_STOP_ASSERTTEST(18);
    }

    TEST_F(ShaderResourceGroupBufferTests, TestSetGetBuffer)
    {
        // Test basic set/get operation...

        EXPECT_TRUE(m_testSrg->SetBuffer(m_indexOfBufferA, m_shortBuffer));
        EXPECT_TRUE(m_testSrg->SetBuffer(m_indexOfBufferB, m_mediumBuffer));
        EXPECT_EQ(m_shortBuffer, m_testSrg->GetBuffer(m_indexOfBufferA));
        EXPECT_EQ(m_mediumBuffer, m_testSrg->GetBuffer(m_indexOfBufferB));

        m_testSrg->Compile();
        EXPECT_EQ(m_shortBuffer->GetBufferView(), m_testSrg->GetBufferView(m_indexOfBufferA, 0));
        EXPECT_EQ(m_mediumBuffer->GetBufferView(), m_testSrg->GetBufferView(m_indexOfBufferB, 0));

        // Test changing back to null...

        ProcessQueuedSrgCompilations(m_testSrgShaderAsset, m_testSrgLayout->GetName());

        EXPECT_TRUE(m_testSrg->SetBuffer(m_indexOfBufferA, nullptr));
        m_testSrg->Compile();
        EXPECT_EQ(nullptr, m_testSrg->GetBuffer(m_indexOfBufferA));
        EXPECT_EQ(nullptr, m_testSrg->GetBufferView(m_indexOfBufferA, 0));
    }

    TEST_F(ShaderResourceGroupBufferTests, TestSetGetBufferAtOffset)
    {
        // Test basic set/get operation...

        EXPECT_TRUE(m_testSrg->SetBuffer(m_indexOfBufferArray, m_shortBuffer, 0));
        EXPECT_TRUE(m_testSrg->SetBuffer(m_indexOfBufferArray, m_mediumBuffer, 1));
        EXPECT_TRUE(m_testSrg->SetBuffer(m_indexOfBufferArray, m_longBuffer, 2));
        EXPECT_EQ(m_shortBuffer, m_testSrg->GetBuffer(m_indexOfBufferArray, 0));
        EXPECT_EQ(m_mediumBuffer, m_testSrg->GetBuffer(m_indexOfBufferArray, 1));
        EXPECT_EQ(m_longBuffer, m_testSrg->GetBuffer(m_indexOfBufferArray, 2));

        m_testSrg->Compile();
        EXPECT_EQ(m_shortBuffer->GetBufferView(), m_testSrg->GetBufferView(m_indexOfBufferArray, 0));
        EXPECT_EQ(m_mediumBuffer->GetBufferView(), m_testSrg->GetBufferView(m_indexOfBufferArray, 1));
        EXPECT_EQ(m_longBuffer->GetBufferView(), m_testSrg->GetBufferView(m_indexOfBufferArray, 2));

        // Test changing back to null...

        ProcessQueuedSrgCompilations(m_testSrgShaderAsset, m_testSrgLayout->GetName());

        EXPECT_TRUE(m_testSrg->SetBuffer(m_indexOfBufferArray, nullptr, 1));
        m_testSrg->Compile();
        EXPECT_EQ(nullptr, m_testSrg->GetBuffer(m_indexOfBufferArray, 1));
        EXPECT_EQ(nullptr, m_testSrg->GetBufferView(m_indexOfBufferArray, 1));
    }

    TEST_F(ShaderResourceGroupBufferTests, TestSetGetBufferArray)
    {
        // Test basic set/get operation...

        EXPECT_TRUE(m_testSrg->SetBufferArray(m_indexOfBufferArray, m_threeBuffers));
        m_testSrg->Compile();

        EXPECT_EQ(3, m_testSrg->GetBufferArray(m_indexOfBufferArray).size());
        EXPECT_EQ(m_threeBuffers[0], m_testSrg->GetBufferArray(m_indexOfBufferArray)[0]);
        EXPECT_EQ(m_threeBuffers[1], m_testSrg->GetBufferArray(m_indexOfBufferArray)[1]);
        EXPECT_EQ(m_threeBuffers[2], m_testSrg->GetBufferArray(m_indexOfBufferArray)[2]);
        EXPECT_EQ(m_threeBuffers[0], m_testSrg->GetBuffer(m_indexOfBufferArray, 0));
        EXPECT_EQ(m_threeBuffers[1], m_testSrg->GetBuffer(m_indexOfBufferArray, 1));
        EXPECT_EQ(m_threeBuffers[2], m_testSrg->GetBuffer(m_indexOfBufferArray, 2));
        EXPECT_EQ(m_threeBuffers[0]->GetBufferView(), m_testSrg->GetBufferView(m_indexOfBufferArray, 0));
        EXPECT_EQ(m_threeBuffers[1]->GetBufferView(), m_testSrg->GetBufferView(m_indexOfBufferArray, 1));
        EXPECT_EQ(m_threeBuffers[2]->GetBufferView(), m_testSrg->GetBufferView(m_indexOfBufferArray, 2));

        // Test replacing just two buffers including changing one buffer back to null...

        ProcessQueuedSrgCompilations(m_testSrgShaderAsset, m_testSrgLayout->GetName());

        AZStd::vector<Data::Instance<Buffer>> alternateBuffers = { m_mediumBuffer, nullptr };

        EXPECT_TRUE(m_testSrg->SetBufferArray(m_indexOfBufferArray, alternateBuffers));
        m_testSrg->Compile();

        EXPECT_TRUE(m_testSrg->GetBufferArray(m_indexOfBufferArray).size());
        EXPECT_EQ(m_mediumBuffer, m_testSrg->GetBuffer(m_indexOfBufferArray, 0));
        EXPECT_EQ(nullptr, m_testSrg->GetBuffer(m_indexOfBufferArray, 1));
        EXPECT_EQ(m_longBuffer, m_testSrg->GetBuffer(m_indexOfBufferArray, 2)); // remains unchanged
    }

    TEST_F(ShaderResourceGroupBufferTests, TestSetGetBufferArray_ValidationFailure)
    {
        // Make sure the no changes are made when a validation failure is detected

        AZStd::vector<Data::Instance<Buffer>> tooManyBuffers{ 4, m_shortBuffer };

        AZ_TEST_START_ASSERTTEST;
        EXPECT_FALSE(m_testSrg->SetBufferArray(m_indexOfBufferArray, tooManyBuffers));
        AZ_TEST_STOP_ASSERTTEST(1);

        m_testSrg->Compile();
        EXPECT_EQ(nullptr, m_testSrg->GetBuffer(m_indexOfBufferArray, 0));
        EXPECT_EQ(nullptr, m_testSrg->GetBuffer(m_indexOfBufferArray, 1));
        EXPECT_EQ(nullptr, m_testSrg->GetBuffer(m_indexOfBufferArray, 2));
        EXPECT_EQ(nullptr, m_testSrg->GetBufferView(m_indexOfBufferArray, 0));
        EXPECT_EQ(nullptr, m_testSrg->GetBufferView(m_indexOfBufferArray, 1));
        EXPECT_EQ(nullptr, m_testSrg->GetBufferView(m_indexOfBufferArray, 2));
    }

    TEST_F(ShaderResourceGroupBufferTests, TestSetBufferArrayAtOffset)
    {
        AZStd::vector<Data::Instance<Buffer>> twoBuffers = { m_mediumBuffer, m_longBuffer };

        // Test set operation, skipping the first element...

        EXPECT_TRUE(m_testSrg->SetBufferArray(m_indexOfBufferArray, twoBuffers, 1));
        m_testSrg->Compile();

        EXPECT_EQ(3, m_testSrg->GetBufferArray(m_indexOfBufferArray).size());
        EXPECT_EQ(nullptr, m_testSrg->GetBufferArray(m_indexOfBufferArray)[0]);
        EXPECT_EQ(twoBuffers[0], m_testSrg->GetBufferArray(m_indexOfBufferArray)[1]);
        EXPECT_EQ(twoBuffers[1], m_testSrg->GetBufferArray(m_indexOfBufferArray)[2]);
        EXPECT_EQ(nullptr, m_testSrg->GetBuffer(m_indexOfBufferArray, 0));
        EXPECT_EQ(twoBuffers[0], m_testSrg->GetBuffer(m_indexOfBufferArray, 1));
        EXPECT_EQ(twoBuffers[1], m_testSrg->GetBuffer(m_indexOfBufferArray, 2));
        EXPECT_EQ(nullptr, m_testSrg->GetBufferView(m_indexOfBufferArray, 0));
        EXPECT_EQ(twoBuffers[0]->GetBufferView(), m_testSrg->GetBufferView(m_indexOfBufferArray, 1));
        EXPECT_EQ(twoBuffers[1]->GetBufferView(), m_testSrg->GetBufferView(m_indexOfBufferArray, 2));
    }

    TEST_F(ShaderResourceGroupBufferTests, TestSetBufferArrayAtOffset_ValidationFailure)
    {
        // Make sure the no changes are made when a validation failure is detected

        // 3 entries is too many because we will start at an offset of 1
        AZStd::vector<Data::Instance<Buffer>> tooManyBuffers{ 3, m_shortBuffer };

        AZ_TEST_START_ASSERTTEST;
        EXPECT_FALSE(m_testSrg->SetBufferArray(m_indexOfBufferArray, tooManyBuffers, 1));
        AZ_TEST_STOP_ASSERTTEST(1);

        m_testSrg->Compile();
        EXPECT_EQ(nullptr, m_testSrg->GetBuffer(m_indexOfBufferArray, 0));
        EXPECT_EQ(nullptr, m_testSrg->GetBuffer(m_indexOfBufferArray, 1));
        EXPECT_EQ(nullptr, m_testSrg->GetBuffer(m_indexOfBufferArray, 2));
        EXPECT_EQ(nullptr, m_testSrg->GetBufferView(m_indexOfBufferArray, 0));
        EXPECT_EQ(nullptr, m_testSrg->GetBufferView(m_indexOfBufferArray, 1));
        EXPECT_EQ(nullptr, m_testSrg->GetBufferView(m_indexOfBufferArray, 2));
    }

    TEST_F(ShaderResourceGroupBufferTests, TestSetGetBufferView)
    {
        // Set some RPI::Buffers first, just to make sure these get cleared when setting an RPI::BufferView

        EXPECT_TRUE(m_testSrg->SetBuffer(m_indexOfBufferA, m_mediumBuffer));
        EXPECT_TRUE(m_testSrg->SetBuffer(m_indexOfBufferB, m_mediumBuffer));

        // Test valid set/get operation...

        EXPECT_TRUE(m_testSrg->SetBufferView(m_indexOfBufferA, m_bufferViewA.get()));
        EXPECT_TRUE(m_testSrg->SetBufferView(m_indexOfBufferB, m_bufferViewB.get()));

        m_testSrg->Compile();

        EXPECT_EQ(m_bufferViewA, m_testSrg->GetBufferView(m_indexOfBufferA));
        EXPECT_EQ(m_bufferViewB, m_testSrg->GetBufferView(m_indexOfBufferB));
        EXPECT_EQ(m_bufferViewA, m_testSrg->GetBufferView(m_indexOfBufferA, 0));
        EXPECT_EQ(m_bufferViewB, m_testSrg->GetBufferView(m_indexOfBufferB, 0));

        // The RPI::Buffer should get cleared when you set a RHI::DeviceBufferView directly
        EXPECT_EQ(nullptr, m_testSrg->GetBuffer(m_indexOfBufferA));
        EXPECT_EQ(nullptr, m_testSrg->GetBuffer(m_indexOfBufferB));
    }

    TEST_F(ShaderResourceGroupBufferTests, TestSetGetBufferViewAtOffset)
    {
        // Set some RPI::Buffers first, just to make sure these get cleared when setting an RPI::BufferView

        EXPECT_TRUE(m_testSrg->SetBuffer(m_indexOfBufferArray, m_mediumBuffer, 0));
        EXPECT_TRUE(m_testSrg->SetBuffer(m_indexOfBufferArray, m_mediumBuffer, 1));
        EXPECT_TRUE(m_testSrg->SetBuffer(m_indexOfBufferArray, m_mediumBuffer, 2));

        // Test valid set/get operation...

        EXPECT_TRUE(m_testSrg->SetBufferView(m_indexOfBufferArray, m_bufferViewA.get(), 0));
        EXPECT_TRUE(m_testSrg->SetBufferView(m_indexOfBufferArray, m_bufferViewB.get(), 1));
        EXPECT_TRUE(m_testSrg->SetBufferView(m_indexOfBufferArray, m_bufferViewC.get(), 2));

        m_testSrg->Compile();

        EXPECT_EQ(m_bufferViewA, m_testSrg->GetBufferView(m_indexOfBufferArray, 0));
        EXPECT_EQ(m_bufferViewB, m_testSrg->GetBufferView(m_indexOfBufferArray, 1));
        EXPECT_EQ(m_bufferViewC, m_testSrg->GetBufferView(m_indexOfBufferArray, 2));
        EXPECT_EQ(m_bufferViewA, m_testSrg->GetBufferView(m_indexOfBufferArray, 0));
        EXPECT_EQ(m_bufferViewB, m_testSrg->GetBufferView(m_indexOfBufferArray, 1));
        EXPECT_EQ(m_bufferViewC, m_testSrg->GetBufferView(m_indexOfBufferArray, 2));

        // The RPI::Buffer should get cleared when you set a RHI::DeviceBufferView directly
        EXPECT_EQ(nullptr, m_testSrg->GetBuffer(m_indexOfBufferArray, 0));
        EXPECT_EQ(nullptr, m_testSrg->GetBuffer(m_indexOfBufferArray, 1));
        EXPECT_EQ(nullptr, m_testSrg->GetBuffer(m_indexOfBufferArray, 2));
    }

    TEST_F(ShaderResourceGroupBufferTests, TestSetGetBufferViewArray)
    {
        // Test basic set/get operation...

        EXPECT_TRUE(m_testSrg->SetBufferViewArray(m_indexOfBufferArray, m_threeBufferViews));
        m_testSrg->Compile();

        EXPECT_EQ(3, m_testSrg->GetBufferViewArray(m_indexOfBufferArray).size());
        EXPECT_EQ(m_threeBufferViews[0], m_testSrg->GetBufferViewArray(m_indexOfBufferArray)[0]);
        EXPECT_EQ(m_threeBufferViews[1], m_testSrg->GetBufferViewArray(m_indexOfBufferArray)[1]);
        EXPECT_EQ(m_threeBufferViews[2], m_testSrg->GetBufferViewArray(m_indexOfBufferArray)[2]);
        EXPECT_EQ(m_threeBufferViews[0], m_testSrg->GetBufferView(m_indexOfBufferArray, 0));
        EXPECT_EQ(m_threeBufferViews[1], m_testSrg->GetBufferView(m_indexOfBufferArray, 1));
        EXPECT_EQ(m_threeBufferViews[2], m_testSrg->GetBufferView(m_indexOfBufferArray, 2));
        EXPECT_EQ(m_threeBufferViews[0], m_testSrg->GetBufferView(m_indexOfBufferArray, 0));
        EXPECT_EQ(m_threeBufferViews[1], m_testSrg->GetBufferView(m_indexOfBufferArray, 1));
        EXPECT_EQ(m_threeBufferViews[2], m_testSrg->GetBufferView(m_indexOfBufferArray, 2));

        // Test replacing just two buffer views including changing one back to null...

        ProcessQueuedSrgCompilations(m_testSrgShaderAsset, m_testSrgLayout->GetName());

        AZStd::vector<const RHI::BufferView*> alternateBufferViews = { m_bufferViewB.get(), nullptr };

        EXPECT_TRUE(m_testSrg->SetBufferViewArray(m_indexOfBufferArray, alternateBufferViews));
        m_testSrg->Compile();

        EXPECT_EQ(m_bufferViewB, m_testSrg->GetBufferView(m_indexOfBufferArray, 0));
        EXPECT_EQ(nullptr, m_testSrg->GetBufferView(m_indexOfBufferArray, 1));
        EXPECT_EQ(m_bufferViewC, m_testSrg->GetBufferView(m_indexOfBufferArray, 2)); // remains unchanged
    }

    TEST_F(ShaderResourceGroupBufferTests, TestSetGetBufferViewArray_ValidationFailure)
    {
        // Make sure the no changes are made when a validation failure is detected

        AZStd::vector<const RHI::BufferView*> tooManyBufferViews{ 4, m_bufferViewA.get() };

        AZ_TEST_START_ASSERTTEST;
        EXPECT_FALSE(m_testSrg->SetBufferViewArray(m_indexOfBufferArray, tooManyBufferViews));
        AZ_TEST_STOP_ASSERTTEST(1);

        m_testSrg->Compile();
        EXPECT_EQ(nullptr, m_testSrg->GetBufferView(m_indexOfBufferArray, 0));
        EXPECT_EQ(nullptr, m_testSrg->GetBufferView(m_indexOfBufferArray, 1));
        EXPECT_EQ(nullptr, m_testSrg->GetBufferView(m_indexOfBufferArray, 2));
    }

    TEST_F(ShaderResourceGroupBufferTests, TestSetBufferViewArrayAtOffset)
    {
        AZStd::vector<const RHI::BufferView*> twoBufferViews = { m_bufferViewA.get(), m_bufferViewB.get() };

        // Test set operation, skipping the first element...

        EXPECT_TRUE(m_testSrg->SetBufferViewArray(m_indexOfBufferArray, twoBufferViews, 1));
        m_testSrg->Compile();

        EXPECT_EQ(3, m_testSrg->GetBufferViewArray(m_indexOfBufferArray).size());
        EXPECT_EQ(nullptr, m_testSrg->GetBufferViewArray(m_indexOfBufferArray)[0]);
        EXPECT_EQ(twoBufferViews[0], m_testSrg->GetBufferViewArray(m_indexOfBufferArray)[1]);
        EXPECT_EQ(twoBufferViews[1], m_testSrg->GetBufferViewArray(m_indexOfBufferArray)[2]);
        EXPECT_EQ(nullptr, m_testSrg->GetBufferView(m_indexOfBufferArray, 0));
        EXPECT_EQ(twoBufferViews[0], m_testSrg->GetBufferView(m_indexOfBufferArray, 1));
        EXPECT_EQ(twoBufferViews[1], m_testSrg->GetBufferView(m_indexOfBufferArray, 2));
        EXPECT_EQ(nullptr, m_testSrg->GetBufferView(m_indexOfBufferArray, 0));
        EXPECT_EQ(twoBufferViews[0], m_testSrg->GetBufferView(m_indexOfBufferArray, 1));
        EXPECT_EQ(twoBufferViews[1], m_testSrg->GetBufferView(m_indexOfBufferArray, 2));
    }

    TEST_F(ShaderResourceGroupBufferTests, TestSetBufferViewArrayAtOffset_ValidationFailure)
    {
        // Make sure the no changes are made when a validation failure is detected

        AZStd::vector<const RHI::BufferView*> tooManyBufferViews{ 3, m_bufferViewA.get() };

        AZ_TEST_START_ASSERTTEST;
        EXPECT_FALSE(m_testSrg->SetBufferViewArray(m_indexOfBufferArray, tooManyBufferViews, 1));
        AZ_TEST_STOP_ASSERTTEST(1);

        m_testSrg->Compile();
        EXPECT_EQ(nullptr, m_testSrg->GetBufferView(m_indexOfBufferArray, 0));
        EXPECT_EQ(nullptr, m_testSrg->GetBufferView(m_indexOfBufferArray, 1));
        EXPECT_EQ(nullptr, m_testSrg->GetBufferView(m_indexOfBufferArray, 2));
    }

    TEST_F(ShaderResourceGroupBufferTests, TestCopyShaderResourceGroupDataBuffer)
    {
        EXPECT_TRUE(m_testSrg->SetBufferArray(m_indexOfBufferArray, m_threeBuffers));
        auto testSrg2 =
            ShaderResourceGroup::Create(m_testSrgShaderAsset, AZ::RPI::DefaultSupervariantIndex, m_testSrgLayout->GetName());


        EXPECT_TRUE(testSrg2->CopyShaderResourceGroupData(*m_testSrg));
        EXPECT_EQ(3, testSrg2->GetBufferArray(m_indexOfBufferArray).size());
        EXPECT_EQ(m_testSrg->GetBufferArray(m_indexOfBufferArray)[0], testSrg2->GetBufferArray(m_indexOfBufferArray)[0]);
        EXPECT_EQ(m_testSrg->GetBufferArray(m_indexOfBufferArray)[1], testSrg2->GetBufferArray(m_indexOfBufferArray)[1]);
        EXPECT_EQ(m_testSrg->GetBufferArray(m_indexOfBufferArray)[2], testSrg2->GetBufferArray(m_indexOfBufferArray)[2]);
        EXPECT_EQ(m_testSrg->GetBufferViewArray(m_indexOfBufferArray)[0], testSrg2->GetBufferViewArray(m_indexOfBufferArray)[0]);
        EXPECT_EQ(m_testSrg->GetBufferViewArray(m_indexOfBufferArray)[1], testSrg2->GetBufferViewArray(m_indexOfBufferArray)[1]);
        EXPECT_EQ(m_testSrg->GetBufferViewArray(m_indexOfBufferArray)[2], testSrg2->GetBufferViewArray(m_indexOfBufferArray)[2]);
    }

    TEST_F(ShaderResourceGroupBufferTests, TestCopyShaderResourceGroupDataBufferView)
    {
        EXPECT_TRUE(m_testSrg->SetBufferViewArray(m_indexOfBufferArray, m_threeBufferViews));
        auto testSrg2 = ShaderResourceGroup::Create(m_testSrgShaderAsset, AZ::RPI::DefaultSupervariantIndex, m_testSrgLayout->GetName());

        EXPECT_TRUE(testSrg2->CopyShaderResourceGroupData(*m_testSrg));
        EXPECT_EQ(3, testSrg2->GetBufferViewArray(m_indexOfBufferArray).size());
        EXPECT_EQ(m_testSrg->GetBufferArray(m_indexOfBufferArray)[0], testSrg2->GetBufferArray(m_indexOfBufferArray)[0]);
        EXPECT_EQ(m_testSrg->GetBufferArray(m_indexOfBufferArray)[1], testSrg2->GetBufferArray(m_indexOfBufferArray)[1]);
        EXPECT_EQ(m_testSrg->GetBufferArray(m_indexOfBufferArray)[2], testSrg2->GetBufferArray(m_indexOfBufferArray)[2]);
        EXPECT_EQ(m_testSrg->GetBufferViewArray(m_indexOfBufferArray)[0], testSrg2->GetBufferViewArray(m_indexOfBufferArray)[0]);
        EXPECT_EQ(m_testSrg->GetBufferViewArray(m_indexOfBufferArray)[1], testSrg2->GetBufferViewArray(m_indexOfBufferArray)[1]);
        EXPECT_EQ(m_testSrg->GetBufferViewArray(m_indexOfBufferArray)[2], testSrg2->GetBufferViewArray(m_indexOfBufferArray)[2]);
    }

    TEST_F(ShaderResourceGroupBufferTests, TestPartilCopyShaderResourceGroupData)
    {
        RHI::Ptr<RHI::ShaderResourceGroupLayout> srgLayout2 = RHI::ShaderResourceGroupLayout::Create();
        srgLayout2->SetName(Name("partial"));
        srgLayout2->SetBindingSlot(0);
        srgLayout2->AddShaderInput(RHI::ShaderInputBufferDescriptor{
            Name{ "MyBufferB" }, RHI::ShaderInputBufferAccess::Read, RHI::ShaderInputBufferType::Raw, 1, 4, 2, 2 });
        srgLayout2->AddShaderInput(RHI::ShaderInputBufferDescriptor{
            Name{ "MyBufferC" }, RHI::ShaderInputBufferAccess::Read, RHI::ShaderInputBufferType::Raw, 1, 4, 2, 2 });
        srgLayout2->Finalize();

        auto testSrgShaderAsset2 = CreateTestShaderAsset(Uuid::CreateRandom(), srgLayout2);
        auto testSrg2 = ShaderResourceGroup::Create(testSrgShaderAsset2, AZ::RPI::DefaultSupervariantIndex, srgLayout2->GetName());

        EXPECT_TRUE(m_testSrg->SetBuffer(m_indexOfBufferA, m_shortBuffer));
        EXPECT_TRUE(m_testSrg->SetBuffer(m_indexOfBufferB, m_mediumBuffer));

        EXPECT_FALSE(testSrg2->CopyShaderResourceGroupData(*m_testSrg));
        EXPECT_EQ(m_testSrg->GetBuffer(m_indexOfBufferB), testSrg2->GetBuffer(RHI::ShaderInputBufferIndex{ 0 }));
        EXPECT_EQ(m_testSrg->GetBufferView(m_indexOfBufferB), testSrg2->GetBufferView(RHI::ShaderInputBufferIndex{ 0 }));
    }
}

