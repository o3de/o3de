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

#ifndef CRYINCLUDE_CRY3DENGINE_CGF_CGFSAVER_H
#define CRYINCLUDE_CRY3DENGINE_CGF_CGFSAVER_H
#pragma once

#if defined(RESOURCE_COMPILER) || defined(INCLUDE_SAVECGF)

#include "CGFContent.h"

class CChunkFile;

//////////////////////////////////////////////////////////////////////////
class CSaverCGF
{
public:
    CSaverCGF(CChunkFile& chunkFile);

    void SaveContent(CContentCGF* pCGF, bool bSwapEndian, bool bStorePositionsAsF16, bool bUseQtangents, bool bStoreIndicesAsU16);

    void SetContent(CContentCGF* pCGF);

    const CContentCGF* GetContent() const;

    // Enable/Disable saving of the node mesh.
    void SetMeshDataSaving(bool bEnable);

    // Enable/Disable saving of non-mesh related data.
    void SetNonMeshDataSaving(bool bEnable);

    // Enable/disable saving of physics meshes.
    void SetSavePhysicsMeshes(bool bEnable);

    // Enable compaction of vertex streams (for optimised streaming)
    void SetVertexStreamCompacting(bool bEnable);

    // Enable computation of subset texel density
    void SetSubsetTexelDensityComputing(bool bEnable);

    void SaveNodes(bool bSwapEndian, bool bStorePositionsAsF16, bool bUseQtangents, bool bStoreIndicesAsU16);
    int SaveNode(CNodeCGF* pNode, bool bSwapEndian, bool bStorePositionsAsF16, bool bUseQtangents, bool bStoreIndicesAsU16);

    void SaveMaterials(bool bSwapEndian);
    int SaveMaterial(CMaterialCGF* pMtl, bool bNeedSwap);
    int SaveExportFlags(bool bSwapEndian);

    // Compiled chunks for characters
    int SaveCompiledBones(bool bSwapEndian, void* pData, int nSize, int version);
    int SaveCompiledPhysicalBones(bool bSwapEndian, void* pData, int nSize, int version);
    int SaveCompiledPhysicalProxis(bool bSwapEndian, void* pData, int nSize, uint32 numIntMorphTargets, int version);
    int SaveCompiledMorphTargets(bool bSwapEndian, void* pData, int nSize, uint32 numIntMorphTargets, int version);
    int SaveCompiledIntFaces(bool bSwapEndian, void* pData, int nSize, int version);
    int SaveCompiledIntSkinVertices(bool bSwapEndian, void* pData, int nSize, int version);
    int SaveCompiledExt2IntMap(bool bSwapEndian, void* pData, int nSize, int version);
    int SaveCompiledBoneBox(bool bSwapEndian, void* pData, int nSize, int version);

    // Chunks for characters (for Collada->cgf export)
    int SaveBones(bool bSwapEndian, void* pData, int numBones, int nSize);
    int SaveBoneNames(bool bSwapEndian, char* boneList, int numBones, int listSize);
    int SaveBoneInitialMatrices(bool bSwapEndian, SBoneInitPosMatrix* matrices, int numBones, int nSize);
    int SaveBoneMesh(bool bSwapEndian, PhysicalProxy& proxy);
#if defined(RESOURCE_COMPILER)
    void SaveUncompiledNodes();
    int SaveUncompiledNode(CNodeCGF* pNode);
    void SaveUncompiledMorphTargets();
    int SaveUncompiledNodeMesh(CNodeCGF* pNode);
    int SaveUncompiledHelperChunk(CNodeCGF* pNode);
#endif

    int SaveBreakablePhysics(bool bNeedEndianSwap);

    int SaveController831(bool bSwapEndian, const CONTROLLER_CHUNK_DESC_0831& ctrlChunk, void* pData, int nSize);
    int SaveControllerDB905(bool bSwapEndian, const CONTROLLER_CHUNK_DESC_0905& ctrlChunk, void* pData, int nSize);

    int SaveFoliage();

private:
    // Return mesh chunk id
    int SaveNodeMesh(CNodeCGF* pNode, bool bSwapEndian, bool bStorePositionsAsF16, bool bUseQTangents, bool bStoreIndicesAsU16);
    int SaveHelperChunk(CNodeCGF* pNode, bool bSwapEndian);
    int SaveMeshSubsetsChunk(CMesh& mesh, bool bSwapEndian);
    int SaveStreamDataChunk(const void* pStreamData, int nStreamType, int nStreamIndex, int nCount, int nElemSize, bool bSwapEndian);
    int SavePhysicalDataChunk(const void* pData, int nSize, bool bSwapEndian);

private:
    CChunkFile* m_pChunkFile;
    CContentCGF* m_pCGF;
    std::set<CNodeCGF*> m_savedNodes;
    std::set<CMaterialCGF*> m_savedMaterials;
    std::map<CMesh*, int> m_mapMeshToChunk;
    bool m_bDoNotSaveMeshData;
    bool m_bDoNotSaveNonMeshData;
    bool m_bSavePhysicsMeshes;
    bool m_bCompactVertexStreams;
    bool m_bComputeSubsetTexelDensity;
};

#endif
#endif // CRYINCLUDE_CRY3DENGINE_CGF_CGFSAVER_H
