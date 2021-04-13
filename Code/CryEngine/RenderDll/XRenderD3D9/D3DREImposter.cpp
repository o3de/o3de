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

#include "DriverD3D.h"
#include "I3DEngine.h"

//=======================================================================

CTexture* gTexture;

int IntersectRayAABB(Vec3 p, Vec3 d, SMinMaxBox a, Vec3& q);


static void GetEdgeNo(const uint32 dwEdgeNo, uint32& dwA, uint32& dwB)
{
    switch (dwEdgeNo)
    {
    case  0:
        dwA = 0;
        dwB = 1;
        break;
    case  1:
        dwA = 2;
        dwB = 3;
        break;
    case  2:
        dwA = 4;
        dwB = 5;
        break;
    case  3:
        dwA = 6;
        dwB = 7;
        break;

    case  4:
        dwA = 0;
        dwB = 2;
        break;
    case  5:
        dwA = 4;
        dwB = 6;
        break;
    case  6:
        dwA = 5;
        dwB = 7;
        break;
    case  7:
        dwA = 1;
        dwB = 3;
        break;

    case  8:
        dwA = 0;
        dwB = 4;
        break;
    case  9:
        dwA = 2;
        dwB = 6;
        break;
    case 10:
        dwA = 3;
        dwB = 7;
        break;
    case 11:
        dwA = 1;
        dwB = 5;
        break;

    default:
        assert(0);
    }
}



