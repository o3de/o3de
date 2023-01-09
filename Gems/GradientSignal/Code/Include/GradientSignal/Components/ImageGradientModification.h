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
#include <GradientSignal/Ebuses/GradientRequestBus.h>
#include <GradientSignal/Ebuses/GradientTransformRequestBus.h>
#include <GradientSignal/Ebuses/ImageGradientRequestBus.h>
#include <GradientSignal/Ebuses/ImageGradientModificationBus.h>
#include <GradientSignal/Util.h>

namespace GradientSignal
{
    //! Tracks all of the image modifications for a single continuous paint stroke. Since most modifications will only affect a small
    //! portion of an image, this buffer divides the total image space into fixed-size tiles and only creates an individual tile
    //! buffer when at least one pixel in that tile's space is modified.
    //! While painting the paint stroke, this buffer caches all of the unmodified gradient values and the modifications for each
    //! modified pixel. The buffer is used to create a special "stroke layer" that accumulates opacity for each stroke, which
    //! then combines with the stroke opacity, stroke intensity, and blend mode to blend back into the base layer.
    //! After the paint stroke finishes, the stroke buffer ownership is handed over to the undo/redo system so that it can
    //! be used to undo/redo each individual paint stroke.
    class ImageTileBuffer
    {
    public:
        ImageTileBuffer(uint32_t imageWidth, uint32_t imageHeight, AZ::EntityId imageGradientEntityId);
        ~ImageTileBuffer() = default;

        //! Returns true if we don't have any pixel modifications, false if we do.
        bool Empty() const;

        //! Get the original gradient value for the given pixel index.
        //! Since we "lazy-cache" our unmodified image as tiles, create it here the first time we request a pixel from a tile.
        AZStd::pair<float, float> GetOriginalPixelValueAndOpacity(const PixelIndex& pixelIndex);

        //! Set a modified gradient value for the given pixel index.
        void SetModifiedPixelValue(const PixelIndex& pixelIndex, float modifiedValue, float opacity);

        //! For undo/redo operations, apply the buffer of changes back to the image gradient.
        void ApplyChangeBuffer(bool undo);

    private:
        //! Given a pixel index, get the tile index that it maps to.
        uint32_t GetTileIndex(const PixelIndex& pixelIndex) const;

        //! Given a tile index, get the absolute start pixel index for the upper left corner of the tile.
        PixelIndex GetStartPixelIndex(uint32_t tileIndex) const;

        // Given a pixel index, get the relative pixel index within the tile
        uint32_t GetPixelTileIndex(const PixelIndex& pixelIndex) const;

        //! Create an image tile initialized with the image gradient values if it doesn't already exist.
        void CreateImageTile(uint32_t tileIndex);

        //! Size of each modified image tile that we'll cache off.
        //! This size is chosen somewhat arbitrarily to keep the number of tiles balanced at a reasonable size.
        static inline constexpr uint32_t ImageTileSize = 32;

        //! Keeps track of all the unmodified and modified gradient values, as well as our paint stroke opacity layer, for an NxN tile.
        //! We store it a struct of arrays instead of an array of structs for better compatibility with the ImageGradient APIs,
        //! where we can just pass in a full array of values to update a full tile of values at once.
        struct ImageTile
        {
            AZStd::array<float, ImageTileSize * ImageTileSize> m_unmodifiedData;
            AZStd::array<float, ImageTileSize * ImageTileSize> m_modifiedData;
            AZStd::array<float, ImageTileSize * ImageTileSize> m_modifiedDataOpacity;
        };

        //! A vector of pointers to image tiles.
        //! All of the tile pointer entries are always expected to exist, even if the pointers are null.
        using ImageTileList = AZStd::vector<AZStd::unique_ptr<ImageTile>>;

        //! The actual storage for the set of image tile pointers. Image tiles get created on-demand whenever pixels in them change.
        //! This ultimately contains all of the changes for one continuous brush stroke.
        ImageTileList m_paintedImageTiles;

        //! The number of tiles we're creating in the X and Y directions to contain a full Image Gradient.
        const uint32_t m_numTilesX = 0;
        const uint32_t m_numTilesY = 0;

        //! The entity ID of the Image Gradient that we're modifying.
        const AZ::EntityId m_imageGradientEntityId;

        //! Track whether or not we've modified any pixels.
        bool m_modifiedAnyPixels = false;
    };

