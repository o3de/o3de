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

namespace UnitTest
{
    using namespace AZ;
    using namespace RPI;

    class ShaderResourceGroupImageTests
        : public RPITestFixture
    {
    protected:
        RHI::Ptr<RHI::ShaderResourceGroupLayout> m_testSrgLayout;
        Data::Asset<ShaderAsset> m_testShaderAsset;
        Data::Instance<ShaderResourceGroup> m_testSrg;
        Data::Instance<Image> m_whiteImage;
        Data::Instance<Image> m_blackImage;
        Data::Instance<Image> m_greyImage;
        Ptr<RHI::ImageView> m_imageViewA;
        Ptr<RHI::ImageView> m_imageViewB;
        Ptr<RHI::ImageView> m_imageViewC;
        AZStd::vector<Data::Instance<Image>> m_threeImages;
        AZStd::vector<const RHI::ImageView*> m_threeImageViews;
        const RHI::ShaderInputImageIndex m_indexImageA{ 0 };
        const RHI::ShaderInputImageIndex m_indexImageB{ 1 };
        const RHI::ShaderInputImageIndex m_indexImageArray{ 2 };
        const RHI::ShaderInputImageIndex m_indexImageInvalid{ 3 };

        RHI::Ptr<RHI::ShaderResourceGroupLayout> CreateTestSrgLayout(const char* nameId)
        {
            RHI::Ptr<RHI::ShaderResourceGroupLayout> srgLayout = RHI::ShaderResourceGroupLayout::Create();

            srgLayout->SetName(Name(nameId));
            srgLayout->SetBindingSlot(0);
            srgLayout->AddShaderInput(RHI::ShaderInputImageDescriptor{ Name{ "MyImageA" }, RHI::ShaderInputImageAccess::Read, RHI::ShaderInputImageType::Image2D, 1, 1, 1});
            srgLayout->AddShaderInput(RHI::ShaderInputImageDescriptor{ Name{ "MyImageB" }, RHI::ShaderInputImageAccess::Read, RHI::ShaderInputImageType::Image2D, 1, 2, 2});
            srgLayout->AddShaderInput(RHI::ShaderInputImageDescriptor{ Name{ "MyImageArray" }, RHI::ShaderInputImageAccess::Read, RHI::ShaderInputImageType::Image2D, 3, 3, 3});
            srgLayout->Finalize();

            return srgLayout;
        }

        void SetUp() override
        {
            RPITestFixture::SetUp();

            m_testSrgLayout = CreateTestSrgLayout("TestSrg");
            m_testShaderAsset = CreateTestShaderAsset(Uuid::CreateRandom(), m_testSrgLayout);
            m_testSrg = ShaderResourceGroup::Create(m_testShaderAsset, AZ::RPI::DefaultSupervariantIndex, m_testSrgLayout->GetName());

            m_whiteImage = RPI::ImageSystemInterface::Get()->GetSystemImage(RPI::SystemImage::White);
            m_blackImage = RPI::ImageSystemInterface::Get()->GetSystemImage(RPI::SystemImage::Black);
            m_greyImage = RPI::ImageSystemInterface::Get()->GetSystemImage(RPI::SystemImage::Grey);

            m_threeImages = { m_whiteImage, m_blackImage, m_greyImage };

            RHI::ImageViewDescriptor imageViewDescA = RHI::ImageViewDescriptor::Create(RHI::Format::Unknown, 1, 1);
            m_imageViewA = m_whiteImage->GetRHIImage()->BuildImageView(imageViewDescA);

            RHI::ImageViewDescriptor imageViewDescB = RHI::ImageViewDescriptor::Create(RHI::Format::Unknown, 2, 2);
            m_imageViewB = m_whiteImage->GetRHIImage()->BuildImageView(imageViewDescB);

            RHI::ImageViewDescriptor imageViewDescC = RHI::ImageViewDescriptor::Create(RHI::Format::Unknown, 3, 3);
            m_imageViewC = m_whiteImage->GetRHIImage()->BuildImageView(imageViewDescC);
            
            m_threeImageViews = { m_imageViewA.get(), m_imageViewB.get(), m_imageViewC.get() };
        }

        void TearDown() override
        {
            m_testShaderAsset.Reset();
            m_testSrgLayout = nullptr;
            m_testSrg = nullptr;

            m_threeImages = AZStd::vector<Data::Instance<Image>>();
            m_threeImageViews = AZStd::vector<const RHI::ImageView*>();
            m_imageViewA = nullptr;
            m_imageViewB = nullptr;
            m_imageViewC = nullptr;
            m_whiteImage = nullptr;
            m_blackImage = nullptr;
            m_greyImage = nullptr;

            RPITestFixture::TearDown();
        }
    };

