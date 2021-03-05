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

#include <platform.h> // Needed for MeshAsset.h
#include <LmbrCentral/Rendering/MeshComponentBus.h>
#include <LmbrCentral/Rendering/MeshAsset.h>
#include <IRenderer.h>

#include <IIndexedMesh.h> // Needed for SMeshColor

#include <AzCore/Math/Color.h>

#include <Utils/MeshAssetHelper.h>

namespace NvCloth
{
    MeshAssetHelper::MeshAssetHelper(AZ::EntityId entityId)
        : AssetHelper(entityId)
    {
    }

    void MeshAssetHelper::GatherClothMeshNodes(MeshNodeList& meshNodes)
    {
        AZ::Data::Asset<LmbrCentral::MeshAsset> meshAsset;
        LmbrCentral::MeshComponentRequestBus::EventResult(
            meshAsset, m_entityId, &LmbrCentral::MeshComponentRequestBus::Events::GetMeshAsset);
        if (!meshAsset.IsReady())
        {
            return;
        }

        IStatObj* statObj = meshAsset.Get()->m_statObj.get();
        if (!statObj)
        {
            return;
        }

        if (!statObj->GetClothData().empty())
        {
            meshNodes.push_back(statObj->GetCGFNodeName().c_str());
        }

        const int subObjectCount = statObj->GetSubObjectCount();
        for (int i = 0; i < subObjectCount; ++i)
        {
            IStatObj::SSubObject* subObject = statObj->GetSubObject(i);
            if (subObject &&
                subObject->pStatObj &&
                !subObject->pStatObj->GetClothData().empty())
            {
                meshNodes.push_back(subObject->pStatObj->GetCGFNodeName().c_str());
            }
        }
    }

