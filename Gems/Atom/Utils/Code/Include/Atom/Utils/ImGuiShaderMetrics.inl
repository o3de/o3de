/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Public/Shader/Metrics/ShaderMetricsSystemInterface.h>

namespace AZ
{
    namespace Render
    {
        inline void ImGuiShaderMetrics::Draw(bool& draw, const RPI::ShaderVariantMetrics& metrics)
        {
            ImVec2 windowSize(600, 500.f);
            ImGui::SetNextWindowSize(windowSize, ImGuiCond_Always);
            if (ImGui::Begin("Shader Metrics", &draw, ImGuiWindowFlags_None))
            {
                if (ImGui::Button("Reset"))
                {
                    AZ::RPI::ShaderMetricsSystemInterface::Get()->Reset();
                }

                bool enableMetrics = AZ::RPI::ShaderMetricsSystemInterface::Get()->IsEnabled();
                if (ImGui::Checkbox("Enable Metrics", &enableMetrics))
                {
                    AZ::RPI::ShaderMetricsSystemInterface::Get()->SetEnabled(enableMetrics);
                }

                ImGui::Separator();

                // Set column settings.
                ImGui::Columns(4, "view", false);
                ImGui::SetColumnWidth(0, 100.0f);
                ImGui::SetColumnWidth(1, 300.0f);
                ImGui::SetColumnWidth(2, 100.0f);
                ImGui::SetColumnWidth(3, 100.0f);

                ImGui::Text("Requests");
                ImGui::NextColumn();

                ImGui::Text("Shader");
                ImGui::NextColumn();

                ImGui::Text("Variant");
                ImGui::NextColumn();

                ImGui::Text("Branches");
                ImGui::NextColumn();

                AZStd::vector<RPI::ShaderVariantRequest> requests = metrics.m_requests;

                auto sortFunction = [](const RPI::ShaderVariantRequest& left, const RPI::ShaderVariantRequest& right) {return left.m_requestCount > right.m_requestCount; };

                AZStd::sort(requests.begin(), requests.end(), sortFunction);

                for (const auto& request : requests)
                {
                    ImGui::Text("%d", request.m_requestCount);
                    ImGui::NextColumn();

                    ImGui::Text("%s", request.m_shaderName.GetCStr());
                    ImGui::NextColumn();

                    ImGui::Text("%d", request.m_shaderVariantStableId.GetIndex());
                    ImGui::NextColumn();

                    if (request.m_dynamicOptionCount == 0)
                    {
                        ImGui::Text("%d", request.m_dynamicOptionCount);
                    }
                    else
                    {
                        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "%d", request.m_dynamicOptionCount);
                    }
                    ImGui::NextColumn();
                }
            }
            ImGui::End();
        }
    } // namespace Render
} // namespace AZ
