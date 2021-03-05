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

#ifndef CRYINCLUDE_CRY3DENGINE_TERRAIN_WATER_H
#define CRYINCLUDE_CRY3DENGINE_TERRAIN_WATER_H
#pragma once

#include <AzCore/Math/Vector2.h>

#define CYCLE_BUFFERS_NUM    4

class COcean: public IRenderNode
    , public Cry3DEngineBase
{
public:
    COcean(_smart_ptr<IMaterial> pMat, float fWaterLevel);
    ~COcean();


    void SetWaterLevel(float fWaterLevel);
    static void SetWaterLevelInfo(float fWaterLevelInfo) { m_fWaterLevelInfo = fWaterLevelInfo; }
    static float GetWaterLevelInfo() { return m_fWaterLevelInfo; }

    void Create();
    void Update(const SRenderingPassInfo& passInfo);
    void Render(const SRenderingPassInfo& passInfo);

    bool IsVisible(const SRenderingPassInfo& passInfo);

    void SetLastFov(float fLastFov) {m_fLastFov = fLastFov; }
    static void SetTimer(ITimer* pTimer);

    float GetWaterLevel() { return m_fWaterLevel; }
    static float GetWave(const Vec3& pPosition, int32 nFrameID);
    static uint32 GetVisiblePixelsCount();
    int32 GetMemoryUsage();

    // fake IRenderNode implementation
    virtual EERType GetRenderNodeType();
    virtual const char* GetEntityClassName(void) const { return "Ocean"; }
    virtual const char* GetName(void) const { return "Ocean"; }
    virtual Vec3 GetPos(bool) const;
    virtual void Render(const SRendParams&, [[maybe_unused]] const SRenderingPassInfo& passInfo) {}
    virtual void SetMaterial(_smart_ptr<IMaterial> pMat) override;
    virtual _smart_ptr<IMaterial> GetMaterial(Vec3* pHitPos = NULL);
    virtual _smart_ptr<IMaterial> GetMaterialOverride() { return m_pMaterial; }
    virtual float GetMaxViewDist() { return 1000000.f; }
    virtual void GetMemoryUsage([[maybe_unused]] ICrySizer* pSizer) const{}
    virtual const AABB GetBBox() const { return AABB(Vec3(-1000000.f, -1000000.f, -1000000.f), Vec3(1000000.f, 1000000.f, 1000000.f)); }
    virtual void SetBBox([[maybe_unused]] const AABB& WSBBox) { }
    virtual void FillBBox(AABB& aabb);

    virtual void OffsetPosition([[maybe_unused]] const Vec3& delta) {}

private:

    void RenderFog(const SRenderingPassInfo& passInfo);
    void RenderBottomCap(const SRenderingPassInfo& passInfo);
    void GetOceanGridSize(int& outX, int& outY) const;

private:
    static bool IsWaterVisibleOcclusionCheck();
    
    // Ocean data
    _smart_ptr<IMaterial> m_pMaterial;
    _smart_ptr<IRenderMesh> m_pRenderMesh;

    PodArray<SVF_P3F_C4B_T2F> m_pMeshVerts;
    PodArray<vtx_idx> m_pMeshIndices;

    int32 m_nPrevGridDim;
    int32 m_nVertsCount;
    int32 m_nIndicesCount;

    int32 m_nTessellationType;
    int32 m_nTessellationTiles;

    // Ocean bottom cap data
    _smart_ptr< IMaterial > m_pBottomCapMaterial;
    _smart_ptr<IRenderMesh> m_pBottomCapRenderMesh;

    PodArray<SVF_P3F_C4B_T2F> m_pBottomCapVerts;
    PodArray<vtx_idx> m_pBottomCapIndices;

    // Visibility data
    CCamera m_Camera;
    class CREOcclusionQuery* m_pREOcclusionQueries[CYCLE_BUFFERS_NUM];
    IShader* m_pShaderOcclusionQuery;
    float m_fLastFov;
    float m_fLastVisibleFrameTime;
    int32 m_nLastVisibleFrameId;
    static uint32 m_nVisiblePixelsCount;
    float m_fWaterLevel;

    float m_fRECustomData[12]; // used for passing data to renderer
    float m_fREOceanBottomCustomData[8]; // used for passing data to renderer
    AZ::Vector2 m_windUvTransform; // used for texture offset due to wind
    bool m_bOceanFFT;

    // Ocean fog related members
    CREWaterVolume::SParams m_wvParams[RT_COMMAND_BUF_COUNT];
    CREWaterVolume::SOceanParams m_wvoParams[RT_COMMAND_BUF_COUNT];

    _smart_ptr< IMaterial > m_pFogIntoMat;
    _smart_ptr< IMaterial > m_pFogOutofMat;
    _smart_ptr< IMaterial > m_pFogIntoMatLowSpec;
    _smart_ptr< IMaterial > m_pFogOutofMatLowSpec;

    CREWaterVolume* m_pWVRE[RT_COMMAND_BUF_COUNT];
    std::vector<SVF_P3F_C4B_T2F> m_wvVertices[RT_COMMAND_BUF_COUNT];
    std::vector<uint16> m_wvIndices[RT_COMMAND_BUF_COUNT];

    int32 m_swathWidth;
    bool m_bUsingFFT;
    bool m_bUseTessHW;

    static ITimer* m_pTimer;
    static CREWaterOcean* m_pOceanRE;
    static float m_fWaterLevelInfo;
};

#endif // CRYINCLUDE_CRY3DENGINE_TERRAIN_WATER_H
