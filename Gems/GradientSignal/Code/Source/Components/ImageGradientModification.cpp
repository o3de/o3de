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

    ImageGradientModifier::ImageGradientModifier(
        const AZ::EntityComponentIdPair& entityComponentIdPair)
        : m_ownerEntityComponentId(entityComponentIdPair)
    {
        AzFramework::PaintBrushNotificationBus::Handler::BusConnect(entityComponentIdPair);
    }

    ImageGradientModifier::~ImageGradientModifier()
    {
        AzFramework::PaintBrushNotificationBus::Handler::BusDisconnect();
    }

    void ImageGradientModifier::OnBrushStrokeBegin(const AZ::Color& color)
    {
        AZ::EntityId entityId = m_ownerEntityComponentId.GetEntityId();

        ImageGradientModificationNotificationBus::Event(
            entityId, &ImageGradientModificationNotificationBus::Events::OnImageGradientBrushStrokeBegin);

        // Get the spacing to map individual pixels to world space positions.
        AZ::Vector2 imagePixelsPerMeter(0.0f);
        ImageGradientRequestBus::EventResult(imagePixelsPerMeter, entityId, &ImageGradientRequestBus::Events::GetImagePixelsPerMeter);
        if ((imagePixelsPerMeter.GetX() <= 0.0f) || (imagePixelsPerMeter.GetY() <= 0.0f))
        {
            return;
        }

        m_paintStrokeData.m_intensity = color.GetR();
        m_paintStrokeData.m_opacity = color.GetA();

        m_paintStrokeData.m_metersPerPixelX = 1.0f / imagePixelsPerMeter.GetX();
        m_paintStrokeData.m_metersPerPixelY = 1.0f / imagePixelsPerMeter.GetY();

        uint32_t imageWidth = 0;
        ImageGradientRequestBus::EventResult(imageWidth, entityId, &ImageGradientRequestBus::Events::GetImageWidth);

        uint32_t imageHeight = 0;
        ImageGradientRequestBus::EventResult(imageHeight, entityId, &ImageGradientRequestBus::Events::GetImageHeight);

        // Create the buffer for holding all the changes for a single continuous paint brush stroke.
        // This buffer will get used during the stroke to hold our accumulated stroke opacity layer,
        // and then after the stroke finishes we'll hand the buffer over to the undo system as an undo/redo buffer.
        m_paintStrokeData.m_strokeBuffer = AZStd::make_shared<ImageTileBuffer>(imageWidth, imageHeight, entityId);
    }

    void ImageGradientModifier::OnBrushStrokeEnd()
    {
        AZ::EntityId entityId = m_ownerEntityComponentId.GetEntityId();

        if (m_paintStrokeData.m_dirtyRegion.IsValid())
        {
            // Expand the dirty region for this brush stroke by one pixel in each direction
            // to account for any data affected by bilinear filtering as well.
            m_paintStrokeData.m_dirtyRegion.Expand(
                AZ::Vector3(m_paintStrokeData.m_metersPerPixelX, m_paintStrokeData.m_metersPerPixelY, 0.0f));

            // Expand the dirty region to encompass the full Z range since image gradients are 2D.
            auto dirtyRegionMin = m_paintStrokeData.m_dirtyRegion.GetMin();
            auto dirtyRegionMax = m_paintStrokeData.m_dirtyRegion.GetMax();
            m_paintStrokeData.m_dirtyRegion.Set(
                AZ::Vector3(dirtyRegionMin.GetX(), dirtyRegionMin.GetY(), AZStd::numeric_limits<float>::lowest()),
                AZ::Vector3(dirtyRegionMax.GetX(), dirtyRegionMax.GetY(), AZStd::numeric_limits<float>::max()));
        }

        ImageGradientModificationNotificationBus::Event(
            entityId,
            &ImageGradientModificationNotificationBus::Events::OnImageGradientBrushStrokeEnd,
            m_paintStrokeData.m_strokeBuffer,
            m_paintStrokeData.m_dirtyRegion);

        // Make sure we've cleared out our paint stroke data until the next paint stroke begins.
        m_paintStrokeData = {};
    }

    AZ::Color ImageGradientModifier::OnGetColor(const AZ::Vector3& brushCenter)
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
        // We're either painting or smoothing new values into our image gradient.
        // To do this, we need to calculate the set of world space positions that map to individual pixels in the image,
        // then ask the paint brush for each position what value we should set that pixel to. Finally, we use those modified
        // values to change the image gradient.

        const AZ::Vector3 minDistances = dirtyArea.GetMin();
        const AZ::Vector3 maxDistances = dirtyArea.GetMax();
        const float zMinDistance = minDistances.GetZ();

        const int32_t xPoints = aznumeric_cast<int32_t>((maxDistances.GetX() - minDistances.GetX()) / m_paintStrokeData.m_metersPerPixelX);
        const int32_t yPoints = aznumeric_cast<int32_t>((maxDistances.GetY() - minDistances.GetY()) / m_paintStrokeData.m_metersPerPixelY);

        // Early out if the dirty area is smaller than our point size.
        if ((xPoints <= 0) || (yPoints <= 0))
        {
            return;
        }

        // Calculate the minimum set of world space points that map to those pixels.
        AZStd::vector<AZ::Vector3> points;
        points.reserve(xPoints * yPoints);
        for (float y = minDistances.GetY(); y <= maxDistances.GetY(); y += m_paintStrokeData.m_metersPerPixelY)
        {
            for (float x = minDistances.GetX(); x <= maxDistances.GetX(); x += m_paintStrokeData.m_metersPerPixelX)
            {
                points.emplace_back(x, y, zMinDistance);
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

            // Track the overall dirty region for everything we modify so that we don't have to recalculate it for undos/redos.
            m_paintStrokeData.m_dirtyRegion.AddPoint(validPoints[index]);
        }

        // Modify the image gradient with all of the changed values
        ImageGradientModificationBus::Event(
            entityId, &ImageGradientModificationBus::Events::SetPixelValuesByPixelIndex, pixelIndices, paintedValues);

        // Because Image Gradients support bilinear filtering, we need to expand our dirty area by an extra pixel in each direction
        // so that the effects of the painted values on adjacent pixels are taken into account when refreshing.
        AZ::Aabb expandedDirtyArea(dirtyArea);
        expandedDirtyArea.Expand(AZ::Vector3(m_paintStrokeData.m_metersPerPixelX, m_paintStrokeData.m_metersPerPixelY, 0.0f));

        // Expand the dirty region to encompass the full Z range since image gradients are 2D.
        auto dirtyRegionMin = expandedDirtyArea.GetMin();
        auto dirtyRegionMax = expandedDirtyArea.GetMax();
        expandedDirtyArea.Set(
            AZ::Vector3(dirtyRegionMin.GetX(), dirtyRegionMin.GetY(), AZStd::numeric_limits<float>::lowest()),
            AZ::Vector3(dirtyRegionMax.GetX(), dirtyRegionMax.GetY(), AZStd::numeric_limits<float>::max()));

        // Notify anything listening to the image gradient that the modified region has changed.
        LmbrCentral::DependencyNotificationBus::Event(
            entityId, &LmbrCentral::DependencyNotificationBus::Events::OnCompositionRegionChanged, expandedDirtyArea);
    }

    void ImageGradientModifier::OnPaint(const AZ::Aabb& dirtyArea, ValueLookupFn& valueLookupFn, BlendFn& blendFn)
    {
        // For paint notifications, we'll use the given blend function to blend the original value and the paint brush intensity
        // using the built-up opacity.
        auto combineFn = [this,
                          blendFn]([[maybe_unused]] const AZ::Vector3& worldPosition, float gradientValue, float opacityValue) -> float
        {
            return blendFn(gradientValue, m_paintStrokeData.m_intensity, opacityValue * m_paintStrokeData.m_opacity);
        };

        // Perform all the common logic between painting and smoothing to modify our image gradient.
        OnPaintSmoothInternal(dirtyArea, valueLookupFn, combineFn);
    }

    void ImageGradientModifier::OnSmooth(
        const AZ::Aabb& dirtyArea, ValueLookupFn& valueLookupFn, AZStd::span<const AZ::Vector3> valuePointOffsets, SmoothFn& smoothFn)
    {
        AZ::EntityId entityId = m_ownerEntityComponentId.GetEntityId();

        // Declare our vectors of kernel point locations and values once outside of the combine function so that we
        // don't keep reallocating them on every point.
        AZStd::vector<AZ::Vector3> kernelPoints;
        AZStd::vector<float> kernelValues;

        const AZ::Vector3 valuePointOffsetScale(m_paintStrokeData.m_metersPerPixelX, m_paintStrokeData.m_metersPerPixelY, 0.0f);

        kernelPoints.reserve(valuePointOffsets.size());
        kernelValues.reserve(valuePointOffsets.size());

        // For smoothing notifications, we'll need to gather all of the neighboring gradient values to feed into the given smoothing
        // function for our blend operation.
        auto combineFn = [this, entityId, smoothFn, &valuePointOffsets, valuePointOffsetScale, &kernelPoints, &kernelValues](
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
            return smoothFn(gradientValue, kernelValues, opacityValue * m_paintStrokeData.m_opacity);
        };

        // Perform all the common logic between painting and smoothing to modify our image gradient.
        OnPaintSmoothInternal(dirtyArea, valueLookupFn, combineFn);
    }



} // namespace GradientSignal
