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
#include "ShadowUtils.h"

bool CShadowUtils::bShadowFrustumCacheValid = false;

void CShadowUtils::CalcDifferentials(const CCamera& cam, float fViewWidth, [[maybe_unused]] float fViewHeight, float& fFragSizeX)
{
    Vec3 vNearEdge = cam.GetEdgeN();

    float fWorldWidthDiv2 = abs(vNearEdge.x);

    fFragSizeX = (fWorldWidthDiv2 * 2.0f) / fViewWidth;
}

void CShadowUtils::CalcScreenToWorldExpansionBasis(const CCamera& cam, const Vec2& vJitter, float fViewWidth, float fViewHeight, Vec3& vWBasisX, Vec3& vWBasisY, Vec3& vWBasisZ, bool bWPos)
{
    const Matrix34& camMatrix = cam.GetMatrix();

    Vec3 vNearEdge = cam.GetEdgeN();

    //all values are in camera space
    float fFar = cam.GetFarPlane();
    float fNear = abs(vNearEdge.y);
    float fWorldWidthDiv2 = abs(vNearEdge.x);
    float fWorldHeightDiv2 = abs(vNearEdge.z);

    float k = fFar / fNear;

    Vec3 vNearX = camMatrix.GetColumn0().GetNormalized() * fWorldWidthDiv2;
    Vec3 vNearY = camMatrix.GetColumn2().GetNormalized() * fWorldHeightDiv2;

    Vec3 vJitterShiftX = vNearX * vJitter.x;
    Vec3 vJitterShiftY = vNearY * vJitter.y;

    Vec3 vStereoShift = camMatrix.GetColumn0().GetNormalized() * cam.GetAsymL() + camMatrix.GetColumn2() * cam.GetAsymB();

    Vec3 vZ = (camMatrix.GetColumn1().GetNormalized() * fNear + vStereoShift + vJitterShiftX + vJitterShiftY) * k; // size of vZ is the distance from camera pos to near plane
    Vec3 vX = camMatrix.GetColumn0().GetNormalized() * (fWorldWidthDiv2 * k);
    Vec3 vY = camMatrix.GetColumn2().GetNormalized() * (fWorldHeightDiv2 * k);

    //WPos basis adjustments
    if (bWPos)
    {
        vZ = vZ - vX;
        vX *= (2.0f / fViewWidth);

        vZ = vZ + vY;
        vY *= -(2.0f / fViewHeight);
    }

    vWBasisX = vX;
    vWBasisY = vY;
    vWBasisZ = vZ;
}

void CShadowUtils::ProjectScreenToWorldExpansionBasis(const Matrix44r& mShadowTexGen, const CCamera& cam, const Vec2& vJitter, float fViewWidth, float fViewHeight, Vec4r& vWBasisX, Vec4r& vWBasisY, Vec4r& vWBasisZ, Vec4r& vCamPos, bool bWPos, SRenderTileInfo* pSTileInfo)
{
    const Matrix34& camMatrix = cam.GetMatrix();
    const bool bTileRendering = pSTileInfo && (pSTileInfo->nGridSizeX > 1 || pSTileInfo->nGridSizeY > 1);

    Vec3 vNearEdge = cam.GetEdgeN();

    //all values are in camera space
    float fFar = cam.GetFarPlane();
    float fNear = abs(vNearEdge.y);
    float fWorldWidthDiv2 = abs(vNearEdge.x);
    float fWorldHeightDiv2 = abs(vNearEdge.z);

    float k = fFar / fNear;

    //simple non-general hack to shift stereo with off center projection
    Vec3 vStereoShift = camMatrix.GetColumn0().GetNormalized() * cam.GetAsymL() + camMatrix.GetColumn2() * cam.GetAsymB();

    Vec3 vNearX = camMatrix.GetColumn0().GetNormalized() * fWorldWidthDiv2;
    Vec3 vNearY = camMatrix.GetColumn2().GetNormalized() * fWorldHeightDiv2;
    Vec3 vNearZ = camMatrix.GetColumn1().GetNormalized() * fNear;

    Vec3 vJitterShiftX = vNearX * vJitter.x;
    Vec3 vJitterShiftY = vNearY * vJitter.y;

    Vec3 vZ = (vNearZ + vJitterShiftX + vJitterShiftY + vStereoShift) * k; // size of vZ is the distance from camera pos to near plane

    Vec3 vX = camMatrix.GetColumn0().GetNormalized() * fWorldWidthDiv2 * k;
    Vec3 vY = camMatrix.GetColumn2().GetNormalized() * fWorldHeightDiv2 * k;

    //multi-tiled render handling
    if (bTileRendering)
    {
        vZ = vZ + (vX * (2.0f * (pSTileInfo->nGridSizeX - 1.0f - pSTileInfo->nPosX) / pSTileInfo->nGridSizeX));
        vZ = vZ - (vY * (2.0f * (pSTileInfo->nGridSizeY - 1.0f - pSTileInfo->nPosY) / pSTileInfo->nGridSizeY));
    }

    //WPos basis adjustments
    if (bWPos)
    {
        vZ = vZ - vX;
        vX *= (2.0f / fViewWidth);

        vZ = vZ + vY;
        vY *= -(2.0f / fViewHeight);
    }

    //multi-tiled render handling
    if (bTileRendering)
    {
        vX *= 1.0f / pSTileInfo->nGridSizeX;
        vY *= 1.0f / pSTileInfo->nGridSizeY;
    }

    //creating common projection matrix for depth reconstruction
    vWBasisX = mShadowTexGen * Vec4r(vX, 0.0f);
    vWBasisY = mShadowTexGen * Vec4r(vY, 0.0f);
    vWBasisZ = mShadowTexGen * Vec4r(vZ, 0.0f);
    vCamPos =  mShadowTexGen * Vec4r(cam.GetPosition(), 1.0f);
}

