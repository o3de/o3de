/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/ComponentBus.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/std/containers/vector.h>

#include <GradientSignal/Util.h>

namespace GradientSignal
{
    //! OutputFormat dictates the subset of output types supported for gradient image creation.
    enum class OutputFormat : AZ::u8
    {
        R8,         //! single-channel 8-bit uint
        R16,        //! single-channel 16-bit uint
        R32,        //! single-channel 32-bit float
        R8G8B8A8    //! four-channel 32-bit uint (8 bits per channel)
    };

    //! EBus that is used by any Editor Gradient components that create images (Gradient Baker, Image Gradient).
    //! It contains the common APIs needed for image creation.
    class GradientImageCreatorRequests
        : public AZ::ComponentBus
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;

        //! Get the output resolution that we will create the image with.
        //! @return Returns the output width, height in a Vector2
        virtual AZ::Vector2 GetOutputResolution() const = 0;
        //! Sets the output resolution for creating the image.
        //! @param resolution The width and height for the created image.
        virtual void SetOutputResolution(const AZ::Vector2& resolution) = 0;

        //! Get the output format that we will create the image with.
        //! @return Returns the output format.
        virtual OutputFormat GetOutputFormat() const = 0;
        //! Sets the output format for creating the image.
        //! @param outputFormat The output format to use.
        virtual void SetOutputFormat(OutputFormat outputFormat) = 0;

        //! Get the output image path (including file name) for where we'll save the created image.
        //! @return The output path and file name.
        virtual AZ::IO::Path GetOutputImagePath() const = 0;
        //! Sets the output image path and file name for creating the image.
        //! @param outputImagePath The output image path and file name to use.
        virtual void SetOutputImagePath(const AZ::IO::Path& outputImagePath) = 0;
    };

    using GradientImageCreatorRequestBus = AZ::EBus<GradientImageCreatorRequests>;
}
