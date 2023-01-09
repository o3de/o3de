/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <GradientSignal/Components/ImageGradientModification.h>
#include <GradientSignal/Components/ImageGradientComponent.h>
#include <LmbrCentral/Dependency/DependencyNotificationBus.h>

namespace GradientSignal
{
    ImageTileBuffer::ImageTileBuffer(uint32_t imageWidth, uint32_t imageHeight, AZ::EntityId imageGradientEntityId)
        : m_imageGradientEntityId(imageGradientEntityId)
        // Calculate the number of image tiles in each direction that we'll need, rounding up so that we create an image tile
        // for fractional tiles as well.
        , m_numTilesX((imageWidth + ImageTileSize - 1) / ImageTileSize)
        , m_numTilesY((imageHeight + ImageTileSize - 1) / ImageTileSize)
    {
        // Create empty entries for every tile. Each entry is just a null pointer at the start, so the memory overhead
        // of these empty entries at 32x32 pixels per tile, a 1024x1024 image will have 8 KB of overhead.
        m_paintedImageTiles.resize(m_numTilesX * m_numTilesY);
    }

    bool ImageTileBuffer::Empty() const
    {
        return !m_modifiedAnyPixels;
    }

    AZStd::pair<float, float> ImageTileBuffer::GetOriginalPixelValueAndOpacity(const PixelIndex& pixelIndex)
    {
        uint32_t tileIndex = GetTileIndex(pixelIndex);
        uint32_t pixelTileIndex = GetPixelTileIndex(pixelIndex);

        // Create the tile if it doesn't already exist.
        CreateImageTile(tileIndex);

        return { m_paintedImageTiles[tileIndex]->m_unmodifiedData[pixelTileIndex],
                    m_paintedImageTiles[tileIndex]->m_modifiedDataOpacity[pixelTileIndex] };
    }

    void ImageTileBuffer::SetModifiedPixelValue(const PixelIndex& pixelIndex, float modifiedValue, float opacity)
    {
        uint32_t tileIndex = GetTileIndex(pixelIndex);
        uint32_t pixelTileIndex = GetPixelTileIndex(pixelIndex);

        AZ_Assert(m_paintedImageTiles[tileIndex], "Cached image tile hasn't been created yet!");

        m_paintedImageTiles[tileIndex]->m_modifiedData[pixelTileIndex] = modifiedValue;
        m_paintedImageTiles[tileIndex]->m_modifiedDataOpacity[pixelTileIndex] = opacity;
    }

    void ImageTileBuffer::ApplyChangeBuffer(bool undo)
    {
        AZStd::array<PixelIndex, ImageTileSize * ImageTileSize> pixelIndices;

        for (int32_t tileIndex = 0; tileIndex < m_paintedImageTiles.size(); tileIndex++)
        {
            // If we never created this tile, skip it and move on.
            if (m_paintedImageTiles[tileIndex] == nullptr)
            {
                continue;
            }

            // Create an array of pixel indices for every pixel in this tile.
            PixelIndex startIndex = GetStartPixelIndex(tileIndex);
            uint32_t index = 0;
            for (int16_t y = 0; y < ImageTileSize; y++)
            {
                for (int16_t x = 0; x < ImageTileSize; x++)
                {
                    pixelIndices[index++] =
                        PixelIndex(aznumeric_cast<int16_t>(startIndex.first + x), aznumeric_cast<int16_t>(startIndex.second + y));
                }
            }

            // Set the image gradient values for this tile either to the original or the modified values.
            // It's possible that not every pixel in the tile was modified, but it's cheaper just to update per-tile
            // than to track each individual pixel in the tile and set them individually.
            ImageGradientModificationBus::Event(
                m_imageGradientEntityId,
                &ImageGradientModificationBus::Events::SetPixelValuesByPixelIndex,
                pixelIndices,
                undo ? m_paintedImageTiles[tileIndex]->m_unmodifiedData : m_paintedImageTiles[tileIndex]->m_modifiedData);
        }
    }

    uint32_t ImageTileBuffer::GetTileIndex(const PixelIndex& pixelIndex) const
    {
        return ((pixelIndex.second / ImageTileSize) * m_numTilesX) + (pixelIndex.first / ImageTileSize);
    }

    PixelIndex ImageTileBuffer::GetStartPixelIndex(uint32_t tileIndex) const
    {
        return PixelIndex(
            aznumeric_cast<int16_t>((tileIndex % m_numTilesX) * ImageTileSize),
            aznumeric_cast<int16_t>((tileIndex / m_numTilesX) * ImageTileSize));
    }

