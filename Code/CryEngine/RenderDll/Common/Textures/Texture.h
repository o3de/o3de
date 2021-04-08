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

// change file hash again

#pragma once


#include <set>
#include "../ResFile.h"
#include "PowerOf2BlockPacker.h"
#include "STLPoolAllocator.h"

#include "ITextureStreamer.h"

#include "TextureArrayAlloc.h"
#include "Image/DDSImage.h"

#include "ImageExtensionHelper.h"
#include <AzCore/Jobs/LegacyJobExecutor.h>
#include <AzCore/std/parallel/atomic.h>
#include <AzCore/std/containers/map.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

#if AZ_RENDER_TO_TEXTURE_GEM_ENABLED
#include <RenderContextConfig.h>
#endif // if AZ_RENDER_TO_TEXTURE_GEM_ENABLED

#if defined(AZ_RESTRICTED_PLATFORM)
#undef AZ_RESTRICTED_SECTION
#define TEXTURE_H_SECTION_1 1
#define TEXTURE_H_SECTION_2 2
#define TEXTURE_H_SECTION_3 3
#define TEXTURE_H_SECTION_4 4
#define TEXTURE_H_SECTION_5 5
#define TEXTURE_H_SECTION_6 6
#define TEXTURE_H_SECTION_7 7
#endif

// Only the editor needs to use a unique mutex per texture.
// In the editor, multiple worker threads could destroy device resource sets which point to the same texture
#if defined(AZ_PLATFORM_WINDOWS)
    #define USE_UNIQUE_MUTEX_PER_TEXTURE
#endif

class CTexture;
class CImageFile;
class CTextureStreamPoolMgr;
struct SDepthTexture;

#define TEXPOOL_LOADSIZE 6 * 1024 * 1024
#define MAX_MIP_LEVELS (100)
#define CTEXTURE_RENDER_TARGET_USE_COUNT_NUMBER_BITS 2

// Custom Textures IDs
enum
{
    TO_RT_2D = 1,

    TO_FOG,
    TO_FROMOBJ,
    TO_SVOTREE,
    TO_SVOTRIS,
    TO_SVOGLCM,
    TO_SVORGBS,
    TO_SVONORM,
    TO_SVOOPAC,
    TO_FROMOBJ_CM,

    TO_SHADOWID0,
    TO_SHADOWID1,
    TO_SHADOWID2,
    TO_SHADOWID3,
    TO_SHADOWID4,
    TO_SHADOWID5,
    TO_SHADOWID6,
    TO_SHADOWID7,

    TO_FROMRE0,
    TO_FROMRE1,

    TO_FROMRE0_FROM_CONTAINER,
    TO_FROMRE1_FROM_CONTAINER,

    TO_SCREENMAP,
    TO_SHADOWMASK,
    TO_CLOUDS_LM,
    TO_BACKBUFFERMAP,
    TO_PREVBACKBUFFERMAP0,
    TO_PREVBACKBUFFERMAP1,
    TO_MIPCOLORS_DIFFUSE,
    TO_MIPCOLORS_BUMP,

    TO_DOWNSCALED_ZTARGET_FOR_AO,
    TO_QUARTER_ZTARGET_FOR_AO,
    TO_WATEROCEANMAP,
    TO_WATERVOLUMEMAP,

    TO_WATERVOLUMEREFLMAP,
    TO_WATERVOLUMEREFLMAPPREV,
    TO_WATERVOLUMECAUSTICSMAP,
    TO_WATERVOLUMECAUSTICSMAPTEMP,

    TO_WATERRIPPLESMAP,

    TO_VOLOBJ_DENSITY,
    TO_VOLOBJ_SHADOW,

    TO_COLORCHART,

    TO_ZTARGET_MS,

    TO_SCENE_NORMALMAP,
    TO_SCENE_NORMALMAP_MS,

    TO_LPV_R,
    TO_LPV_G,
    TO_LPV_B,
    TO_RSM_NORMAL,
    TO_RSM_COLOR,
    TO_RSM_DEPTH,

    TO_SCENE_DIFFUSE_ACC,
    TO_SCENE_SPECULAR_ACC,
    TO_SCENE_DIFFUSE_ACC_MS,
    TO_SCENE_SPECULAR_ACC_MS,
    TO_SCENE_TEXTURES,
    TO_SCENE_TARGET,

    TO_BACKBUFFERSCALED_D2,
    TO_BACKBUFFERSCALED_D4,
    TO_BACKBUFFERSCALED_D8,

    TO_SKYDOME_MIE,
    TO_SKYDOME_RAYLEIGH,
    TO_SKYDOME_MOON,

    TO_VOLFOGSHADOW_BUF,

    TO_HDR_MEASURED_LUMINANCE,
    TO_MODELHUD,

    TO_DEFAULT_ENVIRONMENT_PROBE,
};


#define NUM_HDR_TONEMAP_TEXTURES 4
#define NUM_HDR_BLOOM_TEXTURES   3
#define MIN_DOF_COC_K 6


#if defined(OPENGL_ES) || defined(CRY_USE_METAL)
#define MAX_OCCLUSION_READBACK_TEXTURES 2
#else
#define MAX_OCCLUSION_READBACK_TEXTURES 8
#endif

#define DYNTEXTURE_TEXCACHE_LIMIT 32

inline int LogBaseTwo(int iNum)
{
    int i, n;
    for (i = iNum - 1, n = 0; i > 0; i >>= 1, n++)
    {
        ;
    }
    return n;
}

enum EShadowBuffers_Pool
{
    SBP_D16,
    SBP_D24S8,
    SBP_D32FS8,
    SBP_R16G16,
    SBP_MAX,
    SBP_UNKNOWN
};

//dx9 usages
enum ETexture_Usage
{
    eTXUSAGE_AUTOGENMIPMAP,
    eTXUSAGE_DEPTHSTENCIL,
    eTXUSAGE_RENDERTARGET
};



#define SHADOWS_POOL_SIZE 1024//896 //768
#define TEX_POOL_BLOCKLOGSIZE 5  // 32
#define TEX_POOL_BLOCKSIZE   (1 << TEX_POOL_BLOCKLOGSIZE)

struct SDynTexture_Shadow;
struct SDepthTexture;

//======================================================================
// Dynamic textures
struct SDynTexture
    : public IDynTexture
{
    static uint32           s_nMemoryOccupied;
    static SDynTexture      s_Root;

    SDynTexture*           m_Next;          //!<
    SDynTexture*           m_Prev;          //!<
    char                    m_sSource[128]; //!< pointer to the given name in the constructor call

    CTexture*              m_pTexture;
    ETEX_Format             m_eTF;
    ETEX_Type               m_eTT;
    uint32                  m_nWidth;
    uint32                  m_nHeight;
    uint32                  m_nReqWidth;
    uint32                  m_nReqHeight;
    uint32                  m_nTexFlags;
    uint32                  m_nFrameReset;


    bool                    m_bLocked;
    byte                    m_nUpdateMask;

    //////////////////////////////////////////////////////////////////////////
    // Shadow specific vars.
    //////////////////////////////////////////////////////////////////////////
    int m_nUniqueID;

    SDynTexture_Shadow* m_NextShadow;           //!<
    SDynTexture_Shadow* m_PrevShadow;           //!<

    ShadowMapFrustum* m_pFrustumOwner;
    IRenderNode* pLightOwner;
    int nObjectsRenderedCount;
    //////////////////////////////////////////////////////////////////////////

    SDynTexture(const char* szSource);
    SDynTexture(int nWidth, int nHeight, ETEX_Format eTF, ETEX_Type eTT, int nTexFlags, const char* szSource);
    ~SDynTexture();

    //////////////////////////////////////////////////////////////////////////
    // Custom new/delete for pool allocator.
    //////////////////////////////////////////////////////////////////////////
    ILINE void* operator new(size_t nSize);
    ILINE void operator delete(void* ptr);
    //////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////


    virtual int GetTextureID();
    virtual bool Update(int nNewWidth, int nNewHeight);
    bool RT_Update(int nNewWidth, int nNewHeight);
    virtual void Apply(int nTUnit, int nTS = -1);
    virtual bool ClearRT();
    virtual bool SetRT(int nRT, bool bPush, struct SDepthTexture* pDepthSurf, bool bScreenVP = false);
    virtual bool RT_SetRT(int nRT, int nWidth, int nHeight, bool bPush, bool bScreenVP = false);
    virtual bool SetRectStates() { return true; }
    virtual bool RestoreRT(int nRT, bool bPop);
    virtual ITexture* GetTexture() { return (ITexture*)m_pTexture; }
    virtual void Release() { delete this; }
    virtual void ReleaseForce();
    virtual void SetUpdateMask();
    virtual void ResetUpdateMask();
    virtual bool IsSecondFrame() { return m_nUpdateMask == 3; }
    virtual byte GetFlags() const { return 0; }
    virtual void GetSubImageRect(uint32& nX, uint32& nY, uint32& nWidth, uint32& nHeight)
    {
        nX = 0;
        nY = 0;
        nWidth = m_nWidth;
        nHeight = m_nHeight;
    }
    virtual void GetImageRect(uint32& nX, uint32& nY, uint32& nWidth, uint32& nHeight);
    virtual int GetWidth() { return m_nWidth; }
    virtual int GetHeight() { return m_nHeight; }

    void Lock() { m_bLocked = true; }
    void UnLock() { m_bLocked = false; }

    virtual void AdjustRealSize();
    virtual bool IsValid();

    _inline void UnlinkGlobal()
    {
        if (!m_Next || !m_Prev)
        {
            return;
        }
        m_Next->m_Prev = m_Prev;
        m_Prev->m_Next = m_Next;
        m_Next = m_Prev = NULL;
    }
    _inline void LinkGlobal(SDynTexture* Before)
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
    virtual void Unlink()
    {
        UnlinkGlobal();
    }
    virtual void Link()
    {
        LinkGlobal(&s_Root);
    }
    ETEX_Format GetFormat() { return m_eTF; }
    bool FreeTextures(bool bOldOnly, int nNeedSpace);

    typedef AZStd::multimap<unsigned int, CTexture*, AZStd::less<unsigned int>, AZ::AZStdAlloc<AZ::LegacyAllocator>> TextureSubset;
    typedef TextureSubset::iterator TextureSubsetItor;
    typedef AZStd::multimap<unsigned int, TextureSubset*, AZStd::less<unsigned int>, AZ::AZStdAlloc<AZ::LegacyAllocator>>  TextureSet;
    typedef TextureSet::iterator TextureSetItor;

    static TextureSet      s_availableTexturePool2D_BC1;
    static TextureSubset   s_checkedOutTexturePool2D_BC1;
    static TextureSet      s_availableTexturePool2D_R8G8B8A8;
    static TextureSubset   s_checkedOutTexturePool2D_R8G8B8A8;
    static TextureSet      s_availableTexturePool2D_R32F;
    static TextureSubset   s_checkedOutTexturePool2D_R32F;
    static TextureSet      s_availableTexturePool2D_R16F;
    static TextureSubset   s_checkedOutTexturePool2D_R16F;

    static TextureSet      s_availableTexturePool2D_R16G16F;
    static TextureSubset   s_checkedOutTexturePool2D_R16G16F;
    static TextureSet      s_availableTexturePool2D_R8G8B8A8S;
    static TextureSubset   s_checkedOutTexturePool2D_R8G8B8A8S;
    static TextureSet      s_availableTexturePool2D_R16G16B16A16F;
    static TextureSubset   s_checkedOutTexturePool2D_R16G16B16A16F;
    static TextureSet      s_availableTexturePool2D_R10G10B10A2;
    static TextureSubset   s_checkedOutTexturePool2D_R10G10B10A2;

    static TextureSet      s_availableTexturePool2D_R11G11B10F;
    static TextureSubset   s_checkedOutTexturePool2D_R11G11B10F;
    static TextureSet      s_availableTexturePoolCube_R11G11B10F;
    static TextureSubset   s_checkedOutTexturePoolCube_R11G11B10F;
    static TextureSet      s_availableTexturePool2D_R8G8S;
    static TextureSubset   s_checkedOutTexturePool2D_R8G8S;
    static TextureSet      s_availableTexturePoolCube_R8G8S;
    static TextureSubset   s_checkedOutTexturePoolCube_R8G8S;

    static TextureSet    s_availableTexturePool2D_Shadows[SBP_MAX];
    static TextureSubset s_checkedOutTexturePool2D_Shadows[SBP_MAX];
    static TextureSet    s_availableTexturePoolCube_Shadows[SBP_MAX];
    static TextureSubset s_checkedOutTexturePoolCube_Shadows[SBP_MAX];

    static TextureSet      s_availableTexturePool2DCustom_R16G16F;
    static TextureSubset   s_checkedOutTexturePool2DCustom_R16G16F;

    static TextureSet      s_availableTexturePoolCube_BC1;
    static TextureSubset   s_checkedOutTexturePoolCube_BC1;
    static TextureSet      s_availableTexturePoolCube_R8G8B8A8;
    static TextureSubset   s_checkedOutTexturePoolCube_R8G8B8A8;
    static TextureSet      s_availableTexturePoolCube_R32F;
    static TextureSubset   s_checkedOutTexturePoolCube_R32F;
    static TextureSet      s_availableTexturePoolCube_R16F;
    static TextureSubset   s_checkedOutTexturePoolCube_R16F;
    static TextureSet      s_availableTexturePoolCube_R16G16F;
    static TextureSubset   s_checkedOutTexturePoolCube_R16G16F;
    static TextureSet      s_availableTexturePoolCube_R8G8B8A8S;
    static TextureSubset   s_checkedOutTexturePoolCube_R8G8B8A8S;
    static TextureSet      s_availableTexturePoolCube_R16G16B16A16F;
    static TextureSubset   s_checkedOutTexturePoolCube_R16G16B16A16F;

    static TextureSet      s_availableTexturePoolCube_R10G10B10A2;
    static TextureSubset   s_checkedOutTexturePoolCube_R10G10B10A2;

    static TextureSet      s_availableTexturePoolCubeCustom_R16G16F;
    static TextureSubset   s_checkedOutTexturePoolCubeCustom_R16G16F;

    static uint32 s_iNumTextureBytesCheckedOut;
    static uint32 s_iNumTextureBytesCheckedIn;

    static uint32 s_SuggestedDynTexAtlasCloudsMaxsize;
    static uint32 s_SuggestedTexAtlasSize;
    static uint32 s_SuggestedDynTexMaxSize;
    static uint32 s_CurDynTexAtlasCloudsMaxsize;
    static uint32 s_CurTexAtlasSize;
    static uint32 s_CurDynTexMaxSize;

    CTexture* CreateDynamicRT();
    CTexture* GetDynamicRT();
    void ReleaseDynamicRT(bool bForce);

    EShadowBuffers_Pool ConvertTexFormatToShadowsPool(ETEX_Format e);

    static bool FreeAvailableDynamicRT(int nNeedSpace, TextureSet* pSet, bool bOldOnly);

    static void ShutDown();

    static void Init();
    static void Tick();

    void GetMemoryUsage(ICrySizer* pSizer) const
    {
        pSizer->AddObject(this, sizeof(*this));
    }
};


