/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/Utils/ImGuiFeatureConfig.h>

#include <AzCore/std/containers/array.h>
#include <imgui/imgui.h>
#include <AzCore/Console/Console.h>

AZ_CVAR_EXTERNED(float, r_renderScale);
AZ_CVAR_EXTERNED(float, r_renderScaleMin);
AZ_CVAR_EXTERNED(float, r_renderScaleMax);
AZ_CVAR_EXTERNED(bool, r_fsr2SharpeningEnabled);
AZ_CVAR_EXTERNED(float, r_fsr2SharpeningStrength);

namespace AZ::Render
{
    struct Fsr2Preset
    {
        const char* label;
        float renderScale;
    };
    // These values correspond to various FSR2 quality modes defined in the FfxFsr2QualityMode enum
    constexpr static AZStd::array<Fsr2Preset, 5> Fsr2Presets = { Fsr2Preset{ "Display Resolution (No upscaling)", 1.f },
                                                                 Fsr2Preset{ "Ultra (1.5x upscale)", 1.5f },
                                                                 Fsr2Preset{ "High (1.7x upscale)", 1.7f },
                                                                 Fsr2Preset{ "Medium (2x upscale)", 2.f },
                                                                 Fsr2Preset{ "Low (3x upscale)", 3.f } };

    void ImGuiFeatureConfig::Draw(bool& draw)
    {
        if (ImGui::Begin("Feature Config", &draw, 0))
        {
            float renderScale = r_renderScale;
            float renderScaleMin = r_renderScaleMin;
            float renderScaleMax = r_renderScaleMax;

            ImGui::Text("Default FSR2 Presets");
            for (const Fsr2Preset& preset : Fsr2Presets)
            {
                bool selected = renderScale == preset.renderScale;
                if (ImGui::RadioButton(preset.label, selected))
                {
                    r_renderScale = preset.renderScale;
                }
            }
            ImGui::Separator();

            if (ImGui::SliderFloat("Render Scale", &renderScale, renderScaleMin, renderScaleMax))
            {
                r_renderScale = renderScale;
            }

            if (ImGui::SliderFloat("Render Scale Min", &renderScaleMin, 1.f, renderScaleMax))
            {
                r_renderScaleMin = renderScaleMin;
            }

            if (ImGui::SliderFloat("Render Scale Max", &renderScaleMax, renderScaleMin, 3.f))
            {
                r_renderScaleMax = renderScaleMax;
            }

            if (IConsole* console = AZ::Interface<IConsole>::Get())
            {
                bool fsr2SharpeningEnabled = true;
                console->GetCvarValue("r_fsr2SharpeningEnabled", fsr2SharpeningEnabled);
                if (ImGui::Checkbox("FSR2 (RCAS) Sharpening", &fsr2SharpeningEnabled))
                {
                    if (fsr2SharpeningEnabled)
                    {
                        console->PerformCommand("r_fsr2SharpeningEnabled 1");
                    }
                    else
                    {
                        console->PerformCommand("r_fsr2SharpeningEnabled 0");
                    }
                }

                float fsr2SharpeningStrength = 0.8f;
                console->GetCvarValue("r_fsr2SharpeningStrength", fsr2SharpeningStrength);
                if (ImGui::SliderFloat("FSR2 Sharpening Strength", &fsr2SharpeningStrength, 0.f, 1.f))
                {
                    console->PerformCommand(AZStd::string::format("r_fsr2SharpeningStrength %f", fsr2SharpeningStrength).c_str());
                }

                ImGui::Separator();

                uint32_t vsyncInterval = 1;
                console->GetCvarValue("vsync_interval", vsyncInterval);
                bool vsyncEnabled = vsyncInterval >= 1;
                if (ImGui::Checkbox("Vsync", &vsyncEnabled))
                {
                    if (vsyncEnabled)
                    {
                        console->PerformCommand("vsync_interval 1");
                    }
                    else
                    {
                        console->PerformCommand("vsync_interval 0");
                    }
                }
            }
        }
        ImGui::End();
    }
} // namespace AZ::Render
