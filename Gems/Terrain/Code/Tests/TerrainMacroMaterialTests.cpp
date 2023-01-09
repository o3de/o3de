/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Memory/MemoryComponent.h>
#include <AzFramework/PaintBrush/PaintBrush.h>

#include <Atom/RPI.Reflect/Image/ImageMipChainAssetCreator.h>
#include <Atom/RPI.Reflect/Image/StreamingImageAssetCreator.h>

#include <AzFramework/Terrain/TerrainDataRequestBus.h>

#include <TerrainRenderer/Components/TerrainMacroMaterialComponent.h>
#include <LmbrCentral/Shape/BoxShapeComponentBus.h>
#include <AzTest/AzTest.h>
#include <AZTestShared/Math/MathTestHelpers.h>

#include <Terrain/MockTerrain.h>
#include <Terrain/MockTerrainMacroMaterialBus.h>
#include <TerrainTestFixtures.h>

using ::testing::NiceMock;
using ::testing::AtLeast;
using ::testing::_;

namespace UnitTest
{
    class TerrainMacroMaterialComponentTest : public UnitTest::TerrainSystemTestFixture
    {
    public:
        AZ::Data::Asset<AZ::RPI::ImageMipChainAsset> BuildBasicMipChainAsset(
            uint32_t width, uint32_t height, uint32_t pixelSize, const AZStd::vector<uint32_t>& pixels)
        {
            using namespace AZ;

            RPI::ImageMipChainAssetCreator assetCreator;

            const uint16_t mipLevels = 1;
            const uint16_t arraySize = 1;

            assetCreator.Begin(Data::AssetId(AZ::Uuid::CreateRandom()), mipLevels, arraySize);

            RHI::ImageSubresourceLayout layout(AZ::RHI::Size(width, height, 1), width, width * pixelSize, width * height * pixelSize, 1, 1);

            assetCreator.BeginMip(layout);
            assetCreator.AddSubImage(pixels.data(), pixels.size() * sizeof(uint32_t));
            assetCreator.EndMip();

            Data::Asset<RPI::ImageMipChainAsset> asset;
            EXPECT_TRUE(assetCreator.End(asset));
            EXPECT_TRUE(asset.IsReady());
            EXPECT_NE(asset.Get(), nullptr);

            return asset;
        }

        AZ::Data::Asset<AZ::RPI::StreamingImageAsset> CreateImageAsset(
            uint32_t width, uint32_t height, const AZStd::vector<uint32_t>& pixels)
        {
            AZ_Assert(pixels.size() == (width * height), "Pixel array is the wrong size for this image width and height.");

            auto randomAssetId = AZ::Data::AssetId(AZ::Uuid::CreateRandom());
            auto imageAsset = AZ::Data::AssetManager::Instance().CreateAsset<AZ::RPI::StreamingImageAsset>(
                randomAssetId, AZ::Data::AssetLoadBehavior::Default);

            const auto format = AZ::RHI::Format::R8G8B8A8_UNORM;
            const uint32_t pixelSize = AZ::RHI::GetFormatComponentCount(format);

            AZ::Data::Asset<AZ::RPI::ImageMipChainAsset> mipChain = BuildBasicMipChainAsset(width, height, pixelSize, pixels);

            AZ::RPI::StreamingImageAssetCreator assetCreator;
            assetCreator.Begin(randomAssetId);

            AZ::RHI::ImageDescriptor imageDesc =
                AZ::RHI::ImageDescriptor::Create2D(AZ::RHI::ImageBindFlags::ShaderRead, width, height, format);

            assetCreator.SetImageDescriptor(imageDesc);
            assetCreator.AddMipChainAsset(*mipChain.Get());

            EXPECT_TRUE(assetCreator.End(imageAsset));
            EXPECT_TRUE(imageAsset.IsReady());
            EXPECT_NE(imageAsset.Get(), nullptr);

            return imageAsset;
        }

