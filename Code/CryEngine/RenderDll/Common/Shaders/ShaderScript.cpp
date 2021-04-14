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
#include "CryHeaders.h"
#include "../Common/Shaders/RemoteCompiler.h"
#include <MainThreadRenderRequestBus.h>
#include <Pak/CryPakUtils.h>

#include <IMaterial.h>

#ifndef NULL_RENDERER
#include "DriverD3D.h"
#endif

#if defined(WIN32) || defined(WIN64)
#include <direct.h>
#include <io.h>
#endif

extern char* gShObjectNotFound;

//=================================================================================================

bool CShader::Reload(int nFlags, const char* szShaderName)
{
    CShader* pShaderGen = NULL;
    if (m_ShaderGenParams)
    {
        pShaderGen = this;
    }
    else
    if (m_pGenShader)
    {
        pShaderGen = m_pGenShader;
    }
    uint32 nFl = EF_RELOAD;
    if (nFlags & FRO_FORCERELOAD)
    {
        nFl |= EF_FORCE_RELOAD;
    }
    if (pShaderGen && pShaderGen->m_DerivedShaders)
    {
        for (uint32 i = 0; i < pShaderGen->m_DerivedShaders->size(); i++)
        {
            CShader* pShader = (*pShaderGen->m_DerivedShaders)[i];
            if (!pShader)
            {
                continue;
            }
            if (pShader->m_nRefreshFrame == gRenDev->m_cEF.m_nFrameForceReload)
            {
                continue;
            }
            pShader->m_nRefreshFrame = gRenDev->m_cEF.m_nFrameForceReload;

            gRenDev->m_cEF.mfForName(szShaderName, pShader->m_Flags | nFl, NULL, pShader->m_nMaskGenFX);
        }
    }
    else
    {
        assert(!m_nMaskGenFX);
        gRenDev->m_cEF.mfForName(szShaderName, m_Flags | nFl, NULL, m_nMaskGenFX);
    }

    return true;
}

bool CShaderMan::mfReloadShaderIncludes(const char* szPath, int nFlags)
{
    assert(szPath);

    bool bChanged = false;
    char dirn[256];
    cry_strcpy(dirn, szPath);
    cry_strcat(dirn, "*");
    AZ::IO::ArchiveFileIterator handle = gEnv->pCryPak->FindFirst (dirn);
    if (handle)
    {
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

                bChanged = mfReloadShaderIncludes(ddd, nFlags);
                continue;
            }
            char nmf[256];
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
            if (!azstricmp(&nmf[len], ".cfi"))
            {
                fpStripExtension(handle.m_filename.data(), nmf);
                bool bCh = false;
                m_Bin.GetBinShader(nmf, true, 0, &bCh);
                if (bCh)
                {
                    bChanged = true;
                    mfReloadFile(szPath, handle.m_filename.data(), nFlags);
                }
            }
        } while (handle = gEnv->pCryPak->FindNext(handle));

        gEnv->pCryPak->FindClose (handle);
    }
    return bChanged;
}

bool CShaderMan::mfReloadAllShaders([[maybe_unused]] int nFlags, [[maybe_unused]] uint32 nFlagsHW)
{
    bool bState = true;
    m_nFrameForceReload++;

    gRenDev->FlushRTCommands(true, true, true);
    m_Bin.InvalidateCache();
    CHWShader::mfFlushPendedShadersWait(-1);
    
#ifndef NULL_RENDERER
    // Ensure all shaders are unbound before forcing a reload of all shaders
    gRenDev->m_pRT->RC_UnbindResources();
    if (!gRenDev->IsShaderCacheGenMode())
    {
        gRenDev->m_pRT->RC_ResetToDefault();
    }
    gRenDev->FlushRTCommands(true, true, true);
    
    CDebugAllowFileAccess ignoreInvalidFileAccess;

    // Check include changing
    if IsCVarConstAccess(constexpr) (!CRenderer::CV_r_shadersignoreincludeschanging)
    {
        mfReloadShaderIncludes(m_ShadersPath.c_str(), nFlags);
    }
    CCryNameTSCRC Name = CShader::mfGetClassName();
    SResourceContainer* pRL = CBaseResource::GetResourcesForClass(Name);
    if (pRL)
    {
        ResourcesMapItor itor;
        for (itor = pRL->m_RMap.begin(); itor != pRL->m_RMap.end(); itor++)
        {
            CShader* pS = (CShader*)itor->second;
            if (!pS)
            {
                continue;
            }
            if (nFlagsHW)
            {
                if (!pS->m_pGenShader)
                {
                    continue;
                }
                SShaderGen* pGen = pS->m_pGenShader->m_ShaderGenParams;
                assert(pGen);
                if (!pGen)
                {
                    continue;
                }
                uint32 i;
                SShaderGenBit* pBit;
                for (i = 0; i < pGen->m_BitMask.Num(); i++)
                {
                    pBit = pGen->m_BitMask[i];
                    if ((pBit->m_nDependencySet | pBit->m_nDependencyReset) & nFlagsHW)
                    {
                        break;
                    }
                }
                if (i == pGen->m_BitMask.Num())
                {
                    continue;
                }
                pS->Reload(nFlags | FRO_FORCERELOAD, pS->GetName());
            }
            else
            {
                char name[256];
                sprintf_s(name, "%sCryFX/%s.cfx", m_ShadersPath.c_str(), pS->GetName());
                AZ::IO::HandleType fileHandle = gEnv->pCryPak->FOpen(name, "rb");
                if (fileHandle != AZ::IO::InvalidHandle)
                {
                    uint32 nSourceCRC32 = gEnv->pCryPak->ComputeCRC(name);
                    gEnv->pCryPak->FClose(fileHandle);
                    if ((nFlags & FRO_FORCERELOAD) || nSourceCRC32 != pS->m_SourceCRC32)
                    {
                        pS->m_SourceCRC32 = nSourceCRC32;
                        pS->Reload(nFlags, pS->GetName());
                    }
                }
            }
        }
    }

    //Force all PSOs to be rebuilt
    CDeviceObjectFactory::GetInstance().InvalidatePSOCache();
    //Tell the graphics pipeline to reset and throw out existing PSOs since they're now invalid
    gcpRendD3D->GetGraphicsPipeline().Reset();

#endif

    gRenDev->FlushRTCommands(true, true, true);
    CHWShader::mfFlushPendedShadersWait(-1);

    return bState;
}

