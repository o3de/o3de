/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ImGuiLYAssetExplorer.h"

#ifdef IMGUI_ENABLED
#include <AzCore/Component/TransformBus.h>
#include <AzFramework/Entity/EntityContext.h>
#include <AzFramework/Entity/EntityContextBus.h>
#include <AzFramework/Entity/GameEntityContextBus.h>
#include <AzCore/std/sort.h>
#include <AzCore/std/string/conversions.h>
#include <IRenderAuxGeom.h>
#include <IConsole.h>

#include "ImGuiColorDefines.h"

namespace ImGui
{
    // Colors specific to AssetExplorer ( maybe use common ones in ImGuiColorDefines.h too? )
    static const ImVec4 s_lodColor_0 = ImColor(1.0f, 1.0f, 1.0f);
    static const ImVec4 s_lodColor_1 = ImColor(0.0f, 0.0f, 1.0f);
    static const ImVec4 s_lodColor_2 = ImColor(0.0f, 1.0f, 0.0f);
    static const ImVec4 s_lodColor_3 = ImColor(0.0f, 1.0f, 1.0f);
    static const ImVec4 s_lodColor_4 = ImColor(1.0f, 0.0f, 0.0f);
    static const ImVec4 s_lodColor_5 = ImColor(1.0f, 0.0f, 1.0f);

    ImGuiLYAssetExplorer::ImGuiLYAssetExplorer()
        : m_enabled(false)
        , m_meshDebugEnabled(false)
        , m_selectionFilter(false)
        , m_anyMousedOverForDraw(false)
        , m_enabledMouseOvers(true)
        , m_distanceFilter(true)
        , m_distanceFilter_near(40.0f)
        , m_distanceFilter_far(80.0f)
        , m_entityNameFilter(true)
        , m_meshNameFilter(true)
        , m_lodDebugEnabled(false)
        , m_inWorld_drawOriginSphere(true)
        , m_inWorld_originSphereRadius(0.1f)
        , m_inWorld_drawLabel(true)
        , m_inWorld_label_framed(true)
        , m_inWorld_label_monoSpace(false)
        , m_inWorld_label_textColor(1.0f, 0.65f, 0.0f, 1.0f)
        , m_inWorld_labelTextSize(1.5f)
        , m_inWorld_drawAABB(true)
        , m_inWorld_debugDrawMesh(false)
        , m_inWorld_label_entityName(true)
        , m_inWorld_label_materialName(false)
        , m_inWorld_label_totalLods(true)
        , m_inWorld_label_miscLod(false)
    {
    }

    ImGuiLYAssetExplorer::~ImGuiLYAssetExplorer()
    {
    }

    void ImGuiLYAssetExplorer::Initialize()
    {
        // Connect to EBUSes
        ImGuiAssetExplorerRequestBus::Handler::BusConnect();
    }

    void ImGuiLYAssetExplorer::Shutdown()
    {
        // Disconnect EBUSes
        ImGuiAssetExplorerRequestBus::Handler::BusDisconnect();
    }

    void ImGuiLYAssetExplorer::ImGuiUpdate()
    {
        // Manage main window visibility
        if (m_enabled)
        {
            if (ImGui::Begin("Asset Explorer", &m_enabled, ImGuiWindowFlags_MenuBar|ImGuiWindowFlags_HorizontalScrollbar|ImGuiWindowFlags_NoSavedSettings))
            {
                // Draw the Entire Main Menu Window Area
                ImGuiUpdate_DrawMenu();

                // Draw Menu Bar
                if (ImGui::BeginMenuBar())
                {
                    if (ImGui::BeginMenu("View Options##assetExplorer"))
                    {
                        ImGuiUpdate_DrawViewOptions();

                        ImGui::EndMenu();
                    }

                    ImGui::EndMenuBar();
                }
            }
            ImGui::End();
        }
    }