void CShadowUtils::CalcLightBoundRect(const SRenderLight* pLight, const CameraViewParameters& RCam, Matrix44A& mView, Matrix44A& mProj, Vec2* pvMin, Vec2* pvMax, IRenderAuxGeom* pAuxRend)
{
    Vec3 vViewVec = Vec3(pLight->m_Origin - RCam.vOrigin);
    float fDistToLS =  vViewVec.GetLength();

    if (fDistToLS <= pLight->m_fRadius)
    {
        //optimization when we are inside light frustum
        *pvMin = Vec2(0, 0);
        *pvMax = Vec2(1, 1);
        return;
    }


    float fRadiusSquared = pLight->m_fRadius * pLight->m_fRadius;
    float fDistToBoundPlane = fRadiusSquared / fDistToLS;

    float fQuadEdge = sqrtf(fRadiusSquared - fDistToBoundPlane * fDistToBoundPlane);

    vViewVec.SetLength(fDistToLS - fDistToBoundPlane);

    Vec3 vCenter = RCam.vOrigin +  vViewVec;

    Vec3 vUp = vViewVec.Cross(RCam.vY.Cross(vViewVec));
    Vec3 vRight = vViewVec.Cross(RCam.vX.Cross(vViewVec));

    vUp.normalize();
    vRight.normalize();

    float fRadius = pLight->m_fRadius;

    Vec3 pBRectVertices[4] =
    {
        vCenter + (vUp * fQuadEdge) -  (vRight * fQuadEdge),
        vCenter + (vUp * fQuadEdge) +  (vRight * fQuadEdge),
        vCenter - (vUp * fQuadEdge) +  (vRight * fQuadEdge),
        vCenter - (vUp * fQuadEdge) -  (vRight * fQuadEdge)
    };

    Vec3 pBBoxVertices[8] =
    {
        Vec3(Vec3(vCenter.x, vCenter.y, vCenter.z) + Vec3(-fRadius, -fRadius, fRadius)),
        Vec3(Vec3(vCenter.x, vCenter.y, vCenter.z) + Vec3(-fRadius, fRadius, fRadius)),
        Vec3(Vec3(vCenter.x, vCenter.y, vCenter.z) + Vec3(fRadius, -fRadius, fRadius)),
        Vec3(Vec3(vCenter.x, vCenter.y, vCenter.z) + Vec3(fRadius, fRadius, fRadius)),

        Vec3(Vec3(vCenter.x, vCenter.y, vCenter.z) + Vec3(-fRadius, -fRadius, -fRadius)),
        Vec3(Vec3(vCenter.x, vCenter.y, vCenter.z) + Vec3(-fRadius, fRadius, -fRadius)),
        Vec3(Vec3(vCenter.x, vCenter.y, vCenter.z) + Vec3(fRadius, -fRadius, -fRadius)),
        Vec3(Vec3(vCenter.x, vCenter.y, vCenter.z) + Vec3(fRadius, fRadius, -fRadius))
    };

    Matrix44A mInvView = mView.GetInverted();

    *pvMin = Vec2(1, 1);
    *pvMax = Vec2(0, 0);

    for (int i = 0; i < 4; i++)
    {
        if (pAuxRend != NULL)
        {
            pAuxRend->DrawPoint(pBRectVertices[i], RGBA8(0xff, 0xff, 0xff, 0xff), 10);

            int32 nPrevVert = (i - 1) < 0 ? 3 : (i - 1);

            pAuxRend->DrawLine(pBRectVertices[nPrevVert], RGBA8(0xff, 0xff, 0x0, 0xff), pBRectVertices[i], RGBA8(0xff, 0xff, 0x0, 0xff), 3.0f);
        }


        Vec4 vScreenPoint = Vec4(pBRectVertices[i], 1.0) * mProj;

        //projection space clamping
        vScreenPoint.w = max(vScreenPoint.w, 0.00000000000001f);

        vScreenPoint.x = max(vScreenPoint.x, -(vScreenPoint.w));
        vScreenPoint.x = min(vScreenPoint.x, vScreenPoint.w);
        vScreenPoint.y = max(vScreenPoint.y, -(vScreenPoint.w));
        vScreenPoint.y = min(vScreenPoint.y, vScreenPoint.w);
        vScreenPoint /= vScreenPoint.w;

        Vec2 vWin;
        vWin.x = (1 + vScreenPoint.x) / 2;
        vWin.y = (1 + vScreenPoint.y) / 2;

        assert(vWin.x >= 0.0f && vWin.x <= 1.0f);
        assert(vWin.y >= 0.0f && vWin.y <= 1.0f);

        pvMin->x = min(pvMin->x, vWin.x);
        pvMin->y = min(pvMin->y, vWin.y);
        pvMax->x = max(pvMax->x, vWin.x);
        pvMax->y = max(pvMax->y, vWin.y);
    }
}

