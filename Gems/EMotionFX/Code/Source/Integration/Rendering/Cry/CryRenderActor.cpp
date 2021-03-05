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

#include <EMotionFX_precompiled.h>

#include <AzCore/base.h>
#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/Component/EntityId.h>
#include <AzCore/Jobs/LegacyJobExecutor.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/Debug/Profiler.h>
#include <AzCore/Math/Transform.h>

#include <LmbrCentral/Rendering/MeshComponentBus.h>

#include <Integration/Rendering/Cry/CryRenderBackendCommon.h>
#include <Integration/Rendering/Cry/CryRenderActor.h>
#include <Integration/Assets/ActorAsset.h>
#include <Integration/System/SystemCommon.h>

#include <I3DEngine.h>
#include <IRenderMesh.h>
#include <MathConversion.h>
#include <QTangent.h>

namespace EMotionFX
{
    namespace Integration
    {
        AZ_CLASS_ALLOCATOR_IMPL(CryRenderActor, EMotionFXAllocator, 0);

        CryRenderActor::CryRenderActor(ActorAsset* actorAsset)
            : RenderActor()
            , m_actorAsset(actorAsset)
        {
        }

        CryRenderActor::~CryRenderActor()
        {
        }

        bool CryRenderActor::Init()
        {
            // Populate CMeshes on the job thread, so we can at least build data streams asynchronously.
            if (!BuildLODMeshes())
            {
                return false;
            }

            // RenderMesh creation must be performed on the main thread, as required by the renderer.  It will get lazily created
            // in the Finalize() call, which will get called when each CryRenderActorInstance is created.

            return true;
        }

