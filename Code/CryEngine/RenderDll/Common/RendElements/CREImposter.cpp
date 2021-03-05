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

#include "RendElement.h"
#include "CREImposter.h"
#include "I3DEngine.h"

int CREImposter::m_MemUpdated;
int CREImposter::m_MemPostponed;
int CREImposter::m_PrevMemUpdated;
int CREImposter::m_PrevMemPostponed;

IDynTexture* CREImposter::m_pScreenTexture = NULL;

bool CREImposter::IsImposterValid(const CameraViewParameters& cam, [[maybe_unused]] float fRadiusX, [[maybe_unused]] float fRadiusY, [[maybe_unused]] float fCamRadiusX, [[maybe_unused]] float fCamRadiusY,
    const int iRequiredLogResX, const int iRequiredLogResY, const uint32 dwBestEdge)
{
    if (dwBestEdge != m_nLastBestEdge)
    {
        return false;
    }

    float fTransparency = gRenDev->m_RP.m_pCurObject->m_II.m_AmbColor.a;
    if (gRenDev->m_RP.m_pShaderResources)
    {
        fTransparency *= gRenDev->m_RP.m_pShaderResources->GetStrengthValue(EFTT_OPACITY);
    }
    if (m_fCurTransparency != fTransparency)
    {
        m_fCurTransparency = fTransparency;
        return false;
    }

    // screen impostors should always be updated
    if (m_bScreenImposter)
    {
        m_vFarPoint = Vec3(0, 0, 0);
        m_vNearPoint = Vec3(0, 0, 0);
        return false;
    }
    if (m_bSplit)
    {
        return false;
    }

    Vec3 vEye = m_vPos - cam.vOrigin;
    float fDistance = vEye.GetLength();

    if (fDistance < 0.0001f)
    {
        return false;                   // to avoid float exceptions
    }
    vEye /= fDistance;

    Vec3 vOldEye = m_vFarPoint - m_LastViewParameters.vOrigin;
    float fOldEyeDist = vOldEye.GetLength();

    if (fOldEyeDist < 0.0001f)
    {
        return false;                   // to avoid float exceptions
    }
    vOldEye /= fOldEyeDist;

    float fCosAlpha = vEye * vOldEye; // dot product of normalized vectors = cosine
    if (fCosAlpha < m_fErrorToleranceCosAngle)
    {
        return false;
    }

    Vec3 curSunDir(gEnv->p3DEngine->GetSunDir().GetNormalized());
    if (m_vLastSunDir.Dot(curSunDir) < 0.995)
    {
        return false;
    }

    // equal pow-of-2 size comparison for consistent look

    if (iRequiredLogResX != m_nLogResolutionX)
    {
        return false;
    }

    if (iRequiredLogResY != m_nLogResolutionY)
    {
        return false;
    }

    if (gRenDev->m_nFrameReset != m_nFrameReset)
    {
        return false;
    }

    return true;
}

void CREImposter::ReleaseResources()
{
    SAFE_DELETE(m_pTexture);
    SAFE_DELETE(m_pScreenTexture);
    SAFE_DELETE(m_pFrontTexture);
    SAFE_DELETE(m_pTextureDepth);
}

int IntersectRayAABB(Vec3 p, Vec3 d, SMinMaxBox a, Vec3& q)
{
    float tmin = 0;
    float tmax = FLT_MAX;
    int i;
    const Vec3& min = a.GetMin();
    const Vec3& max = a.GetMax();
    for (i = 0; i < 3; i++)
    {
        if (fabs(d[i]) < 0.001f)
        {
            if (p[i] < min[i] || p[i] > max[i])
            {
                return 0;
            }
        }
        else
        {
            float ood = 1.0f / d[i];
            float t1 = (min[i] - p[i]) * ood;
            float t2 = (max[i] - p[i]) * ood;
            if (t1 > t2)
            {
                Exchange(t1, t2);
            }
            if (t1 > tmin)
            {
                tmin = t1;
            }
            if (t2 > tmax)
            {
                tmax = t2;
            }
        }
    }
    q = p + d * tmin;

    return 1;
}

Vec3 CREImposter::GetPosition()
{
    Vec3 vNearest = m_WorldSpaceBV.GetCenter();

    return vNearest;
}

void CREImposter::mfPrepare(bool bCheckOverflow)
{
    CRenderer* rd = gRenDev;
    if (bCheckOverflow)
    {
        rd->FX_CheckOverflow(0, 0, this);
    }

    CRenderObject* pObj = rd->m_RP.m_pCurObject;

    CShaderResources* pRes = rd->m_RP.m_pShaderResources;
    CShader* pShader = rd->m_RP.m_pShader;
    int nTech = rd->m_RP.m_nShaderTechnique;

    UpdateImposter();

    rd->FX_Start(pShader, nTech, pRes, this);
    rd->m_RP.m_pCurObject = pObj;

    rd->m_RP.m_pRE = this;
    rd->m_RP.m_RendNumIndices = 0;
    rd->m_RP.m_RendNumVerts = 4;
    rd->m_RP.m_FirstVertex = 0;
}