// Recursively iterate through all the shaders included by pBin to see if any match szShaderName
// Returns true if there is a match, meaning pBin is affected by szShaderName.
static bool sCheckAffecting_r(SShaderBin* pBin, const char* szShaderName)
{
    pBin->Lock();
    int nTok = 0;
    // Check first level
    while (nTok >= 0)
    {
        // For each include that is found, check to see if it matches szShaderName
        nTok = CParserBin::FindToken(nTok, pBin->m_Tokens.size() - 1, &pBin->m_Tokens[0], eT_include);
        if (nTok >= 0)
        {
            // If an include was found, check to see if it matches szShaderName
            nTok++;
            uint32 nTokName = pBin->m_Tokens[nTok];
            const char* szNameInc = CParserBin::GetString(nTokName, pBin->m_TokenTable);
            if (!azstricmp(szNameInc, szShaderName))
            {
                // If szShaderName matches the included shader, then pBin is affected by that shader
                // Since nTok is >= 0, breaking here will result in returning 'true'
                break;
            }
        }
    }
    // Check each of the included shaders recursively
    if (nTok < 0)
    {
        nTok = 0;
        while (nTok >= 0)
        {
            nTok = CParserBin::FindToken(nTok, pBin->m_Tokens.size() - 1, &pBin->m_Tokens[0], eT_include);
            if (nTok >= 0)
            {
                // For each include that is found, check to see if it matches szShaderName
                nTok++;
                uint32 nTokName = pBin->m_Tokens[nTok];
                const char* szNameInc = CParserBin::GetString(nTokName, pBin->m_TokenTable);
                if (!azstricmp(szNameInc, szShaderName))
                {
                    // If szShaderName matches the included shader, then pBin is affected by that shader
                    // Since nTok is >= 0, breaking here will result in returning 'true'
                    break;
                }
                SShaderBin* pBinIncl = gRenDev->m_cEF.m_Bin.GetBinShader(szNameInc, true, 0);
                if (!pBinIncl)
                {
                    AZ_Assert(false, "Error attempting to load shader %s while checking all the shaders included by %s.", szNameInc, pBin->m_szName);
                    break;
                }
                // Since the included file did not match szShaderName, recursively check all of its included shaders to see if they match
                bool bAffect = sCheckAffecting_r(pBinIncl, szShaderName);
                if (bAffect)
                {
                    // One of the included shaders matched szShaderName
                    // Since nTok is >= 0, breaking here will result in returning 'true'
                    break;
                }
            }
        }
    }
    pBin->Unlock();
    // Return true if a shader was found that matched szShaderName, false otherwise
    return (nTok >= 0);
}

