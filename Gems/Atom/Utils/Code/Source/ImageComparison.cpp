/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/Utils/ImageComparison.h>

#include <AzCore/Casting/numeric_cast.h>

namespace AZ
{
    namespace Utils
    {
        int16_t CalcMaxChannelDifference(AZStd::span<const uint8_t> bufferA, AZStd::span<const uint8_t> bufferB, size_t index)
        {
            // We use the max error from a single channel instead of accumulating the error from each channel.
            // This normalizes differences so that for example black vs red has the same weight as black vs yellow.
            const int16_t diffR = static_cast<int16_t>(abs(aznumeric_cast<int16_t>(bufferA[index]) - aznumeric_cast<int16_t>(bufferB[index])));
            const int16_t diffG = static_cast<int16_t>(abs(aznumeric_cast<int16_t>(bufferA[index + 1]) - aznumeric_cast<int16_t>(bufferB[index + 1])));
            const int16_t diffB = static_cast<int16_t>(abs(aznumeric_cast<int16_t>(bufferA[index + 2]) - aznumeric_cast<int16_t>(bufferB[index + 2])));
            const int16_t diffA = static_cast<int16_t>(abs(aznumeric_cast<int16_t>(bufferA[index + 3]) - aznumeric_cast<int16_t>(bufferB[index + 3])));
            return AZ::GetMax(AZ::GetMax(AZ::GetMax(diffR, diffG), diffB), diffA);
        }

        ImageDiffResultCode CalcImageDiffRms(
            AZStd::span<const uint8_t> bufferA, const RHI::Size& sizeA, RHI::Format formatA,
            AZStd::span<const uint8_t> bufferB, const RHI::Size& sizeB, RHI::Format formatB,
            float* diffScore,
            float* filteredDiffScore,
            float minDiffFilter)
        {
            static constexpr size_t BytesPerPixel = 4;

            if (formatA != formatB)
            {
                return ImageDiffResultCode::FormatMismatch;
            }

            if (formatA != AZ::RHI::Format::R8G8B8A8_UNORM || formatB != AZ::RHI::Format::R8G8B8A8_UNORM)
            {
                return ImageDiffResultCode::UnsupportedFormat;
            }

            if (sizeA != sizeB)
            {
                return ImageDiffResultCode::SizeMismatch;
            }

            uint32_t totalPixelCount = sizeA.m_width * sizeA.m_height;

            if (bufferA.size() != bufferB.size())
            {
                return ImageDiffResultCode::SizeMismatch;
            }

            if (bufferA.size() != BytesPerPixel * totalPixelCount)
            {
                return ImageDiffResultCode::SizeMismatch;
            }

            if (bufferB.size() != BytesPerPixel * totalPixelCount)
            {
                return ImageDiffResultCode::SizeMismatch;
            }

            if (diffScore)
            {
                *diffScore = 0.0f;
            }

            if (filteredDiffScore)
            {
                *filteredDiffScore = 0.0f;
            }

            for (size_t i = 0; i < bufferA.size(); i += BytesPerPixel)
            {
                const float finalDiffNormalized = aznumeric_cast<float>(CalcMaxChannelDifference(bufferA, bufferB, i)) / 255.0f;
                const float squared = finalDiffNormalized * finalDiffNormalized;

                if (diffScore)
                {
                    *diffScore += squared;
                }

                if (filteredDiffScore && finalDiffNormalized > minDiffFilter)
                {
                    *filteredDiffScore += squared;
                }
            }

            if (diffScore)
            {
                *diffScore = sqrt(*diffScore / totalPixelCount);
            }

            if (filteredDiffScore)
            {
                *filteredDiffScore = sqrt(*filteredDiffScore / totalPixelCount);
            }

            return ImageDiffResultCode::Success;
        }

    }
}
