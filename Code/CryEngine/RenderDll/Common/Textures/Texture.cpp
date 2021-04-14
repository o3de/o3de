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

// Description : Common texture manager implementation.


#include "RenderDll_precompiled.h"
#include <CryPath.h>
#include <ImageExtensionHelper.h>
#include "Image/CImage.h"
#include "Image/DDSImage.h"
#include "TextureManager.h"
#include <IResourceManager.h>
#include "I3DEngine.h"
#include <Pak/CryPakUtils.h>
#include "StringUtils.h"                                // stristr()
#include "TextureManager.h"
#include "TextureStreamPool.h"
#include "TextureHelpers.h"
#include <AzCore/IO/SystemFile.h> // for AZ_MAX_PATH_LEN
#include "StereoTexture.h"
#include <AzCore/Debug/AssetTracking.h>
#include <AzCore/std/string/conversions.h>
#include <AzFramework/StringFunc/StringFunc.h>

#if defined(AZ_RESTRICTED_PLATFORM)
#undef AZ_RESTRICTED_SECTION
#define TEXTURE_CPP_SECTION_1 1
#define TEXTURE_CPP_SECTION_2 2
#define TEXTURE_CPP_SECTION_3 3
#define TEXTURE_CPP_SECTION_4 4
#define TEXTURE_CPP_SECTION_5 5
#define TEXTURE_CPP_SECTION_6 6
#define TEXTURE_CPP_SECTION_7 7
#endif

#if defined(OPENGL_ES) || defined(CRY_USE_METAL)
#include "../../XRenderD3D9/DriverD3D.h" // for gcpRendD3D
#endif

#define TEXTURE_LEVEL_CACHE_PAK "dds0.pak"

STexState CTexture::s_sDefState;
STexStageInfo CTexture::s_TexStages[MAX_TMU];
int CTexture::s_nStreamingMode;
int CTexture::s_nStreamingUpdateMode;
bool CTexture::s_bPrecachePhase;
bool CTexture::s_bInLevelPhase = false;
bool CTexture::s_bPrestreamPhase;
int CTexture::s_nStreamingThroughput = 0;
float CTexture::s_nStreamingTotalTime = 0;
AZStd::vector<STexState, AZ::StdLegacyAllocator> CTexture::s_TexStates;
CTextureStreamPoolMgr* CTexture::s_pPoolMgr;
AZStd::set<string, AZStd::less<string>, AZ::StdLegacyAllocator> CTexture::s_vTexReloadRequests;
CryCriticalSection CTexture::s_xTexReloadLock;
#ifdef TEXTURE_GET_SYSTEM_COPY_SUPPORT
StaticInstance<CTexture::LowResSystemCopyType> CTexture::s_LowResSystemCopy;
#endif

StaticInstance<AZStd::mutex> CTexture::m_staticInvalidateCallbacksMutex;

bool CTexture::m_bLoadedSystem;

CTexture* CTexture::s_ptexMipColors_Diffuse;
CTexture* CTexture::s_ptexMipColors_Bump;
CTexture* CTexture::s_ptexFromRE[8];
CTexture* CTexture::s_ptexShadowID[8];
CTexture* CTexture::s_ptexShadowMask;
CTexture* CTexture::s_ptexCachedShadowMap[MAX_GSM_LODS_NUM];
CTexture* CTexture::s_ptexNearestShadowMap;
CTexture* CTexture::s_ptexHeightMapAO[2];
CTexture* CTexture::s_ptexHeightMapAODepth[2];
CTexture* CTexture::s_ptexFromRE_FromContainer[2];
CTexture* CTexture::s_ptexFromObj;
CTexture* CTexture::s_ptexSvoTree;
CTexture* CTexture::s_ptexSvoTris;
CTexture* CTexture::s_ptexSvoGlobalCM;
CTexture* CTexture::s_ptexSvoRgbs;
CTexture* CTexture::s_ptexSvoNorm;
CTexture* CTexture::s_ptexSvoOpac;
CTexture* CTexture::s_ptexFromObjCM;
CTexture* CTexture::s_ptexRT_2D;
CTexture* CTexture::s_ptexSceneNormalsMap;
CTexture* CTexture::s_ptexSceneNormalsMapMS;
CTexture* CTexture::s_ptexSceneNormalsBent;
CTexture* CTexture::s_ptexAOColorBleed;
CTexture* CTexture::s_ptexSceneDiffuse;
CTexture* CTexture::s_ptexSceneSpecular;
#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION TEXTURE_CPP_SECTION_1
    #include AZ_RESTRICTED_FILE(Texture_cpp)
#endif
CTexture* CTexture::s_ptexAmbientLookup;

// Post-process related textures
CTexture* CTexture::s_ptexBackBuffer;
CTexture* CTexture::s_ptexModelHudBuffer;
CTexture* CTexture::s_ptexPrevBackBuffer[2][2] = {
    {NULL}
};
CTexture* CTexture::s_ptexCached3DHud;
CTexture* CTexture::s_ptexCached3DHudScaled;
CTexture* CTexture::s_ptexBackBufferScaled[3];
CTexture* CTexture::s_ptexBackBufferScaledTemp[2];
CTexture* CTexture::s_ptexPrevFrameScaled;

CTexture* CTexture::s_ptexDepthBufferQuarter;

CTexture* CTexture::s_ptexWaterOcean;
CTexture* CTexture::s_ptexWaterVolumeTemp;
CTexture* CTexture::s_ptexWaterVolumeDDN;
CTexture* CTexture::s_ptexWaterVolumeRefl[2];
CTexture* CTexture::s_ptexWaterCaustics[2];
CTexture* CTexture::s_ptexWaterRipplesDDN;
CTexture* CTexture::s_ptexRainOcclusion;
CTexture* CTexture::s_ptexRainSSOcclusion[2];

CTexture* CTexture::s_ptexRainDropsRT[2];

CTexture* CTexture::s_ptexRT_ShadowPool;
CTexture* CTexture::s_ptexRT_ShadowStub;
CTexture* CTexture::s_ptexCloudsLM;

CTexture* CTexture::s_ptexSceneTarget = NULL;
CTexture* CTexture::s_ptexSceneTargetR11G11B10F[2] = {NULL};
CTexture* CTexture::s_ptexSceneTargetScaledR11G11B10F[4] = {NULL};
CTexture* CTexture::s_ptexCurrSceneTarget;
CTexture* CTexture::s_ptexCurrentSceneDiffuseAccMap;
CTexture* CTexture::s_ptexSceneDiffuseAccMap;
CTexture* CTexture::s_ptexSceneSpecularAccMap;
CTexture* CTexture::s_ptexSceneDiffuseAccMapMS;
CTexture* CTexture::s_ptexSceneSpecularAccMapMS;
CTexture* CTexture::s_ptexZTarget;
CTexture* CTexture::s_ptexZTargetDownSample[4];
CTexture* CTexture::s_ptexZTargetScaled;
CTexture* CTexture::s_ptexZTargetScaled2;
CTexture* CTexture::s_ptexHDRTarget;
CTexture* CTexture::s_ptexVelocity;
CTexture* CTexture::s_ptexVelocityTiles[3] = {NULL};
CTexture* CTexture::s_ptexVelocityObjects[2] = {NULL};

CTexture* CTexture::s_ptexFurZTarget;
CTexture* CTexture::s_ptexFurLightAcc;
CTexture* CTexture::s_ptexFurPrepass;

// Confetti Begin: David Srour
CTexture* CTexture::s_ptexGmemStenLinDepth = NULL;
// Confetti End
CTexture* CTexture::s_ptexHDRTargetPrev = NULL;
CTexture* CTexture::s_ptexHDRTargetScaled[4];
CTexture* CTexture::s_ptexHDRTargetScaledTmp[4];
CTexture* CTexture::s_ptexHDRTargetScaledTempRT[4];
CTexture* CTexture::s_ptexHDRDofLayers[2];

CTexture* CTexture::s_ptexSceneCoCHistory[2] = {};
CTexture* CTexture::s_ptexSceneCoC[MIN_DOF_COC_K] = {};
CTexture* CTexture::s_ptexSceneCoCTemp = NULL;
CTexture* CTexture::s_ptexHDRTempBloom[2];
CTexture* CTexture::s_ptexHDRFinalBloom;
CTexture* CTexture::s_ptexHDRAdaptedLuminanceCur[8];
int CTexture::s_nCurLumTextureIndex;
CTexture* CTexture::s_ptexCurLumTexture;
CTexture* CTexture::s_ptexHDRToneMaps[NUM_HDR_TONEMAP_TEXTURES];
CTexture* CTexture::s_ptexHDRMeasuredLuminance[MAX_GPU_NUM];
CTexture* CTexture::s_ptexHDRMeasuredLuminanceDummy;
CTexture* CTexture::s_ptexSkyDomeMie;
CTexture* CTexture::s_ptexSkyDomeRayleigh;
CTexture* CTexture::s_ptexSkyDomeMoon;
CTexture* CTexture::s_ptexVolObj_Density;
CTexture* CTexture::s_ptexVolObj_Shadow;
CTexture* CTexture::s_ptexColorChart;
CTexture* CTexture::s_ptexSceneTargetScaled;
CTexture* CTexture::s_ptexSceneTargetScaledBlurred;
CTexture* CTexture::s_ptexStereoL = NULL;
CTexture* CTexture::s_ptexStereoR = NULL;

CTexture* CTexture::s_ptexFlaresOcclusionRing[MAX_OCCLUSION_READBACK_TEXTURES] = {NULL};
CTexture* CTexture::s_ptexFlaresGather = NULL;

SEnvTexture CTexture::s_EnvCMaps[MAX_ENVCUBEMAPS];
SEnvTexture CTexture::s_EnvTexts[MAX_ENVTEXTURES];

StaticInstance<TArray<SEnvTexture>> CTexture::s_CustomRT_2D;

StaticInstance<TArray<CTexture>> CTexture::s_ShaderTemplates(EFTT_MAX);
bool CTexture::s_ShaderTemplatesInitialized = false;

CTexture* CTexture::s_pTexNULL = 0;

CTexture* CTexture::s_pBackBuffer;
CTexture* CTexture::s_FrontBufferTextures[2] = { NULL };

CTexture* CTexture::s_ptexVolumetricFog = NULL;
CTexture* CTexture::s_ptexVolumetricFogDensityColor = NULL;
CTexture* CTexture::s_ptexVolumetricFogDensity = NULL;
CTexture* CTexture::s_ptexVolumetricClipVolumeStencil = NULL;

#if defined(TEXSTRM_DEFERRED_UPLOAD)
ID3D11DeviceContext* CTexture::s_pStreamDeferredCtx;
#endif

#if defined(VOLUMETRIC_FOG_SHADOWS)
CTexture* CTexture::s_ptexVolFogShadowBuf[2] = {0};
#endif

CTexture* CTexture::s_defaultEnvironmentProbeDummy = nullptr;

ETEX_Format CTexture::s_eTFZ = eTF_R32F;

//============================================================

SResourceView SResourceView::ShaderResourceView(ETEX_Format nFormat, int nFirstSlice, int nSliceCount, int nMostDetailedMip, int nMipCount, bool bSrgbRead, bool bMultisample)
{
    SResourceView result(0);

    result.m_Desc.eViewType = eShaderResourceView;
    result.m_Desc.nFormat = nFormat;
    result.m_Desc.nFirstSlice = nFirstSlice;
    result.m_Desc.nSliceCount = nSliceCount;
    result.m_Desc.nMostDetailedMip = nMostDetailedMip;
    result.m_Desc.nMipCount = nMipCount;
    result.m_Desc.bSrgbRead = bSrgbRead ? 1 : 0;
    result.m_Desc.bMultisample = bMultisample ? 1 : 0;

    return result;
}

SResourceView SResourceView::RenderTargetView(ETEX_Format nFormat, int nFirstSlice, int nSliceCount, int nMipLevel, bool bMultisample)
{
    SResourceView result(0);

    result.m_Desc.eViewType = eRenderTargetView;
    result.m_Desc.nFormat = nFormat;
    result.m_Desc.nFirstSlice = nFirstSlice;
    result.m_Desc.nSliceCount = nSliceCount;
    result.m_Desc.nMostDetailedMip = nMipLevel;
    result.m_Desc.bMultisample = bMultisample ? 1 : 0;

    return result;
}

SResourceView SResourceView::DepthStencilView(ETEX_Format nFormat, int nFirstSlice, int nSliceCount, int nMipLevel, int nFlags, bool bMultisample)
{
    SResourceView result(0);

    result.m_Desc.eViewType = eDepthStencilView;
    result.m_Desc.nFormat = nFormat;
    result.m_Desc.nFirstSlice = nFirstSlice;
    result.m_Desc.nSliceCount = nSliceCount;
    result.m_Desc.nMostDetailedMip = nMipLevel;
    result.m_Desc.nFlags = nFlags;
    result.m_Desc.bMultisample = bMultisample ? 1 : 0;

    return result;
}

SResourceView SResourceView::UnorderedAccessView(ETEX_Format nFormat, int nFirstSlice, int nSliceCount, int nMipLevel, int nFlags)
{
    SResourceView result(0);

    result.m_Desc.eViewType = eUnorderedAccessView;
    result.m_Desc.nFormat = nFormat;
    result.m_Desc.nFirstSlice = nFirstSlice;
    result.m_Desc.nSliceCount = nSliceCount;
    result.m_Desc.nMostDetailedMip = nMipLevel;
    result.m_Desc.nFlags = (nFlags & FT_USAGE_UAV_RWTEXTURE) ? 1 : 0;

    return result;
}

//============================================================

CTexture::~CTexture()
{
    // sizes of these structures should NOT exceed L2 cache line!
    // offsetof with MSVC's crt and clang produces an error
#if defined(PLATFORM_64BIT) && !(defined(AZ_PLATFORM_WINDOWS) && defined(AZ_COMPILER_CLANG))
    COMPILE_TIME_ASSERT((offsetof(CTexture, m_composition) - offsetof(CTexture, m_pFileTexMips)) <= 64);
    COMPILE_TIME_ASSERT((offsetof(CTexture, m_pFileTexMips) % 64) == 0);
#endif

#ifndef _RELEASE
    if (!gRenDev->m_pRT->IsRenderThread() || gRenDev->m_pRT->IsRenderLoadingThread())
    {
        __debugbreak();
    }
#endif

#ifndef _RELEASE
    if (IsStreaming())
    {
        __debugbreak();
    }
#endif

    if (gRenDev && gRenDev->m_pRT)
    {
        gRenDev->m_pRT->RC_ReleaseDeviceTexture(this);
    }

    if (m_pFileTexMips)
    {
        Unlink();
        StreamState_ReleaseInfo(this, m_pFileTexMips);
        m_pFileTexMips = NULL;
    }

#ifdef ENABLE_TEXTURE_STREAM_LISTENER
    if (s_pStreamListener)
    {
        s_pStreamListener->OnDestroyedStreamedTexture(this);
    }
#endif

#ifndef _RELEASE
    if (m_bInDistanceSortedList)
    {
        __debugbreak();
    }
#endif

#ifdef TEXTURE_GET_SYSTEM_COPY_SUPPORT
    s_LowResSystemCopy.erase(this);
#endif

#if defined(USE_UNIQUE_MUTEX_PER_TEXTURE)
    if (gEnv->IsEditor())
    {
        // Only the editor allocated a unique mutex per texture.
        delete m_invalidateCallbacksMutex;
        m_invalidateCallbacksMutex = nullptr;
    }
#endif
}

void CTexture::RT_ReleaseDevice()
{
    ReleaseDeviceTexture(false);
}

const CCryNameTSCRC& CTexture::mfGetClassName()
{
    return s_sClassName;
}

CCryNameTSCRC CTexture::GenName(const char* name, uint32 nFlags)
{
    char buffer[AZ_MAX_PATH_LEN];
    IResourceCompilerHelper::GetOutputFilename(name, buffer, sizeof(buffer));   // change texture filename extensions to dds before we compute the crc
    stack_string strName = buffer;
    strName.MakeLower();

    //'\\' in texture names causing duplication
    PathUtil::ToUnixPath(strName);

    if (nFlags & FT_ALPHA)
    {
        strName += "_a";
    }

    return CCryNameTSCRC(strName.c_str());
}

class StrComp
{
public:
    bool operator () (const char* s1, const char* s2) const {return strcmp(s1, s2) < 0; }
};


CTexture* CTexture::GetByID(int nID)
{
    CTexture* pTex = NULL;

    const CCryNameTSCRC& className = mfGetClassName();
    CBaseResource* pBR = CBaseResource::GetResource(className, nID, false);
    if (!pBR)
    {
        return CTextureManager::Instance()->GetNoTexture();
    }
    pTex = (CTexture*)pBR;
    return pTex;
}

CTexture* CTexture::GetByName(const char* szName, uint32 flags)
{
    CTexture* pTex = NULL;

    CCryNameTSCRC Name = GenName(szName, flags);

    CBaseResource* pBR = CBaseResource::GetResource(mfGetClassName(), Name, false);
    if (!pBR)
    {
        return NULL;
    }
    pTex = (CTexture*)pBR;
    return pTex;
}

CTexture* CTexture::GetByNameCRC(CCryNameTSCRC Name)
{
    CTexture* pTex = NULL;

    CBaseResource* pBR = CBaseResource::GetResource(mfGetClassName(), Name, false);
    if (!pBR)
    {
        return NULL;
    }
    pTex = (CTexture*)pBR;
    return pTex;
}

