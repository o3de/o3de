/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/PackedVector3.h>
#include <AzCore/Debug/Trace.h>
#include <AzCore/Name/Name.h>
#include <AzCore/base.h>
#include <AzCore/std/containers/variant.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/utility/move.h>
#include <EMotionFX/Source/VertexAttributeLayerBuffer.h>
#include "EMotionFXConfig.h"
#include "Mesh.h"
#include "SubMesh.h"
#include "Node.h"
#include "SkinningInfoVertexAttributeLayer.h"
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
        // Atom meshes might have different vertex features like some might contain tangents or multiple UV sets while other meshes do not
        // have tangents or don't contain any UV set at all. The corresponding EMFX sub-meshes do not support that, so we need to pad the
        // vertex buffers.
        // @param[in] sourceModelLod Source model LOD asset to read the vertex buffers from.
        // @param[in] sourceBufferName The name of the buffer to translate into a vertex attribute layer.
        // @param[in] inputBufferData Pointer to the source buffer from the Atom model LOD asset. This contains the data to be translated
        // and copied over.
        // @param[in] vertexAttributeLayerTypeId The type ID of the attribute layer used to identify type of vertex data (positions,
        // normals, etc.)
        // @param[in] keepOriginals True in case the vertex elements change when deforming the mesh, false if not.
        // @param[in] defaultPaddingValue The value used for the sub-meshes in the EMFX that do not contain a given vertex feature in Atom
        // for the given mesh.
        template<typename TargetType, typename SourceType>
        AZStd::vector<TargetType> CreateBuffer(
            const AZ::Data::Asset<AZ::RPI::ModelLodAsset>& sourceModelLod,
            const AZ::Name& sourceBufferName,
            const void* inputBufferData,
            const TargetType& defaultPaddingValue)
        {
            AZStd::vector<TargetType> targetBuffer;

            // Fill the vertex attribute layer by iterating through the Atom meshes and copying over the vertex data for each.
            // [[maybe_unused]] size_t addedElements = 0;
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
                        targetBuffer.emplace_back(ConvertVector(atomMeshBuffer[vertexIndex]));
                    }
                }
                else
                {
                    AZ_Warning("EMotionFX", false, "Padding %s buffer for mesh %s. Mesh has %d vertices while buffer is empty.",
                        sourceBufferName.GetCStr(), atomMesh.GetName().GetCStr(), atomMeshVertexCount);

                    for (size_t vertexIndex = 0; vertexIndex < atomMeshVertexCount; ++vertexIndex)
                    {
                        targetBuffer.emplace_back(defaultPaddingValue);
                    }
                }
            }
            return targetBuffer;
        }
    };

    Mesh* Mesh::CreateFromModelLod(const AZ::Data::Asset<AZ::RPI::ModelLodAsset>& sourceModelLod, const AZStd::unordered_map<AZ::u16, AZ::u16>& skinToSkeletonIndexMap)
    {
        AZ::u32 modelVertexCount = 0;
        AZ::u32 modelIndexCount = 0;
        
        // Find the maximum skin influences across all meshes to use when pre-allocating memory
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
            }
        }
        AZ_Assert(maxSkinInfluences > 0 && maxSkinInfluences < 100, "Expect max skin influences in a reasonable value range.");

        // IndicesPerFace defined in atom is 3.
        const AZ::u32 numPolygons = modelIndexCount / 3;
        Mesh* mesh = Create(modelVertexCount, modelIndexCount, numPolygons, modelVertexCount, false);

        // The lod has shared buffers that combine the data from each submesh.
        // These buffers can be accessed through the first submesh in their entirety
        // by using the BufferViewDescriptor from the Buffer instead of the one from the sub-mesh's BufferAssetView
        const AZ::RPI::ModelLodAsset::Mesh& sourceMesh0 = sourceModelLod->GetMeshes()[0];

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
                auto buffer = AtomMeshHelpers::CreateBuffer<AZ::Vector3, AZ::PackedVector3f>(
                    sourceModelLod, name, bufferData, AZ::Vector3(0.0f, 0.0f, 0.0f));
                mesh->CreateVertexAttribute<AttributeType::Position>(AZStd::move(buffer), true);
            }
            else if (name == AZ::Name("NORMAL"))
            {
                auto buffer = AtomMeshHelpers::CreateBuffer<AZ::Vector3, AZ::PackedVector3f>(
                    sourceModelLod, name, bufferData, AZ::Vector3(1.0f, 0.0f, 0.0f));
                mesh->CreateVertexAttribute<AttributeType::Normal>(AZStd::move(buffer), true);
            }
            else if (name == AZ::Name("UV"))
            {
                auto buffer = AtomMeshHelpers::CreateBuffer<AZ::Vector2, AtomMeshHelpers::Vector2>(
                    sourceModelLod, name, bufferData, AZ::Vector2(1.0f, 0.0f));
                mesh->CreateVertexAttribute<AttributeType::UVCoords>(AZStd::move(buffer), false);
            }
            else if (name == AZ::Name("TANGENT"))
            {
                auto buffer = AtomMeshHelpers::CreateBuffer<AZ::Vector4, AtomMeshHelpers::Vector4>(
                    sourceModelLod,name, bufferData, AZ::Vector4(1.0f, 0.0f, 0.0f, 0.0f));
                mesh->CreateVertexAttribute<AttributeType::Tangent>(AZStd::move(buffer), true);
            }
            else if (name == AZ::Name("BITANGENT"))
            {
                auto buffer =  AtomMeshHelpers::CreateBuffer<AZ::Vector3, AZ::PackedVector3f>(
                    sourceModelLod, name, bufferData,AZ::Vector3(1.0f, 0.0f, 0.0f));
                mesh->CreateVertexAttribute<AttributeType::Bitangent>(AZStd::move(buffer), true);
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
        AZStd::vector<AZ::u32> orignalVertexData;
        for (AZ::u32 i = 0; i < modelVertexCount; ++i)
        {
            orignalVertexData.emplace_back(static_cast<AZ::u32>(i));
        }
        mesh->CreateVertexAttribute<AttributeType::OrginalVertexNumber>(AZStd::move(orignalVertexData), false);

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
        for (auto& vertexAttribute : m_vertexAttributeLayer)
        {
            AZStd::visit([](auto& attr) {
                if(attr) {
                    attr->ResetToOriginalData();
                }
            }, vertexAttribute);
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
        auto orgVertexAttr = GetVertexAttribute<AttributeType::OrginalVertexNumber>();

        Skeleton* skeleton = actor->GetSkeleton();

        // get the skinning info for all three vertices
        for (uint32 i = 0; i < 3; ++i)
        {
            // get the original vertex number
            // remember that a cube can have 24 vertices to render (stored in this mesh), while it has only 8 original vertices
            uint32 originalVertex = orgVertexAttr->GetData()[ indices[startIndexOfFace + i] ];

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
        auto orignalVertexAttribute = GetVertexAttribute<AttributeType::OrginalVertexNumber>();

        // get the skinning info for all three vertices
        size_t maxInfluences = 0;
        for (uint32 i = 0; i < 3; ++i)
        {
            // get the original vertex number
            // remember that a cube can have 24 vertices to render (stored in this mesh), while it has only 8 original vertices
            uint32 originalVertex = orignalVertexAttribute->GetData()[ indices[startIndexOfFace + i] ];

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
        
        const auto vertexAttribute = GetVertexAttribute<AttributeType::OrginalVertexNumber>();

        // Get the vertex counts for the influences.
        size_t maxInfluences = 0;
        const uint32 numVerts = GetNumVertices();
        for (uint32 i = 0; i < numVerts; ++i)
        {
            const uint32 orgVertex = vertexAttribute->GetData()[i];

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
        size_t count = 0;
        for (auto& vertexAttribute : m_vertexAttributeLayer)
        {
            AZStd::visit([&count](auto& attr) {
                if(attr) {
                    count++;
                }
            }, vertexAttribute);
        }
        return count;
    }

    void Mesh::RemoveAllVertexAttributeLayers()
    {
        for(auto& attr: m_vertexAttributeLayer) {
            AZStd::visit([](auto& att) {
                if(att) {
                    att.reset();
                }
            }, attr);
        }
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
        for(auto& attr: m_vertexAttributeLayer) {
            AZStd::visit([&](auto& it) {
                using CurrentType = AZStd::decay_t<decltype(it)>;
                if(it) {
                    clone->GetVertexAttribute(it->GetType()) = AZStd::make_unique<typename CurrentType::element_type>(*it);
                }
            }, attr);
        }
        // return the resulting cloned mesh
        return clone;
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

    void Mesh::CalcAabb(AZ::Aabb* outBoundingBox, const Transform& transform, uint32 vertexFrequency)
    {
        MCORE_ASSERT(vertexFrequency >= 1);
        *outBoundingBox = AZ::Aabb::CreateNull();

        auto positionAttr = GetVertexAttribute<AttributeType::Position>();
        const uint32 numVerts = GetNumVertices();
        for (uint32 i = 0; i < numVerts; i += vertexFrequency)
        {
            outBoundingBox->AddPoint(transform.TransformPoint(positionAttr->GetData()[i]));
        }
    }



    // intersection test between the mesh and a ray
    bool Mesh::Intersects(const Transform& transform, const MCore::Ray& ray)
    {
        // get the positions and indices and calculate the inverse of the transformation matrix
        const auto attrPositions = GetVertexAttribute<AttributeType::Position>();
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

                if (testRay.Intersects(attrPositions->GetData()[indexA], attrPositions->GetData()[indexB], attrPositions->GetData()[indexC]))
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
        auto positionAttr = GetVertexAttribute<AttributeType::Position>();
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
                if (testRay.Intersects(positionAttr->GetData()[indexA], positionAttr->GetData()[indexB], positionAttr->GetData()[indexC], &intersectionPoint, &baryU, &baryV))
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

    // extract the original vertex positions
    void Mesh::ExtractOriginalVertexPositions(AZStd::vector<AZ::Vector3>& outPoints) const
    {
        // allocate space
        outPoints.resize(m_numOrgVerts);

        // get the mesh data
        auto position = GetVertexAttribute<AttributeType::Position>();
        auto orginalVertexNumbers = GetVertexAttribute<AttributeType::OrginalVertexNumber>();
        AZ_Assert(position != nullptr, "Mesh", "Position is null");   
        AZ_Assert(orginalVertexNumbers != nullptr, "Mesh", "OrginalVertexNumber is null");          

        // init all org vertices
        for (uint32 v = 0; v < m_numOrgVerts; ++v)
        {
            outPoints[v] = position->GetData()[0]; // init them, as there are some unused original vertices sometimes
        }
        // output the points
        for (uint32 i = 0; i < m_numVertices; ++i)
        {
            outPoints[ orginalVertexNumbers->GetOrignal()[i] ] = position->GetData()[i];
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

    // scale all positional data
    void Mesh::Scale(float scaleFactor)
    {

        for (VertexAttributeLayer* layer : m_sharedVertexAttributes)
        {
            layer->Scale(scaleFactor);
        }

        // scale the positional data
        auto positionAttr = GetVertexAttribute<AttributeType::Position>();

        const uint32 numVerts = m_numVertices;
        for (uint32 i = 0; i < numVerts; ++i)
        {
            positionAttr->GetData()[i] = positionAttr->GetData()[i] * scaleFactor;
            if(positionAttr->hasOrignal()) {
                positionAttr->GetOrignal()[i] = positionAttr->GetOrignal()[i] * scaleFactor;
            }
        }
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
