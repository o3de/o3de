/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
    AZ_CLASS_ALLOCATOR_IMPL(Mesh, MeshAllocator, 0)

    Mesh::Mesh()
        : BaseObject()
    {
        mNumVertices        = 0;
        mNumIndices         = 0;
        mNumOrgVerts        = 0;
        mNumPolygons        = 0;
        mIndices            = nullptr;
        mPolyVertexCounts   = nullptr;
        mIsCollisionMesh    = false;

        // set memory categories of the arrays
        mSubMeshes.SetMemoryCategory(EMFX_MEMCATEGORY_GEOMETRY_MESHES);
        mVertexAttributes.SetMemoryCategory(EMFX_MEMCATEGORY_GEOMETRY_MESHES);
        mSharedVertexAttributes.SetMemoryCategory(EMFX_MEMCATEGORY_GEOMETRY_MESHES);
    }

    // allocation constructor
    Mesh::Mesh(uint32 numVerts, uint32 numIndices, uint32 numPolygons, uint32 numOrgVerts, bool isCollisionMesh)
    {
        mNumVertices        = 0;
        mNumIndices         = 0;
        mNumPolygons        = 0;
        mNumOrgVerts        = 0;
        mIndices            = nullptr;
        mPolyVertexCounts   = nullptr;
        mIsCollisionMesh    = isCollisionMesh;

        // set memory categories of the arrays
        mSubMeshes.SetMemoryCategory(EMFX_MEMCATEGORY_GEOMETRY_MESHES);
        mVertexAttributes.SetMemoryCategory(EMFX_MEMCATEGORY_GEOMETRY_MESHES);
        mSharedVertexAttributes.SetMemoryCategory(EMFX_MEMCATEGORY_GEOMETRY_MESHES);

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
            float x;
            float y;
        };

        struct Vector4
        {
            float x;
            float y;
            float z;
            float w;
        };

        // Needed for converting the packed vectors from the Atom buffers that normally load directly into GPU into AzCore versions used by EMFX.
        AZ::Vector2 ConvertVector(const Vector2& input)
        {
            return AZ::Vector2(input.x, input.y);
        }

        AZ::Vector3 ConvertVector(const AZ::PackedVector3f& input)
        {
            return AZ::Vector3(input.GetX(), input.GetY(), input.GetZ());
        }

        AZ::Vector4 ConvertVector(const Vector4& input)
        {
            return AZ::Vector4(input.x, input.y, input.z, input.w);
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
            size_t addedElements = 0;
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
        for (const AZ::RPI::ModelLodAsset::Mesh& mesh : sourceModelLod->GetMeshes())
        {
            modelVertexCount += mesh.GetVertexCount();
            modelIndexCount += mesh.GetIndexCount();
        }

        // IndicesPerFace defined in atom is 3.
        const AZ::u32 numPolygons = modelIndexCount / 3;
        Mesh* mesh = Create(modelVertexCount, modelIndexCount, numPolygons, modelVertexCount, false);

        // The lod has shared buffers that combine the data from each submesh.
        // These buffers can be accessed through the first submesh in their entirety
        // by using the BufferViewDescriptor from the Buffer instead of the one from the sub-mesh's BufferAssetView
        const AZ::RPI::ModelLodAsset::Mesh& sourceMesh = sourceModelLod->GetMeshes()[0];

        // Copy the index buffer for the entire lod
        AZStd::array_view<uint8_t> indexBuffer = sourceMesh.GetIndexBufferAssetView().GetBufferAsset()->GetBuffer();
        const AZ::RHI::BufferViewDescriptor& indexBufferViewDescriptor = sourceMesh.GetIndexBufferAssetView().GetBufferAsset()->GetBufferViewDescriptor();
        AZ_ErrorOnce("EMotionFX", indexBufferViewDescriptor.m_elementSize == 4, "Index buffer must stored as 4 bytes.");
        const size_t indexBufferCountsInBytes = indexBufferViewDescriptor.m_elementCount * indexBufferViewDescriptor.m_elementSize;
        const size_t indexBufferOffsetInBytes = indexBufferViewDescriptor.m_elementOffset * indexBufferViewDescriptor.m_elementSize;
        memcpy(mesh->mIndices, indexBuffer.begin() + indexBufferOffsetInBytes, indexBufferCountsInBytes);

        // Set the polygon buffer
        AZStd::fill(mesh->mPolyVertexCounts, mesh->mPolyVertexCounts + mesh->mNumPolygons, 3);

        // Skinning data from atom are stored in two separate buffer layer.
        AZ::u8 maxSkinInfluences = 255; // Later we will calculate this value from skinning data.
        const AZ::u16* skinJointIndices = nullptr;
        const float* skinWeights = nullptr;

        // Copy the vertex buffers
        for (const AZ::RPI::ModelLodAsset::Mesh::StreamBufferInfo& streamBufferInfo : sourceMesh.GetStreamBufferInfoList())
        {
            const AZ::RHI::BufferViewDescriptor& bufferAssetViewDescriptor = streamBufferInfo.m_bufferAssetView.GetBufferAsset()->GetBufferViewDescriptor();
            const size_t elementCountInBytes = bufferAssetViewDescriptor.m_elementSize * bufferAssetViewDescriptor.m_elementCount;
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
            else if (name == AZ::Name("COLOR"))
            {
                AtomMeshHelpers::CreateAndAddVertexAttributeLayer<AZ::Vector4, AtomMeshHelpers::Vector4>(sourceModelLod, modelVertexCount,
                    name, bufferData,
                    mesh, Mesh::ATTRIB_COLORS128, /*keepOriginals=*/false,
                    AZ::Vector4(0.0f, 0.0f, 0.0f, 0.0f));
            }
            else if (name == AZ::Name("SKIN_JOINTINDICES"))
            {
                // Atom stores the skin indices as uint16, but the buffer itself is a buffer of uint32 with two id's per element
                size_t influenceCount = elementCountInBytes / sizeof(AZ::u16);
                maxSkinInfluences = influenceCount / modelVertexCount;
                AZ_Assert(maxSkinInfluences > 0 && maxSkinInfluences < 100, "Expect max skin influences in a reasonable value range.");
                AZ_Assert(influenceCount % modelVertexCount == 0, "Expect an equal number of influences for each vertex.");
                AZ_Assert(bufferAssetViewDescriptor.m_elementSize == 4, "Expect skin joint indices to be stored in a raw 32-bit per element buffer"); 

                // Multiply element offset by 2 here since m_elementOffset is referring to 32-bit elements
                // and the pointer we're offsetting is pointing to 16-bit data
                skinJointIndices = static_cast<const AZ::u16*>(bufferData) + (bufferAssetViewDescriptor.m_elementOffset * 2);
            }
            else if (name == AZ::Name("SKIN_WEIGHTS"))
            {
                // Atom stores joint weights as float (range 0 - 1)
                size_t influenceCount = elementCountInBytes / sizeof(float);
                maxSkinInfluences = influenceCount / modelVertexCount;
                AZ_Assert(maxSkinInfluences > 0 && maxSkinInfluences < 100, "Expect max skin influences in a reasonable value range.");
                skinWeights = static_cast<const float*>(bufferData) + bufferAssetViewDescriptor.m_elementOffset;
            }
        }

        // Add the original vertex layer
        VertexAttributeLayerAbstractData* originalVertexData = VertexAttributeLayerAbstractData::Create(modelVertexCount, Mesh::ATTRIB_ORGVTXNUMBERS, sizeof(AZ::u32), false);
        AZ::u32* originalVertexDataRaw = static_cast<AZ::u32*>(originalVertexData->GetData());
        for (size_t i = 0; i < modelVertexCount; ++i)
        {
            originalVertexDataRaw[i] = i;
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

            // Fill in skinning data from atom buffer
            for (uint32 v = 0; v < modelVertexCount; ++v)
            {
                for (uint32 i = 0; i < maxSkinInfluences; ++i)
                {
                    const float weight = skinWeights[v * maxSkinInfluences + i];
                    if (!AZ::IsClose(weight, 0.0f, FLT_EPSILON))
                    {
                        const AZ::u16 skinJointIndex = skinJointIndices[v * maxSkinInfluences + i];
                        if (skinToSkeletonIndexMap.find(skinJointIndex) == skinToSkeletonIndexMap.end())
                        {
                            AZ_WarningOnce("EMotionFX", false, "Missing skin influences for index %d", skinJointIndex);
                            continue;
                        }

                        const AZ::u16 skeltonJointIndex = skinToSkeletonIndexMap.at(skinJointIndex);
                        skinningLayer->AddInfluence(v, skeltonJointIndex, weight, 0);
                    }
                }
            }
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
                /*materialIndex*/0,
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
            mIndices            = (uint32*)MCore::AlignedAllocate(sizeof(uint32) * numIndices,  32, EMFX_MEMCATEGORY_GEOMETRY_MESHES, Mesh::MEMORYBLOCK_ID);
            mPolyVertexCounts   = (uint8*)MCore::AlignedAllocate(sizeof(uint8) * numPolygons, 16, EMFX_MEMCATEGORY_GEOMETRY_MESHES, Mesh::MEMORYBLOCK_ID);
        }

        // set number values
        mNumVertices    = numVerts;
        mNumPolygons    = numPolygons;
        mNumIndices     = numIndices;
        mNumOrgVerts    = numOrgVerts;
    }


    // copy all original data over the output data
    void Mesh::ResetToOriginalData()
    {
        const uint32 numLayers = mVertexAttributes.GetLength();
        for (uint32 i = 0; i < numLayers; ++i)
        {
            mVertexAttributes[i]->ResetToOriginalData();
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
        const uint32 numSubMeshes = mSubMeshes.GetLength();
        for (uint32 i = 0; i < numSubMeshes; ++i)
        {
            mSubMeshes[i]->Destroy();
        }
        mSubMeshes.Clear();

        if (mIndices)
        {
            MCore::AlignedFree(mIndices);
        }

        if (mPolyVertexCounts)
        {
            MCore::AlignedFree(mPolyVertexCounts);
        }

        // re-init members
        mIndices        = nullptr;
        mPolyVertexCounts = nullptr;
        mNumIndices     = 0;
        mNumVertices    = 0;
        mNumOrgVerts    = 0;
        mNumPolygons    = 0;
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
        uint32 i, f;
        AZ::Vector4* tangents = nullptr;
        AZ::Vector4* orgTangents = nullptr;
        AZ::Vector3* bitangents = nullptr;
        AZ::Vector3* orgBitangents = nullptr;
        const uint32 numTangentLayers = CalcNumAttributeLayers(Mesh::ATTRIB_TANGENTS);

        // make sure we have tangent data allocated for all uv layers before the given one
        for (i = numTangentLayers; i <= uvSet; ++i)
        {
            // add a new tangent layer
            AddVertexAttributeLayer(VertexAttributeLayerAbstractData::Create(mNumVertices, Mesh::ATTRIB_TANGENTS, sizeof(AZ::Vector4), true));
            tangents    = static_cast<AZ::Vector4*>(FindVertexData(Mesh::ATTRIB_TANGENTS, i));
            orgTangents = static_cast<AZ::Vector4*>(FindOriginalVertexData(Mesh::ATTRIB_TANGENTS, i));

            // Add the bitangents layer.
            if (storeBitangents)
            {
                AddVertexAttributeLayer(VertexAttributeLayerAbstractData::Create(mNumVertices, Mesh::ATTRIB_BITANGENTS, sizeof(AZ::PackedVector3f), true));
                bitangents    = static_cast<AZ::Vector3*>(FindVertexData(Mesh::ATTRIB_BITANGENTS, i));
                orgBitangents = static_cast<AZ::Vector3*>(FindOriginalVertexData(Mesh::ATTRIB_BITANGENTS, i));
            }

            // default all tangents for the newly created layer
            AZ::Vector4 defaultTangent(1.0f, 0.0f, 0.0f, 0.0f);
            AZ::Vector3 defaultBitangent(0.0f, 0.0f, 1.0f);
            for (uint32 vtx = 0; vtx < mNumVertices; ++vtx)
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
        for (i = 0; i < mNumVertices; ++i)
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
        for (f = 0; f < numPolygons; f++)
        {
            const uint32 numPolyVerts = vertCounts[f];

            // explanation: numPolyVerts-2 == number of triangles
            // triangle has got 3 polygon vertices  -> 1 triangle
            // quad has got 4 polygon vertices      -> 2 triangles
            // pentagon has got 5 polygon vertices  -> 3 triangles
            for (i = 2; i < numPolyVerts; i++)
            {
                indexA = indices[polyStartIndex];
                indexB = indices[polyStartIndex + i];
                indexC = indices[polyStartIndex + i - 1];

                // calculate the tangent and bitangent for the face
                CalcTangentAndBitangentForFace(positions[indexA], positions[indexB], positions[indexC],
                    uvData[indexA], uvData[indexB], uvData[indexC],
                    &curTangent, &curBitangent);

                // normalize the vectors
                curTangent = MCore::SafeNormalize(curTangent);
                curBitangent = MCore::SafeNormalize(curBitangent);

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
        for (i = 0; i < mNumVertices; ++i)
        {
            // get the normal
            AZ::Vector3 normal(normals[i]);
            normal = MCore::SafeNormalize(normal);

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
            fixedTangent = MCore::SafeNormalize(fixedTangent);

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
    void Mesh::GatherBonesForFace(uint32 startIndexOfFace, MCore::Array<Node*>& outBones, Actor* actor)
    {
        // get rid of existing data
        outBones.Clear();

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
                if (outBones.Find(bone) == MCORE_INVALIDINDEX32)
                {
                    outBones.Add(bone);
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
    uint32 Mesh::CalcMaxNumInfluences() const
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
        return aznumeric_cast<uint32>(maxInfluences);
    }


    // returns the maximum number of weights/influences for this mesh plus some extra information
    uint32 Mesh::CalcMaxNumInfluences(AZStd::vector<uint32>& outVertexCounts) const
    {
        size_t maxInfluences = 0;

        // Reset values.
        outVertexCounts.resize(CalcMaxNumInfluences() + 1);
        for (size_t j = 0; j < outVertexCounts.size(); ++j)
        {
            outVertexCounts[j] = 0;
        }

        // Does the mesh have a skinning layer? If no we can quit directly as this means there are only unskinned vertices.
        SkinningInfoVertexAttributeLayer* skinningLayer = (SkinningInfoVertexAttributeLayer*)FindSharedVertexAttributeLayer(SkinningInfoVertexAttributeLayer::TYPE_ID);
        if (!skinningLayer)
        {
            outVertexCounts[0] = GetNumVertices();
            return aznumeric_cast<uint32>(maxInfluences);
        }

        uint32* orgVerts = (uint32*)FindVertexData(Mesh::ATTRIB_ORGVTXNUMBERS);

        // Get the vertex counts for the influences.
        const uint32 numVerts = GetNumVertices();
        for (uint32 i = 0; i < numVerts; ++i)
        {
            uint32 orgVertex = orgVerts[i];

            // Increase the number of vertices for the given influence value.
            const size_t numInfluences = skinningLayer->GetNumInfluences(orgVertex);
            outVertexCounts[numInfluences]++;

            // Update the number of max influences.
            maxInfluences = AZStd::max(maxInfluences, numInfluences);
        }

        return aznumeric_cast<uint32>(maxInfluences);
    }


    // remove a given submesh
    void Mesh::RemoveSubMesh(uint32 nr, bool delFromMem)
    {
        SubMesh* subMesh = mSubMeshes[nr];
        mSubMeshes.Remove(nr);
        if (delFromMem)
        {
            subMesh->Destroy();
        }
    }


    // insert a given submesh
    void Mesh::InsertSubMesh(uint32 insertIndex, SubMesh* subMesh)
    {
        mSubMeshes.Insert(insertIndex, subMesh);
    }


    // count the given type of vertex attribute layers
    uint32 Mesh::CalcNumAttributeLayers(uint32 type) const
    {
        uint32 numLayers = 0;

        // check the types of all vertex attribute layers
        const uint32 numAttributes = mVertexAttributes.GetLength();
        for (uint32 i = 0; i < numAttributes; ++i)
        {
            if (mVertexAttributes[i]->GetType() == type)
            {
                numLayers++;
            }
        }

        return numLayers;
    }


    // get the number of UV layers
    uint32 Mesh::CalcNumUVLayers() const
    {
        return CalcNumAttributeLayers(Mesh::ATTRIB_UVCOORDS);
    }

    //---------------------------------------------------------------

    VertexAttributeLayer* Mesh::GetSharedVertexAttributeLayer(uint32 layerNr)
    {
        MCORE_ASSERT(layerNr < mSharedVertexAttributes.GetLength());
        return mSharedVertexAttributes[layerNr];
    }


    void Mesh::AddSharedVertexAttributeLayer(VertexAttributeLayer* layer)
    {
        MCORE_ASSERT(mSharedVertexAttributes.Contains(layer) == false);
        mSharedVertexAttributes.Add(layer);
    }


    uint32 Mesh::GetNumSharedVertexAttributeLayers() const
    {
        return mSharedVertexAttributes.GetLength();
    }


    uint32 Mesh::FindSharedVertexAttributeLayerNumber(uint32 layerTypeID, uint32 occurrence) const
    {
        uint32 layerCounter = 0;

        // check all vertex attributes of our first vertex, and find where the specific attribute is
        const uint32 numLayers = mSharedVertexAttributes.GetLength();
        for (uint32 i = 0; i < numLayers; ++i)
        {
            VertexAttributeLayer* layer = mSharedVertexAttributes[i];
            if (layer->GetType() == layerTypeID)
            {
                if (occurrence == layerCounter)
                {
                    return i;
                }

                layerCounter++;
            }
        }

        // not found
        return MCORE_INVALIDINDEX32;
    }


    // find the vertex attribute layer and return a pointer
    VertexAttributeLayer* Mesh::FindSharedVertexAttributeLayer(uint32 layerTypeID, uint32 occurence) const
    {
        uint32 layerNr = FindSharedVertexAttributeLayerNumber(layerTypeID, occurence);
        if (layerNr == MCORE_INVALIDINDEX32)
        {
            return nullptr;
        }

        return mSharedVertexAttributes[layerNr];
    }



    // delete all shared attribute layers
    void Mesh::RemoveAllSharedVertexAttributeLayers()
    {
        while (mSharedVertexAttributes.GetLength())
        {
            mSharedVertexAttributes.GetLast()->Destroy();
            mSharedVertexAttributes.RemoveLast();
        }
    }


    // remove a layer by its index
    void Mesh::RemoveSharedVertexAttributeLayer(uint32 layerNr)
    {
        MCORE_ASSERT(layerNr < mSharedVertexAttributes.GetLength());
        mSharedVertexAttributes[layerNr]->Destroy();
        mSharedVertexAttributes.Remove(layerNr);
    }


    uint32 Mesh::GetNumVertexAttributeLayers() const
    {
        return mVertexAttributes.GetLength();
    }


    VertexAttributeLayer* Mesh::GetVertexAttributeLayer(uint32 layerNr)
    {
        MCORE_ASSERT(layerNr < mVertexAttributes.GetLength());
        return mVertexAttributes[layerNr];
    }


    void Mesh::AddVertexAttributeLayer(VertexAttributeLayer* layer)
    {
        MCORE_ASSERT(mVertexAttributes.Contains(layer) == false);
        mVertexAttributes.Add(layer);
    }


    // find the layer number
    uint32 Mesh::FindVertexAttributeLayerNumber(uint32 layerTypeID, uint32 occurrence) const
    {
        uint32 layerCounter = 0;

        // check all vertex attributes of our first vertex, and find where the specific attribute is
        const uint32 numLayers = mVertexAttributes.GetLength();
        for (uint32 i = 0; i < numLayers; ++i)
        {
            VertexAttributeLayer* layer = mVertexAttributes[i];
            if (layer->GetType() == layerTypeID)
            {
                if (occurrence == layerCounter)
                {
                    return i;
                }

                layerCounter++;
            }
        }

        // not found
        return MCORE_INVALIDINDEX32;
    }


    // find the layer number
    uint32 Mesh::FindVertexAttributeLayerNumberByName(uint32 layerTypeID, const char* name) const
    {
        // check all vertex attributes of our first vertex, and find where the specific attribute is
        const uint32 numLayers = mVertexAttributes.GetLength();
        for (uint32 i = 0; i < numLayers; ++i)
        {
            VertexAttributeLayer* layer = mVertexAttributes[i];
            if (layer->GetType() == layerTypeID)
            {
                if (layer->GetNameString() == name)
                {
                    return i;
                }
            }
        }

        return MCORE_INVALIDINDEX32;
    }



    // find the vertex attribute layer and return a pointer
    VertexAttributeLayer* Mesh::FindVertexAttributeLayer(uint32 layerTypeID, uint32 occurence) const
    {
        const uint32 layerNr = FindVertexAttributeLayerNumber(layerTypeID, occurence);
        if (layerNr == MCORE_INVALIDINDEX32)
        {
            return nullptr;
        }

        return mVertexAttributes[layerNr];
    }


    // find the vertex attribute layer and return a pointer
    VertexAttributeLayer* Mesh::FindVertexAttributeLayerByName(uint32 layerTypeID, const char* name) const
    {
        const uint32 layerNr = FindVertexAttributeLayerNumberByName(layerTypeID, name);
        if (layerNr == MCORE_INVALIDINDEX32)
        {
            return nullptr;
        }

        return mVertexAttributes[layerNr];
    }


    void Mesh::RemoveAllVertexAttributeLayers()
    {
        while (mVertexAttributes.GetLength())
        {
            mVertexAttributes.GetLast()->Destroy();
            mVertexAttributes.RemoveLast();
        }
    }


    void Mesh::RemoveVertexAttributeLayer(uint32 layerNr)
    {
        MCORE_ASSERT(layerNr < mVertexAttributes.GetLength());
        mVertexAttributes[layerNr]->Destroy();
        mVertexAttributes.Remove(layerNr);
    }



    // clone the mesh
    Mesh* Mesh::Clone()
    {
        // allocate a mesh of the same dimensions
        Mesh* clone = aznew Mesh(mNumVertices, mNumIndices, mNumPolygons, mNumOrgVerts, mIsCollisionMesh);

        // copy the mesh data
        MCore::MemCopy(clone->mIndices, mIndices, sizeof(uint32) * mNumIndices);
        MCore::MemCopy(clone->mPolyVertexCounts, mPolyVertexCounts, sizeof(uint8) * mNumPolygons);

        // copy the submesh data
        uint32 i;
        const uint32 numSubMeshes = mSubMeshes.GetLength();
        clone->mSubMeshes.Resize(numSubMeshes);
        for (i = 0; i < numSubMeshes; ++i)
        {
            clone->mSubMeshes[i] = mSubMeshes[i]->Clone(clone);
        }

        // clone the shared vertex attributes
        const uint32 numSharedAttributes = mSharedVertexAttributes.GetLength();
        clone->mSharedVertexAttributes.Resize(numSharedAttributes);
        for (i = 0; i < numSharedAttributes; ++i)
        {
            clone->mSharedVertexAttributes[i] = mSharedVertexAttributes[i]->Clone();
        }

        // clone the non-shared vertex attributes
        const uint32 numAttributes = mVertexAttributes.GetLength();
        clone->mVertexAttributes.Resize(numAttributes);
        for (i = 0; i < numAttributes; ++i)
        {
            clone->mVertexAttributes[i] = mVertexAttributes[i]->Clone();
        }

        // return the resulting cloned mesh
        return clone;
    }


    // swap the data for two vertices
    void Mesh::SwapVertex(uint32 vertexA, uint32 vertexB)
    {
        MCORE_ASSERT(vertexA < mNumVertices);
        MCORE_ASSERT(vertexB < mNumVertices);

        // if we try to swap itself then there is nothing to do
        if (vertexA == vertexB)
        {
            return;
        }

        // swap all vertex attribute layers
        const uint32 numLayers = mVertexAttributes.GetLength();
        for (uint32 i = 0; i < numLayers; ++i)
        {
            mVertexAttributes[i]->SwapAttributes(vertexA, vertexB);
        }
    }

    /*
    // remove indexed null triangles (triangles that use 2 or 3 of the same vertices, so which are invisible)
    uint32 Mesh::RemoveIndexedNullTriangles(bool removeEmptySubMeshes)
    {
    uint32 numRemoved = 0;
    uint32 i;

    // for all triangles
    uint32 numIndices = mNumIndices;
    uint32 offset = 0;
    for (i=0; i<mNumIndices; i+=3)
    {
    uint32 indexA = mIndices[offset];
    uint32 indexB = mIndices[offset+1];
    uint32 indexC = mIndices[offset+2];

    // if we need to remove this triangle
    if (indexA == indexB || indexA == indexC || indexB == indexC)
    {
    // re-arrange the array in memory
    uint32 numBytesToMove = (mNumIndices - (offset+3)) * sizeof(uint32);
    if (numBytesToMove > 0)
    MCore::MemMove(((uint8*)mIndices + (offset * sizeof(uint32))), ((uint8*)mIndices + (offset+3)*sizeof(uint32)), numBytesToMove);

    numRemoved++;
    numIndices -= 3;

    // adjust all submesh start index offsets changed
    //const uint32 numSubMeshes = mSubMeshes.GetLength();
    for (uint32 s=0; s<mSubMeshes.GetLength(); )
    {
    SubMesh* subMesh = mSubMeshes[s];

    // if this isn't the last submesh
    if (s < mSubMeshes.GetLength() - 1)
    {
    // if we remove a triangle from this submesh
    if (subMesh->GetStartIndex() <= offset && mSubMeshes[s+1]->GetStartIndex() > offset)
    subMesh->SetNumIndices( subMesh->GetNumIndices() - 3 );
    }
    else
    {
    if (subMesh->GetStartIndex() <= offset)
    subMesh->SetNumIndices( subMesh->GetNumIndices() - 3 );
    }

    // now find out if we need to adjust the index offset of the submesh
    if (subMesh->GetStartIndex() >= offset)
    {
    if (subMesh->GetStartIndex() != offset)
    subMesh->SetStartIndex( subMesh->GetStartIndex() - 3 );
    }


    // remove the submesh if it's empty
    if (subMesh->GetNumIndices() == 0 && removeEmptySubMeshes)
    mSubMeshes.Remove(s);
    else
    s++;

    }
    }   // if we gotta remove
    else
    offset += 3;
    }

    // reallocate the array, if we removed anything
    if (numIndices != mNumIndices)
    mIndices = (uint32*)MCore::AlignedRealloc(mIndices, sizeof(uint32) * numIndices, mNumIndices*sizeof(uint32), 32, EMFX_MEMCATEGORY_GEOMETRY_MESHES, Mesh::MEMORYBLOCK_ID);

    // update the number of indices
    MCORE_ASSERT(numRemoved == (mNumIndices - numIndices) / 3);
    mNumIndices = numIndices;

    // return the number of removed triangles
    return numRemoved;
    }
    */

    // remove vertex data from the mesh
    void Mesh::RemoveVertices(uint32 startVertexNr, uint32 endVertexNr, bool changeIndexBuffer, bool removeEmptySubMeshes)
    {
        // perform some checks on the input data
        MCORE_ASSERT(endVertexNr < mNumVertices);
        MCORE_ASSERT(startVertexNr < mNumVertices);

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
        mNumVertices -= numVertsToRemove;

        // remove the attributes from the vertex attribute layers
        const uint32 numLayers = GetNumVertexAttributeLayers();
        for (uint32 i = 0; i < numLayers; ++i)
        {
            GetVertexAttributeLayer(i)->RemoveAttributes(startVertexNr, endVertexNr);
        }

        //------------------------------------------------------------------------
        // Fix the submesh number of vertices and start vertex offset values
        //------------------------------------------------------------------------
        uint32 numRemoved = 0;
        const uint32 v = startVertexNr;
        for (uint32 w = 0; w < numVertsToRemove; ++w)
        {
            // adjust all submesh start index offsets changed
            for (uint32 s = 0; s < mSubMeshes.GetLength();)
            {
                SubMesh* subMesh = mSubMeshes[s];

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
                    mSubMeshes.Remove(s);
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
            for (uint32 i = 0; i < mNumIndices; ++i)
            {
                if (mIndices[i] > startVertexNr)
                {
                    mIndices[i] -= numVertsToRemove;
                }
            }
        }
    }


    // remove empty submeshes
    uint32 Mesh::RemoveEmptySubMeshes(bool onlyRemoveOnZeroVertsAndTriangles)
    {
        uint32 numRemoved = 0;

        // for all the submeshes
        for (uint32 i = 0; i < mSubMeshes.GetLength();)
        {
            SubMesh* subMesh = mSubMeshes[i];

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
                mSubMeshes.Remove(i);
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
    void* Mesh::FindVertexData(uint32 layerID, uint32 occurrence) const
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
    void* Mesh::FindOriginalVertexData(uint32 layerID, uint32 occurrence) const
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


    void Mesh::CalcAABB(MCore::AABB* outBoundingBox, const Transform& transform, uint32 vertexFrequency)
    {
        MCORE_ASSERT(vertexFrequency >= 1);

        // init the bounding box
        outBoundingBox->Init();

        // get the position data
        AZ::Vector3* positions = (AZ::Vector3*)FindVertexData(ATTRIB_POSITIONS);

        const uint32 numVerts = GetNumVertices();
        for (uint32 i = 0; i < numVerts; i += vertexFrequency)
        {
            outBoundingBox->Encapsulate(transform.TransformPoint(positions[i]));
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
        uint32 i;

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
        const uint32 numSubMeshes = GetNumSubMeshes();
        for (uint32 s = 0; s < numSubMeshes; ++s)
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
            MCore::LogDebug("     + MaterialNr   = %d", subMesh->GetMaterial());

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
            const uint32 numBones = subMesh->GetNumBones();
            for (i = 0; i < numBones; ++i)
            {
                const uint32 nodeNr   = subMesh->GetBone(i);
                MCore::LogDebug("         + NodeNr %d", nodeNr);
            }
        }
    }


    // check for a given mesh how we categorize it
    Mesh::EMeshType Mesh::ClassifyMeshType(uint32 lodLevel, Actor* actor, uint32 nodeIndex, bool forceCPUSkinning, uint32 maxInfluences, uint32 maxBonesPerSubMesh) const
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
                    uint32 meshMaxInfluences = mesh->CalcMaxNumInfluences();
                    if (meshMaxInfluences > maxInfluences)
                    {
                        MCore::LogWarning("*** PERFORMANCE WARNING *** Mesh for node '%s' in geometry LOD %d uses more than %d (%d) bones. Forcing CPU deforms for this mesh.", node->GetName(), lodLevel, maxInfluences, meshMaxInfluences);
                        return MESHTYPE_CPU_DEFORMED;
                    }

                    // check if there is any submesh with more than the given number of bones, which would mean we cannot skin on the GPU
                    // then force CPU skinning as well
                    const uint32 numSubMeshes = mesh->GetNumSubMeshes();
                    for (uint32 i = 0; i < numSubMeshes; ++i)
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
        if (mIndices == nullptr)
        {
            return false;
        }

        // use our 32-bit index buffer as new 16-bit index array directly
        uint16* indices = (uint16*)mIndices;

        // iterate over all indices and convert the values
        for (uint32 i = 0; i < mNumIndices; ++i)
        {
            // create a temporary copy of our 32-bit vertex index
            const uint32 oldVertexIndex = mIndices[i];

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
        mIndices = (uint32*)MCore::AlignedRealloc(mIndices, sizeof(uint16) * mNumIndices, 32, EMFX_MEMCATEGORY_GEOMETRY_MESHES, Mesh::MEMORYBLOCK_ID);
        return result;
    }


    // function to convert the 128bit colors into 32bit ones, if they exist
    void Mesh::ConvertTo32BitColors()
    {
        // get the colors
        MCore::RGBAColor*   colors128   = (MCore::RGBAColor*)FindOriginalVertexData(Mesh::ATTRIB_COLORS128);
        uint32*             colors32    = (uint32*)FindOriginalVertexData(Mesh::ATTRIB_COLORS32);

        // check if 32bit colors already exist or 128bit colors do not exist
        if (colors128 == nullptr || colors32)
        {
            return;
        }

        // get the number of original vertices
        const uint32 numVertices = GetNumVertices();

        // create new vertex attribute layer for the 32bit colors
        VertexAttributeLayerAbstractData* layer = VertexAttributeLayerAbstractData::Create(numVertices, Mesh::ATTRIB_COLORS32, sizeof(uint32));

        // fill the layer with the converted float colors
        uint32* data = (uint32*)layer->GetData();
        for (uint32 i = 0; i < numVertices; ++i)
        {
            data[i] = colors128[i].ToInt();
        }

        // add the new layer
        AddVertexAttributeLayer(layer);
    }


    // extract the original vertex positions
    void Mesh::ExtractOriginalVertexPositions(AZStd::vector<AZ::Vector3>& outPoints) const
    {
        // allocate space
        outPoints.resize(mNumOrgVerts);

        // get the mesh data
        const AZ::Vector3*      positions = (AZ::Vector3*)FindOriginalVertexData(ATTRIB_POSITIONS);
        const uint32*           orgVerts  = (uint32*) FindVertexData(ATTRIB_ORGVTXNUMBERS);

        // init all org vertices
        for (uint32 v = 0; v < mNumOrgVerts; ++v)
        {
            outPoints[v] = positions[0]; // init them, as there are some unused original vertices sometimes
        }
        // output the points
        for (uint32 i = 0; i < mNumVertices; ++i)
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
            AZStd::vector<AZ::Vector3>    smoothNormals(mNumOrgVerts);
            for (uint32 i = 0; i < mNumOrgVerts; ++i)
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
                    AZ::Vector3 faceNormal = MCore::SafeNormalize((posB - posA).Cross(posC - posB));

                    // store the tangents in the orgTangents array
                    smoothNormals[ orgVerts[indexA] ] += faceNormal;
                    smoothNormals[ orgVerts[indexB] ] += faceNormal;
                    smoothNormals[ orgVerts[indexC] ] += faceNormal;
                }

                polyStartIndex += numPolyVerts;
            }
            /*
            for (uint32 f=0; f<mNumIndices; f+=3)
            {
            // get the face indices
            const uint32 indexA = indices[f];
            const uint32 indexB = indices[f+1];
            const uint32 indexC = indices[f+2];

            const MCore::Vector3& posA = positions[ indexA ];
            const MCore::Vector3& posB = positions[ indexB ];
            const MCore::Vector3& posC = positions[ indexC ];
            MCore::Vector3 faceNormal = (posB - posA).Cross( posC - posB ).SafeNormalize();

            // store the tangents in the orgTangents array
            smoothNormals[ orgVerts[indexA] ] += faceNormal;
            smoothNormals[ orgVerts[indexB] ] += faceNormal;
            smoothNormals[ orgVerts[indexC] ] += faceNormal;
            }
            */
            // normalize
            for (uint32 i = 0; i < mNumOrgVerts; ++i)
            {
                smoothNormals[i] = MCore::SafeNormalize(smoothNormals[i]);
            }

            for (uint32 i = 0; i < mNumVertices; ++i)
            {
                normals[i] = smoothNormals[ orgVerts[i] ];
            }
        }
        else
        {
            for (uint32 i = 0; i < mNumVertices; ++i)
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
                    AZ::Vector3 faceNormal = MCore::SafeNormalize((posB - posA).Cross(posC - posB));

                    // store the tangents in the orgTangents array
                    normals[indexA] = normals[indexA] + faceNormal;
                    normals[indexB] = normals[indexB] + faceNormal;
                    normals[indexC] = normals[indexC] + faceNormal;
                }

                polyStartIndex += numPolyVerts;
            }
            /*
            // sum all face normals
            for (uint32 f=0; f<mNumIndices; f+=3)
            {
            // get the face indices
            const uint32 indexA = indices[f];
            const uint32 indexB = indices[f+1];
            const uint32 indexC = indices[f+2];

            const MCore::Vector3& posA = positions[ indexA ];
            const MCore::Vector3& posB = positions[ indexB ];
            const MCore::Vector3& posC = positions[ indexC ];
            MCore::Vector3 faceNormal = (posB - posA).Cross( posC - posB ).SafeNormalize();

            // store the tangents in the orgTangents array
            normals[indexA] += faceNormal;
            normals[indexB] += faceNormal;
            normals[indexC] += faceNormal;
            }
            */
            // normalize the normals
            for (uint32 i = 0; i < mNumVertices; ++i)
            {
                normals[i] = MCore::SafeNormalize(normals[i]);
            }
        }
    }


    // check if we are a triangle mesh
    bool Mesh::CheckIfIsTriangleMesh() const
    {
        for (uint32 i = 0; i < mNumPolygons; ++i)
        {
            if (mPolyVertexCounts[i] != 3)
            {
                return false;
            }
        }

        return true;
    }


    // check if we are a quad mesh
    bool Mesh::CheckIfIsQuadMesh() const
    {
        for (uint32 i = 0; i < mNumPolygons; ++i)
        {
            if (mPolyVertexCounts[i] != 4)
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
        mVertexAttributes.Reserve(numLayers);
    }


    // scale all positional data
    void Mesh::Scale(float scaleFactor)
    {
        // all unique layers
        const uint32 numLayers = GetNumVertexAttributeLayers();
        for (uint32 i = 0; i < numLayers; ++i)
        {
            GetVertexAttributeLayer(i)->Scale(scaleFactor);
        }

        // scale all shared layers
        const uint32 numSharedLayers = GetNumSharedVertexAttributeLayers();
        for (uint32 i = 0; i < numSharedLayers; ++i)
        {
            GetSharedVertexAttributeLayer(i)->Scale(scaleFactor);
        }

        // scale the positional data
        AZ::Vector3* positions       = (AZ::Vector3*)FindVertexData(ATTRIB_POSITIONS);
        AZ::Vector3* orgPositions    = (AZ::Vector3*)FindOriginalVertexData(ATTRIB_POSITIONS);

        const uint32 numVerts = mNumVertices;
        for (uint32 i = 0; i < numVerts; ++i)
        {
            positions[i] = positions[i] * scaleFactor;
            orgPositions[i] = orgPositions[i] * scaleFactor;
        }
    }


    // find by name
    uint32 Mesh::FindVertexAttributeLayerIndexByName(const char* name) const
    {
        const uint32 numLayers = mVertexAttributes.GetLength();
        for (uint32 i = 0; i < numLayers; ++i)
        {
            if (mVertexAttributes[i]->GetNameString() == name)
            {
                return i;
            }
        }

        return MCORE_INVALIDINDEX32;
    }


    // find by name as string
    uint32 Mesh::FindVertexAttributeLayerIndexByNameString(const AZStd::string& name) const
    {
        const uint32 numLayers = mVertexAttributes.GetLength();
        for (uint32 i = 0; i < numLayers; ++i)
        {
            if (mVertexAttributes[i]->GetNameString() == name)
            {
                return i;
            }
        }

        return MCORE_INVALIDINDEX32;
    }


    // find by name ID
    uint32 Mesh::FindVertexAttributeLayerIndexByNameID(uint32 nameID) const
    {
        const uint32 numLayers = mVertexAttributes.GetLength();
        for (uint32 i = 0; i < numLayers; ++i)
        {
            if (mVertexAttributes[i]->GetNameID() == nameID)
            {
                return i;
            }
        }

        return MCORE_INVALIDINDEX32;
    }


    // find by name
    uint32 Mesh::FindSharedVertexAttributeLayerIndexByName(const char* name) const
    {
        const uint32 numLayers = mSharedVertexAttributes.GetLength();
        for (uint32 i = 0; i < numLayers; ++i)
        {
            if (mSharedVertexAttributes[i]->GetNameString() == name)
            {
                return i;
            }
        }

        return MCORE_INVALIDINDEX32;
    }


    // find by name as string
    uint32 Mesh::FindSharedVertexAttributeLayerIndexByNameString(const AZStd::string& name) const
    {
        const uint32 numLayers = mSharedVertexAttributes.GetLength();
        for (uint32 i = 0; i < numLayers; ++i)
        {
            if (mSharedVertexAttributes[i]->GetNameString() == name)
            {
                return i;
            }
        }

        return MCORE_INVALIDINDEX32;
    }


    // find by name ID
    uint32 Mesh::FindSharedVertexAttributeLayerIndexByNameID(uint32 nameID) const
    {
        const uint32 numLayers = mSharedVertexAttributes.GetLength();
        for (uint32 i = 0; i < numLayers; ++i)
        {
            if (mSharedVertexAttributes[i]->GetNameID() == nameID)
            {
                return i;
            }
        }

        return MCORE_INVALIDINDEX32;
    }
} // namespace EMotionFX
