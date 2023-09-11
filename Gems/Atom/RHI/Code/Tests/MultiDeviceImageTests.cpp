/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "RHITestFixture.h"
#include <Atom/RHI/MultiDeviceImagePool.h>
#include <Atom/RHI/ResourceInvalidateBus.h>
#include <Tests/Device.h>

namespace UnitTest
{
    using namespace AZ;

    class MultiDeviceImageTests : public MultiDeviceRHITestFixture
    {
    public:
        MultiDeviceImageTests()
            : MultiDeviceRHITestFixture()
        {}

        void SetUp() override
        {
            MultiDeviceRHITestFixture::SetUp();
        }

        void TearDown() override
        {
            MultiDeviceRHITestFixture::TearDown();
        }
    };

    TEST_F(MultiDeviceImageTests, TestNoop)
    {
        RHI::Ptr<RHI::MultiDeviceImage> noopImage;
        noopImage = aznew RHI::MultiDeviceImage;
    }

    TEST_F(MultiDeviceImageTests, TestAll)
    {
        RHI::Ptr<RHI::MultiDeviceImage> imageA;
        imageA = aznew RHI::MultiDeviceImage;
        imageA->SetName(Name("ImageA"));

        ASSERT_TRUE(imageA->GetName().GetStringView() == "ImageA");
        ASSERT_TRUE(imageA->use_count() == 1);

        {
            RHI::Ptr<RHI::MultiDeviceImage> imageB;
            imageB = aznew RHI::MultiDeviceImage;

            ASSERT_TRUE(imageB->use_count() == 1);

            RHI::Ptr<RHI::MultiDeviceImagePool> imagePool;
            imagePool = aznew RHI::MultiDeviceImagePool;

            ASSERT_TRUE(imagePool->use_count() == 1);

            RHI::ImagePoolDescriptor imagePoolDesc;
            imagePoolDesc.m_bindFlags = RHI::ImageBindFlags::Color;
            imagePool->Init(DeviceMask, imagePoolDesc);

            ASSERT_TRUE(imageA->IsInitialized() == false);
            ASSERT_TRUE(imageB->IsInitialized() == false);

            RHI::MultiDeviceImageInitRequest initRequest;
            initRequest.m_image = imageA.get();
            initRequest.m_descriptor = RHI::ImageDescriptor::Create2D(RHI::ImageBindFlags::Color, 16, 16, RHI::Format::R8G8B8A8_UNORM_SRGB);
            imagePool->InitImage(initRequest);
            ASSERT_TRUE(imageA->use_count() == 1);

            for(auto deviceIndex{0}; deviceIndex < DeviceCount; ++deviceIndex) 
            {
                RHI::Ptr<RHI::ImageView> imageView;
                imageView = imageA->GetDeviceImage(deviceIndex)->GetImageView(RHI::ImageViewDescriptor(RHI::Format::R8G8B8A8_UINT));
                AZ_TEST_ASSERT(imageView->IsStale() == false);
                ASSERT_TRUE(imageView->IsInitialized());
                ASSERT_TRUE(imageA->GetDeviceImage(deviceIndex)->use_count() == 3);
            }

            ASSERT_TRUE(imageA->use_count() == 1);
            ASSERT_TRUE(imageA->IsInitialized());

            initRequest.m_image = imageB.get();
            initRequest.m_descriptor = RHI::ImageDescriptor::Create2D(RHI::ImageBindFlags::Color, 8, 8, RHI::Format::R8G8B8A8_UNORM_SRGB);
            imagePool->InitImage(initRequest);

            ASSERT_TRUE(imageB->IsInitialized());

            ASSERT_TRUE(imageA->GetPool() == imagePool.get());
            ASSERT_TRUE(imageB->GetPool() == imagePool.get());
            ASSERT_TRUE(imagePool->GetResourceCount() == 2);

            {
                uint32_t imageIndex = 0;

                const RHI::MultiDeviceImage* images[] =
                {
                    imageA.get(),
                    imageB.get()
                };

                imagePool->ForEach<RHI::MultiDeviceImage>([&imageIndex, &images]([[maybe_unused]] const RHI::MultiDeviceImage& image)
                {
                    AZ_UNUSED(images); // Prevent unused warning in release builds
                    AZ_Assert(images[imageIndex] == &image, "images don't match");
                    imageIndex++;
                });
            }

            imageB->Shutdown();
            ASSERT_TRUE(imageB->GetPool() == nullptr);

            RHI::Ptr<RHI::MultiDeviceImagePool> imagePoolB;
            imagePoolB = aznew RHI::MultiDeviceImagePool;
            imagePoolB->Init(DeviceMask, imagePoolDesc);

            initRequest.m_image = imageB.get();
            initRequest.m_descriptor = RHI::ImageDescriptor::Create2D(RHI::ImageBindFlags::Color, 8, 8, RHI::Format::R8G8B8A8_UNORM_SRGB);
            imagePoolB->InitImage(initRequest);
            ASSERT_TRUE(imageB->GetPool() == imagePoolB.get());

            //Since we are switching imagePools for imageB it adds a refcount and invalidates the views.
            //We need this to ensure the views are fully invalidated in order to release the refcount and avoid a leak.
            RHI::ResourceInvalidateBus::ExecuteQueuedEvents();

            imagePoolB->Shutdown();
            ASSERT_TRUE(imagePoolB->GetResourceCount() == 0);
        }

        ASSERT_TRUE(imageA->GetPool() == nullptr);
        ASSERT_TRUE(imageA->use_count() == 1);
    }