void CShadowUtils::GetProjectiveTexGen(const SRenderLight* pLight, int nFace, Matrix44A* mTexGen)
{
    assert(pLight != NULL);
    float fOffsetX = 0.5f;
    float fOffsetY = 0.5f;
    Matrix44A mTexScaleBiasMat = Matrix44A(0.5f,     0.0f,     0.0f,    0.0f,
            0.0f,    -0.5f,     0.0f,    0.0f,
            0.0f,     0.0f,     1.0f,    0.0f,
            fOffsetX, fOffsetY, 0.0f,    1.0f);

    Matrix44A mLightProj, mLightView;
    CShadowUtils::GetCubemapFrustumForLight(pLight, nFace, pLight->m_fLightFrustumAngle * 2.f, &mLightProj, &mLightView, true);

    Matrix44 mLightViewProj = mLightView * mLightProj;

    *mTexGen = mLightViewProj * mTexScaleBiasMat;
}

void CShadowUtils::GetCubemapFrustumForLight(const SRenderLight* pLight, int nS, float fFov, Matrix44A* pmProj, Matrix44A* pmView, bool bProjLight)
{
    Vec3 zaxis, yaxis, xaxis;

    //new cubemap calculation
    float sCubeVector[6][7] =
    {
        {1, 0, 0,  0, 0, -1, -90},//posx
        {-1, 0, 0, 0, 0, 1,  90},//negx
        {0, 1, 0,  -1, 0, 0, 0},//posy
        {0, -1, 0, 1, 0, 0,  0},//negy
        {0, 0, 1,  0, -1, 0,  0},//posz
        {0, 0, -1, 0, 1, 0,  0},//negz
    };

    Vec3 vForward = Vec3(sCubeVector[nS][0], sCubeVector[nS][1], sCubeVector[nS][2]);
    Vec3 vUp      = Vec3(sCubeVector[nS][3], sCubeVector[nS][4], sCubeVector[nS][5]);
    Vec3 vEyePt = pLight->m_Origin;
    vForward = vForward + vEyePt;

    //adjust light rotation
    Matrix34 mLightRot = pLight->m_ObjMatrix;

    //coord systems conversion(from orientation to shader matrix)
    if (bProjLight)
    {
        zaxis = mLightRot.GetColumn1().GetNormalized();
        yaxis = -mLightRot.GetColumn0().GetNormalized();
        xaxis = -mLightRot.GetColumn2().GetNormalized();
    }
    else
    {
        zaxis = mLightRot.GetColumn2().GetNormalized();
        yaxis = -mLightRot.GetColumn0().GetNormalized();
        xaxis = mLightRot.GetColumn1().GetNormalized();
    }

    (*pmView)(0, 0) = xaxis.x;
    (*pmView)(0, 1) = zaxis.x;
    (*pmView)(0, 2) = yaxis.x;
    (*pmView)(0, 3) = 0;
    (*pmView)(1, 0) = xaxis.y;
    (*pmView)(1, 1) = zaxis.y;
    (*pmView)(1, 2) = yaxis.y;
    (*pmView)(1, 3) = 0;
    (*pmView)(2, 0) = xaxis.z;
    (*pmView)(2, 1) = zaxis.z;
    (*pmView)(2, 2) = yaxis.z;
    (*pmView)(2, 3) = 0;
    (*pmView)(3, 0) = -xaxis.Dot(vEyePt);
    (*pmView)(3, 1) = -zaxis.Dot(vEyePt);
    (*pmView)(3, 2) = -yaxis.Dot(vEyePt);
    (*pmView)(3, 3) = 1;

    float zn = max(pLight->m_fProjectorNearPlane, 0.01f);
    float zf = max(pLight->m_fRadius, zn + 0.01f);
    mathMatrixPerspectiveFov(pmProj, (float)DEG2RAD_R(fFov), 1.f, zn, zf);
}

