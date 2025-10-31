/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
                AZ_CRC_CE("LODMesh1"),
                AZ_CRC_CE("LODMesh2"),
                AZ_CRC_CE("LODMesh3"),
                AZ_CRC_CE("LODMesh4"),
                AZ_CRC_CE("LODMesh5")
            };

            size_t LODSelector::SelectLODBones(
                const SceneContainers::Scene& scene, SceneDataTypes::ISceneNodeSelectionList& selection, size_t lodRuleIndex,
                bool selectBaseBones)
            {
                const SceneContainers::SceneGraph& graph = scene.GetGraph();
                const auto contentStorage = graph.GetContentStorage();
                const auto nameStorage = graph.GetNameStorage();
                const auto keyValueView = SceneContainers::Views::MakePairView(nameStorage, contentStorage);
                AZ::u32 lodBoneCount = 0;

                AZ::SceneAPI::Utilities::SceneGraphSelector::UnselectAll(graph, selection);

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

                if (lodBoneCount == 0 && selectBaseBones)
                {
                    // If the group does not contain any bones, add all the bones in base LOD by default.
                    SceneContainers::SceneGraph::NodeIndex lodZeroGroupNode = FindLODRootIndex(scene, 0);

                    const auto downwardView = AZ::SceneAPI::Containers::Views::MakeSceneGraphDownwardsView<AZ::SceneAPI::Containers::Views::DepthFirst>(graph, lodZeroGroupNode, keyValueView.begin(), true);
                    const auto filteredView = SceneContainers::Views::MakeFilterView(downwardView, SceneContainers::DerivedTypeFilter<SceneDataTypes::IBoneData>());
                    for (auto it = filteredView.begin(); it != filteredView.end(); ++it)
                    {
                        selection.AddSelectedNode(it->first.GetPath());
                        lodBoneCount++;
                    }
                }

                return lodBoneCount;
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