typedef stl::PoolAllocatorNoMT<sizeof(SDynTexture)> SDynTexture_PoolAlloc;
extern SDynTexture_PoolAlloc* g_pSDynTexture_PoolAlloc;

//////////////////////////////////////////////////////////////////////////
// Custom new/delete for pool allocator.
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
inline void* SDynTexture::operator new(size_t nSize)
{
    void* ptr = g_pSDynTexture_PoolAlloc->Allocate();
    if (ptr)
    {
        memset(ptr, 0, nSize); // Clear objects memory.
    }
    return ptr;
}
//////////////////////////////////////////////////////////////////////////
inline void SDynTexture::operator delete(void* ptr)
{
    if (ptr)
    {
        g_pSDynTexture_PoolAlloc->Deallocate(ptr);
    }
}


//==============================================================================

enum ETexPool : int
{
    eTP_Clouds,
    eTP_Sprites,

    eTP_Max
};

struct STextureSetFormat
{
    struct SDynTexture2*    m_pRoot;
    ETEX_Format             m_eTF;
    ETexPool                m_eTexPool;
    ETEX_Type               m_eTT;
    int                     m_nTexFlags;
    std::vector<CPowerOf2BlockPacker*> m_TexPools;

    STextureSetFormat(ETEX_Format eTF, ETexPool eTexPool, uint32 nTexFlags)
    {
        m_eTF = eTF;
        m_eTexPool = eTexPool;
        m_eTT = eTT_2D;
        m_pRoot = NULL;
        m_nTexFlags = nTexFlags;
    }
    ~STextureSetFormat();
};

struct SDynTexture2
    : public IDynTexture
{
#ifndef _DEBUG
    char* m_sSource;                        //!< pointer to the given name in the constructor call
#else
    char                   m_sSource[128];      //!< pointer to the given name in the constructor call
#endif
    STextureSetFormat* m_pOwner;
    CPowerOf2BlockPacker* m_pAllocator;
    CTexture* m_pTexture;
    uint32                 m_nBlockID;
    ETexPool               m_eTexPool;

    SDynTexture2* m_Next;                   //!<
    SDynTexture2** m_PrevLink;              //!<

    void UnlinkGlobal()
    {
        if (m_Next)
        {
            m_Next->m_PrevLink = m_PrevLink;
        }
        if (m_PrevLink)
        {
            *m_PrevLink = m_Next;
        }
    }
    void LinkGlobal(SDynTexture2*& Before)
    {
        if (Before)
        {
            Before->m_PrevLink = &m_Next;
        }
        m_Next = Before;
        m_PrevLink = &Before;
        Before = this;
    }

    void Link()
    {
        LinkGlobal(m_pOwner->m_pRoot);
    }

    void Unlink()
    {
        UnlinkGlobal();
        m_Next = NULL;
        m_PrevLink = NULL;
    }

    bool Remove();
    virtual bool IsValid();
    bool _IsValid() { return IsValid(); }

    SDynTexture2(uint32 nWidth, uint32 nHeight, uint32 nTexFlags, const char* szSource, ETexPool eTexPool);
    SDynTexture2(const char* szSource, ETexPool eTexPool);
    ~SDynTexture2();

    bool UpdateAtlasSize(int nNewWidth, int nNewHeight);
    void ReleaseForce();

    virtual bool Update(int nNewWidth, int nNewHeight);
    virtual void Apply(int nTUnit, int nTS = -1);
    virtual bool ClearRT();
    virtual bool SetRT(int nRT, bool bPush, SDepthTexture* pDepthSurf, bool bScreenVP = false);
    virtual bool SetRectStates();
    virtual bool RestoreRT(int nRT, bool bPop);
    virtual ITexture* GetTexture() { return (ITexture*)m_pTexture; }
    ETEX_Format GetFormat() { return m_pOwner->m_eTF; }
    virtual void SetUpdateMask();
    virtual void ResetUpdateMask();
    virtual bool IsSecondFrame() { return m_nUpdateMask == 3; }
    virtual byte GetFlags() const { return m_nFlags; }
    virtual void SetFlags(byte flags) { m_nFlags = flags; }

    // IDynTexture implementation
    virtual void Release() { delete this; }
    virtual void GetSubImageRect(uint32& nX, uint32& nY, uint32& nWidth, uint32& nHeight)
    {
        nX = m_nX;
        nY = m_nY;
        nWidth = m_nWidth;
        nHeight = m_nHeight;
    }

    virtual void GetImageRect(uint32& nX, uint32& nY, uint32& nWidth, uint32& nHeight);
    virtual int GetTextureID();
    virtual void Lock() { m_bLocked = true; }
    virtual void UnLock() { m_bLocked = false; }
    virtual int GetWidth() { return m_nWidth; }
    virtual int GetHeight() { return m_nHeight; }

    typedef std::map<uint32, STextureSetFormat*>  TextureSet2;
    typedef TextureSet2::iterator TextureSet2Itor;

    static AZStd::unique_ptr<TextureSet2>      s_TexturePool[eTP_Max];
    static int s_nMemoryOccupied[eTP_Max];

    static void ShutDown();
    static void Init(ETexPool eTexPool);

    static int GetPoolMaxSize(ETexPool eTexPool);
    static void SetPoolMaxSize(ETexPool eTexPool, int nSize, bool bWarn);
    static const char* GetPoolName(ETexPool eTexPool);
    static ETEX_Format GetPoolTexFormat(ETexPool eTexPool);
    static int GetPoolTexNum(ETexPool eTexPool)
    {
        int nT = 0;
        for (TextureSet2Itor it = s_TexturePool[eTexPool]->begin(); it != s_TexturePool[eTexPool]->end(); it++)
        {
            STextureSetFormat* pF = it->second;
            nT += pF->m_TexPools.size();
        }
        return nT;
    }

    uint32 m_nX;
    uint32 m_nY;
    uint32 m_nWidth;
    uint32 m_nHeight;

    bool m_bLocked;
    byte m_nFlags;
    byte m_nUpdateMask;  // Crossfire odd/even frames
    uint32 m_nFrameReset;
    uint32 m_nAccessFrame;
};

//////////////////////////////////////////////////////////////////////////
// Dynamic texture for the shadow.
// This class must not contain any non static member variables,
//  because SDynTexture allocated used constant size pool.
//////////////////////////////////////////////////////////////////////////
struct SDynTexture_Shadow
    : public SDynTexture
{
    static SDynTexture_Shadow  s_RootShadow;

    //////////////////////////////////////////////////////////////////////////
    SDynTexture_Shadow(int nWidth, int nHeight, ETEX_Format eTF, ETEX_Type eTT, int nTexFlags, const char* szSource);
    SDynTexture_Shadow(const char* szSource);
    ~SDynTexture_Shadow();

    _inline void UnlinkShadow()
    {
        //assert(gRenDev->m_pRT->IsRenderThread());
        if (!m_NextShadow || !m_PrevShadow)
        {
            return;
        }
        m_NextShadow->m_PrevShadow = m_PrevShadow;
        m_PrevShadow->m_NextShadow = m_NextShadow;
        m_NextShadow = m_PrevShadow = NULL;
    }
    _inline void LinkShadow(SDynTexture_Shadow* Before)
    {
        assert(gRenDev->m_pRT->IsRenderThread());
        if (m_NextShadow || m_PrevShadow)
        {
            return;
        }
        m_NextShadow = Before->m_NextShadow;
        Before->m_NextShadow->m_PrevShadow = this;
        Before->m_NextShadow = this;
        m_PrevShadow = Before;
    }

    SDynTexture_Shadow* GetByID(int nID)
    {
        assert(gRenDev->m_pRT->IsRenderThread());
        SDynTexture_Shadow* pTX = SDynTexture_Shadow::s_RootShadow.m_NextShadow;
        for (pTX = SDynTexture_Shadow::s_RootShadow.m_NextShadow; pTX != &SDynTexture_Shadow::s_RootShadow; pTX = pTX->m_NextShadow)
        {
            if (pTX->m_nUniqueID == nID)
            {
                return pTX;
            }
        }
        return NULL;
    }
    static void RT_EntityDelete(IRenderNode* pRenderNode);

    virtual void Unlink()
    {
        assert(gRenDev->m_pRT->IsRenderThread());
        UnlinkGlobal();
        UnlinkShadow();
    }
    virtual void Link()
    {
        assert(gRenDev->m_pRT->IsRenderThread());
        LinkGlobal(&s_Root);
        LinkShadow(&s_RootShadow);
    }
    virtual void AdjustRealSize();

    void GetMemoryUsage(ICrySizer* pSizer) const
    {
        SDynTexture::GetMemoryUsage(pSizer);
    }

    static SDynTexture_Shadow* GetForFrustum(ShadowMapFrustum* pFrustum);

    static void ShutDown();
};

//==========================================================================
// Texture

struct STexCacheFileHeader
{
    int8 m_nSides;
    int8 m_nMipsPersistent;
    STexCacheFileHeader()
    {
        m_nSides = 0;
        m_nMipsPersistent = 0;
    }
};

struct SMipData
{
public:
    byte* DataArray; // Data.
    bool m_bNative : 1;

    SMipData()
        : m_bNative(false)
        , DataArray(NULL)
    {}
    ~SMipData()
    {
        Free();
    }

    void Free()
    {
        SAFE_DELETE_ARRAY(DataArray);
        m_bNative = false;
    }
    void Init(int InSize, [[maybe_unused]] int nWidth, [[maybe_unused]] int nHeight)
    {
        assert(DataArray == NULL);
        DataArray = new byte[InSize];
        m_bNative = false;
    }
};