    TEST_F(ShaderResourceGroupImageTests, TestInvalidInputIndex)
    {
        // Test invalid indexes for all get/set functions

        const int imageInvalidArrayOffset = 3;

        AZ_TEST_START_ASSERTTEST;

        // Images...

        EXPECT_FALSE(m_testSrg->SetImage(m_indexImageInvalid, m_whiteImage));
        EXPECT_FALSE(m_testSrg->GetImage(m_indexImageInvalid));

        EXPECT_FALSE(m_testSrg->SetImage(m_indexImageInvalid, m_whiteImage, 1));
        EXPECT_FALSE(m_testSrg->SetImage(m_indexImageArray, m_whiteImage, imageInvalidArrayOffset));
        EXPECT_FALSE(m_testSrg->GetImage(m_indexImageInvalid, 1));
        EXPECT_FALSE(m_testSrg->GetImage(m_indexImageArray, imageInvalidArrayOffset));

        EXPECT_FALSE(m_testSrg->SetImageArray(m_indexImageInvalid, m_threeImages));
        EXPECT_FALSE(m_testSrg->SetImageArray(m_indexImageInvalid, m_threeImages, 0));
        EXPECT_EQ(0, m_testSrg->GetImageArray(m_indexImageInvalid).size());

        // Image Views...

        EXPECT_FALSE(m_testSrg->SetImageView(m_indexImageInvalid, m_imageViewA.get()));
        EXPECT_FALSE(m_testSrg->GetImageView(m_indexImageInvalid));

        EXPECT_FALSE(m_testSrg->SetImageView(m_indexImageInvalid, m_imageViewA.get(), 1));
        EXPECT_FALSE(m_testSrg->GetImageView(m_indexImageInvalid, 1));

        EXPECT_FALSE(m_testSrg->SetImageView(m_indexImageArray, m_imageViewA.get(), imageInvalidArrayOffset));
        EXPECT_FALSE(m_testSrg->GetImageView(m_indexImageArray, imageInvalidArrayOffset));

        EXPECT_FALSE(m_testSrg->SetImageViewArray(m_indexImageInvalid, m_threeImageViews));
        EXPECT_FALSE(m_testSrg->SetImageViewArray(m_indexImageInvalid, m_threeImageViews, 0));
        EXPECT_EQ(0, m_testSrg->GetImageViewArray(m_indexImageInvalid).size());

        AZ_TEST_STOP_ASSERTTEST(18);
    }

    TEST_F(ShaderResourceGroupImageTests, TestSetGetImage)
    {
        // Test basic set/get operation...

        EXPECT_TRUE(m_testSrg->SetImage(m_indexImageA, m_whiteImage));
        EXPECT_TRUE(m_testSrg->SetImage(m_indexImageB, m_blackImage));
        EXPECT_EQ(m_whiteImage, m_testSrg->GetImage(m_indexImageA));
        EXPECT_EQ(m_blackImage, m_testSrg->GetImage(m_indexImageB));

        m_testSrg->Compile();
        EXPECT_EQ(m_whiteImage->GetImageView(), m_testSrg->GetImageView(m_indexImageA, 0));
        EXPECT_EQ(m_blackImage->GetImageView(), m_testSrg->GetImageView(m_indexImageB, 0));

        // Test changing back to null...

        ProcessQueuedSrgCompilations(m_testShaderAsset, m_testSrgLayout->GetName());

        EXPECT_TRUE(m_testSrg->SetImage(m_indexImageA, nullptr));
        m_testSrg->Compile();
        EXPECT_EQ(nullptr, m_testSrg->GetImage(m_indexImageA));
        EXPECT_EQ(nullptr, m_testSrg->GetImageView(m_indexImageA, 0));
    }

