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

#ifndef CRYINCLUDE_CRY3DENGINE_CGF_CGFLOADER_H
#define CRYINCLUDE_CRY3DENGINE_CGF_CGFLOADER_H
#pragma once

#include "../MeshCompiler/MeshCompiler.h"
#include "ChunkFile.h"
#include "CGFContent.h"

#if defined(RESOURCE_COMPILER) || defined(ENABLE_NON_COMPILED_CGF)
#include "../../../Tools/CryCommonTools/Export/MeshUtils.h"
#endif

//////////////////////////////////////////////////////////////////////////
class ILoaderCGFListener
{
public:
    virtual ~ILoaderCGFListener(){}
    virtual void Warning(const char* format) = 0;
    virtual void Error(const char* format) = 0;
    virtual bool IsValidationEnabled() { return true; }
};

class CLoaderCGF
{
public:
    typedef void* (* AllocFncPtr)(size_t);
    typedef void (* DestructFncPtr)(void*);

    CLoaderCGF(AllocFncPtr pAlloc = operator new, DestructFncPtr pDestruct = operator delete, bool bAllowStreamSharing = true);
    ~CLoaderCGF();

    CContentCGF* LoadCGF(const char* filename, IChunkFile& chunkFile, ILoaderCGFListener* pListener, unsigned long nLoadingFlags = 0);
    bool LoadCGF(CContentCGF* pContentCGF, const char* filename, IChunkFile& chunkFile, ILoaderCGFListener* pListener, unsigned long nLoadingFlags = 0);
    bool LoadCGFFromMem(CContentCGF* pContentCGF, const void* pData, size_t nDataLen, IChunkFile& chunkFile, ILoaderCGFListener* pListener, unsigned long nLoadingFlags = 0);
    bool LoadCGFWork(CContentCGF* pContentCGF, const char* filename, IChunkFile& chunkFile, ILoaderCGFListener* pListener, unsigned long nLoadingFlags);

    const char* GetLastError() { return m_LastError; }
    CContentCGF* GetCContentCGF() { return m_pCompiledCGF; }

    void SetMaxWeightsPerVertex(int maxWeightsPerVertex) { m_maxWeightsPerVertex = maxWeightsPerVertex; }

private:
    bool LoadChunks(bool bJustGeometry);
    bool LoadExportFlagsChunk(IChunkFile::ChunkDesc* pChunkDesc);
    bool LoadNodeChunk(IChunkFile::ChunkDesc* pChunkDesc, bool bJustGeometry);

    bool LoadHelperChunk(CNodeCGF* pNode, IChunkFile::ChunkDesc* pChunkDesc);
    bool LoadGeomChunk(CNodeCGF* pNode, IChunkFile::ChunkDesc* pChunkDesc);
    bool LoadCompiledMeshChunk(CNodeCGF* pNode, IChunkFile::ChunkDesc* pChunkDesc);
    template<class MESH_CHUNK_DESC>
    bool LoadCompiledMeshChunk(CNodeCGF* pNode, IChunkFile::ChunkDesc* pChunkDesc, MESH_CHUNK_DESC chunk);
    bool LoadMeshSubsetsChunk(CMesh& mesh, IChunkFile::ChunkDesc* pChunkDesc, std::vector<std::vector<uint16> >& globalBonesPerSubset);
    bool LoadStreamDataChunk(int nChunkId, void*& pStreamData, int& nStreamType, int& nStreamIndex, int& nCount, int& nElemSize, bool& bSwapEndianness);
    template<class T, class MESH_CHUNK_DESC>
    bool LoadStreamChunk(CMesh& mesh, const MESH_CHUNK_DESC& chunk, ECgfStreamType Type, int streamIndex, CMesh::EStream MStream);
    //! Used to load data into one of two potential destination streams in CMesh determined by the element size of the data stored in the .cgf. e.g. load into either CMesh::POSITIONS or CMesh::POSITIONS16
    template<class TA, class TB, class MESH_CHUNK_DESC>
    bool LoadStreamChunk(CMesh& mesh, const MESH_CHUNK_DESC& chunk, ECgfStreamType Type, int streamIndex, CMesh::EStream MStreamA, CMesh::EStream MStreamB);
    template<class MESH_CHUNK_DESC>
    bool LoadBoneMappingStreamChunk(CMesh& mesh, const MESH_CHUNK_DESC& chunk, const std::vector<std::vector<uint16> >& globalBonesPerSubset);
    template<class MESH_CHUNK_DESC>
    bool LoadIndexStreamChunk(CMesh& mesh, const MESH_CHUNK_DESC& chunk);