struct STexPool;
struct STexPoolItem;

struct STexStreamRoundInfo
{
    STexStreamRoundInfo()
    {
        nRoundUpdateId = -1;
        bHighPriority = false;
        bLastHighPriority = false;
    }

    int32 nRoundUpdateId : 30;
    uint32 bHighPriority : 1;
    uint32 bLastHighPriority : 1;
};

struct STexStreamZoneInfo
{
    STexStreamZoneInfo()
    {
        fMinMipFactor = fLastMinMipFactor = 1000000.f;
    }

    float fMinMipFactor;
    float fLastMinMipFactor;
};

struct STexMipHeader
{
    uint32 m_SideSizeWithMips : 31;
    uint32 m_InPlaceStreamable : 1;
    uint32 m_SideSize : 29;
    uint32 m_eMediaType : 3;
#if defined(TEXSTRM_STORE_DEVSIZES)
    uint32 m_DevSideSizeWithMips;
#endif
    SMipData* m_Mips;
    STexMipHeader()
    {
        m_SideSizeWithMips = 0;
        m_InPlaceStreamable = 0;
        m_SideSize = 0;
        m_eMediaType = 0;
#if defined(TEXSTRM_STORE_DEVSIZES)
        m_DevSideSizeWithMips = 0;
#endif
        m_Mips = NULL;
    }
    ~STexMipHeader()
    {
        SAFE_DELETE_ARRAY(m_Mips);
    }
};

struct STexStreamingInfo
{
    STexStreamZoneInfo m_arrSPInfo[MAX_PREDICTION_ZONES];

    STexPoolItem* m_pPoolItem;

    uint32 m_nSrcStart;

    STexMipHeader* m_pMipHeader;
    DDSSplitted::DDSDesc m_desc;
    float m_fMinMipFactor;

    STexStreamingInfo(){}
    STexStreamingInfo(const uint8 nMips)
    {
        m_pPoolItem = NULL;
        // +1 to accomodate queries for the size of the texture with no mips
        m_pMipHeader = new STexMipHeader[nMips + 1];
        m_nSrcStart = 0;
        m_fMinMipFactor = 0.0f;
    }

    ~STexStreamingInfo()
    {
        SAFE_DELETE_ARRAY(m_pMipHeader);
    }

    void GetMemoryUsage(ICrySizer* pSizer, int nMips, int nSides) const
    {
        pSizer->AddObject(this, sizeof(*this));
        pSizer->AddObject(m_pMipHeader, sizeof(STexMipHeader) * nMips);
        for (int i = 0; i < nMips; i++)
        {
            pSizer->AddObject(m_pMipHeader[i].m_Mips, sizeof(SMipData) * nSides);
            for (int j = 0; j < nSides; j++)
            {
                pSizer->AddObject(m_pMipHeader[i].m_Mips[j].DataArray, m_pMipHeader[i].m_SideSize);
            }
        }
    }

    int GetSize(int nMips, int nSides)
    {
        int nSize = sizeof(*this);
        for (int i = 0; i < nMips; i++)
        {
            nSize += sizeof(m_pMipHeader[i]);
            for (int j = 0; j < nSides; j++)
            {
                nSize += sizeof(m_pMipHeader[i].m_Mips[j]);
                if (m_pMipHeader[i].m_Mips[j].DataArray)
                {
                    nSize += m_pMipHeader[i].m_SideSize;
                }
            }
        }
        return nSize;
    }

    void* operator new ([[maybe_unused]] size_t sz, void* p){ return p; }
    void operator delete ([[maybe_unused]] void* ptr, [[maybe_unused]] void* p){}

private:
    void* operator new (size_t sz);
    void operator delete ([[maybe_unused]] void* ptr){__debugbreak(); }
};

struct STexStreamInMipState
{
    uint32 m_nSideDelta : 28;
    bool m_bStreamInPlace : 1;
    bool m_bExpanded : 1;
    bool m_bUploaded : 1;

    STexStreamInMipState()
    {
        m_nSideDelta = 0;
        m_bStreamInPlace = false;
        m_bExpanded = false;
        m_bUploaded = false;
    }
};

struct STexStreamInState
    : public IStreamCallback
{
public:
    enum
    {
        MaxMips = 12,
        MaxStreams = MaxMips * 6,
    };

public:
    STexStreamInState();

    void Reset();
    bool TryCommit();
    virtual void StreamAsyncOnComplete (IReadStream* pStream, unsigned nError);

#ifdef TEXSTRM_ASYNC_TEXCOPY
    void CopyMips();
#endif

public:
    CTexture*              m_pTexture;
    STexPoolItem*          m_pNewPoolItem;
    volatile int                        m_nAsyncRefCount;
    uint8                                     m_nHigherUploadedMip;
    uint8                                     m_nLowerUploadedMip;
    uint8                                     m_nActivateMip;
    bool                                        m_bAborted;
    bool                                        m_bValidLowMips;
    volatile bool                       m_bAllStreamsComplete;
#if defined(TEXSTRM_COMMIT_COOLDOWN)
    int                                         m_nStallFrames;
#endif

#ifndef _RELEASE
    float                                       m_fStartTime;
#endif
#if defined(TEXSTRM_DEFERRED_UPLOAD)
    ID3D11CommandList*          m_pCmdList;
#endif

#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION TEXTURE_H_SECTION_1
    #include AZ_RESTRICTED_FILE(Texture_h)
#endif

    IReadStreamPtr                  m_pStreams[MaxStreams];
    STexStreamInMipState        m_mips[MaxMips];
};

struct STexStreamPrepState
    : IImageFileStreamCallback
{
public:
    STexStreamPrepState()
        : m_bCompleted(false)
        , m_bFailed(false)
        , m_bNeedsFinalise(false)
    {
    }

    bool Commit();

public: // IImageFileStreamCallback Members
    virtual void OnImageFileStreamComplete(CImageFile* pImFile);

public:
    _smart_ptr<CTexture> m_pTexture;
    _smart_ptr<CImageFile> m_pImage;
    volatile bool m_bCompleted;
    volatile bool m_bFailed;
    volatile bool m_bNeedsFinalise;

private:
    STexStreamPrepState(const STexStreamPrepState&);
    STexStreamPrepState& operator = (const STexStreamPrepState&);
};

#ifdef TEXSTRM_ASYNC_TEXCOPY
struct STexStreamOutState
{
public:
    void Reset();
    bool TryCommit();

    void CopyMips();

public:
    AZ::LegacyJobExecutor m_jobExecutor;

    CTexture*                               m_pTexture;
    STexPoolItem*                       m_pNewPoolItem;
    uint8                                       m_nStartMip;
    volatile bool                       m_bDone;
    volatile bool                       m_bAborted;

#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION TEXTURE_H_SECTION_2
    #include AZ_RESTRICTED_FILE(Texture_h)
#endif
};
#endif

//===================================================================

struct SEnvTexture
{
    bool m_bInprogress;
    bool m_bReady;
    bool m_bWater;
    bool m_bReflected;
    bool m_bReserved;
    int m_MaskReady;
    int m_Id;
    int m_TexSize;
    // Result Cube-Map or 2D RT texture
    SDynTexture* m_pTex;
    float m_TimeLastUpdated;
    Vec3 m_CamPos;
    Vec3 m_ObjPos;
    Ang3 m_Angle;
    int m_nFrameReset;
    Matrix44 m_Matrix;
    //void *m_pQuery[6];
    //void *m_pRenderTargets[6];

    // Cube maps average colors (used for RT radiosity)
    int m_nFrameCreated[6];
    ColorF m_EnvColors[6];

    SEnvTexture()
    {
        m_bInprogress = false;
        m_bReady = false;
        m_bWater = false;
        m_bReflected = false;
        m_bReserved = false;
        m_MaskReady = 0;
        m_Id = 0;
        m_pTex = NULL;
        m_nFrameReset = -1;
        m_CamPos.Set(0.0f, 0.0f, 0.0f);
        m_ObjPos.Set(0.0f, 0.0f, 0.0f);
        m_Angle.Set(0.0f, 0.0f, 0.0f);

        for (int i = 0; i < 6; i++)
        {
            //m_pQuery[i] = NULL;
            m_nFrameCreated[i] = -1;
            //m_pRenderTargets[i] = NULL;
        }
    }

    void Release();
    void ReleaseDeviceObjects();
    void RT_SetMatrix();
};


//===============================================================================

struct SPixFormat
{
    // Pixel format info.
    D3DFormat       DeviceFormat;// Pixel format from Direct3D.
    const char*     Desc;   // Stat: Human readable name for stats.
    int8            BytesPerBlock;// Total bits per pixel.
    uint8           bCanDS : 1;
    uint8           bCanRT : 1;
    uint8           bCanMultiSampleRT : 1;
    uint8           bCanMipsAutoGen : 1;
    uint8           bCanReadSRGB : 1;
    uint8           bCanGather : 1;
    uint8           bCanGatherCmp : 1;
    uint8           bCanBlend : 1;
    int16           MaxWidth;
    int16           MaxHeight;
    SPixFormat* Next;

    SPixFormat()
    {
        Init();
    }
    void Init()
    {
        BytesPerBlock = 0;
        DeviceFormat = (D3DFormat)0;
        Desc = NULL;
        Next = NULL;
        bCanDS = false;
        bCanRT = false;
        bCanMultiSampleRT = false;
        bCanMipsAutoGen = false;
        bCanReadSRGB = false;
        bCanGather = false;
        bCanGatherCmp = false;
        bCanBlend = false;
    }
    bool IsValid() const
    {
        if (BytesPerBlock)
        {
            return true;
        }
        return false;
    }
    bool CheckSupport(D3DFormat Format, const char* szDescr, ETexture_Usage eTxUsage = eTXUSAGE_AUTOGENMIPMAP);
};

struct SPixFormatSupport
{
    SPixFormat        m_FormatA8;                  //8 bit alpha
    SPixFormat        m_FormatR8;
    SPixFormat        m_FormatR8S;
    SPixFormat        m_FormatR8G8;                //16 bit
    SPixFormat        m_FormatR8G8S;               //16 bit
    SPixFormat        m_FormatR8G8B8A8;            //32 bit
    SPixFormat        m_FormatR8G8B8A8S;           //32 bit
    SPixFormat        m_FormatR10G10B10A2;         //32 bit

    SPixFormat        m_FormatR16;                 //16 bit
    SPixFormat        m_FormatR16U;                //16 bit
    SPixFormat        m_FormatR16G16U;             //32 bit
    SPixFormat        m_FormatR10G10B10A2UI;       //32 bit
    SPixFormat        m_FormatR16F;                //16 bit
    SPixFormat        m_FormatR32F;                //32 bit
    SPixFormat        m_FormatR16G16;              //32 bit
    SPixFormat        m_FormatR16G16S;             //32 bit
    SPixFormat        m_FormatR16G16F;             //32 bit
    SPixFormat        m_FormatR11G11B10F;          //32 bit
    SPixFormat        m_FormatR16G16B16A16F;       //64 bit
    SPixFormat        m_FormatR16G16B16A16;        //64 bit
    SPixFormat        m_FormatR16G16B16A16S;       //64 bit
    SPixFormat        m_FormatR32G32B32A32F;       //128 bit

    SPixFormat        m_FormatBC1;                 //Compressed RGB
    SPixFormat        m_FormatBC2;                 //Compressed RGBA
    SPixFormat        m_FormatBC3;                 //Compressed RGBA
    SPixFormat        m_FormatBC4U;                // ATI1, single channel compression, unsigned
    SPixFormat        m_FormatBC4S;                // ATI1, single channel compression, signed
    SPixFormat        m_FormatBC5U;                //
    SPixFormat        m_FormatBC5S;                //
    SPixFormat        m_FormatBC6UH;               //Compressed RGB
    SPixFormat        m_FormatBC6SH;               //Compressed RGB
    SPixFormat        m_FormatBC7;                 //Compressed RGBA

    SPixFormat        m_FormatR9G9B9E5;            //Shared exponent RGB

    SPixFormat        m_FormatEAC_R11;             //EAC compressed single channel for mobile
    SPixFormat        m_FormatEAC_RG11;            //EAC compressed dual channel for mobile
    SPixFormat        m_FormatETC2;                //ETC2 compressed RGB for mobile
    SPixFormat        m_FormatETC2A;               //ETC2a compressed RGBA for mobile

