/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/ComponentBus.h>
#include <AzCore/Math/Aabb.h>

namespace GradientSignal
{
    class ImageTileBuffer;

    using PixelIndex = AZStd::pair<int16_t, int16_t>;

    //! EBus that can be used to modify the image data for an Image Gradient.
    //! The following APIs are the low-level image modification APIs that enable image modifications at the per-pixel level.
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
