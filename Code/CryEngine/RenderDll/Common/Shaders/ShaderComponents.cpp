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
#include "../Shadow_Renderer.h"

#if defined(WIN32) || defined(WIN64)
#include <direct.h>
#include <io.h>
#elif defined(LINUX)
#endif

static void sParseTexMatrix([[maybe_unused]] const char* szScr, const char* szAnnotations, std::vector<STexSamplerFX>* pSamplers, SCGParam* vpp, [[maybe_unused]] int nComp, [[maybe_unused]] CShader* ef)
{
    uint32 i;
    const char* szSampler = gRenDev->m_cEF.mfParseFX_Parameter(szAnnotations, eType_STRING, "Sampler");
    assert (szSampler);
    if (szSampler)
    {
        for (i = 0; i < pSamplers->size(); i++)
        {
            STexSamplerFX* sm = &(*pSamplers)[i];
            if (!azstricmp(sm->m_szName.c_str(), szSampler))
            {
                vpp->m_nID = (UINT_PTR)sm->m_pTarget;
                break;
            }
        }
    }
}

//======================================================================================

// PB = Per-Batch
// PI = Per-Instance
// SI = Per-Instance Static
// PF = Per-Frame
// PM = Per-Material
// SG = Shadow-Generation

#define PARAM(a, b) #a, b
static SParamDB sParams[] =
{
    SParamDB(PARAM(SI_AlphaTest, ECGP_SI_AlphaTest), 0),
    SParamDB(PARAM(SI_AmbientOpacity, ECGP_SI_AmbientOpacity), 0),
    SParamDB(PARAM(SI_ObjectAmbColComp, ECGP_SI_ObjectAmbColComp), 0),
    SParamDB(PARAM(SI_BendInfo, ECGP_SI_BendInfo), 0),
    SParamDB(PARAM(SI_PrevBendInfo, ECGP_SI_PrevBendInfo), 0),

    SParamDB(PARAM(PI_ViewProjection, ECGP_Matr_PI_ViewProj), 0),

    SParamDB(PARAM(PI_Composite, ECGP_Matr_PI_Composite), 0),
    SParamDB(PARAM(PB_UnProjMatrix, ECGP_Matr_PB_UnProjMatrix), 0),
    SParamDB(PARAM(PB_ProjMatrix, ECGP_Matr_PB_ProjMatrix), 0),
    SParamDB(PARAM(PB_TerrainBaseMatrix, ECGP_Matr_PB_TerrainBase), 0),
    SParamDB(PARAM(PB_TerrainLayerGen, ECGP_Matr_PB_TerrainLayerGen), 0),
    SParamDB(PARAM(PI_TransObjMatrix, ECGP_Matr_PI_Obj_T), 0), // Due to some bug in Parser, ObjMatrix_T or something

    SParamDB(PARAM(PB_GmemStencilValue, ECGP_PB_GmemStencilValue), 0),
    SParamDB(PARAM(PI_MotionBlurData, ECGP_PI_MotionBlurData), 0),

    SParamDB(PARAM(PI_TessParams, ECGP_PI_TessParams), 0),

    SParamDB(PARAM(PB_TempMatr0, ECGP_Matr_PB_Temp4_0), PD_INDEXED),
    SParamDB(PARAM(PB_TempMatr1, ECGP_Matr_PB_Temp4_1), PD_INDEXED),
    SParamDB(PARAM(PB_TempMatr2, ECGP_Matr_PB_Temp4_2), PD_INDEXED),
    SParamDB(PARAM(PB_TempMatr3, ECGP_Matr_PB_Temp4_3), PD_INDEXED),
    SParamDB(PARAM(PI_TexMatrix, ECGP_Matr_PI_TexMatrix), 0, sParseTexMatrix),  // used for reflections (water) matrix
    SParamDB(PARAM(PI_TCGMatrix, ECGP_Matr_PI_TCGMatrix), PD_INDEXED),
    SParamDB(PARAM(PB_DLightsInfo, ECGP_PB_DLightsInfo), 0),

    SParamDB(PARAM(PM_DiffuseColor, ECGP_PM_DiffuseColor), 0),
    SParamDB(PARAM(PM_SpecularColor, ECGP_PM_SpecularColor), 0),
    SParamDB(PARAM(PM_EmissiveColor, ECGP_PM_EmissiveColor), 0),
    SParamDB(PARAM(PM_DeformWave, ECGP_PM_DeformWave), 0),
    SParamDB(PARAM(PM_DetailTiling, ECGP_PM_DetailTiling), 0),
    SParamDB(PARAM(PM_TexelDensity, ECGP_PM_TexelDensity), 0),
    SParamDB(PARAM(PM_UVMatrixDiffuse, ECGP_PM_UVMatrixDiffuse), 0),
    SParamDB(PARAM(PM_UVMatrixCustom, ECGP_PM_UVMatrixCustom), 0),
    SParamDB(PARAM(PM_UVMatrixEmissiveMultiplier, ECGP_PM_UVMatrixEmissiveMultiplier), 0),
    SParamDB(PARAM(PM_UVMatrixEmittance, ECGP_PM_UVMatrixEmittance), 0),
    SParamDB(PARAM(PM_UVMatrixDetail, ECGP_PM_UVMatrixDetail), 0),

    SParamDB(PARAM(PI_OSCameraPos, ECGP_PI_OSCameraPos), 0),
    SParamDB(PARAM(PB_BlendTerrainColInfo, ECGP_PB_BlendTerrainColInfo), 0),
    SParamDB(PARAM(PI_Ambient, ECGP_PI_Ambient), 0),
    SParamDB(PARAM(PI_VisionParams, ECGP_PI_VisionParams), 0),
    SParamDB(PARAM(PB_VisionMtlParams, ECGP_PB_VisionMtlParams), 0),

    SParamDB(PARAM(PB_IrregKernel, ECGP_PB_IrregKernel), 0),

    SParamDB(PARAM(PB_TFactor, ECGP_PB_TFactor), 0),
    SParamDB(PARAM(PB_TempData, ECGP_PB_TempData), PD_INDEXED | 0),
    SParamDB(PARAM(PB_RTRect, ECGP_PB_RTRect), 0),
    SParamDB(PARAM(PI_AvgFogVolumeContrib, ECGP_PI_AvgFogVolumeContrib), 0),
    SParamDB(PARAM(PI_NumInstructions, ECGP_PI_NumInstructions), PD_INDEXED),
    SParamDB(PARAM(PB_FromRE, ECGP_PB_FromRE), PD_INDEXED | 0),
    SParamDB(PARAM(PB_ObjVal, ECGP_PB_ObjVal), PD_INDEXED),
    SParamDB(PARAM(PI_TextureTileSize, ECGP_PI_TextureTileSize), 0),
    SParamDB(PARAM(PI_MotionBlurInfo, ECGP_PI_MotionBlurInfo), 0),
    SParamDB(PARAM(PI_ParticleParams, ECGP_PI_ParticleParams), 0),
    SParamDB(PARAM(PI_ParticleSoftParams, ECGP_PI_ParticleSoftParams), 0),
    SParamDB(PARAM(PI_ParticleExtParams, ECGP_PI_ParticleExtParams), 0),
    SParamDB(PARAM(PI_ParticleAlphaTest, ECGP_PI_ParticleAlphaTest), 0),
    SParamDB(PARAM(PI_ParticleEmissiveColor, ECGP_PI_ParticleEmissiveColor), 0),
    SParamDB(PARAM(PB_ScreenSize, ECGP_PB_ScreenSize), 0),

    SParamDB(PARAM(PI_OceanMat, ECGP_Matr_PI_OceanMat), 0),

    SParamDB(PARAM(PI_WrinklesMask0, ECGP_PI_WrinklesMask0), 0),
    SParamDB(PARAM(PI_WrinklesMask1, ECGP_PI_WrinklesMask1), 0),
    SParamDB(PARAM(PI_WrinklesMask2, ECGP_PI_WrinklesMask2), 0),

    SParamDB(PARAM(PB_ClipVolumeParams, ECGP_PB_ClipVolumeParams), 0),
    
    SParamDB(PARAM(PB_ResInfoDiffuse, ECGP_PB_ResInfoDiffuse), 0),
    SParamDB(PARAM(PB_FromObjSB, ECGP_PB_FromObjSB), 0),
    SParamDB(PARAM(PB_TexelDensityParam, ECGP_PB_TexelDensityParam), 0),
    SParamDB(PARAM(PB_TexelDensityColor, ECGP_PB_TexelDensityColor), 0),
    SParamDB(PARAM(PB_TexelsPerMeterInfo, ECGP_PB_TexelsPerMeterInfo), 0),

    SParamDB(PARAM(PB_WaterRipplesLookupParams, ECGP_PB_WaterRipplesLookupParams), 0),
    SParamDB(PARAM(PB_SkinningExtraWeights, ECGP_PB_SkinningExtraWeights), 0),

    SParamDB(PARAM(PI_FurLODInfo, ECGP_PI_FurLODInfo), 0),
    SParamDB(PARAM(PI_FurParams, ECGP_PI_FurParams), 0),
    SParamDB(PARAM(PI_PrevObjWorldMatrix, ECGP_PI_PrevObjWorldMatrix), 0),

    SParamDB()
};

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