        AZ::Vector3 PixelCoordinatesToWorldSpace(
            uint32_t pixelX, uint32_t pixelY, const AZ::Aabb& bounds, uint32_t width, uint32_t height)
        {
            AZ::Vector2 pixelSize(bounds.GetXExtent() / aznumeric_cast<float>(width), bounds.GetYExtent() / aznumeric_cast<float>(height));

            // Return the center point of the pixel in world space.
            // Note that Y gets flipped because of the way images map into world space. (0,0) is the lower left corner in world space,
            // but the upper left corner in image space.
            return AZ::Vector3(
                bounds.GetMin().GetX() + ((pixelX + 0.5f) * pixelSize.GetX()),
                bounds.GetMin().GetY() + ((height - (pixelY + 0.5f)) * pixelSize.GetY()),
                0.0f
            );
        }
    };

    TEST_F(TerrainMacroMaterialComponentTest, MissingRequiredComponentsActivateFailure)
    {
        auto entity = CreateEntity();

        entity->CreateComponent<Terrain::TerrainMacroMaterialComponent>();

        const AZ::Entity::DependencySortOutcome sortOutcome = entity->EvaluateDependenciesGetDetails();
        EXPECT_FALSE(sortOutcome.IsSuccess());

        entity.reset();
    }

    TEST_F(TerrainMacroMaterialComponentTest, RequiredComponentsPresentEntityActivateSuccess)
    {
        // No macro material asset is getting attached, so we shouldn't get any macro material create/destroy notifications.
        NiceMock<UnitTest::MockTerrainMacroMaterialNotificationBus> mockMacroMaterialNotifications;
        EXPECT_CALL(mockMacroMaterialNotifications, OnTerrainMacroMaterialCreated).Times(0);
        EXPECT_CALL(mockMacroMaterialNotifications, OnTerrainMacroMaterialDestroyed).Times(0);

        constexpr float BoxHalfBounds = 128.0f;
        auto entity = CreateTestBoxEntity(BoxHalfBounds);

        entity->CreateComponent<Terrain::TerrainMacroMaterialComponent>();

        ActivateEntity(entity.get());
        EXPECT_EQ(entity->GetState(), AZ::Entity::State::Active);

        entity.reset();
    }

    TEST_F(TerrainMacroMaterialComponentTest, ComponentWithMacroColorAssetNotifiesMacroMaterialCreationAndDestruction)
    {
        // We're attaching a loaded macro color asset, so should get macro material create/destroy notifications.
        NiceMock<UnitTest::MockTerrainMacroMaterialNotificationBus> mockMacroMaterialNotifications;
        EXPECT_CALL(mockMacroMaterialNotifications, OnTerrainMacroMaterialCreated).Times(1);
        EXPECT_CALL(mockMacroMaterialNotifications, OnTerrainMacroMaterialDestroyed).Times(1);

        // Create an entity with a box
        constexpr float BoxHalfBounds = 128.0f;
        auto entity = CreateTestBoxEntity(BoxHalfBounds);

        // Create a dummy image asset to use for the macro color.
        constexpr uint32_t width = 4;
        constexpr uint32_t height = 4;
        AZStd::vector<uint32_t> pixels(width * height);
        auto macroColorAsset = CreateImageAsset(width, height, pixels);

        // Create the component.
        Terrain::TerrainMacroMaterialConfig config;
        config.m_macroColorAsset = macroColorAsset;
        entity->CreateComponent<Terrain::TerrainMacroMaterialComponent>(config);

        // Activate the entity and verify that it activated successfully. This should generate the macro material create notification.
        ActivateEntity(entity.get());
        EXPECT_EQ(entity->GetState(), AZ::Entity::State::Active);

        // Decativate the entity, which should generate the macro material destroy notification.
        entity.reset();
    }