    TEST_F(MultiDeviceImageTests, TestViews)
    {
        RHI::Ptr<RHI::ImageView> imageViewA;
        
        {
            RHI::Ptr<RHI::MultiDeviceImagePool> imagePool;
            imagePool = aznew RHI::MultiDeviceImagePool;

            RHI::ImagePoolDescriptor imagePoolDesc;
            imagePoolDesc.m_bindFlags = RHI::ImageBindFlags::Color;
            imagePool->Init(DeviceMask, imagePoolDesc);

            RHI::Ptr<RHI::MultiDeviceImage> image;
            image = aznew RHI::MultiDeviceImage;

            RHI::MultiDeviceImageInitRequest initRequest;
            initRequest.m_image = image.get();
            initRequest.m_descriptor = RHI::ImageDescriptor::Create2DArray(RHI::ImageBindFlags::Color, 8, 8, 2, RHI::Format::R8G8B8A8_UNORM_SRGB);
            imagePool->InitImage(initRequest);

            // Should report initialized and not stale.
            for(auto deviceIndex{0}; deviceIndex < DeviceCount; ++deviceIndex)
            {
                imageViewA = image->GetDeviceImage(deviceIndex)->GetImageView(RHI::ImageViewDescriptor{});
                AZ_TEST_ASSERT(imageViewA->IsInitialized());
                AZ_TEST_ASSERT(imageViewA->IsStale() == false);
                AZ_TEST_ASSERT(imageViewA->IsFullView());
            }

            // Should report as still initialized and also stale.
            image->Shutdown();
            for(auto deviceIndex{0}; deviceIndex < DeviceCount; ++deviceIndex)
            {
                AZ_TEST_ASSERT(imageViewA->IsStale());
                AZ_TEST_ASSERT(imageViewA->IsInitialized());
            }
            

            // Should *still* report as stale since resource invalidation events are queued.
            imagePool->InitImage(initRequest);
            for(auto deviceIndex{0}; deviceIndex < DeviceCount; ++deviceIndex)
            {
                AZ_TEST_ASSERT(imageViewA->IsStale());
                AZ_TEST_ASSERT(imageViewA->IsInitialized());
            }

            // This should re-initialize the views.
            RHI::ResourceInvalidateBus::ExecuteQueuedEvents();
            for(auto deviceIndex{0}; deviceIndex < DeviceCount; ++deviceIndex)
            {
                AZ_TEST_ASSERT(imageViewA->IsInitialized());
                AZ_TEST_ASSERT(imageViewA->IsStale() == false);
            }

            // Explicit invalidation should mark it stale.
            image->InvalidateViews();
            for(auto deviceIndex{0}; deviceIndex < DeviceCount; ++deviceIndex)
            {
                AZ_TEST_ASSERT(imageViewA->IsStale());
                AZ_TEST_ASSERT(imageViewA->IsInitialized());
            }

            // This should re-initialize the views.
            RHI::ResourceInvalidateBus::ExecuteQueuedEvents();
            for(auto deviceIndex{0}; deviceIndex < DeviceCount; ++deviceIndex)
            {
                AZ_TEST_ASSERT(imageViewA->IsInitialized());
                AZ_TEST_ASSERT(imageViewA->IsStale() == false);
            }

            // Test re-initialization.
            RHI::ImageViewDescriptor imageViewDesc = RHI::ImageViewDescriptor::Create(RHI::Format::Unknown, 0, 0, 0, 0);
            for(auto deviceIndex{0}; deviceIndex < DeviceCount; ++deviceIndex)
            {
                imageViewA = image->GetDeviceImage(deviceIndex)->GetImageView(imageViewDesc);
                AZ_TEST_ASSERT(imageViewA->IsFullView() == false);
                AZ_TEST_ASSERT(imageViewA->IsInitialized());
                AZ_TEST_ASSERT(imageViewA->IsStale() == false);
            }

            // Test re-initialization.
            imageViewDesc = RHI::ImageViewDescriptor::Create(RHI::Format::Unknown, 0, 0, 0, 1);
            for(auto deviceIndex{0}; deviceIndex < DeviceCount; ++deviceIndex)
            {
                imageViewA = image->GetDeviceImage(deviceIndex)->GetImageView(imageViewDesc);
                AZ_TEST_ASSERT(imageViewA->IsFullView());
                AZ_TEST_ASSERT(imageViewA->IsInitialized());
                AZ_TEST_ASSERT(imageViewA->IsStale() == false);
            }
        }

        // The parent image was shut down. This should report as being stale.
        AZ_TEST_ASSERT(imageViewA->IsStale());
    }

