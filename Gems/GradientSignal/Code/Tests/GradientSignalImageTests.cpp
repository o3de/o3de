/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "Tests/GradientSignalTestMocks.h"

#include <AzTest/AzTest.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Memory/PoolAllocator.h>
#include <AzCore/Math/Vector2.h>
#include <AzFramework/Asset/AssetCatalogBus.h>

#include <Source/Components/ImageGradientComponent.h>
#include <Source/Components/GradientTransformComponent.h>

namespace UnitTest
{
    struct GradientSignalImageTestsFixture
        : public GradientSignalTest
    {
        struct MockAssetHandler
            : public AZ::Data::AssetHandler
        {
            AZ::Data::AssetPtr CreateAsset([[maybe_unused]] const AZ::Data::AssetId& id, [[maybe_unused]] const AZ::Data::AssetType& type) override
            {
                return AZ::Data::AssetPtr();
            }

            void DestroyAsset(AZ::Data::AssetPtr ptr) override
            {
                if (ptr)
                {
                    delete ptr;
                }
            }

            void GetHandledAssetTypes([[maybe_unused]] AZStd::vector<AZ::Data::AssetType>& assetTypes) override
            {
            }

            AZ::Data::AssetHandler::LoadResult LoadAssetData(
                [[maybe_unused]] const AZ::Data::Asset<AZ::Data::AssetData>& asset,
                [[maybe_unused]] AZStd::shared_ptr<AZ::Data::AssetDataStream> stream,
                [[maybe_unused]] const AZ::Data::AssetFilterCB& assetLoadFilterCB) override
            {
                return AZ::Data::AssetHandler::LoadResult::LoadComplete;
            }

        };

        MockAssetHandler* m_mockHandler = nullptr;
        GradientSignal::ImageAsset* m_imageData = nullptr;

        void SetUp() override
        {
            GradientSignalTest::SetUp();
            AZ::AllocatorInstance<AZ::ThreadPoolAllocator>::Create();
            AZ::Data::AssetManager::Descriptor desc;
            AZ::Data::AssetManager::Create(desc);
            m_mockHandler = new MockAssetHandler();
            AZ::Data::AssetManager::Instance().RegisterHandler(m_mockHandler, azrtti_typeid<GradientSignal::ImageAsset>());
        }

        void TearDown() override
        {
            AZ::Data::AssetManager::Instance().UnregisterHandler(m_mockHandler);
            delete m_mockHandler; // delete after removing from the asset manager
            AzFramework::LegacyAssetEventBus::ClearQueuedEvents();
            AZ::Data::AssetManager::Destroy();
            AZ::AllocatorInstance<AZ::ThreadPoolAllocator>::Destroy();
            GradientSignalTest::TearDown();
        }

        struct AssignIdToAsset
            : public AZ::Data::AssetData
        {
            void MakeReady()
            {
                m_assetId = AZ::Data::AssetId(AZ::Uuid::CreateRandom());
                m_status.store(AZ::Data::AssetData::AssetStatus::Ready);
            }
        };

        struct PixelTestSetup
        {
            // How to create the source image
            AZ::u32 m_imageSize;
            AZ::Vector2 m_pixel;

            // How to initialize the gradient components
            AZ::u32 m_shapeBoundsSize;
            float   m_tiling;
            GradientSignal::WrappingType m_wrappingType;

            // How to loop through and validate the results
            AZ::u32 m_validationSize;
            float m_stepSize;
            AZ::Vector2 m_expectedPixels[32];

            static const AZ::Vector2 EndOfList;
        };

        AZ::Data::Asset<GradientSignal::ImageAsset> CreateImageAsset(AZ::u32 width, AZ::u32 height, AZ::s32 seed)
        {
            m_imageData = aznew GradientSignal::ImageAsset();
            m_imageData->m_imageWidth = width;
            m_imageData->m_imageHeight = height;
            m_imageData->m_bytesPerPixel = 1;
            m_imageData->m_imageFormat = ImageProcessingAtom::EPixelFormat::ePixelFormat_R8;

            size_t value = 0;
            AZStd::hash_combine(value, seed);

            for (AZ::u32 x = 0; x < width; ++x)
            {
                for (AZ::u32 y = 0; y < height; ++y)
                {
                    AZStd::hash_combine(value, x);
                    AZStd::hash_combine(value, y);
                    m_imageData->m_imageData.push_back(static_cast<AZ::u8>(value));
                }
            }

            reinterpret_cast<AssignIdToAsset*>(m_imageData)->MakeReady();
            return AZ::Data::Asset<GradientSignal::ImageAsset>(m_imageData, AZ::Data::AssetLoadBehavior::Default);
        }

        AZ::Data::Asset<GradientSignal::ImageAsset> CreateSpecificPixelImageAsset(AZ::u32 width, AZ::u32 height, AZ::u32 pixelX, AZ::u32 pixelY)
        {
            m_imageData = aznew GradientSignal::ImageAsset();
            m_imageData->m_imageWidth = width;
            m_imageData->m_imageHeight = height;
            m_imageData->m_bytesPerPixel = 1;
            m_imageData->m_imageFormat = ImageProcessingAtom::EPixelFormat::ePixelFormat_R8;

            const AZ::u8 pixelValue = 255;

            // Image data should be stored inverted on the y axis relative to our engine, so loop backwards through y.
            for (int y = static_cast<int>(height) - 1; y >= 0; --y)
            {
                for (AZ::u32 x = 0; x < width; ++x)
                {
                    if ((x == static_cast<int>(pixelX)) && (y == static_cast<int>(pixelY)))
                    {
                        m_imageData->m_imageData.push_back(pixelValue);
                    }
                    else
                    {
                        m_imageData->m_imageData.push_back(0);
                    }
                }
            }

            reinterpret_cast<AssignIdToAsset*>(m_imageData)->MakeReady();
            return AZ::Data::Asset<GradientSignal::ImageAsset>(m_imageData, AZ::Data::AssetLoadBehavior::Default);
        }

        void TestPixels(GradientSignal::GradientSampler& sampler, AZ::u32 width, AZ::u32 height, float stepSize, const AZStd::vector<AZ::Vector3>& expectedPoints)
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
                    if (value == 1.0f)
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

        void RunPixelTest(const PixelTestSetup& test)
        {
            // Create the base entity
            auto entity = CreateEntity();

            float shapeHalfBounds = test.m_shapeBoundsSize / 2.0f;

            // Create the Image Gradient Component.
            GradientSignal::ImageGradientConfig config;
            config.m_imageAsset = CreateSpecificPixelImageAsset(test.m_imageSize, test.m_imageSize, static_cast<AZ::u32>(test.m_pixel.GetX()), static_cast<AZ::u32>(test.m_pixel.GetY()));
            config.m_tilingX = test.m_tiling;
            config.m_tilingY = test.m_tiling;
            CreateComponent<GradientSignal::ImageGradientComponent>(entity.get(), config);

            // Create the Gradient Transform Component.
            GradientSignal::GradientTransformConfig gradientTransformConfig;
            gradientTransformConfig.m_wrappingType = test.m_wrappingType;
            CreateComponent<GradientSignal::GradientTransformComponent>(entity.get(), gradientTransformConfig);

            // Create a mock Shape component that describes the bounds that we're using to map our ImageGradient into world space.
            CreateComponent<MockShapeComponent>(entity.get());
            MockShapeComponentHandler mockShapeHandler(entity->GetId());
            mockShapeHandler.m_GetLocalBounds = AZ::Aabb::CreateCenterRadius(AZ::Vector3(shapeHalfBounds), shapeHalfBounds);

            // Create a mock Transform component that locates our ImageGradient in the center of our desired mock Shape.
            MockTransformHandler mockTransformHandler;
            mockTransformHandler.m_GetLocalTMOutput = AZ::Transform::CreateTranslation(AZ::Vector3(shapeHalfBounds));
            mockTransformHandler.m_GetWorldTMOutput = AZ::Transform::CreateTranslation(AZ::Vector3(shapeHalfBounds));
            mockTransformHandler.BusConnect(entity->GetId());

            // All components are created, so activate the entity
            ActivateEntity(entity.get());

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
            TestPixels(gradientSampler, test.m_validationSize, test.m_validationSize, test.m_stepSize, expectedPoints);
        }
    };