    TEST_F(TerrainMacroMaterialComponentTest, ComponentWithMacroColorHasWorkingEyedropper)
    {
        // We're attaching a loaded macro color asset, so should get macro material create/destroy notifications.
        NiceMock<UnitTest::MockTerrainMacroMaterialNotificationBus> mockMacroMaterialNotifications;
        EXPECT_CALL(mockMacroMaterialNotifications, OnTerrainMacroMaterialCreated).Times(1);
        EXPECT_CALL(mockMacroMaterialNotifications, OnTerrainMacroMaterialDestroyed).Times(1);

        // Create a Terrain Macro Material in a box that goes from (0, 0, 0) to (4, 4, 4) in world space.
        // We'll create a 4x4 image to map onto it, so each pixel is 1 x 1 m in size.
        // The lower left corner of the image maps to (0, 0) and the upper right to (4, 4).

        constexpr float BoxHalfBounds = 2.0f;
        auto entity = CreateTestBoxEntity(BoxHalfBounds);
        uint32_t width = 4;
        uint32_t height = 4;

        // The pixel values themselves are arbitrary, they're just all set to different values to help verify that the correct pixel
        // colors are getting read by the eyedropper at each world location.
        AZStd::vector<uint32_t> pixels = {
            // 0 - 1 m   1 - 2 m     2 - 3 m     3 - 4 m
            0xF0000000, 0xFFFF0000, 0xFF00FF00, 0xFF0000FF, // 3 - 4 m
            0xC0000000, 0xFFC00000, 0xFF00C000, 0xFF0000C0, // 2 - 3 m
            0x80000000, 0xFF800000, 0xFF008000, 0xFF000080, // 1 - 2 m
            0x40000000, 0xFF400000, 0xFF004000, 0xFF000040, // 0 - 1 m
        };

        auto macroColorAsset = CreateImageAsset(width, height, pixels);
        Terrain::TerrainMacroMaterialConfig config;
        config.m_macroColorAsset = macroColorAsset;

        auto macroMaterialComponent = entity->CreateComponent<Terrain::TerrainMacroMaterialComponent>(config);

        ActivateEntity(entity.get());
        EXPECT_EQ(entity->GetState(), AZ::Entity::State::Active);

        AzFramework::PaintBrushSettings brushSettings;

        AzFramework::PaintBrush paintBrush({ entity->GetId(), macroMaterialComponent->GetId() });
        paintBrush.BeginPaintMode();

        AZ::Aabb shapeBounds = AZ::Aabb::CreateNull();
        LmbrCentral::ShapeComponentRequestsBus::EventResult(
            shapeBounds, entity->GetId(), &LmbrCentral::ShapeComponentRequestsBus::Events::GetEncompassingAabb);

        // Loop through each pixel, use the eyedropper in world space to try to look it up, and verify the colors match.
        for (uint32_t pixelIndex = 0; pixelIndex < pixels.size(); pixelIndex++)
        {
            uint32_t pixelX = pixelIndex % width;
            uint32_t pixelY = pixelIndex / width;

            auto location = PixelCoordinatesToWorldSpace(pixelX, pixelY, shapeBounds, width, height);

            // Use the eyedropper for each world position and verify that it matches the color in the image.
            AZ::Color pixelColor = paintBrush.UseEyedropper(location);
            uint32_t expectedPixel = pixels[pixelIndex];
            AZ::Color expectedColor(
                aznumeric_cast<uint8_t>((expectedPixel >> 0) & 0xFF),
                aznumeric_cast<uint8_t>((expectedPixel >> 8) & 0xFF),
                aznumeric_cast<uint8_t>((expectedPixel >> 16) & 0xFF),
                aznumeric_cast<uint8_t>((expectedPixel >> 24) & 0xFF));
            EXPECT_THAT(pixelColor, UnitTest::IsClose(expectedColor));
        }

        paintBrush.EndPaintMode();

        entity.reset();
    }

