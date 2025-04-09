#pragma once

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/std/functional.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/set.h>
#include <AzCore/std/containers/unordered_set.h>
#include <SceneAPI/SceneCore/Containers/SceneGraph.h>

namespace AZ::SceneAPI::DataTypes {
    class ISceneNodeSelectionList;
    class ISceneNodeGroup;
}

namespace AZ::SceneAPI::Utilities
{
    inline constexpr AZStd::string_view OptimizedMeshSuffix = "_optimized";
    inline constexpr AZStd::string_view OptimizedMeshPropertyMapKey = "o3de_optimized_mesh_node";
    inline constexpr AZStd::string_view OriginalUnoptimizedMeshPropertyMapKey = "o3de_original_unoptimized_mesh_node";

    // SceneGraphSelector provides utilities including converting selected and unselected node lists
    // in the MeshGroup into the final target node list.
    class SceneGraphSelector
    {
    public:
        using NodeFilterFunction = bool(const Containers::SceneGraph& graph, Containers::SceneGraph::NodeIndex& index);
        using NodeRemapFunction = Containers::SceneGraph::NodeIndex(const Containers::SceneGraph& graph, const Containers::SceneGraph::NodeIndex& index);

        SCENE_CORE_API static AZStd::vector<AZStd::string> GenerateTargetNodes(
            const Containers::SceneGraph& graph,
            const DataTypes::ISceneNodeSelectionList& list,
            NodeFilterFunction nodeFilter,
            NodeRemapFunction nodeRemap = NoRemap);
        SCENE_CORE_API static void SelectAll(const Containers::SceneGraph& graph, DataTypes::ISceneNodeSelectionList& list);
        SCENE_CORE_API static void UnselectAll(const Containers::SceneGraph& graph, DataTypes::ISceneNodeSelectionList& list);
        SCENE_CORE_API static void UpdateNodeSelection(const Containers::SceneGraph& graph, DataTypes::ISceneNodeSelectionList& list);
        SCENE_CORE_API static void UpdateTargetNodes(const Containers::SceneGraph& graph, DataTypes::ISceneNodeSelectionList& list,
            const AZStd::unordered_set<AZStd::string>& targetNodes, NodeFilterFunction nodeFilter);

        SCENE_CORE_API static bool IsTreeViewType(const Containers::SceneGraph& graph, Containers::SceneGraph::NodeIndex& index);
        SCENE_CORE_API static bool IsMesh(const Containers::SceneGraph& graph, Containers::SceneGraph::NodeIndex& index);
        SCENE_CORE_API static bool IsMeshObject(const AZStd::shared_ptr<const DataTypes::IGraphObject>& object);

        //! Returns the index that is passed in without remapping it.
        //! GenerateTargetNodes takes a NodeRemapFunction as input. NoRemap is used as the default
        //! if you want to call GenerateTargetNodes without doing any re-mapping
        SCENE_CORE_API static Containers::SceneGraph::NodeIndex NoRemap(
            [[maybe_unused]] const Containers::SceneGraph& /*graph*/,
            const Containers::SceneGraph::NodeIndex& index)
        {
            return index;
        }

        //! Remaps unoptimizedMeshNodeIndex to the optimized version of the mesh, if it exists.
        //! @param graph The scene graph
        //! @param unoptimizedMeshNodeIndex The node index of the unoptimized mesh
        //! @return Returns the node index of the optimized mesh if it exists, or unoptimizedMeshNodeIndex if it doesn't exist
        SCENE_CORE_API static Containers::SceneGraph::NodeIndex RemapToOptimizedMesh(
            const Containers::SceneGraph& graph, const Containers::SceneGraph::NodeIndex& unoptimizedMeshNodeIndex);

        //! Remap optimizedMeshNodeIndex to the original unoptimized version of the mesh, if it exists
        //! @param graph The scene graph
        //! @param optimizedMeshNodeIndex The node index of the optimized mesh
        //! @return Returns the node index of the original unoptimized mesh if it exists. Returns optimizedMeshNodeIndex if it doesn't exist.
        SCENE_CORE_API static Containers::SceneGraph::NodeIndex RemapToOriginalUnoptimizedMesh(
            const Containers::SceneGraph& graph, const Containers::SceneGraph::NodeIndex& optimizedMeshNodeIndex);

        //! Look for a ICustomPropertyData child node. If it exists, use the property map
        //! to look for an entry that matches customPropertyKey, and return the result
        //! @param graph The scene graph
        //! @param index The node index that is being remapped
        //! @param customPropertyKey The key to use to look up the remapped index in the property map
        //! @return Returns the remapped node index if one matching customPropertyKey is found. Returns the input index if it doesn't exist.
        SCENE_CORE_API static Containers::SceneGraph::NodeIndex RemapNodeIndex(
            const Containers::SceneGraph& graph,
            const Containers::SceneGraph::NodeIndex& index,
            const AZStd::string_view customPropertyKey);

        //! Generate a name for an optimized mesh node based on the name of the original node and the mesh group it belongs to
        SCENE_CORE_API static AZStd::string GenerateOptimizedMeshNodeName(
            const Containers::SceneGraph& graph,
            const Containers::SceneGraph::NodeIndex& unoptimizedMeshNodeIndex,
            const DataTypes::ISceneNodeGroup& meshGroup);

    private:
        static void CopySelectionToSet(
            AZStd::unordered_set<AZStd::string>& selected, AZStd::unordered_set<AZStd::string>& unselected,
            const DataTypes::ISceneNodeSelectionList& list);
        static void CorrectRootNode(
            const Containers::SceneGraph& graph,
            AZStd::unordered_set<AZStd::string>& selected, AZStd::unordered_set<AZStd::string>& unselected);
    };
}
