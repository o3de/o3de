/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/ComponentBus.h>
#include <AzFramework/PaintBrush/PaintBrushSettings.h>
#include <GradientSignal/Util.h>


namespace GradientSignal
{
    class ImageTileBuffer;

    using PixelIndex = AZStd::pair<int16_t, int16_t>;

    //! EBus that can be used to modify the image data for an Image Gradient.
    class ImageGradientModifications
        : public AZ::ComponentBus
    {
    public:
        // Overrides the default AZ::EBusTraits handler policy to allow only one listener per entity.
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;

        //! Start an image modification session.
        //! This will create a modification buffer that contains an uncompressed copy of the current image data.
        virtual void StartImageModification() = 0;

        //! Finish an image modification session.
        //! Clean up any helper structures used during image modification.
        virtual void EndImageModification() = 0;

        //! The following APIs are the high-level image modification APIs.
        //! These enable image modifications through paintbrush controls.

        //! Start a brush stroke with the given brush settings for color/intensity/opacity.
        //! @param brushSettings The paintbrush settings containing the color/intensity/opacity to use for the brush stroke.
        virtual void BeginBrushStroke(const AzFramework::PaintBrushSettings& brushSettings) = 0;

        //! End the current brush stroke.
        virtual void EndBrushStroke() = 0;

        //! Returns whether or not the brush is currently in a brush stroke.
        //! @return True if a brush stroke has been started, false if not.
        virtual bool IsInBrushStroke() const = 0;

        //! Reset the brush tracking so that the next action will be considered the start of a stroke movement instead of a continuation.
        //! This is useful for handling discontinuous movement (like moving off the edge of the surface on one side and back on from
        //! a different side).
        virtual void ResetBrushStrokeTracking() = 0;

        //! Apply a paint color to the underlying data based on brush movement and settings.
        //! @param brushCenter The current center of the paintbrush.
        //! @param brushSettings The current paintbrush settings.
        virtual void PaintToLocation(const AZ::Vector3& brushCenter, const AzFramework::PaintBrushSettings& brushSettings) = 0;

        //! Smooth the underlying data based on brush movement and settings.
        //! @param brushCenter The current center of the paintbrush.
        //! @param brushSettings The current paintbrush settings.
        virtual void SmoothToLocation(const AZ::Vector3& brushCenter, const AzFramework::PaintBrushSettings& brushSettings) = 0;

        //! The following APIs are the low-level image modification APIs. 
        //! These enable image modifications at the per-pixel level.

        //! Given a list of world positions, return a list of pixel indices into the image.
        //! @param positions The list of world positions to query
        //! @param outIndices [out] The list of output PixelIndex values giving the (x,y) pixel coordinates for each world position.
        virtual void GetPixelIndicesForPositions(AZStd::span<const AZ::Vector3> positions, AZStd::span<PixelIndex> outIndices) const = 0;

        //! Get the image pixel values at a list of positions.
        //! This provides different results than GradientRequestBus::GetValues because it's returning raw pixel values.
        //!  - This will always use point sampling instead of the Image Gradient sampler type
        //!  - This will always return back an unscaled value, instead of using the Image Gradient scale mode and range
        //! @param positions The list of world positions to query
        //! @param outValues [out] The list of output values. This list is expected to be the same size as the positions list.
        virtual void GetPixelValuesByPosition(AZStd::span<const AZ::Vector3> positions, AZStd::span<float> outValues) const = 0;

        //! Get the image pixel values at a list of pixel indices.
        //! This provides different results than GradientRequestBus::GetValues because it's returning raw pixel values.
        //!  - This will always use point sampling instead of the Image Gradient sampler type
        //!  - This will always return back an unscaled value, instead of using the Image Gradient scale mode and range
        //! @param positions The list of pixel indices to query
        //! @param outValues [out] The list of output values. This list is expected to be the same size as the positions list.
        virtual void GetPixelValuesByPixelIndex(AZStd::span<const PixelIndex> indices, AZStd::span<float> outValues) const = 0;

        //! Set the value at the given world position.
        //! @param position The world position to set the value at.
        //! @param value The value to set it to.
        virtual void SetPixelValueByPosition(const AZ::Vector3& position, float value) = 0;

        //! Set the value at the given pixel index.
        //! @param index The pixel index to set the value at.
        //! @param value The value to set it to.
        virtual void SetPixelValueByPixelIndex(const PixelIndex& index, float value) = 0;

        //! Given a list of world positions, set the pixels at those positions to the given values.
        //! @param positions The list of world positions to set the values for.
        //! @param values The list of values to set. This list is expected to be the same size as the positions list.
        virtual void SetPixelValuesByPosition(AZStd::span<const AZ::Vector3> positions, AZStd::span<const float> values) = 0;

        //! Given a list of pixel indices, set those pixels to the given values.
        //! @param indicdes The list of pixel indices to set the values for.
        //! @param values The list of values to set. This list is expected to be the same size as the positions list.
        virtual void SetPixelValuesByPixelIndex(AZStd::span<const PixelIndex> indices, AZStd::span<const float> values) = 0;
    };

    using ImageGradientModificationBus = AZ::EBus<ImageGradientModifications>;

    //! EBus that notifies about the current state of Image Gradient modifications
    class ImageGradientModificationNotifications : public AZ::ComponentBus
    {
    public:
        // Overrides the default AZ::EBusTraits handler policy to allow only one listener per entity.
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;

        //! Notify any listeners that a brush stroke has started on this image gradient.
        virtual void OnImageGradientBrushStrokeBegin() {}

        //! Notify any listeners that a brush stroke has ended on this image gradient.
        //! @param changedDataBuffer A pointer to the ImageTileBuffer containing the changed data. The buffer will be deleted
        //! after this notification unless a listener keeps a copy of the pointer (for undo/redo, for example).
        //! @param dirtyRegion The AABB defining the world space region affected by the brush stroke.
        virtual void OnImageGradientBrushStrokeEnd(
            [[maybe_unused]] AZStd::shared_ptr<ImageTileBuffer> changedDataBuffer, [[maybe_unused]] const AZ::Aabb& dirtyRegion)
        {
        }
    };

    using ImageGradientModificationNotificationBus = AZ::EBus<ImageGradientModificationNotifications>;
} // namespace GradientSignal
