/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/UnitTest/TestTypes.h>
#include <Atom/Utils/ImageComparison.h>

namespace UnitTest
{
    using namespace AZ::Utils;

    class ImageComparisonTests
        : public AllocatorsFixture
    {
    protected:
        static const AZ::RHI::Format DefaultFormat = AZ::RHI::Format::R8G8B8A8_UNORM;
        static constexpr size_t BytesPerPixel = 4;

        AZStd::vector<uint8_t> CreateTestRGBAImageData(AZ::RHI::Size size)
        {
            AZStd::vector<uint8_t> buffer;
            const size_t bufferSize = BytesPerPixel * size.m_width * size.m_height;
            buffer.reserve(bufferSize);
            for (uint32_t i = 0; i < bufferSize; ++i)
            {
                buffer.push_back(aznumeric_cast<uint8_t>(i % 255));
            }
            return buffer;
        }

        void SetPixel(AZStd::vector<uint8_t>& image, size_t pixelIndex, uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255)
        {
            image[pixelIndex * BytesPerPixel + 0] = r;
            image[pixelIndex * BytesPerPixel + 1] = g;
            image[pixelIndex * BytesPerPixel + 2] = b;
            image[pixelIndex * BytesPerPixel + 3] = a;
        };

    };

    TEST_F(ImageComparisonTests, Error_ImageSizesDontMatch)
    {
        AZ::RHI::Size sizeA{1, 1, 1};
        AZ::RHI::Size sizeB{1, 2, 1};

        auto result = CalcImageDiffRms(
            CreateTestRGBAImageData(sizeA), sizeA, DefaultFormat,
            CreateTestRGBAImageData(sizeB), sizeB, DefaultFormat);

        EXPECT_EQ(result.m_resultCode, ImageDiffResultCode::SizeMismatch);
    }

    TEST_F(ImageComparisonTests, Error_BufferSizeDoesntMatchImageSize)
    {
        AZ::RHI::Size size{1, 1, 1};
        AZ::RHI::Size wrongSize{1, 2, 1};

        auto result = CalcImageDiffRms(
            CreateTestRGBAImageData(size), wrongSize, DefaultFormat,
            CreateTestRGBAImageData(size), wrongSize, DefaultFormat);

        EXPECT_EQ(result.m_resultCode, ImageDiffResultCode::SizeMismatch);
    }
        
    TEST_F(ImageComparisonTests, Error_UnsupportedFormat)
    {
        AZ::RHI::Format format = AZ::RHI::Format::G8R8_G8B8_UNORM;
        AZ::RHI::Size size{1, 1, 1};

        auto result = CalcImageDiffRms(
            CreateTestRGBAImageData(size), size, format,
            CreateTestRGBAImageData(size), size, format);

        EXPECT_EQ(result.m_resultCode, ImageDiffResultCode::UnsupportedFormat);
    }

    TEST_F(ImageComparisonTests, Error_FormatsDontMatch)
    {
        AZ::RHI::Size size{1, 1, 1};

        auto result = CalcImageDiffRms(
            CreateTestRGBAImageData(size), size, AZ::RHI::Format::R8G8B8A8_SNORM,
            CreateTestRGBAImageData(size), size, AZ::RHI::Format::R8G8B8A8_UNORM);

        EXPECT_EQ(result.m_resultCode, ImageDiffResultCode::FormatMismatch);
    }

    TEST_F(ImageComparisonTests, CheckThreshold_SmallIdenticalImages)
    {
        AZ::RHI::Size size{16, 9, 1};

        auto result = CalcImageDiffRms(
            CreateTestRGBAImageData(size), size, DefaultFormat,
            CreateTestRGBAImageData(size), size, DefaultFormat);

        EXPECT_EQ(result.m_resultCode, ImageDiffResultCode::Success);

        EXPECT_EQ(0.0f, result.m_diffScore);
    }

    TEST_F(ImageComparisonTests, CheckThreshold_LargeIdenticalImages)
    {
        AZ::RHI::Size size{1620, 1080, 1};

        AZStd::vector<uint8_t> imageA = CreateTestRGBAImageData(size);
        AZStd::vector<uint8_t> imageB = imageA;

        auto result = CalcImageDiffRms(
            imageA, size, DefaultFormat,
            imageB, size, DefaultFormat);

        EXPECT_EQ(result.m_resultCode, ImageDiffResultCode::Success);

        EXPECT_EQ(0.0f, result.m_diffScore);
    }

    
    TEST_F(ImageComparisonTests, CheckMaxChannelDifference_R)
    {
        const AZStd::vector<uint8_t> imageA = { 255, 255, 255, 255 };
        const AZStd::vector<uint8_t> imageB = { 0, 125, 255, 255 };
        const int16_t maxChannelDiff = 255;
        const int16_t res = CalcMaxChannelDifference(imageA, imageB, 0);
        EXPECT_EQ(res, maxChannelDiff);
    }
    
    TEST_F(ImageComparisonTests, CheckMaxChannelDifference_G)
    {
        const AZStd::vector<uint8_t> imageA = { 255, 255, 255, 255 };
        const AZStd::vector<uint8_t> imageB = { 250, 125, 255, 255 };
        const int16_t maxChannelDiff = 130;
        const int16_t res = CalcMaxChannelDifference(imageA, imageB, 0);
        EXPECT_EQ(res, maxChannelDiff);
    }
    