    TEST_F(ShaderResourceGroupImageTests, TestSetGetImageAtOffset)
    {
        // Test basic set/get operation...

        EXPECT_TRUE(m_testSrg->SetImage(m_indexImageArray, m_whiteImage, 0));
        EXPECT_TRUE(m_testSrg->SetImage(m_indexImageArray, m_blackImage, 1));
        EXPECT_TRUE(m_testSrg->SetImage(m_indexImageArray, m_greyImage, 2));
        EXPECT_EQ(m_whiteImage, m_testSrg->GetImage(m_indexImageArray, 0));
        EXPECT_EQ(m_blackImage, m_testSrg->GetImage(m_indexImageArray, 1));
        EXPECT_EQ(m_greyImage, m_testSrg->GetImage(m_indexImageArray, 2));

        m_testSrg->Compile();
        EXPECT_EQ(m_whiteImage->GetImageView(), m_testSrg->GetImageView(m_indexImageArray, 0));
        EXPECT_EQ(m_blackImage->GetImageView(), m_testSrg->GetImageView(m_indexImageArray, 1));
        EXPECT_EQ(m_greyImage->GetImageView(), m_testSrg->GetImageView(m_indexImageArray, 2));

        // Test changing back to null...

        ProcessQueuedSrgCompilations(m_testShaderAsset, m_testSrgLayout->GetName());

        EXPECT_TRUE(m_testSrg->SetImage(m_indexImageArray, nullptr, 1));
        m_testSrg->Compile();
        EXPECT_EQ(nullptr, m_testSrg->GetImage(m_indexImageArray, 1));
        EXPECT_EQ(nullptr, m_testSrg->GetImageView(m_indexImageArray, 1));
    }

    TEST_F(ShaderResourceGroupImageTests, TestSetGetImageArray)
    {
        // Test basic set/get operation...

        EXPECT_TRUE(m_testSrg->SetImageArray(m_indexImageArray, m_threeImages));
        m_testSrg->Compile();

        EXPECT_EQ(3, m_testSrg->GetImageArray(m_indexImageArray).size());
        EXPECT_EQ(m_threeImages[0], m_testSrg->GetImageArray(m_indexImageArray)[0]);
        EXPECT_EQ(m_threeImages[1], m_testSrg->GetImageArray(m_indexImageArray)[1]);
        EXPECT_EQ(m_threeImages[2], m_testSrg->GetImageArray(m_indexImageArray)[2]);
        EXPECT_EQ(m_threeImages[0], m_testSrg->GetImage(m_indexImageArray, 0));
        EXPECT_EQ(m_threeImages[1], m_testSrg->GetImage(m_indexImageArray, 1));
        EXPECT_EQ(m_threeImages[2], m_testSrg->GetImage(m_indexImageArray, 2));
        EXPECT_EQ(m_threeImages[0]->GetImageView(), m_testSrg->GetImageView(m_indexImageArray, 0));
        EXPECT_EQ(m_threeImages[1]->GetImageView(), m_testSrg->GetImageView(m_indexImageArray, 1));
        EXPECT_EQ(m_threeImages[2]->GetImageView(), m_testSrg->GetImageView(m_indexImageArray, 2));

        // Test replacing just two images including changing one image back to null...

        ProcessQueuedSrgCompilations(m_testShaderAsset, m_testSrgLayout->GetName());

        AZStd::vector<Data::Instance<Image>> alternateImages = { m_blackImage, nullptr };

        EXPECT_TRUE(m_testSrg->SetImageArray(m_indexImageArray, alternateImages));
        m_testSrg->Compile();

        EXPECT_TRUE(m_testSrg->GetImageArray(m_indexImageArray).size());
        EXPECT_EQ(m_blackImage, m_testSrg->GetImage(m_indexImageArray, 0));
        EXPECT_EQ(nullptr, m_testSrg->GetImage(m_indexImageArray, 1));
        EXPECT_EQ(m_greyImage, m_testSrg->GetImage(m_indexImageArray, 2)); // remains unchanged
    }

    TEST_F(ShaderResourceGroupImageTests, TestSetGetImageArray_ValidationFailure)
    {
        // Make sure the no changes are made when a validation failure is detected

        AZStd::vector<Data::Instance<Image>> tooManyImages{ 4, m_whiteImage };

        AZ_TEST_START_ASSERTTEST;
        EXPECT_FALSE(m_testSrg->SetImageArray(m_indexImageArray, tooManyImages));
        AZ_TEST_STOP_ASSERTTEST(1);

        m_testSrg->Compile();
        EXPECT_EQ(nullptr, m_testSrg->GetImage(m_indexImageArray, 0));
        EXPECT_EQ(nullptr, m_testSrg->GetImage(m_indexImageArray, 1));
        EXPECT_EQ(nullptr, m_testSrg->GetImage(m_indexImageArray, 2));
        EXPECT_EQ(nullptr, m_testSrg->GetImageView(m_indexImageArray, 0));
        EXPECT_EQ(nullptr, m_testSrg->GetImageView(m_indexImageArray, 1));
        EXPECT_EQ(nullptr, m_testSrg->GetImageView(m_indexImageArray, 2));
    }

