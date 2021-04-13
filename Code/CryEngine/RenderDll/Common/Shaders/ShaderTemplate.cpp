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
#include "StringUtils.h"                                // stristr()
#include "../Common/Textures/TextureHelpers.h"
#include "../Common/Textures/TextureManager.h"

#if defined(WIN32) || defined(WIN64)
#include <direct.h>
#include <io.h>
#elif defined(LINUX)

#endif

//===============================================================================
#if !defined(_RELEASE)
    // Texture names to be used for error / process loading indications
    static const char* szReplaceMe = "EngineAssets/TextureMsg/ReplaceMe.tif";
#else
    // Some of the textures here will direct to the regular DefaultSolids_diff to prevent eye catching
    // bug textures in release mode
    static const char* szReplaceMe = "EngineAssets/TextureMsg/ReplaceMeRelease.tif";
#endif

//===============================================================================
CShaderResources* CShaderMan::mfCreateShaderResources(const SInputShaderResources* Res, bool bShare)
{
    uint16      i, j;
    SInputShaderResources localCopySR = *Res;    // pay attention to this local copy

    // prepare local resources for cache-check, textures are looked up but not triggered for load
    for ( auto& iter : localCopySR.m_TexturesResourcesMap )
    {
        SEfResTexture*      pTextureRes = &(iter.second);

        // Store off the previous texture's flags before we clean up the sampler slot
        uint32              textureFlags = (pTextureRes->m_Sampler.m_pTex) ? pTextureRes->m_Sampler.m_pTex->GetFlags() : 0;

        pTextureRes->m_Sampler.Cleanup();
        if (!pTextureRes->m_Name.empty())
        {   // If the texture that used to exist in this resource slot was created as an alpha texture
            // e.g. - a gloss map stored in the alpha channel of a normal map (see CShaderMan::mfRefreshResources for some
            // extra details on the texture slots that use the FT_ALPHA texture path), we need to pass the FT_ALPHA flag
            // into mfFindResourceTexture so it can find the actual texture resource.
            const uint32    alphaTextureFlags = textureFlags & FT_ALPHA;

            // Find the actual texture resource according to the given name
            pTextureRes->m_Sampler.m_pTex = mfFindResourceTexture(
                pTextureRes->m_Name.c_str(), localCopySR.m_TexturePath.c_str(),
                pTextureRes->m_Sampler.GetTexFlags() | alphaTextureFlags, pTextureRes);

            if (pTextureRes->m_Sampler.m_pTex)
            {   // increase usage counter if texture was found
                pTextureRes->m_Sampler.m_pTex->AddRef();
            }
        }
/*      [Shader System TO DO] - verify if possible to remove slot
        else
        {
            AZ_Warning( "ShadersSystem", false, "CShaderMan::mfCreateShaderResources - texture name does not exist at slot %d - removing slot", iter->first);
            pLocalCopySR.m_TexturesResourcesMap.erase(iter);
        }
*/
    }

    // check local resources vs' global cache
    int      nFree = -1;
    for (i = 1; i < CShader::s_ShaderResources_known.Num(); i++)    // first entry is a null entry
    {
        // NOT thread safe can be modified from render thread in SRenderShaderResources dtor ()
        // (if flushing of unloaded textures (UnloadLevel) is not complete before pre-loading of new materials)
        CShaderResources*   pLoadedSRes = CShader::s_ShaderResources_known[i];
        if (!pLoadedSRes)
        {
            nFree = i;  // find the first free slot in the bank
            if (!bShare || Res->m_ShaderParams.size())
            {
                break;
            }
            continue;
        }
        if (!bShare || Res->m_ShaderParams.size())
        {
            continue;
        }

        // [Shader System TO DO] - see if can be replaced with unified compare function
        if (localCopySR.m_ResFlags == pLoadedSRes->GetResFlags() &&
            localCopySR.m_LMaterial.m_Opacity == pLoadedSRes->GetStrengthValue(EFTT_OPACITY) &&
            localCopySR.m_LMaterial.m_Emittance.a == pLoadedSRes->GetStrengthValue(EFTT_EMITTANCE) &&
            localCopySR.m_AlphaRef == pLoadedSRes->GetAlphaRef() &&
            localCopySR.m_TexturePath == pLoadedSRes->m_TexturePath)
        {
            // if there is not shader deformation or both deformations are identical
            if ((!pLoadedSRes->m_pDeformInfo && !localCopySR.m_DeformInfo.m_eType) || (pLoadedSRes->m_pDeformInfo && *pLoadedSRes->m_pDeformInfo == localCopySR.m_DeformInfo))
            {
                // [Shader System TO DO] - optimize
                // The following code runs over all slots and verify a match between current shader and
                // loaded shaders If no match - break and add to the loaded.
                for (j = 0; j < EFTT_MAX; j++)
                {
                    SEfResTexture*  pLoadedTexRes = pLoadedSRes->GetTextureResource(j);
                    SEfResTexture*  pShaderTexRes = localCopySR.GetTextureResource(j);

                    // No loaded (cached) texture slot or no texture name for this slot
                    if (!pLoadedTexRes || pLoadedTexRes->m_Name.empty())
                    {
                        if (!pShaderTexRes || pShaderTexRes->m_Name.empty())
                        {   // match - no texture slot or texture name - move to the next slot
                            continue;
                        }

                        // slot exists but cannot be found in the resource - no shaders match - try the next cached shader
                        break;
                    }
                    // Cached slot entry with texture name exists - test to see if exists for the current shader
                    else if (!pShaderTexRes || pShaderTexRes->m_Name.empty())
                    {
                        break;  // texture slot doesn't exist / no texture name for current shader - no match - move to next cached shader
                    }

                    // both slots exist - run a full comparison between the two shaders (current and cached)
                    if (*pLoadedTexRes != *pShaderTexRes)
                    {
                        break;  // no match - move to the next cached shader
                    }
                }

                // The two shader resources fully match - return the cached shader resource (no need to load)
                if (j == EFTT_MAX)
                {
                    pLoadedSRes->AddRef();      // add usage count to this loaded shader resource
                    return pLoadedSRes;
                }
            }
        }
    }

    // The current shader resource does not exist - create a dynamic copy to be loaded and
    // insert to the cached resources bank.
    CShaderResources*   pSR = new CShaderResources(&localCopySR);
    pSR->m_nRefCounter = 1;
    if (!CShader::s_ShaderResources_known.Num())
    {
        CShader::s_ShaderResources_known.AddIndex(1);
        CShaderResources* pSRNULL = new CShaderResources;
        pSRNULL->m_nRefCounter = 1;
        CShader::s_ShaderResources_known[0] = pSRNULL;
    }
    else if (nFree < 0 && CShader::s_ShaderResources_known.Num() >= MAX_REND_SHADER_RES)
    {
        Warning("ERROR: CShaderMan::mfCreateShaderResources: MAX_REND_SHADER_RESOURCES hit");
        pSR->Release();
        return CShader::s_ShaderResources_known[1];
    }
    if (nFree > 0)
    {
        pSR->m_Id = nFree;
        pSR->m_IdGroup = pSR->m_Id;
        CShader::s_ShaderResources_known[nFree] = pSR;
    }
    else
    {
        pSR->m_Id = CShader::s_ShaderResources_known.Num();
        pSR->m_IdGroup = pSR->m_Id;
        CShader::s_ShaderResources_known.AddElem(pSR);
    }

    return pSR;
}