    TEST_F(ImageComparisonTests, CheckMaxChannelDifference_B)
    {
        const AZStd::vector<uint8_t> imageA = { 255, 255, 255, 255 };
        const AZStd::vector<uint8_t> imageB = { 250, 125, 100, 255 };
        const int16_t maxChannelDiff = 155;
        const int16_t res = CalcMaxChannelDifference(imageA, imageB, 0);
        EXPECT_EQ(res, maxChannelDiff);
    }
    
    TEST_F(ImageComparisonTests, CheckMaxChannelDifference_A)
    {
        const AZStd::vector<uint8_t> imageA = { 0, 0, 0, 255 };
        const AZStd::vector<uint8_t> imageB = { 0, 1, 2, 0 };
        const int16_t maxChannelDiff = 255;
        const int16_t res = CalcMaxChannelDifference(imageA, imageB, 0);
        EXPECT_EQ(res, maxChannelDiff);
    }

    TEST_F(ImageComparisonTests, CheckThreshold_SmallImagesWithDifferences)
    {
        AZ::RHI::Size size{2, 2, 1};

        AZStd::vector<uint8_t> imageA = CreateTestRGBAImageData(size);
        AZStd::vector<uint8_t> imageB = CreateTestRGBAImageData(size);

        // Difference of 1 (R)
        SetPixel(imageA, 0, 100, 200, 5);
        SetPixel(imageB, 0, 101, 200, 5);

        // Difference of 2 (G)
        SetPixel(imageA, 1, 255, 255, 255);
        SetPixel(imageB, 1, 255, 253, 255);

        // Difference of 5 (B)
        SetPixel(imageA, 2, 0, 0, 0);
        SetPixel(imageB, 2, 0, 0, 5);

        // Difference of 100 (RGB all different)
        SetPixel(imageA, 3, 100, 100, 100);
        SetPixel(imageB, 3, 101, 102, 0);

        auto result = CalcImageDiffRms(
            imageA, size, DefaultFormat,
            imageB, size, DefaultFormat);

        EXPECT_EQ(result.m_resultCode, ImageDiffResultCode::Success);

        // Result should be:
        // sqrt( (1^2 + 2^2 + 5^2 + 100^2) / (255.0^2) / 4 )
        EXPECT_FLOAT_EQ(0.19637232876f, result.m_diffScore);
    }
    
    TEST_F(ImageComparisonTests, CheckThreshold_SmallImagesWithAlphaDifference)
    {
        AZ::RHI::Size size{2, 2, 1};

        AZStd::vector<uint8_t> imageA = CreateTestRGBAImageData(size);
        AZStd::vector<uint8_t> imageB = CreateTestRGBAImageData(size);

        // Difference of 1 (R)
        SetPixel(imageA, 0, 100, 200, 5);
        SetPixel(imageB, 0, 101, 200, 5);

        // Difference of 2 (G)
        SetPixel(imageA, 1, 255, 255, 255);
        SetPixel(imageB, 1, 255, 253, 255);

        // Difference of 5 (B)
        SetPixel(imageA, 2, 0, 0, 0);
        SetPixel(imageB, 2, 0, 0, 5);

        // Difference of 100 in the alpha channel
        SetPixel(imageA, 3, 0, 0, 0, 100);
        SetPixel(imageB, 3, 0, 0, 0, 0);

        auto result = CalcImageDiffRms(
            imageA, size, DefaultFormat,
            imageB, size, DefaultFormat);

        EXPECT_EQ(result.m_resultCode, ImageDiffResultCode::Success);

        // Result should be:
        // sqrt( (1^2 + 2^2 + 5^2 + 100^2) / (255.0^2) / 4 )
        EXPECT_FLOAT_EQ(0.19637232876f, result.m_diffScore);
    }

    TEST_F(ImageComparisonTests, CheckThreshold_IgnoreImperceptibleDifferences)
    {
        AZ::RHI::Size size{2, 2, 1};

        AZStd::vector<uint8_t> imageA = CreateTestRGBAImageData(size);
        AZStd::vector<uint8_t> imageB = CreateTestRGBAImageData(size);

        // Difference of 1 (R)
        SetPixel(imageA, 0, 100, 200, 5);
        SetPixel(imageB, 0, 101, 200, 5);

        // Difference of 2 (G)
        SetPixel(imageA, 1, 255, 255, 255);
        SetPixel(imageB, 1, 255, 253, 255);

        // Difference of 5 (B)
        SetPixel(imageA, 2, 0, 0, 0);
        SetPixel(imageB, 2, 0, 0, 5);

        // Difference of 4 (RGB all different)
        SetPixel(imageA, 3, 100, 100, 100);
        SetPixel(imageB, 3, 101, 102, 96);

        const float minDiffFilter = 3.9f / 255.0f;

        auto result = CalcImageDiffRms(
            imageA, size, DefaultFormat,
            imageB, size, DefaultFormat,
            minDiffFilter);

        EXPECT_EQ(result.m_resultCode, ImageDiffResultCode::Success);

        // Result should be:
        // sqrt( (1^2 + 2^2 + 5^2 + 4^2) / (255.0^2) / 4 )
        EXPECT_FLOAT_EQ(0.01329868624, result.m_diffScore);

        // Result should be:
        // sqrt( (5^2 + 4^2) / (255.0^2) / 4 )
        EXPECT_FLOAT_EQ(0.01255514556f, result.m_filteredDiffScore);
    }
}
