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

#include <SceneAPI/SceneCore/Utilities/SceneGraphSelector.h>
#include <SceneAPI/SceneCore/Containers/Scene.h>
#include <SceneAPI/SceneCore/Containers/Utilities/Filters.h>
#include <SceneAPI/SceneCore/Containers/Views/PairIterator.h>
#include <SceneAPI/SceneCore/Containers/Views/FilterIterator.h>
#include <SceneAPI/SceneCore/Containers/Views/SceneGraphDownwardsIterator.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IBoneData.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IMeshData.h>
#include <SceneAPI/SceneCore/Events/GraphMetaInfoBus.h>
#include <SceneAPIExt/Utilities/LODSelector.h>



namespace EMotionFX
{
    namespace Pipeline
    {
        namespace Utilities
        {
            static AZStd::fixed_vector<AZ::Crc32, g_maxLods> s_lodVirtualTypeKeys =
            {
                AZ_CRC("LODMesh1", 0xcbea988c),
                AZ_CRC("LODMesh2", 0x52e3c936),
                AZ_CRC("LODMesh3", 0x25e4f9a0),
                AZ_CRC("LODMesh4", 0xbb806c03),
                AZ_CRC("LODMesh5", 0xcc875c95)
            };

            size_t LODSelector::SelectLODMeshes(const SceneContainers::Scene& scene, SceneDataTypes::ISceneNodeSelectionList& selection, size_t lodRuleIndex)
            {
                size_t lodMeshCount = 0;

                const SceneContainers::SceneGraph& graph = scene.GetGraph();

                // Find the lod group name, if the scene contains one.
                // There's two ways for user to create LOD for now. 
                // 1. Use soft naming (_lod1, _lod2...) as suffix on the meshes
                // 2. Use LOD group (LOD_1, LOD_2...) and put lod meshes in each group.
                AZStd::string lodGroupName = AZStd::string::format("LOD_%zu", lodRuleIndex + 1);

                // Loop through all the mesh data.
                auto contentStorage = graph.GetContentStorage();
                auto nameStorage = graph.GetNameStorage();
                auto keyValueView = SceneContainers::Views::MakePairView(nameStorage, contentStorage);
                auto filteredView = SceneContainers::Views::MakeFilterView(keyValueView, SceneContainers::DerivedTypeFilter<SceneDataTypes::IMeshData>());
                for (const auto& keyValue : filteredView)
                {
                    AZStd::set<AZ::Crc32> types;
                    SceneContainers::SceneGraph::Name name = keyValue.first;
                    SceneContainers::SceneGraph::NodeIndex index = graph.Find(name.GetPath());
                    EBUS_EVENT(SceneEvents::GraphMetaInfoBus, GetVirtualTypes, types, scene, index);

                    bool isLodMesh = false;
                    if (types.find(s_lodVirtualTypeKeys[lodRuleIndex]) != types.end() && types.find(SceneEvents::GraphMetaInfo::GetIgnoreVirtualType()) == types.end())
                    {
                        // If the node uses the lod soft naming, it is a lod mesh.
                        isLodMesh = true;
                    }
                    else
                    {
                        // If the node has a parent that matches the lod group name, it is a lod mesh.
                        SceneContainers::SceneGraph::NodeIndex curIdx = index;
                        while (graph.HasNodeParent(curIdx))
                        {
                            SceneContainers::SceneGraph::NodeIndex parent = graph.GetNodeParent(curIdx);
                            if (lodGroupName.compare(graph.GetNodeName(parent).GetName()) == 0)
                            {
                                isLodMesh = true;
                                break;
                            }
                            curIdx = parent;
                        }
                    }

                    if (isLodMesh)
                    {
                        // add lod mesh that belong to this lod level.
                        selection.AddSelectedNode(name.GetPath());
                        lodMeshCount++;
                    }
                }
                return lodMeshCount;
            }

            void LODSelector::SelectLODBones(const SceneContainers::Scene & scene, SceneDataTypes::ISceneNodeSelectionList & selection, size_t lodRuleIndex)
            {
                const SceneContainers::SceneGraph& graph = scene.GetGraph();
                const auto contentStorage = graph.GetContentStorage();
                const auto nameStorage = graph.GetNameStorage();
                const auto keyValueView = SceneContainers::Views::MakePairView(nameStorage, contentStorage);
                AZ::u32 lodBoneCount = 0;

                // Since LOD Rule doesn't include level 0, the lod level is the rule index plus 1.
                SceneContainers::SceneGraph::NodeIndex lodGroupNode = FindLODRootIndex(scene, lodRuleIndex + 1);

                if (lodGroupNode != graph.GetRoot())
                {
                    const auto downwardView = AZ::SceneAPI::Containers::Views::MakeSceneGraphDownwardsView<AZ::SceneAPI::Containers::Views::DepthFirst>(graph, lodGroupNode, keyValueView.begin(), true);
                    const auto filteredView = SceneContainers::Views::MakeFilterView(downwardView, SceneContainers::DerivedTypeFilter<SceneDataTypes::IBoneData>());

                    for (const auto& keyValue : filteredView)
                    {
                        selection.AddSelectedNode(keyValue.first.GetPath());
                        lodBoneCount++;
                    }
                }

                if (lodBoneCount == 0)
                {
                    // If the group does not contain any bones, add all the bones in base LOD by default.
                    SceneContainers::SceneGraph::NodeIndex lodZeroGroupNode = FindLODRootIndex(scene, 0);

                    const auto downwardView = AZ::SceneAPI::Containers::Views::MakeSceneGraphDownwardsView<AZ::SceneAPI::Containers::Views::DepthFirst>(graph, lodZeroGroupNode, keyValueView.begin(), true);
                    const auto filteredView = SceneContainers::Views::MakeFilterView(downwardView, SceneContainers::DerivedTypeFilter<SceneDataTypes::IBoneData>());
                    for (auto it = filteredView.begin(); it != filteredView.end(); ++it)
                    {
                        selection.AddSelectedNode(it->first.GetPath());
                    }
                }
            }

            size_t LODSelector::SelectLODNodes(const SceneContainers::Scene & scene, SceneDataTypes::ISceneNodeSelectionList & selection, size_t lodRuleIndex)
            {
                SceneUtilities::SceneGraphSelector::UnselectAll(scene.GetGraph(), selection);
                const size_t lodSize = SelectLODMeshes(scene, selection, lodRuleIndex);
                SelectLODBones(scene, selection, lodRuleIndex);
                return lodSize;
            }

            SceneContainers::SceneGraph::NodeIndex LODSelector::FindLODRootIndex(const SceneContainers::Scene& scene, size_t lodLevel)
            {
                const SceneContainers::SceneGraph& graph = scene.GetGraph();
                const auto nameStorage = graph.GetNameStorage();

                AZStd::string lodGroupName = AZStd::string::format("LOD_%zu", lodLevel);

                // First, find if there's any node in the graph matches the LOD Group name.
                for (auto it = nameStorage.begin(); it != nameStorage.end(); ++it)
                {
                    if (lodGroupName.compare(it->GetName()) == 0)
                    {
                        return graph.ConvertToNodeIndex(it);
                    }
                }
                return graph.GetRoot();
            }

            const char* LODSelector::FindLODRootPath(const SceneContainers::Scene & scene, size_t lodLevel)
            {
                SceneContainers::SceneGraph::NodeIndex LodRootIndex = FindLODRootIndex(scene, lodLevel);
                return scene.GetGraph().GetNodeName(LodRootIndex).GetPath();
            }
        }
    }
}
