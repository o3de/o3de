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

// Description : implementation of the Shaders manager.


#include "RenderDll_precompiled.h"
#include "I3DEngine.h"
#include "CryHeaders.h"
#include "CryPath.h"
#include <Common/RenderCapabilities.h>
#include <AzCore/std/algorithm.h>
#include <AzFramework/Archive/IArchive.h>
#include <AzCore/NativeUI/NativeUIRequests.h>

#if defined(AZ_RESTRICTED_PLATFORM)
#undef AZ_RESTRICTED_SECTION
#define SHADERCORE_CPP_SECTION_1 1
#define SHADERCORE_CPP_SECTION_2 2
#endif

CShader* CShaderMan::s_DefaultShader;
CShader* CShaderMan::s_shPostEffects;
CShader* CShaderMan::s_shPostDepthOfField;
CShader* CShaderMan::s_shPostMotionBlur;
CShader* CShaderMan::s_shPostSunShafts;
CShader* CShaderMan::s_shDeferredShading;
CShader* CShaderMan::s_ShaderDeferredCaustics;
CShader* CShaderMan::s_ShaderDeferredRain;
CShader* CShaderMan::s_ShaderDeferredSnow;
#ifndef NULL_RENDERER
CShader* CShaderMan::s_ShaderFPEmu;
CShader* CShaderMan::s_ShaderUI;
CShader* CShaderMan::s_ShaderFallback;
CShader* CShaderMan::s_ShaderStars;

CShader* CShaderMan::s_ShaderShadowBlur;
CShader* CShaderMan::s_ShaderShadowMaskGen;
#if defined(FEATURE_SVO_GI)
CShader* CShaderMan::s_ShaderSVOGI;
#endif
CShader* CShaderMan::s_shHDRPostProcess;
CShader* CShaderMan::s_ShaderDebug;
CShader* CShaderMan::s_ShaderLensOptics;
CShader* CShaderMan::s_ShaderSoftOcclusionQuery;
CShader* CShaderMan::s_ShaderLightStyles;
CShader* CShaderMan::s_shPostEffectsGame;
CShader* CShaderMan::s_shPostAA;
CShader* CShaderMan::s_ShaderCommon;
CShader* CShaderMan::s_ShaderOcclTest;
CShader* CShaderMan::s_ShaderDXTCompress = nullptr;
CShader* CShaderMan::s_ShaderStereo = nullptr;
CShader* CShaderMan::s_ShaderFur = nullptr;
CShader* CShaderMan::s_ShaderVideo = nullptr;
#else
SShaderItem CShaderMan::s_DefaultShaderItem;
#endif

CCryNameTSCRC CShaderMan::s_cNameHEAD;

TArray<CShaderResources*> CShader::s_ShaderResources_known;  // Based on BatteryPark
TArray <CLightStyle*> CLightStyle::s_LStyles;

SResourceContainer* CShaderMan::s_pContainer;  // List/Map of objects for shaders resource class
FXCompressedShaders CHWShader::m_CompressedShaders;

uint64 g_HWSR_MaskBit[HWSR_MAX];
AZStd::pair<const char*, uint64> g_HWSST_Flags[] =
{
#undef FX_STATIC_FLAG
#define FX_STATIC_FLAG(flag) AZStd::make_pair("%ST_"#flag, 0),
#include "ShaderStaticFlags.inl"
};

bool gbRgb;

////////////////////////////////////////////////////////////////////////////////
// Pool for texture modificators

#if POOL_TEXMODIFICATORS

SEfTexModPool::ModificatorList SEfTexModPool::s_pool;
volatile int SEfTexModPool::s_lockState = 0;

SEfTexModificator* SEfTexModPool::Add(SEfTexModificator& mod)
{
    mod.m_crc = CCrc32::Compute((const char*)&mod, sizeof(SEfTexModificator) - sizeof(uint16) - sizeof(uint32));
    ModificatorList::iterator it = s_pool.find(mod.m_crc);
    if (it != s_pool.end())
    {
        ++((*it).second->m_refs);
        return it->second;
    }
    mod.m_refs = 1;
    SEfTexModificator* pMod = new SEfTexModificator(mod);
    s_pool.insert(std::pair<uint32, SEfTexModificator*>(mod.m_crc, pMod));

    return pMod;
}

void SEfTexModPool::AddRef(SEfTexModificator* pMod)
{
    Lock();
    if (pMod)
    {
        ++(pMod->m_refs);
    }
    Unlock();
}

void SEfTexModPool::Remove(SEfTexModificator* pMod)
{
    Lock();
    Remove_NoLock(pMod);
    Unlock();
}

void SEfTexModPool::Remove_NoLock(SEfTexModificator* pMod)
{
    if (pMod)
    {
        if (pMod->m_refs > 1)
        {
            --(pMod->m_refs);
        }
        else
        {
            ModificatorList::iterator it = s_pool.find(pMod->m_crc);
            if (it != s_pool.end())
            {
                delete pMod;
                s_pool.erase(it);
            }
        }
    }
}

void SEfTexModPool::Update(SEfTexModificator*& pMod, SEfTexModificator& newMod)
{
    Lock();
    if (pMod)
    {
        if (pMod->m_refs == 1)
        {
            *pMod = newMod;
        }
        else if (memcmp(pMod, &newMod, sizeof(SEfTexModificator) - sizeof(uint16) - sizeof(uint32)))            // Ignore reference count and crc
        {
            Remove_NoLock(pMod);
            pMod = Add(newMod);
        }
    }
    else
    {
        pMod = Add(newMod);
    }
    Unlock();
}

void SEfTexModPool::Lock(void)
{
    CrySpinLock(&s_lockState, 0, 1);
    // Locked
}

void SEfTexModPool::Unlock(void)
{
    CrySpinLock(&s_lockState, 1, 0);
    // Unlocked
}
#endif

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////


EShaderLanguage GetShaderLanguage()
{
    EShaderLanguage shaderLanguage = eSL_Unknown;
    if (CParserBin::m_nPlatform == SF_ORBIS)
    {
        shaderLanguage = eSL_Orbis;
    }
    else if (CParserBin::m_nPlatform == SF_D3D11)
    {
        shaderLanguage = eSL_D3D11;
    }
    else if (CParserBin::m_nPlatform == SF_GL4)
    {
        shaderLanguage = eSL_GL4_4;
    }
    else if (CParserBin::m_nPlatform == SF_GLES3)
    {
        shaderLanguage = gRenDev->m_cEF.HasStaticFlag(HWSST_GLES3_0) ? eSL_GLES3_0 : eSL_GLES3_1;
    }
    else if (CParserBin::m_nPlatform == SF_METAL)
    {
        shaderLanguage = eSL_METAL;
    }
    else if (CParserBin::m_nPlatform == SF_JASPER)
    {
        shaderLanguage = eSL_Jasper;
    }

    return shaderLanguage;
}

const char* GetShaderLanguageName()
{
    static const char *platformNames[eSL_MAX] =
    {
        "Unknown",
        "Orbis",
        "D3D11",
        "GL4",
        "GL4",
        "GLES3",
        "GLES3",
        "METAL",
        "Jasper"
    };

    EShaderLanguage shaderLanguage = GetShaderLanguage();
    return platformNames[shaderLanguage];
}

const char* GetShaderLanguageResourceName()
{
    static const char *platformResourceNames[eSL_MAX] =
    {
        "(UNK)",
        "(O)",
        "(DX1)",
        "(G4)",
        "(G4)",
        "(E3)",
        "(E3)",
        "(MET)",
        "(JAS)"
    };

    EShaderLanguage shaderLanguage = GetShaderLanguage();
    return platformResourceNames[shaderLanguage];
}

AZStd::string GetShaderListFilename()
{
    return AZStd::string::format("ShaderList_%s.txt", GetShaderLanguageName());
}


//////////////////////////////////////////////////////////////////////////
// Global shader parser helper pointer.
CShaderParserHelper* g_pShaderParserHelper = 0;

//=================================================================================================

int CShader::GetTexId()
{
    CTexture* tp = (CTexture*)GetBaseTexture(NULL, NULL);
    if (!tp)
    {
        return -1;
    }
    return tp->GetTextureID();
}

int CShader::mfSize()
{
    uint32 i;

    int nSize = sizeof(CShader);
    nSize += m_NameFile.capacity();
    nSize += m_NameShader.capacity();
    nSize += m_HWTechniques.GetMemoryUsage();
    for (i = 0; i < m_HWTechniques.Num(); i++)
    {
        nSize += m_HWTechniques[i]->Size();
    }

    return nSize;
}

void CShader::GetMemoryUsage(ICrySizer* pSizer) const
{
    pSizer->Add(*this);
    pSizer->AddObject(m_NameFile);
    pSizer->AddObject(m_NameShader);
    pSizer->AddObject(m_HWTechniques);
}

void CShader::mfFree()
{
    uint32 i;

    for (i = 0; i < m_HWTechniques.Num(); i++)
    {
        SShaderTechnique* pTech = m_HWTechniques[i];
        SAFE_DELETE(pTech);
    }
    m_HWTechniques.Free();

    //SAFE_DELETE(m_ShaderGenParams);
    m_Flags &= ~(EF_PARSE_MASK | EF_NODRAW);
    m_nMDV = 0;
}

CShader::~CShader()
{
    gRenDev->m_cEF.m_Bin.mfRemoveFXParams(this);

    if (m_pGenShader && m_pGenShader->m_DerivedShaders)
    {
        uint32 i;
        for (i = 0; i < m_pGenShader->m_DerivedShaders->size(); i++)
        {
            CShader* pSH = (*m_pGenShader->m_DerivedShaders)[i];
            if (pSH == this)
            {
                (*m_pGenShader->m_DerivedShaders)[i] = NULL;
                break;
            }
        }
        assert(i != m_pGenShader->m_DerivedShaders->size());
    }
    mfFree();

    SAFE_RELEASE(m_pGenShader);
    SAFE_DELETE(m_DerivedShaders);
}

CShader& CShader::operator = (const CShader& src)
{
    uint32 i;

    mfFree();

    int Offs = (int)(INT_PTR)&(((CShader*)0)->m_eSHDType);
    byte* d = (byte*)this;
    byte* s = (byte*)&src;
    memcpy(&d[Offs], &s[Offs], sizeof(CShader) - Offs);

    m_NameShader = src.m_NameShader;
    m_NameFile = src.m_NameFile;
    m_NameShaderICRC = src.m_NameShaderICRC;

    if (src.m_HWTechniques.Num())
    {
        m_HWTechniques.Create(src.m_HWTechniques.Num());
        for (i = 0; i < src.m_HWTechniques.Num(); i++)
        {
            m_HWTechniques[i] = new SShaderTechnique(this);
            *m_HWTechniques[i] = *src.m_HWTechniques[i];
            m_HWTechniques[i]->m_shader = this; // copy operator will override m_shader
        }
    }

    return *this;
}

SShaderPass::SShaderPass()
{
    m_RenderState = GS_DEPTHWRITE;

    m_PassFlags = 0;
    m_AlphaRef = ~0;

    m_VShader = NULL;
    m_PShader = NULL;
    m_GShader = NULL;
    m_DShader = NULL;
    m_HShader = NULL;
}


bool SShaderItem::IsMergable(SShaderItem& PrevSI)
{
    if (!PrevSI.m_pShader)
    {
        return true;
    }
    CShaderResources* pRP = (CShaderResources*)PrevSI.m_pShaderResources;
    CShaderResources* pR = (CShaderResources*)m_pShaderResources;
    if (pRP && pR)
    {
        if (pRP->m_AlphaRef != pR->m_AlphaRef)
        {
            return false;
        }
        if (pRP->GetStrengthValue(EFTT_OPACITY) != pR->GetStrengthValue(EFTT_OPACITY))
        {
            return false;
        }
        if (pRP->m_pDeformInfo != pR->m_pDeformInfo)
        {
            return false;
        }
        if ((pRP->m_ResFlags & MTL_FLAG_2SIDED) != (pR->m_ResFlags & MTL_FLAG_2SIDED))
        {
            return false;
        }
        if ((pRP->m_ResFlags & MTL_FLAG_NOSHADOW) != (pR->m_ResFlags & MTL_FLAG_NOSHADOW))
        {
            return false;
        }
        if (m_pShader->GetCull() != PrevSI.m_pShader->GetCull())
        {
            return false;
        }
    }
    return true;
}

void SShaderTechnique::UpdatePreprocessFlags([[maybe_unused]] CShader* pSH)
{
    uint32 i;
    for (i = 0; i < m_Passes.Num(); i++)
    {
        SShaderPass* pPass = &m_Passes[i];
        if (pPass->m_PShader)
        {
            pPass->m_PShader->mfUpdatePreprocessFlags(this);
        }
    }
}

SShaderTechnique* CShader::mfGetStartTechnique(int nTechnique)
{
    FUNCTION_PROFILER_RENDER_FLAT
    SShaderTechnique* pTech;
    if (m_HWTechniques.Num())
    {
        pTech = m_HWTechniques[0];
        if (nTechnique > 0)
        {
            assert(nTechnique < (int)m_HWTechniques.Num());
            if (nTechnique < (int)m_HWTechniques.Num())
            {
                pTech = m_HWTechniques[nTechnique];
            }
            else
            {
                iLog->Log("ERROR: CShader::mfGetStartTechnique: Technique %d for shader '%s' is out of range", nTechnique, GetName());
            }
        }
    }
    else
    {
        pTech = NULL;
    }

    return pTech;
}
SShaderTechnique* CShader::GetTechnique(int nStartTechnique, int nRequestedTechnique)
{
    SShaderTechnique* pTech = 0;
    if (m_HWTechniques.Num())
    {
        pTech = m_HWTechniques[0];
        if (nStartTechnique > 0)
        {
            assert(nStartTechnique < (int)m_HWTechniques.Num());
            if (nStartTechnique < (int)m_HWTechniques.Num())
            {
                pTech = m_HWTechniques[nStartTechnique];
            }
            else
            {
                LogWarning("ERROR: CShader::GetTechnique: Technique %d for shader '%s' is out of range", nStartTechnique, GetName());
            }
        }
    }
    else
    {
        pTech = nullptr;
    }

    if (!pTech ||
        pTech->m_nTechnique[nRequestedTechnique] < 0 ||
        pTech->m_nTechnique[nRequestedTechnique] >= (int)m_HWTechniques.Num())
    {
        LogWarning("ERROR: CShader::GetTechnique: No Technique (%d,%d) for shader '%s' ", nStartTechnique, nRequestedTechnique, GetName());
        return nullptr;
    }
    pTech = m_HWTechniques[pTech->m_nTechnique[nRequestedTechnique]];

    return pTech;
}

#if !defined(CONSOLE) && !defined(NULL_RENDERER)
void CShader::mfFlushCache()
{
    uint32 n, m;

    mfFlushPendedShaders();

    if (SEmptyCombination::s_Combinations.size())
    {
        // Flush the cache before storing any empty combinations
        CHWShader::mfFlushPendedShadersWait(-1);
        for (m = 0; m < SEmptyCombination::s_Combinations.size(); m++)
        {
            SEmptyCombination& Comb = SEmptyCombination::s_Combinations[m];
            Comb.pShader->mfStoreEmptyCombination(Comb);
        }
        SEmptyCombination::s_Combinations.clear();
    }

    for (m = 0; m < m_HWTechniques.Num(); m++)
    {
        SShaderTechnique* pTech = m_HWTechniques[m];
        for (n = 0; n < pTech->m_Passes.Num(); n++)
        {
            SShaderPass* pPass = &pTech->m_Passes[n];
            if (pPass->m_PShader)
            {
                pPass->m_PShader->mfFlushCacheFile();
            }
            if (pPass->m_VShader)
            {
                pPass->m_VShader->mfFlushCacheFile();
            }
        }
    }
}
#endif

void CShaderResources::PostLoad(CShader* pSH)
{
    AdjustForSpec();
    if (pSH && (pSH->m_Flags & EF_SKY))
    {
        SEfResTexture*      pTextureRes = GetTextureResource(EFTT_DIFFUSE);
        if (pTextureRes && !pTextureRes->m_Name.empty())
        {
            char    sky[128];
            char    path[1024];

            azstrcpy(sky, AZ_ARRAY_SIZE(sky), pTextureRes->m_Name.c_str());
            int size = strlen(sky);
            const char* ext = fpGetExtension(sky);
            if (size > 0)
            {
                while (sky[size] != '_')   
                {
                    size--;
                    if (!size)
                    {
                        break;
                    }
                }
            }
            sky[size] = 0;
            if (size)
            {
                m_pSky = new SSkyInfo;
                sprintf_s(path, "%s_12%s", sky, ext);
                m_pSky->m_SkyBox[0] = CTexture::ForName(path, 0, eTF_Unknown);
                sprintf_s(path, "%s_34%s", sky, ext);
                m_pSky->m_SkyBox[1] = CTexture::ForName(path, 0, eTF_Unknown);
                sprintf_s(path, "%s_5%s", sky, ext);
                m_pSky->m_SkyBox[2] = CTexture::ForName(path, 0, eTF_Unknown);
            }
        }
    }

    UpdateConstants(pSH);
}

SShaderTexSlots* CShader::GetUsedTextureSlots(int nTechnique)
{
    if IsCVarConstAccess(constexpr) (!CRenderer::CV_r_ReflectTextureSlots)
    {
        return NULL;
    }
    if (nTechnique < 0 || nTechnique >= TTYPE_MAX)
    {
        return NULL;
    }
    return m_ShaderTexSlots[nTechnique];
}

