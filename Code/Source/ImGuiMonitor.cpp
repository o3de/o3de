/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifdef IMGUI_ENABLED
#include <Allocators.h>
#include <ImGuiMonitor.h>

namespace EMotionFX::MotionMatching
{
    AZ_CLASS_ALLOCATOR_IMPL(ImGuiMonitor, MotionMatchAllocator, 0)

    ImGuiMonitor::ImGuiMonitor()
    {
        m_performanceStats.m_name = "Performance Statistics";

        m_featureCosts.m_name = "Feature Costs";
        m_featureCosts.m_histogramContainerCount = 100;

        ImGui::ImGuiUpdateListenerBus::Handler::BusConnect();
        ImGuiMonitorRequestBus::Handler::BusConnect();
    }

    ImGuiMonitor::~ImGuiMonitor()
    {
        ImGui::ImGuiUpdateListenerBus::Handler::BusDisconnect();
        ImGuiMonitorRequestBus::Handler::BusDisconnect();
    }

    void ImGuiMonitor::OnImGuiUpdate()
    {
        if (!m_performanceStats.m_show && !m_featureCosts.m_show)
        {
            return;
        }

        if (ImGui::Begin("Motion Matching"))
        {
            if (ImGui::CollapsingHeader("Feature Matrix", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed))
            {
                ImGui::Text("Memory Usage: %.2f MB", m_featureMatrixMemoryUsageInBytes / 1024.0f / 1024.0f);
                ImGui::Text("Num Frames: %zu", m_featureMatrixNumFrames);
                ImGui::Text("Num Feature Components: %zu", m_featureMatrixNumComponents);
            }

            if (ImGui::CollapsingHeader("Kd-Tree", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed))
            {
                ImGui::Text("Memory Usage: %.2f MB", m_kdTreeMemoryUsageInBytes / 1024.0f / 1024.0f);
                ImGui::Text("Num Nodes: %zu", m_kdTreeNumNodes);
                ImGui::Text("Num Dimensions: %zu", m_kdTreeNumDimensions);
            }

            m_performanceStats.OnImGuiUpdate();
            m_featureCosts.OnImGuiUpdate();
        }
    }

    void ImGuiMonitor::OnImGuiMainMenuUpdate()
    {
        if (ImGui::BeginMenu("Motion Matching"))
        {
            ImGui::MenuItem(m_performanceStats.m_name.c_str(), "", &m_performanceStats.m_show);
            ImGui::MenuItem(m_featureCosts.m_name.c_str(), "", &m_featureCosts.m_show);
            ImGui::EndMenu();
        }
    }

    void ImGuiMonitor::PushPerformanceHistogramValue(const char* performanceMetricName, float value)
    {
        m_performanceStats.PushHistogramValue(performanceMetricName, value, AZ::Color::CreateFromRgba(229,56,59,255));
    }

    void ImGuiMonitor::PushCostHistogramValue(const char* costName, float value, const AZ::Color& color)
    {
        m_featureCosts.PushHistogramValue(costName, value, color);
    }

    void ImGuiMonitor::HistogramGroup::PushHistogramValue(const char* valueName, float value, const AZ::Color& color)
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
                /*containerCount=*/m_histogramContainerCount,
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

    void ImGuiMonitor::HistogramGroup::OnImGuiUpdate()
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
} // namespace EMotionFX::MotionMatching

#endif // IMGUI_ENABLED
