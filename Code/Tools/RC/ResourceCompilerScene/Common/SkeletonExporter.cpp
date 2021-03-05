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

#include <Cry_Geo.h> // Needed for CGFContent.h
#include <CryCrc32.h>
#include <CGFContent.h>
#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzToolsFramework/Debug/TraceContext.h>
#include <RC/ResourceCompilerScene/Common/CommonExportContexts.h>
#include <RC/ResourceCompilerScene/Common/SkeletonExporter.h>
#include <RC/ResourceCompilerScene/Common/AssetExportUtilities.h>
#include <SceneAPI/SceneCore/Containers/Scene.h>
#include <SceneAPI/SceneCore/Containers/Views/PairIterator.h>
#include <SceneAPI/SceneCore/Containers/Views/SceneGraphChildIterator.h>
#include <SceneAPI/SceneCore/Containers/Views/SceneGraphDownwardsIterator.h>
#include <SceneAPI/SceneCore/Containers/Views/SceneGraphUpwardsIterator.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IBoneData.h>
#include <SceneAPI/SceneCore/Utilities/Reporting.h>

namespace AZ
{
    namespace RC
    {
        namespace SceneEvents = AZ::SceneAPI::Events;
        namespace SceneUtil = AZ::SceneAPI::Utilities;
        namespace SceneContainers = AZ::SceneAPI::Containers;
        namespace SceneDataTypes = AZ::SceneAPI::DataTypes;

        SkeletonExporter::SkeletonExporter()
        {
            BindToCall(&SkeletonExporter::ResolveRootBoneFromBone);
            BindToCall(&SkeletonExporter::BuildBoneMap);
            BindToCall(&SkeletonExporter::AddBonesToSkinningInfo);
            BindToCall(&SkeletonExporter::ProcessSkeleton);
        }

        void SkeletonExporter::Reflect(ReflectContext* context)
        {
            SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context);
            if (serializeContext)
            {
                serializeContext->Class<SkeletonExporter, SceneAPI::SceneCore::RCExportingComponent>()->Version(1);
            }
        }

        SceneAPI::Events::ProcessingResult SkeletonExporter::ResolveRootBoneFromBone(ResolveRootBoneFromBoneContext& context)
        {
            const SceneContainers::SceneGraph& graph = context.m_scene.GetGraph();

            const AZStd::string& boneName = context.m_boneName;
            AZ_TraceContext("Bone name", boneName.c_str());

            auto contentStorage = graph.GetContentStorage();
            auto nameStorage = graph.GetNameStorage();
            auto nameContentView = SceneContainers::Views::MakePairView(nameStorage, contentStorage);

            // If the boneName is a full graph path, use that particular bone.
            SceneContainers::SceneGraph::NodeIndex boneIndex = graph.Find(boneName);
            if (!boneIndex.IsValid())
            {
                // If the the bone index is only the name, start looking for the first bone found with that name. The bone closest to
                //      the root of the graph is preferred.
                auto graphDownwardsView = SceneContainers::Views::MakeSceneGraphDownwardsView<SceneContainers::Views::BreadthFirst>(
                    graph, graph.GetRoot(), nameContentView.begin(), true);

                auto it = AZStd::find_if(graphDownwardsView.begin(), graphDownwardsView.end(),
                    [&boneName](const decltype(*nameContentView.begin())& entry) -> bool
                    {
                        if (!entry.second || !entry.second->RTTI_IsTypeOf(SceneDataTypes::IBoneData::TYPEINFO_Uuid()))
                        {
                            return false;
                        }
                        return azstrnicmp(entry.first.GetName(), boneName.c_str(), entry.first.GetNameLength()) == 0;
                    });

                if (it == graphDownwardsView.end())
                {
                    AZ_TracePrintf(SceneUtil::ErrorWindow, "Unable to find the skeleton root bone for bone");
                    return  SceneEvents::ProcessingResult::Failure;
                }
                boneIndex = graph.ConvertToNodeIndex(it.GetHierarchyIterator());
            }

            AZ_Assert(boneIndex.IsValid(), "A bone was found but the index for it's node is still invalid.");

            // Now that the bone has been found, search upwards to find the root bone of the skeleton the bone belongs to.
            auto graphUpwardsView = SceneContainers::Views::MakeSceneGraphUpwardsView(graph, boneIndex, nameContentView.begin(), true);
            const char* rootBoneName = nullptr;
            for (const auto& it : graphUpwardsView)
            {
                if (it.second && it.second->RTTI_IsTypeOf(SceneDataTypes::IBoneData::TYPEINFO_Uuid()))
                {
                    rootBoneName = it.first.GetPath();
                }
                else
                {
                    break;
                }
            }
            AZ_Assert(rootBoneName, "The name of the first bone should have been found.");
            context.m_rootBoneName = rootBoneName;

            return SceneEvents::ProcessingResult::Success;
        }
        