AZStd::vector<SShaderParam>& CShader::GetPublicParams()
{
    SShaderFXParams& FXP = gRenDev->m_cEF.m_Bin.mfGetFXParams(this);
    return FXP.m_PublicParams;
}

CTexture* CShader::mfFindBaseTexture([[maybe_unused]] TArray<SShaderPass>& Passes, [[maybe_unused]] int* nPass, [[maybe_unused]] int* nTU)
{
    return NULL;
}

ITexture* CShader::GetBaseTexture(int* nPass, int* nTU)
{
    for (uint32 i = 0; i < m_HWTechniques.Num(); i++)
    {
        SShaderTechnique* hw = m_HWTechniques[i];
        CTexture* tx = mfFindBaseTexture(hw->m_Passes, nPass, nTU);
        if (tx)
        {
            return tx;
        }
    }
    if (nPass)
    {
        *nPass = -1;
    }
    if (nTU)
    {
        *nTU = -1;
    }
    return NULL;
}

unsigned int CShader::GetUsedTextureTypes (void)
{
    uint32 nMask = 0xffffffff;

    return nMask;
}

//================================================================================

void CShaderMan::mfReleaseShaders ()
{
    CCryNameTSCRC Name = CShader::mfGetClassName();

    AUTO_LOCK(CBaseResource::s_cResLock);

    SResourceContainer* pRL = CBaseResource::GetResourcesForClass(Name);
    if (pRL)
    {
        int n = 0;
        ResourcesMapItor itor;
        for (itor = pRL->m_RMap.begin(); itor != pRL->m_RMap.end(); )
        {
            CShader* sh = (CShader*)itor->second;
            itor++;
            if (!sh)
            {
                continue;
            }
            if IsCVarConstAccess(constexpr) (CRenderer::CV_r_printmemoryleaks && !(sh->m_Flags & EF_SYSTEM))
            {
                iLog->Log("Warning: CShaderMan::mfClearAll: Shader %s was not deleted (%d)", sh->GetName(), sh->GetRefCounter());
            }
            SAFE_RELEASE(sh);
            n++;
        }
    }
}