void CShaderResources::SetShaderParams(SInputShaderResources* pDst, IShader* pSH)
{
    ReleaseParams();
    m_ShaderParams = pDst->m_ShaderParams;

    UpdateConstants(pSH);
}


uint32 SShaderItem::PostLoad()
{
    uint32 nPreprocessFlags = 0;
    CShader* pSH = (CShader*)m_pShader;
    CShaderResources* pR = (CShaderResources*)m_pShaderResources;

    if (pSH->m_Flags2 & EF2_PREPR_GENCLOUDS)
    {
        nPreprocessFlags |= FSPR_GENCLOUDS;
    }
    if (pSH->m_Flags2 & EF2_PREPR_SCANWATER)
    {
        nPreprocessFlags |= FSPR_SCANTEXWATER | FB_PREPROCESS;
    }

    nPreprocessFlags |= FB_GENERAL;
    const SShaderTechnique* pTech = GetTechnique();
    if (pR)
    {
        pR->PostLoad(pSH);

        for (auto& iter : pR->m_TexturesResourcesMap )
        {
            SEfResTexture*  pTexture = &(iter.second);
            bool            is2DTexture = (pTexture->m_Sampler.m_eTexType == eTT_Auto2D || pTexture->m_Sampler.m_eTexType == eTT_Dyn2D);

            if (!pTexture->m_Sampler.m_pTarget)
            {
                continue;
            }

            if (is2DTexture)
            {
                if (gRenDev->m_RP.m_eQuality == eRQ_Low)
                {
                    pTexture->m_Sampler.m_eTexType = eTT_2D;
                }
                pR->m_ResFlags |= MTL_FLAG_NOTINSTANCED;
                nPreprocessFlags |= FSPR_SCANTEX;
            }
            pR->m_RTargets.AddElem(pTexture->m_Sampler.m_pTarget);
        }
    }

    if (pTech && pTech->m_Passes.Num() && (pTech->m_Passes[0].m_RenderState & GS_ALPHATEST_MASK))
    {
        if (pR && !pR->m_AlphaRef)
        {
            pR->m_AlphaRef = 0.5f;
        }
    }

    // Update persistent batch flags
    if (pTech)
    {
        if (pTech->m_nTechnique[TTYPE_Z] > 0)
        {
            nPreprocessFlags |= FB_Z;

            // ZPrepass only for non-alpha tested/blended geometry (decals, terrain).
            // We assume vegetation is special case due to potential massive overdraw
            if (pTech->m_nTechnique[TTYPE_ZPREPASS] > 0)
            {
                const bool bAlphaTested = (pR && pR->IsAlphaTested());
                if (!bAlphaTested)
                {
                    nPreprocessFlags |= FB_ZPREPASS;
                }
            }
        }

        if (!(pTech->m_Flags & FHF_POSITION_INVARIANT) && ((pTech->m_Flags & FHF_TRANSPARENT) || (pR && pR->IsTransparent())))
        {
            nPreprocessFlags |= FB_TRANSPARENT;
        }

        if (pTech->m_nTechnique[TTYPE_WATERREFLPASS] > 0)
        {
            nPreprocessFlags |= FB_WATER_REFL;
        }

        if (pTech->m_nTechnique[TTYPE_WATERCAUSTICPASS] > 0)
        {
            nPreprocessFlags |= FB_WATER_CAUSTIC;
        }

        if ((pSH->m_Flags2 & EF2_SKINPASS))
        {
            nPreprocessFlags |= FB_SKIN;
        }

        if (CRenderer::CV_r_SoftAlphaTest && pTech->m_nTechnique[TTYPE_SOFTALPHATESTPASS] > 0)
        {
            nPreprocessFlags |= FB_SOFTALPHATEST;
        }

        if (pSH->m_Flags2 & EF2_EYE_OVERLAY)
        {
            nPreprocessFlags |= FB_EYE_OVERLAY;
        }

        if (pSH->m_Flags & EF_REFRACTIVE)
        {
            if (CRenderer::CV_r_Refraction)
            {
                nPreprocessFlags |= FB_TRANSPARENT;
            }
            else
            {
                AZ_Warning("ShadersSystem", false,
                    "Shader %s use refraction but it's not enabled for this configuration. Check the value of the CVAR r_Refraction.",
                    pSH->m_NameShader.c_str());
            }
        }
    }

    if (pTech)
    {
        nPreprocessFlags |= pTech->m_nPreprocessFlags;
    }

    if (nPreprocessFlags & FSPR_MASK)
    {
        nPreprocessFlags |= FB_PREPROCESS;
    }

    return nPreprocessFlags;
}

