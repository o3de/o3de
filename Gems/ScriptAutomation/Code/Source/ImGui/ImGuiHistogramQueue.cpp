/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ImGuiHistogramQueue.h"
#include <AzCore/std/string/string.h>
#include <imgui/imgui.h>

namespace ScriptAutomation
{

    ImGuiHistogramQueue::ImGuiHistogramQueue(
        AZStd::size_t maxSamples,
        AZStd::size_t runningAverageSamples,
        float numericDisplayUpdateDelay)
        : m_maxSamples(maxSamples)
        , m_runningAverageSamples(runningAverageSamples)
        , m_numericDisplayDelay(numericDisplayUpdateDelay)
    {
        AZ_Assert(m_maxSamples >= m_runningAverageSamples, "maxSamples must be larger");

        m_valueLog.reserve(m_maxSamples);
        m_averageLog.reserve(m_maxSamples);
    }

    float ImGuiHistogramQueue::UpdateDisplayedValues(AZStd::size_t maxSampleCount, float& minValue, float& maxValue)
    {
        size_t sampleCount = AZStd::min<size_t>(maxSampleCount, m_valueLog.size());
        float average = 0.f;
        if (sampleCount > 0)
        {
            average = m_valueLog.at(0);
            minValue = maxValue = average;
            for (size_t i = 1; i < sampleCount; ++i)
            {
                float curValue = m_valueLog.at(i);
                average += curValue;
                minValue = minValue > curValue ? curValue : minValue;
                maxValue = maxValue < curValue ? curValue : maxValue;
            }
            average /= sampleCount;
        }
        return average;
    }

    void ImGuiHistogramQueue::PushValue(float value)
    {
        m_samplesSinceLastDisplayUpdate++;

        // Update the log of all values
        if (m_valueLog.size() == m_maxSamples)
        {
            m_valueLog.pop_back();
        }
        m_valueLog.insert(m_valueLog.begin(), value);

        // Calculate running average for line graph
        if (m_averageLog.size() == m_maxSamples)
        {
            m_averageLog.pop_back();
        }
        [[maybe_unused]] float minValue, maxValue; // unused required call parameters
        m_averageLog.insert(m_averageLog.begin(), UpdateDisplayedValues(m_runningAverageSamples, minValue, maxValue));

        // Calculate average for numeric display
        if (m_timeSinceLastDisplayUpdate >= m_numericDisplayDelay || m_samplesSinceLastDisplayUpdate >= m_maxSamples)
        {
            m_displayedAverage = UpdateDisplayedValues(m_maxSamples, m_displayedMinimum, m_displayedMaximum);

            m_timeSinceLastDisplayUpdate = 0.0f;
            m_samplesSinceLastDisplayUpdate = 0;
        }
    }

    void ImGuiHistogramQueue::Tick(float deltaTime, WidgetSettings settings)
    {
        if (m_averageLog.empty() || m_valueLog.empty())
        {
            return;
        }

        m_timeSinceLastDisplayUpdate += deltaTime;

        ImVec2 pos = ImGui::GetCursorPos();

        AZStd::string valueString;
        if (settings.m_reportInverse)
        {
            valueString  = AZStd::string::format("%4.2f %s", 1.0 / m_displayedAverage, settings.m_units);
        }
        else
        {
            valueString = AZStd::string::format("avg:%4.2f %s | min:%4.2f %s | max:%4.2f %s ", m_displayedAverage, settings.m_units, m_displayedMinimum, settings.m_units, m_displayedMaximum, settings.m_units);
        }

        // Draw moving average of values first
        ImGui::PushStyleColor(ImGuiCol_PlotLines, ImVec4(0.6, 0.8, 0.9, 1.0));
        ImGui::PlotLines("##Average", &m_averageLog[0], int32_t(m_averageLog.size()), 0, nullptr, 0.0f, m_displayedAverage * 2.0f, ImVec2(400, 50));
        ImGui::PopStyleColor();

        // Draw individual value bars on top of it (with no background).
        ImGui::SetCursorPos(pos);
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0, 0, 0, 0));
        ImGui::PlotHistogram("##Value", &m_valueLog[0], int32_t(m_valueLog.size()), 0, valueString.c_str(), 0.0f, m_displayedAverage * 2.0f, ImVec2(400, 50));
        ImGui::PopStyleColor();
    }

} // namespace ScriptAutomation