const char* CShaderMan::mfGetShaderParamName(ECGParam ePR)
{
    int n = 0;
    const char* szName;
    while (szName = sParams[n].szName)
    {
        if (sParams[n].eParamType == ePR)
        {
            return szName;
        }
        n++;
    }
    if (ePR == ECGP_PM_Tweakable)
    {
        return "PM_Tweakable";
    }
    return NULL;
}

SParamDB* CShaderMan::mfGetShaderParamDB(const char* szSemantic)
{
    const char* szName;
    int n = 0;
    while (szName = sParams[n].szName)
    {
        int nLen = strlen(szName);
        if (!_strnicmp(szName, szSemantic, nLen) || (sParams[n].szAliasName && !_strnicmp(sParams[n].szAliasName, szSemantic, strlen(sParams[n].szAliasName))))
        {
            return &sParams[n];
        }
        n++;
    }
    return NULL;
}

bool CShaderMan::mfParseParamComp(int comp, SCGParam* pCurParam, const char* szSemantic, char* params, const char* szAnnotations, SShaderFXParams& FXParams, CShader* ef, uint32 nParamFlags, [[maybe_unused]] EHWShaderClass eSHClass, bool bExpressionOperand)
{
    if (comp >= 4 || comp < -1)
    {
        return false;
    }
    if (!pCurParam)
    {
        return false;
    }
    if (comp > 0)
    {
        pCurParam->m_Flags &= ~PF_SINGLE_COMP;
    }
    else
    {
        pCurParam->m_Flags |= PF_SINGLE_COMP;
    }
    if (!szSemantic || !szSemantic[0])
    {
        if (!pCurParam->m_pData)
        {
            pCurParam->m_pData = new SParamData;
        }
        pCurParam->m_pData->d.fData[comp] = shGetFloat(params);
        if (!((nParamFlags >> comp) & PF_TWEAKABLE_0))
        {
            pCurParam->m_eCGParamType = (ECGParam)((int)pCurParam->m_eCGParamType | (int)(ECGP_PB_Scalar << (comp * 8)));
        }
        else
        {
            pCurParam->m_eCGParamType = (ECGParam)((int)pCurParam->m_eCGParamType | (int)(ECGP_PM_Tweakable << (comp * 8)));
            if (!bExpressionOperand)
            {
                pCurParam->m_eCGParamType = (ECGParam)((int)pCurParam->m_eCGParamType | (int)ECGP_PM_Tweakable);
                pCurParam->m_Flags |= PF_MATERIAL | PF_SINGLE_COMP;
            }
        }
        return true;
    }
    if (!azstricmp(szSemantic, "NULL"))
    {
        return true;
    }
    if (szSemantic[0] == '(')
    {
        pCurParam->m_eCGParamType = ECGP_PM_Tweakable;
        pCurParam->m_Flags |= PF_SINGLE_COMP | PF_MATERIAL;
        return true;
    }
    const char* szName;
    int n = 0;
    while (szName = sParams[n].szName)
    {
        int nLen = strlen(szName);
        if (!_strnicmp(szName, szSemantic, nLen) || (sParams[n].szAliasName && !_strnicmp(sParams[n].szAliasName, szSemantic, strlen(sParams[n].szAliasName))))
        {
            if (sParams[n].nFlags & PD_MERGED)
            {
                pCurParam->m_Flags |= PF_CANMERGED;
            }
            if (!_strnicmp(szName, "PI_", 3))
            {
                pCurParam->m_Flags |= PF_INSTANCE;
            }
            else
            if (!_strnicmp(szName, "SI_", 3))
            {
                pCurParam->m_Flags |= PF_INSTANCE;
            }
            else
            if (!_strnicmp(szName, "PF_", 3))
            {
                AZ_Assert(false, "PF_ no longer supported");
            }
            else
            if (!_strnicmp(szName, "PM_", 3))
            {
                pCurParam->m_Flags |= PF_MATERIAL;
            }
            else
            if (!_strnicmp(szName, "SG_", 3))
            {
                AZ_Assert(false, "SG_ no longer supported");
            }
            static_assert(ECGP_COUNT <= 256, "ECGParam does not fit into 1 byte.");
            if (comp > 0)
            {
                pCurParam->m_eCGParamType = (ECGParam)((int)pCurParam->m_eCGParamType | (int)(sParams[n].eParamType << (comp * 8)));
            }
            else
            {
                pCurParam->m_eCGParamType = sParams[n].eParamType;
            }
            AZ_Assert(pCurParam->m_RegisterCount == 1, "Should be in default value as pCurParam is just initialized");

            if (sParams[n].nFlags & PD_INDEXED)
            {
                if (szSemantic[nLen] == '[')
                {
                    int nID = shGetInt(&szSemantic[nLen + 1]);
                    assert(nID < 256);
                    if (comp > 0)
                    {
                        nID <<= (comp * 8);
                    }
                    pCurParam->m_nID |= nID;
                }
            }
            if (sParams[n].ParserFunc)
            {
                sParams[n].ParserFunc(params ? params : szSemantic, szAnnotations, &FXParams.m_FXSamplersOld, pCurParam, comp, ef);
            }
            break;
        }
        n++;
    }
    if (!szName)
    {
        return false;
    }
    return true;
}

