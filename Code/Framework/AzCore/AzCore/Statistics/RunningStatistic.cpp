/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <cmath>

#include "RunningStatistic.h"

namespace AZ::Statistics
{
    void RunningStatistic::Reset()
    {
        m_numSamples = 0;
        m_mostRecentSample = 0.0;
        m_minimum = 0.0;
        m_maximum = 0.0;
        m_sum = 0.0;
        m_average = 0.0;
        m_varianceTracking = 0.0;
    }

    void RunningStatistic::PushSample(double value)
    {
        m_numSamples++;
        m_mostRecentSample = value;
        m_sum += value;

        if (m_numSamples == 1)
        {
            m_minimum = value;
            m_maximum = value;
            m_average = value;
            return;
        }

        if (value < m_minimum)
        {
            m_minimum = value;
        }
        else if (value > m_maximum)
        {
            m_maximum = value;
        }

        //See header notes and references to understand this way of calculating
        //running average & variance.
        const double newAverage = m_average + (value - m_average) / m_numSamples;
        m_varianceTracking = m_varianceTracking + (value - m_average)*(value - newAverage);

        m_average = newAverage;
    }

    double RunningStatistic::GetVariance(VarianceType varianceType) const
    {
        if (m_numSamples > 1)
        {
            const AZ::u64 varianceDivisor = (varianceType == VarianceType::S) ? m_numSamples - 1 : m_numSamples;
            return m_varianceTracking / varianceDivisor;
        }
        return 0.0;
    }

    double RunningStatistic::GetStdev(VarianceType varianceType) const
    {
        return sqrt(GetVariance(varianceType));
    }

} // namespace AZ::Statistics