    bool LoadPhysicsDataChunk(CNodeCGF* pNode, int nPhysGeomType, int nChunkId);

    bool LoadFoliageInfoChunk(IChunkFile::ChunkDesc* pChunkDesc);

    CMaterialCGF* LoadMaterialFromChunk(int nChunkId);

    CMaterialCGF* LoadMaterialNameChunk(IChunkFile::ChunkDesc* pChunkDesc);

    void ProcessNodes();
    void SetupMeshSubsets(CMesh& mesh, CMaterialCGF* pMaterialCGF);

    //////////////////////////////////////////////////////////////////////////
    // loading of skinned meshes
    //////////////////////////////////////////////////////////////////////////
    bool ProcessSkinning();
    CContentCGF* MakeCompiledSkinCGF(CContentCGF* pCGF, std::vector<int>* pVertexRemapping, std::vector<int>* pIndexRemapping);

    //old chunks
    bool ReadBoneNameList(IChunkFile::ChunkDesc* pChunkDesc);
    bool ReadMorphTargets(IChunkFile::ChunkDesc* pChunkDesc);
    bool ReadBoneInitialPos(IChunkFile::ChunkDesc* pChunkDesc);
    bool ReadBoneHierarchy(IChunkFile::ChunkDesc* pChunkDesc);
    uint32 RecursiveBoneLoader(int nBoneParentIndex, int nBoneIndex);
    bool ReadBoneMesh(IChunkFile::ChunkDesc* pChunkDesc);

    //new chunks
    bool ReadCompiledBones(IChunkFile::ChunkDesc* pChunkDesc);
    bool ReadCompiledPhysicalBones(IChunkFile::ChunkDesc* pChunkDesc);
    bool ReadCompiledPhysicalProxies(IChunkFile::ChunkDesc* pChunkDesc);
    bool ReadCompiledMorphTargets(IChunkFile::ChunkDesc* pChunkDesc);

    bool ReadCompiledIntFaces(IChunkFile::ChunkDesc* pChunkDesc);
    bool ReadCompiledIntSkinVertice(IChunkFile::ChunkDesc* pChunkDesc);
    bool ReadCompiledExt2IntMap(IChunkFile::ChunkDesc* pChunkDesc);
    bool ReadCompiledBonesBoxes(IChunkFile::ChunkDesc* pChunkDesc);

    bool ReadCompiledBreakablePhysics(IChunkFile::ChunkDesc* pChunkDesc);

    void Warning(const char* szFormat, ...) PRINTF_PARAMS(2, 3);

private:
    uint32 m_IsCHR;
    uint32 m_CompiledBones;
    uint32 m_CompiledBonesBoxes;
    uint32 m_CompiledMesh;

    uint32 m_numBonenameList;
    uint32 m_numBoneInitialPos;
    uint32 m_numMorphTargets;
    uint32 m_numBoneHierarchy;

    std::vector<uint32> m_arrIndexToId;     // the mapping BoneIndex -> BoneID
    std::vector<uint32> m_arrIdToIndex;     // the mapping BoneID -> BineIndex
    std::vector<string> m_arrBoneNameTable; // names of bones
    std::vector<Matrix34> m_arrInitPose34;
#if defined(RESOURCE_COMPILER) || defined(ENABLE_NON_COMPILED_CGF)
    std::vector<MeshUtils::VertexLinks> m_arrLinksTmp;
    std::vector<int> m_vertexOldToNew;      // used to re-map uncompiled Morph Target vertices right after reading them
#endif

    CContentCGF* m_pCompiledCGF;
    const void* m_pBoneAnimRawData, * m_pBoneAnimRawDataEnd;
    uint32 m_numBones;
    int m_nNextBone;

    //////////////////////////////////////////////////////////////////////////

    string m_LastError;

    char m_filename[260];

    IChunkFile* m_pChunkFile;
    CContentCGF* m_pCGF;

    // To find really used materials
    uint16 MatIdToSubset[MAX_SUB_MATERIALS];
    int nLastChunkId;

    ILoaderCGFListener* m_pListener;

    bool m_bUseReadOnlyMesh;
    bool m_bAllowStreamSharing;

    int m_maxWeightsPerVertex;

    AllocFncPtr m_pAllocFnc;
    DestructFncPtr m_pDestructFnc;
};

#endif // CRYINCLUDE_CRY3DENGINE_CGF_CGFLOADER_H
