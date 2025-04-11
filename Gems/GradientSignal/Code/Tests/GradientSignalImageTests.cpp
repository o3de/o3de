/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <Tests/GradientSignalTestFixtures.h>
#include <Tests/GradientSignalTestHelpers.h>

#include <AzTest/AzTest.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Memory/PoolAllocator.h>
#include <AzCore/Math/MathUtils.h>
#include <AzCore/Math/Vector2.h>
#include <AzFramework/Asset/AssetCatalogBus.h>
#include <AzFramework/Components/TransformComponent.h>
#include <AZTestShared/Math/MathTestHelpers.h>

#include <GradientSignal/Components/ImageGradientComponent.h>
#include <GradientSignal/Components/GradientTransformComponent.h>

#include <LmbrCentral/Shape/BoxShapeComponentBus.h>

namespace UnitTest
{
    struct GradientSignalImageTestsFixture
        : public GradientSignalTest
    {
        struct PixelTestSetup
        {
            // How to create the source image
            AZ::u32 m_imageSize;            // size of the image to create.
            AZ::Vector2 m_pixel;            // location of the one pixel to set in the image.
            uint8_t m_setPixelValues[4];    // values to set the RGBA channels to for the one pixel that's set.

            // How to initialize the gradient components
            AZ::u32 m_shapeBoundsSize;      // size of the gradient bounding box in meters
            float   m_tiling;               // value to use for tilingX and tilingY setting
            GradientSignal::WrappingType m_wrappingType;    // wrapping type to use on the Gradient Transform

            // How to loop through and validate the results
            AZ::u32 m_validationSize;       // number of points in X and Y to loop through for querying the gradient
            float m_stepSize;               // step size for walking through X and Y in world space for the gradient query
            float m_expectedSetPixelGradientValue;  // the gradient value we expect to find for the pixel that's been set
            AZ::Vector2 m_expectedPixels[32];       // the list of expected locations that we expect to find a non-zero gradient for

            bool m_advancedMode = false;
            GradientSignal::ChannelToUse m_channelToUse = GradientSignal::ChannelToUse::Red;
            GradientSignal::CustomScaleType m_customScaleType = GradientSignal::CustomScaleType::None;
            float m_scaleRangeMin = 0.0f;
            float m_scaleRangeMax = 1.0f;
            GradientSignal::SamplingType m_samplingType = GradientSignal::SamplingType::Point;

            static const AZ::Vector2 EndOfList;
        };

        void TestPixels(GradientSignal::GradientSampler& sampler, AZ::u32 width, AZ::u32 height, float stepSize, float expectedValue, const AZStd::vector<AZ::Vector3>& expectedPoints)
        {
            AZStd::vector<AZ::Vector3> foundPoints;

            for (float y = 0.0f; y < static_cast<float>(height); y += stepSize)
            {
                for (float x = 0.0f; x < static_cast<float>(width); x += stepSize)
                {
                    float texelOffset = 0.0f; 
                    GradientSignal::GradientSampleParams params;
                    params.m_position = AZ::Vector3(x + texelOffset, y + texelOffset, 0.0f); 
                    float value = sampler.GetValue(params);
                    if (AZ::IsClose(value, expectedValue))
                    {
                        foundPoints.push_back(AZ::Vector3(x, y, 0.0f));
                    }
                    else
                    {
                        EXPECT_TRUE(value == 0.0f);
                    }
                }
            }

            EXPECT_EQ(expectedPoints.size(), foundPoints.size());
            if (expectedPoints.size() == foundPoints.size())
            {
                for (int point = 0; point < expectedPoints.size(); point++)
                {
                    EXPECT_EQ(static_cast<float>(expectedPoints[point].GetX()), static_cast<float>(foundPoints[point].GetX()));
                    EXPECT_EQ(static_cast<float>(expectedPoints[point].GetY()), static_cast<float>(foundPoints[point].GetY()));
                }
            }
        }

        AZStd::unique_ptr<AZ::Entity> CreateGradientConfigEntity(const PixelTestSetup& test)
        {
            // Create the base entity
            auto entity = CreateEntity();

            float shapeHalfBounds = test.m_shapeBoundsSize / 2.0f;

            // Create the Image Gradient Component.
            GradientSignal::ImageGradientConfig config;
            config.m_imageAsset = UnitTest::CreateSpecificPixelImageAsset(
                test.m_imageSize, test.m_imageSize,
                static_cast<AZ::u32>(test.m_pixel.GetX()), static_cast<AZ::u32>(test.m_pixel.GetY()), test.m_setPixelValues);
            config.m_tiling = AZ::Vector2(test.m_tiling);
            config.m_channelToUse = test.m_channelToUse;
            config.m_customScaleType = test.m_customScaleType;
            config.m_scaleRangeMin = test.m_scaleRangeMin;
            config.m_scaleRangeMax = test.m_scaleRangeMax;
            config.m_samplingType = test.m_samplingType;
            entity->CreateComponent<GradientSignal::ImageGradientComponent>(config);

            // Create the Gradient Transform Component.
            GradientSignal::GradientTransformConfig gradientTransformConfig;
            gradientTransformConfig.m_wrappingType = test.m_wrappingType;
            entity->CreateComponent<GradientSignal::GradientTransformComponent>(gradientTransformConfig);

            LmbrCentral::BoxShapeConfig boxConfig(AZ::Vector3(shapeHalfBounds * 2.0f));
            auto boxComponent = entity->CreateComponent(LmbrCentral::AxisAlignedBoxShapeComponentTypeId);
            boxComponent->SetConfiguration(boxConfig);

            // Create a transform that locates our gradient in the center of our desired mock Shape.
            auto transform = entity->CreateComponent<AzFramework::TransformComponent>();
            transform->SetWorldTM(AZ::Transform::CreateTranslation(AZ::Vector3(shapeHalfBounds)));

            // All components are created, so activate the entity
            ActivateEntity(entity.get());

            return entity;
        }

        void RunPixelTest(const PixelTestSetup& test)
        {
            // Create the gradient entity with the proper configuration
            auto entity = CreateGradientConfigEntity(test);

            // Build up a list of the locations that we expect to have non-zero values.
            AZStd::vector<AZ::Vector3> expectedPoints;
            for (const auto& expectedPoint : test.m_expectedPixels)
            {
                if (expectedPoint == PixelTestSetup::EndOfList)
                {
                    break;
                }

                expectedPoints.push_back(AZ::Vector3(expectedPoint.GetX(), expectedPoint.GetY(), 0.0f));
            }

            // Create a gradient sampler and run through a series of points to see if they match expectations.
            GradientSignal::GradientSampler gradientSampler;
            gradientSampler.m_gradientId = entity->GetId();
            TestPixels(
                gradientSampler, test.m_validationSize, test.m_validationSize, test.m_stepSize, test.m_expectedSetPixelGradientValue,
                expectedPoints);
        }
    };

