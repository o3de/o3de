/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ImGuiLYEntityOutliner.h"

#ifdef IMGUI_ENABLED

#include <AzCore/Component/TransformBus.h>
#include <AzCore/Asset/AssetSerializer.h>
#include <AzCore/std/sort.h>
#include <AzCore/std/string/conversions.h>
#include <AzFramework/Entity/EntityContext.h>
#include <AzFramework/Entity/EntityContextBus.h>
#include <AzFramework/Entity/GameEntityContextBus.h>

#include "ImGuiColorDefines.h"

namespace ImGui
{
    // Text and Color Consts 
    static const char* s_OnText =           "On:";
    static const char* s_ColorText =        "Color:";

    static const ImVec4 s_DisplayNameDefaultColor =             ImColor(1.0f, 0.0f, 1.0f, 0.9f);
    static const ImVec4 s_DisplayChildCountDefaultColor =       ImColor(0.32f, 0.38f, 0.16f);
    static const ImVec4 s_DisplayDescendantCountDefaultColor =  ImColor(0.32f, 0.64f, 0.38f);
    static const ImVec4 s_DisplayEntityStateDefaultColor =      ImColor(0.73f, 0.97f, 0.6f);
    static const ImVec4 s_DisplayParentInfoDefaultColor =       ImColor(0.32f, 0.55f, 1.0f);
    static const ImVec4 s_DisplayLocalPosDefaultColor =         ImColor(0.0f, 0.8f, 0.12f);
    static const ImVec4 s_DisplayLocalRotationDefaultColor =    ImColor(0.0f, 0.8f, 0.12f, 0.55f);
    static const ImVec4 s_DisplayWorldPosDefaultColor =         ImColor(1.0f, 0.8f, 0.12f);
    static const ImVec4 s_DisplayWorldRotationDefaultColor =    ImColor(1.0f, 0.8f, 0.12f, 0.55f);
    static const ImVec4 s_ComponentParamColor_Type =            ImColor(1.0f, 0.0f, 1.0f, 0.9f);
    static const ImVec4 s_ComponentParamColor_Name =            ImColor(1.0f, 0.8f, 0.12f);
    static const ImVec4 s_ComponentParamColor_Value =           ImColor(0.32f, 1.0f, 1.0f);


    ImGuiLYEntityOutliner::ImGuiLYEntityOutliner()
        : m_enabled(false)
        , m_displayName(true, s_DisplayNameDefaultColor)
        , m_displayChildCount(false, s_DisplayChildCountDefaultColor)
        , m_displayDescentdantCount(true, s_DisplayDescendantCountDefaultColor)
        , m_displayEntityState(false, s_DisplayEntityStateDefaultColor)
        , m_displayParentInfo(false, s_DisplayParentInfoDefaultColor)
        , m_displayLocalPos(false, s_DisplayLocalPosDefaultColor)
        , m_displayLocalRotation(false, s_DisplayLocalRotationDefaultColor)
        , m_displayWorldPos(true, s_DisplayWorldPosDefaultColor)
        , m_displayWorldRotation(true, s_DisplayWorldRotationDefaultColor)
        , m_hierarchyUpdateType(HierarchyUpdateType::Constant)
        , m_hierarchyUpdateTickTimeCurrent(0.0f)
        , m_hierarchyUpdateTickTimeTotal(1.0f)
        , m_rootEntityInfo(nullptr)
        , m_totalEntitiesFound(0)
        , m_drawTargetViewButton(false)
    {
    }

    ImGuiLYEntityOutliner::~ImGuiLYEntityOutliner()
    {
    }

    void ImGuiLYEntityOutliner::Initialize()
    {
        // Connect to Ebusses
        ImGuiEntityOutlinerRequestBus::Handler::BusConnect();
    }

    void ImGuiLYEntityOutliner::Shutdown()
    {
        // Disconnect Ebusses
        ImGuiEntityOutlinerRequestBus::Handler::BusDisconnect();
    }

