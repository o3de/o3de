/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <imgui/imgui.h>

namespace AZ::Render
{
    namespace ImGuiShaderUtils
    {
        void DrawShaderVariantTable(const AZ::RPI::ShaderOptionGroupLayout* layout, AZ::RPI::ShaderVariantId requestedVariantId, AZ::RPI::ShaderVariantId selectedVariantId)
        {
            AZ::RPI::ShaderOptionGroup requestedShaderVariantOptions{layout, requestedVariantId};
            AZ::RPI::ShaderOptionGroup selectedShaderVariantOptions{layout, selectedVariantId};
            auto& shaderOptionDescriptors = layout->GetShaderOptions();

            // Note I tried using ImGui columns but they don't work so well, and I already had this manual formatting approach shelved elsewhere
            // so decided to just stick with this is it seems to be more reliable this way.

            int longestOptionNameLength = 0;
            for (auto& shaderOption : shaderOptionDescriptors)
            {
                longestOptionNameLength = AZStd::GetMax(longestOptionNameLength, aznumeric_cast<int>(shaderOption.GetName().GetStringView().size()));
            }

            AZStd::string header = AZStd::string::format("%-*s |  Bits | Requested | Selected", longestOptionNameLength, "Option Name");
            AZStd::string divider{header.size(), '-'};

            ImGui::Text("%s", header.c_str());
            ImGui::Text("%s", divider.c_str());

            for (size_t i = 0; i < shaderOptionDescriptors.size(); ++i)
            {
                // TODO Consider giving ShaderOptionDescriptor its index value so we can use for-each.
                const AZ::RPI::ShaderOptionDescriptor& shaderOptionDesc = shaderOptionDescriptors[i];
                AZ::RPI::ShaderOptionIndex optionIndex{i};

                const char* optionName = shaderOptionDesc.GetName().GetCStr();
                AZ::RPI::ShaderOptionValue requestedValue = requestedShaderVariantOptions.GetValue(optionIndex);
                AZ::RPI::ShaderOptionValue selectedValue = selectedShaderVariantOptions.GetValue(optionIndex);

                AZStd::string optionNameLabel = AZStd::string::format("%-*s", longestOptionNameLength, optionName);
                AzFramework::StringFunc::Replace(optionNameLabel, ' ', '.');

                if (shaderOptionDesc.GetBitCount() == 1)
                {
                    ImGui::Text("%s | %4d  | %9d | %8d",
                        optionNameLabel.c_str(),
                        shaderOptionDesc.GetBitOffset(),
                        requestedValue.GetIndex(),
                        selectedValue.GetIndex());
                }
                else
                {
                    ImGui::Text("%s | %2d-%-2d | %9d | %8d",
                        optionNameLabel.c_str(),
                        shaderOptionDesc.GetBitOffset(),
                        shaderOptionDesc.GetBitOffset() + shaderOptionDesc.GetBitCount() - 1,
                        requestedValue.GetIndex(),
                        selectedValue.GetIndex());
                }
            }

        }

    } // namespace Utils
} // namespace AtomSampleViewer