    TEST_F(TerrainMacroMaterialComponentTest, ComponentWithMacroColorCanBePainted)
    {
        // We're attaching a loaded macro color asset, so should get macro material create/destroy notifications.
        NiceMock<UnitTest::MockTerrainMacroMaterialNotificationBus> mockMacroMaterialNotifications;
        EXPECT_CALL(mockMacroMaterialNotifications, OnTerrainMacroMaterialCreated).Times(1);
        EXPECT_CALL(mockMacroMaterialNotifications, OnTerrainMacroMaterialDestroyed).Times(1);

        // Create a Terrain Macro Material in a box that goes from (0, 0, 0) to (4, 4, 4) in world space.
        // We'll create a 4x4 image to map onto it, so each pixel is 1 x 1 m in size.
        // The lower left corner of the image maps to (0, 0) and the upper right to (4, 4).

        constexpr float BoxHalfBounds = 2.0f;
        auto entity = CreateTestBoxEntity(BoxHalfBounds);
        uint32_t width = 4;
        uint32_t height = 4;
        AZStd::vector<uint32_t> pixels(4 * 4);

        auto macroColorAsset = CreateImageAsset(width, height, pixels);
        Terrain::TerrainMacroMaterialConfig config;
        config.m_macroColorAsset = macroColorAsset;

        auto macroMaterialComponent = entity->CreateComponent<Terrain::TerrainMacroMaterialComponent>(config);

        ActivateEntity(entity.get());
        EXPECT_EQ(entity->GetState(), AZ::Entity::State::Active);

        AZ::Aabb shapeBounds = AZ::Aabb::CreateNull();
        LmbrCentral::ShapeComponentRequestsBus::EventResult(
            shapeBounds, entity->GetId(), &LmbrCentral::ShapeComponentRequestsBus::Events::GetEncompassingAabb);

        // Choose color values that are arbitrary and different except for the alpha, which is set to opaque.
        AZ::Color brushColor(
            aznumeric_cast<uint8_t>(20), aznumeric_cast<uint8_t>(40), aznumeric_cast<uint8_t>(60), aznumeric_cast<uint8_t>(255));

        AzFramework::PaintBrushSettings brushSettings;
        brushSettings.SetColor(brushColor);
        brushSettings.SetSize(1.0f);
        EXPECT_THAT(brushSettings.GetColor(), UnitTest::IsClose(brushColor));

        constexpr uint32_t paintedPixelX = 2;
        constexpr uint32_t paintedPixelY = 1;
        auto paintedPixelLocation = PixelCoordinatesToWorldSpace(paintedPixelX, paintedPixelY, shapeBounds, width, height);

        AzFramework::PaintBrush paintBrush({ entity->GetId(), macroMaterialComponent->GetId() });
        paintBrush.BeginPaintMode();

        AZ::Color startColor = paintBrush.UseEyedropper(paintedPixelLocation);
        EXPECT_THAT(startColor, UnitTest::IsClose(AZ::Color(0.0f, 0.0f, 0.0f, 0.0f)));

        paintBrush.BeginBrushStroke(brushSettings);
        paintBrush.PaintToLocation(paintedPixelLocation, brushSettings);
        paintBrush.EndBrushStroke();

        // Loop through each pixel, use the eyedropper in world space to try to look it up, and verify the colors match expectations.
        // Most of the pixels should still be (0, 0, 0, 0), but one pixel should be (0.25, 0.5, 0.75, 0). Note that the alpha remains 0
        // even though we're painting with full opacity. This is because the alpha in the original image is preserved through painting
        // and isn't modified. The opacity of the brush just affects how the brush merges with the image.
        for (uint32_t pixelIndex = 0; pixelIndex < pixels.size(); pixelIndex++)
        {
            uint32_t pixelX = pixelIndex % width;
            uint32_t pixelY = pixelIndex / width;

            auto queryLocation = PixelCoordinatesToWorldSpace(pixelX, pixelY, shapeBounds, width, height);

            AZ::Color expectedColor = (pixelX == paintedPixelX) && (pixelY == paintedPixelY)
                ? AZ::Color(brushColor.GetR(), brushColor.GetG(), brushColor.GetB(), 0.0f)
                : AZ::Color(0.0f);

            // Use the eyedropper for each world position and verify that it matches the expected color.
            AZ::Color pixelColor = paintBrush.UseEyedropper(queryLocation);
            EXPECT_THAT(pixelColor, UnitTest::IsCloseTolerance(expectedColor, 0.001f));
        }

        paintBrush.EndPaintMode();

        entity.reset();
    }
} // namespace UnitTest
