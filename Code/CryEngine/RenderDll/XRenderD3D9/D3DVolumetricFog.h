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

/* A short note on MultiGPU:
 Without knowing for sure if textures are copied between GPUs, the only 'correct' solution would be to have 2 * NUM_GPUs copies of rendertargets
 This would use too much memory though, so we keep one copy and swap it once every NUM_GPUs frames.This will work correctly if copy between GPUs
 is disabled. In case copies are performed though, every second frame the re-projection matrix will be off by a frame.
*/
#ifndef _D3DVOLUMETRICFOG_H_
#define _D3DVOLUMETRICFOG_H_


class CVolumetricFog
{
public:
    CVolumetricFog();

    void CreateResources();
    void DestroyResources(bool destroyResolutionIndependentResources);
    void Clear();
    void ClearAll();

    void PrepareLightList(TArray<SRenderLight>* envProbes, TArray<SRenderLight>* ambientLights, TArray<SRenderLight>* defLights, uint32 firstShadowLight, uint32 curShadowPoolLight);

    void RenderVolumetricsToVolume(void (* RenderFunc)());
    void RenderVolumetricFog();

    void ClearVolumeStencil();
    void RenderClipVolumeToVolumeStencil(int nClipAreaReservedStencilBit);

    float GetDepthIndex(float linearDepth) const;
    bool IsEnableInFrame() const;
    const Vec4& GetGlobalEnvProbeShaderParam0() const {return m_globalEnvProbeParam0; }
    const Vec4& GetGlobalEnvProbeShaderParam1() const {return m_globalEnvProbeParam1; }
    CTexture* GetGlobalEnvProbeTex0() const {return m_globalEnvProveTex0; }
    CTexture* GetGlobalEnvProbeTex1() const {return m_globalEnvProveTex1; }

    void PushFogVolume(class CREFogVolume* pFogVolume, const SRenderingPassInfo& passInfo);

private:
    static const uint32         MaxFrameNum = 4;
    static const uint32         MaxNumFogVolumeType = 2;

    struct SFogVolumeInfo
    {
        Vec3 m_center;
        uint32 m_viewerInsideVolume : 1;
        uint32 m_affectsThisAreaOnly : 1;
        uint32 m_stencilRef : 8;
        uint32 m_volumeType : 1;
        uint32 m_reserved : 21;
        AABB m_localAABB;
        Matrix34 m_matWSInv;
        float m_globalDensity;
        float m_densityOffset;
        Vec2 m_softEdgesLerp;
        ColorF m_fogColor;
        Vec3 m_heightFallOffDirScaled;
        Vec3 m_heightFallOffBasePoint;
        Vec3 m_eyePosInOS;
        Vec3 m_rampParams;
        Vec3 m_windOffset;
        float m_noiseScale;
        Vec3 m_noiseFreq;
        float m_noiseOffset;
        float m_noiseElapsedTime;
        Vec3 m_scale;
    };

    void ClearFogVolumes();
    void ClearAllFogVolumes();
    void UpdateFrame();
    bool IsViable() const;
    void InjectInscatteringLight();
    void BuildLightListGrid();
    void RaymarchVolumetricFog();
    void BlurInscatterVolume();
    void BlurDensityVolume();
    void ReprojectVolume();
    CTexture* GetInscatterTex() const;
    CTexture* GetPrevInscatterTex() const;
    CTexture* GetDensityTex() const;
    CTexture* GetPrevDensityTex() const;
    void RenderDownscaledDepth();
    void InjectExponentialHeightFog();
    void PrepareFogVolumeList();
    void InjectFogDensity();
    void RenderDownscaledShadowmap();
    void SetupStaticShadowMap(int nSlot) const;

private:
    Matrix44A                       m_viewProj[MaxFrameNum];
    CTexture*                       m_InscatteringVolume;
    CTexture*                       m_fogInscatteringVolume[2];
    CTexture*                       m_MaxDepth;
    CTexture*                       m_MaxDepthTemp;
    D3DDepthSurface**               m_ClipVolumeDSVArray;
    CTexture*                       m_fogDensityVolume[2];
    CTexture*                       m_downscaledShadow[3];
    CTexture*                       m_downscaledShadowTemp;
    CTexture*                       m_globalEnvProveTex0;
    CTexture*                       m_globalEnvProveTex1;

    int                                 m_nTexStateTriLinear;
    int                                 m_nTexStateCompare;
    int                                 m_nTexStatePoint;
    int32                               m_Cleared;
    int32                               m_Destroyed;
    uint32                          m_numTileLights;
    uint32                          m_numFogVolumes;
    int32                               m_frameID;
    int32                               m_tick;
    int32                               m_ReverseDepthMode;
    float                               m_raymarchDistance;
    Vec4                                m_globalEnvProbeParam0;
    Vec4                                m_globalEnvProbeParam1;

    WrappedDX11Buffer       m_lightCullInfoBuf;
    WrappedDX11Buffer       m_LightShadeInfoBuf;
    WrappedDX11Buffer       m_lightGridBuf;
    WrappedDX11Buffer       m_lightCountBuf;
    WrappedDX11Buffer       m_fogVolumeCullInfoBuf;
    WrappedDX11Buffer       m_fogVolumeInjectInfoBuf;

    TArray<SFogVolumeInfo>  m_fogVolumeInfoArray[RT_COMMAND_BUF_COUNT][MAX_REND_RECURSION_LEVELS][MaxNumFogVolumeType];
};

#endif
