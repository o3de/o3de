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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#include "ResourceCompilerPC_precompiled.h"
#include "CGFNodeMerger.h"
#include "CGFContent.h"
#include "StringHelpers.h"

//////////////////////////////////////////////////////////////////////////
bool CGFNodeMerger::SetupMeshSubsets(CContentCGF* pCGF, CMesh& mesh, CMaterialCGF* pMaterialCGF, string& errorMessage)
{
    const DynArray<int>& usedMaterialIds = pCGF->GetUsedMaterialIDs();

    unsigned int i;
    if (mesh.m_subsets.empty())
    {
        //////////////////////////////////////////////////////////////////////////
        // Setup mesh subsets.
        //////////////////////////////////////////////////////////////////////////
        mesh.m_subsets.clear();
        for (i = 0; i < usedMaterialIds.size(); i++)
        {
            SMeshSubset meshSubset;
            int nMatID = usedMaterialIds[i];
            meshSubset.nMatID = nMatID;
            meshSubset.nPhysicalizeType = PHYS_GEOM_TYPE_NONE;
            mesh.m_subsets.push_back(meshSubset);
        }
    }
    //////////////////////////////////////////////////////////////////////////
    // Setup physicalization type from materials (and autofix matId if needed)
    //////////////////////////////////////////////////////////////////////////
    if (pMaterialCGF)
    {
        for (i = 0; i < mesh.m_subsets.size(); i++)
        {
            SMeshSubset& meshSubset = mesh.m_subsets[i];
            if (pMaterialCGF->subMaterials.size() > 0)
            {
                int id = meshSubset.nMatID;
                if (id >= (int)pMaterialCGF->subMaterials.size())
                {
                    // Let's use 3dsMax's approach of handling material ids out of range
                    id %= (int)pMaterialCGF->subMaterials.size();
                }

                if (id >= 0 && pMaterialCGF->subMaterials[id] != NULL)
                {
                    meshSubset.nMatID = id;
                    meshSubset.nPhysicalizeType = pMaterialCGF->subMaterials[id]->nPhysicalizeType;
                }
                else
                {
                    errorMessage = StringHelpers::Format(
                            "%s: Submaterial %d is not available for subset %d (%d subsets) in %s",
                            __FUNCTION__, meshSubset.nMatID, i, (int)mesh.m_subsets.size(), pCGF->GetFilename());
                    return false;
                }
            }
            else
            {
                meshSubset.nPhysicalizeType = pMaterialCGF->nPhysicalizeType;
            }
        }
    }

    return true;
}


//////////////////////////////////////////////////////////////////////////
bool CGFNodeMerger::MergeNodes(CContentCGF* pCGF, std::vector<CNodeCGF*> nodes, string& errorMessage, CMesh* pMergedMesh)
{
    assert(pMergedMesh);

    AABB meshBBox;
    meshBBox.Reset();
    for (size_t i = 0; i < nodes.size(); i++)
    {
        CNodeCGF* pNode = nodes[i];
        assert(pNode->pMesh == 0 || pNode->pMesh->m_pPositionsF16 == 0);
        int nOldVerts = pMergedMesh->GetVertexCount();
        if (pMergedMesh->GetVertexCount() == 0)
        {
            pMergedMesh->Copy(*pNode->pMesh);
        }
        else
        {
            const char* const errText = pMergedMesh->Append(*pNode->pMesh);
            if (errText)
            {
                errorMessage = errText;
                return false;
            }
            // Keep color stream in sync size with vertex/normals stream.
            if (pMergedMesh->m_streamSize[CMesh::COLORS][0] > 0 && pMergedMesh->m_streamSize[CMesh::COLORS][0] < pMergedMesh->GetVertexCount())
            {
                int nOldCount = pMergedMesh->m_streamSize[CMesh::COLORS][0];
                pMergedMesh->ReallocStream(CMesh::COLORS, 0, pMergedMesh->GetVertexCount());
                memset(pMergedMesh->m_pColor0 + nOldCount, 255, (pMergedMesh->GetVertexCount() - nOldCount) * sizeof(SMeshColor));
            }
            if (pMergedMesh->m_streamSize[CMesh::COLORS][1] > 0 && pMergedMesh->m_streamSize[CMesh::COLORS][1] < pMergedMesh->GetVertexCount())
            {
                int nOldCount = pMergedMesh->m_streamSize[CMesh::COLORS][1];
                pMergedMesh->ReallocStream(CMesh::COLORS, 1, pMergedMesh->GetVertexCount());
                memset(pMergedMesh->m_pColor1 + nOldCount, 255, (pMergedMesh->GetVertexCount() - nOldCount) * sizeof(SMeshColor));
            }
        }

        AABB bbox = pNode->pMesh->m_bbox;
        if (!pNode->bIdentityMatrix)
        {
            bbox.SetTransformedAABB(pNode->worldTM, bbox);
        }

        meshBBox.Add(bbox.min);
        meshBBox.Add(bbox.max);
        pMergedMesh->m_bbox = meshBBox;

        if (!pNode->bIdentityMatrix)
        {
            // Transform merged mesh into the world space.
            // Only transform newly added vertices.
            for (int j = nOldVerts; j < pMergedMesh->GetVertexCount(); j++)
            {
                pMergedMesh->m_pPositions[j] = pNode->worldTM.TransformPoint(pMergedMesh->m_pPositions[j]);
                pMergedMesh->m_pNorms[j].RotateSafelyBy(pNode->worldTM);
            }
        }
    }

    bool setupMeshSuccessful = true;
    if (pCGF != NULL)
    {
        if (!SetupMeshSubsets(pCGF, *pMergedMesh, pCGF->GetCommonMaterial(), errorMessage))
        {
            return false;
        }
    }
    pMergedMesh->RecomputeTexMappingDensity();

    return setupMeshSuccessful;
}
