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

#pragma once

#if defined(AZ_RESTRICTED_PLATFORM)
    #include AZ_RESTRICTED_FILE(D3DHWShader_h)
#endif

#define MERGE_SHADER_PARAMETERS 1

// Streams redefinitions (TEXCOORD#)
#define VSTR_COLOR1        4  //Base Stream
#define VSTR_NORMAL1       5  //Base Stream
#define VSTR_PSIZE         6  //Base particles stream
//#define VSTR_MORPHTARGETDELTA 7 // MorphTarget stream (VSF_HWSKIN_MORPHTARGET_INFO)
#define VSTR_TANGENT       8  // Tangents stream (VSF_TANGENTS)
#define VSTR_BITANGENT     9  // Tangents stream (VSF_TANGENTS) -> unused, remove
#define VSTR_BLENDWEIGHTS 10  // HWSkin stream (VSF_HWSKIN_INFO)
#define VSTR_BLENDINDICES 11  // HWSkin stream (VSF_HWSKIN_INFO)
#define VSTR_BONESPACE    12  // HWSkin stream (VSF_HWSKIN_INFO)

#define INST_PARAM_SIZE sizeof(Vec4)

#define MAX_PF_TEXTURES     (32)
#define MAX_PF_SAMPLERS     (4)

#if defined(_CPU_SSE)
typedef __m128 VECTOR_TYPE;
#define VECTOR_CONST(x, y, z, w)  { x, y, z, w }   //{ w, z, y, x }
#define VECTOR_ZERO()                               _mm_setzero_ps()
#define VECTOR_XOR(a, b)                  _mm_xor_ps(a, b)
#else
union USoftVector
{
    struct
    {
        float x, y, z, w;
    };
    uint32 u[4];
};

#if defined(LINUX32)
typedef DEFINE_ALIGNED_DATA (USoftVector, VECTOR_TYPE, 8);
#else
typedef DEFINE_ALIGNED_DATA (USoftVector, VECTOR_TYPE, 16);
#endif

#define VECTOR_CONST(x, y, z, w)  { \
        { x, y, z, w }              \
}
const VECTOR_TYPE   g_VectorZero = VECTOR_CONST(0.f, 0.f, 0.f, 0.f);
#define VECTOR_ZERO()                               (g_VectorZero)
inline VECTOR_TYPE VECTOR_XOR(const VECTOR_TYPE& a, const VECTOR_TYPE& b)
{
    VECTOR_TYPE r;
    r.u[0] = a.u[0] ^ b.u[0];
    r.u[1] = a.u[1] ^ b.u[1];
    r.u[2] = a.u[2] ^ b.u[2];
    r.u[3] = a.u[3] ^ b.u[3];
    return r;
}
#endif

union UFloat4
{
    float f[4];
    VECTOR_TYPE m128;
};

namespace AzRHI{ class ConstantBuffer; }

//==============================================================================

int D3DXGetSHParamHandle(void* pSH, SCGBind* pParam);

struct SParamsGroup
{
    std::vector<SCGParam> Params[2];
    std::vector<SCGParam> Params_Inst;
};

enum ED3DShError
{
    ED3DShError_NotCompiled,
    ED3DShError_CompilingError,
    ED3DShError_Fake,
    ED3DShError_Ok,
    ED3DShError_Compiling,
};

//====================================================================================

struct SCGParamsGroup
{
    uint16 nParams;
    uint16 nSamplers;
    uint16 nTextures;
    SCGParam* pParams;
    int nPool;
    int nRefCounter;
    SCGParamsGroup()
    {
        nParams = 0;
        nPool = 0;
        pParams = NULL;
        nRefCounter = 1;
    }
    unsigned Size() { return sizeof(*this); }
    void GetMemoryUsage([[maybe_unused]] ICrySizer* pSizer) const {}
};

#define PARAMS_POOL_SIZE 256

struct alloc_info_struct
{
    int ptr;
    int bytes_num;
    bool busy;
    const char* szSource;
    unsigned Size() { return sizeof(*this); }

    void GetMemoryUsage([[maybe_unused]] ICrySizer* pSizer) const {}
};

struct SCGParamPool
{
protected:
    PodArray<alloc_info_struct> m_alloc_info;
    Array<SCGParam> m_Params;

public:

    SCGParamPool(int nEntries = 0);
    ~SCGParamPool();
    SCGParamsGroup Alloc(int nEntries);
    bool Free(SCGParamsGroup& Group);

    size_t Size()
    {
        return sizeof(*this) + sizeOfV(m_alloc_info) + m_Params.size_mem();
    }
    void GetMemoryUsage(ICrySizer* pSizer) const
    {
        pSizer->AddObject(m_alloc_info);
        pSizer->Add(m_Params.begin(), m_Params.size());
    }
};

template<>
inline bool raw_movable(const SCGParamPool&)
{
    return true;
}

class CGParamManager
{
    friend class CHWShader_D3D;
    //friend struct CHWShader_D3D::SHWSInstance;

    static AZStd::vector<uint32, AZ::StdLegacyAllocator> s_FreeGroups;

public:
    static void Init();
    static void Shutdown();

    static SCGParamPool* NewPool(int nEntries);
    static int GetParametersGroup(SParamsGroup& Gr, int nId);
    static bool FreeParametersGroup(int nID);

    static AZStd::vector<SCGParamsGroup, AZ::StdLegacyAllocator> s_Groups;
    static DynArray<SCGParamPool> s_Pools;
};

