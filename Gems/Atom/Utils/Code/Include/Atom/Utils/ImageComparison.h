/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/containers/span.h>
#include <Atom/RHI.Reflect/Size.h>
#include <Atom/RHI.Reflect/Format.h>

namespace AZ
{
    namespace Utils
    {
        enum class ImageDiffResultCode
        {
            Success,
            SizeMismatch,
            FormatMismatch,
            UnsupportedFormat
        };

        //! Calculates the maximum difference of the rgb channels between two image buffers.
        int16_t CalcMaxChannelDifference(AZStd::span<const uint8_t> bufferA, AZStd::span<const uint8_t> bufferB, size_t index);

        //! Compares two images and returns the RMS (root mean square) of the difference.
        //! @param buffer[A|B] the raw buffer of image data
        //! @param size[A|B] the dimensions of the image in the buffer
        //! @param format[A|B] the pixel format of the image
        //! @param diffScore [out] the RMS value
        //! @param filteredDiff [out] an alternate RMS value calculated after removing any diffs less than @minDiffFilter.
        //! @param minDiffFilter diff values less than this will be filtered out before calculating @filteredDiff.
        ImageDiffResultCode CalcImageDiffRms(
            AZStd::span<const uint8_t> bufferA, const RHI::Size& sizeA, RHI::Format formatA,
            AZStd::span<const uint8_t> bufferB, const RHI::Size& sizeB, RHI::Format formatB,
            float* diffScore = nullptr,
            float* filteredDiffScore = nullptr,
            float minDiffFilter = 0.0);

    }
} // namespace AZ
