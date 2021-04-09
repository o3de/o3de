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

// Description : Abstract renderer API


#include "RenderDll_precompiled.h"

#include "Shadow_Renderer.h"
#include "IStatObj.h"
#include "I3DEngine.h"
#include "IMovieSystem.h"
#include "IIndexedMesh.h"
#include "BitFiddling.h"                                                            // IntegerLog2()
#include "ImageExtensionHelper.h"                                           // CImageExtensionHelper
#include "IResourceCompilerHelper.h"

#include "Textures/Image/CImage.h"
#include "Textures/TextureManager.h"
#include "PostProcess/PostEffects.h"
#include "RendElements/CRELensOptics.h"

#include "RendElements/OpticsFactory.h"

#include "XRenderD3D9/GraphicsPipeline/FurBendData.h"
#include "XRenderD3D9/GraphicsPipeline/FurPasses.h"
#include "RenderView.h"

///////////////////////////////////////////
// moved from "../CryCommon/branchmask.h"
// helper functions for branch elimination
//
// msb/lsb - most/less significant byte
//
// mask - 0xFFFFFFFF
// nz   - not zero
// zr   - is zero

ILINE const uint32 nz2msb(const uint32 x)
{
    return -(int32)x | x;
}

ILINE const uint32 msb2mask(const uint32 x)
{
    return (int32)(x) >> 31;
}

ILINE const uint32 nz2one(const uint32 x)
{
    return nz2msb(x) >> 31; // int((bool)x);
}

ILINE const uint32 nz2mask(const uint32 x)
{
    return (int32)msb2mask(nz2msb(x)); // -(int32)(bool)x;
}

ILINE const uint32 iselmask(const uint32 mask, uint32 x, const uint32 y)// select integer with mask (0xFFFFFFFF or 0x0 only!!!)
{
    return (x & mask) | (y & ~mask);
}


ILINE const uint32 mask_nz_nz(const uint32 x, const uint32 y)// mask if( x != 0 && y != 0)
{
    return msb2mask(nz2msb(x) & nz2msb(y));
}

ILINE const uint32 mask_nz_zr(const uint32 x, const uint32 y)// mask if( x != 0 && y == 0)
{
    return msb2mask(nz2msb(x) & ~nz2msb(y));
}