void CShaderMan::ShutDown(void)
{
    uint32 i;

    m_Bin.InvalidateCache();

    mfReleaseSystemShaders();
    gRenDev->ForceFlushRTCommands();

    if IsCVarConstAccess(constexpr) (CRenderer::CV_r_releaseallresourcesonexit)
    {
        //if (gRenDev->m_cEF.m_bInitialized)
        //  mfReleaseShaders();

        for (i = 0; i < CShader::s_ShaderResources_known.Num(); i++)
        {
            CShaderResources* pSR = CShader::s_ShaderResources_known[i];
            if (!pSR)
            {
                continue;
            }
            if (i)
            {
                if IsCVarConstAccess(constexpr) (CRenderer::CV_r_printmemoryleaks)
                {
                    iLog->Log("Warning: CShaderMan::mfClearAll: Shader resource %p was not deleted", pSR);
                }
            }
            delete pSR;
        }
        CShader::s_ShaderResources_known.Free();
    }

    {
        AZStd::unique_lock<AZStd::mutex> lock(m_shaderLoadMutex);

        m_ShaderNames.clear();

        std::for_each(m_pShadersGlobalFlags.begin(), m_pShadersGlobalFlags.end(), SShaderMapNameFlagsContainerDelete());

        m_pShadersGlobalFlags.clear();
        m_pShaderCommonGlobalFlag.clear();
    }


    CCryNameTSCRC Name;

    SAFE_DELETE(m_pGlobalExt);

    for (i = 0; i < CLightStyle::s_LStyles.Num(); i++)
    {
        delete CLightStyle::s_LStyles[i];
    }
    CLightStyle::s_LStyles.Free();
    m_SGC.clear();
    m_ShaderCacheCombinations[0].clear();
    m_ShaderCacheCombinations[1].clear();
    m_ShaderCacheExportCombinations.clear();
    SAFE_DELETE(m_pGlobalExt);
    mfCloseShadersCache(0);
    mfCloseShadersCache(1);

    m_bInitialized = false;

    Terrain::TerrainShaderRequestBus::Handler::BusDisconnect();
    AZ::MaterialNotificationEventBus::Handler::BusDisconnect();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void CShaderMan:: mfCreateCommonGlobalFlags([[maybe_unused]] const char* szName)
{
    // Create globals.txt

    assert(szName);
    uint32 nCurrMaskCount = 0;
    const char* pszShaderExtPath = "Shaders/";

    char dirn[256];
    cry_strcpy(dirn, pszShaderExtPath);
    cry_strcat(dirn, "*");

    AZ::IO::ArchiveFileIterator handle = gEnv->pCryPak->FindFirst (dirn);
    if (!handle)// failed search
    {
        return;
    }

    do
    {
        // Scan for extension script files - add common flags names into globals list

        if (handle.m_filename.front() == '.' || (handle.m_fileDesc.nAttrib & AZ::IO::FileDesc::Attribute::Subdirectory) == AZ::IO::FileDesc::Attribute::Subdirectory)
        {
            continue;
        }

        const char* pszExt = PathUtil::GetExt(handle.m_filename.data());
        if (azstricmp(pszExt, "ext"))
        {
            continue;
        }


        char pszFileName[256];
        azsnprintf(pszFileName, AZ_ARRAY_SIZE(pszFileName), "%s%.*s", pszShaderExtPath, aznumeric_cast<int>(handle.m_filename.size()), handle.m_filename.data());

        AZ::IO::HandleType fileHandle = AZ::IO::InvalidHandle;
        if (AZ::IO::FileIOBase::GetInstance()->Open(pszFileName, AZ::IO::OpenMode::ModeRead, fileHandle))
        {
            
            AZ::u64 fileSize = 0;
            AZ::u64 bytesRead = 0;
            AZ::IO::FileIOBase::GetInstance()->Size(fileHandle, fileSize);
            char* buf = new char [fileSize + 1];
            AZ::IO::FileIOBase::GetInstance()->Read(fileHandle, buf, fileSize, false, &bytesRead);
            AZ::IO::FileIOBase::GetInstance()->Close(fileHandle);
            buf[bytesRead] = 0;

            // Check if global flags are common
            char* pCurrOffset = strstr(buf, "UsesCommonGlobalFlags");
            if (pCurrOffset)
            {
                AZStd::unique_lock<AZStd::mutex> lock(m_shaderLoadMutex);

                // add shader to list
                string pszShaderName = PathUtil::GetFileName(handle.m_filename.data());
                pszShaderName.MakeUpper();
                m_pShadersRemapList += string("%") + pszShaderName;

                while (pCurrOffset = strstr(pCurrOffset, "Name"))
                {
                    pCurrOffset += 4;
                    char dummy[256] = "\n";
                    char name[256] = "\n";
                    int res = azsscanf(pCurrOffset, "%s %s", dummy
#if AZ_TRAIT_USE_SECURE_CRT_FUNCTIONS
                    , (unsigned int)AZ_ARRAY_SIZE(dummy)
#endif
                    , name
#if AZ_TRAIT_USE_SECURE_CRT_FUNCTIONS
                    , (unsigned int)AZ_ARRAY_SIZE(name)
#endif
                    ); // Get flag name..
                    assert(res);

                    string pszNameFlag = name;
                    int nSzSize = pszNameFlag.size();
                    pszNameFlag.MakeUpper();

                    MapNameFlagsItor pCheck = m_pShaderCommonGlobalFlag.find(pszNameFlag);
                    if (pCheck == m_pShaderCommonGlobalFlag.end())
                    {
                        m_pShaderCommonGlobalFlag.insert(MapNameFlagsItor::value_type(pszNameFlag, 0));
                        if (++nCurrMaskCount >= 64) // sanity check
                        {
                            assert(0);
                            break;
                        }
                    }
                }
            }

            SAFE_DELETE_ARRAY(buf);
        }

        if (nCurrMaskCount >= 64)
        {
            break;
        }
    } while (handle = gEnv->pCryPak->FindNext(handle));

    gEnv->pCryPak->FindClose (handle);

    if (nCurrMaskCount >= 64)
    {
        iLog->Log("ERROR: CShaderMan::mfCreateCommonGlobalFlags: too many common global flags");
    }

    uint64 nCurrGenMask = 0;

    {
        AZStd::unique_lock<AZStd::mutex> lock(m_shaderLoadMutex);

        MapNameFlagsItor pIter = m_pShaderCommonGlobalFlag.begin();
        MapNameFlagsItor pEnd = m_pShaderCommonGlobalFlag.end();
        for (; pIter != pEnd; ++pIter, ++nCurrGenMask)
        {
            // Set mask value
            pIter->second = ((uint64)1) << nCurrGenMask;
        }
    }

    mfRemapCommonGlobalFlagsWithLegacy();
#if !defined (_RELEASE)
    if (nCurrMaskCount > 0)
    {
        mfSaveCommonGlobalFlagsToDisk(szName, nCurrMaskCount);
    }
#endif
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void CShaderMan::mfSaveCommonGlobalFlagsToDisk(const char* szName, [[maybe_unused]] uint32 nMaskCount)
{
    // Write all flags
    assert(nMaskCount);

    AZ::IO::HandleType fileHandle = gEnv->pCryPak->FOpen(szName, "w");
    if (fileHandle != AZ::IO::InvalidHandle)
    {
        gEnv->pCryPak->FPrintf(fileHandle, "FX_CACHE_VER %f\n", FX_CACHE_VER);
        gEnv->pCryPak->FPrintf(fileHandle, "%s\n\n", m_pShadersRemapList.c_str());

        MapNameFlagsItor pIter = m_pShaderCommonGlobalFlag.begin();
        MapNameFlagsItor pEnd = m_pShaderCommonGlobalFlag.end();

        for (; pIter != pEnd; ++pIter)
        {
            gEnv->pCryPak->FPrintf(fileHandle, "%s "
#if defined(__GNUC__)
                "%llx\n"
#else
                "%I64x\n"
#endif
                , (pIter->first).c_str(), pIter->second);
        }

        gEnv->pCryPak->FClose(fileHandle);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void CShaderMan::mfInitCommonGlobalFlagsLegacyFix(void)
{
    // Store original values since common material flags where introduced
    m_pSCGFlagLegacyFix.insert(MapNameFlagsItor::value_type("%ALPHAGLOW", (uint64)0x2));
    m_pSCGFlagLegacyFix.insert(MapNameFlagsItor::value_type("%ALPHAMASK_DETAILMAP", (uint64)0x4));
    m_pSCGFlagLegacyFix.insert(MapNameFlagsItor::value_type("%ANISO_SPECULAR", (uint64)0x8));

    // 0x10 is unused
    m_pSCGFlagLegacyFix.insert(MapNameFlagsItor::value_type("%BUMP_DIFFUSE", (uint64)0x20));
    m_pSCGFlagLegacyFix.insert(MapNameFlagsItor::value_type("%CHARACTER_DECAL", (uint64)0x40));

    m_pSCGFlagLegacyFix.insert(MapNameFlagsItor::value_type("%CUSTOM_SPECULAR", (uint64)0x400));
    m_pSCGFlagLegacyFix.insert(MapNameFlagsItor::value_type("%DECAL", (uint64)0x800));

    m_pSCGFlagLegacyFix.insert(MapNameFlagsItor::value_type("%DETAIL_BENDING", (uint64)0x1000));
    m_pSCGFlagLegacyFix.insert(MapNameFlagsItor::value_type("%DETAIL_BUMP_MAPPING", (uint64)0x2000));
    m_pSCGFlagLegacyFix.insert(MapNameFlagsItor::value_type("%DISABLE_RAIN_PASS", (uint64)0x4000));

    m_pSCGFlagLegacyFix.insert(MapNameFlagsItor::value_type("%ENVIRONMENT_MAP", (uint64)0x10000));
    m_pSCGFlagLegacyFix.insert(MapNameFlagsItor::value_type("%EYE_OVERLAY", (uint64)0x20000));
    m_pSCGFlagLegacyFix.insert(MapNameFlagsItor::value_type("%GLOSS_DIFFUSEALPHA", (uint64)0x40000));
    m_pSCGFlagLegacyFix.insert(MapNameFlagsItor::value_type("%GLOSS_MAP", (uint64)0x80000));

    m_pSCGFlagLegacyFix.insert(MapNameFlagsItor::value_type("%GRADIENT_COLORING", (uint64)0x100000));
    m_pSCGFlagLegacyFix.insert(MapNameFlagsItor::value_type("%GRASS", (uint64)0x200000));
    m_pSCGFlagLegacyFix.insert(MapNameFlagsItor::value_type("%IRIS", (uint64)0x400000));
    m_pSCGFlagLegacyFix.insert(MapNameFlagsItor::value_type("%LEAVES", (uint64)0x800000));

    m_pSCGFlagLegacyFix.insert(MapNameFlagsItor::value_type("%NANOSUIT_EFFECTS", (uint64)0x1000000));
    m_pSCGFlagLegacyFix.insert(MapNameFlagsItor::value_type("%OFFSET_BUMP_MAPPING", (uint64)0x2000000));
    m_pSCGFlagLegacyFix.insert(MapNameFlagsItor::value_type("%PARALLAX_OCCLUSION_MAPPING", (uint64)0x8000000));

    m_pSCGFlagLegacyFix.insert(MapNameFlagsItor::value_type("%REALTIME_MIRROR_REFLECTION", (uint64)0x10000000));
    m_pSCGFlagLegacyFix.insert(MapNameFlagsItor::value_type("%REFRACTION_MAP", (uint64)0x20000000));
    m_pSCGFlagLegacyFix.insert(MapNameFlagsItor::value_type("%RIM_LIGHTING", (uint64)0x40000000));
    m_pSCGFlagLegacyFix.insert(MapNameFlagsItor::value_type("%SPECULARPOW_GLOSSALPHA", (uint64)0x80000000));

    m_pSCGFlagLegacyFix.insert(MapNameFlagsItor::value_type("%TEMP_TERRAIN", (uint64)0x200000000ULL));
    m_pSCGFlagLegacyFix.insert(MapNameFlagsItor::value_type("%TEMP_VEGETATION", (uint64)0x400000000ULL));
    m_pSCGFlagLegacyFix.insert(MapNameFlagsItor::value_type("%TERRAINHEIGHTADAPTION", (uint64)0x800000000ULL));

    m_pSCGFlagLegacyFix.insert(MapNameFlagsItor::value_type("%TWO_SIDED_SORTING", (uint64)0x1000000000ULL));
    m_pSCGFlagLegacyFix.insert(MapNameFlagsItor::value_type("%VERTCOLORS", (uint64)0x2000000000ULL));
    m_pSCGFlagLegacyFix.insert(MapNameFlagsItor::value_type("%WIND_BENDING", (uint64)0x4000000000ULL));
    m_pSCGFlagLegacyFix.insert(MapNameFlagsItor::value_type("%WRINKLE_BLENDING", (uint64)0x8000000000ULL));
}

////////////////////////////////////////////////////////////////////////////////////////////////////

bool CShaderMan::mfRemapCommonGlobalFlagsWithLegacy(void)
{
    MapNameFlagsItor pFixIter = m_pSCGFlagLegacyFix.begin();
    MapNameFlagsItor pFixEnd = m_pSCGFlagLegacyFix.end();

    AZStd::unique_lock<AZStd::mutex> lock(m_shaderLoadMutex);

    MapNameFlagsItor pCommonGlobalsEnd = m_pShaderCommonGlobalFlag.end();

    bool bRemaped = false;
    for (; pFixIter != pFixEnd; ++pFixIter)
    {
        MapNameFlagsItor pFoundMatch = m_pShaderCommonGlobalFlag.find(pFixIter->first);
        if (pFoundMatch != pCommonGlobalsEnd)
        {
            // Found a match, replace value
            uint64 nRemapedMask = pFixIter->second;
            uint64 nOldMask = pFoundMatch->second;
            pFoundMatch->second = nRemapedMask;

            // Search for duplicates and swap with old mask
            MapNameFlagsItor pCommonGlobalsIter = m_pShaderCommonGlobalFlag.begin();
            uint64 test = (uint64)0x10;
            for (; pCommonGlobalsIter != pCommonGlobalsEnd; ++pCommonGlobalsIter)
            {
                if (pFoundMatch != pCommonGlobalsIter && pCommonGlobalsIter->second == nRemapedMask)
                {
                    uint64 nPrev = pCommonGlobalsIter->second;
                    pCommonGlobalsIter->second = nOldMask;
                    bRemaped = true;
                    break;
                }
            }
        }
    }

    // Create existing flags mask
    MapNameFlagsItor pIter = m_pShaderCommonGlobalFlag.begin();
    MapNameFlagsItor pEnd = m_pShaderCommonGlobalFlag.end();
    m_nSGFlagsFix = 0;
    for (; pIter != pEnd; ++pIter)
    {
        m_nSGFlagsFix |= (pIter->second);
    }

    return bRemaped;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void CShaderMan::mfInitCommonGlobalFlags(void)
{
    mfInitCommonGlobalFlagsLegacyFix();

    AZStd::string pszGlobalsPath = m_szCachePath + CONCAT_PATHS(g_shaderCache,"globals.txt");
    AZ::IO::HandleType fileHandle = gEnv->pCryPak->FOpen(pszGlobalsPath.c_str(), "r", AZ::IO::IArchive::FOPEN_HINT_QUIET);
    if (fileHandle != AZ::IO::InvalidHandle)
    {
        char str[256] = "\n";
        char name[128] = "\n";

        gEnv->pCryPak->FGets(str, 256, fileHandle);
        if (strstr(str, "FX_CACHE_VER"))
        {
            float fCacheVer = 0.0f;
            int res = azsscanf(str, "%s %f",
            name,
#if AZ_TRAIT_USE_SECURE_CRT_FUNCTIONS
            (unsigned int)AZ_ARRAY_SIZE(name),
#endif
            &fCacheVer);
            assert(res);
            if (!azstricmp(name, "FX_CACHE_VER") && fabs(FX_CACHE_VER - fCacheVer) >= 0.01f)
            {
                gEnv->pCryPak->FClose(fileHandle);
                // re-create common global flags (shader cache bumped)
                mfCreateCommonGlobalFlags(pszGlobalsPath.c_str());

                return;
            }
        }

        // get shader that need remapping slist
        gEnv->pCryPak->FGets(str, 256, fileHandle);

        uint32 nCurrMaskCount = 0;
        {
            AZStd::unique_lock<AZStd::mutex> lock(m_shaderLoadMutex);

            m_pShadersRemapList = str;

            while (!gEnv->pCryPak->FEof(fileHandle))
            {
                uint64 nGenMask = 0;
                gEnv->pCryPak->FGets(str, 256, fileHandle);

                if (azsscanf(str, "%s "
#if defined(__GNUC__)
                    "%llx"
#else
                    "%I64x"
#endif
                    , name,
#if AZ_TRAIT_USE_SECURE_CRT_FUNCTIONS
                    (unsigned int)AZ_ARRAY_SIZE(name),
#endif
                    &nGenMask) > 1)
                {
                    m_pShaderCommonGlobalFlag.insert(MapNameFlagsItor::value_type(name, nGenMask));
                    nCurrMaskCount++;
                }
            }
        }

        gEnv->pCryPak->FClose(fileHandle);

        if (mfRemapCommonGlobalFlagsWithLegacy())
        {
            mfSaveCommonGlobalFlagsToDisk(pszGlobalsPath.c_str(), nCurrMaskCount);
        }

        return;
    }

    // create common global flags - not existing globals.txt
    mfCreateCommonGlobalFlags(pszGlobalsPath.c_str());
}

void CShaderMan::mfInitLookups()
{
    m_ResLookupDataMan[CACHE_READONLY].Clear();
    AZStd::string dirdatafilename(m_ShadersCache);
    dirdatafilename += "lookupdata.bin";
    m_ResLookupDataMan[CACHE_READONLY].LoadData(dirdatafilename.c_str(), CParserBin::m_bEndians, true);

    m_ResLookupDataMan[CACHE_USER].Clear();
    dirdatafilename = m_szCachePath + m_ShadersCache;
    dirdatafilename += "lookupdata.bin";
    m_ResLookupDataMan[CACHE_USER].LoadData(dirdatafilename.c_str(), CParserBin::m_bEndians, false);
}
void CShaderMan::mfInitLevelPolicies(void)
{
    SAFE_DELETE(m_pLevelsPolicies);

    SShaderLevelPolicies* pPL = NULL;
    char szN[256];
    cry_strcpy(szN, "Shaders/");
    cry_strcat(szN, "Levels.txt");
    AZ::IO::HandleType fileHandle = gEnv->pCryPak->FOpen(szN, "rb", AZ::IO::IArchive::FOPEN_HINT_QUIET);
    if (fileHandle != AZ::IO::InvalidHandle)
    {
        pPL = new SShaderLevelPolicies;
        AZ::u64 fileSize = 0;
        AZ::IO::FileIOBase::GetInstance()->Size(fileHandle, fileSize);
        char* buf = new char [fileSize + 1];
        if (buf)
        {
            buf[fileSize] = 0;
            gEnv->pCryPak->FRead(buf, fileSize, fileHandle);
            mfCompileShaderLevelPolicies(pPL, buf);
            delete [] buf;
        }
        gEnv->pCryPak->FClose(fileHandle);
    }
    m_pLevelsPolicies = pPL;
}

void CShaderMan::mfInitGlobal (void)
{
    SAFE_DELETE(m_pGlobalExt);
    SShaderGen* pShGen = mfCreateShaderGenInfo("RunTime", true);

#if defined(_RELEASE)
    AZ_Assert(pShGen, "Fatal error: could not find required shader 'RunTime'.  This is typically placed in @assets@/shaders.pak for release builds.  Make sure BuildReleaseAuxiliaryContent.py has been run and all shaders have been included in the release packaging build.");
#endif
    
    m_pGlobalExt = pShGen;
    if (pShGen)
    {
        uint32 i;

        for (i = 0; i < pShGen->m_BitMask.Num(); i++)
        {
            SShaderGenBit* gb = pShGen->m_BitMask[i];
            if (!gb)
            {
                continue;
            }
            if (gb->m_ParamName == "%_RT_FOG")
            {
                g_HWSR_MaskBit[HWSR_FOG] = gb->m_Mask;
            }
            else
            if (gb->m_ParamName == "%_RT_AMBIENT")
            {
                g_HWSR_MaskBit[HWSR_AMBIENT] = gb->m_Mask;
            }
            else
            if (gb->m_ParamName == "%_RT_HDR_ENCODE")
            {
                g_HWSR_MaskBit[HWSR_HDR_ENCODE] = gb->m_Mask;
            }
            else
            if (gb->m_ParamName == "%_RT_ALPHATEST")
            {
                g_HWSR_MaskBit[HWSR_ALPHATEST] = gb->m_Mask;
            }
            else
            if (gb->m_ParamName == "%_RT_HDR_MODE")
            {
                g_HWSR_MaskBit[HWSR_HDR_MODE] = gb->m_Mask;
            }
            else
            if (gb->m_ParamName == "%_RT_NEAREST")
            {
                g_HWSR_MaskBit[HWSR_NEAREST] = gb->m_Mask;
            }
            else
            if (gb->m_ParamName == "%_RT_SHADOW_MIXED_MAP_G16R16")
            {
                g_HWSR_MaskBit[HWSR_SHADOW_MIXED_MAP_G16R16] = gb->m_Mask;
            }
            else
            if (gb->m_ParamName == "%_RT_HW_PCF_COMPARE")
            {
                g_HWSR_MaskBit[HWSR_HW_PCF_COMPARE] = gb->m_Mask;
            }
            else
            if (gb->m_ParamName == "%_RT_SAMPLE0")
            {
                g_HWSR_MaskBit[HWSR_SAMPLE0] = gb->m_Mask;
            }
            else
            if (gb->m_ParamName == "%_RT_SAMPLE1")
            {
                g_HWSR_MaskBit[HWSR_SAMPLE1] = gb->m_Mask;
            }
            else
            if (gb->m_ParamName == "%_RT_SAMPLE2")
            {
                g_HWSR_MaskBit[HWSR_SAMPLE2] = gb->m_Mask;
            }
            else
            if (gb->m_ParamName == "%_RT_SAMPLE3")
            {
                g_HWSR_MaskBit[HWSR_SAMPLE3] = gb->m_Mask;
            }
            else
            if (gb->m_ParamName == "%_RT_ALPHABLEND")
            {
                g_HWSR_MaskBit[HWSR_ALPHABLEND] = gb->m_Mask;
            }
            else
            if (gb->m_ParamName == "%_RT_QUALITY")
            {
                g_HWSR_MaskBit[HWSR_QUALITY] = gb->m_Mask;
            }
            else
            if (gb->m_ParamName == "%_RT_QUALITY1")
            {
                g_HWSR_MaskBit[HWSR_QUALITY1] = gb->m_Mask;
            }
            else
            if (gb->m_ParamName == "%_RT_INSTANCING_ATTR")
            {
                g_HWSR_MaskBit[HWSR_INSTANCING_ATTR] = gb->m_Mask;
            }
            else
            if (gb->m_ParamName == "%_RT_NOZPASS")
            {
                g_HWSR_MaskBit[HWSR_NOZPASS] = gb->m_Mask;
            }
            else
            if (gb->m_ParamName == "%_RT_NO_TESSELLATION")
            {
                g_HWSR_MaskBit[HWSR_NO_TESSELLATION] = gb->m_Mask;
            }
            else
            if (gb->m_ParamName == "%_RT_APPLY_TOON_SHADING")
            {
                g_HWSR_MaskBit[HWSR_APPLY_TOON_SHADING] = gb->m_Mask;
            }
            else
            if (gb->m_ParamName == "%_RT_VERTEX_VELOCITY")
            {
                g_HWSR_MaskBit[HWSR_VERTEX_VELOCITY] = gb->m_Mask;
            }
            else
            if (gb->m_ParamName == "%_RT_OBJ_IDENTITY")
            {
                g_HWSR_MaskBit[HWSR_OBJ_IDENTITY] = gb->m_Mask;
            }
            else
            if (gb->m_ParamName == "%_RT_SKINNING_DUAL_QUAT")
            {
                g_HWSR_MaskBit[HWSR_SKINNING_DUAL_QUAT] = gb->m_Mask;
            }
            else
            if (gb->m_ParamName == "%_RT_SKINNING_DQ_LINEAR")
            {
                g_HWSR_MaskBit[HWSR_SKINNING_DQ_LINEAR] = gb->m_Mask;
            }
            else
            if (gb->m_ParamName == "%_RT_SKINNING_MATRIX")
            {
                g_HWSR_MaskBit[HWSR_SKINNING_MATRIX] = gb->m_Mask;
            }
            else
            if (gb->m_ParamName == "%_RT_DISSOLVE")
            {
                g_HWSR_MaskBit[HWSR_DISSOLVE] = gb->m_Mask;
            }
            else
            if (gb->m_ParamName == "%_RT_SOFT_PARTICLE")
            {
                g_HWSR_MaskBit[HWSR_SOFT_PARTICLE] = gb->m_Mask;
            }
            else
            if (gb->m_ParamName == "%_RT_LIGHT_TEX_PROJ")
            {
                g_HWSR_MaskBit[HWSR_LIGHT_TEX_PROJ] = gb->m_Mask;
            }
            else
            if (gb->m_ParamName == "%_RT_SHADOW_JITTERING")
            {
                g_HWSR_MaskBit[HWSR_SHADOW_JITTERING] = gb->m_Mask;
            }
            else
            if (gb->m_ParamName == "%_RT_PARTICLE_SHADOW")
            {
                g_HWSR_MaskBit[HWSR_PARTICLE_SHADOW] = gb->m_Mask;
            }
            else
            if (gb->m_ParamName == "%_RT_BLEND_WITH_TERRAIN_COLOR")
            {
                // Just leaving this here for backwards compatibility - nothing to do
            }
            else
            if (gb->m_ParamName == "%_RT_SPRITE")
            {
                g_HWSR_MaskBit[HWSR_SPRITE] = gb->m_Mask;
            }
            else
            if (gb->m_ParamName == "%_RT_DEBUG0")
            {
                g_HWSR_MaskBit[HWSR_DEBUG0] = gb->m_Mask;
            }
            else
            if (gb->m_ParamName == "%_RT_DEBUG1")
            {
                g_HWSR_MaskBit[HWSR_DEBUG1] = gb->m_Mask;
            }
            else
            if (gb->m_ParamName == "%_RT_DEBUG2")
            {
                g_HWSR_MaskBit[HWSR_DEBUG2] = gb->m_Mask;
            }
            else
            if (gb->m_ParamName == "%_RT_DEBUG3")
            {
                g_HWSR_MaskBit[HWSR_DEBUG3] = gb->m_Mask;
            }
            else
            if (gb->m_ParamName == "%_RT_POINT_LIGHT")
            {
                g_HWSR_MaskBit[HWSR_POINT_LIGHT] = gb->m_Mask;
            }
            else
            if (gb->m_ParamName == "%_RT_CUBEMAP0")
            {
                g_HWSR_MaskBit[HWSR_CUBEMAP0] = gb->m_Mask;
            }
            else
            if (gb->m_ParamName == "%_RT_DECAL_TEXGEN_2D")
            {
                g_HWSR_MaskBit[HWSR_DECAL_TEXGEN_2D] = gb->m_Mask;
            }
            else
            if (gb->m_ParamName == "%_RT_OCEAN_PARTICLE")
            {
                g_HWSR_MaskBit[HWSR_OCEAN_PARTICLE] = gb->m_Mask;
            }
            else
            if (gb->m_ParamName == "%_RT_SAMPLE4")
            {
                g_HWSR_MaskBit[HWSR_SAMPLE4] = gb->m_Mask;
            }
            else
            if (gb->m_ParamName == "%_RT_SAMPLE5")
            {
                g_HWSR_MaskBit[HWSR_SAMPLE5] = gb->m_Mask;
            }
            else
            if (gb->m_ParamName == "%_RT_FOG_VOLUME_HIGH_QUALITY_SHADER")
            {
                g_HWSR_MaskBit[HWSR_FOG_VOLUME_HIGH_QUALITY_SHADER] = gb->m_Mask;
            }
            else
            if (gb->m_ParamName == "%_RT_APPLY_SSDO")
            {
                g_HWSR_MaskBit[HWSR_APPLY_SSDO] = gb->m_Mask;
            }
            else
            if (gb->m_ParamName == "%_RT_GLOBAL_ILLUMINATION")
            {
                g_HWSR_MaskBit[HWSR_GLOBAL_ILLUMINATION] = gb->m_Mask;
            }
            else
            if (gb->m_ParamName == "%_RT_ANIM_BLEND")
            {
                g_HWSR_MaskBit[HWSR_ANIM_BLEND] = gb->m_Mask;
            }
            else
            if (gb->m_ParamName == "%_RT_MOTION_BLUR")
            {
                g_HWSR_MaskBit[HWSR_MOTION_BLUR] = gb->m_Mask;
            }
            else
            if (gb->m_ParamName == "%_RT_ENVIRONMENT_CUBEMAP")
            {
                g_HWSR_MaskBit[HWSR_ENVIRONMENT_CUBEMAP] = gb->m_Mask;
            }
            else
            if (gb->m_ParamName == "%_RT_LIGHTVOLUME0")
            {
                g_HWSR_MaskBit[HWSR_LIGHTVOLUME0] = gb->m_Mask;
            }
            else
            if (gb->m_ParamName == "%_RT_LIGHTVOLUME1")
            {
                g_HWSR_MaskBit[HWSR_LIGHTVOLUME1] = gb->m_Mask;
            }
            else
            if (gb->m_ParamName == "%_RT_TILED_SHADING")
            {
                g_HWSR_MaskBit[HWSR_TILED_SHADING] = gb->m_Mask;
            }
            else
            if (gb->m_ParamName == "%_RT_VOLUMETRIC_FOG")
            {
                g_HWSR_MaskBit[HWSR_VOLUMETRIC_FOG] = gb->m_Mask;
            }
            else
            if (gb->m_ParamName == "%_RT_REVERSE_DEPTH")
            {
                g_HWSR_MaskBit[HWSR_REVERSE_DEPTH] = gb->m_Mask;
            }
            else
            if (gb->m_ParamName == "%_RT_GPU_PARTICLE_SHADOW_PASS")
            {
                g_HWSR_MaskBit[HWSR_GPU_PARTICLE_SHADOW_PASS] = gb->m_Mask;
            }
            else
            if (gb->m_ParamName == "%_RT_GPU_PARTICLE_DEPTH_COLLISION")
            {
                g_HWSR_MaskBit[HWSR_GPU_PARTICLE_DEPTH_COLLISION] = gb->m_Mask;
            }
            else
            if (gb->m_ParamName == "%_RT_GPU_PARTICLE_TURBULENCE")
            {
                g_HWSR_MaskBit[HWSR_GPU_PARTICLE_TURBULENCE] = gb->m_Mask;
            }
            else
            if (gb->m_ParamName == "%_RT_GPU_PARTICLE_UV_ANIMATION")
            {
                g_HWSR_MaskBit[HWSR_GPU_PARTICLE_UV_ANIMATION] = gb->m_Mask;
            }
            else if (gb->m_ParamName == "%_RT_GPU_PARTICLE_NORMAL_MAP")
            {
                g_HWSR_MaskBit[HWSR_GPU_PARTICLE_NORMAL_MAP] = gb->m_Mask;
            }
            else if (gb->m_ParamName == "%_RT_GPU_PARTICLE_GLOW_MAP")
            {
                g_HWSR_MaskBit[HWSR_GPU_PARTICLE_GLOW_MAP] = gb->m_Mask;
            }
            else if (gb->m_ParamName == "%_RT_GPU_PARTICLE_CUBEMAP_DEPTH_COLLISION")
            {
                g_HWSR_MaskBit[HWSR_GPU_PARTICLE_CUBEMAP_DEPTH_COLLISION] = gb->m_Mask;
            }
            else if (gb->m_ParamName == "%_RT_GPU_PARTICLE_WRITEBACK_DEATH_LOCATIONS")
            {
                g_HWSR_MaskBit[HWSR_GPU_PARTICLE_WRITEBACK_DEATH_LOCATIONS] = gb->m_Mask;
            }
            else if (gb->m_ParamName == "%_RT_GPU_PARTICLE_TARGET_ATTRACTION")
            {
                g_HWSR_MaskBit[HWSR_GPU_PARTICLE_TARGET_ATTRACTION] = gb->m_Mask;
            }
            else if (gb->m_ParamName == "%_RT_GPU_PARTICLE_SHAPE_ANGLE")
            {
                g_HWSR_MaskBit[HWSR_GPU_PARTICLE_SHAPE_ANGLE] = gb->m_Mask;
            }
            else if (gb->m_ParamName == "%_RT_GPU_PARTICLE_SHAPE_BOX")
            {
                g_HWSR_MaskBit[HWSR_GPU_PARTICLE_SHAPE_BOX] = gb->m_Mask;
            }
            else if (gb->m_ParamName == "%_RT_GPU_PARTICLE_SHAPE_POINT")
            {
                g_HWSR_MaskBit[HWSR_GPU_PARTICLE_SHAPE_POINT] = gb->m_Mask;
            }
            else if (gb->m_ParamName == "%_RT_GPU_PARTICLE_SHAPE_CIRCLE")
            {
                g_HWSR_MaskBit[HWSR_GPU_PARTICLE_SHAPE_CIRCLE] = gb->m_Mask;
            }
            else if (gb->m_ParamName == "%_RT_GPU_PARTICLE_SHAPE_SPHERE")
            {
                g_HWSR_MaskBit[HWSR_GPU_PARTICLE_SHAPE_SPHERE] = gb->m_Mask;
            }
            else if (gb->m_ParamName == "%_RT_GPU_PARTICLE_WIND")
            {
                g_HWSR_MaskBit[HWSR_GPU_PARTICLE_WIND] = gb->m_Mask;
            }
            else if (gb->m_ParamName == "%_RT_MULTI_LAYER_ALPHA_BLEND")
            {
                g_HWSR_MaskBit[HWSR_MULTI_LAYER_ALPHA_BLEND] = gb->m_Mask;
            }
            else if (gb->m_ParamName == "%_RT_ADDITIVE_BLENDING")
            {
                g_HWSR_MaskBit[HWSR_ADDITIVE_BLENDING] = gb->m_Mask;
            }
            else
            if (gb->m_ParamName == "%_RT_SRGB0")
            {
                g_HWSR_MaskBit[HWSR_SRGB0] = gb->m_Mask;
            }
            else
            if (gb->m_ParamName == "%_RT_SRGB1")
            {
                g_HWSR_MaskBit[HWSR_SRGB1] = gb->m_Mask;
            }
            else
            if (gb->m_ParamName == "%_RT_SRGB2")
            {
                g_HWSR_MaskBit[HWSR_SRGB2] = gb->m_Mask;
            }
            else
            if (gb->m_ParamName == "%_RT_SLIM_GBUFFER")
            {
                g_HWSR_MaskBit[HWSR_SLIM_GBUFFER] = gb->m_Mask;
            }
            else
            if (gb->m_ParamName == "%_RT_DEFERRED_RENDER_TARGET_OPTIMIZATION")
            {
                g_HWSR_MaskBit[HWSR_DEFERRED_RENDER_TARGET_OPTIMIZATION] = gb->m_Mask;
            }
            else
            if (gb->m_ParamName == "%_RT_DEPTHFIXUP")
            {
                g_HWSR_MaskBit[HWSR_DEPTHFIXUP] = gb->m_Mask;
            }
            else
            {
                AZ_Assert(false, "Invalid shader param %s", gb->m_ParamName.c_str());
            }
        }
    }
}

void CShaderMan::InitStaticFlags()
{
    SAFE_DELETE(m_staticExt);
    m_staticExt = mfCreateShaderGenInfo("Statics", true);
    if (m_staticExt)
    {
        AZ_Assert(m_staticExt->m_BitMask.Num() == AZ_ARRAY_SIZE(g_HWSST_Flags), "Mismatch static flags count. Expected %u flags but got %u instead", AZ_ARRAY_SIZE(g_HWSST_Flags), m_staticExt->m_BitMask.Num());
        for (unsigned i = 0; i < m_staticExt->m_BitMask.Num(); ++i)
        {
            SShaderGenBit* gb = m_staticExt->m_BitMask[i];
            if (!gb)
            {
                continue;
            }

            auto found = AZStd::find_if(std::begin(g_HWSST_Flags), std::end(g_HWSST_Flags), [&gb](const AZStd::pair<const char*, uint64>& element) { return azstricmp(element.first, gb->m_ParamName) == 0; });
            if (found != std::end(g_HWSST_Flags))
            {
                found->second = gb->m_Mask;
            }
            else
            {
                AZ_Error("Renderer", false, "Invalid static flag param %s", gb->m_ParamName.c_str());
            }
        }
    }
}

void CShaderMan::AddStaticFlag(EHWSSTFlag flag)
{
    if (!m_staticExt)
    {
        InitStaticFlags();
    }

    AZ_Assert(static_cast<int>(flag) >= 0 && static_cast<int>(flag) < AZ_ARRAY_SIZE(g_HWSST_Flags), "Invalid static flag %d", static_cast<int>(flag));
    m_staticFlags |= g_HWSST_Flags[flag].second;
}

void CShaderMan::RemoveStaticFlag(EHWSSTFlag flag)
{
    if (!m_staticExt)
    {
        InitStaticFlags();
    }

    AZ_Assert(static_cast<int>(flag) >= 0 && static_cast<int>(flag) < AZ_ARRAY_SIZE(g_HWSST_Flags), "Invalid static flag %d", static_cast<int>(flag));
    m_staticFlags &= ~g_HWSST_Flags[flag].second;
}

bool CShaderMan::HasStaticFlag(EHWSSTFlag flag)
{
    if (!m_staticExt)
    {
        InitStaticFlags();
    }

    AZ_Assert(static_cast<int>(flag) >= 0 && static_cast<int>(flag) < AZ_ARRAY_SIZE(g_HWSST_Flags), "Invalid static flag %d", static_cast<int>(flag));
    return (m_staticFlags & g_HWSST_Flags[flag].second) != 0;
}

void CShaderMan::mfInit (void)
{
    LOADING_TIME_PROFILE_SECTION;
    s_cNameHEAD = CCryNameTSCRC("HEAD");

    CTexture::Init();

    if (!m_bInitialized)
    {
        GetISystem()->GetISystemEventDispatcher()->RegisterListener(this);

        m_ShadersPath = "Shaders/HWScripts/";
        m_ShadersMergeCachePath = "Shaders/MergeCache/";
#if defined(LINUX32)
        m_ShadersCache = CONCAT_PATHS(g_shaderCache, "LINUX32/");
#elif defined(LINUX64)
        m_ShadersCache = CONCAT_PATHS(g_shaderCache, "LINUX64/");
#elif defined(MAC)
        m_ShadersCache = CONCAT_PATHS(g_shaderCache, "Mac/");
#elif defined(IOS)
        m_ShadersCache = CONCAT_PATHS(g_shaderCache, "iOS/");
#else
        m_ShadersCache = CONCAT_PATHS(g_shaderCache, "D3D11");
#endif
        m_szCachePath = "@cache@/";
        
        if (CRenderer::CV_r_shadersImport == 3)
        {
#if defined(PERFORMANCE_BUILD) || defined(_RELEASE)
            // Disable all runtime shader compilation and force use of the shader importing system in Performance and Release builds only
            // We want to still build shaders in Profile builds so we do not miss generating new permutations
            CRenderer::CV_r_shadersAllowCompilation = 0;
#else
            // Disable shader importing and allow r_shadersAllowCompilation and r_shadersremotecompiler to be used to compile shaders at runtime 
            CRenderer::CV_r_shadersImport = 0;
#endif
        }

#ifndef CONSOLE_CONST_CVAR_MODE
        if (CRenderer::CV_r_shadersediting)
        {
            CRenderer::CV_r_shadersAllowCompilation = 1; // allow compilation
            CRenderer::CV_r_shaderslogcachemisses = 0; // don't bother about cache misses
            CRenderer::CV_r_shaderspreactivate = 0; // don't load the level caches
            CParserBin::m_bEditable = true;
            CRenderer::CV_r_shadersImport = 0; // don't allow shader importing

            ICVar* pPakPriortyCVar = gEnv->pConsole->GetCVar("sys_PakPriority");
            if (pPakPriortyCVar)
            {
                pPakPriortyCVar->Set(0); // shaders are loaded from disc, always
            }
            ICVar* pInvalidFileAccessCVar = gEnv->pConsole->GetCVar("sys_PakLogInvalidFileAccess");
            if (pInvalidFileAccessCVar)
            {
                pInvalidFileAccessCVar->Set(0); // don't bother logging invalid access when editing shaders
            }
        }
#endif
        if (CRenderer::CV_r_shadersAllowCompilation)
        {
            CRenderer::CV_r_shadersasyncactivation = 0;
            
            // don't allow shader importing when shader compilation is enabled.
            AZ_Warning("Rendering", CRenderer::CV_r_shadersImport == 0, "Warning: Shader compilation is enabled, but shader importing was requested.  Disabling r_shadersImport.");
            CRenderer::CV_r_shadersImport = 0;
        }

        // make sure correct paks are open - shaders.pak will be unloaded from memory after init
        gEnv->pCryPak->OpenPack("@assets@", "Shaders.pak", AZ::IO::IArchive::FLAGS_PAK_IN_MEMORY);
#if defined(AZ_PLATFORM_ANDROID)
        // When the ShaderCache.pak is inside the APK the initialization process takes for ever (around 4 minutes to initialize the engine).
        // Loading it into memory during the initialization seems to bypass the issue.
        // All paks are unloaded from memory after the game init (during the ESYSTEM_EVENT_GAME_POST_INIT_DONE event).
        // LY-40729 is tracking the problem. Once that's fixed this need to be removed.
        gEnv->pCryPak->OpenPack("@assets@", "shaderCache.pak", AZ::IO::IArchive::FLAGS_PAK_IN_MEMORY);
#endif
        ParseShaderProfiles();

        fxParserInit();
        CParserBin::Init();
        CResFile::Tick();

        //CShader::m_Shaders_known.Alloc(MAX_SHADERS);
        //memset(&CShader::m_Shaders_known[0], 0, sizeof(CShader *)*MAX_SHADERS);

        mfInitGlobal();
        InitStaticFlags();
        mfInitLevelPolicies();

        //mfInitLookups();

        // Generate/or load globals.txt - if not existing or shader cache version bumped
        mfInitCommonGlobalFlags();

        mfPreloadShaderExts();

        if (CRenderer::CV_r_shadersAllowCompilation && !gRenDev->IsShaderCacheGenMode())
        {
            mfInitShadersList(&m_ShaderNames);
        }

        mfInitShadersCacheMissLog();

#if !defined(NULL_RENDERER)
        if (!gRenDev->IsEditorMode() && !gRenDev->IsShaderCacheGenMode())
        {
            const char* shaderPakDir = "@assets@";
            const char* shaderPakPath = "shaderCache.pak";

#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION SHADERCORE_CPP_SECTION_1
    #include AZ_RESTRICTED_FILE(ShaderCore_cpp)
#endif

            if (CRenderer::CV_r_shaderspreactivate == 3)
            {
                gEnv->pCryPak->LoadPakToMemory(shaderPakPath, AZ::IO::IArchive::eInMemoryPakLocale_CPU);

                mfPreactivateShaders2("", "shaders/cache/", true, shaderPakDir);

                gEnv->pCryPak->LoadPakToMemory(shaderPakPath, AZ::IO::IArchive::eInMemoryPakLocale_Unload);
            }
            else if (CRenderer::CV_r_shaderspreactivate)
            {
                mfPreactivateShaders2("", "shadercache/", true, shaderPakDir);
            }
        }

#endif

        if (CRenderer::CV_r_shadersAllowCompilation)
        {
            mfInitShadersCache(false, NULL, NULL, 0);
            if (CRenderer::CV_r_shaderspreactivate == 2)
            {
                mfInitShadersCache(false, NULL, NULL, 1);
            }
        }

#if !defined(CONSOLE) && !defined(NULL_RENDERER)
        if (!CRenderer::CV_r_shadersAllowCompilation)
        {
            AZStd::string cgpShaders = m_ShadersCache + "cgpshaders";
            AZStd::string cgvShaders = m_ShadersCache + "cgvshaders";
            
            // make sure we can write to the shader cache
            if (!CheckAllFilesAreWritable(cgpShaders.c_str()) ||
                !CheckAllFilesAreWritable(cgvShaders.c_str()))
            {
                // message box causes problems in fullscreen
                //          MessageBox(0,"WARNING: Shader cache cannot be updated\n\n"
                //              "files are write protected / media is read only / windows user setting don't allow file changes","CryEngine",MB_ICONEXCLAMATION|MB_OK);
                gEnv->pLog->LogError("ERROR: Shader cache cannot be updated (files are write protected / media is read only / windows user setting don't allow file changes)");
            }
        }
#endif  // WIN

        mfSetDefaults();

        //mfPrecacheShaders(NULL);

        // flash all the current commands (parse default shaders)
        gRenDev->m_pRT->FlushAndWait();

        m_bInitialized = true;
    }
    //mfPrecacheShaders();
}

bool CShaderMan::LoadShaderStartupCache()
{
    const char* shaderPakDir = "@assets@/ShaderCacheStartup.pak";

#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION SHADERCORE_CPP_SECTION_2
    #include AZ_RESTRICTED_FILE(ShaderCore_cpp)
#endif
    
    return gEnv->pCryPak->OpenPack("@assets@", shaderPakDir, AZ::IO::IArchive::FLAGS_PAK_IN_MEMORY | AZ::IO::IArchive::FLAGS_PATH_REAL);
}

void CShaderMan::UnloadShaderStartupCache()
{
    // Called from the MT so need to flush RT
    IF (gRenDev->m_pRT, 1)
    {
        gRenDev->m_pRT->FlushAndWait();
    }

#if defined(SHADERS_SERIALIZING)
    // Free all import data allowing us to close the startup pack
    ClearSResourceCache();
#endif

    gEnv->pCryPak->ClosePack("ShaderCacheStartup.pak");
}

void CShaderMan::mfPostInit()
{
    LOADING_TIME_PROFILE_SECTION;
#if !defined(NULL_RENDERER)
    CTexture::PostInit();

    if (!gRenDev->IsEditorMode() && !gRenDev->IsShaderCacheGenMode())
    {
        mfPreloadBinaryShaders();
    }
#endif

    // (enabled also for NULL Renderer, so at least the default shader is initialized)
    if (!gRenDev->IsShaderCacheGenMode())
    {
        mfLoadDefaultSystemShaders();
    }
}

void CShaderMan::OnSystemEvent(ESystemEvent event, [[maybe_unused]] UINT_PTR wparam, [[maybe_unused]] UINT_PTR lparam)
{
    switch (event)
    {
    case ESYSTEM_EVENT_LEVEL_LOAD_END:
    {
        break;
    }
    }
}

void CShaderMan::ParseShaderProfile(char* scr, SShaderProfile* pr)
{
    char* name;
    long cmd;
    char* params;
    char* data;

    enum
    {
        eUseNormalAlpha = 1
    };
    static STokenDesc commands[] =
    {
        {eUseNormalAlpha, "UseNormalAlpha"},

        {0, 0}
    };

    while ((cmd = shGetObject (&scr, commands, &name, &params)) > 0)
    {
        data = NULL;
        if (name)
        {
            data = name;
        }
        else
        if (params)
        {
            data = params;
        }

        switch (cmd)
        {
        case eUseNormalAlpha:
            pr->m_nShaderProfileFlags |= SPF_LOADNORMALALPHA;
            break;
        }
    }
}

void CShaderMan::ParseShaderProfiles()
{
    int i;

    for (i = 0; i < eSQ_Max; i++)
    {
        m_ShaderFixedProfiles[i].m_iShaderProfileQuality = i;
        m_ShaderFixedProfiles[i].m_nShaderProfileFlags = 0;
    }

    char* name;
    long cmd;
    char* params;
    char* data;

    enum
    {
        eProfile = 1, eVersion
    };
    static STokenDesc commands[] =
    {
        {eProfile, "Profile"},
        {eVersion, "Version"},

        {0, 0}
    };

    char* scr = NULL;
    AZ::IO::HandleType fileHandle = gEnv->pCryPak->FOpen("Shaders/ShaderProfiles.txt", "rb");
    if (fileHandle != AZ::IO::InvalidHandle)
    {
        AZ::u64 fileSize = 0;
        AZ::IO::FileIOBase::GetInstance()->Size(fileHandle, fileSize);
        scr = new char [fileSize + 1];
        if (scr)
        {
            scr[fileSize] = 0;
            gEnv->pCryPak->FRead(scr, fileSize, fileHandle);
            gEnv->pCryPak->FClose(fileHandle);
        }
    }
    char* pScr = scr;
    if (scr)
    {
        while ((cmd = shGetObject (&scr, commands, &name, &params)) > 0)
        {
            data = NULL;
            if (name)
            {
                data = name;
            }
            else
            if (params)
            {
                data = params;
            }

            switch (cmd)
            {
            case eProfile:
            {
                SShaderProfile* pr = NULL;
                assert(name);
                PREFAST_ASSUME(name);
                if (!azstricmp(name, "Low"))
                {
                    pr = &m_ShaderFixedProfiles[eSQ_Low];
                }
                else
                {
                    pr = &m_ShaderFixedProfiles[eSQ_High];
                }
                if (pr)
                {
                    ParseShaderProfile(params, pr);
                }
                break;
            }
            case eVersion:
                break;
            }
        }
    }
    SAFE_DELETE_ARRAY(pScr);
}

const SShaderProfile& CRenderer::GetShaderProfile(EShaderType eST) const
{
    assert((int)eST < sizeof(m_cEF.m_ShaderProfiles) / sizeof(SShaderProfile));

    return m_cEF.m_ShaderProfiles[eST];
}

void CShaderMan::RT_SetShaderQuality(EShaderType eST, EShaderQuality eSQ)
{
    eSQ = CLAMP(eSQ, eSQ_Low, eSQ_VeryHigh);
    if (eST == eST_All)
    {
        for (int i = 0; i < eST_Max; i++)
        {
            m_ShaderProfiles[i] = m_ShaderFixedProfiles[eSQ];
            m_ShaderProfiles[i].m_iShaderProfileQuality = eSQ;      // good?
        }
    }
    else
    {
        m_ShaderProfiles[eST] = m_ShaderFixedProfiles[eSQ];
        m_ShaderProfiles[eST].m_iShaderProfileQuality = eSQ;      // good?
    }
    if (eST == eST_All || eST == eST_General)
    {
        bool bPS20 = ((gRenDev->m_Features & (RFT_HW_SM2X | RFT_HW_SM30)) == 0) || (eSQ == eSQ_Low);
        m_Bin.InvalidateCache();
        mfReloadAllShaders(FRO_FORCERELOAD, 0);
    }
}

#if !defined(NULL_RENDERER)
static byte bFirst = 1;

static bool sLoadShader(const char* szName, CShader*& pStorage)
{
    CShader* ef = NULL;
    bool bRes = true;
    if (bFirst)
    {
        CryComment("Load System Shader '%s'...", szName);
    }
    ef = gRenDev->m_cEF.mfForName(szName, EF_SYSTEM);
    if (bFirst)
    {
        if (!ef || (ef->m_Flags & EF_NOTFOUND))
        {
            Warning("Load System Shader Failed %s", szName);
            bRes = false;
        }
        else
        {
            CryComment("ok");
        }
    }
    pStorage = ef;
    return bRes;
}
#endif

void CShaderMan::mfReleaseSystemShaders ()
{
#ifndef NULL_RENDERER
    SAFE_RELEASE_FORCE(s_DefaultShader);
    SAFE_RELEASE_FORCE(s_ShaderDebug);
    SAFE_RELEASE_FORCE(s_ShaderLensOptics);
    SAFE_RELEASE_FORCE(s_ShaderSoftOcclusionQuery);
    SAFE_RELEASE_FORCE(s_ShaderOcclTest);
    SAFE_RELEASE_FORCE(s_ShaderStereo);
    SAFE_RELEASE_FORCE(s_ShaderDXTCompress);
    SAFE_RELEASE_FORCE(s_ShaderCommon);
#if defined(FEATURE_SVO_GI)
    SAFE_RELEASE_FORCE(s_ShaderSVOGI);
#endif
    SAFE_RELEASE_FORCE(s_ShaderShadowBlur);
    SAFE_RELEASE_FORCE(s_shPostEffectsGame);
    SAFE_RELEASE_FORCE(s_shPostAA);
    SAFE_RELEASE_FORCE(s_shPostEffects);
    SAFE_RELEASE_FORCE(s_ShaderFallback);
    SAFE_RELEASE_FORCE(s_ShaderFPEmu);
    SAFE_RELEASE_FORCE(s_ShaderUI);
    SAFE_RELEASE_FORCE(s_ShaderLightStyles);
    SAFE_RELEASE_FORCE(s_ShaderShadowMaskGen);
    SAFE_RELEASE_FORCE(s_shHDRPostProcess);
    SAFE_RELEASE_FORCE(s_shPostDepthOfField);
    SAFE_RELEASE_FORCE(s_shPostMotionBlur);
    SAFE_RELEASE_FORCE(s_shPostSunShafts);
    SAFE_RELEASE_FORCE(s_ShaderDeferredCaustics);
    SAFE_RELEASE_FORCE(s_shDeferredShading);
    SAFE_RELEASE_FORCE(s_ShaderDeferredRain);
    SAFE_RELEASE_FORCE(s_ShaderDeferredSnow);
    SAFE_RELEASE_FORCE(s_ShaderStars);
    SAFE_RELEASE_FORCE(s_ShaderFur);
    SAFE_RELEASE_FORCE(s_ShaderVideo);
    m_bLoadedSystem = false;
    m_systemShaders.clear();
#endif
}

void CShaderMan::OnShaderLoaded([[maybe_unused]] IShader* shader)
{
#if defined(AZ_ENABLE_TRACING) && defined(_RELEASE)
    static bool displayedErrorOnce = false;

    if((shader->GetFlags() & EF_NOTFOUND) && m_systemShaders.find(shader) != m_systemShaders.end())
    {
        static constexpr char message[] = "Unable to find system shader '%s'.  This will likely cause rendering issues, including a black screen.  Please make sure all required shaders are included in your pak files.";

        AZ_Error("ShaderCore", false, message, shader->GetName());

        if(!displayedErrorOnce)
        {
            displayedErrorOnce = true;

            AZStd::string displayMessage = AZStd::string::format(message, shader->GetName());
            displayMessage.append("  Check Game.log for the complete list of missing shaders.");

            AZ::NativeUI::NativeUIRequestBus::Broadcast(&AZ::NativeUI::NativeUIRequestBus::Events::DisplayOkDialog, "Missing System Shader", displayMessage.c_str(), false);
        }
    }
#endif
}

#if !defined(NULL_RENDERER)
void CShaderMan::mfLoadSystemShader(const char* szName, CShader*& pStorage)
{
    sLoadShader(szName, pStorage);
    m_systemShaders.emplace(pStorage);
}
#endif

void CShaderMan::mfLoadBasicSystemShaders()
{
    LOADING_TIME_PROFILE_SECTION;
    if (!s_DefaultShader)
    {
        s_DefaultShader = mfNewShader("<Default>");
        s_DefaultShader->m_NameShader = "<Default>";
        s_DefaultShader->m_Flags |= EF_SYSTEM;
    }
#ifndef NULL_RENDERER
    if (!m_bLoadedSystem && !gRenDev->IsShaderCacheGenMode())
    {
        mfLoadSystemShader("Fallback", s_ShaderFallback);
        mfLoadSystemShader("FixedPipelineEmu", s_ShaderFPEmu);
        mfLoadSystemShader("UI", s_ShaderUI);

        mfRefreshSystemShader("Stereo", CShaderMan::s_ShaderStereo);
        mfRefreshSystemShader("Video", CShaderMan::s_ShaderVideo);
    }
#endif
}

void CShaderMan::mfLoadDefaultSystemShaders()
{
    LOADING_TIME_PROFILE_SECTION;
    if (!s_DefaultShader)
    {
        s_DefaultShader = mfNewShader("<Default>");
        s_DefaultShader->m_NameShader = "<Default>";
        s_DefaultShader->m_Flags |= EF_SYSTEM;
    }
#ifndef NULL_RENDERER
    if (!m_bLoadedSystem)
    {
        m_bLoadedSystem = true;

        mfLoadSystemShader("Fallback", s_ShaderFallback);
        mfLoadSystemShader("FixedPipelineEmu", s_ShaderFPEmu);
        mfLoadSystemShader("UI", s_ShaderUI);
        mfLoadSystemShader("Light", s_ShaderLightStyles);

        mfLoadSystemShader("ShadowMaskGen", s_ShaderShadowMaskGen);
        mfLoadSystemShader("HDRPostProcess", s_shHDRPostProcess);

        mfLoadSystemShader("PostEffects", s_shPostEffects);

#if defined(FEATURE_SVO_GI)
        mfRefreshSystemShader("Total_Illumination", CShaderMan::s_ShaderSVOGI);
#endif
        mfRefreshSystemShader("Common", CShaderMan::s_ShaderCommon);
        mfRefreshSystemShader("Debug",  CShaderMan::s_ShaderDebug);
        mfRefreshSystemShader("DeferredCaustics",   CShaderMan::s_ShaderDeferredCaustics);
        mfRefreshSystemShader("DeferredRain",   CShaderMan::s_ShaderDeferredRain);
        mfRefreshSystemShader("DeferredSnow",   CShaderMan::s_ShaderDeferredSnow);
        mfRefreshSystemShader("DeferredShading",    CShaderMan::s_shDeferredShading);
        mfRefreshSystemShader("DepthOfField",   CShaderMan::s_shPostDepthOfField);
        mfRefreshSystemShader("DXTCompress",    CShaderMan::s_ShaderDXTCompress);
        mfRefreshSystemShader("LensOptics", CShaderMan::s_ShaderLensOptics);
        mfRefreshSystemShader("SoftOcclusionQuery", CShaderMan::s_ShaderSoftOcclusionQuery);
        mfRefreshSystemShader("MotionBlur", CShaderMan::s_shPostMotionBlur);
        mfRefreshSystemShader("OcclusionTest",  CShaderMan::s_ShaderOcclTest);
        mfRefreshSystemShader("PostEffectsGame",    CShaderMan::s_shPostEffectsGame);
        mfRefreshSystemShader("PostAA", CShaderMan::s_shPostAA);
        mfRefreshSystemShader("ShadowBlur", CShaderMan::s_ShaderShadowBlur);
        mfRefreshSystemShader("Sunshafts", CShaderMan::s_shPostSunShafts);
        mfRefreshSystemShader("Fur", CShaderMan::s_ShaderFur);
    }
#endif
}

void CShaderMan::mfSetDefaults (void)
{
#ifndef NULL_RENDERER
    mfReleaseSystemShaders();
    mfLoadBasicSystemShaders();
#else
    mfLoadBasicSystemShaders();
    s_DefaultShaderItem.m_pShader = s_DefaultShader;
    SInputShaderResources ShR;
    s_DefaultShaderItem.m_pShaderResources = new CShaderResources(&ShR);
#endif

    memset(&gRenDev->m_cEF.m_PF, 0, sizeof(PerFrameParameters));

    if (gRenDev->IsEditorMode())
    {
        gRenDev->RefreshSystemShaders();
    }

#if !defined(NULL_RENDERER)
    bFirst = 0;
#endif

    m_bInitialized = true;
}

bool CShaderMan::mfGatherShadersList(const char* szPath, bool bCheckIncludes, bool bUpdateCRC, std::vector<string>* Names)
{
    char nmf[256];
    char dirn[256];

    cry_strcpy(dirn, szPath);
    cry_strcat(dirn, "*");

    bool bChanged = false;

    AZ::IO::ArchiveFileIterator handle = gEnv->pCryPak->FindFirst(dirn);
    if (!handle)
    {
        return bChanged;
    }
    do
    {
        if (handle.m_filename.front() == '.')
        {
            continue;
        }
        if ((handle.m_fileDesc.nAttrib & AZ::IO::FileDesc::Attribute::Subdirectory) == AZ::IO::FileDesc::Attribute::Subdirectory)
        {
            char ddd[256];
            azsnprintf(ddd, AZ_ARRAY_SIZE(ddd), "%s%.*s/", szPath, aznumeric_cast<int>(handle.m_filename.size()), handle.m_filename.data());

            bChanged = mfGatherShadersList(ddd, bCheckIncludes, bUpdateCRC, Names);
            if (bChanged)
            {
                break;
            }
            continue;
        }
        cry_strcpy(nmf, szPath);
        cry_strcat(nmf, handle.m_filename.data());
        int len = strlen(nmf) - 1;
        while (len >= 0 && nmf[len] != '.')
        {
            len--;
        }
        if (len <= 0)
        {
            continue;
        }
        if (bCheckIncludes)
        {
            if (!azstricmp(&nmf[len], ".cfi"))
            {
                fpStripExtension(handle.m_filename.data(), nmf);
                SShaderBin* pBin = m_Bin.GetBinShader(nmf, true, 0, &bChanged);

                // If any include file was not found in the read only cache, we'll need to update the CRCs
                if (pBin && pBin->IsReadOnly() == false)
                {
                    bChanged = true;
                }

                if (bChanged)
                {
                    break;
                }
            }
            continue;
        }
        if (azstricmp(&nmf[len], ".cfx"))
        {
            continue;
        }
        fpStripExtension(handle.m_filename.data(), nmf);
        mfAddFXShaderNames(nmf, Names, bUpdateCRC);
    } while (handle = gEnv->pCryPak->FindNext(handle));

    gEnv->pCryPak->FindClose (handle);

    return bChanged;
}

void CShaderMan::mfGatherFilesList(const char* szPath, std::vector<CCryNameR>& Names, int nLevel, bool bUseFilter, bool bMaterial)
{
    char nmf[256];
    char dirn[256];

    cry_strcpy(dirn, szPath);
    cry_strcat(dirn, "*");

    AZ::IO::ArchiveFileIterator handle = gEnv->pCryPak->FindFirst(dirn);
    if (!handle)
    {
        return;
    }
    do
    {
        if (handle.m_filename.front() == '.')
        {
            continue;
        }
        if ((handle.m_fileDesc.nAttrib & AZ::IO::FileDesc::Attribute::Subdirectory) == AZ::IO::FileDesc::Attribute::Subdirectory)
        {
            if (!bUseFilter || nLevel != 1 || !azstricmp(handle.m_filename.data(), m_ShadersFilter.c_str()))
            {
                char ddd[256];
                azsnprintf(ddd, AZ_ARRAY_SIZE(ddd), "%s%.*s/", szPath, aznumeric_cast<int>(handle.m_filename.size()), handle.m_filename.data());

                mfGatherFilesList(ddd, Names, nLevel + 1, bUseFilter, bMaterial);
            }
            continue;
        }
        cry_strcpy(nmf, szPath);
        cry_strcat(nmf, handle.m_filename.data());
        int len = strlen(nmf) - 1;
        while (len >= 0 && nmf[len] != '.')
        {
            len--;
        }
        if (len <= 0)
        {
            continue;
        }
        if (!bMaterial && azstricmp(&nmf[len], ".fxcb"))
        {
            continue;
        }
        if (bMaterial && azstricmp(&nmf[len], ".mtl"))
        {
            continue;
        }
        Names.push_back(CCryNameR(nmf));
    } while (handle = gEnv->pCryPak->FindNext(handle));

    gEnv->pCryPak->FindClose (handle);
}

int CShaderMan::mfInitShadersList(std::vector<string>* Names)
{
    // Detect include changes
    bool bChanged = mfGatherShadersList(m_ShadersPath.c_str(), true, false, Names);

    if (gRenDev->m_bShaderCacheGen)
    {
        // flush out EXT files, so we reload them again after proper per-platform setup
        for (ShaderExtItor it = gRenDev->m_cEF.m_ShaderExts.begin(); it != gRenDev->m_cEF.m_ShaderExts.end(); ++it)
        {
            delete it->second;
        }
        gRenDev->m_cEF.m_ShaderExts.clear();
        gRenDev->m_cEF.m_SGC.clear();

        m_ShaderCacheCombinations[0].clear();
        m_ShaderCacheCombinations[1].clear();
        m_ShaderCacheExportCombinations.clear();
        mfCloseShadersCache(0);
        mfInitShadersCache(false, NULL, NULL, 0);
    }

#ifndef NULL_RENDERER
    mfGatherShadersList(m_ShadersPath.c_str(), false, bChanged, Names);
    return m_ShaderNames.size();
#else
    return 0;
#endif
}

void CShaderMan::mfPreloadShaderExts(void)
{
    AZ::IO::ArchiveFileIterator handle = gEnv->pCryPak->FindFirst ("Shaders/*.ext");
    if (!handle)
    {
        return;
    }
    do
    {
        if (handle.m_filename.front() == '.')
        {
            continue;
        }
        if ((handle.m_fileDesc.nAttrib & AZ::IO::FileDesc::Attribute::Subdirectory) == AZ::IO::FileDesc::Attribute::Subdirectory)
        {
            continue;
        }

        if (!azstricmp(handle.m_filename.data(), "runtime.ext") || !azstricmp(handle.m_filename.data(), "statics.ext"))
        {
            continue;
        }
        char s[256];
        fpStripExtension(handle.m_filename.data(), s);
        SShaderGen* pShGen = mfCreateShaderGenInfo(s, false);
        assert(pShGen);
    } while (handle = gEnv->pCryPak->FindNext(handle));

    gEnv->pCryPak->FindClose (handle);
}


//===================================================================

CShader* CShaderMan::mfNewShader(const char* szName)
{
    CShader* pSH;

    CCryNameTSCRC className = CShader::mfGetClassName();
    CCryNameTSCRC Name = szName;
    CBaseResource* pBR = CBaseResource::GetResource(className, Name, false);
    if (!pBR)
    {
        pSH = new CShader;
        pSH->Register(className, Name);
    }
    else
    {
        pSH = (CShader*)pBR;
        pSH->AddRef();
    }
    if (!s_pContainer)
    {
        s_pContainer = CBaseResource::GetResourcesForClass(className);
    }

    if (pSH->GetID() >= MAX_REND_SHADERS)
    {
        SAFE_RELEASE(pSH);
        iLog->Log("ERROR: MAX_REND_SHADERS hit\n");
        return NULL;
    }

    return pSH;
}

//=========================================================

bool CShaderMan::mfUpdateMergeStatus(SShaderTechnique* hs, std::vector<SCGParam>* p)
{
    if (!p)
    {
        return false;
    }
    for (uint32 n = 0; n < p->size(); n++)
    {
        if ((*p)[n].m_Flags & PF_DONTALLOW_DYNMERGE)
        {
            hs->m_Flags |= FHF_NOMERGE;
            break;
        }
    }
    if (hs->m_Flags & FHF_NOMERGE)
    {
        return true;
    }
    return false;
}


//=================================================================================================

SEnvTexture* SHRenderTarget::GetEnv2D()
{
    CRenderer* rd = gRenDev;
    SEnvTexture* pEnvTex = NULL;
    if (m_nIDInPool >= 0)
    {
        assert(m_nIDInPool < (int)CTexture::s_CustomRT_2D->Num());
        if (m_nIDInPool < (int)CTexture::s_CustomRT_2D->Num())
        {
            pEnvTex = &(*CTexture::s_CustomRT_2D)[m_nIDInPool];
        }
    }
    else
    {
        const CCamera& cam = rd->GetCamera();
        Matrix33 orientation = Matrix33(cam.GetMatrix());
        Ang3 Angs = CCamera::CreateAnglesYPR(orientation);
        Vec3 Pos = cam.GetPosition();
        bool bReflect = false;
        if (m_nFlags & (FRT_CAMERA_REFLECTED_PLANE | FRT_CAMERA_REFLECTED_WATERPLANE))
        {
            bReflect = true;
        }
        pEnvTex = CTexture::FindSuitableEnvTex(Pos, Angs, true, 0, false, rd->m_RP.m_pShader, rd->m_RP.m_pShaderResources, rd->m_RP.m_pCurObject, bReflect, rd->m_RP.m_pRE, NULL);
    }
    return pEnvTex;
}

void SHRenderTarget::GetMemoryUsage(ICrySizer* pSizer) const
{
    pSizer->Add(*this);
    pSizer->AddObject(m_TargetName);
    pSizer->AddObject(m_pTarget[0]);
    pSizer->AddObject(m_pTarget[1]);
}

void CShaderResources::CreateModifiers(SInputShaderResources* pInRes)
{
    for (auto iter = m_TexturesResourcesMap.begin(); iter != m_TexturesResourcesMap.end(); ++iter)
    {
        uint            slotIdx = iter->first;
        SEfResTexture*  pDst = &(iter->second);  

        pDst->m_Ext.m_nUpdateFlags = 0;

        SEfResTexture*  pInTex = pInRes->GetTextureResource(slotIdx);
        if (!pInTex || !pInTex->m_Ext.m_pTexModifier)
        {
            continue;
        }

        pInTex->m_Ext.m_nUpdateFlags = 0;
        pInTex->m_Ext.m_nLastRecursionLevel = -1;
        SEfTexModificator* pMod = pInTex->m_Ext.m_pTexModifier;


            if (pMod->m_eTGType >= ETG_Max)
            {
                pMod->m_eTGType = ETG_Stream;
            }
            if (pMod->m_eRotType >= ETMR_Max)
            {
                pMod->m_eRotType = ETMR_NoChange;
            }

            if (pMod->m_eMoveType[0] >= ETMM_Max)
            {
                pMod->m_eMoveType[0] = ETMM_NoChange;
            }
            if (pMod->m_eMoveType[1] >= ETMM_Max)
            {
                pMod->m_eMoveType[1] = ETMM_NoChange;
            }

            if (pMod->m_eMoveType[0] == ETMM_Pan && (pMod->m_OscAmplitude[0] == 0 || pMod->m_OscRate[0] == 0))
            {
                pMod->m_eMoveType[0] = ETMM_NoChange;
            }
            if (pMod->m_eMoveType[1] == ETMM_Pan && (pMod->m_OscAmplitude[1] == 0 || pMod->m_OscRate[1] == 0))
            {
                pMod->m_eMoveType[1] = ETMM_NoChange;
            }

            if (pMod->m_eMoveType[0] == ETMM_Fixed && pMod->m_OscRate[0] == 0)
            {
                pMod->m_eMoveType[0] = ETMM_NoChange;
            }
            if (pMod->m_eMoveType[1] == ETMM_Fixed && pMod->m_OscRate[1] == 0)
            {
                pMod->m_eMoveType[1] = ETMM_NoChange;
            }

            if (pMod->m_eMoveType[0] == ETMM_Constant && (pMod->m_OscAmplitude[0] == 0 || pMod->m_OscRate[0] == 0))
            {
                pMod->m_eMoveType[0] = ETMM_NoChange;
            }
            if (pMod->m_eMoveType[1] == ETMM_Constant && (pMod->m_OscAmplitude[1] == 0 || pMod->m_OscRate[1] == 0))
            {
                pMod->m_eMoveType[1] = ETMM_NoChange;
            }

            if (pMod->m_eMoveType[0] == ETMM_Stretch && (pMod->m_OscAmplitude[0] == 0 || pMod->m_OscRate[0] == 0))
            {
                pMod->m_eMoveType[0] = ETMM_NoChange;
            }
            if (pMod->m_eMoveType[1] == ETMM_Stretch && (pMod->m_OscAmplitude[1] == 0 || pMod->m_OscRate[1] == 0))
            {
                pMod->m_eMoveType[1] = ETMM_NoChange;
            }

            if (pMod->m_eMoveType[0] == ETMM_StretchRepeat && (pMod->m_OscAmplitude[0] == 0 || pMod->m_OscRate[0] == 0))
            {
                pMod->m_eMoveType[0] = ETMM_NoChange;
            }
            if (pMod->m_eMoveType[1] == ETMM_StretchRepeat && (pMod->m_OscAmplitude[1] == 0 || pMod->m_OscRate[1] == 0))
            {
                pMod->m_eMoveType[1] = ETMM_NoChange;
            }

            if (pMod->m_eTGType != ETG_Stream)
            {
                m_ResFlags |= MTL_FLAG_NOTINSTANCED;
            }

        pInTex->UpdateForCreate(slotIdx);
        if (pInTex->m_Ext.m_nUpdateFlags & HWMD_TEXCOORD_FLAG_MASK)
        {
            SEfTexModificator* pDstMod = new SEfTexModificator;
            *pDstMod = *pMod;
            SAFE_DELETE(pDst->m_Ext.m_pTexModifier);
            pDst->m_Ext.m_pTexModifier = pDstMod;
        }
        else
        {
            if (pDst->m_Sampler.m_eTexType == eTT_Auto2D)
            {
                if (!pDst->m_Ext.m_pTexModifier)
                {
                    m_ResFlags |= MTL_FLAG_NOTINSTANCED;
                    SEfTexModificator* pDstMod = new SEfTexModificator;
                    *pDstMod = *pMod;
                    pDst->m_Ext.m_pTexModifier = pDstMod;
                }
            }

            if (pMod->m_bTexGenProjected)
            {
                pDst->m_Ext.m_nUpdateFlags |= HWMD_TEXCOORD_PROJ;
            }
        }
    }
}

int32 GetTextCoordGenObjLinearFlag(int textSlot)
{
    switch (static_cast<EEfResTextures>(textSlot))
    {
    case EFTT_DIFFUSE:
        return HWMD_TEXCOORD_GEN_OBJECT_LINEAR_DIFFUSE;
    case  EFTT_DETAIL_OVERLAY:
        return HWMD_TEXCOORD_GEN_OBJECT_LINEAR_DETAIL;
    case EFTT_DECAL_OVERLAY:
        return HWMD_TEXCOORD_GEN_OBJECT_LINEAR_EMITTANCE_MULT;
    case EFTT_EMITTANCE:
        return HWMD_TEXCOORD_GEN_OBJECT_LINEAR_EMITTANCE;
    case EFTT_CUSTOM:
        return HWMD_TEXCOORD_GEN_OBJECT_LINEAR_CUSTOM;
    default:
        return 0;
    }
}

void SEfResTexture::UpdateForCreate(int textSlot)
{
    FUNCTION_PROFILER_RENDER_FLAT

    SEfTexModificator* pMod = m_Ext.m_pTexModifier;
    if (!pMod)
    {
        return;
    }

    m_Ext.m_nUpdateFlags = 0;

    ETEX_Type eTT = (ETEX_Type)m_Sampler.m_eTexType;
    if (eTT == eTT_Auto2D && m_Sampler.m_pTarget)
    {
        SEnvTexture* pEnv = m_Sampler.m_pTarget->GetEnv2D();
        assert(pEnv);
        if (pEnv && pEnv->m_pTex)
        {
            m_Ext.m_nUpdateFlags |= HWMD_TEXCOORD_PROJ | GetTextCoordGenObjLinearFlag(textSlot);
        }
    }

    bool bTr = false;

    if (pMod->m_Tiling[0] == 0)
    {
        pMod->m_Tiling[0] = 1.0f;
    }
    if (pMod->m_Tiling[1] == 0)
    {
        pMod->m_Tiling[1] = 1.0f;
    }

    bTr = pMod->isModified();

    if (pMod->m_eTGType != ETG_Stream)
    {
        switch (pMod->m_eTGType)
        {
        case ETG_World:
        case ETG_Camera:
            m_Ext.m_nUpdateFlags |= GetTextCoordGenObjLinearFlag(textSlot);
            break;
        }
    }

    if (bTr)
    {
        m_Ext.m_nUpdateFlags |= HWMD_TEXCOORD_MATRIX;
    }


    if (pMod->m_bTexGenProjected)
    {
        m_Ext.m_nUpdateFlags |= HWMD_TEXCOORD_PROJ;
    }
}

// Update TexGen and TexTransform matrices for current material texture
void SEfResTexture::Update(int nTSlot)
{
    FUNCTION_PROFILER_RENDER_FLAT
        PrefetchLine(m_Sampler.m_pTex, 0);
    CRenderer* rd = gRenDev;

    assert(nTSlot < MAX_TMU);
    rd->m_RP.m_ShaderTexResources[nTSlot] = this;

    const SEfTexModificator* const pMod = m_Ext.m_pTexModifier;

    if(!pMod)
    {
        if (IsTextureModifierSupportedForTextureMap(static_cast<EEfResTextures>(nTSlot)))
        {
            rd->m_RP.m_FlagsShader_MD |= m_Ext.m_nUpdateFlags;
        }
    }
    else
    {
        UpdateWithModifier(nTSlot);
    }
}

void SEfResTexture::UpdateWithModifier(int nTSlot)
{
    CRenderer* rd = gRenDev;
    int nFrameID = rd->m_RP.m_TI[rd->m_RP.m_nProcessThreadID].m_nFrameID;

    // Skip update if the modifier was updated (same frame id, except frame id with default value -1,  and same recursion level) 
    if (m_Ext.m_nFrameUpdated != -1 && m_Ext.m_nFrameUpdated == nFrameID && m_Ext.m_nLastRecursionLevel == SRendItem::m_RecurseLevel[rd->m_RP.m_nProcessThreadID])
    {
        if (IsTextureModifierSupportedForTextureMap(static_cast<EEfResTextures>(nTSlot)))
        {
            rd->m_RP.m_FlagsShader_MD |= m_Ext.m_nUpdateFlags;
        }

        return;
    }

    m_Ext.m_nFrameUpdated = nFrameID;
    m_Ext.m_nLastRecursionLevel = SRendItem::m_RecurseLevel[rd->m_RP.m_nProcessThreadID];
    m_Ext.m_nUpdateFlags = 0;

    ETEX_Type eTT = (ETEX_Type)m_Sampler.m_eTexType;
    SEfTexModificator* pMod = m_Ext.m_pTexModifier;
    if (eTT == eTT_Auto2D && m_Sampler.m_pTarget)
    {
        SEnvTexture* pEnv = m_Sampler.m_pTarget->GetEnv2D();
        assert(pEnv);
        if (pEnv && pEnv->m_pTex)
        {
            pMod->m_TexGenMatrix = Matrix44A(rd->m_RP.m_pCurObject->m_II.m_Matrix).GetTransposed() * pEnv->m_Matrix;
            pMod->m_TexGenMatrix = pMod->m_TexGenMatrix.GetTransposed();
            m_Ext.m_nUpdateFlags |= HWMD_TEXCOORD_PROJ | GetTextCoordGenObjLinearFlag(nTSlot);
        }
    }

    bool bTr = false;
    bool bTranspose = false;
    Plane Pl;
    Plane PlTr;

    const float fTiling0 = pMod->m_Tiling[0];
    const float fTiling1 = pMod->m_Tiling[1];
    pMod->m_Tiling[0] = (float)fsel(-fabsf(fTiling0), 1.0f, fTiling0);
    pMod->m_Tiling[1] = (float)fsel(-fabsf(fTiling1), 1.0f, fTiling1);

    if (pMod->isModified())
    {
        pMod->m_TexMatrix.SetIdentity();
        float fTime = gRenDev->m_RP.m_TI[gRenDev->m_RP.m_nProcessThreadID].m_RealTime;

        bTr = true;

        switch (pMod->m_eRotType)
        {
        case ETMR_Fixed:
        {
            //translation matrix for rotation center
            pMod->m_TexMatrix = pMod->m_TexMatrix *
                Matrix44(1, 0, 0, 0,
                    0, 1, 0, 0,
                    0, 0, 1, 0,
                    -pMod->m_RotOscCenter[0], -pMod->m_RotOscCenter[1], -pMod->m_RotOscCenter[2], 1);

            if (pMod->m_RotOscAmplitude[0])
            {
                pMod->m_TexMatrix = pMod->m_TexMatrix * Matrix33::CreateRotationX(Word2Degr(pMod->m_RotOscAmplitude[0]) * PI / 180.0f);
            }
            if (pMod->m_RotOscAmplitude[1])
            {
                pMod->m_TexMatrix = pMod->m_TexMatrix * Matrix33::CreateRotationY(Word2Degr(pMod->m_RotOscAmplitude[1]) * PI / 180.0f);
            }
            if (pMod->m_RotOscAmplitude[2])
            {
                pMod->m_TexMatrix = pMod->m_TexMatrix * Matrix33::CreateRotationZ(Word2Degr(pMod->m_RotOscAmplitude[2]) * PI / 180.0f);
            }

            //translation matrix for rotation center
            pMod->m_TexMatrix =  pMod->m_TexMatrix *
                Matrix44(1, 0, 0, 0,
                    0, 1, 0, 0,
                    0, 0, 1, 0,
                    pMod->m_RotOscCenter[0], pMod->m_RotOscCenter[1], pMod->m_RotOscCenter[2], 1);
        }
        break;

        case ETMR_Constant:
        {
            fTime *= 1000.0f;
            float fxAmp = Word2Degr(pMod->m_RotOscAmplitude[0]) * fTime * PI / 180.0f + Word2Degr(pMod->m_RotOscPhase[0]);
            float fyAmp = Word2Degr(pMod->m_RotOscAmplitude[1]) * fTime * PI / 180.0f + Word2Degr(pMod->m_RotOscPhase[1]);
            float fzAmp = Word2Degr(pMod->m_RotOscAmplitude[2]) * fTime * PI / 180.0f + Word2Degr(pMod->m_RotOscPhase[2]);

            //translation matrix for rotation center
            pMod->m_TexMatrix = pMod->m_TexMatrix *
                Matrix44(1, 0, 0, 0,
                    0, 1, 0, 0,
                    0, 0, 1, 0,
                    -pMod->m_RotOscCenter[0], -pMod->m_RotOscCenter[1], -pMod->m_RotOscCenter[2], 1);

            //rotation matrix
            if (fxAmp)
            {
                pMod->m_TexMatrix = pMod->m_TexMatrix * Matrix44A(Matrix33::CreateRotationX(fxAmp)).GetTransposed();
            }
            if (fyAmp)
            {
                pMod->m_TexMatrix = pMod->m_TexMatrix * Matrix44A(Matrix33::CreateRotationY(fyAmp)).GetTransposed();
            }
            if (fzAmp)
            {
                pMod->m_TexMatrix = pMod->m_TexMatrix * Matrix44A(Matrix33::CreateRotationZ(fzAmp)).GetTransposed();
            }

            //translation matrix for rotation center
            pMod->m_TexMatrix =  pMod->m_TexMatrix *
                Matrix44(1, 0, 0, 0,
                    0, 1, 0, 0,
                    0, 0, 1, 0,
                    pMod->m_RotOscCenter[0], pMod->m_RotOscCenter[1], pMod->m_RotOscCenter[2], 1);
        }
        break;

        case ETMR_Oscillated:
        {
            //translation matrix for rotation center
            pMod->m_TexMatrix = pMod->m_TexMatrix *
                Matrix44(1, 0, 0, 0,
                    0, 1, 0, 0,
                    0, 0, 1, 0,
                    -pMod->m_RotOscCenter[0], -pMod->m_RotOscCenter[1], -pMod->m_RotOscCenter[2], 1);

            float S_X = fTime * Word2Degr(pMod->m_RotOscRate[0]);
            float S_Y = fTime * Word2Degr(pMod->m_RotOscRate[1]);
            float S_Z = fTime * Word2Degr(pMod->m_RotOscRate[2]);

            float d_X = Word2Degr(pMod->m_RotOscAmplitude[0]) * sin_tpl(2.0f * PI * ((S_X - floor_tpl(S_X)) + Word2Degr(pMod->m_RotOscPhase[0])));
            float d_Y = Word2Degr(pMod->m_RotOscAmplitude[1]) * sin_tpl(2.0f * PI * ((S_Y - floor_tpl(S_Y)) + Word2Degr(pMod->m_RotOscPhase[1])));
            float d_Z = Word2Degr(pMod->m_RotOscAmplitude[2]) * sin_tpl(2.0f * PI * ((S_Z - floor_tpl(S_Z)) + Word2Degr(pMod->m_RotOscPhase[2])));

            if (d_X)
            {
                pMod->m_TexMatrix = pMod->m_TexMatrix * Matrix33::CreateRotationX(d_X);
            }
            if (d_Y)
            {
                pMod->m_TexMatrix = pMod->m_TexMatrix * Matrix33::CreateRotationY(d_Y);
            }
            if (d_Z)
            {
                pMod->m_TexMatrix = pMod->m_TexMatrix * Matrix33::CreateRotationZ(d_Z);
            }

            //translation matrix for rotation center
            pMod->m_TexMatrix =  pMod->m_TexMatrix *
                Matrix44(1, 0, 0, 0,
                    0, 1, 0, 0,
                    0, 0, 1, 0,
                    pMod->m_RotOscCenter[0], pMod->m_RotOscCenter[1], pMod->m_RotOscCenter[2], 1);
        }
        break;
        }


        float Su = rd->m_RP.m_TI[gRenDev->m_RP.m_nProcessThreadID].m_RealTime * pMod->m_OscRate[0];
        float Sv = rd->m_RP.m_TI[gRenDev->m_RP.m_nProcessThreadID].m_RealTime * pMod->m_OscRate[1];
        switch (pMod->m_eMoveType[0])
        {
        case ETMM_Pan:
        {
            float  du = pMod->m_OscAmplitude[0] * sin_tpl(2.0f * PI * (Su - floor_tpl(Su)) + 2.f * PI * pMod->m_OscPhase[0]);
            pMod->m_TexMatrix(3, 0) = du;
        }
        break;
        case ETMM_Fixed:
        {
            float du = pMod->m_OscRate[0];
            pMod->m_TexMatrix(3, 0) = du;
        }
        break;
        case ETMM_Constant:
        {
            float du = pMod->m_OscAmplitude[0] * Su;         //(Su - floor_tpl(Su));
            pMod->m_TexMatrix(3, 0) = du;
        }
        break;
        case ETMM_Jitter:
        {
            if (pMod->m_LastTime[0] < 1.0f || pMod->m_LastTime[0] > Su + 1.0f)
            {
                pMod->m_LastTime[0] = pMod->m_OscPhase[0] + floor_tpl(Su);
            }
            if (Su - pMod->m_LastTime[0] > 1.0f)
            {
                pMod->m_CurrentJitter[0] = cry_random(0.0f, pMod->m_OscAmplitude[0]);
                pMod->m_LastTime[0] = pMod->m_OscPhase[0] + floor_tpl(Su);
            }
            pMod->m_TexMatrix(3, 0) = pMod->m_CurrentJitter[0];
        }
        break;
        case ETMM_Stretch:
        {
            float du = pMod->m_OscAmplitude[0] * sin_tpl(2.0f * PI * (Su - floor_tpl(Su)) + 2.0f * PI * pMod->m_OscPhase[0]);
            pMod->m_TexMatrix(0, 0) = 1.0f + du;
        }
        break;
        case ETMM_StretchRepeat:
        {
            float du = pMod->m_OscAmplitude[0] * sin_tpl(0.5f * PI * (Su - floor_tpl(Su)) + 2.0f * PI * pMod->m_OscPhase[0]);
            pMod->m_TexMatrix(0, 0) = 1.0f + du;
        }
        break;
        }

        switch (pMod->m_eMoveType[1])
        {
        case ETMM_Pan:
        {
            float dv = pMod->m_OscAmplitude[1] * sin_tpl(2.0f * PI * (Sv - floor_tpl(Sv)) + 2.0f * PI * pMod->m_OscPhase[1]);
            pMod->m_TexMatrix(3, 1) = dv;
        }
        break;
        case ETMM_Fixed:
        {
            float dv = pMod->m_OscRate[1];
            pMod->m_TexMatrix(3, 1) = dv;
        }
        break;
        case ETMM_Constant:
        {
            float dv = pMod->m_OscAmplitude[1] * Sv;         //(Sv - floor_tpl(Sv));
            pMod->m_TexMatrix(3, 1) = dv;
        }
        break;
        case ETMM_Jitter:
        {
            if (pMod->m_LastTime[1] < 1.0f || pMod->m_LastTime[1] > Sv + 1.0f)
            {
                pMod->m_LastTime[1] = pMod->m_OscPhase[1] + floor_tpl(Sv);
            }
            if (Sv - pMod->m_LastTime[1] > 1.0f)
            {
                pMod->m_CurrentJitter[1] = cry_random(0.0f, pMod->m_OscAmplitude[1]);
                pMod->m_LastTime[1] = pMod->m_OscPhase[1] + floor_tpl(Sv);
            }
            pMod->m_TexMatrix(3, 1) = pMod->m_CurrentJitter[1];
        }
        break;
        case ETMM_Stretch:
        {
            float dv = pMod->m_OscAmplitude[1] * sin_tpl(2.0f * PI * (Sv - floor_tpl(Sv)) + 2.0f * PI * pMod->m_OscPhase[1]);
            pMod->m_TexMatrix(1, 1) = 1.0f + dv;
        }
        break;
        case ETMM_StretchRepeat:
        {
            float dv = pMod->m_OscAmplitude[1] * sin_tpl(0.5f * PI * (Sv - floor_tpl(Sv)) + 2.0f * PI * pMod->m_OscPhase[1]);
            pMod->m_TexMatrix(1, 1) = 1.0f + dv;
        }
        break;
        }

        if (pMod->m_Offs[0] != 0.0f ||
            pMod->m_Offs[1] != 0.0f ||
            pMod->m_Tiling[0] != 1.0f ||
            pMod->m_Tiling[1] != 1.0f ||
            pMod->m_Rot[0] != 0.0f ||
            pMod->m_Rot[1] != 0.0f ||
            pMod->m_Rot[2] != 0.0f)
        {
            float du = pMod->m_Offs[0];
            float dv = pMod->m_Offs[1];
            float su = pMod->m_Tiling[0];
            float sv = pMod->m_Tiling[1];

            if (pMod->m_Rot[0])
            {
                pMod->m_TexMatrix = pMod->m_TexMatrix * Matrix33::CreateRotationX(Word2Degr(pMod->m_Rot[0]) * PI / 180.0f);
            }
            if (pMod->m_Rot[1])
            {
                pMod->m_TexMatrix = pMod->m_TexMatrix * Matrix33::CreateRotationY(Word2Degr(pMod->m_Rot[1]) * PI / 180.0f);
            }
            if (pMod->m_Rot[2])
            {
                pMod->m_TexMatrix = pMod->m_TexMatrix * Matrix33::CreateRotationZ(Word2Degr(pMod->m_Rot[2]) * PI / 180.0f);
            }

            pMod->m_TexMatrix = pMod->m_TexMatrix *
                Matrix44(su, 0, 0, 0,
                    0, sv, 0, 0,
                    0, 0, 1, 0,
                    du, dv, 0, 1);
        }
    }
    else
    {   // This can be avoided - why would you have an empty modulator?
        pMod->m_TexMatrix.SetIdentity();
    }

    if (pMod->m_eTGType != ETG_Stream)
    {
        switch (pMod->m_eTGType)
        {
        case ETG_World:
        {
            m_Ext.m_nUpdateFlags |= GetTextCoordGenObjLinearFlag(nTSlot);
            for (int i = 0; i < 4; i++)
            {
                memset(&Pl, 0, sizeof(Pl));
                float* fPl = (float*)&Pl;
                fPl[i] = 1.0f;
                if (rd->m_RP.m_pCurObject)
                {
                    PlTr = TransformPlane2_NoTrans(Matrix44A(rd->m_RP.m_pCurObject->m_II.m_Matrix).GetTransposed(), Pl);
                }
                else
                {
                    // LY-60094 - TexGenType of "World" will give incorrect results
                    AZ_Warning("Rendering", false, "Warning: Material has TexGenType of 'World', but the requested object is unavailable while generating the TexGen Matrix.  Results may be incorrect.");
                    PlTr = TransformPlane2_NoTrans(Matrix44A(rd->m_RP.m_pIdendityRenderObject->m_II.m_Matrix).GetTransposed(), Pl);
                }
                pMod->m_TexGenMatrix(i, 0) = PlTr.n.x;
                pMod->m_TexGenMatrix(i, 1) = PlTr.n.y;
                pMod->m_TexGenMatrix(i, 2) = PlTr.n.z;
                pMod->m_TexGenMatrix(i, 3) = PlTr.d;
            }
        }
        break;
        case ETG_Camera:
        {
            m_Ext.m_nUpdateFlags |= GetTextCoordGenObjLinearFlag(nTSlot);
            for (int i = 0; i < 4; i++)
            {
                memset(&Pl, 0, sizeof(Pl));
                float* fPl = (float*)&Pl;
                fPl[i] = 1.0f;
                PlTr = TransformPlane2_NoTrans(rd->m_ViewMatrix, Pl);
                pMod->m_TexGenMatrix(i, 0) = PlTr.n.x;
                pMod->m_TexGenMatrix(i, 1) = PlTr.n.y;
                pMod->m_TexGenMatrix(i, 2) = PlTr.n.z;
                pMod->m_TexGenMatrix(i, 3) = PlTr.d;
            }
        }
        break;
        }
    }

    if (bTr)
    {
        m_Ext.m_nUpdateFlags |= HWMD_TEXCOORD_MATRIX;
        pMod->m_TexMatrix(0, 3) = pMod->m_TexMatrix(0, 2);
        pMod->m_TexMatrix(1, 3) = pMod->m_TexMatrix(1, 2);
        pMod->m_TexMatrix(2, 3) = pMod->m_TexMatrix(2, 2);
    }


    if (pMod->m_bTexGenProjected)
    {
        m_Ext.m_nUpdateFlags |= HWMD_TEXCOORD_PROJ;
    }

    if (IsTextureModifierSupportedForTextureMap(static_cast<EEfResTextures>(nTSlot)))
    {
        rd->m_RP.m_FlagsShader_MD |= m_Ext.m_nUpdateFlags;
    }
}

//---------------------------------------------------------------------------
// Wave evaluator

float CShaderMan::EvalWaveForm(SWaveForm* wf)
{
    int val;

    float Amp;
    float Freq;
    float Phase;
    float Level;

    if (wf->m_Flags & WFF_LERP)
    {
        val = (int)(gRenDev->m_RP.m_TI[gRenDev->m_RP.m_nProcessThreadID].m_RealTime * 597.0f);
        val &= SRenderPipeline::sSinTableCount - 1;
        float fLerp = gRenDev->m_RP.m_tSinTable[val] * 0.5f + 0.5f;

        if (wf->m_Amp != wf->m_Amp1)
        {
            Amp = LERP(wf->m_Amp, wf->m_Amp1, fLerp);
        }
        else
        {
            Amp = wf->m_Amp;
        }

        if (wf->m_Freq != wf->m_Freq1)
        {
            Freq = LERP(wf->m_Freq, wf->m_Freq1, fLerp);
        }
        else
        {
            Freq = wf->m_Freq;
        }

        if (wf->m_Phase != wf->m_Phase1)
        {
            Phase = LERP(wf->m_Phase, wf->m_Phase1, fLerp);
        }
        else
        {
            Phase = wf->m_Phase;
        }

        if (wf->m_Level != wf->m_Level1)
        {
            Level = LERP(wf->m_Level, wf->m_Level1, fLerp);
        }
        else
        {
            Level = wf->m_Level;
        }
    }
    else
    {
        Level = wf->m_Level;
        Amp = wf->m_Amp;
        Phase = wf->m_Phase;
        Freq = wf->m_Freq;
    }

    switch (wf->m_eWFType)
    {
    case eWF_None:
        Warning("WARNING: CShaderMan::EvalWaveForm called with 'EWF_None' in Shader '%s'\n", gRenDev->m_RP.m_pShader->GetName());
        break;

    case eWF_Sin:
        val = (int)((gRenDev->m_RP.m_TI[gRenDev->m_RP.m_nProcessThreadID].m_RealTime * Freq + Phase) * (float)SRenderPipeline::sSinTableCount);
        return Amp * gRenDev->m_RP.m_tSinTable[val & (SRenderPipeline::sSinTableCount - 1)] + Level;

    // Other wave types aren't supported anymore
    case eWF_HalfSin:
        Warning("WARNING: CShaderMan::EvalWaveForm: bad WaveType '%d' in Shader '%s'\n", wf->m_eWFType, gRenDev->m_RP.m_pShader->GetName());
        assert(0);
        return 0;

    case eWF_InvHalfSin:
        Warning("WARNING: CShaderMan::EvalWaveForm: bad WaveType '%d' in Shader '%s'\n", wf->m_eWFType, gRenDev->m_RP.m_pShader->GetName());
        assert(0);
        return 0;

    case eWF_SawTooth:
        Warning("WARNING: CShaderMan::EvalWaveForm: bad WaveType '%d' in Shader '%s'\n", wf->m_eWFType, gRenDev->m_RP.m_pShader->GetName());
        assert(0);
        return 0;

    case eWF_InvSawTooth:
        Warning("WARNING: CShaderMan::EvalWaveForm: bad WaveType '%d' in Shader '%s'\n", wf->m_eWFType, gRenDev->m_RP.m_pShader->GetName());
        assert(0);
        return 0;

    case eWF_Square:
        Warning("WARNING: CShaderMan::EvalWaveForm: bad WaveType '%d' in Shader '%s'\n", wf->m_eWFType, gRenDev->m_RP.m_pShader->GetName());
        assert(0);
        return 0;

    case eWF_Triangle:
        Warning("WARNING: CShaderMan::EvalWaveForm: bad WaveType '%d' in Shader '%s'\n", wf->m_eWFType, gRenDev->m_RP.m_pShader->GetName());
        assert(0);
        return 0;

    case eWF_Hill:
        Warning("WARNING: CShaderMan::EvalWaveForm: bad WaveType '%d' in Shader '%s'\n", wf->m_eWFType, gRenDev->m_RP.m_pShader->GetName());
        assert(0);
        return 0;

    case eWF_InvHill:
        Warning("WARNING: CShaderMan::EvalWaveForm: bad WaveType '%d' in Shader '%s'\n", wf->m_eWFType, gRenDev->m_RP.m_pShader->GetName());
        assert(0);
        return 0;

    default:
        Warning("WARNING: CShaderMan::EvalWaveForm: bad WaveType '%d' in Shader '%s'\n", wf->m_eWFType, gRenDev->m_RP.m_pShader->GetName());
        break;
    }
    return 1;
}

float CShaderMan::EvalWaveForm(SWaveForm2* wf)
{
    int val;

    switch (wf->m_eWFType)
    {
    case eWF_None:
        //Warning( 0,0,"WARNING: CShaderMan::EvalWaveForm called with 'EWF_None' in Shader '%s'\n", gRenDev->m_RP.m_pShader->GetName());
        break;

    case eWF_Sin:
        val = (int)((gRenDev->m_RP.m_TI[gRenDev->m_RP.m_nProcessThreadID].m_RealTime * wf->m_Freq + wf->m_Phase) * (float)SRenderPipeline::sSinTableCount);
        return wf->m_Amp * gRenDev->m_RP.m_tSinTable[val & (SRenderPipeline::sSinTableCount - 1)] + wf->m_Level;

    // Other wave types aren't supported anymore
    case eWF_HalfSin:
        Warning("WARNING: CShaderMan::EvalWaveForm: bad WaveType '%d' in Shader '%s'\n", wf->m_eWFType, gRenDev->m_RP.m_pShader->GetName());
        assert(0);
        return 0;

    case eWF_InvHalfSin:
        Warning("WARNING: CShaderMan::EvalWaveForm: bad WaveType '%d' in Shader '%s'\n", wf->m_eWFType, gRenDev->m_RP.m_pShader->GetName());
        assert(0);
        return 0;

    case eWF_SawTooth:
        Warning("WARNING: CShaderMan::EvalWaveForm: bad WaveType '%d' in Shader '%s'\n", wf->m_eWFType, gRenDev->m_RP.m_pShader->GetName());
        assert(0);
        return 0;

    case eWF_InvSawTooth:
        Warning("WARNING: CShaderMan::EvalWaveForm: bad WaveType '%d' in Shader '%s'\n", wf->m_eWFType, gRenDev->m_RP.m_pShader->GetName());
        assert(0);
        return 0;

    case eWF_Square:
        Warning("WARNING: CShaderMan::EvalWaveForm: bad WaveType '%d' in Shader '%s'\n", wf->m_eWFType, gRenDev->m_RP.m_pShader->GetName());
        assert(0);
        return 0;

    case eWF_Triangle:
        Warning("WARNING: CShaderMan::EvalWaveForm: bad WaveType '%d' in Shader '%s'\n", wf->m_eWFType, gRenDev->m_RP.m_pShader->GetName());
        assert(0);
        return 0;

    case eWF_Hill:
        Warning("WARNING: CShaderMan::EvalWaveForm: bad WaveType '%d' in Shader '%s'\n", wf->m_eWFType, gRenDev->m_RP.m_pShader->GetName());
        assert(0);
        return 0;

    case eWF_InvHill:
        Warning("WARNING: CShaderMan::EvalWaveForm: bad WaveType '%d' in Shader '%s'\n", wf->m_eWFType, gRenDev->m_RP.m_pShader->GetName());
        assert(0);
        return 0;

    default:
        Warning("WARNING: CShaderMan::EvalWaveForm: bad WaveType '%d' in Shader '%s'\n", wf->m_eWFType, gRenDev->m_RP.m_pShader->GetName());
        break;
    }
    return 1;
}

float CShaderMan::EvalWaveForm2(SWaveForm* wf, float frac)
{
    int val;

    if (!(wf->m_Flags & WFF_CLAMP))
    {
        switch (wf->m_eWFType)
        {
        case eWF_None:
            Warning("CShaderMan::EvalWaveForm2 called with 'EWF_None' in Shader '%s'\n", gRenDev->m_RP.m_pShader->GetName());
            break;

        case eWF_Sin:
            val = QRound((frac * wf->m_Freq + wf->m_Phase) * (float)SRenderPipeline::sSinTableCount);
            val &= (SRenderPipeline::sSinTableCount - 1);
            return wf->m_Amp * gRenDev->m_RP.m_tSinTable[val] + wf->m_Level;

        // Other wave types aren't supported anymore
        case eWF_SawTooth:
            Warning("Warning: CShaderMan::EvalWaveForm2: bad EWF '%d' in Shader '%s'\n", wf->m_eWFType, gRenDev->m_RP.m_pShader->GetName());
            assert(0);
            return 0;

        case eWF_InvSawTooth:
            Warning("Warning: CShaderMan::EvalWaveForm2: bad EWF '%d' in Shader '%s'\n", wf->m_eWFType, gRenDev->m_RP.m_pShader->GetName());
            assert(0);
            return 0;

        case eWF_Square:
            Warning("Warning: CShaderMan::EvalWaveForm2: bad EWF '%d' in Shader '%s'\n", wf->m_eWFType, gRenDev->m_RP.m_pShader->GetName());
            assert(0);
            return 0;

        case eWF_Triangle:
            Warning("Warning: CShaderMan::EvalWaveForm2: bad EWF '%d' in Shader '%s'\n", wf->m_eWFType, gRenDev->m_RP.m_pShader->GetName());
            assert(0);
            return 0;

        case eWF_Hill:
            Warning("Warning: CShaderMan::EvalWaveForm2: bad EWF '%d' in Shader '%s'\n", wf->m_eWFType, gRenDev->m_RP.m_pShader->GetName());
            assert(0);
            return 0;

        case eWF_InvHill:
            Warning("Warning: CShaderMan::EvalWaveForm2: bad EWF '%d' in Shader '%s'\n", wf->m_eWFType, gRenDev->m_RP.m_pShader->GetName());
            assert(0);
            return 0;

        default:
            Warning("Warning: CShaderMan::EvalWaveForm2: bad EWF '%d' in Shader '%s'\n", wf->m_eWFType, gRenDev->m_RP.m_pShader->GetName());
            break;
        }
    }
    else
    {
        switch (wf->m_eWFType)
        {
        case eWF_None:
            Warning("Warning: CShaderMan::EvalWaveForm2 called with 'EWF_None' in Shader '%s'\n", gRenDev->m_RP.m_pShader->GetName());
            break;

        case eWF_Sin:
            val = QRound((frac * wf->m_Freq + wf->m_Phase) * (float)SRenderPipeline::sSinTableCount);
            val &= (SRenderPipeline::sSinTableCount - 1);
            return wf->m_Amp * gRenDev->m_RP.m_tSinTable[val] + wf->m_Level;

        // Other wave types aren't supported anymore
        case eWF_SawTooth:
            Warning("Warning: CShaderMan::EvalWaveForm2: bad EWF '%d' in Shader '%s'\n", wf->m_eWFType, gRenDev->m_RP.m_pShader->GetName());
            assert(0);
            return 0;

        case eWF_InvSawTooth:
            Warning("Warning: CShaderMan::EvalWaveForm2: bad EWF '%d' in Shader '%s'\n", wf->m_eWFType, gRenDev->m_RP.m_pShader->GetName());
            assert(0);
            return 0;

        case eWF_Square:
            Warning("Warning: CShaderMan::EvalWaveForm2: bad EWF '%d' in Shader '%s'\n", wf->m_eWFType, gRenDev->m_RP.m_pShader->GetName());
            assert(0);
            return 0;

        case eWF_Triangle:
            Warning("Warning: CShaderMan::EvalWaveForm2: bad EWF '%d' in Shader '%s'\n", wf->m_eWFType, gRenDev->m_RP.m_pShader->GetName());
            assert(0);
            return 0;

        case eWF_Hill:
            Warning("Warning: CShaderMan::EvalWaveForm2: bad EWF '%d' in Shader '%s'\n", wf->m_eWFType, gRenDev->m_RP.m_pShader->GetName());
            assert(0);
            return 0;

        case eWF_InvHill:
            Warning("Warning: CShaderMan::EvalWaveForm2: bad EWF '%d' in Shader '%s'\n", wf->m_eWFType, gRenDev->m_RP.m_pShader->GetName());
            assert(0);
            return 0;

        default:
            Warning("Warning: CShaderMan::EvalWaveForm2: bad EWF '%d' in Shader '%s'\n", wf->m_eWFType, gRenDev->m_RP.m_pShader->GetName());
            break;
        }
    }
    return 1;
}


void CShaderMan::mfBeginFrame()
{
    LOADING_TIME_PROFILE_SECTION;
}

void CHWShader::mfCleanupCache()
{
    FXShaderCacheItor FXitor;
    for (FXitor = m_ShaderCache.begin(); FXitor != m_ShaderCache.end(); FXitor++)
    {
        SShaderCache* sc = FXitor->second;
        if (!sc)
        {
            continue;
        }
        sc->Cleanup();
    }
    assert (CResFile::m_nNumOpenResources == 0);
    CResFile::m_nMaxOpenResFiles = 4;
}

EHWShaderClass CHWShader::mfStringProfile(const char* profile)
{
    EHWShaderClass Profile = eHWSC_Num;
    if (!strncmp(profile, "vs_5_0", 6) || !strncmp(profile, "vs_4_0", 6) || !strncmp(profile, "vs_3_0", 6))
    {
        Profile = eHWSC_Vertex;
    }
    else
    if (!strncmp(profile, "ps_5_0", 6) || !strncmp(profile, "ps_4_0", 6) || !strncmp(profile, "ps_3_0", 6))
    {
        Profile = eHWSC_Pixel;
    }
    else
    if (!strncmp(profile, "gs_5_0", 6) || !strncmp(profile, "gs_4_0", 6))
    {
        Profile = eHWSC_Geometry;
    }
    else
    if (!strncmp(profile, "hs_5_0", 6))
    {
        Profile = eHWSC_Hull;
    }
    else
    if (!strncmp(profile, "ds_5_0", 6))
    {
        Profile = eHWSC_Domain;
    }
    else
    if (!strncmp(profile, "cs_5_0", 6))
    {
        Profile = eHWSC_Compute;
    }
    else
    {
        assert(0);
    }

    return Profile;
}
EHWShaderClass CHWShader::mfStringClass(const char* szClass)
{
    EHWShaderClass Profile = eHWSC_Num;
    if (!_strnicmp(szClass, "VS", 2))
    {
        Profile = eHWSC_Vertex;
    }
    else
    if (!_strnicmp(szClass, "PS", 2))
    {
        Profile = eHWSC_Pixel;
    }
    else
    if (!_strnicmp(szClass, "GS", 2))
    {
        Profile = eHWSC_Geometry;
    }
    else
    if (!_strnicmp(szClass, "HS", 2))
    {
        Profile = eHWSC_Hull;
    }
    else
    if (!_strnicmp(szClass, "DS", 2))
    {
        Profile = eHWSC_Domain;
    }
    else
    if (!_strnicmp(szClass, "CS", 2))
    {
        Profile = eHWSC_Compute;
    }
    else
    {
        assert(0);
    }

    return Profile;
}
const char* CHWShader::mfProfileString(EHWShaderClass eClass)
{
    const char* szProfile = "Unknown";
    switch (eClass)
    {
    case eHWSC_Vertex:
        szProfile = "vs_5_0";
        break;
    case eHWSC_Pixel:
        szProfile = "ps_5_0";
        break;
    case eHWSC_Geometry:
        if (CParserBin::m_nPlatform == SF_D3D11 || CParserBin::m_nPlatform == SF_ORBIS || CParserBin::m_nPlatform == SF_JASPER || CParserBin::m_nPlatform == SF_GL4)
        {
            szProfile = "gs_5_0";
        }
        else
        {
            assert(0);
        }
        break;
    case eHWSC_Domain:
        if (CParserBin::m_nPlatform == SF_D3D11 || CParserBin::m_nPlatform == SF_ORBIS || CParserBin::m_nPlatform == SF_JASPER || CParserBin::m_nPlatform == SF_GL4)
        {
            szProfile = "ds_5_0";
        }
        else
        {
            assert(0);
        }
        break;
    case eHWSC_Hull:
        if (CParserBin::m_nPlatform == SF_D3D11 || CParserBin::m_nPlatform == SF_ORBIS || CParserBin::m_nPlatform == SF_JASPER || CParserBin::m_nPlatform == SF_GL4)
        {
            szProfile = "hs_5_0";
        }
        else
        {
            assert(0);
        }
        break;
    case eHWSC_Compute:
        if (CParserBin::m_nPlatform == SF_D3D11 || CParserBin::m_nPlatform == SF_ORBIS || CParserBin::m_nPlatform == SF_JASPER || CParserBin::m_nPlatform == SF_GL4 || CParserBin::m_nPlatform == SF_METAL || CParserBin::m_nPlatform == SF_GLES3)
        {
            szProfile = "cs_5_0";
        }
        else
        {
            assert(0);
        }
        break;
    default:
        assert(0);
    }
    return szProfile;
}
const char* CHWShader::mfClassString(EHWShaderClass eClass)
{
    const char* szClass = "Unknown";
    switch (eClass)
    {
    case eHWSC_Vertex:
        szClass = "VS";
        break;
    case eHWSC_Pixel:
        szClass = "PS";
        break;
    case eHWSC_Geometry:
        szClass = "GS";
        break;
    case eHWSC_Domain:
        szClass = "DS";
        break;
    case eHWSC_Hull:
        szClass = "HS";
        break;
    case eHWSC_Compute:
        szClass = "CS";
        break;
    default:
        assert(0);
    }
    return szClass;
}

//==========================================================================================================

inline bool sCompareRes(CShaderResources* a, CShaderResources* b)
{
    if (!a || !b)
    {
        return a < b;
    }

    if (a->m_AlphaRef != b->m_AlphaRef)
    {
        return a->m_AlphaRef < b->m_AlphaRef;
    }

    if (a->GetStrengthValue(EFTT_OPACITY) != b->GetStrengthValue(EFTT_OPACITY))
    {
        return a->GetStrengthValue(EFTT_OPACITY) < b->GetStrengthValue(EFTT_OPACITY);
    }

    if (a->m_pDeformInfo != b->m_pDeformInfo)
    {
        return a->m_pDeformInfo < b->m_pDeformInfo;
    }

    if (a->m_RTargets.Num() != b->m_RTargets.Num())
    {
        return a->m_RTargets.Num() < b->m_RTargets.Num();
    }

    if ((a->m_ResFlags & MTL_FLAG_2SIDED) != (b->m_ResFlags & MTL_FLAG_2SIDED))
    {
        return (a->m_ResFlags & MTL_FLAG_2SIDED) < (b->m_ResFlags & MTL_FLAG_2SIDED);
    }

    // [Shader System TO DO] - optimize - should be data driven based on marked slots only

    uint16      testSlots[] = { 
        EFTT_SPECULAR , 
        EFTT_DIFFUSE , 
        EFTT_NORMALS ,
        EFTT_ENV , 
        EFTT_DECAL_OVERLAY , 
        EFTT_DETAIL_OVERLAY , 
        EFTT_MAX 
    };

    ITexture        *pTA = nullptr, *pTB = nullptr;
    for (int i=0 ; testSlots[i] != EFTT_MAX; i++)
    {
        uint16          texSlot = testSlots[i];
        SEfResTexture*  pTexResA = a->GetTextureResource(texSlot);
        SEfResTexture*  pTexResB = b->GetTextureResource(texSlot);
        pTA = pTexResA ? pTexResA->m_Sampler.m_pITex : nullptr;
        pTB = pTexResB ? pTexResB->m_Sampler.m_pITex : nullptr;

        if (pTA != pTB)
        {
            return pTA < pTB;
        }
    }
    return (pTA < pTB);
}

inline bool sIdenticalRes(CShaderResources* a, CShaderResources* b)
{
    if (a->m_AlphaRef != b->m_AlphaRef)
    {
        return false;
    }

    if (a->GetStrengthValue(EFTT_OPACITY) != b->GetStrengthValue(EFTT_OPACITY))
    {
        return false;
    }

    if (a->m_pDeformInfo != b->m_pDeformInfo)
    {
        return false;
    }

    if (a->m_RTargets.Num() != b->m_RTargets.Num())
    {
        return false;
    }

    if ((a->m_ResFlags & (MTL_FLAG_2SIDED | MTL_FLAG_ADDITIVE)) != (b->m_ResFlags & (MTL_FLAG_2SIDED | MTL_FLAG_ADDITIVE)))
    {
        return false;
    }

    // [Shader System TO DO] - revisit this comparison!!!
    for (int texSlot = 0; texSlot < EFTT_MAX; texSlot++)
    {
        SEfResTexture*      pTextureA = a->GetTextureResource(texSlot);
        if (pTextureA && pTextureA->IsHasModificators())
            return false;

        SEfResTexture*      pTextureB = b->GetTextureResource(texSlot);
        if (pTextureB && pTextureB->IsHasModificators())
            return false;

        // [Shader System] - To Do - test and add this for being more correct 
        // Finally compare the pointer to the texture resource itself
//        if (pTextureA->m_Sampler.m_pITex != pTextureB->m_Sampler.m_pITex)     
//            return false;
    }

    const float emissiveIntensity = a->GetStrengthValue(EFTT_EMITTANCE);
    if (emissiveIntensity != b->GetStrengthValue(EFTT_EMITTANCE))
    {
        return false;
    }
    if (emissiveIntensity > 0)
    {
        if (a->GetColorValue(EFTT_EMITTANCE) != b->GetColorValue(EFTT_EMITTANCE))
        {
            return false;
        }
    }

    return true;
}

inline bool sCompareShd(CBaseResource* a, CBaseResource* b)
{
    if (!a || !b)
    {
        return a < b;
    }

    CShader* pA = (CShader*)a;
    CShader* pB = (CShader*)b;
    SShaderPass* pPA = NULL;
    SShaderPass* pPB = NULL;
    if (pA->m_HWTechniques.Num() && pA->m_HWTechniques[0]->m_Passes.Num())
    {
        pPA = &pA->m_HWTechniques[0]->m_Passes[0];
    }
    if (pB->m_HWTechniques.Num() && pB->m_HWTechniques[0]->m_Passes.Num())
    {
        pPB = &pB->m_HWTechniques[0]->m_Passes[0];
    }
    if (!pPA || !pPB)
    {
        return pPA < pPB;
    }

    if (pPA->m_VShader != pPB->m_VShader)
    {
        return pPA->m_VShader < pPB->m_VShader;
    }
    return (pPA->m_PShader < pPB->m_PShader);
}

void CShaderMan::mfSortResources()
{
    uint32 i;
    for (i = 0; i < MAX_TMU; i++)
    {
        gRenDev->m_RP.m_ShaderTexResources[i] = NULL;
    }
    iLog->Log("-- Presort shaders by states...");
    //return;
    std::sort(&CShader::s_ShaderResources_known.begin()[1], CShader::s_ShaderResources_known.end(), sCompareRes);

    int                 nGroups = 20000;
    CShaderResources*   pPrev = nullptr;

    // Now that the shader resources has been sorted, run over them and create groups of 
    // identical resources.
    for (i = 1; i < CShader::s_ShaderResources_known.Num(); i++)
    {
        CShaderResources* pRes = CShader::s_ShaderResources_known[i];
        if (pRes)
        {
            pRes->m_Id = i;
            pRes->m_IdGroup = i;
            if IsCVarConstAccess(constexpr) (CRenderer::CV_r_materialsbatching)
            {
                if (pPrev)
                {
                    if (!sIdenticalRes(pRes, pPrev))
                    {
                        nGroups++;
                    }
                }
                pRes->m_IdGroup = nGroups;
            }
            pPrev = pRes;
        }
    }
    iLog->Log("--- [Shaders System] : %d Shaders Resources, %d Shaders Resource groups.", CShader::s_ShaderResources_known.Num(), nGroups - 20000);

    // now run over the list of active (compiled binary) shaders
    {
        AUTO_LOCK(CBaseResource::s_cResLock);

        SResourceContainer* pRL = CShaderMan::s_pContainer;
        assert(pRL);
        if (pRL)
        {
            std::sort(pRL->m_RList.begin(), pRL->m_RList.end(), sCompareShd);
            for (i = 0; i < pRL->m_RList.size(); i++)
            {
                CShader* pSH = (CShader*)pRL->m_RList[i];
                if (pSH)
                {
                    pSH->SetID(CBaseResource::IdFromRListIndex(i));
                }
            }

            pRL->m_AvailableIDs.clear();
            for (i = 0; i < pRL->m_RList.size(); i++)
            {
                CShader* pSH = (CShader*)pRL->m_RList[i];
                if (!pSH)
                {
                    pRL->m_AvailableIDs.push_back(CBaseResource::IdFromRListIndex(i));
                }
            }
        }
    }
}

//---------------------------------------------------------------------------

void CLightStyle::mfUpdate(float fTime)
{
    float m = fTime * m_TimeIncr;
    //    if (m != m_LastTime)
    {
        m_LastTime = m;
        if (m_Map.Num())
        {
            if (m_Map.Num() == 1)
            {
                m_Color = m_Map[0].cColor;
            }
            else
            {
                int first = (int)QInt(m);
                int second = (first + 1);
                float fLerp = m - (float)first;

                // Interpolate between key-frames
                // todo: try different interpolation method

                ColorF& cColA = m_Map[first % m_Map.Num()].cColor;
                ColorF& cColB = m_Map[second % m_Map.Num()].cColor;
                m_Color = LERP(cColA, cColB, fLerp);

                Vec3& vPosA = m_Map[first % m_Map.Num()].vPosOffset;
                Vec3& vPosB = m_Map[second % m_Map.Num()].vPosOffset;
                m_vPosOffset = LERP(vPosA, vPosB, fLerp);
            }
        }
    }
}
