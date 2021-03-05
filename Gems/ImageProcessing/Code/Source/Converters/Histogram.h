/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#pragma once

namespace ImageProcessing
{
    template <size_t BIN_COUNT>
    class Histogram
    {
    public:
        typedef AZ::u64 Bins[BIN_COUNT];

    public:
        Histogram()
        {
        }

        static void clearBins(Bins& bins)
        {
            memset(&bins, 0, sizeof(bins));
        }

        void set(const Bins& bins)
        {
            m_bins[0] = bins[0];
            m_binsCumulative[0] = bins[0];
            double sum = 0.0f;
            for (size_t i = 1; i < BIN_COUNT; ++i)
            {
                m_bins[i] = bins[i];
                m_binsCumulative[i] = m_binsCumulative[i - 1] + bins[i];
                sum += i * double(bins[i]);
            }

            const AZ::u64 totalCount = getTotalSampleCount();
            m_meanBin = (totalCount <= 0) ? 0.0f : float(sum / totalCount);
        }

        AZ::u64 getTotalSampleCount() const
        {
            return m_binsCumulative[BIN_COUNT - 1];
        }

        float getPercentage(size_t minBin, size_t maxBin) const
        {
            const AZ::u64 totalCount = getTotalSampleCount();

            if ((totalCount <= 0) || (minBin > maxBin) || (maxBin < 0) || (minBin >= BIN_COUNT))
            {
                return 0.0f;
            }

            minBin = AZ::GetMax(minBin, size_t(0));
            maxBin = AZ::GetMin(maxBin, BIN_COUNT-1);

            const AZ::u64 count = m_binsCumulative[maxBin] - ((minBin <= 0) ? 0 : m_binsCumulative[minBin-1]);

            return float((double(count) * 100.0) / double(totalCount));
        }

        float getMeanBin() const
        {
            return m_meanBin;
        }

    private:
        Bins m_bins;
        Bins m_binsCumulative;
        float m_meanBin;
    };

    bool ComputeLuminanceHistogram(IImageObjectPtr imageObject, Histogram<256>& histogram);
}