    uint32_t ImageTileBuffer::GetPixelTileIndex(const PixelIndex& pixelIndex) const
    {
        uint32_t xIndex = pixelIndex.first % ImageTileSize;
        uint32_t yIndex = pixelIndex.second % ImageTileSize;

        return (yIndex * ImageTileSize) + xIndex;
    }

    void ImageTileBuffer::CreateImageTile(uint32_t tileIndex)
    {
        // If it already exists, there's nothing more to do.
        if (m_paintedImageTiles[tileIndex])
        {
            return;
        }

        auto imageTile = AZStd::make_unique<ImageTile>();

        // Initialize the list of pixel indices for this tile.
        AZStd::array<PixelIndex, ImageTileSize * ImageTileSize> pixelIndices;
        PixelIndex startIndex = GetStartPixelIndex(tileIndex);
        for (int16_t index = 0; index < (ImageTileSize * ImageTileSize); index++)
        {
            pixelIndices[index] = PixelIndex(
                aznumeric_cast<int16_t>(startIndex.first + (index % ImageTileSize)),
                aznumeric_cast<int16_t>(startIndex.second + (index / ImageTileSize)));
        }

        AZ_Assert(imageTile->m_unmodifiedData.size() == pixelIndices.size(), "ImageTile and PixelIndices are out of sync.");

        // Read all of the original gradient values into the image tile buffer.
        ImageGradientModificationBus::Event(
            m_imageGradientEntityId,
            &ImageGradientModificationBus::Events::GetPixelValuesByPixelIndex,
            pixelIndices,
            imageTile->m_unmodifiedData);

        // Initialize the modified value buffer with the original values. This way we can always undo/redo an entire tile at a time
        // without tracking which pixels in the tile have been modified.
        imageTile->m_modifiedData = imageTile->m_unmodifiedData;

        AZStd::fill(imageTile->m_modifiedDataOpacity.begin(), imageTile->m_modifiedDataOpacity.end(), 0.0f);

        m_paintedImageTiles[tileIndex] = AZStd::move(imageTile);

        // If we create a tile, we'll use that as shorthand for tracking that changed data exists.
        m_modifiedAnyPixels = true;
    }

    ModifiedImageRegion::ModifiedImageRegion(const ImageGradientSizeData& imageData)
        : m_minModifiedPixelIndex(AZStd::numeric_limits<int16_t>::max(), AZStd::numeric_limits<int16_t>::max())
        , m_maxModifiedPixelIndex(aznumeric_cast<int16_t>(-1), aznumeric_cast<int16_t>(-1))
        , m_isModified(false)
        , m_imageData(imageData)
    {
    }

    void ModifiedImageRegion::AddPoint(const PixelIndex& pixelIndex)
    {
        // Each time we modify a pixel, adjust our min and max pixel ranges to include it.
        m_minModifiedPixelIndex = PixelIndex(
            AZStd::min(m_minModifiedPixelIndex.first, pixelIndex.first), AZStd::min(m_minModifiedPixelIndex.second, pixelIndex.second));
        m_maxModifiedPixelIndex = PixelIndex(
            AZStd::max(m_maxModifiedPixelIndex.first, pixelIndex.first), AZStd::max(m_maxModifiedPixelIndex.second, pixelIndex.second));

        // Keep track of whether or not we've modified a pixel that occurs on an edge. This is used when the wrapping type
        // is 'ClampToEdge' to determine if our modified region needs to stretch out to infinity in that direction.
        m_modifiedLeftEdge = m_modifiedLeftEdge || (pixelIndex.first == m_imageData.m_topLeftPixelIndex.first);
        m_modifiedRightEdge = m_modifiedRightEdge || (pixelIndex.first == m_imageData.m_bottomRightPixelIndex.first);
        m_modifiedTopEdge = m_modifiedTopEdge || (pixelIndex.second == m_imageData.m_topLeftPixelIndex.second);
        m_modifiedBottomEdge = m_modifiedBottomEdge || (pixelIndex.second == m_imageData.m_bottomRightPixelIndex.second);

        // Track that we've modified at least one pixel.
        m_isModified = true;
    }