//------------------------------------------------------------------------------
// [Shader System TO DO] - remove and replace with data driven from the shader files
// This method should be per shader type and not generic to the entire engine.

// This method associates fixed slots with known contextual material textures
// The engine slots are left unhandled and will be assigned through the
// shader and engine side - explore how!
//------------------------------------------------------------------------------
EEfResTextures CShaderMan::mfCheckTextureSlotName(const char* mapname)
{
    EEfResTextures slot = EFTT_UNKNOWN;

    if (!azstricmp(mapname, "$Diffuse"))
    {
        slot = EFTT_DIFFUSE;
    }
    else if (!azstricmp(mapname, "$Normal"))
    {
        slot = EFTT_NORMALS;
    }
    else if (!azstricmp(mapname, "$Specular"))
    {
        slot = EFTT_SPECULAR;
    }
    else if (!azstricmp(mapname, "$Env"))
    {
        slot = EFTT_ENV;
    }
    else if (!azstricmp(mapname, "$Detail"))
    {
        slot = EFTT_DETAIL_OVERLAY;
    }
    else if (!azstricmp(mapname, "$SecondSmoothness"))
    {
        slot = EFTT_SECOND_SMOOTHNESS;
    }
    else if (!azstricmp(mapname, "$Height"))
    {
        slot = EFTT_HEIGHT;
    }
    else if (!azstricmp(mapname, "$DecalOverlay"))
    {
        slot = EFTT_DECAL_OVERLAY;
    }
    else if (!azstricmp(mapname, "$Subsurface"))
    {
        slot = EFTT_SUBSURFACE;
    }
    else if (!azstricmp(mapname, "$CustomMap"))
    {   // Used as Diffuse 2 when BlendLayer is enabled
        slot = EFTT_CUSTOM;
    }
    else if (!azstricmp(mapname, "$Specular2"))
    {   // Used as Specular 2 when BlendLayer is enabled
        slot = EFTT_SPECULAR_2;
    }
    else if (!azstricmp(mapname, "$CustomSecondaryMap"))
    {   // Used as Normal 2 when BlendLayer is enabled
        slot = EFTT_CUSTOM_SECONDARY;
    }
    else if (!azstricmp(mapname, "$Opacity"))
    {   // Used as Blend Map for BlendLayer is enabled
        slot = EFTT_OPACITY;
    }
    else if (!azstricmp(mapname, "$Smoothness"))
    {
        slot = EFTT_SMOOTHNESS;
    }
    else if (!azstricmp(mapname, "$Emittance"))
    {
        slot = EFTT_EMITTANCE;
    }
    else if (!azstricmp(mapname, "$Occlusion"))
    {
        slot = EFTT_OCCLUSION;
    }
    // backwards compatible names
    else if (!azstricmp(mapname, "$Cubemap"))
    {
        slot = EFTT_ENV;
    }
    else if (!azstricmp(mapname, "$Translucency"))
    {
        slot = EFTT_SECOND_SMOOTHNESS;
    }
    else if (!azstricmp(mapname, "$BumpDiffuse"))
    {
        slot = EFTT_SECOND_SMOOTHNESS;                                                //EFTT_BUMP_DIFFUSE;
    }
    else if (!azstricmp(mapname, "$BumpHeight"))
    {
        slot = EFTT_HEIGHT;                                                //EFTT_BUMP_HEIGHT;
    }
    else if (!azstricmp(mapname, "$Bump"))
    {
        slot = EFTT_NORMALS;                                                //EFTT_BUMP;
    }
    else if (!azstricmp(mapname, "$Gloss"))
    {
        slot = EFTT_SPECULAR;                                                //EFTT_GLOSS;
    }
    else if (!azstricmp(mapname, "$GlossNormalA"))
    {
        slot = EFTT_SMOOTHNESS;                                                //EFTT_GLOSS_NORMAL_A;
    }

    return slot;
}