    struct MultiDeviceImageAndViewBindFlags
    {
        RHI::ImageBindFlags imageBindFlags;
        RHI::ImageBindFlags viewBindFlags;
    };

    class MultiDeviceImageBindFlagTests
        : public MultiDeviceImageTests
        , public ::testing::WithParamInterface <MultiDeviceImageAndViewBindFlags>
    {
    public:
        void SetUp() override
        {
            MultiDeviceImageTests::SetUp();

            // Create a pool and image with the image bind flags from the parameterized test
            m_imagePool = aznew RHI::MultiDeviceImagePool;
            RHI::ImagePoolDescriptor imagePoolDesc;
            imagePoolDesc.m_bindFlags = GetParam().imageBindFlags;
            m_imagePool->Init(DeviceMask, imagePoolDesc);

            RHI::ImageDescriptor imageDescriptor;
            imageDescriptor.m_bindFlags = GetParam().imageBindFlags;

            m_image = aznew RHI::MultiDeviceImage;
            RHI::MultiDeviceImageInitRequest initRequest;
            initRequest.m_image = m_image.get();
            initRequest.m_descriptor = imageDescriptor;
            m_imagePool->InitImage(initRequest);
        }

        void TearDown() override
        {
            m_imagePool.reset();
            m_image.reset();
            m_imageView.reset();
            
            MultiDeviceImageTests::TearDown();
        }

        RHI::Ptr<RHI::MultiDeviceImagePool> m_imagePool;
        RHI::Ptr<RHI::MultiDeviceImage> m_image;
        RHI::Ptr<RHI::ImageView> m_imageView;
    };

