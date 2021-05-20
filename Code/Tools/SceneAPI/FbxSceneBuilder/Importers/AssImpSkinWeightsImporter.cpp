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
#include <AzCore/std/string/conversions.h>
#include <AzToolsFramework/Debug/TraceContext.h>
#include <SceneAPI/FbxSceneBuilder/Importers/AssImpSkinWeightsImporter.h>
#include <SceneAPI/FbxSceneBuilder/Importers/ImporterUtilities.h>
#include <SceneAPI/FbxSceneBuilder/Importers/Utilities/AssImpMeshImporterUtilities.h>
#include <SceneAPI/FbxSceneBuilder/Importers/Utilities/RenamedNodesMap.h>
#include <SceneAPI/SceneCore/Events/ImportEventContext.h>
#include <SceneAPI/SceneData/GraphData/MeshData.h>
#include <SceneAPI/SceneData/GraphData/SkinWeightData.h>
#include <SceneAPI/SDKWrapper/AssImpNodeWrapper.h>
#include <SceneAPI/SDKWrapper/AssImpSceneWrapper.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace FbxSceneBuilder
        {
            const AZStd::string AssImpSkinWeightsImporter::s_skinWeightName = "SkinWeight_";

            AssImpSkinWeightsImporter::AssImpSkinWeightsImporter()
            {
                BindToCall(&AssImpSkinWeightsImporter::ImportSkinWeights);
                BindToCall(&AssImpSkinWeightsImporter::SetupNamedBoneLinks);
            }

            void AssImpSkinWeightsImporter::Reflect(ReflectContext* context)
            {
                SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context);
                if (serializeContext)
                {
                    serializeContext->Class<AssImpSkinWeightsImporter, SceneCore::LoadingComponent>()->Version(3); // LYN-2576
                }
            }

            Events::ProcessingResult AssImpSkinWeightsImporter::ImportSkinWeights(AssImpSceneNodeAppendedContext& context)
            {
                AZ_TraceContext("Importer", "Skin Weights");

                const aiNode* currentNode = context.m_sourceNode.GetAssImpNode();
                const aiScene* scene = context.m_sourceScene.GetAssImpScene();

                if(currentNode->mNumMeshes <= 0)
                {
                    return Events::ProcessingResult::Ignored;
                }

                Events::ProcessingResultCombiner combinedSkinWeightsResult;

                // Don't create this until a bone with weights is encountered
                Containers::SceneGraph::NodeIndex weightsIndexForMesh;
                AZStd::string skinWeightName;
                AZStd::shared_ptr<SceneData::GraphData::SkinWeightData> skinWeightData;

                const uint64_t totalVertices = GetVertexCountForAllMeshesOnNode(*currentNode, *scene);

                int vertexCount = 0;
                for(unsigned nodeMeshIndex = 0; nodeMeshIndex < currentNode->mNumMeshes; ++nodeMeshIndex)
                {
                    int sceneMeshIndex = currentNode->mMeshes[nodeMeshIndex];
                    const aiMesh* mesh = scene->mMeshes[sceneMeshIndex];

                    for(unsigned b = 0; b < mesh->mNumBones; ++b)
                    {
                        const aiBone* bone = mesh->mBones[b];

                        if(bone->mNumWeights <= 0)
                        {
                            continue;
                        }

                        if (!weightsIndexForMesh.IsValid())
                        {
                            skinWeightName = s_skinWeightName;
                            RenamedNodesMap::SanitizeNodeName(skinWeightName, context.m_scene.GetGraph(), context.m_currentGraphPosition);

                            weightsIndexForMesh =
                                context.m_scene.GetGraph().AddChild(context.m_currentGraphPosition, skinWeightName.c_str());

                            AZ_Error("SkinWeightsImporter", weightsIndexForMesh.IsValid(), "Failed to create SceneGraph node for attribute.");
                            if (!weightsIndexForMesh.IsValid())
                            {
                                combinedSkinWeightsResult += Events::ProcessingResult::Failure;
                                continue;
                            }
                            skinWeightData = AZStd::make_shared<SceneData::GraphData::SkinWeightData>();
                        }
                        Pending pending;
                        pending.m_bone = bone;
                        pending.m_numVertices = totalVertices;
                        pending.m_skinWeightData = skinWeightData;
                        pending.m_vertOffset = vertexCount;
                        m_pendingSkinWeights.push_back(pending);
                    }
                    vertexCount += mesh->mNumVertices;
                }

                Events::ProcessingResult skinWeightsResult;
                AssImpSceneAttributeDataPopulatedContext dataPopulated(context, skinWeightData, weightsIndexForMesh, skinWeightName);
                skinWeightsResult = Events::Process(dataPopulated);

                if (skinWeightsResult != Events::ProcessingResult::Failure)
                {
                    skinWeightsResult = AddAttributeDataNodeWithContexts(dataPopulated);
                }

                combinedSkinWeightsResult += skinWeightsResult;

                return combinedSkinWeightsResult.GetResult();
            }

            Events::ProcessingResult AssImpSkinWeightsImporter::SetupNamedBoneLinks(AssImpFinalizeSceneContext& /*context*/)
            {
                AZ_TraceContext("Importer", "Skin Weights");

                for (auto& it : m_pendingSkinWeights)
                {
                    it.m_skinWeightData->ResizeContainerSpace(it.m_numVertices);

                    AZStd::string boneName = it.m_bone->mName.C_Str();
                    int boneId = it.m_skinWeightData->GetBoneId(boneName);

                    for(unsigned weight = 0; weight < it.m_bone->mNumWeights; ++weight)
                    {
                        DataTypes::ISkinWeightData::Link link;
                        link.boneId = boneId;
                        link.weight = it.m_bone->mWeights[weight].mWeight;

                        it.m_skinWeightData->AddAndSortLink(it.m_bone->mWeights[weight].mVertexId + it.m_vertOffset, link);
                    }
                }
                const auto result = m_pendingSkinWeights.empty() ? Events::ProcessingResult::Ignored : Events::ProcessingResult::Success;
                m_pendingSkinWeights.clear();
                return result;
            }
        } // namespace FbxSceneBuilder
    } // namespace SceneAPI
} // namespace AZ
