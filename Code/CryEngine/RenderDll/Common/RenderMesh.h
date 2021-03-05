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

#ifndef __RENDERMESH_H__
#define __RENDERMESH_H__

#include <intrusive_list.hpp>
#include <CryPool/PoolAlloc.h>
#include <VectorMap.h>
#include <VectorSet.h>
#include <GeomQuery.h>
#include "Shaders/Vertex.h"
#include <AzCore/Jobs/LegacyJobExecutor.h>

// Enable the below to get fatal error is some holds a rendermesh buffer lock for longer than 1 second
//#define RM_CATCH_EXCESSIVE_LOCKS

#define DELETE_SUBSET_MESHES_AFTER_NOTUSED_FRAMES 30

struct SMeshSubSetIndicesJobEntry
{
    AZ::LegacyJobExecutor jobExecutor;
    _smart_ptr<IRenderMesh> m_pSrcRM;                           // source mesh to create a new index mesh from
    _smart_ptr<IRenderMesh> m_pIndexRM;                     // when finished: newly created index mesh for this mask, else NULL
    uint64 m_nMeshSubSetMask;                       // mask to use

    void CreateSubSetRenderMesh();
};

class RenderMesh_hash_int32
{
public:
    ILINE size_t operator()(int32 key) const
    {
        return stl::hash_uint32()((uint32)key);
    }
};

struct SBufInfoTable
{
    int OffsTC;
    int OffsColor;
    int OffsNorm;
};

struct SMeshStream
{
    buffer_handle_t m_nID; // device buffer handle from device buffer manager
    void* m_pUpdateData;   // system buffer for updating (used for async. mesh updates)
    void* m_pLockedData;   // locked device buffer data (hmm, not a good idea to store)
    uint32 m_nLockFlags : 16;
    uint32 m_nLockCount : 16;
    uint32 m_nElements;
    int32  m_nFrameAccess;
    int32  m_nFrameRequest;
    int32  m_nFrameUpdate;
    int32  m_nFrameCreate;

    SMeshStream()
    {
        m_nID = ~0u;
        m_pUpdateData = NULL;
        m_pLockedData = NULL;
        m_nFrameRequest = 0;
        m_nFrameUpdate = -1;
        m_nFrameAccess = -1;
        m_nFrameCreate = -1;
        m_nLockFlags = 0;
        m_nLockCount = 0;
        m_nElements = 0;
    }

    ~SMeshStream() { memset(this, 0x0, sizeof(*this)); }
};

// CRenderMesh::m_nFlags
#define FRM_RELEASED 1
#define FRM_DEPRECTATED_FLAG 2
#define FRM_READYTOUPLOAD 4
#define FRM_ALLOCFAILURE 8
#define FRM_SKINNED 0x10
#define FRM_SKINNEDNEXTDRAW 0x20 // no proper support yet for objects that can be skinned and not skinned.
#define FRM_ENABLE_NORMALSTREAM 0x40

#define MAX_RELEASED_MESH_FRAMES (2)

struct SSetMeshIntData
{
    CMesh* m_pMesh;
    char* m_pVBuff;
    SPipTangents* m_pTBuff;
    SPipQTangents* m_pQTBuff;
    SVF_P3F* m_pVelocities;
    uint32 m_nVerts;
    uint32 m_nInds;
    vtx_idx* m_pInds;
    uint32 m_flags;
    Vec3* m_pNormalsBuff;
};