bool CShaderMan::mfReloadFile([[maybe_unused]] const char* szPath, const char* szName, int nFlags)
{
    CHWShader::mfFlushPendedShadersWait(-1);

    m_nFrameForceReload++;

    const char* szExt = fpGetExtension(szName);
    if (!azstricmp(szExt, ".cfx"))
    {
        m_bReload = true;
        char szShaderName[256];
        cry_strcpy(szShaderName, szName, (size_t)(szExt - szName));
        azstrlwr(szShaderName, AZ_ARRAY_SIZE(szShaderName));

        // Check if this shader already loaded
        CBaseResource* pBR = CBaseResource::GetResource(CShader::mfGetClassName(), szShaderName, false);
        if (pBR)
        {
            CShader* pShader = (CShader*)pBR;
            pShader->Reload(nFlags, szShaderName);
        }
        m_bReload = false;
    }
    else
    if (!azstricmp(szExt, ".cfi"))
    {
        CCryNameTSCRC Name = CShader::mfGetClassName();
        SResourceContainer* pRL = CBaseResource::GetResourcesForClass(Name);
        if (pRL)
        {
            m_bReload = true;
            char szShaderName[256];
            cry_strcpy(szShaderName, szName, (size_t)(szExt - szName));
            azstrlwr(szShaderName, AZ_ARRAY_SIZE(szShaderName));
            SShaderBin* pBin = m_Bin.GetBinShader(szShaderName, true, 0);
            bool bAffect = false;

            // Since this is a .cfi file, iterate through the existing resources to see if any of them are including it and consequently must be re-loaded
            ResourcesMapItor itor;
            for (itor = pRL->m_RMap.begin(); itor != pRL->m_RMap.end(); itor++)
            {
                CShader* sh = (CShader*)itor->second;
                if (!sh || !sh->GetName()[0])
                {
                    continue;
                }
                pBin = m_Bin.GetBinShader(sh->GetName(), false, 0);
                if (!pBin)
                {
                    continue;
                }

                // Recursively check to see if sh is affected by the .cfi that is being reloaded
                bAffect = sCheckAffecting_r(pBin, szShaderName);
                if (bAffect)
                {
                    // If sh is affected, it also needs to be reloaded
                    sh->Reload(nFlags | FRO_FORCERELOAD, sh->GetName());
                }
            }
        }
        m_bReload = false;
    }


    return false;
}

//=============================================================================

void CShaderMan::mfFillGenMacroses(SShaderGen* shG, TArray<char>& buf, uint64 nMaskGen)
{
    uint32 i;

    if (!nMaskGen || !shG)
    {
        return;
    }

    for (i = 0; i < shG->m_BitMask.Num(); i++)
    {
        if (shG->m_BitMask[i]->m_Mask & nMaskGen)
        {
            char macro[256];
#if defined(__GNUC__)
            sprintf_s(macro, "#define %s 0x%llx\n", shG->m_BitMask[i]->m_ParamName.c_str(), (unsigned long long)shG->m_BitMask[i]->m_Mask);
#else
            sprintf_s(macro, "#define %s 0x%I64x\n", shG->m_BitMask[i]->m_ParamName.c_str(), (uint64)shG->m_BitMask[i]->m_Mask);
#endif
            int size = strlen(macro);
            int nOffs = buf.Num();
            buf.Grow(size);
            memcpy(&buf[nOffs], macro, size);
        }
    }
}