    const AZ::Vector2 GradientSignalImageTestsFixture::PixelTestSetup::EndOfList = AZ::Vector2(-1.0f);

    TEST_F(GradientSignalImageTestsFixture, ImageGradientComponentSinglePixelLower)
    {
        // Set one pixel, map Gradient 1:1 to lookup space, get same pixel back
        PixelTestSetup test =
        {
            4, AZ::Vector2( 0, 0 ),                                       // Source image:  4 x 4 with (0, 0) set
            4, 1.0f,                                                      // Mapped Shape:  4 x 4 with tiling (1.0, 1.0), unbounded
            GradientSignal::WrappingType::None,     
            4, 1.0f,                                                      // Validate that in 4 x 4 range, only 0, 0 is set
            { AZ::Vector2(0, 0), PixelTestSetup::EndOfList }
        };

        RunPixelTest(test);
    }

    TEST_F(GradientSignalImageTestsFixture, ImageGradientComponentSinglePixelUpper)
    {
        // Set one pixel, map Gradient 1:1 to lookup space, get same pixel back
        PixelTestSetup test =
        {
            4, AZ::Vector2(3, 3),                                         // Source image:  4 x 4 with (3, 3) set
            4, 1.0f,                                                      // Mapped Shape:  4 x 4 with tiling (1.0, 1.0), unbounded
            GradientSignal::WrappingType::None,
            4, 1.0f,                                                      // Validate that in 4 x 4 range, only 3, 3 is set
            { AZ::Vector2(3, 3), PixelTestSetup::EndOfList }
        };

        RunPixelTest(test);
    }