    const AZ::Vector2 GradientSignalImageTestsFixture::PixelTestSetup::EndOfList = AZ::Vector2(-1.0f);

    TEST_F(GradientSignalImageTestsFixture, ImageGradientComponentSinglePixelLower)
    {
        // Set one pixel, map Gradient 1:1 to lookup space, get same pixel back
        PixelTestSetup test =
        {
            4, AZ::Vector2( 0, 0 ), {255, 255, 255, 255},                 // Source image:  4 x 4 with (0, 0) set to 0xFFFFFFFF
            4, 1.0f,                                                      // Mapped Shape:  4 x 4 with tiling (1.0, 1.0), unbounded
            GradientSignal::WrappingType::None,
            4, 1.0f, 1.0f,                                                // Validate that in 4 x 4 range, only 0, 0 is set to 1.0
            { AZ::Vector2(0, 0), PixelTestSetup::EndOfList }
        };

        RunPixelTest(test);
    }

    TEST_F(GradientSignalImageTestsFixture, ImageGradientComponentSinglePixelUpper)
    {
        // Set one pixel, map Gradient 1:1 to lookup space, get same pixel back
        PixelTestSetup test =
        {
            4, AZ::Vector2(3, 3), {255, 255, 255, 255},                   // Source image:  4 x 4 with (3, 3) set to 0xFFFFFFFF
            4, 1.0f,                                                      // Mapped Shape:  4 x 4 with tiling (1.0, 1.0), unbounded
            GradientSignal::WrappingType::None,
            4, 1.0f, 1.0f,                                                // Validate that in 4 x 4 range, only 3, 3 is set to 1.0
            { AZ::Vector2(3, 3), PixelTestSetup::EndOfList }
        };

        RunPixelTest(test);
    }

    TEST_F(GradientSignalImageTestsFixture, ImageGradientComponentSinglePixelUnbounded)
    {
        // Validate that our image repeats correctly when using "unbounded"
        PixelTestSetup test =
        {
            4, AZ::Vector2( 0, 0 ), {255, 255, 255, 255},                 // Source image:  4 x 4 with (0, 0) set to 0xFFFFFFFF
            4, 1.0f,                                                      // Mapped Shape:  4 x 4 with tiling (1.0, 1.0), unbounded
            GradientSignal::WrappingType::None,
            8, 1.0f, 1.0f,                                                // Validate that in 8 x 8 range, the pixel repeats every 4 pixels
            { AZ::Vector2(0, 0), AZ::Vector2(4, 0), AZ::Vector2(0, 4), AZ::Vector2(4, 4), PixelTestSetup::EndOfList }
        };

        RunPixelTest(test);
    }

    TEST_F(GradientSignalImageTestsFixture, ImageGradientComponentSinglePixelClampToZero)
    {
        // Validate that our image does *not* repeat when using "Clamp to Zero"
        PixelTestSetup test =
        {
            4, AZ::Vector2(0, 0), {255, 255, 255, 255},                   // Source image:  4 x 4 with (0, 0) set to 0xFFFFFFFF
            4, 1.0f,                                                      // Mapped Shape:  4 x 4 with tiling (1.0, 1.0), unbounded
            GradientSignal::WrappingType::ClampToZero,
            8, 1.0f, 1.0f,                                                // Validate that in 8 x 8 range, the pixel does *not* repeat
            { AZ::Vector2(0, 0), PixelTestSetup::EndOfList }
        };

        RunPixelTest(test);
    }

    TEST_F(GradientSignalImageTestsFixture, ImageGradientComponentSinglePixelClampToEdge)
    {
        // Validate that our image stretches the edge correctly when using "Clamp to Edge"
        PixelTestSetup test =
        {
            4, AZ::Vector2(3, 3), {255, 255, 255, 255},                   // Source image:  4 x 4 with (3, 3) set to 0xFFFFFFFF
            4, 1.0f,                                                      // Mapped Shape:  4 x 4 with tiling (1.0, 1.0), unbounded
            GradientSignal::WrappingType::ClampToEdge,
            8, 1.0f, 1.0f,                                                // Validate that in 8 x 8 range, a corner pixel "stretches" to everything right and down from it
            { AZ::Vector2(3, 3), AZ::Vector2(4, 3), AZ::Vector2(5, 3), AZ::Vector2(6, 3), AZ::Vector2(7, 3),
              AZ::Vector2(3, 4), AZ::Vector2(4, 4), AZ::Vector2(5, 4), AZ::Vector2(6, 4), AZ::Vector2(7, 4),
              AZ::Vector2(3, 5), AZ::Vector2(4, 5), AZ::Vector2(5, 5), AZ::Vector2(6, 5), AZ::Vector2(7, 5),
              AZ::Vector2(3, 6), AZ::Vector2(4, 6), AZ::Vector2(5, 6), AZ::Vector2(6, 6), AZ::Vector2(7, 6),
              AZ::Vector2(3, 7), AZ::Vector2(4, 7), AZ::Vector2(5, 7), AZ::Vector2(6, 7), AZ::Vector2(7, 7),
              PixelTestSetup::EndOfList }
        };

        RunPixelTest(test);
    }

    TEST_F(GradientSignalImageTestsFixture, ImageGradientComponentSinglePixelRepeat)
    {
        // Validate that our image repeats correctly when using "Repeat"
        PixelTestSetup test =
        {
            4, AZ::Vector2(0, 0), {255, 255, 255, 255},                   // Source image:  4 x 4 with (0, 0) set to 0xFFFFFFFF
            4, 1.0f,                                                      // Mapped Shape:  4 x 4 with tiling (1.0, 1.0), unbounded
            GradientSignal::WrappingType::Repeat,
            8, 1.0f, 1.0f,                                                // Validate that in 8 x 8 range, the pixel repeats every 4 pixels
            { AZ::Vector2(0, 0), AZ::Vector2(4, 0), AZ::Vector2(0, 4), AZ::Vector2(4, 4),
              PixelTestSetup::EndOfList }
        };

        RunPixelTest(test);
    }