bool CShaderMan::mfParseCGParam(char* scr, const char* szAnnotations, SShaderFXParams& FXParams, CShader* ef, std::vector<SCGParam>* pParams, [[maybe_unused]] int nComps, uint32 nParamFlags, EHWShaderClass eSHClass, bool bExpressionOperand)
{
    char* name;
    long cmd;
    char* params;
    char* data;

    int nComp = 0;

    enum
    {
        eComp = 1, eParam, eName
    };
    static STokenDesc commands[] =
    {
        {eName, "Name"},
        {eComp, "Comp"},
        {eParam, "Param"},
        {0, 0}
    };
    SCGParam vpp;
    int n = pParams->size();
    bool bRes = true;

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
        case eName:
            vpp.m_Name = data;
            break;
        case eComp:
        {
            if (nComp < 4)
            {
                bRes &= mfParseParamComp(nComp, &vpp, name, params, szAnnotations, FXParams, ef, nParamFlags, eSHClass, bExpressionOperand);
                nComp++;
            }
        }
        break;
        case eParam:
            if (!name)
            {
                name = params;
            }
            bRes &= mfParseParamComp(-1, &vpp, name, params, szAnnotations, FXParams, ef, nParamFlags, eSHClass, bExpressionOperand);
            break;
        }
    }

    AZ_Assert(n == pParams->size(), "");
    pParams->push_back(vpp);

    AZ_Assert(bRes, "Error: CShaderMan::mfParseCGParam - bRes is false");
    return bRes;
}