bool CShaderMan::mfModifyGenFlags(CShader* efGen, const CShaderResources* pRes, uint64& nMaskGen, uint64& nMaskGenH)
{
    if (!efGen || !efGen->m_ShaderGenParams)
    {
        return false;
    }
    uint64 nAndMaskHW = -1;
    uint64 nMaskGenHW = 0;

    // Remove non-used flags first
    unsigned int i = 0;
    SShaderGen* pGen = efGen->m_ShaderGenParams;
    int j;

    //char _debug_[256];
    //sprintf_s( _debug_, "curr shadergen %I64x %I64x\n", nMaskGen, nMaskGenH);
    //OutputDebugString(_debug_);

    if (nMaskGen)
    {
        for (j = 0; j < 64; j++)
        {
            uint64 nMask = (((uint64)1) << j);
            if (nMaskGen & nMask)
            {
                for (i = 0; i < pGen->m_BitMask.Num(); i++)
                {
                    SShaderGenBit* pBit = pGen->m_BitMask[i];
                    if (pBit->m_Mask & nMask)
                    {
                        break;
                    }
                }

                if (i == pGen->m_BitMask.Num())
                {
                    nMaskGen &= ~nMask;
                }
            }
        }
    }

    for (i = 0; i < pGen->m_BitMask.Num(); i++)
    {
        SShaderGenBit* pBit = pGen->m_BitMask[i];
        if (!pBit || (!pBit->m_nDependencySet && !pBit->m_nDependencyReset))
        {
            continue;
        }
        if (pRes)
        {
            if (pBit->m_nDependencySet & SHGD_TEX_NORMALS)
            {
                if (pRes->TextureSlotExists(EFTT_NORMALS))
                {
                    nMaskGen |= pBit->m_Mask;
                }
            }
            if (pBit->m_nDependencyReset & SHGD_TEX_NORMALS)
            {
                if (!pRes->TextureSlotExists(EFTT_NORMALS))
                {
                    nMaskGen &= ~pBit->m_Mask;
                }
            }
            if (pBit->m_nDependencySet & SHGD_TEX_HEIGHT)
            {
                if (pRes->TextureSlotExists(EFTT_HEIGHT))
                {
                    nMaskGen |= pBit->m_Mask;
                }
            }
            if (pBit->m_nDependencyReset & SHGD_TEX_HEIGHT)
            {
                if (!pRes->TextureSlotExists(EFTT_HEIGHT))
                {
                    nMaskGen &= ~pBit->m_Mask;
                }
            }

            if (pBit->m_nDependencySet & SHGD_TEX_DETAIL)
            {
                if (pRes->TextureSlotExists(EFTT_DETAIL_OVERLAY))
                {
                    nMaskGen |= pBit->m_Mask;
                }
            }
            if (pBit->m_nDependencyReset & SHGD_TEX_DETAIL)
            {
                if (!pRes->TextureSlotExists(EFTT_DETAIL_OVERLAY))
                {
                    nMaskGen &= ~pBit->m_Mask;
                }
            }

            if (pBit->m_nDependencySet & SHGD_TEX_SECOND_SMOOTHNESS)
            {
                if (pRes->TextureSlotExists(EFTT_SECOND_SMOOTHNESS))
                {
                    nMaskGen |= pBit->m_Mask;
                }
            }
            if (pBit->m_nDependencyReset & SHGD_TEX_SECOND_SMOOTHNESS)
            {
                if (!pRes->TextureSlotExists(EFTT_SECOND_SMOOTHNESS))
                {
                    nMaskGen &= ~pBit->m_Mask;
                }
            }

            if (pBit->m_nDependencySet & SHGD_TEX_SPECULAR)
            {
                if (pRes->TextureSlotExists(EFTT_SPECULAR))
                {
                    nMaskGen |= pBit->m_Mask;
                }
            }
            if (pBit->m_nDependencyReset & SHGD_TEX_SPECULAR)
            {
                if (!pRes->TextureSlotExists(EFTT_SPECULAR))
                {
                    nMaskGen &= ~pBit->m_Mask;
                }
            }

            if (pBit->m_nDependencySet & SHGD_TEX_ENVCM)
            {
                if (pRes->TextureSlotExists(EFTT_ENV))
                {
                    nMaskGen |= pBit->m_Mask;
                }
            }
            if (pBit->m_nDependencyReset & SHGD_TEX_ENVCM)
            {
                if (!pRes->TextureSlotExists(EFTT_ENV))
                {
                    nMaskGen &= ~pBit->m_Mask;
                }
            }

            if (pBit->m_nDependencySet & SHGD_TEX_SUBSURFACE)
            {
                if (pRes->TextureSlotExists(EFTT_SUBSURFACE))
                {
                    nMaskGen |= pBit->m_Mask;
                }
            }
            if (pBit->m_nDependencyReset & SHGD_TEX_SUBSURFACE)
            {
                if (!pRes->TextureSlotExists(EFTT_SUBSURFACE))
                {
                    nMaskGen &= ~pBit->m_Mask;
                }
            }

            if (pBit->m_nDependencySet & SHGD_TEX_DECAL)
            {
                if (pRes->TextureSlotExists(EFTT_DECAL_OVERLAY))
                {
                    nMaskGen |= pBit->m_Mask;
                }
            }
            if (pBit->m_nDependencyReset & SHGD_TEX_DECAL)
            {
                if (!pRes->TextureSlotExists(EFTT_DECAL_OVERLAY))
                {
                    nMaskGen &= ~pBit->m_Mask;
                }
            }

            if (pBit->m_nDependencySet & SHGD_TEX_CUSTOM)
            {
                if (pRes->TextureSlotExists(EFTT_CUSTOM))
                {
                    nMaskGen |= pBit->m_Mask;
                }
            }
            if (pBit->m_nDependencyReset & SHGD_TEX_CUSTOM)
            {
                if (!pRes->TextureSlotExists(EFTT_CUSTOM))
                {
                    nMaskGen &= ~pBit->m_Mask;
                }
            }

            if (pBit->m_nDependencySet & SHGD_TEX_CUSTOM_SECONDARY)
            {
                if (pRes->TextureSlotExists(EFTT_CUSTOM_SECONDARY))
                {
                    nMaskGen |= pBit->m_Mask;
                }
            }
            if (pBit->m_nDependencyReset & SHGD_TEX_CUSTOM_SECONDARY)
            {
                if (!pRes->TextureSlotExists(EFTT_CUSTOM_SECONDARY))
                {
                    nMaskGen &= ~pBit->m_Mask;
                }
            }

            if (pBit->m_nDependencySet & SHGD_TEX_SPECULAR_2)
            {
                if (pRes->TextureSlotExists(EFTT_SPECULAR_2))
                {
                    nMaskGen |= pBit->m_Mask;
                }
            }
            if (pBit->m_nDependencyReset & SHGD_TEX_SPECULAR_2)
            {
                if (!pRes->TextureSlotExists(EFTT_SPECULAR_2))
                {
                    nMaskGen &= ~pBit->m_Mask;
                }
            }

            if (pBit->m_nDependencySet & SHGD_TEX_EMITTANCE)
            {
                // If either the emittance or the decal overlay (emissive intensity) are used, SHGD_TEX_EMITTANCE should be defined
                if (pRes->TextureSlotExists(EFTT_EMITTANCE) || pRes->TextureSlotExists(EFTT_DECAL_OVERLAY))
                {
                    nMaskGen |= pBit->m_Mask;
                }
            }
            if (pBit->m_nDependencyReset & SHGD_TEX_EMITTANCE)
            {
                // If neither the emittance nor the decal overlay (emissive intensity) are used, SHGD_TEX_EMITTANCE should not be defined
                if (!pRes->TextureSlotExists(EFTT_EMITTANCE) && !pRes->TextureSlotExists(EFTT_DECAL_OVERLAY))
                {
                    nMaskGen &= ~pBit->m_Mask;
                }
            }


            if (pBit->m_nDependencySet & SHGD_TEX_OCC)
            {
                if (pRes->TextureSlotExists(EFTT_OCCLUSION))
                {
                    nMaskGen |= pBit->m_Mask;
                }
            }
            if (pBit->m_nDependencyReset & SHGD_TEX_OCC)
            {
                if (!pRes->TextureSlotExists(EFTT_OCCLUSION))
                {
                    nMaskGen &= ~pBit->m_Mask;
                }
            }
        }

        // Specific case - for user gl flags (eg: TEMP_TERRAIN, TEMP_VEGETATION)
        // this is needed since we now use common shader global flags - else define inside shader.cfi will override correct shared value
        if (pBit->m_nDependencySet & SHGD_USER_ENABLED)
        {
            nMaskGen |= pBit->m_Mask;
        }

        //if (m_nCombinationsProcess < 0 || m_bActivatePhase)
        {
            if (pBit->m_nDependencySet & SHGD_HW_BILINEARFP16)
            {
                nAndMaskHW &= ~pBit->m_Mask;
                if (gRenDev->m_bDeviceSupportsFP16Filter)
                {
                    nMaskGenHW |= pBit->m_Mask;
                }
            }
            if (pBit->m_nDependencyReset & SHGD_HW_BILINEARFP16)
            {
                nAndMaskHW &= ~pBit->m_Mask;
                if (!gRenDev->m_bDeviceSupportsFP16Filter)
                {
                    nMaskGenHW &= ~pBit->m_Mask;
                }
            }
            if (pBit->m_nDependencySet & SHGD_HW_SEPARATEFP16)
            {
                nAndMaskHW &= ~pBit->m_Mask;
                if (gRenDev->m_bDeviceSupportsFP16Separate)
                {
                    nMaskGenHW |= pBit->m_Mask;
                }
            }
            if (pBit->m_nDependencyReset & SHGD_HW_SEPARATEFP16)
            {
                nAndMaskHW &= ~pBit->m_Mask;
                if (!gRenDev->m_bDeviceSupportsFP16Separate)
                {
                    nMaskGenHW &= ~pBit->m_Mask;
                }
            }

            if (CParserBin::m_bShaderCacheGen)
            {
                // during shader cache gen, disable the special features in non D3D11 mode, and just accept
                // the lines as they come in D3D11 mode
                if (CParserBin::m_nPlatform != SF_D3D11 && CParserBin::m_nPlatform != SF_JASPER && CParserBin::m_nPlatform != SF_GL4 && CParserBin::m_nPlatform != SF_ORBIS)
                {
                    if (pBit->m_nDependencySet & SHGD_HW_WATER_TESSELLATION)
                    {
                        nAndMaskHW &= ~pBit->m_Mask;
                    }
                    if (pBit->m_nDependencySet & SHGD_HW_SILHOUETTE_POM)
                    {
                        nAndMaskHW &= ~pBit->m_Mask;
                    }
                }
            }
            else
            {
                PREFAST_SUPPRESS_WARNING(6326)
                bool bWaterTessHW = CRenderer::CV_r_WaterTessellationHW != 0 && gRenDev->m_bDeviceSupportsTessellation;
                if (pBit->m_nDependencySet & SHGD_HW_WATER_TESSELLATION)
                {
                    nAndMaskHW &= ~pBit->m_Mask;
                    if (bWaterTessHW)
                    {
                        nMaskGenHW |= pBit->m_Mask;
                    }
                }
                if (pBit->m_nDependencyReset & SHGD_HW_WATER_TESSELLATION)
                {
                    nAndMaskHW &= ~pBit->m_Mask;
                    if (!bWaterTessHW)
                    {
                        nMaskGenHW &= ~pBit->m_Mask;
                    }
                }

                PREFAST_SUPPRESS_WARNING(6326)
                const bool useSilhouettePOM = CRenderer::CV_r_SilhouettePOM != 0;
                if (pBit->m_nDependencySet & SHGD_HW_SILHOUETTE_POM)
                {
                    nAndMaskHW &= ~pBit->m_Mask;
                    if (useSilhouettePOM)
                    {
                        nMaskGenHW |= pBit->m_Mask;
                    }
                }
                if (pBit->m_nDependencyReset & SHGD_HW_SILHOUETTE_POM)
                {
                    nAndMaskHW &= ~pBit->m_Mask;
                    if (!useSilhouettePOM)
                    {
                        nMaskGenHW &= ~pBit->m_Mask;
                    }
                }

                PREFAST_SUPPRESS_WARNING(6326)
                const bool useSAA = CRenderer::CV_r_SpecularAntialiasing != 0;
                if (pBit->m_nDependencySet & SHGD_HW_SAA)
                {
                    nAndMaskHW &= ~pBit->m_Mask;
                    if (useSAA)
                    {
                        nMaskGenHW |= pBit->m_Mask;
                    }
                }
                if (pBit->m_nDependencyReset & SHGD_HW_SAA)
                {
                    nAndMaskHW &= ~pBit->m_Mask;
                    if (!useSAA)
                    {
                        nMaskGenHW &= ~pBit->m_Mask;
                    }
                }
            }
        }
    }
    nMaskGen &= nAndMaskHW;
    nMaskGenH |= nMaskGenHW;

    //sprintf_s( _debug_, "remap shadergen %I64x %I64x\n", nMaskGen, nMaskGenH);
    //OutputDebugString(_debug_);


    return true;
}

