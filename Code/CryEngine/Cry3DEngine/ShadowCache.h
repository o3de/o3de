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

#ifndef STATIC_SHADOWS_H
#define STATIC_SHADOWS_H

#include <platform.h>
#include "../RenderDll/Common/Shadow_Renderer.h"

class ShadowCache
    : public Cry3DEngineBase
{
public:
    ShadowCache(CLightEntity* pLightEntity, ShadowMapFrustum::ShadowCacheData::eUpdateStrategy nUpdateStrategy)
        : m_pLightEntity(pLightEntity)
        , m_nUpdateStrategy(nUpdateStrategy)
    {}

    void InitShadowFrustum(ShadowMapFrustum*& pFr, int nLod, int nFirstStaticLod, float fDistFromViewDynamicLod, float fRadiusDynamicLod, const SRenderingPassInfo& passInfo);
    void InitHeightMapAOFrustum(ShadowMapFrustum*& pFr, int nLod, const SRenderingPassInfo& passInfo);

private:
    static const int MAX_RENDERNODES_PER_FRAME = 50;
    static const float AO_FRUSTUM_SLOPE_BIAS;
    static const uint64 kHashMul = 0x9ddfea08eb382d69ULL;


    void InitCachedFrustum(ShadowMapFrustum*& pFr, ShadowMapFrustum::ShadowCacheData::eUpdateStrategy nUpdateStrategy, int nLod, int nTexSize, const Vec3& vLightPos, const AABB& projectionBoundsLS, const SRenderingPassInfo& passInfo);

    void GetCasterBox(AABB& BBoxWS, AABB& BBoxLS, float fRadius, const Matrix34& matView, const SRenderingPassInfo& passInfo);
    Matrix44 GetViewMatrix(const SRenderingPassInfo& passInfo);

    ILINE uint64 HashValue(uint64 value);

    CLightEntity* m_pLightEntity;
    ShadowMapFrustum::ShadowCacheData::eUpdateStrategy m_nUpdateStrategy;
};
#endif