//=========================================================================================

struct SD3DShader
{
    int m_nRef;
    void* m_pHandle;
    bool m_bBound;

    SD3DShader()
    {
        m_nRef = 1;
        m_pHandle = NULL;
        m_bBound = false;
    }
    int AddRef()
    {
        return m_nRef++;
    }
    int Release(EHWShaderClass eSHClass, int nSize);

    void GetMemoryUsage(ICrySizer* pSizer) const
    {
        pSizer->AddObject(this, sizeof(*this));
    }
};

struct SD3DShaderHandle
{
    SD3DShader* m_pShader;
    byte* m_pData;
    int m_nData;
    byte m_bStatus;
    SD3DShaderHandle()
    {
        m_pShader = NULL;
        m_bStatus = 0;
        m_nData = 0;
        m_pData = NULL;
    }
    void SetShader(SD3DShader* pShader)
    {
        m_bStatus = 0;
        m_pShader = pShader;
    }
    void SetFake()
    {
        m_bStatus = 2;
    }
    void SetNonCompilable()
    {
        m_bStatus = 1;
    }
    int AddRef()
    {
        if (!m_pShader)
        {
            return 0;
        }
        return m_pShader->AddRef();
    }
    int Release(EHWShaderClass eSHClass, int nSize)
    {
        if (!m_pShader)
        {
            return 0;
        }
        return m_pShader->Release(eSHClass, nSize);
    }

    void GetMemoryUsage(ICrySizer* pSizer) const
    {
        pSizer->AddObject(m_pShader);
    }
};

struct SShaderAsyncInfo
{
    SShaderAsyncInfo* m_Next;         //!<
    SShaderAsyncInfo* m_Prev;         //!<
    inline void Unlink()
    {
        if (!m_Next || !m_Prev)
        {
            return;
        }
        m_Next->m_Prev = m_Prev;
        m_Prev->m_Next = m_Next;
        m_Next = m_Prev = NULL;
    }
    inline void Link(SShaderAsyncInfo* Before)
    {
        if (m_Next || m_Prev)
        {
            return;
        }
        m_Next = Before->m_Next;
        Before->m_Next->m_Prev = this;
        Before->m_Next = this;
        m_Prev = Before;
    }
    static void FlushPendingShaders();

#if D3DHWSHADER_H_TRAIT_DEFINE_D3D_BLOB || defined(OPENGL)
#define LPD3DXBUFFER D3DBlob *
#define ID3DXBuffer D3DBlob
#define D3D10CreateBlob D3DCreateBlob
#endif

    int m_nHashInst;
    uint64 m_RTMask;
    uint32 m_LightMask;
    uint32 m_MDMask;
    uint32 m_MDVMask;
    EHWShaderClass m_eClass;
    UPipelineState m_pipelineState;
    class CHWShader_D3D* m_pShader;
    CShader* m_pFXShader;

    LPD3D10BLOB m_pDevShader;
    LPD3D10BLOB m_pErrors;
    void* m_pConstants;
    string m_Name;
    string m_Text;
    string m_Errors;
    string m_Profile;
    string m_RequestLine;
    string m_shaderList;
    //CShaderThread *m_pThread;
    AZStd::vector<SCGBind, AZ::StdLegacyAllocator> m_InstBindVars;
    byte m_bPending;
    bool m_bPendedFlush;
    bool m_bPendedSamplers;
    bool m_bPendedEnv;
    bool m_bDeleteAfterRequest;
    float m_fMinDistance;
    int m_nFrame;
    int m_nThread;
    int m_nCombination;

    SShaderAsyncInfo()
        : m_Next(nullptr)
        , m_Prev(nullptr)
        , m_nHashInst(-1)
        , m_RTMask(0)
        , m_LightMask(0)
        , m_MDMask(0)
        , m_MDVMask(0)
        , m_eClass(eHWSC_Num)
        , m_pShader(nullptr)
        , m_pFXShader(nullptr)
        , m_pDevShader(nullptr)
        , m_pErrors(nullptr)
        , m_pConstants(nullptr)
        , m_bPending(true) // this flag is now used as an atomic indication that if the async shader has been compiled
        , m_bPendedFlush(false)
        , m_bPendedSamplers(false)
        , m_bPendedEnv(false)
        , m_bDeleteAfterRequest(false)
        , m_fMinDistance(0.0f)
        , m_nFrame(0)
        , m_nThread(0)
        , m_nCombination(-1)
    {
    }

    ~SShaderAsyncInfo();
    static volatile int s_nPendingAsyncShaders;
    static SShaderAsyncInfo& PendingList();
    static SShaderAsyncInfo& PendingListT();
    static CryEvent s_RequestEv;
};

#ifdef SHADER_ASYNC_COMPILATION

#include "CryThread.h"
#define SHADER_THREAD_NAME "ShaderCompile"

class CAsyncShaderTask
{
    friend class CD3D9Renderer; // so it can instantiate us

public:
    CAsyncShaderTask();

    static void InsertPendingShader(SShaderAsyncInfo* pAsync);
    int GetThread()
    {
        return m_nThread;
    }
    int GetThreadFXC()
    {
        return m_nThreadFXC;
    }
    void SetThread(int nThread)
    {
        m_nThread = nThread;
        m_nThreadFXC = -1;
    }
    void SetThreadFXC(int nThread)
    {
        m_nThreadFXC = nThread;
    }

private:

    void FlushPendingShaders();