    TEST_F(GradientSignalImageTestsFixture, ImageGradientComponentSinglePixelUnbounded)
    {
        // Validate that our image repeats correctly when using "unbounded"
        PixelTestSetup test =
        {
            4, AZ::Vector2( 0, 0 ),                                       // Source image:  4 x 4 with (0, 0) set
            4, 1.0f,                                                      // Mapped Shape:  4 x 4 with tiling (1.0, 1.0), unbounded
            GradientSignal::WrappingType::None,
            8, 1.0f,                                                      // Validate that in 8 x 8 range, the pixel repeats every 4 pixels
            { AZ::Vector2(0, 0), AZ::Vector2(4, 0), AZ::Vector2(0, 4), AZ::Vector2(4, 4), PixelTestSetup::EndOfList }
        };

        RunPixelTest(test);
    }

    TEST_F(GradientSignalImageTestsFixture, ImageGradientComponentSinglePixelClampToZero)
    {
        // Validate that our image does *not* repeat when using "Clamp to Zero"
        PixelTestSetup test =
        {
            4, AZ::Vector2(0, 0),                                         // Source image:  4 x 4 with (0, 0) set
            4, 1.0f,                                                      // Mapped Shape:  4 x 4 with tiling (1.0, 1.0), unbounded
            GradientSignal::WrappingType::ClampToZero,
            8, 1.0f,                                                      // Validate that in 8 x 8 range, the pixel does *not* repeat
            { AZ::Vector2(0, 0), PixelTestSetup::EndOfList }
        };

        RunPixelTest(test);
    }

