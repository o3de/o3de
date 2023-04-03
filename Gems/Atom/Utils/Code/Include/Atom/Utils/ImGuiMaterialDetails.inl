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
    inline void ImGuiMaterialDetails::SetSelectedDrawPacket(const RPI::MeshDrawPacket* drawPacket)
    {
        m_selectedDrawPacket = drawPacket;

        if (!m_selectedDrawPacket)
        {
            // If the draw packet selection was explicitly set to null, we don't want to continue showing
            // some other draw packet because that could confuse the user.
            m_selectedLod = AZStd::numeric_limits<decltype(m_selectedLod)>::max();
            m_selectedDrawPacketIndex = AZStd::numeric_limits<decltype(m_selectedDrawPacketIndex)>::max();
        }
    }

    inline void ImGuiMaterialDetails::OpenDialog()
    {
        m_dialogIsOpen = true;
    }
    
    inline void ImGuiMaterialDetails::CloseDialog()
    {
        m_dialogIsOpen = false;
    }

    inline bool ImGuiMaterialDetails::Tick(const AZ::RPI::MeshDrawPacketLods* drawPackets, const char* selectionName)
    {
        if (m_dialogIsOpen)
        {
            // Make sure the window doesn't have a 0 size the first time it's opened.
            ImGui::SetNextWindowSizeConstraints(ImVec2(200, 100), ImVec2(10'000, 10'000));

            if (ImGui::Begin("Material Shader Details", &m_dialogIsOpen, ImGuiWindowFlags_None))
            {
                if (selectionName && selectionName[0])
                {
                    ImGui::Text("Selection: %s", selectionName);
                }
                
                if (drawPackets)
                {
                    // First determine which draw packet is currently selected, if any
                    const RPI::MeshDrawPacket* currentSelectedDrawPacket = nullptr;
                    {
                        const RPI::MeshDrawPacket* explicitSelectedDrawPacket = nullptr;
                        const RPI::MeshDrawPacket* indexSelectedDrawPacket = nullptr;

                        // The first priority for draw packet selection is to use the explicitly set m_selectedDrawPacket
                        for (size_t lod = 0; lod < drawPackets->size(); ++lod)
                        {
                            const AZ::RPI::MeshDrawPacketList& drawPacketsOneLod = (*drawPackets)[lod];
                            for (size_t drawPacketIndex = 0; drawPacketIndex < drawPacketsOneLod.size(); ++drawPacketIndex)
                            {
                                const AZ::RPI::MeshDrawPacket& drawPacket = drawPacketsOneLod[drawPacketIndex];
                                if (m_selectedDrawPacket == &drawPacket)
                                {
                                    explicitSelectedDrawPacket = &drawPacket;
                                }

                                if (m_selectedLod == lod && m_selectedDrawPacketIndex == drawPacketIndex)
                                {
                                    indexSelectedDrawPacket = &drawPacket;
                                }
                            }
                        }

                        currentSelectedDrawPacket = explicitSelectedDrawPacket ? explicitSelectedDrawPacket : indexSelectedDrawPacket;
                    }

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
                                    if (currentSelectedDrawPacket == &drawPacket)
                                    {
                                        drawPacketNodeFlags |= ImGuiTreeNodeFlags_Selected;
                                    }

                                    ImGui::TreeNodeEx(drawPacketNodeId.c_str(), drawPacketNodeFlags, "Mesh %zu \"%s\"",
                                        drawPacketIndex, drawPacket.GetMesh().m_materialSlotName.GetCStr());
                                    
                                    if (ImGui::IsItemClicked())
                                    {
                                        m_selectedLod = lod;
                                        m_selectedDrawPacketIndex = drawPacketIndex;
                                        m_selectedDrawPacket = &drawPacket;
                                        currentSelectedDrawPacket = &drawPacket;
                                    }

                                    ImGui::TreePop();
                                }
                                
                                ImGui::TreePop();
                            }
                        }
                    }
                    ImGui::EndChild();

                    ImGui::SameLine();

                    ImGui::BeginChild("Shaders", ImVec2(0.0f, 0.0f), true);
                    {
                        if (currentSelectedDrawPacket)
                        {
                            ImGui::Text("Material: %s", currentSelectedDrawPacket->GetMaterial()->GetAsset().GetHint().c_str());

                            for (size_t shaderIndex = 0; shaderIndex < currentSelectedDrawPacket->GetActiveShaderList().size(); ++shaderIndex)
                            {
                                const AZ::RPI::MeshDrawPacket::ShaderData& shaderData = currentSelectedDrawPacket->GetActiveShaderList()[shaderIndex];

                                const ImGuiTreeNodeFlags shaderNodeFlags = ImGuiTreeNodeFlags_DefaultOpen;

                                if (ImGui::TreeNodeEx(AZStd::string::format("Shader: %s - %s - %s", shaderData.m_materialPipelineName.GetCStr(), shaderData.m_shaderTag.GetCStr(), shaderData.m_shader->GetAsset().GetHint().c_str()).c_str(), shaderNodeFlags))
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

        return m_dialogIsOpen;
    }
} // namespace AtomSampleViewer
