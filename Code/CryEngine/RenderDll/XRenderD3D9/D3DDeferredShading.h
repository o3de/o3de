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

#ifndef CRYINCLUDE_CRYENGINE_RENDERDLL_XRENDERD3D9_D3DDEFERREDSHADING_H
#define CRYINCLUDE_CRYENGINE_RENDERDLL_XRENDERD3D9_D3DDEFERREDSHADING_H
#pragma once
struct IVisArea;

enum EDecalType
{
    DTYP_DARKEN,
    DTYP_BRIGHTEN,
    DTYP_ALPHABLEND,
    DTYP_ALPHABLEND_AND_BUMP,
    DTYP_ALPHABLEND_SPECULAR,
    DTYP_DARKEN_LIGHTBUF,
    DTYP_NUM,
};

#define MAX_DEFERRED_CLIP_VOLUMES 64
// Note: 2 stencil values reserved for stencil+outdoor fragments
#define VIS_AREAS_OUTDOOR_STENCIL_OFFSET 2

class CTexPoolAtlas;

struct SShadowAllocData
{
    int         m_lightID;
    uint16  m_blockID;
    uint8       m_side;
    uint8       m_frameID;

    void Clear(){ m_blockID = 0xFFFF; m_lightID = -1; m_frameID = 0; }
    bool isFree() { return (m_blockID == 0xFFFF) ? true : false; }

    SShadowAllocData(){ Clear(); }
    ~SShadowAllocData(){ Clear(); }
};

class CDeferredShading
{
public:

    static inline bool IsValid()
    {
        return m_pInstance != NULL;
    }

    static CDeferredShading& Instance()
    {
        return (*m_pInstance);
    }

    void Render();
    void SetupPasses();
    void SetupGlobalConsts();


    // This will setup shadows and sort lights.
    // It is called before Z-Pass and is used so that we don't have
    // to resolve any buffers because of shadows setup during deferred passes.
    void SetupGmemPath();


    //shadows
    void ResolveCurrentBuffers();
    void RestoreCurrentBuffers();
    bool PackAllShadowFrustums(TArray<SRenderLight>& arrLights, bool bPreLoop);
    void DebugShadowMaskClear();
    bool PackToPool(CPowerOf2BlockPacker* pBlockPack, SRenderLight& light, bool bClearPool);

    void FilterGBuffer();
    void AmbientOcclusionPasses();
    void PrepareClipVolumeData(bool& bOutdoorVisible);
    void RenderClipVolumesToStencil(int nStencilInsideBit);
    void RenderPortalBlendValues(int nStencilInsideBit);
    bool AmbientPass(SRenderLight* pGlobalCubemap, bool& bOutdoorVisible);

    bool DeferredDecalPass(const SDeferredDecal& rDecal, uint32 indDecal);
    void DeferredDecalEmissivePass(const SDeferredDecal& rDecal, uint32 indDecal);
    bool ShadowLightPasses(const SRenderLight& light);
    void DrawDecalVolume(const SDeferredDecal& rDecal, Matrix44A& mDecalLightProj, ECull volumeCull);
    void DrawLightVolume(EShapeMeshType meshType, const Matrix44& mVolumeToWorld, const Vec4& vSphereAdjust = Vec4(ZERO));
    void LightPass(const SRenderLight* const __restrict pDL, bool bForceStencilDisable = false);
    void DeferredCubemaps(const TArray<SRenderLight>& rCubemaps, const uint32 nStartIndex = 0);
    void DeferredCubemapPass(const SRenderLight* const __restrict pDL);
    void ScreenSpaceReflectionPass();
    void ApplySSReflections();
    void DirectionalOcclusionPass();
    void HeightMapOcclusionPass(ShadowMapFrustum*& pHeightMapFrustum, CTexture*& pHeightMapAOScreenDepth, CTexture*& pHeightmapAO);
    void DeferredLights(TArray<SRenderLight>& rLights, bool bCastShadows);

    void DeferredSubsurfaceScattering(CTexture* tmpTex);
    void DeferredShadingPass();

    void CreateDeferredMaps();
    void DestroyDeferredMaps();
    void Release();
    void Debug();
    void DebugGBuffer();