void CShadowUtils::GetCubemapFrustum(EFrustum_Type eFrustumType, ShadowMapFrustum* pFrust, int nS, Matrix44A* pmProj, Matrix44A* pmView, Matrix33* pmLightRot)
{
    float sCubeVector[6][7] =
    {
        {1, 0, 0,  0, 0, -1, -90},//posx
        {-1, 0, 0, 0, 0, 1,  90},//negx
        {0, 1, 0,  -1, 0, 0, 0},//posy
        {0, -1, 0, 1, 0, 0,  0},//negy
        {0, 0, 1,  0, -1, 0,  0},//posz
        {0, 0, -1, 0, 1, 0,  0},//negz
    };

    Vec3 vForward = Vec3(sCubeVector[nS][0], sCubeVector[nS][1], sCubeVector[nS][2]);
    Vec3 vUp      = Vec3(sCubeVector[nS][3], sCubeVector[nS][4], sCubeVector[nS][5]);
    float fMinDist = max(pFrust->fNearDist, 0.03f);
    float fMaxDist = pFrust->fFarDist;

    Vec3 vEyePt = Vec3(
            pFrust->vLightSrcRelPos.x + pFrust->vProjTranslation.x,
            pFrust->vLightSrcRelPos.y + pFrust->vProjTranslation.y,
            pFrust->vLightSrcRelPos.z + pFrust->vProjTranslation.z
            );
    Vec3 At = vEyePt;

    if (eFrustumType == FTYP_OMNILIGHTVOLUME)
    {
        vEyePt -= (2.0f * fMinDist) * vForward.GetNormalized();
    }

    vForward = vForward + At;

    //mathMatrixTranslation(&mLightTranslate, -vPos.x, -vPos.y, -vPos.z);2
    mathMatrixLookAt(pmView, vEyePt, vForward, vUp);

    //adjust light rotation
    if (pmLightRot)
    {
        (*pmView) = (*pmView) * (*pmLightRot);
    }
    if (eFrustumType == FTYP_SHADOWOMNIPROJECTION)
    {
        //near plane does have big influence on the precision (depth distribution) for non linear frustums
        mathMatrixPerspectiveFov(pmProj, (float)DEG2RAD_R(g_fOmniShadowFov), 1.f, fMinDist, fMaxDist);
    }
    else
    if (eFrustumType == FTYP_OMNILIGHTVOLUME)
    {
        //near plane should be extremely small in order to avoid  seams on between frustums
        mathMatrixPerspectiveFov(pmProj, (float)DEG2RAD_R(g_fOmniLightFov), 1.f, fMinDist, fMaxDist);
    }
}

