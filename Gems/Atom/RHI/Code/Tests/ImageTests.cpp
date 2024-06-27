/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "RHITestFixture.h"
#include <Tests/Factory.h>
#include <Tests/Device.h>

namespace UnitTest
{
    using namespace AZ;

    class ImageTests
        : public RHITestFixture
    {
    public:
        ImageTests()
            : RHITestFixture()
        {}

        void SetUp() override
        {
            RHITestFixture::SetUp();
            m_factory.reset(aznew Factory());
        }

        void TearDown() override
        {
            m_factory.reset();
            RHITestFixture::TearDown();
        }

    private:
        AZStd::unique_ptr<Factory> m_factory;
    };

    TEST_F(ImageTests, TestNoop)
    {
        RHI::Ptr<RHI::DeviceImage> noopImage;
        noopImage = RHI::Factory::Get().CreateImage();
    }

    TEST_F(ImageTests, Test)
    {
        RHI::Ptr<RHI::Device> device = MakeTestDevice();

        RHI::Ptr<RHI::DeviceImage> imageA;
        imageA = RHI::Factory::Get().CreateImage();
        imageA->SetName(Name("ImageA"));

        ASSERT_TRUE(imageA->GetName().GetStringView() == "ImageA");
        ASSERT_TRUE(imageA->use_count() == 1);

        {
            RHI::Ptr<RHI::DeviceImage> imageB;
            imageB = RHI::Factory::Get().CreateImage();

            ASSERT_TRUE(imageB->use_count() == 1);

            RHI::Ptr<RHI::DeviceImagePool> imagePool;
            imagePool = RHI::Factory::Get().CreateImagePool();

            ASSERT_TRUE(imagePool->use_count() == 1);

            RHI::ImagePoolDescriptor imagePoolDesc;
            imagePoolDesc.m_bindFlags = RHI::ImageBindFlags::Color;
            imagePool->Init(*device, imagePoolDesc);

            ASSERT_TRUE(imageA->IsInitialized() == false);
            ASSERT_TRUE(imageB->IsInitialized() == false);

            RHI::DeviceImageInitRequest initRequest;
            initRequest.m_image = imageA.get();
            initRequest.m_descriptor = RHI::ImageDescriptor::Create2D(RHI::ImageBindFlags::Color, 16, 16, RHI::Format::R8G8B8A8_UNORM_SRGB);
            imagePool->InitImage(initRequest);
            ASSERT_TRUE(imageA->use_count() == 1);

            RHI::Ptr<RHI::DeviceImageView> imageView;
            imageView = imageA->GetImageView(RHI::ImageViewDescriptor(RHI::Format::R8G8B8A8_UINT));
            AZ_TEST_ASSERT(imageView->IsStale() == false);
            ASSERT_TRUE(imageView->IsInitialized());

            ASSERT_TRUE(imageA->use_count() == 2);
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

                const RHI::DeviceImage* images[] =
                {
                    imageA.get(),
                    imageB.get()
                };

                imagePool->ForEach<RHI::DeviceImage>([&imageIndex, &images]([[maybe_unused]] const RHI::DeviceImage& image)
                {
                    AZ_UNUSED(images); // Prevent unused warning in release builds
                    AZ_Assert(images[imageIndex] == &image, "images don't match");
                    imageIndex++;
                });
            }

            imageB->Shutdown();
            ASSERT_TRUE(imageB->GetPool() == nullptr);

            RHI::Ptr<RHI::DeviceImagePool> imagePoolB;
            imagePoolB = RHI::Factory::Get().CreateImagePool();
            imagePoolB->Init(*device, imagePoolDesc);

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

    TEST_F(ImageTests, TestViews)
    {
        RHI::Ptr<RHI::Device> device = MakeTestDevice();

        RHI::Ptr<RHI::DeviceImageView> imageViewA;
        
        {
            RHI::Ptr<RHI::DeviceImagePool> imagePool;
            imagePool = RHI::Factory::Get().CreateImagePool();

            RHI::ImagePoolDescriptor imagePoolDesc;
            imagePoolDesc.m_bindFlags = RHI::ImageBindFlags::Color;
            imagePool->Init(*device, imagePoolDesc);

            RHI::Ptr<RHI::DeviceImage> image;
            image = RHI::Factory::Get().CreateImage();

            RHI::DeviceImageInitRequest initRequest;
            initRequest.m_image = image.get();
            initRequest.m_descriptor = RHI::ImageDescriptor::Create2DArray(RHI::ImageBindFlags::Color, 8, 8, 2, RHI::Format::R8G8B8A8_UNORM_SRGB);
            imagePool->InitImage(initRequest);

            // Should report initialized and not stale.
            imageViewA = image->GetImageView(RHI::ImageViewDescriptor{});
            AZ_TEST_ASSERT(imageViewA->IsInitialized());
            AZ_TEST_ASSERT(imageViewA->IsStale() == false);
            AZ_TEST_ASSERT(imageViewA->IsFullView());

            // Should report as still initialized and also stale.
            image->Shutdown();
            AZ_TEST_ASSERT(imageViewA->IsStale());
            AZ_TEST_ASSERT(imageViewA->IsInitialized());

            // Should *still* report as stale since resource invalidation events are queued.
            imagePool->InitImage(initRequest);
            AZ_TEST_ASSERT(imageViewA->IsStale());
            AZ_TEST_ASSERT(imageViewA->IsInitialized());

            // This should re-initialize the views.
            RHI::ResourceInvalidateBus::ExecuteQueuedEvents();
            AZ_TEST_ASSERT(imageViewA->IsInitialized());
            AZ_TEST_ASSERT(imageViewA->IsStale() == false);

            // Explicit invalidation should mark it stale.
            image->InvalidateViews();
            AZ_TEST_ASSERT(imageViewA->IsStale());
            AZ_TEST_ASSERT(imageViewA->IsInitialized());

            // This should re-initialize the views.
            RHI::ResourceInvalidateBus::ExecuteQueuedEvents();
            AZ_TEST_ASSERT(imageViewA->IsInitialized());
            AZ_TEST_ASSERT(imageViewA->IsStale() == false);

            // Test re-initialization.
            RHI::ImageViewDescriptor imageViewDesc = RHI::ImageViewDescriptor::Create(RHI::Format::Unknown, 0, 0, 0, 0);
            imageViewA = image->GetImageView(imageViewDesc);
            AZ_TEST_ASSERT(imageViewA->IsFullView() == false);
            AZ_TEST_ASSERT(imageViewA->IsInitialized());
            AZ_TEST_ASSERT(imageViewA->IsStale() == false);

            // Test re-initialization.
            imageViewDesc = RHI::ImageViewDescriptor::Create(RHI::Format::Unknown, 0, 0, 0, 1);
            imageViewA = image->GetImageView(imageViewDesc);
            AZ_TEST_ASSERT(imageViewA->IsFullView());
            AZ_TEST_ASSERT(imageViewA->IsInitialized());
            AZ_TEST_ASSERT(imageViewA->IsStale() == false);
        }

        // The parent image was shut down. This should report as being stale.
        AZ_TEST_ASSERT(imageViewA->IsStale());
    }

    struct ImageAndViewBindFlags
    {
        RHI::ImageBindFlags imageBindFlags;
        RHI::ImageBindFlags viewBindFlags;
    };

    class ImageBindFlagTests
        : public ImageTests
        , public ::testing::WithParamInterface <ImageAndViewBindFlags>
    {
    public:
        void SetUp() override
        {
            ImageTests::SetUp();

            m_device = MakeTestDevice();

            // Create a pool and image with the image bind flags from the parameterized test
            m_imagePool = RHI::Factory::Get().CreateImagePool();
            RHI::ImagePoolDescriptor imagePoolDesc;
            imagePoolDesc.m_bindFlags = GetParam().imageBindFlags;
            m_imagePool->Init(*m_device, imagePoolDesc);

            RHI::ImageDescriptor imageDescriptor;
            imageDescriptor.m_bindFlags = GetParam().imageBindFlags;

            m_image = RHI::Factory::Get().CreateImage();
            RHI::DeviceImageInitRequest initRequest;
            initRequest.m_image = m_image.get();
            initRequest.m_descriptor = imageDescriptor;
            m_imagePool->InitImage(initRequest);
        }

        RHI::Ptr<RHI::Device> m_device;
        RHI::Ptr<RHI::DeviceImagePool> m_imagePool;
        RHI::Ptr<RHI::DeviceImage> m_image;
        RHI::Ptr<RHI::DeviceImageView> m_imageView;
    };

    TEST_P(ImageBindFlagTests, InitView_ViewIsCreated)
    {
        RHI::ImageViewDescriptor imageViewDescriptor;
        imageViewDescriptor.m_overrideBindFlags = GetParam().viewBindFlags;
        m_imageView = m_image->GetImageView(imageViewDescriptor);
        EXPECT_EQ(m_imageView.get()!=nullptr, true);
    }


    // This test fixture is the same as ImageBindFlagTests, but exists separately so that
    // we can instantiate different test cases that are expected to fail
    class ImageBindFlagFailureCases
        : public ImageBindFlagTests
    {

    };

    TEST_P(ImageBindFlagFailureCases, InitView_ViewIsNotCreated)
    {
        RHI::ImageViewDescriptor imageViewDescriptor;
        imageViewDescriptor.m_overrideBindFlags = GetParam().viewBindFlags;
        m_imageView = m_image->GetImageView(imageViewDescriptor);
        EXPECT_EQ(m_imageView.get()==nullptr, true);
    }

    // These combinations should result in a successful creation of the image view
    std::vector<ImageAndViewBindFlags> GenerateCompatibleImageBindFlagCombinations()
    {
        std::vector<ImageAndViewBindFlags> testCases;
        ImageAndViewBindFlags flags;

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
    std::vector<ImageAndViewBindFlags> GenerateIncompatibleImageBindFlagCombinations()
    {
        std::vector<ImageAndViewBindFlags> testCases;
        ImageAndViewBindFlags flags;

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

    std::string ImageBindFlagsToString(RHI::ImageBindFlags bindFlags)
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

    std::string GenerateImageBindFlagTestCaseName(const ::testing::TestParamInfo<ImageAndViewBindFlags>& info)
    {
        return ImageBindFlagsToString(info.param.imageBindFlags) + "ImageWith" + ImageBindFlagsToString(info.param.viewBindFlags) + "ImageView";
    }

    INSTANTIATE_TEST_CASE_P(ImageView, ImageBindFlagTests, ::testing::ValuesIn(GenerateCompatibleImageBindFlagCombinations()), GenerateImageBindFlagTestCaseName);
    INSTANTIATE_TEST_CASE_P(ImageView, ImageBindFlagFailureCases, ::testing::ValuesIn(GenerateIncompatibleImageBindFlagCombinations()), GenerateImageBindFlagTestCaseName);
}