bool CREImposter::PrepareForUpdate()
{
    CD3D9Renderer* rd = gcpRendD3D;
    CameraViewParameters cam = rd->GetViewParameters();
    int i;

    if (SRendItem::m_RecurseLevel[rd->m_RP.m_nProcessThreadID] > 0)
    {
        return false;
    }


    float fMinX, fMaxX, fMinY, fMaxY;
    uint32 dwBestEdge = 0xffffffff;     // favor the last edge we found
    float fBestArea = FLT_MAX;

    Vec3 vCenter = GetPosition();
    Vec3 vEye = vCenter - cam.vOrigin;
    float fDistance  = vEye.GetLength();
    vEye /= fDistance;

    int32 D3DVP[4];
    rd->GetViewport(&D3DVP[0], &D3DVP[1], &D3DVP[2], &D3DVP[3]);

    Vec3 vProjPos[9];
    Vec3 vUnProjPos[9];
    for (i = 0; i < 8; i++)
    {
        if (i & 1)
        {
            vUnProjPos[i].x = m_WorldSpaceBV.GetMax().x;
        }
        else
        {
            vUnProjPos[i].x = m_WorldSpaceBV.GetMin().x;
        }

        if (i & 2)
        {
            vUnProjPos[i].y = m_WorldSpaceBV.GetMax().y;
        }
        else
        {
            vUnProjPos[i].y = m_WorldSpaceBV.GetMin().y;
        }

        if (i & 4)
        {
            vUnProjPos[i].z = m_WorldSpaceBV.GetMax().z;
        }
        else
        {
            vUnProjPos[i].z = m_WorldSpaceBV.GetMin().z;
        }
    }

    vUnProjPos[8] = vCenter;
    /*
        // test
        rd->GetIRenderAuxGeom()->SetRenderFlags(SAuxGeomRenderFlags());
        rd->GetIRenderAuxGeom()->DrawAABB(AABB(m_WorldSpaceBV.GetMin(),m_WorldSpaceBV.GetMax()),false,ColorB(0,255,255,255),eBBD_Faceted);
    */

    CameraViewParameters tempCam;
    Matrix44A viewMat, projMat;

    tempCam.fNear = cam.fNear;
    tempCam.fFar = cam.fFar;

    mathMatrixPerspectiveOffCenter(&projMat, -1, 1, 1, -1, tempCam.fNear, tempCam.fFar);

    float fOldEdgeArea = -FLT_MAX;

    // try to find minimal enclosing rectangle assuming the best projection frustum must be aligned to a AABB edge
    for (uint32 dwEdge = 0; dwEdge < 13; ++dwEdge)     // 12 edges and iteration no 13 processes the best again
    {
        uint32 dwEdgeA, dwEdgeB;

        if (dwEdge == 12 && fBestArea > fOldEdgeArea * 0.98f)      // not a lot better than old axis then keep old axis (to avoid jittering)
        {
            dwBestEdge = m_nLastBestEdge;
        }

        if (dwEdge == 12)
        {
            GetEdgeNo(dwBestEdge, dwEdgeA, dwEdgeB);      // the best again
        }
        else
        {
            GetEdgeNo(dwEdge, dwEdgeA, dwEdgeB);              // edge no dwEdge
        }
        Vec3 vEdge[2] = { vUnProjPos[dwEdgeA], vUnProjPos[dwEdgeB]};
        /*
                if(dwEdge==12)
                {
                    // axis that allows smallest enclosing rectangle
                    rd->GetIRenderAuxGeom()->DrawLine( vEdge[0],ColorB(0,0,255,255),vEdge[1],ColorB(0,255,255,255),10.0f);
                }
        */

        Vec3 vR = vEdge[0] - vEdge[1];
        Vec3 vU = (vEdge[0] - cam.vOrigin) ^ vR;

        tempCam.LookAt(cam.vOrigin, vCenter, vU);
        tempCam.GetModelviewMatrix(viewMat.GetData());
        mathVec3ProjectArray((Vec3*)&vProjPos[0].x, sizeof(Vec3), (Vec3*)&vUnProjPos[0].x, sizeof(Vec3), D3DVP, &projMat, &viewMat, &rd->m_IdentityMatrix, 9, g_CpuFlags);

        // Calculate 2D extents
        fMinX = fMinY = FLT_MAX;
        fMaxX = fMaxY = -FLT_MAX;
        for (i = 0; i < 8; i++)
        {
            if (fMinX > vProjPos[i].x)
            {
                fMinX = vProjPos[i].x;
            }
            if (fMinY > vProjPos[i].y)
            {
                fMinY = vProjPos[i].y;
            }

            if (fMaxX < vProjPos[i].x)
            {
                fMaxX = vProjPos[i].x;
            }
            if (fMaxY < vProjPos[i].y)
            {
                fMaxY = vProjPos[i].y;
            }
        }

        float fArea = (fMaxX - fMinX) * (fMaxY - fMinY);

        if (dwEdge == m_nLastBestEdge)
        {
            fOldEdgeArea = fArea;
        }

        if (fArea < fBestArea)
        {
            dwBestEdge = dwEdge;
            fBestArea = fArea;
        }
    }

    /*
        // low precision - jitters
      // LB, RB, RT, LT
      vProjPos[0] = Vec3(fMinX, fMinY, vProjPos[8].z);
      vProjPos[1] = Vec3(fMaxX, fMinY, vProjPos[8].z);
      vProjPos[2] = Vec3(fMaxX, fMaxY, vProjPos[8].z);
      vProjPos[3] = Vec3(fMinX, fMaxY, vProjPos[8].z);
      // Unproject back to the world with new z-value
      mathVec3UnprojectArray((Vec3 *)&vUnProjPos[0].x, sizeof(Vec3), (Vec3 *)&vProjPos[0].x, sizeof(Vec3), D3DVP, &projMat, &viewMat, &sIdentityMatrix, 4, g_CpuFlags);
    */
    // high precision - no jitter
    float fCamZ = (tempCam.vOrigin - vCenter).Dot(tempCam.ViewDir());
    float f = -fCamZ / tempCam.fNear;
    vUnProjPos[0] = tempCam.CamToWorld(Vec3((fMinX / D3DVP[2] * 2.0f - 1.0f) * f, (fMinY / D3DVP[3] * 2.0f - 1.0f) * f, fCamZ));
    vUnProjPos[1] = tempCam.CamToWorld(Vec3((fMaxX / D3DVP[2] * 2.0f - 1.0f) * f, (fMinY / D3DVP[3] * 2.0f - 1.0f) * f, fCamZ));
    vUnProjPos[2] = tempCam.CamToWorld(Vec3((fMaxX / D3DVP[2] * 2.0f - 1.0f) * f, (fMaxY / D3DVP[3] * 2.0f - 1.0f) * f, fCamZ));
    vUnProjPos[3] = tempCam.CamToWorld(Vec3((fMinX / D3DVP[2] * 2.0f - 1.0f) * f, (fMaxY / D3DVP[3] * 2.0f - 1.0f) * f, fCamZ));



    // test
    //  rd->GetIRenderAuxGeom()->SetRenderFlags(SAuxGeomRenderFlags());
    //  rd->GetIRenderAuxGeom()->DrawPolyline(vUnProjPos,4,true,ColorB(0,0,255,0),20);

    m_vPos = vCenter;
    Vec3 vProjCenter = (vUnProjPos[0] + vUnProjPos[1] + vUnProjPos[2] + vUnProjPos[3]) / 4.0f;
    Vec3 vDif = vProjCenter - vCenter;
    float fDerivX = vDif * tempCam.vX;
    float fDerivY = vDif * tempCam.vY;

    float fRadius = m_WorldSpaceBV.GetRadius();
    Vec3 vRight = vUnProjPos[0] - vUnProjPos[1];
    Vec3 vUp = vUnProjPos[0] - vUnProjPos[2];
    float fRadiusX = vRight.len() / 2 + fabsf(fDerivX);
    float fRadiusY = vUp.len() / 2 + fabsf(fDerivY);

    Vec3 vNearest;
    IntersectRayAABB(cam.vOrigin, vEye, m_WorldSpaceBV, vNearest);
    Vec4 v4Nearest = Vec4(vNearest, 1);
    Vec4 v4Far = Vec4(vNearest + vEye * fRadius * 2, 1);
    Vec4 v4ZRange = Vec4(0, 0, 0, 0);
    Vec4 v4Column2 = rd->m_ViewProjMatrix.GetColumn4(2);
    Vec4 v4Column3 = rd->m_ViewProjMatrix.GetColumn4(3);

    bool bScreen = false;

    float fZ = v4Nearest.Dot(v4Column2);
    float fW = v4Nearest.Dot(v4Column3);

    float fNewNear = m_fNear;
    float fNewFar = m_fFar;
    if (fabs(fW) < 0.001f)              // to avoid division by 0 (near the object Screen is used and the value doesn't matter anyway)
    {
        fNewNear = 0.0f;
        bScreen = true;
    }
    else
    {
        fNewNear = 0.999f * fZ / fW;
    }

    fZ = v4Far.Dot(v4Column2);
    fW = v4Far.Dot(v4Column3);

    if (fabs(fW) < 0.001f)              // to avoid division by 0 (near the object Screen is used and the value doesn't matter anyway)
    {
        fNewFar = 1.0f;
        bScreen = true;
    }
    else
    {
        fNewFar = fZ / fW;
    }

    float fCamRadiusX = sqrtf(cam.fWR * cam.fWR + cam.fNear * cam.fNear);
    float fCamRadiusY = sqrtf(cam.fWT * cam.fWT + cam.fNear * cam.fNear);

    float fWidth     = cam.fWR - cam.fWL;
    float fHeight    = cam.fWT - cam.fWB;

    if (!bScreen)
    {
        bScreen = (fRadiusX * cam.fNear / fDistance >= fWidth || fRadiusY * cam.fNear / fDistance >= fHeight || (fDistance - fRadiusX <= fCamRadiusX) || (fDistance - fRadiusY <= fCamRadiusY));
    }

    IDynTexture* pDT = bScreen ? m_pScreenTexture : m_pTexture;
    SDynTexture2* pDT2 = (SDynTexture2*)pDT;


    float fRequiredResX = 1024;
    float fRequiredResY = 512;

    float fTexScale = CRenderer::CV_r_imposterratio > 0.1f ? 1.0f / CRenderer::CV_r_imposterratio : 1.0f / 0.1f;

    if (!bScreen)  // outside cloud
    {
        assert(D3DVP[0] == 0 && D3DVP[1] == 0);         // otherwise the following lines don't make sense

        float fRadPixelX = (fMaxX - fMinX) * 2;     // for some reason *2 is needed, most likely /near (*4) is the correct
        float fRadPixelY = (fMaxY - fMinY) * 2;

        fRequiredResX = min(fRequiredResX, max(16.0f, fRadPixelX));
        fRequiredResY = min(fRequiredResY, max(16.0f, fRadPixelY));
    }

    int nRequiredLogXRes = LogBaseTwo((int)(fRequiredResX * fTexScale));
    int nRequiredLogYRes = LogBaseTwo((int)(fRequiredResY * fTexScale));

    if (IsImposterValid(cam, fRadiusX, fRadiusY, fCamRadiusX, fCamRadiusY, nRequiredLogXRes, nRequiredLogYRes, dwBestEdge))
    {
        if (!pDT2 || !pDT2->_IsValid())
        {
            return true;
        }

        if IsCVarConstAccess(constexpr) (!CRenderer::CV_r_cloudsupdatealways)
        {
            return false;
        }
    }
    if (pDT2)
    {
        pDT2->ResetUpdateMask();
    }

    bool bPostpone = false;
    int nCurFrame = rd->GetFrameID(false);
    if (gRenDev->GetActiveGPUCount() == 1)
    {
        if (!CRenderer::CV_r_cloudsupdatealways && !bScreen && !m_bScreenImposter && pDT && pDT->GetTexture() && m_fRadiusX && m_fRadiusY)
        {
            if (m_MemUpdated > CRenderer::CV_r_impostersupdateperframe)
            {
                bPostpone = true;
            }
            if (m_PrevMemPostponed)
            {
                int nDeltaFrames = m_PrevMemPostponed / CRenderer::CV_r_impostersupdateperframe;
                if (nCurFrame - m_FrameUpdate > nDeltaFrames)
                {
                    bPostpone = false;
                }
            }
            if (bPostpone)
            {
                m_MemPostponed += (1 << nRequiredLogXRes) * (1 << nRequiredLogYRes) * 4 / 1024;
                return false;
            }
        }
    }
    m_FrameUpdate = nCurFrame;
    m_fNear = fNewNear;
    m_fFar = fNewFar;

    Matrix44A M;


    m_LastViewParameters = cam;
    m_vLastSunDir = gEnv->p3DEngine->GetSunDir().GetNormalized();

    m_nLogResolutionX = nRequiredLogXRes;
    m_nLogResolutionY = nRequiredLogYRes;

    if (!bScreen)
    { // outside cloud
      //    m_LastCamera.TightlyFitToSphere(cam.vOrigin, vU, m_vPos, fRadiusX, fRadiusY);

        m_LastViewParameters = tempCam;
        /*
                float DistToCntr = (m_vPos-cam.vOrigin) | m_LastCamera.ViewDir();

                // are the following 2 lines correct ?
                m_LastCamera.fNear = DistToCntr-fRadiusX;
                m_LastCamera.fFar = DistToCntr+fRadiusX;
        */

        assert(D3DVP[0] == 0 && D3DVP[1] == 0);         // otherwise the following lines don't make sense

        //      float fRadX = max(-fMinX/D3DVP[2]*2.0f+1.0f,fMaxX/D3DVP[2]*2.0f-1.0f);
        //      float fRadY = max(-fMinY/D3DVP[3]*2.0f+1.0f,fMaxY/D3DVP[3]*2.0f-1.0f);
        m_LastViewParameters.fWL = (fMinX / D3DVP[2] * 2 - 1);
        m_LastViewParameters.fWR = (fMaxX / D3DVP[2] * 2 - 1);
        m_LastViewParameters.fWT = (fMaxY / D3DVP[3] * 2 - 1);
        m_LastViewParameters.fWB = (fMinY / D3DVP[3] * 2 - 1);

        m_fRadiusX = 0.5f * (m_LastViewParameters.fWR - m_LastViewParameters.fWL) * fDistance / m_LastViewParameters.fNear;
        m_fRadiusY = 0.5f * (m_LastViewParameters.fWT - m_LastViewParameters.fWB) * fDistance / m_LastViewParameters.fNear;

        m_vQuadCorners[0] = vUnProjPos[0] - m_vPos;
        m_vQuadCorners[1] = vUnProjPos[1] - m_vPos;
        m_vQuadCorners[2] = vUnProjPos[2] - m_vPos;
        m_vQuadCorners[3] = vUnProjPos[3] - m_vPos;
        m_nLastBestEdge = dwBestEdge;

        m_bScreenImposter = false;
        // store points used in later error estimation
        m_vNearPoint = -m_LastViewParameters.vZ * m_LastViewParameters.fNear + m_LastViewParameters.vOrigin;
        m_vFarPoint = -m_LastViewParameters.vZ * m_LastViewParameters.fFar + m_LastViewParameters.vOrigin;
    }
    else
    { // inside cloud
      //m_LastCamera.fFar = m_LastCamera.fNear + 3 * fRadius;
        m_bScreenImposter =  true;
    }

    return true;
}

