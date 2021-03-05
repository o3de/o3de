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

// Shader cache directory
#define g_shaderCache "Shaders/Cache/"

#define CONCAT_PATHS(a, b) a b

struct SPreprocessMasks
{
    uint64 nRT, nRTSet;
    uint64 nGL, nGLSet;
    uint32 nLT, nLTSet;
    uint32 nMD, nMDSet;
    uint32 nMDV, nMDVSet;
};

typedef std::map<uint32, uint32> MapPreprocessFlags;
typedef MapPreprocessFlags::iterator MapPreprocessFlagsItor;

struct SPreprocessNode
{
    TArray<uint32> m_Expression;
    int m_nNode;
    uint64 m_RTMask;
    uint64 m_GLMask;
    uint32 m_LTMask;
    uint32 m_MDMask;
    uint32 m_MDVMask;
    std::vector<SPreprocessNode*> m_Nodes[2];
    short m_nCode[2]; // 0 - No Code, 1 - Has Code, -1 - Depends
    SPreprocessNode()
    {
        m_nNode = 0;
        m_RTMask = 0;
        m_GLMask = 0;
        m_LTMask = 0;
        m_MDMask = 0;
        m_MDVMask = 0;
        m_nCode[0] = m_nCode[1] = 0;
    }
    ~SPreprocessNode()
    {
        uint32 i;
        for (i = 0; i < m_Nodes[0].size(); i++)
        {
            SAFE_DELETE(m_Nodes[0][i]);
        }
        for (i = 0; i < m_Nodes[1].size(); i++)
        {
            SAFE_DELETE(m_Nodes[1][i]);
        }
    }
};

struct SPreprocessFlagDesc
{
    CCryNameR m_Name;
    uint64 m_nFlag;
    bool m_bWasReferenced;
    SPreprocessFlagDesc() { m_nFlag = 0; m_bWasReferenced = true; }
};

struct SPreprocessTree
{
    static std::vector<SPreprocessFlagDesc> s_GLDescs;
    static std::vector<SPreprocessFlagDesc> s_RTDescs;
    static std::vector<SPreprocessFlagDesc> s_LTDescs;
    std::vector<SPreprocessFlagDesc> m_GLDescsLocal;
    std::vector<SPreprocessFlagDesc> m_MDDescs;
    std::vector<SPreprocessFlagDesc> m_MDVDescs;

    static MapPreprocessFlags s_MapFlags;
    MapPreprocessFlags m_MapFlagsLocal;

    std::vector<SPreprocessNode*> m_Root;
    ~SPreprocessTree()
    {
        uint32 i;
        for (i = 0; i < m_Root.size(); i++)
        {
            SAFE_DELETE(m_Root[i]);
        }
        m_Root.clear();
    }
    CShader* m_pSH;
};

//=======================================================================================================

struct SShaderLevelPolicies
{
    std::vector<string> m_WhiteGlobalList;
    std::vector<string> m_WhitePerLevelList;
};
// CRY DX12
union UPipelineState // Pipeline state relevant for shader instantiation
{
#if defined(AZ_RESTRICTED_PLATFORM)
    #include AZ_RESTRICTED_FILE(ShaderCache_h)
#endif
    uint64 opaque;
    UPipelineState()
        : opaque(0) {
    }
};
static_assert(sizeof(UPipelineState) == sizeof(uint64), "UPipelineState needs to be 64 bit");

struct SShaderCombIdent
{
    uint64 m_RTMask;    // run-time mask
    uint64 m_GLMask;    // global mask
    uint64 m_STMask;    // static mask
    UPipelineState m_pipelineState;
    union
    {
        struct
        {
            uint32 m_LightMask;   // light mask
            uint32 m_MDMask;      // texture coordinates modifier mask
            uint32 m_MDVMask;     // vertex modifier mask | SF_PLATFORM
            uint32 m_nHash;
        };
        struct
        {
            uint64 m_FastCompare1;
            uint64 m_FastCompare2;
        };
    };