        SceneEvents::ProcessingResult SkeletonExporter::BuildBoneMap(BuildBoneMapContext& context)
        {
            return BuildBoneMap(context.m_boneNameIdMap, context.m_scene.GetGraph(), context.m_rootBoneName) ?
                SceneEvents::ProcessingResult::Success : SceneEvents::ProcessingResult::Failure;
        }

        SceneEvents::ProcessingResult SkeletonExporter::AddBonesToSkinningInfo(AddBonesToSkinningInfoContext& context)
        {
            return AddBonesToSkinningInfo(context.m_skinningInfo, context.m_scene.GetGraph(), context.m_rootBoneName) ?
                SceneEvents::ProcessingResult::Success : SceneEvents::ProcessingResult::Failure;
        }

        SceneEvents::ProcessingResult SkeletonExporter::ProcessSkeleton(SkeletonExportContext& context)
        {
            if (context.m_phase != Phase::Filling)
            {
                return SceneEvents::ProcessingResult::Ignored;
            }

            return AddBonesToSkinningInfo(context.m_skinningInfo, context.m_scene.GetGraph(), context.m_rootBoneName) ?
                SceneEvents::ProcessingResult::Success : SceneEvents::ProcessingResult::Failure;
        }

        bool SkeletonExporter::AddBonesToSkinningInfo(CSkinningInfo& skinningInfo, const SceneContainers::SceneGraph& graph, const AZStd::string& rootBoneName) const
        {
            if (rootBoneName.empty())
            {
                AZ_TracePrintf(SceneUtil::ErrorWindow, "Root bone name cannot be empty.");
                return false;
            }
            AZ_TraceContext("Root bone", rootBoneName);

            AZStd::unordered_map<AZStd::string, int> boneNameIdMap;
            if (!BuildBoneMap(boneNameIdMap, graph, rootBoneName))
            {
                // Error already reported by BuildBoneMap.
                return false;
            }

            SceneContainers::SceneGraph::NodeIndex nodeIndex = graph.Find(rootBoneName);
            if (!nodeIndex.IsValid())
            {
                AZ_TracePrintf(SceneUtil::ErrorWindow, "Unable to find root bone in scene graph.");
                return false;
            }

            auto contentStorage = graph.GetContentStorage();
            auto nameStorage = graph.GetNameStorage();
            auto pairView = SceneContainers::Views::MakePairView(contentStorage, nameStorage);
            auto view = SceneContainers::Views::MakeSceneGraphDownwardsView<SceneContainers::Views::DepthFirst>(graph, nodeIndex, pairView.begin(), true);
            for (auto it = view.begin(); it != view.end(); ++it)
            {
                if (it->first && it->first->RTTI_IsTypeOf(SceneDataTypes::IBoneData::TYPEINFO_Uuid()))
                {
                    AZ_TraceContext("Bone", it->second.GetPath());
                    AZStd::shared_ptr<const SceneDataTypes::IBoneData> boneData = azrtti_cast<const SceneDataTypes::IBoneData*>(it->first);
                    AZ_Assert(boneData, "Graph object couldn't be converted to bone data even though it matched the type.");

                    // Example fbx file exported from Maya will have default unit in centimeter.
                    // E.g. A global transformation in meter unit:
                    // 0.01 0    0    | 0.05
                    // 0    0.01 0    | 0
                    // 0    0    0.01 | 0
                    // while a global transform in centimeter unit:
                    // 1    0    0    | 5
                    // 0    1    0    | 0
                    // 0    0    1    | 0
                    // We need to remove scale from transform matrix (so the root bone's rotation matrix is identity) to satisfy the
                    // input requirement of AssetWriter
                    SceneAPI::DataTypes::MatrixType transformNoScale = boneData->GetWorldTransform();
                    AZ_Assert(transformNoScale.RetrieveScale().GetLength() >= Constants::FloatEpsilon, "Transform on bone %s has 0 scale", it->second.GetName());
                    transformNoScale.ExtractScale();
                    AddBoneDescriptor(skinningInfo, it->second.GetName(), it->second.GetNameLength(), transformNoScale);
                    if (!AddBoneEntity(skinningInfo, graph, graph.ConvertToNodeIndex(it.GetHierarchyIterator()), boneNameIdMap,
                        it->second.GetName(), it->second.GetPath(), rootBoneName))
                    {
                        // Error already reported in AddBoneEntity.
                        return false;
                    }
                }
                else
                {
                    // End of bone chain or interruption in the bone chain. In both cases stop looking into this part of hierarchy further.
                    it.IgnoreNodeDescendants();
                }
            }

            return true;
        }

