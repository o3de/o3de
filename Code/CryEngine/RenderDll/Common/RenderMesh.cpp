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

#include "RenderDll_precompiled.h"
#include "I3DEngine.h"
#include "IIndexedMesh.h"
#include "CGFContent.h"
#include "GeomQuery.h"
#include "RenderMesh.h"
#include "PostProcess/PostEffects.h"
#include "QTangent.h"
#include <Common/Memory/VRAMDrillerBus.h>

#if !defined(NULL_RENDERER)
#include "XRenderD3D9/DriverD3D.h"
#endif

#define RENDERMESH_ASYNC_MEMCPY_THRESHOLD (1 << 10)
#define MESH_DATA_DEFAULT_ALIGN (128u)

#ifdef MESH_TESSELLATION_RENDERER
// Rebuilding the adjaency information needs access to system copies of the buffers. Needs fixing
#undef BUFFER_ENABLE_DIRECT_ACCESS
#define BUFFER_ENABLE_DIRECT_ACCESS 0
#endif

namespace
{
    class CConditionalLock
    {
    public:
        CConditionalLock(CryCriticalSection& lock, bool doConditionalLock)
            : m_critSection(lock)
            , m_doConditionalLock(doConditionalLock)
        {
            if (m_doConditionalLock)
            {
                m_critSection.Lock();
            }
        }
        ~CConditionalLock()
        {
            if (m_doConditionalLock)
            {
                m_critSection.Unlock();
            }
        }

    private:
        CryCriticalSection& m_critSection;
        bool m_doConditionalLock = false;
    };

    static inline void RelinkTail(util::list<CRenderMesh>& instance, util::list<CRenderMesh>& list, int threadId)
    {
        // Conditional lock logic - When multi-threaded rendering is enabled, 
        // this data is double buffered and we must only lock when attempting 
        // to modify the fill thread data.  Only the render thread should  
        // access the process thread data so we don't need to lock in that case. 
        // In single-threaded rendering (editor) we must always lock because
        // the data is not double buffered.
        bool isRenderThread = gRenDev->m_pRT->IsRenderThread();
        bool doConditionalLock = !isRenderThread || threadId == gRenDev->m_pRT->CurThreadFill() || CRenderer::CV_r_multithreaded == 0;

        CConditionalLock lock(CRenderMesh::m_sLinkLock, doConditionalLock);
        instance.relink_tail(&list);
    }

    struct SMeshPool
    {
        IGeneralMemoryHeap* m_MeshDataPool;
        IGeneralMemoryHeap* m_MeshInstancePool;
        void* m_MeshDataMemory;
        void* m_MeshInstanceMemory;
        CryCriticalSection m_MeshPoolCS;
        SMeshPoolStatistics m_MeshDataPoolStats;
        SMeshPool()
            : m_MeshPoolCS()
            , m_MeshDataPool()
            , m_MeshInstancePool()
            , m_MeshDataMemory()
            , m_MeshInstanceMemory()
            , m_MeshDataPoolStats()
        {}
    };
    static SMeshPool s_MeshPool;

    //////////////////////////////////////////////////////////////////////////
    static void* AllocateMeshData(size_t nSize, size_t nAlign = MESH_DATA_DEFAULT_ALIGN, [[maybe_unused]] bool bFlush = false)
    {
        nSize = (nSize + (nAlign - 1)) & ~(nAlign - 1);

        if (s_MeshPool.m_MeshDataPool && s_MeshPool.m_MeshDataPoolStats.nPoolSize > nSize)
        {
try_again:
            s_MeshPool.m_MeshPoolCS.Lock();
            void* ptr = s_MeshPool.m_MeshDataPool->Memalign(nAlign, nSize, "RENDERMESH_POOL");
            if (ptr)
            {
                s_MeshPool.m_MeshDataPoolStats.nPoolInUse += s_MeshPool.m_MeshDataPool->UsableSize(ptr);
                s_MeshPool.m_MeshDataPoolStats.nPoolInUsePeak =
                    std::max(
                        s_MeshPool.m_MeshDataPoolStats.nPoolInUsePeak,
                        s_MeshPool.m_MeshDataPoolStats.nPoolInUse);
                s_MeshPool.m_MeshPoolCS.Unlock();
                return ptr;
            }
            else
            {
                s_MeshPool.m_MeshPoolCS.Unlock();
                // Clean up the stale mesh temporary data - and do it from the main thread.
                if (gRenDev->m_pRT->IsMainThread() && CRenderMesh::ClearStaleMemory(false, gRenDev->m_RP.m_nFillThreadID))
                {
                    goto try_again;
                }
                else if (gRenDev->m_pRT->IsRenderThread() && CRenderMesh::ClearStaleMemory(false, gRenDev->m_RP.m_nProcessThreadID))
                {
                    goto try_again;
                }
            }
            s_MeshPool.m_MeshPoolCS.Lock();
            s_MeshPool.m_MeshDataPoolStats.nFallbacks += nSize;
            s_MeshPool.m_MeshPoolCS.Unlock();
        }
        return CryModuleMemalign(nSize, nAlign);
    }

    //////////////////////////////////////////////////////////////////////////
    static void FreeMeshData(void* ptr)
    {
        if (ptr == NULL)
        {
            return;
        }
        {
            AUTO_LOCK(s_MeshPool.m_MeshPoolCS);
            size_t nSize = 0u;
            if (s_MeshPool.m_MeshDataPool && (nSize = s_MeshPool.m_MeshDataPool->Free(ptr)) > 0)
            {
                s_MeshPool.m_MeshDataPoolStats.nPoolInUse -=
                    (nSize < s_MeshPool.m_MeshDataPoolStats.nPoolInUse) ? nSize : s_MeshPool.m_MeshDataPoolStats.nPoolInUse;
                return;
            }
        }
        CryModuleMemalignFree(ptr);
    }

    template<typename Type>
    static Type* AllocateMeshData(size_t nCount = 1)
    {
        void* pStorage = AllocateMeshData(sizeof(Type) * nCount, std::max((size_t)TARGET_DEFAULT_ALIGN, (size_t)__alignof(Type)));
        if (!pStorage)
        {
            return NULL;
        }
        Type* pTypeArray = reinterpret_cast<Type*>(pStorage);
        for (size_t i = 0; i < nCount; ++i)
        {
            new (pTypeArray + i)Type;
        }
        return pTypeArray;
    }

    static bool InitializePool()
    {
        if (gRenDev->CV_r_meshpoolsize > 0)
        {
            if (s_MeshPool.m_MeshDataPool || s_MeshPool.m_MeshDataMemory)
            {
                CryFatalError("render meshpool already initialized");
                return false;
            }
            size_t poolSize = static_cast<size_t>(gRenDev->CV_r_meshpoolsize) * 1024U;
            s_MeshPool.m_MeshDataMemory = CryModuleMemalign(poolSize, 128u);
            if (!s_MeshPool.m_MeshDataMemory)
            {
                CryFatalError("could not allocate render meshpool");
                return false;
            }

            // Initialize the actual pool
            s_MeshPool.m_MeshDataPool = gEnv->pSystem->GetIMemoryManager()->CreateGeneralMemoryHeap(
                    s_MeshPool.m_MeshDataMemory, poolSize,  "RENDERMESH_POOL");
            s_MeshPool.m_MeshDataPoolStats.nPoolSize = poolSize;
        }
        if (gRenDev->CV_r_meshinstancepoolsize && !s_MeshPool.m_MeshInstancePool)
        {
            size_t poolSize = static_cast<size_t>(gRenDev->CV_r_meshinstancepoolsize) * 1024U;
            s_MeshPool.m_MeshInstanceMemory = CryModuleMemalign(poolSize, 128u);
            if (!s_MeshPool.m_MeshInstanceMemory)
            {
                CryFatalError("could not allocate render mesh instance pool");
                return false;
            }

            s_MeshPool.m_MeshInstancePool = gEnv->pSystem->GetIMemoryManager()->CreateGeneralMemoryHeap(
                    s_MeshPool.m_MeshInstanceMemory, poolSize, "RENDERMESH_INSTANCE_POOL");
            s_MeshPool.m_MeshDataPoolStats.nInstancePoolInUse = 0;
            s_MeshPool.m_MeshDataPoolStats.nInstancePoolInUsePeak = 0;
            s_MeshPool.m_MeshDataPoolStats.nInstancePoolSize = gRenDev->CV_r_meshinstancepoolsize * 1024;
        }
        return true;
    }

    static void ShutdownPool()
    {
        if (s_MeshPool.m_MeshDataPool)
        {
            s_MeshPool.m_MeshDataPool->Release();
            s_MeshPool.m_MeshDataPool = NULL;
        }
        if (s_MeshPool.m_MeshDataMemory)
        {
            CryModuleMemalignFree(s_MeshPool.m_MeshDataMemory);
            s_MeshPool.m_MeshDataMemory = NULL;
        }
        if (s_MeshPool.m_MeshInstancePool)
        {
            s_MeshPool.m_MeshInstancePool->Cleanup();
            s_MeshPool.m_MeshInstancePool->Release();
            s_MeshPool.m_MeshInstancePool = NULL;
        }
        if (s_MeshPool.m_MeshInstanceMemory)
        {
            CryModuleMemalignFree(s_MeshPool.m_MeshInstanceMemory);
            s_MeshPool.m_MeshInstanceMemory = NULL;
        }
    }

    static void* AllocateMeshInstanceData(size_t size, size_t align)
    {
        if (s_MeshPool.m_MeshInstancePool)
        {
            if (void* ptr = s_MeshPool.m_MeshInstancePool->Memalign(align, size, "rendermesh instance data"))
            {
#       if !defined(_RELEASE)
                AUTO_LOCK(s_MeshPool.m_MeshPoolCS);
                s_MeshPool.m_MeshDataPoolStats.nInstancePoolInUsePeak = std::max(
                        s_MeshPool.m_MeshDataPoolStats.nInstancePoolInUsePeak,
                        s_MeshPool.m_MeshDataPoolStats.nInstancePoolInUse += size);
#       endif
                return ptr;
            }
        }
        return CryModuleMemalign(size, align);
    }

    static void FreeMeshInstanceData(void* ptr)
    {
        size_t size = 0u;
        if (s_MeshPool.m_MeshInstancePool && (size = s_MeshPool.m_MeshInstancePool->UsableSize(ptr)))
        {
#     if !defined(_RELEASE)
            AUTO_LOCK(s_MeshPool.m_MeshPoolCS);
            s_MeshPool.m_MeshDataPoolStats.nInstancePoolInUse -= size;
#     endif
            s_MeshPool.m_MeshInstancePool->Free(ptr);
            return;
        }
        CryModuleMemalignFree(ptr);
    }
}

#define alignup(alignment, value)   ((((uintptr_t)(value)) + ((alignment) - 1)) & (~((uintptr_t)(alignment) - 1)))
#define alignup16(value)                        alignup(16, value)

namespace
{
    inline uint32 GetCurrentRenderFrameID()
    {
        ASSERT_IS_MAIN_THREAD(gRenDev->m_pRT);
        return gRenDev->m_RP.m_TI[gRenDev->m_RP.m_nFillThreadID].m_nFrameUpdateID;
    };
}

#if defined(USE_VBIB_PUSH_DOWN)
static inline bool VidMemPushDown(void* pDst, const void* pSrc, size_t nSize, void* pDst1 = NULL, const void* pSrc1 = NULL, size_t nSize1 = 0, int cachePosStride = -1, void* pFP16Dst = NULL, uint32 nVerts = 0)
{
}
    #define ASSERT_LOCK
static std::vector<CRenderMesh*> g_MeshCleanupVec;    //for cleanup of meshes itself
#else
    #define ASSERT_LOCK assert((m_nVerts == 0) || pData)
#endif

#if !defined(_RELEASE)
ILINE void CheckVideoBufferAccessViolation([[maybe_unused]] CRenderMesh& mesh)
{
    //LogWarning("CRenderMesh::LockVB: accessing video buffer for cgf=%s",mesh.GetSourceName());
}
    #define MESSAGE_VIDEO_BUFFER_ACC_ATTEMPT CheckVideoBufferAccessViolation(*this)
#else
    #define MESSAGE_VIDEO_BUFFER_ACC_ATTEMPT
#endif

CryCriticalSection CRenderMesh::m_sLinkLock;

// Additional streams stride
int32 CRenderMesh::m_cSizeStream[VSF_NUM] =
{
    -1,
    sizeof(SPipTangents),      // VSF_TANGENTS
    sizeof(SPipQTangents),     // VSF_QTANGENTS
    sizeof(SVF_W4B_I4S),       // VSF_HWSKIN_INFO
    sizeof(SVF_P3F),           // VSF_VERTEX_VELOCITY
# if ENABLE_NORMALSTREAM_SUPPORT
    sizeof(SPipNormal),        // VSF_NORMALS
#endif
};

util::list<CRenderMesh> CRenderMesh::m_MeshList;
util::list<CRenderMesh> CRenderMesh::m_MeshGarbageList[MAX_RELEASED_MESH_FRAMES];
util::list<CRenderMesh> CRenderMesh::m_MeshDirtyList[2];
util::list<CRenderMesh> CRenderMesh::m_MeshModifiedList[2];

int CRenderMesh::Release()
{
    long refCnt = CryInterlockedDecrement(&m_nRefCounter);
# if !defined(_RELEASE)
    if (refCnt < 0)
    {
        CryLogAlways("CRenderMesh::Release() called so many times on rendermesh that refcount became negative");
        if (CRenderer::CV_r_BreakOnError)
        {
            __debugbreak();
        }
    }
# endif
    if (refCnt == 0)
    {
        AUTO_LOCK(m_sLinkLock);
#   if !defined(_RELEASE)
        if (m_nFlags & FRM_RELEASED)
        {
            CryLogAlways("CRenderMesh::Release() mesh already in the garbage list (double delete pending)");
            if (CRenderer::CV_r_BreakOnError)
            {
                __debugbreak();
            }
        }
#   endif
        m_nFlags |= FRM_RELEASED;
        int nFrame = gRenDev->GetFrameID(false);
        util::list<CRenderMesh>* garbage = &CRenderMesh::m_MeshGarbageList[nFrame & (MAX_RELEASED_MESH_FRAMES - 1)];
        m_Chain.relink_tail(garbage);
    }

    return refCnt;
}

CRenderMesh::CRenderMesh()
{
#if defined(USE_VBIB_PUSH_DOWN)
    m_VBIBFramePushID = 0;
#endif
    memset(m_VBStream, 0x0, sizeof(m_VBStream));

    SMeshStream* streams = (SMeshStream*)AllocateMeshInstanceData(sizeof(SMeshStream) * VSF_NUM, 64u);
    for (signed i = 0; i < VSF_NUM; ++i)
    {
        new (m_VBStream[i] = &streams[i])SMeshStream();
    }

    m_keepSysMesh = false;
    m_nLastRenderFrameID = 0;
    m_nLastSubsetGCRenderFrameID = 0;
    m_nThreadAccessCounter = 0;
    for (size_t i = 0; i < 2; ++i)
    {
        m_asyncUpdateState[i] = m_asyncUpdateStateCounter[i] = 0;
    }

# if !defined(_RELEASE) && defined(RM_CATCH_EXCESSIVE_LOCKS)
    m_lockTime = 0.f;
# endif
}

CRenderMesh::CRenderMesh (const char* szType, const char* szSourceName, bool bLock)
{
    memset(m_VBStream, 0x0, sizeof(m_VBStream));

    SMeshStream* streams = (SMeshStream*)AllocateMeshInstanceData(sizeof(SMeshStream) * VSF_NUM, 64u);
    for (signed i = 0; i < VSF_NUM; ++i)
    {
        new (m_VBStream[i] = &streams[i])SMeshStream();
    }

    m_keepSysMesh = false;
    m_nRefCounter = 0;
    m_nLastRenderFrameID = 0;
    m_nLastSubsetGCRenderFrameID = 0;

    m_sType = szType;
    m_sSource = szSourceName;

    m_vBoxMin = m_vBoxMax = Vec3(0, 0, 0); //used for hw occlusion test
    m_nVerts = 0;
    m_nInds = 0;
    m_vertexFormat = AZ::Vertex::Format(eVF_P3F_C4B_T2F);
    m_pVertexContainer = NULL;

    {
        AUTO_LOCK(m_sLinkLock);
        m_Chain.relink_tail(&m_MeshList);
    }
    m_nPrimetiveType = eptTriangleList;

    //  m_nFrameRender = 0;
    //m_nFrameUpdate = 0;
    m_nClientTextureBindID = 0;
#ifdef RENDER_MESH_TRIANGLE_HASH_MAP_SUPPORT
    m_pTrisMap = NULL;
#endif

    m_pCachePos = NULL;
    m_nFrameRequestCachePos = 0;
    m_nFlagsCachePos = 0;

    m_nFrameRequestCacheUVs = 0;
    m_nFlagsCacheUVs = 0;

    _SetRenderMeshType(eRMT_Static);

    m_nFlags = 0;
    m_fGeometricMeanFaceArea = 0.f;
    m_nLod = 0;

#if defined(USE_VBIB_PUSH_DOWN)
    m_VBIBFramePushID = 0;
#endif

    m_nThreadAccessCounter = 0;
    for (size_t i = 0; i < 2; ++i)
    {
        m_asyncUpdateState[i] = m_asyncUpdateStateCounter[i] = 0;
    }

    IF (bLock, 0)//when called from another thread like the Streaming AsyncCallbackCGF, we need to lock it
    {
        LockForThreadAccess();
    }
}