bool CShaderMan::mfUpdateTechnik (SShaderItem& SI, CCryNameTSCRC& Name)
{
    CShader* pSH = (CShader*)SI.m_pShader;
    if (!(pSH->m_Flags & EF_LOADED))
    {
        return false;
    }
    uint32 i;
    for (i = 0; i < pSH->m_HWTechniques.Num(); i++)
    {
        SShaderTechnique* pTech = pSH->m_HWTechniques[i];
        //if (!(pTech->m_Flags & FHF_PUBLIC))
        //  continue;
        if (pTech->m_NameCRC == Name)
        {
            break;
        }
    }
    if (i == pSH->m_HWTechniques.Num())
    {
        SI.m_nTechnique = -1;
        Warning("ERROR: CShaderMan::mfUpdateTechnik: couldn't find public technique for shader '%s'", pSH->GetName());
    }
    else
    {
        SI.m_nTechnique = i;
    }

    return true;
}

SShaderItem CShaderMan::mfShaderItemForName (const char* nameEf, bool bShare, int flags, SInputShaderResources* Res, uint64 nMaskGen)
{
    SShaderItem SI;

    CShaderResources* pResource = NULL;
    if (Res)
    {
        SI.m_pShaderResources = mfCreateShaderResources(Res, bShare);
        pResource = (CShaderResources*)SI.m_pShaderResources;
        pResource->m_ShaderParams = Res->m_ShaderParams;
        m_pCurInputResources = Res;
    }

    string strShader = nameEf;
    string strTechnique;
    string strNew = strShader.SpanExcluding(".");
    if (strNew.size() != strShader.size())
    {
        string second = string(&strShader.c_str()[strNew.size() + 1]);
        strShader = strNew;
        strTechnique = second;
    }

    if (SI.m_pShaderResources)
    {
        const threadID currentThread = CryGetCurrentThreadId();
        threadID renderThread, mainThread;
         
        gEnv->pRenderer->GetThreadIDs(mainThread, renderThread);
         
        if (currentThread == mainThread)
        {
            mfRefreshResources((CShaderResources*)SI.m_pShaderResources);
        }
        else
        {
            AZStd::function<void()> runOnMainThread = [this, SI]()
            {
                mfRefreshResources((CShaderResources*)SI.m_pShaderResources);
            };
         
            EBUS_QUEUE_FUNCTION(AZ::MainThreadRenderRequestBus, runOnMainThread);
        }
    }

    char shadName[256];
    if (nameEf && nameEf[0])
    {
        cry_strcpy(shadName, strShader.c_str());
    }
    else
    {
        shadName[0] = 0;
    }

    SI.m_pShader = mfForName(shadName, flags, (CShaderResources*)SI.m_pShaderResources, nMaskGen);

    // Get technique
    if (strTechnique.size())
    {
        CCryNameTSCRC nTech(strTechnique.c_str());
        if (!mfUpdateTechnik(SI, nTech))
        {
            SI.m_nTechnique = nTech.get(); // Postpone
        }
    }
    SI.m_nPreprocessFlags = -1;
    if (Res)
    {
        SI.m_pShaderResources = pResource;
        if (pResource)
        {
            pResource->CreateModifiers(Res);
        }
    }
    m_pCurInputResources = NULL;
    return SI;
}


