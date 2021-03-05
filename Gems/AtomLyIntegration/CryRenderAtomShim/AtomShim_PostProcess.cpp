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

/*
Todo:
* Erradicate StretchRect usage
* Cleanup code
* When we have a proper static branching support use it instead of shader switches inside code
*/
#include "CryRenderOther_precompiled.h"
#include "AtomShim_Renderer.h"
#include "I3DEngine.h"
#include "../Common/PostProcess/PostEffects.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////

AZStd::unique_ptr<CMotionBlur::ObjectMap> CMotionBlur::m_Objects[3];
CThreadSafeRendererContainer<CMotionBlur::ObjectMap::value_type> CMotionBlur::m_FillData[RT_COMMAND_BUF_COUNT];

bool CPostAA::Preprocess()
{
    return true;
}

void CPostAA::Render()
{
}

void CMotionBlur::InsertNewElements()
{
}

void CMotionBlur::FreeData()
{
}

bool CMotionBlur::Preprocess()
{
    return true;
}

void CMotionBlur::Render()
{
}

void CMotionBlur::GetPrevObjToWorldMat([[maybe_unused]] CRenderObject* pObj, Matrix44A& res)
{
    res = Matrix44(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1);
}

void CMotionBlur::OnBeginFrame()
{
}


bool CSunShafts::Preprocess()
{
    return true;
}

void CSunShafts::Render()
{
}


void CFilterSharpening::Render()
{
}
void CFilterBlurring::Render()
{
}


void CUnderwaterGodRays::Render()
{
}

void CVolumetricScattering::Render()
{
}

void CWaterDroplets::Render()
{
}

void CWaterFlow::Render()
{
}


void CWaterRipples::AddHit([[maybe_unused]] const Vec3& vPos, [[maybe_unused]] const float scale, [[maybe_unused]] const float strength)
{
}

void CWaterRipples::DEBUG_DrawWaterHits()
{
}

bool CWaterRipples::Preprocess()
{
    return true;
}

void CWaterRipples::Reset([[maybe_unused]] bool bOnSpecChange)
{
}

void CWaterRipples::Render()
{
}

void CScreenFrost::Render()
{
}

bool CRainDrops::Preprocess()
{
    return true;
}
void CRainDrops::Render()
{
}

bool CFlashBang::Preprocess()
{
    return true;
}

void CFlashBang::Render()
{
}

void CAlienInterference::Render()
{
}

void CGhostVision::Render()
{
}

void CHudSilhouettes::Render()
{
}

void CColorGrading::Render()
{
}

void CWaterVolume::Render()
{
}

void CSceneRain::CreateBuffers([[maybe_unused]] uint16 nVerts, [[maybe_unused]] void*& pINpVB, [[maybe_unused]] SVF_P3F_C4B_T2F* pVtxList)
{
}

int CSceneRain::CreateResources()
{
    return 1;
}

void CSceneRain::Release()
{
}

void CSceneRain::Render()
{
}

bool CSceneSnow::Preprocess()
{
    return true;
}
void CSceneSnow::Render()
{
}

int CSceneSnow::CreateResources()
{
    return 1;
}

void CSceneSnow::Release()
{
}

void CImageGhosting::Render()
{
}

void CFilterKillCamera::Render()
{
}

void CUberGamePostProcess::Render()
{
}

void CSoftAlphaTest::Render()
{
}

void CScreenBlood::Render() { }

void CPost3DRenderer::Render()
{
}

/////////////////////////////////////////////////////////////////////////////////////////////////////


namespace WaterVolumeStaticData
{
    void GetMemoryUsage([[maybe_unused]] ICrySizer* pSizer){}
}
/////////////////////////////////////////////////////////////////////////////////////////////////////

bool CSceneRain::Preprocess() { return true; }
//void CSceneRain::Render() {}
void CSceneRain::Reset([[maybe_unused]] bool bOnSpecChange) {}
void CSceneRain::OnLostDevice() {}

const char* CSceneRain::GetName() const {return 0; }
const char* CRainDrops::GetName() const {return 0; }

/////////////////////////////////////////////////////////////////////////////////////////////////////

void CSceneSnow::Reset([[maybe_unused]] bool bOnSpecChange) {}


const char* CSceneSnow::GetName() const {return 0; }

/////////////////////////////////////////////////////////////////////////////////////////////////////

bool CREPostProcess::mfDraw([[maybe_unused]] CShader* ef, [[maybe_unused]] SShaderPass* sfm)
{
    return true;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////

void ScreenFader::Render()
{

}

/////////////////////////////////////////////////////////////////////////////////////////////////////