    TEST_F(GradientSignalImageTestsFixture, ImageGradientComponentSinglePixelMirror)
    {
        // Validate that our image repeats correctly when using "Mirror"
        PixelTestSetup test =
        {
            4, AZ::Vector2(0, 0), {255, 255, 255, 255},                   // Source image:  4 x 4 with (0, 0) set to 0xFFFFFFFF
            4, 1.0f,                                                      // Mapped Shape:  4 x 4 with tiling (1.0, 1.0), unbounded
            GradientSignal::WrappingType::Mirror,
            16, 1.0f, 1.0f,                                               // Validate that in 16 x 16 range, we get a mirrored repeat
            { AZ::Vector2(0, 0), AZ::Vector2(7, 0), AZ::Vector2(8, 0), AZ::Vector2(15, 0),
              AZ::Vector2(0, 7), AZ::Vector2(7, 7), AZ::Vector2(8, 7), AZ::Vector2(15, 7),
              AZ::Vector2(0, 8), AZ::Vector2(7, 8), AZ::Vector2(8, 8), AZ::Vector2(15, 8),
              AZ::Vector2(0,15), AZ::Vector2(7,15), AZ::Vector2(8,15), AZ::Vector2(15,15),
              PixelTestSetup::EndOfList }
        };

        RunPixelTest(test);
    }

    TEST_F(GradientSignalImageTestsFixture, ImageGradientComponentSinglePixelTilingUnbounded)
    {
        // Validate that our image repeats correctly when using "Unbounded" with a tiling factor.
        // Because we advance by 3/4 pixel, we expect to read values from pixels 0, 0, 1, 2, 3, 4 (0), 5 (1).
        // So we expect sample pixels 0, 1, and 6 to have values.
        PixelTestSetup test =
        {
            4, AZ::Vector2(0, 0), {255, 255, 255, 255},                   // Source image:  4 x 4 with (0, 0) set to 0xFFFFFFFF
            4, 0.75f,                                                     // Mapped Shape:  4 x 4 with tiling (0.75, 0.75), unbounded
            GradientSignal::WrappingType::None,
            8, 1.0f, 1.0f,                                                // Validate that in 8 x 8 range, unbounded tiling works
            { AZ::Vector2(0, 0), AZ::Vector2(1, 0), AZ::Vector2(6, 0),
              AZ::Vector2(0, 1), AZ::Vector2(1, 1), AZ::Vector2(6, 1),
              AZ::Vector2(0, 6), AZ::Vector2(1, 6), AZ::Vector2(6, 6),
              PixelTestSetup::EndOfList }
        };

        RunPixelTest(test);
    }

    TEST_F(GradientSignalImageTestsFixture, ImageGradientComponentSinglePixelTilingRepeat)
    {
        // Validate that our image repeats correctly when using "Repeat" with a tiling factor.
        // Because we advance by 3/4 pixel, but repeat our UVs after 4 pixels, we expect to read values from pixels 0, 0, 1, 2, 0, 0, 1, 2
        // So we expect sample pixels 0, 1, 4, and 5 to have values.
        PixelTestSetup test =
        {
            4, AZ::Vector2(0, 0), {255, 255, 255, 255},                   // Source image:  4 x 4 with (0, 0) set to 0xFFFFFFFF
            4, 0.75f,                                                     // Mapped Shape:  4 x 4 with tiling (0.75, 0.75), repeating
            GradientSignal::WrappingType::Repeat,
            8, 1.0f, 1.0f,                                                // Validate that in 8 x 8 range, repeat tiling works
            { AZ::Vector2(0, 0), AZ::Vector2(1, 0), AZ::Vector2(4, 0), AZ::Vector2(5, 0),
              AZ::Vector2(0, 1), AZ::Vector2(1, 1), AZ::Vector2(4, 1), AZ::Vector2(5, 1),
              AZ::Vector2(0, 4), AZ::Vector2(1, 4), AZ::Vector2(4, 4), AZ::Vector2(5, 4),
              AZ::Vector2(0, 5), AZ::Vector2(1, 5), AZ::Vector2(4, 5), AZ::Vector2(5, 5),
              PixelTestSetup::EndOfList }
        };

        RunPixelTest(test);
    }

    TEST_F(GradientSignalImageTestsFixture, ImageGradientComponentSinglePixelUnboundedScaled)
    {
        // Validate that our image is sampled correctly when scaling our sampling area
        PixelTestSetup test =
        {
            4, AZ::Vector2(0, 0), {255, 255, 255, 255},                   // Source image:  4 x 4 with (0, 0) set to 0xFFFFFFFF
            4, 1.0f,                                                      // Mapped Shape:  4 x 4 with tiling (1.0, 1.0), unbounded
            GradientSignal::WrappingType::None,
            4, 0.5f, 1.0f,                                                // Validate that in 4 x 4 range sampled with 8 x 8 pixels, our 1 pixel turns into 4 pixels
            { AZ::Vector2(0, 0), AZ::Vector2(0.5f, 0), 
              AZ::Vector2(0, 0.5f), AZ::Vector2(0.5f, 0.5f),
              PixelTestSetup::EndOfList }
        };

        RunPixelTest(test);
    }

    TEST_F(GradientSignalImageTestsFixture, ImageGradientComponentAdvancedChannelR)
    {
        // Set one pixel, map Gradient 1:1 to lookup space, get same pixel back
        PixelTestSetup test =
        {
            4, AZ::Vector2(0, 0), {200, 150, 100, 50},                  // Source image:  4 x 4 with (0, 0) set to different values in each channel
            4, 1.0f,                                                    // Mapped Shape:  4 x 4 with tiling (1.0, 1.0), unbounded
            GradientSignal::WrappingType::None,
            4, 1.0f, 200.0f/255.0f,                                     // Validate that in 4 x 4 range, only 0, 0 is set to 200/255 (red channel)
            { AZ::Vector2(0, 0), PixelTestSetup::EndOfList },
            true,                                                       // Enabled the advanced mode
            GradientSignal::ChannelToUse::Red                           // Use default Red channel
        };

        RunPixelTest(test);
    }

    TEST_F(GradientSignalImageTestsFixture, ImageGradientComponentAdvancedChannelG)
    {
        // Set one pixel, map Gradient 1:1 to lookup space, get same pixel back
        PixelTestSetup test =
        {
            4, AZ::Vector2(0, 0), {200, 150, 100, 50},                  // Source image:  4 x 4 with (0, 0) set to different values in each channel
            4, 1.0f,                                                    // Mapped Shape:  4 x 4 with tiling (1.0, 1.0), unbounded
            GradientSignal::WrappingType::None,
            4, 1.0f, 150.0f/255.0f,                                     // Validate that in 4 x 4 range, only 0, 0 is set to 150/255 (green channel)
            { AZ::Vector2(0, 0), PixelTestSetup::EndOfList },
            true,                                                       // Enabled the advanced mode
            GradientSignal::ChannelToUse::Green                         // Use Green channel
        };

        RunPixelTest(test);
    }

