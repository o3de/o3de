/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/containers/bitset.h>
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
                    // Revision 3: Fixed an issue where jack.fbx was failing to process
                    // Revision 4: Handle duplicate blend shape animations
                    serializeContext->Class<AssImpBlendShapeImporter, SceneCore::LoadingComponent>()->Version(4);
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
                // Can't de-dupe nodes in the first loop because we can't generate names until we create nodes later.
                // Because meshes are split on material at this point and need to be recombined, we can be in a position where
                // There is a legit duped anim mesh that needs to be combined based on the outer non-anim mesh,
                // or this is a duplicately named anim mesh that needs to be de-duped. There is also the case where both are true,
                // it's a duplicate name and the non-anim mesh has to be deduped.

                // Helper struct to track an anim mesh and its associated mesh.
                struct AnimMeshAndSceneMeshIndex
                {
                    AnimMeshAndSceneMeshIndex(const aiAnimMesh* aiAnimMesh, const aiMesh* aiMesh)
                        : m_aiAnimMesh(aiAnimMesh)
                        , m_aiMesh(aiMesh)
                    {
                    }
                    const aiAnimMesh* m_aiAnimMesh = nullptr;
                    const aiMesh* m_aiMesh = nullptr;
                };

                // Helper struct to track all anim meshes at an index for all scene meshes.
                struct AnimMeshAndSceneMeshes
                {
                    AZStd::vector<AnimMeshAndSceneMeshIndex> m_animMeshAndSceneMeshIndex;
                };

                // Map the animation index to the list of anim meshes at that index, and the mesh associated with those anim meshes.
                AZStd::map<int, AnimMeshAndSceneMeshes> animMeshIndexToSceneMeshes;
                for (int nodeMeshIdx = 0; nodeMeshIdx < numMesh; nodeMeshIdx++)
                {
                    int sceneMeshIdx = context.m_sourceNode.GetAssImpNode()->mMeshes[nodeMeshIdx];
                    const aiMesh* aiMesh = context.m_sourceScene.GetAssImpScene()->mMeshes[sceneMeshIdx];
                    for (unsigned int animIdx = 0; animIdx < aiMesh->mNumAnimMeshes; animIdx++)
                    {
                        aiAnimMesh* aiAnimMesh = aiMesh->mAnimMeshes[animIdx];

                        // This code executes if:
                        //  A mesh in the FBX file had multiple materials and blend shapes.
                        //  This means that AssImp splits that mesh to one material per mesh.
                        //  AssImp creates a set of anim meshes for each mesh based on that split.
                        // This verifies that those anim mesh arrays are in the same order across all split meshes, if it fails
                        // it means this logic needs to be updated, but it also catches that here earlier in an obvious way,
                        // instead of failing later in a harder to track way.
                        if (animMeshIndexToSceneMeshes.contains(animIdx))
                        {
                            const AnimMeshAndSceneMeshIndex& firstExistingAnim(
                                animMeshIndexToSceneMeshes[animIdx].m_animMeshAndSceneMeshIndex[0]);
                            if (strcmp(
                                    firstExistingAnim.m_aiAnimMesh->mName.C_Str(),
                                    aiAnimMesh->mName.C_Str()) != 0)
                            {
                                AZ_Error(
                                    Utilities::ErrorWindow, false,
                                    "Meshes %s and %s on node %s have mismatched animations %s and %s at index %d. This can be resolved by "
                                    "either manually separating meshes by material in the source scene file, or by updating this logic to "
                                    "handle out of order animation indices.",
                                    firstExistingAnim.m_aiMesh->mName.C_Str(),
                                    aiMesh->mName.C_Str(),
                                    context.m_sourceNode.GetName(),
                                    firstExistingAnim.m_aiAnimMesh->mName.C_Str(),
                                    aiAnimMesh->mName.C_Str(), animIdx);
                                return Events::ProcessingResult::Failure;
                            }
                        }

                        animMeshIndexToSceneMeshes[animIdx].m_animMeshAndSceneMeshIndex.emplace_back(
                            AnimMeshAndSceneMeshIndex(aiAnimMesh, aiMesh));
                    }
                }

                for (const auto& animMeshToSceneMeshes : animMeshIndexToSceneMeshes)
                {
                    AZStd::shared_ptr<SceneData::GraphData::BlendShapeData> blendShapeData =
                        AZStd::make_shared<SceneData::GraphData::BlendShapeData>();

                    if (animMeshToSceneMeshes.second.m_animMeshAndSceneMeshIndex.size() == 0)
                    {
                        AZ_Error(Utilities::ErrorWindow, false, "Blend shape animations were expected but missing on node %s.",
                            context.m_sourceNode.GetName());
                        return Events::ProcessingResult::Failure;
                    }
                    // Some DCC tools, like Maya, include a full path separated by '.' in the node names.
                    // For example, "cone_skin_blendShapeNode.cone_squash"
                    // Downstream processing doesn't want anything but the last part of that node name,
                    // so find the last '.' and remove anything before it.
                    AZStd::string nodeName(animMeshToSceneMeshes.second.m_animMeshAndSceneMeshIndex[0].m_aiAnimMesh->mName.C_Str());
                    size_t dotIndex = nodeName.rfind('.');
                    if (dotIndex != AZStd::string::npos)
                    {
                        nodeName.erase(0, dotIndex + 1);
                    }
                    int vertexOffset = 0;
                    RenamedNodesMap::SanitizeNodeName(nodeName, context.m_scene.GetGraph(), context.m_currentGraphPosition, "BlendShape");

                    for (const auto& animMeshAndSceneIndex : animMeshToSceneMeshes.second.m_animMeshAndSceneMeshIndex)
                    {
                        const aiAnimMesh* aiAnimMesh = animMeshAndSceneIndex.m_aiAnimMesh;
                        const aiMesh* aiMesh = animMeshAndSceneIndex.m_aiMesh;

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

                        for (unsigned int vertIdx = 0; vertIdx < aiAnimMesh->mNumVertices; ++vertIdx)
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
                        for (unsigned int faceIdx = 0; faceIdx < aiMesh->mNumFaces; ++faceIdx)
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

                            for (unsigned int idx = 0; idx < face.mNumIndices; ++idx)
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
