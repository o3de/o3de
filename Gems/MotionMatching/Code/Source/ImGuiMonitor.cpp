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
    AZ_CLASS_ALLOCATOR_IMPL(ImGuiMonitor, MotionMatchAllocator)

    ImGuiMonitor::ImGuiMonitor()
    {
        m_performanceStats.SetName("Performance Statistics");
        m_performanceStats.SetHistogramBinCount(500);

        m_featureCosts.SetName("Feature Costs");
        m_featureCosts.SetHistogramBinCount(100);

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
            if (ImGui::CollapsingHeader("Motion Database", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed))
            {
                if (ImGui::BeginTable("MDB", 2))
                {
                    ImGui::TableNextColumn(); ImGui::Text("Memory Usage: %.2f MB", m_frameDatabaseInfo.m_memoryUsedInBytes / 1024.0f / 1024.0f);
                    ImGui::TableNextColumn(); ImGui::Text("Motion Data: %.0f minutes", m_frameDatabaseInfo.m_motionDataInSeconds / 60.0f);
                    ImGui::TableNextColumn(); ImGui::Text("Num Frames: %zu", m_frameDatabaseInfo.m_numFrames);
                    ImGui::TableNextColumn(); ImGui::Text("Num Motions: %zu", m_frameDatabaseInfo.m_numMotions);
                    ImGui::EndTable();
                }
            }

            if (ImGui::CollapsingHeader("Feature Matrix", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed))
            {
                if (ImGui::BeginTable("FM", 2))
                {
                    ImGui::TableNextColumn(); ImGui::Text("Memory Usage: %.2f MB", m_featurMatrixInfo.m_memoryUsedInBytes / 1024.0f / 1024.0f);
                    ImGui::TableNextColumn();
                    ImGui::TableNextColumn(); ImGui::Text("Num Frames: %zu", m_featurMatrixInfo.m_numFrames);
                    ImGui::TableNextColumn(); ImGui::Text("Num Feature Components: %zu", m_featurMatrixInfo.m_numDimensions);
                    ImGui::EndTable();
                }
            }

            if (ImGui::CollapsingHeader("Kd-Tree", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed))
            {
                if (ImGui::BeginTable("KDT", 2))
                {
                    ImGui::TableNextColumn(); ImGui::Text("Memory Usage: %.2f MB", m_kdTreeInfo.m_memoryUsedInBytes / 1024.0f / 1024.0f);
                    ImGui::TableNextColumn();
                    ImGui::TableNextColumn(); ImGui::Text("Num Nodes: %zu", m_kdTreeInfo.m_numNodes);
                    ImGui::TableNextColumn(); ImGui::Text("Num Dimensions: %zu", m_kdTreeInfo.m_numDimensions);
                    ImGui::EndTable();
                }
            }

            m_performanceStats.OnImGuiUpdate();
            m_featureCosts.OnImGuiUpdate();
        }
    }

    void ImGuiMonitor::OnImGuiMainMenuUpdate()
    {
        if (ImGui::BeginMenu("Motion Matching"))
        {
            ImGui::MenuItem(m_performanceStats.GetName(), "", &m_performanceStats.m_show);
            ImGui::MenuItem(m_featureCosts.GetName(), "", &m_featureCosts.m_show);
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
} // namespace EMotionFX::MotionMatching

#endif // IMGUI_ENABLED
