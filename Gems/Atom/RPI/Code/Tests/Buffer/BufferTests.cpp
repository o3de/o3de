/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Reflect/Buffer/BufferAssetCreator.h>
#include <Atom/RPI.Reflect/ResourcePoolAssetCreator.h>

#include <AzTest/AzTest.h>

#include <Common/RHI/Factory.h>
#include <Common/RPITestFixture.h>
#include <Common/SerializeTester.h>
#include <Common/ErrorMessageFinder.h>

#include <Atom/RPI.Public/Buffer/Buffer.h>
#include <Atom/RPI.Public/Buffer/BufferSystemInterface.h>

namespace UnitTest
{
    class BufferTests
        : public RPITestFixture
    {
    protected:

        struct ExpectedBuffer
        {
            AZStd::vector<uint8_t> m_data;
            AZ::RHI::BufferDescriptor m_bufferDescriptor;
            AZ::RHI::BufferViewDescriptor m_viewDescriptor;
        };

        AZ::Data::Asset<AZ::RPI::ResourcePoolAsset> CreateTestBufferPoolAsset()
        {
            using namespace AZ;
            using namespace RPI;

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

        ExpectedBuffer CreateValidBuffer()
        {
            using namespace AZ;

            // Build a simple buffer with 100 elements where
            // each element is 3 floats in size
            const uint32_t elementSize = sizeof(float) * 3;
            const uint32_t elementCount = 100;

            const size_t bufferSize = elementSize * elementCount;
            AZStd::vector<uint8_t> bufferData;
            bufferData.resize(bufferSize);

            // The actual data doesn't matter
            for (uint32_t i = 0; i < bufferData.size(); ++i)
            {
                bufferData[i] = i;
            }

            RHI::BufferDescriptor bufferDescriptor;
            bufferDescriptor.m_bindFlags = RHI::BufferBindFlags::ShaderRead;
            bufferDescriptor.m_byteCount = bufferSize;

            RHI::BufferViewDescriptor viewDescriptor = RHI::BufferViewDescriptor::CreateStructured(0, elementCount, elementSize);

            ExpectedBuffer validBuffer;
            validBuffer.m_data = bufferData;
            validBuffer.m_bufferDescriptor = bufferDescriptor;
            validBuffer.m_viewDescriptor = viewDescriptor;

            return validBuffer;
        }

        AZ::Data::Asset<AZ::RPI::BufferAsset> BuildTestBuffer(ExpectedBuffer& expectedBuffer)
        {
            using namespace AZ;

            // Copy valid buffer to output buffer
            expectedBuffer = CreateValidBuffer();
            const size_t bufferSize = expectedBuffer.m_bufferDescriptor.m_byteCount;

            RPI::BufferAssetCreator creator;

            creator.Begin(Data::AssetId(AZ::Uuid::CreateRandom()));
            creator.SetBuffer(expectedBuffer.m_data.data(), bufferSize, expectedBuffer.m_bufferDescriptor);
            creator.SetBufferViewDescriptor(expectedBuffer.m_viewDescriptor);
            creator.SetPoolAsset(CreateTestBufferPoolAsset());

            Data::Asset<RPI::BufferAsset> asset;
            EXPECT_TRUE(creator.End(asset));
            EXPECT_TRUE(asset.IsReady());
            EXPECT_NE(asset.Get(), nullptr);

            return asset;
        }

        void ValidateBufferAsset(AZ::RPI::BufferAsset* bufferAsset, const ExpectedBuffer& expectedBuffer)
        {
            using namespace AZ;

            EXPECT_NE(bufferAsset, nullptr);

            EXPECT_TRUE(bufferAsset->GetBuffer().size() == expectedBuffer.m_data.size());

            const bool sameBuffer = memcmp(&bufferAsset->GetBuffer()[0], expectedBuffer.m_data.data(), bufferAsset->GetBuffer().size()) == 0;
            EXPECT_TRUE(sameBuffer);

            const RHI::BufferDescriptor& assetBufferDescriptor = bufferAsset->GetBufferDescriptor();
            const RHI::BufferDescriptor& expectedBufferDescriptor = expectedBuffer.m_bufferDescriptor;

            EXPECT_TRUE(assetBufferDescriptor.GetHash() == expectedBufferDescriptor.GetHash());
            EXPECT_TRUE(assetBufferDescriptor.m_bindFlags == expectedBufferDescriptor.m_bindFlags);
            EXPECT_TRUE(assetBufferDescriptor.m_byteCount == expectedBufferDescriptor.m_byteCount);
            EXPECT_TRUE(assetBufferDescriptor.m_sharedQueueMask == expectedBufferDescriptor.m_sharedQueueMask);

            const RHI::BufferViewDescriptor& assetViewDescriptor = bufferAsset->GetBufferViewDescriptor();
            const RHI::BufferViewDescriptor& expectedViewDescriptor = expectedBuffer.m_viewDescriptor;

            EXPECT_TRUE(assetViewDescriptor.GetHash() == expectedViewDescriptor.GetHash());
            EXPECT_TRUE(assetViewDescriptor.m_elementCount == expectedViewDescriptor.m_elementCount);
            EXPECT_TRUE(assetViewDescriptor.m_elementFormat == expectedViewDescriptor.m_elementFormat);
            EXPECT_TRUE(assetViewDescriptor.m_elementOffset == expectedViewDescriptor.m_elementOffset);
            EXPECT_TRUE(assetViewDescriptor.m_elementSize == expectedViewDescriptor.m_elementSize);
        }

        AZStd::unique_ptr<AZ::RPI::BufferAssetHandler> m_bufferAssetHandler;
    };

    // Test that a buffer can be created and that the contents match 
    // what we expect.
    TEST_F(BufferTests, Creation)
    {
        using namespace AZ;

        ExpectedBuffer expectedBuffer;

        Data::Asset<RPI::BufferAsset> bufferAsset = BuildTestBuffer(expectedBuffer);
        ValidateBufferAsset(bufferAsset.Get(), expectedBuffer);
    }

    // Test that a buffer can be created, serialized and de-serialized 
    // without losing any data.
    TEST_F(BufferTests, SerializeDeserialize)
    {
        using namespace AZ;

        ExpectedBuffer expectedBuffer;

        Data::Asset<RPI::BufferAsset> bufferAsset = BuildTestBuffer(expectedBuffer);

        SerializeTester<RPI::BufferAsset> tester(GetSerializeContext());
        tester.SerializeOut(bufferAsset.Get());

        Data::Asset<RPI::BufferAsset> serializedBufferAsset = tester.SerializeIn(Data::AssetId(Uuid::CreateRandom()));
        ValidateBufferAsset(serializedBufferAsset.Get(), expectedBuffer);
    }

    // Test that if we try to set data on the BufferAssetCreator
    // before Begin has been called that it fails
    TEST_F(BufferTests, SetBufferNoBegin)
    {
        using namespace AZ;

        ExpectedBuffer validBuffer = CreateValidBuffer();
        const size_t bufferSize = validBuffer.m_bufferDescriptor.m_byteCount;

        RPI::BufferAssetCreator creator;

        ErrorMessageFinder messageFinder("Begin() was not called", 2);

        creator.SetBuffer(validBuffer.m_data.data(), bufferSize, validBuffer.m_bufferDescriptor);

        //Ending the buffer here should also result in failure
        Data::Asset<RPI::BufferAsset> asset;
        ASSERT_FALSE(creator.End(asset));
        ASSERT_FALSE(asset.IsReady());
        ASSERT_EQ(asset.Get(), nullptr);
    }

    // Test that if we try to set empty data on the BufferAssetCreator
    // that it works as intended
    TEST_F(BufferTests, SetEmptyBuffer)
    {
        using namespace AZ;

        //Use a valid buffer for the buffer descriptor
        ExpectedBuffer validBuffer = CreateValidBuffer();

        RPI::BufferAssetCreator creator;

        creator.Begin(Data::AssetId(AZ::Uuid::CreateRandom()));

        // Setting nullptr should succeed if the buffer descriptor is valid
        // as the BufferAsset should be usable for R/W buffers
        creator.SetBuffer(nullptr, 0, validBuffer.m_bufferDescriptor);

        ASSERT_EQ(0, creator.GetErrorCount());
        ASSERT_EQ(0, creator.GetWarningCount());
    }

    // Test that if we try to set an invalid BufferDescriptor
    // on the BufferAssetCreator that it fails
    TEST_F(BufferTests, SetInvalidBufferDescriptor)
    {
        using namespace AZ;

        //Use a valid buffer for the buffer data
        ExpectedBuffer validBuffer = CreateValidBuffer();
        const size_t bufferSize = validBuffer.m_bufferDescriptor.m_byteCount;

        RHI::BufferDescriptor invalidBufferDescriptor;
        invalidBufferDescriptor.m_byteCount = 0;

        RPI::BufferAssetCreator creator;

        creator.Begin(Data::AssetId(AZ::Uuid::CreateRandom()));
        creator.SetPoolAsset(CreateTestBufferPoolAsset());

        ErrorMessageFinder messageFinder("Size of the buffer in the descriptor was 0");
        messageFinder.AddIgnoredErrorMessage("Cannot continue building BufferAsset", true);
        
        creator.SetBuffer(validBuffer.m_data.data(), bufferSize, invalidBufferDescriptor);
        

        // Ending the buffer here should also result in failure
        Data::Asset<RPI::BufferAsset> asset;
        ASSERT_FALSE(creator.End(asset));
        ASSERT_FALSE(asset.IsReady());
        ASSERT_EQ(asset.Get(), nullptr);
    }

    // Test that if we try to set initial buffer data that's more than 
    // we describe in the buffer descriptor that the creator fails as expected.
    TEST_F(BufferTests, SetBufferTooMuchInitialData)
    {
        using namespace AZ;

        //Use a valid buffer for the buffer data
        ExpectedBuffer validBuffer = CreateValidBuffer();

        const size_t invalidInitialDataSize = validBuffer.m_bufferDescriptor.m_byteCount + 1;

        RPI::BufferAssetCreator creator;

        creator.Begin(Data::AssetId(AZ::Uuid::CreateRandom()));
        creator.SetPoolAsset(CreateTestBufferPoolAsset());

        ErrorMessageFinder messageFinder("initialSize is larger than the total size in the descriptor.");
        messageFinder.AddIgnoredErrorMessage("Cannot continue building BufferAsset", true);
        
        creator.SetBuffer(validBuffer.m_data.data(), invalidInitialDataSize, validBuffer.m_bufferDescriptor);

        // Ending the buffer here should also result in failure
        Data::Asset<RPI::BufferAsset> asset;
        ASSERT_FALSE(creator.End(asset));
        ASSERT_FALSE(asset.IsReady());
        ASSERT_EQ(asset.Get(), nullptr);
    }

    // Test that if we try to set an empty buffer with a non-zero
    // initial size that the creator fails as expected.
    TEST_F(BufferTests, SetEmptyBufferWithNonZeroSize)
    {
        using namespace AZ;

        //Use a valid buffer for the bufferDescriptor
        ExpectedBuffer validBuffer = CreateValidBuffer();

        RPI::BufferAssetCreator creator;

        creator.Begin(Data::AssetId(AZ::Uuid::CreateRandom()));
        creator.SetPoolAsset(CreateTestBufferPoolAsset());

        ErrorMessageFinder messageFinder("Initial buffer data was not provided but the initial size was non-zero.");
        messageFinder.AddIgnoredErrorMessage("Cannot continue building BufferAsset", true);
        
        creator.SetBuffer(nullptr, 1, validBuffer.m_bufferDescriptor);

        // Ending the buffer here should also result in failure
        Data::Asset<RPI::BufferAsset> asset;
        ASSERT_FALSE(creator.End(asset));
        ASSERT_FALSE(asset.IsReady());
        ASSERT_EQ(asset.Get(), nullptr);
    }

    // Test that if we try to set a buffer with an initial size of 0
    // that the creator fails as expected.
    TEST_F(BufferTests, SetBufferWithZeroSize)
    {
        using namespace AZ;

        //Use a valid buffer for the bufferDescriptor
        ExpectedBuffer validBuffer = CreateValidBuffer();

        RPI::BufferAssetCreator creator;

        creator.Begin(Data::AssetId(AZ::Uuid::CreateRandom()));
        creator.SetPoolAsset(CreateTestBufferPoolAsset());

        ErrorMessageFinder messageFinder("Initial buffer data was not null but the initial size was zero.");
        messageFinder.AddIgnoredErrorMessage("Cannot continue building BufferAsset", true);
        
        creator.SetBuffer(validBuffer.m_data.data(), 0, validBuffer.m_bufferDescriptor);

        // Ending the buffer here should also result in failure
        Data::Asset<RPI::BufferAsset> asset;
        ASSERT_FALSE(creator.End(asset));
        ASSERT_FALSE(asset.IsReady());
        ASSERT_EQ(asset.Get(), nullptr);
    }

    // Tests that if we try to set the view descriptor on the 
    // BufferAssetCreator before Begin has been called that it fails
    TEST_F(BufferTests, SetViewDescriptorNoBegin)
    {
        using namespace AZ;

        ExpectedBuffer validBuffer = CreateValidBuffer();

        RPI::BufferAssetCreator creator;

        ErrorMessageFinder messageFinder("Begin() was not called", 2);
        
        creator.SetBufferViewDescriptor(validBuffer.m_viewDescriptor);

        //Ending the buffer here should also result in failure
        Data::Asset<RPI::BufferAsset> asset;
        ASSERT_FALSE(creator.End(asset));
        ASSERT_FALSE(asset.IsReady());
        ASSERT_EQ(asset.Get(), nullptr);
    }

    TEST_F(BufferTests, SetInvalidViewDescriptor)
    {
        using namespace AZ;

        ExpectedBuffer validBuffer = CreateValidBuffer();

        RHI::BufferViewDescriptor invalidViewDescriptor;

        RPI::BufferAssetCreator creator;

        creator.Begin(Data::AssetId(AZ::Uuid::CreateRandom()));
        creator.SetPoolAsset(CreateTestBufferPoolAsset());

        ErrorMessageFinder messageFinder("BufferAssetCreator::SetBufferViewDescriptor was given a view descriptor with an element count of 0.");
        messageFinder.AddIgnoredErrorMessage("Cannot continue building BufferAsset", true);
        
        creator.SetBufferViewDescriptor(invalidViewDescriptor);

        //Ending the buffer here should also result in failure
        Data::Asset<RPI::BufferAsset> asset;
        ASSERT_FALSE(creator.End(asset));
        ASSERT_FALSE(asset.IsReady());
        ASSERT_EQ(asset.Get(), nullptr);
    }

    // Buffer Asset tests
    // Buffer asset without initial data
    TEST_F(BufferTests, BufferAssetCreation_NoInitialData_Success)
    {
        using namespace AZ;

        ExpectedBuffer bufferInfo = CreateValidBuffer();

        RPI::BufferAssetCreator creator;
        creator.Begin(Data::AssetId(AZ::Uuid::CreateRandom()));
        creator.SetPoolAsset(CreateTestBufferPoolAsset());
        creator.SetBufferViewDescriptor(bufferInfo.m_viewDescriptor);

        // empty initial data
        creator.SetBuffer(nullptr, 0, bufferInfo.m_bufferDescriptor);

        Data::Asset<RPI::BufferAsset> asset;

        EXPECT_TRUE(creator.End(asset));
        EXPECT_TRUE(asset.IsReady());
        EXPECT_NE(asset.Get(), nullptr);
    }

    // Neither pool asset specified nor common pool type specified
    TEST_F(BufferTests, BufferAssetCreation_NoPoolSpecified_Fail)
    {
        using namespace AZ;

        ExpectedBuffer bufferInfo = CreateValidBuffer();

        RPI::BufferAssetCreator creator;
        creator.Begin(Data::AssetId(AZ::Uuid::CreateRandom()));
        creator.SetBufferViewDescriptor(bufferInfo.m_viewDescriptor);
        creator.SetBuffer(nullptr, 0, bufferInfo.m_bufferDescriptor);

        Data::Asset<RPI::BufferAsset> asset;
        
        // Failed
        ErrorMessageFinder messageFinder("BufferAssetCreator::ValidateBuffer Failed; need valid pool asset or select a valid common pool.", 1);
        ASSERT_FALSE(creator.End(asset));
        EXPECT_FALSE(asset.IsReady());
        EXPECT_EQ(asset.Get(), nullptr);
    }

    // Buffer system interface unit tests
    // Get buffer pools by type
    TEST_F(BufferTests, BufferSystem_GetCommonPools_Success)
    {
        using namespace AZ;

        for (uint32_t type = 0; type < static_cast<uint32_t>(RPI::CommonBufferPoolType::Count); type++)
        {
            ASSERT_NE(RPI::BufferSystemInterface::Get()->GetCommonBufferPool((RPI::CommonBufferPoolType)type).get(), nullptr);
        }
    }

    TEST_F(BufferTests, BufferSystem_CreateCommonBuffer_Success)
    {
        using namespace AZ;
        ExpectedBuffer bufferInfo = CreateValidBuffer();
        
        RPI::CommonBufferDescriptor desc;
        desc.m_poolType = RPI::CommonBufferPoolType::ReadOnly;
        desc.m_bufferName = "Buffer1";
        desc.m_byteCount = bufferInfo.m_bufferDescriptor.m_byteCount;
        desc.m_elementSize = bufferInfo.m_viewDescriptor.m_elementSize;
        desc.m_bufferData = bufferInfo.m_data.data();

        // with data
        Data::Instance<RPI::Buffer> bufferInst = RPI::BufferSystemInterface::Get()->CreateBufferFromCommonPool(desc);

        // buffer created
        ASSERT_NE(bufferInst.get(), nullptr);
        // buffer has same size as creation size
        EXPECT_EQ(bufferInst->GetBufferSize(), bufferInfo.m_bufferDescriptor.m_byteCount);
        // buffer has same buffer view descriptor as expected
        EXPECT_EQ(bufferInst->GetBufferViewDescriptor().m_elementCount, bufferInfo.m_viewDescriptor.m_elementCount);
        EXPECT_EQ(bufferInst->GetBufferViewDescriptor().m_elementSize, bufferInfo.m_viewDescriptor.m_elementSize);

        // without initial data
        RPI::CommonBufferDescriptor desc2;
        desc.m_poolType = RPI::CommonBufferPoolType::ReadOnly;
        desc.m_bufferName = "Buffer2";
        desc.m_byteCount = bufferInfo.m_bufferDescriptor.m_byteCount;

        Data::Instance<RPI::Buffer> bufferInst2 = RPI::BufferSystemInterface::Get()->CreateBufferFromCommonPool(desc);
        // buffer created
        EXPECT_NE(bufferInst2.get(), nullptr);
    }

    TEST_F(BufferTests, BufferSystem_FindCommonBuffer_Success_Fail)
    {
        using namespace AZ;

        ExpectedBuffer bufferInfo = CreateValidBuffer();

        RPI::CommonBufferDescriptor desc;
        desc.m_poolType = RPI::CommonBufferPoolType::ReadOnly;
        desc.m_bufferName = "Buffer1";
        desc.m_byteCount = bufferInfo.m_bufferDescriptor.m_byteCount;
        desc.m_isUniqueName = true;

        Data::Instance<RPI::Buffer> bufferInst = RPI::BufferSystemInterface::Get()->CreateBufferFromCommonPool(desc);
        // buffer created
        ASSERT_NE(bufferInst.get(), nullptr);

        // found
        Data::Instance<RPI::Buffer> bufferFound = RPI::BufferSystemInterface::Get()->FindCommonBuffer("Buffer1");
        EXPECT_EQ(bufferInst, bufferFound);

        // not found
        Data::Instance<RPI::Buffer> bufferFound2 = RPI::BufferSystemInterface::Get()->FindCommonBuffer("Buffer2");
        EXPECT_EQ(bufferFound2.get(), nullptr);
    }

    // Failed if creates a buffe which has a same name with existing buffer
    // and has m_isUniqueName is enabled
    TEST_F(BufferTests, BufferSystem_CreateDuplicatedNamedBufferEnableUniqueName_Fail)
    {
        using namespace AZ;

        ExpectedBuffer bufferInfo = CreateValidBuffer();

        RPI::CommonBufferDescriptor desc;
        desc.m_poolType = RPI::CommonBufferPoolType::ReadOnly;
        desc.m_bufferName = "Buffer1";
        desc.m_byteCount = bufferInfo.m_bufferDescriptor.m_byteCount;
        desc.m_isUniqueName = true;

        Data::Instance<RPI::Buffer> bufferInst = RPI::BufferSystemInterface::Get()->CreateBufferFromCommonPool(desc);
        // buffer created
        EXPECT_NE(bufferInst.get(), nullptr);

        AZ_TEST_START_ASSERTTEST;
        Data::Instance<RPI::Buffer> bufferInst2 = RPI::BufferSystemInterface::Get()->CreateBufferFromCommonPool(desc);
        AZ_TEST_STOP_ASSERTTEST(1);
        // buffer NOT created
        EXPECT_EQ(bufferInst2.get(), nullptr);
    }

    // create a buffer which has a same name with existing buffer 
    TEST_F(BufferTests, BufferSystem_CreateDuplicatedNamedBuffers_Success)
    {
        using namespace AZ;

        ExpectedBuffer bufferInfo = CreateValidBuffer();

        RPI::CommonBufferDescriptor desc;
        desc.m_poolType = RPI::CommonBufferPoolType::ReadOnly;
        desc.m_bufferName = "Buffer1";
        desc.m_byteCount = bufferInfo.m_bufferDescriptor.m_byteCount;

        Data::Instance<RPI::Buffer> bufferInst = RPI::BufferSystemInterface::Get()->CreateBufferFromCommonPool(desc);
        // buffer created
        EXPECT_NE(bufferInst.get(), nullptr);
                
        Data::Instance<RPI::Buffer> bufferInst2 = RPI::BufferSystemInterface::Get()->CreateBufferFromCommonPool(desc);
        // buffer created
        EXPECT_NE(bufferInst2.get(), nullptr);
    }

    // Buffer instance creation unit tests
    TEST_F(BufferTests, Buffer_CreationUsingPoolAsset_Success)
    {
        using namespace AZ;
        
        ExpectedBuffer expectedBuffer;

        Data::Asset<RPI::BufferAsset> bufferAsset = BuildTestBuffer(expectedBuffer);

        Data::Instance<RPI::Buffer> bufferInst = RPI::Buffer::FindOrCreate(bufferAsset);

        ASSERT_NE(bufferInst.get(), nullptr);
    }
    
    TEST_F(BufferTests, Buffer_CreationUsingCommonPool_Success)
    {
        using namespace AZ;

        ExpectedBuffer bufferInfo = CreateValidBuffer();

        RPI::BufferAssetCreator creator;
        creator.Begin(Data::AssetId(AZ::Uuid::CreateRandom()));
        creator.SetBufferViewDescriptor(bufferInfo.m_viewDescriptor);
        creator.SetBuffer(nullptr, 0, bufferInfo.m_bufferDescriptor);
        creator.SetUseCommonPool(RPI::CommonBufferPoolType::ReadOnly);

        Data::Asset<RPI::BufferAsset> asset;
        creator.End(asset);

        Data::Instance<RPI::Buffer> bufferInst = RPI::Buffer::FindOrCreate(asset);

        ASSERT_NE(bufferInst.get(), nullptr);
    }

    // Resize buffer function: validate new size and buffer view descriptor
    TEST_F(BufferTests, Buffer_Resize_Success)
    {
        using namespace AZ;
        ExpectedBuffer bufferInfo = CreateValidBuffer();

        RPI::BufferAssetCreator creator;
        creator.Begin(Data::AssetId(AZ::Uuid::CreateRandom()));
        creator.SetBufferViewDescriptor(bufferInfo.m_viewDescriptor);
        creator.SetBuffer(nullptr, 0, bufferInfo.m_bufferDescriptor);
        creator.SetUseCommonPool(RPI::CommonBufferPoolType::ReadOnly);

        Data::Asset<RPI::BufferAsset> asset;
        creator.End(asset);

        uint64_t initialSize = bufferInfo.m_bufferDescriptor.m_byteCount;
        Data::Instance<RPI::Buffer> bufferInst = RPI::Buffer::FindOrCreate(asset);
        ASSERT_NE(bufferInst.get(), nullptr);
        EXPECT_EQ(bufferInst->GetBufferSize(), initialSize);

        // size up
        uint64_t newSize = 2 * initialSize;
        bufferInst->Resize(newSize);
        ASSERT_NE(bufferInst.get(), nullptr);
        EXPECT_EQ(bufferInst->GetBufferSize(), newSize);
        EXPECT_EQ(bufferInst->GetBufferViewDescriptor().m_elementCount*bufferInst->GetBufferViewDescriptor().m_elementSize, newSize);

        // size down
        newSize = initialSize/2;
        bufferInst->Resize(newSize);
        ASSERT_NE(bufferInst.get(), nullptr);
        EXPECT_EQ(bufferInst->GetBufferSize(), newSize);
        EXPECT_EQ(bufferInst->GetBufferViewDescriptor().m_elementCount*bufferInst->GetBufferViewDescriptor().m_elementSize, newSize);
    }

    TEST_F(BufferTests, Buffer_SetAsStructured_Success)
    {
        using namespace AZ;
        
        ExpectedBuffer bufferInfo = CreateValidBuffer();

        RPI::BufferAssetCreator creator;
        creator.Begin(Data::AssetId(AZ::Uuid::CreateRandom()));
        creator.SetBufferViewDescriptor(bufferInfo.m_viewDescriptor);
        creator.SetBuffer(nullptr, 0, bufferInfo.m_bufferDescriptor);
        creator.SetUseCommonPool(RPI::CommonBufferPoolType::ReadOnly);

        Data::Asset<RPI::BufferAsset> asset;
        creator.End(asset);

        bufferInfo.m_bufferDescriptor.m_byteCount;
        Data::Instance<RPI::Buffer> bufferInst = RPI::Buffer::FindOrCreate(asset);
        ASSERT_NE(bufferInst.get(), nullptr);

        bufferInst->SetAsStructured<uint16_t>();
        EXPECT_EQ(bufferInst->GetBufferViewDescriptor().m_elementSize, 2);
        EXPECT_EQ(bufferInst->GetBufferView()->GetDescriptor().m_elementCount, bufferInst->GetBufferViewDescriptor().m_elementCount);
        EXPECT_EQ(bufferInst->GetBufferView()->GetDescriptor().m_elementSize, bufferInst->GetBufferViewDescriptor().m_elementSize);

        bufferInst->SetAsStructured<uint64_t>();
        EXPECT_EQ(bufferInst->GetBufferViewDescriptor().m_elementSize, 8);
        EXPECT_EQ(bufferInst->GetBufferView()->GetDescriptor().m_elementCount, bufferInst->GetBufferViewDescriptor().m_elementCount);
        EXPECT_EQ(bufferInst->GetBufferView()->GetDescriptor().m_elementSize, bufferInst->GetBufferViewDescriptor().m_elementSize);
    }
    
} // namespace UnitTest