    SPixFormat        m_FormatD16;         //16bit fixed point depth
    SPixFormat        m_FormatD24S8;           //24bit fixed point depth + 8bit stencil
    SPixFormat        m_FormatD32F;                //32bit floating point depth
    SPixFormat        m_FormatD32FS8;              //32bit floating point depth + 8bit stencil

    SPixFormat        m_FormatB5G6R5;              //16 bit
    SPixFormat        m_FormatB5G5R5;              //16 bit
    SPixFormat        m_FormatB4G4R4A4;            //16 bit
    SPixFormat        m_FormatB8G8R8X8;            //32 bit
    SPixFormat        m_FormatB8G8R8A8;            //32 bit

#ifdef CRY_USE_METAL
    SPixFormat        m_FormatPVRTC2;               //ETC2 compressed RGB for mobile
    SPixFormat        m_FormatPVRTC4;               //ETC2a compressed RGBA for mobile
#endif
#if defined(ANDROID) || defined(CRY_USE_METAL)
    SPixFormat        m_FormatASTC_4x4;
    SPixFormat        m_FormatASTC_5x4;
    SPixFormat        m_FormatASTC_5x5;
    SPixFormat        m_FormatASTC_6x5;
    SPixFormat        m_FormatASTC_6x6;
    SPixFormat        m_FormatASTC_8x5;
    SPixFormat        m_FormatASTC_8x6;
    SPixFormat        m_FormatASTC_8x8;
    SPixFormat        m_FormatASTC_10x5;
    SPixFormat        m_FormatASTC_10x6;
    SPixFormat        m_FormatASTC_10x8;
    SPixFormat        m_FormatASTC_10x10;
    SPixFormat        m_FormatASTC_12x10;
    SPixFormat        m_FormatASTC_12x12;
#endif

    SPixFormat*       m_FirstPixelFormat;

    void CheckFormatSupport();
};

struct STexStageInfo
{
    int m_nCurState;
    CDeviceTexture* m_DevTexture;

    STexState  m_State;
    float            m_fMipBias;
#if !defined(NULL_RENDERER)
    D3DShaderResourceView* m_pCurResView;
    EHWShaderClass            m_eHWSC;
#endif

    STexStageInfo()
    {
        Flush();
    }
    void Flush()
    {
        m_nCurState = -1;
        m_State.m_nMipFilter = -1;
        m_State.m_nMinFilter = -1;
        m_State.m_nMagFilter = -1;
        m_State.m_nAddressU = -1;
        m_State.m_nAddressV = -1;
        m_State.m_nAddressW = -1;
        m_State.m_nAnisotropy = 0;
#if !defined(NULL_RENDERER)
        m_pCurResView = NULL;
        m_eHWSC = eHWSC_Pixel;
#endif
        m_DevTexture = NULL;
        m_fMipBias = 0.f;
    }
};


struct STexData
{
    const byte* m_pData[6];
    uint16 m_nWidth;
    uint16 m_nHeight;
    uint16 m_nDepth;
protected:
    uint8 m_reallocated;
public:
    ETEX_Format m_eTF;
    uint8 m_nMips;
    int m_nFlags;
    float m_fAvgBrightness;
    ColorF m_cMinColor;
    ColorF m_cMaxColor;
    const char* m_pFilePath;

    STexData()
    {
        m_pData[0] = m_pData[1] = m_pData[2] = m_pData[3] = m_pData[4] = m_pData[5] = 0;
        m_nWidth = 0;
        m_nHeight = 0;
        m_nDepth = 1;
        m_reallocated = 0;
        m_eTF = eTF_Unknown;
        m_nMips = 0;
        m_nFlags = 0;
        m_fAvgBrightness = 1.0f;
        m_cMinColor = 0.0f;
        m_cMaxColor = 1.0f;
        m_pFilePath = 0;
    }
    void AssignData(unsigned int i, const byte* pNewData)
    {
        assert(i < 6);
        if (WasReallocated(i))
        {
            delete [] m_pData[i];
        }
        m_pData[i] = pNewData;
        SetReallocated(i);
    }
    bool WasReallocated(unsigned int i) const
    {
        return (m_reallocated & (1 << i)) != 0;
    }
    void SetReallocated(unsigned int i)
    {
        m_reallocated |= (1 << i);
    }
};

struct SDepthTexture
{
    int nWidth;
    int nHeight;
    bool bBusy;
    int nFrameAccess;
    D3DTexture* pTarget;
    D3DDepthSurface* pSurf;
    CTexture* pTex;
    SDepthTexture()
        : nWidth(0)
        , nHeight(0)
        , bBusy(false)
        , nFrameAccess(-1)
        , pTarget(nullptr)
        , pSurf(nullptr)
        , pTex(nullptr)
    {}

    void Release(bool bReleaseTex = false);

    ~SDepthTexture();
};


struct DirtyRECT
{
    RECT srcRect;
    uint16 dstX;
    uint16 dstY;
};


struct SResourceView
{
    typedef uint64 KeyType;

    static const KeyType DefaultView = (KeyType) - 1LL;
    static const KeyType DefaultViewMS = (KeyType) - 2LL;
    static const KeyType DefaultViewSRGB = (KeyType) - 3LL;

    enum ResourceViewType
    {
        eShaderResourceView = 0,
        eRenderTargetView,
        eDepthStencilView,
        eUnorderedAccessView,

        eNumResourceViews
    };

    union ResourceViewDesc
    {
        struct
        {
            uint64 eViewType        : 3;
            uint64 nFormat          : 7;
            uint64 nFirstSlice      : 11;
            uint64 nSliceCount      : 11;
            uint64 nMostDetailedMip : 4;
            uint64 nMipCount        : 4;
            uint64 bSrgbRead        : 1;
            uint64 bMultisample     : 1;
            uint64 nFlags           : 2;
            uint64 nUnused          : 20;
        };

        KeyType Key;
    };

    SResourceView(uint64 nKey = DefaultView)
    {
        COMPILE_TIME_ASSERT(sizeof(m_Desc) <= sizeof(KeyType));

        m_Desc.Key = nKey;
        m_pDeviceResourceView = NULL;
    }

    static SResourceView ShaderResourceView(ETEX_Format nFormat, int nFirstSlice = 0, int nSliceCount = -1, int nMostDetailedMip = 0, int nMipCount = -1, bool bSrgbRead = false, bool bMultisample = false);
    static SResourceView RenderTargetView(ETEX_Format nFormat, int nFirstSlice = 0, int nSliceCount = -1, int nMipLevel = 0, bool bMultisample = false);
    static SResourceView DepthStencilView(ETEX_Format nFormat, int nFirstSlice = 0, int nSliceCount = -1, int nMipLevel = 0, int nFlags = 0, bool bMultisample = false);
    static SResourceView UnorderedAccessView(ETEX_Format nFormat, int nFirstSlice = 0, int nSliceCount = -1, int nMipLevel = 0, int nFlags = 0);

    bool operator==(const SResourceView& other) const
    {
        return m_Desc.Key == other.m_Desc.Key;
    }

    ResourceViewDesc m_Desc;
    void* m_pDeviceResourceView;
};

// properties of the render targets ONLY
struct RenderTargetData
{
    int m_nRTSetFrameID;                    // last read access, compare with GetFrameID(false)
    struct
    {
        uint8 m_nMSAASamples : 4;
        uint8 m_nMSAAQuality : 4;
    };
#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION TEXTURE_H_SECTION_3
    #include AZ_RESTRICTED_FILE(Texture_h)
#endif
    TArray<SResourceView> m_ResourceViews;
    CDeviceTexture* m_pDeviceTextureMSAA;

    RenderTargetData()
    {
        memset(this, 0, sizeof(*this));
        m_nRTSetFrameID = -1;
#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION TEXTURE_H_SECTION_4
    #include AZ_RESTRICTED_FILE(Texture_h)
#endif
    }
    ~RenderTargetData();
};

//////////////////////////////////////////////////////////////////////////

struct SStreamFormatCodeKey
{
    union
    {
        struct
        {
            uint16 nWidth;
            uint16 nHeight;
            ETEX_Format fmt;
            uint8 nTailMips;
        } s;
        uint64 u;
    };

    SStreamFormatCodeKey(uint32 nWidth, uint32 nHeight, ETEX_Format fmt, uint8 nTailMips)
    {
        u = 0;
        s.nWidth = (uint16)nWidth;
        s.nHeight = (uint16)nHeight;
        s.fmt = fmt;
        s.nTailMips = nTailMips;
    }

    friend bool operator < (const SStreamFormatCodeKey& a, const SStreamFormatCodeKey& b)
    {
        return a.u < b.u;
    }
};

struct SStreamFormatSize
{
    uint32 size : 31;
    uint32 alignSlices : 1;
};

struct SStreamFormatCode
{
    enum
    {
        MaxMips = 14
    };
    SStreamFormatSize sizes[MaxMips];
};

//////////////////////////////////////////////////////////////////////////