CTexture* CTexture::NewTexture(const char* name, uint32 nFlags, ETEX_Format eTFDst, bool& bFound)
{
    AZ_ASSET_NAMED_SCOPE("CTexture::NewTexture: %s", name);

    CTexture* pTex = NULL;
    AZStd::string normalizedFile;
    AZStd::string fileExtension;
    AzFramework::StringFunc::Path::GetExtension(name, fileExtension);
    if (name[0] == '$' || fileExtension.empty())
    {
        //If the name starts with $ or it does not have any extension then it is one of the special texture
        // that the engine requires and we would not be modifying the name
        normalizedFile = name;
    }
    else
    {
        char buffer[AZ_MAX_PATH_LEN];
        IResourceCompilerHelper::GetOutputFilename(name, buffer, sizeof(buffer));   // change texture filename extensions to dds
        normalizedFile = buffer;
        AZStd::to_lower(normalizedFile.begin(), normalizedFile.end());
        PathUtil::ToUnixPath(normalizedFile.c_str());
    }

    CCryNameTSCRC Name = GenName(normalizedFile.c_str(), nFlags);

    CBaseResource* pBR = CBaseResource::GetResource(mfGetClassName(), Name, false);
    if (!pBR)
    {
        //If a texture name ends in _stereo we want to create a stereo texture
        const AZStd::string ending = "_stereo";
        AZStd::string fullName = normalizedFile.c_str();
        size_t nameLength = fullName.length();
        size_t endingLength = ending.length();
        if (nameLength > endingLength && fullName.compare(nameLength - endingLength, endingLength, ending) == 0)
        {
            pTex = new CStereoTexture(normalizedFile.c_str(), eTFDst, nFlags);
        }
        else
        {
            pTex = new CTexture(nFlags);
        }
        pTex->Register(mfGetClassName(), Name);
        bFound = false;
        pTex->m_nFlags = nFlags;
        pTex->m_eTFDst = eTFDst;
        pTex->m_SrcName = normalizedFile.c_str();
    }
    else
    {
        pTex = (CTexture*)pBR;
        pTex->AddRef();
        bFound = true;
    }

    return pTex;
}

void CTexture::SetDevTexture([[maybe_unused]] CDeviceTexture* pDeviceTex)
{
#if !defined(NULL_RENDERER)
    SAFE_RELEASE(m_pDevTexture);
    m_pDevTexture = pDeviceTex;
    if (m_pDevTexture)
    {
        m_pDevTexture->SetNoDelete(!!(m_nFlags & FT_DONT_RELEASE));
    }
    InvalidateDeviceResource(eDeviceResourceDirty);
#endif
}

void CTexture::PostCreate()
{
    m_nUpdateFrameID = gRenDev->GetFrameID(false);
    m_bPostponed = false;
}

CTexture* CTexture::CreateTextureObject(const char* name, uint32 nWidth, uint32 nHeight, int nDepth, ETEX_Type eTT, uint32 nFlags, ETEX_Format eTF, int nCustomID)
{
    SYNCHRONOUS_LOADING_TICK();

    bool bFound = false;

    CTexture* pTex = NewTexture(name, nFlags, eTF, bFound);
    if (bFound)
    {
        if (!pTex->m_nWidth)
        {
            pTex->m_nWidth = nWidth;
        }
        if (!pTex->m_nHeight)
        {
            pTex->m_nHeight = nHeight;
        }
        pTex->m_nFlags |= nFlags & (FT_DONT_RELEASE | FT_USAGE_RENDERTARGET);

        return pTex;
    }
    pTex->m_nDepth = nDepth;
    pTex->m_nWidth = nWidth;
    pTex->m_nHeight = nHeight;
    pTex->m_eTT = eTT;
    pTex->m_eTFDst = eTF;
    pTex->m_nCustomID = nCustomID;
    pTex->m_SrcName = name;

    return pTex;
}

void CTexture::GetMemoryUsage(ICrySizer* pSizer) const
{
    pSizer->Add(*this);
    pSizer->AddObject(m_SrcName);

#ifdef TEXTURE_GET_SYSTEM_COPY_SUPPORT
    const LowResSystemCopyType::iterator& it = s_LowResSystemCopy.find(this);
    if (it != CTexture::s_LowResSystemCopy.end())
    {
        pSizer->AddObject((*it).second.m_lowResSystemCopy);
    }
#endif

    if (m_pFileTexMips)
    {
        m_pFileTexMips->GetMemoryUsage(pSizer, m_nMips, m_CacheFileHeader.m_nSides);
    }
}


CTexture* CTexture::CreateTextureArray(const char* name, ETEX_Type eType, uint32 nWidth, uint32 nHeight, uint32 nArraySize, int nMips, uint32 nFlags, ETEX_Format eTF, int nCustomID)
{
    assert(eType == eTT_2D || eType == eTT_Cube);

    if (nArraySize > 255)
    {
        assert(0);
        return NULL;
    }

    if (nMips <= 0)
    {
        nMips = CTexture::CalcNumMips(nWidth, nHeight);
    }

    bool sRGB = (nFlags & FT_USAGE_ALLOWREADSRGB) != 0;
    nFlags &= ~FT_USAGE_ALLOWREADSRGB;

    CTexture* pTex = CreateTextureObject(name, nWidth, nHeight, 1, eType, nFlags, eTF, nCustomID);
    pTex->m_nWidth = nWidth;
    pTex->m_nHeight = nHeight;
    pTex->m_nArraySize = nArraySize;
    pTex->m_nFlags |= eType == eTT_Cube ? FT_REPLICATE_TO_ALL_SIDES : 0;

    if (nFlags & FT_USAGE_RENDERTARGET)
    {
        bool bRes = pTex->CreateRenderTarget(eTF, Clr_Unknown);
        if (!bRes)
        {
            pTex->m_nFlags |= FT_FAILED;
        }
        pTex->PostCreate();
    }
    else
    {
        STexData td;
        td.m_eTF = eTF;
        td.m_nDepth = 1;
        td.m_nWidth = nWidth;
        td.m_nHeight = nHeight;
        td.m_nMips = nMips;
        td.m_nFlags = sRGB ? FIM_SRGB_READ : 0;

        bool bRes = pTex->CreateTexture(td);
        if (!bRes)
        {
            pTex->m_nFlags |= FT_FAILED;
        }
        pTex->PostCreate();
    }

    pTex->m_nFlags &= ~FT_REPLICATE_TO_ALL_SIDES;

    return pTex;
}

CTexture* CTexture::CreateRenderTarget(const char* name, uint32 nWidth, uint32 nHeight, const ColorF& cClear, ETEX_Type eTT, uint32 nFlags, ETEX_Format eTF, int nCustomID)
{
    AZ_ASSET_NAMED_SCOPE("CTexture::CreateRenderTarget: %s", name);

    CTexture* pTex = CreateTextureObject(name, nWidth, nHeight, 1, eTT, nFlags | FT_USAGE_RENDERTARGET, eTF, nCustomID);
    pTex->m_nWidth = nWidth;
    pTex->m_nHeight = nHeight;
    pTex->m_nFlags |= nFlags;

    bool bRes = pTex->CreateRenderTarget(eTF, cClear);
    if (!bRes)
    {
        pTex->m_nFlags |= FT_FAILED;
    }
    pTex->PostCreate();

    return pTex;
}

// Create2DTextureWithMips is similar to Create2DTexture, but it also propagates the mip argument correctly.
// The original Create2DTexture function force sets mips to 1.
// This has been separated from Create2DTexture to ensure that we preserve backwards compatibility.
bool CTexture::Create2DTextureWithMips(int nWidth, int nHeight, int nMips, [[maybe_unused]] int nFlags, const byte* pData, ETEX_Format eTFSrc, [[maybe_unused]] ETEX_Format eTFDst)
{
    if (nMips <= 0)
    {
        nMips = CTexture::CalcNumMips(nWidth, nHeight);
    }
    m_eTFSrc = eTFSrc;
    m_nMips = nMips;

    STexData td;
    td.m_eTF = eTFSrc;
    td.m_nDepth = 1;
    td.m_nWidth = nWidth;
    td.m_nHeight = nHeight;
    // Propagate mips correctly.  (Create2DTexture always sets this to 1)
    td.m_nMips = nMips; 
    td.m_pData[0] = pData;

    bool bRes = CreateTexture(td);
    if (!bRes)
    {
        m_nFlags |= FT_FAILED;
    }

    PostCreate();

    return bRes;
}

bool CTexture::Create2DTexture(int nWidth, int nHeight, int nMips, [[maybe_unused]] int nFlags, const byte* pData, ETEX_Format eTFSrc, [[maybe_unused]] ETEX_Format eTFDst)
{
    if (nMips <= 0)
    {
        nMips = CTexture::CalcNumMips(nWidth, nHeight);
    }
    m_eTFSrc = eTFSrc;
    m_nMips = nMips;

    STexData td;
    td.m_eTF = eTFSrc;
    td.m_nDepth = 1;
    td.m_nWidth = nWidth;
    td.m_nHeight = nHeight;
    td.m_nMips = 1;
    td.m_pData[0] = pData;

    bool bRes = CreateTexture(td);
    if (!bRes)
    {
        m_nFlags |= FT_FAILED;
    }

    PostCreate();

    return bRes;
}

CTexture* CTexture::Create2DTexture(const char* szName, int nWidth, int nHeight, int nMips, int nFlags, const byte* pData, ETEX_Format eTFSrc, ETEX_Format eTFDst, bool bAsyncDevTexCreation)
{
    FUNCTION_PROFILER_FAST(GetISystem(), PROFILE_RENDERER, g_bProfilerEnabled);

    CTexture* pTex = CreateTextureObject(szName, nWidth, nHeight, 1, eTT_2D, nFlags, eTFDst, -1);
    pTex->m_bAsyncDevTexCreation = bAsyncDevTexCreation;

    pTex->Create2DTexture(nWidth, nHeight, nMips, nFlags, pData, eTFSrc, eTFDst);

    return pTex;
}

bool CTexture::Create3DTexture(int nWidth, int nHeight, int nDepth, int nMips, [[maybe_unused]] int nFlags, const byte* pData, ETEX_Format eTFSrc, [[maybe_unused]] ETEX_Format eTFDst)
{
    m_eTFSrc = eTFSrc;
    m_nMips = nMips;

    STexData td;
    td.m_eTF = eTFSrc;
    td.m_nWidth = nWidth;
    td.m_nHeight = nHeight;
    td.m_nDepth = nDepth;
    td.m_nMips = nMips;
    td.m_pData[0] = pData;

    bool bRes = CreateTexture(td);
    if (!bRes)
    {
        m_nFlags |= FT_FAILED;
    }

    PostCreate();

    return bRes;
}

CTexture* CTexture::Create3DTexture(const char* szName, int nWidth, int nHeight, int nDepth, int nMips, int nFlags, const byte* pData, ETEX_Format eTFSrc, ETEX_Format eTFDst)
{
    CTexture* pTex = CreateTextureObject(szName, nWidth, nHeight, nDepth, eTT_3D, nFlags, eTFDst, -1);

    pTex->Create3DTexture(nWidth, nHeight, nDepth, nMips, nFlags, pData, eTFSrc, eTFDst);

    return pTex;
}

CTexture* CTexture::Create2DCompositeTexture(const char* szName, int nWidth, int nHeight, int nMips, int nFlags, ETEX_Format eTFDst, const STexComposition* pCompositions, size_t nCompositions)
{
    nFlags |= FT_COMPOSITE;
    nFlags &= ~FT_DONT_STREAM;

    bool bFound = false;
    CTexture* pTex = NewTexture(szName, nFlags, eTFDst, bFound);

    if (!bFound)
    {
        pTex->m_nWidth = nWidth;
        pTex->m_nHeight = nHeight;
        pTex->m_nMips = nMips;
        pTex->m_composition.assign(pCompositions, pCompositions + nCompositions);

        // Strip all invalid textures from the composition

        int w = 0;
        for (int r = 0, c = pTex->m_composition.size(); r != c; ++r)
        {
            if (!pTex->m_composition[r].pTexture)
            {
                CryWarning(VALIDATOR_MODULE_RENDERER, VALIDATOR_WARNING, "Composition %i for '%s' is missing", r, szName);
                continue;
            }

            if (r != w)
            {
                pTex->m_composition[w] = pTex->m_composition[r];
            }
            ++w;
        }
        pTex->m_composition.resize(w);

        if (CTexture::s_bPrecachePhase)
        {
            pTex->m_bPostponed = true;
            pTex->m_bWasUnloaded = true;
        }
        else
        {
            pTex->StreamPrepareComposition();
        }
    }

    return pTex;
}

bool CTexture::Reload()
{
    const byte* pData[6];
    int i;
    bool bOK = false;

    if (IsStreamed())
    {
        ReleaseDeviceTexture(false);
        return ToggleStreaming(true);
    }

    for (i = 0; i < 6; i++)
    {
        pData[i] = 0;
    }
    if (m_nFlags & FT_FROMIMAGE)
    {
        assert(!(m_nFlags & FT_USAGE_RENDERTARGET));
        bOK = LoadFromImage(m_SrcName.c_str()); // true=reloading
        if (!bOK)
        {
            SetNoTexture(m_eTT == eTT_Cube ? CTextureManager::Instance()->GetNoTextureCM() : CTextureManager::Instance()->GetNoTexture() );
        }
    }
    else
    if (m_nFlags & (FT_USAGE_RENDERTARGET | FT_USAGE_DYNAMIC))
    {
        bOK = CreateDeviceTexture(pData);
        assert(bOK);
    }

    // Post Create assumes the texture loaded successfully so don't call it if that's not the case
    if (bOK)
    {
        PostCreate();
    }

    return bOK;
}

CTexture* CTexture::ForName(const char* name, uint32 nFlags, ETEX_Format eTFDst)
{
    SLICE_AND_SLEEP();
    AZ_ASSET_NAMED_SCOPE("CTexture::ForName: %s", name);

    bool bFound = false;

    CRY_DEFINE_ASSET_SCOPE("Texture", name);

    CTexture* pTex = NewTexture(name, nFlags, eTFDst, bFound);
    if (bFound || name[0] == '$')
    {
        if (!bFound)
        {
            pTex->m_SrcName = name;
        }
        else
        {
            // switch off streaming for the same texture with the same flags except DONT_STREAM
            if ((nFlags & FT_DONT_STREAM) != 0 && (pTex->GetFlags() & FT_DONT_STREAM) == 0)
            {
                if (!pTex->m_bPostponed)
                {
                    pTex->ReleaseDeviceTexture(false);
                }
                pTex->m_nFlags |= FT_DONT_STREAM;
                if (!pTex->m_bPostponed)
                {
                    pTex->Reload();
                }
            }
        }

        return pTex;
    }
    pTex->m_SrcName = name;

#ifndef _RELEASE
    pTex->m_sAssetScopeName = gEnv->pLog->GetAssetScopeString();
#endif
    bool bPrecachePhase = CTexture::s_bPrecachePhase && !(nFlags & FT_IGNORE_PRECACHE);

    ESystemGlobalState currentGlobalState = GetISystem()->GetSystemGlobalState();
    const bool levelLoading = currentGlobalState == ESYSTEM_GLOBAL_STATE_LEVEL_LOAD_START;

    // Load textures immediately during level load since texture load
    // requests during this phase are probably coming from a loading screen.
    if (levelLoading || !bPrecachePhase)
    {
        pTex->Load(eTFDst);
    }
    else
    {
        // NOTE: attached alpha isn't detectable by flags before the header is loaded, so we do it by file-suffix
        if (/*(nFlags & FT_TEX_NORMAL_MAP) &&*/ TextureHelpers::VerifyTexSuffix(EFTT_NORMALS, name) && TextureHelpers::VerifyTexSuffix(EFTT_SMOOTHNESS, name))
        {
            nFlags |= FT_HAS_ATTACHED_ALPHA;
        }

        pTex->m_eTFDst = eTFDst;
        pTex->m_nFlags = nFlags;
        pTex->m_bPostponed = true;
        pTex->m_bWasUnloaded = true;
    }

    return pTex;
}

struct CompareTextures
{
    bool operator()(const CTexture* a, const CTexture* b)
    {
        return (azstricmp(a->GetSourceName(), b->GetSourceName()) < 0);
    }
};

void CTexture::Precache()
{
    LOADING_TIME_PROFILE_SECTION(iSystem);

    if (!s_bPrecachePhase)
    {
        return;
    }
    if (!gRenDev)
    {
        return;
    }

    CryLog("Requesting textures precache ...");

    gRenDev->m_pRT->RC_PreloadTextures();
}

