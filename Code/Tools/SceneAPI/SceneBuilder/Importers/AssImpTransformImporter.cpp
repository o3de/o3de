/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <SceneAPI/SceneBuilder/Importers/AssImpTransformImporter.h>

#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzToolsFramework/Debug/TraceContext.h>
#include <SceneAPI/SceneBuilder/SceneSystem.h>
#include <SceneAPI/SceneBuilder/Importers/ImporterUtilities.h>
#include <SceneAPI/SceneBuilder/Importers/Utilities/RenamedNodesMap.h>
#include <SceneAPI/SceneCore/Utilities/Reporting.h>
#include <SceneAPI/SceneData/GraphData/TransformData.h>
#include <SceneAPI/SDKWrapper/AssImpTypeConverter.h>
#include <SceneAPI/SDKWrapper/AssImpNodeWrapper.h>
#include <SceneAPI/SDKWrapper/AssImpSceneWrapper.h>
#include <assimp/scene.h>
#include <SceneAPI/SceneBuilder/Importers/AssImpImporterUtilities.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace SceneBuilder
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
            
            void GetAllBones(const aiScene* scene, AZStd::unordered_multimap<AZStd::string, const aiBone*>& boneLookup)
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

                for (unsigned meshIndex = 0; meshIndex < scene->mNumMeshes; ++meshIndex)
                {
                    const aiMesh* mesh = scene->mMeshes[meshIndex];

                    for (unsigned boneIndex = 0; boneIndex < mesh->mNumBones; ++boneIndex)
                    {
                        const aiBone* bone = mesh->mBones[boneIndex];

                        if (nodesWithNoMesh.contains(bone->mName.C_Str()))
                        {
                            boneLookup.emplace(bone->mName.C_Str(), bone);
                        }
                    }
                }
            }

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
                
                DataTypes::MatrixType localTransform;

                if (isBone)
                {
                    AZStd::vector<DataTypes::MatrixType> offsets, inverseOffsets;
                    auto iteratingNode = currentNode;

                    while (iteratingNode && boneLookup.count(iteratingNode->mName.C_Str()))
                    {
                        AZStd::string name = iteratingNode->mName.C_Str();

                        auto range = boneLookup.equal_range(name);

                        if (range.first != range.second)
                        {
                            // There can be multiple offsetMatrices for a given bone, we're only interested in grabbing the first one
                            auto boneFirstOffsetMatrix = range.first->second->mOffsetMatrix;
                            auto azMat = AssImpSDKWrapper::AssImpTypeConverter::ToTransform(boneFirstOffsetMatrix);
                            offsets.push_back(azMat);
                            inverseOffsets.push_back(azMat.GetInverseFull());
                        }

                        iteratingNode = iteratingNode->mParent;
                    }

                    if (inverseOffsets.size() == 1)
                    {
                        // If this is the root bone, just use the inverseOffset, otherwise the equation below just results in the identity matrix
                        localTransform = inverseOffsets[0];
                    }
                    else
                    {
                        localTransform = offsets.at(1) // parent bone offset
                            * inverseOffsets.at(inverseOffsets.size() - 1) // Inverse of root bone offset
                            * offsets.at(offsets.size() - 1) // Root bone offset
                            * inverseOffsets.at(0); // Inverse of current node offset
                    }
                }
                else
                {
                    localTransform = AssImpSDKWrapper::AssImpTypeConverter::ToTransform(GetConcatenatedLocalTransform(currentNode));
                }

                // Don't bother adding a node with the identity matrix
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
        } // namespace SceneBuilder
    } // namespace SceneAPI
} // namespace AZ