//------------------------------------------------------------------------------
// [Shader System TO DO] - data driven per update frequencies and the slots map
//------------------------------------------------------------------------------
CTexture* CShaderMan::mfCheckTemplateTexName(const char* mapname, [[maybe_unused]] ETEX_Type eTT)
{
    CTexture* TexPic = NULL;
    if (mapname[0] != '$')
    {
        return NULL;
    }

    {
        EEfResTextures slot = mfCheckTextureSlotName(mapname);

        if (slot != EFTT_MAX)
        {
            return &(*CTexture::s_ShaderTemplates)[slot];
        }
    }

    if (!azstricmp(mapname, "$ShadowPoolAtlas"))
    {
        TexPic = CTexture::s_ptexRT_ShadowPool;
    }
    else
    if (!azstrnicmp(mapname, "$ShadowID", 9))
    {
        int n = atoi(&mapname[9]);
        TexPic = CTexture::s_ptexShadowID[n];
    }
    else
    if (!azstricmp(mapname, "$FromRE") || !azstricmp(mapname, "$FromRE0"))
    {
        TexPic = CTexture::s_ptexFromRE[0];
    }
    else
    if (!azstricmp(mapname, "$FromRE1"))
    {
        TexPic = CTexture::s_ptexFromRE[1];
    }
    else
    if (!azstricmp(mapname, "$FromRE2"))
    {
        TexPic = CTexture::s_ptexFromRE[2];
    }
    else
    if (!azstricmp(mapname, "$FromRE3"))
    {
        TexPic = CTexture::s_ptexFromRE[3];
    }
    else
    if (!azstricmp(mapname, "$FromRE4"))
    {
        TexPic = CTexture::s_ptexFromRE[4];
    }
    else
    if (!azstricmp(mapname, "$FromRE5"))
    {
        TexPic = CTexture::s_ptexFromRE[5];
    }
    else
    if (!azstricmp(mapname, "$FromRE6"))
    {
        TexPic = CTexture::s_ptexFromRE[6];
    }
    else
    if (!azstricmp(mapname, "$FromRE7"))
    {
        TexPic = CTexture::s_ptexFromRE[7];
    }
    else
    if (!azstricmp(mapname, "$VolObj_Density"))
    {
        TexPic = CTexture::s_ptexVolObj_Density;
    }
    else
    if (!azstricmp(mapname, "$VolObj_Shadow"))
    {
        TexPic = CTexture::s_ptexVolObj_Shadow;
    }
    else
    if (!azstricmp(mapname, "$ColorChart"))
    {
        TexPic = CTexture::s_ptexColorChart;
    }
    else
    if (!azstricmp(mapname, "$FromObj"))
    {
        TexPic = CTexture::s_ptexFromObj;
    }
    else
    if (!azstricmp(mapname, "$SvoTree"))
    {
        TexPic = CTexture::s_ptexSvoTree;
    }
    else
    if (!azstricmp(mapname, "$SvoTris"))
    {
        TexPic = CTexture::s_ptexSvoTris;
    }
    else
    if (!azstricmp(mapname, "$SvoGlobalCM"))
    {
        TexPic = CTexture::s_ptexSvoGlobalCM;
    }
    else
    if (!azstricmp(mapname, "$SvoRgbs"))
    {
        TexPic = CTexture::s_ptexSvoRgbs;
    }
    else
    if (!azstricmp(mapname, "$SvoNorm"))
    {
        TexPic = CTexture::s_ptexSvoNorm;
    }
    else
    if (!azstricmp(mapname, "$SvoOpac"))
    {
        TexPic = CTexture::s_ptexSvoOpac;
    }
    else
    if (!azstricmp(mapname, "$FromObjCM"))
    {
        TexPic = CTexture::s_ptexFromObjCM;
    }
    else
    if (!azstrnicmp(mapname, "$White", 6))
    {
        TexPic = CTextureManager::Instance()->GetWhiteTexture();
    }
    else
    if (!azstrnicmp(mapname, "$RT_2D", 6))
    {
        TexPic = CTexture::s_ptexRT_2D;
    }
    else
    if (!azstricmp(mapname, "$PrevFrameScaled"))
    {
        TexPic = CTexture::s_ptexPrevFrameScaled;
    }
    else
    if (!azstricmp(mapname, "$BackBuffer"))
    {
        TexPic = CTexture::s_ptexBackBuffer;
    }
    else
    if (!azstricmp(mapname, "$ModelHUD"))
    {
        TexPic = CTexture::s_ptexModelHudBuffer;
    }
    else
    if (!azstricmp(mapname, "$BackBufferScaled_d2"))
    {
        TexPic = CTexture::s_ptexBackBufferScaled[0];
    }
    else
    if (!azstricmp(mapname, "$BackBufferScaled_d4"))
    {
        TexPic = CTexture::s_ptexBackBufferScaled[1];
    }
    else
    if (!azstricmp(mapname, "$BackBufferScaled_d8"))
    {
        TexPic = CTexture::s_ptexBackBufferScaled[2];
    }
    else
    if (!azstricmp(mapname, "$HDR_BackBuffer"))
    {
        TexPic = CTexture::s_ptexSceneTarget;
    }
    else
    if (!azstricmp(mapname, "$HDR_BackBufferScaled_d2"))
    {
        TexPic = CTexture::s_ptexHDRTargetScaled[0];
    }
    else
    if (!azstricmp(mapname, "$HDR_BackBufferScaled_d4"))
    {
        TexPic = CTexture::s_ptexHDRTargetScaled[1];
    }
    else
    if (!azstricmp(mapname, "$HDR_BackBufferScaled_d8"))
    {
        TexPic = CTexture::s_ptexHDRTargetScaled[2];
    }
    else
    if (!azstricmp(mapname, "$HDR_FinalBloom"))
    {
        TexPic = CTexture::s_ptexHDRFinalBloom;
    }
    else
    if (!azstricmp(mapname, "$HDR_TargetPrev"))
    {
        TexPic = CTexture::s_ptexHDRTargetPrev;
    }
    else
    if (!azstricmp(mapname, "$HDR_AverageLuminance"))
    {
        TexPic = CTexture::s_ptexHDRMeasuredLuminanceDummy;
    }
    else
    if (!azstricmp(mapname, "$ZTarget"))
    {
        TexPic = CTexture::s_ptexZTarget;
    }
    else
    if (!azstricmp(mapname, "$ZTargetScaled"))
    {
        TexPic = CTexture::s_ptexZTargetScaled;
    }
    else
    if (!azstricmp(mapname, "$ZTargetScaled2"))
    {
        TexPic = CTexture::s_ptexZTargetScaled2;
    }
    else
    if (!azstricmp(mapname, "$SceneTarget"))
    {
        TexPic = CTexture::s_ptexSceneTarget;
    }
    else
    if (!azstricmp(mapname, "$CloudsLM"))
    {
        TexPic = CTexture::s_ptexCloudsLM;
    }
    else
    if (!azstricmp(mapname, "$WaterVolumeDDN"))
    {
        TexPic = CTexture::s_ptexWaterVolumeDDN;
    }
    else
    if (!azstricmp(mapname, "$WaterVolumeReflPrev"))
    {
        TexPic = CTexture::s_ptexWaterVolumeRefl[1];
    }
    else
    if (!azstricmp(mapname, "$WaterVolumeRefl"))
    {
        TexPic = CTexture::s_ptexWaterVolumeRefl[0];
    }
    else
    if (!azstricmp(mapname, "$WaterVolumeCaustics"))
    {
        TexPic = CTexture::s_ptexWaterCaustics[0];
    }
    else
    if (!azstricmp(mapname, "$WaterVolumeCausticsTemp"))
    {
        TexPic = CTexture::s_ptexWaterCaustics[1];
    }
    else
    if (!azstricmp(mapname, "$SceneNormalsMap"))
    {
        TexPic = CTexture::s_ptexSceneNormalsMap;
    }
    else
    if (!azstricmp(mapname, "$SceneNormalsMapMS"))
    {
        TexPic = CTexture::s_ptexSceneNormalsMapMS;
    }
    else
    if (!azstricmp(mapname, "$SceneDiffuse"))
    {
        TexPic = CTexture::s_ptexSceneDiffuse;
    }
    else
    if (!azstricmp(mapname, "$SceneSpecular"))
    {
        TexPic = CTexture::s_ptexSceneSpecular;
    }
    else
    if (!azstricmp(mapname, "$SceneNormalsBent"))
    {
        TexPic = CTexture::s_ptexSceneNormalsBent;
    }
    else
    if (!azstricmp(mapname, "$SceneDiffuseAcc"))
    {
        TexPic = CTexture::s_ptexCurrentSceneDiffuseAccMap;
    }
    else
    if (!azstricmp(mapname, "$SceneSpecularAcc"))
    {
        TexPic = CTexture::s_ptexSceneSpecularAccMap;
    }
    else
    if (!azstricmp(mapname, "$SceneDiffuseAccMS"))
    {
        TexPic = CTexture::s_ptexSceneDiffuseAccMapMS;
    }
    else
    if (!azstricmp(mapname, "$SceneSpecularAccMS"))
    {
        TexPic = CTexture::s_ptexSceneSpecularAccMapMS;
    }
    else
    if (!azstricmp(mapname, "$DefaultEnvironmentProbe"))
    {
        TexPic = CTexture::s_defaultEnvironmentProbeDummy;
    }

    return TexPic;
}