        bool SkeletonExporter::BuildBoneMap(AZStd::unordered_map<AZStd::string, int>& boneNameIdMap, const SceneContainers::SceneGraph& graph, const AZStd::string& rootBoneName) const
        {
            if (rootBoneName.empty())
            {
                AZ_TracePrintf(SceneUtil::ErrorWindow, "Root bone name cannot be empty.");
                return false;
            }

            AZ_TraceContext("Root bone", rootBoneName);
            SceneContainers::SceneGraph::NodeIndex nodeIndex = graph.Find(rootBoneName);
            if (!nodeIndex.IsValid())
            {
                AZ_TracePrintf(SceneUtil::ErrorWindow, "Unable to find root bone in scene graph.");
                return false;
            }

            auto contentStorage = graph.GetContentStorage();
            auto nameStorage = graph.GetNameStorage();
            auto pairView = SceneContainers::Views::MakePairView(contentStorage, nameStorage);
            auto view = SceneContainers::Views::MakeSceneGraphDownwardsView<SceneContainers::Views::DepthFirst>(graph, nodeIndex, pairView.begin(), true);
            int index = 0;
            for (auto it = view.begin(); it != view.end(); ++it)
            {
                if (it->first && it->first->RTTI_IsTypeOf(SceneDataTypes::IBoneData::TYPEINFO_Uuid()))
                {
                    boneNameIdMap[it->second.GetName()] = index;
                    index++;
                }
                else
                {
                    // End of bone chain or interruption in the bone chain. In both cases stop looking into this part of hierarchy further.
                    it.IgnoreNodeDescendants();
                }
            }

            return true;
        }

        void SkeletonExporter::AddBoneDescriptor(CSkinningInfo& skinningInfo, const char* boneName, size_t boneNameLength,
            const SceneAPI::DataTypes::MatrixType& worldTransform) const
        {
            CryBoneDescData boneDesc;
            auto convertedTransform{ AssetExportUtilities::ConvertToCryMatrix34(worldTransform) };

            AZ_Assert(convertedTransform.IsValid(), "Bone %s has invalid world transform", boneName);

            // Invalid transform will set off an assertion in the equals operator below - the check above is so
            // in case of that assertion AP will give a hint of what to look at in the logs
            boneDesc.m_DefaultB2W = convertedTransform;
            boneDesc.m_DefaultW2B = boneDesc.m_DefaultB2W.GetInverted();

            SetBoneName(boneName, boneNameLength, boneDesc);
            boneDesc.m_nControllerID = CCrc32::ComputeLowercase(boneName);

            skinningInfo.m_arrBonesDesc.push_back(boneDesc);
        }

        bool SkeletonExporter::AddBoneEntity(CSkinningInfo& skinningInfo, const SceneContainers::SceneGraph& graph, const SceneContainers::SceneGraph::NodeIndex index,
            const AZStd::unordered_map<AZStd::string, int>& boneNameIdMap, const char* boneName, const char* bonePath, const AZStd::string& rootBoneName) const
        {
            BONE_ENTITY boneEntity;
            memset(&boneEntity, 0, sizeof(boneEntity));

            auto boneIndex = boneNameIdMap.find(boneName);
            if (boneIndex != boneNameIdMap.end())
            {
                boneEntity.BoneID = boneIndex->second;
                boneEntity.ParentID = -1;
                if (rootBoneName.compare(bonePath) != 0)
                {
                    SceneContainers::SceneGraph::NodeIndex parentIndex = graph.GetNodeParent(index);
                    auto parentIt = boneNameIdMap.find(graph.GetNodeName(parentIndex).GetName());
                    if (parentIt != boneNameIdMap.end())
                    {
                        boneEntity.ParentID = parentIt->second;
                    }
                    else
                    {
                        AZ_TracePrintf(SceneUtil::ErrorWindow, "Bone is not the root bone but doesn't have another bone as it's parent.");
                        return false;
                    }
                }
            }
            boneEntity.ControllerID = CCrc32::ComputeLowercase(boneName);
            boneEntity.phys.nPhysGeom = -1;

            auto childBones = SceneContainers::Views::MakeSceneGraphChildView<SceneContainers::Views::AcceptNodesOnly>(
                graph, index, graph.GetNameStorage().begin(), true);
            boneEntity.nChildren = aznumeric_caster(AZStd::count_if(childBones.begin(), childBones.end(),
                [&boneNameIdMap](const SceneContainers::SceneGraph::Name& name)
                {
                    return boneNameIdMap.find(name.GetName()) != boneNameIdMap.end();
                }));

            skinningInfo.m_arrBoneEntities.push_back(boneEntity);

            return true;
        }

        void SkeletonExporter::SetBoneName(const char* name, size_t nameLength, CryBoneDescData& boneDesc) const
        {
            static const size_t nodeNameCount = sizeof(boneDesc.m_arrBoneName) / sizeof(boneDesc.m_arrBoneName[0]);
            size_t offset = (nameLength < nodeNameCount) ? 0 : (nameLength - nodeNameCount + 1);
            azstrcpy(boneDesc.m_arrBoneName, nodeNameCount, name + offset);
        }
    } // namespace RC
} // namespace AZ