    TEST_F(ShaderResourceGroupImageTests, TestSetImageArrayAtOffset)
    {
        AZStd::vector<Data::Instance<Image>> twoImages = { m_blackImage, m_greyImage };

        // Test set operation, skipping the first element...

        EXPECT_TRUE(m_testSrg->SetImageArray(m_indexImageArray, twoImages, 1));
        m_testSrg->Compile();

        EXPECT_EQ(3, m_testSrg->GetImageArray(m_indexImageArray).size());
        EXPECT_EQ(nullptr, m_testSrg->GetImageArray(m_indexImageArray)[0]);
        EXPECT_EQ(twoImages[0], m_testSrg->GetImageArray(m_indexImageArray)[1]);
        EXPECT_EQ(twoImages[1], m_testSrg->GetImageArray(m_indexImageArray)[2]);
        EXPECT_EQ(nullptr, m_testSrg->GetImage(m_indexImageArray, 0));
        EXPECT_EQ(twoImages[0], m_testSrg->GetImage(m_indexImageArray, 1));
        EXPECT_EQ(twoImages[1], m_testSrg->GetImage(m_indexImageArray, 2));
        EXPECT_EQ(nullptr, m_testSrg->GetImageView(m_indexImageArray, 0));
        EXPECT_EQ(twoImages[0]->GetImageView(), m_testSrg->GetImageView(m_indexImageArray, 1));
        EXPECT_EQ(twoImages[1]->GetImageView(), m_testSrg->GetImageView(m_indexImageArray, 2));
    }

    TEST_F(ShaderResourceGroupImageTests, TestSetImageArrayAtOffset_ValidationFailure)
    {
        // Make sure the no changes are made when a validation failure is detected

        // 3 entries is too many because we will start at an offset of 1
        AZStd::vector<Data::Instance<Image>> tooManyImages{ 3, m_whiteImage };

        AZ_TEST_START_ASSERTTEST;
        EXPECT_FALSE(m_testSrg->SetImageArray(m_indexImageArray, tooManyImages, 1));
        AZ_TEST_STOP_ASSERTTEST(1);

        m_testSrg->Compile();
        EXPECT_EQ(nullptr, m_testSrg->GetImage(m_indexImageArray, 0));
        EXPECT_EQ(nullptr, m_testSrg->GetImage(m_indexImageArray, 1));
        EXPECT_EQ(nullptr, m_testSrg->GetImage(m_indexImageArray, 2));
        EXPECT_EQ(nullptr, m_testSrg->GetImageView(m_indexImageArray, 0));
        EXPECT_EQ(nullptr, m_testSrg->GetImageView(m_indexImageArray, 1));
        EXPECT_EQ(nullptr, m_testSrg->GetImageView(m_indexImageArray, 2));
    }

    TEST_F(ShaderResourceGroupImageTests, TestSetGetImageView)
    {
        // Set some RPI::Images first, just to make sure these get cleared when setting an RPI::ImageView

        EXPECT_TRUE(m_testSrg->SetImage(m_indexImageA, m_blackImage));
        EXPECT_TRUE(m_testSrg->SetImage(m_indexImageB, m_blackImage));

        // Test valid set/get operation...

        EXPECT_TRUE(m_testSrg->SetImageView(m_indexImageA, m_imageViewA.get()));
        EXPECT_TRUE(m_testSrg->SetImageView(m_indexImageB, m_imageViewB.get()));

        m_testSrg->Compile();

        EXPECT_EQ(m_imageViewA, m_testSrg->GetImageView(m_indexImageA));
        EXPECT_EQ(m_imageViewB, m_testSrg->GetImageView(m_indexImageB));
        EXPECT_EQ(m_imageViewA, m_testSrg->GetImageView(m_indexImageA, 0));
        EXPECT_EQ(m_imageViewB, m_testSrg->GetImageView(m_indexImageB, 0));

        // The RPI::Image should get cleared when you set a RHI::DeviceImageView directly
        EXPECT_EQ(nullptr, m_testSrg->GetImage(m_indexImageA));
        EXPECT_EQ(nullptr, m_testSrg->GetImage(m_indexImageB));
    }