    TEST_F(GradientSignalImageTestsFixture, ImageGradientComponentAdvancedChannelB)
    {
        // Set one pixel, map Gradient 1:1 to lookup space, get same pixel back
        PixelTestSetup test =
        {
            4, AZ::Vector2(0, 0), {200, 150, 100, 50},                  // Source image:  4 x 4 with (0, 0) set to different values in each channel
            4, 1.0f,                                                    // Mapped Shape:  4 x 4 with tiling (1.0, 1.0), unbounded
            GradientSignal::WrappingType::None,
            4, 1.0f, 100.0f/255.0f,                                     // Validate that in 4 x 4 range, only 0, 0 is set to 100/255 (blue channel)
            { AZ::Vector2(0, 0), PixelTestSetup::EndOfList },
            true,                                                       // Enabled the advanced mode
            GradientSignal::ChannelToUse::Blue                          // Use Blue channel
        };

        RunPixelTest(test);
    }

    TEST_F(GradientSignalImageTestsFixture, ImageGradientComponentAdvancedChannelA)
    {
        // Set one pixel, map Gradient 1:1 to lookup space, get same pixel back
        PixelTestSetup test =
        {
            4, AZ::Vector2(0, 0), {200, 150, 100, 50},                  // Source image:  4 x 4 with (0, 0) set to different values in each channel
            4, 1.0f,                                                    // Mapped Shape:  4 x 4 with tiling (1.0, 1.0), unbounded
            GradientSignal::WrappingType::None,
            4, 1.0f, 50.0f/255.0f,                                      // Validate that in 4 x 4 range, only 0, 0 is set to 50/255 (alpha channel)
            { AZ::Vector2(0, 0), PixelTestSetup::EndOfList },
            true,                                                       // Enabled the advanced mode
            GradientSignal::ChannelToUse::Alpha                         // Use Alpha channel
        };

        RunPixelTest(test);
    }

    TEST_F(GradientSignalImageTestsFixture, ImageGradientComponentAdvancedTerrarium)
    {
        // Set one pixel, map Gradient 1:1 to lookup space, get same pixel back
        PixelTestSetup test =
        {
            4, AZ::Vector2(0, 0), {200, 150, 100, 50},                  // Source image:  4 x 4 with (0, 0) set to different values in each channel
            4, 1.0f,                                                    // Mapped Shape:  4 x 4 with tiling (1.0, 1.0), unbounded
            GradientSignal::WrappingType::None,
            4, 1.0f, 1.0f,                                              // Validate that in 4 x 4 range, only 0, 0 is set
            { AZ::Vector2(0, 0), PixelTestSetup::EndOfList },
            true,                                                       // Enabled the advanced mode
            GradientSignal::ChannelToUse::Terrarium                     // Use Terrarium format
        };

        // The expected value is based on Terrarium file format equation:
        // (red * 256 + green + blue / 256) - 32768
        // More information can be found here:  https://www.mapzen.com/blog/terrain-tile-service/
        // an RGB of (200, 150, 100) produces a Terrarium world height of (200*256 + 150 + 100/256) - 32768 = 18582.390625
        // However, the final gradient value is expected to be 0-1, so the terrarium value range of [-32768, 32768) is mapped by
        // adding 32768 and dividing by 65536.
        float terrariumBaseHeight =
            (test.m_setPixelValues[0] * 256.0f) + test.m_setPixelValues[1] + (test.m_setPixelValues[2] / 256.0f) - 32768.0f;
        test.m_expectedSetPixelGradientValue = (terrariumBaseHeight + 32768.0f) / 65536.0f;

        RunPixelTest(test);
    }

    TEST_F(GradientSignalImageTestsFixture, ImageGradientComponentAdvancedManualScale)
    {
        // Set one pixel, map Gradient 1:1 to lookup space, get same pixel back
        const float customMin = 0.0f;
        const float customMax = 0.5f;
        PixelTestSetup test =
        {
            4, AZ::Vector2(0, 0), {32, 64, 16, 0},                      // Source image:  4 x 4 with (0, 0) set to different values in each channel
            4, 1.0f,                                                    // Mapped Shape:  4 x 4 with tiling (1.0, 1.0), unbounded
            GradientSignal::WrappingType::None,
            4, 1.0f, 1.0f,                                              // Validate that in 4 x 4 range, only 0, 0 is set
            { AZ::Vector2(0, 0), PixelTestSetup::EndOfList },
            true,                                                       // Enabled the advanced mode
            GradientSignal::ChannelToUse::Red,
            GradientSignal::CustomScaleType::Manual,                    // Enable manual scale
            customMin,                                                  // Custom min
            customMax                                                   // Custom max
        };

        // This test uses the red channel, so we expect the output to be our red value inverse lerped between customMin and customMax.
        // Since red is 32/255 (~1/8) and our scale range is 0 - 1/2, the expected value should be ~1/4.
        test.m_expectedSetPixelGradientValue = AZ::LerpInverse(customMin, customMax, test.m_setPixelValues[0] / 255.0f);

        RunPixelTest(test);
    }

    TEST_F(GradientSignalImageTestsFixture, ImageGradientComponentAdvancedAutoScale)
    {
        // Set one pixel, map Gradient 1:1 to lookup space, get same pixel back
        PixelTestSetup test =
        {
            4, AZ::Vector2(0, 0), {200, 150, 100, 50},                  // Source image:  4 x 4 with (0, 0) set to different values in each channel
            4, 1.0f,                                                    // Mapped Shape:  4 x 4 with tiling (1.0, 1.0), unbounded
            GradientSignal::WrappingType::None,
            4, 1.0f, 1.0f,                                              // Validate that in 4 x 4 range, only 0, 0 is set
            { AZ::Vector2(0, 0), PixelTestSetup::EndOfList },
            true,                                                       // Enabled the advanced mode
            GradientSignal::ChannelToUse::Green,                        // Use Green channel
            GradientSignal::CustomScaleType::Auto                       // Enable Auto scale
        };

        // Since all of our pixels are set to 0, except one which is set to our specified value, our specified value should get
        // auto-scaled to 1.0 since it's the highest value in the image.
        test.m_expectedSetPixelGradientValue = 1.0f;

        RunPixelTest(test);
    }

