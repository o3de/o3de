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

#ifndef __SHADOWUTILS_H__
#define __SHADOWUTILS_H__

#define DEG2RAD_R(a) (f64(a) * (g_PI / 180.0))
#define RAD2DEG_R(a) float((f64)(a) * (180.0 / g_PI))

static const float g_fOmniShadowFov = 95.0f;
static const float g_fOmniLightFov = 90.5f;

class CRndGen;

class CPoissonDiskGen
{
    std::vector<Vec2> m_vSamples;

private:
    static void RandomPoint(CRndGen& rand, Vec2& p);
    void InitSamples();

public:
    Vec2& GetSample(int ind);

    static CPoissonDiskGen& GetGenForKernelSize(int num);
    static void FreeMemory();
};

enum EFrustum_Type
{
    FTYP_SHADOWOMNIPROJECTION,
    FTYP_SHADOWPROJECTION,
    FTYP_OMNILIGHTVOLUME,
    FTYP_LIGHTVOLUME,
    FTYP_MAX,
    FTYP_UNKNOWN
};

struct SRenderTileInfo;

class CShadowUtils
{
public:
    typedef uint16 ShadowFrustumID;
    typedef PodArray<ShadowFrustumID> ShadowFrustumIDs;

public:
    static void CalcDifferentials(const CCamera& cam, float fViewWidth, float fViewHeight, float& fFragSizeX);
    static void ProjectScreenToWorldExpansionBasis(const Matrix44r& mShadowTexGen, const CCamera& cam, const Vec2& vJitter, float fViewWidth, float fViewHeight, Vec4r& vWBasisX, Vec4r& vWBasisY, Vec4r& vWBasisZ, Vec4r& vCamPos, bool bWPos, SRenderTileInfo* pSTileInfo);
    static void CalcScreenToWorldExpansionBasis(const CCamera& cam, const Vec2& vJitter, float fViewWidth, float fViewHeight, Vec3& vWBasisX, Vec3& vWBasisY, Vec3& vWBasisZ, bool bWPos);

    static void CalcLightBoundRect(const SRenderLight* pLight, const CameraViewParameters& RCam, Matrix44A& mView, Matrix44A& mProj, Vec2* pvMin, Vec2* pvMax, IRenderAuxGeom* pAuxRend);
    static void GetProjectiveTexGen(const SRenderLight* pLight, int nFace, Matrix44A* mTexGen);
    static void GetCubemapFrustumForLight(const SRenderLight* pLight, int nS, float fFov, Matrix44A* pmProj, Matrix44A* pmView, bool bProjLight);

    static void GetShadowMatrixForObject(Matrix44A& mLightProj, Matrix44A& mLightView, ShadowMapFrustum* lof);
    static AABB GetShadowMatrixForCasterBox(Matrix44A& mLightProj, Matrix44A& mLightView, ShadowMapFrustum* lof, float fFarPlaneOffset = 0);
    static void GetCubemapFrustum(EFrustum_Type eFrustumType, ShadowMapFrustum* pFrust, int nS, Matrix44A* pmProj, Matrix44A* pmView, Matrix33* pmLightRot = NULL);

    static Matrix34 GetAreaLightMatrix(const SRenderLight* pLight, Vec3 vScale);

    static void mathMatrixLookAtSnap(Matrix44A* pMatr, const Vec3& Eye, const Vec3& At, ShadowMapFrustum* pFrust);
    static void GetShadowMatrixOrtho(Matrix44A& mLightProj, Matrix44A& mLightView, const Matrix44A& mViewMatrix, ShadowMapFrustum* lof, bool bViewDependent);

    static void GetIrregKernel(float sData[][4], int nSamplesNum);

    static ShadowMapFrustum* GetFrustum(ShadowFrustumID nFrustumID);
    static ShadowMapFrustum& GetFirstFrustum(int nLightID);

    // Get list of encoded frustum ids for given mask
    static const ShadowFrustumIDs* GetShadowFrustumList(uint64 nMask);
    // Get light id and LOD id from encoded id in shadow frustum cache
    static int32 GetShadowLightID(int32& nLod, ShadowFrustumID nFrustumID)
    {
        nLod = int32((nFrustumID >> 8) & 0xFF);
        return int32(nFrustumID & 0xFF);
    }
    static void InvalidateShadowFrustumCache()
    {
        bShadowFrustumCacheValid = false;
    }

    CShadowUtils();
    ~CShadowUtils();

private:
    static bool bShadowFrustumCacheValid;
    // Currently forced to use always ID 0 for sun (if sun present)
    static const int nSunLightID = 0;
};

#endif
