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
#include <Atom/RPI.Public/MeshDrawPacket.h>

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
    
    inline void ImGuiMaterialDetails::CloseDialog()
    {
        m_dialogIsOpen = false;
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
    
    inline void ImGuiMaterialDetails::Tick(const char* selectionName, const AZ::RPI::MeshDrawPacketLods* drawPackets)
    {
        if (m_dialogIsOpen)
        {
            if (ImGui::Begin("Material Shader Details", &m_dialogIsOpen, ImGuiWindowFlags_None))
            {
                if (selectionName && selectionName[0])
                {
                    ImGui::Text("Selection: %s", selectionName);
                }
                
                if (drawPackets)
                {
                    ImGui::BeginChild("DrawPackets", ImVec2(ImGui::GetWindowContentRegionWidth() * 0.25f, 0.0f), true);
                    {
                        for (size_t lod = 0; lod < drawPackets->size(); ++lod)
                        {
                            const AZ::RPI::MeshDrawPacketList& drawPacketsOneLod = (*drawPackets)[lod];

                            ImGuiTreeNodeFlags lodNodeFlags = ImGuiTreeNodeFlags_DefaultOpen;
                            if (ImGui::TreeNodeEx(AZStd::string::format("LOD %zu", lod).c_str(), lodNodeFlags))
                            {
                                for (size_t drawPacketIndex = 0; drawPacketIndex < drawPacketsOneLod.size(); ++drawPacketIndex)
                                {
                                    const AZ::RPI::MeshDrawPacket& drawPacket = drawPacketsOneLod[drawPacketIndex];

                                    AZStd::string drawPacketNodeId = AZStd::string::format("DrawPacket[%zu][%zu]", lod, drawPacketIndex);

                                    ImGuiTreeNodeFlags drawPacketNodeFlags = ImGuiTreeNodeFlags_Leaf;
                                    if (m_selectedLod == lod && m_selectedDrawPacket == drawPacketIndex)
                                    {
                                        drawPacketNodeFlags |= ImGuiTreeNodeFlags_Selected;
                                    }

                                    ImGui::TreeNodeEx(drawPacketNodeId.c_str(), drawPacketNodeFlags, "Mesh %zu \"%s\"",
                                        drawPacketIndex, drawPacket.GetMesh().m_materialSlotName.GetCStr());
                                    
                                    if (ImGui::IsItemClicked())
                                    {
                                        m_selectedLod = lod;
                                        m_selectedDrawPacket = drawPacketIndex;
                                    }

                                    ImGui::TreePop();
                                }
                                
                                ImGui::TreePop();
                            }
                        }
                    }
                    ImGui::EndChild();

                    const RPI::MeshDrawPacket* selectedDrawPacket = nullptr;
                    if (m_selectedLod < drawPackets->size() && m_selectedDrawPacket < (*drawPackets)[0].size())
                    {
                        selectedDrawPacket = &(*drawPackets)[m_selectedLod][m_selectedDrawPacket];
                    }
                    
                    ImGui::SameLine();

                    ImGui::BeginChild("Shaders", ImVec2(0.0f, 0.0f), true);
                    {
                        if (selectedDrawPacket)
                        {
                            ImGui::Text("Material: %s", selectedDrawPacket->GetMaterial()->GetAsset().GetHint().c_str());

                            for (size_t shaderIndex = 0; shaderIndex < selectedDrawPacket->GetActiveShaderList().size(); ++shaderIndex)
                            {
                                const AZ::RPI::MeshDrawPacket::ShaderData& shaderData = selectedDrawPacket->GetActiveShaderList()[shaderIndex];

                                const ImGuiTreeNodeFlags shaderNodeFlags = ImGuiTreeNodeFlags_DefaultOpen;

                                if (ImGui::TreeNodeEx(AZStd::string::format("Shader: %s - %s", shaderData.m_shaderTag.GetCStr(), shaderData.m_shader->GetAsset().GetHint().c_str()).c_str(), shaderNodeFlags))
                                {
                                    ImGui::Indent();

                                    ImGuiShaderUtils::DrawShaderDetails(shaderData);

                                    ImGui::Unindent();

                                    ImGui::TreePop();
                                }
                            }
                        }
                    }
                    ImGui::EndChild();
                }
                else
                {
                    ImGui::Text("No draw packets provided");
                }
            }
            ImGui::End();
        }
    }
} // namespace AtomSampleViewer
