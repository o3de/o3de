/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/PackedVector3.h>
#include "EMotionFXConfig.h"
#include "Mesh.h"
#include "SubMesh.h"
#include "Node.h"
#include "SkinningInfoVertexAttributeLayer.h"
#include "VertexAttributeLayerAbstractData.h"
#include "Actor.h"
#include "SoftSkinDeformer.h"
#include "MeshDeformerStack.h"
#include <EMotionFX/Source/Allocators.h>
#include <MCore/Source/LogManager.h>

#include <Atom/RPI.Reflect/Model/ModelAsset.h>

namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(Mesh, MeshAllocator)

    Mesh::Mesh()
        : MCore::RefCounted()
    {
        m_numVertices        = 0;
        m_numIndices         = 0;
        m_numOrgVerts        = 0;
        m_numPolygons        = 0;
        m_indices            = nullptr;
        m_polyVertexCounts   = nullptr;
        m_isCollisionMesh    = false;
    }

    // allocation constructor
    Mesh::Mesh(uint32 numVerts, uint32 numIndices, uint32 numPolygons, uint32 numOrgVerts, bool isCollisionMesh)
        : MCore::RefCounted()
    {
        m_numVertices        = 0;
        m_numIndices         = 0;
        m_numPolygons        = 0;
        m_numOrgVerts        = 0;
        m_indices            = nullptr;
        m_polyVertexCounts   = nullptr;
        m_isCollisionMesh    = isCollisionMesh;

        // allocate the mesh data
        Allocate(numVerts, numIndices, numPolygons, numOrgVerts);
    }

    Mesh::~Mesh()
    {
        ReleaseData();
    }

    Mesh* Mesh::Create()
    {
        return aznew Mesh();
    }

    Mesh* Mesh::Create(uint32 numVerts, uint32 numIndices, uint32 numPolygons, uint32 numOrgVerts, bool isCollisionMesh)
    {
        return aznew Mesh(numVerts, numIndices, numPolygons, numOrgVerts, isCollisionMesh);
    }

    namespace AtomMeshHelpers
    {
        // Local 2D and 4D packed vector structs as we don't have packed representations in AzCore and don't plan to add these.
        struct Vector2
        {
            float m_x;
            float m_y;
        };

        struct Vector4
        {
            float m_x;
            float m_y;
            float m_z;
            float m_w;
        };

        // Needed for converting the packed vectors from the Atom buffers that normally load directly into GPU into AzCore versions used by EMFX.
        AZ::Vector2 ConvertVector(const Vector2& input)
        {
            return AZ::Vector2(input.m_x, input.m_y);
        }

        AZ::Vector3 ConvertVector(const AZ::PackedVector3f& input)
        {
            return AZ::Vector3(input.GetX(), input.GetY(), input.GetZ());
        }

        AZ::Vector4 ConvertVector(const Vector4& input)
        {
            return AZ::Vector4(input.m_x, input.m_y, input.m_z, input.m_w);
        }

        // Convert Atom buffer storing elements of type SourceType to an EMFX vertex attribute layer storing elements of type TargetType.
        // The vertex attribute layer is created within and added to the given target mesh.
        // Atom meshes might have different vertex features like some might contain tangents or multiple UV sets while other meshes do not have
        // tangents or don't contain any UV set at all. The corresponding EMFX sub-meshes do not support that, so we need to pad the vertex buffers.
        // @param[in] sourceModelLod Source model LOD asset to read the vertex buffers from.
        // @param[in] modelVertexCount Accumulated vertex count for all Atom meshes calculated and cached by the caller.
        // @param[in] sourceBufferName The name of the buffer to translate into a vertex attribute layer.
        // @param[in] inputBufferData Pointer to the source buffer from the Atom model LOD asset. This contains the data to be translated and copied over.
        // @param[in] targetMesh The EMFX mesh where the newly created vertex attribute layer should be added to.
        // @param[in] vertexAttributeLayerTypeId The type ID of the attribute layer used to identify type of vertex data (positions, normals, etc.)
        // @param[in] keepOriginals True in case the vertex elements change when deforming the mesh, false if not.
        // @param[in] defaultPaddingValue The value used for the sub-meshes in the EMFX that do not contain a given vertex feature in Atom for the given mesh.
        template<typename TargetType, typename SourceType>
        void CreateAndAddVertexAttributeLayer(const AZ::Data::Asset<AZ::RPI::ModelLodAsset>& sourceModelLod, AZ::u32 modelVertexCount,
            const AZ::Name& sourceBufferName, const void* inputBufferData,
            Mesh* targetMesh, AZ::u32 vertexAttributeLayerTypeId, bool keepOriginals,
            const TargetType& defaultPaddingValue)
        {
            VertexAttributeLayerAbstractData* targetVertexAttributeLayer = VertexAttributeLayerAbstractData::Create(modelVertexCount, vertexAttributeLayerTypeId, sizeof(TargetType), keepOriginals);
            TargetType* targetBuffer = static_cast<TargetType*>(targetVertexAttributeLayer->GetData());

            // Fill the vertex attribute layer by iterating through the Atom meshes and copying over the vertex data for each.
            [[maybe_unused]] size_t addedElements = 0;
            for (const AZ::RPI::ModelLodAsset::Mesh& atomMesh : sourceModelLod->GetMeshes())
            {
                const uint32_t atomMeshVertexCount = atomMesh.GetVertexCount();
                const AZ::RPI::BufferAssetView* bufferAssetView = atomMesh.GetSemanticBufferAssetView(sourceBufferName);
                if (bufferAssetView)
                {
                    const AZ::RHI::BufferViewDescriptor& bufferAssetViewDescriptor = bufferAssetView->GetBufferViewDescriptor();
                    const SourceType* atomMeshBuffer = reinterpret_cast<const SourceType*>(inputBufferData) + bufferAssetViewDescriptor.m_elementOffset;

                    for (size_t vertexIndex = 0; vertexIndex < atomMeshVertexCount; ++vertexIndex)
                    {
                        targetBuffer[vertexIndex] = ConvertVector(atomMeshBuffer[vertexIndex]);
                    }
                }
                else
                {
                    AZ_Warning("EMotionFX", false, "Padding %s buffer for mesh %s. Mesh has %d vertices while buffer is empty.",
                        sourceBufferName.GetCStr(), atomMesh.GetName().GetCStr(), atomMeshVertexCount);

                    for (size_t vertexIndex = 0; vertexIndex < atomMeshVertexCount; ++vertexIndex)
                    {
                        targetBuffer[vertexIndex] = defaultPaddingValue;
                    }
                }

                targetBuffer += atomMeshVertexCount;
                addedElements += atomMeshVertexCount;
            }

            AZ_Assert(addedElements == modelVertexCount, "The model has %d vertices while we only added %d elements to the %s buffer.",
                modelVertexCount, addedElements, sourceBufferName.GetCStr());

            // In case we want to keep the original values, the target buffer will have double the size and we copy the same thing twice.
            // The original values won't be touched when morphing or skinning but are needed as a base for the mesh deformations.
            if (keepOriginals)
            {
                memcpy(targetBuffer, targetVertexAttributeLayer->GetData(), sizeof(TargetType) * modelVertexCount);
            }

            targetMesh->AddVertexAttributeLayer(targetVertexAttributeLayer);
        }
    };

    Mesh* Mesh::CreateFromModelLod(const AZ::Data::Asset<AZ::RPI::ModelLodAsset>& sourceModelLod, const AZStd::unordered_map<AZ::u16, AZ::u16>& skinToSkeletonIndexMap)
    {
        AZ::u32 modelVertexCount = 0;
        AZ::u32 modelIndexCount = 0;

        // Find the maximum skin influences across all meshes to use when pre-allocating memory
        bool hasSkinInfluence = false;
        AZ::u32 maxSkinInfluences = 0;
        for (const AZ::RPI::ModelLodAsset::Mesh& mesh : sourceModelLod->GetMeshes())
        {
            modelVertexCount += mesh.GetVertexCount();
            modelIndexCount += mesh.GetIndexCount();
            const AZ::RPI::BufferAssetView* weightView = mesh.GetSemanticBufferAssetView(AZ::Name{"SKIN_WEIGHTS"});
            if(weightView)
            {
                const AZ::u32 meshInfluenceCount = weightView->GetBufferViewDescriptor().m_elementCount / mesh.GetVertexCount();
                maxSkinInfluences = AZStd::max(maxSkinInfluences, meshInfluenceCount);
                hasSkinInfluence = true;
            }
        }

        // Assert the skin influence range only if the model has skin influence data
        if (hasSkinInfluence)
        {
            AZ_Assert(maxSkinInfluences > 0 && maxSkinInfluences < 100, "Expect max skin influences in a reasonable value range.");
        }

        // IndicesPerFace defined in atom is 3.
        const AZ::u32 numPolygons = modelIndexCount / 3;
        Mesh* mesh = Create(modelVertexCount, modelIndexCount, numPolygons, modelVertexCount, false);

        // The lod has shared buffers that combine the data from each submesh.
        // These buffers can be accessed through the first submesh in their entirety
        // by using the BufferViewDescriptor from the Buffer instead of the one from the sub-mesh's BufferAssetView
        const auto sourceLodMeshes = sourceModelLod->GetMeshes();
        const AZ::RPI::ModelLodAsset::Mesh& sourceMesh0 = sourceLodMeshes[0];

        // Copy the index buffer for the entire lod
        AZStd::span<const uint8_t> indexBuffer = sourceMesh0.GetIndexBufferAssetView().GetBufferAsset()->GetBuffer();
        const AZ::RHI::BufferViewDescriptor& indexBufferViewDescriptor =
            sourceMesh0.GetIndexBufferAssetView().GetBufferAsset()->GetBufferViewDescriptor();
        AZ_ErrorOnce("EMotionFX", indexBufferViewDescriptor.m_elementSize == 4, "Index buffer must stored as 4 bytes.");
        const size_t indexBufferCountsInBytes = indexBufferViewDescriptor.m_elementCount * indexBufferViewDescriptor.m_elementSize;
        const size_t indexBufferOffsetInBytes = indexBufferViewDescriptor.m_elementOffset * indexBufferViewDescriptor.m_elementSize;
        memcpy(mesh->m_indices, indexBuffer.begin() + indexBufferOffsetInBytes, indexBufferCountsInBytes);

        // Set the polygon buffer
        AZ_PUSH_DISABLE_WARNING_MSVC(4244); //  warning C4244: '=': conversion from 'const int' to 'uint8', possible loss of data
        AZStd::fill(mesh->m_polyVertexCounts, mesh->m_polyVertexCounts + mesh->m_numPolygons, 3);
        AZ_POP_DISABLE_WARNING_MSVC

        // Skinning data from atom are stored in two separate buffer layer.
        const AZ::u16* skinJointIndices = nullptr;
        const float* skinWeights = nullptr;

        // Copy the vertex buffers
        for (const AZ::RPI::ModelLodAsset::Mesh::StreamBufferInfo& streamBufferInfo : sourceMesh0.GetStreamBufferInfoList())
        {
            const AZ::RHI::BufferViewDescriptor& bufferAssetViewDescriptor = streamBufferInfo.m_bufferAssetView.GetBufferAsset()->GetBufferViewDescriptor();
            const void* bufferData = streamBufferInfo.m_bufferAssetView.GetBufferAsset()->GetBuffer().data();
            const AZ::Name& name = streamBufferInfo.m_semantic.m_name;

            if (name == AZ::Name("POSITION"))
            {
                AtomMeshHelpers::CreateAndAddVertexAttributeLayer<AZ::Vector3, AZ::PackedVector3f>(sourceModelLod, modelVertexCount,
                    name, bufferData,
                    mesh, Mesh::ATTRIB_POSITIONS, /*keepOriginals=*/true,
                    AZ::Vector3(0.0f, 0.0f, 0.0f));
            }
            else if (name == AZ::Name("NORMAL"))
            {
                AtomMeshHelpers::CreateAndAddVertexAttributeLayer<AZ::Vector3, AZ::PackedVector3f>(sourceModelLod, modelVertexCount,
                    name, bufferData,
                    mesh, Mesh::ATTRIB_NORMALS, /*keepOriginals=*/true,
                    AZ::Vector3(1.0f, 0.0f, 0.0f));
            }
            else if (name == AZ::Name("UV"))
            {
                AtomMeshHelpers::CreateAndAddVertexAttributeLayer<AZ::Vector2, AtomMeshHelpers::Vector2>(sourceModelLod, modelVertexCount,
                    name, bufferData,
                    mesh, Mesh::ATTRIB_UVCOORDS, /*keepOriginals=*/false,
                    AZ::Vector2(0.0f, 0.0f));
            }
            else if (name == AZ::Name("TANGENT"))
            {
                AtomMeshHelpers::CreateAndAddVertexAttributeLayer<AZ::Vector4, AtomMeshHelpers::Vector4>(sourceModelLod, modelVertexCount,
                    name, bufferData,
                    mesh, Mesh::ATTRIB_TANGENTS, /*keepOriginals=*/true,
                    AZ::Vector4(1.0f, 0.0f, 0.0f, 0.0f));
            }
            else if (name == AZ::Name("BITANGENT"))
            {
                AtomMeshHelpers::CreateAndAddVertexAttributeLayer<AZ::Vector3, AZ::PackedVector3f>(sourceModelLod, modelVertexCount,
                    name, bufferData,
                    mesh, Mesh::ATTRIB_BITANGENTS, /*keepOriginals=*/true,
                    AZ::Vector3(1.0f, 0.0f, 0.0f));
            }
            else if (name == AZ::Name("SKIN_JOINTINDICES"))
            {
                // Atom stores the skin indices as uint16, but the buffer itself is a buffer of uint32 with two id's per element
                AZ_Assert(bufferAssetViewDescriptor.m_elementSize == 4, "Expect skin joint indices to be stored in a raw 32-bit per element buffer");

                // Multiply element offset by 2 here since m_elementOffset is referring to 32-bit elements
                // and the pointer we're offsetting is pointing to 16-bit data
                skinJointIndices = static_cast<const AZ::u16*>(bufferData) + (bufferAssetViewDescriptor.m_elementOffset * 2);
            }
            else if (name == AZ::Name("SKIN_WEIGHTS"))
            {
                skinWeights = static_cast<const float*>(bufferData) + bufferAssetViewDescriptor.m_elementOffset;
            }
        }

        // Add the original vertex layer
        VertexAttributeLayerAbstractData* originalVertexData = VertexAttributeLayerAbstractData::Create(modelVertexCount, Mesh::ATTRIB_ORGVTXNUMBERS, sizeof(AZ::u32), false);
        AZ::u32* originalVertexDataRaw = static_cast<AZ::u32*>(originalVertexData->GetData());
        for (AZ::u32 i = 0; i < modelVertexCount; ++i)
        {
            originalVertexDataRaw[i] = static_cast<AZ::u32>(i);
        }
        mesh->AddVertexAttributeLayer(originalVertexData);

        // Add skinning layer
        if (skinJointIndices && skinWeights)
        {
            // Create the skinning layer
            SkinningInfoVertexAttributeLayer* skinningLayer = SkinningInfoVertexAttributeLayer::Create(modelVertexCount, /*allocData=*/false);
            mesh->AddSharedVertexAttributeLayer(skinningLayer);
            MCore::Array2D<SkinInfluence>& skin2dArray = skinningLayer->GetArray2D();
            skin2dArray.SetNumPreCachedElements(maxSkinInfluences);
            skin2dArray.Resize(modelVertexCount);

            // Keep track of the number of unique jointIds
            AZStd::bitset<AZStd::numeric_limits<uint16>::max()> usedJoints;
            uint16 highestJointIndex = 0;
            AZ::u32 currentVertex = 0;
            for (const AZ::RPI::ModelLodAsset::Mesh& sourceMesh : sourceModelLod->GetMeshes())
            {
                AZ::u32 meshVertexCount = sourceMesh.GetVertexCount();
                const AZ::RPI::BufferAssetView* weightView = sourceMesh.GetSemanticBufferAssetView(AZ::Name{"SKIN_WEIGHTS"});
                const AZ::RPI::BufferAssetView* jointIdView = sourceMesh.GetSemanticBufferAssetView(AZ::Name{"SKIN_JOINTINDICES"});
                if (weightView && jointIdView)
                {
                    const AZ::u32 meshInfluenceCount = weightView->GetBufferViewDescriptor().m_elementCount / meshVertexCount;
                    const AZ::u32 weightOffsetInElements = weightView->GetBufferViewDescriptor().m_elementOffset;
                    // We multiply by two here since there are two jointId's packed per-element
                    const AZ::u32 jointIdOffsetInElements = jointIdView->GetBufferViewDescriptor().m_elementOffset * 2;

                    // Fill in skinning data from atom buffer
                    for (AZ::u32 v = 0; v < meshVertexCount; ++v)
                    {
                        for (AZ::u32 i = 0; i < meshInfluenceCount; ++i)
                        {
                            const float weight = skinWeights[weightOffsetInElements + v * meshInfluenceCount + i];
                            if (!AZ::IsClose(weight, 0.0f, FLT_EPSILON))
                            {
                                const AZ::u16 skinJointIndex = skinJointIndices[jointIdOffsetInElements + v * meshInfluenceCount + i];
                                if (skinToSkeletonIndexMap.find(skinJointIndex) == skinToSkeletonIndexMap.end())
                                {
                                    AZ_WarningOnce("EMotionFX", false, "Missing skin influences for index %d", skinJointIndex);
                                    continue;
                                }

                                const AZ::u16 skeletonJointIndex = skinToSkeletonIndexMap.at(skinJointIndex);
                                skinningLayer->AddInfluence(currentVertex, skeletonJointIndex, weight, 0);

                                usedJoints.set(skeletonJointIndex);
                                highestJointIndex = AZStd::max(highestJointIndex, skeletonJointIndex);
                            }
                        }

                        currentVertex++;
                    }
                }
            }

            mesh->SetNumUniqueJoints(aznumeric_caster(usedJoints.count()));
            mesh->SetHighestJointIndex(highestJointIndex);
        }

        AZ::u32 vertexOffset = 0;
        AZ::u32 indexOffset = 0;
        AZ::u32 startPolygon = 0;
        AZ::u32 subMeshIndex = 0;

        // One ModelLodAsset::Mesh corresponds to one EMotionFX::SubMesh
        for (const AZ::RPI::ModelLodAsset::Mesh& sourceSubMesh : sourceModelLod->GetMeshes())
        {
            AZ::u32 subMeshVertexCount = sourceSubMesh.GetVertexCount();
            AZ::u32 subMeshIndexCount = sourceSubMesh.GetIndexCount();
            AZ::u32 subMeshPolygonCount = subMeshIndexCount / 3;
            // Create sub mesh
            SubMesh* subMesh = SubMesh::Create(mesh,
                vertexOffset,
                indexOffset,
                startPolygon,
                subMeshVertexCount,
                subMeshIndexCount,
                subMeshPolygonCount,
                /*numJoints*/0);

            mesh->InsertSubMesh(subMeshIndex, subMesh);

            vertexOffset += subMeshVertexCount;
            indexOffset += subMeshIndexCount;
            startPolygon += subMeshPolygonCount;
            subMeshIndex++;
        }

        return mesh;
    }


    // allocate mesh data
    void Mesh::Allocate(uint32 numVerts, uint32 numIndices, uint32 numPolygons, uint32 numOrgVerts)
    {
        // get rid of existing data
        ReleaseData();

        // allocate the indices
        if (numIndices > 0 && numPolygons > 0)
        {
            m_indices            = (uint32*)MCore::AlignedAllocate(sizeof(uint32) * numIndices,  32, EMFX_MEMCATEGORY_GEOMETRY_MESHES, Mesh::MEMORYBLOCK_ID);
            m_polyVertexCounts   = (uint8*)MCore::AlignedAllocate(sizeof(uint8) * numPolygons, 16, EMFX_MEMCATEGORY_GEOMETRY_MESHES, Mesh::MEMORYBLOCK_ID);
        }

        // set number values
        m_numVertices    = numVerts;
        m_numPolygons    = numPolygons;
        m_numIndices     = numIndices;
        m_numOrgVerts    = numOrgVerts;
    }


    // copy all original data over the output data
    void Mesh::ResetToOriginalData()
    {
        for (VertexAttributeLayer* vertexAttribute : m_vertexAttributes)
        {
            vertexAttribute->ResetToOriginalData();
        }
    }


    // release allocated mesh data from memory
    void Mesh::ReleaseData()
    {
        // get rid of all shared vertex attributes
        RemoveAllSharedVertexAttributeLayers();

        // get rid of all non-shared vertex attributes
        RemoveAllVertexAttributeLayers();

        // get rid of all sub meshes
        for (SubMesh* subMesh : m_subMeshes)
        {
            subMesh->Destroy();
        }
        m_subMeshes.clear();

        if (m_indices)
        {
            MCore::AlignedFree(m_indices);
        }

        if (m_polyVertexCounts)
        {
            MCore::AlignedFree(m_polyVertexCounts);
        }

        // re-init members
        m_indices        = nullptr;
        m_polyVertexCounts = nullptr;
        m_numIndices     = 0;
        m_numVertices    = 0;
        m_numOrgVerts    = 0;
        m_numPolygons    = 0;
    }


    // calculate the tangent and bitangent for a given triangle
    void Mesh::CalcTangentAndBitangentForFace(const AZ::Vector3& posA, const AZ::Vector3& posB, const AZ::Vector3& posC,
        const AZ::Vector2& uvA,  const AZ::Vector2& uvB,  const AZ::Vector2& uvC,
        AZ::Vector3* outTangent, AZ::Vector3* outBitangent)
    {
        // reset the tangent and bitangent
        *outTangent = AZ::Vector3::CreateZero();
        if (outBitangent)
        {
            *outBitangent = AZ::Vector3::CreateZero();
        }

        const float x1 = posB.GetX() - posA.GetX();
        const float x2 = posC.GetX() - posA.GetX();
        const float y1 = posB.GetY() - posA.GetY();
        const float y2 = posC.GetY() - posA.GetY();
        const float z1 = posB.GetZ() - posA.GetZ();
        const float z2 = posC.GetZ() - posA.GetZ();

        const float s1 = uvB.GetX() - uvA.GetX();
        const float s2 = uvC.GetX() - uvA.GetX();
        const float t1 = uvB.GetY() - uvA.GetY();
        const float t2 = uvC.GetY() - uvA.GetY();

        const float divider = (s1 * t2 - s2 * t1);

        float r;
        if (MCore::Math::Abs(divider) < MCore::Math::epsilon)
        {
            r = 1.0f;
        }
        else
        {
            r = 1.0f / divider;
        }

        const AZ::Vector3 sdir((t2* x1 - t1* x2) * r,  (t2* y1 - t1* y2) * r,    (t2* z1 - t1* z2) * r);
        const AZ::Vector3 tdir((s1* x2 - s2* x1) * r,  (s1* y2 - s2* y1) * r,    (s1* z2 - s2* z1) * r);

        *outTangent = sdir;
        if (outBitangent)
        {
            *outBitangent = tdir;
        }
    }


    // calculate the S and T vectors
    bool Mesh::CalcTangents(uint32 uvSet, bool storeBitangents)
    {
        if (!CheckIfIsTriangleMesh())
        {
            AZ_Warning("EMotionFX", false, "Cannot calculate tangents for mesh that isn't a pure triangle mesh.");
            return false;
        }

        // find the uv layer, if it exists, otherwise return
        AZ::Vector2* uvData = static_cast<AZ::Vector2*>(FindVertexData(Mesh::ATTRIB_UVCOORDS, uvSet));
        if (!uvData)
        {
            // Try UV 0 as fallback.
            if (uvSet != 0)
            {
                uvData = static_cast<AZ::Vector2*>(FindVertexData(Mesh::ATTRIB_UVCOORDS, 0));
            }

            if (!uvData)
            {
                return false;
            }

            AZ_Warning("EMotionFX", false, "Cannot find UV set %d for this mesh during tangent generation. Falling back to UV set 0.", uvSet);
            uvSet = 0;
        }

        // calculate the number of tangent layers that are already available
        AZ::Vector4* tangents = nullptr;
        AZ::Vector4* orgTangents = nullptr;
        AZ::Vector3* bitangents = nullptr;
        AZ::Vector3* orgBitangents = nullptr;
        const size_t numTangentLayers = CalcNumAttributeLayers(Mesh::ATTRIB_TANGENTS);

        // make sure we have tangent data allocated for all uv layers before the given one
        for (size_t i = numTangentLayers; i <= uvSet; ++i)
        {
            // add a new tangent layer
            AddVertexAttributeLayer(VertexAttributeLayerAbstractData::Create(m_numVertices, Mesh::ATTRIB_TANGENTS, sizeof(AZ::Vector4), true));
            tangents    = static_cast<AZ::Vector4*>(FindVertexData(Mesh::ATTRIB_TANGENTS, i));
            orgTangents = static_cast<AZ::Vector4*>(FindOriginalVertexData(Mesh::ATTRIB_TANGENTS, i));

            // Add the bitangents layer.
            if (storeBitangents)
            {
                AddVertexAttributeLayer(VertexAttributeLayerAbstractData::Create(m_numVertices, Mesh::ATTRIB_BITANGENTS, sizeof(AZ::PackedVector3f), true));
                bitangents    = static_cast<AZ::Vector3*>(FindVertexData(Mesh::ATTRIB_BITANGENTS, i));
                orgBitangents = static_cast<AZ::Vector3*>(FindOriginalVertexData(Mesh::ATTRIB_BITANGENTS, i));
            }

            // default all tangents for the newly created layer
            AZ::Vector4 defaultTangent(1.0f, 0.0f, 0.0f, 0.0f);
            AZ::Vector3 defaultBitangent(0.0f, 0.0f, 1.0f);
            for (uint32 vtx = 0; vtx < m_numVertices; ++vtx)
            {
                tangents[vtx]      = defaultTangent;
                orgTangents[vtx]   = defaultTangent;

                if (orgBitangents && bitangents)
                {
                    bitangents[vtx]    = defaultBitangent;
                    orgBitangents[vtx] = defaultBitangent;
                }
            }
        }

        // get access to the tangent layer for the given uv set
        tangents      = static_cast<AZ::Vector4*>(FindVertexData(Mesh::ATTRIB_TANGENTS, uvSet));
        orgTangents   = static_cast<AZ::Vector4*>(FindOriginalVertexData(Mesh::ATTRIB_TANGENTS, uvSet));
        bitangents    = static_cast<AZ::Vector3*>(FindVertexData(Mesh::ATTRIB_BITANGENTS, uvSet));
        orgBitangents = static_cast<AZ::Vector3*>(FindOriginalVertexData(Mesh::ATTRIB_BITANGENTS, uvSet));

        AZ::Vector3* positions   = static_cast<AZ::Vector3*>(FindOriginalVertexData(Mesh::ATTRIB_POSITIONS));
        AZ::Vector3* normals     = static_cast<AZ::Vector3*>(FindOriginalVertexData(Mesh::ATTRIB_NORMALS));
        uint32*         indices     = GetIndices(); // the indices (face data)
        uint8*          vertCounts  = GetPolygonVertexCounts();
        AZ::Vector3     curTangent;
        AZ::Vector3     curBitangent;

        // calculate for every vertex the tangent and bitangent
        for (uint32 i = 0; i < m_numVertices; ++i)
        {
            orgTangents[i] = AZ::Vector4::CreateZero();
            tangents[i] = AZ::Vector4::CreateZero();

            if (orgBitangents && bitangents)
            {
                orgBitangents[i].Set(0.0f, 0.0f, 0.0f);
                bitangents[i].Set(0.0f, 0.0f, 0.0f);
            }
        }

        // calculate the tangents and bitangents for all vertices by traversing all polys
        uint32 polyStartIndex = 0;
        uint32 indexA, indexB, indexC;
        const uint32 numPolygons = GetNumPolygons();
        for (uint32 f = 0; f < numPolygons; f++)
        {
            const uint32 numPolyVerts = vertCounts[f];

            // explanation: numPolyVerts-2 == number of triangles
            // triangle has got 3 polygon vertices  -> 1 triangle
            // quad has got 4 polygon vertices      -> 2 triangles
            // pentagon has got 5 polygon vertices  -> 3 triangles
            for (uint32 i = 2; i < numPolyVerts; i++)
            {
                indexA = indices[polyStartIndex];
                indexB = indices[polyStartIndex + i];
                indexC = indices[polyStartIndex + i - 1];

                // calculate the tangent and bitangent for the face
                CalcTangentAndBitangentForFace(positions[indexA], positions[indexB], positions[indexC],
                    uvData[indexA], uvData[indexB], uvData[indexC],
                    &curTangent, &curBitangent);

                // normalize the vectors
                curTangent.NormalizeSafe();
                curBitangent.NormalizeSafe();

                // store the tangents in the orgTangents array
                const AZ::Vector4 vec4Tangent(curTangent.GetX(), curTangent.GetY(), curTangent.GetZ(), 1.0f);
                orgTangents[indexA] += vec4Tangent;
                orgTangents[indexB] += vec4Tangent;
                orgTangents[indexC] += vec4Tangent;

                // store the bitangents in the tangents array for now
                const AZ::Vector4 vec4Bitangent(curBitangent.GetX(), curBitangent.GetY(), curBitangent.GetZ(), 0.0f);
                tangents[indexA]    += vec4Bitangent;
                tangents[indexB]    += vec4Bitangent;
                tangents[indexC]    += vec4Bitangent;
            }

            polyStartIndex += numPolyVerts;
        }

        // calculate the per vertex tangents now, fixing up orthogonality and handling mirroring of the bitangent
        for (uint32 i = 0; i < m_numVertices; ++i)
        {
            // get the normal
            AZ::Vector3 normal(normals[i]);
            normal.NormalizeSafe();

            // get the tangent
            AZ::Vector3 tangent = AZ::Vector3(orgTangents[i].GetX(), orgTangents[i].GetY(), orgTangents[i].GetZ());
            if (MCore::SafeLength(tangent) < MCore::Math::epsilon)
            {
                tangent.Set(1.0f, 0.0f, 0.0f);
            }
            else
            {
                tangent.NormalizeSafe();
            }

            // get the bitangent
            AZ::Vector3 bitangent = AZ::Vector3(tangents[i].GetX(), tangents[i].GetY(), tangents[i].GetZ());    // We stored the bitangents inside the tangents array temporarily.
            if (MCore::SafeLength(bitangent) < MCore::Math::epsilon)
            {
                bitangent.Set(0.0f, 1.0f, 0.0f);
            }
            else
            {
                bitangent.NormalizeSafe();
            }

            // Gram-Schmidt orthogonalize
            AZ::Vector3 fixedTangent = tangent - (normal * normal.Dot(tangent));
            fixedTangent.NormalizeSafe();

            // calculate handedness
            const AZ::Vector3 crossResult = normal.Cross(tangent);
            const float tangentW = (crossResult.Dot(bitangent) < 0.0f) ? -1.0f : 1.0f;

            // store the real final tangents
            orgTangents[i].Set(fixedTangent.GetX(), fixedTangent.GetY(), fixedTangent.GetZ(), tangentW);
            tangents[i] = orgTangents[i];

            // store the bitangent
            if (bitangents && orgBitangents)
            {
                orgBitangents[i] = bitangent;
                bitangents[i] = orgBitangents[i];
            }
        }

        return true;
    }



    // creates an array of pointers to bones used by this face
    void Mesh::GatherBonesForFace(uint32 startIndexOfFace, AZStd::vector<Node*>& outBones, Actor* actor)
    {
        // get rid of existing data
        outBones.clear();

        // try to locate the skinning attribute information
        SkinningInfoVertexAttributeLayer* skinningLayer = (SkinningInfoVertexAttributeLayer*)FindSharedVertexAttributeLayer(SkinningInfoVertexAttributeLayer::TYPE_ID);

        // if there is no skinning info, there are no bones attached to the vertices, so we can quit
        if (skinningLayer == nullptr)
        {
            return;
        }

        // get the index data and original vertex numbers
        uint32* indices = GetIndices();
        uint32* orgVerts = (uint32*)FindVertexData(Mesh::ATTRIB_ORGVTXNUMBERS);

        Skeleton* skeleton = actor->GetSkeleton();

        // get the skinning info for all three vertices
        for (uint32 i = 0; i < 3; ++i)
        {
            // get the original vertex number
            // remember that a cube can have 24 vertices to render (stored in this mesh), while it has only 8 original vertices
            uint32 originalVertex = orgVerts[ indices[startIndexOfFace + i] ];

            // traverse all influences for this vertex
            const size_t numInfluences = skinningLayer->GetNumInfluences(originalVertex);
            for (size_t n = 0; n < numInfluences; ++n)
            {
                // get the bone of the influence
                Node* bone = skeleton->GetNode(skinningLayer->GetInfluence(originalVertex, n)->GetNodeNr());

                // if it isn't yet in the output array with bones, add it
                if (AZStd::find(begin(outBones), end(outBones), bone) == end(outBones))
                {
                    outBones.emplace_back(bone);
                }
            }
        }
    }


    // returns the maximum number of weights/influences for this face
    uint32 Mesh::CalcMaxNumInfluencesForFace(uint32 startIndexOfFace) const
    {
        // try to locate the skinning attribute information
        SkinningInfoVertexAttributeLayer* skinningLayer = (SkinningInfoVertexAttributeLayer*)FindSharedVertexAttributeLayer(SkinningInfoVertexAttributeLayer::TYPE_ID);

        // if there is no skinning info, there are no bones attached to the vertices, so we can quit
        if (skinningLayer == nullptr)
        {
            return 0;
        }

        // get the index data and original vertex numbers
        uint32* indices = GetIndices();
        uint32* orgVerts = (uint32*)FindVertexData(Mesh::ATTRIB_ORGVTXNUMBERS);

        // get the skinning info for all three vertices
        size_t maxInfluences = 0;
        for (uint32 i = 0; i < 3; ++i)
        {
            // get the original vertex number
            // remember that a cube can have 24 vertices to render (stored in this mesh), while it has only 8 original vertices
            uint32 originalVertex = orgVerts[ indices[startIndexOfFace + i] ];

            // check if the number of influences is higher as the highest recorded value
            size_t numInfluences = skinningLayer->GetNumInfluences(originalVertex);
            if (maxInfluences < numInfluences)
            {
                maxInfluences = numInfluences;
            }
        }

        // return the maximum number of influences for this triangle
        return aznumeric_cast<uint32>(maxInfluences);
    }


    // returns the maximum number of weights/influences for this mesh
    size_t Mesh::CalcMaxNumInfluences() const
    {
        // try to locate the skinning attribute information
        SkinningInfoVertexAttributeLayer* skinningLayer = (SkinningInfoVertexAttributeLayer*)FindSharedVertexAttributeLayer(SkinningInfoVertexAttributeLayer::TYPE_ID);

        // if there is no skinning info, there are no bones attached to the vertices, so we can quit
        if (skinningLayer == nullptr)
        {
            return 0;
        }

        size_t maxInfluences = 0;
        const size_t numOrgVerts = GetNumOrgVertices();
        for (size_t i = 0; i < numOrgVerts; ++i)
        {
            // set the number of max influences
            maxInfluences = AZStd::max(maxInfluences, skinningLayer->GetNumInfluences(i));
        }

        // return the maximum number of influences
        return maxInfluences;
    }


    // returns the maximum number of weights/influences for this mesh plus some extra information
    size_t Mesh::CalcMaxNumInfluences(AZStd::vector<size_t>& outVertexCounts) const
    {
        // Reset values.
        outVertexCounts.resize(CalcMaxNumInfluences() + 1);
        AZStd::fill(begin(outVertexCounts), end(outVertexCounts), 0);

        // Does the mesh have a skinning layer? If no we can quit directly as this means there are only unskinned vertices.
        SkinningInfoVertexAttributeLayer* skinningLayer = (SkinningInfoVertexAttributeLayer*)FindSharedVertexAttributeLayer(SkinningInfoVertexAttributeLayer::TYPE_ID);
        if (!skinningLayer)
        {
            outVertexCounts[0] = GetNumVertices();
            return 0;
        }

        const uint32* orgVerts = (uint32*)FindVertexData(Mesh::ATTRIB_ORGVTXNUMBERS);

        // Get the vertex counts for the influences.
        size_t maxInfluences = 0;
        const uint32 numVerts = GetNumVertices();
        for (uint32 i = 0; i < numVerts; ++i)
        {
            const uint32 orgVertex = orgVerts[i];

            // Increase the number of vertices for the given influence value.
            const size_t numInfluences = skinningLayer->GetNumInfluences(orgVertex);
            outVertexCounts[numInfluences]++;

            // Update the number of max influences.
            maxInfluences = AZStd::max(maxInfluences, numInfluences);
        }

        return maxInfluences;
    }


    // remove a given submesh
    void Mesh::RemoveSubMesh(size_t nr, bool delFromMem)
    {
        SubMesh* subMesh = m_subMeshes[nr];
        m_subMeshes.erase(AZStd::next(begin(m_subMeshes), nr));
        if (delFromMem)
        {
            subMesh->Destroy();
        }
    }


    // insert a given submesh
    void Mesh::InsertSubMesh(size_t insertIndex, SubMesh* subMesh)
    {
        m_subMeshes.emplace(AZStd::next(begin(m_subMeshes), insertIndex), subMesh);
    }


    // count the given type of vertex attribute layers
    size_t Mesh::CalcNumAttributeLayers(uint32 type) const
    {
        size_t numLayers = 0;

        // check the types of all vertex attribute layers
        for (auto* vertexAttribute : m_vertexAttributes)
        {
            if (vertexAttribute->GetType() == type)
            {
                numLayers++;
            }
        }

        return numLayers;
    }


    // get the number of UV layers
    size_t Mesh::CalcNumUVLayers() const
    {
        return CalcNumAttributeLayers(Mesh::ATTRIB_UVCOORDS);
    }

    //---------------------------------------------------------------

    VertexAttributeLayer* Mesh::GetSharedVertexAttributeLayer(size_t layerNr)
    {
        MCORE_ASSERT(layerNr < m_sharedVertexAttributes.size());
        return m_sharedVertexAttributes[layerNr];
    }


    void Mesh::AddSharedVertexAttributeLayer(VertexAttributeLayer* layer)
    {
        MCORE_ASSERT(AZStd::find(begin(m_sharedVertexAttributes), end(m_sharedVertexAttributes), layer) == end(m_sharedVertexAttributes));
        m_sharedVertexAttributes.emplace_back(layer);
    }


    size_t Mesh::GetNumSharedVertexAttributeLayers() const
    {
        return m_sharedVertexAttributes.size();
    }


    size_t Mesh::FindSharedVertexAttributeLayerNumber(uint32 layerTypeID, size_t occurrence) const
    {
        const auto foundLayer = AZStd::find_if(begin(m_sharedVertexAttributes), end(m_sharedVertexAttributes), [layerTypeID, occurrence](const VertexAttributeLayer* layer) mutable
        {
            return layer->GetType() == layerTypeID && occurrence-- == 0;
        });
        return foundLayer != end(m_sharedVertexAttributes) ? AZStd::distance(begin(m_sharedVertexAttributes), foundLayer) : InvalidIndex;
    }


    // find the vertex attribute layer and return a pointer
    VertexAttributeLayer* Mesh::FindSharedVertexAttributeLayer(uint32 layerTypeID, size_t occurence) const
    {
        size_t layerNr = FindSharedVertexAttributeLayerNumber(layerTypeID, occurence);
        if (layerNr == InvalidIndex)
        {
            return nullptr;
        }

        return m_sharedVertexAttributes[layerNr];
    }



    // delete all shared attribute layers
    void Mesh::RemoveAllSharedVertexAttributeLayers()
    {
        while (m_sharedVertexAttributes.size())
        {
            m_sharedVertexAttributes.back()->Destroy();
            m_sharedVertexAttributes.pop_back();
        }
    }


    // remove a layer by its index
    void Mesh::RemoveSharedVertexAttributeLayer(size_t layerNr)
    {
        MCORE_ASSERT(layerNr < m_sharedVertexAttributes.size());
        m_sharedVertexAttributes[layerNr]->Destroy();
        m_sharedVertexAttributes.erase(AZStd::next(begin(m_sharedVertexAttributes), layerNr));
    }


    size_t Mesh::GetNumVertexAttributeLayers() const
    {
        return m_vertexAttributes.size();
    }


    VertexAttributeLayer* Mesh::GetVertexAttributeLayer(size_t layerNr)
    {
        MCORE_ASSERT(layerNr < m_vertexAttributes.size());
        return m_vertexAttributes[layerNr];
    }


    void Mesh::AddVertexAttributeLayer(VertexAttributeLayer* layer)
    {
        MCORE_ASSERT(AZStd::find(begin(m_vertexAttributes), end(m_vertexAttributes), layer) == end(m_vertexAttributes));
        m_vertexAttributes.emplace_back(layer);
    }


    // find the layer number
    size_t Mesh::FindVertexAttributeLayerNumber(uint32 layerTypeID, size_t occurrence) const
    {
        const auto foundLayer = AZStd::find_if(begin(m_vertexAttributes), end(m_vertexAttributes), [layerTypeID, occurrence](const VertexAttributeLayer* layer) mutable
        {
            return layer->GetType() == layerTypeID && occurrence-- == 0;
        });
        return foundLayer != end(m_vertexAttributes) ? AZStd::distance(begin(m_vertexAttributes), foundLayer) : InvalidIndex;
    }


    // find the layer number
    size_t Mesh::FindVertexAttributeLayerNumberByName(uint32 layerTypeID, const char* name) const
    {
        const auto foundLayer = AZStd::find_if(begin(m_vertexAttributes), end(m_vertexAttributes), [layerTypeID, name](const VertexAttributeLayer* layer)
        {
            return layer->GetType() == layerTypeID && layer->GetNameString() == name;
        });
        return foundLayer != end(m_vertexAttributes) ? AZStd::distance(begin(m_vertexAttributes), foundLayer) : InvalidIndex;
    }



    // find the vertex attribute layer and return a pointer
    VertexAttributeLayer* Mesh::FindVertexAttributeLayer(uint32 layerTypeID, size_t occurence) const
    {
        const size_t layerNr = FindVertexAttributeLayerNumber(layerTypeID, occurence);
        if (layerNr == InvalidIndex)
        {
            return nullptr;
        }

        return m_vertexAttributes[layerNr];
    }


    // find the vertex attribute layer and return a pointer
    VertexAttributeLayer* Mesh::FindVertexAttributeLayerByName(uint32 layerTypeID, const char* name) const
    {
        const size_t layerNr = FindVertexAttributeLayerNumberByName(layerTypeID, name);
        if (layerNr == InvalidIndex)
        {
            return nullptr;
        }

        return m_vertexAttributes[layerNr];
    }


    void Mesh::RemoveAllVertexAttributeLayers()
    {
        while (m_vertexAttributes.size())
        {
            m_vertexAttributes.back()->Destroy();
            m_vertexAttributes.pop_back();
        }
    }


    void Mesh::RemoveVertexAttributeLayer(size_t layerNr)
    {
        MCORE_ASSERT(layerNr < m_vertexAttributes.size());
        m_vertexAttributes[layerNr]->Destroy();
        m_vertexAttributes.erase(AZStd::next(begin(m_vertexAttributes), layerNr));
    }



    // clone the mesh
    Mesh* Mesh::Clone()
    {
        // allocate a mesh of the same dimensions
        Mesh* clone = aznew Mesh(m_numVertices, m_numIndices, m_numPolygons, m_numOrgVerts, m_isCollisionMesh);

        // copy the mesh data
        MCore::MemCopy(clone->m_indices, m_indices, sizeof(uint32) * m_numIndices);
        MCore::MemCopy(clone->m_polyVertexCounts, m_polyVertexCounts, sizeof(uint8) * m_numPolygons);

        // copy the submesh data
        const size_t numSubMeshes = m_subMeshes.size();
        clone->m_subMeshes.resize(numSubMeshes);
        for (size_t i = 0; i < numSubMeshes; ++i)
        {
            clone->m_subMeshes[i] = m_subMeshes[i]->Clone(clone);
        }

        // clone the shared vertex attributes
        const size_t numSharedAttributes = m_sharedVertexAttributes.size();
        clone->m_sharedVertexAttributes.resize(numSharedAttributes);
        for (size_t i = 0; i < numSharedAttributes; ++i)
        {
            clone->m_sharedVertexAttributes[i] = m_sharedVertexAttributes[i]->Clone();
        }

        // clone the non-shared vertex attributes
        const size_t numAttributes = m_vertexAttributes.size();
        clone->m_vertexAttributes.resize(numAttributes);
        for (size_t i = 0; i < numAttributes; ++i)
        {
            clone->m_vertexAttributes[i] = m_vertexAttributes[i]->Clone();
        }

        // return the resulting cloned mesh
        return clone;
    }


    // swap the data for two vertices
    void Mesh::SwapVertex(uint32 vertexA, uint32 vertexB)
    {
        MCORE_ASSERT(vertexA < m_numVertices);
        MCORE_ASSERT(vertexB < m_numVertices);

        // if we try to swap itself then there is nothing to do
        if (vertexA == vertexB)
        {
            return;
        }

        // swap all vertex attribute layers
        const size_t numLayers = m_vertexAttributes.size();
        for (size_t i = 0; i < numLayers; ++i)
        {
            m_vertexAttributes[i]->SwapAttributes(vertexA, vertexB);
        }
    }

    // remove vertex data from the mesh
    void Mesh::RemoveVertices(uint32 startVertexNr, uint32 endVertexNr, bool changeIndexBuffer, bool removeEmptySubMeshes)
    {
        // perform some checks on the input data
        MCORE_ASSERT(endVertexNr < m_numVertices);
        MCORE_ASSERT(startVertexNr < m_numVertices);

        // make sure the start vertex is before the end vertex in release mode, to prevent weirdness
        if (startVertexNr > endVertexNr)
        {
            uint32 temp = startVertexNr;
            startVertexNr = endVertexNr;
            endVertexNr = temp;
        }

        //------------------------------------
        // Remove the vertex attributes
        //------------------------------------
        const uint32 numVertsToRemove = (endVertexNr - startVertexNr) + 1; // +1 because we remove the end vertex as well

                                                                           // remove the num verices counter
        m_numVertices -= numVertsToRemove;

        // remove the attributes from the vertex attribute layers
        const size_t numLayers = GetNumVertexAttributeLayers();
        for (size_t i = 0; i < numLayers; ++i)
        {
            GetVertexAttributeLayer(i)->RemoveAttributes(startVertexNr, endVertexNr);
        }

        //------------------------------------------------------------------------
        // Fix the submesh number of vertices and start vertex offset values
        //------------------------------------------------------------------------
        [[maybe_unused]] uint32 numRemoved = 0;
        const uint32 v = startVertexNr;
        for (uint32 w = 0; w < numVertsToRemove; ++w)
        {
            // adjust all submesh start index offsets changed
            for (size_t s = 0; s < m_subMeshes.size();)
            {
                SubMesh* subMesh = m_subMeshes[s];

                // if we remove a vertex from this submesh
                if (subMesh->GetStartVertex() <= v && subMesh->GetStartVertex() + subMesh->GetNumVertices() > v)
                {
                    numRemoved++;
                    subMesh->SetNumVertices(subMesh->GetNumVertices() - 1);
                }

                // now find out if we need to adjust the vertex offset of the submesh
                if (subMesh->GetStartVertex() > v)
                {
                    subMesh->SetStartVertex(subMesh->GetStartVertex() - 1);
                }

                // remove the submesh if it's empty
                if (subMesh->GetNumVertices() == 0 && removeEmptySubMeshes)
                {
                    m_subMeshes.erase(AZStd::next(begin(m_subMeshes), s));
                }
                else
                {
                    s++;
                }
            }
        }

        // make sure they all got removed
        MCORE_ASSERT(numRemoved == numVertsToRemove);

        //------------------------------------
        // Fix the index buffer
        //------------------------------------
        if (changeIndexBuffer)
        {
            for (uint32 i = 0; i < m_numIndices; ++i)
            {
                if (m_indices[i] > startVertexNr)
                {
                    m_indices[i] -= numVertsToRemove;
                }
            }
        }
    }


    // remove empty submeshes
    size_t Mesh::RemoveEmptySubMeshes(bool onlyRemoveOnZeroVertsAndTriangles)
    {
        size_t numRemoved = 0;

        // for all the submeshes
        for (size_t i = 0; i < m_subMeshes.size();)
        {
            SubMesh* subMesh = m_subMeshes[i];

            // get some stats about the submesh
            bool mustRemove;
            bool hasZeroVerts   = (subMesh->GetNumVertices() == 0);
            bool hasZeroTris    = (subMesh->GetNumIndices() == 0);

            // decide if we need to remove it or not
            if (onlyRemoveOnZeroVertsAndTriangles)
            {
                mustRemove = (hasZeroVerts && hasZeroTris);
            }
            else
            {
                mustRemove = (hasZeroVerts || hasZeroTris);
            }

            // remove or skip
            if (mustRemove)
            {
                m_subMeshes.erase(AZStd::next(begin(m_subMeshes), i));
                numRemoved++;
            }
            else
            {
                i++;
            }
        }

        // return the number of removed submeshes
        return numRemoved;
    }


    // find vertex data
    void* Mesh::FindVertexData(uint32 layerID, size_t occurrence) const
    {
        VertexAttributeLayer* layer = FindVertexAttributeLayer(layerID, occurrence);
        if (layer)
        {
            return layer->GetData();
        }

        return nullptr;
    }


    // find vertex data
    void* Mesh::FindVertexDataByName(uint32 layerID, const char* name) const
    {
        VertexAttributeLayer* layer = FindVertexAttributeLayerByName(layerID, name);
        if (layer)
        {
            return layer->GetData();
        }

        return nullptr;
    }



    // find original vertex data
    void* Mesh::FindOriginalVertexData(uint32 layerID, size_t occurrence) const
    {
        VertexAttributeLayer* layer = FindVertexAttributeLayer(layerID, occurrence);
        if (layer)
        {
            return layer->GetOriginalData();
        }

        return nullptr;
    }


    // find original vertex data
    void* Mesh::FindOriginalVertexDataByName(uint32 layerID, const char* name) const
    {
        VertexAttributeLayer* layer = FindVertexAttributeLayerByName(layerID, name);
        if (layer)
        {
            return layer->GetOriginalData();
        }

        return nullptr;
    }


    void Mesh::CalcAabb(AZ::Aabb* outBoundingBox, const Transform& transform, uint32 vertexFrequency)
    {
        MCORE_ASSERT(vertexFrequency >= 1);
        *outBoundingBox = AZ::Aabb::CreateNull();

        AZ::Vector3* positions = (AZ::Vector3*)FindVertexData(ATTRIB_POSITIONS);

        const uint32 numVerts = GetNumVertices();
        for (uint32 i = 0; i < numVerts; i += vertexFrequency)
        {
            outBoundingBox->AddPoint(transform.TransformPoint(positions[i]));
        }
    }



    // intersection test between the mesh and a ray
    bool Mesh::Intersects(const Transform& transform, const MCore::Ray& ray)
    {
        // get the positions and indices and calculate the inverse of the transformation matrix
        const AZ::Vector3* positions = (AZ::Vector3*)FindVertexData(Mesh::ATTRIB_POSITIONS);
        const Transform invTransform = transform.Inversed();

        // transform origin and dest of the ray into space of the mesh
        // on this way we do not have to convert the vertices into world space
        const AZ::Vector3       newOrigin   = invTransform.TransformPoint(ray.GetOrigin());
        const AZ::Vector3       newDest     = invTransform.TransformPoint(ray.GetDest());
        const MCore::Ray        testRay(newOrigin, newDest);

        // iterate over all polygons, triangulate internally
        const uint32* indices       = GetIndices();
        const uint8* vertCounts     = GetPolygonVertexCounts();
        uint32 indexA, indexB, indexC;
        uint32 polyStartIndex = 0;
        const uint32 numPolygons = GetNumPolygons();
        for (uint32 p = 0; p < numPolygons; p++)
        {
            const uint32 numPolyVerts = vertCounts[p];

            // iterate over all triangles inside this polygon
            // explanation: numPolyVerts-2 == number of triangles
            // 3 verts=1 triangle, 4 verts=2 triangles, etc
            for (uint32 i = 2; i < numPolyVerts; i++)
            {
                indexA = indices[polyStartIndex];
                indexB = indices[polyStartIndex + i];
                indexC = indices[polyStartIndex + i - 1];

                if (testRay.Intersects(positions[indexA], positions[indexB], positions[indexC]))
                {
                    return true;
                }
            }

            polyStartIndex += numPolyVerts;
        }

        // there is no intersection
        return false;
    }



    // intersection test between the mesh and a ray, includes calculation of intersection point
    bool Mesh::Intersects(const Transform& transform, const MCore::Ray& ray, AZ::Vector3* outIntersect, float* outBaryU, float* outBaryV, uint32* outIndices)
    {
        AZ::Vector3* positions = (AZ::Vector3*)FindVertexData(Mesh::ATTRIB_POSITIONS);
        Transform           invNodeTransform = transform.Inversed();
        AZ::Vector3         newOrigin = invNodeTransform.TransformPoint(ray.GetOrigin());
        AZ::Vector3         newDest = invNodeTransform.TransformPoint(ray.GetDest());
        float               closestDist = FLT_MAX;
        bool                hasIntersected = false;
        //uint32            closestStartIndex=0;
        AZ::Vector3         intersectionPoint;
        AZ::Vector3         closestIntersect(0.0f, 0.0f, 0.0f);
        uint32              closestIndices[3];
        float               dist, baryU, baryV, closestBaryU = 0, closestBaryV = 0;

        // the test ray, in space of the node (object space)
        // on this way we do not have to convert the vertices into world space
        MCore::Ray testRay(newOrigin, newDest);

        // iterate over all polygons, triangulate internally
        const uint32* indices       = GetIndices();
        const uint8* vertCounts     = GetPolygonVertexCounts();
        uint32 indexA, indexB, indexC;
        uint32 polyStartIndex = 0;
        const uint32 numPolygons = GetNumPolygons();
        for (uint32 p = 0; p < numPolygons; p++)
        {
            const uint32 numPolyVerts = vertCounts[p];

            // iterate over all triangles inside this polygon
            // explanation: numPolyVerts-2 == number of triangles
            // 3 verts=1 triangle, 4 verts=2 triangles, etc
            for (uint32 i = 2; i < numPolyVerts; i++)
            {
                indexA = indices[polyStartIndex];
                indexB = indices[polyStartIndex + i];
                indexC = indices[polyStartIndex + i - 1];

                // test the ray with the triangle (in object space)
                if (testRay.Intersects(positions[indexA], positions[indexB], positions[indexC], &intersectionPoint, &baryU, &baryV))
                {
                    // calculate the squared distance between the intersection point and the ray origin
                    dist = (intersectionPoint - newOrigin).GetLengthSq();

                    // if it is the closest intersection point until now, record it as closest intersection
                    if (dist < closestDist)
                    {
                        closestDist         = dist;
                        closestIntersect    = intersectionPoint;
                        hasIntersected      = true;
                        //closestStartIndex = polyStartIndex;
                        closestBaryU        = baryU;
                        closestBaryV        = baryV;
                        closestIndices[0]   = indexA;
                        closestIndices[1]   = indexB;
                        closestIndices[2]   = indexC;
                    }
                }
            }

            polyStartIndex += numPolyVerts;
        }

        // store the closest intersection point (in world space)
        if (hasIntersected)
        {
            if (outIntersect)
            {
                *outIntersect = transform.TransformPoint(closestIntersect);
            }

            if (outIndices)
            {
                outIndices[0] = closestIndices[0];
                outIndices[1] = closestIndices[1];
                outIndices[2] = closestIndices[2];
            }

            if (outBaryU)
            {
                *outBaryU = closestBaryU;
            }

            if (outBaryV)
            {
                *outBaryV = closestBaryV;
            }
        }

        // return the result
        return hasIntersected;
    }


    // log debugging information
    void Mesh::Log()
    {
        // get all current data
        //  uint32*     indices     = GetIndices();                                             // never returns nullptr
        //uint32*     orgVerts    = (uint32*) FindVertexData( Mesh::ATTRIB_ORGVTXNUMBERS ); // never returns nullptr
        //  uint32*     colors32    = (uint32*) FindVertexData( Mesh::ATTRIB_COLORS32 );        // 32-bit colors
        //  Vector3*    positions   = (Vector3*)FindVertexData( Mesh::ATTRIB_POSITIONS );       // never returns nullptr
        //  Vector3*    normals     = (Vector3*)FindVertexData( Mesh::ATTRIB_NORMALS );         // never returns nullptr
        //  Vector4*    tangents    = (Vector4*)FindVertexData( Mesh::ATTRIB_TANGENTS );        // note the Vector4 instead of Vector3!
        //  AZ::Vector2*    uvSet1  = static_cast<AZ::Vector2*>(FindVertexData( Mesh::ATTRIB_UVCOORDS ));       // the first UV set
        //  AZ::Vector2*    uvSet2  = static_cast<AZ::Vector2*>(FindVertexData( Mesh::ATTRIB_UVCOORDS, 1 ));        // the second UV set
        //  AZ::Vector2*    uvSet3  = static_cast<AZ::Vector2*>(FindVertexData( Mesh::ATTRIB_UVCOORDS, 2 ));        // the third UV set
        //  RGBAColor*  col128      = (RGBAColor*)FindVertexData( Mesh::ATTRIB_COLORS128 );     // 128 bit colors (one float per r/g/b/a)

        // display mesh info
        MCore::LogDebug("- Mesh");
        MCore::LogDebug("  + Num vertices             = %d", GetNumVertices());
        MCore::LogDebug("  + Num indices              = %d (%d polygons)", GetNumIndices(), GetNumPolygons());
        MCore::LogDebug("  + Num original vertices    = %d", GetNumOrgVertices());
        MCore::LogDebug("  + Num submeshes            = %d", GetNumSubMeshes());
        MCore::LogDebug("  + Num attrib layers        = %d", GetNumVertexAttributeLayers());
        MCore::LogDebug("  + Num shared attrib layers = %d", GetNumSharedVertexAttributeLayers());
        MCore::LogDebug("  + Is Triangle Mesh         = %d", CheckIfIsTriangleMesh());
        MCore::LogDebug("  + Is Quad Mesh             = %d", CheckIfIsQuadMesh());
        /*
        // iterate through and log all org vertex numbers
        const uint32 numOrgVertices = GetNumOrgVertices();
        LogDebug("   - Org Vertices (%d)", numOrgVertices);
        for (i=0; i<numOrgVertices; ++i)
        LogDebug("     + %d", orgVerts[i]);

        // iterate through and log all positions
        const uint32 numPositions = GetNumVertices();
        LogDebug("   - Positions / Normals (%d)", numPositions);
        for (i=0; i<numPositions; ++i)
        LogDebug("     + Position: %f %f %f, Normal: %f %f %f", positions[i].x, positions[i].y, positions[i].z, normals[i].x, normals[i].y, normals[i].z);
        */
        // iterate through all of its submeshes
        const size_t numSubMeshes = GetNumSubMeshes();
        for (size_t s = 0; s < numSubMeshes; ++s)
        {
            // get the current submesh
            SubMesh* subMesh = GetSubMesh(s);

            // output the primitive info
            MCore::LogDebug("   - SubMesh / Primitive #%d:", s);
            MCore::LogDebug("     + Start vertex = %d", subMesh->GetStartVertex());
            MCore::LogDebug("     + Start index  = %d", subMesh->GetStartIndex());
            MCore::LogDebug("     + Num vertices = %d", subMesh->GetNumVertices());
            MCore::LogDebug("     + Num indices  = %d (%d polygons)", subMesh->GetNumIndices(), subMesh->GetNumPolygons());
            MCore::LogDebug("     + Num bones    = %d", subMesh->GetNumBones());

            /*      // output all triangle indices that point inside the data we output above
            LogDebug("       - Triangle Indices:");
            const uint32 startVertex    = subMesh->GetStartVertex();
            const uint32 startIndex     = subMesh->GetStartIndex();
            const uint32 endIndex       = subMesh->GetStartIndex() + subMesh->GetNumIndices();
            for (i=startIndex; i<endIndex; i+=3)
            {
            // remove the start index values if you want them local in the submesh vertex buffers
            // otherwise do not remove the start vertex, then they point absolute inside the big
            // vertex attribute buffers of the mesh
            const uint32 indexA = indices[i]   - startVertex;
            const uint32 indexB = indices[i+1] - startVertex;
            const uint32 indexC = indices[i+2] - startVertex;
            LogDebug("         + (%d, %d, %d)", indexA, indexB, indexC);
            }*/

            // output the bones used by this submesh
            MCore::LogDebug("       - Bone list:");
            const size_t numBones = subMesh->GetNumBones();
            for (size_t j = 0; j < numBones; ++j)
            {
                const size_t nodeNr   = subMesh->GetBone(j);
                MCore::LogDebug("         + NodeNr %zu", nodeNr);
            }
        }
    }


    // check for a given mesh how we categorize it
    Mesh::EMeshType Mesh::ClassifyMeshType(size_t lodLevel, Actor* actor, size_t nodeIndex, bool forceCPUSkinning, uint32 maxInfluences, uint32 maxBonesPerSubMesh) const
    {
        // get the mesh deformer stack for the given node at the given detail level
        MeshDeformerStack* deformerStack = actor->GetMeshDeformerStack(lodLevel, nodeIndex);
        if (deformerStack)
        {
            // if there are multiple mesh deformers we have to perform deformations on the CPU
            if (deformerStack->GetNumDeformers() > 1)
            {
                return MESHTYPE_CPU_DEFORMED;
            }

            // is there is only one mesh deformer?
            if (deformerStack->GetNumDeformers() == 1)
            {
                // if the deformer is a skinning deformer, we can process it using a GPU skinning shader
                if (deformerStack->GetDeformer(0)->GetType() == SoftSkinDeformer::TYPE_ID)
                {
                    if (forceCPUSkinning)
                    {
                        return MESHTYPE_CPU_DEFORMED;
                    }

                    // check if the mesh uses more than the given number of weights/bones per vertex
                    // in that case use CPU skinning
                    Mesh* mesh = actor->GetMesh(lodLevel, nodeIndex);
                    Node* node = actor->GetSkeleton()->GetNode(nodeIndex);
                    size_t meshMaxInfluences = mesh->CalcMaxNumInfluences();
                    if (meshMaxInfluences > maxInfluences)
                    {
                        MCore::LogWarning("*** PERFORMANCE WARNING *** Mesh for node '%s' in geometry LOD %d uses more than %d (%d) bones. Forcing CPU deforms for this mesh.", node->GetName(), lodLevel, maxInfluences, meshMaxInfluences);
                        return MESHTYPE_CPU_DEFORMED;
                    }

                    // check if there is any submesh with more than the given number of bones, which would mean we cannot skin on the GPU
                    // then force CPU skinning as well
                    const size_t numSubMeshes = mesh->GetNumSubMeshes();
                    for (size_t i = 0; i < numSubMeshes; ++i)
                    {
                        if (mesh->GetSubMesh(i)->GetNumBones() > maxBonesPerSubMesh)
                        {
                            MCore::LogWarning("*** PERFORMANCE WARNING *** Submesh %d for node '%s' in geometry LOD %d uses more than %d bones (%d). Forcing CPU deforms for this mesh.", i, node->GetName(), lodLevel, maxBonesPerSubMesh, mesh->GetSubMesh(i)->GetNumBones());
                            return MESHTYPE_CPU_DEFORMED;
                        }
                    }

                    // perform skinning on the GPU inside a vertex shader
                    return MESHTYPE_GPU_DEFORMED;
                }
                else
                {
                    return MESHTYPE_CPU_DEFORMED; // it's using a non-skinning controller, so use CPU deformations
                }
            }
        }

        // there are no deformations happening
        return MESHTYPE_STATIC;
    }


    // convert the indices from 32-bit to 16-bit values
    bool Mesh::ConvertTo16BitIndices()
    {
        // set to success as nothing bad happened yet
        bool result = true;

        // check if the indices are valid and return false in case they aren't
        if (m_indices == nullptr)
        {
            return false;
        }

        // use our 32-bit index buffer as new 16-bit index array directly
        uint16* indices = (uint16*)m_indices;

        // iterate over all indices and convert the values
        for (uint32 i = 0; i < m_numIndices; ++i)
        {
            // create a temporary copy of our 32-bit vertex index
            const uint32 oldVertexIndex = m_indices[i];

            // check if our index is in range of an unsigned short
            if (oldVertexIndex < 65536)
            {
                indices[i] = (uint16)oldVertexIndex;
            }
            else
            {
                indices[i]  = MCORE_INVALIDINDEX16;//TODO: or better set to 0?
                result      = false;
                MCore::LogWarning("Vertex index '%i'(%i) not in unsigned short range. Cannot convert indices to 16-bit values.", i, oldVertexIndex);
            }
        }

        // realloc the memory to the new index buffer size using 16-bit values and return the result
        m_indices = (uint32*)MCore::AlignedRealloc(m_indices, sizeof(uint16) * m_numIndices, 32, EMFX_MEMCATEGORY_GEOMETRY_MESHES, Mesh::MEMORYBLOCK_ID);
        return result;
    }

    // extract the original vertex positions
    void Mesh::ExtractOriginalVertexPositions(AZStd::vector<AZ::Vector3>& outPoints) const
    {
        // allocate space
        outPoints.resize(m_numOrgVerts);

        // get the mesh data
        const AZ::Vector3*      positions = (AZ::Vector3*)FindOriginalVertexData(ATTRIB_POSITIONS);
        const uint32*           orgVerts  = (uint32*) FindVertexData(ATTRIB_ORGVTXNUMBERS);

        // init all org vertices
        for (uint32 v = 0; v < m_numOrgVerts; ++v)
        {
            outPoints[v] = positions[0]; // init them, as there are some unused original vertices sometimes
        }
        // output the points
        for (uint32 i = 0; i < m_numVertices; ++i)
        {
            outPoints[ orgVerts[i] ] = positions[i];
        }
    }


    // calculate the normals
    void Mesh::CalcNormals(bool useDuplicates)
    {
        AZ::Vector3* positions = (AZ::Vector3*)FindOriginalVertexData(Mesh::ATTRIB_POSITIONS);
        AZ::Vector3* normals = (AZ::Vector3*)FindOriginalVertexData(Mesh::ATTRIB_NORMALS);
        //uint32*       indices     = GetIndices();     // the indices (face data)

        // if we want to use the original mesh only
        if (useDuplicates == false)
        {
            // the smoothed normals array
            AZStd::vector<AZ::Vector3>    smoothNormals(m_numOrgVerts);
            for (uint32 i = 0; i < m_numOrgVerts; ++i)
            {
                smoothNormals[i] = AZ::Vector3::CreateZero();
            }

            // sum all face normals
            uint32* orgVerts = (uint32*)FindOriginalVertexData(Mesh::ATTRIB_ORGVTXNUMBERS);

            // iterate over all polygons, triangulate internally
            const uint32* indices       = GetIndices();
            const uint8* vertCounts     = GetPolygonVertexCounts();
            uint32 indexA, indexB, indexC;
            uint32 polyStartIndex = 0;
            const uint32 numPolygons = GetNumPolygons();
            for (uint32 p = 0; p < numPolygons; p++)
            {
                const uint32 numPolyVerts = vertCounts[p];

                // iterate over all triangles inside this polygon
                // explanation: numPolyVerts-2 == number of triangles
                // 3 verts=1 triangle, 4 verts=2 triangles, etc
                for (uint32 i = 2; i < numPolyVerts; i++)
                {
                    indexA = indices[polyStartIndex + i - 1];
                    indexB = indices[polyStartIndex + i];
                    indexC = indices[polyStartIndex];

                    const AZ::Vector3& posA = positions[ indexA ];
                    const AZ::Vector3& posB = positions[ indexB ];
                    const AZ::Vector3& posC = positions[ indexC ];
                    AZ::Vector3 faceNormal = (posB - posA).Cross(posC - posB).GetNormalizedSafe();

                    // store the tangents in the orgTangents array
                    smoothNormals[ orgVerts[indexA] ] += faceNormal;
                    smoothNormals[ orgVerts[indexB] ] += faceNormal;
                    smoothNormals[ orgVerts[indexC] ] += faceNormal;
                }

                polyStartIndex += numPolyVerts;
            }
            // normalize
            for (uint32 i = 0; i < m_numOrgVerts; ++i)
            {
                smoothNormals[i].NormalizeSafe();
            }

            for (uint32 i = 0; i < m_numVertices; ++i)
            {
                normals[i] = smoothNormals[ orgVerts[i] ];
            }
        }
        else
        {
            for (uint32 i = 0; i < m_numVertices; ++i)
            {
                normals[i] = AZ::Vector3::CreateZero();
            }

            // iterate over all polygons, triangulate internally
            const uint32* indices       = GetIndices();
            const uint8* vertCounts     = GetPolygonVertexCounts();
            uint32 indexA, indexB, indexC;
            uint32 polyStartIndex = 0;
            const uint32 numPolygons = GetNumPolygons();
            for (uint32 p = 0; p < numPolygons; p++)
            {
                const uint32 numPolyVerts = vertCounts[p];

                // iterate over all triangles inside this polygon
                // explanation: numPolyVerts-2 == number of triangles
                // 3 verts=1 triangle, 4 verts=2 triangles, etc
                for (uint32 i = 2; i < numPolyVerts; i++)
                {
                    indexA = indices[polyStartIndex + i - 1];
                    indexB = indices[polyStartIndex + i];
                    indexC = indices[polyStartIndex];

                    const AZ::Vector3& posA = positions[ indexA ];
                    const AZ::Vector3& posB = positions[ indexB ];
                    const AZ::Vector3& posC = positions[ indexC ];
                    AZ::Vector3 faceNormal = (posB - posA).Cross(posC - posB).GetNormalizedSafe();

                    // store the tangents in the orgTangents array
                    normals[indexA] = normals[indexA] + faceNormal;
                    normals[indexB] = normals[indexB] + faceNormal;
                    normals[indexC] = normals[indexC] + faceNormal;
                }

                polyStartIndex += numPolyVerts;
            }
            // normalize the normals
            for (uint32 i = 0; i < m_numVertices; ++i)
            {
                normals[i].NormalizeSafe();
            }
        }
    }


    // check if we are a triangle mesh
    bool Mesh::CheckIfIsTriangleMesh() const
    {
        for (uint32 i = 0; i < m_numPolygons; ++i)
        {
            if (m_polyVertexCounts[i] != 3)
            {
                return false;
            }
        }

        return true;
    }


    // check if we are a quad mesh
    bool Mesh::CheckIfIsQuadMesh() const
    {
        for (uint32 i = 0; i < m_numPolygons; ++i)
        {
            if (m_polyVertexCounts[i] != 4)
            {
                return false;
            }
        }

        return true;
    }


    // calculate how many triangles it would take to draw this mesh
    uint32 Mesh::CalcNumTriangles() const
    {
        uint32 numTriangles = 0;

        const uint8* polyVertexCounts = GetPolygonVertexCounts();
        const uint32 numPolygons = GetNumPolygons();
        for (uint32 i = 0; i < numPolygons; ++i)
        {
            numTriangles += (polyVertexCounts[i] - 2); // 3 verts=1 triangle, 4 verts=2 triangles, 5 verts=3 triangles, etc
        }
        return numTriangles;
    }


    void Mesh::ReserveVertexAttributeLayerSpace(uint32 numLayers)
    {
        m_vertexAttributes.reserve(numLayers);
    }


    // scale all positional data
    void Mesh::Scale(float scaleFactor)
    {
        for (VertexAttributeLayer* layer : m_vertexAttributes)
        {
            layer->Scale(scaleFactor);
        }

        for (VertexAttributeLayer* layer : m_sharedVertexAttributes)
        {
            layer->Scale(scaleFactor);
        }

        // scale the positional data
        AZ::Vector3* positions       = (AZ::Vector3*)FindVertexData(ATTRIB_POSITIONS);
        AZ::Vector3* orgPositions    = (AZ::Vector3*)FindOriginalVertexData(ATTRIB_POSITIONS);

        const uint32 numVerts = m_numVertices;
        for (uint32 i = 0; i < numVerts; ++i)
        {
            positions[i] = positions[i] * scaleFactor;
            orgPositions[i] = orgPositions[i] * scaleFactor;
        }
    }


    // find by name
    size_t Mesh::FindVertexAttributeLayerIndexByName(const char* name) const
    {
        const auto foundLayer = AZStd::find_if(begin(m_vertexAttributes), end(m_vertexAttributes), [name](const VertexAttributeLayer* layer)
        {
            return layer->GetNameString() == name;
        });
        return foundLayer != end(m_vertexAttributes) ? AZStd::distance(begin(m_vertexAttributes), foundLayer) : InvalidIndex;
    }


    // find by name as string
    size_t Mesh::FindVertexAttributeLayerIndexByNameString(const AZStd::string& name) const
    {
        const auto foundLayer = AZStd::find_if(begin(m_vertexAttributes), end(m_vertexAttributes), [name](const VertexAttributeLayer* layer)
        {
            return layer->GetNameString() == name;
        });
        return foundLayer != end(m_vertexAttributes) ? AZStd::distance(begin(m_vertexAttributes), foundLayer) : InvalidIndex;
    }


    // find by name ID
    size_t Mesh::FindVertexAttributeLayerIndexByNameID(uint32 nameID) const
    {
        const auto foundLayer = AZStd::find_if(begin(m_vertexAttributes), end(m_vertexAttributes), [nameID](const VertexAttributeLayer* layer)
        {
            return layer->GetNameID() == nameID;
        });
        return foundLayer != end(m_vertexAttributes) ? AZStd::distance(begin(m_vertexAttributes), foundLayer) : InvalidIndex;
    }


    // find by name
    size_t Mesh::FindSharedVertexAttributeLayerIndexByName(const char* name) const
    {
        const auto foundLayer = AZStd::find_if(begin(m_sharedVertexAttributes), end(m_sharedVertexAttributes), [name](const VertexAttributeLayer* layer)
        {
            return layer->GetNameString() == name;
        });
        return foundLayer != end(m_sharedVertexAttributes) ? AZStd::distance(begin(m_sharedVertexAttributes), foundLayer) : InvalidIndex;
    }


    // find by name as string
    size_t Mesh::FindSharedVertexAttributeLayerIndexByNameString(const AZStd::string& name) const
    {
        const auto foundLayer = AZStd::find_if(begin(m_sharedVertexAttributes), end(m_sharedVertexAttributes), [name](const VertexAttributeLayer* layer)
        {
            return layer->GetNameString() == name;
        });
        return foundLayer != end(m_sharedVertexAttributes) ? AZStd::distance(begin(m_sharedVertexAttributes), foundLayer) : InvalidIndex;
    }


    // find by name ID
    size_t Mesh::FindSharedVertexAttributeLayerIndexByNameID(uint32 nameID) const
    {
        const auto foundLayer = AZStd::find_if(begin(m_sharedVertexAttributes), end(m_sharedVertexAttributes), [nameID](const VertexAttributeLayer* layer)
        {
            return layer->GetNameID() == nameID;
        });
        return foundLayer != end(m_sharedVertexAttributes) ? AZStd::distance(begin(m_sharedVertexAttributes), foundLayer) : InvalidIndex;
    }
} // namespace EMotionFX