    TEST_F(ShaderResourceGroupImageTests, TestSetGetImageViewAtOffset)
    {
        // Set some RPI::Images first, just to make sure these get cleared when setting an RPI::ImageView

        EXPECT_TRUE(m_testSrg->SetImage(m_indexImageArray, m_blackImage, 0));
        EXPECT_TRUE(m_testSrg->SetImage(m_indexImageArray, m_blackImage, 1));
        EXPECT_TRUE(m_testSrg->SetImage(m_indexImageArray, m_blackImage, 2));

        // Test valid set/get operation...

        EXPECT_TRUE(m_testSrg->SetImageView(m_indexImageArray, m_imageViewA.get(), 0));
        EXPECT_TRUE(m_testSrg->SetImageView(m_indexImageArray, m_imageViewB.get(), 1));
        EXPECT_TRUE(m_testSrg->SetImageView(m_indexImageArray, m_imageViewC.get(), 2));

        m_testSrg->Compile();

        EXPECT_EQ(m_imageViewA, m_testSrg->GetImageView(m_indexImageArray, 0));
        EXPECT_EQ(m_imageViewB, m_testSrg->GetImageView(m_indexImageArray, 1));
        EXPECT_EQ(m_imageViewC, m_testSrg->GetImageView(m_indexImageArray, 2));
        EXPECT_EQ(m_imageViewA, m_testSrg->GetImageView(m_indexImageArray, 0));
        EXPECT_EQ(m_imageViewB, m_testSrg->GetImageView(m_indexImageArray, 1));
        EXPECT_EQ(m_imageViewC, m_testSrg->GetImageView(m_indexImageArray, 2));

        // The RPI::Image should get cleared when you set a RHI::DeviceImageView directly
        EXPECT_EQ(nullptr, m_testSrg->GetImage(m_indexImageArray, 0));
        EXPECT_EQ(nullptr, m_testSrg->GetImage(m_indexImageArray, 1));
        EXPECT_EQ(nullptr, m_testSrg->GetImage(m_indexImageArray, 2));
    }

    TEST_F(ShaderResourceGroupImageTests, TestSetGetImageViewArray)
    {
        // Test basic set/get operation...

        EXPECT_TRUE(m_testSrg->SetImageViewArray(m_indexImageArray, m_threeImageViews));
        m_testSrg->Compile();

        EXPECT_EQ(3, m_testSrg->GetImageViewArray(m_indexImageArray).size());
        EXPECT_EQ(m_threeImageViews[0], m_testSrg->GetImageViewArray(m_indexImageArray)[0]);
        EXPECT_EQ(m_threeImageViews[1], m_testSrg->GetImageViewArray(m_indexImageArray)[1]);
        EXPECT_EQ(m_threeImageViews[2], m_testSrg->GetImageViewArray(m_indexImageArray)[2]);
        EXPECT_EQ(m_threeImageViews[0], m_testSrg->GetImageView(m_indexImageArray, 0));
        EXPECT_EQ(m_threeImageViews[1], m_testSrg->GetImageView(m_indexImageArray, 1));
        EXPECT_EQ(m_threeImageViews[2], m_testSrg->GetImageView(m_indexImageArray, 2));
        EXPECT_EQ(m_threeImageViews[0], m_testSrg->GetImageView(m_indexImageArray, 0));
        EXPECT_EQ(m_threeImageViews[1], m_testSrg->GetImageView(m_indexImageArray, 1));
        EXPECT_EQ(m_threeImageViews[2], m_testSrg->GetImageView(m_indexImageArray, 2));

        // Test replacing just two image views including changing one back to null...

        ProcessQueuedSrgCompilations(m_testShaderAsset, m_testSrgLayout->GetName());

        AZStd::vector<const RHI::ImageView*> alternateImageViews = { m_imageViewB.get(), nullptr };

        EXPECT_TRUE(m_testSrg->SetImageViewArray(m_indexImageArray, alternateImageViews));
        m_testSrg->Compile();

        EXPECT_EQ(m_imageViewB, m_testSrg->GetImageView(m_indexImageArray, 0));
        EXPECT_EQ(nullptr, m_testSrg->GetImageView(m_indexImageArray, 1));
        EXPECT_EQ(m_imageViewC, m_testSrg->GetImageView(m_indexImageArray, 2)); // remains unchanged
    }