CShader* CShaderMan::mfForName (const char* nameSh, int flags, const CShaderResources* Res, uint64 nMaskGen)
{
    CShader* ef, * efGen;
    int id;

    if (!nameSh || !nameSh[0])
    {
        Warning("Warning: CShaderMan::mfForName: NULL name\n");
        s_DefaultShader->AddRef();
        return s_DefaultShader;
    }

    char nameEf[256];
    char nameNew[256];
    char nameRes[256];

    uint64 nMaskGenHW = 0;
    uint64 maskGenStatic = m_staticFlags;

    cry_strcpy(nameEf, nameSh);

    cry_strcpy(nameRes, nameEf);

    cry_strcat(nameRes, GetShaderLanguageResourceName());

    if (maskGenStatic)
    {
        cry_strcat(nameRes, AZStd::string::format("(ST%llx)", maskGenStatic).c_str());
    }

    ef = NULL;
    efGen = NULL;

    // Check if this shader already loaded
    CBaseResource* pBR = CBaseResource::GetResource(CShader::mfGetClassName(), nameRes, false);
    bool bGenModified = false;
    ef = (CShader*)pBR;
    if (ef && ef->m_ShaderGenParams)
    {
        efGen = ef;

        mfModifyGenFlags(efGen, Res, nMaskGen, nMaskGenHW);
        bGenModified = true;
        azsprintf(nameNew, "%s(%llx)", nameRes, nMaskGen);
        pBR = CBaseResource::GetResource(CShader::mfGetClassName(), nameNew, false);
        ef = (CShader*)pBR;
        if (ef)
        {
            // Update the flags if HW specs changed
            ef->m_nMaskGenFX = nMaskGen | nMaskGenHW;
            assert(ef->m_pGenShader == efGen);
        }
    }

    if (ef)
    {
        if (!(flags & EF_RELOAD))
        {
            ef->AddRef();
            ef->m_Flags |= flags;
            return ef;
        }
        else
        {
            ef->mfFree();
            ef->m_Flags |= EF_RELOADED;
        }
    }

    if (!efGen)
    {
        sprintf_s(nameNew, "Shaders/%s.ext", nameEf);
        SShaderGen* pShGen = mfCreateShaderGenInfo(nameEf, false);

        if (pShGen)
        {
            efGen = mfNewShader(nameRes);
            efGen->SetRefCounter(0); // Hack: to avoid leaks in shader-gen's
            efGen->m_NameShader = nameRes;
            efGen->m_ShaderGenParams = pShGen;
        }
    }
    if (!(flags & EF_RELOAD) || !ef)
    {
        if (efGen)
        {
            // Change gen flags based on dependency on resources info
            if (!bGenModified)
            {
                //nMaskGen = gRenDev->EF_GetRemapedShaderMaskGen(nameSh, nMaskGen | nMaskGenHW);
                mfModifyGenFlags(efGen, Res, nMaskGen, nMaskGenHW);
            }
            sprintf_s(nameNew, "%s(%llx)", nameRes, nMaskGen);
            ef = mfNewShader(nameNew);
            if (!ef)
            {
                return s_DefaultShader;
            }

            ef->m_nMaskGenFX = nMaskGen | nMaskGenHW;
            ef->m_maskGenStatic = maskGenStatic;
            ef->m_ShaderGenStaticParams = m_staticExt;
            ef->m_pGenShader = efGen;
        }
        if (efGen && ef)
        {
            assert(efGen != ef);
            if (!efGen->m_DerivedShaders)
            {
                efGen->m_DerivedShaders = new std::vector<CShader*>;
            }
            efGen->m_DerivedShaders->push_back(ef);
            efGen->AddRef();
        }
        if (!ef)
        {
            ef = mfNewShader(nameRes);
            if (!ef)
            {
                return s_DefaultShader;
            }

            ef->m_maskGenStatic = maskGenStatic;
            ef->m_ShaderGenStaticParams = m_staticExt;
        }
    }
    id = ef->GetID();
    ef->m_NameShader = nameEf;
    ef->m_NameShaderICRC = CCrc32::ComputeLowercase(nameEf);

#ifndef NULL_RENDERER
    // Check for the new cryFX format
    sprintf_s(nameNew, "%sCryFX/%s.cfx", m_ShadersPath.c_str(), nameEf);
    ef->m_NameFile = nameNew;
    ef->m_Flags |= flags;
    gRenDev->m_pRT->RC_ParseShader(ef, nMaskGen | nMaskGenHW, flags, (CShaderResources*)Res);
    return ef;
#endif

    return ef;
}