    SShaderCombIdent(uint64 nRT, uint64 nGL, uint32 nLT, uint32 nMD, uint32 nMDV)
    {
        m_RTMask = nRT;
        m_GLMask = nGL;
        m_LightMask = nLT;
        m_MDMask = nMD;
        m_MDVMask = nMDV;
        m_nHash = 0;
        m_STMask = 0;
    }
    SShaderCombIdent(uint64 nGL, const SShaderCombIdent& Ident)
    {
        *this = Ident;
        m_GLMask = nGL;
    }

    SShaderCombIdent()
    {
        m_RTMask = 0;
        m_GLMask = 0;
        m_LightMask = 0;
        m_MDMask = 0;
        m_MDVMask = 0;
        m_nHash = 0;
        m_STMask = 0;
    }

    uint32 PostCreate();
};

//==========================================================================================================================

class CResStreamDirCallback
    : public IStreamCallback
{
    virtual void StreamOnComplete (IReadStream* pStream, unsigned nError);
    virtual void StreamAsyncOnComplete (IReadStream* pStream, unsigned nError);
};
class CResStreamCallback
    : public IStreamCallback
{
    virtual void StreamOnComplete (IReadStream* pStream, unsigned nError);
    virtual void StreamAsyncOnComplete (IReadStream* pStream, unsigned nError);
};

struct SResStreamEntry
{
    struct SResStreamInfo* m_pParent;
    CCryNameTSCRC m_Name;
    IReadStreamPtr m_readStream;
};

struct SResStreamInfo
{
    SShaderCache* m_pCache;
    CResFile* m_pRes;
    CResStreamCallback m_Callback;
    CResStreamDirCallback m_CallbackDir;

    CryCriticalSection m_StreamLock;
    std::vector<SResStreamEntry*> m_EntriesQueue;

    std::vector<IReadStreamPtr> m_dirReadStreams;
    int m_nDirRequestCount;

    SResStreamInfo(SShaderCache* pCache)
    {
        m_pCache = pCache;
        m_pRes = NULL;
        m_nDirRequestCount = 0;
    }
    ~SResStreamInfo()
    {
        assert (m_EntriesQueue.empty() && m_dirReadStreams.empty());
    }
    void AbortJobs()
    {
        using std::swap;

        CryAutoLock<CryCriticalSection> lock(m_StreamLock);

        // Copy the list here, as the streaming callback can modify the list
        std::vector<SResStreamEntry*> entries = m_EntriesQueue;

        for (size_t i = 0, c = entries.size(); i != c; ++i)
        {
            SResStreamEntry* pEntry = entries[i];
            if (pEntry->m_readStream)
            {
                pEntry->m_readStream->Abort();
            }
        }


        // Copy the list here, as the streaming callback can modify the list
        std::vector<IReadStreamPtr> dirStreams = m_dirReadStreams;

        for (size_t i = 0, c = dirStreams.size(); i != c; ++i)
        {
            IReadStream* pReadStream = dirStreams[i];
            if (pReadStream)
            {
                pReadStream->Abort();
            }
        }
    }
    SResStreamEntry* AddEntry(CCryNameTSCRC Name)
    {
        uint32 i;
        for (i = 0; i < m_EntriesQueue.size(); i++)
        {
            SResStreamEntry* pE = m_EntriesQueue[i];
            if (pE->m_Name == Name)
            {
                break;
            }
        }
        if (i == m_EntriesQueue.size())
        {
            SResStreamEntry* pEntry = new SResStreamEntry;
            pEntry->m_pParent = this;
            pEntry->m_Name = Name;
            m_EntriesQueue.push_back(pEntry);
            return pEntry;
        }
        return NULL;
    }
    void Release()
    {
        assert(m_EntriesQueue.empty());
        delete this;
    }

private:
    SResStreamInfo(const SResStreamInfo&);
    SResStreamInfo& operator = (const SResStreamInfo&);
};

struct SShaderDevCache
{
    int m_nRefCount;
    CCryNameR m_Name;

    FXDeviceShader m_DeviceShaders;