    //! Adds a light to the list of lights to be rendered.
    //! \param pDL               Light to be rendered
    //! \param fMult             Multiplier that will be applied to the light's intensity. For example, use this to fade out lights as they exceed distance thresholds.
    //! \param passInfo          Standard SRenderingPassInfo, send to EF_AddEf()
    //! \param rendItemSorter    Standard SRendItemSorter, send to EF_AddEf()
    uint32 AddLight(const CDLight& pDL, float fMult, const SRenderingPassInfo& passInfo, const SRendItemSorter& rendItemSorter);

    inline uint8 AddClipVolume(const IClipVolume* pClipVolume);
    inline bool SetClipVolumeBlendData(const IClipVolume* pClipVolume, const SClipVolumeBlendInfo& blendInfo);

    inline void ResetLights();
    inline void ResetClipVolumes();

    // Renderer must be flushed
    void ResetAllLights();
    void ResetAllClipVolumes();

    // called in between levels to free up memory
    void ReleaseData();

    TArray<SRenderLight>& GetLights(const int nThreadID, const int nCurRecLevel, const eDeferredLightType eType = eDLT_DeferredLight);
    SRenderLight* GetLightByID(const uint16 nLightID, const eDeferredLightType eType = eDLT_DeferredLight);
    uint32 GetLightsNum(const eDeferredLightType eType);
    void GetClipVolumeParams(const Vec4*& pParams, uint32& nCount);
    CTexture* GetResolvedStencilRT() { return m_pResolvedStencilRT; }
    void GetLightRenderSettings(const SRenderLight* const __restrict pDL, bool& bStencilMask, bool& bUseLightVolumes, EShapeMeshType& meshType);

    inline uint32 GetLightsCount() const
    {
        return m_nLightsProcessedCount;
    }

    inline Vec4 GetLightDepthBounds(const SRenderLight* pDL, bool bReverseDepth) const
    {
        float fRadius = pDL->m_fRadius;
        if (pDL->m_Flags & DLF_AREA_LIGHT) // Use max for area lights.
        {
            fRadius += max(pDL->m_fAreaWidth, pDL->m_fAreaHeight);
        }
        else if (pDL->m_Flags & DLF_DEFERRED_CUBEMAPS)
        {
            fRadius = pDL->m_ProbeExtents.len(); // This is not optimal for a box
        }
        return GetLightDepthBounds(pDL->m_Origin, fRadius, bReverseDepth);
    }

    inline Vec4 GetLightDepthBounds(const Vec3& vCenter, float fRadius, bool bReverseDepth) const
    {
        if IsCVarConstAccess(constexpr) (!CRenderer::CV_r_DeferredShadingDepthBoundsTest)
        {
            return Vec4(0.0f, 0.0f, 1.0f, 1.0f);
        }

        float fMinZ = 0.0f, fMaxZ = 1.0f;
        float fMinW = 0.0f, fMaxW = 1.0f;

        Vec3 pBounds = m_pCamFront * fRadius;
        Vec3 pMax = vCenter - pBounds;
        Vec3 pMin = vCenter + pBounds;

        fMinZ = m_mViewProj.m20 * pMin.x + m_mViewProj.m21 * pMin.y + m_mViewProj.m22 * pMin.z + m_mViewProj.m23;
        fMinW = m_mViewProj.m30 * pMin.x + m_mViewProj.m31 * pMin.y + m_mViewProj.m32 * pMin.z + m_mViewProj.m33;

        float fMinDivisor = (float)fsel(-fabsf(fMinW), 1.0f, fMinW);
        fMinZ = (float)fsel(-fabsf(fMinW), 1.0f, fMinZ / fMinDivisor);
        fMinZ = (float)fsel(fMinW, fMinZ, bReverseDepth ? 1.0f : 0.0f);
        

        fMaxZ = m_mViewProj.m20 * pMax.x + m_mViewProj.m21 * pMax.y + m_mViewProj.m22 * pMax.z + m_mViewProj.m23;
        fMaxW = m_mViewProj.m30 * pMax.x + m_mViewProj.m31 * pMax.y + m_mViewProj.m32 * pMax.z + m_mViewProj.m33;
        float fMaxDivisor = (float)fsel(-fabsf(fMaxW), 1.0f, fMaxW);
        fMaxZ = (float)fsel(-fabsf(fMaxW), 1.0f, fMaxZ / fMaxDivisor);
        fMaxZ = (float)fsel(fMaxW, fMaxZ, bReverseDepth ? 1.0f : 0.0f);

        if (bReverseDepth)
        {
            std::swap(fMinZ, fMaxZ);           
        }

        fMinZ = clamp_tpl(fMinZ, 0.000001f, 1.f);
        fMaxZ = clamp_tpl(fMaxZ, fMinZ, 1.f);

        return Vec4(fMinZ, max(fMinW, 0.000001f), fMaxZ, max(fMaxW, 0.000001f));
    }

