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

#ifndef CRYINCLUDE_CRY3DENGINE_LIGHTENTITY_H
#define CRYINCLUDE_CRY3DENGINE_LIGHTENTITY_H
#pragma once

const float LIGHT_PROJECTOR_MAX_FOV = 180.f;

struct CLightEntity
    : public ILightSource
    , public Cry3DEngineBase
{
    static void StaticReset();

public:
    virtual EERType GetRenderNodeType();
    virtual const char* GetEntityClassName(void) const { return "LightEntityClass"; }
    virtual const char* GetName(void) const;
    virtual Vec3 GetPos(bool) const;
    virtual void Render(const SRendParams&, const SRenderingPassInfo& passInfo);
    virtual void SetMaterial(_smart_ptr<IMaterial> pMat) { m_pMaterial = pMat; }
    virtual _smart_ptr<IMaterial> GetMaterial([[maybe_unused]] Vec3* pHitPos = NULL) { return m_pMaterial; }
    virtual _smart_ptr<IMaterial> GetMaterialOverride() { return m_pMaterial; }
    virtual float GetMaxViewDist();
    virtual void SetLightProperties(const CDLight& light);
    virtual CDLight& GetLightProperties() { return m_light; };
    virtual void Release(bool);
    virtual void SetMatrix(const Matrix34& mat);
    virtual const Matrix34& GetMatrix() { return m_Matrix; }
    virtual struct ShadowMapFrustum* GetShadowFrustum(int nId = 0);
    virtual void GetMemoryUsage(ICrySizer* pSizer) const;
    virtual const AABB GetBBox() const { return m_WSBBox; }
    virtual void SetBBox(const AABB& WSBBox) { m_WSBBox = WSBBox; }
    virtual void FillBBox(AABB& aabb);
    virtual void OffsetPosition(const Vec3& delta);
    virtual void SetCastingException(IRenderNode* pNotCaster) { m_pNotCaster = pNotCaster; }
    virtual bool IsLightAreasVisible();

    virtual struct IStatObj* GetEntityStatObj([[maybe_unused]] unsigned int nPartId = 0, [[maybe_unused]] unsigned int nSubPartId = 0, [[maybe_unused]] Matrix34A* pMatrix = NULL, [[maybe_unused]] bool bReturnOnlyVisible = false) { return NULL; }
    virtual int GetSlotCount() const;
    virtual EVoxelGIMode GetVoxelGIMode() override;
    virtual void SetDesiredVoxelGIMode(EVoxelGIMode mode) override;
    virtual void SetName(const char* name);
    void InitEntityShadowMapInfoStructure();
    void UpdateGSMLightSourceShadowFrustum(const SRenderingPassInfo& passInfo);
    int UpdateGSMLightSourceDynamicShadowFrustum(int nDynamicLodCount, int nDistanceLodCount, float& fDistanceFromViewLastDynamicLod, float& fGSMBoxSizeLastDynamicLod, bool bFadeLastCascade, const SRenderingPassInfo& passInfo);
    int UpdateGSMLightSourceCachedShadowFrustum(int nFirstLod, int nLodCount, float& fDistFromViewDynamicLod, float fRadiusDynamicLod, const SRenderingPassInfo& passInfo);
    bool ProcessFrustum(int nLod, float fCamBoxSize, float fDistanceFromView, PodArray<struct SPlaneObject>& lstCastersHull, const SRenderingPassInfo& passInfo);
    static void ProcessPerObjectFrustum(ShadowMapFrustum* pFr, struct SPerObjectShadow* pPerObjectShadow, ILightSource* pLightSource, const SRenderingPassInfo& passInfo);
    void InitShadowFrustum_SUN_Conserv(ShadowMapFrustum* pFr, int dwAllowedTypes, float fGSMBoxSize, float fDistance, int nLod, const SRenderingPassInfo& passInfo);
    void InitShadowFrustum_PROJECTOR(ShadowMapFrustum* pFr, int dwAllowedTypes, const SRenderingPassInfo& passInfo);
    void InitShadowFrustum_OMNI(ShadowMapFrustum* pFr, int dwAllowedTypes, const SRenderingPassInfo& passInfo);
    void FillFrustumCastersList_SUN(ShadowMapFrustum* pFr, int dwAllowedTypes, int nRenderNodeFlags, PodArray<struct SPlaneObject>& lstCastersHull, int nLod, const SRenderingPassInfo& passInfo);
    void FillFrustumCastersList_PROJECTOR(ShadowMapFrustum* pFr, int dwAllowedTypes, const SRenderingPassInfo& passInfo);
    void FillFrustumCastersList_OMNI(ShadowMapFrustum* pFr, int dwAllowedTypes, const SRenderingPassInfo& passInfo);
    void CheckValidFrustums_OMNI(ShadowMapFrustum* pFr, const SRenderingPassInfo& passInfo);
    bool CheckFrustumsIntersect(CLightEntity* lightEnt);
    bool GetGsmFrustumBounds(const CCamera& viewFrustum, ShadowMapFrustum* pShadowFrustum);
    void DetectCastersListChanges(ShadowMapFrustum* pFr, const SRenderingPassInfo& passInfo);
    void OnCasterDeleted(IShadowCaster* pCaster);
    int MakeShadowCastersHullSun(PodArray<SPlaneObject>& lstCastersHull, const SRenderingPassInfo& passInfo);
    static Vec3 GSM_GetNextScreenEdge(float fPrevRadius, float fPrevDistanceFromView, const SRenderingPassInfo& passInfo);
    static float GSM_GetLODProjectionCenter(const Vec3& vEdgeScreen, float fRadius);
    static bool FrustumIntersection(const CCamera& viewFrustum, const CCamera& shadowFrustum);
    void UpdateCastShadowFlag(float fDistance, const SRenderingPassInfo& passInfo);
    void CalculateShadowBias(ShadowMapFrustum* pFr, int nLod, float fGSMBoxSize) const;

    CLightEntity();
    ~CLightEntity();

    CDLight m_light;
    bool m_bShadowCaster : 1;

    _smart_ptr<IMaterial> m_pMaterial;
    Matrix34 m_Matrix;
    IRenderNode* m_pNotCaster;

    // used for shadow maps
    struct ShadowMapInfo
    {
        ShadowMapInfo() { memset(this, 0, sizeof(ShadowMapInfo)); }
        void Release(struct IRenderer* pRenderer);
        void GetMemoryUsage(ICrySizer* pSizer) const
        {
            pSizer->AddObject(this, sizeof(*this));
        }
        struct ShadowMapFrustum* pGSM[MAX_GSM_LODS_NUM];
    }* m_pShadowMapInfo;

    AABB m_WSBBox;

    void SetLayerId(uint16 nLayerId) { m_layerId = nLayerId; }
    uint16 GetLayerId() { return m_layerId; }

private:
    static PodArray<SPlaneObject> s_lstTmpCastersHull;

private:
    IStatObj* m_pStatObj;

    uint16 m_layerId;
    EVoxelGIMode m_VoxelGIMode;
    AZStd::string m_Name;
};
#endif