    static SShaderAsyncInfo& BuildList();
    SShaderAsyncInfo m_flush_list;
    int m_nThread;
    int m_nThreadFXC;

    class CShaderThread
        : public CrySimpleThread<>
    {
    public:
        CShaderThread(CAsyncShaderTask* task)
            : m_task(task)
            , m_quit(false)
        {
            CAsyncShaderTask::BuildList().m_Next = &CAsyncShaderTask::BuildList();
            CAsyncShaderTask::BuildList().m_Prev = &CAsyncShaderTask::BuildList();

            task->m_flush_list.m_Next = &task->m_flush_list;
            task->m_flush_list.m_Prev = &task->m_flush_list;
            Start();
        }

        ~CShaderThread()
        {
            m_quit = true;
            Stop();
            WaitForThread();
        }

    private:
        void Run();

        CAsyncShaderTask* m_task;
        volatile bool m_quit;
    };

    CShaderThread m_thread;

    bool CompileAsyncShader(SShaderAsyncInfo* pAsync);
    void SubmitAsyncRequestLine(SShaderAsyncInfo* pAsync);
    bool PostCompile(SShaderAsyncInfo* pAsync);
};
#endif

class CHWShader_D3D
    : public CHWShader
{
    friend class CD3D9Renderer;
    friend class CAsyncShaderTask;
    friend class CGParamManager;
    friend struct SShaderAsyncInfo;
    friend class CHWShader;
    friend class CShaderMan;
    friend struct InstContainerByHash;
    friend class CREGeomCache;
    friend struct SRenderStatePassD3D;
    friend struct SDeviceObjectHelpers;
    friend class CDeviceGraphicsCommandList;
    friend class CDeviceGraphicsPSO;
    friend class CDeviceGraphicsPSO_DX11;
    friend class CDeviceGraphicsPSO_DX12;
    friend class CGnmGraphicsPipelineState;
    friend class CSceneGBufferPass;  // HACK

    SShaderDevCache* m_pDevCache;

#if !defined(_RELEASE)
    static AZStd::unordered_set<uint32_t, AZStd::hash<uint32_t>, AZStd::equal_to<uint32_t>, AZ::StdLegacyAllocator> s_ErrorsLogged; // CRC32 of message, risk of collision acceptable
#endif

#if !defined(CONSOLE)
    SPreprocessTree* m_pTree;
    CParserBin* m_pParser;
#endif

    struct SHWSInstance
    {
        friend struct SShaderAsyncInfo;

        SShaderCombIdent m_Ident;

        int m_nContIndex;

        SD3DShaderHandle m_Handle;
        EHWShaderClass m_eClass;

        int m_nParams[2]; // 0: Instance independent; 1: Instance depended

        // Shader-specific data (unlike the static per frame which are shared)
        std::vector<STexSamplerRT> m_pSamplers;
        std::vector<SCGSampler> m_Samplers;
        std::vector<SCGTexture> m_Textures;     
        std::vector<SCGBind> m_pBindVars;

        int m_nParams_Inst;
        float m_fLastAccess;
        int m_nUsed;
        int m_nUsedFrame;
        int m_nFrameSubmit;
        void* m_pShaderData;
        int m_nMaxVecs[3];
        short m_nInstMatrixID;
        short m_nInstIndex;
        short m_nInstructions;
        short m_nTempRegs;
        uint16 m_VStreamMask_Stream;
        uint16 m_VStreamMask_Decl;
        short m_nCache;
        short m_nParent;
        byte m_bDeleted : 1;
        byte m_bHasPMParams : 1;
        byte m_bFallback : 1;
        byte m_bCompressed : 1;
        byte m_bAsyncActivating : 1;
        byte m_bHasSendRequest : 1;
        AZ::Vertex::Format m_vertexFormat;
        byte m_nNumInstAttributes;

        int m_nDataSize;
        int m_DeviceObjectID;
        AZ::u32 m_uniqueNameCRC = 0; // CRC generated from the pattern: "Technique@EntryPoint(Permutations)"
#if defined(FEATURE_PER_SHADER_INPUT_LAYOUT_CACHE)
        enum
        {
            NUM_CACHE = 2
        };
        ID3D11InputLayout* m_pInputLayout[NUM_CACHE];
        uint64 m_cacheID[NUM_CACHE];
#endif
        SShaderAsyncInfo* m_pAsync;

        SHWSInstance()
            : m_Ident()
            , m_nContIndex(0)
            , m_Handle()
            , m_eClass(eHWSC_Num)
            , m_nParams_Inst(-1)
            , m_fLastAccess(0.0f)
            , m_nUsed(0)
            , m_nUsedFrame(0)
            , m_nFrameSubmit(0)
            , m_pShaderData(nullptr)
            , m_nInstMatrixID(1)
            , m_nInstIndex(-1)
            , m_nInstructions(0)
            , m_nTempRegs(0)
            , m_VStreamMask_Stream(0)
            , m_VStreamMask_Decl(0)
            , m_nCache(-1)
            , m_nParent(-1)
            , m_bDeleted(false)
            , m_bHasPMParams(false)
            , m_bFallback(false)
            , m_bCompressed(false)
            , m_bAsyncActivating(false)
            , m_bHasSendRequest(false)
            , m_vertexFormat(eVF_P3F_C4B_T2F)
            , m_nNumInstAttributes(0)
            , m_nDataSize(0)
            , m_DeviceObjectID(-1)
            , m_pAsync(nullptr)
        {
            m_nParams[0] = -1;
            m_nParams[1] = -1;
            m_nMaxVecs[0] = 0;
            m_nMaxVecs[1] = 0;
            m_nMaxVecs[2] = 0;

#if defined(FEATURE_PER_SHADER_INPUT_LAYOUT_CACHE)
            for (int i = 0; i < NUM_CACHE; ++i)
            {
                m_pInputLayout[i] = nullptr;
                m_cacheID[i] = 0;
            }
#endif
        }

#if defined(FEATURE_PER_SHADER_INPUT_LAYOUT_CACHE)
        ~SHWSInstance()
        {
            for (uint i = 0; i < NUM_CACHE; ++i)
            {
                SAFE_RELEASE(m_pInputLayout[i]);
            }
        }
        ID3D11InputLayout* GetCachedInputLayout(uint64 cacheID)
        {
            for (uint i = 0; i < NUM_CACHE; ++i)
            {
                if (cacheID == m_cacheID[i])
                {
                    return m_pInputLayout[i];
                }
            }
            return NULL;
        }
        void SetCachedInputLayout(ID3D11InputLayout* pInputLayout, uint64 cacheID)
        {
            // First try empy slot
            for (uint i = 0; i < NUM_CACHE; ++i)
            {
                if (m_pInputLayout[i] == NULL)
                {
                    m_pInputLayout[i] = pInputLayout;
                    m_cacheID[i]            = cacheID;
                    return;
                }
            }
            // Cache is full - look for matching InputLayout
            for (uint i = 0; i < NUM_CACHE; ++i)
            {
                if (m_pInputLayout[i] == pInputLayout)
                {
                    m_cacheID[i] = cacheID;
                    return;
                }
            }
            // Replace first entry (cache is thrashing)
            SAFE_RELEASE(m_pInputLayout[0]);
            m_pInputLayout[0] = pInputLayout;
            m_cacheID[0]            = cacheID;
        }
#endif
        void Release(SShaderDevCache* pCache = NULL, bool bReleaseData = true);
        void GetInstancingAttribInfo(uint8 Attributes[32], int32 & nUsedAttr, int& nInstAttrMask);

        int Size()
        {
            int nSize = sizeof(*this);
            nSize += sizeOfV(m_pSamplers);
            nSize += sizeOfV(m_pBindVars);

            return nSize;
        }

        void GetMemoryUsage(ICrySizer* pSizer) const
        {
            pSizer->AddObject(m_Handle);
            pSizer->AddObject(m_pSamplers);
            pSizer->AddObject(m_pBindVars);
            pSizer->AddObject(m_pShaderData, m_nDataSize);
        }

        bool IsAsyncCompiling()
        {
            if (m_pAsync || m_bAsyncActivating)
            {
                return true;
            }

            return false;
        }
        
        // CreateInputLayout in D3D11 generates a unique resource depending on the vertex format and the compiled vertex shader.
        // If an associated vertex shader does not reference certain vertexFormat inputs, then CreateInputLayout can return
        // a result that will not work for other vertexFormat + vertex shader combinations that do reference the previously unreferenced input.
        // On some platforms we must generate a key for SOnDemandD3DVertexDeclarationCache that is based both on the vertex shader binary and the vertex format layout.
        AZ::u32 GenerateVertexDeclarationCacheKey(const AZ::Vertex::Format& vertexFormat);
    };

    typedef std::vector<SHWSInstance*> InstContainer;
    typedef InstContainer::iterator InstContainerIt;

    using SCGTextures = AZStd::fixed_vector<SCGTexture, MAX_PF_TEXTURES>;
    using SCGSamplers = AZStd::fixed_vector<STexSamplerRT, MAX_PF_SAMPLERS>;

    typedef std::multimap<uint64, uint32> THWInstanceLookupMap;

    THWInstanceLookupMap m_LookupMap;
    bool m_bUseLookUpTable;

public:

    SHWSInstance* m_pCurInst;
    InstContainer m_Insts;

    static int m_FrameObj;
    int m_nCurInstFrame;

    // Bin FX support
    FXShaderToken m_TokenTable;
    TArray<uint32> m_TokenData;

    virtual int Size()
    {
        int nSize = sizeof(*this);
        nSize += sizeOfVP(m_Insts);
        nSize += sizeofVector(m_TokenData);
        nSize += sizeOfV(m_TokenTable);

        return nSize;
    }

    virtual void GetMemoryUsage(ICrySizer* pSizer) const
    {
        pSizer->AddObject(this, sizeof(*this));
        pSizer->AddObject(m_pGlobalCache);
        pSizer->AddObject(m_pDevCache);
        pSizer->AddObject(m_Insts);
        pSizer->AddObject(m_TokenData);
        pSizer->AddObject(m_TokenTable);
    }
    CHWShader_D3D()
    {
#if !defined(CONSOLE)
        m_pTree = NULL;
        m_pParser = NULL;
#endif
        mfConstruct();
    }
    
    static void mfInit();
    void mfConstruct()
    {
        if (s_bInitShaders)
        {
            s_bInitShaders = false;
        }

        m_pCurInst = NULL;
        m_pDevCache = NULL;
        m_nCurInstFrame = 0;

        m_dwShaderType = 0;

        m_bUseLookUpTable = false;

    }

    void mfFree(uint32 CRC32)
    {
#if !defined(CONSOLE)
        SAFE_DELETE(m_pTree);
        SAFE_DELETE(m_pParser);
#endif

        m_Flags = 0;
        mfReset(CRC32);
    }

    // Binary cache support
    SShaderCacheHeaderItem* mfGetCompressedItem(uint32 nFlags, int32& nSize);
    SShaderCacheHeaderItem* mfGetCacheItem(uint32& nFlags, int32& nSize);
    static bool mfAddCacheItem(SShaderCache* pCache, SShaderCacheHeaderItem* pItem, const byte* pData, int nLen, bool bFlush, CCryNameTSCRC Name);

    bool mfCloseCacheFile()
    {
        SAFE_RELEASE (m_pDevCache);
        SAFE_RELEASE (m_pGlobalCache);

        return true;
    }

    static byte* mfBindsToCache(SHWSInstance* pInst, std::vector<SCGBind>* Binds, int nParams, byte* pP);
    byte* mfBindsFromCache(std::vector<SCGBind>*& Binds, int nParams, byte* pP);

    bool mfActivateCacheItem(CShader* pSH, SShaderCacheHeaderItem* pItem, uint32 nSize, uint32 nFlags);
    static bool mfCreateCacheItem(SHWSInstance* pInst, std::vector<SCGBind>& InstBinds, byte* pData, int nLen, CHWShader_D3D* pSH, bool bShaderThread);

    int mfGetParams(int Type)
    {
        assert(m_pCurInst);
        return m_pCurInst->m_nParams[Type];
    }

    bool mfSetHWStartProfile(uint32 nFlags);

    void mfSaveCGFile(const char* scr, const char* path);
    void mfOutputCompilerError(string& strErr, const char* szSrc);
    static bool mfCreateShaderEnv(int nThread, SHWSInstance* pInst, LPD3D10BLOB pShader, void* pConstantTable, LPD3D10BLOB pErrorMsgs, std::vector<SCGBind>& InstBindVars, CHWShader_D3D* pSH, bool bShaderThread, CShader* pFXShader, int nCombination, const char* src = NULL);
    void mfPrintCompileInfo(SHWSInstance* pInst);
    bool mfCompileHLSL_Int(CShader* pSH, char* prog_text, LPD3D10BLOB* ppShader, void** ppConstantTable, LPD3D10BLOB* ppErrorMsgs, string& strErr, std::vector<SCGBind>& InstBindVars);

    int mfAsyncCompileReady(SHWSInstance* pInst);
    bool mfRequestAsync(CShader* pSH, SHWSInstance* pInst, std::vector<SCGBind>& InstBindVars, const char* prog_text, const char* szProfile, const char* szEntry);

    void mfSubmitRequestLine(SHWSInstance* pInst, string* pRequestLine = NULL);
    LPD3D10BLOB mfCompileHLSL(CShader* pSH, char* prog_text, void** ppConstantTable, LPD3D10BLOB* ppErrorMsgs, uint32 nFlags, std::vector<SCGBind>& InstBindVars);
    bool mfUploadHW(SHWSInstance* pInst, byte* pBuf, uint32 nSize, CShader* pSH, uint32 nFlags);
    bool mfUploadHW(LPD3D10BLOB pShader, SHWSInstance* pInst, CShader* pSH, uint32 nFlags);

    ED3DShError mfIsValid_Int(SHWSInstance*& pInst, bool bFinalise);

    //ILINE most common outcome
    ILINE ED3DShError mfIsValid(SHWSInstance*& pInst, bool bFinalise)
    {
        if (pInst->m_Handle.m_pShader)
        {
            return ED3DShError_Ok;
        }
        if (pInst->m_bAsyncActivating)
        {
            return ED3DShError_NotCompiled;
        }

        return mfIsValid_Int(pInst, bFinalise);
    }

    ED3DShError mfFallBack(SHWSInstance*& pInst, int nStatus);
    void mfCommitCombinations(int nFrame, int nFrameDiff);
    void mfCommitCombination(SHWSInstance* pInst, int nFrame, int nFrameDiff);
    void mfLogShaderCacheMiss(SHWSInstance* pInst);
    void mfLogShaderRequest(SHWSInstance* pInst);

    void mfBindInstance(SHWSInstance *pInst, EHWShaderClass eHWSC)
    {
        m_pCurInst = pInst;
        if (eHWSC == eHWSC_Vertex)
        {
            s_pCurInstVS = pInst;
            if (s_pCurVS != pInst->m_Handle.m_pShader)
            {
                s_pCurVS = pInst->m_Handle.m_pShader;
#ifndef _RELEASE
                gRenDev->m_RP.m_PS[gRenDev->m_RP.m_nProcessThreadID].m_NumVShadChanges++;
#endif
                mfBind();
            }
        }
        else if (eHWSC == eHWSC_Pixel)
        {
            s_pCurInstPS = pInst;
            if (s_pCurPS != pInst->m_Handle.m_pShader)
            {
                s_pCurPS = pInst->m_Handle.m_pShader;
#ifndef _RELEASE
                gRenDev->m_RP.m_PS[gRenDev->m_RP.m_nProcessThreadID].m_NumPShadChanges++;
#endif
                mfBind();
            }
        }
    }

    void mfBind()
    {
        if (mfIsValid(m_pCurInst, true) == ED3DShError_Ok)
        {
            if (gRenDev->m_nFrameSwapID != m_pCurInst->m_nUsedFrame)
            {
                m_pCurInst->m_nUsedFrame = gRenDev->m_nFrameSwapID;
                m_pCurInst->m_nUsed++;
            }
            gcpRendD3D->m_DevMan.BindShader(m_eSHClass, (ID3D11Resource*)m_pCurInst->m_Handle.m_pShader->m_pHandle);
        }
    }

    static inline void mfBindGS(SD3DShader* pShader, void* pHandle)
    {
        if (s_pCurGS != pShader)
        {
            s_pCurGS = pShader;
#if !defined(_RELEASE)
            gcpRendD3D->m_RP.m_PS[gcpRendD3D->m_RP.m_nProcessThreadID].m_NumGShadChanges++;
#endif
            gcpRendD3D->GetDeviceContext().GSSetShader((ID3D11GeometryShader*)pHandle, NULL, 0);
        }
        if (!s_pCurGS)
        {
            s_pCurInstGS = NULL;
            s_nActivationFailMask &= ~(1 << eHWSC_Geometry);
        }
    }
    static inline void mfBindDS(SD3DShader* pShader, void* pHandle)
    {
        if (s_pCurDS != pShader)
        {
            s_pCurDS = pShader;
#if !defined(_RELEASE)
            gcpRendD3D->m_RP.m_PS[gcpRendD3D->m_RP.m_nProcessThreadID].m_NumDShadChanges++;
#endif
            gcpRendD3D->GetDeviceContext().DSSetShader((ID3D11DomainShader*)pHandle, NULL, 0);
        }
        if (!s_pCurDS)
        {
            s_pCurInstDS = NULL;
            s_nActivationFailMask &= ~(1 << eHWSC_Domain);
        }
    }
    static inline void mfBindHS(SD3DShader* pShader, void* pHandle)
    {
        if (s_pCurHS != pShader)
        {
            s_pCurHS = pShader;
#if !defined(_RELEASE)
            gcpRendD3D->m_RP.m_PS[gcpRendD3D->m_RP.m_nProcessThreadID].m_NumHShadChanges++;
#endif
            gcpRendD3D->GetDeviceContext().HSSetShader((ID3D11HullShader*)pHandle, NULL, 0);
        }
        if (!s_pCurHS)
        {
            s_pCurInstHS = NULL;
            s_nActivationFailMask &= ~(1 << eHWSC_Hull);
        }
    }
    static inline void mfBindCS(SD3DShader* pShader, void* pHandle)
    {
        if (s_pCurCS != pShader)
        {
            s_pCurCS = pShader;
#if !defined(_RELEASE)
            gcpRendD3D->m_RP.m_PS[gcpRendD3D->m_RP.m_nProcessThreadID].m_NumCShadChanges++;
#endif
            gcpRendD3D->GetDeviceContext().CSSetShader((ID3D11ComputeShader*)pHandle, NULL, 0);
        }
        if (!s_pCurCS)
        {
            s_pCurInstCS = NULL;
            s_nActivationFailMask &= ~(1 << eHWSC_Compute);
        }
    }

    static void UpdatePerMaterialConstantBuffer();
    static void mfCommitParamsGlobal();

    SCGBind* mfGetParameterBind(const CCryNameR& Name)
    {
        if (!m_pCurInst)
        {
            return NULL;
        }

        std::vector<SCGBind>& pBinds = m_pCurInst->m_pBindVars;
        int i;
        int nSize = pBinds.size();
        for (i = 0; i < nSize; i++)
        {
            if (Name == pBinds[i].m_Name)
            {
                return &pBinds[i];
            }
        }
        return NULL;
    }

    static void UpdatePerInstanceConstants(
        EHWShaderClass shaderClass,
        const SCGParam* parameters,
        AZ::u32 parameterCount,
        void* outputData);

    void UpdatePerInstanceConstantBuffer();
    void UpdatePerBatchConstantBuffer();
    static void UpdatePerFrameResourceGroup();

    inline void mfSetSamplers(const std::vector<STexSamplerRT>& Samplers, EHWShaderClass eHWClass)
    {
        mfSetSamplers_Old(Samplers, eHWClass);
        if (!m_pCurInst)
        {
            return;
        }
        SHWSInstance* pInst = m_pCurInst;
        mfSetSamplers(pInst->m_Samplers, eHWClass);
        mfSetTextures(pInst->m_Textures, eHWClass);
    }

    void mfLostDevice(SHWSInstance* pInst, byte* pBuf, int nSize)
    {
        pInst->m_Handle.SetFake();
        pInst->m_Handle.m_pData = new byte[nSize];
        memcpy(pInst->m_Handle.m_pData, pBuf, nSize);
        pInst->m_Handle.m_nData = nSize;
    }

    ILINE int mfCheckActivation(CShader* pSH, SHWSInstance*& pInst, uint32 nFlags)
    {
        ED3DShError eError = mfIsValid(pInst, true);
        if (eError == ED3DShError_NotCompiled)
        {
            if (!mfActivate(pSH, nFlags))
            {
                pInst = m_pCurInst;
                if (gRenDev->m_cEF.m_bActivatePhase)
                {
                    return 0;
                }
                if (!pInst->IsAsyncCompiling())
                {
                    pInst->m_Handle.SetNonCompilable();
                }
                else
                {
                    eError = mfIsValid(pInst, true);
                    if (eError == ED3DShError_CompilingError || pInst->m_bAsyncActivating)
                    {
                        return 0;
                    }
                    if (m_eSHClass == eHWSC_Vertex)
                    {
                        return 1;
                    }
                    else
                    {
                        return -1;
                    }
                }
                return 0;
            }
            if (gRenDev->m_RP.m_pCurTechnique)
            {
                mfUpdatePreprocessFlags(gRenDev->m_RP.m_pCurTechnique);
            }
            pInst = m_pCurInst;
        }
        else
            if (eError == ED3DShError_Fake)
            {
                if (pInst->m_Handle.m_pData)
                {
                    if (gRenDev && !gRenDev->CheckDeviceLost())
                    {
                        mfUploadHW(pInst, pInst->m_Handle.m_pData, pInst->m_Handle.m_nData, pSH, nFlags);
                        SAFE_DELETE_ARRAY(pInst->m_Handle.m_pData);
                        pInst->m_Handle.m_nData = 0;
                    }
                    else
                    {
                        eError = ED3DShError_CompilingError;
                    }
                }
            }
        if (eError == ED3DShError_CompilingError)
        {
            return 0;
        }
        return 1;
    }

    void mfSetForOverdraw(SHWSInstance* pInst, uint32 nFlags, uint64& RTMask);

#if !defined(_RELEASE)
    static void LogSamplerTextureMismatch(const CTexture* pTex, const STexSamplerRT* pSampler, EHWShaderClass shaderClass, const char* pMaterialName);
#endif
    static bool mfSetTextures(const std::vector<SCGTexture>& Textures, EHWShaderClass eHWClass);
    static bool mfSetSamplers(const std::vector<SCGSampler>& Samplers, EHWShaderClass eHWClass);
    static bool mfSetSamplers_Old(const std::vector<STexSamplerRT>& Samplers, EHWShaderClass eHWClass);
    SHWSInstance* mfGetInstance(CShader* pSH, SShaderCombIdent& Ident, uint32 nFlags);
    SHWSInstance* mfGetInstance(CShader* pSH, int nHashInstance, uint64 GLMask);
    static void mfPrepareShaderDebugInfo(SHWSInstance* pInst, CHWShader_D3D* pSH, const char* szAsm, std::vector<SCGBind>& InstBindVars, void* pBuffer);
    void mfGetSrcFileName(char* srcName, int nSize);
    static void mfGetDstFileName(SHWSInstance* pInst, CHWShader_D3D* pSH, char* dstname, int nSize, byte bType);
    static void mfGenName(SHWSInstance* pInst, char* dstname, int nSize, byte bType);
    void CorrectScriptEnums(CParserBin& Parser, SHWSInstance* pInst, std::vector<SCGBind>& InstBindVars, FXShaderToken* Table);
    bool ConvertBinScriptToASCII(CParserBin& Parser, SHWSInstance* pInst, std::vector<SCGBind>& InstBindVars, FXShaderToken* Table, TArray<char>& Scr);
    void RemoveUnaffectedParameters_D3D10(CParserBin& Parser, SHWSInstance* pInst, std::vector<SCGBind>& InstBindVars);
    bool mfStoreCacheTokenMap(FXShaderToken*& Table, TArray<uint32>*& pSHData, const char* szName);
    void mfGetTokenMap(CResFile* pRes, SDirEntry* pDE, FXShaderToken*& Table, TArray<uint32>*& pSHData);
    void mfSetDefaultRT(uint64& nAndMask, uint64& nOrMask);

public:
    bool mfGetCacheTokenMap(FXShaderToken*& Table, TArray<uint32>*& pSHData, uint64 nMaskGen);
    bool mfGenerateScript(CShader* pSH, SHWSInstance*& pInst, std::vector<SCGBind>& InstBindVars, uint32 nFlags, FXShaderToken* Table, TArray<uint32>* pSHData, TArray<char>& sNewScr);
    bool mfActivate(CShader* pSH, uint32 nFlags, FXShaderToken* Table = NULL, TArray<uint32>* pSHData = NULL, bool bCompressedOnly = false);

    void SetTokenFlags(uint32 nToken);
    uint64 CheckToken(uint32 nToken);
    uint64 CheckIfExpr_r(uint32* pTokens, uint32& nCur, uint32 nSize);
    void mfConstructFX_Mask_RT(FXShaderToken* Table, TArray<uint32>* pSHData);
    void mfConstructFX(FXShaderToken* Table, TArray<uint32>* pSHData);

    static bool mfAddFXSampler(SHWSInstance* pInst, SShaderFXParams& FXParams, SFXSampler* pr, const char* ParamName, SCGBind* pBind, CShader* ef, EHWShaderClass eSHClass);
    static bool mfAddFXTexture(SHWSInstance* pInst, SShaderFXParams& FXParams, SFXTexture* pr, const char* ParamName, SCGBind* pBind, CShader* ef, EHWShaderClass eSHClass);

    /* 
    Add shader parameters during the shader parsing.  A parameters can be Sampler, Texture or Constant 
    */
    static void mfAddFXParameter(SHWSInstance* pInst, SParamsGroup& OutParams, SShaderFXParams& FXParams, SFXParam* pr, const char* ParamName, SCGBind* pBind, CShader* ef, bool bInstParam, EHWShaderClass eSHClass);
    static bool mfAddFXParameter(SHWSInstance* pInst, SParamsGroup& OutParams, SShaderFXParams& FXParams, const char* param, SCGBind* bn, bool bInstParam, EHWShaderClass eSHClass, CShader* pFXShader);

    static void mfGatherFXParameters(SHWSInstance* pInst, std::vector<SCGBind>* InstBindVars, CHWShader_D3D* pSH, int nFlags, CShader* pFXShader);

    /*
    Associates the final binding of all resources slots per shader instance.
    */
    static void mfCreateBinds(SHWSInstance* pInst, void* pConstantTable, byte* pShader, int nSize);

    bool mfUpdateSamplers(CShader* shader);
    static void mfPostVertexFormat(SHWSInstance * pInst, CHWShader_D3D * pHWSH, bool bCol, byte bNormal, bool bTC0, bool bTC1, bool bPSize, bool bTangent[2], bool bBitangent[2], bool bHWSkin, bool bSH[2], bool bMorphTarget, bool bMorph);
    void mfUpdateFXVertexFormat(SHWSInstance* pInst, CShader* pSH);

    void ModifyLTMask(uint32& nMask);

public:
    virtual ~CHWShader_D3D();
    bool mfSetVS(int nFlags = 0);
    bool mfSetPS(int nFlags = 0);
    bool mfSetGS(int nFlags = 0);
    bool mfSetHS(int nFlags = 0);
    bool mfSetCS(int nFlags = 0);
    bool mfSetDS(int nFlags = 0);

    inline bool mfSet(int nFlags = 0)
    {
        switch (m_eSHClass)
        {
        case eHWSC_Vertex:
            return mfSetVS(nFlags);
        case eHWSC_Pixel:
            return mfSetPS(nFlags);
        case eHWSC_Geometry:
            return mfSetGS(nFlags);
        case eHWSC_Compute:
            return mfSetCS(nFlags);
        case eHWSC_Hull:
            return mfSetHS(nFlags);
        case eHWSC_Domain:
            return mfSetDS(nFlags);
        }
        return false;
    }

    virtual bool mfAddEmptyCombination(CShader* pSH, uint64 nRT, uint64 nGL, uint32 nLT);
    virtual bool mfStoreEmptyCombination(SEmptyCombination& Comb);
    virtual bool mfSetV(int nFlags = 0){ return mfSet(nFlags); };
    virtual void mfReset(uint32 CRC32);
    virtual const char* mfGetEntryName() { return m_EntryFunc.c_str(); }
    virtual bool mfFlushCacheFile();
    virtual bool Export(SShaderSerializeContext& SC);
    virtual bool mfPrecache(SShaderCombination& cmb, bool bForce, bool bFallback, bool bCompressedOnly, CShader* pSH, CShaderResources* pRes);

    // Vertex shader specific functions
    virtual AZ::Vertex::Format mfVertexFormat(bool& bUseTangents, bool& bUseLM, bool& bUseHWSkin, bool& bUseVertexVelocity);
    static AZ::Vertex::Format  mfVertexFormat(SHWSInstance* pInst, CHWShader_D3D* pSH, LPD3D10BLOB pBuffer);
    virtual void mfUpdatePreprocessFlags(SShaderTechnique* pTech);

    virtual const char* mfGetActivatedCombinations(bool bForLevel);

    static void mfSetGlobalParams();
    static void mfSetCameraParams();
    static void UpdatePerViewConstantBuffer();
    static bool mfAddGlobalTexture(SCGTexture& Texture);
    static bool mfAddGlobalSampler(STexSamplerRT& Sampler);

    static void* GetVSDataForDecl(const D3D11_INPUT_ELEMENT_DESC* pDecl, int nCount, int& nDataSize);

    static void ShutDown();

    static Vec4 GetVolumetricFogParams();
    static Vec4 GetVolumetricFogRampParams();
    static void GetFogColorGradientConstants(Vec4& fogColGradColBase, Vec4& fogColGradColDelta);
    static Vec4 GetFogColorGradientRadial();

    // Import/Export
    bool ExportSamplers(SCHWShader& SHW, SShaderSerializeContext& SC);
    bool ExportParams(SCHWShader& SHW, SShaderSerializeContext& SC);

    static CHWShader_D3D::SHWSInstance* s_pCurInstGS;
    static CHWShader_D3D::SHWSInstance* s_pCurInstDS;
    static CHWShader_D3D::SHWSInstance* s_pCurInstHS;
    static CHWShader_D3D::SHWSInstance* s_pCurInstCS;
    static CHWShader_D3D::SHWSInstance* s_pCurInstVS;
    static CHWShader_D3D::SHWSInstance* s_pCurInstPS;

    static bool s_bFirstGS;
    static bool s_bFirstDS;
    static bool s_bFirstHS;
    static bool s_bFirstCS;
    static bool s_bFirstVS;
    static bool s_bFirstPS;

    static int s_nActivationFailMask;

    static bool s_bInitShaders;

    static int s_nResetDeviceFrame;
    static int s_nInstFrame;

    static int s_nDevicePSDataSize;
    static int s_nDeviceVSDataSize;

    static SCGTextures s_PF_Textures;   // Per frame textures (shared between all)
    static SCGSamplers s_PF_Samplers;   // Per-frame samplers (shared between all)

    friend struct SShaderTechniqueStat;
};

struct SShaderTechniqueStat
{
    SShaderTechnique* pTech;
    CShader* pShader;
    CHWShader_D3D* pVS;
    CHWShader_D3D* pPS;
    CHWShader_D3D::SHWSInstance* pVSInst;
    CHWShader_D3D::SHWSInstance* pPSInst;
};

extern AZStd::vector<SShaderTechniqueStat, AZ::StdLegacyAllocator> g_SelectedTechs;