bool CREImposter::UpdateImposter()
{
    if (!PrepareForUpdate())
    {
        return true;
    }

    //PrepareForUpdate();

    PROFILE_FRAME(Imposter_Update);

    CD3D9Renderer* rd = gcpRendD3D;

    int iResX = 1 << m_nLogResolutionX;
    int iResY = 1 << m_nLogResolutionY;

    rd->FX_SetState(GS_DEPTHWRITE);

    int iOldVP[4];
    rd->GetViewport(&iOldVP[0], &iOldVP[1], &iOldVP[2], &iOldVP[3]);

    IDynTexture** pDT; //, *pDTDepth;
    if (!m_bSplit)
    {
        if (!m_bScreenImposter)
        {
            pDT = &m_pTexture;
        }
        else
        {
            pDT = &m_pScreenTexture;
        }

        if (!*pDT)
        {
            *pDT = new SDynTexture2(iResX, iResY,  FT_STATE_CLAMP, "Imposter", eTP_Clouds);
        }

        //if (!m_pTextureDepth)
        //  m_pTextureDepth = new SDynTexture2(iResX, iResY, eTF_G16R16F, eTT_2D,  FT_STATE_CLAMP, "ImposterDepth");
#ifndef _RELEASE
        rd->m_RP.m_PS[rd->m_RP.m_nProcessThreadID].m_NumImpostersUpdates++;
#endif
        if (*pDT) // && m_pTextureDepth)
        {
            //pDTDepth = m_pTextureDepth;
            (*pDT)->Update(iResX, iResY);
            CTexture* pT = (CTexture*)(*pDT)->GetTexture();
            int nSize = pT->GetDataSize();
            m_MemUpdated += nSize / 1024;
            rd->m_RP.m_PS[rd->m_RP.m_nProcessThreadID].m_ImpostersSizeUpdate += nSize;
            //pDTDepth->Update(iResX, iResY);
            //ColorF col = ColorF(0,0,0,0);
            //pDT->m_pTexture->Fill(col);
            //pDTDepth->m_pTexture->Fill(Col_White);
            SDepthTexture* pDepth = rd->FX_GetDepthSurface(iResX, iResY, false);
            (*pDT)->ClearRT();
            (*pDT)->SetRT(0, true, pDepth);
            //rd->FX_PushRenderTarget(1, pDTDepth->m_pTexture, NULL);
            gTexture = pT;
            rd->FX_ClearTarget(pDepth);
            float fYFov, fXFov, fAspect, fNearest, fFar;
            m_LastViewParameters.GetPerspectiveParams(&fYFov, &fXFov, &fAspect, &fNearest, &fFar);
            CCamera EngCam;
            CCamera OldCam = rd->GetCamera();
            int nW = iResX;
            int nH = iResY;
            //fXFov = DEG2RAD(fXFov);
            fYFov = DEG2RAD(fYFov);
            fXFov = DEG2RAD(fXFov);
            if (m_bScreenImposter)
            {
                nW = rd->GetWidth();
                nH = rd->GetHeight();
                fXFov = EngCam.GetFov();
            }
            Matrix34 matr;
            matr = Matrix34::CreateFromVectors(m_LastViewParameters.vX, -m_LastViewParameters.vZ, m_LastViewParameters.vY, m_LastViewParameters.vOrigin);
            EngCam.SetMatrix(matr);
            EngCam.SetFrustum(nW, nH, fXFov, fNearest, fFar);
            //rd->SetCamera(EngCam);
            //m_LastCamera.fFar += 100;
            rd->m_TranspOrigCameraProjMatrix = rd->m_ViewProjMatrix.GetTransposed();
            rd->ApplyViewParameters(m_LastViewParameters);

            if (rd->m_logFileHandle != AZ::IO::InvalidHandle)
            {
                rd->Logv(SRendItem::m_RecurseLevel[rd->m_RP.m_nProcessThreadID], " +++ Start Imposter scene +++ \n");
            }

            int nFL = rd->m_RP.m_PersFlags2;
            rd->m_RP.m_TI[rd->m_RP.m_nProcessThreadID].m_PersFlags |= RBPF_IMPOSTERGEN;
            rd->m_RP.m_PersFlags2 |= RBPF2_NOALPHABLEND | RBPF2_NOALPHATEST;
            rd->m_RP.m_StateAnd &= ~(GS_BLEND_MASK | GS_ALPHATEST_MASK);

            assert(!"GetI3DEngine()->RenderImposterContent() does not exist");
            //gEnv->p3DEngine->RenderImposterContent(this, EngCam);
            rd->m_RP.m_PersFlags2 = nFL;

            if (rd->m_logFileHandle != AZ::IO::InvalidHandle)
            {
                rd->Logv(SRendItem::m_RecurseLevel[rd->m_RP.m_nProcessThreadID], " +++ End Imposter scene +++ \n");
            }

            if (rd->m_logFileHandle != AZ::IO::InvalidHandle)
            {
                rd->Logv(SRendItem::m_RecurseLevel[rd->m_RP.m_nProcessThreadID], " +++ Postprocess Imposter +++ \n");
            }

            (*pDT)->RestoreRT(0, true);
            //rd->FX_PopRenderTarget(1);

            rd->SetCamera(OldCam);
        }
    }
    rd->RT_SetViewport(iOldVP[0], iOldVP[1], iOldVP[2], iOldVP[3]);

    return true;
}