    void ModifiedImageRegion::AddLocalSpacePixelAabbFromTopLeft(
        const ImageGradientSizeData& imageData, int16_t pixelX, int16_t pixelY, AZ::Aabb& region)
    {
        // This adds an AABB representing the size of one pixel in local space.
        // This method calculates the pixel's location from the top left corner of the local bounds.

        // Get the local bounds of the image gradient.
        const AZ::Aabb localBounds = imageData.m_gradientTransform.GetBounds();

        // ShiftedPixel* contains the number of pixels to offset from the first pixel in the top left corner.
        // The double-mod is used to wrap around any negative results that can occur with certain combinations of tiling
        // and frequency zoom settings.

        int16_t shiftedPixelX = (pixelX - imageData.m_topLeftPixelIndex.first) % imageData.m_imageWidth;
        shiftedPixelX = (shiftedPixelX + imageData.m_imageWidth) % imageData.m_imageWidth;

        int16_t shiftedPixelY = (pixelY - imageData.m_topLeftPixelIndex.second) % imageData.m_imageHeight;
        shiftedPixelY = (shiftedPixelY + imageData.m_imageHeight) % imageData.m_imageHeight;

        // X pixels run left to right (min to max), but Y pixels run top to bottom (max to min), so we account for that
        // in the math below.
        region.AddPoint(AZ::Vector3(
            localBounds.GetMin().GetX() + (imageData.m_localMetersPerPixelX * shiftedPixelX),
            localBounds.GetMax().GetY() - (imageData.m_localMetersPerPixelY * shiftedPixelY),
            0.0f));
        region.AddPoint(AZ::Vector3(
            localBounds.GetMin().GetX() + (imageData.m_localMetersPerPixelX * (shiftedPixelX + 1)),
            localBounds.GetMax().GetY() - (imageData.m_localMetersPerPixelY * shiftedPixelY),
            0.0f));
        region.AddPoint(AZ::Vector3(
            localBounds.GetMin().GetX() + (imageData.m_localMetersPerPixelX * shiftedPixelX),
            localBounds.GetMax().GetY() - (imageData.m_localMetersPerPixelY * (shiftedPixelY + 1)),
            0.0f));
        region.AddPoint(AZ::Vector3(
            localBounds.GetMin().GetX() + (imageData.m_localMetersPerPixelX * (shiftedPixelX + 1)),
            localBounds.GetMax().GetY() - (imageData.m_localMetersPerPixelY * (shiftedPixelY + 1)),
            0.0f));
    };

    void ModifiedImageRegion::AddLocalSpacePixelAabbFromBottomRight(
        const ImageGradientSizeData& imageData, int16_t pixelX, int16_t pixelY, AZ::Aabb& region)
    {
        // This adds an AABB representing the size of one pixel in local space.
        // This method calculates the pixel's location from the bottom right corner of the local bounds.

        // Get the local bounds of the image gradient.
        const AZ::Aabb localBounds = imageData.m_gradientTransform.GetBounds();

        // ShiftedPixel* contains the number of pixels to offset from the first pixel in the top left corner.
        // The double-mod is used to wrap around any negative results that can occur with certain combinations of tiling
        // and frequency zoom settings.

        int16_t shiftedPixelX = (imageData.m_bottomRightPixelIndex.first - pixelX) % imageData.m_imageWidth;
        shiftedPixelX = (shiftedPixelX + imageData.m_imageWidth) % imageData.m_imageWidth;

        int16_t shiftedPixelY = (imageData.m_bottomRightPixelIndex.second - pixelY) % imageData.m_imageHeight;
        shiftedPixelY = (shiftedPixelY + imageData.m_imageHeight) % imageData.m_imageHeight;

        // X pixels run left to right (min to max), but Y pixels run top to bottom (max to min), so we account for that
        // in the math below.
        region.AddPoint(AZ::Vector3(
            localBounds.GetMax().GetX() - (imageData.m_localMetersPerPixelX * shiftedPixelX),
            localBounds.GetMin().GetY() + (imageData.m_localMetersPerPixelY * shiftedPixelY),
            0.0f));
        region.AddPoint(AZ::Vector3(
            localBounds.GetMax().GetX() - (imageData.m_localMetersPerPixelX * (shiftedPixelX + 1)),
            localBounds.GetMin().GetY() + (imageData.m_localMetersPerPixelY * shiftedPixelY),
            0.0f));
        region.AddPoint(AZ::Vector3(
            localBounds.GetMax().GetX() - (imageData.m_localMetersPerPixelX * shiftedPixelX),
            localBounds.GetMin().GetY() + (imageData.m_localMetersPerPixelY * (shiftedPixelY + 1)),
            0.0f));
        region.AddPoint(AZ::Vector3(
            localBounds.GetMax().GetX() - (imageData.m_localMetersPerPixelX * (shiftedPixelX + 1)),
            localBounds.GetMin().GetY() + (imageData.m_localMetersPerPixelY * (shiftedPixelY + 1)),
            0.0f));
    };

