/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/std/string/conversions.h>
#include <AzToolsFramework/Debug/TraceContext.h>
#include <SceneAPI/SceneBuilder/Importers/AssImpBlendShapeImporter.h>
#include <SceneAPI/SceneBuilder/Importers/Utilities/AssImpMeshImporterUtilities.h>
#include <SceneAPI/SceneBuilder/Importers/Utilities/RenamedNodesMap.h>
#include <SceneAPI/SceneBuilder/Importers/ImporterUtilities.h>
#include <SceneAPI/SceneBuilder/SceneSystem.h>
#include <SceneAPI/SDKWrapper/AssImpNodeWrapper.h>
#include <SceneAPI/SDKWrapper/AssImpSceneWrapper.h>
#include <SceneAPI/SDKWrapper/AssImpTypeConverter.h>
#include <SceneAPI/SceneCore/Utilities/Reporting.h>
#include <SceneAPI/SceneData/GraphData/MeshData.h>
#include <SceneAPI/SceneData/GraphData/SkinMeshData.h>
#include <SceneAPI/SceneData/GraphData/BlendShapeData.h>
#include <assimp/scene.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace SceneBuilder
        {
            AssImpBlendShapeImporter::AssImpBlendShapeImporter()
            {
                BindToCall(&AssImpBlendShapeImporter::ImportBlendShapes);
            }

            void AssImpBlendShapeImporter::Reflect(ReflectContext* context)
            {
                SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context);
                if (serializeContext)
                {
                    serializeContext->Class<AssImpBlendShapeImporter, SceneCore::LoadingComponent>()->Version(3); // LYN-2576
                }
            }

            Events::ProcessingResult AssImpBlendShapeImporter::ImportBlendShapes(AssImpSceneNodeAppendedContext& context)
            {
                AZ_TraceContext("Importer", "Blend Shapes");
                int numMesh = context.m_sourceNode.GetAssImpNode()->mNumMeshes;
                bool animMeshExists = false;
                for (int idx = 0; idx < numMesh; idx++)
                {
                    int meshId = context.m_sourceNode.GetAssImpNode()->mMeshes[idx];
                    aiMesh* aiMesh = context.m_sourceScene.GetAssImpScene()->mMeshes[meshId];
                    if (aiMesh->mNumAnimMeshes)
                    {
                        animMeshExists = true;
                        break;
                    }
                }

                if (!animMeshExists)
                {
                    return Events::ProcessingResult::Ignored;
                }

                GetMeshDataFromParentResult meshDataResult(GetMeshDataFromParent(context));
                if (!meshDataResult.IsSuccess())
                {
                    return meshDataResult.GetError();
                }

                Events::ProcessingResultCombiner combinedBlendShapeResult;

                // 1. Loop through meshes & anims
                //      Create storage: Anim to meshes
                // 2. Loop through anims & meshes
                //      Create an anim mesh for each anim, with meshes re-combined.
                // AssImp separates meshes that have multiple materials.
                // This code re-combines them to match previous FBX SDK behavior,
                // so they can be separated by engine code instead.
                AZStd::map<AZStd::string_view, AZStd::vector<AZStd::pair<int, int>>> animToMeshToAnimMeshIndices;
                for (int nodeMeshIdx = 0; nodeMeshIdx < numMesh; nodeMeshIdx++)
                {
                    int sceneMeshIdx = context.m_sourceNode.GetAssImpNode()->mMeshes[nodeMeshIdx];
                    const aiMesh* aiMesh = context.m_sourceScene.GetAssImpScene()->mMeshes[sceneMeshIdx];
                    for (int animIdx = 0; animIdx < aiMesh->mNumAnimMeshes; animIdx++)
                    {
                        aiAnimMesh* aiAnimMesh = aiMesh->mAnimMeshes[animIdx];
                        animToMeshToAnimMeshIndices[aiAnimMesh->mName.C_Str()].emplace_back(nodeMeshIdx, animIdx);
                    }
                }

                for (const auto& animToMeshIndex : animToMeshToAnimMeshIndices)
                {
                    AZStd::shared_ptr<SceneData::GraphData::BlendShapeData> blendShapeData =
                        AZStd::make_shared<SceneData::GraphData::BlendShapeData>();

                    // Some DCC tools, like Maya, include a full path separated by '.' in the node names.
                    // For example, "cone_skin_blendShapeNode.cone_squash"
                    // Downstream processing doesn't want anything but the last part of that node name,
                    // so find the last '.' and remove anything before it.
                    AZStd::string nodeName(animToMeshIndex.first);
                    size_t dotIndex = nodeName.rfind('.');
                    if (dotIndex != AZStd::string::npos)
                    {
                        nodeName.erase(0, dotIndex + 1);
                    }
                    int vertexOffset = 0;
                    RenamedNodesMap::SanitizeNodeName(nodeName, context.m_scene.GetGraph(), context.m_currentGraphPosition, "BlendShape");
                    AZ_TraceContext("Blend shape name", nodeName);
                    for (const auto& meshIndex : animToMeshIndex.second)
                    {
                        int sceneMeshIdx = context.m_sourceNode.GetAssImpNode()->mMeshes[meshIndex.first];
                        const aiMesh* aiMesh = context.m_sourceScene.GetAssImpScene()->mMeshes[sceneMeshIdx];
                        const aiAnimMesh* aiAnimMesh = aiMesh->mAnimMeshes[meshIndex.second];

                        AZStd::bitset<SceneData::GraphData::BlendShapeData::MaxNumUVSets> uvSetUsedFlags;
                        for (AZ::u8 uvSetIndex = 0; uvSetIndex < SceneData::GraphData::BlendShapeData::MaxNumUVSets; ++uvSetIndex)
                        {
                            uvSetUsedFlags.set(uvSetIndex, aiAnimMesh->HasTextureCoords(uvSetIndex));
                        }
                        AZStd::bitset<SceneData::GraphData::BlendShapeData::MaxNumColorSets> colorSetUsedFlags;
                        for (AZ::u8 colorSetIndex = 0; colorSetIndex < SceneData::GraphData::BlendShapeData::MaxNumColorSets;
                             ++colorSetIndex)
                        {
                            colorSetUsedFlags.set(colorSetIndex, aiAnimMesh->HasVertexColors(colorSetIndex));
                        }
                        blendShapeData->ReserveData(
                            aiAnimMesh->mNumVertices, aiAnimMesh->HasTangentsAndBitangents(), uvSetUsedFlags, colorSetUsedFlags);

                        for (int vertIdx = 0; vertIdx < aiAnimMesh->mNumVertices; ++vertIdx)
                        {
                            AZ::Vector3 vertex(AssImpSDKWrapper::AssImpTypeConverter::ToVector3(aiAnimMesh->mVertices[vertIdx]));
                   
                            context.m_sourceSceneSystem.SwapVec3ForUpAxis(vertex);
                            context.m_sourceSceneSystem.ConvertUnit(vertex);

                            blendShapeData->AddPosition(vertex);
                            blendShapeData->SetVertexIndexToControlPointIndexMap(vertIdx + vertexOffset, vertIdx + vertexOffset);

                            // Add normals
                            if (aiAnimMesh->HasNormals())
                            {
                                AZ::Vector3 normal(AssImpSDKWrapper::AssImpTypeConverter::ToVector3(aiAnimMesh->mNormals[vertIdx]));
                                context.m_sourceSceneSystem.SwapVec3ForUpAxis(normal);
                                normal.NormalizeSafe();
                                blendShapeData->AddNormal(normal);
                            }

                            // Add tangents and bitangents
                            if (aiAnimMesh->HasTangentsAndBitangents())
                            {
                                // Vector4's constructor that takes in a vector3 sets w to 1.0f automatically.
                                const AZ::Vector4 tangent(AssImpSDKWrapper::AssImpTypeConverter::ToVector3(aiAnimMesh->mTangents[vertIdx]));
                                const AZ::Vector3 bitangent = AssImpSDKWrapper::AssImpTypeConverter::ToVector3(aiAnimMesh->mBitangents[vertIdx]);
                                blendShapeData->AddTangentAndBitangent(tangent, bitangent);
                            }

                            // Add UVs
                            for (AZ::u8 uvSetIdx = 0; uvSetIdx < SceneData::GraphData::BlendShapeData::MaxNumUVSets; ++uvSetIdx)
                            {
                                if (aiAnimMesh->HasTextureCoords(uvSetIdx))
                                {
                                    const AZ::Vector2 vertexUV(
                                        aiAnimMesh->mTextureCoords[uvSetIdx][vertIdx].x,
                                        // The engine's V coordinate is reverse of how it's stored in assImp.
                                        1.0f - aiAnimMesh->mTextureCoords[uvSetIdx][vertIdx].y);
                                    blendShapeData->AddUV(vertexUV, uvSetIdx);
                                }
                            }

                            // Add colors
                            for (AZ::u8 colorSetIdx = 0; colorSetIdx < SceneData::GraphData::BlendShapeData::MaxNumColorSets; ++colorSetIdx)
                            {
                                if (aiAnimMesh->HasVertexColors(colorSetIdx))
                                {
                                    SceneAPI::DataTypes::Color color =
                                        AssImpSDKWrapper::AssImpTypeConverter::ToColor(aiAnimMesh->mColors[colorSetIdx][vertIdx]);
                                    blendShapeData->AddColor(color, colorSetIdx);
                                }
                            }
                        }

                        // aiAnimMesh just has a list of positions for vertices. The face indices are on the original mesh.
                        for (int faceIdx = 0; faceIdx < aiMesh->mNumFaces; ++faceIdx)
                        {
                            aiFace face = aiMesh->mFaces[faceIdx];
                            DataTypes::IBlendShapeData::Face blendFace;
                            if (face.mNumIndices != 3)
                            {
                                // AssImp should have triangulated everything, so if this happens then someone has
                                // probably changed AssImp's import settings. The engine only supports triangles.
                                AZ_Error(
                                    Utilities::ErrorWindow, false,
                                    "Mesh for node %s has a face with %d vertices, only 3 vertices are supported per face.",
                                    nodeName.c_str(),
                                    face.mNumIndices);
                                continue;
                            }
                            for (int idx = 0; idx < face.mNumIndices; ++idx)
                            {
                                blendFace.vertexIndex[idx] = face.mIndices[idx] + vertexOffset;
                            }

                            blendShapeData->AddFace(blendFace);
                        }
                        vertexOffset += aiMesh->mNumVertices;


                    }


                    // Report problem if no vertex or face converted to MeshData
                    if (blendShapeData->GetVertexCount() <= 0 || blendShapeData->GetFaceCount() <= 0)
                    {
                        AZ_Error(Utilities::ErrorWindow, false, "Missing geometry data in blendshape node %s.", nodeName.c_str());
                        return Events::ProcessingResult::Failure;
                    }

                    Containers::SceneGraph::NodeIndex newIndex =
                        context.m_scene.GetGraph().AddChild(context.m_currentGraphPosition, nodeName.c_str());

                    Events::ProcessingResult blendShapeResult;
                    AssImpSceneAttributeDataPopulatedContext dataPopulated(context, blendShapeData, newIndex, nodeName);
                    blendShapeResult = Events::Process(dataPopulated);

                    if (blendShapeResult != Events::ProcessingResult::Failure)
                    {
                        blendShapeResult = AddAttributeDataNodeWithContexts(dataPopulated);
                    }
                    combinedBlendShapeResult += blendShapeResult;
                }

                return combinedBlendShapeResult.GetResult();
            }
        } // namespace SceneBuilder
    } // namespace SceneAPI
} // namespace AZ