void CTexture::RT_Precache()
{
    if (gRenDev->CheckDeviceLost())
    {
        return;
    }

    LOADING_TIME_PROFILE_SECTION(iSystem);
    AZ_TRACE_METHOD();

    // Disable invalid file access logging if texture streaming is disabled
    // If texture streaming is turned off, we will hit this on the renderthread
    // and stall due to the invalid file access stalls
    ICVar* sysPakLogInvalidAccess = NULL;
    int pakLogFileAccess = 0;
    if IsCVarConstAccess(constexpr) (!CRenderer::CV_r_texturesstreaming)
    {
        sysPakLogInvalidAccess = gEnv->pConsole->GetCVar("sys_PakLogInvalidFileAccess");
        if (sysPakLogInvalidAccess)
        {
            pakLogFileAccess = sysPakLogInvalidAccess->GetIVal();
        }
    }

    CTimeValue t0 = gEnv->pTimer->GetAsyncTime();
    CryLog("-- Precaching textures...");
    iLog->UpdateLoadingScreen(0);

    std::vector<CTexture*> TexturesForPrecaching;
    std::vector<CTexture*> TexturesForComposition;

    bool bTextureCacheExists = false;

    {
        AUTO_LOCK(CBaseResource::s_cResLock);

        SResourceContainer* pRL = CBaseResource::GetResourcesForClass(CTexture::mfGetClassName());
        if (pRL)
        {
            TexturesForPrecaching.reserve(pRL->m_RMap.size());

            ResourcesMapItor itor;
            for (itor = pRL->m_RMap.begin(); itor != pRL->m_RMap.end(); itor++)
            {
                CTexture* tp = (CTexture*)itor->second;
                if (!tp)
                {
                    continue;
                }
                if (tp->CTexture::IsPostponed())
                {
                    if (tp->CTexture::GetFlags() & FT_COMPOSITE)
                    {
                        TexturesForComposition.push_back(tp);
                    }
                    else
                    {
                        TexturesForPrecaching.push_back(tp);
                    }
                }
            }
        }
    }

    // Preload all the post poned textures
    {
        if (!gEnv->IsEditor())
        {
            CryLog("=============================== Loading textures ================================");
        }

        std::vector<CTexture*>& Textures = TexturesForPrecaching;
        std::sort(Textures.begin(), Textures.end(), CompareTextures());

        gEnv->pSystem->GetStreamEngine()->PauseStreaming(false, 1 << eStreamTaskTypeTexture);

        for (uint32 i = 0; i < Textures.size(); i++)
        {
            CTexture* tp = Textures[i];

            if (!CRenderer::CV_r_texturesstreaming || !tp->m_bStreamPrepared)
            {
                tp->m_bPostponed = false;
                tp->Load(tp->m_eTFDst);
            }
        }

        while (s_StreamPrepTasks.GetNumLive())
        {
            if (gRenDev->m_pRT->IsRenderThread() && !gRenDev->m_pRT->IsRenderLoadingThread())
            {
                StreamState_Update();
                StreamState_UpdatePrep();
            }
            else if (gRenDev->m_pRT->IsRenderLoadingThread())
            {
                StreamState_UpdatePrep();
            }

            CrySleep(10);
        }

        for (uint32 i = 0; i < Textures.size(); i++)
        {
            CTexture* tp = Textures[i];

            if (tp->m_bStreamed && tp->m_bForceStreamHighRes)
            {
                tp->m_bStreamHighPriority |= 1;
                tp->m_fpMinMipCur = 0;
                s_pTextureStreamer->Precache(tp);
            }
        }

        if (!gEnv->IsEditor())
        {
            CryLog("========================== Finished loading textures ============================");
        }
    }

    {
        std::vector<CTexture*>& Textures = TexturesForComposition;

        for (uint32 i = 0; i < Textures.size(); i++)
        {
            CTexture* tp = Textures[i];

            if (!CRenderer::CV_r_texturesstreaming || !tp->m_bStreamPrepared)
            {
                tp->m_bPostponed = false;
                tp->StreamPrepareComposition();
            }
        }

        for (uint32 i = 0; i < Textures.size(); i++)
        {
            CTexture* tp = Textures[i];

            if (tp->m_bStreamed && tp->m_bForceStreamHighRes)
            {
                tp->m_bStreamHighPriority |= 1;
                tp->m_fpMinMipCur = 0;
                s_pTextureStreamer->Precache(tp);
            }
        }
    }

    if (bTextureCacheExists)
    {
        //GetISystem()->GetIResourceManager()->UnloadLevelCachePak( TEXTURE_LEVEL_CACHE_PAK );
    }

    CTimeValue t1 = gEnv->pTimer->GetAsyncTime();
    float dt = (t1 - t0).GetSeconds();
    CryLog("Precaching textures done in %.2f seconds", dt);

    s_bPrecachePhase = false;

    // Restore pakLogFileAccess if it was disabled during precaching
    // because texture precaching was disabled
    if (pakLogFileAccess)
    {
        sysPakLogInvalidAccess->Set(pakLogFileAccess);
    }
}

bool CTexture::Load([[maybe_unused]] ETEX_Format eTFDst)
{
    LOADING_TIME_PROFILE_SECTION_NAMED_ARGS("CTexture::Load(ETEX_Format eTFDst)", m_SrcName.c_str());
    m_bWasUnloaded = false;
    m_bStreamed = false;

#if !defined(NULL_RENDERER)
    bool bFound = LoadFromImage(m_SrcName.c_str(), eTFDst);     // false=not reloading
#else
    bool bFound = false;
#endif

    if (!bFound)
    {
        SetNoTexture(m_eTT == eTT_Cube ? CTextureManager::Instance()->GetNoTextureCM() : CTextureManager::Instance()->GetNoTexture());
    }

    m_nFlags |= FT_FROMIMAGE;
    PostCreate();

    return bFound;
}

bool CTexture::ToggleStreaming(const bool bEnable)
{
    if (!(m_nFlags & (FT_FROMIMAGE | FT_DONT_RELEASE)) || (m_nFlags & FT_DONT_STREAM))
    {
        return false;
    }
    AbortStreamingTasks(this);
    if (bEnable)
    {
        if (IsStreamed())
        {
            return true;
        }
        ReleaseDeviceTexture(false);
        m_bStreamed = true;
        if (StreamPrepare(true))
        {
            return true;
        }
        if (m_pFileTexMips)
        {
            Unlink();
            StreamState_ReleaseInfo(this, m_pFileTexMips);
            m_pFileTexMips = NULL;
        }
        m_bStreamed = false;
        if (m_bNoTexture)
        {
            return true;
        }
    }
    ReleaseDeviceTexture(false);
    return Reload();
}

bool CTexture::LoadFromImage(const char* name, ETEX_Format eTFDst)
{
    LOADING_TIME_PROFILE_SECTION_ARGS(name);

    if IsCVarConstAccess(constexpr) (CRenderer::CV_r_texnoload)
    {
        if (SetNoTexture(CTextureManager::Instance()->GetNoTexture() ))
        {
            return true;
        }
    }

    string sFileName(name);
    sFileName.MakeLower();

    m_eTFDst = eTFDst;

    // try to stream-in the texture
    if (CRenderer::CV_r_texturesstreaming && !(m_nFlags & FT_DONT_STREAM) && (m_eTT == eTT_2D || m_eTT == eTT_Cube))
    {
        m_bStreamed = true;
        if (StreamPrepare(true))
        {
            assert(m_pDevTexture);
            return true;
        }
        m_nFlags |= FT_DONT_STREAM;
        m_bStreamed = false;
        m_bForceStreamHighRes = false;
        if (m_bNoTexture)
        {
            if (m_pFileTexMips)
            {
                Unlink();
                StreamState_ReleaseInfo(this, m_pFileTexMips);
                m_pFileTexMips = NULL;
                m_bStreamed = false;
            }
            return true;
        }
    }

#ifndef _RELEASE
    CRY_DEFINE_ASSET_SCOPE("Texture", m_sAssetScopeName);
#endif

    if (m_bPostponed)
    {
        if (s_pTextureStreamer->BeginPrepare(this, sFileName, (m_nFlags & FT_ALPHA) ? FIM_ALPHA : 0))
        {
            return true;
        }
    }


    uint32 nImageFlags = (m_nFlags & FT_ALPHA) ? FIM_ALPHA : 0;

    if (I3DEngine* p3DEngine = gEnv->p3DEngine)
    {
        if (ITextureLoadHandler* pTextureHandler = p3DEngine->GetTextureLoadHandlerForImage(sFileName.c_str()))
        {
            STextureLoadData loadData;
            loadData.m_pTexture = this;
            loadData.m_nFlags = m_nFlags;
            if (pTextureHandler->LoadTextureData(sFileName.c_str(), loadData))
            {
                bool bHasAlphaFlag = false;

                //we must clear this or else our texture won't load properly
                if ((m_nFlags & FT_ALPHA) == FT_ALPHA)
                {
                    m_nFlags &= ~FT_ALPHA;
                    bHasAlphaFlag = true;
                }

                _smart_ptr<CImageFile> pImage = CImageFile::mfLoad_mem(sFileName, loadData.m_pData, loadData.m_Width, loadData.m_Height, loadData.m_Format, loadData.m_NumMips, loadData.m_nFlags);
                m_bisTextureMissing = !pImage || pImage->mfGet_IsImageMissing();
                loadData.m_pData = nullptr;
                bool bLoadResult = Load(&*pImage);

                if (bHasAlphaFlag)
                {
                    m_nFlags |= FT_ALPHA;
                }

                return bLoadResult;
            }
            SetNoTexture(CTextureManager::Instance()->GetNoTexture() );
            return true;
        }
    }
    _smart_ptr<CImageFile> pImage = CImageFile::mfLoad_file(sFileName, nImageFlags);
    m_bisTextureMissing = !pImage || pImage->mfGet_IsImageMissing();
    return Load(pImage);
}

bool CTexture::Load([[maybe_unused]] CImageFile* pImage)
{
#if !defined(NULL_RENDERER)
    if (!pImage || pImage->mfGetFormat() == eTF_Unknown)
    {
        return false;
    }

    LOADING_TIME_PROFILE_SECTION_NAMED_ARGS("CTexture::Load(CImageFile* pImage)", pImage->mfGet_filename().c_str());

    // DHX-104: If this failed previously (maybe because the DDS was being generated), we must unset the failure flag
    // so it doesn't appear to have failed again.
    m_nFlags &= ~FT_FAILED;
    if ((m_nFlags & FT_ALPHA) && !pImage->mfIs_image(0))
    {
        SetNoTexture( CTextureManager::Instance()->GetWhiteTexture() );
        return true;
    }
    const char* name = pImage->mfGet_filename().c_str();
    if (pImage->mfGet_Flags() & FIM_SPLITTED)                            // propagate splitted file flag
    {
        m_nFlags |= FT_SPLITTED;
    }
    if (pImage->mfGet_Flags() & FIM_X360_NOT_PRETILED)
    {
        m_nFlags |= FT_TEX_WAS_NOT_PRE_TILED;
    }
    if (pImage->mfGet_Flags() & FIM_NORMALMAP)
    {
        if (!(m_nFlags & FT_TEX_NORMAL_MAP) && !CryStringUtils::stristr(name, "_ddn"))
        {
            // becomes reported as editor error
            gEnv->pSystem->Warning(VALIDATOR_MODULE_RENDERER, VALIDATOR_WARNING, VALIDATOR_FLAG_FILE | VALIDATOR_FLAG_TEXTURE,
                name, "Not a normal map texture attempted to be used as a normal map: %s", name);
        }
    }

    if (!(m_nFlags & FT_ALPHA) && !(
            pImage->mfGetFormat() == eTF_BC5U || pImage->mfGetFormat() == eTF_BC5S || pImage->mfGetFormat() == eTF_BC7 || pImage->mfGetFormat() == eTF_EAC_RG11
            ) && CryStringUtils::stristr(name, "_ddn") != 0 && GetDevTexture()) // improvable code
    {
        // becomes reported as editor error
        gEnv->pSystem->Warning(VALIDATOR_MODULE_RENDERER, VALIDATOR_WARNING, VALIDATOR_FLAG_FILE | VALIDATOR_FLAG_TEXTURE,
            name, "Wrong format '%s' for normal map texture '%s'", CTexture::GetFormatName(), name);
    }

    if (pImage->mfGet_Flags() & FIM_NOTSUPPORTS_MIPS && !(m_nFlags & FT_NOMIPS))
    {
        m_nFlags |= FT_FORCE_MIPS;
    }
    if (pImage->mfGet_Flags() & FIM_HAS_ATTACHED_ALPHA)
    {
        m_nFlags |= FT_HAS_ATTACHED_ALPHA;      // if the image has alpha attached we store this in the CTexture
    }
    m_eSrcTileMode = pImage->mfGetTileMode();

    STexData td;
    td.m_nFlags = pImage->mfGet_Flags();
    td.m_pData[0] = pImage->mfGet_image(0);
    td.m_nWidth = pImage->mfGet_width();
    td.m_nHeight = pImage->mfGet_height();
    td.m_nDepth = pImage->mfGet_depth();
    td.m_eTF = pImage->mfGetFormat();
    td.m_nMips = pImage->mfGet_numMips();
    td.m_fAvgBrightness = pImage->mfGet_avgBrightness();
    td.m_cMinColor = pImage->mfGet_minColor();
    td.m_cMaxColor = pImage->mfGet_maxColor();
    if ((m_nFlags & FT_NOMIPS) || td.m_nMips <= 0)
    {
        td.m_nMips = 1;
    }
    td.m_pFilePath = pImage->mfGet_filename();

    // base range after normalization, fe. [0,1] for 8bit images, or [0,2^15] for RGBE/HDR data
    if ((td.m_eTF == eTF_R9G9B9E5) || (td.m_eTF == eTF_BC6UH) || (td.m_eTF == eTF_BC6SH))
    {
        td.m_cMinColor /= td.m_cMaxColor.a;
        td.m_cMaxColor /= td.m_cMaxColor.a;
    }

    // check if it's a cubemap
    if (pImage->mfIs_image(1))
    {
        for (int i = 1; i < 6; i++)
        {
            td.m_pData[i] = pImage->mfGet_image(i);
        }
    }

    bool bRes = false;
    if (pImage)
    {
        FormatFixup(td);
        bRes = CreateTexture(td);
    }

    for (int i = 0; i < 6; i++)
    {
        if (td.m_pData[i] && td.WasReallocated(i))
        {
            SAFE_DELETE_ARRAY(td.m_pData[i]);
        }
    }

    return bRes;
#else
    SetNoTexture(CTextureManager::Instance()->GetWhiteTexture() );
    return true;
#endif
}

bool CTexture::CreateTexture(STexData& td)
{
    m_nWidth = td.m_nWidth;
    m_nHeight = td.m_nHeight;
    m_nDepth = td.m_nDepth;
    m_eTFSrc = td.m_eTF;
    m_nMips = td.m_nMips;
    m_fAvgBrightness = td.m_fAvgBrightness;
    m_cMinColor = td.m_cMinColor;
    m_cMaxColor = td.m_cMaxColor;
    m_cClearColor = ColorF(0.0f, 0.0f, 0.0f, 1.0f);
    m_bUseDecalBorderCol = (td.m_nFlags & FIM_DECAL) != 0;
    m_bIsSRGB = (td.m_nFlags & FIM_SRGB_READ) != 0;

    assert(m_nWidth && m_nHeight && m_nMips);

    if (td.m_pData[1] || (m_nFlags & FT_REPLICATE_TO_ALL_SIDES))
    {
        m_eTT = eTT_Cube;
    }
    else
    {
        if (m_nDepth > 1 || m_eTT == eTT_3D)
        {
            m_eTT = eTT_3D;
        }
        else
        {
            m_eTT = eTT_2D;
        }
    }

    if (m_eTFDst == eTF_Unknown)
    {
        m_eTFDst = m_eTFSrc;
    }

    if (!ImagePreprocessing(td))
    {
        return false;
    }

    assert(m_nWidth && m_nHeight && m_nMips);

    const int nMaxTextureSize = gRenDev->GetMaxTextureSize();
    if (nMaxTextureSize > 0)
    {
        if (m_nWidth > nMaxTextureSize || m_nHeight > nMaxTextureSize)
        {
            return false;
        }
    }

    const byte* pData[6];
    for (uint32 i = 0; i < 6; i++)
    {
        pData[i] = td.m_pData[i];
    }

    bool bRes = CreateDeviceTexture(pData);

    return bRes;
}

ETEX_Format CTexture::FormatFixup(ETEX_Format src)
{
    switch (src)
    {
    case eTF_L8V8U8X8:
        return eTF_R8G8B8A8S;
    case eTF_B8G8R8:
        return eTF_R8G8B8A8;
    case eTF_L8V8U8:
        return eTF_R8G8B8A8S;
    case eTF_L8:
        return eTF_R8G8B8A8;
    case eTF_A8L8:
        return eTF_R8G8B8A8;

    case eTF_B5G5R5:
        return eTF_R8G8B8A8;
    case eTF_B5G6R5:
        return eTF_R8G8B8A8;
    case eTF_B4G4R4A4:
        return eTF_R8G8B8A8;

    default:
        return src;
    }
}

bool CTexture::FormatFixup(STexData& td)
{
    const ETEX_Format targetFmt = FormatFixup(td.m_eTF);

    if (m_eSrcTileMode == eTM_None)
    {
        // Try and expand
        int nSourceSize = CTexture::TextureDataSize(td.m_nWidth, td.m_nHeight, td.m_nDepth, td.m_nMips, 1, td.m_eTF);
        int nTargetSize = CTexture::TextureDataSize(td.m_nWidth, td.m_nHeight, td.m_nDepth, td.m_nMips, 1, targetFmt);

        for (int nImage = 0; nImage < sizeof(td.m_pData) / sizeof(td.m_pData[0]); ++nImage)
        {
            if (td.m_pData[nImage])
            {
                byte* pNewImage = new byte[nTargetSize];
                CTexture::ExpandMipFromFile(pNewImage, nTargetSize, td.m_pData[nImage], nSourceSize, td.m_eTF);
                td.AssignData(nImage, pNewImage);
            }
        }

        td.m_eTF = targetFmt;
    }
    else
    {
#ifndef _RELEASE
        if (targetFmt != td.m_eTF)
        {
            __debugbreak();
        }
#endif
    }

    return true;
}

bool CTexture::ImagePreprocessing(STexData& td)
{
    FUNCTION_PROFILER_FAST(GetISystem(), PROFILE_RENDERER, g_bProfilerEnabled);

#if !defined(_RELEASE)
    const char* pTexFileName = td.m_pFilePath ? td.m_pFilePath : "$Unknown";
#endif

    const ETEX_Format eTFDst = ClosestFormatSupported(m_eTFDst);
    if (eTFDst == eTF_Unknown)
    {
        td.m_pData[0] = td.m_pData[1] = td.m_pData[2] = td.m_pData[3] = td.m_pData[4] = td.m_pData[5] = 0;
        m_nWidth = m_nHeight = m_nDepth = m_nMips = 0;

#if !defined(_RELEASE)
        TextureError(pTexFileName, "Trying to create a texture with unsupported target format %s!", NameForTextureFormat(eTFDst));
#endif
        return false;
    }

    const ETEX_Format eTF = td.m_eTF;
    const bool fmtConversionNeeded = eTFDst != m_eTFDst || eTF != eTFDst;

#if !(defined(WIN32) || defined(WIN64)) || defined(OPENGL) || defined(NULL_RENDERER)
    if (fmtConversionNeeded)
    {
        td.m_pData[0] = td.m_pData[1] = td.m_pData[2] = td.m_pData[3] = td.m_pData[4] = td.m_pData[5] = 0;
        m_nWidth = m_nHeight = m_nDepth = m_nMips = 0;

#   if !defined(_RELEASE)
        TextureError(pTexFileName, "Trying an image format conversion from %s to %s. This is not supported on this platform!", NameForTextureFormat(eTF), NameForTextureFormat(eTFDst));
#   endif
        return false;
    }
#else
    const bool doProcessing = fmtConversionNeeded && (m_nFlags & FT_TEX_FONT) == 0; // we generate the font in native format
    if (doProcessing)
    {
        m_eTFSrc = eTFDst;
        m_eTFDst = eTFDst;

        const int nSrcWidth = td.m_nWidth;
        const int nSrcHeight = td.m_nHeight;

        for (int i = 0; i < 6; i++)
        {
            const byte* pTexData = td.m_pData[i];
            if (pTexData)
            {
                int nOutSize = 0;
                const byte* pNewData = Convert(pTexData, nSrcWidth, nSrcHeight, td.m_nMips, eTF, eTFDst, nOutSize, true);
                if (pNewData)
                {
                    td.AssignData(i, pNewData);
                }
            }
        }
    }
#endif

#if defined(TEXTURE_GET_SYSTEM_COPY_SUPPORT) && !defined(NULL_RENDERER)
    if (m_nFlags & FT_KEEP_LOWRES_SYSCOPY)
    {
        PrepareLowResSystemCopy(td.m_pData[0], true);
    }
#endif

    return true;
}