class CTexture
    : public ITexture
    , public CBaseResource
{
    friend struct SDynTexture;
    friend struct SDynTexture2;
    friend class CD3D9Renderer;
    friend struct STexStreamInState;
    friend struct STexStreamOutState;
    friend class ITextureStreamer;
    friend class CPlanningTextureStreamer;
    friend struct SPlanningTextureOrder;
    friend struct SPlanningTextureRequestOrder;
    friend struct STexPoolItem;
    friend struct CompareStreamingPrioriry;
    friend struct CompareStreamingPrioriryInv;

public:
    virtual void ApplyTexture(int nTUnit, int nState = -1) override
    {
        Apply(nTUnit, nState);
    }

    virtual void Apply(int nTUnit, int nState = -1, int nTexMatSlot = EFTT_UNKNOWN, int nSUnit = -1, SResourceView::KeyType nResViewKey = SResourceView::DefaultView, EHWShaderClass eHWSC = eHWSC_Pixel);

private:
    D3DSurface* m_pDeviceRTV;
    D3DSurface* m_pDeviceRTVMS;

    CDeviceTexture* m_pDevTexture;
    const SPixFormat* m_pPixelFormat;

    // Start block that should fit in one cache-line
    // Reason is to minimize cache misses during texture streaming update job
    // Note: This is checked COMPILE_TIME_ASSERTS in ~CTexture implementation (Texture.cpp)

    STexStreamingInfo* m_pFileTexMips;      // properties of the streamable texture ONLY

    uint16 m_nWidth;
    uint16 m_nHeight;
    uint16 m_nDepth;
    ETEX_Format m_eTFDst;
    ETEX_TileMode m_eSrcTileMode;
    uint8 m_nStreamFormatCode;

    uint32 m_nFlags;                                // e.g. FT_USAGE_DYNAMIC

    bool m_bStreamed : 1;
    bool m_bStreamPrepared : 1;
    bool m_bStreamRequested : 1;
    bool m_bWasUnloaded : 1;
    bool m_bPostponed : 1;
    bool m_bForceStreamHighRes : 1;
    bool m_bNeedRestoring : 1;
    bool m_bNoDevTexture : 1;

    bool m_bNoTexture : 1;
    bool m_bResolved : 1;
    bool m_bUseMultisampledRTV : 1; // Allows switching rendering between multisampled/non-multisampled rendertarget views
    bool m_bHighQualityFiltering : 1;
    bool m_bCustomFormat : 1;   // Allow custom texture formats - for faster texture fetches
    bool m_bVertexTexture : 1;
    bool m_bUseDecalBorderCol : 1;
    bool m_bIsSRGB : 1;

    bool m_bAsyncDevTexCreation : 1;
    bool m_bInDistanceSortedList : 1;
    bool m_bCreatedInLevel : 1;
    bool m_bUsedRecently : 1;
    bool m_bStatTracked : 1;
    bool m_bStreamHighPriority : 1;
    uint8 m_renderTargetUseCount : CTEXTURE_RENDER_TARGET_USE_COUNT_NUMBER_BITS; // Not a bool because a render target can be pushed on the RTStack multiple times

    int8 m_nCustomID;

    uint8 m_nArraySize;
    int8 m_nMips;
    ETEX_Type m_eTT;

    ETEX_Format m_eTFSrc;
    int8 m_nStreamingPriority;
    int8 m_nMinMipVidUploaded;
    int8 m_nMinMipVidActive;

    uint16 m_nStreamSlot;
    int16 m_fpMinMipCur;

    STexCacheFileHeader m_CacheFileHeader;
    uint16 m_nDefState;

    int32 m_nActualSize;
    int32 m_nPersistentSize;

    int m_nAccessFrameID;                                   // last read access, compare with GetFrameID(false)
    STexStreamRoundInfo m_streamRounds[MAX_PREDICTION_ZONES];

    // End block that should fit in one cache-line

    DynArray<STexComposition> m_composition;
    float   m_fCurrentMipBias;                          // streaming mip fading
    float m_fAvgBrightness;
    ColorF m_cMinColor;
    ColorF m_cMaxColor;
    ColorF m_cClearColor;

    RenderTargetData* m_pRenderTargetData;

    string m_SrcName;

#ifndef _RELEASE
    string m_sAssetScopeName;
#endif
    D3DShaderResourceView* m_pDeviceShaderResource;
    D3DShaderResourceView* m_pDeviceShaderResourceSRGB;

    typedef AZStd::function<void(uint32)> InvalidateCallbackType;
    AZStd::unordered_multimap<void*, InvalidateCallbackType> m_invalidateCallbacks;

    AZStd::mutex* m_invalidateCallbacksMutex = nullptr;
    static StaticInstance<AZStd::mutex> m_staticInvalidateCallbacksMutex;

    bool m_bisTextureMissing = false;

public:
    int m_nUpdateFrameID;                   // last write access, compare with GetFrameID(false)

private:

#ifdef TEXTURE_GET_SYSTEM_COPY_SUPPORT
    struct SLowResSystemCopy
    {
        SLowResSystemCopy()
        {
            m_nLowResSystemCopyAtlasId = 0;
            m_nLowResCopyWidth = m_nLowResCopyHeight = 0;
        }
        PodArray<ColorB> m_lowResSystemCopy;
        uint16 m_nLowResCopyWidth, m_nLowResCopyHeight;
        int m_nLowResSystemCopyAtlasId;
    };
    typedef std::map<const CTexture*, SLowResSystemCopy> LowResSystemCopyType;
    static StaticInstance<LowResSystemCopyType> s_LowResSystemCopy;
    void PrepareLowResSystemCopy(const byte* pTexData, bool bTexDataHasAllMips);
    const ColorB* GetLowResSystemCopy(uint16& nWidth, uint16& nHeight, int** ppLowResSystemCopyAtlasId);
#endif

    static CCryNameTSCRC GenName(const char* name, uint32 nFlags = 0);
    static CTexture* NewTexture(const char* name, uint32 nFlags, ETEX_Format eTFDst, bool& bFound);

    ETEX_Format FormatFixup(ETEX_Format src);
    bool FormatFixup(STexData& td);
    bool ImagePreprocessing(STexData& td);

#ifndef _RELEASE
    static void OutputDebugInfo();
#endif

    static CCryNameTSCRC    s_sClassName;

protected:
    virtual ~CTexture();

public:
    //! Dirty flags will indicate what kind of device data was invalidated
    enum EDeviceDirtyFlags
    {
        eDeviceResourceDirty = BIT(0),
        eDeviceResourceViewDirty = BIT(1),
    };

public:
    CTexture(const uint32 nFlags)
    {
        m_nFlags = nFlags;
        m_eTFDst = eTF_Unknown;
        m_eTFSrc = eTF_Unknown;
        m_nMips = 1;
        m_nWidth = 0;
        m_nHeight = 0;
        m_eTT = eTT_2D;
        m_nDepth = 1;
        m_nArraySize = 1;
        m_nActualSize = 0;
        m_fAvgBrightness = 1.0f;
        m_cMinColor = 0.0f;
        m_cMaxColor = 1.0f;
        m_cClearColor = ColorF(0.0f, 0.0f, 0.0f, 1.0f);
        m_nPersistentSize = 0;
        m_fAvgBrightness = 0.0f;

        m_nUpdateFrameID = -1;
        m_nAccessFrameID = -1;
        m_nCustomID = -1;
        m_pPixelFormat = NULL;
        m_pDevTexture = NULL;
        m_pDeviceRTV = NULL;
        m_pDeviceRTVMS = NULL;
        m_pDeviceShaderResource = NULL;
        m_pDeviceShaderResourceSRGB = NULL;

        m_bAsyncDevTexCreation = false;

        m_renderTargetUseCount = 0;
        m_bNeedRestoring = false;
        m_bNoTexture = false;
        m_bResolved = true;
        m_bUseMultisampledRTV = true;
        m_bHighQualityFiltering = false;
        m_bCustomFormat = false;
        m_eSrcTileMode = eTM_None;

        m_bPostponed = false;
        m_bForceStreamHighRes = false;
        m_bWasUnloaded = false;
        m_bStreamed = false;
        m_bStreamPrepared = false;
        m_bStreamRequested = false;
        m_bVertexTexture = false;
        m_bUseDecalBorderCol = false;
        m_bIsSRGB = false;
        m_bNoDevTexture = false;
        m_bInDistanceSortedList = false;
        m_bCreatedInLevel = gRenDev->m_bInLevel;
        m_bUsedRecently = 0;
        m_bStatTracked = 0;
        m_bStreamHighPriority = 0;
        m_nStreamingPriority = 0;
        m_nMinMipVidUploaded = MAX_MIP_LEVELS;
        m_nMinMipVidActive = MAX_MIP_LEVELS;
        m_nStreamSlot = InvalidStreamSlot;
        m_fpMinMipCur = MAX_MIP_LEVELS << 8;
        m_nStreamFormatCode = 0;

        m_nDefState = 0;
        m_pFileTexMips = NULL;
        m_fCurrentMipBias = 0.f;

        m_pRenderTargetData = (nFlags & FT_USAGE_RENDERTARGET) ? new RenderTargetData : NULL;

        COMPILE_TIME_ASSERT(MaxStreamTasks < 32767);

#if defined(USE_UNIQUE_MUTEX_PER_TEXTURE)
        // Only the editor needs to use a unique mutex per texture.
        // In the editor, multiple worker threads could destroy device resource sets which point to the same texture
        if (gEnv->IsEditor())
        {
            m_invalidateCallbacksMutex = new AZStd::mutex();
        }
#endif
        // If we do not need a unique mutex per texture, use the StaticInstance
        if (m_invalidateCallbacksMutex == nullptr)
        {
            m_invalidateCallbacksMutex = &m_staticInvalidateCallbacksMutex;
        }
    }



    bool GetIsTextureMissing() const
    {
        return m_bisTextureMissing;
    }

    // ITexture interface
    virtual int AddRef() { return CBaseResource::AddRef(); }
    virtual int Release()
    {
        if (!(m_nFlags & FT_DONT_RELEASE) || IsActiveRenderTarget())
        {
            return CBaseResource::Release();
        }

        return -1;
    }
    virtual int ReleaseForce()
    {
        m_nFlags &= ~FT_DONT_RELEASE;
        int nRef = 0;
        while (true)
        {
            nRef = Release();
            if (nRef <= 0)
            {
                break;
            }
        }
        return nRef;
    }

    virtual const char* GetName() const { return GetSourceName(); }
    virtual const int GetWidth() const { return m_nWidth; }
    ILINE const int GetWidthNonVirtual() const { return m_nWidth; }
    virtual const int GetHeight() const { return m_nHeight; }
    ILINE const int GetHeightNonVirtual() const { return m_nHeight; }
    virtual const int GetDepth() const { return m_nDepth; }
    ILINE const int GetDepthNonVirtual() const { return m_nDepth; }
    ILINE const int GetNumSides() const { return m_CacheFileHeader.m_nSides; }
    ILINE const int8 GetNumPersistentMips() const { return m_CacheFileHeader.m_nMipsPersistent; }
    ILINE const bool IsForceStreamHighRes() const { return m_bForceStreamHighRes; }
    ILINE const bool IsStreamHighPriority() const { return m_bStreamHighPriority; }
    virtual const int GetTextureID() const;
    void SetFlags(uint32 nFlags) { m_nFlags = nFlags; }
    virtual const uint32 GetFlags() const { return m_nFlags; }
    ILINE const int GetNumMipsNonVirtual() const { return m_nMips; }
    virtual const int GetNumMips() const { return m_nMips; }
    virtual const int GetRequiredMip() const { return max(0, m_fpMinMipCur >> 8); }
    ILINE const int GetRequiredMipNonVirtual() const { return max(0, m_fpMinMipCur >> 8); }
    ILINE const int GetRequiredMipNonVirtualFP() const { return m_fpMinMipCur; }
    virtual const ETEX_Type GetTextureType() const;
    virtual void SetTextureType(ETEX_Type type);

    ILINE const int GetDefState() const { return m_nDefState; }
    virtual void SetClamp(bool bEnable)
    {
        int nMode = bEnable ? TADDR_CLAMP : TADDR_WRAP;
        SetClampingMode(nMode, nMode, nMode);
        UpdateTexStates();
    }

    ILINE const bool IsTextureMissing() const { return m_bisTextureMissing; }
    virtual const bool IsTextureLoaded() const { return IsLoaded(); }
    virtual void PrecacheAsynchronously(float fMipFactor, int nFlags, int nUpdateId, int nCounter = 1);
    virtual byte* GetData32(int nSide = 0, int nLevel = 0, byte* pDst = NULL, ETEX_Format eDstFormat = eTF_R8G8B8A8);
    virtual const int GetDeviceDataSize() const { return m_nActualSize; }
    virtual const int GetDataSize() const
    {
        if (IsStreamed())
        {
            return StreamComputeDevDataSize(0);
        }
        return m_nActualSize;
    }
    virtual bool SetFilter(int nFilter) { return SetFilterMode(nFilter); }
    virtual bool Clear();
    virtual bool Clear(const ColorF& color);
    virtual float GetAvgBrightness() const { return m_fAvgBrightness; }
    virtual void SetAvgBrightness(float fBrightness)  { m_fAvgBrightness = fBrightness; }
    virtual const ColorF& GetMinColor() const { return m_cMinColor; }
    virtual void SetMinColor(const ColorF& cMinColor)  { m_cMinColor = cMinColor; }
    virtual const ColorF& GetMaxColor() const { return m_cMaxColor; }
    virtual void SetMaxColor(const ColorF& cMaxColor)  { m_cMaxColor = cMaxColor; }
    virtual const ColorF& GetClearColor() const override { return m_cClearColor; }
    virtual void SetClearColor(const ColorF& cClearColor) { m_cClearColor = cClearColor; }

    virtual void GetMemoryUsage(ICrySizer* pSizer) const;
    virtual const char* GetFormatName() const;
    virtual const char* GetTypeName() const;

    virtual const bool IsShared() const
    {
        return CBaseResource::GetRefCounter() > 1;
    }

    virtual const ETEX_Format GetTextureDstFormat() const { return m_eTFDst; }
    virtual const ETEX_Format GetTextureSrcFormat() const { return m_eTFSrc; }

    virtual const bool IsParticularMipStreamed(float fMipFactor) const;

    // Internal functions
    const ETEX_Format GetDstFormat() const override { return m_eTFDst; }
    const ETEX_Format GetSrcFormat() const override { return m_eTFSrc; }
    const ETEX_Type GetTexType() const { return m_eTT; }
    const uint32 StreamGetNumSlices() const
    {
        switch (m_eTT)
        {
        case eTT_2D:
        case eTT_2DArray:
            return m_nArraySize;

        case eTT_Cube:
            return 6 * m_nArraySize;

        default:
#ifndef _RELEASE
            __debugbreak();
#endif
            return 1;
        }
    }

    void RT_ReleaseDevice();

    static _inline bool IsTextureExist(const ITexture* pTex) { return pTex && pTex->GetDevTexture(); }

    const bool IsNoTexture() const { return m_bNoTexture; };
    void SetNeedRestoring() { m_bNeedRestoring = true; }
    void SetWasUnload(bool bSet) { m_bWasUnloaded = bSet; }
    const bool IsPartiallyLoaded() const { return m_nMinMipVidUploaded != 0; }
    const bool IsUnloaded(void) const { return m_bWasUnloaded; }
    void SetKeepSystemCopy(bool bKeepSystemCopy)
    {
        if (bKeepSystemCopy)
        {
            m_nFlags |= FT_KEEP_LOWRES_SYSCOPY;
        }
        else
        {
            m_nFlags &= ~FT_KEEP_LOWRES_SYSCOPY;
        }
    }
    void SetStreamingInProgress(uint16 nStreamSlot)
    {
        assert (nStreamSlot == InvalidStreamSlot || m_nStreamSlot == InvalidStreamSlot);
        m_nStreamSlot = nStreamSlot;
    }
    void SetStreamingPriority(const uint8 nPriority) { m_nStreamingPriority = nPriority; }
    ILINE const STexStreamRoundInfo& GetStreamRoundInfo(int zone) const { return m_streamRounds[zone]; }
    void ResetNeedRestoring() { m_bNeedRestoring = false; }
    const bool IsNeedRestoring() const { return m_bNeedRestoring; }
    const int StreamGetLoadedMip() const { return m_nMinMipVidUploaded; }
    const uint8 StreamGetFormatCode() const { return m_nStreamFormatCode; }
    const int StreamGetActiveMip() const { return m_nMinMipVidActive; }
    const int StreamGetPriority() const { return m_nStreamingPriority; }
    const bool IsResolved() const { return m_bResolved; }
    void SetUseMultisampledRTV(bool bSet) { m_bUseMultisampledRTV = bSet; }
    const bool UseMultisampledRTV() const { return m_bUseMultisampledRTV; }
    const bool IsVertexTexture() const { return m_bVertexTexture; }
    void SetVertexTexture(bool bEnable) { m_bVertexTexture = bEnable; }
    const bool IsDynamic() const { return ((m_nFlags & (FT_USAGE_DYNAMIC | FT_USAGE_RENDERTARGET)) != 0); }
    bool IsStillUsedByGPU();

    // Render targets can be bound for write on the render target stack multiple times at once,
    // so increment and decrement when it is pushed/popped from the stack
    void IncrementRenderTargetUseCount()
    {
        assert(m_renderTargetUseCount < (1 << CTEXTURE_RENDER_TARGET_USE_COUNT_NUMBER_BITS));
        m_renderTargetUseCount++;
    }
    void DecrementRenderTargetUseCount()
    {
        assert(m_renderTargetUseCount > 0);
        m_renderTargetUseCount--;
    }

    // Is this texture bound on the render target stack?
    const bool IsActiveRenderTarget() const { return m_renderTargetUseCount > 0; }

    const bool IsLoaded() const { return (m_nFlags & FT_FAILED) == 0; }
    ILINE const bool IsStreamed() const { return m_bStreamed; }
    ILINE const bool IsInDistanceSortedList() const { return m_bInDistanceSortedList; }
    bool IsPostponed() const { return m_bPostponed; }
    ILINE const bool IsStreaming() const { return m_nStreamSlot != InvalidStreamSlot; }
    virtual const bool IsStreamedVirtual() const { return IsStreamed(); }
    virtual bool IsStreamedIn(const int nMinPrecacheRoundIds[MAX_STREAM_PREDICTION_ZONES]) const;
    virtual const int GetAccessFrameId() const { return m_nAccessFrameID; }
    ILINE const int GetAccessFrameIdNonVirtual() const { return m_nAccessFrameID; }
    void SetResolved(bool bResolved) { m_bResolved = bResolved; }
    const int GetCustomID() const { return m_nCustomID; }
    void SetCustomID(int nID) { m_nCustomID = nID; }
    const bool UseDecalBorderCol() const { return m_bUseDecalBorderCol; }
    const bool IsSRGB() const { return m_bIsSRGB; }
    void SRGBRead(bool bEnable = false) { m_bIsSRGB = bEnable; }
    const bool IsCustomFormat() const { return m_bCustomFormat; }
    void SetCustomFormat() { m_bCustomFormat = true; }
    void SetWidth(short width) { m_nWidth = width; }
    void SetHeight(short height) { m_nHeight = height; }
    int GetUpdateFrameID() const { return m_nUpdateFrameID; }
    ILINE const int32 GetActualSize() const { return m_nActualSize; }
    ILINE const int32 GetPersistentSize() const { return m_nPersistentSize; }

    ILINE void PrefetchStreamingInfo() const { PrefetchLine(m_pFileTexMips, 0); }
    const STexStreamingInfo* GetStreamingInfo() const { return m_pFileTexMips; }

    virtual const bool IsStreamable() const { return IsStreamed(); }

    void Readback(AZ::u32 subresourceIndex, StagingHook callback) override;

    const bool IsStreamableNonVirtual() const { return !(m_nFlags & FT_DONT_STREAM) && !(m_eTT == eTT_3D); }

#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION TEXTURE_H_SECTION_5
    #include AZ_RESTRICTED_FILE(Texture_h)
#endif

    bool IsFPFormat() const { return CImageExtensionHelper::IsRangeless(m_eTFDst); };

    D3DSurface* GetDeviceRT() const { return m_pDeviceRTV; }

    CDeviceTexture* GetDevTexture() const { return m_pDevTexture; }
    void SetDevTexture(CDeviceTexture* pDeviceTex);
    bool IsAsyncDevTexCreation() const { return m_bAsyncDevTexCreation; }

    // note: render target should be created with FT_FORCE_MIPS flag
    bool GenerateMipMaps(bool bSetOrthoProj = false, bool bUseHW = true, bool bNormalMap = false);

    D3DShaderResourceView* GetShaderResourceView(SResourceView::KeyType resourceViewID = SResourceView::DefaultView, bool bLegacySrgbLookup = false);
    void SetShaderResourceView(D3DShaderResourceView* pDeviceShaderResource, bool bMultisampled = false);

    CDeviceTexture* GetDevTextureMSAA() const { return m_pRenderTargetData->m_pDeviceTextureMSAA; }
    D3DUnorderedAccessView* GetDeviceUAV();

    SResourceView& GetResourceView(const SResourceView& rvDesc);
    void* CreateDeviceResourceView(const SResourceView& rvDesc);

    D3DDepthSurface* GetDeviceDepthStencilSurf(int nFirstSlice = 0, int nSliceCount = -1);
#ifndef NULL_RENDERER
    D3DSurface* GetSurface(int nCMSide, int nLevel);
#endif
    const SPixFormat* GetPixelFormat() const { return m_pPixelFormat; }
    bool Invalidate(int nNewWidth, int nNewHeight, ETEX_Format eTF);
    const char* GetSourceName() const { return m_SrcName.c_str(); }
    void SetSourceName( const char* srcName) { m_SrcName = srcName; }
    const int GetSize(bool bIncludePool) const;
    void PostCreate();

    //////////////////////////////////////////////////////////////////////////
    // Will notify resource's user that some data of the the resource was invalidated.
    // dirtyFlags - one or more of the EDeviceDirtyFlags enum bits
    void InvalidateDeviceResource(uint32 dirtyFlags);
    void AddInvalidateCallback(void* listener, InvalidateCallbackType callback);
    void RemoveInvalidateCallbacks(void* listener);
    //////////////////////////////////////////////////////////////////////////

public:
    //===================================================================
    // Streaming support

    // Global streaming constants
    static int s_nStreamingMode;
    static int s_nStreamingUpdateMode;
    static int s_nStreamingThroughput;  // in bytes
    static float s_nStreamingTotalTime; // in secs
    static bool s_bStreamDontKeepSystem;
    static bool s_bPrecachePhase;
    static bool s_bInLevelPhase;
    static bool s_bPrestreamPhase;
    static bool s_bStreamingFromHDD;
    static int s_nTexturesDataBytesLoaded;
    static volatile int s_nTexturesDataBytesUploaded;
    static int s_nStatsAllocFails;
    static bool s_bOutOfMemoryTotally;

    static volatile size_t s_nStatsStreamPoolInUseMem;                  // Amount of stream pool currently in use by texture streaming
    static volatile size_t s_nStatsStreamPoolBoundMem;                  // Amount of stream pool currently bound and in use by textures (avail + non avail)
    static volatile size_t s_nStatsStreamPoolBoundPersMem;          // Amount of stream pool currently bound and in use by persistent texture mem (avail + non avail)
    static AZStd::atomic_uint s_nStatsCurManagedNonStreamedTexMem;
    static AZStd::atomic_uint s_nStatsCurDynamicTexMem;
    static volatile size_t s_nStatsStreamPoolWanted;
    static bool s_bStatsComputeStreamPoolWanted;

    struct WantedStat
    {
        _smart_ptr<ITexture> pTex;
        uint32 nWanted;
    };

    static std::vector<WantedStat>* s_pStatsTexWantedLists;
    static AZStd::set<string, AZStd::less<string>, AZ::StdLegacyAllocator> s_vTexReloadRequests;
    static CryCriticalSection s_xTexReloadLock;

    static CTextureStreamPoolMgr* s_pPoolMgr;

    static ITextureStreamer* s_pTextureStreamer;

    static CryCriticalSection s_streamFormatLock;
    static SStreamFormatCode s_formatCodes[256];
    static uint32 s_nFormatCodes;
    typedef std::map<SStreamFormatCodeKey, uint32> TStreamFormatCodeKeyMap;
    static StaticInstance<TStreamFormatCodeKeyMap> s_formatCodeMap;

    static const int LOW_SPEC_PC;
    static const int MEDIUM_SPEC_PC;
    static const int HIGH_SPEC_PC;
    static const int VERYHIGH_SPEC_PC;

    enum
    {
        MaxStreamTasks = 512,
        MaxStreamPrepTasks = 8192,
        StreamOutMask = 0x8000,
        StreamPrepMask = 0x4000,
        StreamIdxMask = 0x4000 - 1,
        InvalidStreamSlot = 0xffff,
    };

    static CTextureArrayAlloc<STexStreamInState, MaxStreamTasks> s_StreamInTasks;
    static CTextureArrayAlloc<STexStreamPrepState*, MaxStreamPrepTasks> s_StreamPrepTasks;

#ifdef TEXSTRM_ASYNC_TEXCOPY
    static CTextureArrayAlloc<STexStreamOutState, MaxStreamTasks> s_StreamOutTasks;
#endif

    static volatile TIntAtomic s_nBytesSubmittedToStreaming;
    static volatile TIntAtomic s_nMipsSubmittedToStreaming;
    static int s_nBytesRequiredNotSubmitted;

#if !defined (_RELEASE)
    static int s_TextureUpdates;
    static float s_TextureUpdatesTime;
    static int s_TexturesUpdatedRendered;
    static float s_TextureUpdatedRenderedTime;
    static int s_StreamingRequestsCount;
    static float s_StreamingRequestsTime;
    static int s_nStatsCurManagedStreamedTexMemRequired;
#endif

#ifdef ENABLE_TEXTURE_STREAM_LISTENER
    static ITextureStreamListener* s_pStreamListener;
#endif

    static void Precache();
    static void RT_Precache();
    static void StreamValidateTexSize();
    static uint8 StreamComputeFormatCode(uint32 nWidth, uint32 nHeight, uint32 nMips, ETEX_Format fmt);

#ifdef ENABLE_TEXTURE_STREAM_LISTENER
    static void StreamUpdateStats();
#endif

#if defined(TEXSTRM_STORE_DEVSIZES)
    int StreamComputeDevDataSize(int nFromMip) const
    {
        if (m_pFileTexMips)
        {
            return m_pFileTexMips->m_pMipHeader[nFromMip].m_DevSideSizeWithMips;
        }
        else
        {
            return CDeviceTexture::TextureDataSize(
                max(1, m_nWidth >> nFromMip),
                max(1, m_nHeight >> nFromMip),
                max(1, m_nDepth >> nFromMip),
                m_nMips - nFromMip,
                StreamGetNumSlices(),
                m_eTFDst);
        }
    }
#else
    int StreamComputeDevDataSize(int nFromMip) const
    {
        return CDeviceTexture::TextureDataSize(
            max(1, m_nWidth >> nFromMip),
            max(1, m_nHeight >> nFromMip),
            max(1, m_nDepth >> nFromMip),
            m_nMips - nFromMip,
            StreamGetNumSlices(),
            m_eTFDst);
    }
#endif

#ifndef NULL_RENDERER
    void StreamUploadMip(IReadStream* pStream, int nMip, int nBaseMipOffset, STexPoolItem* pNewPoolItem, STexStreamInMipState& mipState);
    void StreamUploadMipSide(
        int const iSide, int const Sides, const byte* const pRawData, int nSrcPitch,
        const STexMipHeader& mh, bool const bStreamInPlace,
        int const nCurMipWidth, int const nCurMipHeight, int const nMip,
        CDeviceTexture* pDeviceTexture, uint32 nBaseTexWidth, uint32 nBaseTexHeight, int nBaseMipOffset);
#endif
    void StreamExpandMip(const void* pRawData, int nMip, int nBaseMipOffset, int nSideDelta);
    static void RT_FlushAllStreamingTasks(const bool bAbort = false);
    static bool IsStreamingInProgress() { return s_StreamInTasks.GetNumLive() > 0; }
    static void AbortStreamingTasks(CTexture* pTex);
    static bool StartStreaming(CTexture* pTex, STexPoolItem* pNewPoolItem, const int nStartMip, const int nEndMip, const int nActivateMip, EStreamTaskPriority estp);
    bool CanStreamInPlace(int nMip, STexPoolItem* pNewPoolItem);
#if defined(TEXSTRM_ASYNC_TEXCOPY)
    bool CanAsyncCopy();
#endif
    void StreamCopyMipsTexToMem(int nStartMip, int nEndMip, bool bToDevice, STexPoolItem* pNewPoolItem);
    static void StreamCopyMipsTexToTex(STexPoolItem* pSrcItem, int nMipSrc, STexPoolItem* pDestItem, int nMipDest, int nNumMips);   // GPU-assisted platform-dependent
    static void CopySliceChain(CDeviceTexture* const pDevTexture, int ownerMips, int nDstSlice, int nDstMip, CDeviceTexture* pSrcDevTex, int nSrcSlice, int nSrcMip, int nSrcMips, int nNumMips);
#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION TEXTURE_H_SECTION_6
    #include AZ_RESTRICTED_FILE(Texture_h)
#endif
#if defined(TEXSTRM_DEFERRED_UPLOAD)
    ID3D11CommandList* StreamCreateDeferred(int nStartMip, int nEndMip, STexPoolItem* pNewPoolItem, STexPoolItem* pSrcPoolItem);
    void StreamApplyDeferred(ID3D11CommandList* pCmdList);
#endif
    void StreamReleaseMipsData(int nStartMip, int nEndMip);
    int16 StreamCalculateMipsSignedFP(float fMipFactor) const;
    float StreamCalculateMipFactor(int16 nMipsSigned) const;
    virtual int StreamCalculateMipsSigned(float fMipFactor) const;
    virtual int GetStreamableMipNumber()  const;
    virtual int GetStreamableMemoryUsage(int nStartMip) const;
    virtual int GetMinLoadedMip() const { return m_nMinMipVidUploaded; }
    void SetMinLoadedMip(int nMinMip);
    void StreamUploadMips(int nStartMip, int nEndMip, STexPoolItem* pNewPoolItem);
    int StreamUnload();
    int StreamTrim(int nToMip);
    void StreamActivateLod(int nMinMip);
    void StreamLoadFromCache(const int nFlags);
    bool StreamPrepare(bool bFromLoad);
    bool StreamPrepare(CImageFile* pImage);
    bool StreamPrepareComposition();
    bool StreamPrepare_Platform();
    bool StreamPrepare_Finalise(bool bFromLoad);
    STexPool* StreamGetPool(int nStartMip, int nMips);
    STexPoolItem* StreamGetPoolItem(int nStartMip, int nMips, bool bShouldBeCreated, bool bCreateFromMipData = false, bool bCanCreate = true, bool bForStreamOut = false);
    void StreamRemoveFromPool();
    void StreamAssignPoolItem(STexPoolItem* pItem, int nMinMip);

    static void StreamState_Update();
    static void StreamState_UpdatePrep();

    static STexStreamInState* StreamState_AllocateIn();
    static void StreamState_ReleaseIn(STexStreamInState* pState);
#ifdef TEXSTRM_ASYNC_TEXCOPY
    static STexStreamOutState* StreamState_AllocateOut();
    static void StreamState_ReleaseOut(STexStreamOutState* pState);
#endif
    static STexStreamingInfo* StreamState_AllocateInfo(int nMips);
    static void StreamState_ReleaseInfo(CTexture* pOwnerTex, STexStreamingInfo* pInfo);

    ILINE void Relink()
    {
        gRenDev->m_pRT->RC_RelinkTexture(this);
    }

    ILINE void Unlink()
    {
        gRenDev->m_pRT->RC_UnlinkTexture(this);
    }

    _inline void RT_Relink()
    {
        if (!IsStreamed() || IsInDistanceSortedList())
        {
            return;
        }

        s_pTextureStreamer->Relink(this);
    }
    _inline void RT_Unlink()
    {
        if (IsInDistanceSortedList())
        {
            s_pTextureStreamer->Unlink(this);
        }
    }
    //=======================================================

    static void ApplyForID(int nTUnit, int nID, int nState, int nSUnit)
    {
        CTexture* pTex = GetByID(nID);
        assert (pTex);
        if (pTex)
        {
            pTex->Apply(nTUnit, nState, EFTT_UNKNOWN, nSUnit);
        }
    }

    static void ApplyForID(int nID, int nTUnit, int nTState, int nTexMaterialSlot, int nSUnit, bool useWhiteDefault);

    static const CCryNameTSCRC& mfGetClassName();
    static CTexture* GetByID(int nID);
    static CTexture* GetByName(const char* szName, uint32 flags = 0);
    static CTexture* GetByNameCRC(CCryNameTSCRC Name);
    static CTexture* ForName(const char* name, uint32 nFlags, ETEX_Format eTFDst);
    static CTexture* CreateTextureArray(const char* name, ETEX_Type eType, uint32 nWidth, uint32 nHeight, uint32 nArraySize, int nMips, uint32 nFlags, ETEX_Format eTF, int nCustomID = -1);
    static CTexture* CreateTextureObject(const char* name, uint32 nWidth, uint32 nHeight, int nDepth, ETEX_Type eTT, uint32 nFlags, ETEX_Format eTF, int nCustomID = -1);

    // Methods exposed to external libraries
    static CTexture* CreateRenderTarget(const char* name, uint32 nWidth, uint32 nHeight, const ColorF& cClear, ETEX_Type eTT, uint32 nFlags, ETEX_Format eTF, int nCustomID = -1);
    static void ApplyDepthTextureState(int unit, int nFilter, bool clamp);
    static CTexture* GetZTargetTexture();
    static int GetTextureState(const STexState& TS);
    static uint32 TextureDataSize(uint32 nWidth, uint32 nHeight, uint32 nDepth, uint32 nMips, uint32 nSlices, const ETEX_Format eTF, ETEX_TileMode eTM = eTM_None);

    static void InitStreaming();
    static void InitStreamingDev();
    static void Init();
    static void PostInit();
    static void RT_FlushStreaming(bool bAbort);
    static void ShutDown();

    static void CreateSystemTargets();
    static void ReleaseSystemTargets();
    static void ReleaseMiscTargets();
    static void ReleaseSystemTextures();
    static void LoadDefaultSystemTextures();

    static bool ReloadFile(const char* szFileName);
    static bool ReloadFile_Request(const char* szFileName);
    static void ReloadTextures();
    static CTexture* Create2DTexture(const char* szName, int nWidth, int nHeight, int nMips, int nFlags, const byte* pData, ETEX_Format eTFSrc, ETEX_Format eTFDst, bool bAsyncDevTexCreation = false);
    static CTexture* Create3DTexture(const char* szName, int nWidth, int nHeight, int nDepth, int nMips, int nFlags, const byte* pData, ETEX_Format eTFSrc, ETEX_Format eTFDst);
    static CTexture* Create2DCompositeTexture(const char* szName, int nWidth, int nHeight, int nMips, int nFlags, ETEX_Format eTFDst, const STexComposition* pCompositions, size_t nCompositions);
    static void Update();
    static void RT_LoadingUpdate();
    static void RLT_LoadingUpdate();

    // Loading/creating functions
    bool Load(ETEX_Format eTFDst);
    bool Load(CImageFile* pImage);
    bool LoadFromImage(const char* name, ETEX_Format eTFDst = eTF_Unknown);
    virtual bool Reload();
    bool ToggleStreaming(const bool bEnable);
    bool CreateTexture(STexData& td);

    byte* GetSubImageData32(int nX, int nY, int nW, int nH, int& nOutTexDim);

    // API depended functions
    void Unbind();
    bool Resolve(int nTarget = 0, bool bUseViewportSize = false);
    bool CreateDeviceTexture(const byte* pData[6]);
    bool RT_CreateDeviceTexture(const byte* pData[6]);
    bool CreateRenderTarget(ETEX_Format eTF, const ColorF& cClear);
    void ReleaseDeviceTexture(bool bKeepLastMips, bool bFromUnload = false);
    void ApplySamplerState(int nSUnit, EHWShaderClass eHWSC = eHWSC_Pixel, int nState = -1);
    void ApplyTexture(int nTUnit, EHWShaderClass eHWSC = eHWSC_Pixel, SResourceView::KeyType nResViewKey = SResourceView::DefaultView);
    ETEX_Format ClosestFormatSupported(ETEX_Format eTFDst);
    static ETEX_Format ClosestFormatSupported(ETEX_Format eTFDst, const SPixFormat*& pPF);
    void SetTexStates();
    void UpdateTexStates();
    bool SetFilterMode(int nFilter);
    bool SetClampingMode(int nAddressU, int nAddressV, int nAddressW);
    void UpdateTextureRegion(const uint8_t* data, int nX, int nY, int nZ, int USize, int VSize, int ZSize, ETEX_Format eTFSrc);
    void RT_UpdateTextureRegion(const byte* data, int nX, int nY, int nZ, int USize, int VSize, int ZSize, ETEX_Format eTFSrc);

    // Create2DTextureWithMips is similar to Create2DTexture, but it also propagates the mip argument correctly.
    // The original Create2DTexture function force sets mips to 1.
    // This has been separated from Create2DTexture to ensure that we preserve backwards compatibility.
    bool Create2DTextureWithMips(int nWidth, int nHeight, int nMips, int nFlags, const byte* pData, ETEX_Format eTFSrc, ETEX_Format eTFDst);

    bool Create2DTexture(int nWidth, int nHeight, int nMips, int nFlags, const byte* pData, ETEX_Format eTFSrc, ETEX_Format eTFDst);
    bool Create3DTexture(int nWidth, int nHeight, int nDepth, int nMips, int nFlags, const byte* pData, ETEX_Format eTFSrc, ETEX_Format eTFDst);
    bool SetNoTexture( const CTexture* pDefaultTexture );
    bool IsMSAAChanged();

    static byte* Convert(const byte* srcData, int width, int height, int srcMipCount, ETEX_Format srcFormat, ETEX_Format dstFormat, int& outSize, bool bLinear);

    static void SetSamplerState(int nTS, int nSSlot, EHWShaderClass eHWSC = eHWSC_Pixel);

    // Helper functions
    static bool IsFormatSupported(ETEX_Format eTFDst);
    static void GenerateZMaps();
    static void DestroyZMaps();
    static void GenerateHDRMaps();
    // allocate or deallocate star maps
    static void DestroyHDRMaps();

    static void GenerateSceneMap(ETEX_Format eTF);
    static void DestroySceneMap();
    static void GenerateCachedShadowMaps();
    static void DestroyCachedShadowMaps();
    static void GenerateNearestShadowMap();
    static void DestroyNearestShadowMap();

    static ILINE Vec2i GetBlockDim(const ETEX_Format eTF)
    {
        return CImageExtensionHelper::GetBlockDim(eTF);
    }

    static int CalcNumMips(int nWidth, int nHeight);
    // upload mip data from file regarding to platform specifics
    static bool IsInPlaceFormat(const ETEX_Format fmt);
    static void ExpandMipFromFile(byte* dest, const int destSize, const byte* src, const int srcSize, const ETEX_Format fmt);

    static ILINE bool IsBlockCompressed(const ETEX_Format eTF) { return CImageExtensionHelper::IsBlockCompressed(eTF); }
    static ILINE int BytesPerBlock(ETEX_Format eTF) { return CImageExtensionHelper::BytesPerBlock(eTF); }
    static const char* NameForTextureFormat(ETEX_Format eTF) { return CImageExtensionHelper::NameForTextureFormat(eTF); }
    static const char* NameForTextureType(ETEX_Type eTT) { return CImageExtensionHelper::NameForTextureType(eTT); }
    static ETEX_Format TextureFormatForName(const char* str) { return CImageExtensionHelper::TextureFormatForName(str); }
    static ETEX_Type TextureTypeForName(const char* str) { return CImageExtensionHelper::TextureTypeForName(str); }
    static bool IsDeviceFormatTypeless(D3DFormat nFormat);
    static bool IsDeviceFormatSRGBReadable(D3DFormat nFormat);
    static D3DFormat DeviceFormatFromTexFormat(ETEX_Format eTF);
    static ETEX_Format TexFormatFromDeviceFormat(D3DFormat nFormat);
    static D3DFormat GetD3DLinFormat(D3DFormat nFormat);
    static D3DFormat ConvertToDepthStencilFmt(D3DFormat nFormat);
    static D3DFormat ConvertToStencilFmt(D3DFormat nFormat);
    static D3DFormat ConvertToShaderResourceFmt(D3DFormat nFormat);

    static D3DFormat ConvertToSRGBFmt(D3DFormat fmt);
    static D3DFormat ConvertToSignedFmt(D3DFormat fmt);
    static D3DFormat ConvertToTypelessFmt(D3DFormat fmt);

    static SEnvTexture* FindSuitableEnvTex(Vec3& Pos, Ang3& Angs, bool bMustExist, int RendFlags, bool bUseExistingREs, CShader* pSH, CShaderResources* pRes, CRenderObject* pObj, bool bReflect, IRenderElement* pRE, bool* bMustUpdate);
    static bool RenderEnvironmentCMHDR(int size, Vec3& Pos, TArray<unsigned short>& vecData);

#if AZ_RENDER_TO_TEXTURE_GEM_ENABLED
    static bool RenderToTexture(int handle, const CCamera& camera, AzRTT::RenderContextId contextId);
#endif // if AZ_RENDER_TO_TEXTURE_GEM_ENABLED

    static void DrawCubeSide(Vec3& Pos, int tex_size, int side, float fMaxDist);
    static void DrawSceneToCubeSide(Vec3& Pos, int tex_size, int side);
    static void GetAverageColor(SEnvTexture* cm, int nSide);

public:

    static AZStd::vector<STexState, AZ::StdLegacyAllocator> s_TexStates;
    static int GetTexState(const STexState& TS)
    {
        uint32 i;

        const uint32 nTexStatesSize = s_TexStates.size();
        for (i = 0; i < nTexStatesSize; i++)
        {
            STexState* pTS = &s_TexStates[i];
            if (*pTS == TS)
            {
                break;
            }
        }

        if (i == nTexStatesSize)
        {

            s_TexStates.push_back(TS);
            s_TexStates[i].PostCreate();
        }

        return i;
    }

    static void ComputeRootedTexturePath(const char* sourcePath, char* destPath, size_t destPathLength);

    static bool m_bLoadedSystem;

    static STexState s_sDefState;
    static STexStageInfo s_TexStages[MAX_TMU];
    static uint32 s_TexState_MipSRGBMask[MAX_TMU];

    static ETEX_Format s_eTFZ;

    // ==============================================================================
    static CTexture* s_ptexMipColors_Diffuse;
    static CTexture* s_ptexMipColors_Bump;
    static CTexture* s_ptexFromRE[8];
    static CTexture* s_ptexShadowID[8];
    static CTexture* s_ptexShadowMask;
    static CTexture* s_ptexFromRE_FromContainer[2];
    static CTexture* s_ptexFromObj;
    static CTexture* s_ptexSvoTree;
    static CTexture* s_ptexSvoTris;
    static CTexture* s_ptexSvoGlobalCM;
    static CTexture* s_ptexSvoRgbs;
    static CTexture* s_ptexSvoNorm;
    static CTexture* s_ptexSvoOpac;
    static CTexture* s_ptexFromObjCM;
    static CTexture* s_ptexRT_2D;
    static CTexture* s_ptexCachedShadowMap[MAX_GSM_LODS_NUM];
    static CTexture* s_ptexNearestShadowMap;
    static CTexture* s_ptexHeightMapAO[2];
    static CTexture* s_ptexHeightMapAODepth[2];

    static CTexture* s_ptexSceneNormalsMap;         // RT with normals for deferred shading
    static CTexture* s_ptexSceneNormalsMapMS;         // Dummy normals target for binding multisampled rt
    static CTexture* s_ptexSceneNormalsBent;
    static CTexture* s_ptexAOColorBleed;
    static CTexture* s_ptexSceneDiffuse;
    static CTexture* s_ptexSceneSpecular;
#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION TEXTURE_H_SECTION_7
    #include AZ_RESTRICTED_FILE(Texture_h)
#endif
    static CTexture* s_ptexAmbientLookup;

    static CTexture* s_ptexBackBuffer;              // back buffer copy
    static CTexture* s_ptexModelHudBuffer;            // used by Menu3DModelRenderer to postprocess render models
    static CTexture* s_ptexPrevBackBuffer[2][2];      // previous frame back buffer copies (for left and right eye)
    static CTexture* s_ptexCached3DHud;               // 3d hud cached overframes
    static CTexture* s_ptexCached3DHudScaled;                   // downsampled 3d hud cached overframes
    static CTexture* s_ptexBackBufferScaled[3];     // backbuffer low-resolution/blurred version. 2x/4x/8x/16x smaller than screen
    static CTexture* s_ptexBackBufferScaledTemp[2];   // backbuffer low-resolution/blurred version. 2x/4x/8x/16x smaller than screen, temp textures (used for blurring/ping-pong)

    static CTexture* s_ptexPrevFrameScaled;                     // 2x

    static CTexture* s_ptexDepthBufferQuarter;              //Quater res depth buffer

    static CTexture* s_ptexWaterOcean;              // water ocean vertex texture
    static CTexture* s_ptexWaterVolumeDDN;          // water volume heightmap
    static CTexture* s_ptexWaterVolumeTemp;         // water volume heightmap
    static CTexture* s_ptexWaterRipplesDDN;         // xy: wave propagation normals, z: frame t-2, w: frame t-1
    static CTexture* s_ptexWaterVolumeRefl[2];        // water volume reflections buffer
    static CTexture* s_ptexWaterCaustics[2];                    // caustics buffers
    static CTexture* s_ptexRainOcclusion;                           // top-down rain occlusion
    static CTexture* s_ptexRainSSOcclusion[2];              // screen-space rain occlusion accumulation

    static CTexture* s_ptexRainDropsRT[2];

    static CTexture* s_ptexRT_ShadowPool;
    static CTexture* s_ptexRT_ShadowStub;
    static CTexture* s_ptexCloudsLM;

    static CTexture* s_ptexSceneTarget;  // Shared rt for generic usage (refraction/srgb/diffuse accumulation/hdr motionblur/etc)
    static CTexture* s_ptexCurrSceneTarget; // Pointer to current scene target, mostly for reading from destination rt
    static CTexture* s_ptexSceneTargetR11G11B10F[2];
    static CTexture* s_ptexSceneTargetScaledR11G11B10F[4];

    static CTexture* s_ptexSceneCoCHistory[2];

    static CTexture* s_ptexCurrentSceneDiffuseAccMap;
    static CTexture* s_ptexSceneDiffuseAccMap;
    static CTexture* s_ptexSceneSpecularAccMap;
    static CTexture* s_ptexSceneTexturesMap;
    static CTexture* s_ptexSceneDiffuseAccMapMS;
    static CTexture* s_ptexSceneSpecularAccMapMS;

    static CTexture* s_ptexZTarget;

    static CTexture* s_ptexZTargetDownSample[4];
    static CTexture* s_ptexZTargetScaled;
    static CTexture* s_ptexZTargetScaled2;

    static CTexture* s_ptexHDRTarget;
    static CTexture* s_ptexVelocityObjects[2];  // Dynamic object velocity (for left and right eye)
    static CTexture* s_ptexVelocity;
    static CTexture* s_ptexVelocityTiles[3];

    // Intermediate textures used for fur rendering
    static CTexture* s_ptexFurZTarget;  // Z target with outermost shell stipples. s_ptexZTarget has the stipples blurred out
    static CTexture* s_ptexFurLightAcc; // Lighting accumulation after deferred shading
    static CTexture* s_ptexFurPrepass;  // Packed diffuse, specular, and depth for shell passes

    // Confetti Begin: David Srour
    // Dedicated stencil & linear depth buffer for GMEM render path.
    static CTexture* s_ptexGmemStenLinDepth;
    // Confetti End

    static CTexture* s_ptexHDRTargetPrev;
    static CTexture* s_ptexHDRTargetScaled[4];
    static CTexture* s_ptexHDRTargetScaledTmp[4];
    static CTexture* s_ptexHDRTargetScaledTempRT[4];
    static CTexture* s_ptexHDRDofLayers[2];
    static CTexture* s_ptexSceneCoC[MIN_DOF_COC_K];
    static CTexture* s_ptexSceneCoCTemp;
    static CTexture* s_ptexHDRTempBloom[2];
    static CTexture* s_ptexHDRFinalBloom;
    static CTexture* s_ptexHDRAdaptedLuminanceCur[8];
    static int       s_nCurLumTextureIndex;
    static CTexture* s_ptexCurLumTexture;
    static CTexture* s_ptexHDRToneMaps[NUM_HDR_TONEMAP_TEXTURES];
    static CTexture* s_ptexHDRMeasuredLuminance[MAX_GPU_NUM];
    static CTexture* s_ptexHDRMeasuredLuminanceDummy;
    static CTexture* s_ptexSkyDomeMie;
    static CTexture* s_ptexSkyDomeRayleigh;
    static CTexture* s_ptexSkyDomeMoon;

    static CTexture* s_ptexSceneTargetScaled;
    static CTexture* s_ptexSceneTargetScaledBlurred;

    static CTexture* s_ptexVolObj_Density;
    static CTexture* s_ptexVolObj_Shadow;

    static CTexture* s_ptexColorChart;

    static CTexture* s_ptexStereoL;
    static CTexture* s_ptexStereoR;

    static CTexture* s_ptexFlaresOcclusionRing[MAX_OCCLUSION_READBACK_TEXTURES];
    static CTexture* s_ptexFlaresGather;

    static CTexture* s_ptexDepthStencilRemapped;
    static SEnvTexture s_EnvCMaps[MAX_ENVCUBEMAPS];
    static SEnvTexture s_EnvTexts[MAX_ENVTEXTURES];
    static StaticInstance<TArray<SEnvTexture>> s_CustomRT_2D;
    static StaticInstance<TArray<CTexture>> s_ShaderTemplates;      // [Shader System TO DO] bad design - change (or shoot)
    static bool s_ShaderTemplatesInitialized;

    static CTexture* s_pTexNULL;

    static CTexture* s_pBackBuffer;
    static CTexture* s_FrontBufferTextures[2];

    static CTexture* s_ptexVolumetricFog;
    static CTexture* s_ptexVolumetricFogDensityColor;
    static CTexture* s_ptexVolumetricFogDensity;
    static CTexture* s_ptexVolumetricClipVolumeStencil;

#if defined(VOLUMETRIC_FOG_SHADOWS)
    static CTexture* s_ptexVolFogShadowBuf[2];
#endif

#if defined(TEXSTRM_DEFERRED_UPLOAD)
    static ID3D11DeviceContext* s_pStreamDeferredCtx;
#endif

    static CTexture* s_defaultEnvironmentProbeDummy;
    void* operator new(size_t sz)
    {
        return CryModuleMemalign(sz, std::alignment_of<CTexture>::value);
    }

    void operator delete(void* ptr)
    {
        CryModuleMemalignFree(ptr);
    }
} _ALIGN(128);

////////////////////////////////////////////////////////////////

class CTexAnim : public ITexAnim
{
    friend class CShaderMan;
    friend struct SCGTexture;
    friend struct STexSamplerRT;

public:
    CTexAnim();
    ~CTexAnim();

    AZ_DISABLE_COPY_MOVE(CTexAnim)

    void Release() override;
    void AddRef() override;

    int Size() const;

private:
    int m_nRefCount;
    TArray<CTexture*> m_TexPics;
    int m_Rand;
    int m_NumAnimTexs;
    bool m_bLoop;
    float m_Time;
};



bool WriteTGA(const byte* dat, int wdt, int hgt, const char* name, int src_bits_per_pixel, int dest_bits_per_pixel);
bool WriteJPG(const byte* dat, int wdt, int hgt, const char* name, int bpp, int nQuality = 100);
#if defined(WIN32) || defined(WIN64)
byte* WriteDDS(const byte* dat, int wdt, int hgt, int dpth, const char* name, ETEX_Format eTF, int nMips, ETEX_Type eTT, bool bToMemory = false, int* nSize = NULL);
#endif