    TEST_P(MultiDeviceImageBindFlagTests, InitView_ViewIsCreated)
    {
        RHI::ImageViewDescriptor imageViewDescriptor;
        imageViewDescriptor.m_overrideBindFlags = GetParam().viewBindFlags;
        for(auto deviceIndex{0}; deviceIndex < DeviceCount; ++deviceIndex)
        {
            m_imageView = m_image->GetDeviceImage(deviceIndex)->GetImageView(imageViewDescriptor);
            EXPECT_EQ(m_imageView.get()!=nullptr, true);
        }
    }


    // This test fixture is the same as MultiDeviceImageBindFlagTests, but exists separately so that
    // we can instantiate different test cases that are expected to fail
    class MultiDeviceImageBindFlagFailureCases
        : public MultiDeviceImageBindFlagTests
    {

    };

    TEST_P(MultiDeviceImageBindFlagFailureCases, InitView_ViewIsNotCreated)
    {
        RHI::ImageViewDescriptor imageViewDescriptor;
        imageViewDescriptor.m_overrideBindFlags = GetParam().viewBindFlags;
        for(auto deviceIndex{0}; deviceIndex < DeviceCount; ++deviceIndex)
        {
            m_imageView = m_image->GetDeviceImage(deviceIndex)->GetImageView(imageViewDescriptor);
            EXPECT_EQ(m_imageView.get()==nullptr, true);
        }
    }

    // These combinations should result in a successful creation of the image view
    std::vector<MultiDeviceImageAndViewBindFlags> GenerateCompatibleMultiDeviceImageBindFlagCombinations()
    {
        std::vector<MultiDeviceImageAndViewBindFlags> testCases;
        MultiDeviceImageAndViewBindFlags flags;

        // When the image bind flags are equal to or a superset of the image view bind flags, the view is compatible with the image
        flags.imageBindFlags = RHI::ImageBindFlags::Color;
        flags.viewBindFlags = RHI::ImageBindFlags::Color;
        testCases.push_back(flags);

        flags.imageBindFlags = RHI::ImageBindFlags::ShaderReadWrite;
        flags.viewBindFlags = RHI::ImageBindFlags::ShaderRead;
        testCases.push_back(flags);

        flags.imageBindFlags = RHI::ImageBindFlags::ShaderReadWrite;
        flags.viewBindFlags = RHI::ImageBindFlags::ShaderWrite;
        testCases.push_back(flags);

        flags.imageBindFlags = RHI::ImageBindFlags::ShaderReadWrite;
        flags.viewBindFlags = RHI::ImageBindFlags::ShaderReadWrite;
        testCases.push_back(flags);

        flags.imageBindFlags = RHI::ImageBindFlags::ShaderRead;
        flags.viewBindFlags = RHI::ImageBindFlags::ShaderRead;
        testCases.push_back(flags);

        flags.imageBindFlags = RHI::ImageBindFlags::ShaderWrite;
        flags.viewBindFlags = RHI::ImageBindFlags::ShaderWrite;
        testCases.push_back(flags);

        // When the image view bind flags are None, they have no effect and should work with any bind flag used by the image
        flags.imageBindFlags = RHI::ImageBindFlags::ShaderRead;
        flags.viewBindFlags = RHI::ImageBindFlags::None;
        testCases.push_back(flags);

        flags.imageBindFlags = RHI::ImageBindFlags::ShaderWrite;
        flags.viewBindFlags = RHI::ImageBindFlags::None;
        testCases.push_back(flags);

        flags.imageBindFlags = RHI::ImageBindFlags::ShaderReadWrite;
        flags.viewBindFlags = RHI::ImageBindFlags::None;
        testCases.push_back(flags);

        flags.imageBindFlags = RHI::ImageBindFlags::None;
        flags.viewBindFlags = RHI::ImageBindFlags::None;
        testCases.push_back(flags);

        flags.imageBindFlags = RHI::ImageBindFlags::Color;
        flags.viewBindFlags = RHI::ImageBindFlags::None;
        testCases.push_back(flags);

        return testCases;
    };