bool CShaderMan::mfParseFXParameter(SShaderFXParams& FXParams, SFXParam* pr, const char* ParamName, CShader* ef, [[maybe_unused]] bool bInstParam, int nParams, std::vector<SCGParam>* pParams, EHWShaderClass eSHClass, bool bExpressionOperand)
{
    SCGParam CGpr;
    char scr[256];

    uint32 nParamFlags = pr->GetFlags();

    CryFixedStringT<512> Semantic = "Name=";
    Semantic += ParamName;
    Semantic += " ";
    int nComps = 0;
    if (!pr->m_Semantic.empty())
    {
        Semantic += "Param=";
        Semantic += pr->m_Semantic.c_str();
        nComps = pr->m_ComponentCount;
    }
    else
    {
        for (int i = 0; i < pr->m_ComponentCount; i++)
        {
            if (i)
            {
                Semantic += " ";
            }
            CryFixedStringT<128> cur;
            pr->GetParamComp(i, cur);
            if (cur.empty())
            {
                break;
            }
            nComps++;
            if ((cur.at(0) == '-' && isdigit(uint8(cur.at(1)))) || isdigit(uint8(cur.at(0))))
            {
                azsprintf(scr, "Comp = %s", cur.c_str());
            }
            else
            {
                azsprintf(scr, "Comp '%s'", cur.c_str());
            }
            Semantic += scr;
        }
    }
    // Process parameters only with semantics
    if (nComps)
    {
        uint32 nOffs = pParams->size();
        bool bRes = mfParseCGParam((char*)Semantic.c_str(), pr->m_Annotations.c_str(), FXParams, ef, pParams, nComps, nParamFlags, eSHClass, bExpressionOperand);
        AZ_Assert(bRes, "Error: CShaderMan::mfParseFXParameter - bRes is false");

        // pParams->size() > nOffs means we Added 1 new param into pParams in mfParseCGParam 
        if (pParams->size() > nOffs)
        {
            //assert(pBind->m_nComponents == 1);
            SCGParam& p = (*pParams)[nOffs];
            p.m_RegisterOffset = -1;
            p.m_RegisterCount = nParams;
            p.m_Flags |= nParamFlags;
            if (p.m_Flags & PF_AUTOMERGED)
            {
                if (!p.m_pData)
                {
                    p.m_pData = new SParamData;
                }
                const char* src = p.m_Name.c_str();
                for (uint32 i = 0; i < 4; i++)
                {
                    char param[128];
                    char dig = i + 0x30;
                    while (*src)
                    {
                        if (src[0] == '_' && src[1] == '_' && src[2] == dig)
                        {
                            int n = 0;
                            src += 3;
                            while (src[n])
                            {
                                if (src[n] == '_' && src[n + 1] == '_')
                                {
                                    break;
                                }
                                param[n] = src[n];
                                n++;
                            }
                            param[n] = 0;
                            src += n;
                            if (param[0])
                            {
                                p.m_pData->m_CompNames[i] = param;
                            }
                            break;
                        }
                        else
                        {
                            src++;
                        }
                    }
                }
            }
        }
        return bRes;
    }
    // Parameter without semantic
    return false;
}