    void GetScissors(const Vec3& vCenter, float fRadius, short& sX, short& sY, short& sWidth, short& sHeight) const;
    void SetupScissors(bool bEnable, uint16 x, uint16 y, uint16 w, uint16 h) const;

    // Calculate the individual screen-space scissor bounds for all of our bound lights
    void CalculateLightScissorBounds();

    const Matrix44A& GetCameraProjMatrix() const { return m_mViewProj; }

    void SortLigths(TArray<SRenderLight>& ligths) const;

private:
    void SetSSDOParameters(const int texSlot);
    ITexture* SetTexture(const SShaderItem& sItem, EEfResTextures tex, int slot, const RectF texRect, float surfaceSize, float& mipLevelFactor, int flags);

    // Flags used for the above SetTexture function
    enum ESetTextureFlags : uint32
    {
        ESetTexture_Transform           = 1 << 0,   // Will calculate 2 Vec3s used for transforming tex coords in the shader
        ESetTexture_HWSR                = 1 << 1,   // Will set the HWSR_SAMPLE flag for the specified slot
        ESetTexture_bSRGBLookup         = 1 << 2,   // Value to set on the STexState
        ESetTexture_AllowDefault        = 1 << 3,   // Whether a default texture should be used as backup
        ESetTexture_MipFactorProvided   = 1 << 4,   // Whether to use the mipLevelFactor provide or calculate our own and output it to the same parameter
    };

    enum
    {
        // Number of textures available in PostEffectsLib.cfi (_tex0 to _texF)
        EMaxTextureSlots = 16,
    };

    CDeferredShading()
    {
        m_pShader = 0;
        m_pTechName = "DeferredLightPass";
        m_pAmbientOutdoorTechName = "AmbientPass";
        m_pCubemapsTechName = "DeferredCubemapPass";
        m_pCubemapsVolumeTechName = "DeferredCubemapVolumePass";
        m_pReflectionTechName = "SSR_Raytrace";
        m_pDebugTechName = "Debug";
        m_pDeferredDecalTechName = "DeferredDecal";
        m_pLightVolumeTechName = "DeferredLightVolume";

        m_pParamLightPos = "g_LightPos";
        m_pParamLightProjMatrix = "g_mLightProj";
        m_pGeneralParams = "g_GeneralParams";
        m_pParamLightDiffuse = "g_LightDiffuse";

        m_pParamAmbient = "g_cDeferredAmbient";
        m_pParamAmbientGround = "g_cAmbGround";
        m_pParamAmbientHeight = "g_vAmbHeightParams";

        m_pAttenParams = "g_vAttenParams";

        m_pParamDecalTS = "g_mDecalTS";
        m_pParamDecalDiffuse = "g_DecalDiffuse";
        m_pParamDecalAngleAttenuation = "g_DecalAngleAttenuation";
        m_pParamDecalSpecular = "g_DecalSpecular";
        m_pParamDecalMipLevels = "g_DecalMipLevels";
        m_pParamDecalEmissive = "g_DecalEmissive";
        m_pParamTexTransforms = "g_texTransforms";
        m_pClipVolumeParams = "g_vVisAreasParams";

        m_pLBufferDiffuseRT = CTexture::s_ptexCurrentSceneDiffuseAccMap;
        m_pLBufferSpecularRT = CTexture::s_ptexSceneSpecularAccMap;
        m_pNormalsRT = CTexture::s_ptexSceneNormalsMap;
        m_pDepthRT = CTexture::s_ptexZTarget;
        m_pMSAAMaskRT = CTexture::s_ptexBackBuffer;
        m_pResolvedStencilRT =  CTexture::s_ptexStereoR;
        
        m_pDiffuseRT = CTexture::s_ptexSceneDiffuse;
        m_pSpecularRT = CTexture::s_ptexSceneSpecular;

        memset(m_vClipVolumeParams, 0, sizeof(Vec4) * (MAX_DEFERRED_CLIP_VOLUMES));

        m_nLightsProcessedCount = 0;

        m_nCurLighID = -1;

        m_nWarningFrame = 0;

        m_bSpecularState = false;

        m_nShadowPoolSize = 0;

        for (int i = 0; i < MAX_GPU_NUM; ++i)
        {
            m_prevViewProj[i].SetIdentity();
        }

        m_nRenderState = GS_BLSRC_ONE | GS_BLDST_ONE;

        m_nTexStateLinear = CTexture::GetTexState(STexState(FILTER_LINEAR, true));
        m_nTexStatePoint = CTexture::GetTexState(STexState(FILTER_POINT, true));

        m_nThreadID = 0;
        m_nRecurseLevel = 0;

        m_nBindResourceMsaa = -1;

        for (int i = 0; i < RT_COMMAND_BUF_COUNT; ++i)
        {
            for (int j = 0; j < MAX_REND_RECURSION_LEVELS; ++j)
            {
                m_nClipVolumesCount[i][j] = 0;
                m_nVisAreasGIRef[i][j] = 0;
            }
        }
    }