    TEST_F(ShaderResourceGroupImageTests, TestSetGetImageViewArray_ValidationFailure)
    {
        // Make sure the no changes are made when a validation failure is detected

        AZStd::vector<const RHI::ImageView*> tooManyImageViews{ 4, m_imageViewA.get() };

        AZ_TEST_START_ASSERTTEST;
        EXPECT_FALSE(m_testSrg->SetImageViewArray(m_indexImageArray, tooManyImageViews));
        AZ_TEST_STOP_ASSERTTEST(1);

        m_testSrg->Compile();
        EXPECT_EQ(nullptr, m_testSrg->GetImageView(m_indexImageArray, 0));
        EXPECT_EQ(nullptr, m_testSrg->GetImageView(m_indexImageArray, 1));
        EXPECT_EQ(nullptr, m_testSrg->GetImageView(m_indexImageArray, 2));
    }

    TEST_F(ShaderResourceGroupImageTests, TestSetImageViewArrayAtOffset)
    {
        AZStd::vector<const RHI::ImageView*> twoImageViews = { m_imageViewA.get(), m_imageViewB.get() };

        // Test set operation, skipping the first element...

        EXPECT_TRUE(m_testSrg->SetImageViewArray(m_indexImageArray, twoImageViews, 1));
        m_testSrg->Compile();

        EXPECT_EQ(3, m_testSrg->GetImageViewArray(m_indexImageArray).size());
        EXPECT_EQ(nullptr, m_testSrg->GetImageViewArray(m_indexImageArray)[0]);
        EXPECT_EQ(twoImageViews[0], m_testSrg->GetImageViewArray(m_indexImageArray)[1]);
        EXPECT_EQ(twoImageViews[1], m_testSrg->GetImageViewArray(m_indexImageArray)[2]);
        EXPECT_EQ(nullptr, m_testSrg->GetImageView(m_indexImageArray, 0));
        EXPECT_EQ(twoImageViews[0], m_testSrg->GetImageView(m_indexImageArray, 1));
        EXPECT_EQ(twoImageViews[1], m_testSrg->GetImageView(m_indexImageArray, 2));
        EXPECT_EQ(nullptr, m_testSrg->GetImageView(m_indexImageArray, 0));
        EXPECT_EQ(twoImageViews[0], m_testSrg->GetImageView(m_indexImageArray, 1));
        EXPECT_EQ(twoImageViews[1], m_testSrg->GetImageView(m_indexImageArray, 2));
    }

    TEST_F(ShaderResourceGroupImageTests, TestSetImageViewArrayAtOffset_ValidationFailure)
    {
        // Make sure the no changes are made when a validation failure is detected

        AZStd::vector<const RHI::ImageView*> tooManyImageViews{ 3, m_imageViewA.get() };

        AZ_TEST_START_ASSERTTEST;
        EXPECT_FALSE(m_testSrg->SetImageViewArray(m_indexImageArray, tooManyImageViews, 1));
        AZ_TEST_STOP_ASSERTTEST(1);

        m_testSrg->Compile();
        EXPECT_EQ(nullptr, m_testSrg->GetImageView(m_indexImageArray, 0));
        EXPECT_EQ(nullptr, m_testSrg->GetImageView(m_indexImageArray, 1));
        EXPECT_EQ(nullptr, m_testSrg->GetImageView(m_indexImageArray, 2));
    }