    TEST_F(GradientSignalImageTestsFixture, ImageGradientComponentAdvancedBilinearSamplingClampToZero)
    {
        // Set one pixel, map Gradient 1:1 to lookup space, get same pixel back
        PixelTestSetup test =
        {
            4, AZ::Vector2(0.25f, 0.25f), {200, 150, 100, 50},          // Source image:  4 x 4 with (0, 0) set to different values in each channel
            4, 1.0f,                                                    // Mapped Shape:  4 x 4 with tiling (1.0, 1.0), unbounded
            GradientSignal::WrappingType::ClampToZero,
            4, 0.25f, 1.0f,                                             // Validate that in 4 x 4 range, only 0, 0 is set
            { AZ::Vector2(0.25f, 0.25f), PixelTestSetup::EndOfList },
            true,                                                       // Enabled the advanced mode
            GradientSignal::ChannelToUse::Red,                          // Use Red channel (default)
            GradientSignal::CustomScaleType::None,                      // No custom scale (default)
            0.0f,                                                       // Min scale (won't be used since None scale type is set)
            1.0f,                                                       // Max scale (won't be used since None scale type is set)
            GradientSignal::SamplingType::Bilinear                      // Enable the bilinear sampling type
        };

        // Create the gradient entity with the proper configuration
        auto entity = CreateGradientConfigEntity(test);

        // Create a gradient sampler and run through a series of points to see if they match expectations.
        GradientSignal::GradientSampler gradientSampler;
        gradientSampler.m_gradientId = entity->GetId();

        // Iterate over the points with a 0.25 step size so that we can test
        // fractional values to verify the interpolation is working
        for (float y = 0.0f; y < static_cast<float>(test.m_validationSize); y += test.m_stepSize)
        {
            for (float x = 0.0f; x < static_cast<float>(test.m_validationSize); x += test.m_stepSize)
            {
                GradientSignal::GradientSampleParams params;
                params.m_position = AZ::Vector3(x, y, 0.0f);
                float value = gradientSampler.GetValue(params);

                // Only the 0,0 point has a non-zero value, so anything out of that bounds
                // will be 0
                if (floor(x) > 0 || floor(y) > 0)
                {
                    EXPECT_TRUE(value == 0.0f);
                }
                // Otherwise, figure out the expected interpolated value
                else
                {
                    // Compute the delta X/Y for sampling pixel
                    float deltaX = x - floor(x);
                    float deltaY = y - floor(y);

                    // Only the 0,0 pixel is set to a specific value, so the other
                    // points in the grid we will lookup will all be 0
                    const float valueX0Y0 = test.m_setPixelValues[0] / 255.0f;
                    const float valueX1Y0 = 0.0f;
                    const float valueX0Y1 = 0.0f;
                    const float valueX1Y1 = 0.0f;
                    const float valueXY0 = AZ::Lerp(valueX0Y0, valueX1Y0, deltaX);
                    const float valueXY1 = AZ::Lerp(valueX0Y1, valueX1Y1, deltaX);
                    const float expectedValue = AZ::Lerp(valueXY0, valueXY1, deltaY);

                    EXPECT_TRUE(AZ::IsClose(value, expectedValue));
                }
            }
        }
    }

    TEST_F(GradientSignalImageTestsFixture, ImageGradientComponentAdvancedBilinearSamplingClampToEdge)
    {
        // Set one pixel, map Gradient 1:1 to lookup space, get same pixel back
        PixelTestSetup test =
        {
            4, AZ::Vector2(0.25f, 0.25f), {200, 150, 100, 50},          // Source image:  4 x 4 with (0, 0) set to different values in each channel
            4, 1.0f,                                                    // Mapped Shape:  4 x 4 with tiling (1.0, 1.0), unbounded
            GradientSignal::WrappingType::ClampToEdge,
            4, 0.25f, 1.0f,                                             // Validate that in 4 x 4 range, only 0, 0 is set
            { AZ::Vector2(0.25f, 0.25f), PixelTestSetup::EndOfList },
            true,                                                       // Enabled the advanced mode
            GradientSignal::ChannelToUse::Red,                          // Use Red channel (default)
            GradientSignal::CustomScaleType::None,                      // No custom scale (default)
            0.0f,                                                       // Min scale (won't be used since None scale type is set)
            1.0f,                                                       // Max scale (won't be used since None scale type is set)
            GradientSignal::SamplingType::Bilinear                      // Enable the bilinear sampling type
        };

        // Create the gradient entity with the proper configuration
        auto entity = CreateGradientConfigEntity(test);

        // Create a gradient sampler and run through a series of points to see if they match expectations.
        GradientSignal::GradientSampler gradientSampler;
        gradientSampler.m_gradientId = entity->GetId();

        // Iterate over the points with a 0.25 step size so that we can test
        // fractional values to verify the interpolation is working
        for (float y = 0.0f; y < static_cast<float>(test.m_validationSize); y += test.m_stepSize)
        {
            for (float x = 0.0f; x < static_cast<float>(test.m_validationSize); x += test.m_stepSize)
            {
                GradientSignal::GradientSampleParams params;
                params.m_position = AZ::Vector3(x, y, 0.0f);
                float value = gradientSampler.GetValue(params);

                // Only the 0,0 point has a non-zero value, so anything out of that bounds
                // will be 0
                if (floor(x) > 0 || floor(y) > 0)
                {
                    EXPECT_TRUE(value == 0.0f);
                }
                // Otherwise, figure out the expected interpolated value
                else
                {
                    // Compute the delta X/Y for sampling pixel
                    float deltaX = x - floor(x);
                    float deltaY = y - floor(y);

                    // Only the 0,0 pixel is set to a specific value, so the other
                    // points in the grid we will lookup will all be 0
                    const float valueX0Y0 = test.m_setPixelValues[0] / 255.0f;
                    const float valueX1Y0 = 0.0f;
                    const float valueX0Y1 = 0.0f;
                    const float valueX1Y1 = 0.0f;
                    const float valueXY0 = AZ::Lerp(valueX0Y0, valueX1Y0, deltaX);
                    const float valueXY1 = AZ::Lerp(valueX0Y1, valueX1Y1, deltaX);
                    const float expectedValue = AZ::Lerp(valueXY0, valueXY1, deltaY);

                    EXPECT_TRUE(AZ::IsClose(value, expectedValue));
                }
            }
        }
    }

