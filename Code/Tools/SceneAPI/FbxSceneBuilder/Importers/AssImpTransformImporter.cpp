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
#include <SceneAPI/FbxSceneBuilder/Importers/AssImpTransformImporter.h>

#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzToolsFramework/Debug/TraceContext.h>
#include <SceneAPI/FbxSceneBuilder/FbxSceneSystem.h>
#include <SceneAPI/FbxSceneBuilder/Importers/ImporterUtilities.h>
#include <SceneAPI/FbxSceneBuilder/Importers/Utilities/RenamedNodesMap.h>
#include <SceneAPI/SceneCore/Utilities/Reporting.h>
#include <SceneAPI/SceneData/GraphData/TransformData.h>
#include <SceneAPI/SDKWrapper/AssImpTypeConverter.h>
#include <SceneAPI/SDKWrapper/AssImpNodeWrapper.h>
#include <SceneAPI/SDKWrapper/AssImpSceneWrapper.h>
#include <assimp/scene.h>
#include <SceneAPI/FbxSceneBuilder/Importers/AssImpImporterUtilities.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace FbxSceneBuilder
        {
            const char* AssImpTransformImporter::s_transformNodeName = "transform";

            AssImpTransformImporter::AssImpTransformImporter()
            {
                BindToCall(&AssImpTransformImporter::ImportTransform);
            }

            void AssImpTransformImporter::Reflect(ReflectContext* context)
            {
                SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context);
                if (serializeContext)
                {
                    serializeContext->Class<AssImpTransformImporter, SceneCore::LoadingComponent>()->Version(1);
                }
            }
            
            void GetAllBones(
                const aiScene* scene, AZStd::unordered_multimap<AZStd::string, const aiBone*>& boneLookup)
            {
                for (unsigned meshIndex = 0; meshIndex < scene->mNumMeshes; ++meshIndex)
                {
                    const aiMesh* mesh = scene->mMeshes[meshIndex];

                    for (unsigned boneIndex = 0; boneIndex < mesh->mNumBones; ++boneIndex)
                    {
                        const aiBone* bone = mesh->mBones[boneIndex];

                        boneLookup.emplace(bone->mName.C_Str(), bone);
                    }
                }
            }
