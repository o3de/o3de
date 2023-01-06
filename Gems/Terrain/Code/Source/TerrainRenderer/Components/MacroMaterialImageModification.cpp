/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <TerrainRenderer/Components/MacroMaterialImageModification.h>
#include <LmbrCentral/Dependency/DependencyNotificationBus.h>
#include <TerrainSystem/TerrainSystemBus.h>

namespace Terrain
{
    ImageTileBuffer::ImageTileBuffer(uint32_t imageWidth, uint32_t imageHeight, AZ::EntityId modifiedEntityId)
        : m_modifiedEntityId(modifiedEntityId)
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

    AZStd::pair<AZ::Color, float> ImageTileBuffer::GetOriginalPixelValueAndOpacity(const PixelIndex& pixelIndex)
    {
        uint32_t tileIndex = GetTileIndex(pixelIndex);
        uint32_t pixelTileIndex = GetPixelTileIndex(pixelIndex);

        // Create the tile if it doesn't already exist.
        // We lazy-create the tile on reads as well as writes because reading the original pixel value isn't necessarily very cheap
        // and we may need to re-read the same pixel multiple times for things like smoothing operations.
        CreateImageTile(tileIndex);

        return { m_paintedImageTiles[tileIndex]->m_unmodifiedData[pixelTileIndex],
                    m_paintedImageTiles[tileIndex]->m_modifiedDataOpacity[pixelTileIndex] };
    }

    void ImageTileBuffer::SetModifiedPixelValue(const PixelIndex& pixelIndex, AZ::Color modifiedValue, float opacity)
    {
        uint32_t tileIndex = GetTileIndex(pixelIndex);
        uint32_t pixelTileIndex = GetPixelTileIndex(pixelIndex);

        // Create the tile if it doesn't already exist.
        CreateImageTile(tileIndex);

        m_paintedImageTiles[tileIndex]->m_modifiedData[pixelTileIndex] = modifiedValue;
        m_paintedImageTiles[tileIndex]->m_modifiedDataOpacity[pixelTileIndex] = opacity;
    }

    void ImageTileBuffer::ApplyChangeBuffer(bool undo)
    {
        AZStd::array<PixelIndex, ImageTileSize * ImageTileSize> pixelIndices;

        TerrainMacroColorModificationBus::Event(
            m_modifiedEntityId, &TerrainMacroColorModificationBus::Events::StartMacroColorPixelModifications);

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

            // Set the pixel values for this tile either to the original or the modified values.
            // It's possible that not every pixel in the tile was modified, but it's cheaper just to update per-tile
            // than to track each individual pixel in the tile and set them individually.
            TerrainMacroColorModificationBus::Event(
                m_modifiedEntityId,
                &TerrainMacroColorModificationBus::Events::SetMacroColorPixelValuesByPixelIndex,
                pixelIndices,
                undo ? m_paintedImageTiles[tileIndex]->m_unmodifiedData : m_paintedImageTiles[tileIndex]->m_modifiedData);
        }

        TerrainMacroColorModificationBus::Event(
            m_modifiedEntityId, &TerrainMacroColorModificationBus::Events::EndMacroColorPixelModifications);
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

        // Read all of the original pixel values into the image tile buffer.
        TerrainMacroColorModificationBus::Event(
            m_modifiedEntityId,
            &TerrainMacroColorModificationBus::Events::GetMacroColorPixelValuesByPixelIndex,
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

    ModifiedImageRegion::ModifiedImageRegion(const MacroMaterialImageSizeData& imageData)
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

        // Track that we've modified at least one pixel.
        m_isModified = true;
    }