    TEST_F(GradientSignalImageTestsFixture, ImageGradientComponentAdvancedBilinearSamplingMirror)
    {
        // Set one pixel, map Gradient 1:1 to lookup space, get same pixel back
        PixelTestSetup test =
        {
            4, AZ::Vector2(0.25f, 0.25f), {200, 150, 100, 50},          // Source image:  4 x 4 with (0, 0) set to different values in each channel
            4, 1.0f,                                                    // Mapped Shape:  4 x 4 with tiling (1.0, 1.0), unbounded
            GradientSignal::WrappingType::Mirror,
            4, 0.25f, 1.0f,                                             // Validate that in 4 x 4 range, only 0, 0 is set
            { AZ::Vector2(0.25f, 0.25f), PixelTestSetup::EndOfList },
            true,                                                       // Enabled the advanced mode
            GradientSignal::ChannelToUse::Red,                          // Use Red channel (default)
            GradientSignal::CustomScaleType::None,                      // No custom scale (default)
            0.0f,                                                       // Min scale (won't be used since None scale type is set)
            1.0f,                                                       // Max scale (won't be used since None scale type is set)
            GradientSignal::SamplingType::Bilinear                      // Enable the bilinear sampling type
        };

        // Create the gradient entity with the proper configuration
        auto entity = CreateGradientConfigEntity(test);

        // Create a gradient sampler and run through a series of points to see if they match expectations.
        GradientSignal::GradientSampler gradientSampler;
        gradientSampler.m_gradientId = entity->GetId();

        // Iterate over the points with a 0.25 step size so that we can test
        // fractional values to verify the interpolation is working
        for (float y = 0.0f; y < static_cast<float>(test.m_validationSize); y += test.m_stepSize)
        {
            for (float x = 0.0f; x < static_cast<float>(test.m_validationSize); x += test.m_stepSize)
            {
                GradientSignal::GradientSampleParams params;
                params.m_position = AZ::Vector3(x, y, 0.0f);
                float value = gradientSampler.GetValue(params);

                // Only the 0,0 point has a non-zero value, so anything out of that bounds
                // will be 0
                if (floor(x) > 0 || floor(y) > 0)
                {
                    EXPECT_TRUE(value == 0.0f);
                }
                // Otherwise, figure out the expected interpolated value
                else
                {
                    // Compute the delta X/Y for sampling pixel
                    float deltaX = x - floor(x);
                    float deltaY = y - floor(y);

                    // Only the 0,0 pixel is set to a specific value, so the other
                    // points in the grid we will lookup will all be 0
                    const float valueX0Y0 = test.m_setPixelValues[0] / 255.0f;
                    const float valueX1Y0 = 0.0f;
                    const float valueX0Y1 = 0.0f;
                    const float valueX1Y1 = 0.0f;
                    const float valueXY0 = AZ::Lerp(valueX0Y0, valueX1Y0, deltaX);
                    const float valueXY1 = AZ::Lerp(valueX0Y1, valueX1Y1, deltaX);
                    const float expectedValue = AZ::Lerp(valueXY0, valueXY1, deltaY);

                    EXPECT_TRUE(AZ::IsClose(value, expectedValue));
                }
            }
        }
    }

    TEST_F(GradientSignalImageTestsFixture, ImageGradientComponentAdvancedBilinearSamplingRepeat)
    {
        // Set one pixel, map Gradient 1:1 to lookup space, get same pixel back
        PixelTestSetup test =
        {
            4, AZ::Vector2(0.25f, 0.25f), {200, 150, 100, 50},          // Source image:  4 x 4 with (0, 0) set to different values in each channel
            4, 1.0f,                                                    // Mapped Shape:  4 x 4 with tiling (1.0, 1.0), unbounded
            GradientSignal::WrappingType::Repeat,
            4, 0.25f, 1.0f,                                             // Validate that in 4 x 4 range, only 0, 0 is set
            { AZ::Vector2(0.25f, 0.25f), PixelTestSetup::EndOfList },
            true,                                                       // Enabled the advanced mode
            GradientSignal::ChannelToUse::Red,                          // Use Red channel (default)
            GradientSignal::CustomScaleType::None,                      // No custom scale (default)
            0.0f,                                                       // Min scale (won't be used since None scale type is set)
            1.0f,                                                       // Max scale (won't be used since None scale type is set)
            GradientSignal::SamplingType::Bilinear                      // Enable the bilinear sampling type
        };

        // Create the gradient entity with the proper configuration
        auto entity = CreateGradientConfigEntity(test);

        // Create a gradient sampler and run through a series of points to see if they match expectations.
        GradientSignal::GradientSampler gradientSampler;
        gradientSampler.m_gradientId = entity->GetId();

        // Helper method to return the pixel value given x/y coordinate
        // This helper method will handle the repeating logic for us
        auto getPixelValue = [test](AZ::u32 pixelX, AZ::u32 pixelY) {
            pixelX = pixelX % test.m_validationSize;
            pixelY = pixelY % test.m_validationSize;
            if (pixelX == 0 && pixelY == 0)
            {
                return test.m_setPixelValues[0] / 255.0f;
            }
            else
            {
                return 0.0f;
            }
        };

        // Iterate over the points with a 0.25 step size so that we can test
        // fractional values to verify the interpolation is working
        for (float y = 0.0f; y < static_cast<float>(test.m_validationSize); y += test.m_stepSize)
        {
            for (float x = 0.0f; x < static_cast<float>(test.m_validationSize); x += test.m_stepSize)
            {
                GradientSignal::GradientSampleParams params;
                params.m_position = AZ::Vector3(x, y, 0.0f);
                float value = gradientSampler.GetValue(params);

                // Compute the delta X/Y for sampling pixel
                float deltaX = x - floor(x);
                float deltaY = y - floor(y);

                // Any x/y pixel value that ends up being 0,0 based on the
                // Repeat wrapping type has a specific pixel value.
                // All other pixel values are 0.0f, so we use the getPixelValue
                // helper method to handle getting the proper pixel value for us.
                AZ::u32 pixelX = aznumeric_cast<AZ::u32>(floor(x));
                AZ::u32 pixelY = aznumeric_cast<AZ::u32>(floor(y));
                const float valueX0Y0 = getPixelValue(pixelX, pixelY);
                const float valueX1Y0 = getPixelValue(pixelX + 1, pixelY);;
                const float valueX0Y1 = getPixelValue(pixelX, pixelY + 1);
                const float valueX1Y1 = getPixelValue(pixelX + 1, pixelY + 1);
                const float valueXY0 = AZ::Lerp(valueX0Y0, valueX1Y0, deltaX);
                const float valueXY1 = AZ::Lerp(valueX0Y1, valueX1Y1, deltaX);
                const float expectedValue = AZ::Lerp(valueXY0, valueXY1, deltaY);

                EXPECT_TRUE(AZ::IsClose(value, expectedValue));
            }
        }
    }