    //! Tracks all of the data related to the image gradient size, including its transform.
    struct ImageGradientSizeData
    {
        //! The meters per pixel in each direction for this image gradient.
        //! These help us query the paintbrush for exactly one world position per image pixel.
        float m_metersPerPixelX = 0.0f;
        float m_metersPerPixelY = 0.0f;

        //! The meters per pixel in each direction for this image gradient in the image's local space.
        //! This accounts for image tiling and frequency zoom, but removes the effects of the transform's scale, since
        //! the scale is applied outside of the local space calculations.
        float m_localMetersPerPixelX = 0.0f;
        float m_localMetersPerPixelY = 0.0f;

        //! Image width and height in pixels.
        int16_t m_imageWidth = 0;
        int16_t m_imageHeight = 0;

        //! The pixel indices for the pixels on the edges of the local bounds. These are used for calculating dirty region bounding boxes.
        PixelIndex m_topLeftPixelIndex;
        PixelIndex m_bottomRightPixelIndex;

        //! The gradient transform for this image gradient.
        GradientTransform m_gradientTransform;
    };

    //! Tracks all of the data that's specific to a paint stroke.
    struct PaintStrokeData
    {
        //! A buffer to accumulate a single paint stroke into. This buffer is used to ensure that within a single paint stroke,
        //! we only perform an operation on a pixel once, not multiple times.
        //! After the paint stroke is complete, this buffer is handed off to the undo/redo batch so that we can undo/redo each stroke.
        AZStd::shared_ptr<ImageTileBuffer> m_strokeBuffer;
    };

    //! Handles all of the calculations for figuring out the dirty region AABB for the image gradient based on all its settings.
    //! Depending on the tiling and gradient transform settings, painting one pixel on an image can result in dirty regions that
    //! are much larger than the one pixel, potentially even infinite in size if the image settings are "mirror" or "repeat".
    struct ModifiedImageRegion
    {
    public:
        ModifiedImageRegion() = default;
        ModifiedImageRegion(const ImageGradientSizeData& imageData);

        //! Add a pixel's pixel index into the dirty region.
        void AddPoint(const PixelIndex& pixelIndex);

        //! Calculate the dirty region that reflects everywhere that's changed.
        //! The output dirty region accounts for image repeats (via tiling / frequency zoom / scale), transform wrapping modes,
        //! rotation, and bilinear filtering.
        AZ::Aabb GetDirtyRegion();

        //! Returns true if there is a dirty region, false if there isn't.
        bool IsModified() const
        {
            return m_isModified;
        }

    private:
        // Adds the full bounds of a pixel in local space to the given AABB. 
        // We have two variations of this method - one that calculates from the top left corner in local space, and one
        // that calculates from the bottom right corner. Depending on the various tiling and frequency zoom settings, these will
        // produce different results since the same pixel can appear multiple times within the image gradient's local bounds.
        static void AddLocalSpacePixelAabbFromTopLeft(
            const ImageGradientSizeData& imageData, int16_t pixelX, int16_t pixelY, AZ::Aabb& region);
        static void AddLocalSpacePixelAabbFromBottomRight(
            const ImageGradientSizeData& imageData, int16_t pixelX, int16_t pixelY, AZ::Aabb& region);

        ImageGradientSizeData m_imageData;

        PixelIndex m_minModifiedPixelIndex;
        PixelIndex m_maxModifiedPixelIndex;

        bool m_modifiedLeftEdge = false;
        bool m_modifiedRightEdge = false;
        bool m_modifiedTopEdge = false;
        bool m_modifiedBottomEdge = false;

        bool m_isModified = false;
    };

    //! Top-level class that handles all of the actual image modification calculations for a paintbrush.
    class ImageGradientModifier : private AzFramework::PaintBrushNotificationBus::Handler
    {
    public:
        ImageGradientModifier(const AZ::EntityComponentIdPair& entityComponentIdPair);
        ~ImageGradientModifier() override;

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
            AZStd::function<float(const AZ::Vector3& worldPosition, float gradientValue, float opacity)> combineFn);

    private:
        //! Keeps a local copy of all the image size data that's needed for locating pixels and calculating dirty regions.
        ImageGradientSizeData m_imageData;

        //! Keeps track of all the data for a full brush stroke.
        PaintStrokeData m_paintStrokeData;

        //! Track the dirty region for each brush stroke so that we can store it in the undo/redo buffer
        //! to send with change notifications.
        ModifiedImageRegion m_modifiedStrokeRegion;

        //! The entity/component that owns this paintbrush.
        AZ::EntityComponentIdPair m_ownerEntityComponentId;
    };

} // namespace GradientSignal