Matrix34 CShadowUtils::GetAreaLightMatrix(const SRenderLight* pLight, Vec3 vScale)
{
    // Box needs to be scaled by 2x for correct radius.
    vScale *= 2.0f;

    // Add width and height to scale.
    float fFOVScale = 2.16f * (max(0.001f, pLight->m_fLightFrustumAngle * 2.0f) / 180.0f);
    vScale.y += pLight->m_fAreaWidth * fFOVScale;
    vScale.z += pLight->m_fAreaHeight * fFOVScale;

    Matrix34 mAreaMatr;
    mAreaMatr.SetIdentity();
    mAreaMatr.SetScale(vScale, pLight->m_Origin);

    // Apply rotation.
    mAreaMatr = pLight->m_ObjMatrix * mAreaMatr;

    // Move box center to light center and pull it back slightly.
    Vec3 vOffsetDir = vScale.y * 0.5f * pLight->m_ObjMatrix.GetColumn1().GetNormalized() +
        vScale.z * 0.5f * pLight->m_ObjMatrix.GetColumn2().GetNormalized() +
        0.1f * pLight->m_ObjMatrix.GetColumn0().GetNormalized();

    mAreaMatr.SetTranslation(pLight->m_Origin - vOffsetDir);

    return mAreaMatr;
}

static _inline Vec3 frac(Vec3 in)
{
    Vec3 out;
    out.x = in.x - (float)(int)in.x;
    out.y = in.y - (float)(int)in.y;
    out.z = in.z - (float)(int)in.z;

    return out;
}

float snap_frac2(float fVal, float fSnap)
{
    float fValSnapped = fSnap * int64(fVal / fSnap);
    return fValSnapped;
}

//Right Handed
void CShadowUtils::mathMatrixLookAtSnap(Matrix44A* pMatr, const Vec3& Eye, const Vec3& At, ShadowMapFrustum* pFrust)
{
    const Vec3 vZ(0.f, 0.f, 1.f);
    const Vec3 vY(0.f, 1.f, 0.f);

    Vec3 vUp;
    Vec3 vEye = Eye;
    Vec3 vAt = At;

    Vec3 vLightDir = (vEye - vAt);
    vLightDir.Normalize();

    if (fabsf(vLightDir.Dot(vZ)) > 0.9995f)
    {
        vUp = vY;
    }
    else
    {
        vUp = vZ;
    }

    float fKatetSize = 1000000.0f  * tan_tpl(DEG2RAD(pFrust->fFOV) * 0.5f);

    assert(pFrust->nTexSize > 0);
    float fSnapXY = fKatetSize * 2.f / pFrust->nTexSize; //texture size should be valid already

    //TD - add ratios to the frustum
    fSnapXY *= 2.0f;

    Vec3 zaxis = vLightDir.GetNormalized();
    Vec3 xaxis = (vUp.Cross(zaxis)).GetNormalized();
    Vec3 yaxis = zaxis.Cross(xaxis);

    (*pMatr)(0, 0) = xaxis.x;
    (*pMatr)(0, 1) = yaxis.x;
    (*pMatr)(0, 2) = zaxis.x;
    (*pMatr)(0, 3) = 0;
    (*pMatr)(1, 0) = xaxis.y;
    (*pMatr)(1, 1) = yaxis.y;
    (*pMatr)(1, 2) = zaxis.y;
    (*pMatr)(1, 3) = 0;
    (*pMatr)(2, 0) = xaxis.z;
    (*pMatr)(2, 1) = yaxis.z;
    (*pMatr)(2, 2) = zaxis.z;
    (*pMatr)(2, 3) = 0;
    (*pMatr)(3, 0) = -xaxis.Dot(vEye);
    (*pMatr)(3, 1) = -yaxis.Dot(vEye);
    (*pMatr)(3, 2) = -zaxis.Dot(vEye);
    (*pMatr)(3, 3) = 1;

    float fTranslX = (*pMatr)(3, 0);
    float fTranslY = (*pMatr)(3, 1);

    (*pMatr)(3, 0) = snap_frac2(fTranslX, fSnapXY);
    (*pMatr)(3, 1) = snap_frac2(fTranslY, fSnapXY);
}