    void ModifiedImageRegion::AddPixelAabb(const MacroMaterialImageSizeData& imageData, int16_t pixelX, int16_t pixelY, AZ::Aabb& region)
    {
        // This adds an AABB representing the size of one pixel in local space.
        // This method calculates the pixel's location from the top left corner of the local bounds.

        // Get the local bounds of the image gradient.
        const AZ::Aabb localBounds = imageData.m_macroMaterialBounds;

        int16_t shiftedPixelX = pixelX;
        int16_t shiftedPixelY = pixelY;

        // X pixels run left to right (min to max), but Y pixels run top to bottom (max to min), so we account for that
        // in the math below (it's why we do "min X +" and "max Y -").
        region.AddPoint(AZ::Vector3(
            localBounds.GetMin().GetX() + (imageData.m_metersPerPixelX * shiftedPixelX),
            localBounds.GetMax().GetY() - (imageData.m_metersPerPixelY * shiftedPixelY),
            0.0f));
        region.AddPoint(AZ::Vector3(
            localBounds.GetMin().GetX() + (imageData.m_metersPerPixelX * (shiftedPixelX + 1)),
            localBounds.GetMax().GetY() - (imageData.m_metersPerPixelY * shiftedPixelY),
            0.0f));
        region.AddPoint(AZ::Vector3(
            localBounds.GetMin().GetX() + (imageData.m_metersPerPixelX * shiftedPixelX),
            localBounds.GetMax().GetY() - (imageData.m_metersPerPixelY * (shiftedPixelY + 1)),
            0.0f));
        region.AddPoint(AZ::Vector3(
            localBounds.GetMin().GetX() + (imageData.m_metersPerPixelX * (shiftedPixelX + 1)),
            localBounds.GetMax().GetY() - (imageData.m_metersPerPixelY * (shiftedPixelY + 1)),
            0.0f));
    };

    AZ::Aabb ModifiedImageRegion::GetDirtyRegion()
    {
        // If the image hasn't been modified, return an invalid/unbounded dirty region.
        if (!m_isModified)
        {
            return AZ::Aabb::CreateNull();
        }

        // Create an AABB for our modified region based on the min/max pixels that were modified
        AZ::Aabb modifiedRegion = AZ::Aabb::CreateNull();
        AddPixelAabb(m_imageData, m_minModifiedPixelIndex.first, m_minModifiedPixelIndex.second, modifiedRegion);
        AddPixelAabb(m_imageData, m_maxModifiedPixelIndex.first, m_maxModifiedPixelIndex.second, modifiedRegion);

        // Because macro color textures use bilinear filtering, expand the dirty area by an extra pixel in each direction
        // so that the effects of the painted values on adjacent pixels are taken into account when refreshing.
        modifiedRegion.Expand(AZ::Vector3(m_imageData.m_metersPerPixelX, m_imageData.m_metersPerPixelY, 0.0f));

        // Finally, set the region to encompass the full Z range since macro materials are effectively 2D.
        modifiedRegion.Set(
            AZ::Vector3(modifiedRegion.GetMin().GetX(), modifiedRegion.GetMin().GetY(), AZStd::numeric_limits<float>::lowest()),
            AZ::Vector3(modifiedRegion.GetMax().GetX(), modifiedRegion.GetMax().GetY(), AZStd::numeric_limits<float>::max()));

        return modifiedRegion;
    }

    MacroMaterialImageModifier::MacroMaterialImageModifier(
        const AZ::EntityComponentIdPair& entityComponentIdPair)
        : m_ownerEntityComponentId(entityComponentIdPair)
    {
        AzFramework::PaintBrushNotificationBus::Handler::BusConnect(entityComponentIdPair);

        auto entityId = entityComponentIdPair.GetEntityId();

        // Get the spacing to map individual pixels to world space positions.
        AZ::Vector2 imagePixelsPerMeter(0.0f);
        TerrainMacroMaterialRequestBus::EventResult(
            imagePixelsPerMeter, entityId, &TerrainMacroMaterialRequestBus::Events::GetMacroColorImagePixelsPerMeter);

        // Convert from pixels per meter to meters per pixel so that we can guard for division by 0 here instead of everywhere.
        m_imageData.m_metersPerPixelX = (imagePixelsPerMeter.GetX() > 0.0f) ? (1.0f / imagePixelsPerMeter.GetX()) : 0.0f;
        m_imageData.m_metersPerPixelY = (imagePixelsPerMeter.GetY() > 0.0f) ? (1.0f / imagePixelsPerMeter.GetY()) : 0.0f;

        // Get the macro material world bounds.
        MacroMaterialData macroMaterialData;
        TerrainMacroMaterialRequestBus::EventResult(
            macroMaterialData, entityId, &TerrainMacroMaterialRequestBus::Events::GetTerrainMacroMaterialData);
        m_imageData.m_macroMaterialBounds = macroMaterialData.m_bounds;

        // Get the image width and height in pixels.
        AZ::RHI::Size imageSize;
        TerrainMacroMaterialRequestBus::EventResult(
            imageSize, entityId, &TerrainMacroMaterialRequestBus::Events::GetMacroColorImageSize);
        m_imageData.m_imageWidth = aznumeric_cast<int16_t>(imageSize.m_width);
        m_imageData.m_imageHeight = aznumeric_cast<int16_t>(imageSize.m_height);
    }

