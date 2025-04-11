/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/Component/TransformBus.h>
#include <AzFramework/PaintBrush/PaintBrush.h>

#include <Atom/RPI.Reflect/Image/ImageMipChainAssetCreator.h>
#include <Atom/RPI.Reflect/Image/StreamingImageAssetCreator.h>

#include <AzFramework/Terrain/TerrainDataRequestBus.h>

#include <GradientSignalTestHelpers.h>
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
        AZ::Data::Asset<AZ::RPI::StreamingImageAsset> CreateMacroColorAsset(
            AZ::u32 width, AZ::u32 height, AZStd::span<const uint32_t> data)
        {
            AZStd::span<const uint8_t> rawPixels(reinterpret_cast<const uint8_t*>(data.data()), data.size() * sizeof(uint32_t));
            return CreateImageAssetFromPixelData(width, height, AZ::RHI::Format::R8G8B8A8_UNORM, rawPixels);
        }

        AZStd::unique_ptr<AZ::Entity> CreateTestMacroMaterialEntity(
            float bounds, AZ::Data::Asset<AZ::RPI::StreamingImageAsset> macroColorAsset)
        {
            auto entity = CreateTestBoxEntity(bounds / 2.0f);

            Terrain::TerrainMacroMaterialConfig config;
            config.m_macroColorAsset = macroColorAsset;

            m_macroMaterialComponent = entity->CreateComponent<Terrain::TerrainMacroMaterialComponent>(config);

            ActivateEntity(entity.get());
            EXPECT_EQ(entity->GetState(), AZ::Entity::State::Active);

            return entity;
        }

        // Keep track of the Macro Material component so that we have an easy way to access the component ID.
        Terrain::TerrainMacroMaterialComponent* m_macroMaterialComponent = nullptr;
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

        // Create a dummy image asset to use for the macro color.
        constexpr uint32_t width = 4;
        constexpr uint32_t height = 4;
        AZStd::vector<uint32_t> pixels(width * height);
        
        auto macroColorAsset = CreateMacroColorAsset(width, height, pixels);

        // Create and activate the test entity.
        constexpr float BoxBounds = 256.0f;
        auto entity = CreateTestMacroMaterialEntity(BoxBounds, macroColorAsset);

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

        auto macroColorAsset = CreateMacroColorAsset(width, height, pixels);

        // Create and activate the test entity.
        constexpr float BoxBounds = 4.0f;
        auto entity = CreateTestMacroMaterialEntity(BoxBounds, macroColorAsset);

        AzFramework::PaintBrushSettings brushSettings;

        AzFramework::PaintBrush paintBrush({ entity->GetId(), m_macroMaterialComponent->GetId() });
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

        uint32_t width = 4;
        uint32_t height = 4;
        AZStd::vector<uint32_t> pixels(4 * 4);

        auto macroColorAsset = CreateMacroColorAsset(width, height, pixels);

        // Create and activate the test entity.
        constexpr float BoxBounds = 4.0f;
        auto entity = CreateTestMacroMaterialEntity(BoxBounds, macroColorAsset);

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

        AzFramework::PaintBrush paintBrush({ entity->GetId(), m_macroMaterialComponent->GetId() });
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
