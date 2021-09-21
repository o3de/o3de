/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>

#include <Atom/RPI.Reflect/Image/ImageMipChainAsset.h>
#include <Atom/RPI.Reflect/Image/ImageMipChainAssetCreator.h>
#include <Atom/RPI.Reflect/Image/StreamingImageAsset.h>
#include <Atom/RPI.Reflect/Image/StreamingImageAssetHandler.h>
#include <Atom/RPI.Reflect/Image/StreamingImageAssetCreator.h>
#include <Atom/RPI.Reflect/Image/StreamingImagePoolAsset.h>
#include <Atom/RPI.Reflect/Image/StreamingImagePoolAssetCreator.h>
#include <Atom/RPI.Reflect/Image/DefaultStreamingImageControllerAsset.h>
#include <Atom/RPI.Reflect/Asset/BuiltInAssetHandler.h>

#include <Atom/RPI.Public/Image/ImageSystemInterface.h>
#include <Atom/RPI.Public/Image/StreamingImage.h>
#include <Atom/RPI.Public/Image/StreamingImagePool.h>
#include <Atom/RPI.Public/Image/DefaultStreamingImageController.h>

#include <AtomCore/Instance/InstanceDatabase.h>

#include <AzCore/Interface/Interface.h>
#include <AzCore/std/containers/intrusive_list.h>

#include <Common/RPITestFixture.h>
#include <Common/ErrorMessageFinder.h>
#include <Common/SerializeTester.h>

namespace AZ
{
    namespace RPI
    {
        class StreamingImageAssetTester
            : public UnitTest::AssetTester<StreamingImageAsset>
        {
        public:
            StreamingImageAssetTester() = default;
            ~StreamingImageAssetTester() override = default;

            void SetAssetReady(Data::Asset<StreamingImageAsset>& asset) override
            {
                asset->SetReady();
            }
        };

        class ImageMipChainAssetTester
            : public UnitTest::AssetTester<ImageMipChainAsset>
        {
        public:
            ImageMipChainAssetTester() = default;
            ~ImageMipChainAssetTester() override = default;

            void SetAssetReady(Data::Asset<ImageMipChainAsset>& asset) override
            {
                asset->SetReady();
            }
        };

        class StreamingImagePoolAssetTester
            : public UnitTest::SerializeTester<StreamingImagePoolAsset>
        {
            using Base = UnitTest::SerializeTester<StreamingImagePoolAsset>;
        public:
            StreamingImagePoolAssetTester(AZ::SerializeContext* serializeContext)
                : Base(serializeContext)
            {}

            AZ::Data::Asset<StreamingImagePoolAsset> SerializeInHelper(const AZ::Data::AssetId& assetId)
            {
                AZ::Data::Asset<StreamingImagePoolAsset> asset = Base::SerializeIn(assetId);
                asset->SetReady();
                return asset;
            }
        };
    }
}

namespace UnitTest
{
    struct TestStreamingImagePoolDescriptor
        : public AZ::RHI::StreamingImagePoolDescriptor
    {
        AZ_CLASS_ALLOCATOR(TestStreamingImagePoolDescriptor, AZ::SystemAllocator, 0);
        AZ_RTTI(TestStreamingImagePoolDescriptor, "{8D0CA5A2-F886-42EF-9B00-09E6C9F6B90B}", AZ::RHI::StreamingImagePoolDescriptor);

        static constexpr uint32_t Magic = 0x1234;

        static void Reflect(AZ::ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<TestStreamingImagePoolDescriptor, AZ::RHI::StreamingImagePoolDescriptor>()
                    ->Version(0)
                    ->Field("m_magic", &TestStreamingImagePoolDescriptor::m_magic)
                    ;
            }
        }

        TestStreamingImagePoolDescriptor() = default;
        TestStreamingImagePoolDescriptor(size_t budgetInBytes)
        {
            m_budgetInBytes = budgetInBytes;
        }