// SM_ - material slots
// SR_ - global engine RT's
static SSamplerDB sSamplers[] =
{
    SSamplerDB(PARAM(SM_Diffuse, ECGS_MatSlot_Diffuse), 0),
    SSamplerDB(PARAM(SM_Normalmap, ECGS_MatSlot_Normalmap), 0),
    SSamplerDB(PARAM(SM_Glossmap, ECGS_MatSlot_Gloss), 0),
    SSamplerDB(PARAM(SM_Env, ECGS_MatSlot_Env), 0),
    SSamplerDB(PARAM(SS_Shadow0, ECGS_Shadow0), 0),
    SSamplerDB(PARAM(SS_Shadow1, ECGS_Shadow1), 0),
    SSamplerDB(PARAM(SS_Shadow2, ECGS_Shadow2), 0),
    SSamplerDB(PARAM(SS_Shadow3, ECGS_Shadow3), 0),
    SSamplerDB(PARAM(SS_Shadow4, ECGS_Shadow4), 0),
    SSamplerDB(PARAM(SS_Shadow5, ECGS_Shadow5), 0),
    SSamplerDB(PARAM(SS_Shadow6, ECGS_Shadow6), 0),
    SSamplerDB(PARAM(SS_Shadow7, ECGS_Shadow7), 0),
    SSamplerDB(PARAM(SS_TrilinearClamp, ECGS_TrilinearClamp), 0),
    SSamplerDB(PARAM(SS_MaterialAnisoHighWrap, ECGS_MatAnisoHighWrap), 0),
    SSamplerDB(PARAM(SS_MaterialAnisoLowWrap, ECGS_MatAnisoLowWrap), 0),
    SSamplerDB(PARAM(SS_MaterialTrilinearWrap, ECGS_MatTrilinearWrap), 0),
    SSamplerDB(PARAM(SS_MaterialBilinearWrap, ECGS_MatBilinearWrap), 0),
    SSamplerDB(PARAM(SS_MaterialTrilinearClamp, ECGS_MatTrilinearClamp), 0),
    SSamplerDB(PARAM(SS_MaterialBilinearClamp, ECGS_MatBilinearClamp), 0),
    SSamplerDB(PARAM(SS_MaterialAnisoHighBorder, ECGS_MatAnisoHighBorder), 0),
    SSamplerDB(PARAM(SS_MaterialTrilinearBorder, ECGS_MatTrilinearBorder), 0),
    SSamplerDB()
};

bool CShaderMan::mfParseFXSampler([[maybe_unused]] SShaderFXParams& FXParams, SFXSampler* pr, [[maybe_unused]] const char* ParamName, [[maybe_unused]] CShader* ef, [[maybe_unused]] int nParams, std::vector<SCGSampler>* pParams, [[maybe_unused]] EHWShaderClass eSHClass)
{
    SCGSampler CGpr;
    CGpr.m_nStateHandle = pr->m_nTexState;
    if (pr->m_Semantic.empty() && pr->m_Values.empty())
    {
        if (CGpr.m_nStateHandle >= 0)
        {
            pParams->push_back(CGpr);
            return true;
        }
        return false;
    }
    const char* szSemantic = pr->m_Semantic.c_str();
    const char* szName;
    int n = 0;
    while (szName = sSamplers[n].szName)
    {
        if (!azstricmp(szName, szSemantic))
        {
            AZ_PUSH_DISABLE_WARNING(,"-Wtautological-constant-out-of-range-compare")
            assert(sSamplers[n].eSamplerType < 256);
            AZ_POP_DISABLE_WARNING
            CGpr.m_eCGSamplerType = sSamplers[n].eSamplerType;
            pParams->push_back(CGpr);
            break;
        }
        n++;
    }
    if (!szName)
    {
        return false;
    }
    return true;
}