    void ImGuiLYEntityOutliner::ImGuiUpdate_DrawViewOptions()
    {
        // Create a child to help better size the menu
        ImGui::BeginChild("EntityOutliner_ViewOptionsMenuChild", ImVec2(580.0f, 260.0f));

        // Options for view entity entries
        ImGui::Columns(3);
        ImGui::TextColored(m_displayName.m_color, "Display Name"); 
        ImGui::NextColumn();
        ImGui::TextColored(ImGui::Colors::s_PlainLabelColor, "%s", s_OnText); 
        ImGui::SameLine();
        ImGui::Checkbox("##DisplayNameCB", &m_displayName.m_enabled); 
        ImGui::NextColumn();
        ImGui::TextColored(ImGui::Colors::s_PlainLabelColor, "%s", s_ColorText);
        ImGui::SameLine();
        ImGui::ColorEdit4("##DisplayNameCol", reinterpret_cast<float*>(&m_displayName.m_color));
        ImGui::NextColumn();

        ImGui::TextColored(m_displayChildCount.m_color, "Display Child Count"); 
        ImGui::NextColumn();
        ImGui::TextColored(ImGui::Colors::s_PlainLabelColor, "%s", s_OnText);
        ImGui::SameLine();
        ImGui::Checkbox("##DisplayChildCountCB", &m_displayChildCount.m_enabled);
        ImGui::NextColumn();
        ImGui::TextColored(ImGui::Colors::s_PlainLabelColor, "%s", s_ColorText);
        ImGui::SameLine();
        ImGui::ColorEdit4("##DisplayChildCountCol", reinterpret_cast<float*>(&m_displayChildCount.m_color));
        ImGui::NextColumn();

        ImGui::TextColored(m_displayDescentdantCount.m_color, "Display Descendant Count");
        ImGui::NextColumn();
        ImGui::TextColored(ImGui::Colors::s_PlainLabelColor, "%s", s_OnText);
        ImGui::SameLine();
        ImGui::Checkbox("##DisplayDescendantCountCB", &m_displayDescentdantCount.m_enabled);
        ImGui::NextColumn();
        ImGui::TextColored(ImGui::Colors::s_PlainLabelColor, "%s", s_ColorText);
        ImGui::SameLine();
        ImGui::ColorEdit4("##DisplayDescendantCountCol", reinterpret_cast<float*>(&m_displayDescentdantCount.m_color));
        ImGui::NextColumn();

        ImGui::TextColored(m_displayEntityState.m_color, "Display Entity Status");
        ImGui::NextColumn();
        ImGui::TextColored(ImGui::Colors::s_PlainLabelColor, "%s", s_OnText);
        ImGui::SameLine();
        ImGui::Checkbox("##DisplayEntityStateCB", &m_displayEntityState.m_enabled);
        ImGui::NextColumn();
        ImGui::TextColored(ImGui::Colors::s_PlainLabelColor, "%s", s_ColorText);
        ImGui::SameLine();
        ImGui::ColorEdit4("##DisplayEntityStateCol", reinterpret_cast<float*>(&m_displayEntityState.m_color));
        ImGui::NextColumn();

        ImGui::TextColored(m_displayParentInfo.m_color, "Display Parent Info");
        ImGui::NextColumn();
        ImGui::TextColored(ImGui::Colors::s_PlainLabelColor, "%s", s_OnText);
        ImGui::SameLine();
        ImGui::Checkbox("##DisplayParentInfoCB", &m_displayParentInfo.m_enabled);
        ImGui::NextColumn();
        ImGui::TextColored(ImGui::Colors::s_PlainLabelColor, "%s", s_ColorText);
        ImGui::SameLine(); 
        ImGui::ColorEdit4("##DisplayParentInfoCol", reinterpret_cast<float*>(&m_displayParentInfo.m_color));
        ImGui::NextColumn();

        ImGui::TextColored(m_displayLocalPos.m_color, "Display Local Position"); 
        ImGui::NextColumn();
        ImGui::TextColored(ImGui::Colors::s_PlainLabelColor, "%s", s_OnText);
        ImGui::SameLine();
        ImGui::Checkbox("##DisplayLocalPosCB", &m_displayLocalPos.m_enabled);
        ImGui::NextColumn();
        ImGui::TextColored(ImGui::Colors::s_PlainLabelColor, "%s", s_ColorText);
        ImGui::SameLine();
        ImGui::ColorEdit4("##DisplayLocalPosCol", reinterpret_cast<float*>(&m_displayLocalPos.m_color));
        ImGui::NextColumn();

        ImGui::TextColored(m_displayLocalRotation.m_color, "Display Local Rotation");
        ImGui::NextColumn();
        ImGui::TextColored(ImGui::Colors::s_PlainLabelColor, "%s", s_OnText);
        ImGui::SameLine();
        ImGui::Checkbox("##DisplayLocalRotationCB", &m_displayLocalRotation.m_enabled);
        ImGui::NextColumn();
        ImGui::TextColored(ImGui::Colors::s_PlainLabelColor, "%s", s_ColorText);
        ImGui::SameLine();
        ImGui::ColorEdit4("##DisplayLocalRotationCol", reinterpret_cast<float*>(&m_displayLocalRotation.m_color));
        ImGui::NextColumn();

        ImGui::TextColored(m_displayWorldPos.m_color, "Display World Position"); 
        ImGui::NextColumn();
        ImGui::TextColored(ImGui::Colors::s_PlainLabelColor, "%s", s_OnText);
        ImGui::SameLine();
        ImGui::Checkbox("##DisplayWorldPosCB", &m_displayWorldPos.m_enabled);
        ImGui::NextColumn();
        ImGui::TextColored(ImGui::Colors::s_PlainLabelColor, "%s", s_ColorText);
        ImGui::SameLine();
        ImGui::ColorEdit4("##DisplayWorldPosCol", reinterpret_cast<float*>(&m_displayWorldPos.m_color));
        ImGui::NextColumn();

        ImGui::TextColored(m_displayWorldRotation.m_color, "Display World Rotation"); 
        ImGui::NextColumn();
        ImGui::TextColored(ImGui::Colors::s_PlainLabelColor, "%s", s_OnText);
        ImGui::SameLine();
        ImGui::Checkbox("##DisplayWorldRotationCB", &m_displayWorldRotation.m_enabled);
        ImGui::NextColumn();
        ImGui::TextColored(ImGui::Colors::s_PlainLabelColor, "%s", s_ColorText);
        ImGui::SameLine();
        ImGui::ColorEdit4("##DisplayWorldRotationCol", reinterpret_cast<float*>(&m_displayWorldRotation.m_color));

        // Set Column positions
        ImGui::SetColumnOffset(1, 200.0f);
        ImGui::SetColumnOffset(2, 270.0f);

        ImGui::Columns(1);

        // The 3rd parameter of this Combo box HAS to match the order of ImGuiLYEntityOutliner::HierarchyUpdateType
        ImGui::Combo("Hierarchy Update Type", reinterpret_cast<int*>(&m_hierarchyUpdateType), "Constant\0Update Tick");

        // Refresh the hierarchy / display further options, based on update type
        switch (m_hierarchyUpdateType)
        {
            default:
                break;

            case HierarchyUpdateType::UpdateTick:
                // allow a slider to determine tick time
                ImGui::SliderFloat("Update Tick Time", &m_hierarchyUpdateTickTimeTotal, 0.1f, 10.0f);
                ImGui::SameLine();
                ImGui::ProgressBar(m_hierarchyUpdateTickTimeCurrent / m_hierarchyUpdateTickTimeTotal);
                break;
        }

        ImGui::EndChild(); // "EntityOutliner_ViewOptionsMenuChild"
    }