    AZ::Aabb ModifiedImageRegion::GetDirtyRegion()
    {
        // If the image hasn't been modified, return an invalid/unbounded dirty region.
        if (!m_isModified)
        {
            return AZ::Aabb::CreateNull();
        }

        // If the wrapping type uses infinite image repeats, by definition we need to have an unbounded dirty region.
        switch (m_imageData.m_gradientTransform.GetWrappingType())
        {
        case WrappingType::Mirror:
        case WrappingType::None:
        case WrappingType::Repeat:
            return AZ::Aabb::CreateNull();
        }

        // Our input dirty region is finite, and our wrapping type clamps to the image gradient's shape boundary,
        // which means that we can potentially have a finite output dirty region as well.
        // We just need to handle indirect effects caused by the painting:
        // - Image repeats within the shape boundary means that we need the dirty region to encompass all of the repeating changed data.
        // - Changing an edge pixel for ClampToEdge affects an infinite range stretching out from that pixel.
        // - The dirty region needs to expand to add a buffer for bilinear filtering.

        const AZ::Matrix3x4 gradientTransformMatrix = m_imageData.m_gradientTransform.GetTransformMatrix();

        // Create a local space AABB for our modified region based on the min/max pixels. We add the min/max pixels offset from
        // both the top left and the bottom right corners to account for any tiling and frequency zoom settings that make the pixels
        // appear multiple times in the image.
        AZ::Aabb modifiedRegionLocal = AZ::Aabb::CreateNull();
        AddLocalSpacePixelAabbFromTopLeft(
            m_imageData, m_minModifiedPixelIndex.first, m_minModifiedPixelIndex.second, modifiedRegionLocal);
        AddLocalSpacePixelAabbFromBottomRight(
            m_imageData, m_minModifiedPixelIndex.first, m_minModifiedPixelIndex.second, modifiedRegionLocal);
        AddLocalSpacePixelAabbFromTopLeft(
            m_imageData, m_maxModifiedPixelIndex.first, m_maxModifiedPixelIndex.second, modifiedRegionLocal);
        AddLocalSpacePixelAabbFromBottomRight(
            m_imageData, m_maxModifiedPixelIndex.first, m_maxModifiedPixelIndex.second, modifiedRegionLocal);

        AZ::Aabb expandedDirtyRegionLocal(modifiedRegionLocal);

        // If our wrapping type is ClampToEdge, check for intersections between the modified region and the image gradient bounds.
        // Any modifications that occur on the edge of the image gradient will affect all positions outward infinitely from that edge,
        // so we need to make the dirtyRegion stretch infinitely in that direction.
        if (m_imageData.m_gradientTransform.GetWrappingType() == WrappingType::ClampToEdge)
        {
            // If we modified the leftmost pixel, stretch left to -inf in X.
            if (m_modifiedLeftEdge)
            {
                expandedDirtyRegionLocal.SetMin(AZ::Vector3(
                    AZStd::numeric_limits<float>::lowest(),
                    expandedDirtyRegionLocal.GetMin().GetY(),
                    expandedDirtyRegionLocal.GetMin().GetZ()));
            }

            // If we modified the rightmost pixel, stretch right to +inf in X.
            if (m_modifiedRightEdge)
            {
                expandedDirtyRegionLocal.SetMax(AZ::Vector3(
                    AZStd::numeric_limits<float>::max(),
                    expandedDirtyRegionLocal.GetMax().GetY(),
                    expandedDirtyRegionLocal.GetMax().GetZ()));
            }

            // If we modified the bottommost pixel, stretch down to -inf in Y.
            if (m_modifiedBottomEdge)
            {
                expandedDirtyRegionLocal.SetMin(AZ::Vector3(
                    expandedDirtyRegionLocal.GetMin().GetX(),
                    AZStd::numeric_limits<float>::lowest(),
                    expandedDirtyRegionLocal.GetMin().GetZ()));
            }

            // If we modified the topmost pixel, stretch up to +inf in Y.
            if (m_modifiedTopEdge)
            {
                expandedDirtyRegionLocal.SetMax(AZ::Vector3(
                    expandedDirtyRegionLocal.GetMax().GetX(),
                    AZStd::numeric_limits<float>::max(),
                    expandedDirtyRegionLocal.GetMax().GetZ()));
            }
        }

        // Because Image Gradients support bilinear filtering, we need to expand our dirty area by an extra pixel in each direction
        // so that the effects of the painted values on adjacent pixels are taken into account when refreshing.
        expandedDirtyRegionLocal.Expand(AZ::Vector3(m_imageData.m_localMetersPerPixelX, m_imageData.m_localMetersPerPixelY, 0.0f));

        // Finally, transform the dirty region back into world space and
        // set it to encompass the full Z range since image gradients are 2D.
        AZ::Aabb expandedDirtyRegion = expandedDirtyRegionLocal.GetTransformedAabb(gradientTransformMatrix);
        expandedDirtyRegion.Set(
            AZ::Vector3(expandedDirtyRegion.GetMin().GetX(), expandedDirtyRegion.GetMin().GetY(), AZStd::numeric_limits<float>::lowest()),
            AZ::Vector3(expandedDirtyRegion.GetMax().GetX(), expandedDirtyRegion.GetMax().GetY(), AZStd::numeric_limits<float>::max()));

        return expandedDirtyRegion;
    }