// TM_ - material slots
// TR_ - global engine RT's
static STextureDB sTextures[] =
{
    STextureDB(PARAM(TM_Diffuse, ECGT_MatSlot_Diffuse), 0),
    STextureDB(PARAM(TM_Normalmap, ECGT_MatSlot_Normals), 0),
    STextureDB(PARAM(TM_BumpHeight, ECGT_MatSlot_Height), 0),
    STextureDB(PARAM(TM_Glossmap, ECGT_MatSlot_Specular), 0),
    STextureDB(PARAM(TM_Env, ECGT_MatSlot_Env), 0),
    STextureDB(PARAM(TM_SubSurface, ECGT_MatSlot_SubSurface), 0),
    STextureDB(PARAM(TM_GlossNormalA, ECGT_MatSlot_Smoothness), 0),
    STextureDB(PARAM(TM_DecalOverlay, ECGT_MatSlot_DecalOverlay), 0),
    STextureDB(PARAM(TM_Custom, ECGT_MatSlot_Custom), 0),
    STextureDB(PARAM(TM_CustomSecondary, ECGT_MatSlot_CustomSecondary), 0),
    STextureDB(PARAM(TM_Opacity, ECGT_MatSlot_Opacity), 0),
    STextureDB(PARAM(TM_Detail, ECGT_MatSlot_Detail), 0),
    STextureDB(PARAM(TM_Emittance, ECGT_MatSlot_Emittance), 0),
    STextureDB(PARAM(TM_Occlusion, ECGT_MatSlot_Occlusion), 0),
    STextureDB(PARAM(TM_Specular2, ECGT_MatSlot_Specular2), 0),
    STextureDB(PARAM(TSF_Slot0, ECGT_SF_Slot0), 0),
    STextureDB(PARAM(TSF_Slot1, ECGT_SF_Slot1), 0),
    STextureDB(PARAM(TSF_SlotY, ECGT_SF_SlotY), 0),
    STextureDB(PARAM(TSF_SlotU, ECGT_SF_SlotU), 0),
    STextureDB(PARAM(TSF_SlotV, ECGT_SF_SlotV), 0),
    STextureDB(PARAM(TSF_SlotA, ECGT_SF_SlotA), 0),
    STextureDB(PARAM(TS_Shadow0, ECGT_Shadow0), 0),
    STextureDB(PARAM(TS_Shadow1, ECGT_Shadow1), 0),
    STextureDB(PARAM(TS_Shadow2, ECGT_Shadow2), 0),
    STextureDB(PARAM(TS_Shadow3, ECGT_Shadow3), 0),
    STextureDB(PARAM(TS_Shadow4, ECGT_Shadow4), 0),
    STextureDB(PARAM(TS_Shadow5, ECGT_Shadow5), 0),
    STextureDB(PARAM(TS_Shadow6, ECGT_Shadow6), 0),
    STextureDB(PARAM(TS_Shadow7, ECGT_Shadow7), 0),
    STextureDB(PARAM(TS_ShadowMask, ECGT_ShadowMask), 0),
    STextureDB(PARAM(TS_ZTarget, ECGT_ZTarget), 0),
    STextureDB(PARAM(TS_ZTargetScaled, ECGT_ZTargetScaled), 0),
    STextureDB(PARAM(TS_ZTargetMS, ECGT_ZTargetMS), 0),
    STextureDB(PARAM(TS_ShadowMaskZTarget, ECGT_ShadowMaskZTarget), 0),
    STextureDB(PARAM(TS_SceneNormalsBent, ECGT_SceneNormalsBent), 0),
    STextureDB(PARAM(TS_SceneNormals, ECGT_SceneNormals), 0),
    STextureDB(PARAM(TS_SceneDiffuse, ECGT_SceneDiffuse), 0),
    STextureDB(PARAM(TS_SceneSpecular, ECGT_SceneSpecular), 0),
    STextureDB(PARAM(TS_SceneDiffuseAcc, ECGT_SceneDiffuseAcc), 0),
    STextureDB(PARAM(TS_SceneSpecularAcc, ECGT_SceneSpecularAcc), 0),
    STextureDB(PARAM(TS_SceneNormalsMapMS, ECGT_SceneNormalsMapMS), 0),
    STextureDB(PARAM(TS_SceneDiffuseAccMS, ECGT_SceneDiffuseAccMS), 0),
    STextureDB(PARAM(TS_SceneSpecularAccMS, ECGT_SceneSpecularAccMS), 0),
    STextureDB(PARAM(TS_VolumetricClipVolumeStencil, ECGT_VolumetricClipVolumeStencil), 0),
    STextureDB(PARAM(TS_VolumetricFog, ECGT_VolumetricFog), 0),
    STextureDB(PARAM(TS_VolumetricFogGlobalEnvProbe0, ECGT_VolumetricFogGlobalEnvProbe0), 0),
    STextureDB(PARAM(TS_VolumetricFogGlobalEnvProbe1, ECGT_VolumetricFogGlobalEnvProbe1), 0),

    STextureDB()
};