//------------------------------------------------------------------------------
// [Shader System TO DO] - remove and replace with data driven from the shader files
//------------------------------------------------------------------------------
const char* CShaderMan::mfTemplateTexIdToName(int Id)
{
    switch (Id)
    {
    case EFTT_DIFFUSE:
        return "Diffuse";
    case EFTT_SPECULAR:
        return "Gloss";
    case EFTT_NORMALS:
        return "Bump";
    case EFTT_ENV:
        return "Environment";
    case EFTT_SUBSURFACE:
        return "SubSurface";
    case EFTT_CUSTOM:
        return "CustomMap";
    case EFTT_CUSTOM_SECONDARY:
        return "CustomSecondaryMap";
    case EFTT_DETAIL_OVERLAY:
        return "Detail";
    case EFTT_OPACITY:
        return "Opacity";
    case EFTT_DECAL_OVERLAY:
        return "Decal";
    case EFTT_OCCLUSION:
        return "Occlusion";
    case EFTT_SPECULAR_2:
        return "Specular2";
    case EFTT_SMOOTHNESS:
        return "GlossNormalA";
    case EFTT_EMITTANCE:
        return "Emittance";
    default:
        return "Unknown";
    }
    return "Unknown";
}

CTexAnim* CShaderMan::mfReadTexSequence(const char* na, int Flags, [[maybe_unused]] bool bFindOnly)
{
    char prefix[_MAX_PATH];
    char postfix[_MAX_PATH];
    char* nm;
    int i, j, l, m;
    char nam[_MAX_PATH];
    int n;
    CTexture* tx, * tp;
    int startn, endn, nums;
    char name[_MAX_PATH];

    tx = NULL;

    cry_strcpy(name, na);
    const char* ext = fpGetExtension (na);
    fpStripExtension(name, name);

    char chSep = '#';
    nm = strchr(name, chSep);
    if (!nm)
    {
        nm = strchr(name, '$');
        if (!nm)
        {
            return 0;
        }
        chSep = '$';
    }

    float fSpeed = 0.05f;
    {
        char nName[_MAX_PATH];
        azstrcpy(nName, _MAX_PATH, name);
        nm = strchr(nName, '(');
        if (nm)
        {
            name[nm - nName] = 0;
            char* speed = &nName[nm - nName + 1];
            nm = strchr(speed, ')');
            if (nm)
            {
                speed[nm - speed] = 0;
            }
            fSpeed = (float)atof(speed);
        }
    }

    j = 0;
    n = 0;
    l = -1;
    m = -1;
    while (name[n])
    {
        if (name[n] == chSep)
        {
            j++;
            if (l == -1)
            {
                l = n;
            }
        }
        else
        if (l >= 0 && m < 0)
        {
            m = n;
        }
        n++;
    }
    if (!j)
    {
        return 0;
    }

    cry_strcpy(prefix, name, l);

    char dig[_MAX_PATH];
    l = 0;
    if (m < 0)
    {
        startn = 0;
        endn = 999;
        postfix[0] = 0;
    }
    else
    {
        while (isdigit((unsigned char)name[m]))
        {
            dig[l++] = name[m];
            m++;
        }
        dig[l] = 0;
        startn = strtol(dig, NULL, 10);
        m++;

        l = 0;
        while (isdigit((unsigned char)name[m]))
        {
            dig[l++] = name[m];
            m++;
        }
        dig[l] = 0;
        endn = strtol(dig, NULL, 10);

        azstrcpy(postfix, AZ_ARRAY_SIZE(postfix), &name[m]);
    }

    nums = endn - startn + 1;

    n = 0;
    char frm[256];
    char frd[4];

    frd[0] = j + '0';
    frd[1] = 0;

    cry_strcpy(frm, "%s%.");
    cry_strcat(frm, frd);
    cry_strcat(frm, "d%s%s");
    CTexAnim* ta = NULL;
    for (i = 0; i < nums; i++)
    {
        sprintf_s(nam, frm, prefix, startn + i, postfix, ext);
        tp = (CTexture*)gRenDev->EF_LoadTexture(nam, Flags);
        if (!tp || !tp->IsLoaded())
        {
            if (tp)
            {
                tp->Release();
            }
            break;
        }
        if (!ta)
        {
            ta = new CTexAnim;
            ta->m_bLoop = true;
            ta->m_Time = fSpeed;
        }

        ta->m_TexPics.AddElem(tp);
        n++;
    }

    if (ta)
    {
        ta->m_NumAnimTexs = ta->m_TexPics.Num();
    }

    return ta;
}