class CRenderMesh
    : public IRenderMesh
{
    friend class CREMeshImpl;

public:

    static void ClearJobResources();

private:
    friend class CD3D9Renderer;

    SMeshStream  m_IBStream;
    SMeshStream* m_VBStream[VSF_NUM];
    struct SBoneIndexStream
    {
        buffer_handle_t buffer;
        uint32 guid;
        uint32 refcount;
    };

    struct SBoneIndexStreamRequest
    {
        SBoneIndexStreamRequest(uint32 _guid, SVF_W4B_I4S* _pStream)
            : pStream(_pStream)
            , guid(_guid)
            , refcount(1) {}

        SVF_W4B_I4S* pStream;
        uint32 guid;
        uint32 refcount;
    };

    std::vector<SBoneIndexStream> m_RemappedBoneIndices;
    std::vector< SBoneIndexStreamRequest > m_CreatedBoneIndices[2];
    std::vector< uint32 > m_DeletedBoneIndices[2];

    uint32 m_nInds;
    uint32 m_nVerts;
    int   m_nRefCounter;
    AZ::Vertex::Format m_vertexFormat;          // Base stream vertex format (optional streams are hardcoded: VSF_)

    Vec3* m_pCachePos;       // float positions (cached)
    int   m_nFrameRequestCachePos;

    std::vector<Vec2*> m_UVCache;         // float UVs (cached)
    int   m_nFrameRequestCacheUVs;

    CRenderMesh* m_pVertexContainer;
    PodArray<CRenderMesh*>  m_lstVertexContainerUsers;

#ifdef RENDER_MESH_TRIANGLE_HASH_MAP_SUPPORT
    typedef AZStd::unordered_map<int, PodArray< std::pair<int, int> >, RenderMesh_hash_int32> TrisMap;
    TrisMap* m_pTrisMap;
#endif

    SRecursiveSpinLock m_sResLock;

    AZStd::atomic<int> m_nThreadAccessCounter;// counter to ensure that no system rendermesh streams are freed since they are in use
    volatile int m_asyncUpdateState[2];
    int m_asyncUpdateStateCounter[2];

    eRenderPrimitiveType m_nPrimetiveType           : 8;
    ERenderMeshType m_eType                   : 4;
    uint16 m_nFlags                                                     : 8;          // FRM_
    int16  m_nLod                             : 4;        // used for LOD debug visualization
    bool m_keepSysMesh                        : 1;
    bool m_nFlagsCachePos                     : 1;        // only checked for FSL_WRITE, which can be represented as a single bit
    bool m_nFlagsCacheUVs                                           : 1;



public:
    enum ESizeUsageArg
    {
        SIZE_ONLY_SYSTEM = 0,
        SIZE_VB = 1,
        SIZE_IB = 2,
    };

private:
    SMeshStream* GetVertexStream(int nStream, uint32 nFlags = 0);
    SMeshStream* GetVertexStream(int nStream, [[maybe_unused]] uint32 nFlags = 0) const { return m_VBStream[nStream]; }
    bool UpdateVidIndices(SMeshStream& IBStream, bool stall = true);

    bool CreateVidVertices(int nStream = VSF_GENERAL);
    bool UpdateVidVertices(int nStream, bool stall = true);

    bool CopyStreamToSystemForUpdate(SMeshStream& MS, size_t nSize);

    void ReleaseVB(int nStream);
    void ReleaseIB();

    void InitTriHash(_smart_ptr<IMaterial> pMaterial);

    bool CreateCachePos(byte* pSrc, uint32 nStrideSrc, uint32 nFlags);
    bool PrepareCachePos();
    bool CreateUVCache(byte* pSrc, uint32 nStrideSrc, uint32 nFlags, uint32 uvSetIndex);

    //Internal versions of funcs - no lock
    bool UpdateVertices_Int(const void* pVertBuffer, int nVertCount, int nOffset, int nStream, uint32 copyFlags);
    bool UpdateIndices_Int(const vtx_idx* pNewInds, int nInds, int nOffsInd, uint32 copyFlags);
    size_t SetMesh_Int(CMesh& mesh, int nSecColorsSetOffset, uint32 flags);

#ifdef MESH_TESSELLATION_RENDERER
    template<class VecPos, class VecUV>
    bool UpdateUVCoordsAdjacency(SMeshStream& IBStream, const AZ::Vertex::Format& vertexFormat);

    template<class VecPos, class VecUV>
    static void BuildAdjacency(const byte* pVerts, const AZ::Vertex::Format& vertexFormat, uint nVerts, const vtx_idx* pIndexBuffer, uint nTrgs, std::vector<VecUV>& pTxtAdjBuffer);
#endif

    void Cleanup();

public:

    void AddShadowPassMergedChunkIndicesAndVertices(CRenderChunk* pCurrentChunk, _smart_ptr<IMaterial> pMaterial, int& rNumVertices, int& rNumIndices);
    static bool RenderChunkMergeAbleInShadowPass(CRenderChunk* pPreviousChunk, CRenderChunk* pCurrentChunk, _smart_ptr<IMaterial> pMaterial);

    inline void PrefetchVertexStreams() const
    {
        for (int i = 0; i < VSF_NUM; CryPrefetch(m_VBStream[i++]))
        {
            ;
        }
    }

    void SetMesh_IntImpl(SSetMeshIntData data);

    //! constructor
    //! /param szSource this pointer is stored - make sure the memory stays
    CRenderMesh(const char* szType, const char* szSourceName, bool bLock = false);
    CRenderMesh();

    //! destructor
    ~CRenderMesh();

    virtual bool CanRender(){return (m_nFlags & FRM_ALLOCFAILURE) == 0; }

    virtual void AddRef()
    {
#   if !defined(_RELEASE)
        if (m_nFlags & FRM_RELEASED)
        {
            CryFatalError("CRenderMesh::AddRef() mesh already in the garbage list (resurrecting deleted mesh)");
        }
#   endif
        CryInterlockedIncrement(&m_nRefCounter);
    }
    virtual int Release();
    void ReleaseForce()
    {
        while (true)
        {
            int nRef = Release();
            if (nRef <= 0)
            {
                return;
            }
        }
    }

    // ----------------------------------------------------------------
    // Helper functions
   _inline int GetStreamStride(int nStream) const override
    {
        if (nStream == VSF_GENERAL)
        {
            return m_vertexFormat.GetStride();
        }
        else
        {
            return m_cSizeStream[nStream];
        }
    }

    _inline uint32 _GetFlags() const { return m_nFlags; }
    _inline int GetStreamSize(int nStream, int nVerts = 0) const { return GetStreamStride(nStream) * (nVerts ? nVerts : m_nVerts); }
    _inline const buffer_handle_t GetVBStream(int nStream) const
    {
        if (!m_VBStream[nStream])
        {
            return ~0u;
        }
        return m_VBStream[nStream]->m_nID;
    }
    _inline const buffer_handle_t GetIBStream() const { return m_IBStream.m_nID; }
    _inline bool _HasVBStream(int nStream) const { return m_VBStream[nStream] && m_VBStream[nStream]->m_nID != ~0u; }
    _inline bool _HasIBStream() const { return m_IBStream.m_nID != ~0u; }
    _inline int _IsVBStreamLocked(int nStream) const
    {
        if (!m_VBStream[nStream])
        {
            return 0;
        }
        return (m_VBStream[nStream]->m_nLockFlags & FSL_LOCKED);
    }
    _inline int _IsIBStreamLocked() const { return m_IBStream.m_nLockFlags & FSL_LOCKED; }
    _inline AZ::Vertex::Format _GetVertexFormat() const { return m_vertexFormat; }
    _inline void _SetVertexFormat(const AZ::Vertex::Format& vertexFormat) { m_vertexFormat = vertexFormat; }
    _inline int GetNumVerts() const override { return m_nVerts; }
    _inline void _SetNumVerts(int nVerts) { m_nVerts = max(nVerts, 0); }
    _inline int GetNumInds() const override { return m_nInds; }
    _inline void _SetNumInds(int nInds) { m_nInds = nInds; }
    _inline const eRenderPrimitiveType GetPrimitiveType() const override { return m_nPrimetiveType; }
    _inline void _SetPrimitiveType(const eRenderPrimitiveType nPrimType) { m_nPrimetiveType = nPrimType; }
    _inline void _SetRenderMeshType(ERenderMeshType eType) { m_eType = eType; }
    _inline CRenderMesh* _GetVertexContainer()
    {
        if (m_pVertexContainer)
        {
            return m_pVertexContainer;
        }
        return this;
    }
#  if !defined(NULL_RENDERER)
    D3DBuffer* _GetD3DVB(int nStream, size_t* offs) const;
    D3DBuffer* _GetD3DIB(size_t* offs) const;
# endif

    size_t Size(uint32 nFlags) const;
    void Size(uint32 nFlags, ICrySizer* pSizer) const;

    void* LockVB(int nStream, uint32 nFlags, int nVerts = 0, int* nStride = NULL, bool prefetchIB = false, bool inplaceCachePos = false);

    template<class T>
    T* GetStridedArray(strided_pointer<T>& arr, EStreamIDs stream)
    {
        arr.data = (T*)LockVB(stream, FSL_READ, 0, &arr.iStride);
        assert(!arr.data || arr.iStride >= sizeof(T));
        return arr.data;
    }

    vtx_idx* LockIB(uint32 nFlags, int nOffset = 0, int nInds = 0);
    void UnlockVB(int nStream);
    void UnlockIB();

    bool RT_CheckUpdate(CRenderMesh* pVContainer, uint32 nStreamMask, bool bTessellation = false, bool stall = true);
    void RT_SetMeshCleanup();
    void RT_AllocationFailure(const char* sPurpose, uint32 nSize);
    bool CheckUpdate(uint32 nStreamMask) override;
    void AssignChunk(CRenderChunk * pChunk, class CREMeshImpl * pRE);
    void InitRenderChunk(CRenderChunk& rChunk);

    void FreeVB(int nStream);
    void FreeIB();
    void FreeDeviceBuffers(bool bRestoreSys);
    void FreeSystemBuffers();
    void FreePreallocatedData();

    bool SyncAsyncUpdate(int threadId, bool block = true);

    //===========================================================================================
    // IRenderMesh interface
    virtual const char* GetTypeName()   { return m_sType; }
    virtual const char* GetSourceName() const { return m_sSource; }

    virtual int  GetIndicesCount()  { return m_nInds; }
    virtual int  GetVerticesCount() { return m_nVerts; }

    virtual AZ::Vertex::Format GetVertexFormat() { return m_vertexFormat; }
    virtual ERenderMeshType GetMeshType() { return m_eType; }

    virtual void SetSkinned(bool bSkinned = true) override
    {
        if (bSkinned)
        {
            m_nFlags |=  FRM_SKINNED;
        }
        else
        {
            m_nFlags &= ~FRM_SKINNED;
        }
    };
    virtual uint GetSkinningWeightCount() const override;

    virtual float GetGeometricMeanFaceArea() const{ return m_fGeometricMeanFaceArea; }

    virtual void NextDrawSkinned() { m_nFlags |= FRM_SKINNEDNEXTDRAW; }

    virtual void GenerateQTangents();
    virtual void CreateChunksSkinned();
    virtual void CopyTo(IRenderMesh* pDst, int nAppendVtx = 0, bool bDynamic = false, bool fullCopy = true);
    virtual void SetSkinningDataVegetation(struct SMeshBoneMapping_uint8* pBoneMapping);
    virtual void SetSkinningDataCharacter(CMesh& mesh, struct SMeshBoneMapping_uint16* pBoneMapping, struct SMeshBoneMapping_uint16* pExtraBoneMapping);
    // Creates an indexed mesh from this render mesh (accepts an optional pointer to an IIndexedMesh object that should be used)
    virtual IIndexedMesh* GetIndexedMesh(IIndexedMesh* pIdxMesh = 0);
    virtual int GetRenderChunksCount(_smart_ptr<IMaterial> pMat, int& nRenderTrisCount);

    virtual IRenderMesh* GenerateMorphWeights() { return NULL; }
    virtual IRenderMesh* GetMorphBuddy() { return NULL; }
    virtual void SetMorphBuddy([[maybe_unused]] IRenderMesh* pMorph) {}

    // Create render buffers from render mesh. Returns the final size of the render mesh or ~0U on failure
    virtual size_t SetMesh(CMesh& mesh, int nSecColorsSetOffset, uint32 flags, bool requiresLock);

    // Update system vertices buffer
    virtual bool UpdateVertices(const void* pVertBuffer, int nVertCount, int nOffset, int nStream, uint32 copyFlags, bool requiresLock = true);
    // Update system indices buffer
    virtual bool UpdateIndices(const vtx_idx* pNewInds, int nInds, int nOffsInd, uint32 copyFlags, bool requiresLock = true);

    virtual void SetCustomTexID(int nCustomTID);
    virtual void SetChunk(int nIndex, CRenderChunk& chunk);
    virtual void SetChunk(_smart_ptr<IMaterial> pNewMat, int nFirstVertId, int nVertCount, int nFirstIndexId, int nIndexCount, float texelAreaDensity, const AZ::Vertex::Format& vertexFormat, int nMatID = 0);

    virtual void SetRenderChunks(CRenderChunk* pChunksArray, int nCount, bool bSubObjectChunks);

    virtual TRenderChunkArray& GetChunks() { return m_Chunks; }
    virtual TRenderChunkArray& GetChunksSkinned() { return m_ChunksSkinned; }
    virtual TRenderChunkArray& GetChunksSubObjects() { return m_ChunksSubObjects; }
    virtual IRenderMesh* GetVertexContainer() { return _GetVertexContainer(); }
    virtual void SetVertexContainer(IRenderMesh* pBuf);

    virtual void Render(const struct SRendParams& rParams, CRenderObject* pObj, _smart_ptr<IMaterial> pMaterial, const SRenderingPassInfo& passInfo, bool bSkinned = false);
    virtual void Render(CRenderObject* pObj, const SRenderingPassInfo& passInfo, const SRendItemSorter& rendItemSorter);
    virtual void AddRenderElements(_smart_ptr<IMaterial> pIMatInfo, CRenderObject* pObj, const SRenderingPassInfo& passInfo, int nSortId = EFSLIST_GENERAL, int nAW = 1);
    virtual void SetREUserData(float* pfCustomData, float fFogScale = 0, float fAlpha = 1);
    virtual void AddRE(_smart_ptr<IMaterial> pMaterial, CRenderObject* pObj, IShader* pEf, const SRenderingPassInfo& passInfo, int nList, int nAW, const SRendItemSorter& rendItemSorter);

    virtual void DrawImmediately();

    virtual byte* GetPosPtrNoCache(int32& nStride, uint32 nFlags);
    virtual byte* GetPosPtr(int32& nStride, uint32 nFlags);
    virtual byte* GetNormPtr(int32& nStride, uint32 nFlags);
    virtual byte* GetColorPtr(int32& nStride, uint32 nFlags);
    virtual byte* GetUVPtrNoCache(int32& nStride, uint32 nFlags, uint32 uvSetIndex = 0);
    virtual byte* GetUVPtr(int32& nStride, uint32 nFlags, uint32 uvSetIndex = 0);

    virtual byte* GetTangentPtr(int32& nStride, uint32 nFlags);
    virtual byte* GetQTangentPtr(int32& nStride, uint32 nFlags);

    virtual byte* GetHWSkinPtr(int32& nStride, uint32 nFlags, bool remapped = false);
    virtual byte* GetVelocityPtr(int32& nStride, uint32 nFlags);

    virtual void UnlockStream(int nStream);
    virtual void UnlockIndexStream();

    virtual vtx_idx* GetIndexPtr(uint32 nFlags, int32 nOffset = 0);
    virtual const PodArray<std::pair<int, int> >* GetTrisForPosition(const Vec3& vPos, _smart_ptr<IMaterial> pMaterial);
    virtual float GetExtent(EGeomForm eForm);
    virtual void GetRandomPos(PosNorm& ran, EGeomForm eForm, SSkinningData const* pSkinning = NULL);
    virtual uint32* GetPhysVertexMap() { return NULL; }
    virtual bool IsEmpty();

    virtual size_t GetMemoryUsage(ICrySizer* pSizer, EMemoryUsageArgument nType) const;
    virtual void GetMemoryUsage(ICrySizer* pSizer) const;
    virtual float GetAverageTrisNumPerChunk(_smart_ptr<IMaterial> pMat);
    virtual int GetTextureMemoryUsage(const _smart_ptr<IMaterial> pMaterial, ICrySizer* pSizer = NULL, bool bStreamedIn = true) const;
    // Get allocated only in video memory or only in system memory.
    virtual int GetAllocatedBytes(bool bVideoMem) const;

    virtual void SetBBox(const Vec3& vBoxMin, const Vec3& vBoxMax) { m_vBoxMin = vBoxMin; m_vBoxMax = vBoxMax; }
    virtual void GetBBox(Vec3& vBoxMin, Vec3& vBoxMax) { vBoxMin = m_vBoxMin; vBoxMax = m_vBoxMax; };
    virtual void UpdateBBoxFromMesh();

    // Debug draw this render mesh.
    virtual void DebugDraw(const struct SGeometryDebugDrawInfo& info, uint32 nVisibleChunksMask = ~0, float fExtrdueScale = 0.01f);
    virtual void KeepSysMesh(bool keep);
    virtual void UnKeepSysMesh();
    virtual void LockForThreadAccess();
    virtual void UnLockForThreadAccess();

    virtual void SetMeshLod(int nLod) { m_nLod = nLod; }

    virtual volatile int* SetAsyncUpdateState();
    void CreateRemappedBoneIndicesPair(const uint pairGuid, const TRenderChunkArray& Chunks);
    virtual void CreateRemappedBoneIndicesPair(const DynArray<JointIdType>& arrRemapTable, const uint pairGuid);
    virtual void ReleaseRemappedBoneIndicesPair(const uint pairGuid);

    virtual void OffsetPosition(const Vec3& delta) { m_vBoxMin += delta; m_vBoxMax += delta; }

    IRenderMesh* GetRenderMeshForSubsetMask(SRenderObjData* pOD, uint64 nMeshSubSetMask, _smart_ptr<IMaterial> pMaterial, const SRenderingPassInfo& passInfo);
    void GarbageCollectSubsetRenderMeshes();
    void CreateSubSetRenderMesh();

    void ReleaseRenderChunks(TRenderChunkArray* pChunks);

    void BindStreamsToRenderPipeline();

    bool GetRemappedSkinningData(uint32 guid, CRendElementBase::SGeometryStreamInfo &streamInfo);
    bool FillGeometryInfo(CRendElementBase::SGeometryInfo &geomInfo);

    // --------------------------------------------------------------
    // Members

    static int32 m_cSizeStream[VSF_NUM];

    // When modifying or traversing any of the lists below, be sure to always hold the link lock
    static CryCriticalSection m_sLinkLock;

    // intrusive list entries - a mesh can be in multiple lists at the same time
    util::list<CRenderMesh>          m_Chain; // mesh will either be in the mesh list or garbage mesh list
    util::list<CRenderMesh>          m_Dirty[2]; // if linked, mesh has volatile data (data read back from vram)
    util::list<CRenderMesh>          m_Modified[2]; // if linked, mesh has modified data (to be uploaded to vram)

    // The static list heads, corresponds to the entries above
    static util::list<CRenderMesh> m_MeshList;
    static util::list<CRenderMesh> m_MeshGarbageList[MAX_RELEASED_MESH_FRAMES];
    static util::list<CRenderMesh> m_MeshDirtyList[2];
    static util::list<CRenderMesh> m_MeshModifiedList[2];

    TRenderChunkArray  m_Chunks;
    TRenderChunkArray  m_ChunksSubObjects; // Chunks of sub-objects.
    TRenderChunkArray  m_ChunksSkinned;

    int                     m_nClientTextureBindID;
    Vec3                    m_vBoxMin;
    Vec3                    m_vBoxMax;

    float                   m_fGeometricMeanFaceArea;
    CGeomExtents                        m_Extents;

    // Frame id when this render mesh was last rendered.
    uint32                  m_nLastRenderFrameID;
    uint32                  m_nLastSubsetGCRenderFrameID;

    string                                  m_sType;          //!< pointer to the type name in the constructor call
    string                                  m_sSource;        //!< pointer to the source  name in the constructor call

    // For debugging purposes to catch longstanding data accesses
# if !defined(_RELEASE) && defined(RM_CATCH_EXCESSIVE_LOCKS)
    AZStd::atomic<float> m_lockTime;
# endif

    typedef VectorMap<uint64, _smart_ptr<IRenderMesh> > MeshSubSetIndices;
    MeshSubSetIndices m_meshSubSetIndices;

    static TSRC_ALIGN CThreadSafeRendererContainer<SMeshSubSetIndicesJobEntry> m_meshSubSetRenderMeshJobs[RT_COMMAND_BUF_COUNT];
    static TSRC_ALIGN CThreadSafeRendererContainer<CRenderMesh*> m_deferredSubsetGarbageCollection[RT_COMMAND_BUF_COUNT];

#ifdef RENDER_MESH_TRIANGLE_HASH_MAP_SUPPORT
    CryCriticalSection m_getTrisForPositionLock;
#endif

#if !defined(NULL_RENDERER)
    WrappedDX11Buffer m_extraBonesBuffer;
#ifdef MESH_TESSELLATION_RENDERER
    WrappedDX11Buffer m_adjBuffer; // buffer containing adjacency information to fix displacement seams
#endif
#endif

    static void Initialize();
    static void ShutDown();
    static void Tick();
    static void UpdateModified();
    static void UpdateModifiedMeshes(bool bLocked, int threadId);
    static bool ClearStaleMemory(bool bLocked, int threadId);
    static void PrintMeshLeaks();
    static void GetPoolStats(SMeshPoolStatistics* stats);

    void* operator new(size_t size);
    void operator delete(void* ptr);

    static void FinalizeRendItems(int nThreadID);
};

//////////////////////////////////////////////////////////////////////
// General VertexBuffer created by CreateVertexBuffer() function
class CVertexBuffer
{
public:
    CVertexBuffer()
    {
        m_nVerts = 0;
    }
    CVertexBuffer(void* pData, const AZ::Vertex::Format& vertexFormat, int nVerts = 0)
    {
        m_VS.m_pLocalData = pData;
        m_vertexFormat = vertexFormat;
        m_nVerts = nVerts;
    }
#ifdef _RENDERER
    ~CVertexBuffer();
#endif

    SBufferStream m_VS;
    AZ::Vertex::Format m_vertexFormat;
    int32 m_nVerts;
};

class CIndexBuffer
{
public:
    CIndexBuffer()
    {
        m_nInds = 0;
    }

    CIndexBuffer(uint16* pData)
    {
        m_VS.m_pLocalData = pData;
        m_nInds = 0;
    }
#ifdef _RENDERER
    ~CIndexBuffer();
#endif

    SBufferStream m_VS;
    int32 m_nInds;
};

#endif // CRYINCLUDE_CRYENGINE_RENDERDLL_COMMON_RENDERMESH_H
