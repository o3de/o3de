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

#include <Integration/ActorComponentBus.h>

// Needed to access the Mesh information inside Actor.
#include <EMotionFX/Source/Node.h>
#include <EMotionFX/Source/Mesh.h>
#include <EMotionFX/Source/SubMesh.h>
#include <EMotionFX/Source/ActorInstance.h>

#include <Utils/ActorAssetHelper.h>

namespace NvCloth
{
    ActorAssetHelper::ActorAssetHelper(AZ::EntityId entityId)
        : AssetHelper(entityId)
    {
    }

    void ActorAssetHelper::GatherClothMeshNodes(MeshNodeList& meshNodes)
    {
        EMotionFX::ActorInstance* actorInstance = nullptr;
        EMotionFX::Integration::ActorComponentRequestBus::EventResult(
            actorInstance, m_entityId, &EMotionFX::Integration::ActorComponentRequestBus::Events::GetActorInstance);
        if (!actorInstance)
        {
            return;
        }

        const EMotionFX::Actor* actor = actorInstance->GetActor();
        if (!actor)
        {
            return;
        }

        const uint32 numNodes = actor->GetNumNodes();
        const uint32 numLODs = actor->GetNumLODLevels();

        for (uint32 lodLevel = 0; lodLevel < numLODs; ++lodLevel)
        {
            for (uint32 nodeIndex = 0; nodeIndex < numNodes; ++nodeIndex)
            {
                const EMotionFX::Mesh* mesh = actor->GetMesh(lodLevel, nodeIndex);
                if (!mesh)
                {
                    continue;
                }

                const bool hasClothData = (mesh->FindOriginalVertexData(EMotionFX::Mesh::ATTRIB_CLOTH_DATA) != nullptr);
                if (hasClothData)
                {
                    const EMotionFX::Node* node = actor->GetSkeleton()->GetNode(nodeIndex);
                    AZ_Assert(node, "Invalid node %u in actor '%s'", nodeIndex, actor->GetFileNameString().c_str());
                    meshNodes.push_back(node->GetNameString());
                }
            }
        }
    }