int CTexture::CalcNumMips(int nWidth, int nHeight)
{
    int nMips = 0;
    while (nWidth || nHeight)
    {
        if (!nWidth)
        {
            nWidth = 1;
        }
        if (!nHeight)
        {
            nHeight = 1;
        }
        nWidth >>= 1;
        nHeight >>= 1;
        nMips++;
    }
    //For DX11 hardware, the number of mips must be between 1 and 7, inclusive
    //0 is a valid result but means that D3D11 will generate a full series of mipmaps
    if (nMips > 7)
    {
        return 7;
    }

    return nMips;
}

uint32 CTexture::TextureDataSize(uint32 nWidth, uint32 nHeight, uint32 nDepth, uint32 nMips, uint32 nSlices, const ETEX_Format eTF, ETEX_TileMode eTM)
{
    if (eTF == eTF_Unknown)
    {
        return 0;
    }

    if (eTM != eTM_None)
    {
#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION TEXTURE_CPP_SECTION_2
    #include AZ_RESTRICTED_FILE(Texture_cpp)
#endif

#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION TEXTURE_CPP_SECTION_3
    #include AZ_RESTRICTED_FILE(Texture_cpp)
#endif

        __debugbreak();
        return 0;
    }
    else
    {
        const Vec2i BlockDim = GetBlockDim(eTF);
        const int nBytesPerBlock = CImageExtensionHelper::BytesPerBlock(eTF);
        uint32 nSize = 0;

        while ((nWidth || nHeight || nDepth) && nMips)
        {
            nWidth = max(1U, nWidth);
            nHeight = max(1U, nHeight);
            nDepth = max(1U, nDepth);

            nSize += ((nWidth + BlockDim.x - 1) / BlockDim.x) * ((nHeight + BlockDim.y - 1) / BlockDim.y) * nDepth * nBytesPerBlock;

            nWidth  >>= 1;
            nHeight >>= 1;
            nDepth  >>= 1;
            --nMips;
        }

        return nSize * nSlices;
    }
}

bool CTexture::IsInPlaceFormat(const ETEX_Format fmt)
{
    switch (fmt)
    {
    case eTF_R8G8B8A8S:
    case eTF_R8G8B8A8:

    case eTF_A8:
    case eTF_R8:
    case eTF_R8S:
    case eTF_R16:
    case eTF_R16U:
    case eTF_R16G16U:
    case eTF_R10G10B10A2UI:
    case eTF_R16F:
    case eTF_R32F:
    case eTF_R8G8:
    case eTF_R8G8S:
    case eTF_R16G16:
    case eTF_R16G16S:
    case eTF_R16G16F:
    case eTF_R11G11B10F:
    case eTF_R10G10B10A2:
    case eTF_R16G16B16A16:
    case eTF_R16G16B16A16S:
    case eTF_R16G16B16A16F:
    case eTF_R32G32B32A32F:

    case eTF_CTX1:
    case eTF_BC1:
    case eTF_BC2:
    case eTF_BC3:
    case eTF_BC4U:
    case eTF_BC4S:
    case eTF_BC5U:
    case eTF_BC5S:
#if defined(CRY_DDS_DX10_SUPPORT)
    case eTF_BC6UH:
    case eTF_BC6SH:
    case eTF_BC7:
    case eTF_R9G9B9E5:
#endif
    case eTF_EAC_R11:
    case eTF_EAC_RG11:
    case eTF_ETC2:
    case eTF_ETC2A:

    case eTF_B8G8R8A8:
    case eTF_B8G8R8X8:
#ifdef CRY_USE_METAL
    case eTF_PVRTC2:
    case eTF_PVRTC4:
#endif
#if defined(ANDROID) || defined(CRY_USE_METAL)
    case eTF_ASTC_4x4:
    case eTF_ASTC_5x4:
    case eTF_ASTC_5x5:
    case eTF_ASTC_6x5:
    case eTF_ASTC_6x6:
    case eTF_ASTC_8x5:
    case eTF_ASTC_8x6:
    case eTF_ASTC_8x8:
    case eTF_ASTC_10x5:
    case eTF_ASTC_10x6:
    case eTF_ASTC_10x8:
    case eTF_ASTC_10x10:
    case eTF_ASTC_12x10:
    case eTF_ASTC_12x12:
#endif
        return true;
    default:
        return false;
    }
}

void CTexture::ExpandMipFromFile(byte* dest, [[maybe_unused]] const int destSize, const byte* src, const int srcSize, const ETEX_Format fmt)
{
    if (IsInPlaceFormat(fmt))
    {
        assert(destSize == srcSize);
        if (dest != src)
        {
            cryMemcpy(dest, src, srcSize);
        }

        return;
    }

    // upload mip from file with conversions depending on format and platform specifics
    switch (fmt)
    {
    case eTF_B8G8R8:
        for (int i = srcSize / 3 - 1; i >= 0; --i)
        {
            dest[i * 4 + 0] = src[i * 3 + 2];
            dest[i * 4 + 1] = src[i * 3 + 1];
            dest[i * 4 + 2] = src[i * 3 + 0];
            dest[i * 4 + 3] = 255;
        }
        break;
    case eTF_L8V8U8X8:
        assert(destSize == srcSize);
        if (dest != src)
        {
            cryMemcpy(dest, src, srcSize);
        }
        for (int i = srcSize / 4 - 1; i >= 0; --i)
        {
            dest[i * 4 + 0] = src[i * 3 + 0];
            dest[i * 4 + 1] = src[i * 3 + 1];
            dest[i * 4 + 2] = src[i * 3 + 2];
            dest[i * 4 + 3] = src[i * 3 + 3];
        }
        break;
    case eTF_L8V8U8:
        for (int i = srcSize / 3 - 1; i >= 0; --i)
        {
            dest[i * 4 + 0] = src[i * 3 + 0];
            dest[i * 4 + 1] = src[i * 3 + 1];
            dest[i * 4 + 2] = src[i * 3 + 2];
            dest[i * 4 + 3] = 255;
        }
        break;
    case eTF_L8:
        for (int i = srcSize - 1; i >= 0; --i)
        {
            const byte bSrc = src[i];
            dest[i * 4 + 0] = bSrc;
            dest[i * 4 + 1] = bSrc;
            dest[i * 4 + 2] = bSrc;
            dest[i * 4 + 3] = 255;
        }
        break;
    case eTF_A8L8:
        for (int i = srcSize - 1; i >= 0; i -= 2)
        {
            const byte bSrcL = src[i - 1];
            const byte bSrcA = src[i - 0];
            dest[i * 4 + 0] = bSrcL;
            dest[i * 4 + 1] = bSrcL;
            dest[i * 4 + 2] = bSrcL;
            dest[i * 4 + 3] = bSrcA;
        }
        break;
    case eTF_B5G5R5:
    case eTF_B5G6R5:
    case eTF_B4G4R4A4:
    default:
        assert(0);
    }
}

bool CTexture::Invalidate(int nNewWidth, int nNewHeight, ETEX_Format eTF)
{
    bool bRelease = false;
    if (nNewWidth > 0 && nNewWidth != m_nWidth)
    {
        m_nWidth = nNewWidth;
        bRelease = true;
    }
    if (nNewHeight > 0 && nNewHeight != m_nHeight)
    {
        m_nHeight = nNewHeight;
        bRelease = true;
    }
    if (eTF != eTF_Unknown && eTF != m_eTFDst)
    {
        m_eTFDst = eTF;
        bRelease = true;
    }

    if (!m_pDevTexture)
    {
        return false;
    }

    if (bRelease)
    {
        if (m_nFlags & FT_FORCE_MIPS)
        {
            m_nMips = 1;
        }

        ReleaseDeviceTexture(true);
    }

    return bRelease;
}

SResourceView& CTexture::GetResourceView(const SResourceView& rvDesc)
{
    assert(m_pRenderTargetData);

    int nIndex = m_pRenderTargetData->m_ResourceViews.Find(rvDesc);

    if (nIndex < 0)
    {
        SResourceView* pRvDesc = m_pRenderTargetData->m_ResourceViews.AddIndex(1);
        pRvDesc->m_Desc = rvDesc.m_Desc;
        pRvDesc->m_pDeviceResourceView = CreateDeviceResourceView(rvDesc);

        nIndex = m_pRenderTargetData->m_ResourceViews.size() - 1;
    }

    return m_pRenderTargetData->m_ResourceViews[nIndex];
}

D3DShaderResourceView* CTexture::GetShaderResourceView(SResourceView::KeyType resourceViewID /*= SResourceView::DefaultView*/, bool bLegacySrgbLookup /*= false*/)
{
    if ((int64)resourceViewID <= (int64)SResourceView::DefaultView)
    {
        void* pResult = m_pDeviceShaderResource;

        if (resourceViewID == SResourceView::DefaultViewMS && m_pRenderTargetData && m_pRenderTargetData->m_pDeviceTextureMSAA)
        {
            SResourceView& pMultisampledRV = GetResourceView(SResourceView::ShaderResourceView(m_eTFDst, 0, -1, 0, -1, false, true));
            pResult = pMultisampledRV.m_pDeviceResourceView;
        }

        // NOTE: "m_pDeviceShaderResourceSRGB != nullptr" implies FT_USAGE_ALLOWREADSRGB
        if ((resourceViewID == SResourceView::DefaultViewSRGB || bLegacySrgbLookup) && m_pDeviceShaderResourceSRGB)
        {
            pResult = m_pDeviceShaderResourceSRGB;
        }

        return (D3DShaderResourceView*)pResult;
    }
    else
    {
        return (D3DShaderResourceView*)GetResourceView(resourceViewID).m_pDeviceResourceView;
    }
}

void CTexture::SetShaderResourceView(D3DShaderResourceView* pDeviceShaderResource, bool bMultisampled)
{
    if (bMultisampled && m_pRenderTargetData && m_pRenderTargetData->m_pDeviceTextureMSAA)
    {
        SResourceView& pMultisampledRV = GetResourceView(SResourceView::ShaderResourceView(m_eTFDst, 0, -1, 0, -1, false, true));

        if (pMultisampledRV.m_pDeviceResourceView != pDeviceShaderResource)
        {
            pMultisampledRV.m_pDeviceResourceView = pDeviceShaderResource;
            InvalidateDeviceResource(eDeviceResourceViewDirty);
        }
    }
    else
    {
        if (m_pDeviceShaderResource != pDeviceShaderResource)
        {
            m_pDeviceShaderResource = pDeviceShaderResource;
            InvalidateDeviceResource(eDeviceResourceViewDirty);
        }
    }
}

D3DUnorderedAccessView* CTexture::GetDeviceUAV()
{
    const SResourceView& rvDesc = m_pRenderTargetData ? GetResourceView(SResourceView::UnorderedAccessView(m_eTFDst, 0, -1, 0, m_nFlags)) : NULL;
    return (D3DUnorderedAccessView*)rvDesc.m_pDeviceResourceView;
}

D3DDepthSurface* CTexture::GetDeviceDepthStencilSurf(int nFirstSlice, int nSliceCount)
{
    const SResourceView& rvDesc = GetResourceView(SResourceView::DepthStencilView(m_eTFDst, nFirstSlice, nSliceCount));
    return (D3DDepthSurface*)rvDesc.m_pDeviceResourceView;
}

byte* CTexture::GetData32([[maybe_unused]] int nSide, [[maybe_unused]] int nLevel, [[maybe_unused]] byte* pDst, [[maybe_unused]] ETEX_Format eDstFormat)
{
#if (defined(WIN32) || defined(WIN64)) && !defined(NULL_RENDERER)
    CDeviceTexture* pDevTexture = GetDevTexture();
    pDevTexture->DownloadToStagingResource(D3D11CalcSubresource(nLevel, nSide, m_nMips), [&](void* pData, [[maybe_unused]] uint32 rowPitch, [[maybe_unused]] uint32 slicePitch)
        {
            if (m_eTFDst != eTF_R8G8B8A8)
            {
                int nOutSize = 0;

                if (m_eTFSrc == eDstFormat && pDst)
                {
                    memcpy(pDst, pData, GetDeviceDataSize());
                }
                else
                {
                    pDst = Convert((byte*)pData, m_nWidth, m_nHeight, 1, m_eTFSrc, eDstFormat, nOutSize, true);
                }
            }
            else
            {
                if (!pDst)
                {
                    pDst = new byte[m_nWidth * m_nHeight * 4];
                }

                memcpy(pDst, pData, m_nWidth * m_nHeight * 4);
            }

            return true;
        });

    return pDst;
#else
    return 0;
#endif
}

const int CTexture::GetSize(bool bIncludePool) const
{
    int nSize = sizeof(CTexture);
    nSize += m_SrcName.capacity();
    if (m_pRenderTargetData)
    {
        nSize +=  sizeof(*m_pRenderTargetData);
    }
    if (m_pFileTexMips)
    {
        nSize += m_pFileTexMips->GetSize(m_nMips, m_CacheFileHeader.m_nSides);
        if (bIncludePool && m_pFileTexMips->m_pPoolItem)
        {
            nSize += m_pFileTexMips->m_pPoolItem->GetSize();
        }
    }

    return nSize;
}

void CTexture::Init()
{
    SDynTexture::Init();
    InitStreaming();
    CTexture::s_TexStates.reserve(300); // this likes to expand, so it'd be nice if it didn't; 300 => ~6Kb, there were 171 after one level

    SDynTexture2::Init(eTP_Clouds);
}

void CTexture::PostInit()
{
    LOADING_TIME_PROFILE_SECTION;
    if (!gRenDev->IsShaderCacheGenMode())
    {
        CTexture::LoadDefaultSystemTextures();
    }
}

int __cdecl TexCallback(const VOID * arg1, const VOID * arg2)
{
    CTexture** pi1 = (CTexture**)arg1;
    CTexture** pi2 = (CTexture**)arg2;
    CTexture* ti1 = *pi1;
    CTexture* ti2 = *pi2;

    if (ti1->GetDeviceDataSize() > ti2->GetDeviceDataSize())
    {
        return -1;
    }
    if (ti1->GetDeviceDataSize() < ti2->GetDeviceDataSize())
    {
        return 1;
    }
    return azstricmp(ti1->GetSourceName(), ti2->GetSourceName());
}

int __cdecl TexCallbackMips(const VOID * arg1, const VOID * arg2)
{
    CTexture** pi1 = (CTexture**)arg1;
    CTexture** pi2 = (CTexture**)arg2;
    CTexture* ti1 = *pi1;
    CTexture* ti2 = *pi2;

    int nSize1, nSize2;

    nSize1 = ti1->GetActualSize();
    nSize2 = ti2->GetActualSize();

    if (nSize1 > nSize2)
    {
        return -1;
    }
    if (nSize1 < nSize2)
    {
        return 1;
    }
    return azstricmp(ti1->GetSourceName(), ti2->GetSourceName());
}