//------------------------------------------------------------------------------
// Starting point for texture data during shader parse stage.
// Based on the texture name, this function will prepare the binding data (SCGTexture)
// that will be held within the list of parameters to be bound to the shader.
// 
// Important - the resources are loaded according to the order of arrival.
//
// Texture Structures and Their Usage:
// SCGTexture - texture binding structure used for the bind of the resource to the HW
// SFXTexture - Any SFX structure will be the structure gathered from the shader during
//      parsing and associated later on.   This structure contains a meta data regarding the 
//      texture such as UI name and hints, usage, type and other flags.    
//      It doesn't contain the actual texture data and doesn't apply to the binding directly 
//      but used as the data associated with the SCGTexture binding structure.
// SEfResTexture - the actual data representing a texture and its associated sampler.
//------------------------------------------------------------------------------
bool CShaderMan::mfParseFXTexture(
    [[maybe_unused]] SShaderFXParams& FXParams, SFXTexture* pr, [[maybe_unused]] const char* ParamName, 
    [[maybe_unused]] CShader* ef, [[maybe_unused]] int nParams, std::vector<SCGTexture>* pParams, [[maybe_unused]] EHWShaderClass eSHClass)
{
    SCGTexture CGpr;
    if (pr->m_Semantic.empty())
    {   // No texture semantic is assigned - assign textures according to usage ($, # or name)
        // Semantic is the name text associated with the resource in the shader right after 
        // the name of the resource.
        // Example:   
        //  Texture2D <uint> sceneDepthSampler : TS_ZTarget; - here TS_ZTarget is the semantic
        if (pr->m_szTexture.size())
        {
            const char* nameTex = pr->m_szTexture.c_str();

            // FT_DONT_STREAM = disable streaming for explicitly specified textures
            if (nameTex[0] == '$')
            {   // Maps the name to the pointer in the static array it will be stored at.
                // [Shaders System] - refactor to use map to store any dynamic name usage.
                CGpr.m_pTexture = mfCheckTemplateTexName(nameTex, eTT_MaxTexType /*unused*/);
            }
            else if (strchr(nameTex, '#')) // test for " #" to skip max material names
            {
                CGpr.m_pAnimInfo = mfReadTexSequence(nameTex, pr->GetTexFlags() | FT_DONT_STREAM, false);
            }

            // load texture by name (no context)
            if (!CGpr.m_pTexture && !CGpr.m_pAnimInfo)
            {
                CGpr.m_pTexture = (CTexture*)gRenDev->EF_LoadTexture(nameTex, pr->GetTexFlags() | FT_DONT_STREAM);
            }

            if (CGpr.m_pTexture)
            {
                CGpr.m_pTexture->AddRef();
            }

            CGpr.m_bSRGBLookup = pr->m_bSRGBLookup;
            CGpr.m_bGlobal = false;

            pParams->push_back(CGpr);
            return true;
        }

        return false;
    }

    const char* szSemantic = pr->m_Semantic.c_str();
    const char* szName;
    int n = 0;

    // Texture semantic exists and will be used to compare to the semantic texture table
    // Handling textures that are not material based textures, but parsed from the shader.
    // An example of this will be:  Texture2D<float4> sceneGBufferA : TS_SceneNormals;
    while (szName = sTextures[n].szName)
    {   // Run over all slots and try to associates the semantic name.
        // The semantic enum will be used in 'mfSetTexture' for setting texture 
        // loading and default properties.
        if (!azstricmp(szName, szSemantic))
        {
            static_assert(ECGT_COUNT <= 256, "ECGTexture does not fit into 1 byte.");
            // Set the association with the texture enum as per above
            CGpr.m_eCGTextureType = sTextures[n].eTextureType;
            CGpr.m_bSRGBLookup = pr->m_bSRGBLookup;
            CGpr.m_bGlobal = false;
            pParams->push_back(CGpr);
            break;
        }
        n++;
    }
    return szName != nullptr;
}

//===========================================================================================
bool SShaderParam::GetValue(const char* szName, AZStd::vector<SShaderParam>* Params, float* v, int nID)
{
    bool bRes = false;

    for (int i = 0; i < Params->size(); i++)
    {
        SShaderParam* sp = &(*Params)[i];
        if (!sp)
        {
            continue;
        }

        if (azstricmp(sp->m_Name.c_str(), szName) == 0)
        {
            bRes = true;
            switch (sp->m_Type)
            {
            case eType_HALF:
            case eType_FLOAT:
                v[nID] = sp->m_Value.m_Float;
                break;
            case eType_SHORT:
                v[nID] = (float)sp->m_Value.m_Short;
                break;
            case eType_INT:
            case eType_TEXTURE_HANDLE:
                v[nID] = (float)sp->m_Value.m_Int;
                break;

            case eType_VECTOR:
                v[0] = sp->m_Value.m_Vector[0];
                v[1] = sp->m_Value.m_Vector[1];
                v[2] = sp->m_Value.m_Vector[2];
                break;

            case eType_FCOLOR:
            case eType_FCOLORA:
                v[0] = sp->m_Value.m_Color[0];
                v[1] = sp->m_Value.m_Color[1];
                v[2] = sp->m_Value.m_Color[2];
                v[3] = sp->m_Value.m_Color[3];
                break;

            case eType_STRING:
                assert(0);
                bRes = false;
                break;
            case eType_UNKNOWN:
                assert(0);
                bRes = false;
                break;
            }

            break;
        }
    }

    return bRes;
}