    ImageGradientModifier::ImageGradientModifier(
        const AZ::EntityComponentIdPair& entityComponentIdPair)
        : m_ownerEntityComponentId(entityComponentIdPair)
    {
        AzFramework::PaintBrushNotificationBus::Handler::BusConnect(entityComponentIdPair);

        auto entityId = entityComponentIdPair.GetEntityId();

        // Get the gradient transform. We'll need this to update the dirty region appropriately.
        GradientTransformRequestBus::EventResult(
            m_imageData.m_gradientTransform, entityId, &GradientTransformRequests::GetGradientTransform);

        // Get the spacing to map individual pixels to world space positions.
        AZ::Vector2 imagePixelsPerMeter(0.0f);
        ImageGradientRequestBus::EventResult(imagePixelsPerMeter, entityId, &ImageGradientRequestBus::Events::GetImagePixelsPerMeter);

        // Meters Per Pixel is in world space, so it takes into account the bounds, tiling, frequency zoom, and scale parameters.
        m_imageData.m_metersPerPixelX = (imagePixelsPerMeter.GetX() > 0.0f) ? (1.0f / imagePixelsPerMeter.GetX()) : 0.0f;
        m_imageData.m_metersPerPixelY = (imagePixelsPerMeter.GetY() > 0.0f) ? (1.0f / imagePixelsPerMeter.GetY()) : 0.0f;

        // Since scaling takes place outside of the image's local space, but tiling & frequency zoom take place inside the image's
        // local space, we'll create a version of meters per pixel without scaling applied so that we can calculate pixel sizes
        // when working in local space.
        m_imageData.m_localMetersPerPixelX = m_imageData.m_metersPerPixelX / m_imageData.m_gradientTransform.GetScale().GetX();
        m_imageData.m_localMetersPerPixelY = m_imageData.m_metersPerPixelY / m_imageData.m_gradientTransform.GetScale().GetY();

        // Get the image width and height in pixels. We'll use these to calculate the pixel indices for the image borders and also
        // to verify that the image is valid to modify.
        uint32_t imageWidth = 0;
        uint32_t imageHeight = 0;
        ImageGradientRequestBus::EventResult(imageWidth, entityId, &ImageGradientRequestBus::Events::GetImageWidth);
        ImageGradientRequestBus::EventResult(imageHeight, entityId, &ImageGradientRequestBus::Events::GetImageHeight);
        m_imageData.m_imageWidth = aznumeric_cast<int16_t>(imageWidth);
        m_imageData.m_imageHeight = aznumeric_cast<int16_t>(imageHeight);

        // Get the image tiling values. These are used to calculate the pixel indices for the image borders.
        float imageTilingX = 1.0f;
        float imageTilingY = 1.0f;
        ImageGradientRequestBus::EventResult(imageTilingX, entityId, &ImageGradientRequestBus::Events::GetTilingX);
        ImageGradientRequestBus::EventResult(imageTilingY, entityId, &ImageGradientRequestBus::Events::GetTilingY);

        // Get the normalized UVW values for the image gradient at the corners. Note that "min UVW" is the bottom left corner of the
        // AABB and "max UVW" is the top right corner. Depending on tiling, we can get numbers outside the 0-1 range in both the positive
        // and negative directions.
        AZ::Vector3 bottomLeftUvw;
        AZ::Vector3 topRightUvw;
        m_imageData.m_gradientTransform.GetMinMaxUvwValuesNormalized(bottomLeftUvw, topRightUvw);

        // Calculate min/max pixel values at the boundaries. Depending on tiling, these can be negative or positive, and they might
        // fall outside the number of pixels in the image. They will need to be modded to get back into pixel index range.
        const float leftImagePixelX = m_imageData.m_imageWidth * imageTilingX * bottomLeftUvw.GetX();
        const float bottomImagePixelY = m_imageData.m_imageHeight * imageTilingY * bottomLeftUvw.GetY();
        const float rightImagePixelX = m_imageData.m_imageWidth * imageTilingX * topRightUvw.GetX();
        const float topImagePixelY = m_imageData.m_imageHeight * imageTilingY * topRightUvw.GetY();

        // Calculate the pixel indices for each boundary pixel by double-modding. The second mod is to wrap negative values back into
        // the positive value range.

        m_imageData.m_topLeftPixelIndex.first = aznumeric_cast<int64_t>(leftImagePixelX) % m_imageData.m_imageWidth;
        m_imageData.m_topLeftPixelIndex.first =
            (m_imageData.m_topLeftPixelIndex.first + m_imageData.m_imageWidth) % m_imageData.m_imageWidth;

        m_imageData.m_topLeftPixelIndex.second = aznumeric_cast<int64_t>(topImagePixelY) % m_imageData.m_imageHeight;
        m_imageData.m_topLeftPixelIndex.second =
            (m_imageData.m_topLeftPixelIndex.second + m_imageData.m_imageHeight) % m_imageData.m_imageHeight;

        m_imageData.m_bottomRightPixelIndex.first = aznumeric_cast<int64_t>(rightImagePixelX) % m_imageData.m_imageWidth;
        m_imageData.m_bottomRightPixelIndex.first =
            (m_imageData.m_bottomRightPixelIndex.first + m_imageData.m_imageWidth) % m_imageData.m_imageWidth;

        m_imageData.m_bottomRightPixelIndex.second = aznumeric_cast<int64_t>(bottomImagePixelY) % m_imageData.m_imageHeight;
        m_imageData.m_bottomRightPixelIndex.second =
            (m_imageData.m_bottomRightPixelIndex.second + m_imageData.m_imageHeight) % m_imageData.m_imageHeight;

        // X pixels go from min to max (left to right), but Y pixels go from max to min (top to bottom), so flip the index
        // values to account for this.
        m_imageData.m_topLeftPixelIndex.second = m_imageData.m_imageHeight - m_imageData.m_topLeftPixelIndex.second - 1;
        m_imageData.m_bottomRightPixelIndex.second = m_imageData.m_imageHeight - m_imageData.m_bottomRightPixelIndex.second - 1;
    }