void CTexture::Update()
{
    FUNCTION_PROFILER_RENDERER;

    CRenderer* rd = gRenDev;
    char buf[256] = "";

    // reload pending texture reload requests
    {
        AZStd::set<string, AZStd::less<string>, AZ::StdLegacyAllocator> queue;

        s_xTexReloadLock.Lock();
        s_vTexReloadRequests.swap(queue);
        s_xTexReloadLock.Unlock();

        for (auto i = queue.begin(); i != queue.end(); i++)
        {
            ReloadFile(*i);
        }
    }

    CTexture::s_bStreamingFromHDD = gEnv->pSystem->GetStreamEngine()->IsStreamDataOnHDD();
    CTexture::s_nStatsStreamPoolInUseMem = CTexture::s_pPoolMgr->GetInUseSize();

    s_pTextureStreamer->ApplySchedule(ITextureStreamer::eASF_Full);
    s_pTextureStreamer->BeginUpdateSchedule();

#ifdef ENABLE_TEXTURE_STREAM_LISTENER
    StreamUpdateStats();
#endif

    SDynTexture::Tick();

    SResourceContainer* pRL = CBaseResource::GetResourcesForClass(CTexture::mfGetClassName());
    ResourcesMapItor itor;

    if ((s_nStreamingMode != CRenderer::CV_r_texturesstreaming) || (s_nStreamingUpdateMode != CRenderer::CV_r_texturesstreamingUpdateType))
    {
        InitStreaming();
    }

    if (pRL)
    {
#ifndef CONSOLE_CONST_CVAR_MODE
        uint32 i;
        if (CRenderer::CV_r_texlog == 2 || CRenderer::CV_r_texlog == 3 || CRenderer::CV_r_texlog == 4)
        {
            AZ::IO::HandleType fileHandle = AZ::IO::InvalidHandle;
            TArray<CTexture*> Texs;
            int Size = 0;
            int PartSize = 0;
            if (CRenderer::CV_r_texlog == 2 || CRenderer::CV_r_texlog == 3)
            {
                for (itor = pRL->m_RMap.begin(); itor != pRL->m_RMap.end(); itor++)
                {
                    CTexture* tp = (CTexture*)itor->second;
                    if (CRenderer::CV_r_texlog == 3 && tp->IsNoTexture())
                    {
                        Texs.AddElem(tp);
                    }
                    else
                    if (CRenderer::CV_r_texlog == 2 && !tp->IsNoTexture() && tp->m_pFileTexMips) // (tp->GetFlags() & FT_FROMIMAGE))
                    {
                        Texs.AddElem(tp);
                    }
                }
                if (CRenderer::CV_r_texlog == 3)
                {
                    CryLogAlways("Logging to MissingTextures.txt...");
                    fileHandle = fxopen("MissingTextures.txt", "w");
                }
                else
                {
                    CryLogAlways("Logging to UsedTextures.txt...");
                    fileHandle = fxopen("UsedTextures.txt", "w");
                }
                AZ::IO::Print(fileHandle, "*** All textures: ***\n");

                if (Texs.Num())
                {
                    qsort(&Texs[0], Texs.Num(), sizeof(CTexture*), TexCallbackMips);
                }

                for (i = 0; i < Texs.Num(); i++)
                {
                    int w = Texs[i]->GetWidth();
                    int h = Texs[i]->GetHeight();

                    int nTSize = Texs[i]->m_pFileTexMips->GetSize(Texs[i]->GetNumMips(), Texs[i]->GetNumSides());

                    AZ::IO::Print(fileHandle, "%d\t\t%d x %d\t\tType: %s\t\tMips: %d\t\tFormat: %s\t\t(%s)\n", nTSize, w, h, Texs[i]->NameForTextureType(Texs[i]->GetTextureType()), Texs[i]->GetNumMips(), Texs[i]->NameForTextureFormat(Texs[i]->GetDstFormat()), Texs[i]->GetName());
                    //Size += Texs[i]->GetDataSize();
                    Size += nTSize;

                    PartSize += Texs[i]->GetDeviceDataSize();
                }
                AZ::IO::Print(fileHandle, "*** Total Size: %d\n\n", Size /*, PartSize, PartSize */);

                Texs.Free();
            }
            for (itor = pRL->m_RMap.begin(); itor != pRL->m_RMap.end(); itor++)
            {
                CTexture* tp = (CTexture*)itor->second;
                if (tp->m_nAccessFrameID == rd->m_RP.m_TI[rd->m_RP.m_nProcessThreadID].m_nFrameUpdateID)
                {
                    Texs.AddElem(tp);
                }
            }

            if (fileHandle != AZ::IO::InvalidHandle)
            {
                gEnv->pFileIO->Close(fileHandle);
                fileHandle = AZ::IO::InvalidHandle;
            }

            fileHandle = fxopen("UsedTextures_Frame.txt", "w");

            if (fileHandle != AZ::IO::InvalidHandle)
            {
                AZ::IO::Print(fileHandle, "\n\n*** Textures used in current frame: ***\n");
            }
            else
            {
                rd->TextToScreenColor(4, 13, 1, 1, 0, 1, "*** Textures used in current frame: ***");
            }
            int nY = 17;

            if (Texs.Num())
            {
                qsort(&Texs[0], Texs.Num(), sizeof(CTexture*), TexCallback);
            }

            Size = 0;
            for (i = 0; i < Texs.Num(); i++)
            {
                if (fileHandle != AZ::IO::InvalidHandle)
                {
                    AZ::IO::Print(fileHandle, "%.3fKb\t\tType: %s\t\tFormat: %s\t\t(%s)\n", Texs[i]->GetDeviceDataSize() / 1024.0f, CTexture::NameForTextureType(Texs[i]->GetTextureType()), CTexture::NameForTextureFormat(Texs[i]->GetDstFormat()), Texs[i]->GetName());
                }
                else
                {
                    sprintf_s(buf, "%.3fKb  Type: %s  Format: %s  (%s)", Texs[i]->GetDeviceDataSize() / 1024.0f, CTexture::NameForTextureType(Texs[i]->GetTextureType()), CTexture::NameForTextureFormat(Texs[i]->GetDstFormat()), Texs[i]->GetName());
                    rd->TextToScreenColor(4, nY, 0, 1, 0, 1, buf);
                    nY += 3;
                }
                PartSize += Texs[i]->GetDeviceDataSize();
                Size += Texs[i]->GetDataSize();
            }
            if (fileHandle != AZ::IO::InvalidHandle)
            {
                AZ::IO::Print(fileHandle, "*** Total Size: %.3fMb, Device Size: %.3fMb\n\n", Size / (1024.0f * 1024.0f), PartSize / (1024.0f * 1024.0f));
            }
            else
            {
                sprintf_s(buf, "*** Total Size: %.3fMb, Device Size: %.3fMb", Size / (1024.0f * 1024.0f), PartSize / (1024.0f * 1024.0f));
                rd->TextToScreenColor(4, nY + 1, 0, 1, 1, 1, buf);
            }

            Texs.Free();
            for (itor = pRL->m_RMap.begin(); itor != pRL->m_RMap.end(); itor++)
            {
                CTexture* tp = (CTexture*)itor->second;
                if (tp && !tp->IsNoTexture())
                {
                    Texs.AddElem(tp);
                }
            }

            if (fileHandle != AZ::IO::InvalidHandle)
            {
                gEnv->pFileIO->Close(fileHandle);
                fileHandle = AZ::IO::InvalidHandle;
            }
            fileHandle = fxopen("UsedTextures_All.txt", "w");

            if (fileHandle != AZ::IO::InvalidHandle)
            {
                AZ::IO::Print(fileHandle, "\n\n*** All Existing Textures: ***\n");
            }
            else
            {
                rd->TextToScreenColor(4, 13, 1, 1, 0, 1, "*** Textures loaded: ***");
            }

            if (Texs.Num())
            {
                qsort(&Texs[0], Texs.Num(), sizeof(CTexture*), TexCallback);
            }

            Size = 0;
            for (i = 0; i < Texs.Num(); i++)
            {
                if (fileHandle != AZ::IO::InvalidHandle)
                {
                    int w = Texs[i]->GetWidth();
                    int h = Texs[i]->GetHeight();
                    AZ::IO::Print(fileHandle, "%d\t\t%d x %d\t\t%d mips (%.3fKb)\t\tType: %s \t\tFormat: %s\t\t(%s)\n", Texs[i]->GetDataSize(), w, h, Texs[i]->GetNumMips(), Texs[i]->GetDeviceDataSize() / 1024.0f, CTexture::NameForTextureType(Texs[i]->GetTextureType()), CTexture::NameForTextureFormat(Texs[i]->GetDstFormat()), Texs[i]->GetName());
                }
                else
                {
                    sprintf_s(buf, "%.3fKb  Type: %s  Format: %s  (%s)", Texs[i]->GetDataSize() / 1024.0f, CTexture::NameForTextureType(Texs[i]->GetTextureType()), CTexture::NameForTextureFormat(Texs[i]->GetDstFormat()), Texs[i]->GetName());
                    rd->TextToScreenColor(4, nY, 0, 1, 0, 1, buf);
                    nY += 3;
                }
                Size += Texs[i]->GetDeviceDataSize();
            }
            if (fileHandle != AZ::IO::InvalidHandle)
            {
                AZ::IO::Print(fileHandle, "*** Total Size: %.3fMb\n\n", Size / (1024.0f * 1024.0f));
            }
            else
            {
                sprintf_s(buf, "*** Total Size: %.3fMb", Size / (1024.0f * 1024.0f));
                rd->TextToScreenColor(4, nY + 1, 0, 1, 1, 1, buf);
            }


            Texs.Free();
            for (itor = pRL->m_RMap.begin(); itor != pRL->m_RMap.end(); itor++)
            {
                CTexture* tp = (CTexture*)itor->second;
                if (tp && !tp->IsNoTexture() && !tp->IsStreamed())
                {
                    Texs.AddElem(tp);
                }
            }

            if (fileHandle != AZ::IO::InvalidHandle)
            {
                gEnv->pFileIO->Close(fileHandle);
                fileHandle = AZ::IO::InvalidHandle;
            }

            if (CRenderer::CV_r_texlog != 4)
            {
                CRenderer::CV_r_texlog = 0;
            }
        }
        else
        if (CRenderer::CV_r_texlog == 1)
        {
            //char *str = GetTexturesStatusText();

            TArray<CTexture*> Texs;
            //TArray<CTexture *> TexsNM;
            for (itor = pRL->m_RMap.begin(); itor != pRL->m_RMap.end(); itor++)
            {
                CTexture* tp = (CTexture*)itor->second;
                if (tp && !tp->IsNoTexture())
                {
                    Texs.AddElem(tp);
                    //if (tp->GetFlags() & FT_TEX_NORMAL_MAP)
                    //  TexsNM.AddElem(tp);
                }
            }

            if (Texs.Num())
            {
                qsort(&Texs[0], Texs.Num(), sizeof(CTexture*), TexCallback);
            }

            int64 AllSize = 0;
            int64 Size = 0;
            int64 PartSize = 0;
            int64 NonStrSize = 0;
            int nNoStr = 0;
            int64 SizeNM = 0;
            int64 SizeDynCom = 0;
            int64 SizeDynAtl = 0;
            int64 PartSizeNM = 0;
            int nNumTex = 0;
            int nNumTexNM = 0;
            int nNumTexDynAtl = 0;
            int nNumTexDynCom = 0;
            for (i = 0; i < Texs.Num(); i++)
            {
                CTexture* tex = Texs[i];
                const uint32 texFlags = tex->GetFlags();
                const int texDataSize = tex->GetDataSize();
                const int texDeviceDataSize = tex->GetDeviceDataSize();

                if (tex->GetDevTexture() && !(texFlags & (FT_USAGE_DYNAMIC | FT_USAGE_RENDERTARGET)))
                {
                    AllSize += texDataSize;
                    if (!Texs[i]->IsStreamed())
                    {
                        NonStrSize += texDataSize;
                        nNoStr++;
                    }
                }

                if (texFlags & (FT_USAGE_RENDERTARGET | FT_USAGE_DYNAMIC))
                {
                    if (texFlags & FT_USAGE_ATLAS)
                    {
                        ++nNumTexDynAtl;
                        SizeDynAtl += texDataSize;
                    }
                    else
                    {
                        ++nNumTexDynCom;
                        SizeDynCom += texDataSize;
                    }
                }
                else
                if (0 == (texFlags & FT_TEX_NORMAL_MAP))
                {
                    if (!Texs[i]->IsUnloaded())
                    {
                        ++nNumTex;
                        Size += texDataSize;
                        PartSize += texDeviceDataSize;
                    }
                }
                else
                {
                    if (!Texs[i]->IsUnloaded())
                    {
                        ++nNumTexNM;
                        SizeNM += texDataSize;
                        PartSizeNM += texDeviceDataSize;
                    }
                }
            }

            sprintf_s(buf, "All texture objects: %d (Size: %.3fMb), NonStreamed: %d (Size: %.3fMb)", Texs.Num(), AllSize / (1024.0 * 1024.0), nNoStr, NonStrSize / (1024.0 * 1024.0));
            rd->TextToScreenColor(4, 13, 1, 1, 0, 1, buf);
            sprintf_s(buf, "All Loaded Texture Maps: %d (All MIPS: %.3fMb, Loaded MIPS: %.3fMb)", nNumTex, Size / (1024.0f * 1024.0f), PartSize / (1024.0 * 1024.0));
            rd->TextToScreenColor(4, 16, 1, 1, 0, 1, buf);
            sprintf_s(buf, "All Loaded Normal Maps: %d (All MIPS: %.3fMb, Loaded MIPS: %.3fMb)", nNumTexNM, SizeNM / (1024.0 * 1024.0), PartSizeNM / (1024.0 * 1024.0));
            rd->TextToScreenColor(4, 19, 1, 1, 0, 1, buf);
            sprintf_s(buf, "All Dynamic textures: %d (%.3fMb), %d Atlases (%.3fMb), %d Separared (%.3fMb)", nNumTexDynAtl + nNumTexDynCom, (SizeDynAtl + SizeDynCom) / (1024.0 * 1024.0), nNumTexDynAtl, SizeDynAtl / (1024.0 * 1024.0), nNumTexDynCom, SizeDynCom / (1024.0 * 1024.0));
            rd->TextToScreenColor(4, 22, 1, 1, 0, 1, buf);

            Texs.Free();
            for (itor = pRL->m_RMap.begin(); itor != pRL->m_RMap.end(); itor++)
            {
                CTexture* tp = (CTexture*)itor->second;
                if (tp && !tp->IsNoTexture() && tp->m_nAccessFrameID == rd->m_RP.m_TI[rd->m_RP.m_nProcessThreadID].m_nFrameUpdateID)
                {
                    Texs.AddElem(tp);
                }
            }

            if (Texs.Num())
            {
                qsort(&Texs[0], Texs.Num(), sizeof(CTexture*), TexCallback);
            }

            Size = 0;
            SizeDynAtl = 0;
            SizeDynCom = 0;
            PartSize = 0;
            NonStrSize = 0;
            for (i = 0; i < Texs.Num(); i++)
            {
                Size += Texs[i]->GetDataSize();
                if (Texs[i]->GetFlags() & (FT_USAGE_DYNAMIC | FT_USAGE_RENDERTARGET))
                {
                    if (Texs[i]->GetFlags() & FT_USAGE_ATLAS)
                    {
                        SizeDynAtl += Texs[i]->GetDataSize();
                    }
                    else
                    {
                        SizeDynCom += Texs[i]->GetDataSize();
                    }
                }
                else
                {
                    PartSize += Texs[i]->GetDeviceDataSize();
                }
                if (!Texs[i]->IsStreamed())
                {
                    NonStrSize += Texs[i]->GetDataSize();
                }
            }
            sprintf_s(buf, "Current tex. objects: %d (Size: %.3fMb, Dyn. Atlases: %.3f, Dyn. Separated: %.3f, Loaded: %.3f, NonStreamed: %.3f)", Texs.Num(), Size / (1024.0f * 1024.0f), SizeDynAtl / (1024.0f * 1024.0f), SizeDynCom / (1024.0f * 1024.0f), PartSize / (1024.0f * 1024.0f), NonStrSize / (1024.0f * 1024.0f));
            rd->TextToScreenColor(4, 27, 1, 0, 0, 1, buf);
        }
#endif
    }
}

void CTexture::RT_LoadingUpdate()
{
    CTexture::s_bStreamingFromHDD = gEnv->pSystem->GetStreamEngine()->IsStreamDataOnHDD();
    CTexture::s_nStatsStreamPoolInUseMem = CTexture::s_pPoolMgr->GetInUseSize();

    ITextureStreamer::EApplyScheduleFlags asf = CTexture::s_bPrecachePhase
        ? ITextureStreamer::eASF_InOut // Exclude the prep update, as it will be done by the RLT (and can be expensive)
        : ITextureStreamer::eASF_Full;

    s_pTextureStreamer->ApplySchedule(asf);
}

void CTexture::RLT_LoadingUpdate()
{
    AZ_TRACE_METHOD();
    s_pTextureStreamer->BeginUpdateSchedule();
}

Ang3 sDeltAngles(Ang3& Ang0, Ang3& Ang1)
{
    Ang3 out;
    for (int i = 0; i < 3; i++)
    {
        float a0 = Ang0[i];
        a0 = (float)((360.0 / 65536) * ((int)(a0 * (65536 / 360.0)) & 65535)); // angmod
        float a1 = Ang1[i];
        a1 = (float)((360.0 / 65536) * ((int)(a0 * (65536 / 360.0)) & 65535));
        out[i] = a0 - a1;
    }
    return out;
}

SEnvTexture* CTexture::FindSuitableEnvTex(Vec3& Pos, Ang3& Angs, bool bMustExist, [[maybe_unused]] int RendFlags, [[maybe_unused]] bool bUseExistingREs, [[maybe_unused]] CShader* pSH, [[maybe_unused]] CShaderResources* pRes, CRenderObject* pObj, bool bReflect, IRenderElement* pRE, bool* bMustUpdate)
{
    float time0 = iTimer->GetAsyncCurTime();

    int i;
    float distO = 999999;
    float adist = 999999;
    int firstForUse = -1;
    int firstFree = -1;
    Vec3 objPos;
    if (bMustUpdate)
    {
        *bMustUpdate = false;
    }
    if (!pObj)
    {
        bReflect = false;
    }
    else
    {
        if (bReflect)
        {
            Plane pl;
            pRE->mfGetPlane(pl);
            objPos = pl.MirrorPosition(Vec3(0, 0, 0));
        }
        else
        if (pRE)
        {
            pRE->mfCenter(objPos, pObj);
        }
        else
        {
            objPos = pObj->GetTranslation();
        }
    }
    float dist = 999999;
    for (i = 0; i < MAX_ENVTEXTURES; i++)
    {
        SEnvTexture* cur = &CTexture::s_EnvTexts[i];
        if (cur->m_bReflected != bReflect)
        {
            continue;
        }
        float s = (cur->m_CamPos - Pos).GetLengthSquared();
        Ang3 angDelta = sDeltAngles(Angs, cur->m_Angle);
        float a = angDelta.x * angDelta.x + angDelta.y * angDelta.y + angDelta.z * angDelta.z;
        float so = 0;
        if (bReflect)
        {
            so = (cur->m_ObjPos - objPos).GetLengthSquared();
        }
        if (s <= dist && a <= adist && so <= distO)
        {
            dist = s;
            adist = a;
            distO = so;
            firstForUse = i;
            if (!so && !s && !a)
            {
                break;
            }
        }
        if (cur->m_pTex && !cur->m_pTex->m_pTexture && firstFree < 0)
        {
            firstFree = i;
        }
    }
    if (bMustExist && firstForUse >= 0)
    {
        return &CTexture::s_EnvTexts[firstForUse];
    }
    if (bReflect)
    {
        dist = distO;
    }

    float curTime = iTimer->GetCurrTime();
    int nUpdate = -2;
    float fTimeInterval = dist * CRenderer::CV_r_envtexupdateinterval + CRenderer::CV_r_envtexupdateinterval * 0.5f;
    float fDelta = curTime - CTexture::s_EnvTexts[firstForUse].m_TimeLastUpdated;
    if (bMustExist)
    {
        nUpdate = -2;
    }
    else
    if (dist > MAX_ENVTEXSCANDIST)
    {
        if (firstFree >= 0)
        {
            nUpdate = firstFree;
        }
        else
        {
            nUpdate = -1;
        }
    }
    else
    if (fDelta > fTimeInterval)
    {
        nUpdate = firstForUse;
    }
    if (nUpdate == -2)
    {
        // No need to update (Up to date)
        return &CTexture::s_EnvTexts[firstForUse];
    }
    if (!CTexture::s_EnvTexts[nUpdate].m_pTex)
    {
        return NULL;
    }
    if (nUpdate >= 0)
    {
        if (!CTexture::s_EnvTexts[nUpdate].m_pTex->m_pTexture || gRenDev->m_RP.m_PS[gRenDev->m_RP.m_nProcessThreadID].m_fEnvTextUpdateTime < 0.1f)
        {
            int n = nUpdate;
            CTexture::s_EnvTexts[n].m_TimeLastUpdated = curTime;
            CTexture::s_EnvTexts[n].m_CamPos = Pos;
            CTexture::s_EnvTexts[n].m_Angle = Angs;
            CTexture::s_EnvTexts[n].m_ObjPos = objPos;
            CTexture::s_EnvTexts[n].m_bReflected = bReflect;
            if (bMustUpdate)
            {
                *bMustUpdate = true;
            }
        }
        gRenDev->m_RP.m_PS[gRenDev->m_RP.m_nProcessThreadID].m_fEnvTextUpdateTime += iTimer->GetAsyncCurTime() - time0;
        return &CTexture::s_EnvTexts[nUpdate];
    }

    dist = 0;
    firstForUse = -1;
    for (i = 0; i < MAX_ENVTEXTURES; i++)
    {
        SEnvTexture* cur = &CTexture::s_EnvTexts[i];
        if (dist < curTime - cur->m_TimeLastUpdated && !cur->m_bInprogress)
        {
            dist = curTime - cur->m_TimeLastUpdated;
            firstForUse = i;
        }
    }
    if (firstForUse < 0)
    {
        return NULL;
    }
    int n = firstForUse;
    CTexture::s_EnvTexts[n].m_TimeLastUpdated = curTime;
    CTexture::s_EnvTexts[n].m_CamPos = Pos;
    CTexture::s_EnvTexts[n].m_ObjPos = objPos;
    CTexture::s_EnvTexts[n].m_Angle = Angs;
    CTexture::s_EnvTexts[n].m_bReflected = bReflect;
    if (bMustUpdate)
    {
        *bMustUpdate = true;
    }

    gRenDev->m_RP.m_PS[gRenDev->m_RP.m_nProcessThreadID].m_fEnvTextUpdateTime += iTimer->GetAsyncCurTime() - time0;
    return &CTexture::s_EnvTexts[n];
}


