/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Component/TransformBus.h>

#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/Manipulators/PaintBrushManipulator.h>
#include <AzToolsFramework/Manipulators/ManipulatorManager.h>
#include <AzToolsFramework/Manipulators/ManipulatorView.h>
#include <AzToolsFramework/PaintBrush/PaintBrushNotificationBus.h>
#include <AzToolsFramework/PaintBrushSettings/PaintBrushSettingsRequestBus.h>
#include <AzToolsFramework/PaintBrushSettings/PaintBrushSettingsWindow.h>
#include <AzToolsFramework/ViewportSelection/EditorSelectionUtil.h>

#include <Editor/EditorImageGradientComponentMode.h>
#include <Editor/EditorImageGradientRequestBus.h>

#include <GradientSignal/Ebuses/GradientRequestBus.h>
#include <GradientSignal/Ebuses/ImageGradientModificationBus.h>
#include <GradientSignal/Ebuses/ImageGradientRequestBus.h>

#include <LmbrCentral/Dependency/DependencyNotificationBus.h>

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
        ImageTileBuffer(uint32_t imageWidth, uint32_t imageHeight, AZ::EntityId imageGradientEntityId)
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

        ~ImageTileBuffer() = default;

        //! Returns true if we don't have any pixel modifications, false if we do.
        bool Empty() const
        {
            return !m_modifiedAnyPixels;
        }

        //! Get the original gradient value for the given pixel index.
        //! Since we "lazy-cache" our unmodified image as tiles, create it here the first time we request a pixel from a tile.
        AZStd::pair<float, float> GetOriginalPixelValueAndOpacity(const PixelIndex& pixelIndex)
        {
            uint32_t tileIndex = GetTileIndex(pixelIndex);
            uint32_t pixelTileIndex = GetPixelTileIndex(pixelIndex);

            // Create the tile if it doesn't already exist.
            CreateImageTile(tileIndex);

            return
            {
                m_paintedImageTiles[tileIndex]->m_unmodifiedData[pixelTileIndex],
                m_paintedImageTiles[tileIndex]->m_modifiedDataOpacity[pixelTileIndex]
            };
        }

        //! Set a modified gradient value for the given pixel index.
        void SetModifiedPixelValue(const PixelIndex& pixelIndex, float modifiedValue, float opacity)
        {
            uint32_t tileIndex = GetTileIndex(pixelIndex);
            uint32_t pixelTileIndex = GetPixelTileIndex(pixelIndex);

            AZ_Assert(m_paintedImageTiles[tileIndex], "Cached image tile hasn't been created yet!");

            m_paintedImageTiles[tileIndex]->m_modifiedData[pixelTileIndex] = modifiedValue;
            m_paintedImageTiles[tileIndex]->m_modifiedDataOpacity[pixelTileIndex] = opacity;
        }

        //! For undo/redo operations, apply the buffer of changes back to the image gradient.
        void ApplyChangeBuffer(bool undo)
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
                        pixelIndices[index++] = PixelIndex(
                            aznumeric_cast<int16_t>(startIndex.first + x), aznumeric_cast<int16_t>(startIndex.second + y));
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

    private:
        //! Given a pixel index, get the tile index that it maps to.
        uint32_t GetTileIndex(const PixelIndex& pixelIndex) const
        {
            return ((pixelIndex.second / ImageTileSize) * m_numTilesX) + (pixelIndex.first / ImageTileSize);
        }

        //! Given a tile index, get the absolute start pixel index for the upper left corner of the tile.
        PixelIndex GetStartPixelIndex(uint32_t tileIndex) const
        {
            return PixelIndex(
                aznumeric_cast<int16_t>((tileIndex % m_numTilesX) * ImageTileSize),
                aznumeric_cast<int16_t>((tileIndex / m_numTilesX) * ImageTileSize));
        }

        // Given a pixel index, get the relative pixel index within the tile
        uint32_t GetPixelTileIndex(const PixelIndex& pixelIndex) const
        {
            uint32_t xIndex = pixelIndex.first % ImageTileSize;
            uint32_t yIndex = pixelIndex.second % ImageTileSize;

            return (yIndex * ImageTileSize) + xIndex;
        }

        //! Create an image tile initialized with the image gradient values if it doesn't already exist.
        void CreateImageTile(uint32_t tileIndex)
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

    //! Class that tracks the data for undoing/redoing a paint stroke.
    class PaintBrushUndoBuffer : public AzToolsFramework::UndoSystem::URSequencePoint
    {
    public:
        AZ_CLASS_ALLOCATOR(PaintBrushUndoBuffer, AZ::SystemAllocator, 0);
        AZ_RTTI(PaintBrushUndoBuffer, "{E37936AC-22E1-403A-A36B-55390832EDE4}");

        PaintBrushUndoBuffer(AZ::EntityId imageEntityId)
            : AzToolsFramework::UndoSystem::URSequencePoint("PaintStroke")
            , m_entityId(imageEntityId)
        {
        }

        virtual ~PaintBrushUndoBuffer() = default;

        void Undo() override
        {
            if (m_strokeImageBuffer->Empty())
            {
                return;
            }

            // Apply the "undo" buffer
            const bool undo = true;
            m_strokeImageBuffer->ApplyChangeBuffer(undo);

            // Notify anything listening to the image gradient that the modified region has changed.
            LmbrCentral::DependencyNotificationBus::Event(
                m_entityId, &LmbrCentral::DependencyNotificationBus::Events::OnCompositionRegionChanged, m_dirtyArea);
        }

        void Redo() override
        {
            if (m_strokeImageBuffer->Empty())
            {
                return;
            }

            // Apply the "redo" buffer
            const bool undo = false;
            m_strokeImageBuffer->ApplyChangeBuffer(undo);

            // Notify anything listening to the image gradient that the modified region has changed.
            LmbrCentral::DependencyNotificationBus::Event(
                m_entityId, &LmbrCentral::DependencyNotificationBus::Events::OnCompositionRegionChanged, m_dirtyArea);
        }

        bool Changed() const override
        {
            return !m_strokeImageBuffer->Empty();
        }

        void SetUndoBufferAndDirtyArea(AZStd::unique_ptr<ImageTileBuffer>&& buffer, const AZ::Aabb& dirtyArea)
        {
            m_strokeImageBuffer = AZStd::move(buffer);
            m_dirtyArea = dirtyArea;
        }

    private:
        //! The entity containing the modified image gradient.
        const AZ::EntityId m_entityId;

        //! The undo/redo data for the paint strokes.
        AZStd::unique_ptr<ImageTileBuffer> m_strokeImageBuffer;

        //! Cached dirty area
        AZ::Aabb m_dirtyArea;
    };

    EditorImageGradientComponentMode::EditorImageGradientComponentMode(
        const AZ::EntityComponentIdPair& entityComponentIdPair, AZ::Uuid componentType)
        : EditorBaseComponentMode(entityComponentIdPair, componentType)
        , m_ownerEntityComponentId(entityComponentIdPair)
        , m_anyValuesChanged(false)
    {
        EditorImageGradientRequestBus::Event(GetEntityId(), &EditorImageGradientRequests::StartImageModification);
        ImageGradientModificationBus::Event(GetEntityId(), &ImageGradientModifications::StartImageModification);

        AzToolsFramework::PaintBrushNotificationBus::Handler::BusConnect(entityComponentIdPair);

        // Set our paint brush min/max world size range. The minimum size should be large enough to paint at least one pixel, and
        // the max size is clamped so that we can't paint more than 256 x 256 pixels per brush stamp.
        // 256 is an arbitrary number, but if we start getting much larger, performance can drop precipitously.
        // Note: To truly control performance, additional clamping is still needed, because large mouse movements in world space with
        // a tiny brush can still cause extremely large numbers of brush points to get calculated and checked.

        constexpr float MaxBrushPixelSize = 256.0f;
        AZ::Vector2 imagePixelsPerMeter(0.0f);
        ImageGradientRequestBus::EventResult(imagePixelsPerMeter, GetEntityId(), &ImageGradientRequestBus::Events::GetImagePixelsPerMeter);

        float minBrushSize = AZStd::min(imagePixelsPerMeter.GetX(), imagePixelsPerMeter.GetY());
        float maxBrushSize = AZStd::max(imagePixelsPerMeter.GetX(), imagePixelsPerMeter.GetY());

        minBrushSize = (minBrushSize <= 0.0f) ? 0.0f : (1.0f / minBrushSize);
        maxBrushSize = (maxBrushSize <= 0.0f) ? 0.0f : (MaxBrushPixelSize / maxBrushSize);

        AzToolsFramework::PaintBrushSettingsRequestBus::Broadcast(
            &AzToolsFramework::PaintBrushSettingsRequestBus::Events::SetSizeRange, minBrushSize, maxBrushSize);

        AZ::Transform worldFromLocal = AZ::Transform::CreateIdentity();
        AZ::TransformBus::EventResult(worldFromLocal, GetEntityId(), &AZ::TransformInterface::GetWorldTM);

        m_brushManipulator = AzToolsFramework::PaintBrushManipulator::MakeShared(
            worldFromLocal, entityComponentIdPair, AzToolsFramework::PaintBrushColorMode::Greyscale);
        m_brushManipulator->Register(AzToolsFramework::g_mainManipulatorManagerId);
    }

    EditorImageGradientComponentMode::~EditorImageGradientComponentMode()
    {
        AzToolsFramework::PaintBrushNotificationBus::Handler::BusDisconnect();
        m_brushManipulator->Unregister();
        m_brushManipulator.reset();

        EndUndoBatch();

        // It's possible that we're leaving component mode as the result of an "undo" action.
        // If that's the case, don't prompt the user to save the changes.
        if (!AzToolsFramework::UndoRedoOperationInProgress() && m_anyValuesChanged)
        {
            EditorImageGradientRequestBus::Event(GetEntityId(), &EditorImageGradientRequests::SaveImage);
        }

        ImageGradientModificationBus::Event(GetEntityId(), &ImageGradientModifications::EndImageModification);
        EditorImageGradientRequestBus::Event(GetEntityId(), &EditorImageGradientRequests::EndImageModification);
    }

    AZStd::vector<AzToolsFramework::ActionOverride> EditorImageGradientComponentMode::PopulateActionsImpl()
    {
        return m_brushManipulator->PopulateActionsImpl();
    }

    AZStd::string EditorImageGradientComponentMode::GetComponentModeName() const
    {
        return "Image Gradient Paint Mode";
    }

    bool EditorImageGradientComponentMode::HandleMouseInteraction(
        const AzToolsFramework::ViewportInteraction::MouseInteractionEvent& mouseInteraction)
    {
        return m_brushManipulator->HandleMouseInteraction(mouseInteraction);
    }

    void EditorImageGradientComponentMode::Refresh()
    {
    }

    void EditorImageGradientComponentMode::BeginUndoBatch()
    {
        AZ_Assert(m_undoBatch == nullptr, "Starting an undo batch while one is already active!");

        AzToolsFramework::ToolsApplicationRequests::Bus::BroadcastResult(
            m_undoBatch, &AzToolsFramework::ToolsApplicationRequests::Bus::Events::BeginUndoBatch, "PaintStroke");

        m_paintBrushUndoBuffer = aznew PaintBrushUndoBuffer(GetEntityId());
        m_paintBrushUndoBuffer->SetParent(m_undoBatch);
    }

    void EditorImageGradientComponentMode::EndUndoBatch()
    {
        if (m_undoBatch != nullptr)
        {
            AzToolsFramework::ToolsApplicationRequests::Bus::Broadcast(
                &AzToolsFramework::ToolsApplicationRequests::Bus::Events::EndUndoBatch);
            m_undoBatch = nullptr;
            m_paintBrushUndoBuffer = nullptr;
        }
    }

    void EditorImageGradientComponentMode::OnBrushStrokeBegin(const AZ::Color& color)
    {
        BeginUndoBatch();

        // Get the spacing to map individual pixels to world space positions.
        AZ::Vector2 imagePixelsPerMeter(0.0f);
        ImageGradientRequestBus::EventResult(imagePixelsPerMeter, GetEntityId(), &ImageGradientRequestBus::Events::GetImagePixelsPerMeter);
        if ((imagePixelsPerMeter.GetX() <= 0.0f) || (imagePixelsPerMeter.GetY() <= 0.0f))
        {
            return;
        }

        m_paintStrokeData.m_intensity = color.GetR();
        m_paintStrokeData.m_opacity = color.GetA();

        m_paintStrokeData.m_metersPerPixelX = 1.0f / imagePixelsPerMeter.GetX();
        m_paintStrokeData.m_metersPerPixelY = 1.0f / imagePixelsPerMeter.GetY();

        uint32_t imageWidth = 0;
        ImageGradientRequestBus::EventResult(imageWidth, GetEntityId(), &ImageGradientRequestBus::Events::GetImageWidth);

        uint32_t imageHeight = 0;
        ImageGradientRequestBus::EventResult(imageHeight, GetEntityId(), &ImageGradientRequestBus::Events::GetImageHeight);

        // Create the buffer for holding all the changes for a single continuous paint brush stroke.
        // This buffer will get used during the stroke to hold our accumulated stroke opacity layer,
        // and then after the stroke finishes we'll hand the buffer over to the undo system as an undo/redo buffer.
        m_paintStrokeData.m_strokeBuffer = AZStd::make_unique<ImageTileBuffer>(imageWidth, imageHeight, GetEntityId());
    }

    void EditorImageGradientComponentMode::OnBrushStrokeEnd()
    {
        AZ_Assert(m_paintBrushUndoBuffer != nullptr, "Undo batch is expected to exist while painting");

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

        // Hand over ownership of the paint stroke buffer to the undo/redo buffer.
        m_paintBrushUndoBuffer->SetUndoBufferAndDirtyArea(
            AZStd::move(m_paintStrokeData.m_strokeBuffer), m_paintStrokeData.m_dirtyRegion);

        EndUndoBatch();

        // Make sure we've cleared out our paint stroke data until the next paint stroke begins.
        m_paintStrokeData = {};
    }

    AZ::Color EditorImageGradientComponentMode::OnGetColor(const AZ::Vector3& brushCenter)
    {
        // Get the gradient value at the given point.

        float gradientValue = 0.0f;
        GradientSampleParams sampleParams = {brushCenter};
        GradientRequestBus::EventResult(gradientValue, GetEntityId(), &GradientRequestBus::Events::GetValue, sampleParams);
        return AZ::Color(gradientValue, gradientValue, gradientValue, 1.0f);
    }

    void EditorImageGradientComponentMode::OnPaintSmoothInternal(
        const AZ::Aabb& dirtyArea, ValueLookupFn& valueLookupFn,
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

        AZ::EntityId entityId = GetEntityId();

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
            GetEntityId(), &ImageGradientModificationBus::Events::SetPixelValuesByPixelIndex, pixelIndices, paintedValues);

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
            GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionRegionChanged, expandedDirtyArea);

        // Track that we've changed image values, so we should prompt to save on exit.
        m_anyValuesChanged = true;
    }

    void EditorImageGradientComponentMode::OnPaint(const AZ::Aabb& dirtyArea, ValueLookupFn& valueLookupFn, BlendFn& blendFn)
    {
        // For paint notifications, we'll use the given blend function to blend the original value and the paint brush intensity
        // using the built-up opacity.
        auto combineFn = [this, blendFn](
            [[maybe_unused]] const AZ::Vector3& worldPosition, float gradientValue, float opacityValue) -> float
        {
            return blendFn(gradientValue, m_paintStrokeData.m_intensity, opacityValue * m_paintStrokeData.m_opacity);
        };

        // Perform all the common logic between painting and smoothing to modify our image gradient.
        OnPaintSmoothInternal(dirtyArea, valueLookupFn, combineFn);
    }

    void EditorImageGradientComponentMode::OnSmooth(
        const AZ::Aabb& dirtyArea, ValueLookupFn& valueLookupFn, AZStd::span<const AZ::Vector3> valuePointOffsets, SmoothFn& smoothFn)
    {
        AZ::EntityId entityId = GetEntityId();

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