    MacroMaterialImageModifier::~MacroMaterialImageModifier()
    {
        AzFramework::PaintBrushNotificationBus::Handler::BusDisconnect();
    }

    void MacroMaterialImageModifier::OnBrushStrokeBegin([[maybe_unused]] const AZ::Color& color)
    {
        AZ::EntityId entityId = m_ownerEntityComponentId.GetEntityId();

        TerrainMacroColorModificationNotificationBus::Event(
            entityId, &TerrainMacroColorModificationNotificationBus::Events::OnTerrainMacroColorBrushStrokeBegin);

        // We can't create a stroke buffer if there isn't any pixel data.
        if ((m_imageData.m_imageWidth == 0) || (m_imageData.m_imageHeight == 0))
        {
            return;
        }

        // Create the buffer for holding all the changes for a single continuous paint brush stroke.
        // This buffer will get used during the stroke to hold our accumulated stroke opacity layer,
        // and then after the stroke finishes we'll hand the buffer over to the undo system as an undo/redo buffer.
        m_strokeBuffer =
            AZStd::make_shared<ImageTileBuffer>(m_imageData.m_imageWidth, m_imageData.m_imageHeight, entityId);

        m_modifiedStrokeRegion = ModifiedImageRegion(m_imageData);
    }

    void MacroMaterialImageModifier::OnBrushStrokeEnd()
    {
        const AZ::EntityId entityId = m_ownerEntityComponentId.GetEntityId();
        const AZ::Aabb dirtyRegion = m_modifiedStrokeRegion.GetDirtyRegion();

        TerrainMacroColorModificationNotificationBus::Event(
            entityId,
            &TerrainMacroColorModificationNotificationBus::Events::OnTerrainMacroColorBrushStrokeEnd,
            m_strokeBuffer, dirtyRegion);

        // Make sure we've cleared out our paint stroke and dirty region data until the next paint stroke begins.
        m_strokeBuffer = {};
        m_modifiedStrokeRegion = {};
    }

    AZ::Color MacroMaterialImageModifier::OnGetColor(const AZ::Vector3& brushCenter) const
    {
        AZ::EntityId entityId = m_ownerEntityComponentId.GetEntityId();

        AZ::Color color(0.0f, 0.0f, 0.0f, 1.0f);

        TerrainMacroColorModificationBus::Event(
            entityId,
            &TerrainMacroColorModificationBus::Events::GetMacroColorPixelValuesByPosition,
            AZStd::span<const AZ::Vector3>(&brushCenter, 1),
            AZStd::span<AZ::Color>(&color, 1));

        return color;
    }