    ImageGradientModifier::~ImageGradientModifier()
    {
        AzFramework::PaintBrushNotificationBus::Handler::BusDisconnect();
    }

    void ImageGradientModifier::OnBrushStrokeBegin([[maybe_unused]] const AZ::Color& color)
    {
        AZ::EntityId entityId = m_ownerEntityComponentId.GetEntityId();

        ImageGradientModificationNotificationBus::Event(
            entityId, &ImageGradientModificationNotificationBus::Events::OnImageGradientBrushStrokeBegin);

        // We can't create a stroke buffer if there isn't any pixel data.
        if ((m_imageData.m_imageWidth == 0) || (m_imageData.m_imageHeight == 0))
        {
            return;
        }

        // Create the buffer for holding all the changes for a single continuous paint brush stroke.
        // This buffer will get used during the stroke to hold our accumulated stroke opacity layer,
        // and then after the stroke finishes we'll hand the buffer over to the undo system as an undo/redo buffer.
        m_paintStrokeData.m_strokeBuffer =
            AZStd::make_shared<ImageTileBuffer>(m_imageData.m_imageWidth, m_imageData.m_imageHeight, entityId);

        m_modifiedStrokeRegion = ModifiedImageRegion(m_imageData);
    }

    void ImageGradientModifier::OnBrushStrokeEnd()
    {
        const AZ::EntityId entityId = m_ownerEntityComponentId.GetEntityId();
        const AZ::Aabb dirtyRegion = m_modifiedStrokeRegion.GetDirtyRegion();

        ImageGradientModificationNotificationBus::Event(
            entityId,
            &ImageGradientModificationNotificationBus::Events::OnImageGradientBrushStrokeEnd,
            m_paintStrokeData.m_strokeBuffer,
            dirtyRegion);

        // Make sure we've cleared out our paint stroke and dirty region data until the next paint stroke begins.
        m_paintStrokeData = {};
        m_modifiedStrokeRegion = {};
    }

    AZ::Color ImageGradientModifier::OnGetColor(const AZ::Vector3& brushCenter) const
    {
        AZ::EntityId entityId = m_ownerEntityComponentId.GetEntityId();

        // Get the gradient value at the given point.
        // We use "GetPixelValuesByPosition" instead of "GetGradientValue" because we want to select unscaled, unsmoothed values.
        float gradientValue = 0.0f;
        ImageGradientModificationBus::Event(
            entityId,
            &ImageGradientModificationBus::Events::GetPixelValuesByPosition,
            AZStd::span<const AZ::Vector3>(&brushCenter, 1),
            AZStd::span<float>(&gradientValue, 1));

        return AZ::Color(gradientValue, gradientValue, gradientValue, 1.0f);
    }

