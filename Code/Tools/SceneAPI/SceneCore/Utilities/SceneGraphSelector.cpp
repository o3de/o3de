/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <SceneAPI/SceneCore/Containers/Utilities/SceneGraphUtilities.h>
#include <SceneAPI/SceneCore/Containers/Views/SceneGraphDownwardsIterator.h>
#include <SceneAPI/SceneCore/Containers/Views/PairIterator.h>
#include <SceneAPI/SceneCore/Utilities/SceneGraphSelector.h>
#include <AzCore/std/containers/set.h>
#include <AzCore/Math/Transform.h>
#include <SceneAPI/SceneCore/DataTypes/ManifestBase/ISceneNodeSelectionList.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/ICustomPropertyData.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IMeshData.h>
#include <SceneAPI/SceneCore/DataTypes/Groups/ISceneNodeGroup.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace Utilities
        {
            bool SceneGraphSelector::IsTreeViewType(const Containers::SceneGraph& graph, Containers::SceneGraph::NodeIndex& index)
            {
                if (index == graph.GetRoot())
                {
                    return false;
                }
                return !graph.IsNodeEndPoint(index);
            }

            bool SceneGraphSelector::IsMesh(const Containers::SceneGraph& graph, Containers::SceneGraph::NodeIndex& index)
            {
                AZStd::shared_ptr<const DataTypes::IGraphObject> object = graph.GetNodeContent(index);
                return SceneGraphSelector::IsMeshObject(object);
            }

            bool SceneGraphSelector::IsMeshObject(const AZStd::shared_ptr<const DataTypes::IGraphObject>& object)
            {
                return object && object->RTTI_IsTypeOf(DataTypes::IMeshData::TYPEINFO_Uuid());
            }

            Containers::SceneGraph::NodeIndex SceneGraphSelector::RemapToOptimizedMesh(
                const Containers::SceneGraph& graph, const Containers::SceneGraph::NodeIndex& unoptimizedMeshNodeIndex)
            {
                return RemapNodeIndex(graph, unoptimizedMeshNodeIndex, SceneAPI::Utilities::OptimizedMeshPropertyMapKey);
            }

            Containers::SceneGraph::NodeIndex SceneGraphSelector::RemapToOriginalUnoptimizedMesh(
                const Containers::SceneGraph& graph, const Containers::SceneGraph::NodeIndex& optimizedMeshNodeIndex)
            {
                return RemapNodeIndex(graph, optimizedMeshNodeIndex, SceneAPI::Utilities::OriginalUnoptimizedMeshPropertyMapKey);
            }

            Containers::SceneGraph::NodeIndex SceneGraphSelector::RemapNodeIndex(
                const Containers::SceneGraph& graph,
                const Containers::SceneGraph::NodeIndex& index,
                const AZStd::string_view customPropertyKey)
            {
                // Search the immediate children for an ICustomPropertyData node to lookup the optimized mesh index
                const Containers::SceneGraph::NodeIndex customPropertyIndex =
                    GetImmediateChildOfType(graph, index, azrtti_typeid<AZ::SceneAPI::DataTypes::ICustomPropertyData>());

                if (customPropertyIndex.IsValid())
                {
                    const DataTypes::ICustomPropertyData* customPropertyDataNode =
                        azrtti_cast<const DataTypes::ICustomPropertyData*>(graph.GetNodeContent(customPropertyIndex).get());

                    // Now look up the optimized index
                    const auto& propertyMap = customPropertyDataNode->GetPropertyMap();
                    const auto iter = propertyMap.find(customPropertyKey);

                    if (iter != propertyMap.end())
                    {
                        const AZStd::any& remappedNodeIndex = iter->second;

                        if (!remappedNodeIndex.empty() && remappedNodeIndex.is<Containers::SceneGraph::NodeIndex>())
                        {
                            return AZStd::any_cast<Containers::SceneGraph::NodeIndex>(remappedNodeIndex);
                        }
                    }
                }

                // Return the original index if there is no optimized mesh node
                return index;
            }

            AZStd::string SceneGraphSelector::GenerateOptimizedMeshNodeName(
                const Containers::SceneGraph& graph,
                const Containers::SceneGraph::NodeIndex& unoptimizedMeshNodeIndex,
                const DataTypes::ISceneNodeGroup& sceneNodeGroup)
            {
                const auto& nodeName = graph.GetNodeName(unoptimizedMeshNodeIndex);

                return AZStd::string::format(
                    "%s_%s%s", nodeName.GetName(), sceneNodeGroup.GetName().c_str(), SceneAPI::Utilities::OptimizedMeshSuffix.data());
            }

            AZStd::vector<AZStd::string> SceneGraphSelector::GenerateTargetNodes(
                const Containers::SceneGraph& graph,
                const DataTypes::ISceneNodeSelectionList& list,
                NodeFilterFunction nodeFilter,
                NodeRemapFunction nodeRemap)
            {
                AZStd::vector<AZStd::string> targetNodes;
                AZStd::unordered_set<AZStd::string> selectedNodesSet;
                AZStd::unordered_set<AZStd::string> unselectedNodesSet;
                CopySelectionToSet(selectedNodesSet, unselectedNodesSet, list);
                CorrectRootNode(graph, selectedNodesSet, unselectedNodesSet);

                const auto nodeIterator = graph.ConvertToHierarchyIterator(graph.GetRoot());
                const auto view = Containers::Views::MakeSceneGraphDownwardsView<Containers::Views::BreadthFirst>(graph, nodeIterator, graph.GetContentStorage().cbegin(), true);
                if (view.begin() == view.end())
                {
                    return targetNodes;
                }
                for (auto it = ++view.begin(); it != view.end(); ++it)
                {
                    Containers::SceneGraph::NodeIndex index = graph.ConvertToNodeIndex(it.GetHierarchyIterator());
                    AZStd::string currentNodeName = graph.GetNodeName(index).GetPath();
                    if (unselectedNodesSet.contains(currentNodeName))
                    {
                        continue;
                    }
                    if (selectedNodesSet.contains(currentNodeName))
                    {
                        if (nodeFilter(graph, index))
                        {
                            Containers::SceneGraph::NodeIndex remappedIndex = nodeRemap(graph, index);
                            if (remappedIndex == index)
                            {
                                targetNodes.emplace_back(AZStd::move(currentNodeName));
                            }
                            else
                            {
                                const Containers::SceneGraph::Name& nodeName = graph.GetNodeName(remappedIndex);
                                targetNodes.emplace_back(nodeName.GetPath(), nodeName.GetPathLength());
                            }
                        }
                        continue;
                    }

                    Containers::SceneGraph::NodeIndex parentIndex = graph.GetNodeParent(index);
                    if (parentIndex.IsValid())
                    {
                        AZStd::string parentNodeName = graph.GetNodeName(parentIndex).GetPath();
                        if (unselectedNodesSet.contains(parentNodeName))
                        {
                            unselectedNodesSet.insert(currentNodeName);
                        }
                        else if (selectedNodesSet.contains(parentNodeName))
                        {
                            selectedNodesSet.insert(currentNodeName);
                            if (nodeFilter(graph, index))
                            {
                                Containers::SceneGraph::NodeIndex remappedIndex = nodeRemap(graph, index);
                                if (remappedIndex == index)
                                {
                                    targetNodes.emplace_back(AZStd::move(currentNodeName));
                                }
                                else
                                {
                                    const Containers::SceneGraph::Name& nodeName = graph.GetNodeName(remappedIndex);
                                    targetNodes.emplace_back(nodeName.GetPath(), nodeName.GetPathLength());
                                }
                            }
                        }
                        else
                        {
                            AZ_Assert(false, "SceneGraphSelector does a breadth first iteration so parent should be processed, "
                                "but '%s' wasn't found in either selected or unselected list.", currentNodeName.c_str());
                        }
                    }
                }
                return targetNodes;
            }

            void SceneGraphSelector::SelectAll(const Containers::SceneGraph& graph, DataTypes::ISceneNodeSelectionList& list)
            {
                list.ClearSelectedNodes();
                list.ClearUnselectedNodes();
                
                auto hierarchyStorage = graph.GetHierarchyStorage();
                auto nameStorage = graph.GetNameStorage();                
                auto range = Containers::Views::MakePairView(hierarchyStorage, nameStorage);
                for (auto it : range)
                {
                    if (!it.first.IsEndPoint())
                    {
                        list.AddSelectedNode(it.second.GetPath());
                    }
                }
            }

            void SceneGraphSelector::UnselectAll(const Containers::SceneGraph& graph, DataTypes::ISceneNodeSelectionList& list)
            {
                list.ClearSelectedNodes();
                list.ClearUnselectedNodes();
                
                auto hierarchyStorage = graph.GetHierarchyStorage();
                auto nameStorage = graph.GetNameStorage();                
                auto range = Containers::Views::MakePairView(hierarchyStorage, nameStorage);
                for (auto it : range)
                {
                    if (!it.first.IsEndPoint())
                    {
                        list.RemoveSelectedNode(it.second.GetPath());
                    }
                }
            }

            void SceneGraphSelector::UpdateNodeSelection(const Containers::SceneGraph& graph, DataTypes::ISceneNodeSelectionList& list)
            {
                AZStd::unordered_set<AZStd::string> selectedNodesSet;
                AZStd::unordered_set<AZStd::string> unselectedNodesSet;
                CopySelectionToSet(selectedNodesSet, unselectedNodesSet, list);
                CorrectRootNode(graph, selectedNodesSet, unselectedNodesSet);

                list.ClearSelectedNodes();
                list.ClearUnselectedNodes();

                auto nameStorage = graph.GetNameStorage();
                auto contentStorage = graph.GetContentStorage();                
                auto keyValueView = Containers::Views::MakePairView(nameStorage, contentStorage);
                auto nodeIterator = graph.ConvertToHierarchyIterator(graph.GetRoot());
                auto view = Containers::Views::MakeSceneGraphDownwardsView<Containers::Views::BreadthFirst>(graph, nodeIterator, keyValueView.cbegin(), true);
                if (view.begin() == view.end())
                {
                    return;
                }
                for (auto it = ++view.begin(); it != view.end(); ++it)
                {
                    Containers::SceneGraph::NodeIndex index = graph.ConvertToNodeIndex(it.GetHierarchyIterator());
                    AZStd::string currentNodeName = it->first.GetPath();
                    // Check if item is already registered and if so add it to the appropriate list.
                    if (unselectedNodesSet.contains(currentNodeName))
                    {
                        list.RemoveSelectedNode(AZStd::move(currentNodeName));
                        continue;
                    }
                    if (selectedNodesSet.contains(currentNodeName))
                    {
                        list.AddSelectedNode(AZStd::move(currentNodeName));
                        continue;
                    }

                    // If not already registered, look at the parent and determine what container it's in
                    //      and add the unregistered node to that list.
                    Containers::SceneGraph::NodeIndex parentIndex = graph.GetNodeParent(index);
                    if (parentIndex.IsValid())
                    {
                        AZStd::string parentNodeName = graph.GetNodeName(parentIndex).GetPath();
                        if (unselectedNodesSet.contains(parentNodeName))
                        {
                            unselectedNodesSet.insert(currentNodeName);
                            list.RemoveSelectedNode(AZStd::move(currentNodeName));
                        }
                        else
                        {
                            selectedNodesSet.insert(currentNodeName);
                            list.AddSelectedNode(AZStd::move(currentNodeName));
                        }
                    }
                }
            }

            void SceneGraphSelector::UpdateTargetNodes(const Containers::SceneGraph& graph, DataTypes::ISceneNodeSelectionList& list, const AZStd::unordered_set<AZStd::string>& targetNodes,
                NodeFilterFunction nodeFilter)
            {
                list.ClearSelectedNodes();
                list.ClearUnselectedNodes();
                auto nameStorage = graph.GetNameStorage();
                auto contentStorage = graph.GetContentStorage();                
                auto view = Containers::Views::MakePairView(nameStorage, contentStorage);
                if (view.begin() == view.end())
                {
                    return;
                }
                for (auto it = ++view.begin(); it != view.end(); ++it)
                {
                    Containers::SceneGraph::NodeIndex index = graph.ConvertToNodeIndex(it.GetSecondIterator());
                    AZStd::string currentNodeName = it->first.GetPath();
                    if (nodeFilter(graph, index))
                    {
                        if (targetNodes.contains(currentNodeName))
                        {
                            list.AddSelectedNode(currentNodeName);
                        }
                        else
                        {
                            list.RemoveSelectedNode(currentNodeName);
                        }
                    }
                }
            }

            void SceneGraphSelector::CopySelectionToSet(
                AZStd::unordered_set<AZStd::string>& selected, AZStd::unordered_set<AZStd::string>& unselected,
                const DataTypes::ISceneNodeSelectionList& list)
            {
                list.EnumerateSelectedNodes([&selected](const AZStd::string& name)
                    {
                        selected.insert(name);
                        return true;
                    });

                list.EnumerateUnselectedNodes([&unselected](const AZStd::string& name)
                    {
                        unselected.insert(name);
                        return true;
                    });
            }

            void SceneGraphSelector::CorrectRootNode(const Containers::SceneGraph& graph,
                AZStd::unordered_set<AZStd::string>& selected, AZStd::unordered_set<AZStd::string>& unselected)
            {
                // If both of the unselected and selected node lists are empty or don't exist, deselect all the nodes
                // in the graph by deselecting the root node.
                // 
                // If only the unselected node list is empty or doesn't exist, deselect the root node (which will deselect
                // all the nodes in the graph) by default and then reselect nodes based on the selected node list.
                // 
                // Otherwise select the root node (which will select all the nodes in the graph) by default and then
                // remove selected nodes based on the deselected list.
                bool selectRootNode = !unselected.empty();
                AZStd::string rootNodeName = graph.GetNodeName(graph.GetRoot()).GetPath();
                auto& nodeSetToAdd = selectRootNode ? selected : unselected;
                auto& nodeSetToRemove = selectRootNode ? unselected : selected;

                if (nodeSetToAdd.find(rootNodeName) == nodeSetToAdd.end())
                {
                    nodeSetToAdd.insert(rootNodeName);
                }
                auto root = nodeSetToRemove.find(rootNodeName);
                if (root != nodeSetToRemove.end())
                {
                    nodeSetToRemove.erase(root);
                }
            }
        }
    }
} // namespace AZ

