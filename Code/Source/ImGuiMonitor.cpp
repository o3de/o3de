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

#ifdef IMGUI_ENABLED
#include <Allocators.h>
#include <ImGuiMonitor.h>

namespace EMotionFX::MotionMatching
{
    AZ_CLASS_ALLOCATOR_IMPL(ImGuiMonitor, MotionMatchAllocator, 0)

    ImGuiMonitor::ImGuiMonitor()
    {
        m_performanceStats.m_name = "Performance Statistics";
        m_performanceStats.m_barColor = ImColor(206, 0, 13);

        m_featureCosts.m_name = "Feature Costs";
        m_featureCosts.m_histogramContainerCount = 128;
        m_featureCosts.m_barColor = ImColor(66, 166, 178);

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
        m_performanceStats.PushHistogramValue(performanceMetricName, value);
    }

    void ImGuiMonitor::PushCostHistogramValue(const char* costName, float value)
    {
        m_featureCosts.PushHistogramValue(costName, value);
    }

    void ImGuiMonitor::HistogramGroup::PushHistogramValue(const char* valueName, float value)
    {
        auto iterator = m_histogramIndexByName.find(valueName);
        if (iterator != m_histogramIndexByName.end())
        {
            m_histograms[iterator->second].PushValue(value);
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
                ImGui::PushStyleColor(ImGuiCol_PlotHistogram, m_barColor.Value);
                histogram.Draw(ImGui::GetColumnWidth(), s_histogramHeight);
                ImGui::PopStyleColor();
            }
        }
    }
} // namespace EMotionFX::MotionMatching

#endif // IMGUI_ENABLED
