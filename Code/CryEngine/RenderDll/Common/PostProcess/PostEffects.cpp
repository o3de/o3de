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
#include "PostEffects.h"
#include "PostProcessUtils.h"
#include "IPostEffectGroup.h"
#include <AzCore/std/sort.h>

#include "../../../Cry3DEngine/Environment/OceanEnvironmentBus.h"

AZStd::vector< CWaterRipples::SWaterHit, AZ::StdLegacyAllocator > CWaterRipples::s_pWaterHits[RT_COMMAND_BUF_COUNT];
AZStd::vector< CWaterRipples::SWaterHit, AZ::StdLegacyAllocator > CWaterRipples::s_pWaterHitsMGPU;
AZStd::vector< CWaterRipples::SWaterHitRecord, AZ::StdLegacyAllocator > CWaterRipples::m_DebugWaterHits;
Vec3 CWaterRipples::s_CameraPos = Vec3(ZERO);
Vec2 CWaterRipples::s_SimOrigin = Vec2(ZERO);
int CWaterRipples::s_nUpdateMask;
Vec4 CWaterRipples::s_vParams = Vec4(0.0f, 0.0f, 0.0f, 0.0f);
Vec4 CWaterRipples::s_vLookupParams = Vec4(0.0f, 0.0f, 0.0f, 0.0f);
bool CWaterRipples::s_bInitializeSim;

int CSunShafts::Initialize()
{
    Release();

    m_pOcclQuery = new COcclusionQuery;
    m_pOcclQuery->Create();

    return true;
}

void CSunShafts::Release()
{
    SAFE_DELETE(m_pOcclQuery);
}

void CSunShafts::Reset([[maybe_unused]] bool bOnSpecChange)
{
}