ILINE const uint32 mask_zr_zr(const uint32 x, const uint32 y)// mask if( x == 0 && y == 0)
{
    return ~nz2mask(x | y);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
struct SCompareByShadowFrustumID
{
    bool operator()(const SRendItem& rA, const SRendItem& rB) const
    {
        return rA.rendItemSorter.ShadowFrustumID() < rB.rendItemSorter.ShadowFrustumID();
    }
};

///////////////////////////////////////////////////////////////////////////////
struct SCompareByLightIds
{
    bool operator()(const SRenderPipeline::SShadowFrustumToRender& rA, const SRenderPipeline::SShadowFrustumToRender& rB) const
    {
        if (rA.nLightID != rB.nLightID)
        {
            return rA.nLightID < rB.nLightID;
        }
        else
        {
            if (rA.pFrustum->m_eFrustumType != rB.pFrustum->m_eFrustumType)
            {
                return int(rA.pFrustum->m_eFrustumType) < int(rB.pFrustum->m_eFrustumType);
            }
            else
            {
                return rA.pFrustum->nShadowMapLod < rB.pFrustum->nShadowMapLod;
            }
        }
    }
};

///////////////////////////////////////////////////////////////////////////////
static inline void HandleForceFlags(int& nList, int& nAW, uint32& nBatchFlags, const uint32 nShaderFlags, const uint32 nShaderFlags2, CRenderObject* obj)
{
    // Force rendering in last place
    // branchless

    const int32 sort1 = nz2mask(nShaderFlags2 & EF2_FORCE_DRAWLAST);
    const int32 sort2 = nz2one(nShaderFlags2 & EF2_FORCE_DRAWFIRST);
    float fSort = static_cast<float>(100000 * (sort1 + sort2));

    if (nShaderFlags2 & EF2_FORCE_ZPASS && !((nShaderFlags & EF_REFRACTIVE) && (nBatchFlags & FB_MULTILAYERS)))
    {
        nBatchFlags |= FB_Z;
    }

    {
        // below branchlessw version of:
        //if      (nShaderFlags2 & EF2_FORCE_TRANSPASS  ) nList = EFSLIST_TRANSP;
        //else if (nShaderFlags2 & EF2_FORCE_GENERALPASS) nList = EFSLIST_GENERAL;
        //else if (nShaderFlags2 & EF2_FORCE_WATERPASS  ) nList = EFSLIST_WATER;

        uint32 mb1 = nShaderFlags2 & EF2_FORCE_TRANSPASS;
        uint32 mb2 = nShaderFlags2 & EF2_FORCE_GENERALPASS;
        uint32 mb3 = nShaderFlags2 & EF2_FORCE_WATERPASS;

        mb1 = nz2msb(mb1);
        mb2 = nz2msb(mb2) & ~mb1;
        mb3 = nz2msb(mb3) & ~(mb1 ^ mb2);

        mb1 = msb2mask(mb1);
        mb2 = msb2mask(mb2);
        mb3 = msb2mask(mb3);

        const uint32 mask = mb1 | mb2 | mb3;
        mb1 &= EFSLIST_TRANSP;
        mb2 &= EFSLIST_GENERAL;
        mb3 &= EFSLIST_WATER;

        nList = iselmask(mask, mb1 | mb2 | mb3, nList);
    }


    // if (nShaderFlags2 & EF2_AFTERHDRPOSTPROCESS) // now it's branchless
    {
        uint32 predicate = nz2mask(nShaderFlags2 & EF2_AFTERHDRPOSTPROCESS);

        const uint32 mask = nz2mask(nShaderFlags2 & EF2_FORCE_DRAWLAST);
        nList = iselmask(predicate, iselmask(mask, EFSLIST_AFTER_POSTPROCESS, EFSLIST_AFTER_HDRPOSTPROCESS), nList);
    }

    if (nShaderFlags2 & EF2_AFTERPOSTPROCESS)
    {
        nList = EFSLIST_AFTER_POSTPROCESS;
    }

    //if (nShaderFlags2 & EF2_FORCE_DRAWAFTERWATER) nAW = 1;   -> branchless
    nAW |= nz2one(nShaderFlags2 & EF2_FORCE_DRAWAFTERWATER);

    obj->m_fSort += fSort;
}

///////////////////////////////////////////////////////////////////////////////
#if !defined(NULL_RENDERER)
static void HandleOldRTMask(CRenderObject* obj)
{
    const uint64 objFlags = obj->m_ObjFlags;
    obj->m_nRTMask = 0;
    if (objFlags & (FOB_NEAREST | FOB_DECAL_TEXGEN_2D | FOB_DISSOLVE | FOB_GLOBAL_ILLUMINATION | FOB_SOFT_PARTICLE))
    {
        if (objFlags & FOB_DECAL_TEXGEN_2D)
        {
            obj->m_nRTMask |= g_HWSR_MaskBit[HWSR_DECAL_TEXGEN_2D];
        }

        if (objFlags & FOB_NEAREST)
        {
            obj->m_nRTMask |= g_HWSR_MaskBit[HWSR_NEAREST];
        }

        if (objFlags & FOB_DISSOLVE)
        {
            obj->m_nRTMask |= g_HWSR_MaskBit[HWSR_DISSOLVE];
        }

        if (objFlags & FOB_GLOBAL_ILLUMINATION)
        {
            obj->m_nRTMask |= g_HWSR_MaskBit[HWSR_GLOBAL_ILLUMINATION];
        }

        if (gRenDev->CV_r_ParticlesSoftIsec && (objFlags & FOB_SOFT_PARTICLE))
        {
            obj->m_nRTMask |= g_HWSR_MaskBit[HWSR_SOFT_PARTICLE];
        }
    }

    obj->m_ObjFlags |= FOB_UPDATED_RTMASK;
}
#endif


///////////////////////////////////////////////////////////////////////////////
void CRenderer::EF_AddEf_NotVirtual([[maybe_unused]] IRenderElement* re, [[maybe_unused]] SShaderItem& SH, [[maybe_unused]] CRenderObject* obj, [[maybe_unused]] const SRenderingPassInfo& passInfo, [[maybe_unused]] int nList, [[maybe_unused]] int nAW, [[maybe_unused]] const SRendItemSorter& rendItemSorter)
{
#ifndef NULL_RENDERER
    int nThreadID = passInfo.ThreadID();
    assert(nList > 0 && nList < EFSLIST_NUM);

    if (!re || !SH.m_pShader)
    {
        return;
    }

    // shader item is not set up yet
    if (SH.m_nPreprocessFlags == -1)
    {
        return;
    }

    CShader* const __restrict pSH = (CShader*)SH.m_pShader;
    const uint32 nShaderFlags = pSH->m_Flags;
    if (nShaderFlags & EF_NODRAW)
    {
        return;
    }
    const uint32 nMaterialLayers = obj->m_nMaterialLayers;

    CShaderResources* const __restrict pShaderResources = (CShaderResources*)SH.m_pShaderResources;
    // Need to differentiate between something rendered with cloak layer material, and sorted with cloak.
    // e.g. ironsight glows on gun should be sorted with cloak to not write depth - can be inconsistent with no depth from gun.


    // store AABBs for all FOB_NEAREST objects for r_DrawNearest
    // TODO (bethelz) TRENDRIL: Remove draw nearest shadow hackery.
    IF (CV_r_DrawNearShadows && obj->m_ObjFlags & FOB_NEAREST, 0)
    {
        if (IRenderNode* pRenderNode = static_cast<IRenderNode*>(obj->m_pRenderNode))
        {
            size_t nID = ~0;
            CustomShadowMapFrustumData* pCustomShadowFrustumData = m_RP.m_arrCustomShadowMapFrustumData[nThreadID].push_back_new(nID);
            // The local bounds already contain rotated so just apply translation to that
            pRenderNode->GetLocalBounds(pCustomShadowFrustumData->aabb);
            pCustomShadowFrustumData->aabb.min += obj->GetTranslation();
            pCustomShadowFrustumData->aabb.max += obj->GetTranslation();
        }
    }

    if (passInfo.IsShadowPass())
    {
        if (pSH->m_HWTechniques.Num() && pSH->m_HWTechniques[0]->m_nTechnique[TTYPE_SHADOWGEN] >= 0)
        {
            passInfo.GetRenderView()->AddRenderItem(re, obj, SH, EFSLIST_SHADOW_GEN, SG_SORT_GROUP, FB_GENERAL, passInfo, rendItemSorter);
        }
        return;
    }

    // Discard 0 alpha blended geometry - this should be discarded earlier on 3dengine side preferably
    if (fzero(obj->m_fAlpha))
    {
        return;
    }
    if (pShaderResources && pShaderResources->::CShaderResources::IsInvisible())
    {
        return;
    }

    if (!(obj->m_ObjFlags & FOB_UPDATED_RTMASK))
    {
        HandleOldRTMask(obj);
    }

    uint32 nBatchFlags = EF_BatchFlags(SH, obj, re, passInfo);

    const uint32 nRenderlistsFlags = (FB_PREPROCESS | FB_TRANSPARENT);
    if (nBatchFlags & nRenderlistsFlags)
    {
        if (nBatchFlags & FB_PREPROCESS)
        {
            EShaderType eSHType = pSH->GetShaderType();

            // Prevent water usage on non-water specific meshes (it causes reflections updates). Todo: this should be checked in editor side and not allow such usage
            if (eSHType != eST_Water || (eSHType == eST_Water && nList == EFSLIST_WATER))
            {
                passInfo.GetRenderView()->AddRenderItem(re, obj, SH, EFSLIST_PREPROCESS, 0, nBatchFlags, passInfo, rendItemSorter);
            }
        }

        if ((nBatchFlags & FB_TRANSPARENT) && nList == EFSLIST_GENERAL)
        {
            // Refractive objects go into same list as transparent objects - partial resolves support
            // arbitrary ordering between transparent and refractive correctly.
            nList = EFSLIST_TRANSP;
        }
    }
     
    // FogVolume contribution for transparencies isn't needed when volumetric fog is turned on.
    // TODO (bethelz) Not a great place for this.
    if ((((nBatchFlags & FB_TRANSPARENT) || (pSH->GetFlags2() & EF2_HAIR)) && CRenderer::CV_r_VolumetricFog == 0)
        || passInfo.IsRecursivePass() /* account for recursive scene traversal done in forward fashion*/)
    {
        // Check if we need high fog volume shading quality 
        static ICVar* pCVarFogVolumeShadingQuality = gEnv->pConsole->GetCVar("e_FogVolumeShadingQuality");
        bool fogVolumeShadingQuality = pShaderResources && (pShaderResources->GetResFlags() & MTL_FLAG_FOG_VOLUME_SHADING_QUALITY_HIGH) && (pCVarFogVolumeShadingQuality->GetIVal() > 0);


        SRenderObjData* pOD = obj->GetObjData();
        if (pOD && ( fogVolumeShadingQuality || (pOD->m_FogVolumeContribIdx[nThreadID] == (uint16) - 1) ) )
        {
            I3DEngine* pEng = gEnv->p3DEngine;
  
            SFogVolumeData fogVolData;
            if (obj->m_pRenderNode)
            {
                // Pass the AABB of the object to in order to retrieve fog contribution.
                pEng->TraceFogVolumes(obj->m_pRenderNode->GetBBox().GetCenter(), obj->m_pRenderNode->GetBBox(), fogVolData, passInfo, fogVolumeShadingQuality);
            }
            // TODO (bethelz) Decouple fog volume color from renderer. Just store in render obj data.
            pOD->m_FogVolumeContribIdx[nThreadID] = CRenderer::PushFogVolumeContribution(fogVolData, passInfo);
        }
    }

    nBatchFlags &= ~(FB_Z & (nList ^ EFSLIST_GENERAL));

    nList = (nBatchFlags & FB_SKIN) ? EFSLIST_SKIN : nList;
    nList = (nBatchFlags & FB_EYE_OVERLAY) ? EFSLIST_EYE_OVERLAY : nList;

    if (pSH->GetShaderDrawType() == eSHDT_Fur)
    {
        nList = FurPasses::GetInstance().GetFurRenderList();
        nBatchFlags |= FB_FUR | FB_Z;

        // Opacity and emissive cause incorrect fur rendering, as transparency for shell passes
        // is set up elsewhere. Override the material/objects settings for opacity and emissive.
        nBatchFlags &= ~FB_TRANSPARENT;
        pShaderResources->SetStrengthValue(EFTT_EMITTANCE, 0.0f);
        pShaderResources->SetStrengthValue(EFTT_OPACITY, 1.0f);
        obj->m_fAlpha = 1.0f;

        FurBendData::Get().SetupObject(*obj, passInfo);
    }

    const uint32 nShaderFlags2 = pSH->m_Flags2;
    const uint32 ObjDecalFlag = obj->m_ObjFlags & FOB_DECAL;

    // make sure decals go into proper render list
    if ((ObjDecalFlag || (nShaderFlags & EF_DECAL)))
    {
        nBatchFlags |= FB_Z;
        nList = EFSLIST_DECAL;

        if (ObjDecalFlag == 0 && pShaderResources)
        {
            obj->m_nSort = pShaderResources->m_SortPrio;
        }
    }

    // Enable tessellation for water geometry
    obj->m_ObjFlags |= (pSH->m_Flags2 & EF2_HW_TESSELLATION && pSH->m_eShaderType == eST_Water) ? FOB_ALLOW_TESSELLATION : 0;

    const uint32 nForceFlags = (EF2_FORCE_DRAWLAST | EF2_FORCE_DRAWFIRST | EF2_FORCE_ZPASS | EF2_FORCE_TRANSPASS | EF2_FORCE_GENERALPASS | EF2_FORCE_DRAWAFTERWATER | EF2_FORCE_WATERPASS | EF2_AFTERHDRPOSTPROCESS | EF2_AFTERPOSTPROCESS);

    if (nShaderFlags2 & nForceFlags)
    {
        HandleForceFlags(nList, nAW, nBatchFlags, nShaderFlags, nShaderFlags2, obj);
    }

    {
        if (nShaderFlags & (EF_REFRACTIVE | EF_FORCEREFRACTIONUPDATE))
        {
            SRenderObjData* pOD = EF_GetObjData(obj, CRenderer::CV_r_RefractionPartialResolves == 2, passInfo.ThreadID());  // Creating objData for objs without one

            if (obj->m_pRenderNode && pOD)
            {
                const int32 align16 = (16 - 1);
                const int32 shift16 = 4;
                if IsCVarConstAccess(constexpr) (bool(CRenderer::CV_r_RefractionPartialResolves))
                {
                    AABB aabb;
                    IRenderNode* pRenderNode = static_cast<IRenderNode*>(obj->m_pRenderNode);
                    pRenderNode->FillBBox(aabb);

                    int iOut[4];

                    passInfo.GetCamera().CalcScreenBounds(&iOut[0], &aabb, CRenderer::GetWidth(), CRenderer::GetHeight());
                    pOD->m_screenBounds[0] = min(iOut[0] >> shift16, 255);
                    pOD->m_screenBounds[1] = min(iOut[1] >> shift16, 255);
                    pOD->m_screenBounds[2] = min((iOut[2] + align16) >> shift16, 255);
                    pOD->m_screenBounds[3] = min((iOut[3] + align16) >> shift16, 255);

#if REFRACTION_PARTIAL_RESOLVE_DEBUG_VIEWS
                    if (CRenderer::CV_r_RefractionPartialResolvesDebug == eRPR_DEBUG_VIEW_3D_BOUNDS)
                    {
                        // Debug bounding box view for refraction partial resolves
                        IRenderAuxGeom* pAuxRenderer = gEnv->pRenderer->GetIRenderAuxGeom();
                        if (pAuxRenderer)
                        {
                            SAuxGeomRenderFlags oldRenderFlags = pAuxRenderer->GetRenderFlags();

                            SAuxGeomRenderFlags newRenderFlags;
                            newRenderFlags.SetDepthTestFlag(e_DepthTestOff);
                            newRenderFlags.SetAlphaBlendMode(e_AlphaBlended);
                            pAuxRenderer->SetRenderFlags(newRenderFlags);

                            const bool bSolid = true;
                            const ColorB solidColor(64, 64, 255, 64);
                            pAuxRenderer->DrawAABB(aabb, bSolid, solidColor, eBBD_Faceted);

                            const ColorB wireframeColor(255, 0, 0, 255);
                            pAuxRenderer->DrawAABB(aabb, !bSolid, wireframeColor, eBBD_Faceted);

                            // Set previous Aux render flags back again
                            pAuxRenderer->SetRenderFlags(oldRenderFlags);
                        }
                    }
#endif
                }
                else if (nShaderFlags & EF_FORCEREFRACTIONUPDATE)
                {
                    pOD->m_screenBounds[0] = 0;
                    pOD->m_screenBounds[1] = 0;
                    pOD->m_screenBounds[2] = min((CRenderer::GetWidth()) >> shift16, 255);
                    pOD->m_screenBounds[3] = min((CRenderer::GetHeight()) >> shift16, 255);
                }
            }
        }

        // final step, for post 3d items, remove them from any other list than POST_3D_RENDER
        // (have to do this here as the batch needed to go through the normal nList assign path first)
        nBatchFlags = iselmask(nz2mask(nBatchFlags & FB_POST_3D_RENDER), FB_POST_3D_RENDER, nBatchFlags);

        // No need to sort opaque passes by water/after water. Ensure always on same list for more coherent sorting
        nAW |= nz2one((nList == EFSLIST_GENERAL) | (nList == EFSLIST_DECAL));
        m_RP.m_pRenderViews[nThreadID]->AddRenderItem(re, obj, SH, nList, nAW, nBatchFlags, passInfo, rendItemSorter);
    }
#endif
}

///////////////////////////////////////////////////////////////////////////////
void CRenderer::EF_AddEf (IRenderElement* re, SShaderItem& pSH, CRenderObject* obj, const SRenderingPassInfo& passInfo, int nList, int nAW, const SRendItemSorter& rendItemSorter)
{
    EF_AddEf_NotVirtual (re, pSH, obj, passInfo, nList, nAW, rendItemSorter);
}

//////////////////////////////////////////////////////////////////////////
uint16 CRenderer::PushFogVolumeContribution(const SFogVolumeData& fogVolData, const SRenderingPassInfo& passInfo)
{
    int nThreadID = passInfo.ThreadID();

    const size_t maxElems((1 << (sizeof(uint16) * 8)) - 1);
    size_t numElems(m_RP.m_fogVolumeContibutionsData[nThreadID].size());
    assert(numElems < maxElems);
    if (numElems >= maxElems)
    {
        return (uint16) - 1;
    }

    size_t nIndex = ~0;
    m_RP.m_fogVolumeContibutionsData[nThreadID].push_back(fogVolData, nIndex);
  
    return static_cast<uint16>(nIndex);
}

//////////////////////////////////////////////////////////////////////////
void CRenderer::GetFogVolumeContribution(uint16 idx, SFogVolumeData& fogVolData ) const
{
    int nThreadID = m_RP.m_nProcessThreadID;
    if (idx >= m_RP.m_fogVolumeContibutionsData[nThreadID].size())
    {
        fogVolData.fogColor= ColorF(0.0f, 0.0f, 0.0f, 1.0f);
    }
    else
    {
        fogVolData = m_RP.m_fogVolumeContibutionsData[nThreadID][idx];
    }
}

//////////////////////////////////////////////////////////////////////////
uint32 CRenderer::EF_BatchFlags(SShaderItem& SH, CRenderObject* pObj, [[maybe_unused]] IRenderElement* renderElement, const SRenderingPassInfo& passInfo)
{

    uint32 nFlags = SH.m_nPreprocessFlags & FB_MASK;
    SShaderTechnique* const __restrict pTech = SH.GetTechnique();
    CShaderResources* const __restrict pR = (CShaderResources*)SH.m_pShaderResources;
    CShader* const __restrict pS = (CShader*)SH.m_pShader;

    float  fAlpha = pObj->m_fAlpha;
    uint32 uTransparent = (bool)(fAlpha < 1.0f);
    const uint64 ObjFlags = pObj->m_ObjFlags;

    if (!passInfo.IsRecursivePass() && pTech)
    {
        CryPrefetch(pTech->m_nTechnique);
        CryPrefetch(pR);

        //if (pObj->m_fAlpha < 1.0f) nFlags |= FB_TRANSPARENT;
        nFlags |= FB_TRANSPARENT * uTransparent;

        if (!((nFlags & FB_Z) && (!(pObj->m_RState & OS_NODEPTH_WRITE) || (pS->m_Flags2 & EF2_FORCE_ZPASS))))
        {
            nFlags &= ~FB_Z;
        }

        if ((ObjFlags & FOB_DISSOLVE) || (ObjFlags & FOB_DECAL) || CRenderer::CV_r_usezpass != 2 || pObj->m_fDistance > CRenderer::CV_r_ZPrepassMaxDist)
        {
            nFlags &= ~FB_ZPREPASS;
        }

        if (ObjFlags & FOB_RENDER_TRANS_AFTER_DOF)
        {
            nFlags |= FB_TRANSPARENT_AFTER_DOF;
        }

        pObj->m_ObjFlags |= (nFlags & FB_ZPREPASS) ? FOB_ZPREPASS : 0;

        if (pTech->m_nTechnique[TTYPE_DEBUG] > 0 && 0 != (ObjFlags & FOB_SELECTED))
        {
            nFlags |= FB_DEBUG;
        }

        const uint32 nMaterialLayers = pObj->m_nMaterialLayers;
        const uint32 DecalFlags = pS->m_Flags & EF_DECAL;

        if (passInfo.IsShadowPass())
        {
            nFlags &= ~FB_PREPROCESS;
        }

        nFlags &= ~(FB_PREPROCESS & uTransparent);

        AZ_PUSH_DISABLE_WARNING(, "-Wconstant-logical-operand")
        if ((nMaterialLayers & ~uTransparent) && CV_r_usemateriallayers)
        AZ_POP_DISABLE_WARNING
        {
            const uint32 nResourcesNoDrawFlags = static_cast<CShaderResources*>(pR)->CShaderResources::GetMtlLayerNoDrawFlags();

            // if ((nMaterialLayers & MTL_LAYER_BLEND_DYNAMICFROZEN) && !(nResourcesNoDrawFlags & MTL_LAYER_FROZEN))
            uint32 uMask = mask_nz_zr(nMaterialLayers & MTL_LAYER_BLEND_DYNAMICFROZEN, nResourcesNoDrawFlags & MTL_LAYER_FROZEN);
            nFlags |= FB_MULTILAYERS & uMask;
        }

        if (pTech->m_nTechnique[TTYPE_MOTIONBLURPASS] > 0 && CV_r_MotionVectors && CV_r_MotionVectorsTransparency)
        {
            // Combine material and runtime opacity.
            const float opacity = pR->GetStrengthValue(EFTT_OPACITY) * fAlpha;
            const bool isNotADecal = ((ObjFlags & FOB_DECAL) | DecalFlags) == 0;
            const bool isAboveAlphaThreshold = opacity >= CV_r_MotionVectorsTransparencyAlphaThreshold;

            // We do not want to generate motion for decals, since they adhere to surfaces.
            if (isNotADecal && isAboveAlphaThreshold)
            {
                nFlags |= FB_MOTIONBLUR;
            }
        }

        SRenderObjData* pOD = pObj->GetObjData();
        if (pTech->m_nTechnique[TTYPE_CUSTOMRENDERPASS] > 0)
        {
            const uint32 customvisions = CRenderer::CV_r_customvisions;
            const uint32 nHUDSilhouettesParams = (pOD && pOD->m_nHUDSilhouetteParams);
            if (customvisions && nHUDSilhouettesParams)
            {
                nFlags |= FB_CUSTOM_RENDER;
            }
        }
    }
    else
    if (passInfo.IsRecursivePass() && pTech && m_RP.m_TI[passInfo.ThreadID()].m_PersFlags & RBPF_MIRRORCAMERA)
    {
        nFlags &= (FB_TRANSPARENT | FB_GENERAL);
        nFlags |= FB_TRANSPARENT * uTransparent;                                        // if (pObj->m_fAlpha < 1.0f)                   nFlags |= FB_TRANSPARENT;
    }

    {
        //if ( (objFlags & FOB_ONLY_Z_PASS) || CV_r_ZPassOnly) && !(nFlags & (FB_TRANSPARENT))) - put it to the mask
        const uint32 mask =  mask_nz_zr((uint32)CV_r_ZPassOnly, nFlags & (FB_TRANSPARENT));

        nFlags = iselmask(mask, FB_Z, nFlags);
    }

    int nShaderFlags = (SH.m_pShader ? SH.m_pShader->GetFlags() : 0);
    if ((CRenderer::CV_r_RefractionPartialResolves && nShaderFlags & EF_REFRACTIVE) ||   (nShaderFlags & EF_FORCEREFRACTIONUPDATE))
    {
        pObj->m_ObjFlags |= FOB_REQUIRES_RESOLVE;
    }

    return nFlags;
}

///////////////////////////////////////////////////////////////////////////////
SRenderObjData* CRenderer::EF_GetObjData(CRenderObject* pObj, [[maybe_unused]] bool bCreate, [[maybe_unused]] int nThreadID)
{
    return pObj->GetObjData();
}

///////////////////////////////////////////////////////////////////////////////
SRenderObjData* CRenderer::FX_GetObjData(CRenderObject* pObj, [[maybe_unused]] int nThreadID)
{
    return pObj->GetObjData();
}
///////////////////////////////////////////////////////////////////////////////
CRenderObject* CRenderer::EF_GetObject_Temp(int nThreadID)
{
    CThreadSafeWorkerContainer <CRenderObject*>& Objs = m_RP.m_TempObjects[nThreadID];

    size_t nId = ~0;
    CRenderObject** ppObj = Objs.push_back_new(nId);
    CRenderObject* pObj = NULL;

    if (*ppObj == NULL)
    {
        *ppObj = aznew CRenderObject();
    }
    pObj = *ppObj;


    pObj->AssignId(nId);
    pObj->Init();
    return pObj;
}

///////////////////////////////////////////////////////////////////////////////
CRenderObject* CRenderer::EF_DuplicateRO(CRenderObject* pObj, const SRenderingPassInfo& passInfo)
{
    CRenderObject* pObjNew = CRenderer::EF_GetObject_Temp(passInfo.ThreadID());
    pObjNew->CloneObject(pObj);
    return pObjNew;
}

///////////////////////////////////////////////////////////////////////////////
void CRenderer::FinalizeRendItems_ReorderShadowRendItems([[maybe_unused]] int nThreadID)
{
#ifndef NULL_RENDERER
    ////////////////////////////////////////////////
    // shadow rend items
    auto& rShadowRI = CRenderView::GetRenderViewForThread(nThreadID)->GetRenderItems(SG_SORT_GROUP, EFSLIST_SHADOW_GEN);
    size_t nShadowRISize = rShadowRI.size();
    if (nShadowRISize)
    {
        SRendItem* pShadowRI = &rShadowRI[0];
        std::sort(pShadowRI, pShadowRI + nShadowRISize, SCompareByShadowFrustumID());

        int nCurrentShadowRecur = 0;
        for (size_t i  = 0; i < nShadowRISize; ++i)
        {
            if (rShadowRI[i].rendItemSorter.ShadowFrustumID() != nCurrentShadowRecur)
            {
                assert(nCurrentShadowRecur < MAX_SHADOWMAP_FRUSTUMS);
                SRendItem::m_ShadowsEndRI[nThreadID][nCurrentShadowRecur] = i;
                SRendItem::m_ShadowsStartRI[nThreadID][rShadowRI[i].rendItemSorter.ShadowFrustumID()] = i;

                nCurrentShadowRecur = rShadowRI[i].rendItemSorter.ShadowFrustumID();
            }
        }

        assert(nCurrentShadowRecur < MAX_SHADOWMAP_FRUSTUMS);
        SRendItem::m_ShadowsEndRI[nThreadID][nCurrentShadowRecur] = nShadowRISize;
    }
#endif
}

///////////////////////////////////////////////////////////////////////////////
void CRenderer::FinalizeRendItems_FindShadowFrustums(int nThreadID)
{
    ////////////////////////////////////////////////
    // shadow frustums
    for (int nRecursionLevel = 0; nRecursionLevel < MAX_REND_RECURSION_LEVELS; ++nRecursionLevel)
    {
        m_RP.m_SMFrustums[nThreadID][nRecursionLevel].SetUse(0);
        m_RP.m_SMCustomFrustumIDs[nThreadID][nRecursionLevel].SetUse(0);
    }

    if (m_RP.SShadowFrustumToRenderList[nThreadID].size())
    {
        std::sort(m_RP.SShadowFrustumToRenderList[nThreadID].begin(), m_RP.SShadowFrustumToRenderList[nThreadID].end(), SCompareByLightIds());

        int nCurrentLightId = m_RP.SShadowFrustumToRenderList[nThreadID][0].nLightID;
        int nCurRecusiveLevel = m_RP.SShadowFrustumToRenderList[nThreadID][0].nRecursiveLevel;
        SRendItem::m_StartFrust[nThreadID][nCurrentLightId] = m_RP.m_SMFrustums[nThreadID][nCurRecusiveLevel].Num();
        for (int i = 0; i < (int)m_RP.SShadowFrustumToRenderList[nThreadID].size(); ++i)
        {
            SRenderPipeline::SShadowFrustumToRender& rToRender = m_RP.SShadowFrustumToRenderList[nThreadID][i];
            if (rToRender.pFrustum->nShadowGenMask != 0)
            {
                ShadowMapFrustum* pCopyFrustumTo = m_RP.m_SMFrustums[nThreadID][rToRender.nRecursiveLevel].AddIndex(1);
                memcpy(pCopyFrustumTo, rToRender.pFrustum, sizeof(ShadowMapFrustum));

                const int nFrustumIndex = m_RP.m_SMFrustums[nThreadID][nCurRecusiveLevel].Num() - 1;
                // put shadow frustum into right ligh id group
                if (rToRender.pFrustum->m_eFrustumType != ShadowMapFrustum::e_PerObject && rToRender.pFrustum->m_eFrustumType != ShadowMapFrustum::e_Nearest)
                {
                    if (rToRender.nLightID != nCurrentLightId)
                    {
                        SRendItem::m_EndFrust[nThreadID][nCurrentLightId] = nFrustumIndex;
                        SRendItem::m_StartFrust[nThreadID][rToRender.nLightID] = nFrustumIndex;

                        nCurrentLightId = rToRender.nLightID;
                        nCurRecusiveLevel = rToRender.nRecursiveLevel;
                    }
                }
                else
                {
                    m_RP.m_SMCustomFrustumIDs[nThreadID][rToRender.nRecursiveLevel].Add(nFrustumIndex);
                }
            }
        }

        // Store the end index to use when iterating over the shadow frustums.
        SRendItem::m_EndFrust[nThreadID][nCurrentLightId] = m_RP.m_SMFrustums[nThreadID][nCurRecusiveLevel].Num();
        m_RP.SShadowFrustumToRenderList[nThreadID].SetUse(0);
    }
}

///////////////////////////////////////////////////////////////////////////////
AZ::LegacyJobExecutor* CRenderer::GetGenerateRendItemJobExecutor()
{
    return &m_generateRendItemJobExecutor;
}

///////////////////////////////////////////////////////////////////////////////
AZ::LegacyJobExecutor* CRenderer::GetGenerateShadowRendItemJobExecutor()
{
    return &m_generateShadowRendItemJobExecutor;
}

///////////////////////////////////////////////////////////////////////////////
AZ::LegacyJobExecutor* CRenderer::GetGenerateRendItemJobExecutorPreProcess()
{
    return &m_generateRendItemPreProcessJobExecutor;
}

///////////////////////////////////////////////////////////////////////////////
AZ::LegacyJobExecutor* CRenderer::GetFinalizeRendItemJobExecutor(int nThreadID)
{
    return &m_finalizeRendItemsJobExecutor[nThreadID];
}

///////////////////////////////////////////////////////////////////////////////
AZ::LegacyJobExecutor* CRenderer::GetFinalizeShadowRendItemJobExecutor(int nThreadID)
{
    return &m_finalizeShadowRendItemsJobExecutor[nThreadID];
}

//////////////////////////////////////////////////////////////////////////
// IShaderPublicParams implementation class.
//////////////////////////////////////////////////////////////////////////

struct CShaderPublicParams
    : public IShaderPublicParams
{
    CShaderPublicParams()
    {
        m_nRefCount = 0;
    }

    virtual void AddRef() { m_nRefCount++; };
    virtual void Release()
    {
        if (--m_nRefCount <= 0)
        {
            delete this;
        }
    };

    virtual void SetParamCount(int nParam) { m_shaderParams.resize(nParam); }
    virtual int  GetParamCount() const { return m_shaderParams.size(); };

    virtual SShaderParam& GetParam(int nIndex)
    {
        assert(nIndex >= 0 && nIndex < (int)m_shaderParams.size());
        return m_shaderParams[nIndex];
    }

    virtual const SShaderParam& GetParam(int nIndex) const
    {
        assert(nIndex >= 0 && nIndex < (int)m_shaderParams.size());
        return m_shaderParams[nIndex];
    }

    virtual SShaderParam* GetParamByName(const char* pszName)
    {
        for (int32 i = 0; i < m_shaderParams.size(); i++)
        {
            if (azstricmp(pszName, m_shaderParams[i].m_Name.c_str()) == 0)
            {
                return &m_shaderParams[i];
            }
        }

        return 0;
    }

    virtual const SShaderParam* GetParamByName(const char* pszName) const
    {
        for (int32 i = 0; i < m_shaderParams.size(); i++)
        {
            if (azstricmp(pszName, m_shaderParams[i].m_Name.c_str()) == 0)
            {
                return &m_shaderParams[i];
            }
        }

        return 0;
    }

    virtual SShaderParam* GetParamBySemantic(uint8 eParamSemantic)
    {
        for (int i = 0; i < m_shaderParams.size(); ++i)
        {
            if (m_shaderParams[i].m_eSemantic == eParamSemantic)
            {
                return &m_shaderParams[i];
            }
        }

        return NULL;
    }


    virtual const SShaderParam* GetParamBySemantic(uint8 eParamSemantic) const
    {
        for (int i = 0; i < m_shaderParams.size(); ++i)
        {
            if (m_shaderParams[i].m_eSemantic == eParamSemantic)
            {
                return &m_shaderParams[i];
            }
        }

        return NULL;
    }

    virtual void SetParam(int nIndex, const SShaderParam& param)
    {
        assert(nIndex >= 0 && nIndex < (int)m_shaderParams.size());

        m_shaderParams[nIndex] = param;
    }

    virtual void AddParam(const SShaderParam& param)
    {
        // shouldn't add existing parameter ?
        m_shaderParams.push_back(param);
    }

    virtual void RemoveParamByName(const char* pszName)
    {
        for (int32 i = 0; i < m_shaderParams.size(); i++)
        {
            if (azstricmp(pszName, m_shaderParams[i].m_Name.c_str()) == 0)
            {
                m_shaderParams.erase(m_shaderParams.begin() + i);
            }
        }
    }

    virtual void RemoveParamBySemantic(uint8 eParamSemantic)
    {
        for (int i = 0; i < m_shaderParams.size(); ++i)
        {
            if (eParamSemantic == m_shaderParams[i].m_eSemantic)
            {
                m_shaderParams.erase(m_shaderParams.begin() + i);
            }
        }
    }

    virtual void SetParam(const char* pszName, UParamVal& pParam, EParamType nType = eType_FLOAT, uint8 eSemantic = 0)
    {
        int32 i;

        for (i = 0; i < m_shaderParams.size(); i++)
        {
            if (azstricmp(pszName, m_shaderParams[i].m_Name.c_str()) == 0)
            {
                break;
            }
        }

        if (i == m_shaderParams.size())
        {
            SShaderParam pr;
            pr.m_Name = pszName;
            pr.m_Type = nType;
            pr.m_eSemantic = eSemantic;
            m_shaderParams.push_back(pr);
        }

        SShaderParam::SetParam(pszName, &m_shaderParams, pParam);
    }

    virtual void SetShaderParams(const AZStd::vector<SShaderParam>& pParams)
    {
        m_shaderParams = pParams;
    }

    virtual void AssignToRenderParams(struct SRendParams& rParams)
    {
        if (!m_shaderParams.empty())
        {
            rParams.pShaderParams = &m_shaderParams;
        }
    }

    virtual AZStd::vector<SShaderParam>* GetShaderParams()
    {
        if (m_shaderParams.empty())
        {
            return 0;
        }

        return &m_shaderParams;
    }

    virtual const AZStd::vector<SShaderParam>* GetShaderParams() const
    {
        if (m_shaderParams.empty())
        {
            return 0;
        }

        return &m_shaderParams;
    }

    virtual uint8 GetSemanticByName(const char* szName)
    {
        static_assert(ECGP_COUNT <= 0xff, "8 bits are not enough to store all ECGParam values");

        if (strcmp(szName, "WrinkleMask0") == 0)
        {
            return ECGP_PI_WrinklesMask0;
        }
        if (strcmp(szName, "WrinkleMask1") == 0)
        {
            return ECGP_PI_WrinklesMask1;
        }
        if (strcmp(szName, "WrinkleMask2") == 0)
        {
            return ECGP_PI_WrinklesMask2;
        }

        return ECGP_Unknown;
    }

private:
    int m_nRefCount;
    AZStd::vector<SShaderParam> m_shaderParams;
};

//////////////////////////////////////////////////////////////////////////
IShaderPublicParams* CRenderer::CreateShaderPublicParams()
{
    return new CShaderPublicParams;
}

void CMotionBlur::SetupObject(CRenderObject* renderObject, const SRenderingPassInfo& passInfo)
{
    assert(renderObject);

    uint32 fillThreadId = passInfo.ThreadID();

    if (passInfo.IsRecursivePass())
    {
        return;
    }

    SRenderObjData* const __restrict renderObjectData =  renderObject->GetObjData();
    if (!renderObjectData)
    {
        return;
    }

    renderObject->m_ObjFlags &= ~FOB_HAS_PREVMATRIX;
    if (renderObjectData->m_uniqueObjectId != 0 && renderObject->m_fDistance < CRenderer::CV_r_MotionBlurMaxViewDist)
    {
        const AZ::u32 currentFrameId = passInfo.GetMainFrameID();
        const AZ::u32 bufferIndex = (currentFrameId) % CMotionBlur::s_maxObjectBuffers;
        const uintptr_t objectId = renderObjectData ? renderObjectData->m_uniqueObjectId : 0;
        if (!m_Objects[bufferIndex])
        {
            return;
        }

        auto currentIt = m_Objects[bufferIndex]->find(objectId);
        if (currentIt != m_Objects[bufferIndex]->end())
        {
            const AZ::u32 lastBufferIndex = (currentFrameId - 1) % CMotionBlur::s_maxObjectBuffers;

            auto historyIt = m_Objects[lastBufferIndex]->find(objectId);
            if (historyIt != m_Objects[lastBufferIndex]->end())
            {
                MotionBlurObjectParameters* currentParameters = &currentIt->second;
                MotionBlurObjectParameters* historyParameters = &historyIt->second;
                currentParameters->m_worldMatrix = renderObject->m_II.m_Matrix;

                const float fThreshold = CRenderer::CV_r_MotionBlurThreshold;
                if (renderObject->m_ObjFlags & (FOB_NEAREST | FOB_MOTION_BLUR) ||
                    !Matrix34::IsEquivalent(historyParameters->m_worldMatrix, currentParameters->m_worldMatrix, fThreshold))
                {
                    renderObject->m_ObjFlags |= FOB_HAS_PREVMATRIX;
                }

                currentParameters->m_updateFrameId = currentFrameId;
                currentParameters->m_renderObject = renderObject;
                return;
            }
        }

        m_FillData[fillThreadId].push_back(ObjectMap::value_type(objectId, MotionBlurObjectParameters(renderObject, renderObject->m_II.m_Matrix, currentFrameId)));
    }
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
void SRendItem::mfSortPreprocess(SRendItem* First, int Num)
{
    std::sort(First, First + Num, SCompareItemPreprocess());
}

//////////////////////////////////////////////////////////////////////////
void SRendItem::mfSortForZPass(SRendItem* First, int Num)
{
    std::sort(First, First + Num, SCompareRendItemZPass());
}

//////////////////////////////////////////////////////////////////////////
void SRendItem::mfSortByLight(SRendItem* First, int Num, bool bSort, const bool bIgnoreRePtr, bool bSortDecals)
{
    if (bSort)
    {
        if (bIgnoreRePtr)
        {
            std::sort(First, First + Num, SCompareItem_TerrainLayers());
        }
        else
        {
            if (bSortDecals)
            {
                std::sort(First, First + Num, SCompareItem_Decal());
            }
            else
            {
                std::sort(First, First + Num, SCompareRendItem());
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void SRendItem::mfSortByDist(SRendItem* First, int Num, bool bDecals, bool InvertedOrder)
{
    //Note: Temporary use stable sort for flickering hair (meshes within the same skin attachment don't have a deterministic sort order)
    int i;
    if (!bDecals)
    {
        //Pre-pass to bring in the first 8 entries. 8 cache requests can be in flight
        const int iPrefetchLoopLastIndex = min_branchless(8, Num);
        for (i = 0; i < iPrefetchLoopLastIndex; i++)
        {
            //It's safe to prefetch NULL
            PrefetchLine(First[i].pObj, offsetof(CRenderObject, m_fSort));
        }

        const int iLastValidIndex = Num - 1;

        //Note: this seems like quite a bit of work to do some prefetching but this code was generating a
        //          level 2 cache miss per iteration of the loop - Rich S
        for (i = 0; i < Num; i++)
        {
            SRendItem* pRI = &First[i];
            int iPrefetchIndex = min_branchless(i + 8, iLastValidIndex);
            PrefetchLine(First[iPrefetchIndex].pObj, offsetof(CRenderObject, m_fSort));
            CRenderObject* pObj = pRI->pObj; // no need to flush, data is only read
            assert(pObj);

            // We're prefetching on m_fSort, we're still getting some L2 cache misses on access to m_fDistance,
            // but moving them closer in memory is complicated due to an aligned array that's nestled in there...
            float fAddDist = pObj->m_fSort;
            pRI->fDist = pObj->m_fDistance + fAddDist;
        }


        if (InvertedOrder)
        {
            std::stable_sort(First, First + Num, SCompareDistInverted());
        }
        else
        {
            std::stable_sort(First, First + Num, SCompareDist());
        }
    }
    else
    {
        std::stable_sort(First, First + Num, SCompareItem_Decal());
    }
}

int16 CTexture::StreamCalculateMipsSignedFP(float fMipFactor) const
{
    assert(IsStreamed());
    const uint32 nMaxExtent = max(m_nWidth, m_nHeight);
    float currentMipFactor = fMipFactor * nMaxExtent * nMaxExtent * gRenDev->GetMipDistFactor();
    float fMip = (0.5f * logf(max(currentMipFactor, 0.1f)) / LN2 + (CRenderer::CV_r_TexturesStreamingMipBias + gRenDev->m_fTexturesStreamingGlobalMipFactor));
    int nMip = static_cast<int>(floorf(fMip * 256.0f));
    const int nNewMip = min(nMip, (m_nMips - m_CacheFileHeader.m_nMipsPersistent) << 8);
    return (int16)nNewMip;
}

float CTexture::StreamCalculateMipFactor(int16 nMipsSigned) const
{
    float fMip = nMipsSigned / 256.0f;
    float currentMipFactor = expf((fMip - (CRenderer::CV_r_TexturesStreamingMipBias + gRenDev->m_fTexturesStreamingGlobalMipFactor)) * 2.0f * LN2);

    const uint32 nMaxExtent = max(m_nWidth, m_nHeight);
    float fMipFactor = currentMipFactor / (nMaxExtent * nMaxExtent * gRenDev->GetMipDistFactor());

    return fMipFactor;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

