/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/ComponentBus.h>

#include <GradientSignal/Util.h>

namespace GradientSignal
{
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
        //! Currently does nothing, but might eventually need to perform some cleanup logic.
        virtual void EndImageModification() = 0;

        //! Given a list of world positions, return a list of pixel indices into the image.
        //! @param positions The list of world positions to query
        //! @param outIndices [out] The list of output PixelIndex values giving the (x,y) pixel coordinates for each world position.
        virtual void GetPixelIndicesForPositions(AZStd::span<const AZ::Vector3> positions, AZStd::span<PixelIndex> outIndices) const = 0;

        //! Get the image pixel values at a list of positions.
        //! Unlike GetValues on the GradientRequestBus, this will always use point sampling regardless of
        //! the Image Gradient sampler type because the specific pixel values are being requested.
        //! @param positions The list of world positions to query
        //! @param outValues [out] The list of output values. This list is expected to be the same size as the positions list.
        virtual void GetPixelValuesByPosition(AZStd::span<const AZ::Vector3> positions, AZStd::span<float> outValues) const = 0;

        //! Get the image pixel values at a list of pixel indices.
        //! Unlike GetValues on the GradientRequestBus, this will always use point sampling regardless of
        //! the Image Gradient sampler type because the specific pixel values are being requested.
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
}