    SShaderDevCache()
    {
        m_nRefCount = 1;
    }
    SShaderDevCache(const CCryNameR& name)
        :m_Name(name)
    {
        m_nRefCount = 1;
    }
    int Size();
    void GetMemoryUsage(ICrySizer* pSizer) const;
    int Release()
    {
        m_nRefCount--;
        if (m_nRefCount)
        {
            return m_nRefCount;
        }
        delete this;
        return 0;
    }
    ~SShaderDevCache() {};
};


struct SShaderCache
{
    volatile int32 m_nRefCount;
    CCryNameR m_Name;
    class CResFile* m_pRes[2];
    SResStreamInfo* m_pStreamInfo;
    uint32 m_nPlatform;
    bool m_bReadOnly[2];
    bool m_bValid[2];
    bool m_bNeedPrecache;
    SShaderCache()
    {
        m_nPlatform = 0;
        m_nRefCount = 1;
        m_pStreamInfo = NULL;
        m_pRes[0] = m_pRes[1] = NULL;
        m_bValid[0] = m_bValid[1] = false;
        m_bReadOnly[0] = m_bReadOnly[1] = false;
        m_bNeedPrecache = false;
    }
    bool isValid();
    int Size();
    void GetMemoryUsage(ICrySizer* pSizer) const;
    void Cleanup();
    int AddRef() { return CryInterlockedIncrement(&m_nRefCount); }
    int Release(bool bDelete = true)
    {
        int nRef = CryInterlockedDecrement(&m_nRefCount);
        if (nRef || !bDelete)
        {
            return nRef;
        }
        delete this;
        return 0;
    }
    ~SShaderCache();
};

struct SEmptyCombination
{
    uint64 nRTOrg;
    uint64 nGLOrg;
    uint32 nLTOrg;
    uint64 nRTNew;
    uint64 nGLNew;
    uint32 nLTNew;
    uint32 nMD;
    uint32 nMDV;
    uint64 nST;
    class CHWShader* pShader;

    using Combinations = AZStd::vector<SEmptyCombination, AZ::StdLegacyAllocator>;
    static Combinations s_Combinations;
};

//==========================================================================================================================

struct SShaderCombination
{
    uint64 m_RTMask;
    uint32 m_LTMask;
    uint32 m_MDMask;
    uint32 m_MDVMask;
    UPipelineState m_pipelineState;
    SShaderCombination()
    {
        m_RTMask = 0;
        m_LTMask = 0;
        m_MDMask = 0;
        m_MDVMask = 0;
    }
};


struct SCacheCombination
{
    CCryNameR Name;
    CCryNameR CacheName;
    SShaderCombIdent Ident;
    uint32 nCount;
    EHWShaderClass eCL;
    SCacheCombination()
    {
        nCount = 0;
        eCL = eHWSC_Vertex;
    }

    void GetMemoryUsage([[maybe_unused]] ICrySizer* pSizer) const {}
};

struct SShaderGenComb
{
    CCryNameR Name;
    SShaderGen* pGen;

    SShaderGenComb()
        : Name()
        , pGen(nullptr)
    {
    }

    ~SShaderGenComb()
    {
        //SAFE_RELEASE(pGen);
    }

    _inline SShaderGenComb (const SShaderGenComb& src)
        : Name(src.Name)
        , pGen(src.pGen)
    {
        if (pGen)
        {
            pGen->m_nRefCount++;
        }
    }

    _inline SShaderGenComb& operator = (const SShaderGenComb& src)
    {
        this->~SShaderGenComb();
        new(this)SShaderGenComb(src);
        return *this;
    }

    void GetMemoryUsage(ICrySizer* pSizer) const
    {
        pSizer->AddObject(Name);
        pSizer->AddObject(pGen);
    }
};

typedef std::map<CCryNameR, SCacheCombination> FXShaderCacheCombinations;
typedef FXShaderCacheCombinations::iterator FXShaderCacheCombinationsItor;
typedef std::map<CCryNameR, std::vector<SCacheCombination> > FXShaderCacheCombinationsList;
typedef FXShaderCacheCombinationsList::iterator FXShaderCacheCombinationsListItor;