    TEST_F(GradientSignalImageTestsFixture, GradientTransformComponent_TransformTypes)
    {
        // Verify that each transform type for the transform component works correctly.

        // The setup on this test is rather complex, but the concept is fairly simple.  The idea is that we create an
        // ImageGradient with a single specific pixel set, and then set our transforms in a way that we can verify that
        // the correct transform is used an applied to move the pixel to the place we expect in the sampled output.

        // In specific, we create a 3x3 image with the center pixel set.  We map it to a 2x2 box, since that will cause 3x3
        // samples to be sampled (shapes are inclusive on both sides).  This gives us a 1:1 mapping to sample.  By default,
        // the box centered at (0, 0) means that the one pixel at (0, 0) is set.  
        // In our tests, we change only the transform(s) that we expect to get used to translate the pixel to (2, 2), and
        // validate that only (2, 2) is set in our output.

        struct TransformTypeTest
        {
            GradientSignal::TransformType transformType;    // The type of transform to test
            float entityWorldTM;                            // Set the entity's World translation to (x, x, x)
            float entityLocalTM;                            // Set the entity's Local translation to (x, x, x)
            float shapeWorldTM;                             // Set the shape entity's World translation to (x, x, x)
            float shapeLocalTM;                             // Set the shape entity's Local translation to (x, x, x)
            int expectedPixelLocation;                      // The one pixel we expect to be set in the output is (x, x)
        };

        const TransformTypeTest transformTypeTests[] =
        {
            // For our basic transform tests, if we set the correct transform's translation, that should directly map to which output
            // pixel is set.
            { GradientSignal::TransformType::World_ThisEntity,      2.0f, 1.0f, 1.0f, 1.0f, 2 },
            { GradientSignal::TransformType::Local_ThisEntity,      1.0f, 2.0f, 1.0f, 1.0f, 2 },
            { GradientSignal::TransformType::World_ReferenceEntity, 1.0f, 1.0f, 2.0f, 1.0f, 2 },
            { GradientSignal::TransformType::Local_ReferenceEntity, 1.0f, 1.0f, 1.0f, 2.0f, 2 },

            // No matter what the other transforms are set to, when using origin we expected our image to be centered at 0, so
            // it should be the pixel at (0, 0) that's set, no matter what our transforms are set to.
            { GradientSignal::TransformType::World_Origin,          1.0f, 2.0f, 4.0f, 7.0f, 0 },

            // Since this is "relative to reference", if we put our reference at 3 and our entity at 5, the relative value
            // should be 2.
            { GradientSignal::TransformType::Relative,              5.0f, 0.0f, 3.0f, 0.0f, 2 },
        };

        for (auto test : transformTypeTests)
        {
            constexpr int dataSize = 8;

            // Set our expected output to 0 except for the one pixel we're expecting to find.
            AZStd::vector<float> expectedOutput(dataSize * dataSize, 0.0f);
            expectedOutput[(test.expectedPixelLocation * dataSize) + test.expectedPixelLocation] = 1.0f;

            // Create a reference shape entity.
            auto mockShape = CreateEntity();

            // Set up the local and world transforms for the reference shape entity.
            MockTransformHandler mockShapeTransformHandler;
            mockShapeTransformHandler.m_GetLocalTMOutput = AZ::Transform::CreateTranslation(AZ::Vector3(test.shapeLocalTM)); // Used for Local_ReferenceEntity
            mockShapeTransformHandler.m_GetWorldTMOutput = AZ::Transform::CreateTranslation(AZ::Vector3(test.shapeWorldTM)); // Used for World_ReferenceEntity
            mockShapeTransformHandler.BusConnect(mockShape->GetId());

            // Create the mock shape that maps our 3x3 image to a 3x3 sample space in the world.
            mockShape->CreateComponent<MockShapeComponent>();
            MockShapeComponentHandler mockShapeComponentHandler(mockShape->GetId());
            // Create a 2x2 box shape (shapes are inclusive, so that's 3x3 sampling space), so that each pixel in the image directly maps to 1 meter in the box.
            mockShapeComponentHandler.m_GetEncompassingAabb = AZ::Aabb::CreateFromMinMax(AZ::Vector3(0.0f), AZ::Vector3(2.0f));
            mockShapeComponentHandler.m_GetLocalBounds = AZ::Aabb::CreateFromMinMax(AZ::Vector3(-1.0f), AZ::Vector3(1.0f));
            // Shapes internally just cache the WorldTM, so make sure we've done the same for our test data.
            mockShapeComponentHandler.m_GetTransform = mockShapeTransformHandler.m_GetWorldTMOutput;

            // Create our gradient entity.
            auto entity = CreateEntity();

            // Create an ImageGradient with a 3x3 asset with the center pixel set.
            GradientSignal::ImageGradientConfig gradientConfig;
            uint8_t setPixelValues[] = { 255, 255, 255, 255 };
            gradientConfig.m_imageAsset = UnitTest::CreateSpecificPixelImageAsset(3, 3, 1, 1, setPixelValues);
            entity->CreateComponent<GradientSignal::ImageGradientComponent>(gradientConfig);

            // Create the test GradientTransform
            GradientSignal::GradientTransformConfig config;

            // We use ClampToZero to ensure that the only pixel that's set in the output is the center of where our image has been placed.
            config.m_wrappingType = GradientSignal::WrappingType::ClampToZero;

            // Turn on shape references, as these are needed for some of the transform types.
            config.m_advancedMode = true;
            config.m_allowReference = true;
            config.m_shapeReference = mockShape->GetId();

            // Set the rest of the parameters.
            config.m_transformType = test.transformType;
            config.m_frequencyZoom = 1.0f;
            config.m_overrideBounds = false;
            config.m_overrideTranslate = false;
            config.m_overrideRotate = false;
            config.m_overrideScale = false;
            config.m_is3d = false;
            entity->CreateComponent<GradientSignal::GradientTransformComponent>(config);

            // Set up the transform on the gradient entity.
            MockTransformHandler mockTransformHandler;
            mockTransformHandler.m_GetLocalTMOutput = AZ::Transform::CreateTranslation(AZ::Vector3(test.entityLocalTM)); // Used for Local_ThisEntity
            mockTransformHandler.m_GetWorldTMOutput = AZ::Transform::CreateTranslation(AZ::Vector3(test.entityWorldTM)); // Used for World_ThisEntity
            mockTransformHandler.BusConnect(entity->GetId());

            // Put a default shape on our gradient entity.  This is only used for previews, so it doesn't matter what it gets set to.
            entity->CreateComponent<MockShapeComponent>();
            MockShapeComponentHandler mockShapeHandler(entity->GetId());

            ActivateEntity(entity.get());

            TestFixedDataSampler(expectedOutput, dataSize, entity->GetId());
        }
    }