//todo move frustum computations to the 3d engine
void CShadowUtils::GetShadowMatrixOrtho(Matrix44A& mLightProj, Matrix44A& mLightView, const Matrix44A& mViewMatrix, ShadowMapFrustum* lof, bool bViewDependent)
{
    mathMatrixPerspectiveFov(&mLightProj, (float)DEG2RAD_R(max(lof->fFOV, 0.0000001f)), max(lof->fProjRatio, 0.0001f), lof->fNearDist, lof->fFarDist);

    const Vec3 zAxis(0.f, 0.f, 1.f);
    const Vec3 yAxis(0.f, 1.f, 0.f);
    Vec3 Up;
    Vec3 Eye = Vec3(
            lof->vLightSrcRelPos.x + lof->vProjTranslation.x,
            lof->vLightSrcRelPos.y + lof->vProjTranslation.y,
            lof->vLightSrcRelPos.z + lof->vProjTranslation.z);
    Vec3 At = Vec3(lof->vProjTranslation.x, lof->vProjTranslation.y, lof->vProjTranslation.z);

    Vec3 vLightDir = At - Eye;
    vLightDir.Normalize();

    if (bViewDependent)
    {
        Eye = mViewMatrix.GetTransposed().TransformPoint(Eye);
        At = (mViewMatrix.GetTransposed()).TransformPoint(At);
        vLightDir = (mViewMatrix.GetTransposed()).TransformVector(vLightDir);
    }

    //get look-at matrix
    if (CRenderer::CV_r_ShadowsGridAligned && lof->m_Flags & DLF_DIRECTIONAL)
    {
        mathMatrixLookAtSnap(&mLightView, Eye, At, lof);
    }
    else
    {
        if (fabsf(vLightDir.Dot(zAxis)) > 0.9995f)
        {
            Up = yAxis;
        }
        else
        {
            Up = zAxis;
        }

        mathMatrixLookAt(&mLightView, Eye, At, Up);
    }

    //we should transform coords to the view space, so shadows are oriented according to camera always
    if (bViewDependent)
    {
        mLightView = mViewMatrix * mLightView;
    }
}

//todo move frustum computations to the 3d engine
void CShadowUtils::GetShadowMatrixForObject(Matrix44A& mLightProj, Matrix44A& mLightView, ShadowMapFrustum* lof)
{
    const AABB& aabb = lof->aabbCasters;

    if (aabb.GetRadius() < 0.001f)
    {
        mLightProj.SetIdentity();
        mLightView.SetIdentity();
        lof->fNearDist = 0.1f;
        lof->fFarDist = 100.0f;
        lof->fDepthTestBias = 0.00001f;
        return;
    }

    //Ortho
    f32 yScale = aabb.GetRadius() * 1.11f;
    f32 xScale = yScale;
    f32 fNear = lof->vLightSrcRelPos.GetLength();
    f32 fFar = fNear;
    fNear -= aabb.GetRadius();
    fFar  += aabb.GetRadius();
    mathMatrixOrtho(&mLightProj, yScale, xScale, fNear, fFar);
    const Vec3 zAxis(0.f, 0.f, 1.f);
    const Vec3 yAxis(0.f, 1.f, 0.f);
    Vec3 Up;
    Vec3 At = aabb.GetCenter();
    Vec3 vLightDir = -lof->vLightSrcRelPos;
    vLightDir.Normalize();

    Vec3 Eye = At - lof->vLightSrcRelPos.len() * vLightDir;

    if (fabsf(vLightDir.Dot(zAxis)) > 0.9995f)
    {
        Up = yAxis;
    }
    else
    {
        Up = zAxis;
    }

    mathMatrixLookAt(&mLightView, Eye, At, Up);

    lof->fNearDist = fNear;
    lof->fFarDist = fFar;
    lof->fDepthTestBias = 0.00001f;
}