    void ImGuiLYEntityOutliner::ImGuiUpdate_DrawComponentViewSubMenu()
    {
        AZ::SerializeContext *serializeContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);
        if (serializeContext != nullptr)
        {
            ImGui::TextColored(ImGui::Colors::s_NiceLabelColor, "Open All Debug Component Views for Component:");
            for (const AZ::TypeId& comDebugInfoEntry : m_componentDebugSortedList)
            {
                AZStd::string componentName("**name_not_found**");
                const AZ::SerializeContext::ClassData* classData = serializeContext->FindClassData(comDebugInfoEntry);
                if (classData != nullptr)
                {
                    componentName = classData->m_name;
                }

                // Component Name
                if (ImGui::MenuItem(componentName.c_str()))
                {
                    RequestAllViewsForComponent(comDebugInfoEntry);
                }
            }
        }
    }

    void ImGuiLYEntityOutliner::ImGuiUpdate_DrawAutoEnableOptions()
    {
        // Display/Remove Search Strings
        if (ImGui::CollapsingHeader("Component Auto Enable Search Strings", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed))
        {
            ImGui::BeginChild("ComponentSearchStringList", ImVec2(400.0f, 100.0f));
            ImGui::Columns(2);
            AZStd::string stringToRemove = ""; // Record if we elect to remove a string in any frame. Don't do anything if it remains ""
            for (const AZStd::string& searchString : m_autoEnableComponentSearchStrings)
            {
                ImGui::TextColored(ImGui::Colors::s_PlainLabelColor, "%s", searchString.c_str());
                ImGui::NextColumn();

                if (ImGui::Button(AZStd::string::format("Remove##%s", searchString.c_str()).c_str()))
                {
                    stringToRemove = searchString;
                }
                ImGui::NextColumn();
            }
            if (stringToRemove != "")
            {
                m_autoEnableComponentSearchStrings.erase(stringToRemove);
            }
            ImGui::Columns(1);
            ImGui::EndChild();
        
            // Add Search String
            static char searchCharArray[128] = "";
            ImGui::InputText("", searchCharArray, sizeof(searchCharArray));
            ImGui::SameLine();
            if (ImGui::Button(AZStd::string::format("Add '%s'", searchCharArray).c_str()))
            {
                const AZStd::string& searchString = searchCharArray;
                // Don't add an empty string.
                if (searchString != "")
                {
                    AddAutoEnableSearchString(searchString);
                }
            }
        }

        AZ::SerializeContext *serializeContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);
        if (serializeContext != nullptr)
        {
            if (ImGui::CollapsingHeader("ImGui Registered Components", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed))
            {
                ImGui::BeginChild("ImGuiRegisteredComponents", ImVec2(800.0f, 200.0f));
                ImGui::Columns(4);
                // Column Headers
                ImGui::TextColored(ImGui::Colors::s_PlainLabelColor, "Component Name");
                ImGui::NextColumn();
                ImGui::TextColored(ImGui::Colors::s_PlainLabelColor, "Priority");
                ImGui::NextColumn();
                ImGui::TextColored(ImGui::Colors::s_PlainLabelColor, "Auto Enable");
                ImGui::NextColumn();
                ImGui::TextColored(ImGui::Colors::s_PlainLabelColor, "Open All Of Type");
                ImGui::NextColumn();

                for (const AZ::TypeId& comDebugInfoEntry : m_componentDebugSortedList)
                {
                    AZStd::string componentName("**name_not_found**");
                    const AZ::SerializeContext::ClassData* classData = serializeContext->FindClassData(comDebugInfoEntry);
                    if (classData != nullptr)
                    {
                        componentName = classData->m_name;
                    }
                
                    // Component Name
                    ImGui::TextColored(ImGui::Colors::s_PlainLabelColor, "%s", componentName.c_str());
                    ImGui::NextColumn();

                    // Debug Priority
                    ImGui::TextColored(ImGui::Colors::s_PlainLabelColor, "%d", m_componentDebugInfoMap[comDebugInfoEntry].m_priority);
                    ImGui::NextColumn();

                    // Auto Enable
                    ImGui::TextColored(ImGui::Colors::s_PlainLabelColor, "Set:");
                    ImGui::SameLine();
                    ImGui::Checkbox(AZStd::string::format("##%s", componentName.c_str()).c_str(), &m_componentDebugInfoMap[comDebugInfoEntry].m_autoLaunchEnabled);
                    ImGui::NextColumn();

                    // Open All of Type Button
                    if (ImGui::Button(AZStd::string::format("Open All %s", componentName.c_str()).c_str()))
                    {
                        RequestAllViewsForComponent(comDebugInfoEntry);
                    }
                    ImGui::NextColumn();
                }

                // Set the Column Offsets
                ImGui::SetColumnOffset(1, 290.0f);
                ImGui::SetColumnOffset(2, 360.0f);
                ImGui::SetColumnOffset(3, 455.0f);

                // Turn off Columns
                ImGui::Columns(1);
                ImGui::EndChild();
            }
        }
    }

    void ImGuiLYEntityOutliner::ImGuiUpdate()
    {
        if (m_enabled)
        {
            if (ImGui::Begin("Entity Outliner", &m_enabled, ImGuiWindowFlags_MenuBar|ImGuiWindowFlags_HorizontalScrollbar|ImGuiWindowFlags_NoSavedSettings))
            {
                if (ImGui::BeginMenuBar())
                {
                    if (ImGui::BeginMenu("View Options##entityOutliner"))
                    {
                        ImGuiUpdate_DrawViewOptions();

                        ImGui::EndMenu();
                    }
                    if (ImGui::BeginMenu("Auto-Open Options##entityOutliner"))
                    {
                        ImGuiUpdate_DrawAutoEnableOptions();

                        ImGui::EndMenu();
                    }

                    ImGui::EndMenuBar();
                }
                // Refresh the Entity Hierarchy if we are going to
                // Refresh the hierarchy / display further options, based on update type
                switch (m_hierarchyUpdateType)
                {
                    default:
                    case HierarchyUpdateType::Constant:
                        // constant: just refresh every frame!
                        RefreshEntityHierarchy();
                        break;

                    case HierarchyUpdateType::UpdateTick:
                        // increment the timer
                        m_hierarchyUpdateTickTimeCurrent += ImGui::GetIO().DeltaTime;
                        if (m_hierarchyUpdateTickTimeCurrent > m_hierarchyUpdateTickTimeTotal)
                        {
                            m_hierarchyUpdateTickTimeCurrent = fmod(m_hierarchyUpdateTickTimeCurrent, m_hierarchyUpdateTickTimeTotal);
                            RefreshEntityHierarchy();
                        }
                        break;
                }

                // Draw the entity hierarchy
                ImGui::TextColored(ImGui::Colors::s_NiceLabelColor, "Entity Count: %d   Hierarchy:", m_totalEntitiesFound);

                // Draw the root entity and all its decendants as a collapsable menu
                ImGuiUpdate_RecursivelyDisplayEntityInfoAndDecendants(m_rootEntityInfo, true);
            }

            ImGui::End();
        }

        // Loop through our unordered_set of Entities to draw entity views for, and draw them!
        for (auto itr = m_entitiesToView.begin(); itr != m_entitiesToView.end(); )
        {
            if (!ImGuiUpdate_DrawEntityView(*itr))
            {
                auto itrToErase = itr;
                itr++;

                // ImGuiUpdate_DrawEntityView will return false if we need to close the window, so lets remove the entry at this itr
                m_entitiesToView.erase(*itrToErase);
            }
            else
            {
                itr++;
            }
        }

        // Loop through our unordered_set of Component/Entity pairs to draw component views for, and draw them!
        for (auto itr = m_componentsToView.begin(); itr != m_componentsToView.end(); )
        {
            if (!ImGuiUpdate_DrawComponentView(*itr))
            {
                auto itrToErase = itr;
                itr++;

                // ImGuiUpdate_DrawComponentView will return false if we need to close the window, so lets remove the entry at this itr
                m_componentsToView.erase(*itrToErase);
            }
            else
            {
                itr++;
            }
        }
    }

    bool ImGuiLYEntityOutliner::ImGuiUpdate_DrawEntityView(const AZ::EntityId &ent)
    {
        // Check to make sure the entity is still valid.. 
        AZ::Entity* entity = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationBus::Events::FindEntity, ent);
        bool viewWindow = (entity != nullptr);
        if (viewWindow)
        {
            AZStd::string entityName;
            AZ::ComponentApplicationBus::BroadcastResult(entityName, &AZ::ComponentApplicationBus::Events::GetEntityName, ent);

            AZStd::string windowLabel = AZStd::string::format("Entity View %s%s", entityName.c_str(), ent.ToString().c_str());
            if (ImGui::Begin(windowLabel.c_str(), &viewWindow, ImGuiWindowFlags_HorizontalScrollbar|ImGuiWindowFlags_NoSavedSettings))
            {
                ImGui::TextColored(ImGui::Colors::s_NiceLabelColor, "%s%s", entityName.c_str(), ent.ToString().c_str());

                // Draw the same thing that is in the full hierarchy
                ImGuiUpdate_RecursivelyDisplayEntityInfoAndDecendants(m_entityIdToInfoNodePtrMap[ent], false, false, true, true, false, true);
            }

            ImGui::End();
        }

        return viewWindow;
    }

    bool ImGuiLYEntityOutliner::ImGuiUpdate_DrawComponentView(const ImGui::ImGuiEntComponentId &entCom)
    {
        // Check to make sure the entity is still valid.. 
        AZ::Entity* entity = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationBus::Events::FindEntity, entCom.first);
        bool viewWindow = (entity != nullptr);
        if (viewWindow)
        {
            AZStd::string componentName("**name_not_found**");
            AZ::SerializeContext *serializeContext = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);
            if (serializeContext != nullptr)
            {
                const AZ::SerializeContext::ClassData* classData = serializeContext->FindClassData(entCom.second);
                if (classData != nullptr )
                {
                    componentName = classData->m_name;
                }
            }
            
            ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(250.0f, 200.0f));
            AZStd::string windowLabel = AZStd::string::format("Component View - %s - on Entity %s%s", componentName.c_str(), entity->GetName().c_str(), entCom.first.ToString().c_str());

            ImGuiWindowFlags flags = ImGuiWindowFlags_HorizontalScrollbar|ImGuiWindowFlags_NoSavedSettings;
            if (m_componentDebugInfoMap[entCom.second].m_menuBarEnabled)
            {
                flags |= ImGuiWindowFlags_MenuBar;
            }

            if (ImGui::Begin(windowLabel.c_str(), &viewWindow, flags))
            {
                // Attempt to draw any debug information for this component
                ImGuiUpdateDebugComponentListenerBus::Event(entCom, &ImGuiUpdateDebugComponentListenerBus::Events::OnImGuiDebugLYComponentUpdate);
            }

            ImGui::End();
            ImGui::PopStyleVar();
        }

        return viewWindow;
    }

    void ImGuiLYEntityOutliner::ImGuiUpdate_RecursivelyDisplayEntityInfoAndDecendants(EntityInfoNodePtr node, bool justDrawChildren /*= false*/, bool drawInspectButton /*= true*/, 
            bool drawTargetButton /*= true*/, bool drawDebugButton /*= true*/, bool sameLine /*= true*/, bool drawComponents /*= false*/)
    {
        if (node != nullptr)
        {
            AZStd::string childTreeNodeStr = node->m_entityId.ToString();
            // Draw the stuff
            if (!node->m_children.empty())
            {
                if (justDrawChildren)
                {
                    for (int i = 0; i < node->m_children.size(); i++)
                    {
                        ImGuiUpdate_RecursivelyDisplayEntityInfoAndDecendants(node->m_children[i]);
                    }
                }
                else if (sameLine)
                {
                    if (ImGui::TreeNode(childTreeNodeStr.c_str(), "%s", childTreeNodeStr.c_str()))
                    {
                        ImGuiUpdate_RecursivelyDisplayEntityInfoAndDecendants_DrawDisplayOptions(node, drawInspectButton, drawTargetButton, drawDebugButton, sameLine, drawComponents);

                        for (int i = 0; i < node->m_children.size(); i++)
                        {
                            ImGuiUpdate_RecursivelyDisplayEntityInfoAndDecendants(node->m_children[i]);
                        }
                        
                        ImGui::TreePop();
                    }
                    else
                    {
                        ImGuiUpdate_RecursivelyDisplayEntityInfoAndDecendants_DrawDisplayOptions(node, drawInspectButton, drawTargetButton, drawDebugButton, sameLine, drawComponents);
                    }
                }
                else
                {
                    ImGuiUpdate_RecursivelyDisplayEntityInfoAndDecendants_DrawDisplayOptions(node, drawInspectButton, drawTargetButton, drawDebugButton, sameLine, drawComponents);
                    childTreeNodeStr = AZStd::string::format("Children ##%s", childTreeNodeStr.c_str());
                    if (ImGui::TreeNode(childTreeNodeStr.c_str(), "%s", childTreeNodeStr.c_str()))
                    {
                        for (int i = 0; i < node->m_children.size(); i++)
                        {
                            ImGuiUpdate_RecursivelyDisplayEntityInfoAndDecendants(node->m_children[i]);
                        }

                        ImGui::TreePop();
                    }
                }
            }
            else if (!justDrawChildren)
            {
                ImGui::TextColored(ImGui::Colors::s_PlainLabelColor, "->"); ImGui::SameLine();
                ImGui::Text("%s", childTreeNodeStr.c_str());
                ImGuiUpdate_RecursivelyDisplayEntityInfoAndDecendants_DrawDisplayOptions(node, drawInspectButton, drawTargetButton, drawDebugButton, sameLine, drawComponents);
            }
        }
    }

    void ImGuiLYEntityOutliner::ImGuiUpdate_RecursivelyDisplayEntityInfoAndDecendants_DrawDisplayOptions(EntityInfoNodePtr node, bool drawInspectButton, bool drawTargetButton, bool drawDebugButton, bool sameLine, bool drawComponents)
    {
        if (node != nullptr)
        {
            // Entity Name
            if (m_displayName.m_enabled)
            {
                AZStd::string entityName;
                AZ::ComponentApplicationBus::BroadcastResult(entityName, &AZ::ComponentApplicationBus::Events::GetEntityName, node->m_entityId);
                if (sameLine)
                {
                    ImGui::SameLine();
                }
                ImGui::TextColored(m_displayName.m_color, "%s", entityName.c_str());
            }

            // Draw EntityViewer Button
            if (drawInspectButton)
            {
                if (sameLine)
                {
                    ImGui::SameLine();
                }
                const AZStd::string& inspectLabel = AZStd::string::format("Inspect##%s", node->m_entityId.ToString().c_str());
                if (ImGui::SmallButton(inspectLabel.c_str()))
                {
                    // If we clicked the button, attempt to insert this entity into the set. It will only accept unique values and will limit to 1 entry per entityid
                    RequestEntityView(node->m_entityId);
                }
            }

            // Target Button
            if (m_drawTargetViewButton && drawTargetButton)
            {
                if (sameLine)
                {
                    ImGui::SameLine();
                }
                const AZStd::string& targetLabel = AZStd::string::format("View##%s", node->m_entityId.ToString().c_str());
                if (ImGui::SmallButton(targetLabel.c_str()))
                {
                    // Send EBUS event out to Target an Entity. Up to game code to implement.
                    ImGuiEntityOutlinerNotificationBus::Broadcast(&IImGuiEntityOutlinerNotifications::OnImGuiEntityOutlinerTarget, node->m_entityId);
                }
            }

            // Debug Button
            if (drawDebugButton && !node->m_highestPriorityComponentDebug.IsNull())
            {
                const AZStd::string& debugLabel = AZStd::string::format("Debug##%s", node->m_entityId.ToString().c_str());
                if (sameLine)
                {
                    ImGui::SameLine();
                }
                if (ImGui::SmallButton(debugLabel.c_str()))
                {
                    RequestComponentView(ImGuiEntComponentId(node->m_entityId, node->m_highestPriorityComponentDebug));
                }
            }

            // Child Entity Count
            if (m_displayChildCount.m_enabled)
            {
                if (sameLine)
                {
                    ImGui::SameLine();
                }
                ImGui::TextColored(m_displayChildCount.m_color, "children: %zu", node->m_children.size());
            }

            // Descendant Entity Count
            if (m_displayDescentdantCount.m_enabled)
            {
                if (sameLine)
                {
                    ImGui::SameLine();
                }
                ImGui::TextColored(m_displayDescentdantCount.m_color, "descendants: %d", node->m_descendantCount);
            }

            // Entity State
            if (m_displayEntityState.m_enabled)
            {
                if (sameLine)
                {
                    ImGui::SameLine();
                }

                AZ::Entity* entity(nullptr);
                AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationBus::Events::FindEntity, node->m_entityId);
                
                AZStd::string stateString;
                
                if (entity == nullptr)
                {
                    stateString = "*invalid_entity_found*";
                }
                else
                {
                    switch (entity->GetState())
                    {
                        default:
                            stateString = "*unhandled_entity_state_found*";
                            break;

                        case AZ::Entity::State::Activating:
                            stateString = "ACTIVATING";
                            break;

                        case AZ::Entity::State::Active:
                            stateString = "ACTIVE";
                            break;

                        case AZ::Entity::State::Constructed:
                            stateString = "CONSTRUCTED";
                            break;

                        case AZ::Entity::State::Deactivating:
                            stateString = "DEACTIVATING";
                            break;

                        case AZ::Entity::State::Init:
                            stateString = "INIT";
                            break;

                        case AZ::Entity::State::Initializing:
                            stateString = "INITIALIZING";
                            break;
                    }
                }

                ImGui::TextColored(m_displayEntityState.m_color, "EntityState: %s", stateString.c_str());
            }

            // Parent Entity Information
            if (m_displayParentInfo.m_enabled)
            {
                AZ::EntityId parentId(AZ::EntityId::InvalidEntityId);
                if (node->m_parent != nullptr)
                {
                    parentId = node->m_parent->m_entityId;
                }

                AZStd::string parentName;
                AZ::ComponentApplicationBus::BroadcastResult(parentName, &AZ::ComponentApplicationBus::Events::GetEntityName, parentId);

                if (sameLine)
                {
                    ImGui::SameLine();
                }
                ImGui::TextColored(m_displayParentInfo.m_color, "Parent: %s%s", parentName.c_str(), parentId.ToString().c_str());
            }

            // Local Position 
            if (m_displayLocalPos.m_enabled)
            {
                AZ::Vector3 localPos = AZ::Vector3::CreateOne();
                AZ::TransformBus::EventResult(localPos, node->m_entityId, &AZ::TransformBus::Events::GetLocalTranslation);

                if (sameLine)
                {
                    ImGui::SameLine();
                }
                ImGui::TextColored(m_displayLocalPos.m_color, "localPos: (%.02f, %.02f, %.02f)", (float)localPos.GetX(), (float)localPos.GetY(), (float)localPos.GetZ());
            }

            // Local Rotation 
            if (m_displayLocalRotation.m_enabled)
            {
                AZ::Vector3 localRotation = AZ::Vector3::CreateOne();
                AZ::TransformBus::EventResult(localRotation, node->m_entityId, &AZ::TransformBus::Events::GetLocalRotation);

                if (sameLine)
                {
                    ImGui::SameLine();
                }
                ImGui::TextColored(m_displayLocalRotation.m_color, "localRot: (%.02f, %.02f, %.02f)", (float)localRotation.GetX(), (float)localRotation.GetY(), (float)localRotation.GetZ());
            }

            // World Position 
            if (m_displayWorldPos.m_enabled)
            {
                AZ::Vector3 worldPos = AZ::Vector3::CreateOne();
                AZ::TransformBus::EventResult(worldPos, node->m_entityId, &AZ::TransformBus::Events::GetWorldTranslation);

                if (sameLine)
                {
                    ImGui::SameLine();
                }
                ImGui::TextColored(m_displayWorldPos.m_color, "WorldPos: (%.02f, %.02f, %.02f)", (float)worldPos.GetX(), (float)worldPos.GetY(), (float)worldPos.GetZ());
            }

            // World Rotation 
            if (m_displayWorldRotation.m_enabled)
            {
                AZ::Vector3 worldRotation = AZ::Vector3::CreateOne();
                AZ::TransformBus::EventResult(worldRotation, node->m_entityId, &AZ::TransformBus::Events::GetWorldRotation);

                if (sameLine)
                {
                    ImGui::SameLine();
                }
                ImGui::TextColored(m_displayWorldRotation.m_color, "WorldRot: (%.02f, %.02f, %.02f)", (float)worldRotation.GetX(), (float)worldRotation.GetY(), (float)worldRotation.GetZ());
            }

            // Components
            if (drawComponents)
            {
                AZ::Entity* entity = nullptr;
                AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationBus::Events::FindEntity, node->m_entityId);
                if (entity != nullptr)
                {
                    // Draw collapsible menu for the components set
                    AZStd::string uiLabel = AZStd::string::format("Components##%s", node->m_entityId.ToString().c_str());
                    if (ImGui::TreeNode(uiLabel.c_str(), "%s", uiLabel.c_str()))
                    {
                        AZ::Entity::ComponentArrayType components = entity->GetComponents();
                        // we should sort our array of components based on their names. 
                        static auto sortByComponentName = [](AZ::Component* com1, AZ::Component* com2)
                        {
                            AZStd::string name1 = com1->RTTI_GetTypeName();
                            AZStd::string name2 = com2->RTTI_GetTypeName();
                            AZStd::to_lower(name1.begin(), name1.end());
                            AZStd::to_lower(name2.begin(), name2.end());
                            return name1 < name2;
                        };
                        AZStd::sort(components.begin(), components.end(), sortByComponentName);

                        for (auto component : components)
                        {
                            bool hasDebug = ComponentHasDebug(component->RTTI_GetType());
                            // Draw a collapsible menu for each component
                            uiLabel = AZStd::string::format("%s##%s", component->RTTI_GetTypeName(), node->m_entityId.ToString().c_str());
                            if (ImGui::TreeNode(uiLabel.c_str(), "%s", uiLabel.c_str()))
                            {
                                if (hasDebug)
                                {
                                    ImGui::SameLine();
                                    uiLabel = AZStd::string::format("Component Debug View##%s-%s", node->m_entityId.ToString().c_str(), component->RTTI_GetTypeName());
                                    if (ImGui::SmallButton(uiLabel.c_str()))
                                    {
                                        RequestComponentView(ImGuiEntComponentId(node->m_entityId, component->RTTI_GetType()));
                                    }
                                }

                                // Draw a collapsible menu for all Reflected Properties
                                uiLabel = AZStd::string::format("Reflected Properties##%s", node->m_entityId.ToString().c_str());
                                if (ImGui::TreeNode(uiLabel.c_str(), "%s", uiLabel.c_str()))
                                {
                                    AZ::SerializeContext *serializeContext = nullptr;
                                    AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);
                                    serializeContext->EnumerateObject(const_cast<AZ::Component *>(component),
                                        // beginElemCB
                                        [this](void *instance, const AZ::SerializeContext::ClassData *classData, const AZ::SerializeContext::ClassElement *classElement) -> bool
                                        {
                                            if (classElement != nullptr)
                                            {
                                                ImGuiUpdate_DrawComponent(instance, classData, classElement);
                                            }
                                            return true;
                                        },
                                        // endElemCB
                                        []() -> bool { return true; },
                                        AZ::SerializeContext::ENUM_ACCESS_FOR_READ, nullptr/* errorHandler */);
                                    
                                    ImGui::TreePop();
                                }

                                // Draw a collapsible menu for any potential component debuging stuff. 
                                if (hasDebug)
                                {
                                    uiLabel = AZStd::string::format("Debug##%s", node->m_entityId.ToString().c_str());
                                    if (ImGui::TreeNode(uiLabel.c_str(), "%s", uiLabel.c_str()))
                                    {
                                        // Attempt to draw any debug information for this component
                                        ImGuiUpdateDebugComponentListenerBus::Event(ImGuiEntComponentId(node->m_entityId, component->RTTI_GetType())
                                                    , &ImGuiUpdateDebugComponentListenerBus::Events::OnImGuiDebugLYComponentUpdate);

                                        ImGui::TreePop();
                                    }
                                }

                                ImGui::TreePop();
                            }
                            else if (hasDebug)
                            {
                                ImGui::SameLine();
                                uiLabel = AZStd::string::format("Component Debug View##%s-%s", node->m_entityId.ToString().c_str(), component->RTTI_GetTypeName());
                                if (ImGui::SmallButton(uiLabel.c_str()))
                                {
                                    RequestComponentView(ImGuiEntComponentId(node->m_entityId, component->RTTI_GetType()));
                                }
                            }
                        }

                        ImGui::TreePop();
                    }
                }
            }
        }
    }

    void ImGuiLYEntityOutliner::ImGuiUpdate_DrawComponent(void *instance, const AZ::SerializeContext::ClassData *classData, const AZ::SerializeContext::ClassElement *classElement)
    {
        const char *typeName = classData->m_name;
        AZStd::string value;
        if (classElement->m_typeId == AZ::SerializeGenericTypeInfo<char          >::GetClassTypeId()) { value = AZStd::string::format("%d", *reinterpret_cast<const char *>(instance)); }
        else if (classElement->m_typeId == AZ::SerializeGenericTypeInfo<AZ::s8        >::GetClassTypeId()) { value = AZStd::string::format("%d", *reinterpret_cast<const AZ::s8 *>(instance)); }
        else if (classElement->m_typeId == AZ::SerializeGenericTypeInfo<short         >::GetClassTypeId()) { value = AZStd::string::format("%d", *reinterpret_cast<const short *>(instance)); }
        else if (classElement->m_typeId == AZ::SerializeGenericTypeInfo<int           >::GetClassTypeId()) { value = AZStd::string::format("%d", *reinterpret_cast<const int *>(instance)); }
        else if (classElement->m_typeId == AZ::SerializeGenericTypeInfo<long          >::GetClassTypeId()) { value = AZStd::string::format("%ld", *reinterpret_cast<const long *>(instance)); }
        else if (classElement->m_typeId == AZ::SerializeGenericTypeInfo<AZ::s64       >::GetClassTypeId()) { value = AZStd::string::format("%lld", *reinterpret_cast<const AZ::s64 *>(instance)); }
        else if (classElement->m_typeId == AZ::SerializeGenericTypeInfo<unsigned char >::GetClassTypeId()) { value = AZStd::string::format("%u", *reinterpret_cast<const unsigned char *>(instance)); }
        else if (classElement->m_typeId == AZ::SerializeGenericTypeInfo<unsigned short>::GetClassTypeId()) { value = AZStd::string::format("%u", *reinterpret_cast<const unsigned short *>(instance)); }
        else if (classElement->m_typeId == AZ::SerializeGenericTypeInfo<unsigned int  >::GetClassTypeId()) { value = AZStd::string::format("%u", *reinterpret_cast<const unsigned int *>(instance)); }
        else if (classElement->m_typeId == AZ::SerializeGenericTypeInfo<unsigned long >::GetClassTypeId()) { value = AZStd::string::format("%lu", *reinterpret_cast<const unsigned long *>(instance)); }
        else if (classElement->m_typeId == AZ::SerializeGenericTypeInfo<AZ::u64       >::GetClassTypeId()) { value = AZStd::string::format("%llu", *reinterpret_cast<const AZ::u64 *>(instance)); }
        else if (classElement->m_typeId == AZ::SerializeGenericTypeInfo<float         >::GetClassTypeId()) { value = AZStd::string::format("%.*g", FLT_DIG, *reinterpret_cast<const float *>(instance)); }
        else if (classElement->m_typeId == AZ::SerializeGenericTypeInfo<double        >::GetClassTypeId()) { value = AZStd::string::format("%.*g", DBL_DIG, *reinterpret_cast<const double *>(instance)); }
        else if (classElement->m_typeId == AZ::SerializeGenericTypeInfo<bool          >::GetClassTypeId()) { value = AZStd::string::format(*reinterpret_cast<const bool *>(instance) ? "true" : "false"); }
        else if (classElement->m_typeId == AZ::SerializeGenericTypeInfo<AZStd::string >::GetClassTypeId()) { value = AZStd::string::format("\"%s\"", reinterpret_cast<const AZStd::string *>(instance)->c_str()); }
        else if (classElement->m_typeId == AZ::SerializeGenericTypeInfo<AZ::Vector3   >::GetClassTypeId())
        {
            const AZ::Vector3 *v = reinterpret_cast<const AZ::Vector3 *>(instance);
            value = AZStd::string::format("(%.*g %.*g %.*g)", FLT_DIG, (float)v->GetX(), FLT_DIG, (float)v->GetY(), FLT_DIG, (float)v->GetZ());
        }
        else if (classElement->m_typeId == AZ::SerializeGenericTypeInfo<AZ::Transform >::GetClassTypeId())
        {
            const AZ::Transform *t = reinterpret_cast<const AZ::Transform *>(instance);
            value = AZStd::string::format("pos(%.03f %.03f %.03f) x(%.03f %.03f %.03f) y(%.03f %.03f %.03f) z(%.03f %.03f %.03f)", (float)t->GetTranslation().GetX(), (float)t->GetTranslation().GetY(), (float)t->GetTranslation().GetZ(),
                (float)t->GetBasisX().GetX(), (float)t->GetBasisX().GetY(), (float)t->GetBasisX().GetZ(),
                (float)t->GetBasisY().GetX(), (float)t->GetBasisY().GetY(), (float)t->GetBasisY().GetZ(),
                (float)t->GetBasisZ().GetX(), (float)t->GetBasisZ().GetY(), (float)t->GetBasisZ().GetZ());
        }
        else if (classElement->m_typeId == AZ::GetAssetClassId())
        {
            const AZ::Data::Asset<AZ::Data::AssetData> *a = reinterpret_cast<const AZ::Data::Asset<AZ::Data::AssetData> *>(instance);
            value = AZStd::string::format("\"%s\"", a->GetHint().c_str());
        }
        // FIXME - add other types
        else if ((classData->m_container == nullptr || classData->m_container->Size(instance) == 0) && classData->m_elements.size() == 0)
        {
            // not yet sure if/how this could be useful, but maybe to detect certain types
#if 0
            if (classElement->m_genericClassInfo != nullptr)
            {
                AZ::SerializeContext::ClassData* cd = classElement->m_genericClassInfo->GetClassData();
                size_t numArgs = classElement->m_genericClassInfo->GetNumTemplatedArguments();
                const AZ::Uuid& sId = classElement->m_genericClassInfo->GetSpecializedTypeId();
                for (size_t i = 0; i < numArgs; i++)
                {
                    const AZ::Uuid& tId = classElement->m_genericClassInfo->GetTemplatedTypeId(i);
                    tId = tId;
                }
            }
#endif

            // this is either a leaf type or a type which doesn't expose children types, so this 'value' will be placeholder to let someone know about types whose value parsing is unimplemented
            value = "<value parsing not implemented for this type>";
        }

        // Actually draw the data!
        ImGui::TextColored(s_ComponentParamColor_Type, " -> %s", typeName);
        ImGui::SameLine();
        ImGui::TextColored(s_ComponentParamColor_Name, "\"%s\"", classElement->m_name);
        ImGui::SameLine();
        ImGui::TextColored(ImGui::Colors::s_PlainLabelColor, "(0x%llX)", reinterpret_cast<AZ::u64>(instance));
        if (!value.empty())
        {
            ImGui::SameLine();
            ImGui::TextColored(s_ComponentParamColor_Value, "%s", value.c_str());
        }
    }

    void ImGuiLYEntityOutliner::RefreshEntityHierarchy()
    {
        // Retrieve Id map from game entity context (editor->runtime).
        AzFramework::EntityContextId gameContextId = AzFramework::EntityContextId::CreateNull();
        AzFramework::GameEntityContextRequestBus::BroadcastResult(gameContextId, &AzFramework::GameEntityContextRequests::GetGameEntityContextId);

        // Get the Root Slice Component
        AZ::SliceComponent* rootSliceComponent;
        AzFramework::SliceEntityOwnershipServiceRequestBus::EventResult(rootSliceComponent, gameContextId,
            &AzFramework::SliceEntityOwnershipServiceRequests::GetRootSlice);

        if (rootSliceComponent)
        {
            // Get an unordered_set of all EntityIds in the slice
            AZ::SliceComponent::EntityIdSet entityIds;
            rootSliceComponent->GetEntityIds(entityIds);

            // Save off our count for use later.
            m_totalEntitiesFound = static_cast<int>(entityIds.size());

            // Clear the entityId to InfoNodePtr Map.
            m_entityIdToInfoNodePtrMap.clear();

            // Delete the root Entity Info node and all children recursively 
            DeleteEntityInfoAndDecendants(m_rootEntityInfo);
            m_rootEntityInfo.reset(); // Don't really have to do this, but will further ensure all nodes are deleted

            // Now, lets build the hierarchy! Not sure of the order of the entities, so it's a bit naieve. Will supply timers to control refresh rate
            //    First, build the root Node, which is kind of a fake node
            AZ::EntityId invalidEntId = AZ::EntityId(AZ::EntityId::InvalidEntityId);
            m_rootEntityInfo = new EntityInfoNode(invalidEntId, nullptr);
            m_entityIdToInfoNodePtrMap[invalidEntId] = m_rootEntityInfo;

            // Lets remove entity Ids from this set as we find their place in the hierarchy.
            while (!entityIds.empty())
            {
                // Keep a flag to see if we found any parent entities this round. If not, we should probably bail ( else, loop forever! )
                bool anyParentFound = false;
                for (auto it = entityIds.begin(); it != entityIds.end(); )
                {
                    AZ::EntityId childEntId = *it;
                    AZ::EntityId entityParent;
                    AZ::TransformBus::EventResult(entityParent, childEntId, &AZ::TransformBus::Events::GetParentId);

                    EntityInfoNodePtr parentEntInfo = FindEntityInfoByEntityId(entityParent, m_rootEntityInfo);
                    if (parentEntInfo != nullptr)
                    {
                        // We found our parent node! Lets create a node for ourselves and hang it off our parent
                        EntityInfoNodePtr node = new EntityInfoNode(childEntId, parentEntInfo);
                        parentEntInfo->m_children.push_back(node);
                        m_entityIdToInfoNodePtrMap[childEntId] = node;

                        // Delete this entity id from the unordered set, and get the next iterator
                        it = entityIds.erase(it);

                        // Flag that we have found any parent this round
                        anyParentFound = true;
                    }
                    else
                    {
                        it++; // Advance the iterator
                    }
                }

                // if we haven't found any new parents for remaining entities this round, we probably have rogue entities :(
                //    break here in this case to avoid infinite loop
                if (!anyParentFound)
                {
                    break;
                }
            }

            // with the hierarchy created, lets now traverse recursively and find every node's descendant count
            RefreshEntityHierarchy_FillCacheAndSort(m_rootEntityInfo);
        }
    }

    int ImGuiLYEntityOutliner::RefreshEntityHierarchy_FillCacheAndSort(EntityInfoNodePtr entityInfo)
    {
        int descendantCount = 0;
        for (int i = 0; i < entityInfo->m_children.size(); i++)
        {
            // Add one count for each of this entities children... 
            descendantCount++;

            // .. and additional counts for their children's decendants!
            descendantCount += RefreshEntityHierarchy_FillCacheAndSort(entityInfo->m_children[i]);
        }

        // we should sort our array of children as well, based on their names. 
        static auto sortByEntityName = [](const EntityInfoNodePtr& ent1, const EntityInfoNodePtr& ent2)
        {
            AZStd::string name1, name2;
            AZ::ComponentApplicationBus::BroadcastResult(name1, &AZ::ComponentApplicationBus::Events::GetEntityName, ent1->m_entityId);
            AZ::ComponentApplicationBus::BroadcastResult(name2, &AZ::ComponentApplicationBus::Events::GetEntityName, ent2->m_entityId);
            AZStd::to_lower(name1.begin(), name1.end());
            AZStd::to_lower(name2.begin(), name2.end());
            return name1 < name2;
        };
        AZStd::sort(entityInfo->m_children.begin(), entityInfo->m_children.end(), sortByEntityName);

        // Lets find this entities highest priority Debug Component.
        AZ::Entity* entity = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationBus::Events::FindEntity, entityInfo->m_entityId);
        if (entity != nullptr)
        {
            int highestPriority = -1;
            const AZ::Entity::ComponentArrayType& components = entity->GetComponents();
            for (auto c : components)
            {
                int pri = ComponentHasDebug(c->RTTI_GetType()) ? m_componentDebugInfoMap[c->RTTI_GetType()].m_priority : -1;
                if (pri > highestPriority)
                {
                    highestPriority = pri;
                    entityInfo->m_highestPriorityComponentDebug = c->RTTI_GetType();
                }
            }
            
            // if we didn't find any debug components, create a null TypeId for highest priority component
            if (highestPriority == -1)
            {
                entityInfo->m_highestPriorityComponentDebug = AZ::TypeId::CreateNull();
            }
        }

        return entityInfo->m_descendantCount = descendantCount;
    }

    ImGuiLYEntityOutliner::EntityInfoNodePtr ImGuiLYEntityOutliner::FindEntityInfoByEntityId(const AZ::EntityId &entityId, EntityInfoNodePtr searchNode)
    {
        if (searchNode != nullptr)
        {
            // if the provided node matches, return it!
            if (searchNode->m_entityId == entityId)
            {
                return searchNode;
            }

            // lets check our children
            for (int i = 0; i < searchNode->m_children.size(); i++)
            {
                // See if we find the info in all decendants
                EntityInfoNodePtr foundNode = FindEntityInfoByEntityId(entityId, searchNode->m_children[i]);
                if (foundNode != nullptr)
                {
                    // return the Child Node that was found!
                    return foundNode;
                }
            }
        }

        // we found nothing! Return a nullptr
        return nullptr;
    }

    void ImGuiLYEntityOutliner::DeleteEntityInfoAndDecendants(EntityInfoNodePtr entityInfo)
    {
        if (entityInfo != nullptr)
        {
            for (int i = 0; i < entityInfo->m_children.size(); i++)
            {
                // Recursively Delete Children
                DeleteEntityInfoAndDecendants(entityInfo->m_children[i]);
            }

            // We need to clear the array to remove any child smart pointers
            entityInfo->m_children.clear();

            // Now delete the node contents by reseting the smart pointer
            entityInfo.reset();
        }
    }

    void ImGuiLYEntityOutliner::RequestEntityView(AZ::EntityId entity)
    {
        m_entitiesToView.insert(entity);
    }
    
    void ImGuiLYEntityOutliner::RemoveEntityView(AZ::EntityId entity)
    {
        m_entitiesToView.erase(entity);
    }

    void ImGuiLYEntityOutliner::RequestComponentView(ImGuiEntComponentId component)
    {
        m_componentsToView.insert(component);
    }

    void ImGuiLYEntityOutliner::RemoveComponentView(ImGuiEntComponentId component)
    {
        m_componentsToView.erase(component);
    }

    void ImGuiLYEntityOutliner::RequestAllViewsForComponent(const AZ::TypeId& comType)
    {
        // To do this, we want to iterate through all component views connected to the bus
        ImGui::ImGuiUpdateDebugComponentListenerBus::EnumerateHandlers([&comType, this](ImGui::IImGuiUpdateDebugComponentListener* imGuiComListener)
        {
            if (AZ::Component* com = azrtti_cast<AZ::Component*>(imGuiComListener))
            {
                // If we found a Handler of this component type, open up the component view!
                if (azrtti_istypeof(comType, com))
                {
                    ImGui::ImGuiEntComponentId id(com->GetEntityId(), comType);
                    RequestComponentView(id);
                }
            }
            return true;
        });
    }
    
    void ImGuiLYEntityOutliner::EnableTargetViewMode(bool enabled)
    {
        m_drawTargetViewButton = enabled;
    }

    void ImGuiLYEntityOutliner::SetEnabled(bool enabled)
    {
        m_enabled = enabled;
    }

    void ImGuiLYEntityOutliner::AddAutoEnableSearchString(const AZStd::string& searchString)
    {
        // Copy off the string and to_lower it
        AZStd::string stringToAdd = searchString;
        AZStd::to_lower(stringToAdd.begin(), stringToAdd.end());

        // Insert the lower-cased string into our set
        m_autoEnableComponentSearchStrings.insert(stringToAdd);

        RefreshAutoEnableBasedOnSearchStrings();
    }

    void ImGuiLYEntityOutliner::RefreshAutoEnableBasedOnSearchStrings()
    {
        AZ::SerializeContext *serializeContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);
        if (serializeContext != nullptr)
        {
            // Iterate through the auto Enable set and flick on Component debugs
            for (AZ::TypeId& componentDebugInfoEntry : m_componentDebugSortedList)
            {
                // We are only really checking to add components, so if we are already added, then move on!
                if (!m_componentDebugInfoMap[componentDebugInfoEntry].m_autoLaunchEnabled)
                {
                    AZStd::string componentName("**name_not_found**");
                    const AZ::SerializeContext::ClassData* classData = serializeContext->FindClassData(componentDebugInfoEntry);
                    if (classData != nullptr)
                    {
                        componentName = classData->m_name;
                        AZStd::to_lower(componentName.begin(), componentName.end());
                        // Loop through the known Debugable components and see if we find our search string! If so, flick on autoLaunch
                        for (const AZStd::string& searchString : m_autoEnableComponentSearchStrings)
                        {
                            if (componentName.find(searchString) != AZStd::string::npos)
                            {
                                m_componentDebugInfoMap[componentDebugInfoEntry].m_autoLaunchEnabled = true;
                            }
                        }
                    }
                }
            }
        }
    }

    bool ImGuiLYEntityOutliner::ComponentHasDebug(const AZ::TypeId& comType)
    {
        return m_componentDebugInfoMap.find(comType) != m_componentDebugInfoMap.end();
    }

    void ImGuiLYEntityOutliner::EnableComponentDebug(const AZ::TypeId& comType, int priority /*= 1*/, bool enableMenuBar /*= false*/)
    {
        // if not found, add to vector and sort on priorities!
        if (!ComponentHasDebug(comType))
        {
            // Add to the vector
            ComponentDebugInfo debugInfo(priority, enableMenuBar, false);
            m_componentDebugSortedList.push_back(comType); // Add the entry to a Vector for 1) Constant Iteration, and 2) Ordering and Sorting
            m_componentDebugInfoMap[comType] = debugInfo; // Add the entry to a Map for quick access if needed per frame

            // Sort the list
            static auto sortByComponentPriority = [this](const AZ::TypeId& type1, const AZ::TypeId& type2)
            {
                return m_componentDebugInfoMap[type1].m_priority > m_componentDebugInfoMap[type2].m_priority;
            };            
            m_componentDebugSortedList.sort(sortByComponentPriority);

            // Loop through the Search Strings and see if we should enable any components
            RefreshAutoEnableBasedOnSearchStrings();
        }

        // regardless of if this is a new or existing component debug, this call signifies a new Connection has likely been made
        //  and thus, a new ImGui Component Debug Panel to display. Check here for the Debug Auto Enable Component flag for this component type
        if (m_componentDebugInfoMap[comType].m_autoLaunchEnabled)
        {
            RequestAllViewsForComponent(comType);
        }
    }
} // namespace ImGui

#endif // IMGUI_ENABLED