        bool CryRenderActor::BuildLODMeshes()
        {
            AZ_Assert(m_actorAsset, "Invalid asset data");

            EMotionFX::Actor* actor = m_actorAsset->GetActor();
            EMotionFX::Skeleton* skeleton = actor->GetSkeleton();
            const uint32 numNodes = actor->GetNumNodes();
            const uint32 numLODs = actor->GetNumLODLevels();
            const uint32 maxInfluences = AZ_ARRAY_SIZE(((SMeshBoneMapping_uint16*)nullptr)->boneIds);

            m_meshLODs.clear();
            m_meshLODs.reserve(numLODs);

            //
            // Process all LODs from the EMotionFX actor data.
            //
            for (uint32 lodIndex = 0; lodIndex < numLODs; ++lodIndex)
            {
                m_meshLODs.push_back(MeshLOD());
                MeshLOD& lod = m_meshLODs.back();

                // Get the amount of vertices and indices
                // Get the meshes to process
                bool hasUVs = false;
                bool hasUVs2 = false;
                bool hasTangents = false;
                bool hasBitangents = false;
                bool hasClothData = false;

                // Find the number of submeshes in the full actor.
                // This will be the number of primitives.
                size_t numPrimitives = 0;
                for (uint32 n = 0; n < numNodes; ++n)
                {
                    EMotionFX::Mesh* mesh = actor->GetMesh(lodIndex, n);
                    if (!mesh || mesh->GetIsCollisionMesh())
                    {
                        continue;
                    }

                    numPrimitives += mesh->GetNumSubMeshes();
                }

                lod.m_primitives.resize(numPrimitives);

                bool hasDynamicMeshes = false;

                size_t primitiveIndex = 0;
                for (uint32 n = 0; n < numNodes; ++n)
                {
                    EMotionFX::Mesh* mesh = actor->GetMesh(lodIndex, n);
                    if (!mesh || mesh->GetIsCollisionMesh())
                    {
                        continue;
                    }

                    const EMotionFX::Node* node = skeleton->GetNode(n);
                    const EMotionFX::Mesh::EMeshType meshType = mesh->ClassifyMeshType(lodIndex, actor, node->GetNodeIndex(), false, 4, 255);

                    hasUVs  = (mesh->FindOriginalVertexData(EMotionFX::Mesh::ATTRIB_UVCOORDS, 0) != nullptr);
                    hasUVs2 = (mesh->FindOriginalVertexData(EMotionFX::Mesh::ATTRIB_UVCOORDS, 1) != nullptr);
                    hasTangents = (mesh->FindOriginalVertexData(EMotionFX::Mesh::ATTRIB_TANGENTS) != nullptr);
                    hasClothData = (mesh->FindOriginalVertexData(EMotionFX::Mesh::ATTRIB_CLOTH_DATA) != nullptr);

                    const AZ::Vector3* sourcePositions = static_cast<AZ::Vector3*>(mesh->FindOriginalVertexData(EMotionFX::Mesh::ATTRIB_POSITIONS));
                    const AZ::Vector3* sourceNormals = static_cast<AZ::Vector3*>(mesh->FindOriginalVertexData(EMotionFX::Mesh::ATTRIB_NORMALS));
                    const AZ::u32* sourceOriginalVertex = static_cast<AZ::u32*>(mesh->FindOriginalVertexData(EMotionFX::Mesh::ATTRIB_ORGVTXNUMBERS));
                    const AZ::Vector4* sourceTangents = static_cast<AZ::Vector4*>(mesh->FindOriginalVertexData(EMotionFX::Mesh::ATTRIB_TANGENTS));
                    const AZ::Vector3* sourceBitangents = static_cast<AZ::Vector3*>(mesh->FindOriginalVertexData(EMotionFX::Mesh::ATTRIB_BITANGENTS));
                    const AZ::Vector2* sourceUVs = static_cast<AZ::Vector2*>(mesh->FindOriginalVertexData(EMotionFX::Mesh::ATTRIB_UVCOORDS, 0));
                    const AZ::Vector2* sourceUVs2 = static_cast<AZ::Vector2*>(mesh->FindOriginalVertexData(EMotionFX::Mesh::ATTRIB_UVCOORDS, 1));
                    const AZ::u32* sourceColors32 = static_cast<AZ::u32*>(mesh->FindOriginalVertexData(EMotionFX::Mesh::ATTRIB_COLORS32, 0));
                    const AZ::Vector4* sourceColors128 = static_cast<AZ::Vector4*>(mesh->FindOriginalVertexData(EMotionFX::Mesh::ATTRIB_COLORS128, 0));
                    EMotionFX::SkinningInfoVertexAttributeLayer* sourceSkinningInfo = static_cast<EMotionFX::SkinningInfoVertexAttributeLayer*>(mesh->FindSharedVertexAttributeLayer(EMotionFX::SkinningInfoVertexAttributeLayer::TYPE_ID));

                    // For each sub-mesh within each mesh, we want to create a separate sub-piece.
                    const uint32 numSubMeshes = mesh->GetNumSubMeshes();
                    for (uint32 subMeshIndex = 0; subMeshIndex < numSubMeshes; ++subMeshIndex)
                    {
                        EMotionFX::SubMesh* subMesh = mesh->GetSubMesh(subMeshIndex);

                        AZ_Assert(primitiveIndex < numPrimitives, "Unexpected primitive index");

                        Primitive& primitive = lod.m_primitives[primitiveIndex++];
                        primitive.m_mesh = new CMesh();
                        primitive.m_isDynamic = (meshType == EMotionFX::Mesh::MESHTYPE_CPU_DEFORMED);
                        primitive.m_useUniqueMesh = primitive.m_isDynamic || hasClothData;
                        primitive.m_subMesh = subMesh;

                        if (primitive.m_isDynamic)
                        {
                            hasDynamicMeshes = true;
                        }

                        // Destination initialization. We are going to put all meshes and submeshes for one lod
                        // into one destination mesh
                        primitive.m_vertexBoneMappings.resize(subMesh->GetNumVertices());
                        primitive.m_mesh->SetIndexCount(subMesh->GetNumIndices());
                        primitive.m_mesh->SetVertexCount(subMesh->GetNumVertices());

                        // Positions and normals are reallocated by the SetVertexCount
                        if (hasTangents)
                        {
                            primitive.m_mesh->ReallocStream(CMesh::TANGENTS, 0, subMesh->GetNumVertices());
                        }
                        if (hasUVs)
                        {
                            primitive.m_mesh->ReallocStream(CMesh::TEXCOORDS, 0, subMesh->GetNumVertices());
                        }
                        if (hasUVs2)
                        {
                            primitive.m_mesh->ReallocStream(CMesh::TEXCOORDS, 1, subMesh->GetNumVertices());
                        }

                        if (sourceColors128 || sourceColors32)
                        {
                            primitive.m_mesh->ReallocStream(CMesh::COLORS, 0, subMesh->GetNumVertices());
                        }

                        primitive.m_mesh->m_pBoneMapping = primitive.m_vertexBoneMappings.data();

                        // Pointers to the destination
                        vtx_idx* targetIndices = primitive.m_mesh->GetStreamPtr<vtx_idx>(CMesh::INDICES);
                        Vec3* destVertices = primitive.m_mesh->GetStreamPtr<Vec3>(CMesh::POSITIONS);
                        Vec3* destNormals = primitive.m_mesh->GetStreamPtr<Vec3>(CMesh::NORMALS);
                        SMeshTexCoord* destTexCoords = primitive.m_mesh->GetStreamPtr<SMeshTexCoord>(CMesh::TEXCOORDS);
                        SMeshTexCoord* destTexCoords2 = primitive.m_mesh->GetStreamPtr<SMeshTexCoord>(CMesh::TEXCOORDS, 1);
                        SMeshTangents* destTangents = primitive.m_mesh->GetStreamPtr<SMeshTangents>(CMesh::TANGENTS);
                        SMeshColor* destColors = primitive.m_mesh->GetStreamPtr<SMeshColor>(CMesh::COLORS);
                        SMeshBoneMapping_uint16* destBoneMapping = primitive.m_vertexBoneMappings.data();

                        primitive.m_mesh->m_subsets.push_back();
                        SMeshSubset& subset     = primitive.m_mesh->m_subsets.back();
                        subset.nFirstIndexId    = 0;
                        subset.nNumIndices      = subMesh->GetNumIndices();
                        subset.nFirstVertId     = 0;
                        subset.nNumVerts        = subMesh->GetNumVertices();
                        subset.nMatID           = subMesh->GetMaterial();
                        subset.fTexelDensity    = 0.0f;
                        subset.nPhysicalizeType = -1;

                        const uint32* subMeshIndices = subMesh->GetIndices();
                        const uint32 numSubMeshIndices = subMesh->GetNumIndices();
                        const uint32 subMeshStartVertex = subMesh->GetStartVertex();
                        for (uint32 index = 0; index < numSubMeshIndices; ++index)
                        {
                            targetIndices[index] = subMeshIndices[index] - subMeshStartVertex;
                        }

                        // Process vertices
                        const uint32 numSubMeshVertices = subMesh->GetNumVertices();
                        const AZ::Vector3* subMeshPositions = &sourcePositions[subMesh->GetStartVertex()];
                        for (uint32 vertexIndex = 0; vertexIndex < numSubMeshVertices; ++vertexIndex)
                        {
                            const AZ::Vector3& sourcePosition = subMeshPositions[vertexIndex];
                            destVertices->x = sourcePosition.GetX();
                            destVertices->y = sourcePosition.GetY();
                            destVertices->z = sourcePosition.GetZ();
                            ++destVertices;
                        }

                        // Process normals
                        const AZ::Vector3* subMeshNormals = &sourceNormals[subMesh->GetStartVertex()];
                        for (uint32 vertexIndex = 0; vertexIndex < numSubMeshVertices; ++vertexIndex)
                        {
                            const AZ::Vector3& sourceNormal = subMeshNormals[vertexIndex];
                            destNormals->x = sourceNormal.GetX();
                            destNormals->y = sourceNormal.GetY();
                            destNormals->z = sourceNormal.GetZ();
                            ++destNormals;
                        }

                        // Process UVs (TextCoords)
                        // First UV set.
                        if (hasUVs)
                        {
                            if (sourceUVs)
                            {
                                const AZ::Vector2* subMeshUVs = &sourceUVs[subMeshStartVertex];
                                for (uint32 vertexIndex = 0; vertexIndex < numSubMeshVertices; ++vertexIndex)
                                {
                                    const AZ::Vector2& uv = subMeshUVs[vertexIndex];
                                    *destTexCoords = SMeshTexCoord(uv.GetX(), uv.GetY());
                                    ++destTexCoords;
                                }
                            }
                            else
                            {
                                for (uint32 vertexIndex = 0; vertexIndex < numSubMeshVertices; ++vertexIndex)
                                {
                                    *destTexCoords = SMeshTexCoord(0.0f, 0.0f);
                                    ++destTexCoords;
                                }
                            }
                        }

                        // Second UV set.
                        if (hasUVs2)
                        {
                            if (sourceUVs2)
                            {
                                const AZ::Vector2* subMeshUVs = &sourceUVs2[subMeshStartVertex];
                                for (uint32 vertexIndex = 0; vertexIndex < numSubMeshVertices; ++vertexIndex)
                                {
                                    const AZ::Vector2& uv2 = subMeshUVs[vertexIndex];
                                    *destTexCoords2 = SMeshTexCoord(uv2.GetX(), uv2.GetY());
                                    ++destTexCoords2;
                                }
                            }
                            else
                            {
                                for (uint32 vertexIndex = 0; vertexIndex < numSubMeshVertices; ++vertexIndex)
                                {
                                    *destTexCoords2 = SMeshTexCoord(0.0f, 0.0f);
                                    ++destTexCoords2;
                                }
                            }
                        }

                        // Process tangents
                        if (hasTangents)
                        {
                            if (sourceTangents)
                            {
                                const AZ::Vector4* subMeshTangents = &sourceTangents[subMeshStartVertex];
                                for (uint32 vertexIndex = 0; vertexIndex < numSubMeshVertices; ++vertexIndex)
                                {
                                    const AZ::Vector4& sourceTangent = subMeshTangents[vertexIndex];
                                    const AZ::Vector3 sourceNormal(subMeshNormals[vertexIndex]);

                                    AZ::Vector3 bitangent;
                                    if (sourceBitangents)
                                    {
                                        bitangent = sourceBitangents[vertexIndex + subMeshStartVertex];
                                    }
                                    else
                                    {
                                        bitangent = sourceNormal.Cross(sourceTangent.GetAsVector3()) * sourceTangent.GetW();
                                    }

                                    *destTangents = SMeshTangents(
                                        Vec3(sourceTangent.GetX(), sourceTangent.GetY(), sourceTangent.GetZ()),
                                        Vec3(bitangent.GetX(), bitangent.GetY(), bitangent.GetZ()),
                                        Vec3(sourceNormal.GetX(), sourceNormal.GetY(), sourceNormal.GetZ()));
                                    ++destTangents;
                                }
                            }
                            else
                            {
                                for (uint32 vertexIndex = 0; vertexIndex < numSubMeshVertices; ++vertexIndex)
                                {
                                    *destTangents = SMeshTangents();
                                    ++destTangents;
                                }
                            }
                        }

                        // Pass vertex colors to the renderer.
                        if (sourceColors128 && destColors) // 128 bit colors
                        {
                            const AZ::Vector4* subMeshColors = &sourceColors128[subMeshStartVertex];
                            for (uint32 vertexIndex = 0; vertexIndex < numSubMeshVertices; ++vertexIndex)
                            {
                                const AZ::Vector4& colorVector = subMeshColors[vertexIndex];
                                const AZ::Color color(
                                    AZ::GetClamp(static_cast<float>(colorVector.GetX()), 0.0f, 1.0f),
                                    AZ::GetClamp(static_cast<float>(colorVector.GetY()), 0.0f, 1.0f),
                                    AZ::GetClamp(static_cast<float>(colorVector.GetZ()), 0.0f, 1.0f),
                                    AZ::GetClamp(static_cast<float>(colorVector.GetW()), 0.0f, 1.0f) );

                                *destColors = SMeshColor(color.GetR8(), color.GetG8(), color.GetB8(), color.GetA8());
                                ++destColors;
                            }
                        }
                        else if (sourceColors32 && destColors) // 32 bit colors
                        {
                            AZ::Color color;
                            const AZ::u32* subMeshColors = &sourceColors32[subMeshStartVertex];
                            for (uint32 vertexIndex = 0; vertexIndex < numSubMeshVertices; ++vertexIndex)
                            {
                                color.FromU32(subMeshColors[vertexIndex]);
                                *destColors = SMeshColor(color.GetR8(), color.GetG8(), color.GetB8(), color.GetA8());
                                ++destColors;
                            }
                        }

                        // Process AABB
                        AABB localAabb(AABB::RESET);
                        for (uint32 vertexIndex = 0; vertexIndex < numSubMeshVertices; ++vertexIndex)
                        {
                            const AZ::Vector3& sourcePosition = subMeshPositions[vertexIndex];
                            localAabb.Add(Vec3(sourcePosition.GetX(), sourcePosition.GetY(), sourcePosition.GetZ()));
                        }
                        subset.fRadius = localAabb.GetRadius();
                        subset.vCenter = localAabb.GetCenter();
                        primitive.m_mesh->m_bbox.Add(localAabb.min);
                        primitive.m_mesh->m_bbox.Add(localAabb.max);

                        // Process Skinning info
                        if (sourceSkinningInfo)
                        {
                            for (uint32 vertexIndex = 0; vertexIndex < numSubMeshVertices; ++vertexIndex)
                            {
                                const AZ::u32 originalVertex = sourceOriginalVertex[vertexIndex + subMesh->GetStartVertex()];
                                const AZ::u32 influenceCount = AZ::GetMin<AZ::u32>(maxInfluences, sourceSkinningInfo->GetNumInfluences(originalVertex));
                                AZ::u32 influenceIndex = 0;
                                int weightError = 255;

                                for (; influenceIndex < influenceCount; ++influenceIndex)
                                {
                                    EMotionFX::SkinInfluence* influence = sourceSkinningInfo->GetInfluence(originalVertex, influenceIndex);
                                    destBoneMapping->boneIds[influenceIndex] = influence->GetNodeNr();
                                    destBoneMapping->weights[influenceIndex] = static_cast<AZ::u8>(AZ::GetClamp<float>(influence->GetWeight() * 255.0f, 0.0f, 255.0f));
                                    weightError -= destBoneMapping->weights[influenceIndex];
                                }

                                destBoneMapping->weights[0] += weightError;

                                for (; influenceIndex < maxInfluences; ++influenceIndex)
                                {
                                    destBoneMapping->boneIds[influenceIndex] = 0;
                                    destBoneMapping->weights[influenceIndex] = 0;
                                }

                                ++destBoneMapping;
                            }
                        }

                        // Legacy index buffer fix.
                        primitive.m_mesh->m_subsets[0].FixRanges(primitive.m_mesh->m_pIndices);

                        // Convert tangent frame from matrix to quaternion based.
                        // Without this, materials do NOT render correctly on skinned characters.
                        if (primitive.m_mesh->m_pTangents && !primitive.m_mesh->m_pQTangents)
                        {
                            primitive.m_mesh->m_pQTangents = (SMeshQTangents*)primitive.m_mesh->m_pTangents;
                            MeshTangentsFrameToQTangents(
                                primitive.m_mesh->m_pTangents, sizeof(primitive.m_mesh->m_pTangents[0]), primitive.m_mesh->GetVertexCount(),
                                primitive.m_mesh->m_pQTangents, sizeof(primitive.m_mesh->m_pQTangents[0]));
                        }
                    }   // for all submeshes
                } // for all meshes

                lod.m_hasDynamicMeshes = hasDynamicMeshes;
            } // for all lods

            return true;
        }