    bool ActorAssetHelper::ObtainClothMeshNodeInfo(
        const AZStd::string& meshNode,
        MeshNodeInfo& meshNodeInfo,
        MeshClothInfo& meshClothInfo)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Cloth);

        EMotionFX::ActorInstance* actorInstance = nullptr;
        EMotionFX::Integration::ActorComponentRequestBus::EventResult(
            actorInstance, m_entityId, &EMotionFX::Integration::ActorComponentRequestBus::Events::GetActorInstance);
        if (!actorInstance)
        {
            return false;
        }

        const EMotionFX::Actor* actor = actorInstance->GetActor();
        if (!actor)
        {
            return false;
        }

        const uint32 numNodes = actor->GetNumNodes();
        const uint32 numLODs = actor->GetNumLODLevels();

        const EMotionFX::Mesh* emfxMesh = nullptr;
        uint32 meshFirstPrimitiveIndex = 0;

        // Find the render data of the mesh node
        for (uint32 lodLevel = 0; lodLevel < numLODs; ++lodLevel)
        {
            meshFirstPrimitiveIndex = 0;

            for (uint32 nodeIndex = 0; nodeIndex < numNodes; ++nodeIndex)
            {
                const EMotionFX::Mesh* mesh = actor->GetMesh(lodLevel, nodeIndex);
                if (!mesh || mesh->GetIsCollisionMesh())
                {
                    // Skip invalid and collision meshes.
                    continue;
                }

                const EMotionFX::Node* node = actor->GetSkeleton()->GetNode(nodeIndex);
                if (meshNode != node->GetNameString())
                {
                    // Skip. Increase the index of all primitives of the mesh we're skipping.
                    meshFirstPrimitiveIndex += mesh->GetNumSubMeshes();
                    continue;
                }

                // Mesh found, save the lod in mesh info
                meshNodeInfo.m_lodLevel = lodLevel;
                emfxMesh = mesh;
                break;
            }

            if (emfxMesh)
            {
                break;
            }
        }

        bool infoObtained = false;

        if (emfxMesh)
        {
            bool dataCopied = CopyDataFromEMotionFXMesh(*emfxMesh, meshClothInfo);

            if (dataCopied)
            {
                const uint32 numSubMeshes = emfxMesh->GetNumSubMeshes();
                for (uint32 subMeshIndex = 0; subMeshIndex < numSubMeshes; ++subMeshIndex)
                {
                    const EMotionFX::SubMesh* emfxSubMesh = emfxMesh->GetSubMesh(subMeshIndex);

                    MeshNodeInfo::SubMesh subMesh;
                    subMesh.m_primitiveIndex = static_cast<int>(meshFirstPrimitiveIndex + subMeshIndex);
                    subMesh.m_verticesFirstIndex = emfxSubMesh->GetStartVertex();
                    subMesh.m_numVertices = emfxSubMesh->GetNumVertices();
                    subMesh.m_indicesFirstIndex = emfxSubMesh->GetStartIndex();
                    subMesh.m_numIndices = emfxSubMesh->GetNumIndices();

                    meshNodeInfo.m_subMeshes.push_back(subMesh);
                }

                infoObtained = true;
            }
            else
            {
                AZ_Error("ActorAssetHelper", false, "Failed to extract data from node %s in actor %s",
                    meshNode.c_str(), actor->GetFileNameString().c_str());
            }
        }

        return infoObtained;
    }

    bool ActorAssetHelper::CopyDataFromEMotionFXMesh(
        const EMotionFX::Mesh& emfxMesh,
        MeshClothInfo& meshClothInfo)
    {
        const int numVertices = emfxMesh.GetNumVertices();
        const int numIndices = emfxMesh.GetNumIndices();
        if (numVertices == 0 || numIndices == 0)
        {
            return false;
        }

        const uint32* sourceIndices = emfxMesh.GetIndices();
        const AZ::Vector3* sourcePositions = static_cast<AZ::Vector3*>(emfxMesh.FindOriginalVertexData(EMotionFX::Mesh::ATTRIB_POSITIONS));
        const AZ::u32* sourceClothData = static_cast<AZ::u32*>(emfxMesh.FindOriginalVertexData(EMotionFX::Mesh::ATTRIB_CLOTH_DATA));
        const AZ::Vector2* sourceUVs = static_cast<AZ::Vector2*>(emfxMesh.FindOriginalVertexData(EMotionFX::Mesh::ATTRIB_UVCOORDS, 0)); // first UV set

        if (!sourceIndices || !sourcePositions || !sourceClothData)
        {
            return false;
        }

        const SimUVType uvZero(0.0f, 0.0f);

        meshClothInfo.m_particles.resize_no_construct(numVertices);
        meshClothInfo.m_uvs.resize_no_construct(numVertices);
        meshClothInfo.m_motionConstraints.resize_no_construct(numVertices);
        meshClothInfo.m_backstopData.resize_no_construct(numVertices);
        for (int index = 0; index < numVertices; ++index)
        {
            AZ::Color clothVertexData;
            clothVertexData.FromU32(sourceClothData[index]);

            const float inverseMass = clothVertexData.GetR();
            const float motionConstraint = clothVertexData.GetG();
            const float backstopRadius = clothVertexData.GetA();
            const float backstopOffset = ConvertBackstopOffset(clothVertexData.GetB());

            meshClothInfo.m_particles[index].Set(
                sourcePositions[index],
                inverseMass);

            meshClothInfo.m_motionConstraints[index] = motionConstraint;
            meshClothInfo.m_backstopData[index].Set(backstopOffset, backstopRadius);

            meshClothInfo.m_uvs[index] = (sourceUVs) ? SimUVType(sourceUVs[index].GetX(), sourceUVs[index].GetY()) : uvZero;
        }

        meshClothInfo.m_indices.resize_no_construct(numIndices);
        // Fast copy when SimIndexType is the same size as the EMFX indices type.
        if constexpr (sizeof(SimIndexType) == sizeof(uint32))
        {
            memcpy(meshClothInfo.m_indices.data(), sourceIndices, numIndices * sizeof(SimIndexType));
        }
        else
        {
            for (int index = 0; index < numIndices; ++index)
            {
                meshClothInfo.m_indices[index] = static_cast<SimIndexType>(sourceIndices[index]);
            }
        }

        return true;
    }
} // namespace NvCloth