    ~CDeferredShading()
    {
        Release();
    }

    // Allow disable mrt usage: for double zspeed and on other passes less fillrate hit
    // @return - Returns true if either a push or a pop was performed in this function.  Returns false if no push or pop was executed.
    bool SpecularAccEnableMRT(bool bEnable);

private:

    struct SVisAreaBlendData
    {
        uint8 nBlendIDs[SClipVolumeBlendInfo::BlendPlaneCount];
        Vec4 blendPlanes[SClipVolumeBlendInfo::BlendPlaneCount];
    };

    struct SClipVolumeData
    {
        SClipVolumeData()
            : nStencilRef(0)
            , nFlags(0)
            , m_pRenderMesh(NULL)
            , mAABB(AABB::RESET)
            , mWorldTM(IDENTITY)
        {
        };

        Matrix34 mWorldTM;
        AABB mAABB;
        uint8 nStencilRef;
        uint8 nFlags;

        _smart_ptr<IRenderMesh> m_pRenderMesh;
        SVisAreaBlendData m_BlendData;
    };

    // Vis areas for current view
    std::vector<SClipVolumeData> m_pClipVolumes[RT_COMMAND_BUF_COUNT][MAX_REND_RECURSION_LEVELS];

    // The 2 is for X and Y axis. In PostEffectsLib.cfi:
    // float2x4 g_texTransforms[16];
    Vec4 m_vTextureTransforms[EMaxTextureSlots][2];

    uint32 m_nClipVolumesCount[RT_COMMAND_BUF_COUNT][MAX_REND_RECURSION_LEVELS];
    uint32 m_nVisAreasGIRef[RT_COMMAND_BUF_COUNT][MAX_REND_RECURSION_LEVELS];

    struct SClipShape
    {
        IRenderMesh*    pShape;
        Matrix34            mxTransform;
        SClipShape()
            : pShape(NULL)
            , mxTransform(Matrix34::CreateIdentity()) {}
        SClipShape(IRenderMesh* _pShape, const Matrix34& _mxTransform)
            : pShape(_pShape)
            , mxTransform(_mxTransform) { }
    };

    // Clip volumes for GI for current view
    TArray<SClipShape> m_vecGIClipVolumes[RT_COMMAND_BUF_COUNT][MAX_REND_RECURSION_LEVELS];

    // Deferred passes common
    TArray<SRenderLight> m_pLights[eDLT_NumLightTypes][RT_COMMAND_BUF_COUNT][MAX_REND_RECURSION_LEVELS];

    Vec3 m_pCamPos;
    Vec3 m_pCamFront;
    float m_fCamFar;
    float m_fCamNear;

    float m_fRatioWidth;
    float m_fRatioHeight;

