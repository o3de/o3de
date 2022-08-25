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

        /**
         * Set the value at the given position.
         * @param position The position to set the value at.
         * @param value The value to set it to.
         */
        virtual void SetValue(const AZ::Vector3& position, float value) = 0;

        /**
         * Given a list of positions, set those positions to values.
         * @param positions The list of positions to set the values for.
         * @param values The list of values to set. This list is expected to be the same size as the positions list.
         */
        virtual void SetValues(AZStd::span<const AZ::Vector3> positions, AZStd::span<const float> values) = 0;
    };

    using ImageGradientModificationBus = AZ::EBus<ImageGradientModifications>;
}