//===========================================================================

void CTexture::ShutDown()
{
    uint32 i;

    if (gRenDev->GetRenderType() == eRT_Null) // workaround to fix crash when quitting the dedicated server - because the textures are shared
    {
        return;                                                                                                     // should be fixed soon
    }
    RT_FlushAllStreamingTasks(true);

    ReleaseSystemTextures();

    if IsCVarConstAccess(constexpr) (CRenderer::CV_r_releaseallresourcesonexit)
    {
        SResourceContainer* pRL = CBaseResource::GetResourcesForClass(CTexture::mfGetClassName());
        if (pRL)
        {
            int n = 0;
            ResourcesMapItor itor;
            for (itor = pRL->m_RMap.begin(); itor != pRL->m_RMap.end(); )
            {
                CTexture* pTX = (CTexture*)itor->second;
                itor++;
                if (!pTX)
                {
                    continue;
                }
                if IsCVarConstAccess(constexpr) (CRenderer::CV_r_printmemoryleaks)
                {
                    iLog->Log("Warning: CTexture::ShutDown: Texture %s was not deleted (%d)", pTX->GetName(), pTX->GetRefCounter());
                }
                SAFE_RELEASE_FORCE(pTX);
                n++;
            }
        }
    }

    if (s_ShaderTemplatesInitialized)
    {
        for (i = 0; i < EFTT_MAX; i++)
        {
            (*s_ShaderTemplates)[i].~CTexture();
        }
    }
    s_ShaderTemplates->Free();

    SAFE_DELETE(s_pTexNULL);

    s_pPoolMgr->Flush();
}

bool CTexture::ReloadFile_Request(const char* szFileName)
{
    s_xTexReloadLock.Lock();
    s_vTexReloadRequests.insert(szFileName);
    s_xTexReloadLock.Unlock();

    return true;
}

bool CTexture::ReloadFile(const char* szFileName)
{
    char realNameBuffer[256];
    fpConvertDOSToUnixName(realNameBuffer, szFileName);


    bool bStatus = false;

    SResourceContainer* pRL = CBaseResource::GetResourcesForClass(CTexture::mfGetClassName());
    if (pRL)
    {
        AUTO_LOCK(CBaseResource::s_cResLock);
        AZStd::string normalizedFile;
        AZStd::string fileExtension;
        AzFramework::StringFunc::Path::GetExtension(szFileName, fileExtension);
        if (szFileName[0] == '$' || fileExtension.empty())
        {
            //If the name starts with $ or it does not have any extension then it is one of the special texture that the engine requires and we would not be modifying the name
            normalizedFile = szFileName;
        }
        else
        {
            char buffer[AZ_MAX_PATH_LEN];
            IResourceCompilerHelper::GetOutputFilename(szFileName, buffer, sizeof(buffer));   // change texture filename extensions to dds
            normalizedFile = buffer;
            AZStd::to_lower(normalizedFile.begin(), normalizedFile.end());
            PathUtil::ToUnixPath(normalizedFile.c_str());
        }

        CCryNameTSCRC Name = GenName(normalizedFile.c_str());

        ResourcesMapItor itor = pRL->m_RMap.find(Name);
        if (itor != pRL->m_RMap.end())
        {
            CTexture* tp = (CTexture*)itor->second;

            if (tp->Reload())
            {
                bStatus = true;
            }
        }

        // Since we do not have the information whether the modified file was also reloaded with the FT_ALPHA flag we will try to reload that as well
        Name = GenName(normalizedFile.c_str(), FT_ALPHA);

        itor = pRL->m_RMap.find(Name);
        if (itor != pRL->m_RMap.end())
        {
            CTexture* tp = (CTexture*)itor->second;

            if (tp->Reload())
            {
                bStatus = true;
            }
        }
    }

    return bStatus;
}

void CTexture::ReloadTextures()
{
    SResourceContainer* pRL = CBaseResource::GetResourcesForClass(CTexture::mfGetClassName());
    if (pRL)
    {
        ResourcesMapItor itor;
        int nID = 0;
        for (itor = pRL->m_RMap.begin(); itor != pRL->m_RMap.end(); itor++, nID++)
        {
            CTexture* tp = (CTexture*)itor->second;
            if (!tp)
            {
                continue;
            }
            if (!(tp->m_nFlags & FT_FROMIMAGE))
            {
                continue;
            }
            tp->Reload();
        }
    }
}

bool CTexture::SetNoTexture( const CTexture* pDefaultTexture /* = "NoTexture" entry */)
{
    if (pDefaultTexture)
    {
        m_pDevTexture = pDefaultTexture->m_pDevTexture;
        m_pDeviceShaderResource = pDefaultTexture->m_pDeviceShaderResource;
        m_eTFSrc = pDefaultTexture->GetSrcFormat();
        m_eTFDst = pDefaultTexture->GetDstFormat();
        m_nMips = pDefaultTexture->GetNumMips();
        m_nWidth = pDefaultTexture->GetWidth();
        m_nHeight = pDefaultTexture->GetHeight();
        m_nDepth = 1;
        m_nDefState = pDefaultTexture->m_nDefState;
        m_fAvgBrightness = 1.0f;
        m_cMinColor = 0.0f;
        m_cMaxColor = 1.0f;
        m_cClearColor = ColorF(0.0f, 0.0f, 0.0f, 1.0f);

        m_bNoTexture = true;
        if (m_pFileTexMips)
        {
            Unlink();
            StreamState_ReleaseInfo(this, m_pFileTexMips);
            m_pFileTexMips = NULL;
        }
        m_bStreamed = false;
        m_bPostponed = false;
        m_nFlags |= FT_FAILED;
        m_nActualSize = 0;
        m_nPersistentSize = 0;
        return true;
    }
    return false;
}

//===========================================================================

void CTexture::ReleaseSystemTextures()
{
    if (s_pStatsTexWantedLists)
    {
        for (int i = 0; i < 2; ++i)
        {
            s_pStatsTexWantedLists[i].clear();
        }
    }

    SAFE_RELEASE_FORCE(s_ptexRT_2D);
    SAFE_RELEASE_FORCE(s_ptexCloudsLM);

    SAFE_RELEASE_FORCE(s_ptexVolumetricFog);
    SAFE_RELEASE_FORCE(s_ptexVolumetricFogDensityColor);
    SAFE_RELEASE_FORCE(s_ptexVolumetricFogDensity);
    SAFE_RELEASE_FORCE(s_ptexVolumetricClipVolumeStencil);

    uint32 i;
    for (i = 0; i < 8; i++)
    {
        SAFE_RELEASE_FORCE(s_ptexShadowID[i]);
    }

    SAFE_RELEASE_FORCE(s_ptexFromObj);
    SAFE_RELEASE_FORCE(s_ptexSvoTree);
    SAFE_RELEASE_FORCE(s_ptexSvoTris);
    SAFE_RELEASE_FORCE(s_ptexSvoGlobalCM);
    SAFE_RELEASE_FORCE(s_ptexSvoRgbs);
    SAFE_RELEASE_FORCE(s_ptexSvoNorm);
    SAFE_RELEASE_FORCE(s_ptexSvoOpac);
    SAFE_RELEASE_FORCE(s_ptexFromObjCM);

    SAFE_RELEASE_FORCE(s_ptexVolObj_Density);
    SAFE_RELEASE_FORCE(s_ptexVolObj_Shadow);

    SAFE_RELEASE_FORCE(s_ptexColorChart);

    for (i = 0; i < MAX_ENVCUBEMAPS; i++)
    {
        s_EnvCMaps[i].Release();
    }
    for (i = 0; i < MAX_ENVTEXTURES; i++)
    {
        s_EnvTexts[i].Release();
    }

    SAFE_RELEASE_FORCE(s_ptexMipColors_Diffuse);
    SAFE_RELEASE_FORCE(s_ptexMipColors_Bump);
    SAFE_RELEASE_FORCE(s_ptexSkyDomeMie);
    SAFE_RELEASE_FORCE(s_ptexSkyDomeRayleigh);
    SAFE_RELEASE_FORCE(s_ptexSkyDomeMoon);
    SAFE_RELEASE_FORCE(s_ptexRT_ShadowPool);
    SAFE_RELEASE_FORCE(s_ptexRT_ShadowStub);

    SAFE_RELEASE_FORCE(s_ptexSceneNormalsMapMS);
    SAFE_RELEASE_FORCE(s_ptexSceneDiffuseAccMapMS);
    SAFE_RELEASE_FORCE(s_ptexSceneSpecularAccMapMS);

    SAFE_RELEASE_FORCE(s_defaultEnvironmentProbeDummy);

    s_CustomRT_2D->Free();

    s_pPoolMgr->Flush();

    // release targets pools
    SDynTexture::ShutDown();
    SDynTexture2::ShutDown();

    ReleaseMiscTargets();

    m_bLoadedSystem = false;
}

