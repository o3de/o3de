/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/containers/vector.h>

namespace ScriptAutomation
{
    //! Tracks time values over multiple frames, computes the average, and draws a historgram.
    class ImGuiHistogramQueue 
    {
    public:
        //! @param maxSamples the max number of samples that can be recorded in the queue and displayed in the histogram
        //! @param runningAverageSamples the number of samples to use for calculating running average hash-marks that are overlaid on the histogram
        //! @param numericDisplayUpdateDelay the number of seconds to delay between updates of the numeric display
        ImGuiHistogramQueue(
            AZStd::size_t maxSamples,
            AZStd::size_t runningAverageSamples,
            float numericDisplayUpdateDelay = 0.25f);

        struct WidgetSettings
        {
            bool m_reportInverse = false; //!< Use 1/average instead of average for displaying the numeric value
            const char* m_units = "";
        };

        void PushValue(float value);
        void Tick(float deltaTime, WidgetSettings settings);

        float GetDisplayedAverage() const { return m_displayedAverage; }
        float GetDisplayedMinimum() const { return m_displayedMinimum; }
        float GetDisplayedMaximum() const { return m_displayedMaximum; }

    private:

        float UpdateDisplayedValues(AZStd::size_t maxSampleCount, float& minValue, float& maxValue);

        AZStd::vector<float> m_valueLog;
        AZStd::vector<float> m_averageLog;

        const AZStd::size_t m_maxSamples;
        const AZStd::size_t m_runningAverageSamples;
        const float m_numericDisplayDelay;

        float m_timeSinceLastDisplayUpdate = 0.0f;
        int m_samplesSinceLastDisplayUpdate = 0;

        float m_displayedAverage = 0.0f;
        float m_displayedMinimum = 0.0f;
        float m_displayedMaximum = 0.0f;
    };

} // namespace ScriptAutomation
