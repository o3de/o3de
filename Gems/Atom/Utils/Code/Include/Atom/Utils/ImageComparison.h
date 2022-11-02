/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Preprocessor/Enum.h>
#include <AzCore/std/containers/span.h>
#include <Atom/RHI.Reflect/Size.h>
#include <Atom/RHI.Reflect/Format.h>

namespace AZ
{
    namespace Utils
    {
        AZ_ENUM_CLASS(ImageDiffResultCode,
            Success,
            SizeMismatch,
            FormatMismatch,
            UnsupportedFormat
        );

        class ImageDiffResult
        {
        public:
            AZ_TYPE_INFO(ImageDiffResult, "{6E968463-F80F-465A-AC38-F2790987535B}");
            static void Reflect(ReflectContext* context);

            ImageDiffResultCode m_resultCode = ImageDiffResultCode::Success;
            //! The RMS value calculated through CalcImageDiffRms
            float m_diffScore = 0.0f;
            //! The RMS value calculated after removing any diffs less than a minimal diff filter.
            float m_filteredDiffScore = 0.0f;
        };

        //! Calculates the maximum difference of the rgb channels between two image buffers.
        int16_t CalcMaxChannelDifference(AZStd::span<const uint8_t> bufferA, AZStd::span<const uint8_t> bufferB, size_t index);

        //! Compares two images and returns the RMS (root mean square) of the difference.
        //! @param buffer[A|B] the raw buffer of image data
        //! @param size[A|B] the dimensions of the image in the buffer
        //! @param format[A|B] the pixel format of the image
        //! @param minDiffFilter diff values less than this will be filtered out before calculating ImageDiffResult::m_filteredDiffScore.
        ImageDiffResult CalcImageDiffRms(
            AZStd::span<const uint8_t> bufferA, const RHI::Size& sizeA, RHI::Format formatA,
            AZStd::span<const uint8_t> bufferB, const RHI::Size& sizeB, RHI::Format formatB,
            float minDiffFilter = 0.0);

    }

    AZ_TYPE_INFO_SPECIALIZE(Utils::ImageDiffResultCode, "{24956F09-C665-47B6-9415-E8467F9C8F5E}");
} // namespace AZ