void CSunShafts::OnLostDevice()
{
    Release();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

bool CFilterSharpening::Preprocess()
{
    bool bQualityCheck = CPostEffectsMgr::CheckPostProcessQuality(eRQ_Medium, eSQ_Medium);

    if (!bQualityCheck)
    {
        return false;
    }

    if IsCVarConstAccess(constexpr) (!CRenderer::CV_r_PostProcessFilters)
    {
        return false;
    }

    if (fabs(m_pAmount->GetParam() - 1.0f) + CRenderer::CV_r_Sharpening + CRenderer::CV_r_ChromaticAberration > 0.09f)
    {
        return true;
    }

    return false;
}

void CFilterSharpening::Reset([[maybe_unused]] bool bOnSpecChange)
{
    m_pAmount->ResetParam(1.0f);
    m_pType->ResetParam(0.0f);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

bool CFilterBlurring::Preprocess()
{
    bool bQualityCheck = CPostEffectsMgr::CheckPostProcessQuality(eRQ_Medium, eSQ_Medium);

    if (!bQualityCheck)
    {
        return false;
    }

    if IsCVarConstAccess(constexpr) (!CRenderer::CV_r_PostProcessFilters)
    {
        return false;
    }

    if (m_pAmount->GetParam() > 0.09f)
    {
        return true;
    }

    return false;
}

void CFilterBlurring::Reset([[maybe_unused]] bool bOnSpecChange)
{
    m_pAmount->ResetParam(0.0f);
    m_pType->ResetParam(0.0f);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

bool CUberGamePostProcess::Preprocess()
{
    const float fParamThreshold = 1.0f / 255.0f;
    const Vec4 vWhite = Vec4(1.0f, 1.0f, 1.0f, 1.0f);

    bool bEnable = false;
    bEnable |= m_pColorTint->GetParamVec4() != vWhite;
    bEnable |= m_pNoise->GetParam() > fParamThreshold;
    bEnable |= m_pSyncWaveAmplitude->GetParam() > fParamThreshold;
    bEnable |= m_pGrainAmount->GetParam() > fParamThreshold;
    bEnable |= m_pPixelationScale->GetParam() > fParamThreshold;

    if (m_pInterlationAmount->GetParam() > fParamThreshold || m_pVSyncAmount->GetParam() > fParamThreshold)
    {
        m_nCurrPostEffectsMask |= ePE_SyncArtifacts;
        bEnable = true;
    }

    // todo: looks like some game code/flowgraph doing silly stuff - investigate
    const float fParamThresholdBackCompatibility = 0.09f;

    if (m_pChromaShiftAmount->GetParam() > fParamThresholdBackCompatibility || m_pFilterChromaShiftAmount->GetParam() > fParamThresholdBackCompatibility)
    {
        m_nCurrPostEffectsMask |= ePE_ChromaShift;
        bEnable = true;
    }

    return bEnable;
}

void CUberGamePostProcess::Reset([[maybe_unused]] bool bOnSpecChange)
{
    m_nCurrPostEffectsMask = 0;

    m_pVSyncAmount->ResetParam(0.0f);
    m_pVSyncFreq->ResetParam(1.0f);

    const Vec4 vWhite = Vec4(1.0f, 1.0f, 1.0f, 1.0f);
    m_pColorTint->ResetParamVec4(vWhite);

    m_pInterlationAmount->ResetParam(0.0f);
    m_pInterlationTiling->ResetParam(1.0f);
    m_pInterlationRotation->ResetParam(0.0f);

    m_pPixelationScale->ResetParam(0.0f);
    m_pNoise->ResetParam(0.0f);

    m_pSyncWaveFreq->ResetParam(0.0f);
    m_pSyncWavePhase->ResetParam(0.0f);
    m_pSyncWaveAmplitude->ResetParam(0.0f);

    m_pFilterChromaShiftAmount->ResetParam(0.0f);
    m_pChromaShiftAmount->ResetParam(0.0f);

    m_pGrainAmount->ResetParam(0.0f);
    m_pGrainTile->ResetParam(1.0f);

    m_pMask->Release();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

bool CColorGrading::Preprocess()
{
    // Depreceated: to be removed / replaced by UberPostProcess shader
    return false;
}

void CColorGrading::Reset([[maybe_unused]] bool bOnSpecChange)
{
    // reset user params
    m_pSaturationOffset->ResetParam(0.0f);
    m_pPhotoFilterColorOffset->ResetParamVec4(Vec4(0.0f, 0.0f, 0.0f, 0.0f));
    m_pPhotoFilterColorDensityOffset->ResetParam(0.0f);
    m_pGrainAmountOffset->ResetParam(0.0f);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

bool CUnderwaterGodRays::Preprocess()
{
    bool bQualityCheck = CPostEffectsMgr::CheckPostProcessQuality(eRQ_Medium, eSQ_Medium);
    if (!bQualityCheck)
    {
        return false;
    }

    static ICVar* pVar = iConsole->GetCVar("e_WaterOcean");

    //bool bOceanVolumeVisible = (gEnv->p3DEngine->GetOceanRenderFlags() & OCR_OCEANVOLUME_VISIBLE) != 0;
    bool godRaysEnabled = OceanToggle::IsActive() ? OceanRequest::GetGodRaysEnabled() : (CRenderer::CV_r_water_godrays == 1);
    if (godRaysEnabled && m_pAmount->GetParam() > 0.005f) // && bOceanEnabled && bOceanVolumeVisible)
    {
        float fWatLevel = SPostEffectsUtils::m_fWaterLevel;
        if (fWatLevel - 0.1f > gRenDev->GetViewParameters().vOrigin.z)
        {
            // check water level

            return true;
        }
    }

    return false;
}

void CUnderwaterGodRays::Reset([[maybe_unused]] bool bOnSpecChange)
{
    m_pAmount->ResetParam(1.0f);
    m_pQuality->ResetParam(1.0f);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

bool CVolumetricScattering::Preprocess()
{
    bool bQualityCheck = CPostEffectsMgr::CheckPostProcessQuality(eRQ_High, eSQ_High);
    if (!bQualityCheck)
    {
        return false;
    }

    if IsCVarConstAccess(constexpr) (!CRenderer::CV_r_PostProcessGameFx)
    {
        return false;
    }

    if (m_pAmount->GetParam() > 0.005f)
    {
        return true;
    }

    return false;
}

void CVolumetricScattering::Reset([[maybe_unused]] bool bOnSpecChange)
{
    m_pAmount->ResetParam(0.0f);
    m_pType->ResetParam(0.0f);
    m_pQuality->ResetParam(1.0f);
    m_pTiling->ResetParam(1.0f);
    m_pSpeed->ResetParam(1.0f);
    m_pColor->ResetParamVec4(Vec4(0.5f, 0.75f, 1.0f, 1.0f));
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Game/Hud specific post-effects
////////////////////////////////////////////////////////////////////////////////////////////////////

void CAlienInterference::Reset([[maybe_unused]] bool bOnSpecChange)
{
    m_pAmount->ResetParam(0.0f);
    m_pTintColor->ResetParamVec4(Vec4(Vec3(0.85f, 0.95f, 1.25f) * 0.5f, 1.0f));
}

bool CAlienInterference::Preprocess()
{
    bool bQualityCheck = CPostEffectsMgr::CheckPostProcessQuality(eRQ_Medium, eSQ_Medium);
    if (!bQualityCheck)
    {
        return false;
    }

    if IsCVarConstAccess(constexpr) (!CRenderer::CV_r_PostProcessGameFx)
    {
        return false;
    }

    if (m_pAmount->GetParam() > 0.09f)
    {
        return true;
    }

    return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
int CGhostVision::CreateResources()
{
    Release();

    m_pUserTex1 = CTexture::ForName("EngineAssets/Textures/user_tex1.tif",  FT_DONT_STREAM, eTF_Unknown);
    m_pUserTex2 = CTexture::ForName("EngineAssets/Textures/user_tex2.tif",  FT_DONT_STREAM, eTF_Unknown);

    return true;
}

void CGhostVision::Release()
{
    SAFE_RELEASE(m_pUserTex1);
    SAFE_RELEASE(m_pUserTex2);
}

void CGhostVision::Reset([[maybe_unused]] bool bOnSpecChange)
{
    m_pUserValue1->ResetParam(0.0f);
    m_pUserValue2->ResetParam(0.0f);
    m_pUserValue3->ResetParam(0.0f);
    m_pTintColor->ResetParamVec4(Vec4(Vec3(0.85f, 0.95f, 1.25f) * 0.5f, 1.0f));
}

bool CGhostVision::Preprocess()
{
    bool bQualityCheck = CPostEffectsMgr::CheckPostProcessQuality(eRQ_Low, eSQ_Low);
    if (!bQualityCheck)
    {
        return false;
    }

    if IsCVarConstAccess(constexpr) (!CRenderer::CV_r_PostProcessGameFx)
    {
        return false;
    }

    if (m_pUserValue1->GetParam() > 0.09f || m_pUserValue2->GetParam() > 0.09f || m_pUserValue3->GetParam() > 0.09f)
    {
        return true;
    }

    return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
bool CWaterDroplets::Preprocess()
{
    bool bQualityCheck = CPostEffectsMgr::CheckPostProcessQuality(eRQ_Medium, eSQ_Medium);
    if (!bQualityCheck)
    {
        return false;
    }

    const bool bUserActive = m_pAmount->GetParam() > 0.005f;

    bool godRaysEnabled = OceanToggle::IsActive() ? OceanRequest::GetGodRaysEnabled() : (CRenderer::CV_r_water_godrays == 1);
    if (godRaysEnabled)
    {
        return bUserActive; // user enabled override
    }

    return false;
}

void CWaterDroplets::Reset([[maybe_unused]] bool bOnSpecChange)
{
    m_pAmount->ResetParam(0.0f);
}


////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

bool CWaterFlow::Preprocess()
{
    bool bQualityCheck = CPostEffectsMgr::CheckPostProcessQuality(eRQ_Medium, eSQ_Medium);
    if (!bQualityCheck)
    {
        return false;
    }

    if IsCVarConstAccess(constexpr) (!CRenderer::CV_r_PostProcessGameFx)
    {
        return false;
    }

    if (m_pAmount->GetParam() > 0.005f)
    {
        return true;
    }

    return false;
}

void CWaterFlow::Reset([[maybe_unused]] bool bOnSpecChange)
{
    m_pAmount->ResetParam(0.0f);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

bool CWaterVolume::Preprocess()
{
    if (!gRenDev->m_RP.m_eQuality)
    {
        return false;
    }

    if (m_pAmount->GetParam() > 0.005f)
    {
        return true;
    }

    return false;
}

void CWaterVolume::Reset([[maybe_unused]] bool bOnSpecChange)
{
    m_pAmount->ResetParam(0.0f);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

bool CScreenFrost::Preprocess()
{
    bool bQualityCheck = CPostEffectsMgr::CheckPostProcessQuality(eRQ_Medium, eSQ_Medium);
    if (!bQualityCheck)
    {
        return false;
    }

    if IsCVarConstAccess(constexpr) (!CRenderer::CV_r_PostProcessGameFx)
    {
        return false;
    }

    if (m_pAmount->GetParam() > 0.09f)
    {
        return true;
    }

    return false;
}

void CScreenFrost::Reset([[maybe_unused]] bool bOnSpecChange)
{
    m_pAmount->ResetParam(0.0f);
    m_pCenterAmount->ResetParam(1.0f);
    m_fRandOffset = 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

int CRainDrops::CreateResources()
{
    Release();

    //create texture for HitEffect accumulation
    m_bFirstFrame = true;

    // Already generated ? No need to proceed
    if (!m_pDropsLst.empty())
    {
        return 1;
    }

    m_pDropsLst.reserve(m_nMaxDropsCount);
    for (int p = 0; p < m_nMaxDropsCount; p++)
    {
        SRainDrop* pDrop = new SRainDrop;
        m_pDropsLst.push_back(pDrop);
    }

    return 1;
}

void CRainDrops::Release()
{
    if (m_pDropsLst.empty())
    {
        return;
    }

    SRainDropsItor pItor, pItorEnd = m_pDropsLst.end();
    for (pItor = m_pDropsLst.begin(); pItor != pItorEnd; ++pItor)
    {
        SAFE_DELETE((*pItor));
    }
    m_pDropsLst.clear();
}


void CRainDrops::Reset([[maybe_unused]] bool bOnSpecChange)
{
    m_bFirstFrame = true;
    m_uCurrentDytex = 0;

    m_pAmount->ResetParam(0.0f);
    m_pSpawnTimeDistance->ResetParam(0.35f);
    m_pSize->ResetParam(5.0f);
    m_pSizeVar->ResetParam(2.5f);
    m_nAliveDrops = 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void CHudSilhouettes::Reset([[maybe_unused]] bool bOnSpecChange)
{
    m_pActive->ResetParam(0.0f);
    m_pAmount->ResetParam(1.0f);
    m_pType->ResetParam(1);

    FindIfSilhouettesOptimisedTechAvailable();
}

bool CHudSilhouettes::Preprocess()
{
    if ((CRenderer::CV_r_customvisions != 3) || (m_bSilhouettesOptimisedTechAvailable))
    {
        if (!CRenderer::CV_r_PostProcessGameFx ||
            !CRenderer::CV_r_customvisions ||
            gRenDev->IsPost3DRendererEnabled())
        {
            return false;
        }

        // no need to proceed
        float fType = m_pType->GetParam();
        uint32 nBatchMask = SRendItem::BatchFlags(EFSLIST_GENERAL, gRenDev->m_RP.m_pRLD) | SRendItem::BatchFlags(EFSLIST_TRANSP, gRenDev->m_RP.m_pRLD);

        if ((!(nBatchMask & FB_CUSTOM_RENDER)) && fType == 1.0f)
        {
            return false;
        }

        if (m_pAmount->GetParam() > 0.005f)
        {
            return true;
        }
    }

    return false;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void CFlashBang::Release()
{
    SAFE_DELETE(m_pGhostImage);
}

void CFlashBang::Reset([[maybe_unused]] bool bOnSpecChange)
{
    SAFE_DELETE(m_pGhostImage);
    m_pActive->ResetParam(0.0f);
    m_pTime->ResetParam(2.0f);
    m_pDifractionAmount->ResetParam(1.0f);
    m_pBlindAmount->ResetParam(0.5f);
    m_fBlindAmount = 1.0f;
    m_fSpawnTime = 0.0f;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void CSoftAlphaTest::Reset([[maybe_unused]] bool bOnSpecChange)
{
}

bool CSoftAlphaTest::Preprocess()
{
    uint32 nBatchMask = SRendItem::BatchFlags(EFSLIST_GENERAL, gRenDev->m_RP.m_pRLD);
    return CRenderer::CV_r_SoftAlphaTest != 0 && (nBatchMask & FB_SOFTALPHATEST);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

bool CImageGhosting::Preprocess()
{
    CTexture* pPrevFrame = CTexture::s_ptexPrevFrameScaled;
    if (!pPrevFrame)
    {
        m_bInit = true;
        return false;
    }

    if (m_pAmount->GetParam() > 0.09f)
    {
        return true;
    }

    m_bInit = true;

    return false;
}

void CImageGhosting::Reset([[maybe_unused]] bool bOnSpecChange)
{
    m_bInit = true;
    m_pAmount->ResetParam(0.0f);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

int CFilterKillCamera::Initialize()
{
    m_techName = "KillCameraFilter";
    m_paramName = "psParams";
    return 1;
}

bool CFilterKillCamera::Preprocess()
{
    if IsCVarConstAccess(constexpr) (!CRenderer::CV_r_PostProcessFilters)
    {
        return false;
    }

    if (m_pActive->GetParam() > 0.0f)
    {
        const int mode = int_round(m_pMode->GetParam());
        if (mode != m_lastMode)
        {
            m_blindTimer = 0.0f;
            m_lastMode = mode;
        }

        return true;
    }

    m_blindTimer = 0.0f;

    return false;
}

void CFilterKillCamera::Reset([[maybe_unused]] bool bOnSpecChange)
{
    // Game controls parameters + reset (removed from here due to a race condition).
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void CScreenBlood::Reset([[maybe_unused]] bool bOnSpecChange)
{
    m_pAmount->ResetParam(0.0f);
    m_pBorder->ResetParamVec4(Vec4(0.0f, 0.0f, 2.0f, 1.0f)); // Border: x=xOffset y=yOffset z=range w=alpha
}

bool CScreenBlood::Preprocess()
{
    return (CRenderer::CV_r_PostProcessGameFx && m_pAmount->GetParam() > 0.005f);
}

//////////////////////////////////////////////////////////////////////////

bool CPost3DRenderer::Preprocess()
{
    if (IsActive())
    {
        // Defer turning off post effect for 5 frames - sometimes the flash is left rendering on the screen
        // for a frame, if we don't render the post effect for this frame then junk will be rendered into
        // the flash. Currently the post effect is disabled at the latest point in menu code, thus this is the
        // simplest/safest fix.
        m_deferDisableFrameCountDown = 5;
    }
    else if (m_deferDisableFrameCountDown > 0)
    {
        m_deferDisableFrameCountDown--;
    }

    const bool bRender = (m_deferDisableFrameCountDown > 0) ? true : false;
    return bRender;
}

void CPost3DRenderer::Reset([[maybe_unused]] bool bOnSpecChange)
{
    // Let game code fully control its active status, otherwise in some situations
    // the post effect system will get reset between menus and game and thus this
    // will get turned off when undesired
}

//////////////////////////////////////////////////////////////////////////

// AZStd visitor class to resolve the effect parameter into the appropriate type
class FetchVisitor
{
public:

    void SetEffectParam( CEffectParam* effectParameter )
    {
        m_effectParam = effectParameter;
    }

    void operator()(float param)
    {
        m_effectParam->SetParam(param);
    }

    void operator()(const Vec4& param)
    {
        m_effectParam->SetParamVec4(param);
    }

    void operator()(const AZStd::string& param)
    {
        m_effectParam->SetParamString(param.c_str());
    }

private:

    CEffectParam* m_effectParam = nullptr;
};

// Helper function to set a CEffectParam from a group, visitor and parameter name
void SetEffectParamFromVisitor( IPostEffectGroup* group, FetchVisitor& fetchVisitor, const char* paramName, CEffectParam* effectParam /* in/out */ )
{
    PostEffectGroupParam* groupParam = group->GetParam(paramName);
    fetchVisitor.SetEffectParam( effectParam );
    AZStd::visit(fetchVisitor, *groupParam);
}

bool ScreenFader::Preprocess()
{
    if IsCVarConstAccess(constexpr) (!CRenderer::CV_r_PostProcessGameFx)
    {
        return false;
    }

    IPostEffectGroupManager* groupManager = gEnv->p3DEngine->GetPostEffectGroups();
    const PostEffectGroupList& toggledGroupList = groupManager->GetGroupsToggledThisFrame();
    bool newScreenPassAdded = false;

    // Iterate over all of the groups that had their enabled/disabled flag toggled this frame
    // If the group already exists in the screenpass list, then that should mean it either needs change its
    // state to fade-in (if it was actively fading-out), or it needs to change to fade-out (if it was actively fading in or rendering at full opacity)
    // Once a group has faded out completely, then it will be removed from the screenpass list.
    for ( auto groupIter = toggledGroupList.begin(); groupIter != toggledGroupList.end(); ++groupIter )
    {
        IPostEffectGroup* group = (*groupIter);
        bool foundGroupInList = false;
        for ( ScreenPassList::iterator passIter = m_screenPasses.begin(); passIter != m_screenPasses.end(); ++passIter )
        {
            ScreenFaderPass* pass = (*passIter);
            if ( pass->m_group == group )
            {
                foundGroupInList = true;
                if ( group->GetEnable() )
                {
                    // If the group has changed to enabled, then that means the group was actively fading out before
                    // Change its state to fade-in, and fade from its existing alpha value.
                    pass->m_fadingIn = true;
                    pass->m_fadingOut = false;
                    pass->m_fadeDirection = 1.0f;
                    pass->m_fadeDuration = pass->m_fadeInTime;
                }
                else
                {
                    // If the group has changed to disabled, then that means we were either actively fading in or rendering at full opacity.
                    // Change its state to fade out and fade from its existing alpha value
                    pass->m_fadingIn = false;
                    pass->m_fadingOut = true;
                    pass->m_fadeDirection = -1.0f;
                    pass->m_fadeDuration = pass->m_fadeOutTime;
                }
                break;
            }
        }

        // If the group was not found, then add it to the list
        if ( !foundGroupInList )
        {
            FetchVisitor groupParamVisitor;

            SetEffectParamFromVisitor( group, groupParamVisitor, "ScreenFader_Enable", m_enable );
            bool fadeIn = m_enable->GetParam() ? true : false;

            if ( fadeIn )
            {
                // The screen fader is different from the other PostEffects in the PostEffectsGroups.
                // We do not want the interpolated parameters when enabling another group since we want to render
                // multiple stacked screenfaders.  Fetch the parameters from the original PostEffectGroup for this
                // screen fader pass
                ScreenFaderPass* pass = new ScreenFaderPass();
                pass->m_group = group;
                pass->m_fadingIn = true;
                pass->m_fadeDirection = 1.0f;
                
                SetEffectParamFromVisitor( group, groupParamVisitor, "ScreenFader_FadeColor", m_fadeColor );
                pass->m_currentColor = m_fadeColor->GetParamVec4();
                
                SetEffectParamFromVisitor( group, groupParamVisitor, "ScreenFader_ScreenCoordinates", m_screenCoordinates );
                pass->m_screenCoordinates = m_screenCoordinates->GetParamVec4();
                
                SetEffectParamFromVisitor( group, groupParamVisitor, "ScreenFader_TextureName", m_fadeTextureParam );
                pass->m_fadeTexture = static_cast<CParamTexture*>(m_fadeTextureParam)->GetParamTexture();

                if (pass->m_fadeTexture)
                {
                    // Since we are manually holding onto a CTexture pointer, make sure we increment the ref count
                    pass->m_fadeTexture->AddRef();
                }
                
                SetEffectParamFromVisitor( group, groupParamVisitor, "ScreenFader_FadeOutTime", m_fadeOutTime );
                pass->m_fadeOutTime = m_fadeOutTime->GetParam();

                SetEffectParamFromVisitor( group, groupParamVisitor, "ScreenFader_FadeInTime", m_fadeInTime );
                pass->m_fadeInTime = m_fadeInTime->GetParam();
                pass->m_fadeDuration = pass->m_fadeInTime;

                m_screenPasses.push_back( pass );
                newScreenPassAdded = true;
            }
        }
    }

    // If we added a new ScreenPass, then re-sort our ScreenPasses based on the PostEffectGroup's priorities
    if ( newScreenPassAdded )
    {
        m_screenPasses.sort(SortFaderPasses);
    }
    
    // Update all of the screen passes, removing any that have recently faded out from the list.
    auto passIter = m_screenPasses.begin();
    while ( passIter != m_screenPasses.end() )
    {
        ScreenFaderPass* pass = (*passIter);

        if ( pass->m_fadingOut && pass->m_currentFadeTime <= 0.0f )
        {
            // We have finished fading out.  Now clean up the list and remove the pass.
            delete pass;
            passIter = m_screenPasses.erase(passIter);
        }
        else if ( pass->m_fadingIn && (pass->m_currentFadeTime >= pass->m_fadeDuration) )
        {
            // We have finished fading in.  Stay at 100% fade time until fade out is triggered.
            pass->m_currentFadeTime = pass->m_fadeDuration;
            pass->m_fadingIn = false;
        }
        ++passIter;
    }

    return m_screenPasses.size() > 0;
}

void ScreenFader::Reset([[maybe_unused]] bool bOnSpecChange)
{
    // Do not clear m_screenPasses here, otherwise global and default PostEffectGroups will be removed
}

bool ScreenFader::SortFaderPasses(ScreenFaderPass* pass1, ScreenFaderPass* pass2)
{
    return pass1->m_group->GetPriority() < pass2->m_group->GetPriority();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