int CShaderMan::mfReadTexSequence(STexSamplerRT *smp, const char *na, int Flags, bool bFindOnly)
{
    if (smp->m_pAnimInfo)
    {
        assert(0);
        return 0;
    }

    CTexAnim* ta = mfReadTexSequence(na, Flags, bFindOnly);
    if (ta)
    {
        smp->m_pAnimInfo = ta;
        SAFE_RELEASE(smp->m_pTex);
        return ta->m_NumAnimTexs;
    }

    return 0;
}

void CShaderMan::mfSetResourceTexState(SEfResTexture* Tex)
{
    if (Tex)
    {
        STexState ST;
        ST.SetFilterMode(Tex->m_Filter);
        ST.SetClampMode(Tex->m_bUTile ? TADDR_WRAP : TADDR_CLAMP, Tex->m_bVTile ? TADDR_WRAP : TADDR_CLAMP, Tex->m_bUTile ? TADDR_WRAP : TADDR_CLAMP);
        Tex->m_Sampler.m_nTexState = CTexture::GetTexState(ST);
    }
}

CTexture* CShaderMan::mfTryToLoadTexture(const char* nameTex, STexSamplerRT* smp, int Flags, bool bFindOnly)
{
    CTexture* tx = NULL;
    if (nameTex && strchr(nameTex, '#')) // test for " #" to skip max material names
    {
        int n = mfReadTexSequence(smp, nameTex, Flags, bFindOnly);
        //If we were able to read the texture animation, those textures will all be loaded and set into smp->m_pAnimInfo->m_TexPics by mfReadTexSequence.
        //Other code is dependent on some texture being returned here though, so Just return the first one in the animation.
        if (n > 0 && smp->m_pAnimInfo && !smp->m_pAnimInfo->m_TexPics.empty())
        {
            smp->m_pAnimInfo->m_TexPics[0]->AddRef();
            return smp->m_pAnimInfo->m_TexPics[0];
        }
    }
    if (!tx)
    {
        if (bFindOnly)
        {
            tx = (CTexture*)gRenDev->EF_GetTextureByName(nameTex, Flags);
        }
        else
        {
            tx = (CTexture*)gRenDev->EF_LoadTexture(nameTex, Flags);
        }
    }

    return tx;
}

CTexture* CShaderMan::mfFindResourceTexture(const char* nameTex, [[maybe_unused]] const char* path, int Flags, SEfResTexture* Tex)
{
    mfSetResourceTexState(Tex);

    return mfTryToLoadTexture(nameTex, Tex ? &Tex->m_Sampler : NULL, Flags, true);
}