        // A test value to ensure that serialization occurred correctly.
        uint32_t m_magic = Magic;
    };

    class TestStreamingImageContext
        : public AZ::RPI::StreamingImageContext
        , public AZStd::intrusive_list_node<TestStreamingImageContext>
    {
    public:
        AZ_CLASS_ALLOCATOR(TestStreamingImageContext, AZ::SystemAllocator, 0);
        AZ_RTTI(TestStreamingImageContext, "{E2FC3EB5-4F66-41D0-9ABE-6EDD2622DD88}", AZ::RPI::StreamingImageContext);
    };

    class TestStreamingImageController final
        : public AZ::RPI::StreamingImageController
    {
    public:
        AZ_CLASS_ALLOCATOR(TestStreamingImageController, AZ::SystemAllocator, 0);
        AZ_RTTI(TestStreamingImageController, "{69D1A49C-B07E-4987-86D4-79C1F4E239B8}", AZ::RPI::StreamingImageController);

        TestStreamingImageController() = default;

    private:
        AZ::RPI::StreamingImageContextPtr CreateContextInternal() override
        {
            return aznew TestStreamingImageContext();
        }

        void UpdateInternal(size_t timestamp, const StreamingImageContextList&) override
        {
            EXPECT_EQ(timestamp, m_expectedTimestamp);
            m_expectedTimestamp++;
        }

        size_t m_expectedTimestamp = 0;
    };

    class StreamingImageTests
        : public RPITestFixture
    {
    private:
        AZ::Data::Asset<AZ::RPI::DefaultStreamingImageControllerAsset> m_testControllerAsset;

    protected:
        AZ::RPI::BuiltInAssetHandler* m_testControllerAssetHandler;
        AZ::Data::AssetId m_testControllerAssetId;

        AZ::Data::AssetHandler* m_imageHandler = nullptr;
        AZ::Data::AssetHandler* m_mipChainHandler = nullptr;
        AZ::Data::Instance<AZ::RPI::StreamingImagePool> m_defaultPool = nullptr;

        StreamingImageTests()
            : m_testControllerAssetId(AZ::RPI::DefaultStreamingImageControllerAsset::BuiltInAssetId)
        {}

        void SetUp() override
        {
            using namespace AZ;

            RPITestFixture::SetUp();

            auto* serializeContext = GetSerializeContext();
            TestStreamingImagePoolDescriptor::Reflect(serializeContext);

            m_imageHandler = Data::AssetManager::Instance().GetHandler(RPI::StreamingImageAsset::RTTI_Type());
            m_mipChainHandler = Data::AssetManager::Instance().GetHandler(RPI::ImageMipChainAsset::RTTI_Type());

            AZ::Data::Asset<AZ::RPI::StreamingImagePoolAsset> poolAsset = BuildImagePoolAsset(16 * 1024 * 1024);
            m_defaultPool = AZ::RPI::StreamingImagePool::FindOrCreate(poolAsset);
        }

        void TearDown() override
        {
            using namespace AZ;
            RHI::ResourceInvalidateBus::ExecuteQueuedEvents();

            m_defaultPool = nullptr;

            RPITestFixture::TearDown();
        }

        AZStd::vector<uint8_t> BuildImageData(uint32_t width, uint32_t height, uint32_t pixelSize)
        {
            const size_t imageSize = width * height * pixelSize;

            AZStd::vector<uint8_t> image;
            image.reserve(imageSize);

            uint8_t testValue = 0;

            for (uint32_t y = 0; y < height; ++y)
            {
                for (uint32_t x = 0; x < width; ++x)
                {
                    for (uint32_t channel = 0; channel < pixelSize; ++channel)
                    {
                        image.push_back(testValue);
                        testValue++;
                    }
                }
            }

            EXPECT_EQ(image.size(), imageSize);
            return image;
        }

        void ValidateImageData(AZStd::array_view<uint8_t> data, const AZ::RHI::ImageSubresourceLayout& layout)
        {
            const uint32_t pixelSize = layout.m_size.m_width / layout.m_bytesPerRow;

            uint32_t byteOffset = 0;

            for (uint32_t y = 0; y < layout.m_size.m_height; ++y)
            {
                for (uint32_t x = 0; x < layout.m_size.m_width; ++x)
                {
                    for (uint32_t channel = 0; channel < pixelSize; ++channel)
                    {
                        uint8_t value = data[byteOffset];

                        EXPECT_EQ(value, static_cast<uint8_t>(byteOffset));

                        byteOffset++;
                    }
                }
            }
        }

        void ValidateMipChainAsset(
            AZ::RPI::ImageMipChainAsset* mipChain,
            uint16_t expectedMipLevels,
            uint16_t expectedArraySize,
            uint32_t expectedPixelSize)
        {
            using namespace AZ;

            EXPECT_NE(mipChain, nullptr);
            EXPECT_EQ(expectedMipLevels, mipChain->GetMipLevelCount());
            EXPECT_EQ(expectedArraySize, mipChain->GetArraySize());
            EXPECT_EQ(expectedMipLevels * expectedArraySize, mipChain->GetSubImageCount());

            const uint32_t imageSize = 1 << mipChain->GetMipLevelCount();

            for (uint16_t mipLevel = 0; mipLevel < mipChain->GetMipLevelCount(); ++mipLevel)
            {
                RHI::ImageSubresourceLayout layout = BuildSubImageLayout(imageSize >> mipLevel, expectedPixelSize);

                EXPECT_EQ(memcmp(&layout, &mipChain->GetSubImageLayout(mipLevel), sizeof(RHI::ImageSubresourceLayout)), 0);

                for (uint16_t arrayIndex = 0; arrayIndex < mipChain->GetArraySize(); ++arrayIndex)
                {
                    AZStd::array_view<uint8_t> imageData = mipChain->GetSubImageData(mipLevel, arrayIndex);
                    ValidateImageData(imageData, layout);
                }
            }
        }

        void ValidateImageAsset(AZ::RPI::StreamingImageAsset* imageAsset)
        {
            using namespace AZ;

            EXPECT_NE(imageAsset, nullptr);

            RHI::ImageDescriptor imageDesc = imageAsset->GetImageDescriptor();

            size_t mipCountTotal = 0;
            for (size_t i = 0; i < imageAsset->GetMipChainCount(); ++i)
            {
                // The last mip chain asset (tail mip chain) is expected to be empty since the actual mip chain asset data is embedded in StreamingImageAsset
                if (i != imageAsset->GetMipChainCount() - 1)
                {
                    EXPECT_TRUE(imageAsset->GetMipChainAsset(i).GetId().IsValid());
                }
                EXPECT_EQ(imageAsset->GetMipLevel(i), mipCountTotal);
                mipCountTotal += imageAsset->GetMipCount(i);
            }

            EXPECT_EQ(imageDesc.m_mipLevels, mipCountTotal);
        }

        void ValidateImagePoolAsset(AZ::RPI::StreamingImagePoolAsset* poolAsset, size_t budgetInBytes)
        {
            using namespace AZ;

            EXPECT_EQ(poolAsset->GetPoolDescriptor().m_budgetInBytes, budgetInBytes);

            {
                const auto* desc = azrtti_cast<const TestStreamingImagePoolDescriptor*>(&poolAsset->GetPoolDescriptor());
                EXPECT_NE(desc, nullptr);
                EXPECT_EQ(desc->m_magic, UnitTest::TestStreamingImagePoolDescriptor::Magic);
            }

            {
                Data::Asset<RPI::StreamingImageControllerAsset> asset = poolAsset->GetControllerAsset();
                EXPECT_TRUE(azrtti_typeid<RPI::DefaultStreamingImageControllerAsset>() == asset.GetType());
            }
        }

        void ValidateImageResidency(AZ::RPI::StreamingImage* imageInstance, AZ::RPI::StreamingImageAsset* imageAsset)
        {
            using namespace AZ;

            auto imageSystem = RPI::ImageSystemInterface::Get();

            const size_t mipChainTailIndex = imageAsset->GetMipChainCount() - 1;

            RHI::Ptr<RHI::Image> rhiImage = imageInstance->GetRHIImage();

            // This should no-op.
            imageInstance->TrimToMipChainLevel(mipChainTailIndex);

            // Validate that nothing was actually evicted, since we've set to NoEvict.
            for (size_t i = 0; i < mipChainTailIndex; ++i)
            {
                EXPECT_TRUE(imageAsset->GetMipChainAsset(i).IsReady());
            }
            EXPECT_EQ(rhiImage->GetResidentMipLevel(), imageAsset->GetMipLevel(mipChainTailIndex));

            // Expand to the most detailed mip chain.
            imageInstance->QueueExpandToMipChainLevel(0);

            // We should still be the same residency level, since it's queued.
            EXPECT_EQ(rhiImage->GetResidentMipLevel(), imageAsset->GetMipLevel(mipChainTailIndex));

            // Tick the streaming system.
            imageSystem->Update();

            // Now we should be at the desired residency level.
            EXPECT_EQ(rhiImage->GetResidentMipLevel(), 0);

            // Expanding 'down' is a no-op.
            imageInstance->QueueExpandToMipChainLevel(1);
            imageSystem->Update();
            EXPECT_EQ(rhiImage->GetResidentMipLevel(), 0);

            // Trimming down a notch. This happens instantly.
            imageInstance->TrimToMipChainLevel(1);
            EXPECT_EQ(rhiImage->GetResidentMipLevel(), imageAsset->GetMipLevel(1));

            // Trim down again.
            imageInstance->TrimToMipChainLevel(2);
            EXPECT_EQ(rhiImage->GetResidentMipLevel(), imageAsset->GetMipLevel(2));

            // Expanding back up to 1.
            imageInstance->QueueExpandToMipChainLevel(1);
            imageSystem->Update();
            EXPECT_EQ(rhiImage->GetResidentMipLevel(), imageAsset->GetMipLevel(1));

            // Expanding back up to 0.
            imageInstance->QueueExpandToMipChainLevel(0);
            imageSystem->Update();
            EXPECT_EQ(rhiImage->GetResidentMipLevel(), 0);
        }

        AZ::RHI::ImageSubresourceLayout BuildSubImageLayout(uint32_t imageSize, uint32_t pixelSize)
        {
            using namespace AZ;

            RHI::ImageSubresourceLayout layout;
            layout.m_size = RHI::Size{ imageSize, imageSize, 1 };
            layout.m_rowCount = imageSize;
            layout.m_bytesPerRow = imageSize * pixelSize;
            layout.m_bytesPerImage = imageSize * imageSize * pixelSize;
            return layout;
        }

        AZ::Data::Asset<AZ::RPI::ImageMipChainAsset> BuildMipChainAsset(uint16_t mipOffset, uint16_t mipLevels, uint16_t arraySize, uint32_t pixelSize)
        {
            using namespace AZ;

            RPI::ImageMipChainAssetCreator assetCreator;

            const uint32_t imageSize = 1 << (mipLevels + mipOffset);

            assetCreator.Begin(Data::AssetId(AZ::Uuid::CreateRandom()), mipLevels, arraySize);

            for (uint32_t mipLevel = 0; mipLevel < mipLevels; ++mipLevel)
            {
                const uint32_t mipSize = imageSize >> mipLevel;

                RHI::ImageSubresourceLayout layout = BuildSubImageLayout(mipSize, pixelSize);

                assetCreator.BeginMip(layout);

                for (uint32_t arrayIndex = 0; arrayIndex < arraySize; ++arrayIndex)
                {
                    AZStd::vector<uint8_t> data = BuildImageData(mipSize, mipSize, pixelSize);
                    assetCreator.AddSubImage(data.data(), data.size());
                }

                assetCreator.EndMip();
            }

            Data::Asset<RPI::ImageMipChainAsset> asset;
            EXPECT_TRUE(assetCreator.End(asset));
            EXPECT_TRUE(asset.IsReady());
            EXPECT_NE(asset.Get(), nullptr);

            return asset;
        }

        AZ::Data::Asset<AZ::RPI::StreamingImagePoolAsset> BuildImagePoolAsset(size_t budgetInBytes)
        {
            using namespace AZ;

            RPI::StreamingImagePoolAssetCreator assetCreator;

            assetCreator.Begin(Data::AssetId(Uuid::CreateRandom()));

            assetCreator.SetPoolDescriptor(AZStd::make_unique<TestStreamingImagePoolDescriptor>(budgetInBytes));

            assetCreator.SetControllerAsset(
                Data::AssetManager::Instance().GetAsset<RPI::DefaultStreamingImageControllerAsset>(
                    m_testControllerAssetId,
                    Data::AssetLoadBehavior::PreLoad)
            );

            Data::Asset<RPI::StreamingImagePoolAsset> poolAsset;
            EXPECT_TRUE(assetCreator.End(poolAsset));

            EXPECT_TRUE(poolAsset.IsReady());
            EXPECT_NE(poolAsset.Get(), nullptr);
            return poolAsset;
        }

        AZ::Data::Asset<AZ::RPI::StreamingImageAsset> BuildTestImage()
        {
            using namespace AZ;

            const uint32_t arraySize = 2;
            const uint32_t pixelSize = 4;
            const uint32_t mipCountHead = 1;
            const uint32_t mipCountMiddle = 2;
            const uint32_t mipCountTail = 3;
            const uint32_t mipCountTotal = mipCountHead + mipCountMiddle + mipCountTail;
            const uint32_t imageWidth = 1 << mipCountTotal;
            const uint32_t imageHeight = 1 << mipCountTotal;

            Data::Asset<RPI::ImageMipChainAsset> mipTail = BuildMipChainAsset(0, mipCountTail, arraySize, pixelSize);
            Data::Asset<RPI::ImageMipChainAsset> mipMiddle = BuildMipChainAsset(mipCountTail, mipCountMiddle, arraySize, pixelSize);
            Data::Asset<RPI::ImageMipChainAsset> mipHead = BuildMipChainAsset(mipCountTail + mipCountMiddle, mipCountHead, arraySize, pixelSize);

            RPI::StreamingImageAssetCreator assetCreator;
            assetCreator.Begin(Data::AssetId(Uuid::CreateRandom()));

            RHI::ImageDescriptor imageDesc = RHI::ImageDescriptor::Create2DArray(RHI::ImageBindFlags::ShaderRead, imageWidth, imageHeight, arraySize, RHI::Format::R8G8B8A8_UNORM);
            imageDesc.m_mipLevels = static_cast<uint16_t>(mipCountTotal);

            assetCreator.SetImageDescriptor(imageDesc);
            assetCreator.AddMipChainAsset(*mipHead.Get());
            assetCreator.AddMipChainAsset(*mipMiddle.Get());
            assetCreator.AddMipChainAsset(*mipTail.Get());
            assetCreator.SetPoolAssetId(m_defaultPool->GetAssetId());

            Data::Asset<RPI::StreamingImageAsset> imageAsset;
            EXPECT_TRUE(assetCreator.End(imageAsset));

            EXPECT_TRUE(imageAsset.IsReady());
            EXPECT_NE(imageAsset.Get(), nullptr);
            return imageAsset;
        }
    };

    TEST_F(StreamingImageTests, MipChainCreate)
    {
        using namespace AZ;

        const uint16_t mipLevels = 5;
        const uint16_t arraySize = 4;
        const uint16_t pixelSize = 4;

        Data::Asset<RPI::ImageMipChainAsset> mipChain = BuildMipChainAsset(0, mipLevels, arraySize, pixelSize);

        ValidateMipChainAsset(mipChain.Get(), mipLevels, arraySize, pixelSize);
    }

    TEST_F(StreamingImageTests, MipChainAssetSuccessAfterErrorCases)
    {
        using namespace AZ;

        const uint16_t mipLevels = 1;
        const uint16_t arraySize = 1;

        Data::Asset<RPI::ImageMipChainAsset> mipChain;

        {
            RPI::ImageMipChainAssetCreator assetCreator;

            ErrorMessageFinder messageFinder("Begin() was not called");
            assetCreator.EndMip();
        }

        {
            RPI::ImageMipChainAssetCreator assetCreator;

            ErrorMessageFinder messageFinder("Begin() was not called");
            assetCreator.End(mipChain);
        }


        {
            RPI::ImageMipChainAssetCreator assetCreator;
            assetCreator.Begin(Data::AssetId(AZ::Uuid::CreateRandom()), mipLevels, arraySize);
            assetCreator.BeginMip(RHI::ImageSubresourceLayout());

            ErrorMessageFinder messageFinder("Expected 1 sub-images in mip, but got 0.");
            assetCreator.EndMip();
        }

        {
            RPI::ImageMipChainAssetCreator assetCreator;
            assetCreator.Begin(Data::AssetId(AZ::Uuid::CreateRandom()), mipLevels, arraySize);
            assetCreator.BeginMip(RHI::ImageSubresourceLayout());

            ErrorMessageFinder messageFinder("You must supply a valid data payload.");
            assetCreator.AddSubImage(nullptr, 0);
        }

        {
            RPI::ImageMipChainAssetCreator assetCreator;
            assetCreator.Begin(Data::AssetId(AZ::Uuid::CreateRandom()), mipLevels, arraySize);
            assetCreator.BeginMip(RHI::ImageSubresourceLayout());

            ErrorMessageFinder messageFinder("You must supply a valid data payload.");
            assetCreator.AddSubImage(nullptr, 10);
        }

        uint8_t data[4] = { 0, 5, 10, 15 };
        const uint8_t dataSize = AZ_ARRAY_SIZE(data);

        {
            RPI::ImageMipChainAssetCreator assetCreator;
            assetCreator.Begin(Data::AssetId(AZ::Uuid::CreateRandom()), mipLevels, arraySize);
            assetCreator.BeginMip(RHI::ImageSubresourceLayout());
            assetCreator.AddSubImage(data, dataSize);

            ErrorMessageFinder messageFinder("Exceeded the 1 array slices declared in Begin().");
            assetCreator.AddSubImage(data, dataSize);
        }

        {
            RPI::ImageMipChainAssetCreator assetCreator;
            assetCreator.Begin(Data::AssetId(AZ::Uuid::CreateRandom()), mipLevels, arraySize);
            assetCreator.BeginMip(RHI::ImageSubresourceLayout());
            assetCreator.AddSubImage(data, dataSize);

            ErrorMessageFinder messageFinder("Already building a mip. You must call EndMip() first.");
            assetCreator.BeginMip(RHI::ImageSubresourceLayout());
        }

        // Finally, build a valid one
        {
            RPI::ImageMipChainAssetCreator assetCreator;
            assetCreator.Begin(Data::AssetId(AZ::Uuid::CreateRandom()), mipLevels, arraySize);
            assetCreator.BeginMip(RHI::ImageSubresourceLayout());
            assetCreator.AddSubImage(data, dataSize);
            assetCreator.EndMip();

            EXPECT_TRUE(assetCreator.End(mipChain));

            EXPECT_NE(mipChain.Get(), nullptr);
            EXPECT_EQ(mipChain->GetMipLevelCount(), mipLevels);
            EXPECT_EQ(mipChain->GetArraySize(), arraySize);
            EXPECT_EQ(mipChain->GetSubImageCount(), mipLevels * arraySize);

            AZStd::array_view<uint8_t> dataView = mipChain->GetSubImageData(0);
            EXPECT_EQ(dataView[0], data[0]);
            EXPECT_EQ(dataView[1], data[1]);
            EXPECT_EQ(dataView[2], data[2]);
            EXPECT_EQ(dataView[3], data[3]);
        }
    }

    TEST_F(StreamingImageTests, MipChainAssetSerialize)
    {
        using namespace AZ;

        const uint16_t mipLevels = 6;
        const uint16_t arraySize = 2;
        const uint16_t pixelSize = 2;
        
        Data::Asset<RPI::ImageMipChainAsset> mipChain = BuildMipChainAsset(0, mipLevels, arraySize, pixelSize);
        RPI::ImageMipChainAssetTester tester;
        tester.SerializeOut(mipChain);

        Data::Asset<RPI::ImageMipChainAsset> serializedMipChain = tester.SerializeIn(Data::AssetId(Uuid::CreateRandom()));

        ValidateMipChainAsset(serializedMipChain.Get(), mipLevels, arraySize, pixelSize);
    }

    TEST_F(StreamingImageTests, PoolAssetCreation)
    {
        using namespace AZ;

        const size_t budgetInBytes = 16 * 1024 * 1024;

        Data::Asset<RPI::StreamingImagePoolAsset> poolAsset = BuildImagePoolAsset(budgetInBytes);
        ValidateImagePoolAsset(poolAsset.Get(), budgetInBytes);
    }

    TEST_F(StreamingImageTests, PoolAssetSerialize)
    {
        using namespace AZ;

        const size_t budgetInBytes = 16 * 1024 * 1024;

        Data::Asset<RPI::StreamingImagePoolAsset> poolAsset = BuildImagePoolAsset(budgetInBytes);
        RPI::StreamingImagePoolAssetTester tester(GetSerializeContext());
        tester.SerializeOut(poolAsset.Get());

        Data::Asset<RPI::StreamingImagePoolAsset> serializedPoolAsset = tester.SerializeInHelper(Data::AssetId(Uuid::CreateRandom()));
        ValidateImagePoolAsset(serializedPoolAsset.Get(), budgetInBytes);
    }

    TEST_F(StreamingImageTests, PoolInstanceCreation)
    {
        using namespace AZ;

        const size_t budgetInBytes = 16 * 1024 * 1024;

        Data::Asset<RPI::StreamingImagePoolAsset> poolAsset = BuildImagePoolAsset(budgetInBytes);

        Data::Instance<RPI::StreamingImagePool> poolInstance = RPI::StreamingImagePool::FindOrCreate(poolAsset);
        EXPECT_NE(poolInstance.get(), nullptr);
        EXPECT_NE(poolInstance->GetRHIPool(), nullptr);
    }

    TEST_F(StreamingImageTests, ImageAssetCreation)
    {
        using namespace AZ;

        Data::Asset<RPI::StreamingImageAsset> imageAsset = BuildTestImage();
        ValidateImageAsset(imageAsset.Get());
    }

    TEST_F(StreamingImageTests, ImageAssetSerialize)
    {
        using namespace AZ;

        Data::Asset<RPI::StreamingImageAsset> imageAsset = BuildTestImage();
        
        RPI::StreamingImageAssetTester tester;
        tester.SerializeOut(imageAsset);
        Data::Asset<RPI::StreamingImageAsset> serializedImageAsset = tester.SerializeIn(Data::AssetId(Uuid::CreateRandom()));

        ValidateImageAsset(serializedImageAsset.Get());
    }

    TEST_F(StreamingImageTests, ImageInstanceCreation)
    {
        using namespace AZ;

        Data::Asset<RPI::StreamingImageAsset> imageAsset = BuildTestImage();

        Data::Instance<RPI::StreamingImage> imageInstance = RPI::StreamingImage::FindOrCreate(imageAsset);
        EXPECT_NE(imageInstance.get(), nullptr);
        EXPECT_NE(imageInstance->GetRHIImage(), nullptr);
        EXPECT_NE(imageInstance->GetImageView(), nullptr);
        EXPECT_GE(imageAsset->GetMipChainCount(), 0);

        const size_t mipChainTailIndex = imageAsset->GetMipChainCount() - 1;

        EXPECT_EQ(imageInstance->GetRHIImage()->GetResidentMipLevel(), imageAsset->GetMipLevel(mipChainTailIndex));

        for (size_t i = 0; i < mipChainTailIndex; ++i)
        {
            Data::Asset<RPI::ImageMipChainAsset> mipChainAsset = imageAsset->GetMipChainAsset(i);

            EXPECT_TRUE(mipChainAsset.IsReady());
        }
    }

    TEST_F(StreamingImageTests, ImageInstanceResidency)
    {
        using namespace AZ;

        Data::Asset<RPI::StreamingImageAsset> imageAsset = BuildTestImage();
        Data::Instance<RPI::StreamingImage> imageInstance = RPI::StreamingImage::FindOrCreate(imageAsset);
        ValidateImageResidency(imageInstance.get(), imageAsset.Get());
    }

    TEST_F(StreamingImageTests, ImageInstanceResidencyWithSerialization)
    {
        using namespace AZ;

        // Keep the original around, which holds references to the image mip chain assets and pool asset. We need to
        // keep them in memory to avoid the asset manager trying to hit the catalog.
        Data::Asset<RPI::StreamingImageAsset> imageAsset = BuildTestImage();

        RPI::StreamingImageAssetTester tester;
        tester.SerializeOut(imageAsset);
        Data::Asset<RPI::StreamingImageAsset> serializedImageAsset = tester.SerializeIn(Data::AssetId(Uuid::CreateRandom()));
        
        Data::Instance<RPI::StreamingImage> imageInstance = RPI::StreamingImage::FindOrCreate(serializedImageAsset);
        ValidateImageResidency(imageInstance.get(), imageAsset.Get());
    }

    TEST_F(StreamingImageTests, ImageInternalReferenceTracking)
    {
        using namespace AZ;

        Data::Asset<RPI::StreamingImageAsset> imageAsset = BuildTestImage();

        Data::Instance<RPI::StreamingImagePool> imagePoolInstance;

        {
            Data::Instance<RPI::StreamingImage> imageInstance = RPI::StreamingImage::FindOrCreate(imageAsset);

            // Hold the pool instance to keep it around.
            imagePoolInstance = imageInstance->GetPool();

            // Tests that we can safely destroy an image after queueing something to the system,
            // and the system will properly avoid touching that data.
            imageInstance->QueueExpandToMipChainLevel(0);
        }

        RPI::ImageSystemInterface::Get()->Update();
    }
}
