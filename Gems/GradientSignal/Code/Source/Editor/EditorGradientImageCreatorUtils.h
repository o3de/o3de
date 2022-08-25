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
#include <GradientSignal/Editor/EditorGradientImageCreatorRequestBus.h>

AZ_PUSH_DISABLE_WARNING(4777, "-Wunknown-warning-option")
#include <OpenImageIO/imageio.h>
AZ_POP_DISABLE_WARNING

namespace GradientSignal::ImageCreatorUtils
{
    //! Returns a string containing a file dialog filter that only allows images to be saved in supported format types.
    AZStd::string GetSupportedImagesFilter();

    //! Returns the EditContext enum constants for the GradientSignal::OutputFormat enum.
    AZStd::vector<AZ::Edit::EnumConstant<OutputFormat>> SupportedOutputFormatOptions();

    //! Given an OutputFormat, returns the number of channels in the format.
    int GetChannels(OutputFormat format);

    //! Given an OutputFormat, returns the number of bytes per channel in the format.
    int GetBytesPerChannel(OutputFormat format);

    //! Given a set of image parameters, generate a buffer of default pixel values (black with opaque alpha).
    AZStd::vector<AZ::u8> CreateDefaultImageBuffer(int imageResolutionX, int imageResolutionY, int channels, OutputFormat format);

    //! Write a source image out to disk.
    bool WriteImage(
        const AZStd::string& absoluteFileName,
        int imageResolutionX,
        int imageResolutionY,
        int channels,
        OutputFormat format,
        const AZStd::span<const AZ::u8>& pixelBuffer,
        bool showProgressDialog);

}