    struct GradientSignalImagePaintingTestsFixture
        : public GradientSignalTest
    {
        AZStd::unique_ptr<AZ::Entity> CreateTestImageGradientEntity(
            float boundsSize, uint32_t width, uint32_t height, AZStd::span<const uint8_t> pixels)
        {
            // Create an entity with a Box Shape
            auto entity = CreateTestEntity(boundsSize / 2.0f);

            // Create an Image Gradient Component with an image asset based on the passed-in pixel data.
            GradientSignal::ImageGradientConfig config;
            config.m_imageAsset = CreateImageAssetFromPixelData(width, height, AZ::RHI::Format::R8_UNORM, pixels);
            m_imageGradientComponent = entity->CreateComponent<GradientSignal::ImageGradientComponent>(config);

            // Create a Gradient Transform Component with default parameters
            entity->CreateComponent<GradientSignal::GradientTransformComponent>();

            ActivateEntity(entity.get());
            EXPECT_EQ(entity->GetState(), AZ::Entity::State::Active);

            return entity;
        }

       // Keep track of the image gradient component so that we have an easy way to get its component ID.
       GradientSignal::ImageGradientComponent* m_imageGradientComponent = nullptr;
    };

    TEST_F(GradientSignalImagePaintingTestsFixture, ImageGradientHasWorkingEyedropper)
    {
        // This test verifies that the paintbrush eyedropper can read values correctly from an Image Gradient.

        // Create an Image Gradient in a box that goes from (0, 0, 0) to (4, 4, 4) in world space.
        // We'll create a 4x4 image to map onto it, so each pixel is 1 x 1 m in size.
        // The lower left corner of the image maps to (0, 0) and the upper right to (4, 4).

        constexpr uint32_t width = 4;
        constexpr uint32_t height = 4;

        // The pixel values themselves are arbitrary, they're just all set to different values to help verify that the correct pixel
        // colors are getting read by the eyedropper at each world location.
        AZStd::vector<uint8_t> pixels = {
            // 0 - 1 m   1 - 2 m   2 - 3 m   3 - 4 m
                0x11,     0x22,     0x33,     0x44, // 3 - 4 m
                0x55,     0x66,     0x77,     0x88, // 2 - 3 m
                0x99,     0xAA,     0xBB,     0xCC, // 1 - 2 m
                0xDD,     0xEE,     0xFF,     0x00, // 0 - 1 m
        };

        constexpr float BoxBounds = 4.0f;
        auto entity = CreateTestImageGradientEntity(BoxBounds, width, height, pixels);

        AzFramework::PaintBrushSettings brushSettings;

        AzFramework::PaintBrush paintBrush({ entity->GetId(), m_imageGradientComponent->GetId() });
        paintBrush.BeginPaintMode();

        AZ::Aabb shapeBounds = AZ::Aabb::CreateNull();
        LmbrCentral::ShapeComponentRequestsBus::EventResult(
            shapeBounds, entity->GetId(), &LmbrCentral::ShapeComponentRequestsBus::Events::GetEncompassingAabb);

        // Loop through each pixel, use the eyedropper in world space to try to look it up, and verify the intensities match.
        for (uint32_t pixelIndex = 0; pixelIndex < pixels.size(); pixelIndex++)
        {
            uint32_t pixelX = pixelIndex % width;
            uint32_t pixelY = pixelIndex / width;

            auto location = PixelCoordinatesToWorldSpace(pixelX, pixelY, shapeBounds, width, height);

            // Use the eyedropper for each world position and verify that it matches the value in the gradient image.
            AZ::Color pixelColor = paintBrush.UseEyedropper(location);
            EXPECT_EQ(pixels[pixelIndex], pixelColor.GetR8());
        }

        paintBrush.EndPaintMode();

        entity.reset();
    }

    TEST_F(GradientSignalImagePaintingTestsFixture, ImageGradientCanBePainted)
    {
        // This test verifies that the paintbrush paint commands can modify values correctly in an Image Gradient.

        // Create an Image Gradient in a box that goes from (0, 0, 0) to (4, 4, 4) in world space.
        // We'll create a 4x4 image to map onto it, so each pixel is 1 x 1 m in size.
        // The lower left corner of the image maps to (0, 0) and the upper right to (4, 4).

        constexpr uint32_t width = 4;
        constexpr uint32_t height = 4;
        AZStd::vector<uint8_t> pixels(width * height);

        constexpr float BoxBounds = 4.0f;
        auto entity = CreateTestImageGradientEntity(BoxBounds, width, height, pixels);

        AZ::Aabb shapeBounds = AZ::Aabb::CreateNull();
        LmbrCentral::ShapeComponentRequestsBus::EventResult(
            shapeBounds, entity->GetId(), &LmbrCentral::ShapeComponentRequestsBus::Events::GetEncompassingAabb);

        // Choose arbitrary but equal color values except for the alpha, which is set to opaque.
        AZ::Color brushColor(0.5f, 0.5f, 0.5f, 1.0f);

        AzFramework::PaintBrushSettings brushSettings;
        brushSettings.SetColor(brushColor);
        brushSettings.SetSize(1.0f);
        EXPECT_THAT(brushSettings.GetColor(), UnitTest::IsClose(brushColor));

        // Find the location of the center of an arbitrary pixel in world space.
        constexpr uint32_t paintedPixelX = 2;
        constexpr uint32_t paintedPixelY = 1;
        auto paintedPixelLocation = PixelCoordinatesToWorldSpace(paintedPixelX, paintedPixelY, shapeBounds, width, height);

        AzFramework::PaintBrush paintBrush({ entity->GetId(), m_imageGradientComponent->GetId() });
        paintBrush.BeginPaintMode();

        // Verify that before painting, the pixel intensity is 0.
        AZ::Color startColor = paintBrush.UseEyedropper(paintedPixelLocation);
        EXPECT_EQ(startColor.GetR(), 0.0f);

        // Paint one pixel with the paintbrush.
        paintBrush.BeginBrushStroke(brushSettings);
        paintBrush.PaintToLocation(paintedPixelLocation, brushSettings);
        paintBrush.EndBrushStroke();

        // Loop through each pixel, use the eyedropper in world space to try to look it up, and verify the intensities match expectations.
        // Most of the pixels should still be 0.0, but the one painted pixel should be 0.5 (our brush intensity).
        for (uint32_t pixelIndex = 0; pixelIndex < pixels.size(); pixelIndex++)
        {
            uint32_t pixelX = pixelIndex % width;
            uint32_t pixelY = pixelIndex / width;
            auto queryLocation = PixelCoordinatesToWorldSpace(pixelX, pixelY, shapeBounds, width, height);

            // Use the eyedropper for each world position and verify that it matches the expected color.
            uint8_t expectedIntensity = (pixelX == paintedPixelX) && (pixelY == paintedPixelY) ? brushColor.GetR8() : (uint8_t)(0);
            AZ::Color pixelColor = paintBrush.UseEyedropper(queryLocation);
            EXPECT_EQ(pixelColor.GetR8(), expectedIntensity);
        }

        paintBrush.EndPaintMode();

        entity.reset();
    }
}