CTexture* CShaderMan::mfLoadResourceTexture(const char* nameTex, [[maybe_unused]] const char* path, int Flags, SEfResTexture* Tex)
{
    mfSetResourceTexState(Tex);

    return mfTryToLoadTexture(nameTex, Tex ? &Tex->m_Sampler : NULL, Flags, false);
}


bool CShaderMan::mfLoadResourceTexture(ResourceSlotIndex Id, SInputShaderResources& RS, uint32 CustomFlags, bool bReplaceMeOnFail)
{
    SEfResTexture*  pTextureRes = RS.GetTextureResource(Id);

    if (!pTextureRes || pTextureRes->m_Name.empty())
        return false;

    // Texture slot and texture name exist
    STexSamplerRT&  texSampler(pTextureRes->m_Sampler);

    // No texture or texture not loaded - try to load it
    if (!texSampler.m_pTex || !texSampler.m_pTex->IsTextureLoaded())
    {
        texSampler.m_pTex = mfLoadResourceTexture( pTextureRes->m_Name.c_str(), RS.m_TexturePath.c_str(), texSampler.GetTexFlags() | CustomFlags, pTextureRes);
    }

    bool    bTextureLoadSuccess = texSampler.m_pTex->IsTextureLoaded();
    // Texture was not successfully loaded but is marked for placeholder loading on fail
    if (!bTextureLoadSuccess && bReplaceMeOnFail)
    {
        texSampler.m_pTex = mfLoadResourceTexture( szReplaceMe, RS.m_TexturePath.c_str(), texSampler.GetTexFlags() | CustomFlags, pTextureRes);
    }
    return bTextureLoadSuccess;
}

bool CShaderMan::mfLoadResourceTexture(ResourceSlotIndex Id, CShaderResources& RS, uint32 CustomFlags, bool bReplaceMeOnFail)
{
    SEfResTexture*  pTextureRes = RS.GetTextureResource(Id);

    // Texture slot does not exist
    if (!pTextureRes)
        return false;

    STexSamplerRT&  texSampler(pTextureRes->m_Sampler);
    bool            bTextureLoaded = texSampler.m_pTex && texSampler.m_pTex->IsTextureLoaded();

    if (!pTextureRes->m_Name.empty())
    {   // texture can be retrieved
        if (!bTextureLoaded || (CustomFlags & FT_ALPHA))
        {
            SAFE_RELEASE(texSampler.m_pTex);
            texSampler.m_pTex = mfLoadResourceTexture(pTextureRes->m_Name.c_str(), RS.m_TexturePath.c_str(), texSampler.GetTexFlags() | CustomFlags, pTextureRes);
        }

        if (!bTextureLoaded && texSampler.m_pTex->IsTextureMissing())
        {
            TextureWarning(pTextureRes->m_Name.c_str(), "Texture file is missing: '%s%s' in material \'%s\'", RS.m_TexturePath.c_str(), pTextureRes->m_Name.c_str(), RS.m_szMaterialName);
        }

        bTextureLoaded = texSampler.m_pTex->IsTextureLoaded();
        if (!bTextureLoaded && bReplaceMeOnFail)
        {
            texSampler.m_pTex = mfLoadResourceTexture("EngineAssets/TextureMsg/ReplaceMe.tif", RS.m_TexturePath.c_str(), texSampler.GetTexFlags() | CustomFlags, pTextureRes);
        }
    }

    return bTextureLoaded;
}

//------------------------------------------------------------------------------
// Notice - the function will actively add the marked slot if it does not exist.
//------------------------------------------------------------------------------
void CShaderMan::mfLoadDefaultTexture(ResourceSlotIndex Id, CShaderResources& RS, EEfResTextures Def)
{
    RS.m_TexturesResourcesMap[Id].m_Sampler.m_pTex = TextureHelpers::LookupTexDefault(Def);
}

//------------------------------------------------------------------------------
// Mark refresh required if any texture slot contains a texture
bool CShaderMan::mfRefreshResourceConstants(CShaderResources* Res)
{
    if (!Res)
        return false;

    for (auto iter = Res->m_TexturesResourcesMap.begin(); iter != Res->m_TexturesResourcesMap.end(); ++iter)
    {
        SEfResTexture*  pTexture = &iter->second;
        if (pTexture->m_Sampler.m_pTex)
        {   // marked changed if the sampler contains pointer to a texture
            return true;
        }
    }
    return false;
}

