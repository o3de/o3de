/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "ImGui_precompiled.h"
#include "LYImGuiUtils/HistogramContainer.h"

#ifdef IMGUI_ENABLED

#include "ImGuiColorDefines.h"

namespace ImGui
{
    namespace LYImGuiUtils
    {
        void HistogramContainer::Init(const char* histogramName, int maxValueCountSize, ViewType viewType, bool displayOverlays, float minScale, float maxScale
            , bool autoExpandScale, bool startCollapsed/* = false*/, bool drawMostRecentValue/* = true*/)
        {
            m_histogramName = histogramName;
            m_minScale = minScale;
            m_maxScale = maxScale;
            m_viewType = viewType;
            m_dispalyOverlays = displayOverlays;
            m_autoExpandScale = autoExpandScale;
            m_collapsed = startCollapsed;
            m_drawMostRecentValueText = drawMostRecentValue;
            SetMaxSize(maxValueCountSize);
        }

        void HistogramContainer::PushValue(float val)
        {
            if (m_maxSize == 0)
            {
                return;
            }
            if (m_values.size() == m_maxSize)
            {
                m_values.pop_back();
            }
            else if (m_values.size() > m_maxSize)
            {
                m_values.erase(m_values.begin() + (m_maxSize - 1), m_values.end());
            }
            m_values.push_front(val);

            if (m_autoExpandScale)
            {
                if (val < m_minScale)
                {
                    m_minScale = val;
                }
                else if (val > m_maxScale)
                {
                    m_maxScale = val;
                }
            }
        }

        static auto s_viewTypeToStringLambda = []([[maybe_unused]] void* histogramPtr, int idx, const char** out_text)
        {
            *out_text = HistogramContainer::ViewTypeToString(static_cast<HistogramContainer::ViewType>(idx));
            return true;
        };

        void HistogramContainer::Draw(float histogramWidth, float histogramHeight)
        {
            // If we are collapsed. Force a much smaller height
            if (m_collapsed)
            {
                histogramHeight = 24.0f;
            }
            // Begin Child area
            ImGui::BeginChild(AZStd::string::format("##%s_child", m_histogramName.c_str()).c_str(), ImVec2(histogramWidth, histogramHeight), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

            // Right Click Options menu
            if (ImGui::BeginPopupContextItem(AZStd::string::format("histoContextMenu##%s", m_histogramName.c_str()).c_str(), 1))
            {
                ImGui::TextColored(ImGui_Col_White, "'%s' Histogram Options", m_histogramName.c_str());

                ImGui::Combo("View Type", reinterpret_cast<int*>(&m_viewType), s_viewTypeToStringLambda, this, static_cast<int>(ViewType::COUNT));
                ImGui::Checkbox("Show Overlays", &m_dispalyOverlays);
                ImGui::Checkbox("Show Most Recent Value (requires overlays)", &m_drawMostRecentValueText);
                ImGui::DragInt("History Size", &m_maxSize, 1, 1, 1000, "%f");
                ImGui::DragFloat("Max Scale", &m_maxScale, 0.0001f, -100.0f, 100.0f);
                ImGui::DragFloat("Min Scale", &m_minScale, 0.0001f, -100.0f, 100.0f);
                ImGui::Checkbox("Auto Expand Scale", &m_autoExpandScale);

                ImGui::EndPopup();
            }

            // Toggle collapsing when double clicking this "window"
            if (ImGui::IsMouseDoubleClicked(0) && ImGui::IsWindowHovered())
            {
                m_collapsed = !m_collapsed;
            }

            // Plot the actual Imgui Histogram/Lines widgets.. Plotting size zero can crash ImGui!
            float imGuiHistoWidgetHeight = m_collapsed ? histogramHeight : (histogramHeight - 15);
            if (GetSize() > 0)
            {
                switch (m_viewType)
                {
                    default:
                        break;

                    case ViewType::Histogram:
                        ImGui::PlotHistogram(AZStd::string::format("##%s_histo", m_histogramName.c_str()).c_str(), ImGui::LYImGuiUtils::s_histogramContainerGetter, this, GetSize(), 0, m_histogramName.c_str(), m_minScale, m_maxScale, ImVec2(histogramWidth - 10, imGuiHistoWidgetHeight));
                        break;

                    case ViewType::Lines:
                        ImGui::PlotLines(AZStd::string::format("##%s_lines", m_histogramName.c_str()).c_str(), ImGui::LYImGuiUtils::s_histogramContainerGetter, this, GetSize(), 0, m_histogramName.c_str(), m_minScale, m_maxScale, ImVec2(histogramWidth - 10, imGuiHistoWidgetHeight));
                        break;
                }
            }


            // Display optional overlays to help with scale
            if (m_dispalyOverlays && !m_collapsed)
            {
                AZStd::string labelText = AZStd::string::format("% .03f", m_maxScale);
                ImGui::GetWindowDrawList()->AddText(ImVec2(ImGui::GetWindowPos().x + 12.0f, ImGui::GetWindowPos().y + 10.0f), ImGui::ColorConvertFloat4ToU32(ImGui_Col_White), labelText.c_str());

                float midPointY = ImGui::GetWindowPos().y + histogramHeight * 0.5f + 1.0f;
                labelText = AZStd::string::format("% .03f", (m_maxScale - m_minScale) * 0.5f + m_minScale);
                ImGui::GetWindowDrawList()->AddText(ImVec2(ImGui::GetWindowPos().x + 12.0f, midPointY - 6.0f), ImGui::ColorConvertFloat4ToU32(ImGui_Col_White), labelText.c_str());

                labelText = AZStd::string::format("% .03f", m_minScale);
                ImGui::GetWindowDrawList()->AddText(ImVec2(ImGui::GetWindowPos().x + 12.0f, ImGui::GetWindowPos().y + histogramHeight - 22.0f), ImGui::ColorConvertFloat4ToU32(ImGui_Col_White), labelText.c_str());

                ImGui::GetWindowDrawList()->AddLine(ImVec2(ImGui::GetWindowPos().x + 50.0f, midPointY), ImVec2(ImGui::GetWindowPos().x + histogramWidth - 7.0f, midPointY), ImGui_Col_White, 0.5f);

                if (m_drawMostRecentValueText && !m_values.empty())
                {
                    labelText = AZStd::string::format("% .03f", m_values.front());
                    ImGui::GetWindowDrawList()->AddText(ImVec2(ImGui::GetWindowPos().x + 12.0f, midPointY - 18.0f), ImGui::ColorConvertFloat4ToU32(ImGui_Col_DarkBlue), labelText.c_str());
                }
            }

            // End Child Area
            ImGui::EndChild();
        }

        const char* HistogramContainer::ViewTypeToString(ViewType viewType)
        {
            switch (viewType)
            {
                default:
                    return "*Unimplemented";

                case ViewType::Histogram:
                    return "Histogram";

                case ViewType::Lines:
                    return "Lines";
            }
        }
    }
}
#endif // #ifdef IMGUI_ENABLED
