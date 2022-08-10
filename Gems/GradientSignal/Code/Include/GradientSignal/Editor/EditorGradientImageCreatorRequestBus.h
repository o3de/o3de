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
    enum class OutputFormat : AZ::u8
    {
        R8,
        R16,
        R32
    };

    class GradientImageCreatorRequests
        : public AZ::ComponentBus
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;

        static inline AZStd::string GetSupportedImagesFilter()
        {
            // Build filter for supported streaming image formats that will be used on the
            // native file dialog when creating/picking an output file for the painted image.
            // ImageProcessingAtom::s_SupportedImageExtensions actually has more formats
            // that will produce streaming image assets, but not all of them support
            // all of the bit depths we care about (8/16/32), so we've reduced the list
            // to the image formats that do.
            return "Images (*.png *.tif *.tiff *.tga *.exr)";
        }

        static inline AZStd::vector<AZ::Edit::EnumConstant<OutputFormat>> SupportedOutputFormatOptions()
        {
            return { AZ::Edit::EnumConstant<OutputFormat>(OutputFormat::R8, "R8 (8-bit)"),
                     AZ::Edit::EnumConstant<OutputFormat>(OutputFormat::R16, "R16 (16-bit)"),
                     AZ::Edit::EnumConstant<OutputFormat>(OutputFormat::R32, "R32 (32-bit)") };
        }

        virtual AZ::Vector2 GetOutputResolution() const = 0;
        virtual void SetOutputResolution(const AZ::Vector2& resolution) = 0;

        virtual OutputFormat GetOutputFormat() const = 0;
        virtual void SetOutputFormat(OutputFormat outputFormat) = 0;

        virtual AZ::IO::Path GetOutputImagePath() const = 0;
        virtual void SetOutputImagePath(const AZ::IO::Path& outputImagePath) = 0;
    };

    using GradientImageCreatorRequestBus = AZ::EBus<GradientImageCreatorRequests>;
}