bool CREImposter::Display(bool bDisplayFrontOfSplit)
{
    if (SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID] > 0)
    {
        return false;
    }

    //return true;
    CD3D9Renderer* rd = gcpRendD3D;
    CShader* pSH = rd->m_RP.m_pShader;
    SShaderTechnique* pSHT = rd->m_RP.m_pCurTechnique;
    SShaderPass* pPass = rd->m_RP.m_pCurPass;
#ifndef _RELEASE
    rd->m_RP.m_PS[rd->m_RP.m_nProcessThreadID].m_NumImpostersDraw++;
#endif
    Vec3 vPos = m_vPos;

    uint32 nPasses = 0;
    pSH->FXBegin(&nPasses, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);

    STexState sTexStatePoint = STexState(FILTER_POINT, true);
    STexState sTexStateLinear = STexState(FILTER_LINEAR, true);
    if (!m_pTexture || (bDisplayFrontOfSplit && !m_pFrontTexture))
    {
        Warning("WRANING: CREImposter::mfDisplay: missing texture!");
    }
    else
    {
        IDynTexture* pDT;
        if (bDisplayFrontOfSplit)
        {
            pDT = m_pFrontTexture;
        }
        else
        {
            pDT = m_pTexture;

            pDT->Apply(0, CTexture::GetTexState(sTexStateLinear));
            pDT->Apply(1, CTexture::GetTexState(sTexStatePoint));
        }

        //pDT = m_pTextureDepth;
        //pDT->Apply(1);
    }

    int State = m_State; //GS_BLSRC_ONE | GS_BLDST_ONEMINUSSRCALPHA | GS_ALPHATEST_GREATER0;
    if (m_bSplit)
    {
        if (!bDisplayFrontOfSplit)
        {
            State |= GS_DEPTHWRITE;
        }
        else
        {
            State |= GS_NODEPTHTEST;
        }
    }
    rd->FX_SetState(State, m_AlphaRef);
    if (CRenderer::CV_r_usezpass && CTexture::s_ptexZTarget)
    {
        rd->FX_PushRenderTarget(1, CTexture::s_ptexZTarget, NULL);
    }

    Vec3 x, y, z;

    if (!m_bScreenImposter)
    {
        z = vPos - m_LastViewParameters.vOrigin;
        z.Normalize();
        x = (z ^ m_LastViewParameters.vY);
        x.Normalize();
        x *=    m_fRadiusX;
        y  =    (x ^ z);
        y.Normalize();
        y *=    m_fRadiusY;

        const CameraViewParameters& cam = rd->GetViewParameters();
        Matrix44A* m = &rd->m_RP.m_TI[rd->m_RP.m_nProcessThreadID].m_matView;
        cam.GetModelviewMatrix((float*)m);
        m = &rd->m_RP.m_TI[rd->m_RP.m_nProcessThreadID].m_matProj;
        mathMatrixPerspectiveOffCenter(m, cam.fWL, cam.fWR, cam.fWB, cam.fWT, cam.fNear, cam.fFar);

        rd->D3DSetCull(eCULL_None);
        pSH->FXBeginPass(0);

        rd->DrawQuad3D(vPos - y - x, vPos - y + x, vPos + y + x, vPos + y - x, Col_White, 0, 1, 1, 0);

        if IsCVarConstAccess(constexpr) (CRenderer::CV_r_impostersdraw & 4)
        {
            rd->GetIRenderAuxGeom()->DrawAABB(AABB(m_WorldSpaceBV.GetMin(), m_WorldSpaceBV.GetMax()), false, Col_White, eBBD_Faceted);
        }
        if IsCVarConstAccess(constexpr) (CRenderer::CV_r_impostersdraw & 2)
        {
            ColorB col = Col_Yellow; //ColorB(colR<<4, colG<<4, colB<<4, 255);
            Vec3 v[4];
            v[0] = vPos - y - x;
            v[1] = vPos - y + x;
            v[2] = vPos + y + x;
            v[3] = vPos + y - x;
            vtx_idx inds[6];
            inds[0] = 0;
            inds[1] = 1;
            inds[2] = 2;
            inds[3] = 0;
            inds[4] = 2;
            inds[5] = 3;

            SAuxGeomRenderFlags auxFlags;
            auxFlags.SetFillMode(e_FillModeWireframe);
            auxFlags.SetDepthTestFlag(e_DepthTestOn);
            rd->GetIRenderAuxGeom()->SetRenderFlags(auxFlags);
            rd->GetIRenderAuxGeom()->DrawTriangles(v, 4, inds, 6, col);
        }
    }
    else
    {
        x = m_LastViewParameters.vX;
        x *= 0.5f * (m_LastViewParameters.fWR - m_LastViewParameters.fWL);
        y = m_LastViewParameters.vY;
        y *= 0.5f * (m_LastViewParameters.fWT - m_LastViewParameters.fWB);
        z = -m_LastViewParameters.vZ;
        z *= m_LastViewParameters.fNear;

        if IsCVarConstAccess(constexpr) (CRenderer::CV_r_impostersdraw & 4)
        {
            rd->GetIRenderAuxGeom()->DrawAABB(AABB(m_WorldSpaceBV.GetMin(), m_WorldSpaceBV.GetMax()), false, Col_Red, eBBD_Faceted);
        }
        // draw a polygon with this texture...
        Matrix44A origMatProj = rd->m_RP.m_TI[rd->m_RP.m_nProcessThreadID].m_matProj;
        mathMatrixOrthoOffCenter(&rd->m_RP.m_TI[rd->m_RP.m_nProcessThreadID].m_matProj, -1, 1, -1, 1, -1, 1);

        Matrix44A origMatView = rd->m_RP.m_TI[rd->m_RP.m_nProcessThreadID].m_matView;
        rd->m_RP.m_TI[rd->m_RP.m_nProcessThreadID].m_matView.SetIdentity();
        pSH->FXBeginPass(0);

        rd->DrawQuad3D(Vec3(-1, -1, 0), Vec3(1, -1, 0), Vec3(1, 1, 0), Vec3(-1, 1, 0), Col_White, 0, 1, 1, 0);

        rd->m_RP.m_TI[rd->m_RP.m_nProcessThreadID].m_matView = origMatView;
        rd->m_RP.m_TI[rd->m_RP.m_nProcessThreadID].m_matProj = origMatProj;
    }

    pSH->FXEndPass();
    pSH->FXEnd();

    if (CRenderer::CV_r_usezpass && CTexture::s_ptexZTarget)
    {
        rd->FX_PopRenderTarget(1);
    }

    rd->m_RP.m_pShader = pSH;
    rd->m_RP.m_pCurTechnique = pSHT;
    rd->m_RP.m_pCurPass = pPass;

    return true;
}

bool CREImposter::mfDraw([[maybe_unused]] CShader* ef, [[maybe_unused]] SShaderPass* pPass)
{
    if IsCVarConstAccess(constexpr) (!CRenderer::CV_r_impostersdraw)
    {
        return true;
    }

    Display(false);

    if (IsSplit())
    {
        // display contained instances -- first opaque, then transparent.
        /*InstanceIterator ii;
        for (ii = containedOpaqueInstances.begin(); ii != containedOpaqueInstances.end(); ++ii)
        FAIL_RETURN((*ii)->Display());
        for (ii = containedTransparentInstances.begin(); ii != containedTransparentInstances.end(); ++ii)
        FAIL_RETURN((*ii)->Display());*/

        // now display the front half of the split impostor.
        Display(true);
    }
    return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////
