/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RPI.Reflect/Image/StreamingImageAsset.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Component/Component.h>
#include <AzCore/Math/Vector2.h>
#include <AzCore/Serialization/Json/BaseJsonSerializer.h>
#include <AzCore/Serialization/Json/RegistrationContext.h>
#include <AzCore/std/parallel/shared_mutex.h>
#include <AzFramework/PaintBrush/PaintBrushNotificationBus.h>
#include <TerrainRenderer/TerrainMacroMaterialBus.h>

namespace Terrain
{
    //! Tracks all of the image modifications for a single continuous paint stroke. Since most modifications will only affect a small
    //! portion of an image, this buffer divides the total image space into fixed-size tiles and only creates an individual tile
    //! buffer when at least one pixel in that tile's space is modified.
    //! While painting the paint stroke, this buffer caches all of the unmodified texture values and the modifications for each
    //! modified pixel. The buffer is used to create a special "stroke layer" that accumulates opacity for each stroke, which
    //! then combines with the stroke opacity, stroke color, and blend mode to blend back into the base layer.
    //! After the paint stroke finishes, the stroke buffer ownership is handed over to the undo/redo system so that it can
    //! be used to undo/redo each individual paint stroke.
    class ImageTileBuffer
    {
    public:
        ImageTileBuffer(uint32_t imageWidth, uint32_t imageHeight, AZ::EntityId imageGradientEntityId);
        ~ImageTileBuffer() = default;

        //! Returns true if we don't have any pixel modifications, false if we do.
        bool Empty() const;

        //! Get the original color value for the given pixel index.
        //! Since we "lazy-cache" our unmodified image as tiles, create it here the first time we request a pixel from a tile.
        AZStd::pair<AZ::Color, float> GetOriginalPixelValueAndOpacity(const PixelIndex& pixelIndex);

        //! Set a modified color value for the given pixel index.
        void SetModifiedPixelValue(const PixelIndex& pixelIndex, AZ::Color modifiedValue, float opacity);

        //! For undo/redo operations, apply the buffer of changes back to the terrain macro texture.
        void ApplyChangeBuffer(bool undo);

    private:
        //! Given a pixel index, get the tile index that it maps to.
        uint32_t GetTileIndex(const PixelIndex& pixelIndex) const;

        //! Given a tile index, get the absolute start pixel index for the upper left corner of the tile.
        PixelIndex GetStartPixelIndex(uint32_t tileIndex) const;

        // Given a pixel index, get the relative pixel index within the tile
        uint32_t GetPixelTileIndex(const PixelIndex& pixelIndex) const;

        //! Create an image tile initialized with the macro texture values if the tile doesn't already exist.
        void CreateImageTile(uint32_t tileIndex);

        //! Size of each modified image tile that we'll cache off.
        //! This size is chosen somewhat arbitrarily to keep the number of tiles balanced at a reasonable size.
        //! It should also ideally be a power of 2 for faster division and mods.
        static inline constexpr uint32_t ImageTileSize = 32;

        //! Keeps track of all the unmodified and modified pixel values, as well as our paint stroke opacity layer, for an NxN tile.
        //! We store it a struct of arrays instead of an array of structs for better compatibility with bulk APIs,
        //! where we can just pass in a full array of values to update a full tile's worth of values at once.
        struct ImageTile
        {
            AZStd::array<AZ::Color, ImageTileSize * ImageTileSize> m_unmodifiedData;
            AZStd::array<AZ::Color, ImageTileSize * ImageTileSize> m_modifiedData;
            AZStd::array<float, ImageTileSize * ImageTileSize> m_modifiedDataOpacity;
        };

        //! A vector of pointers to image tiles.
        //! All of the tile pointer entries are always expected to exist, even if the pointers themselves are null.
        using ImageTileList = AZStd::vector<AZStd::unique_ptr<ImageTile>>;

        //! The actual storage for the set of image tile pointers. Image tiles get created on-demand whenever pixels in them change.
        //! This ultimately contains all of the changes for one continuous brush stroke.
        ImageTileList m_paintedImageTiles;

        //! The number of tiles we're creating in the X and Y directions to contain a full texture.
        const uint32_t m_numTilesX = 0;
        const uint32_t m_numTilesY = 0;