void CTexture::LoadDefaultSystemTextures()
{
    LOADING_TIME_PROFILE_SECTION;
#if !defined(NULL_RENDERER)
    char str[256];
    int i;

    if (!m_bLoadedSystem)
    {
        

        m_bLoadedSystem = true;

        // Default Template textures
        int nRTFlags =  FT_DONT_RELEASE | FT_DONT_STREAM | FT_STATE_CLAMP | FT_USAGE_RENDERTARGET;
        s_ptexMipColors_Diffuse = CTexture::CreateTextureObject("$MipColors_Diffuse", 0, 0, 1, eTT_2D,  FT_DONT_RELEASE | FT_DONT_STREAM | FT_USAGE_RENDERTARGET, eTF_Unknown, TO_MIPCOLORS_DIFFUSE);
        s_ptexMipColors_Bump = CTexture::CreateTextureObject("$MipColors_Bump", 0, 0, 1, eTT_2D,  FT_DONT_RELEASE | FT_DONT_STREAM | FT_USAGE_RENDERTARGET, eTF_Unknown, TO_MIPCOLORS_BUMP);

        s_ptexRT_2D = CTexture::CreateTextureObject("$RT_2D", 0, 0, 1, eTT_2D, FT_DONT_RELEASE | FT_DONT_STREAM | FT_USAGE_RENDERTARGET, eTF_Unknown, TO_RT_2D);

        s_ptexRainOcclusion = CTexture::CreateTextureObject("$RainOcclusion", 0, 0, 1, eTT_2D, FT_DONT_RELEASE | FT_DONT_STREAM | FT_USAGE_RENDERTARGET, eTF_Unknown);
        s_ptexRainSSOcclusion[0] = CTexture::CreateTextureObject("$RainSSOcclusion0", 0, 0, 1, eTT_2D, FT_DONT_RELEASE | FT_DONT_STREAM | FT_USAGE_RENDERTARGET, eTF_Unknown);
        s_ptexRainSSOcclusion[1] = CTexture::CreateTextureObject("$RainSSOcclusion1", 0, 0, 1, eTT_2D, FT_DONT_RELEASE | FT_DONT_STREAM | FT_USAGE_RENDERTARGET, eTF_Unknown);

        s_ptexFromObj = CTexture::CreateTextureObject("FromObj", 0, 0, 1, eTT_2D, FT_DONT_RELEASE | FT_DONT_STREAM | FT_USAGE_RENDERTARGET, eTF_Unknown, TO_FROMOBJ);
        s_ptexSvoTree = CTexture::CreateTextureObject("SvoTree", 0, 0, 1, eTT_3D, FT_DONT_RELEASE | FT_DONT_STREAM | FT_USAGE_RENDERTARGET, eTF_Unknown, TO_SVOTREE);
        s_ptexSvoTris = CTexture::CreateTextureObject("SvoTris", 0, 0, 1, eTT_3D, FT_DONT_RELEASE | FT_DONT_STREAM | FT_USAGE_RENDERTARGET, eTF_Unknown, TO_SVOTRIS);
        s_ptexSvoGlobalCM = CTexture::CreateTextureObject("SvoGlobalCM", 0, 0, 1, eTT_Cube, FT_DONT_RELEASE | FT_DONT_STREAM | FT_USAGE_RENDERTARGET, eTF_Unknown, TO_SVOGLCM);
        s_ptexSvoRgbs = CTexture::CreateTextureObject("SvoRgbs", 0, 0, 1, eTT_3D, FT_DONT_RELEASE | FT_DONT_STREAM | FT_USAGE_RENDERTARGET, eTF_Unknown, TO_SVORGBS);
        s_ptexSvoNorm = CTexture::CreateTextureObject("SvoNorm", 0, 0, 1, eTT_3D, FT_DONT_RELEASE | FT_DONT_STREAM | FT_USAGE_RENDERTARGET, eTF_Unknown, TO_SVONORM);
        s_ptexSvoOpac = CTexture::CreateTextureObject("SvoOpac", 0, 0, 1, eTT_3D, FT_DONT_RELEASE | FT_DONT_STREAM | FT_USAGE_RENDERTARGET, eTF_Unknown, TO_SVOOPAC);
        s_ptexFromObjCM = CTexture::CreateTextureObject("$FromObjCM", 0, 0, 1, eTT_Cube, FT_DONT_RELEASE | FT_DONT_STREAM | FT_USAGE_RENDERTARGET, eTF_Unknown, TO_FROMOBJ_CM);

        s_ptexZTargetDownSample[0] = CTexture::CreateTextureObject("$ZTargetDownSample0", 0, 0, 1, eTT_2D, FT_DONT_RELEASE | FT_DONT_STREAM, eTF_Unknown);
        s_ptexZTargetDownSample[1] = CTexture::CreateTextureObject("$ZTargetDownSample1", 0, 0, 1, eTT_2D, FT_DONT_RELEASE | FT_DONT_STREAM, eTF_Unknown);
        s_ptexZTargetDownSample[2] = CTexture::CreateTextureObject("$ZTargetDownSample2", 0, 0, 1, eTT_2D, FT_DONT_RELEASE | FT_DONT_STREAM, eTF_Unknown);
        s_ptexZTargetDownSample[3] = CTexture::CreateTextureObject("$ZTargetDownSample3", 0, 0, 1, eTT_2D, FT_DONT_RELEASE | FT_DONT_STREAM, eTF_Unknown);

        s_ptexSceneNormalsMapMS = CTexture::CreateTextureObject("$SceneNormalsMapMS", 0, 0, 1, eTT_2D, FT_DONT_RELEASE | FT_DONT_STREAM | FT_USAGE_RENDERTARGET, eTF_Unknown, TO_SCENE_NORMALMAP_MS);
        s_ptexSceneDiffuseAccMapMS = CTexture::CreateTextureObject("$SceneDiffuseAccMS", 0, 0, 1, eTT_2D, FT_DONT_RELEASE | FT_DONT_STREAM | FT_USAGE_RENDERTARGET, eTF_Unknown, TO_SCENE_DIFFUSE_ACC_MS);
        s_ptexSceneSpecularAccMapMS = CTexture::CreateTextureObject("$SceneSpecularAccMS", 0, 0, 1, eTT_2D, FT_DONT_RELEASE | FT_DONT_STREAM | FT_USAGE_RENDERTARGET, eTF_Unknown, TO_SCENE_SPECULAR_ACC_MS);

        s_ptexSceneNormalsMapMS = CTexture::CreateTextureObject("$SceneNormalsMapMS", 0, 0, 1, eTT_2D, FT_DONT_RELEASE | FT_DONT_STREAM | FT_USAGE_RENDERTARGET, eTF_Unknown, TO_SCENE_NORMALMAP_MS);
        s_ptexSceneDiffuseAccMapMS = CTexture::CreateTextureObject("$SceneDiffuseAccMS", 0, 0, 1, eTT_2D, FT_DONT_RELEASE | FT_DONT_STREAM | FT_USAGE_RENDERTARGET, eTF_Unknown, TO_SCENE_DIFFUSE_ACC_MS);
        s_ptexSceneSpecularAccMapMS = CTexture::CreateTextureObject("$SceneSpecularAccMS", 0, 0, 1, eTT_2D, FT_DONT_RELEASE | FT_DONT_STREAM | FT_USAGE_RENDERTARGET, eTF_Unknown, TO_SCENE_SPECULAR_ACC_MS);
#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION TEXTURE_CPP_SECTION_4
    #include AZ_RESTRICTED_FILE(Texture_cpp)
#endif
        s_ptexRT_ShadowPool = CTexture::CreateTextureObject("$RT_ShadowPool", 0, 0, 1, eTT_2D, FT_DONT_STREAM | FT_USAGE_RENDERTARGET | FT_USAGE_DEPTHSTENCIL, eTF_Unknown);
        s_ptexRT_ShadowStub = CTexture::CreateTextureObject("$RT_ShadowStub", 0, 0, 1, eTT_2D, FT_DONT_STREAM | FT_USAGE_RENDERTARGET | FT_USAGE_DEPTHSTENCIL, eTF_Unknown);

        s_ptexDepthBufferQuarter = CTexture::CreateTextureObject("$DepthBufferQuarter", 0, 0, 1, eTT_2D, FT_DONT_RELEASE | FT_DONT_STREAM | FT_USAGE_RENDERTARGET | FT_USAGE_DEPTHSTENCIL, eTF_Unknown);

        if (!s_ptexModelHudBuffer)
        {
            s_ptexModelHudBuffer = CTexture::CreateTextureObject("$ModelHud", 0, 0, 1, eTT_2D, FT_DONT_RELEASE | FT_DONT_STREAM | FT_USAGE_RENDERTARGET, eTF_Unknown, TO_MODELHUD);
        }

        if (!s_ptexBackBuffer)
        {
            s_ptexSceneTarget = CTexture::CreateTextureObject("$SceneTarget", 0, 0, 1, eTT_2D, FT_DONT_RELEASE | FT_DONT_STREAM | FT_USAGE_RENDERTARGET, eTF_Unknown, TO_SCENE_TARGET);
            s_ptexCurrSceneTarget = s_ptexSceneTarget;

            s_ptexSceneTargetR11G11B10F[0] = CTexture::CreateTextureObject("$SceneTargetR11G11B10F_0", 0, 0, 1, eTT_2D, FT_DONT_RELEASE | FT_DONT_STREAM | FT_USAGE_RENDERTARGET, eTF_Unknown, -1);
            s_ptexSceneTargetR11G11B10F[1] = CTexture::CreateTextureObject("$SceneTargetR11G11B10F_1", 0, 0, 1, eTT_2D, FT_DONT_RELEASE | FT_DONT_STREAM | FT_USAGE_RENDERTARGET, eTF_Unknown, -1);
            s_ptexSceneTargetScaledR11G11B10F[0] = CTexture::CreateTextureObject("$SceneTargetScaled0R11G11B10F", 0, 0, 1, eTT_2D, FT_DONT_RELEASE | FT_DONT_STREAM | FT_USAGE_RENDERTARGET, eTF_Unknown, -1);
            s_ptexSceneTargetScaledR11G11B10F[1] = CTexture::CreateTextureObject("$SceneTargetScaled1R11G11B10F", 0, 0, 1, eTT_2D, FT_DONT_RELEASE | FT_DONT_STREAM | FT_USAGE_RENDERTARGET, eTF_Unknown, -1);
            s_ptexSceneTargetScaledR11G11B10F[2] = CTexture::CreateTextureObject("$SceneTargetScaled2R11G11B10F", 0, 0, 1, eTT_2D, FT_DONT_RELEASE | FT_DONT_STREAM | FT_USAGE_RENDERTARGET, eTF_Unknown, -1);
            s_ptexSceneTargetScaledR11G11B10F[3] = CTexture::CreateTextureObject("$SceneTargetScaled3R11G11B10F", 0, 0, 1, eTT_2D, FT_DONT_RELEASE | FT_DONT_STREAM | FT_USAGE_RENDERTARGET, eTF_Unknown, -1);

            s_ptexVelocity = CTexture::CreateTextureObject("$Velocity", 0, 0, 1, eTT_2D, FT_DONT_RELEASE | FT_DONT_STREAM | FT_USAGE_RENDERTARGET, eTF_Unknown, -1);
            s_ptexVelocityTiles[0] = CTexture::CreateTextureObject("$VelocityTilesTmp0", 0, 0, 1, eTT_2D, FT_DONT_RELEASE | FT_DONT_STREAM | FT_USAGE_RENDERTARGET, eTF_Unknown, -1);
            s_ptexVelocityTiles[1] = CTexture::CreateTextureObject("$VelocityTilesTmp1", 0, 0, 1, eTT_2D, FT_DONT_RELEASE | FT_DONT_STREAM | FT_USAGE_RENDERTARGET, eTF_Unknown, -1);
            s_ptexVelocityTiles[2] = CTexture::CreateTextureObject("$VelocityTiles", 0, 0, 1, eTT_2D, FT_DONT_RELEASE | FT_DONT_STREAM | FT_USAGE_RENDERTARGET, eTF_Unknown, -1);
            s_ptexVelocityObjects[0] = CTexture::CreateTextureObject("$VelocityObjects", 0, 0, 1, eTT_2D, FT_DONT_RELEASE | FT_DONT_STREAM | FT_USAGE_RENDERTARGET, eTF_Unknown, -1);
            if (gRenDev->m_bDualStereoSupport)
            {
                s_ptexVelocityObjects[1] = CTexture::CreateTextureObject("$VelocityObjects_R", 0, 0, 1, eTT_2D, FT_DONT_RELEASE | FT_DONT_STREAM | FT_USAGE_RENDERTARGET, eTF_Unknown, -1);
            }

#if defined(OPENGL_ES) || defined(CRY_USE_METAL)
            if (gcpRendD3D && gcpRendD3D->FX_GetEnabledGmemPath(nullptr))
            {
                s_ptexGmemStenLinDepth = CTexture::CreateTextureObject("$GmemStenLinDepth", 0, 0, 1, eTT_2D, FT_DONT_RELEASE | FT_DONT_STREAM | FT_USAGE_RENDERTARGET, eTF_Unknown, -1);
            }
#endif

            s_ptexBackBuffer = CTexture::CreateTextureObject("$BackBuffer", 0, 0, 1, eTT_2D, FT_DONT_RELEASE | FT_DONT_STREAM | FT_USAGE_RENDERTARGET, eTF_Unknown, TO_BACKBUFFERMAP);

            s_ptexPrevFrameScaled = CTexture::CreateTextureObject("$PrevFrameScale", 0, 0, 1, eTT_2D, FT_DONT_RELEASE | FT_DONT_STREAM | FT_USAGE_RENDERTARGET, eTF_Unknown);
            s_ptexWaterRipplesDDN = CTexture::CreateTextureObject("$WaterRipplesDDN_0", 256, 256, 1, eTT_2D, /*FT_DONT_RELEASE |*/ FT_DONT_STREAM | FT_USAGE_RENDERTARGET | FT_FORCE_MIPS, eTF_Unknown, TO_WATERRIPPLESMAP);

            s_ptexBackBufferScaled[0] = CTexture::CreateTextureObject("$BackBufferScaled_d2", 0, 0, 1, eTT_2D, FT_DONT_RELEASE | FT_DONT_STREAM | FT_USAGE_RENDERTARGET, eTF_Unknown, TO_BACKBUFFERSCALED_D2);
            s_ptexBackBufferScaled[1] = CTexture::CreateTextureObject("$BackBufferScaled_d4", 0, 0, 1, eTT_2D, FT_DONT_RELEASE | FT_DONT_STREAM | FT_USAGE_RENDERTARGET, eTF_Unknown, TO_BACKBUFFERSCALED_D4);
            s_ptexBackBufferScaled[2] = CTexture::CreateTextureObject("$BackBufferScaled_d8", 0, 0, 1, eTT_2D, FT_DONT_RELEASE | FT_DONT_STREAM | FT_USAGE_RENDERTARGET, eTF_Unknown, TO_BACKBUFFERSCALED_D8);

            s_ptexBackBufferScaledTemp[0] = CTexture::CreateTextureObject("$BackBufferScaledTemp_d2", 0, 0, 1, eTT_2D, FT_DONT_RELEASE | FT_DONT_STREAM | FT_USAGE_RENDERTARGET, eTF_Unknown, -1);
            s_ptexBackBufferScaledTemp[1] = CTexture::CreateTextureObject("$BackBufferScaledTemp_d4", 0, 0, 1, eTT_2D, FT_DONT_RELEASE | FT_DONT_STREAM | FT_USAGE_RENDERTARGET, eTF_Unknown, -1);

            s_ptexSceneNormalsMap = CTexture::CreateTextureObject("$SceneNormalsMap", 0, 0, 1, eTT_2D, nRTFlags, eTF_R8G8B8A8, TO_SCENE_NORMALMAP);
            s_ptexSceneNormalsBent = CTexture::CreateTextureObject("$SceneNormalsBent", 0, 0, 1, eTT_2D, nRTFlags, eTF_R8G8B8A8);
            s_ptexSceneDiffuse = CTexture::CreateTextureObject("$SceneDiffuse", 0, 0, 1, eTT_2D, nRTFlags, eTF_R8G8B8A8);
            ETEX_Format rtTextureFormat = eTF_R8G8B8A8;
            
            // Slimming GBuffer requires only one channel for specular due to packing of RGB values into YPbPr and
            // specular components into less channels, thus saving bandwidth by not including G,B,A channels (75% saving)
            if(CRenderer::CV_r_SlimGBuffer == 1)
            {
                rtTextureFormat = eTF_R8;
            }
            s_ptexSceneSpecular = CTexture::CreateTextureObject("$SceneSpecular", 0, 0, 1, eTT_2D, nRTFlags, rtTextureFormat);
#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION TEXTURE_CPP_SECTION_5
    #include AZ_RESTRICTED_FILE(Texture_cpp)
#endif
            
#if defined(AZ_PLATFORM_IOS)
            int nRTSceneDiffuseFlags =  nRTFlags;
            static ICVar* pVar = gEnv->pConsole->GetCVar("e_ShadowsClearShowMaskAtLoad");
            if (pVar && !pVar->GetIVal())
            {
                nRTSceneDiffuseFlags |= FT_USAGE_MEMORYLESS;
            }
            s_ptexSceneDiffuseAccMap = CTexture::CreateTextureObject("$SceneDiffuseAcc", 0, 0, 1, eTT_2D, nRTSceneDiffuseFlags, eTF_R8G8B8A8, TO_SCENE_DIFFUSE_ACC);
#else
            s_ptexSceneDiffuseAccMap = CTexture::CreateTextureObject("$SceneDiffuseAcc", 0, 0, 1, eTT_2D, nRTFlags, eTF_R8G8B8A8, TO_SCENE_DIFFUSE_ACC);
#endif
            s_ptexSceneSpecularAccMap = CTexture::CreateTextureObject("$SceneSpecularAcc", 0, 0, 1, eTT_2D, nRTFlags, eTF_R8G8B8A8, TO_SCENE_SPECULAR_ACC);
            s_ptexAmbientLookup = CTexture::CreateTextureObject("$AmbientLookup", 0, 0, 1, eTT_2D, nRTFlags, eTF_R8G8B8A8);
            s_ptexShadowMask = CTexture::CreateTextureObject("$ShadowMask", 0, 0, 1, eTT_2D, nRTFlags, eTF_R8G8B8A8, TO_SHADOWMASK);

            s_ptexFlaresGather = CTexture::CreateTextureObject("$FlaresGather", 0, 0, 1, eTT_2D, nRTFlags, eTF_R8G8B8A8);
            for (i = 0; i < MAX_OCCLUSION_READBACK_TEXTURES; i++)
            {
                azsprintf(str, "$FlaresOcclusion_%d", i);
                s_ptexFlaresOcclusionRing[i] = CTexture::CreateTextureObject(str, 0, 0, 1, eTT_2D, nRTFlags, eTF_R8G8B8A8);
            }

            // fixme: get texture resolution from CREWaterOcean
            uint32 waterOceanMapFlags = FT_DONT_RELEASE | FT_NOMIPS | FT_USAGE_DYNAMIC | FT_DONT_STREAM;
            uint32 waterVolumeTempFlags = FT_NOMIPS | FT_USAGE_DYNAMIC | FT_DONT_STREAM;
#if CRY_USE_METAL
            //We are now using the GPU to copy data into this texture. As a result we need to tag this texture as MtlTextureUsageRenderTarget so that on Metal can set the appropriate Usage flag.
            waterOceanMapFlags |= FT_USAGE_RENDERTARGET;
            waterVolumeTempFlags |= FT_USAGE_RENDERTARGET;
#endif
            s_ptexWaterOcean = CTexture::CreateTextureObject("$WaterOceanMap", 64, 64, 1, eTT_2D, waterOceanMapFlags, eTF_Unknown, TO_WATEROCEANMAP);
            s_ptexWaterVolumeTemp = CTexture::CreateTextureObject("$WaterVolumeTemp", 64, 64, 1, eTT_2D, waterVolumeTempFlags, eTF_Unknown);

            s_ptexWaterVolumeDDN = CTexture::CreateTextureObject("$WaterVolumeDDN", 64, 64, 1, eTT_2D, /*FT_DONT_RELEASE |*/ FT_DONT_STREAM | FT_USAGE_RENDERTARGET | FT_FORCE_MIPS, eTF_Unknown,  TO_WATERVOLUMEMAP);
            s_ptexWaterVolumeRefl[0] = CTexture::CreateTextureObject("$WaterVolumeRefl", 64, 64, 1, eTT_2D, FT_DONT_RELEASE | FT_DONT_STREAM | FT_USAGE_RENDERTARGET | FT_FORCE_MIPS, eTF_Unknown, TO_WATERVOLUMEREFLMAP);
            s_ptexWaterVolumeRefl[1] = CTexture::CreateTextureObject("$WaterVolumeReflPrev", 64, 64, 1, eTT_2D, FT_DONT_RELEASE | FT_DONT_STREAM | FT_USAGE_RENDERTARGET | FT_FORCE_MIPS, eTF_Unknown, TO_WATERVOLUMEREFLMAPPREV);
            s_ptexWaterCaustics[0] = CTexture::CreateTextureObject("$WaterVolumeCaustics", 512, 512, 1, eTT_2D, /*FT_DONT_RELEASE |*/ FT_DONT_STREAM | FT_USAGE_RENDERTARGET, eTF_Unknown, TO_WATERVOLUMECAUSTICSMAP);
            s_ptexWaterCaustics[1] = CTexture::CreateTextureObject("$WaterVolumeCausticsTemp", 512, 512, 1, eTT_2D, /*FT_DONT_RELEASE |*/ FT_DONT_STREAM | FT_USAGE_RENDERTARGET, eTF_Unknown, TO_WATERVOLUMECAUSTICSMAPTEMP);

            s_ptexRainDropsRT[0] = CTexture::CreateTextureObject("$RainDropsAccumRT_0", 512, 512, 1, eTT_2D, FT_DONT_RELEASE | FT_DONT_STREAM | FT_USAGE_RENDERTARGET, eTF_Unknown);
            s_ptexRainDropsRT[1] = CTexture::CreateTextureObject("$RainDropsAccumRT_1", 512, 512, 1, eTT_2D, FT_DONT_RELEASE | FT_DONT_STREAM | FT_USAGE_RENDERTARGET, eTF_Unknown);

            if (!s_ptexZTarget)
            {
                //for d3d10 we cannot free it during level transition, therefore allocate once and keep it
#if defined(OPENGL_ES) || defined(CRY_USE_METAL)
                // Custom Z-Target for GMEM render path
                if (gcpRendD3D && gcpRendD3D->FX_GetEnabledGmemPath(nullptr))
                {
                    s_ptexZTarget = CTexture::s_ptexGmemStenLinDepth;
                }
                else
#endif
                {
                    s_ptexZTarget = CTexture::CreateTextureObject("$ZTarget", 0, 0, 1, eTT_2D, FT_DONT_RELEASE | FT_DONT_STREAM | FT_USAGE_RENDERTARGET, eTF_Unknown);
                }
            }

            s_ptexFurZTarget = CTexture::CreateTextureObject("$FurZTarget", 0, 0, 1, eTT_2D, FT_DONT_RELEASE | FT_DONT_STREAM | FT_USAGE_RENDERTARGET, eTF_Unknown);

            s_ptexZTargetScaled = CTexture::CreateTextureObject("$ZTargetScaled", 0, 0, 1, eTT_2D,
                    FT_DONT_RELEASE | FT_DONT_STREAM | FT_USAGE_RENDERTARGET, eTF_Unknown, TO_DOWNSCALED_ZTARGET_FOR_AO);

            s_ptexZTargetScaled2 = CTexture::CreateTextureObject("$ZTargetScaled2", 0, 0, 1, eTT_2D,
                    FT_DONT_RELEASE | FT_DONT_STREAM | FT_USAGE_RENDERTARGET, eTF_Unknown, TO_QUARTER_ZTARGET_FOR_AO);
        }

#if defined(OPENGL_ES) || defined(CRY_USE_METAL)
        // GMEM render path uses CTexture::s_ptexSceneSpecularAccMap as the HDR Target
        // It gets set in CDeferredShading::CreateDeferredMaps()
        if (gcpRendD3D && !gcpRendD3D->FX_GetEnabledGmemPath(nullptr))
        {
            s_ptexHDRTarget = CTexture::CreateTextureObject("$HDRTarget", 0, 0, 1, eTT_2D, nRTFlags, eTF_Unknown);
        }
#elif defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION TEXTURE_CPP_SECTION_6
    #include AZ_RESTRICTED_FILE(Texture_cpp)
#endif

        // Create dummy texture object for terrain and clouds lightmap
        s_ptexCloudsLM = CTexture::CreateTextureObject("$CloudsLM", 0, 0, 1, eTT_2D, FT_DONT_RELEASE | FT_DONT_STREAM | FT_USAGE_RENDERTARGET, eTF_Unknown, TO_CLOUDS_LM);

        for (i = 0; i < 8; i++)
        {
            azsprintf(str, "$FromRE_%d", i);
            if (!s_ptexFromRE[i])
            {
                s_ptexFromRE[i] = CTexture::CreateTextureObject(str, 0, 0, 1, eTT_2D, FT_DONT_RELEASE | FT_DONT_STREAM | FT_USAGE_RENDERTARGET, eTF_Unknown, TO_FROMRE0 + i);
            }
        }

        for (i = 0; i < 8; i++)
        {
            azsprintf(str, "$ShadowID_%d", i);
            s_ptexShadowID[i] = CTexture::CreateTextureObject(str, 0, 0, 1, eTT_2D, FT_DONT_RELEASE | FT_DONT_STREAM | FT_USAGE_RENDERTARGET, eTF_Unknown, TO_SHADOWID0 + i);
        }

        for (i = 0; i < 2; i++)
        {
            azsprintf(str, "$FromRE%d_FromContainer", i);
            if (!s_ptexFromRE_FromContainer[i])
            {
                s_ptexFromRE_FromContainer[i] = CTexture::CreateTextureObject(str, 0, 0, 1, eTT_2D, FT_DONT_RELEASE | FT_DONT_STREAM | FT_USAGE_RENDERTARGET, eTF_Unknown, TO_FROMRE0_FROM_CONTAINER + i);
            }
        }

        s_ptexVolObj_Density = CTexture::CreateTextureObject("$VolObj_Density", 0, 0, 1, eTT_2D, FT_DONT_RELEASE | FT_DONT_STREAM | FT_USAGE_RENDERTARGET, eTF_Unknown, TO_VOLOBJ_DENSITY);
        s_ptexVolObj_Shadow = CTexture::CreateTextureObject("$VolObj_Shadow", 0, 0, 1, eTT_2D, FT_DONT_RELEASE | FT_DONT_STREAM | FT_USAGE_RENDERTARGET, eTF_Unknown, TO_VOLOBJ_SHADOW);

        s_ptexColorChart = CTexture::CreateTextureObject("$ColorChart", 0, 0, 1, eTT_2D, FT_DONT_RELEASE | FT_DONT_STREAM | FT_USAGE_RENDERTARGET, eTF_Unknown, TO_COLORCHART);

        s_ptexSkyDomeMie = CTexture::CreateTextureObject("$SkyDomeMie", 0, 0, 1, eTT_2D, FT_DONT_RELEASE | FT_DONT_STREAM | FT_USAGE_RENDERTARGET, eTF_Unknown, TO_SKYDOME_MIE);
        s_ptexSkyDomeRayleigh = CTexture::CreateTextureObject("$SkyDomeRayleigh", 0, 0, 1, eTT_2D, FT_DONT_RELEASE | FT_DONT_STREAM | FT_USAGE_RENDERTARGET, eTF_Unknown, TO_SKYDOME_RAYLEIGH);
        s_ptexSkyDomeMoon = CTexture::CreateTextureObject("$SkyDomeMoon", 0, 0, 1, eTT_2D, FT_DONT_RELEASE | FT_DONT_STREAM | FT_USAGE_RENDERTARGET, eTF_Unknown, TO_SKYDOME_MOON);

        for (i = 0; i < EFTT_MAX; i++)
        {
            ::new(&((*s_ShaderTemplates)[i]))CTexture(FT_DONT_RELEASE);
            (*s_ShaderTemplates)[i].SetCustomID(EFTT_DIFFUSE + i);
            (*s_ShaderTemplates)[i].SetFlags(FT_DONT_RELEASE);
        }
        s_ShaderTemplatesInitialized = true;

        s_pTexNULL = new CTexture(FT_DONT_RELEASE);

        s_ptexVolumetricFog = CTexture::CreateTextureObject("$VolumetricInscattering", 0, 0, 0, eTT_3D, FT_NOMIPS | FT_DONT_RELEASE | FT_DONT_STREAM | FT_USAGE_UNORDERED_ACCESS, eTF_Unknown);
        s_ptexVolumetricFogDensityColor = CTexture::CreateTextureObject("$DensityColorVolume", 0, 0, 0, eTT_3D, FT_NOMIPS | FT_DONT_RELEASE | FT_DONT_STREAM | FT_USAGE_RENDERTARGET | FT_USAGE_UNORDERED_ACCESS, eTF_Unknown);
        s_ptexVolumetricFogDensity = CTexture::CreateTextureObject("$DensityVolume", 0, 0, 0, eTT_3D, FT_NOMIPS | FT_DONT_RELEASE | FT_DONT_STREAM | FT_USAGE_RENDERTARGET | FT_USAGE_UNORDERED_ACCESS, eTF_Unknown);
        s_ptexVolumetricClipVolumeStencil = CTexture::CreateTextureObject("$ClipVolumeStencilVolume", 0, 0, 0, eTT_2D, FT_NOMIPS | FT_DONT_RELEASE | FT_DONT_STREAM | FT_USAGE_DEPTHSTENCIL | FT_USAGE_RENDERTARGET, eTF_Unknown);

        // Create dummy texture object the "default environment probe".  This is only used for forward rendered passes that do not currently support tiled lighting.
        // This texture object is solely used for the association between the string "$DefaultEnvironmentProbe" and the enum TO_DEFAULT_ENVIRONMENT_PROBE
        if (!s_defaultEnvironmentProbeDummy)
        {
            s_defaultEnvironmentProbeDummy = CTexture::CreateTextureObject("$DefaultEnvironmentProbe", 0, 0, 1, eTT_2D, FT_DONT_RELEASE | FT_DONT_STREAM, eTF_Unknown, TO_DEFAULT_ENVIRONMENT_PROBE);
        }

#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION TEXTURE_CPP_SECTION_7
    #include AZ_RESTRICTED_FILE(Texture_cpp)
#endif
    }
#endif
}