    void ImageGradientModifier::OnPaintSmoothInternal(
        const AZ::Aabb& dirtyArea,
        ValueLookupFn& valueLookupFn,
        AZStd::function<float(const AZ::Vector3& worldPosition, float gradientValue, float opacity)> combineFn)
    {
        ModifiedImageRegion modifiedRegion(m_imageData);

        // We're either painting or smoothing new values into our image gradient.
        // To do this, we need to calculate the set of world space positions that map to individual pixels in the image,
        // then ask the paint brush for each position what value we should set that pixel to. Finally, we use those modified
        // values to change the image gradient.

        const AZ::Matrix3x4 gradientTransformMatrix = m_imageData.m_gradientTransform.GetTransformMatrix();
        AZ::Aabb dirtyAreaLocal = dirtyArea.GetTransformedAabb(gradientTransformMatrix.GetInverseFull());

        const int32_t xPoints = aznumeric_cast<int32_t>(dirtyAreaLocal.GetXExtent() / m_imageData.m_localMetersPerPixelX);
        const int32_t yPoints = aznumeric_cast<int32_t>(dirtyAreaLocal.GetYExtent() / m_imageData.m_localMetersPerPixelY);

        // Early out if the dirty area is smaller than our point size.
        if ((xPoints <= 0) || (yPoints <= 0))
        {
            return;
        }

        // Calculate the minimum set of world space points that map to those pixels.
        AZStd::vector<AZ::Vector3> points;
        points.reserve(xPoints * yPoints);
        for (float y = dirtyAreaLocal.GetMin().GetY() + (m_imageData.m_localMetersPerPixelY / 2.0f);
            y <= dirtyAreaLocal.GetMax().GetY();
            y += m_imageData.m_localMetersPerPixelY)
        {
            for (float x = dirtyAreaLocal.GetMin().GetX() + (m_imageData.m_localMetersPerPixelX / 2.0f);
                 x <= dirtyAreaLocal.GetMax().GetX();
                 x += m_imageData.m_localMetersPerPixelX)
            {
                AZ::Vector3 worldPoint(gradientTransformMatrix * AZ::Vector3(x, y, 0.0f));
                worldPoint.SetZ(dirtyArea.GetMin().GetZ());
                points.push_back(worldPoint);
            }
        }

        // Query the paintbrush with those points to get back the subset of points and brush opacities for each point that's
        // affected by the brush.
        AZStd::vector<AZ::Vector3> validPoints;
        AZStd::vector<float> perPixelOpacities;
        valueLookupFn(points, validPoints, perPixelOpacities);

        // Early out if none of the points were actually affected by the brush.
        if (validPoints.empty())
        {
            return;
        }

        AZ::EntityId entityId = m_ownerEntityComponentId.GetEntityId();

        // Get the pixel indices for each position.
        AZStd::vector<PixelIndex> pixelIndices(validPoints.size());
        ImageGradientModificationBus::Event(
            entityId, &ImageGradientModificationBus::Events::GetPixelIndicesForPositions, validPoints, pixelIndices);

        // Create a buffer for all of the modified, blended gradient values.
        AZStd::vector<float> paintedValues;
        paintedValues.reserve(pixelIndices.size());

        // For each pixel, accumulate the per-pixel opacity in the stroke layer, then (re)blend the stroke layer with
        // the original data by using the stroke intensity, stroke opacity, per-pixel opacity, and original pre-stroke gradient value.
        // The (re)blended value gets sent immediately to the image gradient, as well as getting cached off into the stroke buffer
        // for easier and faster undo/redo operations.
        for (size_t index = 0; index < pixelIndices.size(); index++)
        {
            // If we have an invalid pixel index, fill in a placeholder value into paintedValues and move on to the next pixel.
            if ((pixelIndices[index].first < 0) || (pixelIndices[index].second < 0))
            {
                paintedValues.emplace_back(0.0f);
                continue;
            }

            auto [gradientValue, opacityValue] = m_paintStrokeData.m_strokeBuffer->GetOriginalPixelValueAndOpacity(pixelIndices[index]);

            // Add the new per-pixel opacity to the existing opacity in our stroke layer.
            opacityValue = AZStd::clamp(opacityValue + (1.0f - opacityValue) * perPixelOpacities[index], 0.0f, 1.0f);

            // Combine the pixel (either paint or smooth) and store the blended pixel and new opacity back into our paint stroke buffer.
            float blendedValue = combineFn(validPoints[index], gradientValue, opacityValue);
            m_paintStrokeData.m_strokeBuffer->SetModifiedPixelValue(pixelIndices[index], blendedValue, opacityValue);

            // Also store the blended value into a second buffer that we'll use to immediately modify the image gradient.
            paintedValues.emplace_back(blendedValue);

            // Track the data needed for calculating the dirty region for this specific operation as well as for the overall brush stroke.
            modifiedRegion.AddPoint(pixelIndices[index]);
            m_modifiedStrokeRegion.AddPoint(pixelIndices[index]);
        }

        // Modify the image gradient with all of the changed values
        ImageGradientModificationBus::Event(
            entityId, &ImageGradientModificationBus::Events::SetPixelValuesByPixelIndex, pixelIndices, paintedValues);

        // Get the dirty region that actually encompasses everything that we directly modified,
        // along with everything it indirectly affected.
        if (modifiedRegion.IsModified())
        {
            AZ::Aabb expandedDirtyArea = modifiedRegion.GetDirtyRegion();

            // Notify anything listening to the image gradient that the modified region has changed.
            LmbrCentral::DependencyNotificationBus::Event(
                entityId, &LmbrCentral::DependencyNotificationBus::Events::OnCompositionRegionChanged, expandedDirtyArea);
        }
    }