#pragma optimize("", off)
            Events::ProcessingResult AssImpTransformImporter::ImportTransform(AssImpSceneNodeAppendedContext& context)
            {
                AZ_TraceContext("Importer", "transform");
                const aiNode* currentNode = context.m_sourceNode.GetAssImpNode();
                const aiScene* scene = context.m_sourceScene.GetAssImpScene();

                if (currentNode == scene->mRootNode || IsPivotNode(currentNode->mName))
                {
                    return Events::ProcessingResult::Ignored;
                }

                AZStd::unordered_multimap<AZStd::string, const aiBone*> boneLookup;
                GetAllBones(scene, boneLookup);

                auto boneIterator = boneLookup.find(currentNode->mName.C_Str());
                const bool isBone = boneIterator != boneLookup.end();

                if (currentNode->mName.C_Str() == AZStd::string("spine_C0_0_jnt"))
                {
                    //__debugbreak();
                }

                aiMatrix4x4 combinedTransform;
                DataTypes::MatrixType finalMat;

                if (isBone)
                {
                    auto parentNode = currentNode->mParent;

                    aiMatrix4x4 offsetMatrix = boneIterator->second->mOffsetMatrix;
                    aiMatrix4x4 parentOffset {};
                    aiMatrix4x4 parentTransform{};

                    if (parentNode)
                    {
                        parentTransform = parentNode->mTransformation;
                    }

                    auto azOffset = AssImpSDKWrapper::AssImpTypeConverter::ToTransform(offsetMatrix);
                    decltype(azOffset) azParentOffset{};
                    AZStd::vector<DataTypes::MatrixType> transforms, offsets, inverseTransforms, inverseOffsets;

                    auto addTransform = [&](AZStd::string, aiMatrix4x4 mat)
                    {
                        auto azMat = AssImpSDKWrapper::AssImpTypeConverter::ToTransform(mat);
                        transforms.push_back(azMat);
                        inverseTransforms.push_back(azMat.GetInverseFull());
                    };
                    auto addOffset = [&]([[maybe_unused]] auto name, aiMatrix4x4 mat)
                    {
                        auto azMat = AssImpSDKWrapper::AssImpTypeConverter::ToTransform(mat);
                        offsets.push_back(azMat);
                        inverseOffsets.push_back(azMat.GetInverseFull());
                    };

                    auto curNode = currentNode;

                    while (curNode && boneLookup.count(curNode->mName.C_Str()))
                    {
                        AZStd::string name = curNode->mName.C_Str();

                        auto range = boneLookup.equal_range(name);

                        for (auto it = range.first; it != range.second; ++it)
                        {
                            addOffset(name, it->second->mOffsetMatrix);
                            break;
                        }

                        //addOffset(name, boneLookup.at(name)->mOffsetMatrix);
                        addTransform(name, curNode->mTransformation);

                        curNode = curNode->mParent;
                    }

                    // Go up the tree to find the root bone.  It's parent will be the scene mRootNode
                    while (parentNode && parentNode->mParent && parentNode->mParent != scene->mRootNode)
                    {
                        parentNode = parentNode->mParent;
                    }

                    auto parentBoneIterator = boneLookup.find(parentNode->mName.C_Str());
                    aiMatrix4x4 rootOffsetMatrix{};

                    if (parentBoneIterator != boneLookup.end())
                    {
                        rootOffsetMatrix = parentBoneIterator->second->mOffsetMatrix;
                    }

                    auto inverseOffset = offsetMatrix;
                    inverseOffset.Inverse();

                    auto parentTransformInverse = parentTransform;
                    parentTransformInverse.Inverse();

                    //auto azInverse = azOffset.GetInverseFull();
                    //auto azCombined = azParentOffset * azInverse;
                    //combinedTransform = parentOffset * inverseOffset;

                    //parentTransform ^-1 * parentParent * azOffsetInverse
                    //combinedTransform = parentTransformInverse * rootOffsetMatrix * inverseOffset;

                    //azNodeLocal = offsets[1] * inverseOffsets[3] * (offsets[3] * inverseOffsets[0])
                    finalMat =
                        offsets.at(AZ::GetMin(offsets.size()-1, (decltype(offsets.size()))1)) * inverseOffsets.at(inverseOffsets.size() - 1) * offsets.at(offsets.size() - 1) * inverseOffsets.at(0);
                }
                else
                {
                    combinedTransform = GetConcatenatedLocalTransform(currentNode);
                }

                //DataTypes::MatrixType localTransform = AssImpSDKWrapper::AssImpTypeConverter::ToTransform(combinedTransform);
                DataTypes::MatrixType localTransform = finalMat;

                if (localTransform == DataTypes::MatrixType::Identity())
                {
                    return Events::ProcessingResult::Ignored;
                }

                context.m_sourceSceneSystem.SwapTransformForUpAxis(localTransform);
                context.m_sourceSceneSystem.ConvertUnit(localTransform);

                AZStd::shared_ptr<SceneData::GraphData::TransformData> transformData =
                    AZStd::make_shared<SceneData::GraphData::TransformData>(localTransform);
                AZ_Error(SceneAPI::Utilities::ErrorWindow, transformData, "Failed to allocate transform data.");
                if (!transformData)
                {
                    return Events::ProcessingResult::Failure;
                }

                // If it is non-endpoint data populated node, add a transform attribute
                if (context.m_scene.GetGraph().HasNodeContent(context.m_currentGraphPosition))
                {
                    if (!context.m_scene.GetGraph().IsNodeEndPoint(context.m_currentGraphPosition))
                    {
                        AZStd::string nodeName = s_transformNodeName;
                        RenamedNodesMap::SanitizeNodeName(nodeName, context.m_scene.GetGraph(), context.m_currentGraphPosition);
                        AZ_TraceContext("Transform node name", nodeName);

                        Containers::SceneGraph::NodeIndex newIndex =
                            context.m_scene.GetGraph().AddChild(context.m_currentGraphPosition, nodeName.c_str());

                        AZ_Error(SceneAPI::Utilities::ErrorWindow, newIndex.IsValid(), "Failed to create SceneGraph node for attribute.");
                        if (!newIndex.IsValid())
                        {
                            return Events::ProcessingResult::Failure;
                        }

                        Events::ProcessingResult transformAttributeResult;
                        AssImpSceneAttributeDataPopulatedContext dataPopulated(context, transformData, newIndex, nodeName);
                        transformAttributeResult = Events::Process(dataPopulated);

                        if (transformAttributeResult != Events::ProcessingResult::Failure)
                        {
                            transformAttributeResult = AddAttributeDataNodeWithContexts(dataPopulated);
                        }

                        return transformAttributeResult;
                    }
                }
                else
                {
                    bool addedData = context.m_scene.GetGraph().SetContent(context.m_currentGraphPosition, transformData);

                    AZ_Error(SceneAPI::Utilities::ErrorWindow, addedData, "Failed to add node data");
                    return addedData ? Events::ProcessingResult::Success : Events::ProcessingResult::Failure;
                }

                return Events::ProcessingResult::Ignored;
            }
        } // namespace FbxSceneBuilder
    } // namespace SceneAPI
} // namespace AZ
