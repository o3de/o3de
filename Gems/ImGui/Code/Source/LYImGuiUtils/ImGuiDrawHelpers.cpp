/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifdef IMGUI_ENABLED
#include "LYImGuiUtils/ImGuiDrawHelpers.h"
#include "ImGuiColorDefines.h"

namespace ImGui
{
    namespace LYImGuiUtils
    {
        void Draw2DExpCurve([[maybe_unused]] const char* name, const char* id, const ImVec2& size, const float exp, const float val /*= -1.0f*/)
        {
            // Function for exponential curve
            auto expCurveFunc = [](float val, float exp) -> float
            {
                return powf(fabs(val), exp) * AZ::GetSign(val);
            };
            const float curveFidelity = 2.0f;
            ImVec2 graphSize(size.x - 3.0f, size.y - 3.0f);

            ImGui::BeginChild(id, size, true);
            
            ImVec2 lastPoint(0.0f, 0.0f);
            for (float xVal = curveFidelity; xVal <= (graphSize.x + curveFidelity); xVal += curveFidelity)
            {
                ImVec2 nextPoint(xVal, graphSize.y - (expCurveFunc(xVal / graphSize.x, exp)*graphSize.y));
                ImGui::GetWindowDrawList()->AddLine(ImVec2(ImGui::GetWindowPos().x + lastPoint.x, ImGui::GetWindowPos().y + lastPoint.y),
                    ImVec2(ImGui::GetWindowPos().x + nextPoint.x, ImGui::GetWindowPos().y + nextPoint.y),
                    ImGui::ColorConvertFloat4ToU32(ImGui_Col_Salmon), 1.0f);

                lastPoint = nextPoint;
            }

            ImGui::GetWindowDrawList()->AddCircleFilled(ImVec2(ImGui::GetWindowPos().x + (graphSize.x * fabs(val)), ImGui::GetWindowPos().y + graphSize.y - (graphSize.y * expCurveFunc(val, exp))),
                3.0f, ImGui::ColorConvertFloat4ToU32(ImGui_Col_Salmon), 20);

            ImGui::EndChild(); // Input Curve Graph Container
        }

        void DrawLYCVarCheckbox(const char* cVar, const char* title, ICVar* cvar)
        {
            if (cvar)
            {
                // Find boolean CVAR state
                const bool cVarState = (cvar->GetIVal() != 0);
                bool cVarCheckboxValue = cVarState;

                // Create label string 
                const AZStd::string& labelText = AZStd::string::format("%s %s (Click Checkbox to Toggle)", title, cVarCheckboxValue ? "On" : "Off");
                ImGui::Checkbox(labelText.c_str(), &cVarCheckboxValue);
                if (cVarCheckboxValue != cVarState)
                {
                    cvar->Set(cVarCheckboxValue);
                }
                if (ImGui::IsItemHovered())
                {
                    ImGui::BeginTooltip();
                    ImGui::TextColored(ImGui_Col_Salmon, "'%s' = %d", cVar, cvar->GetIVal());
                    ImGui::EndTooltip();
                }
            }
        }
    }
}
#endif // #ifdef IMGUI_ENABLED