void CShaderMan::CreateShaderExportRequestLine(const CShader* pSH, stack_string& exportString)
{
    if (pSH)
    {
        exportString.Format("<%d>%s/%s(", SHADER_SERIALISE_VER, pSH->GetName(), pSH->GetName());
        CreateShaderMaskGenString(pSH, exportString);
        exportString += ")()(0)(0)(0)(VS)"; //fake normal request line format
    }
}

void CShaderMan::CreateShaderMaskGenString(const CShader* pSH, stack_string& flagString)
{
    uint64 glMask = pSH->m_nMaskGenFX;

    if (glMask)
    {
        SShaderGenComb* c = NULL;
        c = mfGetShaderGenInfo(pSH->GetName());
        if (c && c->pGen && c->pGen->m_BitMask.Num())
        {
            bool bFirstTime = true;
            SShaderGen* pG = c->pGen;
            for (uint32 i = 0; i < 64; i++)
            {
                if (glMask & ((uint64)1 << i))
                {
                    for (uint32 j = 0; j < pG->m_BitMask.Num(); j++)
                    {
                        SShaderGenBit* pBit = pG->m_BitMask[j];
                        if (pBit->m_Mask & (glMask & ((uint64)1 << i)))
                        {
                            if (bFirstTime)
                            {
                                bFirstTime = false;
                            }
                            else
                            {
                                flagString += "|";
                            }

                            flagString += pBit->m_ParamName.c_str();
                            break;
                        }
                    }
                }
            }
        }
    }
}