        void CryRenderActor::Finalize()
        {
            //
            // The CMesh, which contains vertex streams, indices, uvs, bone influences, etc, is computed within
            // the job thread.
            // However, the render mesh and material need to be constructed on the main thread, as imposed by
            // the renderer. Naturally this is undesirable, but a limitation of the engine at the moment.
            //
            // The material also cannot be constructed natively. Materials only seem to be fully valid
            // if loaded from Xml data. Attempts to build procedurally, outside of the renderer code, have
            // been unsuccessful due to some aspects of the data being inaccessible.
            // Jumping through this hoop is acceptable for now since we'll soon be generating the material asset
            // in the asset pipeline and loading it via the game, as opposed to extracting the data here.
            //

            if (!gEnv)
            {
                return;
            }

            // Every CryRenderActorInstance will attempt to finalize the data, so ensure we only perform this action once.  
            if (m_isFinalized)
            {
                return;
            }

            AZ_Assert(m_actorAsset, "Invalid asset data");
            AZ_Assert(m_actorAsset->IsReady(), "Finalize has been called unexpectedly before the Actor asset has finished loading.");

            AZStd::string assetPath;
            EBUS_EVENT_RESULT(assetPath, AZ::Data::AssetCatalogRequestBus, GetAssetPathById, m_actorAsset->GetId());

            EMotionFX::Actor* actor = m_actorAsset->GetActor();
            const AZ::u32 numLODs = actor->GetNumLODLevels();

            // Process all LODs from the EMotionFX actor data.
            for (AZ::u32 lodIndex = 0; lodIndex < numLODs; ++lodIndex)
            {
                MeshLOD& lod = m_meshLODs[lodIndex];

                for (Primitive& primitive : lod.m_primitives)
                {
                    // Create and initialize render mesh.
                    primitive.m_renderMesh = gEnv->pRenderer->CreateRenderMesh("EMotion FX Actor", assetPath.c_str(), nullptr, eRMT_Dynamic);

                    const AZ::u32 renderMeshFlags = FSM_ENABLE_NORMALSTREAM | FSM_VERTEX_VELOCITY;
                    if (primitive.m_mesh)
                    {
                        primitive.m_renderMesh->SetMesh(*primitive.m_mesh, 0, renderMeshFlags, false);
                    }
                    // Free temporary load objects & buffers.
                    primitive.m_vertexBoneMappings.resize(0);
                }

                // It's now safe to use this LOD.
                lod.m_isReady = true;
            }

            m_isFinalized = true;
        }
    } // namespace Integration
} // namespace EMotionFX
