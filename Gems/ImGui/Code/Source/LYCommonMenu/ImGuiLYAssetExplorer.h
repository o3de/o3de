/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once
#include "ImGuiManager.h"

#ifdef IMGUI_ENABLED
#include "ImGuiBus.h"
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Asset/AssetManagerBus.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/std/containers/map.h>

namespace ImGui
{
    struct MeshInstanceOptions
    {
        bool m_selectedForDraw; // Has this Instance been selected in the Selection Filter
        bool m_mousedOverForDraw; // Is this Instance Moused Over in ImGui for Draw In World
        bool m_verifiedThisFrame; // Is this Instance verified as being active still? ( Used when rescanning the Scene heirarchy for Meshes )
        bool m_passesFilter; // Does this Entity Name/ID pass the String Filter ( only figured when filter string changes, flag stored here )
        AZStd::string m_instanceLabel; // A String made at struct creation time with Entity Name and ID, for quick searching later.
    };

    struct MeshInstanceDisplayList
    {
        AZStd::string m_meshPath; // The Full Path String for this Mesh Asset
        AZStd::map<AZ::EntityId, MeshInstanceOptions> m_instanceOptionMap; // A Map of Entities that have been found with this mesh, to options for that instance
        bool m_selectedForDraw; // Has this Mesh been selected in the Selection Filter
        bool m_mousedOverForDraw; // Is this Instance Moused Over in ImGui for Draw In World
        bool m_childMousedOverForDraw; // Are one of this Mesh's children moused over for draw? ( Helps not disclude this Mesh when drawing later )
        bool m_passesFilter; // Does this Mesh Path Name pass the String Filter ( only figured when the filter string changes, flag stored here )
        bool m_childrenPassFilter; // Does even one of this entities Children Pass their Entity Name Filters ( used to hide when there are zero relevant children )
    };

    class ImGuiLYAssetExplorer
        : public ImGuiAssetExplorerRequestBus::Handler

    {
    public:
        ImGuiLYAssetExplorer();
        ~ImGuiLYAssetExplorer();

        // Called from owner
        void Initialize();
        void Shutdown();

        // Draw the ImGui Menu
        void ImGuiUpdate();

        // -- ImGuiAssetExplorerRequestBus::Handler Interface ----------------------
        void SetEnabled(bool enabled) override { m_enabled = enabled; m_meshDebugEnabled = enabled; }
        // -- ImGuiAssetExplorerRequestBus::Handler Interface ----------------------

        // Toggle the menu on and off
        void ToggleEnabled() { m_enabled = !m_enabled; }

    private:
        // flag for if the entire Menu enabled / visible
        bool m_enabled;

        // Mesh Debugger Enabled and Filter Options
        bool m_meshDebugEnabled;
        bool m_lodDebugEnabled;
        bool m_distanceFilter;
        bool m_selectionFilter;
        bool m_enabledMouseOvers;
        bool m_anyMousedOverForDraw;
        float m_distanceFilter_near;
        float m_distanceFilter_far;
        bool m_meshNameFilter;
        AZStd::string m_meshNameFilterStr;
        bool m_entityNameFilter;
        AZStd::string m_entityNameFilterStr;

        // In World Display Options
        bool m_inWorld_drawOriginSphere;
        float m_inWorld_originSphereRadius;
        bool m_inWorld_drawLabel;
        bool m_inWorld_label_monoSpace;
        bool m_inWorld_label_framed;
        ImColor m_inWorld_label_textColor;
        float m_inWorld_labelTextSize;
        bool m_inWorld_drawAABB;
        bool m_inWorld_debugDrawMesh;
        bool m_inWorld_label_entityName;
        bool m_inWorld_label_materialName;
        bool m_inWorld_label_totalLods;
        bool m_inWorld_label_miscLod;
        
        // Get a Mesh Display List from the Map and init it if it is new.
        MeshInstanceDisplayList& FindOrCreateMeshInstanceList(const char* meshName);

        // Iterate through our Map and check for Mesh and Entity Name filters
        void MeshInstanceList_CheckMeshFilter();
        void MeshInstanceList_CheckEntityFilter();

        // The Primary list of Meshes and Instances of them
        AZStd::list<MeshInstanceDisplayList> m_meshInstanceDisplayList;

        // Helper functions for the ImGui Update
        void ImGuiUpdate_DrawMenu();
        void ImGuiUpdate_DrawViewOptions();
        void ImGuiUpdate_DrawMeshMouseOver(MeshInstanceDisplayList& meshDisplayList);
        void ImGuiUpdate_DrawEntityInstanceMouseOver(MeshInstanceDisplayList& meshDisplayList, AZ::EntityId& entityInstance, AZStd::string& entityName, MeshInstanceOptions& instanceOptions);

    };
}
#endif // IMGUI_ENABLED