void CShaderMan::RT_ParseShader(CShader* pSH, uint64 nMaskGen, uint32 flags, [[maybe_unused]] CShaderResources* pRes)
{
    CDebugAllowFileAccess ignoreInvalidFileAccess;

    bool bSuccess = false;
#ifdef SHADERS_SERIALIZING
    if (CRenderer::CV_r_shadersImport)
    {
        // Do not try and import fxb during cache generation
        // PC would need to support import of console data
        if (!gRenDev->IsShaderCacheGenMode())
        {
            ShaderImportResults importResults = ImportShader(pSH, m_Bin);

            if (importResults == SHADER_IMPORT_SUCCESS)
            {
                bSuccess = true;
            }
            else
            {
#ifdef SHADER_SERIALIZE_VERBOSE
                {
                    stack_string flagString;
                    CreateShaderMaskGenString(pSH, flagString);
                    
                    CryLog("[CShaderSerialize] Failed to import shader %s (0x%p) flags: 0x%llx 0x%x (%s)\n", pSH->GetName(), pSH, pSH->m_nMaskGenFX, pSH->m_nMDV, flagString.empty() ? "0" : flagString.c_str());
                }
#endif

                pSH->m_Flags |= EF_FAILED_IMPORT;

                if (CRenderer::CV_r_shadersImport == 2)
                {
                    // Do not fallback to the slow path unless we have a valid permutation in our lookup table (most optimal path)
                    return;
                }
                else
                {
                    // If importResults == SHADER_IMPORT_MISSING_ENTRY, then allow the fallback path if we have a valid .fxb file 
                    // for this shader, but the current permutation is missing from the lookup table.
                    // This will fallback to the slow path to parse the .cfx for this shader permutation
                    if (importResults == SHADER_IMPORT_FAILURE)
                    {
                        // No .fxb was exported for this .cfx, so do not fallback.
                        return;
                    }
                }
            }
        }
    }
#endif
    if (!bSuccess)
    {
#if !defined(SHADER_NO_SOURCES)
        SShaderBin* pBin = m_Bin.GetBinShader(pSH->m_NameShader, false, 0);
        if (pBin)
#endif
        {
#if !defined(SHADER_NO_SOURCES)
            if (flags & EF_FORCE_RELOAD)
            {
                uint32 nCRC32 = pBin->ComputeCRC();
                if (nCRC32 != pBin->m_CRC32)
                {
                    FXShaderBinValidCRCItor itor = gRenDev->m_cEF.m_Bin.m_BinValidCRCs.find(pBin->m_dwName);
                    if (itor == gRenDev->m_cEF.m_Bin.m_BinValidCRCs.end())
                    {
                        gRenDev->m_cEF.m_Bin.m_BinValidCRCs.insert(FXShaderBinValidCRCItor::value_type(pBin->m_dwName, false));
                    }

                    m_Bin.DeleteFromCache(pBin);
                    pBin = m_Bin.GetBinShader(pSH->m_NameShader, false, nCRC32);
                }
            }
#endif
            bSuccess = m_Bin.ParseBinFX(pBin, pSH, nMaskGen);
#ifdef SHADERS_SERIALIZING
            if (CRenderer::CV_r_shadersExport && gRenDev->IsShaderCacheGenMode())
            {
                //Shader compilation must be enabled for export, to allow reading the token table from the fxcbs in the USER dir
                int oldAllowCompilation = CRenderer::CV_r_shadersAllowCompilation;
                CRenderer::CV_r_shadersAllowCompilation = 1;

                if (bSuccess)
                {
                    // CheckFXBExists() used to only be queried here; however, that function will create the SResource under certain
                    // conditions if it does not exist and can erroneously cause ExportShader to not be called on the first time a shader .fxb
                    // is created.
                    if (!DoesSResourceExist(pSH) || !CheckFXBExists(pSH))
                    {
                        bool shaderExported = ExportShader(pSH, m_Bin);
#ifdef SHADER_SERIALIZE_VERBOSE
                        if (!shaderExported)
                        {
                            CryLog("[CShaderSerialize] ExportShader failed for shader %s\n", pSH->GetName());
                        }
#else
                        AZ_UNUSED(shaderExported);
#endif
                    }
#ifdef SHADER_SERIALIZE_VERBOSE
                    else
                    {
                        CryLog("[CShaderSerialize] Not exporting shader %s, it already exists\n", pSH->GetName());
                    }
#endif
                }

                CRenderer::CV_r_shadersAllowCompilation = oldAllowCompilation;
            }
#endif
        }
        else
        {
            CryWarning(VALIDATOR_MODULE_RENDERER, VALIDATOR_ERROR, "[SHADERS] Failed to load shader '%s'!", pSH->m_NameShader.c_str());
            pSH->m_Flags |= EF_NOTFOUND;
        }
    }
    pSH->m_Flags |= EF_LOADED;

    EBUS_QUEUE_EVENT(AZ::MaterialNotificationEventBus, OnShaderLoaded, pSH);
}
