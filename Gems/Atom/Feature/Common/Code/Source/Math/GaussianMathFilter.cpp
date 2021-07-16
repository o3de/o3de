/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Math/GaussianMathFilter.h>

namespace AZ
{
    namespace Render
    {
        // We consider pixels of over 3 sigma distance from the center
        // have negligible weight.
        const float GaussianMathFilter::ReliableSectionFactor = 3.f;
        const float GaussianMathFilter::StandardDeviationMax = 100.f;

        GaussianMathFilter::GaussianMathFilter(const GaussianFilterDescriptor& descriptor)
            : m_descriptor{descriptor}
        {
        }

        uint32_t GaussianMathFilter::GetElementCount() const
        {
            return GetSigmaAndTableSize().second;
        }

        uint32_t GaussianMathFilter::GetElementSize() const
        {
            return sizeof(float);
        }

        RHI::Format GaussianMathFilter::GetElementFormat() const
        {
            return RHI::Format::R32_FLOAT;
        }

        void GaussianMathFilter::StoreDataTo(void* dataPointer)
        {
            float* const bufferData = static_cast<float*>(dataPointer);

            // Get sigma (standard deviation) and table size.
            const auto sigmaAndTableSize = GetSigmaAndTableSize();
            const float sigma = sigmaAndTableSize.first;
            const uint32_t tableSize = sigmaAndTableSize.second;

            // If table size is 1, the filter is identical.  Set table with just 1.
            if (tableSize == 1)
            {
                bufferData[0] = 1.f;
                return;
            }

            // Calculate weight values of the table.
            // The table contains only first half since the weight value is symmetrical.
            float totalWeight = 0.f; // Total coefficient is determined later.
            for (uint32_t index = 0; index < tableSize; ++index)
            {
                // Calculate each weight.
                const float diff = ReliableSectionFactor * sigma * (tableSize - 1.f - index) / (tableSize - 1.f);
                const float weight = expf(-diff * diff / (2 * sigma * sigma));
                bufferData[index] = weight;
                totalWeight += weight;
            }

            // Fix total weight considering they contained weight values in first half.
            // Now total weight become sum of weight appearing in the symmetrical table.
            totalWeight = totalWeight * 2 - bufferData[tableSize - 1];

            // Total coefficient is determined.  Now fix the wieght values
            // to make the sum of weight values in the symmetrical table 1.0.
            for (uint32_t index = 0; index < tableSize; ++index)
            {
                bufferData[index] /= totalWeight;
            }
        }

        AZStd::string GaussianMathFilter::GenerateName(const GaussianFilterDescriptor& descriptor)
        {
            return AZStd::string::format("_%f", descriptor.m_standardDeviation);
        }

        AZStd::pair<float, uint32_t> GaussianMathFilter::GetSigmaAndTableSize() const
        {
            float sigma = m_descriptor.m_standardDeviation;
            if (sigma < 0.f || sigma > StandardDeviationMax)
            {
                AZ_Assert(false, "Standard Deviation should be between 0.0 and %f.", StandardDeviationMax);
                sigma = AZStd::GetMin(StandardDeviationMax, AZStd::GetMax(0.f, sigma));
            }

            const uint32_t windowSizeHalf = static_cast<uint32_t>(ceilf(sigma * ReliableSectionFactor));

            // We consider the kernel of this filter has
            // the width/height of 2 * windowSizeHalf + 1.
            // The filter consists of 2 passes (horizontal/vertical),
            // and its first half including the center are stored in the m_buffer.
            return AZStd::make_pair(sigma, windowSizeHalf + 1);
        }

    } // namespace Render
} // namespace AZ