    bool MeshAssetHelper::ObtainClothMeshNodeInfo(
        const AZStd::string& meshNode, 
        MeshNodeInfo& meshNodeInfo,
        MeshClothInfo& meshClothInfo)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Cloth);

        AZ::Data::Asset<LmbrCentral::MeshAsset> meshAsset;
        LmbrCentral::MeshComponentRequestBus::EventResult(
            meshAsset, m_entityId, &LmbrCentral::MeshComponentRequestBus::Events::GetMeshAsset);
        if (!meshAsset.IsReady())
        {
            return false;
        }

        IStatObj* statObj = meshAsset.Get()->m_statObj.get();
        if (!statObj)
        {
            return false;
        }

        IStatObj* selectedStatObj = nullptr;
        int primitiveIndex = 0;

        // Find the render data of the mesh node
        if (meshNode == statObj->GetCGFNodeName().c_str())
        {
            selectedStatObj = statObj;
        }
        else
        {
            const int subObjectCount = statObj->GetSubObjectCount();
            for (int i = 0; i < subObjectCount; ++i)
            {
                IStatObj::SSubObject* subObject = statObj->GetSubObject(i);
                if (subObject &&
                    subObject->pStatObj &&
                    subObject->pStatObj->GetRenderMesh() &&
                    meshNode == subObject->pStatObj->GetCGFNodeName().c_str())
                {
                    selectedStatObj = subObject->pStatObj;
                    primitiveIndex = i;
                    break;
                }
            }
        }

        bool infoObtained = false;

        if (selectedStatObj)
        {
            bool dataCopied = false;

            if (IRenderMesh* renderMesh = selectedStatObj->GetRenderMesh())
            {
                dataCopied = CopyDataFromRenderMesh(
                    *renderMesh, selectedStatObj->GetClothData(),
                    meshClothInfo);
            }

            if (dataCopied)
            {
                // subStatObj contains the buffers for all its submeshes
                // so only 1 submesh is necessary.
                MeshNodeInfo::SubMesh subMesh;
                subMesh.m_primitiveIndex = primitiveIndex;
                subMesh.m_verticesFirstIndex = 0;
                subMesh.m_numVertices = meshClothInfo.m_particles.size();
                subMesh.m_indicesFirstIndex = 0;
                subMesh.m_numIndices = meshClothInfo.m_indices.size();

                meshNodeInfo.m_lodLevel = 0; // Cloth is alway in LOD 0
                meshNodeInfo.m_subMeshes.push_back(subMesh);

                infoObtained = true;
            }
            else
            {
                AZ_Error("MeshAssetHelper", false, "Failed to extract data from node %s in mesh %s",
                    meshNode.c_str(), statObj->GetFileName().c_str());
            }
        }

        return infoObtained;
    }

    bool MeshAssetHelper::CopyDataFromRenderMesh(
        IRenderMesh& renderMesh,
        const AZStd::vector<SMeshColor>& renderMeshClothData,
        MeshClothInfo& meshClothInfo)
    {
        const int numVertices = renderMesh.GetNumVerts();
        const int numIndices = renderMesh.GetNumInds();
        if (numVertices == 0 || numIndices == 0)
        {
            return false;
        }
        else if (numVertices != renderMeshClothData.size())
        {
            AZ_Error("MeshAssetHelper", false,
                "Number of vertices (%d) doesn't match the number of cloth data (%zu)",
                numVertices,
                renderMeshClothData.size());
            return false;
        }

        {
            IRenderMesh::ThreadAccessLock lockRenderMesh(&renderMesh);

            vtx_idx* renderMeshIndices = nullptr;
            strided_pointer<Vec3> renderMeshVertices;
            strided_pointer<Vec2> renderMeshUVs;

            renderMeshIndices = renderMesh.GetIndexPtr(FSL_READ);
            renderMeshVertices.data = reinterpret_cast<Vec3*>(renderMesh.GetPosPtr(renderMeshVertices.iStride, FSL_READ));
            renderMeshUVs.data = reinterpret_cast<Vec2*>(renderMesh.GetUVPtr(renderMeshUVs.iStride, FSL_READ, 0)); // first UV set

            const SimUVType uvZero(0.0f, 0.0f);

            meshClothInfo.m_particles.resize_no_construct(numVertices);
            meshClothInfo.m_uvs.resize_no_construct(numVertices);
            meshClothInfo.m_motionConstraints.resize_no_construct(numVertices);
            meshClothInfo.m_backstopData.resize_no_construct(numVertices);
            for (int index = 0; index < numVertices; ++index)
            {
                const ColorB clothVertexDataColorB = renderMeshClothData[index].GetRGBA();
                const AZ::Color clothVertexData(
                    clothVertexDataColorB.r,
                    clothVertexDataColorB.g,
                    clothVertexDataColorB.b,
                    clothVertexDataColorB.a);

                const float inverseMass = clothVertexData.GetR();
                const float motionConstraint = clothVertexData.GetG();
                const float backstopRadius = clothVertexData.GetA();
                const float backstopOffset = ConvertBackstopOffset(clothVertexData.GetB());

                meshClothInfo.m_particles[index].Set(
                    renderMeshVertices[index].x,
                    renderMeshVertices[index].y,
                    renderMeshVertices[index].z,
                    inverseMass);

                meshClothInfo.m_motionConstraints[index] = motionConstraint;
                meshClothInfo.m_backstopData[index].Set(backstopOffset, backstopRadius);

                meshClothInfo.m_uvs[index] = (renderMeshUVs.data) ? SimUVType(renderMeshUVs[index].x, renderMeshUVs[index].y) : uvZero;
            }

            meshClothInfo.m_indices.resize_no_construct(numIndices);
            // Fast copy when SimIndexType is the same size as the vtx_idx.
            if constexpr (sizeof(SimIndexType) == sizeof(vtx_idx))
            {
                memcpy(meshClothInfo.m_indices.data(), renderMeshIndices, numIndices * sizeof(SimIndexType));
            }
            else
            {
                for (int index = 0; index < numIndices; ++index)
                {
                    meshClothInfo.m_indices[index] = static_cast<SimIndexType>(renderMeshIndices[index]);
                }
            }

            renderMesh.UnlockStream(VSF_GENERAL);
            renderMesh.UnlockIndexStream();
        }

        return true;
    }
} // namespace NvCloth