    void ImageGradientModifier::OnPaint(const AZ::Color& color, const AZ::Aabb& dirtyArea, ValueLookupFn& valueLookupFn, BlendFn& blendFn)
    {
        const float intensity = color.GetR();
        const float opacity = color.GetA();

        // For paint notifications, we'll use the given blend function to blend the original value and the paint brush intensity
        // using the built-up opacity.
        auto combineFn = [intensity, opacity, blendFn](
            [[maybe_unused]] const AZ::Vector3& worldPosition, float gradientValue, float opacityValue) -> float
        {
            return blendFn(gradientValue, intensity, opacityValue * opacity);
        };

        // Perform all the common logic between painting and smoothing to modify our image gradient.
        OnPaintSmoothInternal(dirtyArea, valueLookupFn, combineFn);
    }

    void ImageGradientModifier::OnSmooth(
        const AZ::Color& color,
        const AZ::Aabb& dirtyArea,
        ValueLookupFn& valueLookupFn,
        AZStd::span<const AZ::Vector3> valuePointOffsets,
        SmoothFn& smoothFn)
    {
        const float opacity = color.GetA();

        AZ::EntityId entityId = m_ownerEntityComponentId.GetEntityId();

        // Declare our vectors of kernel point locations and values once outside of the combine function so that we
        // don't keep reallocating them on every point.
        AZStd::vector<AZ::Vector3> kernelPoints;
        AZStd::vector<float> kernelValues;

        const AZ::Vector3 valuePointOffsetScale(m_imageData.m_metersPerPixelX, m_imageData.m_metersPerPixelY, 0.0f);

        kernelPoints.reserve(valuePointOffsets.size());
        kernelValues.reserve(valuePointOffsets.size());

        // For smoothing notifications, we'll need to gather all of the neighboring gradient values to feed into the given smoothing
        // function for our blend operation.
        auto combineFn = [entityId, opacity, smoothFn, &valuePointOffsets, valuePointOffsetScale, &kernelPoints, &kernelValues](
                             const AZ::Vector3& worldPosition, float gradientValue, float opacityValue) -> float
        {
            kernelPoints.clear();

            // Calculate all of the world positions around our base position that we'll use for fetching our blurring kernel values.
            for (auto& valuePointOffset : valuePointOffsets)
            {
                kernelPoints.emplace_back(worldPosition + (valuePointOffset * valuePointOffsetScale));
            }

            kernelValues.assign(kernelPoints.size(), 0.0f);

            // Read all of the original gradient values for the blurring kernel into the buffer.
            ImageGradientModificationBus::Event(
                entityId, &ImageGradientModificationBus::Events::GetPixelValuesByPosition, kernelPoints, kernelValues);

            // Blend all the blurring kernel values together and store the blended pixel and new opacity back into our paint stroke buffer.
            return smoothFn(gradientValue, kernelValues, opacityValue * opacity);
        };

        // Perform all the common logic between painting and smoothing to modify our image gradient.
        OnPaintSmoothInternal(dirtyArea, valueLookupFn, combineFn);
    }



} // namespace GradientSignal
