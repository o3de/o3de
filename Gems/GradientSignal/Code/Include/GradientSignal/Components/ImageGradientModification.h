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

    struct PaintStrokeData
    {
        //! A buffer to accumulate a single paint stroke into. This buffer is used to ensure that within a single paint stroke,
        //! we only perform an operation on a pixel once, not multiple times.
        //! After the paint stroke is complete, this buffer is handed off to the undo/redo batch so that we can undo/redo each stroke.
        AZStd::shared_ptr<ImageTileBuffer> m_strokeBuffer;

        //! The meters per pixel in each direction for this image gradient.
        //! These help us query the paintbrush for exactly one world position per image pixel.
        float m_metersPerPixelX = 0.0f;
        float m_metersPerPixelY = 0.0f;

        //! The intensity of the paint stroke (0 - 1)
        float m_intensity = 0.0f;

        //! The opacity of the paint stroke (0 - 1)
        float m_opacity = 0.0f;

        //! Track the dirty region for each paint stroke so that we can store it in the undo/redo buffer
        //! to send with change notifications.
        AZ::Aabb m_dirtyRegion = AZ::Aabb::CreateNull();
    };

    class ImageGradientModifier : private AzFramework::PaintBrushNotificationBus::Handler
    {
    public:
        ImageGradientModifier(const AZ::EntityComponentIdPair& entityComponentIdPair);
        ~ImageGradientModifier() override;

    protected:
        // PaintBrushNotificationBus overrides
        void OnBrushStrokeBegin(const AZ::Color& color) override;
        void OnBrushStrokeEnd() override;
        void OnPaint(const AZ::Aabb& dirtyArea, ValueLookupFn& valueLookupFn, BlendFn& blendFn) override;
        void OnSmooth(
            const AZ::Aabb& dirtyArea,
            ValueLookupFn& valueLookupFn,
            AZStd::span<const AZ::Vector3> valuePointOffsets,
            SmoothFn& smoothFn) override;
        AZ::Color OnGetColor(const AZ::Vector3& brushCenter) override;

        void OnPaintSmoothInternal(
            const AZ::Aabb& dirtyArea,
            ValueLookupFn& valueLookupFn,
            AZStd::function<float(const AZ::Vector3& worldPosition, float gradientValue, float opacity)> combineFn);

    private:
        PaintStrokeData m_paintStrokeData;

        //! The entity/component that owns this paintbrush.
        AZ::EntityComponentIdPair m_ownerEntityComponentId;
    };

} // namespace GradientSignal