//////////////////////////////////////////////////////////////////////////
const char* CTexture::GetFormatName() const
{
    return NameForTextureFormat(GetDstFormat());
}

//////////////////////////////////////////////////////////////////////////
const char* CTexture::GetTypeName() const
{
    return NameForTextureType(GetTextureType());
}

//////////////////////////////////////////////////////////////////////////
void CRenderer::EF_AddRTStat(CTexture* pTex, int nFlags, int nW, int nH)
{
    SRTargetStat TS;
    int nSize;
    ETEX_Format eTF;
    if (!pTex)
    {
        eTF = eTF_R8G8B8A8;
        if (nW < 0)
        {
            nW = m_width;
        }
        if (nH < 0)
        {
            nH = m_height;
        }
        nSize = CTexture::TextureDataSize(nW, nH, 1, 1, 1, eTF);
        TS.m_Name = "Back buffer";
    }
    else
    {
        eTF = pTex->GetDstFormat();
        if (nW < 0)
        {
            nW = pTex->GetWidth();
        }
        if (nH < 0)
        {
            nH = pTex->GetHeight();
        }
        nSize = CTexture::TextureDataSize(nW, nH, 1, pTex->GetNumMips(), 1, eTF);
        const char* szName = pTex->GetName();
        if (szName && szName[0] == '$')
        {
            TS.m_Name = string("@") + string(&szName[1]);
        }
        else
        {
            TS.m_Name = szName;
        }
    }
    TS.m_eTF = eTF;

    if (nFlags > 0)
    {
        if (nFlags == 1)
        {
            TS.m_Name += " (Target)";
        }
        else
        if (nFlags == 2)
        {
            TS.m_Name += " (Depth)";
            nSize = nW * nH * 3;
        }
        else
        if (nFlags == 4)
        {
            TS.m_Name += " (Stencil)";
            nSize = nW * nH;
        }
        else
        if (nFlags == 3)
        {
            TS.m_Name += " (Target + Depth)";
            nSize += nW * nH * 3;
        }
        else
        if (nFlags == 6)
        {
            TS.m_Name += " (Depth + Stencil)";
            nSize = nW * nH * 4;
        }
        else
        if (nFlags == 5)
        {
            TS.m_Name += " (Target + Stencil)";
            nSize += nW * nH;
        }
        else
        if (nFlags == 7)
        {
            TS.m_Name += " (Target + Depth + Stencil)";
            nSize += nW * nH * 4;
        }
        else
        {
            assert(0);
        }
    }
    TS.m_nSize = nSize;
    TS.m_nWidth = nW;
    TS.m_nHeight = nH;

    m_RP.m_RTStats.push_back(TS);
}

void CRenderer::EF_PrintRTStats(const char* szName)
{
    const int nYstep = 14;
    int nY = 30; // initial Y pos
    int nX = 20; // initial X pos
    ColorF col = Col_Green;
    Draw2dLabel((float)nX, (float)nY, 1.6f, &col.r, false, szName);
    nX += 10;
    nY += 25;

    col = Col_White;
    int nYstart = nY;
    int nSize = 0;
    for (int i = 0; i < m_RP.m_RTStats.size(); i++)
    {
        SRTargetStat* pRT = &m_RP.m_RTStats[i];

        Draw2dLabel((float)nX, (float)nY, 1.4f, &col.r, false, "%s (%d x %d x %s), Size: %.3f Mb", pRT->m_Name.c_str(), pRT->m_nWidth, pRT->m_nHeight, CTexture::NameForTextureFormat(pRT->m_eTF), (float)pRT->m_nSize / 1024.0f / 1024.0f);
        nY += nYstep;
        if (nY >= m_height - 25)
        {
            nY = nYstart;
            nX += 500;
        }
        nSize += pRT->m_nSize;
    }
    col = Col_Yellow;
    Draw2dLabel((float)nX, (float)(nY + 10), 1.4f, &col.r, false, "Total: %d RT's, Size: %.3f Mb", m_RP.m_RTStats.size(), nSize / 1024.0f / 1024.0f);
}

bool CTexture::IsMSAAChanged()
{
#if defined(NULL_RENDERER)
    return false;
#else
    const RenderTargetData* pRtdt = m_pRenderTargetData;
    return pRtdt && (pRtdt->m_nMSAASamples != gRenDev->m_RP.m_MSAAData.Type || pRtdt->m_nMSAAQuality != gRenDev->m_RP.m_MSAAData.Quality);
#endif
}

STexPool::~STexPool()
{
    STexPoolItemHdr* pITH = m_ItemsList.m_Next;

    while (pITH != &m_ItemsList)
    {
        STexPoolItemHdr* pNext = pITH->m_Next;
        STexPoolItem* pIT = static_cast<STexPoolItem*>(pITH);
        CryLogAlways("***** Texture %p (%s) still in pool %p! Memory leak and crash will follow *****\n", pIT->m_pTex, pIT->m_pTex ? pIT->m_pTex->GetName() : "NULL", this);

        if (pIT->m_pTex)
        {
            pIT->m_pTex->ReleaseDeviceTexture(true); // Try to recover in release
        }

        *const_cast<STexPool**>(&pIT->m_pOwner) = NULL;
        pITH = pNext;
    }
}

const ETEX_Type CTexture::GetTextureType() const
{
    return m_eTT;
}

void CTexture::SetTextureType(ETEX_Type type)
{
    // Only set the type if we have not loaded the file and created the device
    // texture
    if (!m_pDevTexture)
    {
        m_eTT = type;
    }
}

const int CTexture::GetTextureID() const
{
    return GetID();
}

#ifdef TEXTURE_GET_SYSTEM_COPY_SUPPORT

const ColorB* CTexture::GetLowResSystemCopy(uint16& nWidth, uint16& nHeight, int** ppLowResSystemCopyAtlasId)
{
    const LowResSystemCopyType::iterator& it = s_LowResSystemCopy.find(this);
    if (it != CTexture::s_LowResSystemCopy.end())
    {
        nWidth = (*it).second.m_nLowResCopyWidth;
        nHeight = (*it).second.m_nLowResCopyHeight;
        *ppLowResSystemCopyAtlasId = &(*it).second.m_nLowResSystemCopyAtlasId;
        return (*it).second.m_lowResSystemCopy.GetElements();
    }

    return NULL;
}

void CTexture::PrepareLowResSystemCopy(const byte* pTexData, bool bTexDataHasAllMips)
{
    if (m_eTT != eTT_2D || (m_nMips <= 1 && (m_nWidth > 16 || m_nHeight > 16)))
    {
        return;
    }

    // this function handles only compressed textures for now
    if (m_eTFDst != eTF_BC3 && m_eTFDst != eTF_BC1 && m_eTFDst != eTF_BC2)
    {
        return;
    }

    // make sure we skip non diffuse textures
    if (strstr(GetName(), "_ddn")
        || strstr(GetName(), "_ddna")
        || strstr(GetName(), "_mask")
        || strstr(GetName(), "_spec.")
        || strstr(GetName(), "_gloss")
        || strstr(GetName(), "_displ")
        || strstr(GetName(), "characters")
        || strstr(GetName(), "$")
        )
    {
        return;
    }

    if (pTexData)
    {
        SLowResSystemCopy& rSysCopy = s_LowResSystemCopy[this];

        rSysCopy.m_nLowResCopyWidth = m_nWidth;
        rSysCopy.m_nLowResCopyHeight = m_nHeight;

        int nSrcOffset = 0;
        int nMipId = 0;

        while ((rSysCopy.m_nLowResCopyWidth > 16 || rSysCopy.m_nLowResCopyHeight > 16 || nMipId < 2) && (rSysCopy.m_nLowResCopyWidth >= 8 && rSysCopy.m_nLowResCopyHeight >= 8))
        {
            nSrcOffset += TextureDataSize(rSysCopy.m_nLowResCopyWidth, rSysCopy.m_nLowResCopyHeight, 1, 1, 1, m_eTFDst);
            rSysCopy.m_nLowResCopyWidth /= 2;
            rSysCopy.m_nLowResCopyHeight /= 2;
            nMipId++;
        }

        int nSizeDxtMip = TextureDataSize(rSysCopy.m_nLowResCopyWidth, rSysCopy.m_nLowResCopyHeight, 1, 1, 1, m_eTFDst);
        int nSizeRgbaMip = TextureDataSize(rSysCopy.m_nLowResCopyWidth, rSysCopy.m_nLowResCopyHeight, 1, 1, 1, eTF_R8G8B8A8);

        rSysCopy.m_lowResSystemCopy.CheckAllocated(nSizeRgbaMip / sizeof(ColorB));

        gRenDev->DXTDecompress(pTexData + (bTexDataHasAllMips ? nSrcOffset : 0), nSizeDxtMip,
            (byte*)rSysCopy.m_lowResSystemCopy.GetElements(), rSysCopy.m_nLowResCopyWidth, rSysCopy.m_nLowResCopyHeight, 1, m_eTFDst, false, 4);
    }
}

#endif // TEXTURE_GET_SYSTEM_COPY_SUPPORT

void CTexture::InvalidateDeviceResource(uint32 dirtyFlags)
{
    //In the editor, multiple worker threads could destroy device resource sets
    //which point to this texture. We need to lock to avoid a race-condition.
    AZStd::lock_guard<AZStd::mutex> lockGuard(*m_invalidateCallbacksMutex);

    for (const auto& cb : m_invalidateCallbacks)
    {
        cb.second(dirtyFlags);
    }
}

void CTexture::AddInvalidateCallback(void* listener, InvalidateCallbackType callback)
{
    //In the editor, multiple worker threads could destroy device resource sets
    //which point to this texture. We need to lock to avoid a race-condition.
    AZStd::lock_guard<AZStd::mutex> lockGuard(*m_invalidateCallbacksMutex);

    m_invalidateCallbacks.insert(AZStd::pair<void*, InvalidateCallbackType>(listener, callback));
}

void CTexture::RemoveInvalidateCallbacks(void* listener)
{
    //In the editor, multiple worker threads could destroy device resource sets
    //which point to this texture. We need to lock to avoid a race-condition.
    AZStd::lock_guard<AZStd::mutex> lockGuard(*m_invalidateCallbacksMutex);

    m_invalidateCallbacks.erase(listener);
}

void CTexture::ApplyDepthTextureState(int unit, int nFilter, bool clamp)
{
    if (s_ptexZTarget != nullptr)
    {
        STexState depthTextState(nFilter, clamp);
        s_ptexZTarget->Apply(unit, GetTexState(depthTextState));
    }
}

CTexture* CTexture::GetZTargetTexture()
{
    return s_ptexZTarget;
}

int CTexture::GetTextureState(const STexState& TS)
{
    return GetTexState(TS);
}

void CTexture::ApplyForID(int id, int nTUnit, int nTState, int nTexMaterialSlot, int nSUnit, bool useWhiteDefault)
{
    CTexture* pTex = id > 0 ? CTexture::GetByID(id) : nullptr;
    if (pTex)
    {
        pTex->Apply(nTUnit, nTState, nTexMaterialSlot, nSUnit);
    }
    else if (useWhiteDefault)
    {
        CTextureManager::Instance()->GetWhiteTexture()->Apply(nTUnit, nTState, nTexMaterialSlot, nSUnit);
    }
}

CTexAnim::CTexAnim()
{
    m_nRefCount = 1;
    m_Rand = 0;
    m_NumAnimTexs = 0;
    m_bLoop = false;
    m_Time = 0.0f;
}

CTexAnim::~CTexAnim()
{
    for (uint32 i = 0; i < m_TexPics.Num(); i++)
    {
        ITexture* pTex = (ITexture*)m_TexPics[i];
        SAFE_RELEASE(pTex);
    }
    m_TexPics.Free();
}

void CTexAnim::AddRef()
{   
    CryInterlockedIncrement(&m_nRefCount);
}

void CTexAnim::Release()
{
    long refCnt = CryInterlockedDecrement(&m_nRefCount);
    if (refCnt > 0)
    {
        return;
    }
    delete this;
}

int CTexAnim::Size() const
{
    int nSize = sizeof(CTexAnim);
    nSize += m_TexPics.GetMemoryUsage();
    return nSize;
}
