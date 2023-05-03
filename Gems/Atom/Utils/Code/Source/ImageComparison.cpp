/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/Utils/ImageComparison.h>

#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/Preprocessor/EnumReflectUtils.h>

namespace AZ
{
    namespace Utils
    {
        void ImageComparisonError::Reflect(ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<ImageComparisonError>()
                    ->Version(1)
                    ->Field("ErrorMessage", &ImageComparisonError::m_errorMessage);
            }

            if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
            {
                behaviorContext->Class<ImageComparisonError>("ImageComparisonError")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                    ->Attribute(AZ::Script::Attributes::Module, "utils")
                    ->Property("ErrorMessage", BehaviorValueProperty(&ImageComparisonError::m_errorMessage));
            }
        }

        void ImageDiffResult::Reflect(ReflectContext* context)
        {
            ImageComparisonError::Reflect(context);

            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<ImageDiffResult>()
                    ->Version(1)
                    ->Field("DiffScore", &ImageDiffResult::m_diffScore)
                    ->Field("FilteredDiffScore", &ImageDiffResult::m_filteredDiffScore);
            }

            if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
            {
                behaviorContext->Class<ImageDiffResult>("ImageDiffResult")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                    ->Attribute(AZ::Script::Attributes::Module, "utils")
                    ->Property("DiffScore", BehaviorValueProperty(&ImageDiffResult::m_diffScore))
                        ->Attribute(AZ::Script::Attributes::Alias, "diff_score")
                    ->Property("FilteredDiffScore", BehaviorValueProperty(&ImageDiffResult::m_filteredDiffScore))
                        ->Attribute(AZ::Script::Attributes::Alias, "filtered_diff_score")
                    ;
            }
        }

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

        AZ::Outcome<ImageDiffResult, ImageComparisonError> CalcImageDiffRms(
            AZStd::span<const uint8_t> bufferA, const RHI::Size& sizeA, RHI::Format formatA,
            AZStd::span<const uint8_t> bufferB, const RHI::Size& sizeB, RHI::Format formatB,
            float minDiffFilter)
        {
            static constexpr size_t BytesPerPixel = 4;

            ImageDiffResult result;
            ImageComparisonError error;

            if (formatA != formatB)
            {
                error.m_errorMessage = "Images format mismatch.";
                return AZ::Failure(error);
            }

            if (formatA != AZ::RHI::Format::R8G8B8A8_UNORM || formatB != AZ::RHI::Format::R8G8B8A8_UNORM)
            {
                error.m_errorMessage = "Unsupported image format.";
                return AZ::Failure(error);
            }

            if (sizeA != sizeB)
            {
                error.m_errorMessage = "Images size mismatch.";
                return AZ::Failure(error);
            }

            uint32_t totalPixelCount = sizeA.m_width * sizeA.m_height;

            if (bufferA.size() != bufferB.size()
                || bufferA.size() != BytesPerPixel * totalPixelCount
                || bufferB.size() != BytesPerPixel * totalPixelCount)
            {
                error.m_errorMessage = "Images size mismatch.";
                return AZ::Failure(error);
            }

            result.m_diffScore = 0.0f;
            result.m_filteredDiffScore = 0.0f;

            for (size_t i = 0; i < bufferA.size(); i += BytesPerPixel)
            {
                const float finalDiffNormalized = aznumeric_cast<float>(CalcMaxChannelDifference(bufferA, bufferB, i)) / 255.0f;
                const float squared = finalDiffNormalized * finalDiffNormalized;

                result.m_diffScore += squared;

                if (finalDiffNormalized > minDiffFilter)
                {
                    result.m_filteredDiffScore += squared;
                }
            }

            result.m_diffScore = sqrt(result.m_diffScore / totalPixelCount);
            result.m_filteredDiffScore = sqrt(result.m_filteredDiffScore / totalPixelCount);

            return AZ::Success(result);
        }

    }
}