//------------------------------------------------------------------------------
// [Shader System] - TO DO : be very careful when optimizing this as it can lead
// to loss / uncorrelated  of texture slots.
//------------------------------------------------------------------------------
void CShaderMan::mfRefreshResources(CShaderResources* Res)
{
    if (Res)
    {
        for (int i = 0; i < EFTT_MAX; i++)
        {
            int Flags = 0;
            if (i == EFTT_NORMALS)
            {
                if ((!Res->TextureSlotExists(i) || Res->m_TexturesResourcesMap[i].m_Name.empty()))
                {
                    continue;
                }

                Flags |= FT_TEX_NORMAL_MAP;
                if (!Res->TextureSlotExists(i))     // add a slot
                {
                    Res->m_TexturesResourcesMap[i].Cleanup();
                }

                if (!mfLoadResourceTexture((EEfResTextures)i, *Res, Flags))
                {
                    mfLoadDefaultTexture((EEfResTextures)i, *Res, (EEfResTextures)i);
                }

                // Support for gloss in regular normal map
                CTexture* pTexN = (Res->TextureSlotExists(i) ? Res->m_TexturesResourcesMap[i].m_Sampler.m_pTex : NULL);
                if (pTexN && (pTexN->GetFlags() & FT_HAS_ATTACHED_ALPHA))
                {
                    Res->m_TexturesResourcesMap[EFTT_SMOOTHNESS].m_Name = pTexN->GetSourceName();
                    if (!mfLoadResourceTexture(EFTT_SMOOTHNESS, *Res, Flags | FT_ALPHA))
                    {
                        mfLoadDefaultTexture(EFTT_SMOOTHNESS, *Res, (EEfResTextures)i);
                    }
                }

                continue;
            }
            else
                if (i == EFTT_HEIGHT)
                {
                    if (!Res->TextureSlotExists(EFTT_NORMALS) || !Res->m_TexturesResourcesMap[EFTT_NORMALS].m_Sampler.m_pTex)
                    {
                        continue;
                    }
                    if (!Res->TextureSlotExists(i))
                    {
                        continue; //Res->AddTextureMap(i);
                    }
                    mfLoadResourceTexture((EEfResTextures)i, *Res, Flags);
                }
                else
                    if (i == EFTT_CUSTOM_SECONDARY)
                    {
                        if ((!Res->TextureSlotExists(i) || Res->m_TexturesResourcesMap[i].m_Name.empty()))
                        {
                            continue;
                        }

                        if (!Res->TextureSlotExists(i))
                        {   // adds the slot
                            Res->m_TexturesResourcesMap[i].Cleanup();
                        }

                        if (!mfLoadResourceTexture((EEfResTextures)i, *Res, Flags))
                        {
                            mfLoadDefaultTexture((EEfResTextures)i, *Res, (EEfResTextures)i);
                        }

                        // Support for gloss in blend layer normal map
                        CTexture* pTexN = (Res->TextureSlotExists(i) ? Res->m_TexturesResourcesMap[i].m_Sampler.m_pTex : NULL);
                        if (pTexN && (pTexN->GetFlags() & FT_HAS_ATTACHED_ALPHA))
                        {
                            Res->m_TexturesResourcesMap[EFTT_SECOND_SMOOTHNESS].m_Name = pTexN->GetSourceName();
                            if (!mfLoadResourceTexture(EFTT_SECOND_SMOOTHNESS, *Res, Flags | FT_ALPHA))
                            {
                                mfLoadDefaultTexture(EFTT_SECOND_SMOOTHNESS, *Res, EFTT_SMOOTHNESS);
                            }
                        }

                        continue;
                    }

            SEfResTexture* Tex = Res->GetTextureResource(i);
            if (!Tex)
            {
                continue;
            }

            // TODO: fix this bug at the root, a texture is allocated even though "nearest_cubemap"-named textures should be NULL
            if (Tex->m_Sampler.m_eTexType == eTT_NearestCube)
            {
                SAFE_RELEASE(Tex->m_Sampler.m_pTex);
                Tex->m_Sampler.m_pTex = CTexture::s_ptexFromObjCM;
            }

            if (!Tex->m_Sampler.m_pTex)
            {
                if (Tex->m_Sampler.m_eTexType == eTT_NearestCube)
                {
                    Tex->m_Sampler.m_pTex = CTexture::s_ptexFromObjCM;
                }
                else
                    if (Tex->m_Sampler.m_eTexType == eTT_Dyn2D)
                    {
                        // This block was for loading Flash files
                    }
                    else
                        if (Tex->m_Sampler.m_eTexType == eTT_Auto2D)
                        {
                            if (i == EFTT_ENV)
                            {
                                mfSetResourceTexState(Tex);

                                SAFE_RELEASE(Tex->m_Sampler.m_pTarget);
                                Tex->m_Sampler.m_pTarget = new SHRenderTarget;

                                Tex->m_Sampler.m_pTex = CTexture::s_ptexRT_2D;
                                Tex->m_Sampler.m_pTarget->m_pTarget[0] = CTexture::s_ptexRT_2D;

                                Tex->m_Sampler.m_pTarget->m_bTempDepth = true;
                                Tex->m_Sampler.m_pTarget->m_eOrder = eRO_PreProcess;
                                Tex->m_Sampler.m_pTarget->m_eTF = eTF_R8G8B8A8;
                                Tex->m_Sampler.m_pTarget->m_nIDInPool = -1;
                                Tex->m_Sampler.m_pTarget->m_nFlags |= FRT_RENDTYPE_RECURSIVECURSCENE | FRT_CAMERA_CURRENT;
                                Tex->m_Sampler.m_pTarget->m_nFlags |= FRT_CLEAR_DEPTH | FRT_CLEAR_STENCIL | FRT_CLEAR_COLOR;
                            }
                        }
                        else
                            if (Tex->m_Sampler.m_eTexType == eTT_User)
                            {
                                Tex->m_Sampler.m_pTex = NULL;
                            }
                            else
                            {
                                mfLoadResourceTexture((EEfResTextures)i, *Res, Flags);
                            }
            }

            // assign streaming priority based on the importance (sampler slot)
            if (Tex && Tex->m_Sampler.m_pITex && Tex->m_Sampler.m_pITex->IsTextureLoaded() && Tex->m_Sampler.m_pITex->IsStreamedVirtual())
            {
                CTexture* tp = (CTexture*)Tex->m_Sampler.m_pITex;
                tp->SetStreamingPriority(EFTT_MAX - i);
            }
        }
    }

    mfRefreshResourceConstants(Res);
}