void CRenderMesh::Cleanup()
{
    FreeDeviceBuffers(false);
    FreeSystemBuffers();

    m_meshSubSetIndices.clear();

    if (m_pVertexContainer)
    {
        m_pVertexContainer->m_lstVertexContainerUsers.Delete(this);
        m_pVertexContainer = NULL;
    }

    for (int i = 0; i < m_lstVertexContainerUsers.Count(); i++)
    {
        if (m_lstVertexContainerUsers[i]->GetVertexContainer() == this)
        {
            m_lstVertexContainerUsers[i]->m_pVertexContainer = NULL;
        }
    }
    m_lstVertexContainerUsers.Clear();

    ReleaseRenderChunks(&m_ChunksSkinned);
    ReleaseRenderChunks(&m_ChunksSubObjects);
    ReleaseRenderChunks(&m_Chunks);

    m_ChunksSubObjects.clear();
    m_Chunks.clear();

#ifdef RENDER_MESH_TRIANGLE_HASH_MAP_SUPPORT
    SAFE_DELETE(m_pTrisMap);
#endif

    for (size_t i = 0; i < 2; ++i)
    {
        m_asyncUpdateState[i] = m_asyncUpdateStateCounter[i] = 0;
    }

    for (int i = 0; i < VSF_NUM; ++i)
    {
        if (m_VBStream[i])
        {
            m_VBStream[i]->~SMeshStream();
        }
    }
    FreeMeshInstanceData(m_VBStream[0]);
    memset(m_VBStream, 0, sizeof(m_VBStream));

    for (size_t j = 0; j < 2; ++j)
    {
        for (size_t i = 0, end = m_CreatedBoneIndices[j].size(); i < end; ++i)
        {
            delete[] m_CreatedBoneIndices[j][i].pStream;
        }
    }
    for (size_t i = 0, end = m_RemappedBoneIndices.size(); i < end; ++i)
    {
        if (m_RemappedBoneIndices[i].refcount && m_RemappedBoneIndices[i].guid != ~0u)
        {
            CryLogAlways("remapped bone indices with refcount '%d' still around for '%s 0x%p\n", m_RemappedBoneIndices[i].refcount, m_sSource.c_str(), this);
        }
        if (m_RemappedBoneIndices[i].buffer != ~0u)
        {
            // Unregister the allocation with the VRAM driller
            void* address = reinterpret_cast<void*>(m_RemappedBoneIndices[i].buffer);
            EBUS_EVENT(Render::Debug::VRAMDrillerBus, UnregisterAllocation, address);

            gRenDev->m_DevBufMan.Destroy(m_RemappedBoneIndices[i].buffer);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
CRenderMesh::~CRenderMesh()
{
    // make sure to stop and delete all mesh subset indice tasks
    ASSERT_IS_RENDER_THREAD(gRenDev->m_pRT);

    int nThreadID = gRenDev->m_RP.m_nProcessThreadID;
    for (int i = 0; i < RT_COMMAND_BUF_COUNT; SyncAsyncUpdate(i++))
    {
        ;
    }

    // make sure no subset rendermesh job is still running which uses this mesh
    for (int j = 0; j < RT_COMMAND_BUF_COUNT; ++j)
    {
        size_t nNumSubSetRenderMeshJobs = m_meshSubSetRenderMeshJobs[j].size();
        for (size_t i = 0; i < nNumSubSetRenderMeshJobs; ++i)
        {
            SMeshSubSetIndicesJobEntry& rSubSetJob = m_meshSubSetRenderMeshJobs[j][i];
            if (rSubSetJob.m_pSrcRM == this)
            {
                rSubSetJob.jobExecutor.WaitForCompletion();
                rSubSetJob.m_pSrcRM = NULL;
            }
        }
    }

    // remove ourself from deferred subset mesh garbage collection
    for (size_t i = 0; i < m_deferredSubsetGarbageCollection[nThreadID].size(); ++i)
    {
        if (m_deferredSubsetGarbageCollection[nThreadID][i] == this)
        {
            m_deferredSubsetGarbageCollection[nThreadID][i] = NULL;
        }
    }

    assert(m_nThreadAccessCounter == 0);

    {
        AUTO_LOCK(m_sLinkLock);
        for (int i = 0; i < 2; ++i)
        {
            m_Dirty[i].erase(), m_Modified[i].erase();
        }
        m_Chain.erase();
    }

    Cleanup();
}

void CRenderMesh::ReleaseRenderChunks(TRenderChunkArray* pChunks)
{
    if (pChunks)
    {
        for (size_t i = 0, c = pChunks->size(); i != c; ++i)
        {
            CRenderChunk& rChunk = pChunks->at(i);

            if (rChunk.pRE)
            {
                CREMeshImpl* pRE = static_cast<CREMeshImpl*>(rChunk.pRE);
                pRE->Release(false);
                pRE->m_pRenderMesh = NULL;
                rChunk.pRE = 0;
            }
        }
    }
}

SMeshStream* CRenderMesh::GetVertexStream(int nStream, uint32 nFlags)
{
    SMeshStream*& pMS = m_VBStream[nStream];
    IF (!pMS && (nFlags & FSL_WRITE), 0)
    {
        pMS = new (AllocateMeshInstanceData(sizeof(SMeshStream), alignof(SMeshStream)))SMeshStream;
    }
    return pMS;
}

void* CRenderMesh::LockVB(int nStream, uint32 nFlags, int nVerts, int* nStride, [[maybe_unused]] bool prefetchIB, [[maybe_unused]] bool inplaceCachePos)
{
    FUNCTION_PROFILER_RENDERER;
# if !defined(_RELEASE)
    if (!m_nThreadAccessCounter)
    {
        CryLogAlways("rendermesh must be locked via LockForThreadAccess() before LockIB/VB is called");
        if (CRenderer::CV_r_BreakOnError)
        {
            __debugbreak();
        }
    }
#endif
    const int threadId = gRenDev->m_RP.m_nFillThreadID;

    if (!CanRender()) // if allocation failure suffered, don't lock anything anymore
    {
        return NULL;
    }

    SREC_AUTO_LOCK(m_sResLock);//need lock as resource must not be updated concurrently
    SMeshStream* MS = GetVertexStream(nStream, nFlags);

#if defined(USE_VBIB_PUSH_DOWN)
    m_VBIBFramePushID = gRenDev->m_RP.m_TI[gRenDev->m_RP.m_nFillThreadID].m_nFrameUpdateID;
    if (nFlags == FSL_SYSTEM_CREATE || nFlags == FSL_SYSTEM_UPDATE)
    {
        MS->m_nLockFlags &= ~FSL_VBIBPUSHDOWN;
    }
#endif

    assert(nVerts <= (int)m_nVerts);
    if (nVerts > (int)m_nVerts)
    {
        nVerts = m_nVerts;
    }
    if (nStride)
    {
        *nStride = GetStreamStride(nStream);
    }

    m_nFlags |= FRM_READYTOUPLOAD;

    byte* pD;
    int nFrame = gRenDev->m_RP.m_TI[gRenDev->m_RP.m_nFillThreadID].m_nFrameUpdateID;

    if (nFlags == FSL_SYSTEM_CREATE)
    {
lSysCreate:
        RelinkTail(m_Modified[threadId], m_MeshModifiedList[threadId], threadId);
        if (!MS->m_pUpdateData)
        {
            uint32 nSize = GetStreamSize(nStream);
            pD = reinterpret_cast<byte*>(AllocateMeshData(nSize));
            if (!pD)
            {
                return NULL;
            }
            MS->m_pUpdateData = pD;
        }
        else
        {
            pD = (byte*)MS->m_pUpdateData;
        }
        //MS->m_nFrameRequest = nFrame;
        MS->m_nLockFlags = (FSL_SYSTEM_CREATE | (MS->m_nLockFlags & FSL_LOCKED));
        return pD;
    }
    else
    if (nFlags == FSL_SYSTEM_UPDATE)
    {
lSysUpdate:
        RelinkTail(m_Modified[threadId], m_MeshModifiedList[threadId], threadId);
        if (!MS->m_pUpdateData)
        {
            MESSAGE_VIDEO_BUFFER_ACC_ATTEMPT;
            CopyStreamToSystemForUpdate(*MS, GetStreamSize(nStream));
        }
        assert(nStream || MS->m_pUpdateData);
        if (!MS->m_pUpdateData)
        {
            return NULL;
        }
        //MS->m_nFrameRequest = nFrame;
        pD = (byte*)MS->m_pUpdateData;
        MS->m_nLockFlags = (nFlags | (MS->m_nLockFlags & FSL_LOCKED));
        return pD;
    }
    else if (nFlags == FSL_READ)
    {
        if (!MS)
        {
            return NULL;
        }
        RelinkTail(m_Dirty[threadId], m_MeshDirtyList[threadId], threadId);
        if (MS->m_pUpdateData)
        {
            pD = (byte*)MS->m_pUpdateData;
            return pD;
        }
        nFlags = FSL_READ | FSL_VIDEO;
    }

    if (nFlags == (FSL_READ | FSL_VIDEO))
    {
        if (!MS)
        {
            return NULL;
        }
        RelinkTail(m_Dirty[threadId], m_MeshDirtyList[threadId], threadId);
#   if !BUFFER_ENABLE_DIRECT_ACCESS || defined(NULL_RENDERER)
        if (gRenDev->m_pRT && gRenDev->m_pRT->IsMultithreaded())
        {
            // Always use system copy in MT mode
            goto lSysUpdate;
        }
        else
#   endif
        {
            buffer_handle_t nVB = MS->m_nID;
            if (nVB == ~0u)
            {
                return NULL;
            }
            // Try to lock device buffer in single-threaded mode
            if (!MS->m_pLockedData)
            {
                MESSAGE_VIDEO_BUFFER_ACC_ATTEMPT;
                if (MS->m_pLockedData = gRenDev->m_DevBufMan.BeginRead(nVB))
                {
                    MS->m_nLockFlags |= FSL_LOCKED;
                }
            }
            if (MS->m_pLockedData)
            {
                ++MS->m_nLockCount;
                pD = (byte*)MS->m_pLockedData;
                return pD;
            }
        }
    }
    if (nFlags == (FSL_VIDEO_CREATE))
    {
        // Only consoles support direct uploading to vram, but as the data is not
        // double buffered in the non-dynamic case, only creation is supported
        // DX11 has to use the deferred context to call map, which is not threadsafe
        // DX9 can experience huge stalls if resources are used while rendering is performed
        buffer_handle_t nVB = ~0u;
#   if BUFFER_ENABLE_DIRECT_ACCESS && !defined(NULL_RENDERER)
        nVB = MS->m_nID;
        if ((nVB != ~0u && (MS->m_nFrameCreate != nFrame || MS->m_nElements != m_nVerts)) || !CRenderer::CV_r_buffer_enable_lockless_updates)
#   endif
        goto lSysCreate;
        RelinkTail(m_Modified[threadId], m_MeshModifiedList[threadId], threadId);
        if (nVB == ~0u && !CreateVidVertices(nStream))
        {
            RT_AllocationFailure("Create VB-Stream", GetStreamSize(nStream, m_nVerts));
            return NULL;
        }
        nVB = MS->m_nID;
        if (!MS->m_pLockedData)
        {
            MESSAGE_VIDEO_BUFFER_ACC_ATTEMPT;
            if ((MS->m_pLockedData = gRenDev->m_DevBufMan.BeginWrite(nVB)) == NULL)
            {
                return NULL;
            }
            MS->m_nLockFlags |= FSL_DIRECT | FSL_LOCKED;
        }
        ++MS->m_nLockCount;
        pD = (byte*)MS->m_pLockedData;
        return pD;
    }
    if (nFlags == (FSL_VIDEO_UPDATE))
    {
        goto lSysUpdate;
    }

    return NULL;
}

vtx_idx* CRenderMesh::LockIB(uint32 nFlags, int nOffset, [[maybe_unused]] int nInds)
{
    FUNCTION_PROFILER_RENDERER;
    
    byte* pD;
# if !defined(_RELEASE)
    if (!m_nThreadAccessCounter)
    {
        CryLogAlways("rendermesh must be locked via LockForThreadAccess() before LockIB/VB is called");
        if (CRenderer::CV_r_BreakOnError)
        {
            __debugbreak();
        }
    }
#endif
    if (!CanRender()) // if allocation failure suffered, don't lock anything anymore
    {
        return NULL;
    }

    const int threadId = gRenDev->m_RP.m_nFillThreadID;

    int nFrame = gRenDev->m_RP.m_TI[gRenDev->m_RP.m_nFillThreadID].m_nFrameUpdateID;
    SREC_AUTO_LOCK(m_sResLock);//need lock as resource must not be updated concurrently

#if defined(USE_VBIB_PUSH_DOWN)
    m_VBIBFramePushID = gRenDev->m_RP.m_TI[gRenDev->m_RP.m_nFillThreadID].m_nFrameUpdateID;
    if (nFlags == FSL_SYSTEM_CREATE || nFlags == FSL_SYSTEM_UPDATE)
    {
        m_IBStream.m_nLockFlags &= ~FSL_VBIBPUSHDOWN;
    }
#endif
    m_nFlags |= FRM_READYTOUPLOAD;

    assert(nInds <= (int)m_nInds);
    if (nFlags == FSL_SYSTEM_CREATE)
    {
lSysCreate:
        RelinkTail(m_Modified[threadId], m_MeshModifiedList[threadId], threadId);
        if (!m_IBStream.m_pUpdateData)
        {
            uint32 nSize = m_nInds * sizeof(vtx_idx);
            pD = reinterpret_cast<byte*>(AllocateMeshData(nSize));
            if (!pD)
            {
                return NULL;
            }
            m_IBStream.m_pUpdateData = (vtx_idx*)pD;
        }
        else
        {
            pD = (byte*)m_IBStream.m_pUpdateData;
        }
        //m_IBStream.m_nFrameRequest = nFrame;
        m_IBStream.m_nLockFlags = (nFlags | (m_IBStream.m_nLockFlags & FSL_LOCKED));
        return (vtx_idx*)&pD[nOffset];
    }
    else
    if (nFlags == FSL_SYSTEM_UPDATE)
    {
lSysUpdate:
        RelinkTail(m_Modified[threadId], m_MeshModifiedList[threadId], threadId);
        if (!m_IBStream.m_pUpdateData)
        {
            MESSAGE_VIDEO_BUFFER_ACC_ATTEMPT;
            CopyStreamToSystemForUpdate(m_IBStream, sizeof(vtx_idx) * m_nInds);
        }
        assert(m_IBStream.m_pUpdateData);
        if (!m_IBStream.m_pUpdateData)
        {
            return NULL;
        }
        //m_IBStream.m_nFrameRequest = nFrame;
        pD = (byte*)m_IBStream.m_pUpdateData;
        m_IBStream.m_nLockFlags = (nFlags | (m_IBStream.m_nLockFlags & FSL_LOCKED));
        return (vtx_idx*)&pD[nOffset];
    }
    else if (nFlags == FSL_READ)
    {
        RelinkTail(m_Dirty[threadId], m_MeshDirtyList[threadId], threadId);
        if (m_IBStream.m_pUpdateData)
        {
            pD = (byte*)m_IBStream.m_pUpdateData;
            return (vtx_idx*)&pD[nOffset];
        }
        nFlags = FSL_READ | FSL_VIDEO;
    }

    if (nFlags == (FSL_READ | FSL_VIDEO))
    {
        RelinkTail(m_Dirty[threadId], m_MeshDirtyList[threadId], threadId);
        buffer_handle_t nIB = m_IBStream.m_nID;
        if (nIB == ~0u)
        {
            return NULL;
        }
        if (gRenDev->m_pRT && gRenDev->m_pRT->IsMultithreaded())
        {
            // Always use system copy in MT mode
            goto lSysUpdate;
        }
        else
        {
            // TODO: make smart caching mesh algorithm for consoles
            if (!m_IBStream.m_pLockedData)
            {
                MESSAGE_VIDEO_BUFFER_ACC_ATTEMPT;
                if (m_IBStream.m_pLockedData = gRenDev->m_DevBufMan.BeginRead(nIB))
                {
                    m_IBStream.m_nLockFlags |= FSL_LOCKED;
                }
            }
            if (m_IBStream.m_pLockedData)
            {
                pD = (byte*)m_IBStream.m_pLockedData;
                ++m_IBStream.m_nLockCount;
                return (vtx_idx*)&pD[nOffset];
            }
        }
    }
    if (nFlags == (FSL_VIDEO_CREATE))
    {
        // Only consoles support direct uploading to vram, but as the data is not
        // double buffered, only creation is supported
        // DX11 has to use the deferred context to call map, which is not threadsafe
        // DX9 can experience huge stalls if resources are used while rendering is performed
        buffer_handle_t nIB = -1;
#   if BUFFER_ENABLE_DIRECT_ACCESS && !defined(NULL_RENDERER)
        nIB = m_IBStream.m_nID;
        if ((nIB != ~0u && (m_IBStream.m_nFrameCreate || m_IBStream.m_nElements != m_nInds)) || !CRenderer::CV_r_buffer_enable_lockless_updates)
#   endif
        goto lSysCreate;
        RelinkTail(m_Modified[threadId], m_MeshModifiedList[threadId], threadId);
        if (m_IBStream.m_nID == ~0u)
        {
            size_t bufferSize = m_nInds * sizeof(vtx_idx);
            nIB = (m_IBStream.m_nID = gRenDev->m_DevBufMan.Create(BBT_INDEX_BUFFER, (BUFFER_USAGE)m_eType, bufferSize));
            m_IBStream.m_nFrameCreate = nFrame;

            // Register the allocation with the VRAM driller
            void* address = reinterpret_cast<void*>(nIB);
            const char* bufferName = GetSourceName();
            EBUS_EVENT(Render::Debug::VRAMDrillerBus, RegisterAllocation, address, bufferSize, bufferName, Render::Debug::VRAM_CATEGORY_BUFFER, Render::Debug::VRAM_SUBCATEGORY_BUFFER_INDEX_BUFFER);
        }
        if (nIB == ~0u)
        {
            RT_AllocationFailure("Create IB-Stream", m_nInds * sizeof(vtx_idx));
            return NULL;
        }
        m_IBStream.m_nElements = m_nInds;
        if (!m_IBStream.m_pLockedData)
        {
            MESSAGE_VIDEO_BUFFER_ACC_ATTEMPT;
            if ((m_IBStream.m_pLockedData = gRenDev->m_DevBufMan.BeginWrite(nIB)) == NULL)
            {
                return NULL;
            }
            m_IBStream.m_nLockFlags |= FSL_DIRECT | FSL_LOCKED;
        }
        ++m_IBStream.m_nLockCount;
        pD = (byte*)m_IBStream.m_pLockedData;
        return (vtx_idx*)&pD[nOffset];
    }
    if (nFlags == (FSL_VIDEO_UPDATE))
    {
        goto lSysUpdate;
    }

    assert(0);

    return NULL;
}

ILINE void CRenderMesh::UnlockVB(int nStream)
{
    SREC_AUTO_LOCK(m_sResLock);
    SMeshStream* pMS = GetVertexStream(nStream, 0);
    if (pMS && pMS->m_nLockFlags & FSL_LOCKED)
    {
        assert(pMS->m_nLockCount);
        if ((--pMS->m_nLockCount) == 0)
        {
            gRenDev->m_DevBufMan.EndReadWrite(pMS->m_nID);
            pMS->m_nLockFlags &= ~FSL_LOCKED;
            pMS->m_pLockedData = NULL;
        }
    }
    if (pMS && (pMS->m_nLockFlags & FSL_WRITE) && (pMS->m_nLockFlags & (FSL_SYSTEM_CREATE | FSL_SYSTEM_UPDATE)))
    {
        pMS->m_nLockFlags &= ~(FSL_SYSTEM_CREATE | FSL_SYSTEM_UPDATE);
        pMS->m_nFrameRequest = gRenDev->m_RP.m_TI[gRenDev->m_RP.m_nFillThreadID].m_nFrameUpdateID;
    }
}

ILINE void CRenderMesh::UnlockIB()
{
    SREC_AUTO_LOCK(m_sResLock);
    if (m_IBStream.m_nLockFlags & FSL_LOCKED)
    {
        assert(m_IBStream.m_nLockCount);
        if ((--m_IBStream.m_nLockCount) == 0)
        {
            gRenDev->m_DevBufMan.EndReadWrite(m_IBStream.m_nID);
            m_IBStream.m_nLockFlags &= ~FSL_LOCKED;
            m_IBStream.m_pLockedData = NULL;
        }
    }
    if ((m_IBStream.m_nLockFlags & FSL_WRITE) && (m_IBStream.m_nLockFlags & (FSL_SYSTEM_CREATE | FSL_SYSTEM_UPDATE)))
    {
        m_IBStream.m_nLockFlags &= ~(FSL_SYSTEM_CREATE | FSL_SYSTEM_UPDATE);
        m_IBStream.m_nFrameRequest = gRenDev->m_RP.m_TI[gRenDev->m_RP.m_nFillThreadID].m_nFrameUpdateID;
    }
}

void CRenderMesh::UnlockStream(int nStream)
{
    UnlockVB(nStream);
    SREC_AUTO_LOCK(m_sResLock);

    if (nStream == VSF_GENERAL)
    {
        if (m_nFlagsCachePos && m_pCachePos)
        {
            uint32 i;
            int nStride;
            byte* pDst = (byte*)LockVB(nStream, FSL_SYSTEM_UPDATE, m_nVerts, &nStride);
            assert(pDst);
            if (pDst)
            {
                for (i = 0; i < m_nVerts; i++)
                {
                    Vec3f16* pVDst = (Vec3f16*)pDst;
                    *pVDst = m_pCachePos[i];
                    pDst += nStride;
                }
            }
            m_nFlagsCachePos = 0;
        }

        if (m_nFlagsCacheUVs && m_UVCache.size() > 0)
        {
            int nStride;
            byte* streamStart = (byte*)LockVB(nStream, FSL_SYSTEM_UPDATE, m_nVerts, &nStride);

            // Iterate over the cached uv coordinates and write them to the vertex buffer stream
            for (int uvSet = 0; uvSet < m_UVCache.size(); ++uvSet)
            {
                if (m_UVCache[uvSet])
                {
                    uint texCoordOffset = 0;
                    GetVertexFormat().TryCalculateOffset(texCoordOffset, AZ::Vertex::AttributeUsage::TexCoord, uvSet);
                    byte* texCoord = streamStart + texCoordOffset;

                    assert(streamStart);
                    if (streamStart)
                    {
                        for (uint32 i = 0; i < m_nVerts; i++)
                        {
                            Vec2f16* pVDst = (Vec2f16*)texCoord;
                            *pVDst = m_UVCache[uvSet][i];
                            texCoord += nStride;
                        }
                    }
                }
            }
            m_nFlagsCacheUVs = 0;
        }
    }

    SMeshStream* pMS = GetVertexStream(nStream, 0);
    IF (pMS, 1)
    {
        pMS->m_nLockFlags &= ~(FSL_WRITE | FSL_READ | FSL_SYSTEM | FSL_VIDEO);
    }
}
void CRenderMesh::UnlockIndexStream()
{
    
    UnlockIB();
    m_IBStream.m_nLockFlags &= ~(FSL_WRITE | FSL_READ | FSL_SYSTEM | FSL_VIDEO);
}

bool CRenderMesh::CopyStreamToSystemForUpdate(SMeshStream& MS, size_t nSize)
{    
    FUNCTION_PROFILER_RENDERER;
    SREC_AUTO_LOCK(m_sResLock);
    if (!MS.m_pUpdateData)
    {
        buffer_handle_t nVB = MS.m_nID;
        if (nVB == ~0u)
        {
            return false;
        }
        void* pSrc = MS.m_pLockedData;
        if (!pSrc)
        {
            pSrc = gRenDev->m_DevBufMan.BeginRead(nVB);
            MS.m_nLockFlags |= FSL_LOCKED;
        }
        assert(pSrc);
        if (!pSrc)
        {
            return false;
        }
        ++MS.m_nLockCount;
        byte* pD = reinterpret_cast<byte*>(AllocateMeshData(nSize, MESH_DATA_DEFAULT_ALIGN, false));
        if (pD)
        {
            cryMemcpy(pD, pSrc, nSize);
            if (MS.m_nLockFlags & FSL_LOCKED)
            {
                if ((--MS.m_nLockCount) == 0)
                {
                    MS.m_nLockFlags &= ~FSL_LOCKED;
                    MS.m_pLockedData = NULL;
                    gRenDev->m_DevBufMan.EndReadWrite(nVB);
                }
            }
            MS.m_pUpdateData = pD;
            m_nFlags |= FRM_READYTOUPLOAD;
            return true;
        }
    }
    return false;
}

size_t CRenderMesh::SetMesh_Int(CMesh& mesh, [[maybe_unused]] int nSecColorsSetOffset, uint32 flags)
{
    LOADING_TIME_PROFILE_SECTION;
    char* pVBuff = NULL;
    SPipTangents* pTBuff = NULL;
    SPipQTangents* pQTBuff = NULL;
    SVF_P3F* pVelocities = NULL;
    SPipNormal* pNBuff = NULL;
    uint32 nVerts = mesh.GetVertexCount();
    uint32 nInds = mesh.GetIndexCount();
    vtx_idx* pInds = NULL;

    //AUTO_LOCK(m_sResLock);//need a resource lock as mesh could be reseted due to allocation failure
    LockForThreadAccess();

    ReleaseRenderChunks(&m_ChunksSkinned);

    m_vBoxMin = mesh.m_bbox.min;
    m_vBoxMax = mesh.m_bbox.max;

    m_fGeometricMeanFaceArea = mesh.m_geometricMeanFaceArea;

    //////////////////////////////////////////////////////////////////////////
    // Initialize Render Chunks.
    //////////////////////////////////////////////////////////////////////////
    uint32 numSubsets = mesh.GetSubSetCount();

    uint32 numChunks = 0;
    for (uint32 i = 0; i < numSubsets; i++)
    {
        if (mesh.m_subsets[i].nNumIndices == 0)
        {
            continue;
        }

        if (mesh.m_subsets[i].nMatFlags & MTL_FLAG_NODRAW)
        {
            continue;
        }

        ++numChunks;
    }
    m_Chunks.reserve(numChunks);

    // Determine the vertex format of each chunk based on the properties of the mesh
    mesh.SetSubmeshVertexFormats();

    for (uint32 i = 0; i < numSubsets; i++)
    {
        CRenderChunk ChunkInfo;

        if (mesh.m_subsets[i].nNumIndices == 0)
        {
            continue;
        }

        if (mesh.m_subsets[i].nMatFlags & MTL_FLAG_NODRAW)
        {
            continue;
        }

        //add empty chunk, because PodArray is not working with STL-vectors
        m_Chunks.push_back(ChunkInfo);

        uint32 num = m_Chunks.size();
        CRenderChunk* pChunk = &m_Chunks[num - 1];

        pChunk->nFirstIndexId = mesh.m_subsets[i].nFirstIndexId;
        pChunk->nNumIndices   = mesh.m_subsets[i].nNumIndices;
        pChunk->nFirstVertId  = mesh.m_subsets[i].nFirstVertId;
        pChunk->nNumVerts     = mesh.m_subsets[i].nNumVerts;
        pChunk->m_nMatID      = mesh.m_subsets[i].nMatID;
        pChunk->m_nMatFlags   = mesh.m_subsets[i].nMatFlags;
        pChunk->m_vertexFormat = mesh.m_subsets[i].vertexFormat;
        if (mesh.m_subsets[i].nPhysicalizeType == PHYS_GEOM_TYPE_NONE)
        {
            pChunk->m_nMatFlags |= MTL_FLAG_NOPHYSICALIZE;
        }

        float texelAreaDensity = 1.0f;

        if (!(flags & FSM_IGNORE_TEXELDENSITY))
        {
            float posArea;
            float texArea;
            const char* errorText = "";

            if (mesh.m_subsets[i].fTexelDensity > 0.00001f)
            {
                texelAreaDensity = mesh.m_subsets[i].fTexelDensity;
            }
            else
            {
                const bool ok = mesh.ComputeSubsetTexMappingAreas(i, posArea, texArea, errorText);
                if (ok)
                {
                    texelAreaDensity = texArea / posArea;
                }
                else
                {
                    // Commented out to prevent spam (contact Moritz Finck or Marco Corbetta for details)
                    // gEnv->pLog->LogError("Failed to compute texture mapping density for mesh '%s': ?%s", GetSourceName(), errorText);
                }
            }
        }

        pChunk->m_texelAreaDensity = texelAreaDensity;

#define VALIDATE_CHUCKS
#if defined(_DEBUG) && defined(VALIDATE_CHUCKS)
        size_t indStart(pChunk->nFirstIndexId);
        size_t indEnd(pChunk->nFirstIndexId + pChunk->nNumIndices);
        for (size_t j(indStart); j < indEnd; ++j)
        {
            size_t vtxStart(pChunk->nFirstVertId);
            size_t vtxEnd(pChunk->nFirstVertId + pChunk->nNumVerts);
            size_t curIndex0(mesh.m_pIndices[ j ]);   // absolute indexing
            size_t curIndex1(mesh.m_pIndices[ j ] + vtxStart);   // relative indexing using base vertex index
            AZ_Assert((curIndex0 >= vtxStart && curIndex0 < vtxEnd) || (curIndex1 >= vtxStart && curIndex1 < vtxEnd), "Index is out of mesh vertices' range!");
        }
#endif
    }

    //////////////////////////////////////////////////////////////////////////
    // Create RenderElements.
    //////////////////////////////////////////////////////////////////////////
    int nCurChunk = 0;
    for (int i = 0; i < mesh.GetSubSetCount(); i++)
    {
        SMeshSubset& subset = mesh.m_subsets[i];
        if (subset.nNumIndices == 0)
        {
            continue;
        }

        if (subset.nMatFlags & MTL_FLAG_NODRAW)
        {
            continue;
        }

        CRenderChunk* pRenderChunk = &m_Chunks[nCurChunk++];
        CREMeshImpl* pRenderElement = (CREMeshImpl*) gRenDev->EF_CreateRE(eDATA_Mesh);

        // Cross link render chunk with render element.
        pRenderChunk->pRE = pRenderElement;
        AssignChunk(pRenderChunk, pRenderElement);
        if (subset.nNumVerts <= 500 && !mesh.m_pBoneMapping && !(flags & FSM_NO_TANGENTS))
        {
            pRenderElement->mfUpdateFlags(FCEF_MERGABLE);
        }

        if (mesh.m_pBoneMapping)
        {
            pRenderElement->mfUpdateFlags(FCEF_SKINNED);
        }
    }
    if (mesh.m_pBoneMapping)
    {
        m_nFlags |= FRM_SKINNED;
    }

    //////////////////////////////////////////////////////////////////////////
    // Create system vertex buffer in system memory.
    //////////////////////////////////////////////////////////////////////////
#if ENABLE_NORMALSTREAM_SUPPORT
    if (flags & FSM_ENABLE_NORMALSTREAM)
    {
        m_nFlags |= FRM_ENABLE_NORMALSTREAM;
    }
#endif

    m_nVerts = nVerts;
    m_nInds = 0;
    m_vertexFormat = mesh.GetMeshGroupVertexFormat();

    pVBuff = (char*)LockVB(VSF_GENERAL, FSL_VIDEO_CREATE);
    // stop initializing if allocation failed
    if (pVBuff == NULL)
    {
        m_nVerts = 0;
        goto error;
    }

# if ENABLE_NORMALSTREAM_SUPPORT
    if (m_nFlags & FRM_ENABLE_NORMALSTREAM)
    {
        pNBuff = (SPipNormal*)LockVB(VSF_NORMALS, FSL_VIDEO_CREATE);
    }
# endif

    if (!(flags & FSM_NO_TANGENTS))
    {
        if (mesh.m_pQTangents)
        {
            pQTBuff = (SPipQTangents*)LockVB(VSF_QTANGENTS, FSL_VIDEO_CREATE);
        }
        else
        {
            pTBuff = (SPipTangents*)LockVB(VSF_TANGENTS, FSL_VIDEO_CREATE);
        }

        // stop initializing if allocation failed
        if (pTBuff == NULL && pQTBuff == NULL)
        {
            goto error;
        }
    }

    //////////////////////////////////////////////////////////////////////////
    // Copy indices.
    //////////////////////////////////////////////////////////////////////////
    m_nInds = nInds;
    pInds = LockIB(FSL_VIDEO_CREATE);

    // stop initializing if allocation failed
    if (m_nInds && pInds == NULL)
    {
        m_nInds = 0;
        goto error;
    }

    if (flags & FSM_VERTEX_VELOCITY)
    {
        pVelocities = (SVF_P3F*)LockVB(VSF_VERTEX_VELOCITY, FSL_VIDEO_CREATE);

        // stop initializing if allocation failed
        if (pVelocities == NULL)
        {
            goto error;
        }
    }


    // Copy data to mesh
    {
        SSetMeshIntData setMeshIntData =
        {
            &mesh,
            pVBuff,
            pTBuff,
            pQTBuff,
            pVelocities,
            nVerts,
            nInds,
            pInds,
            flags,
            pNBuff
        };
        SetMesh_IntImpl(setMeshIntData);
    }


    // unlock all streams
    UnlockVB(VSF_GENERAL);
#if ENABLE_NORMALSTREAM_SUPPORT
    if (m_nFlags & FRM_ENABLE_NORMALSTREAM)
    {
        UnlockVB(VSF_NORMALS);
    }
# endif
    UnlockIB();

    if (!(flags & FSM_NO_TANGENTS))
    {
        if (mesh.m_pQTangents)
        {
            UnlockVB(VSF_QTANGENTS);
        }
        else
        {
            UnlockVB(VSF_TANGENTS);
        }
    }

    if (flags & FSM_VERTEX_VELOCITY)
    {
        UnlockVB(VSF_VERTEX_VELOCITY);
    }

    //////////////////////////////////////////////////////////////////////////
    // Copy skin-streams.
    //////////////////////////////////////////////////////////////////////////
    if (mesh.m_pBoneMapping)
    {
        SetSkinningDataCharacter(mesh, mesh.m_pBoneMapping, mesh.m_pExtraBoneMapping);
    }

    // Create device buffers immediately in non-multithreaded mode
    if (!gRenDev->m_pRT->IsMultithreaded() && (flags & FSM_CREATE_DEVICE_MESH))
    {
        CheckUpdate(VSM_MASK);
    }

    UnLockForThreadAccess();
    return Size(SIZE_ONLY_SYSTEM);

error:
    UnLockForThreadAccess();
    RT_AllocationFailure("Generic Streaming Error", 0);
    return ~0U;
}

size_t CRenderMesh::SetMesh(CMesh& mesh, int nSecColorsSetOffset, uint32 flags, bool requiresLock)
{
    LOADING_TIME_PROFILE_SECTION;
    
    size_t resultingSize = ~0U;
# ifdef USE_VBIB_PUSH_DOWN
    requiresLock = true;
# endif
    if (requiresLock)
    {
        SREC_AUTO_LOCK(m_sResLock);
        resultingSize = SetMesh_Int(mesh, nSecColorsSetOffset, flags);
    }
    else
    {
        resultingSize = SetMesh_Int(mesh, nSecColorsSetOffset, flags);
    }

    return resultingSize;
}

void CRenderMesh::SetSkinningDataVegetation(struct SMeshBoneMapping_uint8* pBoneMapping)
{
    LockForThreadAccess();
    SVF_W4B_I4S* pSkinBuff = (SVF_W4B_I4S*)LockVB(VSF_HWSKIN_INFO, FSL_VIDEO_CREATE);

    // stop initializing if allocation failed
    if (pSkinBuff == NULL)
    {
        return;
    }

    for (uint32 i = 0; i < m_nVerts; i++)
    {
        // get bone IDs
        uint16 b0 = pBoneMapping[i].boneIds[0];
        uint16 b1 = pBoneMapping[i].boneIds[1];
        uint16 b2 = pBoneMapping[i].boneIds[2];
        uint16 b3 = pBoneMapping[i].boneIds[3];

        // get weights
        const uint8 w0 = pBoneMapping[i].weights[0];
        const uint8 w1 = pBoneMapping[i].weights[1];
        const uint8 w2 = pBoneMapping[i].weights[2];
        const uint8 w3 = pBoneMapping[i].weights[3];

        // if weight is zero set bone ID to zero as the bone has no influence anyway,
        // this will fix some issue with incorrectly exported models (e.g. system freezes on ATI cards when access invalid bones)
        if (w0 == 0)
        {
            b0 = 0;
        }
        if (w1 == 0)
        {
            b1 = 0;
        }
        if (w2 == 0)
        {
            b2 = 0;
        }
        if (w3 == 0)
        {
            b3 = 0;
        }

        pSkinBuff[i].indices[0] = b0;
        pSkinBuff[i].indices[1] = b1;
        pSkinBuff[i].indices[2] = b2;
        pSkinBuff[i].indices[3] = b3;

        pSkinBuff[i].weights.bcolor[0] = w0;
        pSkinBuff[i].weights.bcolor[1] = w1;
        pSkinBuff[i].weights.bcolor[2] = w2;
        pSkinBuff[i].weights.bcolor[3] = w3;

        //  if (pBSStreamTemp)
        //    pSkinBuff[i].boneSpace  = pBSStreamTemp[i];
    }
    UnlockVB(VSF_HWSKIN_INFO);
    UnLockForThreadAccess();

    CreateRemappedBoneIndicesPair(~0u, m_ChunksSkinned);
}

void CRenderMesh::SetSkinningDataCharacter([[maybe_unused]] CMesh& mesh, struct SMeshBoneMapping_uint16* pBoneMapping, [[maybe_unused]] struct SMeshBoneMapping_uint16* pExtraBoneMapping)
{
    SVF_W4B_I4S* pSkinBuff = (SVF_W4B_I4S*)LockVB(VSF_HWSKIN_INFO, FSL_VIDEO_CREATE);

    // stop initializing if allocation failed
    if (pSkinBuff == NULL)
    {
        return;
    }

    for (uint32 i = 0; i < m_nVerts; i++)
    {
        // get bone IDs
        uint16 b0 = pBoneMapping[i].boneIds[0];
        uint16 b1 = pBoneMapping[i].boneIds[1];
        uint16 b2 = pBoneMapping[i].boneIds[2];
        uint16 b3 = pBoneMapping[i].boneIds[3];

        // get weights
        const uint8 w0 = pBoneMapping[i].weights[0];
        const uint8 w1 = pBoneMapping[i].weights[1];
        const uint8 w2 = pBoneMapping[i].weights[2];
        const uint8 w3 = pBoneMapping[i].weights[3];

        // if weight is zero set bone ID to zero as the bone has no influence anyway,
        // this will fix some issue with incorrectly exported models (e.g. system freezes on ATI cards when access invalid bones)
        if (w0 == 0)
        {
            b0 = 0;
        }
        if (w1 == 0)
        {
            b1 = 0;
        }
        if (w2 == 0)
        {
            b2 = 0;
        }
        if (w3 == 0)
        {
            b3 = 0;
        }

        pSkinBuff[i].indices[0] = b0;
        pSkinBuff[i].indices[1] = b1;
        pSkinBuff[i].indices[2] = b2;
        pSkinBuff[i].indices[3] = b3;

        pSkinBuff[i].weights.bcolor[0] = w0;
        pSkinBuff[i].weights.bcolor[1] = w1;
        pSkinBuff[i].weights.bcolor[2] = w2;
        pSkinBuff[i].weights.bcolor[3] = w3;
    }

    UnlockVB(VSF_HWSKIN_INFO);

#if !defined(NULL_RENDERER)
    if (pExtraBoneMapping && m_extraBonesBuffer.m_numElements == 0 && m_nVerts)
    {
        std::vector<SVF_W4B_I4S> pExtraBones;
        pExtraBones.resize(m_nVerts);
        for (uint32 i = 0; i < m_nVerts; i++)
        {
            // get bone IDs
            uint16 b0 = pExtraBoneMapping[i].boneIds[0];
            uint16 b1 = pExtraBoneMapping[i].boneIds[1];
            uint16 b2 = pExtraBoneMapping[i].boneIds[2];
            uint16 b3 = pExtraBoneMapping[i].boneIds[3];

            // get weights
            const uint8 w0 = pExtraBoneMapping[i].weights[0];
            const uint8 w1 = pExtraBoneMapping[i].weights[1];
            const uint8 w2 = pExtraBoneMapping[i].weights[2];
            const uint8 w3 = pExtraBoneMapping[i].weights[3];

            if (w0 == 0)
            {
                b0 = 0;
            }
            if (w1 == 0)
            {
                b1 = 0;
            }
            if (w2 == 0)
            {
                b2 = 0;
            }
            if (w3 == 0)
            {
                b3 = 0;
            }

            pExtraBones[i].indices[0] = b0;
            pExtraBones[i].indices[1] = b1;
            pExtraBones[i].indices[2] = b2;
            pExtraBones[i].indices[3] = b3;

            pExtraBones[i].weights.bcolor[0] = w0;
            pExtraBones[i].weights.bcolor[1] = w1;
            pExtraBones[i].weights.bcolor[2] = w2;
            pExtraBones[i].weights.bcolor[3] = w3;
        }

        m_extraBonesBuffer.Create(pExtraBones.size(), sizeof(SVF_W4B_I4S), DXGI_FORMAT_UNKNOWN, DX11BUF_STRUCTURED | DX11BUF_BIND_SRV, &pExtraBones[0]);
    }
#endif

    CreateRemappedBoneIndicesPair(~0u, m_Chunks);
}

uint CRenderMesh::GetSkinningWeightCount() const
{
#if !defined(NULL_RENDERER)
    if (_HasVBStream(VSF_HWSKIN_INFO))
    {
        return m_extraBonesBuffer.m_numElements > 0 ? 8 : 4;
    }
#endif

    return 0;
}

IIndexedMesh* CRenderMesh::GetIndexedMesh(IIndexedMesh* pIdxMesh)
{    
    struct MeshDataLock
    {
        MeshDataLock(CRenderMesh* pMesh)
            : _Mesh(pMesh) { _Mesh->LockForThreadAccess(); }
        ~MeshDataLock() { _Mesh->UnLockForThreadAccess(); }
        CRenderMesh* _Mesh;
    };
    MeshDataLock _lock(this);

    if (!pIdxMesh)
    {
        pIdxMesh = gEnv->p3DEngine->CreateIndexedMesh();
    }

    // catch failed allocation of IndexedMesh
    if (pIdxMesh == NULL)
    {
        return NULL;
    }

    CMesh* const pMesh = pIdxMesh->GetMesh();

    uint32 numTexCoords = m_vertexFormat.GetAttributeUsageCount(AZ::Vertex::AttributeUsage::TexCoord);

    // These functions also re-allocate the streams
    pIdxMesh->SetVertexCount(m_nVerts);
    pIdxMesh->SetTexCoordCount(m_nVerts, numTexCoords);
    pIdxMesh->SetTangentCount(m_nVerts);
    pIdxMesh->SetIndexCount(m_nInds);
    pIdxMesh->SetSubSetCount(m_Chunks.size());

    strided_pointer<Vec3> pVtx;
    strided_pointer<SPipTangents> pTangs;
    pVtx.data = (Vec3*)GetPosPtr(pVtx.iStride, FSL_READ);

    bool texCoordAllocationSucceeded = true;
    AZStd::vector<strided_pointer<Vec2>> stridedTexCoordPointers;
    stridedTexCoordPointers.resize(numTexCoords);
    for (int streamIndex = 0; streamIndex < stridedTexCoordPointers.size(); ++streamIndex)
    {
        stridedTexCoordPointers[streamIndex].data = (Vec2*)GetUVPtr(stridedTexCoordPointers[streamIndex].iStride, FSL_READ, streamIndex);
        if (stridedTexCoordPointers[streamIndex].data == nullptr || !pMesh->GetStreamPtr<SMeshTexCoord>(CMesh::TEXCOORDS, streamIndex))
        {
            texCoordAllocationSucceeded = false;
            break;
        }
    }
    pTangs.data = (SPipTangents*)GetTangentPtr(pTangs.iStride, FSL_READ);

    // don't copy if some src, or dest buffer is NULL (can happen because of failed allocations)
    if (pVtx.data == NULL           || (pMesh->m_pPositions == NULL && pMesh->m_pPositionsF16 == NULL) ||
        !texCoordAllocationSucceeded ||
        pTangs.data == NULL     || pMesh->m_pTangents  == NULL)
    {
        UnlockStream(VSF_GENERAL);
        delete pIdxMesh;
        return NULL;
    }

    
    for (uint32 i = 0; i < m_nVerts; i++)
    {
        pMesh->m_pPositions[i] = pVtx[i];
        pMesh->m_pNorms    [i] = SMeshNormal(Vec3(0, 0, 1));//pNorm[i];
        pMesh->m_pTangents [i] = SMeshTangents(pTangs[i]);
    }

    for (int streamIndex = 0; streamIndex < stridedTexCoordPointers.size(); ++streamIndex)
    {
        SMeshTexCoord* meshTextureCoordinates = pMesh->GetStreamPtr<SMeshTexCoord>(CMesh::TEXCOORDS, streamIndex);
        for (uint32 i = 0; i < m_nVerts; ++i)
        {
            meshTextureCoordinates[i] = SMeshTexCoord(stridedTexCoordPointers[streamIndex][i]);
        }
    }

    uint offset = 0;
    if (m_vertexFormat.TryCalculateOffset(offset, AZ::Vertex::AttributeUsage::Color))
    {
        strided_pointer<SMeshColor> pColors;
        pColors.data = (SMeshColor*)GetColorPtr(pColors.iStride, FSL_READ);
        pIdxMesh->SetColorCount(m_nVerts);
        SMeshColor* colorStream = pMesh->GetStreamPtr<SMeshColor>(CMesh::COLORS);
        for (int i = 0; i < (int)m_nVerts; i++)
        {
            colorStream[i] = pColors[i];
        }
    }
    UnlockStream(VSF_GENERAL);

    vtx_idx* pInds = GetIndexPtr(FSL_READ);
    for (int i = 0; i < (int)m_nInds; i++)
    {
        pMesh->m_pIndices[i] = pInds[i];
    }
    UnlockIndexStream();

    SVF_W4B_I4S* pSkinBuff = (SVF_W4B_I4S*)LockVB(VSF_HWSKIN_INFO, FSL_READ);
    if (pSkinBuff)
    {
        pIdxMesh->AllocateBoneMapping();
        for (int i = 0; i < (int)m_nVerts; i++)
        {
            for (int j = 0; j < 4; j++)
            {
                pMesh->m_pBoneMapping[i].boneIds[j] = pSkinBuff[i].indices[j];
                pMesh->m_pBoneMapping[i].weights[j] = pSkinBuff[i].weights.bcolor[j];
            }
        }
        UnlockVB(VSF_HWSKIN_INFO);
    }

    for (int i = 0; i < (int)m_Chunks.size(); i++)
    {
        pIdxMesh->SetSubsetIndexVertexRanges(i, m_Chunks[i].nFirstIndexId, m_Chunks[i].nNumIndices, m_Chunks[i].nFirstVertId, m_Chunks[i].nNumVerts);
        pIdxMesh->SetSubsetMaterialId(i, m_Chunks[i].m_nMatID);
        const int nMatFlags = m_Chunks[i].m_nMatFlags;
        const int nPhysicalizeType = (nMatFlags & MTL_FLAG_NOPHYSICALIZE)
            ? PHYS_GEOM_TYPE_NONE
            : ((nMatFlags & MTL_FLAG_NODRAW) ? PHYS_GEOM_TYPE_OBSTRUCT : PHYS_GEOM_TYPE_DEFAULT);
        pIdxMesh->SetSubsetMaterialProperties(i, nMatFlags, nPhysicalizeType, m_Chunks[i].m_vertexFormat);

        const SMeshSubset& mss = pIdxMesh->GetSubSet(i);
        Vec3 vCenter;
        vCenter.zero();
        for (uint32 j = mss.nFirstIndexId; j < mss.nFirstIndexId + mss.nNumIndices; j++)
        {
            vCenter += pMesh->m_pPositions[pMesh->m_pIndices[j]];
        }
        if (mss.nNumIndices)
        {
            vCenter /= (float)mss.nNumIndices;
        }
        float fRadius = 0;
        for (uint32 j = mss.nFirstIndexId; j < mss.nFirstIndexId + mss.nNumIndices; j++)
        {
            fRadius = max(fRadius, (pMesh->m_pPositions[pMesh->m_pIndices[j]] - vCenter).len2());
        }
        fRadius = sqrt_tpl(fRadius);
        pIdxMesh->SetSubsetBounds(i, vCenter, fRadius);
    }

    return pIdxMesh;
}

void CRenderMesh::GenerateQTangents()
{    
    // FIXME: This needs to be cleaned up. Breakable foliage shouldn't need both streams, and this shouldn't be duplicated
    // between here and CryAnimation.
    LockForThreadAccess();
    int srcStride = 0;
    void* pSrcD = LockVB(VSF_TANGENTS, FSL_READ, 0, &srcStride);
    if (pSrcD)
    {
        int dstStride = 0;
        void* pDstD = LockVB(VSF_QTANGENTS, FSL_VIDEO_CREATE, 0, &dstStride);
        assert(pDstD);
        if (pDstD)
        {
            SPipTangents* pTangents = (SPipTangents*)pSrcD;
            SPipQTangents* pQTangents = (SPipQTangents*)pDstD;
            MeshTangentsFrameToQTangents(
                pTangents, srcStride, m_nVerts,
                pQTangents, dstStride);
        }
        UnlockVB(VSF_QTANGENTS);
    }
    UnlockVB(VSF_TANGENTS);
    UnLockForThreadAccess();
}

void CRenderMesh::CreateChunksSkinned()
{
    ReleaseRenderChunks(&m_ChunksSkinned);

    TRenderChunkArray& arrSrcMats = m_Chunks;
    TRenderChunkArray& arrNewMats = m_ChunksSkinned;
    arrNewMats.resize (arrSrcMats.size());
    for (int i = 0; i < arrSrcMats.size(); ++i)
    {
        CRenderChunk& rSrcMat = arrSrcMats[i];
        CRenderChunk& rNewMat = arrNewMats[i];
        rNewMat = rSrcMat;
        CREMeshImpl* re = (CREMeshImpl*) rSrcMat.pRE;
        if (re)
        {
            rNewMat.pRE = (CREMeshImpl*) gRenDev->EF_CreateRE(eDATA_Mesh);
            CRendElement* pNext = rNewMat.pRE->m_NextGlobal;
            CRendElement* pPrev = rNewMat.pRE->m_PrevGlobal;
            *(CREMeshImpl*)rNewMat.pRE = *re;
            if (rNewMat.pRE->m_pChunk) // affects the source mesh!! will only work correctly if the source is deleted after copying
            {
                rNewMat.pRE->m_pChunk = &rNewMat;
            }
            rNewMat.pRE->m_NextGlobal = pNext;
            rNewMat.pRE->m_PrevGlobal = pPrev;
            rNewMat.pRE->m_pRenderMesh = this;
            rNewMat.pRE->m_CustomData = NULL;
        }
    }
}

int CRenderMesh::GetRenderChunksCount(_smart_ptr<IMaterial> pMaterial, int& nRenderTrisCount)
{
    int nCount = 0;
    nRenderTrisCount = 0;

    CRenderer* rd = gRenDev;
    const uint32 ni = (uint32)m_Chunks.size();
    for (uint32 i = 0; i < ni; i++)
    {
        CRenderChunk* pChunk = &m_Chunks[i];
        CRendElementBase* pREMesh = pChunk->pRE;

        SShaderItem* pShaderItem = &pMaterial->GetShaderItem(pChunk->m_nMatID);

        CShaderResources* pR = (CShaderResources*)pShaderItem->m_pShaderResources;
        CShader* pS = (CShader*)pShaderItem->m_pShader;
        if (pREMesh && pS && pR)
        {
            if (pChunk->m_nMatFlags & MTL_FLAG_NODRAW)
            {
                continue;
            }

            if (pS->m_Flags2 & EF2_NODRAW)
            {
                continue;
            }

            if (pChunk->nNumIndices)
            {
                nRenderTrisCount += pChunk->nNumIndices / 3;
                nCount++;
            }
        }
    }

    return nCount;
}

void CRenderMesh::CopyTo(IRenderMesh* _pDst, int nAppendVtx, [[maybe_unused]] bool bDynamic, bool fullCopy)
{    
    CRenderMesh* pDst = (CRenderMesh*)_pDst;
#ifdef USE_VBIB_PUSH_DOWN
    SREC_AUTO_LOCK(m_sResLock);
#endif
    TRenderChunkArray& arrSrcMats = m_Chunks;
    TRenderChunkArray& arrNewMats = pDst->m_Chunks;
    //pDst->m_bMaterialsWasCreatedInRenderer  = true;
    arrNewMats.resize (arrSrcMats.size());
    int i;
    for (i = 0; i < arrSrcMats.size(); ++i)
    {
        CRenderChunk& rSrcMat = arrSrcMats[i];
        CRenderChunk& rNewMat = arrNewMats[i];
        rNewMat = rSrcMat;
        rNewMat.nNumVerts   += ((m_nVerts - 2 - rNewMat.nNumVerts - rNewMat.nFirstVertId) >> 31) & nAppendVtx;
        CREMeshImpl* re = (CREMeshImpl*) rSrcMat.pRE;
        if (re)
        {
            AZ_Assert(!re->m_CustomData, "Trying to copy a render mesh after custom data has been set."); // Custom data cannot be copied over (used by Terrain and Decals)
            rNewMat.pRE = (CREMeshImpl*) gRenDev->EF_CreateRE(eDATA_Mesh);
            CRendElement* pNext = rNewMat.pRE->m_NextGlobal;
            CRendElement* pPrev = rNewMat.pRE->m_PrevGlobal;
            *(CREMeshImpl*)rNewMat.pRE = *re;
            if (rNewMat.pRE->m_pChunk) // affects the source mesh!! will only work correctly if the source is deleted after copying
            {
                rNewMat.pRE->m_pChunk = &rNewMat;
                rNewMat.pRE->m_pChunk->nNumVerts += ((m_nVerts - 2 - re->m_pChunk->nNumVerts - re->m_pChunk->nFirstVertId) >> 31) & nAppendVtx;
            }
            rNewMat.pRE->m_NextGlobal = pNext;
            rNewMat.pRE->m_PrevGlobal = pPrev;
            rNewMat.pRE->m_pRenderMesh = pDst;
            rNewMat.pRE->m_CustomData = NULL;
        }
    }
    LockForThreadAccess();
    pDst->LockForThreadAccess();
    pDst->m_nVerts = m_nVerts + nAppendVtx;
    if (fullCopy)
    {
        pDst->m_vertexFormat = m_vertexFormat;
        for (i = 0; i < VSF_NUM; i++)
        {
            void* pSrcD = LockVB(i, FSL_READ);
            if (pSrcD)
            {
                void* pDstD = pDst->LockVB(i, FSL_VIDEO_CREATE);
                assert(pDstD);
                if (pDstD)
                {
                    cryMemcpy(pDstD, pSrcD, GetStreamSize(i), MC_CPU_TO_GPU);
                }
                pDst->UnlockVB(i);
            }
            UnlockVB(i);
        }

        pDst->m_nInds = m_nInds;
        void* pSrcD = LockIB(FSL_READ);
        if (pSrcD)
        {
            void* pDstD = pDst->LockIB(FSL_VIDEO_CREATE);
            assert(pDstD);
            if (pDstD)
            {
                cryMemcpy(pDstD, pSrcD, m_nInds * sizeof(vtx_idx), MC_CPU_TO_GPU);
            }
            pDst->UnlockIB();
        }

        pDst->m_eType = m_eType;
        pDst->m_nFlags = m_nFlags;
    }
    UnlockIB();
    UnLockForThreadAccess();
    pDst->UnLockForThreadAccess();
}

// set effector for all chunks
void CRenderMesh::SetCustomTexID(int nCustomTID)
{    
    if (m_Chunks.size())
    {
        for (int i = 0; i < m_Chunks.size(); i++)
        {
            CRenderChunk* pChunk = &m_Chunks[i];
            if (pChunk->pRE)
            {
                pChunk->pRE->m_CustomTexBind[0] = nCustomTID;
            }
        }
    }
}

void CRenderMesh::SetChunk(int nIndex, CRenderChunk& inChunk)
{
    if (!inChunk.nNumIndices || !inChunk.nNumVerts || m_nInds == 0)
    {
        return;
    }

    CRenderChunk* pRenderChunk = NULL;

    if (nIndex < 0 || nIndex >= m_Chunks.size())
    {
        // add new chunk
        CRenderChunk matinfo;
        m_Chunks.push_back(matinfo);
        pRenderChunk = &m_Chunks.back();

        pRenderChunk->pRE = (CREMeshImpl*) gRenDev->EF_CreateRE(eDATA_Mesh);
        pRenderChunk->pRE->m_CustomTexBind[0] = m_nClientTextureBindID;
    }
    else
    {
        // use present chunk
        pRenderChunk = &m_Chunks[nIndex];
    }

    pRenderChunk->m_nMatID = inChunk.m_nMatID;
    pRenderChunk->m_nMatFlags = inChunk.m_nMatFlags;

    pRenderChunk->nFirstIndexId   = inChunk.nFirstIndexId;
    pRenderChunk->nNumIndices     = MAX(inChunk.nNumIndices, 0);
    pRenderChunk->nFirstVertId    = inChunk.nFirstVertId;
    pRenderChunk->nNumVerts           = MAX(inChunk.nNumVerts, 0);
    pRenderChunk->nSubObjectIndex = inChunk.nSubObjectIndex;

    pRenderChunk->m_texelAreaDensity = inChunk.m_texelAreaDensity;

    pRenderChunk->m_vertexFormat = inChunk.m_vertexFormat;

    // update chunk RE
    if (pRenderChunk->pRE)
    {
        AssignChunk(pRenderChunk, (CREMeshImpl*) pRenderChunk->pRE);
    }
    CRY_ASSERT(!pRenderChunk->pRE || pRenderChunk->pRE->m_pChunk->nFirstIndexId < 60000); // TODO: do we need to compare with 60000 if vertex indices are 32-bit?
    CRY_ASSERT(pRenderChunk->nFirstIndexId + pRenderChunk->nNumIndices <= m_nInds);
}

void CRenderMesh::SetChunk(_smart_ptr<IMaterial> pNewMat, int nFirstVertId, int nVertCount, int nFirstIndexId, int nIndexCount, float texelAreaDensity, const AZ::Vertex::Format& vertexFormat, int nIndex)
{
    CRenderChunk chunk;

    if (pNewMat)
    {
        chunk.m_nMatFlags = pNewMat->GetFlags();
    }

    if (nIndex < 0 || nIndex >= m_Chunks.size())
    {
        chunk.m_nMatID = m_Chunks.size();
    }
    else
    {
        chunk.m_nMatID = nIndex;
    }

    chunk.nFirstVertId = nFirstVertId;
    chunk.nNumVerts = nVertCount;

    chunk.nFirstIndexId = nFirstIndexId;
    chunk.nNumIndices = nIndexCount;

    chunk.m_texelAreaDensity = texelAreaDensity;
    chunk.m_vertexFormat = vertexFormat;
    SetChunk(nIndex, chunk);
}

//================================================================================================================

bool CRenderMesh::PrepareCachePos()
{
    if (!m_pCachePos && m_vertexFormat.Has16BitFloatPosition())
    {
        m_pCachePos = AllocateMeshData<Vec3>(m_nVerts);
        if (m_pCachePos)
        {
            m_nFrameRequestCachePos = gRenDev->m_RP.m_TI[gRenDev->m_RP.m_nFillThreadID].m_nFrameUpdateID;
            return true;
        }
    }
    return false;
}

bool CRenderMesh::CreateCachePos(byte* pSrc, uint32 nStrideSrc, uint nFlags)
{
    PROFILE_FRAME(Mesh_CreateCachePos);
    if (m_vertexFormat.Has16BitFloatPosition())
    {
    #ifdef USE_VBIB_PUSH_DOWN
        SREC_AUTO_LOCK(m_sResLock);//on USE_VBIB_PUSH_DOWN tick is executed in renderthread
    #endif
        m_nFlagsCachePos = (nFlags & FSL_WRITE) != 0;
        m_nFrameRequestCachePos = gRenDev->m_RP.m_TI[gRenDev->m_RP.m_nFillThreadID].m_nFrameUpdateID;
        if ((nFlags & FSL_READ) && m_pCachePos)
        {
            return true;
        }
        if ((nFlags == FSL_SYSTEM_CREATE) && m_pCachePos)
        {
            return true;
        }
        if (!m_pCachePos)
        {
            m_pCachePos = AllocateMeshData<Vec3>(m_nVerts);
        }
        if (m_pCachePos)
        {
            if (nFlags == FSL_SYSTEM_UPDATE || (nFlags & FSL_READ))
            {
                for (uint32 i = 0; i < m_nVerts; i++)
                {
                    Vec3f16* pVSrc = (Vec3f16*)pSrc;
                    m_pCachePos[i] = pVSrc->ToVec3();
                    pSrc += nStrideSrc;
                }
            }
            return true;
        }
    }
    return false;
}

bool CRenderMesh::CreateUVCache(byte* source, uint32 sourceStride, uint flags, uint32 uvSetIndex)
{
    PROFILE_FRAME(Mesh_CreateUVCache);

    AZ::Vertex::AttributeType attributeType = AZ::Vertex::AttributeType::NumTypes;
    uint offset = 0;
    bool hasUVSetAtIndex = m_vertexFormat.TryGetAttributeOffsetAndType(AZ::Vertex::AttributeUsage::TexCoord, uvSetIndex, offset, attributeType);

    // If the vertex format has a uv set at the given index
    // And the vertex format uses 16 bit floats to represent that uv set
    if (hasUVSetAtIndex && (attributeType == AZ::Vertex::AttributeType::Float16_2))
    {
        m_nFlagsCacheUVs = (flags & FSL_WRITE) != 0;
        m_nFrameRequestCacheUVs = gRenDev->m_RP.m_TI[gRenDev->m_RP.m_nFillThreadID].m_nFrameUpdateID;
        // Increase the size of the cached uvs vector if it is not big enough for the given uv set, and fill any new slots with nullptr
        if (uvSetIndex >= m_UVCache.size())
        {
            m_UVCache.resize(uvSetIndex + 1, nullptr);
        }

        // Return true if the uv cache has already been created and does not need to be updated
        if ((flags & FSL_READ) && m_UVCache[uvSetIndex])
        {
            return true;
        }
        if ((flags == FSL_SYSTEM_CREATE) && m_UVCache[uvSetIndex])
        {
            return true;
        }

        // If the cache doesn't exist yet, allocate memory for it
        if (!m_UVCache[uvSetIndex])
        {
            m_UVCache[uvSetIndex] = AllocateMeshData<Vec2>(m_nVerts);
        }

        if (m_UVCache[uvSetIndex])
        {
            // If the new or existing cache needs to be updated, fill it with the source data
            if (flags == FSL_SYSTEM_UPDATE || (flags & FSL_READ))
            {
                source += offset;

                for (uint32 i = 0; i < m_nVerts; i++)
                {
                    Vec2f16* pVSrc = (Vec2f16*)source;
                    m_UVCache[uvSetIndex][i] = pVSrc->ToVec2();
                    source += sourceStride;
                }
            }
            return true;
        }
    }
    return false;
}

byte* CRenderMesh::GetPosPtrNoCache(int32& nStride, uint32 nFlags)
{
    int nStr = 0;
    byte* pData = NULL;
    pData = (byte*)LockVB(VSF_GENERAL, nFlags, 0, &nStr, true);
    ASSERT_LOCK;
    if (!pData)
    {
        return NULL;
    }
    nStride = nStr;
    return pData;
}

byte* CRenderMesh::GetPosPtr(int32& nStride, uint32 nFlags)
{
    PROFILE_FRAME(Mesh_GetPosPtr);
    int nStr = 0;
    byte* pData = NULL;
    pData = (byte*)LockVB(VSF_GENERAL, nFlags, 0, &nStr, true, true);
    ASSERT_LOCK;
    if (!pData)
    {
        return NULL;
    }

    if (!CreateCachePos(pData, nStr, nFlags))
    {
        nStride = nStr;
        return pData;
    }

    pData = (byte*)m_pCachePos;
    nStride = sizeof(Vec3);
    return pData;
}

vtx_idx* CRenderMesh::GetIndexPtr(uint32 nFlags, int32 nOffset)
{
    vtx_idx* pData = LockIB(nFlags, nOffset, 0);
    assert((m_nInds == 0) || pData);
    return pData;
}

byte* CRenderMesh::GetColorPtr(int32& nStride, uint32 nFlags)
{
    PROFILE_FRAME(Mesh_GetColorPtr);
    int nStr = 0;
    byte* pData = (byte*)LockVB(VSF_GENERAL, nFlags, 0, &nStr);
    ASSERT_LOCK;
    if (!pData)
    {
        return NULL;
    }
    nStride = nStr;
    uint colorOffset = 0;
    if (_GetVertexFormat().TryCalculateOffset(colorOffset, AZ::Vertex::AttributeUsage::Color))
    {
        return &pData[colorOffset];
    }
    return NULL;
}
byte* CRenderMesh::GetNormPtr(int32& nStride, uint32 nFlags)
{
    PROFILE_FRAME(Mesh_GetNormPtr);
    int nStr = 0;
    byte* pData = 0;
# if ENABLE_NORMALSTREAM_SUPPORT
    pData = (byte*)LockVB(VSF_NORMALS, nFlags, 0, &nStr);
    if (pData)
    {
        nStride = sizeof(Vec3);
        return pData;
    }
# endif
    pData = (byte*)LockVB(VSF_GENERAL, nFlags, 0, &nStr);
    ASSERT_LOCK;
    if (!pData)
    {
        return NULL;
    }
    nStride = nStr;
    uint normalOffset = 0;
    if (_GetVertexFormat().TryCalculateOffset(normalOffset, AZ::Vertex::AttributeUsage::Normal))
    {
        return &pData[normalOffset];
    }
    return NULL;
}
byte* CRenderMesh::GetUVPtrNoCache(int32& nStride, uint32 nFlags, uint32 uvSetIndex)
{
    PROFILE_FRAME(Mesh_GetUVPtrNoCache);
    int nStr = 0;
    byte* pData = (byte*)LockVB(VSF_GENERAL, nFlags, 0, &nStr);
    ASSERT_LOCK;
    if (!pData)
    {
        return NULL;
    }
    nStride = nStr;
    uint texCoordOffset = 0;
    if (_GetVertexFormat().TryCalculateOffset(texCoordOffset, AZ::Vertex::AttributeUsage::TexCoord, uvSetIndex))
    {
        return &pData[texCoordOffset];
    }
    return NULL;
}
byte* CRenderMesh::GetUVPtr(int32& nStride, uint32 nFlags, uint32 uvSetIndex)
{
    PROFILE_FRAME(Mesh_GetUVPtr);
    int nStr = 0;
    byte* pData = (byte*)LockVB(VSF_GENERAL, nFlags, 0, &nStr);
    ASSERT_LOCK;
    if (!pData)
    {
        return NULL;
    }

    bool result;
    {
        SREC_AUTO_LOCK(m_sResLock);
        result = CreateUVCache(pData, nStr, nFlags, uvSetIndex);
    }

    if (!result)
    {
        nStride = nStr;
        uint texCoordOffset = 0;
        if (_GetVertexFormat().TryCalculateOffset(texCoordOffset, AZ::Vertex::AttributeUsage::TexCoord, uvSetIndex))
        {
            return &pData[texCoordOffset];
        }
    }
    else
    {
        pData = (byte*)m_UVCache[uvSetIndex];
        nStride = sizeof(Vec2);
        return pData;
    }

    return NULL;
}

byte* CRenderMesh::GetTangentPtr(int32& nStride, uint32 nFlags)
{
    PROFILE_FRAME(Mesh_GetTangentPtr);
    int nStr = 0;
    byte* pData = (byte*)LockVB(VSF_TANGENTS, nFlags, 0, &nStr);
    //ASSERT_LOCK;
    if (!pData)
    {
        pData = (byte*)LockVB(VSF_QTANGENTS, nFlags, 0, &nStr);
    }
    if (!pData)
    {
        return NULL;
    }
    nStride = nStr;
    return pData;
}
byte* CRenderMesh::GetQTangentPtr(int32& nStride, uint32 nFlags)
{
    PROFILE_FRAME(Mesh_GetQTangentPtr);
    int nStr = 0;
    byte* pData = (byte*)LockVB(VSF_QTANGENTS, nFlags, 0, &nStr);
    //ASSERT_LOCK;
    if (!pData)
    {
        return NULL;
    }
    nStride = nStr;
    return pData;
}

byte* CRenderMesh::GetHWSkinPtr(int32& nStride, uint32 nFlags, [[maybe_unused]] bool remapped)
{
    PROFILE_FRAME(Mesh_GetHWSkinPtr);
    int nStr = 0;
    byte* pData = (byte*)LockVB(VSF_HWSKIN_INFO, nFlags, 0, &nStr);
    if (!pData)
    {
        return NULL;
    }
    nStride = nStr;
    return pData;
}

byte* CRenderMesh::GetVelocityPtr(int32& nStride, uint32 nFlags)
{
    PROFILE_FRAME(Mesh_GetMorphTargetPtr);
    int nStr = 0;
    byte* pData = (byte*)LockVB(VSF_VERTEX_VELOCITY, nFlags, 0, &nStr);
    ASSERT_LOCK;
    if (!pData)
    {
        return NULL;
    }
    nStride = nStr;
    return pData;
}

bool CRenderMesh::IsEmpty()
{
    ASSERT_IS_MAIN_THREAD(gRenDev->m_pRT)
    SMeshStream * pMS = GetVertexStream(VSF_GENERAL, 0);
    return (!m_nVerts || (!pMS || pMS->m_nID == ~0u || !pMS->m_pUpdateData) || (!_HasIBStream() && !m_IBStream.m_pUpdateData));
}

//================================================================================================================

bool CRenderMesh::CheckUpdate(uint32 nStreamMask)
{
    CRenderMesh* pRM = _GetVertexContainer();
    if (pRM)
    {
        return gRenDev->m_pRT->RC_CheckUpdate2(this, pRM, nStreamMask);
    }
    return false;
}

void CRenderMesh::RT_AllocationFailure([[maybe_unused]] const char* sPurpose, [[maybe_unused]] uint32 nSize)
{
    SREC_AUTO_LOCK(m_sResLock);
    Cleanup();
    m_nVerts = 0;
    m_nInds = 0;
    m_nFlags |= FRM_ALLOCFAILURE;
# if !defined(_RELEASE) && !defined(NULL_RENDERER)
    CryLogAlways("rendermesh '%s(%s)' suffered from a buffer allocation failure for \"%s\" size %d bytes on thread 0x%" PRI_THREADID, m_sSource.c_str(), m_sType.c_str(), sPurpose, nSize, CryGetCurrentThreadId());
# endif
}


#ifdef MESH_TESSELLATION_RENDERER
namespace
{
    template<class VecPos>
    class SAdjVertCompare
    {
    public:
        SAdjVertCompare(const AZ::Vertex::Format& vertexFormat)
            : m_vertexFormat(vertexFormat)
        {}
        inline bool operator()(const int i1, const int i2) const
        {
            uint stride = m_vertexFormat.GetStride();
            uint offset = 0;
            m_vertexFormat.TryCalculateOffset(offset, AZ::Vertex::AttributeUsage::Position);
            const VecPos* pVert1 = (const VecPos*)&m_pVerts[i1 * stride + offset];
            const VecPos* pVert2 = (const VecPos*)&m_pVerts[i2 * stride + offset];
            if (pVert1->x < pVert2->x)
            {
                return true;
            }
            if (pVert1->x > pVert2->x)
            {
                return false;
            }
            if (pVert1->y < pVert2->y)
            {
                return true;
            }
            if (pVert1->y > pVert2->y)
            {
                return false;
            }
            return pVert1->z < pVert2->z;
        }
        const byte* m_pVerts;
        AZ::Vertex::Format m_vertexFormat;
    };
}

template<class VecPos, class VecUV>
void CRenderMesh::BuildAdjacency(const byte* pVerts, const AZ::Vertex::Format& vertexFormat, unsigned int nVerts, const vtx_idx* pIndexBuffer, uint nTrgs, std::vector<VecUV>& pTxtAdjBuffer)
{
    SAdjVertCompare<VecPos> compare(vertexFormat);
    compare.m_pVerts = pVerts;

    // this array will contain indices of vertices sorted by float3 position - so that we could find vertices with equal positions (they have to be adjacent in the array)
    std::vector<int> arrSortedVertIDs;
    // we allocate a bit more (by one) because we will need extra element for scan operation (lower)
    arrSortedVertIDs.resize(nVerts + 1);
    int* _iSortedVerts = &arrSortedVertIDs[0];
    for (int iv = 0; iv < nVerts; ++iv)
    {
        arrSortedVertIDs[iv] = iv;
    }

    std::sort(arrSortedVertIDs.begin(), arrSortedVertIDs.end() - 1, compare);

    // Get vertex stride, offset, and bytelength for position and texture coordinates
    uint stride = vertexFormat.GetStride();
    uint positionOffset = 0;
    vertexFormat.TryCalculateOffset(positionOffset, AZ::Vertex::AttributeUsage::Position);
    uint texCoordOffset = 0;
    vertexFormat.TryCalculateOffset(texCoordOffset, AZ::Vertex::AttributeUsage::TexCoord);
    uint texCoordByteLength = vertexFormat.GetAttributeByteLength(AZ::Vertex::AttributeUsage::TexCoord);

    // compute how many unique vertices are there, also setup links from each vertex to master vertex
    std::vector<int> arrLinkToMaster;
    arrLinkToMaster.resize(nVerts);
    int nUniqueVertices = 0;
    for (int iv0 = 0; iv0 < nVerts; )
    {
        int iMaster = arrSortedVertIDs[iv0];
        arrLinkToMaster[iMaster] = iMaster;
        int iv1 = iv0 + 1;
        for (; iv1 < nVerts; ++iv1)
        {
            // if slave vertex != master vertex
            if (*(VecPos*)&pVerts[arrSortedVertIDs[iv1] * stride + positionOffset] != *(VecPos*)&pVerts[iMaster * stride + positionOffset])
            {
                break;
            }
            arrLinkToMaster[arrSortedVertIDs[iv1]] = iMaster;
        }
        iv0 = iv1;
        ++nUniqueVertices;
    }
    if (nUniqueVertices == nVerts)
    { // no need to recode anything - the mesh is perfect
        return;
    }
    // compute how many triangles connect to every master vertex
    std::vector<int>& arrConnectedTrianglesCount = arrSortedVertIDs;
    for (int i = 0; i < arrConnectedTrianglesCount.size(); ++i)
    {
        arrConnectedTrianglesCount[i] = 0;
    }
    int* _nConnectedTriangles = &arrConnectedTrianglesCount[0];
    for (int it = 0; it < nTrgs; ++it)
    {
        const vtx_idx* pTrg = &pIndexBuffer[it * 3];
        int iMasterVertex0 = arrLinkToMaster[pTrg[0]];
        int iMasterVertex1 = arrLinkToMaster[pTrg[1]];
        int iMasterVertex2 = arrLinkToMaster[pTrg[2]];
        if (iMasterVertex0 == iMasterVertex1 || iMasterVertex0 == iMasterVertex2 || iMasterVertex1 == iMasterVertex2)
        {
            continue; // degenerate triangle - skip it
        }
        ++arrConnectedTrianglesCount[iMasterVertex0];
        ++arrConnectedTrianglesCount[iMasterVertex1];
        ++arrConnectedTrianglesCount[iMasterVertex2];
    }
    // scan
    std::vector<int>& arrFirstConnectedTriangle = arrSortedVertIDs;
    for (int iv = 0; iv < nVerts; ++iv)
    {
        arrFirstConnectedTriangle[iv + 1] += arrFirstConnectedTriangle[iv];
    }
    {
        int iTmp = arrFirstConnectedTriangle[0];
        arrFirstConnectedTriangle[0] = 0;
        for (int iv = 0; iv < nVerts; ++iv)
        {
            int iTmp1 = arrFirstConnectedTriangle[iv + 1];
            arrFirstConnectedTriangle[iv + 1] = iTmp;
            iTmp = iTmp1;
        }
    }
    // create a list of triangles for each master vertex
    std::vector<int> arrConnectedTriangles;
    arrConnectedTriangles.resize(arrFirstConnectedTriangle[nVerts]);
    for (int it = 0; it < nTrgs; ++it)
    {
        const vtx_idx* pTrg = &pIndexBuffer[it * 3];
        int iMasterVertex0 = arrLinkToMaster[pTrg[0]];
        int iMasterVertex1 = arrLinkToMaster[pTrg[1]];
        int iMasterVertex2 = arrLinkToMaster[pTrg[2]];
        if (iMasterVertex0 == iMasterVertex1 || iMasterVertex0 == iMasterVertex2 || iMasterVertex1 == iMasterVertex2)
        {
            continue; // degenerate triangle - skip it
        }
        arrConnectedTriangles[arrFirstConnectedTriangle[iMasterVertex0]++] = it;
        arrConnectedTriangles[arrFirstConnectedTriangle[iMasterVertex1]++] = it;
        arrConnectedTriangles[arrFirstConnectedTriangle[iMasterVertex2]++] = it;
    }
    // return scan array to initial state
    {
        int iTmp = arrFirstConnectedTriangle[0];
        arrFirstConnectedTriangle[0] = 0;
        for (int iv = 0; iv < nVerts; ++iv)
        {
            int iTmp1 = arrFirstConnectedTriangle[iv + 1];
            arrFirstConnectedTriangle[iv + 1] = iTmp;
            iTmp = iTmp1;
        }
    }
    // find matches for boundary edges
    for (int it = 0; it < nTrgs; ++it)
    {
        const vtx_idx* pTrg = &pIndexBuffer[it * 3];
        for (int ie = 0; ie < 3; ++ie)
        {
            // fix the corner here
            {
                int ivCorner = pTrg[ie];
                memcpy(&pTxtAdjBuffer[it * 12 + 9 + ie], &pVerts[arrLinkToMaster[ivCorner] * stride + texCoordOffset], texCoordByteLength);
            }
            // proceed with fixing the edges
            int iv0 = pTrg[ie];
            int iMasterVertex0 = arrLinkToMaster[iv0];

            int iv1 = pTrg[(ie + 1) % 3];
            int iMasterVertex1 = arrLinkToMaster[iv1];
            if (iMasterVertex0 == iMasterVertex1)
            { // some degenerate case - skip it
                continue;
            }
            // find a triangle that has both iMasterVertex0 and iMasterVertex1
            for (int i0 = arrFirstConnectedTriangle[iMasterVertex0]; i0 < arrFirstConnectedTriangle[iMasterVertex0 + 1]; ++i0)
            {
                int iOtherTriangle = arrConnectedTriangles[i0];
                if (iOtherTriangle >= it) // we are going to stick to this other triangle only if it's index is less than ours
                {
                    continue;
                }
                const vtx_idx* pOtherTrg = &pIndexBuffer[iOtherTriangle * 3];
                int iRecode0 = -1;
                int iRecode1 = -1;
                // setup recode indices
                for (int ieOther = 0; ieOther < 3; ++ieOther)
                {
                    if (arrLinkToMaster[pOtherTrg[ieOther]] == iMasterVertex0)
                    {
                        iRecode0 = pOtherTrg[ieOther];
                    }
                    else if (arrLinkToMaster[pOtherTrg[ieOther]] == iMasterVertex1)
                    {
                        iRecode1 = pOtherTrg[ieOther];
                    }
                }
                if (iRecode0 != -1 && iRecode1 != -1)
                { // this triangle is our neighbor
                    memcpy(&pTxtAdjBuffer[it * 12 + 3 + ie * 2], &pVerts[iRecode0 * stride + texCoordOffset], texCoordByteLength);
                    memcpy(&pTxtAdjBuffer[it * 12 + 3 + ie * 2 + 1], &pVerts[iRecode1 * stride + texCoordOffset], texCoordByteLength);
                }
            }
        }
    }
}
#endif //#ifdef MESH_TESSELLATION_RENDERER

bool CRenderMesh::RT_CheckUpdate(CRenderMesh* pVContainer, uint32 nStreamMask, [[maybe_unused]] bool bTessellation, bool stall)
{
    PrefetchLine(&m_IBStream, 0);

    CRenderer* rd = gRenDev;
    int nThreadID = rd->m_RP.m_nProcessThreadID;
    int nFrame = rd->m_RP.m_TI[nThreadID].m_nFrameUpdateID;
    bool bSkinned = (m_nFlags & (FRM_SKINNED | FRM_SKINNEDNEXTDRAW)) != 0;

    if (nStreamMask & 0x80000000) // Disable skinning in instancing mode
    {
        bSkinned = false;
    }

    m_nFlags &= ~FRM_SKINNEDNEXTDRAW;

    IF (!CanRender(), 0)
    {
        return false;
    }

    FUNCTION_PROFILER_RENDER_FLAT
    AZ_TRACE_METHOD();
    PrefetchVertexStreams();

    PrefetchLine(pVContainer->m_VBStream, 0);
    SMeshStream* pMS = pVContainer->GetVertexStream(VSF_GENERAL, 0);

    if ((m_pVertexContainer || m_nVerts > 2) && pMS)
    {
        PrefetchLine(pVContainer->m_VBStream, 128);
        if (pMS->m_pUpdateData && pMS->m_nFrameAccess != nFrame)
        {
            pMS->m_nFrameAccess = nFrame;
            if (pMS->m_nFrameRequest > pMS->m_nFrameUpdate)
            {
                {
                    PROFILE_FRAME(Mesh_CheckUpdateUpdateGBuf);
                    if (!(pMS->m_nLockFlags & FSL_WRITE))
                    {
                        if (!pVContainer->UpdateVidVertices(VSF_GENERAL, stall))
                        {
                            RT_AllocationFailure("Update General Stream", GetStreamSize(VSF_GENERAL, m_nVerts));
                            return false;
                        }
                        pMS->m_nFrameUpdate = nFrame;
                    }
                    else
                    {
                        if (pMS->m_nID == ~0u)
                        {
                            return false;
                        }
                    }
                }
            }
        }
        // Set both tangent flags
        if (nStreamMask & VSM_TANGENTS)
        {
            nStreamMask |= VSM_TANGENTS;
        }

        // Additional streams updating
        if (nStreamMask & VSM_MASK)
        {
            int i;
            uint32 iMask = 1;

            for (i = 1; i < VSF_NUM; i++)
            {
                iMask = iMask << 1;

                pMS = pVContainer->GetVertexStream(i, 0);
                if ((nStreamMask & iMask) && pMS)
                {
                    if (pMS->m_pUpdateData && pMS->m_nFrameAccess != nFrame)
                    {
                        pMS->m_nFrameAccess = nFrame;
                        if (pMS->m_nFrameRequest > pMS->m_nFrameUpdate)
                        {
                            // Update the device buffer
                            PROFILE_FRAME(Mesh_CheckUpdateUpdateGBuf);
                            if (!(pMS->m_nLockFlags & FSL_WRITE))
                            {
                                if (!pVContainer->UpdateVidVertices(i, stall))
                                {
                                    RT_AllocationFailure("Update VB Stream", GetStreamSize(i, m_nVerts));
                                    return false;
                                }
                                if (i == VSF_HWSKIN_INFO && pVContainer->m_RemappedBoneIndices.size() == 1)
                                {
                                    // Just locking the VB stream doesn't store any information about the GUID so
                                    // if the HWSKIN_INFO has been locked and we only have one set of remapped bone
                                    // indices we will assume they are the same set. This is a valid assumption in non
                                    // legacy code, as we will only have the one set loaded from the asset.
                                    if (!gRenDev->m_DevBufMan.UpdateBuffer(pVContainer->m_RemappedBoneIndices[0].buffer,
                                        pMS->m_pUpdateData,
                                        pVContainer->GetVerticesCount() * pVContainer->GetStreamStride(VSF_HWSKIN_INFO)))
                                    {
                                        RT_AllocationFailure("Update VB Stream", GetStreamSize(i, m_nVerts));
                                        return false;
                                    }
                                }
                                pMS->m_nFrameUpdate = nFrame;
                            }
                            else
                            if (i != VSF_HWSKIN_INFO)
                            {
                                int nnn = 0;
                                if (pMS->m_nID == ~0u)
                                {
                                    return false;
                                }
                            }
                        }
                    }
                }
            }
        }
    }//if (m_pVertexContainer || m_nVerts > 2)

    m_IBStream.m_nFrameAccess = nFrame;
    const bool bIndUpdateNeeded = (m_IBStream.m_pUpdateData != NULL) && (m_IBStream.m_nFrameRequest > m_IBStream.m_nFrameUpdate);
    if (bIndUpdateNeeded)
    {
        PROFILE_FRAME(Mesh_CheckUpdate_UpdateInds);
        if (!(pVContainer->m_IBStream.m_nLockFlags & FSL_WRITE))
        {
            if (!UpdateVidIndices(m_IBStream, stall))
            {
                RT_AllocationFailure("Update IB Stream", m_nInds * sizeof(vtx_idx));
                return false;
            }
            m_IBStream.m_nFrameUpdate = nFrame;
        }
        else if (pVContainer->m_IBStream.m_nID == ~0u)
        {
            return false;
        }
    }

#ifdef MESH_TESSELLATION_RENDERER
    // Build UV adjacency
    if ((bTessellation && m_adjBuffer.m_numElements == 0)       // if needed and not built already
        || (bIndUpdateNeeded && m_adjBuffer.m_numElements > 0)) // if already built but needs update
    {
        if (!(pVContainer->m_IBStream.m_nLockFlags & FSL_WRITE)
            && (pVContainer->_HasVBStream(VSF_NORMALS)))
        {
            if (m_vertexFormat.Has16BitFloatTextureCoordinates())
            {
                UpdateUVCoordsAdjacency<Vec3f16, Vec2f16>(m_IBStream, m_vertexFormat);
            }
            else if (m_vertexFormat.Has32BitFloatTextureCoordinates())
            {
                UpdateUVCoordsAdjacency<Vec3, Vec2>(m_IBStream, m_vertexFormat);
            }
        }
    }
#endif

    const int threadId = gRenDev->m_RP.m_nProcessThreadID;
    for (size_t i = 0, end0 = pVContainer->m_DeletedBoneIndices[threadId].size(); i < end0; ++i)
    {
        for (size_t j = 0, end1 = pVContainer->m_RemappedBoneIndices.size(); j < end1; ++j)
        {
            if (pVContainer->m_RemappedBoneIndices[j].guid == pVContainer->m_DeletedBoneIndices[threadId][i])
            {
                SBoneIndexStream& stream = pVContainer->m_RemappedBoneIndices[j];
                if (stream.buffer != ~0u)
                {
                    // Unregister the allocation with the VRAM driller
                    void* address = reinterpret_cast<void*>(stream.buffer);
                    EBUS_EVENT(Render::Debug::VRAMDrillerBus, UnregisterAllocation, address);

                    gRenDev->m_DevBufMan.Destroy(stream.buffer);
                }
                pVContainer->m_RemappedBoneIndices.erase(pVContainer->m_RemappedBoneIndices.begin() + j);
                break;
            }
        }
    }
    pVContainer->m_DeletedBoneIndices[threadId].clear();
    for (size_t i = 0, end = pVContainer->m_CreatedBoneIndices[threadId].size(); i < end; ++i)
    {
        bool bFound = false;
        SBoneIndexStreamRequest& rBoneIndexStreamRequest = pVContainer->m_CreatedBoneIndices[threadId][i];
        const uint32 guid = rBoneIndexStreamRequest.guid;
        for (size_t j = 0, numIndices = m_RemappedBoneIndices.size(); j < numIndices; ++j)
        {
            if (m_RemappedBoneIndices[j].guid == guid && m_RemappedBoneIndices[j].refcount)
            {
                bFound = true;
                ++m_RemappedBoneIndices[j].refcount;
            }
        }

        if (bFound == false)
        {
            size_t bufferSize = pVContainer->GetVerticesCount() * pVContainer->GetStreamStride(VSF_HWSKIN_INFO);
            SBoneIndexStream stream;
            stream.buffer = gRenDev->m_DevBufMan.Create(BBT_VERTEX_BUFFER, BU_STATIC, bufferSize);
            stream.guid = guid;
            stream.refcount = rBoneIndexStreamRequest.refcount;
            gRenDev->m_DevBufMan.UpdateBuffer(stream.buffer, rBoneIndexStreamRequest.pStream, pVContainer->GetVerticesCount() * pVContainer->GetStreamStride(VSF_HWSKIN_INFO));
            pVContainer->m_RemappedBoneIndices.push_back(stream);

            // Register the allocation with the VRAM driller
            void* address = reinterpret_cast<void*>(stream.buffer);
            const char* bufferName = GetSourceName();
            EBUS_EVENT(Render::Debug::VRAMDrillerBus, RegisterAllocation, address, bufferSize, bufferName, Render::Debug::VRAM_CATEGORY_BUFFER, Render::Debug::VRAM_SUBCATEGORY_BUFFER_VERTEX_BUFFER);
        }

        delete[] rBoneIndexStreamRequest.pStream;
    }
    pVContainer->m_CreatedBoneIndices[threadId].clear();

    return true;
}

void CRenderMesh::ReleaseVB(int nStream)
{
    UnlockVB(nStream);
    if (SMeshStream* pMS = GetVertexStream(nStream, 0))
    {
        if (pMS->m_nID != ~0u)
        {
            // Unregister the allocation with the VRAM driller
            void* address = reinterpret_cast<void*>(pMS->m_nID);
            EBUS_EVENT(Render::Debug::VRAMDrillerBus, UnregisterAllocation, address);

            gRenDev->m_DevBufMan.Destroy(pMS->m_nID);
            pMS->m_nID = ~0u;
        }
        pMS->m_nElements = 0;
        pMS->m_nFrameUpdate = -1;
        pMS->m_nFrameCreate = -1;
    }
}

void CRenderMesh::ReleaseIB()
{
    UnlockIB();
    if (m_IBStream.m_nID != ~0u)
    {
        // Unregister the allocation with the VRAM driller
        void* address = reinterpret_cast<void*>(m_IBStream.m_nID);
        EBUS_EVENT(Render::Debug::VRAMDrillerBus, UnregisterAllocation, address);

        gRenDev->m_DevBufMan.Destroy(m_IBStream.m_nID);
        m_IBStream.m_nID = ~0u;
    }
    m_IBStream.m_nElements = 0;
    m_IBStream.m_nFrameUpdate = -1;
    m_IBStream.m_nFrameCreate = -1;
}

bool CRenderMesh::UpdateIndices_Int(
    const vtx_idx* pNewInds
    , int nInds
    , int nOffsInd
    , uint32 copyFlags)
{
    AZ_TRACE_METHOD();

    //LockVB operates now on a per mesh lock, any thread may access
    //ASSERT_IS_MAIN_THREAD(gRenDev->m_pRT)

    //SREC_AUTO_LOCK(m_sResLock);

    // Resize the index buffer
    if (m_nInds != nInds)
    {
        FreeIB();
        m_nInds = nInds;
    }
    if (!nInds)
    {
        assert(!m_IBStream.m_pUpdateData);
        return true;
    }

    vtx_idx* pDst = LockIB(FSL_VIDEO_CREATE, 0, nInds);
    if (pDst && pNewInds)
    {
        if (copyFlags & FSL_ASYNC_DEFER_COPY && nInds * sizeof(vtx_idx) < RENDERMESH_ASYNC_MEMCPY_THRESHOLD)
        {
            cryAsyncMemcpy(
                &pDst[nOffsInd],
                pNewInds,
                nInds * sizeof(vtx_idx),
                MC_CPU_TO_GPU | copyFlags,
                SetAsyncUpdateState());
        }
        else
        {
            cryMemcpy(
                &pDst[nOffsInd],
                pNewInds,
                nInds * sizeof(vtx_idx),
                MC_CPU_TO_GPU);
            UnlockIndexStream();
        }
    }
    else
    {
        return false;
    }

    return true;
}

bool CRenderMesh::UpdateVertices_Int(
    const void* pVertBuffer
    , int nVertCount
    , int nOffset
    , int nStream
    , uint32 copyFlags)
{
    AZ_TRACE_METHOD();
    //LockVB operates now on a per mesh lock, any thread may access
    //  ASSERT_IS_MAIN_THREAD(gRenDev->m_pRT)

    int nStride;

    //SREC_AUTO_LOCK(m_sResLock);

    // Resize the vertex buffer
    if (m_nVerts != nVertCount)
    {
        for (int i = 0; i < VSF_NUM; i++)
        {
            FreeVB(i);
        }
        m_nVerts = nVertCount;
    }
    if (!m_nVerts)
    {
        return true;
    }

    byte* pDstVB = (byte*)LockVB(nStream, FSL_VIDEO_CREATE, nVertCount, &nStride);
    assert((nVertCount == 0) || pDstVB);
    if (pDstVB && pVertBuffer)
    {
        if (copyFlags & FSL_ASYNC_DEFER_COPY && nStride * nVertCount < RENDERMESH_ASYNC_MEMCPY_THRESHOLD)
        {
            cryAsyncMemcpy(
                &pDstVB[nOffset],
                pVertBuffer,
                nStride * nVertCount,
                MC_CPU_TO_GPU | copyFlags,
                SetAsyncUpdateState());
        }
        else
        {
            cryMemcpy(
                &pDstVB[nOffset],
                pVertBuffer,
                nStride * nVertCount,
                MC_CPU_TO_GPU);
            UnlockStream(nStream);
        }
    }
    else
    {
        return false;
    }

    return true;
}

bool CRenderMesh::UpdateVertices(
    const void* pVertBuffer
    , int nVertCount
    , int nOffset
    , int nStream
    , uint32 copyFlags
    , bool requiresLock)
{
    bool result = false;
    if (requiresLock)
    {
        SREC_AUTO_LOCK(m_sResLock);
        result = UpdateVertices_Int(pVertBuffer, nVertCount, nOffset, nStream, copyFlags);
    }
    else
    {
        result = UpdateVertices_Int(pVertBuffer, nVertCount, nOffset, nStream, copyFlags);
    }
    return result;
}

bool CRenderMesh::UpdateIndices(
    const vtx_idx* pNewInds
    , int nInds
    , int nOffsInd
    , uint32 copyFlags
    , bool requiresLock)
{
    bool result = false;
    if (requiresLock)
    {
        SREC_AUTO_LOCK(m_sResLock);
        result = UpdateIndices_Int(pNewInds, nInds, nOffsInd, copyFlags);
    }
    else
    {
        result = UpdateIndices_Int(pNewInds, nInds, nOffsInd, copyFlags);
    }
    return result;
}

bool CRenderMesh::UpdateVidIndices(SMeshStream& IBStream, [[maybe_unused]] bool stall)
{
    SCOPED_RENDERER_ALLOCATION_NAME_HINT(GetSourceName());
    AZ_TRACE_METHOD();

    assert(gRenDev->m_pRT->IsRenderThread());

    SREC_AUTO_LOCK(m_sResLock);

    assert(gRenDev->m_pRT->IsRenderThread());

    int nInds = m_nInds;

    if (!nInds)
    {
        // 0 size index buffer creation crashes on deprecated platform
        assert(nInds);
        return false;
    }

    if (IBStream.m_nElements != m_nInds && _HasIBStream())
    {
        ReleaseIB();
    }

    if (IBStream.m_nID == ~0u)
    {
        const size_t bufferSize = nInds * sizeof(vtx_idx);
        IBStream.m_nID = gRenDev->m_DevBufMan.Create(BBT_INDEX_BUFFER, (BUFFER_USAGE)m_eType, bufferSize);
        IBStream.m_nElements = m_nInds;
        IBStream.m_nFrameCreate =  gRenDev->m_RP.m_TI[gRenDev->m_pRT->IsMainThread()    ? gRenDev->m_RP.m_nFillThreadID : gRenDev->m_RP.m_nProcessThreadID].m_nFrameUpdateID;

        // Register the allocation with the VRAM driller
        void* address = reinterpret_cast<void*>(IBStream.m_nID);
        const char* bufferName = GetSourceName();
        EBUS_EVENT(Render::Debug::VRAMDrillerBus, RegisterAllocation, address, bufferSize, bufferName, Render::Debug::VRAM_CATEGORY_BUFFER, Render::Debug::VRAM_SUBCATEGORY_BUFFER_INDEX_BUFFER);
    }
    if (IBStream.m_nID != ~0u)
    {
        UnlockIndexStream();
        if (m_IBStream.m_pUpdateData)
        {
            bool bRes = true;
            return gRenDev->m_DevBufMan.UpdateBuffer(IBStream.m_nID, IBStream.m_pUpdateData, m_nInds * sizeof(vtx_idx));
        }
    }
    return false;
}

bool CRenderMesh::CreateVidVertices(int nStream)
{
    SCOPED_RENDERER_ALLOCATION_NAME_HINT(GetSourceName());
    AZ_TRACE_METHOD();

    SREC_AUTO_LOCK(m_sResLock);
    CRenderer* pRend = gRenDev;

    if (gRenDev->m_bDeviceLost)
    {
        return false;
    }

    assert (!_HasVBStream(nStream));
    SMeshStream* pMS = GetVertexStream(nStream, FSL_WRITE);
    int nSize = GetStreamSize(nStream, m_nVerts);
    pMS->m_nID = gRenDev->m_DevBufMan.Create(BBT_VERTEX_BUFFER, (BUFFER_USAGE)m_eType, nSize);
    pMS->m_nElements = m_nVerts;
    pMS->m_nFrameCreate = gRenDev->m_RP.m_TI
        [gRenDev->m_pRT->IsMainThread()
         ? gRenDev->m_RP.m_nFillThreadID
         : gRenDev->m_RP.m_nProcessThreadID].m_nFrameUpdateID;

    // Register the allocation with the VRAM driller
    void* address = reinterpret_cast<void*>(pMS->m_nID);
    const char* bufferName = GetSourceName();
    EBUS_EVENT(Render::Debug::VRAMDrillerBus, RegisterAllocation, address, nSize, bufferName, Render::Debug::VRAM_CATEGORY_BUFFER, Render::Debug::VRAM_SUBCATEGORY_BUFFER_VERTEX_BUFFER);

    return (pMS->m_nID != ~0u);
}

bool CRenderMesh::UpdateVidVertices(int nStream, [[maybe_unused]] bool stall)
{
    AZ_TRACE_METHOD();

    assert(gRenDev->m_pRT->IsRenderThread());

    SREC_AUTO_LOCK(m_sResLock);

    assert(nStream < VSF_NUM);
    SMeshStream* pMS = GetVertexStream(nStream, FSL_WRITE);

    if (m_nVerts != pMS->m_nElements && _HasVBStream(nStream))
    {
        ReleaseVB(nStream);
    }

    if (pMS->m_nID == ~0u)
    {
        if (!CreateVidVertices(nStream))
        {
            return false;
        }
    }
    if (pMS->m_nID != ~0u)
    {
        UnlockStream(nStream);
        if (pMS->m_pUpdateData)
        {
            return gRenDev->m_DevBufMan.UpdateBuffer(pMS->m_nID, pMS->m_pUpdateData, GetStreamSize(nStream));
            ;
        }
        else
        {
            assert(0);
        }
    }
    return false;
}

#ifdef MESH_TESSELLATION_RENDERER
template<class VecPos, class VecUV>
bool CRenderMesh::UpdateUVCoordsAdjacency(SMeshStream& IBStream, const AZ::Vertex::Format& vertexFormat)
{
    SCOPED_RENDERER_ALLOCATION_NAME_HINT(GetSourceName());
    AZ_TRACE_METHOD();

    assert(gRenDev->m_pRT->IsRenderThread());

    SREC_AUTO_LOCK(m_sResLock);

    int nInds = m_nInds * 4;

    if (!nInds)
    {
        // 0 size index buffer creation crashes on deprecated platform
        assert(nInds);
        return false;
    }

    SMeshStream* pMS = GetVertexStream(VSF_GENERAL);

    if (IBStream.m_nID != ~0u && pMS)
    {
        if (m_IBStream.m_pUpdateData)
        {
            bool bRes = true;
            std::vector<VecUV> pTxtAdjBuffer;

            // create triangles with adjacency
            byte* pVertexStream = (byte*)pMS->m_pUpdateData;
            uint stride = vertexFormat.GetStride();
            uint offset = 0;
            // Only handle one uv set for now.
            if ((m_vertexFormat.TryCalculateOffset(offset, AZ::Vertex::AttributeUsage::TexCoord)) && pVertexStream)
            {
                int nTrgs = m_nInds / 3;
                pTxtAdjBuffer.resize(nTrgs * 12);
                int nVerts = GetNumVerts();
                for (int n = 0; n < nTrgs; ++n)
                {
                    // fill in the dummy adjacency first
                    VecUV* pDst = &pTxtAdjBuffer[n * 12];
                    vtx_idx* pSrc = (vtx_idx*)m_IBStream.m_pUpdateData + n * 3;
                    // triangle itself
                    memcpy(&pDst[0], &pVertexStream[pSrc[0] * stride + offset], sizeof(VecUV));
                    memcpy(&pDst[1], &pVertexStream[pSrc[1] * stride + offset], sizeof(VecUV));
                    memcpy(&pDst[2], &pVertexStream[pSrc[2] * stride + offset], sizeof(VecUV));

                    // adjacency by edges
                    memcpy(&pDst[3], &pVertexStream[pSrc[0] * stride + offset], sizeof(VecUV));
                    memcpy(&pDst[4], &pVertexStream[pSrc[1] * stride + offset], sizeof(VecUV));
                    memcpy(&pDst[5], &pVertexStream[pSrc[1] * stride + offset], sizeof(VecUV));
                    memcpy(&pDst[6], &pVertexStream[pSrc[2] * stride + offset], sizeof(VecUV));
                    memcpy(&pDst[7], &pVertexStream[pSrc[2] * stride + offset], sizeof(VecUV));
                    memcpy(&pDst[8], &pVertexStream[pSrc[0] * stride + offset], sizeof(VecUV));
                    // adjacency by corners
                    memcpy(&pDst[9], &pVertexStream[pSrc[0] * stride + offset], sizeof(VecUV));
                    memcpy(&pDst[10], &pVertexStream[pSrc[1] * stride + offset], sizeof(VecUV));
                    memcpy(&pDst[11], &pVertexStream[pSrc[2] * stride + offset], sizeof(VecUV));
                }

                // now real adjacency is computed
                BuildAdjacency<VecPos, VecUV>(pVertexStream, vertexFormat, nVerts, (vtx_idx*)m_IBStream.m_pUpdateData, nTrgs, pTxtAdjBuffer);

                m_adjBuffer.Create(pTxtAdjBuffer.size(), sizeof(Vec2f16), DXGI_FORMAT_R16G16_FLOAT, DX11BUF_BIND_SRV, &pTxtAdjBuffer[0]);

                if constexpr (sizeof(VecUV) == sizeof(Vec2f16))
                {
                    m_adjBuffer.Create(pTxtAdjBuffer.size(), sizeof(VecUV), DXGI_FORMAT_R16G16_FLOAT, DX11BUF_BIND_SRV, &pTxtAdjBuffer[0]);
                }
                else
                {
                    m_adjBuffer.Create(pTxtAdjBuffer.size(), sizeof(VecUV), DXGI_FORMAT_R32G32_FLOAT, DX11BUF_BIND_SRV, &pTxtAdjBuffer[0]);
                }

                // HS needs to know iPatchID offset for each drawcall, so we pass this offset in the constant buffer
                // currently texture buffer is created, but when parser support for cbuffer is added (AI: AndreyK), we
                // need to change the below call to:
                // ((CREMeshImpl*) m_Chunks[iChunk].pRE)->m_tessCB.Create(4, sizeof(int), (DXGI_FORMAT)0, &myBuffer[0], D3D11_BIND_CONSTANT_BUFFER);
                for (int iChunk = 0; iChunk < m_Chunks.size(); ++iChunk)
                {
                    int myBuffer[4];
                    myBuffer[0] = m_Chunks[iChunk].nFirstIndexId / 3;
                    ((CREMeshImpl*) m_Chunks[iChunk].pRE)->m_tessCB.Create(4, sizeof(int), DXGI_FORMAT_R32_SINT, DX11BUF_BIND_SRV, &myBuffer[0]);
                }
            }

            return bRes;
        }
    }

    return false;
}
#endif //#ifdef MESH_TESSELLATION_RENDERER

void CRenderMesh::Render(const SRendParams& rParams, CRenderObject* pObj, _smart_ptr<IMaterial> pMaterial, const SRenderingPassInfo& passInfo, bool bSkinned)
{
    FUNCTION_PROFILER_FAST(GetISystem(), PROFILE_RENDERER, g_bProfilerEnabled);

    IF (!CanRender(), 0)
    {
        return;
    }

    int nList = rParams.nRenderList;
    int nAW = rParams.nAfterWater;
    CRenderer* rd = gRenDev;

#if !defined(_RELEASE)
    const char* szExcl = CRenderer::CV_r_excludemesh->GetString();
    if (szExcl[0] && m_sSource)
    {
        char szMesh[1024];
        cry_strcpy(szMesh, this->m_sSource);
        azstrlwr(szMesh, AZ_ARRAY_SIZE(szMesh));
        if (szExcl[0] == '!')
        {
            if (!strstr(&szExcl[1], m_sSource))
            {
                return;
            }
        }
        else
        if (strstr(szExcl, m_sSource))
        {
            return;
        }
    }
#endif

    if (rd->m_pDefaultMaterial && pMaterial)
    {
        pMaterial = rd->m_pDefaultMaterial;
    }

    assert(pMaterial);

    if (!pMaterial || !m_nVerts || !m_nInds || m_Chunks.empty())
    {
        return;
    }

    pObj->m_pRenderNode = rParams.pRenderNode;
    pObj->m_pCurrMaterial = pMaterial;

    if (rParams.nHUDSilhouettesParams || rParams.nVisionParams || rParams.pInstance)
    {
        SRenderObjData* pOD = rd->EF_GetObjData(pObj, true, passInfo.ThreadID());

        pOD->m_nHUDSilhouetteParams = rParams.nHUDSilhouettesParams;
        pOD->m_uniqueObjectId = reinterpret_cast<uintptr_t>(rParams.pInstance);
    }


    assert(!(pObj->m_ObjFlags & FOB_BENDED));

    const bool bSG = passInfo.IsShadowPass();

    if (gRenDev->CV_r_MotionVectors && passInfo.IsGeneralPass() && ((pObj->m_ObjFlags & FOB_DYNAMIC_OBJECT) != 0))
    {
        CMotionBlur::SetupObject(pObj, passInfo);
    }

    TRenderChunkArray* pChunks = bSkinned ? &m_ChunksSkinned : &m_Chunks;

    if (pChunks)
    {
        const uint32 ni = (uint32)pChunks->size();
        for (uint32 i = 0; i < ni; i++)
        {
            CRenderChunk* pChunk = &pChunks->at(i);
            CRendElementBase* pREMesh = pChunk->pRE;

            SShaderItem& ShaderItem = pMaterial->GetShaderItem(pChunk->m_nMatID);

            CShaderResources* pR = (CShaderResources*)ShaderItem.m_pShaderResources;
            CShader* pS = (CShader*)ShaderItem.m_pShader;
            if (pREMesh && pS && pR)
            {
                if (pS->m_Flags2 & EF2_NODRAW)
                {
                    continue;
                }

                if (bSG && (pMaterial->GetSafeSubMtl(pChunk->m_nMatID)->GetFlags() & MTL_FLAG_NOSHADOW))
                {
                    continue;
                }

                rd->EF_AddEf_NotVirtual(pREMesh, ShaderItem, pObj, passInfo, nList, nAW, SRendItemSorter(rParams.rendItemSorter));
            }
        }
    }
}


void CRenderMesh::SetREUserData(float* pfCustomData, [[maybe_unused]] float fFogScale, [[maybe_unused]] float fAlpha)
{
    for (int i = 0; i < m_Chunks.size(); i++)
    {
        if (m_Chunks[i].pRE)
        {
            m_Chunks[i].pRE->m_CustomData = pfCustomData;
        }
    }
}

void CRenderMesh::AddRenderElements(_smart_ptr<IMaterial> pIMatInfo, CRenderObject* pObj, const SRenderingPassInfo& passInfo, int nList, int nAW)
{
    SRendItemSorter rendItemSorter = passInfo.IsShadowPass() ? SRendItemSorter::CreateShadowPassRendItemSorter(passInfo) : SRendItemSorter::CreateRendItemSorter(passInfo);

    assert(!(pObj->m_ObjFlags & FOB_BENDED));
    //assert (!pObj->GetInstanceInfo(0));
    assert(pIMatInfo);

    if (!_GetVertexContainer()->m_nVerts || !m_Chunks.size() || !pIMatInfo)
    {
        return;
    }

    if (gRenDev->m_pDefaultMaterial && gRenDev->m_pTerrainDefaultMaterial)
    {
        IShader* pShader = pIMatInfo->GetShaderItem().m_pShader;
        pIMatInfo = gRenDev->m_pDefaultMaterial;
    }

    for (int i = 0; i < m_Chunks.size(); i++)
    {
        CRenderChunk* pChunk = &m_Chunks[i];
        CREMeshImpl* pOrigRE = (CREMeshImpl*) pChunk->pRE;

        // get material

        SShaderItem& shaderItem = pIMatInfo->GetShaderItem(pChunk->m_nMatID);

        //    if (nTechniqueID > 0)
        //    shaderItem.m_nTechnique = shaderItem.m_pShader->GetTechniqueID(shaderItem.m_nTechnique, nTechniqueID);

        if (shaderItem.m_pShader && pOrigRE)// && pMat->nNumIndices)
        {
            TArray<CRendElementBase*>* pREs = shaderItem.m_pShader->GetREs(shaderItem.m_nTechnique);

            assert(pOrigRE->m_pChunk->nFirstIndexId < 60000);

            if (!pREs || !pREs->Num())
            {
                gRenDev->EF_AddEf_NotVirtual(pOrigRE, shaderItem, pObj, passInfo, nList, nAW, rendItemSorter);
            }
            else
            {
                gRenDev->EF_AddEf_NotVirtual(pREs->Get(0), shaderItem, pObj, passInfo, nList, nAW, rendItemSorter);
            }
        }
    } //i
}

void CRenderMesh::AddRE(_smart_ptr<IMaterial> pMaterial, CRenderObject* obj, IShader* ef, const SRenderingPassInfo& passInfo, int nList, int nAW, const SRendItemSorter& rendItemSorter)
{
    if (!m_nVerts || !m_Chunks.size())
    {
        return;
    }

    assert(!(obj->m_ObjFlags & FOB_BENDED));

    for (int i = 0; i < m_Chunks.size(); i++)
    {
        if (!m_Chunks[i].pRE)
        {
            continue;
        }

        SShaderItem& SH = pMaterial->GetShaderItem();
        if (ef)
        {
            SH.m_pShader = ef;
        }
        if (SH.m_pShader)
        {
            assert(m_Chunks[i].pRE->m_pChunk->nFirstIndexId < 60000);

            TArray<CRendElementBase*>* pRE = SH.m_pShader->GetREs(SH.m_nTechnique);
            if (!pRE || !pRE->Num())
            {
                gRenDev->EF_AddEf_NotVirtual(m_Chunks[i].pRE, SH, obj, passInfo, nList, nAW, rendItemSorter);
            }
            else
            {
                gRenDev->EF_AddEf_NotVirtual(SH.m_pShader->GetREs(SH.m_nTechnique)->Get(0), SH, obj, passInfo, nList, nAW, rendItemSorter);
            }
        }
    }
}

size_t CRenderMesh::GetMemoryUsage(ICrySizer* pSizer, EMemoryUsageArgument nType) const
{
    size_t nSize = 0;
    switch (nType)
    {
    case MEM_USAGE_COMBINED:
        nSize = Size(SIZE_ONLY_SYSTEM) + Size(SIZE_VB | SIZE_IB);
        break;
    case MEM_USAGE_ONLY_SYSTEM:
        nSize = Size(SIZE_ONLY_SYSTEM);
        break;
    case MEM_USAGE_ONLY_VIDEO:
        nSize = Size(SIZE_VB | SIZE_IB);
        return nSize;
        break;
    case MEM_USAGE_ONLY_STREAMS:
        nSize = Size(SIZE_ONLY_SYSTEM) + Size(SIZE_VB | SIZE_IB);

        if (pSizer)
        {
            SIZER_COMPONENT_NAME(pSizer, "STREAM MESH");
            pSizer->AddObject((void*)this, nSize);
        }

        // Not add overhead allocations.
        return nSize;
        break;
    }

    {
        nSize += sizeof(*this);
        for (int i = 0; i < (int)m_Chunks.capacity(); i++)
        {
            if (i < m_Chunks.size())
            {
                nSize += m_Chunks[i].Size();
            }
            else
            {
                nSize += sizeof(CRenderChunk);
            }
        }
        for (int i = 0; i < (int)m_ChunksSkinned.capacity(); i++)
        {
            if (i < m_ChunksSkinned.size())
            {
                nSize += m_ChunksSkinned.at(i).Size();
            }
            else
            {
                nSize += sizeof(CRenderChunk);
            }
        }
    }

    if (pSizer)
    {
        pSizer->AddObject((void*)this, nSize);

#ifdef RENDER_MESH_TRIANGLE_HASH_MAP_SUPPORT
        if (m_pTrisMap)
        {
            SIZER_COMPONENT_NAME(pSizer, "Hash map");
            nSize += stl::size_of_map(*m_pTrisMap);
        }
#endif

        for (MeshSubSetIndices::const_iterator it = m_meshSubSetIndices.begin(); it != m_meshSubSetIndices.end(); ++it)
        {
            // Collect memory usage for index sub-meshes.
            it->second->GetMemoryUsage(pSizer, nType);
        }
    }

    return nSize;
}

void CRenderMesh::GetMemoryUsage(ICrySizer* pSizer) const
{
    pSizer->AddObject(this, sizeof(*this));
    {
        SIZER_COMPONENT_NAME(pSizer, "Vertex Data");
        for (uint32 i = 0; i < VSF_NUM; i++)
        {
            if (SMeshStream* pMS = GetVertexStream(i, 0))
            {
                if (pMS->m_pUpdateData)
                {
                    pSizer->AddObject(pMS->m_pUpdateData, GetStreamSize(i));
                }
            }
        }
    }

    {
        SIZER_COMPONENT_NAME(pSizer, "FP16 Cache");
        if (m_pCachePos)
        {
            pSizer->AddObject(m_pCachePos, m_nVerts * sizeof(Vec3));
        }
        for (int i = 0; i < m_UVCache.size(); ++i)
        {
            if (m_UVCache[i])
            {
                pSizer->AddObject(m_UVCache[i], m_nVerts * sizeof(Vec2));
            }
        }
    }

    {
        SIZER_COMPONENT_NAME(pSizer, "Mesh Chunks");
        pSizer->AddObject(m_Chunks);
    }
    {
        SIZER_COMPONENT_NAME(pSizer, "Mesh Skinned Chunks");
        pSizer->AddObject(m_ChunksSkinned);
    }

#ifdef RENDER_MESH_TRIANGLE_HASH_MAP_SUPPORT
    {
        SIZER_COMPONENT_NAME(pSizer, "Hash map");
        pSizer->AddObject(m_pTrisMap);
    }
#endif
    for (MeshSubSetIndices::const_iterator it = m_meshSubSetIndices.begin(); it != m_meshSubSetIndices.end(); ++it)
    {
        // Collect memory usage for index sub-meshes.
        it->second->GetMemoryUsage(pSizer);
    }
}

int CRenderMesh::GetAllocatedBytes(bool bVideoMem) const
{
    if (!bVideoMem)
    {
        return Size(SIZE_ONLY_SYSTEM);
    }
    else
    {
        return Size(SIZE_VB | SIZE_IB);
    }
}

int CRenderMesh::GetTextureMemoryUsage(const _smart_ptr<IMaterial> pMaterial, ICrySizer* pSizer, bool bStreamedIn) const
{
    // If no input material use internal render mesh material.
    if (!pMaterial)
    {
        return 0;
    }

    int textureSize = 0;
    std::set<const CTexture*>   used;
    for (int a = 0; a < m_Chunks.size(); a++)
    {
        const CRenderChunk* pChunk = &m_Chunks[a];

        // Override default material
        const SShaderItem& shaderItem = pMaterial->GetShaderItem(pChunk->m_nMatID);
        if (!shaderItem.m_pShaderResources)
        {
            continue;
        }

        const CShaderResources*     pRes = (const CShaderResources*)shaderItem.m_pShaderResources;
        for (auto iter = pRes->m_TexturesResourcesMap.begin(); iter != pRes->m_TexturesResourcesMap.end(); ++iter)
        {
            const CTexture*     pTexture = iter->second.m_Sampler.m_pTex;
            if (!pTexture)
            {
                continue;
            }

            if (used.find(pTexture) != used.end()) // Already used in size calculation.
            {
                continue;
            }
            used.insert(pTexture);

            int nTexSize = bStreamedIn ? pTexture->GetDeviceDataSize() : pTexture->GetDataSize();
            textureSize += nTexSize;

            if (pSizer)
            {
                pSizer->AddObject(pTexture, nTexSize);
            }
        }
    }
    return textureSize;
}

float CRenderMesh::GetAverageTrisNumPerChunk(_smart_ptr<IMaterial> pMat)
{
    float fTrisNum = 0;
    float fChunksNum = 0;

    for (int m = 0; m < m_Chunks.size(); m++)
    {
        const CRenderChunk& chunk = m_Chunks[m];
        if ((chunk.m_nMatFlags & MTL_FLAG_NODRAW) || !chunk.pRE)
        {
            continue;
        }

        _smart_ptr<IMaterial> pCustMat;
        if (pMat && chunk.m_nMatID < pMat->GetSubMtlCount())
        {
            pCustMat = pMat->GetSubMtl(chunk.m_nMatID);
        }
        else
        {
            pCustMat = pMat;
        }

        if (!pCustMat)
        {
            continue;
        }

        const IShader* pShader = pCustMat->GetShaderItem().m_pShader;

        if (!pShader)
        {
            continue;
        }

        if (pShader->GetFlags2() & EF2_NODRAW)
        {
            continue;
        }

        fTrisNum += chunk.nNumIndices / 3;
        fChunksNum++;
    }

    return fChunksNum ? (fTrisNum / fChunksNum) : 0;
}

void CRenderMesh::InitTriHash(_smart_ptr<IMaterial> pMaterial)
{
#ifdef RENDER_MESH_TRIANGLE_HASH_MAP_SUPPORT

    SAFE_DELETE(m_pTrisMap);
    m_pTrisMap = new TrisMap;

    int nPosStride = 0;
    int nIndCount = m_nInds;
    const byte* pPositions = GetPosPtr(nPosStride, FSL_READ);
    const vtx_idx* pIndices = GetIndexPtr(FSL_READ);

    iLog->Log("CRenderMesh::InitTriHash: Tris=%d, Verts=%d, Name=%s ...", nIndCount / 3, GetVerticesCount(), GetSourceName() ? GetSourceName() : "Null");

    if (pIndices && pPositions && m_Chunks.size() && nIndCount && GetVerticesCount())
    {
        for (uint32 ii = 0; ii < (uint32)m_Chunks.size(); ii++)
        {
            CRenderChunk* pChunk = &m_Chunks[ii];

            if (pChunk->m_nMatFlags & MTL_FLAG_NODRAW || !pChunk->pRE)
            {
                continue;
            }

            // skip transparent and alpha test
            const SShaderItem& shaderItem = pMaterial->GetShaderItem(pChunk->m_nMatID);
            if (!shaderItem.IsZWrite() || !shaderItem.m_pShaderResources || shaderItem.m_pShaderResources->IsAlphaTested())
            {
                continue;
            }

            if (shaderItem.m_pShader && shaderItem.m_pShader->GetFlags() & EF_DECAL)
            {
                continue;
            }

            uint32 nFirstIndex = pChunk->nFirstIndexId;
            uint32 nLastIndex = pChunk->nFirstIndexId + pChunk->nNumIndices;

            for (uint32 i = nFirstIndex; i < nLastIndex; i += 3)
            {
                int32 I0    =   pIndices[i + 0];
                int32 I1    =   pIndices[i + 1];
                int32 I2    =   pIndices[i + 2];

                Vec3 v0 = *(Vec3*)&pPositions[nPosStride * I0];
                Vec3 v1 = *(Vec3*)&pPositions[nPosStride * I1];
                Vec3 v2 = *(Vec3*)&pPositions[nPosStride * I2];

                AABB triBox;
                triBox.min = triBox.max = v0;
                triBox.Add(v1);
                triBox.Add(v2);

                float fRayLen = CRenderer::CV_r_RenderMeshHashGridUnitSize / 2;
                triBox.min -= Vec3(fRayLen, fRayLen, fRayLen);
                triBox.max += Vec3(fRayLen, fRayLen, fRayLen);

                AABB aabbCell;

                aabbCell.min = triBox.min / CRenderer::CV_r_RenderMeshHashGridUnitSize;
                aabbCell.min.x = floor(aabbCell.min.x);
                aabbCell.min.y = floor(aabbCell.min.y);
                aabbCell.min.z = floor(aabbCell.min.z);

                aabbCell.max = triBox.max / CRenderer::CV_r_RenderMeshHashGridUnitSize;
                aabbCell.max.x = ceil(aabbCell.max.x);
                aabbCell.max.y = ceil(aabbCell.max.y);
                aabbCell.max.z = ceil(aabbCell.max.z);

                for (float x = aabbCell.min.x; x < aabbCell.max.x; x++)
                {
                    for (float y = aabbCell.min.y; y < aabbCell.max.y; y++)
                    {
                        for (float z = aabbCell.min.z; z < aabbCell.max.z; z++)
                        {
                            AABB cellBox;
                            cellBox.min = Vec3(x, y, z) * CRenderer::CV_r_RenderMeshHashGridUnitSize;
                            cellBox.max = cellBox.min + Vec3(CRenderer::CV_r_RenderMeshHashGridUnitSize, CRenderer::CV_r_RenderMeshHashGridUnitSize, CRenderer::CV_r_RenderMeshHashGridUnitSize);
                            cellBox.min -= Vec3(fRayLen, fRayLen, fRayLen);
                            cellBox.max += Vec3(fRayLen, fRayLen, fRayLen);
                            if (!Overlap::AABB_Triangle(cellBox, v0, v1, v2))
                            {
                                continue;
                            }

                            int key = (int)(x * 256.f * 256.f + y * 256.f + z);
                            PodArray<std::pair<int, int> >* pTris = &(*m_pTrisMap)[key];
                            std::pair<int, int> t(i, pChunk->m_nMatID);
                            if (pTris->Find(t) < 0)
                            {
                                pTris->Add(t);
                            }
                        }
                    }
                }
            }
        }
    }

    iLog->LogPlus(" ok (%" PRISIZE_T ")", m_pTrisMap->size());

#endif
}


const PodArray<std::pair<int, int> >* CRenderMesh::GetTrisForPosition([[maybe_unused]] const Vec3& vPos, _smart_ptr<IMaterial> pMaterial)
{
#ifdef RENDER_MESH_TRIANGLE_HASH_MAP_SUPPORT

    if (!m_pTrisMap)
    {
        AUTO_LOCK(m_getTrisForPositionLock);
        if (!m_pTrisMap)
        {
            InitTriHash(pMaterial);
        }
    }

    Vec3 vCellMin = vPos / CRenderer::CV_r_RenderMeshHashGridUnitSize;
    vCellMin.x = floor(vCellMin.x);
    vCellMin.y = floor(vCellMin.y);
    vCellMin.z = floor(vCellMin.z);

    int key = (int)(vCellMin.x * 256.f * 256.f + vCellMin.y * 256.f + vCellMin.z);

    const TrisMap::iterator& iter = (*m_pTrisMap).find(key);
    if (iter != (*m_pTrisMap).end())
    {
        return &iter->second;
    }
#else
    AZ_Assert(false, "NOT IMPLEMENTED: CRenderMesh::GetTrisForPosition(const Vec3& vPos, _smart_ptr<IMaterial> pMaterial)");
#endif

    return 0;
}

void CRenderMesh::UpdateBBoxFromMesh()
{
    PROFILE_FRAME(UpdateBBoxFromMesh);

    AABB aabb;
    aabb.Reset();

    int nVertCount = _GetVertexContainer()->GetVerticesCount();
    int nPosStride = 0;
    int nIndCount = GetIndicesCount();
    const byte* pPositions = GetPosPtr(nPosStride, FSL_READ);
    const vtx_idx* pIndices = GetIndexPtr(FSL_READ);

    if (!pIndices || !pPositions)
    {
        assert(!"Mesh is not ready");
        return;
    }

    for (int32 a = 0; a < m_Chunks.size(); a++)
    {
        CRenderChunk* pChunk = &m_Chunks[a];

        if (pChunk->m_nMatFlags & MTL_FLAG_NODRAW || !pChunk->pRE)
        {
            continue;
        }

        uint32 nFirstIndex = pChunk->nFirstIndexId;
        uint32 nLastIndex = pChunk->nFirstIndexId + pChunk->nNumIndices;

        for (uint32 i = nFirstIndex; i < nLastIndex; i++)
        {
            int32 I0  =   pIndices[i];
            if (I0 < nVertCount)
            {
                Vec3 v0 = *(Vec3*)&pPositions[nPosStride * I0];
                aabb.Add(v0);
            }
            else
            {
                assert(!"Index is out of range");
            }
        }
    }

    if (!aabb.IsReset())
    {
        m_vBoxMax = aabb.max;
        m_vBoxMin = aabb.min;
    }
}

static void ExtractBoneIndicesAndWeights(uint16* outIndices, Vec4& outWeights, const JointIdType* aBoneRemap, const uint16* indices, UCol weights)
{
    // Get 8-bit weights as floats
    outWeights[0] = weights.bcolor[0];
    outWeights[1] = weights.bcolor[1];
    outWeights[2] = weights.bcolor[2];
    outWeights[3] = weights.bcolor[3];

    // Get bone indices
    if (aBoneRemap)
    {
        outIndices[0] = aBoneRemap[indices[0]];
        outIndices[1] = aBoneRemap[indices[1]];
        outIndices[2] = aBoneRemap[indices[2]];
        outIndices[3] = aBoneRemap[indices[3]];
    }
    else
    {
        outIndices[0] = indices[0];
        outIndices[1] = indices[1];
        outIndices[2] = indices[2];
        outIndices[3] = indices[3];
    }
}

static void BlendDualQuats(DualQuat& outBone, Array<const DualQuat> aBoneLocs, const uint16* indices, const Vec4 outWeights)
{
    outBone = aBoneLocs[indices[0]] * outWeights[0] +
              aBoneLocs[indices[1]] * outWeights[1] +
              aBoneLocs[indices[2]] * outWeights[2] +
              aBoneLocs[indices[3]] * outWeights[3];

    outBone.Normalize();
}

static void BlendMatrices(Matrix34& outBone, Array<const Matrix34> aBoneLocs, const uint16* indices, const Vec4 outWeights)
{
    outBone = (aBoneLocs[indices[0]] * outWeights[0]) + 
              (aBoneLocs[indices[1]] * outWeights[1]) + 
              (aBoneLocs[indices[2]] * outWeights[2]) + 
              (aBoneLocs[indices[3]] * outWeights[3]);
}

struct PosNormData
{
    strided_pointer<Vec3> aPos;
    strided_pointer<Vec3> aNorm;
    strided_pointer<SVF_P3S_N4B_C4B_T2S> aVert;
    strided_pointer<SPipQTangents> aQTan;
    strided_pointer<SPipTangents> aTan2;

    void GetPosNorm(PosNorm& ran, int nV)
    {
        // Position
        ran.vPos = aPos[nV];

        // Normal
        if (aNorm.data)
        {
            ran.vNorm = aNorm[nV];
        }
        else if (aVert.data)
        {
            ran.vNorm = aVert[nV].normal.GetN();
        }
        else if (aTan2.data)
        {
            ran.vNorm = aTan2[nV].GetN();
        }
        else if (aQTan.data)
        {
            ran.vNorm = aQTan[nV].GetN();
        }
    }
};

// To do: replace with VSF_MORPHBUDDY support
#define SKIN_MORPHING 0

struct SkinnedPosNormData
    : PosNormData
{
    SSkinningData const* pSkinningData;

#if SKIN_MORPHING
    strided_pointer<SVF_P3F_P3F_I4B> aMorphing;
#endif

    strided_pointer<SVF_W4B_I4S> aSkinning;


    SkinnedPosNormData() {}

    void GetPosNorm(PosNorm& ran, int nV)
    {
        PosNormData::GetPosNorm(ran, nV);

    #if SKIN_MORPHING
        if (aShapeDeform && aMorphing)
        {
            SVF_P3F_P3F_I4B const& morph = aMorphing[nV];
            uint8 idx = (uint8)morph.index.dcolor;
            float fDeform = aShapeDeform[idx];
            if (fDeform < 0.0f)
            {
                ran.vPos = morph.thin * (-fDeform) + ran.vPos * (fDeform + 1);
            }
            else if (fDeform > 0.f)
            {
                ran.vPos = ran.vPos * (1 - fDeform) + morph.fat * fDeform;
            }
        }

        /*if (!g_arrExtMorphStream.empty())
            ran.vPos += g_arrExtMorphStream[nV];*/
    #endif

        // Skinning
        if (pSkinningData)
        {
            uint16 indices[4];
            Vec4 weights;
            ExtractBoneIndicesAndWeights(indices, weights, pSkinningData->pRemapTable, &aSkinning[nV].indices[0], aSkinning[nV].weights);

            if (pSkinningData->nHWSkinningFlags & eHWS_Skinning_Matrix)
            {
                Matrix34 m;
                BlendMatrices(
                    m,
                    ArrayT(pSkinningData->pBoneMatrices, (int)pSkinningData->nNumBones),
                    indices,
                    weights);
                ran <<= m;
            }
            else
            {
                // Transform the vertex with skinning.
                DualQuat dq;
                BlendDualQuats(
                    dq,
                    ArrayT(pSkinningData->pBoneQuatsS, (int)pSkinningData->nNumBones),
                    indices,
                    weights);
                ran <<= dq;
            }
        }
    }
};

float CRenderMesh::GetExtent(EGeomForm eForm)
{
    if (eForm == GeomForm_Vertices)
    {
        return (float)m_nVerts;
    }
    CGeomExtent& ext = m_Extents.Make(eForm);
    if (!ext)
    {
        LockForThreadAccess();

        vtx_idx* pInds = GetIndexPtr(FSL_READ);
        strided_pointer<Vec3> aPos;
        aPos.data = (Vec3*)GetPosPtr(aPos.iStride, FSL_READ);
        if (pInds && aPos.data)
        {
            // Iterate chunks to track renderable verts
            bool* aValidVerts = new bool[m_nVerts];
            memset(aValidVerts, 0, m_nVerts);
            TRenderChunkArray& aChunks = !m_ChunksSkinned.empty() ? m_ChunksSkinned : m_Chunks;
            for (int c = 0; c < aChunks.size(); ++c)
            {
                const CRenderChunk& chunk = aChunks[c];
                if (chunk.pRE && !(chunk.m_nMatFlags & (MTL_FLAG_NODRAW | MTL_FLAG_REQUIRE_FORWARD_RENDERING)))
                {
                    assert(uint32(chunk.nFirstVertId + chunk.nNumVerts) <= m_nVerts);
                    memset(aValidVerts + chunk.nFirstVertId, 1, chunk.nNumVerts);
                }
            }

            int nParts = TriMeshPartCount(eForm, GetIndicesCount());
            ext.ReserveParts(nParts);
            for (int i = 0; i < nParts; i++)
            {
                int aIndices[3];
                Vec3 aVec[3];
                for (int v = TriIndices(aIndices, i, eForm) - 1; v >= 0; v--)
                {
                    aVec[v] = aPos[ pInds[aIndices[v]] ];
                }
                ext.AddPart(aValidVerts[ pInds[aIndices[0]] ] ? max(TriExtent(eForm, aVec), 0.f) : 0.f);
            }
            delete[] aValidVerts;
        }

        UnlockStream(VSF_GENERAL);
        UnlockIndexStream();
        UnLockForThreadAccess();
    }
    return ext.TotalExtent();
}

void CRenderMesh::GetRandomPos(PosNorm& ran, EGeomForm eForm, SSkinningData const* pSkinning)
{
    LockForThreadAccess();

    SkinnedPosNormData vdata;
    if (vdata.aPos.data = (Vec3*)GetPosPtr(vdata.aPos.iStride, FSL_READ))
    {
        // Check possible sources for normals.
#if ENABLE_NORMALSTREAM_SUPPORT
        if (!GetStridedArray(vdata.aNorm, VSF_NORMALS))
#endif
        if (_GetVertexFormat() != eVF_P3S_N4B_C4B_T2S || !GetStridedArray(vdata.aVert, VSF_GENERAL))
        {
            if (!GetStridedArray(vdata.aTan2, VSF_TANGENTS))
            {
                GetStridedArray(vdata.aQTan, VSF_QTANGENTS);
            }
        }

        vtx_idx* pInds = GetIndexPtr(FSL_READ);

        if (vdata.pSkinningData = pSkinning)
        {
            GetStridedArray(vdata.aSkinning, VSF_HWSKIN_INFO);
        #if SKIN_MORPHING
            GetStridedArray(vdata.aMorphing, VSF_HWSKIN_SHAPEDEFORM_INFO);
        #endif
        }

        // Choose part.
        if (eForm == GeomForm_Vertices)
        {
            if (m_nInds == 0)
            {
                ran.zero();
            }
            else
            {
                int nIndex = cry_random(0U, m_nInds - 1);
                vdata.GetPosNorm(ran, pInds[nIndex]);
            }
        }
        else
        {
            CGeomExtent const& extent = m_Extents[eForm];
            if (!extent.NumParts())
            {
                ran.zero();
            }
            else
            {
                int aIndices[3];
                int nPart = extent.RandomPart();
                int nVerts = TriIndices(aIndices, nPart, eForm);

                // Extract vertices, blend.
                PosNorm aRan[3];
                while (--nVerts >= 0)
                {
                    vdata.GetPosNorm(aRan[nVerts], pInds[aIndices[nVerts]]);
                }
                TriRandomPos(ran, eForm, aRan, true);
            }
        }
    }

    UnLockForThreadAccess();
    UnlockStream(VSF_GENERAL);
    UnlockStream(VSF_QTANGENTS);
    UnlockStream(VSF_TANGENTS);
    UnlockStream(VSF_HWSKIN_INFO);
}

int CRenderChunk::Size() const
{
    size_t nSize = sizeof(*this);
    return static_cast<int>(nSize);
}

void CRenderMesh::Size(uint32 nFlags, ICrySizer* pSizer) const
{
    uint32 i;
    if (!nFlags)  // System size
    {
        for (i = 0; i < VSF_NUM; i++)
        {
            if (SMeshStream* pMS = GetVertexStream(i))
            {
                if (pMS->m_pUpdateData)
                {
                    pSizer->AddObject(pMS->m_pUpdateData, GetStreamSize(i));
                }
            }
        }
        if (m_IBStream.m_pUpdateData)
        {
            pSizer->AddObject(m_IBStream.m_pUpdateData, m_nInds * sizeof(vtx_idx));
        }

        if (m_pCachePos)
        {
            pSizer->AddObject(m_pCachePos, m_nVerts * sizeof(Vec3));
        }
    }
}

size_t CRenderMesh::Size(uint32 nFlags) const
{
    size_t nSize = 0;
    uint32 i;
    if (nFlags == SIZE_ONLY_SYSTEM) // System size
    {
        for (i = 0; i < VSF_NUM; i++)
        {
            if (SMeshStream* pMS = GetVertexStream(i))
            {
                if (pMS->m_pUpdateData)
                {
                    nSize += GetStreamSize(i);
                }
            }
        }
        if (m_IBStream.m_pUpdateData)
        {
            nSize += m_nInds * sizeof(vtx_idx);
        }

        if (m_pCachePos)
        {
            nSize += m_nVerts * sizeof(Vec3);
        }
    }
    if (nFlags & SIZE_VB) // VB size
    {
        for (i = 0; i < VSF_NUM; i++)
        {
            if (_HasVBStream(i))
            {
                nSize += GetStreamSize(i);
            }
        }
    }
    if (nFlags & SIZE_IB) // IB size
    {
        if (_HasIBStream())
        {
            nSize += m_nInds * sizeof(vtx_idx);
        }
    }

    return nSize;
}

void CRenderMesh::FreeDeviceBuffers(bool bRestoreSys)
{
    uint32 i;

    for (i = 0; i < VSF_NUM; i++)
    {
        if (_HasVBStream(i))
        {
            if (bRestoreSys)
            {
                LockForThreadAccess();
                void* pSrc = LockVB(i, FSL_READ | FSL_VIDEO);
                void* pDst = LockVB(i, FSL_SYSTEM_CREATE);
                cryMemcpy(pDst, pSrc, GetStreamSize(i));
                UnLockForThreadAccess();
            }
            ReleaseVB(i);
        }
    }

    if (_HasIBStream())
    {
        if (bRestoreSys)
        {
            LockForThreadAccess();
            void* pSrc = LockIB(FSL_READ | FSL_VIDEO);
            void* pDst = LockIB(FSL_SYSTEM_CREATE);
            cryMemcpy(pDst, pSrc, m_nInds * sizeof(vtx_idx));
            UnLockForThreadAccess();
        }
        ReleaseIB();
    }
}

void CRenderMesh::FreeVB(int nStream)
{
    if (SMeshStream* pMS = GetVertexStream(nStream))
    {
        if (pMS->m_pUpdateData)
        {
            FreeMeshData(pMS->m_pUpdateData);
            pMS->m_pUpdateData = NULL;
        }
    }
}

void CRenderMesh::FreeIB()
{
    if (m_IBStream.m_pUpdateData)
    {
        FreeMeshData(m_IBStream.m_pUpdateData);
        m_IBStream.m_pUpdateData = NULL;
    }
}

void CRenderMesh::FreeSystemBuffers()
{
    uint32 i;

    for (i = 0; i < VSF_NUM; i++)
    {
        FreeVB(i);
    }
    FreeIB();

    FreeMeshData(m_pCachePos);
    m_pCachePos = NULL;
}

//////////////////////////////////////////////////////////////////////////
void CRenderMesh::DebugDraw(const struct SGeometryDebugDrawInfo& info, uint32 nVisibleChunksMask, [[maybe_unused]] float fExtrdueScale)
{    
    IRenderAuxGeom* pRenderAuxGeom = gEnv->pRenderer->GetIRenderAuxGeom();
    LockForThreadAccess();

    const Matrix34& mat = info.tm;

    const bool bNoCull = info.bNoCull;
    const bool bNoLines = info.bNoLines;
    const bool bExtrude = info.bExtrude;

    SAuxGeomRenderFlags prevRenderFlags = pRenderAuxGeom->GetRenderFlags();
    SAuxGeomRenderFlags renderFlags = prevRenderFlags;
    renderFlags.SetDepthWriteFlag(e_DepthWriteOff);
    if (bNoCull)
    {
        renderFlags.SetCullMode(e_CullModeNone);
    }
    pRenderAuxGeom->SetRenderFlags(renderFlags);

    const ColorB lineColor = info.lineColor;
    const ColorB color = info.color;

#ifdef WIN32
    const unsigned int kMaxBatchSize = 20000;
    static std::vector<Vec3> s_vertexBuffer;
    static std::vector<vtx_idx> s_indexBuffer;
    s_vertexBuffer.reserve(kMaxBatchSize);
    s_indexBuffer.reserve(kMaxBatchSize * 2);
    s_vertexBuffer.resize(0);
    s_indexBuffer.resize(0);
    uint32 currentIndexBase = 0;
#endif

    const int32 chunkCount = m_Chunks.size();
    for (int32 currentChunkIndex = 0; currentChunkIndex < chunkCount; ++currentChunkIndex)
    {
        const CRenderChunk* pChunk = &m_Chunks[currentChunkIndex];

        if (pChunk->m_nMatFlags & MTL_FLAG_NODRAW || !pChunk->pRE)
        {
            continue;
        }

        if (!((1 << currentChunkIndex) & nVisibleChunksMask))
        {
            continue;
        }

        int posStride = 1;
        const byte* pPositions = GetPosPtr(posStride, FSL_READ);
        const vtx_idx* pIndices = GetIndexPtr(FSL_READ);
        const uint32 numVertices = GetVerticesCount();
        const uint32 indexStep = 3;
        uint32 numIndices = pChunk->nNumIndices;

        // Make sure number of indices is a multiple of 3.
        uint32 indexRemainder = numIndices % indexStep;
        if (indexRemainder != 0)
        {
            numIndices -= indexRemainder;
        }

        const uint32 firstIndex = pChunk->nFirstIndexId;
        const uint32 lastIndex = firstIndex + pChunk->nNumIndices;

        for (uint32 i = firstIndex; i < lastIndex; i += indexStep)
        {
            int32 I0 = pIndices[ i ];
            int32 I1 = pIndices[ i + 1 ];
            int32 I2 = pIndices[ i + 2 ];

            assert((uint32)I0 < numVertices);
            assert((uint32)I1 < numVertices);
            assert((uint32)I2 < numVertices);

            Vec3 v0, v1, v2;
            v0 = *(Vec3*)&pPositions[ posStride * I0 ];
            v1 = *(Vec3*)&pPositions[ posStride * I1 ];
            v2 = *(Vec3*)&pPositions[ posStride * I2 ];

            v0 = mat.TransformPoint(v0);
            v1 = mat.TransformPoint(v1);
            v2 = mat.TransformPoint(v2);

            if (bExtrude)
            {
                // Push vertices towards the camera to alleviate z-fighting.
                Vec3 cameraPosition = gEnv->pRenderer->GetCamera().GetPosition();
                const float VertexToCameraOffsetAmount = 0.02f;
                v0 = Lerp(v0, cameraPosition, VertexToCameraOffsetAmount);
                v1 = Lerp(v1, cameraPosition, VertexToCameraOffsetAmount);
                v2 = Lerp(v2, cameraPosition, VertexToCameraOffsetAmount);
            }

#ifdef WIN32
            s_vertexBuffer.push_back(v0);
            s_vertexBuffer.push_back(v1);
            s_vertexBuffer.push_back(v2);

            if (!bNoLines)
            {
                s_indexBuffer.push_back(currentIndexBase);
                s_indexBuffer.push_back(currentIndexBase + 1);
                s_indexBuffer.push_back(currentIndexBase + 1);
                s_indexBuffer.push_back(currentIndexBase + 2);
                s_indexBuffer.push_back(currentIndexBase + 2);
                s_indexBuffer.push_back(currentIndexBase);

                currentIndexBase += indexStep;
            }

            // Need to limit batch size, because the D3D aux geometry renderer
            // can only process 32768 vertices and 65536 indices
            const size_t vertexBufferSize = s_vertexBuffer.size();
            const bool bOverLimit = vertexBufferSize > kMaxBatchSize;
            const bool bLastTriangle = (i == (lastIndex - indexStep));

            if (bOverLimit || bLastTriangle)
            {
                // Draw all triangles of the chunk in one go for better batching
                pRenderAuxGeom->DrawTriangles(&s_vertexBuffer[0], s_vertexBuffer.size(), color);

                if (!bNoLines)
                {
                    const size_t indexBufferSize = s_indexBuffer.size();
                    pRenderAuxGeom->DrawLines(&s_vertexBuffer[0], vertexBufferSize, &s_indexBuffer[0], indexBufferSize, lineColor);

                    s_indexBuffer.resize(0);
                    currentIndexBase = 0;
                }

                s_vertexBuffer.resize(0);
            }
#else
            pRenderAuxGeom->DrawTriangle(v0, color, v1, color, v2, color);

            if (!bNoLines)
            {
                pRenderAuxGeom->DrawLine(v0, lineColor, v1, lineColor);
                pRenderAuxGeom->DrawLine(v1, lineColor, v2, lineColor);
                pRenderAuxGeom->DrawLine(v2, lineColor, v0, lineColor);
            }
#endif
        }
    }

    pRenderAuxGeom->SetRenderFlags(prevRenderFlags);
    UnLockForThreadAccess();
}


//===========================================================================================================
void CRenderMesh::PrintMeshLeaks()
{    
    AUTO_LOCK(m_sLinkLock);
    for (util::list<CRenderMesh>* iter = CRenderMesh::m_MeshList.next; iter != &CRenderMesh::m_MeshList; iter = iter->next)
    {
        CRenderMesh* pRM = iter->item<& CRenderMesh::m_Chain>();
        Warning("--- CRenderMesh '%s' leak after level unload", (!pRM->m_sSource.empty() ? pRM->m_sSource.c_str() : "NO_NAME"));
        __debugbreak();
    }
}

bool CRenderMesh::ClearStaleMemory(bool bLocked, int threadId)
{    
    FUNCTION_PROFILER(gEnv->pSystem, PROFILE_RENDERER);
    bool cleared = false;
    bool bKeepSystem = false;
    CConditionalLock lock(m_sLinkLock, !bLocked);
    // Clean up the stale mesh temporary data
    for (util::list<CRenderMesh>* iter = m_MeshDirtyList[threadId].next, * pos = iter->next; iter != &m_MeshDirtyList[threadId]; iter = pos, pos = pos->next)
    {
        CRenderMesh* pRM = iter->item<& CRenderMesh::m_Dirty>(threadId);
        if (pRM->m_sResLock.TryLock() == false)
        {
            continue;
        }
        // If the mesh data is still being read, skip it. The stale data will be picked up at a later point
        if (pRM->m_nThreadAccessCounter)
        {
#     if !defined(_RELEASE) && defined(RM_CATCH_EXCESSIVE_LOCKS)
            if (gEnv->pTimer->GetAsyncTime().GetSeconds() - pRM->m_lockTime > 32.f)
            {
                CryError("data lock for mesh '%s:%s' held longer than 32 seconds", (pRM->m_sType ? pRM->m_sType : "unknown"), (pRM->m_sSource ? pRM->m_sSource : "unknown"));
                if (CRenderer::CV_r_BreakOnError)
                {
                    __debugbreak();
                }
            }
#     endif
            goto dirty_done;
        }

        bKeepSystem = pRM->m_keepSysMesh;

        if (!bKeepSystem && pRM->m_pCachePos)
        {
            FreeMeshData(pRM->m_pCachePos);
            pRM->m_pCachePos = NULL;
            cleared = true;
        }

        // In DX11 we cannot lock device buffers efficiently from the MT,
        // so we have to keep system copy. On UMA systems we can clear the buffer
        // and access VRAM directly
        #if BUFFER_ENABLE_DIRECT_ACCESS && !defined(NULL_RENDERER)
        if (!bKeepSystem)
        {
            for (int i = 0; i < VSF_NUM; i++)
            {
                pRM->FreeVB(i);
            }
            pRM->FreeIB();
            cleared = true;
        }
        #endif

        // ToDo: only remove this mesh from the dirty list if no stream contains dirty data anymore
        pRM->m_Dirty[threadId].erase();
dirty_done:
        pRM->m_sResLock.Unlock();
    }

    return cleared;
}

void CRenderMesh::UpdateModifiedMeshes(bool bLocked, int threadId)
{    
    AZ_TRACE_METHOD();
    FUNCTION_PROFILER_LEGACYONLY(gEnv->pSystem, PROFILE_RENDERER);

    //
    // DX12 synchronizes render mesh updates at this point, because we can
    // batch multiple copies together and potentially forward them off
    // to a copy command list.
    //
#ifdef CRY_USE_DX12
    const bool bBlock = true;
#else
    const bool bBlock = gRenDev->m_pRT->m_eVideoThreadMode != SRenderThread::eVTM_Disabled;
#endif

    CConditionalLock lock(m_sLinkLock, !bLocked);
    // Update device buffers on modified meshes
    for (util::list<CRenderMesh>* iter = m_MeshModifiedList[threadId].next, * pos = iter->next; iter != &m_MeshModifiedList[threadId]; iter = pos, pos = pos->next)
    {
        CRenderMesh* pRM = iter->item<& CRenderMesh::m_Modified>(threadId);

#ifdef CRY_USE_DX12
        pRM->m_sResLock.Lock();
        const bool bDoUpdate = true;
#else
        if (pRM->m_sResLock.TryLock() == false)
        {
            continue;
        }
        const bool bDoUpdate = !pRM->m_nThreadAccessCounter;
#endif

        if (bDoUpdate)
        {
            if (pRM->SyncAsyncUpdate(gRenDev->m_RP.m_nProcessThreadID, bBlock) == true)
            {
                // ToDo :
                // - mark the mesh to not update itself if depending on how the async update was scheduled
                // - returns true if no streams need further processing
                if (pRM->RT_CheckUpdate(pRM, VSM_MASK, false, true))
                {
                    pRM->m_Modified[threadId].erase();
                }
            }
        }
        pRM->m_sResLock.Unlock();
    }
}

// Mesh garbage collector
void CRenderMesh::UpdateModified()
{    
    SRenderThread* pRT = gRenDev->m_pRT;
    ASSERT_IS_RENDER_THREAD(pRT);
    const int threadId = gRenDev->m_RP.m_nProcessThreadID;

    // Call the update and clear functions with bLocked == true even if the lock
    // was previously released in the above scope. The resasoning behind this is
    // that only the renderthread can access the below lists as they are double
    // buffered. Note: As the Lock/Unlock functions can come from the mainthread
    // and from any other thread, they still have guarded against contention!
    // The only exception to this is if we have no render thread, as there is no
    // double buffering in that case - so always lock.

    UpdateModifiedMeshes(pRT->IsMultithreaded(), threadId);
}


// Mesh garbage collector
void CRenderMesh::Tick()
{    
    ASSERT_IS_RENDER_THREAD(gRenDev->m_pRT)
    bool bKeepSystem = false;
    const threadID threadId = gRenDev->m_pRT->IsMultithreaded() ? gRenDev->m_RP.m_nProcessThreadID : threadID(1);
    int nFrame = gRenDev->m_RP.m_TI[gRenDev->m_RP.m_nProcessThreadID].m_nFrameUpdateID;

    // Remove deleted meshes from the list completely
    bool deleted = false;
    {
        AUTO_LOCK(m_sLinkLock);
        util::list<CRenderMesh>* garbage = &CRenderMesh::m_MeshGarbageList[nFrame & (MAX_RELEASED_MESH_FRAMES - 1)];
        while (garbage != garbage->prev)
        {
            CRenderMesh* pRM = garbage->next->item<& CRenderMesh::m_Chain>();
            SAFE_DELETE(pRM);
            deleted = true;
        }
    }
    // If an instance pool is used, try to reclaim any used pages if there are any
    if (deleted && s_MeshPool.m_MeshInstancePool)
    {
        s_MeshPool.m_MeshInstancePool->Cleanup();
    }

    // Call the clear functions with bLocked == true even if the lock
    // was previously released in the above scope. The resasoning behind this is
    // that only the renderthread can access the below lists as they are double
    // buffered. Note: As the Lock/Unlock functions can come from the mainthread
    // and from any other thread, they still have guarded against contention!

    ClearStaleMemory(true, threadId);
}

void CRenderMesh::Initialize()
{
    InitializePool();
}

void CRenderMesh::ShutDown()
{
    if IsCVarConstAccess(constexpr) (CRenderer::CV_r_releaseallresourcesonexit)
    {
        AUTO_LOCK(m_sLinkLock);
        while (&CRenderMesh::m_MeshList != CRenderMesh::m_MeshList.prev)
        {
            CRenderMesh* pRM = CRenderMesh::m_MeshList.next->item<& CRenderMesh::m_Chain>();
            PREFAST_ASSUME(pRM);
            if IsCVarConstAccess(constexpr) (CRenderer::CV_r_printmemoryleaks)
            {
                float fSize = pRM->Size(SIZE_ONLY_SYSTEM) / 1024.0f / 1024.0f;
                iLog->Log("Warning: CRenderMesh::ShutDown: RenderMesh leak %s: %0.3fMb", pRM->m_sSource.c_str(), fSize);
            }
            SAFE_RELEASE_FORCE(pRM);
        }
    }
    new (&CRenderMesh::m_MeshList) util::list<CRenderMesh>();
    new (&CRenderMesh::m_MeshGarbageList) util::list<CRenderMesh>();
    new (&CRenderMesh::m_MeshDirtyList) util::list<CRenderMesh>();
    new (&CRenderMesh::m_MeshModifiedList) util::list<CRenderMesh>();

    ShutdownPool();
}

//////////////////////////////////////////////////////////////////////////
void CRenderMesh::KeepSysMesh(bool keep)
{
    m_keepSysMesh = keep;
}

void CRenderMesh::UnKeepSysMesh()
{
    m_keepSysMesh = false;
}

//////////////////////////////////////////////////////////////////////////
void CRenderMesh::SetVertexContainer(IRenderMesh* pBuf)
{
    if (m_pVertexContainer)
    {
        ((CRenderMesh*)m_pVertexContainer)->m_lstVertexContainerUsers.Delete(this);
    }

    m_pVertexContainer = (CRenderMesh*)pBuf;

    if (m_pVertexContainer && ((CRenderMesh*)m_pVertexContainer)->m_lstVertexContainerUsers.Find(this) < 0)
    {
        ((CRenderMesh*)m_pVertexContainer)->m_lstVertexContainerUsers.Add(this);
    }
}

//////////////////////////////////////////////////////////////////////////
void CRenderMesh::AssignChunk(CRenderChunk* pChunk, CREMeshImpl* pRE)
{
    pRE->m_pChunk = pChunk;
    pRE->m_pRenderMesh = this;
    pRE->m_nFirstIndexId = pChunk->nFirstIndexId;
    pRE->m_nNumIndices = pChunk->nNumIndices;
    pRE->m_nFirstVertId = pChunk->nFirstVertId;
    pRE->m_nNumVerts = pChunk->nNumVerts;
}

//////////////////////////////////////////////////////////////////////////
void CRenderMesh::InitRenderChunk(CRenderChunk& rChunk)
{
    AZ_Assert(rChunk.nNumIndices > 0, "Render chunk must have > 0 indices");
    AZ_Assert(rChunk.nNumVerts > 0, "Render chunk must have > 0 vertices");

    if (!rChunk.pRE)
    {
        rChunk.pRE = (CREMeshImpl*) gRenDev->EF_CreateRE(eDATA_Mesh);
        rChunk.pRE->m_CustomTexBind[0] = m_nClientTextureBindID;
    }

    // update chunk RE
    if (rChunk.pRE)
    {
        AssignChunk(&rChunk, (CREMeshImpl*) rChunk.pRE);
    }
    AZ_Assert(rChunk.nFirstIndexId + rChunk.nNumIndices <= m_nInds, "First index of the chunk + number of indices for the chunk must be <= the total number of indices for the mesh.");
}

//////////////////////////////////////////////////////////////////////////
void CRenderMesh::SetRenderChunks(CRenderChunk* pInputChunksArray, int nCount, bool bSubObjectChunks)
{
    TRenderChunkArray* pChunksArray = &m_Chunks;
    if (bSubObjectChunks)
    {
        pChunksArray = &m_ChunksSubObjects;
    }

    ReleaseRenderChunks(pChunksArray);

    pChunksArray->resize(nCount);
    for (int i = 0; i < nCount; i++)
    {
        CRenderChunk& rChunk = (*pChunksArray)[i];
        rChunk = pInputChunksArray[i];
        InitRenderChunk(rChunk);
    }
}

//////////////////////////////////////////////////////////////////////////
void CRenderMesh::GarbageCollectSubsetRenderMeshes()
{
    uint32 nFrameID = GetCurrentRenderFrameID();
    m_nLastSubsetGCRenderFrameID = nFrameID;
    for (MeshSubSetIndices::iterator it = m_meshSubSetIndices.begin(); it != m_meshSubSetIndices.end(); )
    {
        IRenderMesh* pRM = it->second;
        if (abs((int)nFrameID - (int)((CRenderMesh*)pRM)->m_nLastRenderFrameID) > DELETE_SUBSET_MESHES_AFTER_NOTUSED_FRAMES)
        {
            // this mesh not relevant anymore.
            it = m_meshSubSetIndices.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

volatile int* CRenderMesh::SetAsyncUpdateState()
{
    SREC_AUTO_LOCK(m_sResLock);
    ASSERT_IS_MAIN_THREAD(gRenDev->m_pRT);
    int threadID = gRenDev->m_RP.m_nFillThreadID;
    if (m_asyncUpdateStateCounter[threadID] == 0)
    {
        m_asyncUpdateStateCounter[threadID] = 1;
        LockForThreadAccess();
    }
    CryInterlockedIncrement(&m_asyncUpdateState[threadID]);

    for (auto& chunk : m_Chunks)
    {
        if (chunk.pRE)
        {
            chunk.pRE->mfUpdateFlags(FCEF_DIRTY);
        }
    }

    return &m_asyncUpdateState[threadID];
}

bool CRenderMesh::SyncAsyncUpdate(int threadID, bool block)
{
    // If this mesh is being asynchronously prepared, wait for the job to finish prior to uploading
    // the vertices to vram.
    SREC_AUTO_LOCK(m_sResLock);
    if (m_asyncUpdateStateCounter[threadID])
    {
        {
            AZ_TRACE_METHOD();
            FRAME_PROFILER_LEGACYONLY("CRenderMesh::SyncAsyncUpdate() sync", gEnv->pSystem, PROFILE_RENDERER);
            int iter = 0;
            while (m_asyncUpdateState[threadID])
            {
                if (!block)
                {
                    return false;
                }
                CrySleep(iter > 10 ? 1 : 0);
                ++iter;
            }
        }
        UnlockStream(VSF_GENERAL);
        UnlockStream(VSF_TANGENTS);
        UnlockStream(VSF_VERTEX_VELOCITY);
#   if ENABLE_NORMALSTREAM_SUPPORT
        UnlockStream(VSF_NORMALS);
#   endif
        UnlockIndexStream();
        m_asyncUpdateStateCounter[threadID] = 0;
        UnLockForThreadAccess();
    }
    return true;
}

void CRenderMesh::CreateRemappedBoneIndicesPair(const uint pairGuid, const TRenderChunkArray& Chunks)
{
    SREC_AUTO_LOCK(m_sResLock);
    // check already created remapped bone indices
    for (size_t i = 0, end = m_RemappedBoneIndices.size(); i < end; ++i)
    {
        if (m_RemappedBoneIndices[i].guid == pairGuid && m_RemappedBoneIndices[i].refcount)
        {
            ++m_RemappedBoneIndices[i].refcount;
            return;
        }
    }

    // check already currently in flight remapped bone indices
    const int threadId = gRenDev->m_RP.m_nFillThreadID;
    for (size_t i = 0, end = m_CreatedBoneIndices[threadId].size(); i < end; ++i)
    {
        if (m_CreatedBoneIndices[threadId][i].guid == pairGuid)
        {
            ++m_CreatedBoneIndices[threadId][i].refcount;
            return;
        }
    }


    IRenderMesh::ThreadAccessLock access_lock(this);

    int32 stride;
    vtx_idx* indices = GetIndexPtr(FSL_READ);
    uint32 vtxCount = GetVerticesCount();
    std::vector<bool> touched(vtxCount, false);
    const SVF_W4B_I4S* pIndicesWeights = (SVF_W4B_I4S*)GetHWSkinPtr(stride, FSL_READ);
    SVF_W4B_I4S* remappedIndices = new SVF_W4B_I4S[vtxCount];

    for (size_t j = 0; j < Chunks.size(); ++j)
    {
        const CRenderChunk& chunk = Chunks[j];
        for (size_t k = chunk.nFirstIndexId; k < chunk.nFirstIndexId + chunk.nNumIndices; ++k)
        {
            const vtx_idx vIdx = indices[k];
            if (touched[vIdx])
            {
                continue;
            }
            touched[vIdx] = true;
#if 1
            for (int l = 0; l < 4; ++l)
            {
                remappedIndices[vIdx].weights.bcolor[l] = pIndicesWeights[vIdx].weights.bcolor[l];
                remappedIndices[vIdx].indices[l] = pIndicesWeights[vIdx].indices[l];
            }
#else
            // Sokov: The code below doesn't look really safe/logical:
            // 1) Calling CreateRemappedBoneIndicesPair() assumes that bones are used,
            // so why to check m_bUsesBones here? Do we have cases when some chunks use bones
            // and some others do not use bones?
            // 2) There is a risk, that some unaware code uses bones but forgets to
            // set m_bUsesBones to 'true' so it leads to killing all linking (see '= 0;').
            if (chunk.m_bUsesBones)
            {
                for (int l = 0; l < 4; ++l)
                {
                    remappedIndices[vIdx].weights.bcolor[l] = pIndicesWeights[vIdx].weights.bcolor[l];
                    remappedIndices[vIdx].indices[l] = pIndicesWeights[vIdx].indices[l];
                }
            }
            else
            {
                for (int l = 0; l < 4; ++l)
                {
                    remappedIndices[vIdx].weights.bcolor[l] = 0;
                    remappedIndices[vIdx].indices[l] = 0;
                }
            }
#endif
        }
    }
    UnlockStream(VSF_HWSKIN_INFO);
    UnlockIndexStream();

    m_CreatedBoneIndices[threadId].push_back(SBoneIndexStreamRequest(pairGuid, remappedIndices));
    RelinkTail(m_Modified[threadId], m_MeshModifiedList[threadId], threadId);
}


void CRenderMesh::CreateRemappedBoneIndicesPair(const DynArray<JointIdType>& arrRemapTable, const uint pairGuid)
{
    SREC_AUTO_LOCK(m_sResLock);
    // check already created remapped bone indices
    for (size_t i = 0, end = m_RemappedBoneIndices.size(); i < end; ++i)
    {
        if (m_RemappedBoneIndices[i].guid == pairGuid && m_RemappedBoneIndices[i].refcount)
        {
            ++m_RemappedBoneIndices[i].refcount;
            return;
        }
    }

    // check already currently in flight remapped bone indices
    const int threadId = gRenDev->m_RP.m_nFillThreadID;
    for (size_t i = 0, end = m_CreatedBoneIndices[threadId].size(); i < end; ++i)
    {
        if (m_CreatedBoneIndices[threadId][i].guid == pairGuid)
        {
            ++m_CreatedBoneIndices[threadId][i].refcount;
            return;
        }
    }

    IRenderMesh::ThreadAccessLock access_lock(this);

    int32 stride;
    vtx_idx* const indices = GetIndexPtr(FSL_READ);
    uint32 vtxCount = GetVerticesCount();
    std::vector<bool> touched(vtxCount, false);
    const SVF_W4B_I4S* pIndicesWeights = (const SVF_W4B_I4S*)GetHWSkinPtr(stride, FSL_READ);
    SVF_W4B_I4S* remappedIndices = new SVF_W4B_I4S[vtxCount];

    for (size_t j = 0; j < m_Chunks.size(); ++j)
    {
        const CRenderChunk& chunk = m_Chunks[j];
        for (size_t k = chunk.nFirstIndexId; k < chunk.nFirstIndexId + chunk.nNumIndices; ++k)
        {
            const vtx_idx vIdx = indices[k];
            if (touched[vIdx])
            {
                continue;
            }
            touched[vIdx] = true;
#if 1
            for (int l = 0; l < 4; ++l)
            {
                remappedIndices[vIdx].weights.bcolor[l] = pIndicesWeights[vIdx].weights.bcolor[l];
                remappedIndices[vIdx].indices[l] = arrRemapTable[pIndicesWeights[vIdx].indices[l]];
            }
#else
            // Sokov: The code below doesn't look really safe/logical:
            // 1) Calling CreateRemappedBoneIndicesPair() assumes that bones are used,
            // so why to check m_bUsesBones here? Do we have cases when some chunks use bones
            // and some others do not use bones?
            // 2) There is a risk, that some unaware code uses bones but forgets to
            // set m_bUsesBones to 'true' so it leads to killing all linking (see '= 0;').
            if (chunk.m_bUsesBones)
            {
                for (int l = 0; l < 4; ++l)
                {
                    remappedIndices[vIdx].weights.bcolor[l] = pIndicesWeights[vIdx].weights.bcolor[l];
                    remappedIndices[vIdx].indices[l] = arrRemapTable[pIndicesWeights[vIdx].indices[l]];
                }
            }
            else
            {
                for (int l = 0; l < 4; ++l)
                {
                    remappedIndices[vIdx].weights.bcolor[l] = 0;
                    remappedIndices[vIdx].indices[l] = 0;
                }
            }
#endif
        }
    }
    UnlockStream(VSF_HWSKIN_INFO);
    UnlockIndexStream();

    m_CreatedBoneIndices[threadId].push_back(SBoneIndexStreamRequest(pairGuid, remappedIndices));
    RelinkTail(m_Modified[threadId], m_MeshModifiedList[threadId], threadId);
}

void CRenderMesh::ReleaseRemappedBoneIndicesPair(const uint pairGuid)
{
    if (gRenDev->m_pRT->IsMultithreaded() && gRenDev->m_pRT->IsMainThread(true))
    {
        gRenDev->m_pRT->RC_ReleaseRemappedBoneIndices(this, pairGuid);
        return;
    }

    SREC_AUTO_LOCK(m_sResLock);
    size_t deleted = ~0u;
    const int threadId = gRenDev->m_RP.m_nProcessThreadID;
    bool bFound = false;

    for (size_t i = 0, end = m_RemappedBoneIndices.size(); i < end; ++i)
    {
        if (m_RemappedBoneIndices[i].guid == pairGuid)
        {
            bFound = true;
            if (--m_RemappedBoneIndices[i].refcount == 0)
            {
                deleted = i;
                break;
            }
        }
    }

    if (deleted != ~0u)
    {
        m_DeletedBoneIndices[threadId].push_back(pairGuid);
        RelinkTail(m_Modified[threadId], m_MeshModifiedList[threadId], threadId);
    }

    // Check for created but not yet remapped bone indices
    if (!bFound)
    {
        for (size_t i = 0, end = m_CreatedBoneIndices[threadId].size(); i < end; ++i)
        {
            if (m_CreatedBoneIndices[threadId][i].guid == pairGuid)
            {
                bFound = true;
                if (--m_CreatedBoneIndices[threadId][i].refcount == 0)
                {
                    deleted = i;
                    break;
                }
            }
        }

        if (deleted != ~0u)
        {
            m_DeletedBoneIndices[threadId].push_back(pairGuid);
            RelinkTail(m_Modified[threadId], m_MeshModifiedList[threadId], threadId);
        }
    }
}
// Notes: Cry's LockForThreadAccess is not locking anything here, it only increments m_nThreadAccessCounter but m_nThreadAccessCounter is not used to block multithreading access to CRenderMesh
// This is dangerous as the name of function suggests a lock but it actually doesn't lock. There might be quite a few misuse of this function and as the result the data is not protected in multithread.
// The system needs a deeper overhauling later. 
void CRenderMesh::LockForThreadAccess()
{
    ++m_nThreadAccessCounter;

# if !defined(_RELEASE) && defined(RM_CATCH_EXCESSIVE_LOCKS)
    m_lockTime = (m_lockTime > 0.f) ? m_lockTime : gEnv->pTimer->GetAsyncTime().GetSeconds();
# endif
}
void CRenderMesh::UnLockForThreadAccess()
{
    --m_nThreadAccessCounter;
    if (m_nThreadAccessCounter < 0)
    {
        __debugbreak(); // if this triggers, a mismatch betweend rendermesh thread access lock/unlock has occured
    }
# if !defined(_RELEASE) && defined(RM_CATCH_EXCESSIVE_LOCKS)
    m_lockTime = 0.f;
# endif
}
void CRenderMesh::GetPoolStats(SMeshPoolStatistics* stats)
{
    memcpy(stats, &s_MeshPool.m_MeshDataPoolStats, sizeof(SMeshPoolStatistics));
}

void* CRenderMesh::operator new(size_t size)
{
    return AllocateMeshInstanceData(size, alignof(CRenderMesh));
}

void CRenderMesh::operator delete(void* ptr)
{
    FreeMeshInstanceData(ptr);
}

#if !defined(NULL_RENDERER)
D3DBuffer* CRenderMesh::_GetD3DVB(int nStream, size_t* offs) const
{
    if (SMeshStream* pMS = GetVertexStream(nStream))
    {
        IF (pMS->m_nID != ~0u, 1)
        {
            return gRenDev->m_DevBufMan.GetD3D(pMS->m_nID, offs);
        }
    }
    return NULL;
}

D3DBuffer* CRenderMesh::_GetD3DIB(size_t* offs) const
{
    IF (m_IBStream.m_nID != ~0u, 1)
    {
        return gRenDev->m_DevBufMan.GetD3D(m_IBStream.m_nID, offs);
    }
    return NULL;
}
#endif


void CRenderMesh::BindStreamsToRenderPipeline()
{
#if !defined(NULL_RENDERER)
    CRenderMesh *pRenderMesh = this;

    HRESULT h;
    CD3D9Renderer *rd = (CD3D9Renderer*)gRenDev;
    CRenderMesh *pRM = pRenderMesh->_GetVertexContainer();
    void* pIB = NULL;
    size_t nOffs = 0;

    size_t streamStride[VSF_NUM] = { 0 };
    size_t offset[VSF_NUM] = { 0 };
    const void *pVB[VSF_NUM] = { NULL };

    pIB = rd->m_DevBufMan.GetD3D(pRenderMesh->GetIBStream(), &nOffs);
    pVB[0] = rd->m_DevBufMan.GetD3D(pRM->GetVBStream(VSF_GENERAL), &offset[0]);
    h = rd->FX_SetVStream(0, pVB[0], offset[0], pRM->GetStreamStride(VSF_GENERAL));

    for (int i = 1, mask = 1 << 1; i < VSF_NUM; ++i, mask <<= 1)
    {
        if (rd->m_RP.m_FlagsStreams_Stream & (mask) && pRM->_HasVBStream(i))
        {
            streamStride[i] = pRM->GetStreamStride(i);
            pVB[i] = rd->m_DevBufMan.GetD3D(pRM->GetVBStream(i), &offset[i]);
        }
    }

    for (int i = 1, mask = 1 << 1; i < VSF_NUM; ++i, mask <<= 1)
    {
        if (rd->m_RP.m_FlagsStreams_Stream & (mask) && pVB[i])
        {
            rd->m_RP.m_PersFlags1 |= RBPF1_USESTREAM << i;
            h = rd->FX_SetVStream(i, pVB[i], offset[i], streamStride[i]);
        }
        else if (rd->m_RP.m_PersFlags1 & (RBPF1_USESTREAM << i))
        {
            rd->m_RP.m_PersFlags1 &= ~(RBPF1_USESTREAM << i);
            h = rd->FX_SetVStream(i, NULL, 0, 0);
        }
    }

#ifdef MESH_TESSELLATION_RENDERER
    if (CHWShader_D3D::s_pCurInstHS && pRenderMesh->m_adjBuffer.GetShaderResourceView())
    {
        // This buffer contains texture coordinates for triangle itself and its neighbors, in total 12 texture coordinates per triangle.
        D3DShaderResourceView* SRVs[] = { pRenderMesh->m_adjBuffer.GetShaderResourceView() };
        gcpRendD3D->GetDeviceContext().DSSetShaderResources(15, 1, SRVs);
    }
#endif

    // 8 weights skinning. setting the last 4 weights
    if (pRenderMesh->m_extraBonesBuffer.GetShaderResourceView())
    {
        D3DShaderResourceView* SRVs[] = { pRenderMesh->m_extraBonesBuffer.GetShaderResourceView() };
        gcpRendD3D->GetDeviceContext().VSSetShaderResources(14, 1, SRVs);
    }

    assert(pIB);
    h = rd->FX_SetIStream(pIB, nOffs, (sizeof(vtx_idx) == 2 ? Index16 : Index32));

#endif //if !defined(NULL_RENDERER)
}

bool CRenderMesh::GetRemappedSkinningData([[maybe_unused]] uint32 guid, [[maybe_unused]] CRendElementBase::SGeometryStreamInfo &streamInfo)
{
#if !defined(NULL_RENDERER)
    CD3D9Renderer *rd = gcpRendD3D;
    size_t offset = 0;
    const void *pVB = 0;

    for (size_t i = 0, end = m_RemappedBoneIndices.size(); i < end; ++i)
    {
        CRenderMesh::SBoneIndexStream &stream = m_RemappedBoneIndices[i];
        if (stream.guid != guid)
            continue;
        if (stream.buffer != ~0u)
        {
            pVB = gRenDev->m_DevBufMan.GetD3D(stream.buffer, &offset);
            streamInfo.nOffset = offset;
            streamInfo.nStride = GetStreamStride(VSF_HWSKIN_INFO);
            streamInfo.pStream = pVB;
            return true;
        }
    }
#endif //if !defined(NULL_RENDERER)
    return m_RemappedBoneIndices.size() == 0u;
}

bool CRenderMesh::FillGeometryInfo([[maybe_unused]] CRendElementBase::SGeometryInfo &geom)
{
#if !defined(NULL_RENDERER)

    CRenderMesh *pRenderMeshForVertices = _GetVertexContainer();
    size_t nOffs = 0;

    if (!_HasIBStream())
        return false;

    if (!pRenderMeshForVertices->CanRender())
        return false;

    geom.indexStream.pStream = gRenDev->m_DevBufMan.GetD3D(GetIBStream(), &nOffs);
    geom.indexStream.nOffset = nOffs;
    geom.indexStream.nStride = (sizeof(vtx_idx) == 2 ? Index16 : Index32);
    geom.streamMask = 0;
    geom.nMaxVertexStreams = 0;
    for (int nStream = 0; nStream < VSF_NUM; ++nStream)
    {
        if (pRenderMeshForVertices->_HasVBStream(nStream))
        {
            nOffs = 0;
            geom.vertexStream[nStream].pStream = gRenDev->m_DevBufMan.GetD3D(pRenderMeshForVertices->GetVBStream(nStream), &nOffs);
            geom.vertexStream[nStream].nOffset = nOffs;
            geom.vertexStream[nStream].nStride = pRenderMeshForVertices->GetStreamStride(nStream);

            if (geom.vertexStream[nStream].pStream)
            {
                geom.nMaxVertexStreams = nStream + 1;
            }

            //geom.streamMask |= 1 << nStream;
        }
        else
        {
            geom.vertexStream[nStream].pStream = 0;
            geom.vertexStream[nStream].nOffset = 0;
            geom.vertexStream[nStream].nStride = 0;
        }
    }

    if (m_RemappedBoneIndices.size() > 0)
    {
        if (GetRemappedSkinningData(geom.bonesRemapGUID, geom.vertexStream[VSF_HWSKIN_INFO]))
        {
            if (geom.nMaxVertexStreams <= VSF_HWSKIN_INFO)
            {
                geom.nMaxVertexStreams = VSF_HWSKIN_INFO + 1;
            }
        }
    }

    geom.pSkinningExtraBonesBuffer = &m_extraBonesBuffer;

#ifdef MESH_TESSELLATION_RENDERER
    geom.pTessellationAdjacencyBuffer = &m_adjBuffer;
#else
    geom.pTessellationAdjacencyBuffer = nullptr;
#endif

#endif //if !defined(NULL_RENDERER)

    return true;
}

CThreadSafeRendererContainer<CRenderMesh*> CRenderMesh::m_deferredSubsetGarbageCollection[RT_COMMAND_BUF_COUNT];
CThreadSafeRendererContainer<SMeshSubSetIndicesJobEntry> CRenderMesh::m_meshSubSetRenderMeshJobs[RT_COMMAND_BUF_COUNT];


#if RENDERMESH_CPP_TRAIT_BUFFER_ENABLE_DIRECT_ACCESS || defined(CRY_USE_DX12)
    #ifdef BUFFER_ENABLE_DIRECT_ACCESS
        #undef BUFFER_ENABLE_DIRECT_ACCESS
        #define BUFFER_ENABLE_DIRECT_ACCESS 1
    #endif
#endif

