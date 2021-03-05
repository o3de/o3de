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

#ifndef CRYINCLUDE_CRY3DENGINE_FOGVOLUMERENDERNODE_H
#define CRYINCLUDE_CRY3DENGINE_FOGVOLUMERENDERNODE_H
#pragma once


class CREFogVolume;


class CFogVolumeRenderNode
    : public IFogVolumeRenderNode
    , public Cry3DEngineBase
{
public:
    static void StaticReset();

    static void SetTraceableArea(const AABB& traceableArea, const SRenderingPassInfo& passInfo);
    static void TraceFogVolumes(const Vec3& vPos, const AABB& objBBox, SFogVolumeData& fogVolData, const SRenderingPassInfo& passInfo, bool fogVolumeShadingQuality);
    static bool OverlapProjectedAABB(const AABB& objBBox0, const AABB& objBBox1, const CCamera& camera);
public:
    CFogVolumeRenderNode();

    // implements IFogVolumeRenderNode
    virtual void SetFogVolumeProperties(const SFogVolumeProperties& properties);
    virtual const Matrix34& GetMatrix() const;

    virtual void FadeGlobalDensity(float fadeTime, float newGlobalDensity);

    // implements IRenderNode
    virtual void GetLocalBounds(AABB& bbox);
    virtual void SetMatrix(const Matrix34& mat);
    virtual void SetScale(const Vec3& scale);

    virtual EERType GetRenderNodeType();
    virtual const char* GetEntityClassName() const;
    virtual const char* GetName() const;
    virtual Vec3 GetPos(bool bWorldOnly = true) const;
    virtual void Render(const SRendParams& rParam, const SRenderingPassInfo& passInfo);
    void SetMaterial(_smart_ptr<IMaterial> pMat) override;
    virtual _smart_ptr<IMaterial> GetMaterial(Vec3* pHitPos);
    virtual _smart_ptr<IMaterial> GetMaterialOverride() { return NULL; }
    virtual float GetMaxViewDist();
    virtual void GetMemoryUsage(ICrySizer* pSizer) const;
    virtual const AABB GetBBox() const { return m_WSBBox; }
    virtual void SetBBox(const AABB& WSBBox) { m_WSBBox = WSBBox; }
    virtual void FillBBox(AABB& aabb);
    virtual void OffsetPosition(const Vec3& delta);
    float GetGlobalDensity() const  { return  m_globalDensity; }
    float GetDensityoffset() const { return  m_densityOffset; }
    Vec3 GetHeightFallOffBasePoint() const { return  m_heightFallOffBasePoint; }
    Vec3 GetHeightFallOffDirScaled() const { return  m_heightFallOffDirScaled; }


    ILINE bool IsAffectsThisAreaOnly() const { return m_affectsThisAreaOnly; }
    int GetVolumeType() const  { return m_volumeType; }
private:
    static void RegisterFogVolume(const CFogVolumeRenderNode* pFogVolume);
    static void UnregisterFogVolume(const CFogVolumeRenderNode* pFogVolume);

private:
    ~CFogVolumeRenderNode();

    void UpdateFogVolumeMatrices();
    void UpdateWorldSpaceBBox();
    void UpdateHeightFallOffBasePoint();
    ColorF GetFogColor() const;
    Vec2 GetSoftEdgeLerp(const Vec3& viewerPosOS) const;
    bool IsViewerInsideVolume(const SRenderingPassInfo& passInfo) const;

    void GetVolumetricFogColorEllipsoid(const Vec3& worldPos, const SRenderingPassInfo& passInfo, ColorF& resultColor) const;
    void GetVolumetricFogColorBox(const Vec3& worldPos, const SRenderingPassInfo& passInfo, ColorF& resultColor) const;

    static void ForceTraceableAreaUpdate();

private:
    struct SCachedFogVolume
    {
        SCachedFogVolume()
            : m_pFogVol(0)
            , m_distToCenterSq(0) {}

        SCachedFogVolume(const CFogVolumeRenderNode* pFogVol, float distToCenterSq)
            : m_pFogVol(pFogVol)
            , m_distToCenterSq(distToCenterSq)
        {
        }

        bool operator < (const SCachedFogVolume& rhs) const
        {
            return m_distToCenterSq > rhs.m_distToCenterSq;
        }

        const CFogVolumeRenderNode* m_pFogVol;
        float m_distToCenterSq;
    };

    typedef std::vector< SCachedFogVolume > CachedFogVolumes;
    typedef std::set< const CFogVolumeRenderNode* > GlobalFogVolumeMap;

    static AABB s_tracableFogVolumeArea;
    static StaticInstance<CachedFogVolumes> s_cachedFogVolumes;
    static StaticInstance<GlobalFogVolumeMap> s_globalFogVolumeMap;
    static bool s_forceTraceableAreaUpdate;

    struct SFader
    {
        SFader()
            : m_startTime(0)
            , m_endTime(0)
            , m_startValue(0)
            , m_endValue(0)
        {
        }

        void Set(float startTime, float endTime, float startValue, float endValue)
        {
            m_startTime = startTime;
            m_endTime = endTime;
            m_startValue = startValue;
            m_endValue = endValue;
        }

        void SetInvalid()
        {
            Set(0, 0, 0, 0);
        }

        bool IsValid()
        {
            return m_startTime >= 0 && m_endTime > m_startTime && m_startValue != m_endValue;
        }

        bool IsTimeInRange(float time)
        {
            return time >= m_startTime && time <= m_endTime;
        }

        float GetValue(float time)
        {
            float t = clamp_tpl((time - m_startTime) / (m_endTime - m_startTime), 0.0f, 1.0f);
            return m_startValue + t * (m_endValue - m_startValue);
        }

    private:
        float m_startTime;
        float m_endTime;
        float m_startValue;
        float m_endValue;
    };

private:
    Matrix34 m_matNodeWS;

    Matrix34 m_matWS;
    Matrix34 m_matWSInv;

    int m_volumeType;
    Vec3 m_pos;
    Vec3 m_x;
    Vec3 m_y;
    Vec3 m_z;
    // size of fog set by SFogVolumeProperties
    Vec3 m_size;
    // scale on entity
    Vec3 m_scale;

    float m_globalDensity;
    float m_densityOffset;
    float m_nearCutoff;
    float m_fHDRDynamic;
    float m_softEdges;
    ColorF m_color;
    bool m_useGlobalFogColor;
    bool m_affectsThisAreaOnly;
    Vec3 m_rampParams;
    uint32 m_updateFrameID;
    float m_windInfluence;
    Vec3 m_windOffset;
    float m_noiseElapsedTime;
    float m_densityNoiseScale;
    float m_densityNoiseOffset;
    float m_densityNoiseTimeFrequency;
    Vec3 m_densityNoiseFrequency;

    Vec3 m_heightFallOffDir;
    Vec3 m_heightFallOffDirScaled;
    Vec3 m_heightFallOffShift;
    Vec3 m_heightFallOffBasePoint;

    AABB m_localBounds;

    SFader m_globalDensityFader;

    _smart_ptr< IMaterial > m_pMatFogVolEllipsoid;
    _smart_ptr< IMaterial > m_pMatFogVolBox;

    CREFogVolume* m_pFogVolumeRenderElement[RT_COMMAND_BUF_COUNT];
    AABB m_WSBBox;

    Vec2 m_cachedSoftEdgesLerp;
    ColorF m_cachedFogColor;
};


#endif // CRYINCLUDE_CRY3DENGINE_FOGVOLUMERENDERNODE_H