        //! The entity ID of the texture that we're modifying.
        const AZ::EntityId m_modifiedEntityId;

        //! Track whether or not we've modified any pixels.
        bool m_modifiedAnyPixels = false;
    };

    //! Tracks all of the data related to the macro material image size.
    struct MacroMaterialImageSizeData
    {
        //! The meters per pixel in each direction for this macro material.
        //! These help us query the paintbrush for exactly one world position per image pixel.
        float m_metersPerPixelX = 0.0f;
        float m_metersPerPixelY = 0.0f;

        //! The world bounds of the macro material
        AZ::Aabb m_macroMaterialBounds;

        //! Image width and height in pixels.
        int16_t m_imageWidth = 0;
        int16_t m_imageHeight = 0;
    };


    //! Handles all of the calculations for figuring out the dirty region AABB for the macro material texture.
    //! It tracks the dirty pixel region and converts that back to world space bounds on request.
    //! This ensures that our bounds fully encompass the world space pixel volumes, and not just their corners or centers.
    struct ModifiedImageRegion
    {
    public:
        ModifiedImageRegion() = default;
        ModifiedImageRegion(const MacroMaterialImageSizeData& imageData);

        //! Add a pixel's pixel index into the dirty region.
        void AddPoint(const PixelIndex& pixelIndex);

        //! Calculate the dirty region in world space that reflects everywhere that's changed.
        AZ::Aabb GetDirtyRegion();

        //! Returns true if there is a dirty region, false if there isn't.
        bool IsModified() const
        {
            return m_isModified;
        }

    private:
        // Adds the full bounds of a pixel in local pixel indices to the given AABB. 
        static void AddPixelAabb(const MacroMaterialImageSizeData& imageData, int16_t pixelX, int16_t pixelY, AZ::Aabb& region);

        MacroMaterialImageSizeData m_imageData;

        PixelIndex m_minModifiedPixelIndex;
        PixelIndex m_maxModifiedPixelIndex;

        bool m_isModified = false;
    };

    //! Top-level class that handles all of the actual image modification calculations for a paintbrush.
    class MacroMaterialImageModifier : private AzFramework::PaintBrushNotificationBus::Handler
    {
    public:
        MacroMaterialImageModifier(const AZ::EntityComponentIdPair& entityComponentIdPair);
        ~MacroMaterialImageModifier() override;

    protected:
        // PaintBrushNotificationBus overrides
        void OnBrushStrokeBegin(const AZ::Color& color) override;
        void OnBrushStrokeEnd() override;
        void OnPaint(const AZ::Color& color, const AZ::Aabb& dirtyArea, ValueLookupFn& valueLookupFn, BlendFn& blendFn) override;
        void OnSmooth(
            const AZ::Color& color,
            const AZ::Aabb& dirtyArea,
            ValueLookupFn& valueLookupFn,
            AZStd::span<const AZ::Vector3> valuePointOffsets,
            SmoothFn& smoothFn) override;
        AZ::Color OnGetColor(const AZ::Vector3& brushCenter) const override;

        void OnPaintSmoothInternal(
            const AZ::Aabb& dirtyArea,
            ValueLookupFn& valueLookupFn,
            AZStd::function<AZ::Color(const AZ::Vector3& worldPosition, AZ::Color gradientValue, float opacity)> combineFn);

    private:
        //! Keeps a local copy of all the image size data that's needed for locating pixels and calculating dirty regions.
        MacroMaterialImageSizeData m_imageData;

        //! A buffer to accumulate a single paint stroke into. This buffer is used to ensure that within a single paint stroke,
        //! we only perform an operation on a pixel once, not multiple times.
        //! After the paint stroke is complete, this buffer is handed off to the undo/redo batch so that we can undo/redo each stroke.
        AZStd::shared_ptr<ImageTileBuffer> m_strokeBuffer;

        //! Track the dirty region for each brush stroke so that we can store it in the undo/redo buffer
        //! to send with change notifications.
        ModifiedImageRegion m_modifiedStrokeRegion;

        //! The entity/component that owns this paintbrush.
        AZ::EntityComponentIdPair m_ownerEntityComponentId;
    };

} // namespace Terrain
