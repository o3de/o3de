#pragma once

/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/std/functional.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/set.h>
#include <AzCore/std/containers/unordered_set.h>
#include <SceneAPI/SceneCore/Containers/SceneGraph.h>

namespace AZ::SceneAPI::DataTypes { class ISceneNodeSelectionList; }

namespace AZ::SceneAPI::Utilities
{
    inline constexpr AZStd::string_view OptimizedMeshSuffix = "_optimized";

    // SceneGraphSelector provides utilities including converting selected and unselected node lists
    // in the MeshGroup into the final target node list.
    class SceneGraphSelector
    {
    public:
        using NodeFilterFunction = bool(const Containers::SceneGraph& graph, Containers::SceneGraph::NodeIndex& index);
        using NodeRemapFunction = Containers::SceneGraph::NodeIndex(const Containers::SceneGraph& graph, const Containers::SceneGraph::NodeIndex& index);

        SCENE_CORE_API static AZStd::vector<AZStd::string> GenerateTargetNodes(const Containers::SceneGraph& graph, const DataTypes::ISceneNodeSelectionList& list,
            NodeFilterFunction nodeFilter, NodeRemapFunction nodeRemap = NoRemap);
        SCENE_CORE_API static void SelectAll(const Containers::SceneGraph& graph, DataTypes::ISceneNodeSelectionList& list);
        SCENE_CORE_API static void UnselectAll(const Containers::SceneGraph& graph, DataTypes::ISceneNodeSelectionList& list);
        SCENE_CORE_API static void UpdateNodeSelection(const Containers::SceneGraph& graph, DataTypes::ISceneNodeSelectionList& list);
        SCENE_CORE_API static void UpdateTargetNodes(const Containers::SceneGraph& graph, DataTypes::ISceneNodeSelectionList& list,
            const AZStd::unordered_set<AZStd::string>& targetNodes, NodeFilterFunction nodeFilter);

        SCENE_CORE_API static bool IsTreeViewType(const Containers::SceneGraph& graph, Containers::SceneGraph::NodeIndex& index);
        SCENE_CORE_API static bool IsMesh(const Containers::SceneGraph& graph, Containers::SceneGraph::NodeIndex& index);
        SCENE_CORE_API static bool IsMeshObject(const AZStd::shared_ptr<const DataTypes::IGraphObject>& object);

        SCENE_CORE_API static Containers::SceneGraph::NodeIndex NoRemap(const Containers::SceneGraph& /*graph*/, const Containers::SceneGraph::NodeIndex& index)
        {
            return index;
        }
        SCENE_CORE_API static Containers::SceneGraph::NodeIndex RemapToOptimizedMesh(const Containers::SceneGraph& graph, const Containers::SceneGraph::NodeIndex& index);

    private:
        static void CopySelectionToSet(AZStd::set<AZStd::string>& selected, AZStd::set<AZStd::string>& unselected, const DataTypes::ISceneNodeSelectionList& list);
        static void CorrectRootNode(const Containers::SceneGraph& graph, AZStd::set<AZStd::string>& selected, AZStd::set<AZStd::string>& unselected);
    };
}