AABB CShadowUtils::GetShadowMatrixForCasterBox(Matrix44A& mLightProj, Matrix44A& mLightView, ShadowMapFrustum* lof, float fFarPlaneOffset)
{
    GetShadowMatrixForObject(mLightProj, mLightView, lof);

    AABB lightSpaceBounds = AABB::CreateTransformedAABB(Matrix34(mLightView.GetTransposed()), lof->aabbCasters);
    Vec3 lightSpaceRange = lightSpaceBounds.GetSize();

    const float fNear = -lightSpaceBounds.max.z;
    const float fFar =  -lightSpaceBounds.min.z + fFarPlaneOffset;

    const float yfov = atan_tpl(lightSpaceRange.y * 0.5f / fNear) * 2.f;
    const float aspect = lightSpaceRange.x / lightSpaceRange.y;

    mathMatrixPerspectiveFov(&mLightProj, yfov, aspect, fNear, fFar);

    return lightSpaceBounds;
}


CShadowUtils::CShadowUtils()
{
}

CShadowUtils::~CShadowUtils()
{
}


Vec2& CPoissonDiskGen::GetSample(int ind)
{
    assert(ind < m_vSamples.size() && ind >= 0);
    return m_vSamples[ind];
}

// the size of the kernel for each entry is the index in this vector
StaticInstance<std::vector<CPoissonDiskGen>> s_kernelSizeGens;
CPoissonDiskGen& CPoissonDiskGen::GetGenForKernelSize(int size)
{
    if ((int)s_kernelSizeGens.size() <= size)
    {
        s_kernelSizeGens.resize(size + 1);
    }

    CPoissonDiskGen& pdg = s_kernelSizeGens[size];

    if (pdg.m_vSamples.size() == 0)
    {
        pdg.m_vSamples.resize(size);
        pdg.InitSamples();
    }

    return pdg;
}

void CPoissonDiskGen::FreeMemory()
{
    s_kernelSizeGens.clear();
}

void CPoissonDiskGen::RandomPoint(CRndGen& rand, Vec2& p)
{
    //generate random point inside circle
    do
    {
        p.x = rand.GenerateFloat() - 0.5f;
        p.y = rand.GenerateFloat() - 0.5f;
    } while (p.x * p.x + p.y * p.y > 0.25f);

    return;
}

//samples distance-based sorting
struct SamplesDistaceSort
{
    bool operator()(const Vec2& samplA, const Vec2& samplB) const
    {
        float R2sampleA = samplA.x * samplA.x + samplA.y * samplA.y;
        float R2sampleB = samplB.x * samplB.x + samplB.y * samplB.y;

        return
            (R2sampleA < R2sampleB);
    }
};

void CPoissonDiskGen::InitSamples()
{
    //Use a random generator with a fixed seed, so code changes(someone adding a new call to rnd() somehwhere) doesn't cause feature tests to fail.
    CRndGen rand;

    const int nQ = 1000;

    RandomPoint(rand, m_vSamples[0]);

    for (int i = 1; i < (int)m_vSamples.size(); i++)
    {
        float dmax = -1.0;

        for (int c = 0; c < i * nQ; c++)
        {
            Vec2 curSample;
            RandomPoint(rand, curSample);

            float dc = 2.0;
            for (int j = 0; j < i; j++)
            {
                float dj =
                    (m_vSamples[j].x - curSample.x) * (m_vSamples[j].x - curSample.x) +
                    (m_vSamples[j].y - curSample.y) * (m_vSamples[j].y - curSample.y);
                if (dc > dj)
                {
                    dc = dj;
                }
            }

            if (dc > dmax)
            {
                m_vSamples[i] = curSample;
                dmax = dc;
            }
        }
    }

    for (int i = 0; i < (int)m_vSamples.size(); i++)
    {
        m_vSamples[i] *= 2.0f;
    }

    //samples sorting
    std::stable_sort(m_vSamples.begin(), m_vSamples.end(), SamplesDistaceSort());
}

void CShadowUtils::GetIrregKernel(float sData[][4], int nSamplesNum)
{
    CPoissonDiskGen& pdg = CPoissonDiskGen::GetGenForKernelSize(nSamplesNum);

    for (int i = 0, nIdx = 0; i < nSamplesNum; i += 2, nIdx++)
    {
        Vec2 vSample = pdg.GetSample(i);
        sData[nIdx][0] = vSample.x;
        sData[nIdx][1] = vSample.y;
        vSample = pdg.GetSample(i + 1);
        sData[nIdx][2] = vSample.x;
        sData[nIdx][3] = vSample.y;
    }
}

