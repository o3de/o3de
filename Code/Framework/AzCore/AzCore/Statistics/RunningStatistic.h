/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/base.h>

namespace AZ
{
    namespace Statistics
    {
        /**
         * The name P and S come from Excel's function names.
         * Examples: VAR.P() vs VAR.S()
         *           STDEV.P() vs STDEV.S()
         * In terms of benchmarking, the S version is preferred because in reality
         * a function can be benchmarked ad infinitum and we would never know its real
         * average, so we will always be capturing a subsample of an infinite population,
         * and it is wise in this case to apply the unbias factor (divide by n-1, instead of divide by n).
         * This is also known as Bessel's correction:
         * https://en.wikipedia.org/wiki/Bessel%27s_correction
         */
        enum class VarianceType
        {
            P, ///Use this when calculating variance or standard deviation over a finite population.
            S  ///Use this when given a subset of a bigger population (Default).
        };

        /**
         * @brief Keeps track of the running statistics of a given value
         *
         * Efficiently keeps track of min, max, average, variance and stdev without
         * storing the value's history.
         * Based on 1962 paper by B. P. Welford.
         * https://en.wikipedia.org/wiki/Algorithms_for_calculating_variance
         * The algorithm here is the Two-pass algorithm which is numerically stable.
         */
        class RunningStatistic
        {
        public:
            RunningStatistic()
            {
                Reset();
            }
            virtual ~RunningStatistic() = default;

            void Reset();

            void PushSample(double value);

            AZ::u64 GetNumSamples() const
            {
                return m_numSamples;
            }

            double GetMostRecentSample() const
            {
                return m_mostRecentSample;
            }

            double GetMinimum() const
            {
                return m_minimum;
            }

            double GetMaximum() const
            {
                return m_maximum;
            }

            double GetAverage() const
            {
                return m_average;
            }

            double GetSum() const
            {
                return m_sum;
            }

            /**
             * See declaration of enum class VarianceType above on why we choose the S type by default.
             */
            double GetVariance(VarianceType varianceType = VarianceType::S) const;

            /**
             * See declaration of enum class VarianceType above on why we choose the S type by default.
             */
            double GetStdev(VarianceType varianceType = VarianceType::S) const;

        private:
            AZ::u64 m_numSamples;       /// How many times the method PushSample() has been called.
            double m_mostRecentSample;  /// Always returns the most recently pushed sample value.
            double m_minimum;           /// The minimum value of all samples pushed (since Reset).
            double m_maximum;           /// The maximum value of all samples pushed (since Reset).
            double m_sum;               /// The sum toal of all sample values pushed (since Reset).
            double m_average;           /// The running average of all sample values pushed (since Reset).
            double m_varianceTracking;  /// Not the variance, but used to keep track of variance calculation.
        };
    }//namespace Statistics
}//namespace AZ
