/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifdef IMGUI_ENABLED
#include "LYImGuiUtils/HistogramGroup.h"

namespace ImGui::LYImGuiUtils
{
    HistogramGroup::HistogramGroup(const char* name, int histogramBinCount)
        : m_name(name)
        , m_histogramBinCount(histogramBinCount)
    {
    }

    void HistogramGroup::PushHistogramValue(const char* valueName, float value, const AZ::Color& color)
    {
        auto iterator = m_histogramIndexByName.find(valueName);
        if (iterator != m_histogramIndexByName.end())
        {
            ImGui::LYImGuiUtils::HistogramContainer& histogramContiner = m_histograms[iterator->second];
            histogramContiner.PushValue(value);
            histogramContiner.SetBarLineColor(ImColor(color.GetR(), color.GetG(), color.GetB(), color.GetA()));
        }
        else
        {
            ImGui::LYImGuiUtils::HistogramContainer newHistogram;
            newHistogram.Init(/*histogramName=*/valueName,
                /*containerCount=*/m_histogramBinCount,
                /*viewType=*/ImGui::LYImGuiUtils::HistogramContainer::ViewType::Histogram,
                /*displayOverlays=*/true,
                /*min=*/0.0f,
                /*max=*/0.0f);

            newHistogram.SetMoveDirection(ImGui::LYImGuiUtils::HistogramContainer::PushRightMoveLeft);
            newHistogram.PushValue(value);

            m_histogramIndexByName[valueName] = m_histograms.size();
            m_histograms.push_back(newHistogram);
        }
    }

    void HistogramGroup::OnImGuiUpdate()
    {
        if (!m_show)
        {
            return;
        }

        if (ImGui::CollapsingHeader(m_name.c_str(), ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed))
        {
            for (auto& histogram : m_histograms)
            {
                ImGui::BeginGroup();
                {
                    histogram.Draw(ImGui::GetColumnWidth() - 70, s_histogramHeight);

                    ImGui::SameLine();

                    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(0,0,0,255));
                    {
                        const ImColor color = histogram.GetBarLineColor();
                        ImGui::PushStyleColor(ImGuiCol_Button, color.Value);
                        {
                            const AZStd::string valueString = AZStd::string::format("%.2f", histogram.GetLastValue());
                            ImGui::Button(valueString.c_str());
                        }
                        ImGui::PopStyleColor();
                    }
                    ImGui::PopStyleColor();
                }
                ImGui::EndGroup();
            }
        }
    }
} // namespace ImGui::LYImGuiUtils

#endif // IMGUI_ENABLED