    void ImGuiLYAssetExplorer::ImGuiUpdate_DrawViewOptions()
    {
        // In-World Drawing Options ( Sphere, AABB, Debug Mesh, etc )
        ImGui::TextColored(ImGui::Colors::s_NiceLabelColor, "In-World Drawing");
        ImGui::Separator();
        ImGui::Checkbox("Draw Origin Sphere", &m_inWorld_drawOriginSphere);
        ImGui::DragFloat("Origin Sphere Radius", &m_inWorld_originSphereRadius, 0.01f, 0.0f, 100.0f);
        ImGui::Checkbox("Draw AABB", &m_inWorld_drawAABB);
        ImGui::Checkbox("Debug Draw Mesh", &m_inWorld_debugDrawMesh);

        // In-World Label Options
        ImGui::TextColored(ImGui::Colors::s_NiceLabelColor, "In-World Label Options");
        ImGui::Separator();
        ImGui::Checkbox("Draw Label", &m_inWorld_drawLabel);
        ImGui::Checkbox("Label - Entity Name", &m_inWorld_label_entityName);
        ImGui::Checkbox("Label - Monospace", &m_inWorld_label_monoSpace);
        ImGui::Checkbox("Label - Framed", &m_inWorld_label_framed);
        ImGui::Checkbox("Label - Material Name", &m_inWorld_label_materialName);
        ImGui::Checkbox("Label - Total LODs", &m_inWorld_label_totalLods);
        ImGui::Checkbox("Label - Misc LOD data", &m_inWorld_label_miscLod);
        ImGui::DragFloat("Label Text Size", &m_inWorld_labelTextSize, 0.01f, 0.0f, 100000.0f);
        ImGui::ColorEdit4("Label - Text Color", reinterpret_cast<float*>(&m_inWorld_label_textColor.Value));
    }

    void ImGuiLYAssetExplorer::MeshInstanceList_CheckMeshFilter()
    {
        // Iterate through the Mesh Instance list, mark a boolean flag if the mesh name passes the Mesh Name Filter.
        for (MeshInstanceDisplayList& meshInstanceList : m_meshInstanceDisplayList)
        {
            meshInstanceList.m_passesFilter = (meshInstanceList.m_meshPath.find(m_meshNameFilterStr) != AZStd::string::npos);
        }
    }

    void ImGuiLYAssetExplorer::MeshInstanceList_CheckEntityFilter()
    {
        // Iterate through All Meshes..
        for (MeshInstanceDisplayList& meshInstanceList : m_meshInstanceDisplayList)
        {
            // .. reset this flag to see if any child instances pass the name filter
            meshInstanceList.m_childrenPassFilter = false;

            // .. Iterate through all Instance of this mesh, mark any that pass the Name Filter
            for (auto& meshInstance : meshInstanceList.m_instanceOptionMap)
            {
                meshInstance.second.m_passesFilter = (meshInstance.second.m_instanceLabel.find(m_entityNameFilterStr) != AZStd::string::npos);

                // Or in this child's result to help mark if a single Instance of this Mesh passed the filter.
                meshInstanceList.m_childrenPassFilter |= meshInstance.second.m_passesFilter;
            }
        }
    }

