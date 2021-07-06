/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
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

            void MakeBoneMap(const aiScene* scene, AZStd::unordered_map<AZStd::string, const aiBone*>& boneLookup)
            {
                AZStd::queue<const aiNode*> queue;
                AZStd::unordered_set<AZStd::string> nodesWithNoMesh;

                queue.push(scene->mRootNode);

                while (!queue.empty())
                {
                    const aiNode* currentNode = queue.front();
                    queue.pop();

                    if (currentNode->mNumMeshes == 0)
                    {
                        nodesWithNoMesh.emplace(currentNode->mName.C_Str());
                    }

                    for (int childIndex = 0; childIndex < currentNode->mNumChildren; ++childIndex)
                    {
                        queue.push(currentNode->mChildren[childIndex]);
                    }
                }

                for (unsigned int meshIndex = 0; meshIndex < scene->mNumMeshes; ++meshIndex)
                {
                    const aiMesh* mesh = scene->mMeshes[meshIndex];

                    for (unsigned int boneIndex = 0; boneIndex < mesh->mNumBones; ++boneIndex)
                    {
                        const aiBone* bone = mesh->mBones[boneIndex];

                        if (nodesWithNoMesh.contains(bone->mName.C_Str()))
                        {
                            boneLookup.emplace(bone->mName.C_Str(), bone);
                        }
                    }
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

                bool isBone = false;
                
                {
                    AZStd::unordered_map<AZStd::string, const aiBone*> boneLookup;
                    MakeBoneMap(scene, boneLookup);

                    isBone = boneLookup.contains(currentNode->mName.C_Str());

                    // If we have an animation, the bones will be listed in there
                    if (!isBone)
                    {
                        for(unsigned animIndex = 0; animIndex < scene->mNumAnimations; ++animIndex)
                        {
                            aiAnimation* animation = scene->mAnimations[animIndex];

                            for (unsigned channelIndex = 0; channelIndex < animation->mNumChannels; ++channelIndex)
                            {
                                aiNodeAnim* nodeAnim = animation->mChannels[channelIndex];

                                if (nodeAnim->mNodeName == currentNode->mName)
                                {
                                    isBone = true;
                                    break;
                                }
                            }

                            if (isBone)
                            {
                                break;
                            }
                        }
                    }
                }

                if(!isBone)
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