    CShader* m_pShader;
    CCryNameTSCRC m_pDeferredDecalTechName;
    CCryNameTSCRC m_pLightVolumeTechName;
    CCryNameTSCRC m_pTechName;
    CCryNameTSCRC m_pAmbientOutdoorTechName;
    CCryNameTSCRC m_pCubemapsTechName;
    CCryNameTSCRC m_pCubemapsVolumeTechName;
    CCryNameTSCRC m_pReflectionTechName;
    CCryNameTSCRC m_pDebugTechName;
    CCryNameR m_pParamLightPos;
    CCryNameR m_pParamLightDiffuse;
    CCryNameR m_pParamLightProjMatrix;
    CCryNameR m_pGeneralParams;
    CCryNameR m_pParamAmbient;
    CCryNameR m_pParamAmbientGround;
    CCryNameR m_pParamAmbientHeight;
    CCryNameR m_pAttenParams;

    CCryNameR m_pParamDecalTS;
    CCryNameR m_pParamDecalDiffuse;
    CCryNameR m_pParamDecalAngleAttenuation;
    CCryNameR m_pParamDecalSpecular;
    CCryNameR m_pParamDecalMipLevels;
    CCryNameR m_pParamDecalEmissive;
    CCryNameR m_pParamTexTransforms;
    CCryNameR m_pClipVolumeParams;

    Matrix44A m_mViewProj;
    Matrix44A m_pViewProjI;
    Matrix44A m_pView;

    Matrix44A m_prevViewProj[MAX_GPU_NUM];

    Vec4 vWorldBasisX, vWorldBasisY, vWorldBasisZ;

    CTexture* m_pLBufferDiffuseRT;
    CTexture* m_pLBufferSpecularRT;

    DEFINE_ALIGNED_DATA(Vec4, m_vClipVolumeParams[MAX_DEFERRED_CLIP_VOLUMES], 16);

    CTexture* m_pDiffuseRT;
    CTexture* m_pSpecularRT;
    CTexture* m_pNormalsRT;
    CTexture* m_pDepthRT;

    CTexture* m_pMSAAMaskRT;
    CTexture* m_pResolvedStencilRT;

    int m_nWarningFrame;

    int m_nRenderState;
    uint32 m_nLightsProcessedCount;

    uint32 m_nTexStateLinear;
    uint32 m_nTexStatePoint;

    SResourceView::KeyType m_nBindResourceMsaa;

    uint32 m_nThreadID;
    int32 m_nRecurseLevel;

    uint m_nCurrentShadowPoolLight;
    uint m_nFirstCandidateShadowPoolLight;
    uint m_nShadowPoolSize;
    bool m_bClearPool;

    bool m_bSpecularState;
    int m_nCurLighID;
    short m_nCurTargetWidth;
    short m_nCurTargetHeight;
    static CDeferredShading* m_pInstance;

    friend class CTiledShading;

    static StaticInstance<CPowerOf2BlockPacker> m_blockPack;
    static TArray<SShadowAllocData> m_shadowPoolAlloc;

public:
    static CDeferredShading* CreateDeferredShading()
    {
        m_pInstance = new CDeferredShading();

        return m_pInstance;
    }

    static void DestroyDeferredShading()
    {
        SAFE_DELETE(m_pInstance);
    }
};

class CTexPoolAtlas
{
public:
    CTexPoolAtlas()
    {
        m_nSize = 0;
#ifdef _DEBUG
        m_nTotalWaste = 0;
#endif
    }
    ~CTexPoolAtlas()    {}
    void Init(int nSize);
    void Clear();
    void FreeMemory();
    bool AllocateGroup(int32* pOffsetX, int32* pOffsetY, int nSizeX, int nSizeY);

    int m_nSize;
    static const int MAX_BLOCKS = 128;
    uint32 m_arrAllocatedBlocks[MAX_BLOCKS];

#ifdef _DEBUG
    uint32 m_nTotalWaste;
    struct SShadowMapBlock
    {
        uint16 m_nX1, m_nX2, m_nY1, m_nY2;
        bool Intersects(const SShadowMapBlock& b) const
        {
            return max(m_nX1, b.m_nX1) < min(m_nX2, b.m_nX2)
                   && max(m_nY1, b.m_nY1) < min(m_nY2, b.m_nY2);
        }
    };
    std::vector<SShadowMapBlock> m_arrDebugBlocks;

    void _AddDebugBlock(int x, int y, int sizeX, int sizeY);
    float _GetDebugUsage() const;
#endif
};

#endif