    void ImGuiLYAssetExplorer::ImGuiUpdate_DrawMenu()
    {
        // Primary on / off Switch
        ImGui::Checkbox("Mesh Debug Enabled", &m_meshDebugEnabled);
        ImGui::SameLine();

        // Lod Debug Switch, check for changes so we can do things once at change time
        bool lodDebug = m_lodDebugEnabled;
        ImGui::Checkbox("LOD Debug", &lodDebug);
        if (lodDebug != m_lodDebugEnabled)
        {
            // Save off the new debug flag value
            m_lodDebugEnabled = lodDebug;

            // Find the CVAR and flick the value
            static ICVar* eTexelDensityCVAR = gEnv->pConsole->GetCVar("e_texeldensity");
            if (eTexelDensityCVAR)
            {
                eTexelDensityCVAR->Set(m_lodDebugEnabled ? 2 : 0);
            }
        }

        // If the Lod Debug is Enabled. Draw a small legend that
        if (m_lodDebugEnabled)
        {
            ImGui::BeginChild("lodDebugLegend", ImVec2(0.0f, 57.0f), true);

            // Text for legend.
            ImGui::TextColored(ImGui::Colors::s_NiceLabelColor, "Lod Color Legend:");
            ImGui::SameLine();
            ImGui::TextColored(s_lodColor_0, "0 ");
            ImGui::SameLine();
            ImGui::TextColored(s_lodColor_1, "1 ");
            ImGui::SameLine();
            ImGui::TextColored(s_lodColor_2, "2 ");
            ImGui::SameLine();
            ImGui::TextColored(s_lodColor_3, "3 ");
            ImGui::SameLine();
            ImGui::TextColored(s_lodColor_4, "4 ");
            ImGui::SameLine();
            ImGui::TextColored(s_lodColor_5, "5 ");

            // Small boxes of each color to help with the legend
            static float s_boxSize = 21.0f;
            ImVec2 graphUpLeft(ImGui::GetWindowPos().x + 127.5f, ImGui::GetWindowPos().y + 26.0f);
            ImGui::GetWindowDrawList()->AddRectFilled(
                ImVec2(graphUpLeft.x + (0 * s_boxSize), graphUpLeft.y),
                ImVec2(graphUpLeft.x + (1 * s_boxSize), graphUpLeft.y + s_boxSize),
                ImGui::ColorConvertFloat4ToU32(s_lodColor_0), 2.0f);

            ImGui::GetWindowDrawList()->AddRectFilled(
                ImVec2(graphUpLeft.x + (1 * s_boxSize), graphUpLeft.y),
                ImVec2(graphUpLeft.x + (2 * s_boxSize), graphUpLeft.y + s_boxSize),
                ImGui::ColorConvertFloat4ToU32(s_lodColor_1), 2.0f);

            ImGui::GetWindowDrawList()->AddRectFilled(
                ImVec2(graphUpLeft.x + (2 * s_boxSize), graphUpLeft.y),
                ImVec2(graphUpLeft.x + (3 * s_boxSize), graphUpLeft.y + s_boxSize),
                ImGui::ColorConvertFloat4ToU32(s_lodColor_2), 2.0f);

            ImGui::GetWindowDrawList()->AddRectFilled(
                ImVec2(graphUpLeft.x + (3 * s_boxSize), graphUpLeft.y),
                ImVec2(graphUpLeft.x + (4 * s_boxSize), graphUpLeft.y + s_boxSize),
                ImGui::ColorConvertFloat4ToU32(s_lodColor_3), 2.0f);

            ImGui::GetWindowDrawList()->AddRectFilled(
                ImVec2(graphUpLeft.x + (4 * s_boxSize), graphUpLeft.y),
                ImVec2(graphUpLeft.x + (5 * s_boxSize), graphUpLeft.y + s_boxSize),
                ImGui::ColorConvertFloat4ToU32(s_lodColor_4), 2.0f);

            ImGui::GetWindowDrawList()->AddRectFilled(
                ImVec2(graphUpLeft.x + (5 * s_boxSize), graphUpLeft.y),
                ImVec2(graphUpLeft.x + (6 * s_boxSize), graphUpLeft.y + s_boxSize),
                ImGui::ColorConvertFloat4ToU32(s_lodColor_5), 2.0f);

            ImGui::EndChild();
        }

        // Filter Options
        if (ImGui::CollapsingHeader("Filters", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed))
        {
            ImGui::Columns(3);
            // Draw Column Headers
            ImGui::TextColored(ImGui::Colors::s_NiceLabelColor, "Distance Filter");
            ImGui::NextColumn();
            ImGui::TextColored(ImGui::Colors::s_NiceLabelColor, "Mesh Name Filter");
            ImGui::NextColumn();
            ImGui::TextColored(ImGui::Colors::s_NiceLabelColor, "Entity Name Filter");
            ImGui::NextColumn();
            ImGui::Separator();
            {
                // Distance Filter
                ImGui::Checkbox("Enabled##distfilter", &m_distanceFilter);
                ImGui::TextColored(ImGui::Colors::s_PlainLabelColor, "Near Distance:");
                ImGui::SameLine();
                ImGui::DragFloat("##Distance Filter Near", &m_distanceFilter_near, 0.1f, 0.0f, 100000.0f);
                ImGui::TextColored(ImGui::Colors::s_PlainLabelColor, "Far Distance:");
                ImGui::SameLine();
                ImGui::DragFloat("##Distance Filter Far", &m_distanceFilter_far, 0.1f, 0.0f, 100000.0f);

                // We don't really want a far distance that is less than our near distance, so lets check for that and correct here
                if (m_distanceFilter_far < m_distanceFilter_near)
                {
                    m_distanceFilter_far = m_distanceFilter_near;
                }
            }
            ImGui::NextColumn();
            {
                // Mesh Name Filter
                ImGui::Checkbox("Enabled##meshNameFilter", &m_meshNameFilter);

                static char meshNameCharArray[128] = "";
                ImGui::InputText("##meshNameFiltertext", meshNameCharArray, sizeof(meshNameCharArray));

                // Save off the string and to_lower it
                AZStd::string meshNameStr = meshNameCharArray;
                AZStd::to_lower(meshNameStr.begin(), meshNameStr.end());

                if (meshNameStr != m_meshNameFilterStr)
                {
                    // Mesh Name String Change Detected! Check meshes for filtration
                    m_meshNameFilterStr = meshNameStr;
                    MeshInstanceList_CheckMeshFilter();
                }
            }
            ImGui::NextColumn();
            {
                // Entity Name Filter
                ImGui::Checkbox("Enabled##entityNameFilter", &m_entityNameFilter);

                static char entityNameCharArray[128] = "";
                ImGui::InputText("##entityNameFiltertext", entityNameCharArray, sizeof(entityNameCharArray));

                // Save off the string and to_lower it
                AZStd::string entityNameStr = entityNameCharArray;
                AZStd::to_lower(entityNameStr.begin(), entityNameStr.end());

                if (entityNameStr != m_entityNameFilterStr)
                {
                    // Mesh Name String Change Detected! Check meshes for filtration
                    m_entityNameFilterStr = entityNameStr;
                    MeshInstanceList_CheckEntityFilter();
                }
            }

            ImGui::Columns(1);
        }

        // Draw the Mesh Hierarchy
        if (ImGui::CollapsingHeader("Meshes In Scene", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed))
        {
            if (!m_meshInstanceDisplayList.empty())
            {
                // Buttons to sort by Mesh Name and Instance Count
                ImGui::BeginChild("MeshesTitleBarChild", ImVec2(0.0f, 56.0f), true);
                ImGui::Columns(3);
                ImGui::TextColored(ImGui::Colors::s_NiceLabelColor, "Sort By:");
                if (ImGui::Button("Mesh Name"))
                {
                    // A Static int flag to swap to alternate between sorting ascending/descending.
                    static bool s_meshNameSortUp = false;
                    s_meshNameSortUp = !s_meshNameSortUp;
                    if (s_meshNameSortUp)
                    {
                        m_meshInstanceDisplayList.sort([](const MeshInstanceDisplayList& meshList1, const MeshInstanceDisplayList& meshList2)
                        {
                            return meshList1.m_meshPath < meshList2.m_meshPath;
                        });
                    }
                    else
                    {
                        m_meshInstanceDisplayList.sort([](const MeshInstanceDisplayList& meshList1, const MeshInstanceDisplayList& meshList2)
                        {
                            return meshList1.m_meshPath > meshList2.m_meshPath;
                        });
                    }
                }
                ImGui::SameLine();
                if (ImGui::Button("Instance Count"))
                {
                    // A Static int flag to swap to alternate between sorting ascending/descending.
                    static bool s_meshCountSortUp = false;
                    s_meshCountSortUp = !s_meshCountSortUp;
                    if (s_meshCountSortUp)
                    {
                        m_meshInstanceDisplayList.sort([](const MeshInstanceDisplayList& meshList1, const MeshInstanceDisplayList& meshList2)
                        {
                            return meshList1.m_instanceOptionMap.size() < meshList2.m_instanceOptionMap.size();
                        });
                    }
                    else
                    {
                        m_meshInstanceDisplayList.sort([](const MeshInstanceDisplayList& meshList1, const MeshInstanceDisplayList& meshList2)
                        {
                            return meshList1.m_instanceOptionMap.size() > meshList2.m_instanceOptionMap.size();
                        });
                    }
                }
                ImGui::NextColumn();
                // A Small Legend and Hints section for help using this thing
                ImGui::BeginChild("MouseHoverLegendChild", ImVec2(250.0f, 30.0f), true);
                if (ImGui::IsWindowHovered())
                {
                    ImGui::BeginTooltip();
                    ImGui::TextColored(ImGui::Colors::s_NiceLabelColor, "Legend:");
                    if (ImGui::TreeNodeEx("Mesh (Count) - Mesh Path", ImGuiTreeNodeFlags_DefaultOpen))
                    {
                        if (ImGui::SmallButton("-*View Instance Btn*-"))
                        {
                            // Don't need to do anything here. It is just a sample button!
                        }
                        ImGui::SameLine();
                        ImGui::TextColored(ImGui::Colors::s_NiceLabelColor, "[*EntityId*] *EntityName*");
                        ImGui::SameLine();
                        ImGui::TextColored(ImGui::Colors::s_PlainLabelColor, "(*World Position XYZ*)");
                        ImGui::TreePop(); // End Tree
                    }
                    ImGui::Separator();
                    ImGui::TextColored(ImGui::Colors::s_NiceLabelColor, "Tips:");
                    ImGui::Indent();
                    ImGui::TextColored(ImGui::Colors::s_NiceLabelColor, " * Click to the *View Instance Btn* to Snap the camera to any mesh instance local origin");
                    ImGui::TextColored(ImGui::Colors::s_NiceLabelColor, " * Mouse Over any Meshes Groups or Individual Entities to Temporarily only draw those.");
                    ImGui::TextColored(ImGui::Colors::s_NiceLabelColor, " * Use the Selection Filter to only display Meshes and Entities that are manually selected.");
                    ImGui::TextColored(ImGui::Colors::s_NiceLabelColor, " * Selection Filter overrides other filters.");
                    ImGui::TextColored(ImGui::Colors::s_NiceLabelColor, " * Mesh Selection overrides Entity Selection.");
                    ImGui::EndTooltip();
                }

                ImGui::TextColored(ImGui::IsWindowHovered() ? ImGui::Colors::s_NiceLabelColor : ImGui::Colors::s_PlainLabelColor, "Mouse Over For Legend and Tips");
                ImGui::EndChild(); // MouseHover Child
                ImGui::NextColumn();

                // Small area for Mouse Over and Selection Filter options
                ImGui::TextColored(ImGui::Colors::s_NiceLabelColor, "Mouse Overs:    ");
                ImGui::SameLine();
                ImGui::Checkbox("##EnableMouseOversCheckbox", &m_enabledMouseOvers);

                ImGui::TextColored(ImGui::Colors::s_NiceLabelColor, "Filter Selected:");
                ImGui::SameLine();
                ImGui::Checkbox("##FilterSelectedCheckbox", &m_selectionFilter);

                ImGui::SetColumnOffset(2, ImGui::GetWindowWidth() - 168.0f);
                ImGui::Columns(1);
                ImGui::EndChild(); // Sort/Legend Child

                // The Core Mesh Hierarchy
                ImGui::BeginChild("MeshesInSceneContainer", ImVec2(0, 400.0f), true);

                // Before we draw all these meshes, lets mark this frame as no Mouse Over being drawn.. if any are drawn, they will set this flag
                m_anyMousedOverForDraw = false;

                if (m_selectionFilter)
                {
                    ImGui::Columns(2);
                }
                for (MeshInstanceDisplayList& meshInstanceList : m_meshInstanceDisplayList)
                {
                    // See if we should display the mesh: Check for various filters and if they are enabled, & in their status.
                    bool displayMesh = true;
                    if (m_meshNameFilter)
                    {
                        displayMesh &= meshInstanceList.m_passesFilter;
                    }
                    if (m_entityNameFilter)
                    {
                        displayMesh &= meshInstanceList.m_childrenPassFilter;
                    }

                    // Set this flag to false, flip it if any children end up doing Mouse Overs
                    meshInstanceList.m_childMousedOverForDraw = false;
                    meshInstanceList.m_mousedOverForDraw = false;

                    // If we should draw the mesh, draw the tree node!
                    if (displayMesh)
                    {
                        if (ImGui::TreeNode(AZStd::string::format("Mesh (%03zu) - %s", meshInstanceList.m_instanceOptionMap.size(), meshInstanceList.m_meshPath.c_str()).c_str()))
                        {
                            ImGuiUpdate_DrawMeshMouseOver(meshInstanceList);

                            if (m_selectionFilter)
                            {
                                ImGui::NextColumn();
                                ImGui::Checkbox(AZStd::string::format("##meshCheckBox%s", meshInstanceList.m_meshPath.c_str()).c_str(), &meshInstanceList.m_selectedForDraw);
                                ImGuiUpdate_DrawMeshMouseOver(meshInstanceList);
                                ImGui::NextColumn();
                            }
                            // Keep Count of our Mesh instances and loop through them drawing them!
                            int instanceCount = 0;
                            for (auto& meshInstance : meshInstanceList.m_instanceOptionMap)
                            {
                                // See if we should
                                bool displayEntity = true;
                                if (m_entityNameFilter)
                                {
                                    displayEntity = meshInstance.second.m_passesFilter;
                                }

                                if (displayEntity)
                                {
                                    // Get the Name and World Position of this Entity Instance
                                    AZStd::string entityName;
                                    AZ::ComponentApplicationBus::BroadcastResult(entityName, &AZ::ComponentApplicationBus::Events::GetEntityName, meshInstance.first);

                                    AZ::Vector3 worldPos = AZ::Vector3::CreateOne();
                                    AZ::TransformBus::EventResult(worldPos, meshInstance.first, &AZ::TransformBus::Events::GetWorldTranslation);

                                    ImGui::BeginGroup();
                                    if (ImGui::SmallButton(AZStd::string::format("-View #%03d-##%s", ++instanceCount, meshInstance.first.ToString().c_str()).c_str()))
                                    {
                                        ImGuiEntityOutlinerNotificationBus::Broadcast(&IImGuiEntityOutlinerNotifications::OnImGuiEntityOutlinerTarget, meshInstance.first);
                                    }
                                    // Build the Label String.
                                    ImGui::SameLine();
                                    ImGui::TextColored(ImGui::Colors::s_NiceLabelColor, "%s %s", meshInstance.first.ToString().c_str(), entityName.c_str());
                                    ImGui::SameLine();
                                    ImGui::TextColored(ImGui::Colors::s_PlainLabelColor, "(% .02f, % .02f, % .02f)", (float)worldPos.GetX(), (float)worldPos.GetY(), (float)worldPos.GetZ());
                                    ImGui::EndGroup();

                                    // Check for and Draw Entity Instance Mouse Over
                                    meshInstance.second.m_mousedOverForDraw = false;
                                    ImGuiUpdate_DrawEntityInstanceMouseOver(meshInstanceList, meshInstance.first, entityName, meshInstance.second);


                                    if (m_selectionFilter)
                                    {
                                        ImGui::NextColumn();
                                        ImGui::Checkbox(AZStd::string::format("##entityCheckBox%s", meshInstance.first.ToString().c_str()).c_str(), &meshInstance.second.m_selectedForDraw);
                                        ImGuiUpdate_DrawEntityInstanceMouseOver(meshInstanceList, meshInstance.first, entityName, meshInstance.second);
                                        ImGui::NextColumn();
                                    }
                                }
                            }

                            ImGui::TreePop(); // End Mesh Tree
                        }
                        else
                        {
                            ImGuiUpdate_DrawMeshMouseOver(meshInstanceList);

                            if (m_selectionFilter)
                            {
                                ImGui::NextColumn();
                                ImGui::Checkbox(AZStd::string::format("##meshCheckBox%s", meshInstanceList.m_meshPath.c_str()).c_str(), &meshInstanceList.m_selectedForDraw);
                                ImGuiUpdate_DrawMeshMouseOver(meshInstanceList);
                                ImGui::NextColumn();
                            }
                        }
                    }
                }
                if (m_selectionFilter)
                {
                    ImGui::SetColumnOffset(1, ImGui::GetWindowWidth() - 60.0f);
                    ImGui::Columns(1);
                }
                ImGui::EndChild(); // End the "Meshes in Scene" Child
            }
        }
    }