    void MacroMaterialImageModifier::OnPaintSmoothInternal(
        const AZ::Aabb& dirtyArea,
        ValueLookupFn& valueLookupFn,
        AZStd::function<AZ::Color(const AZ::Vector3& worldPosition, AZ::Color gradientValue, float opacity)> combineFn)
    {
        ModifiedImageRegion modifiedRegion(m_imageData);

        // We're either painting or smoothing new values into our macro material.
        // To do this, we need to calculate the set of world space positions that map to individual pixels in the image,
        // then ask the paint brush for each position what value we should set that pixel to. Finally, we use those modified
        // values to change the macro material.

        const int32_t xPoints = aznumeric_cast<int32_t>(AZStd::round(dirtyArea.GetXExtent() / m_imageData.m_metersPerPixelX));
        const int32_t yPoints = aznumeric_cast<int32_t>(AZStd::round(dirtyArea.GetYExtent() / m_imageData.m_metersPerPixelY));

        // Early out if the dirty area is smaller than our point size.
        if ((xPoints <= 0) || (yPoints <= 0))
        {
            return;
        }

        // Calculate the minimum set of world space points that map to those pixels.
        AZStd::vector<AZ::Vector3> points;
        points.reserve(xPoints * yPoints);
        for (float y = dirtyArea.GetMin().GetY() + (m_imageData.m_metersPerPixelY / 2.0f);
            y <= dirtyArea.GetMax().GetY();
            y += m_imageData.m_metersPerPixelY)
        {
            for (float x = dirtyArea.GetMin().GetX() + (m_imageData.m_metersPerPixelX / 2.0f);
                 x <= dirtyArea.GetMax().GetX();
                 x += m_imageData.m_metersPerPixelX)
            {
                points.emplace_back(x, y, dirtyArea.GetMin().GetZ());
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
        TerrainMacroColorModificationBus::Event(
            entityId, &TerrainMacroColorModificationBus::Events::GetMacroColorPixelIndicesForPositions, validPoints, pixelIndices);

        // Create a buffer for all of the modified, blended pixel values.
        AZStd::vector<AZ::Color> paintedValues;
        paintedValues.reserve(pixelIndices.size());

        // For each pixel, accumulate the per-pixel opacity in the stroke layer, then (re)blend the stroke layer with
        // the original data by using the stroke color, stroke opacity, per-pixel opacity, and original pre-stroke pixel color value.
        // The (re)blended value gets sent immediately to the macro material, as well as getting cached off into the stroke buffer
        // for easier and faster undo/redo operations.
        for (size_t index = 0; index < pixelIndices.size(); index++)
        {
            // If we have an invalid pixel index, fill in a placeholder value into paintedValues and move on to the next pixel.
            if ((pixelIndices[index].first < 0) || (pixelIndices[index].second < 0))
            {
                paintedValues.emplace_back(AZ::Color::CreateZero());
                continue;
            }

            auto [originalColor, opacityValue] = m_strokeBuffer->GetOriginalPixelValueAndOpacity(pixelIndices[index]);

            // Add the new per-pixel opacity to the existing opacity in our stroke layer.
            opacityValue = AZStd::clamp(opacityValue + (1.0f - opacityValue) * perPixelOpacities[index], 0.0f, 1.0f);

            // Blend the pixel and store the blended pixel and new opacity back into our paint stroke buffer.
            AZ::Color blendedColor = combineFn(validPoints[index], originalColor, opacityValue);
            m_strokeBuffer->SetModifiedPixelValue(pixelIndices[index], blendedColor, opacityValue);

            // Also store the blended value into a second buffer that we'll use to immediately modify the macro material.
            paintedValues.emplace_back(blendedColor);

            // Track the data needed for calculating the dirty region for this specific operation as well as for the overall brush stroke.
            modifiedRegion.AddPoint(pixelIndices[index]);
            m_modifiedStrokeRegion.AddPoint(pixelIndices[index]);
        }

        // Modify the macro material with all of the changed values
        TerrainMacroColorModificationBus::Event(entityId, &TerrainMacroColorModificationBus::Events::StartMacroColorPixelModifications);
        TerrainMacroColorModificationBus::Event(
            entityId, &TerrainMacroColorModificationBus::Events::SetMacroColorPixelValuesByPixelIndex, pixelIndices, paintedValues);
        TerrainMacroColorModificationBus::Event(entityId, &TerrainMacroColorModificationBus::Events::EndMacroColorPixelModifications);

        // Get the dirty region that actually encompasses everything that we directly modified,
        // along with everything it indirectly affected.
        if (modifiedRegion.IsModified())
        {
            AZ::Aabb expandedDirtyArea = modifiedRegion.GetDirtyRegion();

            // Notify the terrain system that the color data in this region has changed.
            // We don't need to notify anything else because the terrain renderer will automatically get the changes from the
            // uploaded texture changes.
            TerrainSystemServiceRequestBus::Broadcast(
                &TerrainSystemServiceRequestBus::Events::RefreshRegion,
                expandedDirtyArea,
                AzFramework::Terrain::TerrainDataNotifications::TerrainDataChangedMask::ColorData);
        }
    }

    void MacroMaterialImageModifier::OnPaint(
        const AZ::Color& color, const AZ::Aabb& dirtyArea, ValueLookupFn& valueLookupFn, BlendFn& blendFn)
    {
        // For paint notifications, we'll use the given blend function to blend the original value and the paint brush intensity
        // using the built-up opacity.
        auto combineFn = [color, blendFn](
            [[maybe_unused]] const AZ::Vector3& worldPosition, AZ::Color originalColor, float opacityValue) -> AZ::Color
        {
            // There's an optimization opportunity here by finding a way to rework the blendFn so that it can blend
            // multiple channels at once, instead of blending each channel separately.
            float red = blendFn(originalColor.GetR(), color.GetR(), opacityValue * color.GetA());
            float green = blendFn(originalColor.GetG(), color.GetG(), opacityValue * color.GetA());
            float blue = blendFn(originalColor.GetB(), color.GetB(), opacityValue * color.GetA());
            return AZ::Color(red, green, blue, originalColor.GetA());
        };

        // Perform all the common logic between painting and smoothing to modify our macro material.
        OnPaintSmoothInternal(dirtyArea, valueLookupFn, combineFn);
    }

    void MacroMaterialImageModifier::OnSmooth(
        const AZ::Color& color, const AZ::Aabb& dirtyArea,
        ValueLookupFn& valueLookupFn,
        AZStd::span<const AZ::Vector3> valuePointOffsets,
        SmoothFn& smoothFn)
    {
        AZ::EntityId entityId = m_ownerEntityComponentId.GetEntityId();

        // Declare our vectors of kernel point locations and values once outside of the combine function so that we
        // don't keep reallocating them on every point.
        AZStd::vector<AZ::Vector3> kernelPoints;
        AZStd::vector<AZ::Color> kernelValues;

        const AZ::Vector3 valuePointOffsetScale(m_imageData.m_metersPerPixelX, m_imageData.m_metersPerPixelY, 0.0f);

        kernelPoints.reserve(valuePointOffsets.size());
        kernelValues.reserve(valuePointOffsets.size());

        // For smoothing notifications, we'll need to gather all of the neighboring macro material values to feed into the given smoothing
        // function for our blend operation.
        auto combineFn = [entityId, color, smoothFn, &valuePointOffsets, valuePointOffsetScale, &kernelPoints, &kernelValues](
                             const AZ::Vector3& worldPosition, AZ::Color originalColor, float opacityValue) -> AZ::Color
        {
            kernelPoints.clear();

            // Calculate all of the world positions around our base position that we'll use for fetching our blurring kernel values.
            for (auto& valuePointOffset : valuePointOffsets)
            {
                kernelPoints.emplace_back(worldPosition + (valuePointOffset * valuePointOffsetScale));
            }

            kernelValues.assign(kernelPoints.size(), AZ::Color::CreateZero());

            // Read all of the original macro color values for the blurring kernel into the buffer.
            AZStd::vector<PixelIndex> pixelIndices(kernelPoints.size());
            TerrainMacroColorModificationBus::Event(
                entityId,
                &TerrainMacroColorModificationBus::Events::GetMacroColorPixelValuesByPosition,
                kernelPoints, kernelValues);

            AZStd::vector<float> kernelValuesSingleChannel;

            // Blend each color channel separately.
            // Eventually it would be nice to refactor this so that the paint and smooth functions could take in multiple channels of data.
            for (auto& value : kernelValues)
            {
                kernelValuesSingleChannel.push_back(value.GetR());
            }
            float red = smoothFn(originalColor.GetR(), kernelValuesSingleChannel, opacityValue * color.GetA());

            kernelValuesSingleChannel.clear();
            for (auto& value : kernelValues)
            {
                kernelValuesSingleChannel.push_back(value.GetG());
            }
            float green = smoothFn(originalColor.GetG(), kernelValuesSingleChannel, opacityValue * color.GetA());

            kernelValuesSingleChannel.clear();
            for (auto& value : kernelValues)
            {
                kernelValuesSingleChannel.push_back(value.GetB());
            }
            float blue = smoothFn(originalColor.GetB(), kernelValuesSingleChannel, opacityValue * color.GetA());

            // Blend all the blurring kernel values together and store the blended pixel and new opacity back into our paint stroke buffer.
            return AZ::Color(red, green, blue, originalColor.GetA());
        };

        // Perform all the common logic between painting and smoothing to modify our macro material.
        OnPaintSmoothInternal(dirtyArea, valueLookupFn, combineFn);
    }
} // namespace Terrain