    TEST_F(ShaderResourceGroupImageTests, TestCopyShaderResourceGroupDataImage)
    {
        EXPECT_TRUE(m_testSrg->SetImageArray(m_indexImageArray, m_threeImages));
        auto testSrg2 = ShaderResourceGroup::Create(m_testShaderAsset, AZ::RPI::DefaultSupervariantIndex, m_testSrgLayout->GetName());

        EXPECT_TRUE(testSrg2->CopyShaderResourceGroupData(*m_testSrg));
        EXPECT_EQ(3, testSrg2->GetImageArray(m_indexImageArray).size());
        EXPECT_EQ(m_testSrg->GetImageArray(m_indexImageArray)[0], testSrg2->GetImageArray(m_indexImageArray)[0]);
        EXPECT_EQ(m_testSrg->GetImageArray(m_indexImageArray)[1], testSrg2->GetImageArray(m_indexImageArray)[1]);
        EXPECT_EQ(m_testSrg->GetImageArray(m_indexImageArray)[2], testSrg2->GetImageArray(m_indexImageArray)[2]);
        EXPECT_EQ(m_testSrg->GetImageViewArray(m_indexImageArray)[0], testSrg2->GetImageViewArray(m_indexImageArray)[0]);
        EXPECT_EQ(m_testSrg->GetImageViewArray(m_indexImageArray)[1], testSrg2->GetImageViewArray(m_indexImageArray)[1]);
        EXPECT_EQ(m_testSrg->GetImageViewArray(m_indexImageArray)[2], testSrg2->GetImageViewArray(m_indexImageArray)[2]);
    }

    TEST_F(ShaderResourceGroupImageTests, TestCopyShaderResourceGroupDataImageView)
    {
        EXPECT_TRUE(m_testSrg->SetImageViewArray(m_indexImageArray, m_threeImageViews));
        auto testSrg2 = ShaderResourceGroup::Create(m_testShaderAsset, AZ::RPI::DefaultSupervariantIndex, m_testSrgLayout->GetName());

        EXPECT_TRUE(testSrg2->CopyShaderResourceGroupData(*m_testSrg));
        EXPECT_EQ(3, testSrg2->GetImageViewArray(m_indexImageArray).size());
        EXPECT_EQ(m_testSrg->GetImageArray(m_indexImageArray)[0], testSrg2->GetImageArray(m_indexImageArray)[0]);
        EXPECT_EQ(m_testSrg->GetImageArray(m_indexImageArray)[1], testSrg2->GetImageArray(m_indexImageArray)[1]);
        EXPECT_EQ(m_testSrg->GetImageArray(m_indexImageArray)[2], testSrg2->GetImageArray(m_indexImageArray)[2]);
        EXPECT_EQ(m_testSrg->GetImageViewArray(m_indexImageArray)[0], testSrg2->GetImageViewArray(m_indexImageArray)[0]);
        EXPECT_EQ(m_testSrg->GetImageViewArray(m_indexImageArray)[1], testSrg2->GetImageViewArray(m_indexImageArray)[1]);
        EXPECT_EQ(m_testSrg->GetImageViewArray(m_indexImageArray)[2], testSrg2->GetImageViewArray(m_indexImageArray)[2]);
    }


    TEST_F(ShaderResourceGroupImageTests, TestPartilCopyShaderResourceGroupData)
    {
        RHI::Ptr<RHI::ShaderResourceGroupLayout> srgLayout2 = RHI::ShaderResourceGroupLayout::Create();
        srgLayout2->SetName(Name("partial"));
        srgLayout2->SetBindingSlot(0);
        srgLayout2->AddShaderInput(RHI::ShaderInputImageDescriptor{
            Name{ "MyImageB" }, RHI::ShaderInputImageAccess::Read, RHI::ShaderInputImageType::Image2D, 1, 1, 1 });
        srgLayout2->AddShaderInput(RHI::ShaderInputImageDescriptor{
            Name{ "MyImageC" }, RHI::ShaderInputImageAccess::Read, RHI::ShaderInputImageType::Image2D, 1, 1, 1 });
        srgLayout2->Finalize();

        auto testSrgShaderAsset2 = CreateTestShaderAsset(Uuid::CreateRandom(), srgLayout2);
        auto testSrg2 = ShaderResourceGroup::Create(testSrgShaderAsset2, AZ::RPI::DefaultSupervariantIndex, srgLayout2->GetName());

        EXPECT_TRUE(m_testSrg->SetImage(m_indexImageA, m_whiteImage));
        EXPECT_TRUE(m_testSrg->SetImage(m_indexImageB, m_blackImage));

        EXPECT_FALSE(testSrg2->CopyShaderResourceGroupData(*m_testSrg));
        EXPECT_EQ(m_testSrg->GetImage(m_indexImageB), testSrg2->GetImage(RHI::ShaderInputImageIndex{ 0 }));
        EXPECT_EQ(m_testSrg->GetImageView(m_indexImageB), testSrg2->GetImageView(RHI::ShaderInputImageIndex{ 0 }));
    }
}
