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

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/StringFunc/StringFunc.h>
#include <AzToolsFramework/Debug/TraceContext.h>
#include <SceneAPI/FbxSceneBuilder/FbxSceneSystem.h>
#include <SceneAPI/FbxSceneBuilder/Importers/AssImpBoneImporter.h>
#include <SceneAPI/FbxSceneBuilder/Importers/AssImpImporterUtilities.h>
#include <SceneAPI/FbxSceneBuilder/Importers/FbxImporterUtilities.h>
#include <SceneAPI/FbxSDKWrapper/FbxNodeWrapper.h>
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
        namespace FbxSceneBuilder
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
                    serializeContext->Class<AssImpBoneImporter, SceneCore::LoadingComponent>()->Version(1);
                }
            }

            void EnumBonesInNode(
                const aiScene* scene, const aiNode* node, AZStd::unordered_map<AZStd::string, aiNode*>& mainBoneList,
                AZStd::unordered_map<AZStd::string, aiBone*>& boneLookup)
            {
                /* From AssImp Documentation
                    a) Create a map or a similar container to store which nodes are necessary for the skeleton. Pre-initialise it for all nodes with a "no".
                    b) For each bone in the mesh:
                    b1) Find the corresponding node in the scene's hierarchy by comparing their names.
                    b2) Mark this node as "yes" in the necessityMap.
                    b3) Mark all of its parents the same way until you 1) find the mesh's node or 2) the parent of the mesh's node.
                    c) Recursively iterate over the node hierarchy
                    c1) If the node is marked as necessary, copy it into the skeleton and check its children
                    c2) If the node is marked as not necessary, skip it and do not iterate over its children. 
                 */

                for (unsigned meshIndex = 0; meshIndex < node->mNumMeshes; ++meshIndex)
                {
                    aiMesh* mesh = scene->mMeshes[node->mMeshes[meshIndex]];

                    for (unsigned boneIndex = 0; boneIndex < mesh->mNumBones; ++boneIndex)
                    {
                        aiBone* bone = mesh->mBones[boneIndex];

                        aiNode* boneNode = scene->mRootNode->FindNode(bone->mName);
                        aiNode* boneParent = boneNode->mParent;

                        mainBoneList[bone->mName.C_Str()] = boneNode;
                        boneLookup[bone->mName.C_Str()] = bone;

                        while (boneParent && boneParent != node && boneParent != node->mParent && boneParent != scene->mRootNode)
                        {
                            mainBoneList[boneParent->mName.C_Str()] = boneParent;

                            boneParent = boneParent->mParent;
                        }
                    }
                }
            }

            void EnumChildren(
                const aiScene* scene, const aiNode* node, AZStd::unordered_map<AZStd::string, aiNode*>& mainBoneList,
                AZStd::unordered_map<AZStd::string, aiBone*>& boneLookup)
            {
                EnumBonesInNode(scene, node, mainBoneList, boneLookup);

                for (unsigned childIndex = 0; childIndex < node->mNumChildren; ++childIndex)
                {
                    const aiNode* child = node->mChildren[childIndex];
                    
                    EnumChildren(scene, child, mainBoneList, boneLookup);
                }
            }

            Events::ProcessingResult AssImpBoneImporter::ImportBone(AssImpNodeEncounteredContext& context)
            {
                AZ_TraceContext("Importer", "Bone");

                aiNode* currentNode = context.m_sourceNode.GetAssImpNode();
                const aiScene* scene = context.m_sourceScene.GetAssImpScene();

                if (IsPivotNode(currentNode->mName))
                {
                    return Events::ProcessingResult::Ignored;
                }

                bool isBone = false;

                if (NodeParentIsOfType(context.m_scene.GetGraph(), context.m_currentGraphPosition, DataTypes::IBoneData::TYPEINFO_Uuid()))
                {
                    isBone = true;
                }
                else
                {
                    AZStd::unordered_map<AZStd::string, aiNode*> mainBoneList;
                    AZStd::unordered_map<AZStd::string, aiBone*> boneLookup;
                    EnumChildren(scene, scene->mRootNode, mainBoneList, boneLookup);

                    if (mainBoneList.find(currentNode->mName.C_Str()) != mainBoneList.end())
                    {
                        isBone = true;
                    }

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

                aiMatrix4x4 transform = currentNode->mTransformation;
                aiNode* parent = currentNode->mParent;
                
                while (parent)
                {
                    transform = parent->mTransformation * transform;
                    parent = parent->mParent;
                }
                
                SceneAPI::DataTypes::MatrixType globalTransform = AssImpSDKWrapper::AssImpTypeConverter::ToTransform(transform);

                context.m_sourceSceneSystem.SwapTransformForUpAxis(globalTransform);
                context.m_sourceSceneSystem.ConvertBoneUnit(globalTransform);

                createdBoneData->SetWorldTransform(globalTransform);

                context.m_createdData.push_back(AZStd::move(createdBoneData));

                return Events::ProcessingResult::Success;
            }
        } // namespace FbxSceneBuilder
    } // namespace SceneAPI
} // namespace AZ