ShadowMapFrustum* CShadowUtils::GetFrustum(ShadowFrustumID nFrustumID)
{
    int nLOD = 0;
    const int nLightID = GetShadowLightID(nLOD, nFrustumID);
    const int nThreadID = gRenDev->m_RP.m_nProcessThreadID;
    const int nCurRecLevel = SRendItem::m_RecurseLevel[nThreadID];
    assert(nCurRecLevel >= 0);

    const int nDLights = gRenDev->m_RP.m_DLights[nThreadID][nCurRecLevel].size();

    const int nFrustumIdx = nLightID;
    assert((unsigned int) nFrustumIdx < (MAX_REND_LIGHTS + MAX_DEFERRED_LIGHTS));
    const int nStartIdx = SRendItem::m_StartFrust[nThreadID][nFrustumIdx];
    const int nEndIdx = SRendItem::m_EndFrust[nThreadID][nFrustumIdx];

    const int nSize = gRenDev->m_RP.m_SMFrustums[nThreadID][nCurRecLevel].size();
    for (int nFrIdx = nStartIdx; nFrIdx < nEndIdx && nFrIdx < nSize; ++nFrIdx)
    {
        ShadowMapFrustum* frustum = &gRenDev->m_RP.m_SMFrustums[nThreadID][nCurRecLevel][nFrIdx];
        if (frustum->nShadowMapLod == nLOD)
        {
            return frustum;
        }
    }

    assert(0);
    return NULL;
}

ShadowMapFrustum& CShadowUtils::GetFirstFrustum(int nLightID)
{
    const int nThreadID = gRenDev->m_RP.m_nProcessThreadID;
    const int nCurRecLevel = SRendItem::m_RecurseLevel[nThreadID];
    assert(nCurRecLevel >= 0);
    const int nDLights = gRenDev->m_RP.m_DLights[nThreadID][nCurRecLevel].size();

    const int nFrustumIdx = nLightID + nDLights;
    assert((unsigned int) nFrustumIdx < (MAX_REND_LIGHTS + MAX_DEFERRED_LIGHTS));
    const int nStartIdx = SRendItem::m_StartFrust[nThreadID][nFrustumIdx];

    ShadowMapFrustum& firstFrustum = gRenDev->m_RP.m_SMFrustums[nThreadID][nCurRecLevel][nStartIdx];
    return firstFrustum;
}

const CShadowUtils::ShadowFrustumIDs* CShadowUtils::GetShadowFrustumList(uint64 nMask)
{
    // TODO: proper fix for per render object shadow light mask
    return NULL;

    if (!nMask)
    {
        return NULL;
    }

    if (!bShadowFrustumCacheValid)
    {
        // clear all allocated lists
        for (CRenderer::ShadowFrustumListsCache::iterator it = gRenDev->m_FrustumsCache.begin();
             it != gRenDev->m_FrustumsCache.end(); ++it)
        {
            (it->second)->Clear();
        }
        bShadowFrustumCacheValid = true;
    }

    CShadowUtils::ShadowFrustumIDs* pList = stl::find_in_map(gRenDev->m_FrustumsCache, nMask, NULL);
    if (!pList)
    {
        pList = new CShadowUtils::ShadowFrustumIDs;
    }
    if (pList->empty())
    {
        assert(nSunLightID <= 0xFF);
        // add sun index and lods encoded in higher bits
        for (int i = 0; i < 8; ++i)
        {
            if (nMask & (uint64(1) << i))
            {
                pList->Add((i << 8) | (nSunLightID & 0xFF));
            }
        }
        // other lights have 1 lod and index encoded in low bits
        for (int i = 8; i < 64 && (uint64(1) << i) <= nMask; ++i)
        {
            if (nMask & (uint64(1) << i))
            {
                pList->Add(i);
            }
        }
        gRenDev->m_FrustumsCache[nMask] = pList;
    }
    return pList;
}

void ShadowMapFrustum::GetMemoryUsage(ICrySizer* pSizer) const
{
    pSizer->AddObject(this, sizeof(*this));
    pSizer->AddObject(pFrustumOwner);
    pSizer->AddObject(pDepthTex);
}