    // Mesh Mouse Over Helper function
    void ImGuiLYAssetExplorer::ImGuiUpdate_DrawMeshMouseOver(MeshInstanceDisplayList& meshDisplayList)
    {
        if (!m_enabledMouseOvers)
        {
            return;
        }

        if (ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            ImGui::TextColored(ImGui::Colors::s_NiceLabelColor, "Mesh: ");
            ImGui::SameLine();
            ImGui::TextColored(ImGui::Colors::s_PlainLabelColor, "%s", meshDisplayList.m_meshPath.c_str());

            ImGui::TextColored(ImGui::Colors::s_NiceLabelColor, "Instance Count: ");
            ImGui::SameLine();
            ImGui::TextColored(ImGui::Colors::s_PlainLabelColor, "%d", (int) meshDisplayList.m_instanceOptionMap.size());

            // Mark that any Mouse Over has happened ( changes draw mode )
            m_anyMousedOverForDraw = true;

            ImGui::EndTooltip();
        }
        meshDisplayList.m_mousedOverForDraw |= ImGui::IsItemHovered();
    }

    // Entity Instance Helper Function
    void ImGuiLYAssetExplorer::ImGuiUpdate_DrawEntityInstanceMouseOver(MeshInstanceDisplayList& meshDisplayList, AZ::EntityId& entityInstance, AZStd::string& entityName, MeshInstanceOptions& instanceOptions)
    {
        if (!m_enabledMouseOvers)
        {
            return;
        }

        if (ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();

            AZ::Vector3 worldPos = AZ::Vector3::CreateZero();
            AZ::TransformBus::EventResult(worldPos, entityInstance, &AZ::TransformBus::Events::GetWorldTranslation);

            ImGui::TextColored(ImGui::Colors::s_NiceLabelColor, "Entity: ");
            ImGui::SameLine();
            ImGui::TextColored(ImGui::Colors::s_PlainLabelColor, "%s %s", entityInstance.ToString().c_str(), entityName.c_str());

            ImGui::TextColored(ImGui::Colors::s_NiceLabelColor, "Mesh: ");
            ImGui::SameLine();
            ImGui::TextColored(ImGui::Colors::s_PlainLabelColor, "%s", meshDisplayList.m_meshPath.c_str());

            ImGui::TextColored(ImGui::Colors::s_NiceLabelColor, "World Position: ");
            ImGui::SameLine();
            ImGui::TextColored(ImGui::Colors::s_PlainLabelColor, "% .02f , % .02f , % .02f", (float)worldPos.GetX(), (float)worldPos.GetY(), (float)worldPos.GetZ());

            // Mark that any Mouse Over has happened ( changes draw mode )
            m_anyMousedOverForDraw = true;

            // Note that this mesh has a child with a mouse over active
            meshDisplayList.m_childMousedOverForDraw = true;

            ImGui::EndTooltip();
        }
        instanceOptions.m_mousedOverForDraw |= ImGui::IsItemHovered();
    }

    MeshInstanceDisplayList& ImGuiLYAssetExplorer::FindOrCreateMeshInstanceList(const char* meshName)
    {
        // Walk the list and see if an entry for this mesh exists already. If we find one, return it!
        for (MeshInstanceDisplayList& meshInstanceList : m_meshInstanceDisplayList)
        {
            if (meshInstanceList.m_meshPath == meshName)
            {
                return meshInstanceList;
            }
        }

        // Not found, so create a new entry..
        MeshInstanceDisplayList meshList;
        meshList.m_meshPath = meshName;
        meshList.m_passesFilter = true;
        meshList.m_childrenPassFilter = true;
        meshList.m_selectedForDraw = false;
        meshList.m_mousedOverForDraw = false;
        meshList.m_childMousedOverForDraw = false;

        // push it to the end of the list, and then return that last entry
        m_meshInstanceDisplayList.push_back(meshList);
        return m_meshInstanceDisplayList.back();
    }
 } // namespace ImGui

#endif // IMGUI_ENABLED