    TEST_F(GradientSignalImageTestsFixture, ImageGradientComponentSinglePixelClampToEdge)
    {
        // Validate that our image stretches the edge correctly when using "Clamp to Edge"
        PixelTestSetup test =
        {
            4, AZ::Vector2(3, 3),                                         // Source image:  4 x 4 with (3, 3) set
            4, 1.0f,                                                      // Mapped Shape:  4 x 4 with tiling (1.0, 1.0), unbounded
            GradientSignal::WrappingType::ClampToEdge,
            8, 1.0f,                                                      // Validate that in 8 x 8 range, a corner pixel "stretches" to everything right and down from it
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
            4, AZ::Vector2(0, 0),                                         // Source image:  4 x 4 with (0, 0) set
            4, 1.0f,                                                      // Mapped Shape:  4 x 4 with tiling (1.0, 1.0), unbounded
            GradientSignal::WrappingType::Repeat,
            8, 1.0f,                                                      // Validate that in 8 x 8 range, the pixel repeats every 4 pixels
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
            4, AZ::Vector2(0, 0),                                         // Source image:  4 x 4 with (0, 0) set
            4, 1.0f,                                                      // Mapped Shape:  4 x 4 with tiling (1.0, 1.0), unbounded
            GradientSignal::WrappingType::Mirror,
            16, 1.0f,                                                     // Validate that in 16 x 16 range, we get a mirrored repeat
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
            4, AZ::Vector2(0, 0),                                         // Source image:  4 x 4 with (0, 0) set
            4, 0.75f,                                                     // Mapped Shape:  4 x 4 with tiling (0.75, 0.75), unbounded
            GradientSignal::WrappingType::None,
            8, 1.0f,                                                      // Validate that in 8 x 8 range, unbounded tiling works
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
            4, AZ::Vector2(0, 0),                                         // Source image:  4 x 4 with (0, 0) set
            4, 0.75f,                                                     // Mapped Shape:  4 x 4 with tiling (0.75, 0.75), repeating
            GradientSignal::WrappingType::Repeat,
            8, 1.0f,                                                      // Validate that in 8 x 8 range, repeat tiling works
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
            4, AZ::Vector2(0, 0),                                         // Source image:  4 x 4 with (0, 0) set
            4, 1.0f,                                                      // Mapped Shape:  4 x 4 with tiling (1.0, 1.0), unbounded
            GradientSignal::WrappingType::None,
            4, 0.5f,                                                      // Validate that in 4 x 4 range sampled with 8 x 8 pixels, our 1 pixel turns into 4 pixels
            { AZ::Vector2(0, 0), AZ::Vector2(0.5f, 0), 
              AZ::Vector2(0, 0.5f), AZ::Vector2(0.5f, 0.5f),
              PixelTestSetup::EndOfList }
        };

        RunPixelTest(test);
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
            CreateComponent<MockShapeComponent>(mockShape.get());
            MockShapeComponentHandler mockShapeComponentHandler(mockShape->GetId());
            // Create a 2x2 box shape (shapes are inclusive, so that's 3x3 sampling space), so that each pixel in the image directly maps to 1 meter in the box.
            mockShapeComponentHandler.m_GetEncompassingAabb = AZ::Aabb::CreateFromMinMax(AZ::Vector3(0.0f), AZ::Vector3(2.0f));
            mockShapeComponentHandler.m_GetLocalBounds = mockShapeComponentHandler.m_GetEncompassingAabb;
            // Shapes internally just cache the WorldTM, so make sure we've done the same for our test data.
            mockShapeComponentHandler.m_GetTransform = mockShapeTransformHandler.m_GetWorldTMOutput;

            // Create our gradient entity.
            auto entity = CreateEntity();

            // Create an ImageGradient with a 3x3 asset with the center pixel set.
            GradientSignal::ImageGradientConfig gradientConfig;
            gradientConfig.m_imageAsset = CreateSpecificPixelImageAsset(3, 3, 1, 1);
            CreateComponent<GradientSignal::ImageGradientComponent>(entity.get(), gradientConfig);

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
            CreateComponent<GradientSignal::GradientTransformComponent>(entity.get(), config);

            // Set up the transform on the gradient entity.
            MockTransformHandler mockTransformHandler;
            mockTransformHandler.m_GetLocalTMOutput = AZ::Transform::CreateTranslation(AZ::Vector3(test.entityLocalTM)); // Used for Local_ThisEntity
            mockTransformHandler.m_GetWorldTMOutput = AZ::Transform::CreateTranslation(AZ::Vector3(test.entityWorldTM)); // Used for World_ThisEntity
            mockTransformHandler.BusConnect(entity->GetId());

            // Put a default shape on our gradient entity.  This is only used for previews, so it doesn't matter what it gets set to.
            CreateComponent<MockShapeComponent>(entity.get());
            MockShapeComponentHandler mockShapeHandler(entity->GetId());

            ActivateEntity(entity.get());

            TestFixedDataSampler(expectedOutput, dataSize, entity->GetId());
        }
    }

}


