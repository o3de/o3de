/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/StringFunc/StringFunc.h>
#include <AzToolsFramework/Debug/TraceContext.h>
#include <SceneAPI/SceneBuilder/SceneSystem.h>
#include <SceneAPI/SceneBuilder/Importers/AssImpBoneImporter.h>
#include <SceneAPI/SceneBuilder/Importers/AssImpImporterUtilities.h>
#include <SceneAPI/SceneBuilder/Importers/ImporterUtilities.h>
#include <SceneAPI/SceneCore/Containers/Scene.h>
#include <SceneAPI/SceneData/GraphData/BoneData.h>
#include <SceneAPI/SceneData/GraphData/RootBoneData.h>
#include <SceneAPI/SDKWrapper/AssImpNodeWrapper.h>
#include <SceneAPI/SDKWrapper/AssImpSceneWrapper.h>
#include <SceneAPI/SDKWrapper/AssImpTypeConverter.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace SceneBuilder
        {
            AssImpBoneImporter::AssImpBoneImporter()
            {
                BindToCall(&AssImpBoneImporter::ImportBone);
            }

            void AssImpBoneImporter::Reflect(ReflectContext* context)
            {
                SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context);
                if (serializeContext)
                {
                    serializeContext->Class<AssImpBoneImporter, SceneCore::LoadingComponent>()->Version(2);
                }
            }

            aiMatrix4x4 CalculateWorldTransform(const aiNode* currentNode)
            {
                aiMatrix4x4 transform = {};
                const aiNode* iteratingNode = currentNode;
                
                while (iteratingNode)
                {
                    transform = iteratingNode->mTransformation * transform;
                    iteratingNode = iteratingNode->mParent;
                }

                return transform;
            }

            Events::ProcessingResult AssImpBoneImporter::ImportBone(AssImpNodeEncounteredContext& context)
            {
                AZ_TraceContext("Importer", "Bone");

                const aiNode* currentNode = context.m_sourceNode.GetAssImpNode();
                const aiScene* scene = context.m_sourceScene.GetAssImpScene();

                if (IsPivotNode(currentNode->mName))
                {
                    return Events::ProcessingResult::Ignored;
                }

                AZStd::unordered_multimap<AZStd::string, const aiBone*> boneByNameMap;
                FindAllBones(scene, boneByNameMap);

                bool isBone = FindFirstBoneByNodeName(currentNode, boneByNameMap);
                if (!isBone)
                {
                    // In case the bone is not listed in any of the meshes in the scene, populate a set of animated bone names from
                    // the animations. This set of names will help varify if the current node is a bone.
                    AZStd::unordered_set<AZStd::string> animatedNodesMap;
                    for(unsigned animIndex = 0; animIndex < scene->mNumAnimations; ++animIndex)
                    {
                        aiAnimation* animation = scene->mAnimations[animIndex];
                        for (unsigned channelIndex = 0; channelIndex < animation->mNumChannels; ++channelIndex)
                        {
                            aiNodeAnim* nodeAnim = animation->mChannels[channelIndex];
                            animatedNodesMap.emplace(nodeAnim->mNodeName.C_Str());
                        }
                    }

                    // In case any of the children, or children of children is a bone, make sure to not skip this node.
                    // Don't do this for the scene root itself, else wise all mesh nodes will be exported as bones and pollute the skeleton.
                    if (currentNode != scene->mRootNode &&
                        RecursiveHasChildBone(currentNode, boneByNameMap, animatedNodesMap))
                    {
                        isBone = true;
                    }
                }

                if (!isBone)
                {
                    return Events::ProcessingResult::Ignored;
                }

                // If the current scene node (our eventual parent) contains bone data, we are not a root bone
                AZStd::shared_ptr<SceneData::GraphData::BoneData> createdBoneData;

                if (NodeHasAncestorOfType(
                        context.m_scene.GetGraph(), context.m_currentGraphPosition, DataTypes::IBoneData::TYPEINFO_Uuid()))
                {
                    createdBoneData = AZStd::make_shared<SceneData::GraphData::BoneData>();
                }
                else
                {
                    createdBoneData = AZStd::make_shared<SceneData::GraphData::RootBoneData>();
                }
                
                aiMatrix4x4 transform = CalculateWorldTransform(currentNode);
                
                SceneAPI::DataTypes::MatrixType globalTransform = AssImpSDKWrapper::AssImpTypeConverter::ToTransform(transform);

                context.m_sourceSceneSystem.SwapTransformForUpAxis(globalTransform);
                context.m_sourceSceneSystem.ConvertBoneUnit(globalTransform);

                createdBoneData->SetWorldTransform(globalTransform);

                context.m_createdData.push_back(AZStd::move(createdBoneData));

                return Events::ProcessingResult::Success;
            }
        } // namespace SceneBuilder
    } // namespace SceneAPI
} // namespace AZ
