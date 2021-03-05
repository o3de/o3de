#pragma once

/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include <AzCore/std/functional.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/set.h>
#include <AzCore/std/containers/unordered_set.h>
#include <SceneAPI/SceneCore/Containers/SceneGraph.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace DataTypes
        {
            class ISceneNodeSelectionList;
        }
        namespace Utilities
        {
            // SceneGraphSelector provides utilities including converting selected and unselected node lists
            // in the MeshGroup into the final target node list.
            class SceneGraphSelector
            {
            public:
                using NodeFilterFunction = AZStd::function<bool(const Containers::SceneGraph& graph, Containers::SceneGraph::NodeIndex& index)>;

                SCENE_CORE_API static AZStd::vector<AZStd::string> GenerateTargetNodes(const Containers::SceneGraph& graph, const DataTypes::ISceneNodeSelectionList& list,
                    NodeFilterFunction nodeFilter);
                SCENE_CORE_API static void SelectAll(const Containers::SceneGraph& graph, DataTypes::ISceneNodeSelectionList& list);
                SCENE_CORE_API static void UnselectAll(const Containers::SceneGraph& graph, DataTypes::ISceneNodeSelectionList& list);
                SCENE_CORE_API static void UpdateNodeSelection(const Containers::SceneGraph& graph, DataTypes::ISceneNodeSelectionList& list);
                SCENE_CORE_API static void UpdateTargetNodes(const Containers::SceneGraph& graph, DataTypes::ISceneNodeSelectionList& list,
                    const AZStd::unordered_set<AZStd::string>& targetNodes, NodeFilterFunction nodeFilter);

                SCENE_CORE_API static bool IsTreeViewType(const Containers::SceneGraph& graph, Containers::SceneGraph::NodeIndex& index);
                SCENE_CORE_API static bool IsMesh(const Containers::SceneGraph& graph, Containers::SceneGraph::NodeIndex& index);
                SCENE_CORE_API static bool IsMeshObject(const AZStd::shared_ptr<const DataTypes::IGraphObject>& object);

            private:
                static void CopySelectionToSet(AZStd::set<AZStd::string>& selected, AZStd::set<AZStd::string>& unselected, const DataTypes::ISceneNodeSelectionList& list);
                static void CorrectRootNode(const Containers::SceneGraph& graph, AZStd::set<AZStd::string>& selected, AZStd::set<AZStd::string>& unselected);
            };
        }
    }
}