    // These combinations should fail during ImageView::Init
    std::vector<MultiDeviceImageAndViewBindFlags> GenerateIncompatibleMultiDeviceImageBindFlagCombinations()
    {
        std::vector<MultiDeviceImageAndViewBindFlags> testCases;
        MultiDeviceImageAndViewBindFlags flags;

        flags.imageBindFlags = RHI::ImageBindFlags::Color;
        flags.viewBindFlags = RHI::ImageBindFlags::ShaderRead;
        testCases.push_back(flags);

        flags.imageBindFlags = RHI::ImageBindFlags::ShaderRead;
        flags.viewBindFlags = RHI::ImageBindFlags::ShaderWrite;
        testCases.push_back(flags);

        flags.imageBindFlags = RHI::ImageBindFlags::ShaderRead;
        flags.viewBindFlags = RHI::ImageBindFlags::ShaderReadWrite;
        testCases.push_back(flags);

        flags.imageBindFlags = RHI::ImageBindFlags::ShaderWrite;
        flags.viewBindFlags = RHI::ImageBindFlags::ShaderRead;
        testCases.push_back(flags);

        flags.imageBindFlags = RHI::ImageBindFlags::ShaderWrite;
        flags.viewBindFlags = RHI::ImageBindFlags::ShaderReadWrite;
        testCases.push_back(flags);

        flags.imageBindFlags = RHI::ImageBindFlags::None;
        flags.viewBindFlags = RHI::ImageBindFlags::ShaderRead;
        testCases.push_back(flags);

        flags.imageBindFlags = RHI::ImageBindFlags::None;
        flags.viewBindFlags = RHI::ImageBindFlags::ShaderWrite;
        testCases.push_back(flags);

        flags.imageBindFlags = RHI::ImageBindFlags::None;
        flags.viewBindFlags = RHI::ImageBindFlags::ShaderReadWrite;
        testCases.push_back(flags);

        return testCases;
    }

    std::string MultiDeviceImageBindFlagsToString(RHI::ImageBindFlags bindFlags)
    {
        switch (bindFlags)
        {
        case RHI::ImageBindFlags::None:
            return "None";
        case RHI::ImageBindFlags::Color:
            return "Color";
        case RHI::ImageBindFlags::ShaderRead:
            return "ShaderRead";
        case RHI::ImageBindFlags::ShaderWrite:
            return "ShaderWrite";
        case RHI::ImageBindFlags::ShaderReadWrite:
            return "ShaderReadWrite";
        default:
            AZ_Assert(false, "No string conversion was created for this bind flag setting.");
        }

        return "";
    }

    std::string GenerateMultiDeviceImageBindFlagTestCaseName(const ::testing::TestParamInfo<MultiDeviceImageAndViewBindFlags>& info)
    {
        return MultiDeviceImageBindFlagsToString(info.param.imageBindFlags) + "ImageWith" + MultiDeviceImageBindFlagsToString(info.param.viewBindFlags) + "ImageView";
    }

    INSTANTIATE_TEST_CASE_P(ImageView, MultiDeviceImageBindFlagTests, ::testing::ValuesIn(GenerateCompatibleMultiDeviceImageBindFlagCombinations()), GenerateMultiDeviceImageBindFlagTestCaseName);
    INSTANTIATE_TEST_CASE_P(ImageView, MultiDeviceImageBindFlagFailureCases, ::testing::ValuesIn(GenerateIncompatibleMultiDeviceImageBindFlagCombinations()), GenerateMultiDeviceImageBindFlagTestCaseName);
}
