/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/Utils/ImGuiShaderUtils.h>
#include <imgui/imgui.h>
#include <Atom/RPI.Public/Material/Material.h>

#include <AzCore/Casting/numeric_cast.h>

namespace AZ::Render
{
    inline void ImGuiMaterialDetails::SetMaterial(AZ::Data::Instance<AZ::RPI::Material> material)
    {
        m_material = material;
    }

    inline void ImGuiMaterialDetails::OpenDialog()
    {
        m_dialogIsOpen = true;
    }

    inline void ImGuiMaterialDetails::Tick()
    {
        if (m_dialogIsOpen)
        {
            if (ImGui::Begin("Material Details", &m_dialogIsOpen, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings))
            {
                if (m_material)
                {
                    const ImGuiTreeNodeFlags flagDefaultOpen = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_DefaultOpen;
                    if (ImGui::TreeNodeEx("Shaders", flagDefaultOpen))
                    {
                        for (size_t i = 0; i < m_material->GetShaderCollection().size(); i++)
                        {
                            const AZ::RPI::ShaderCollection::Item& shaderItem = m_material->GetShaderCollection()[i];

                            if (ImGui::TreeNodeEx(AZStd::string::format("Shader[%d] - Enabled=%d - %s", aznumeric_cast<int>(i), shaderItem.IsEnabled(), shaderItem.GetShaderAsset()->GetName().GetCStr()).c_str(), flagDefaultOpen))
                            {
                                if (shaderItem.IsEnabled())
                                {
                                    AZ::RPI::ShaderVariantId requestedVariantId = shaderItem.GetShaderVariantId();
                                    AZ::Data::Asset<AZ::RPI::ShaderVariantAsset> selectedVariantAsset =
                                        shaderItem.GetShaderAsset()->GetVariantAsset(requestedVariantId);

                                    if (!selectedVariantAsset)
                                    {
                                        selectedVariantAsset = shaderItem.GetShaderAsset()->GetRootVariantAsset();
                                    }

                                    if (selectedVariantAsset)
                                    {
                                        AZ::RPI::ShaderVariantId selectedVariantId = selectedVariantAsset->GetShaderVariantId();

                                        ImGui::Indent();

                                        ImGuiShaderUtils::DrawShaderVariantTable(
                                            shaderItem.GetShaderAsset()->GetShaderOptionGroupLayout(), requestedVariantId,
                                            selectedVariantId);

                                        ImGui::Unindent();
                                    }
                                }

                                ImGui::TreePop();
                            }
                        }

                        ImGui::TreePop();
                    }
                }
                else
                {
                    ImGui::Text("No material selected");
                }
            }
            ImGui::End();
        }
    }
} // namespace AtomSampleViewer