bool SShaderParam::GetValue(uint8 eSemantic, AZStd::vector<SShaderParam>* Params, float* v, int nID)
{
    bool bRes = false;
    for (int i = 0; i < Params->size(); i++)
    {
        SShaderParam* sp = &(*Params)[i];
        if (!sp)
        {
            continue;
        }

        if (sp->m_eSemantic == eSemantic)
        {
            bRes = true;
            switch (sp->m_Type)
            {
            case eType_HALF:
            case eType_FLOAT:
                v[nID] = sp->m_Value.m_Float;
                break;
            case eType_SHORT:
                v[nID] = (float)sp->m_Value.m_Short;
                break;
            case eType_INT:
            case eType_TEXTURE_HANDLE:
                v[nID] = (float)sp->m_Value.m_Int;
                break;

            case eType_VECTOR:
                v[0] = sp->m_Value.m_Vector[0];
                v[1] = sp->m_Value.m_Vector[1];
                v[2] = sp->m_Value.m_Vector[2];
                break;

            case eType_FCOLOR:
                v[0] = sp->m_Value.m_Color[0];
                v[1] = sp->m_Value.m_Color[1];
                v[2] = sp->m_Value.m_Color[2];
                v[3] = sp->m_Value.m_Color[3];
                break;

            case eType_STRING:
                assert(0);
                bRes = false;
                break;

            case eType_UNKNOWN:
                assert(0);
                bRes = false;
                break;
            }

            break;
        }
    }

    return bRes;
}

bool sGetPublic(const CCryNameR& n, float* v, int nID)
{
    CRenderer* rd = gRenDev;
    bool bFound = false;
    CShaderResources* pRS;
    const char* cName = n.c_str();
    if (pRS = rd->m_RP.m_pShaderResources)
    {
        bFound = SShaderParam::GetValue(cName, &pRS->m_ShaderParams, v, nID);
    }
    if (!bFound && rd->m_RP.m_pShader)
    {
        auto& PublicParams = rd->m_cEF.m_Bin.mfGetFXParams(rd->m_RP.m_pShader).m_PublicParams;
        bFound = SShaderParam::GetValue(cName, &PublicParams, v, nID);
    }

    return bFound;
}

SParamData::~SParamData()
{
}

SParamData::SParamData(const SParamData& sp)
{
    for (int i = 0; i < 4; i++)
    {
        m_CompNames[i] = sp.m_CompNames[i];
        d.nData64[i] = sp.d.nData64[i];
    }
}

SCGTexture::~SCGTexture()
{
    if (!m_pAnimInfo)
    {
        SAFE_RELEASE(m_pTexture);
        m_pAnimInfo = nullptr;
    }
    else
    {
        SAFE_RELEASE(m_pAnimInfo);
        m_pTexture = nullptr;
    }
}
SCGTexture::SCGTexture(const SCGTexture& sp)
    : SCGBind(sp)
{
    if (!sp.m_pAnimInfo)
    {
        m_pAnimInfo = nullptr;
        if (m_pTexture = sp.m_pTexture)
        {
            m_pTexture->AddRef();
        }
    }
    else
    {
        m_pTexture = nullptr;
        if (m_pAnimInfo = sp.m_pAnimInfo)
        {
            m_pAnimInfo->AddRef();
        }
    }

    m_eCGTextureType = sp.m_eCGTextureType;
    m_bSRGBLookup = sp.m_bSRGBLookup;
    m_bGlobal = sp.m_bGlobal;
}

CTexture* SCGTexture::GetTexture() const
{
    if (m_pAnimInfo && m_pAnimInfo->m_Time && gRenDev->m_bPauseTimer == 0)
    {
        assert(gRenDev->m_RP.m_TI[gRenDev->m_RP.m_nProcessThreadID].m_RealTime >= 0);
        uint32 m = (uint32)(gRenDev->m_RP.m_TI[gRenDev->m_RP.m_nProcessThreadID].m_RealTime / m_pAnimInfo->m_Time) % (m_pAnimInfo->m_NumAnimTexs);
        assert(m < (uint32)m_pAnimInfo->m_TexPics.Num());

        return m_pAnimInfo->m_TexPics[m];
    }

    return m_pTexture;
}

