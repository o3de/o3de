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

#ifndef __D3D_SVO__H__
#define __D3D_SVO__H__

#if defined(FEATURE_SVO_GI)

struct SSvoTargetsSet
{
    SSvoTargetsSet() { ZeroStruct(*this); }
    void Release();

    CTexture
        // tracing targets
        *pRT_RGB_0, *pRT_ALD_0,
        *pRT_RGB_1, *pRT_ALD_1,
        // de-mosaic targets
        *pRT_RGB_DEM_MIN_0, *pRT_ALD_DEM_MIN_0,
        *pRT_RGB_DEM_MAX_0, *pRT_ALD_DEM_MAX_0,
        *pRT_RGB_DEM_MIN_1, *pRT_ALD_DEM_MIN_1,
        *pRT_RGB_DEM_MAX_1, *pRT_ALD_DEM_MAX_1,
        // output
        *pRT_FIN_OUT_0,
        *pRT_FIN_OUT_1;
};

class CSvoRenderer
    : public ISvoRenderer
{
public:
    static Vec4 GetDisabledPerFrameShaderParameters();
    static CSvoRenderer* GetInstance(bool bCheckAlloce = false);

    void Release();
    static bool IsActive();
    void UpdateCompute();
    void UpdateRender();
    int GetIntegratioMode();

    Vec4 GetPerFrameShaderParameters() const;

    void DebugDrawStats(const RPProfilerStats* pBasicStats, float& ypos, const float ystep, float xposms);
    static bool SetSamplers(int nCustomID, EHWShaderClass eSHClass, int nTUnit, int nTState, int nTexMaterialSlot, int nSUnit);

    CTexture* GetDiffuseFinRT();
    CTexture* GetSpecularFinRT();
    static CTexture* GetRsmColorMap(ShadowMapFrustum& rFr, bool bCheckUpdate = false);
    static CTexture* GetRsmNormlMap(ShadowMapFrustum& rFr, bool bCheckUpdate = false);
    float GetSsaoAmount() { return IsActive() ? gEnv->pConsole->GetCVar("e_svoTI_SSAOAmount")->GetFVal() : 1.f; }
    CTexture* GetRsmPoolCol() { return IsActive() ? m_pRsmPoolCol : NULL; }
    CTexture* GetRsmPoolNor() { return IsActive() ? m_pRsmPoolNor : NULL; }

    void Lock() { m_renderMutex.lock(); }
    void Unlock() { m_renderMutex.unlock(); }

protected:

    CSvoRenderer();
    virtual ~CSvoRenderer();
    bool IsShaderItemUsedForVoxelization(SShaderItem& rShaderItem, IRenderNode* pRN);
    static CTexture* GetGBuffer(int nId);
    void UpScalePass(SSvoTargetsSet* pTS);
    void DemosaicPass(SSvoTargetsSet* pTS);
    void ConeTracePass(SSvoTargetsSet* pTS);
    void SetupRsmSun(const EHWShaderClass eShClass);
    void SetShaderFloat(const EHWShaderClass eShClass, const CCryNameR& NameParam, const Vec4* fParams, int nParams);
    void CheckCreateUpdateRT(CTexture*& pTex, int nWidth, int nHeight, ETEX_Format eTF, ETEX_Type eTT, int nTexFlags, const char* szName);

    enum EComputeStages
    {
        eCS_ClearBricks,
        eCS_InjectDynamicLights,
        eCS_InjectStaticLights,
        eCS_PropagateLighting_1to2,
        eCS_PropagateLighting_2to3,
    };

    void ExecuteComputeShader(CShader* pSH, const char* szTechFinalName, EComputeStages etiStage, int* nNodesForUpdateStartIndex, int nObjPassId, PodArray<I3DEngine::SSvoNodeInfo>& arrNodesForUpdate);
    void SetShaderFlags(bool bDiffuseMode = true, bool bPixelShader = true);
    void SetupSvoTexturesForRead(I3DEngine::SSvoStaticTexInfo& texInfo, EHWShaderClass eShaderClass, int nStage, int nStageOpa = 0, int nStageNorm = 0);
    void SetupNodesForUpdate(int& nNodesForUpdateStartIndex, PodArray<I3DEngine::SSvoNodeInfo>& arrNodesForUpdate);
    void CheckAllocateRT(bool bSpecPass);
    void SetupLightSources(PodArray<I3DEngine::SLightTI>& lightsTI, CShader* pShader, bool bPS);
    void BindTiledLights(PodArray<I3DEngine::SLightTI>& lightsTI, EHWShaderClass shaderType);
    void DrawPonts(PodArray<SVF_P3F_C4B_T2F>& arrVerts);

    void UpdatePassConstantBuffer();

    SSvoTargetsSet m_tsDiff, m_tsSpec;

    CTexture
        *m_pRT_NID_0,
        *m_pRT_AIR_MIN,
        *m_pRT_AIR_MAX,
        *m_pRT_AIR_SHAD;

    int m_nTexStateTrilinear, m_nTexStateLinear, m_nTexStatePoint, m_nTexStateLinearWrap;
    Matrix44A m_matReproj;
    Matrix44A m_matViewProjPrev;
    CShader* m_pShader;
    CTexture* m_pNoiseTex;
    CTexture* m_pRsmColorMap;
    CTexture* m_pRsmNormlMap;
    CTexture* m_pRsmPoolCol;
    CTexture* m_pRsmPoolNor;
    AzRHI::ConstantBufferPtr m_PassConstantBuffer;

    struct SVoxPool
    {
        SVoxPool() { nTexId = 0; pUAV = 0; }
        void Init(ITexture* pTex);
        int nTexId;
        ID3D11UnorderedAccessView* pUAV;
    };

    SVoxPool vp_OPAC;
    SVoxPool vp_RGB0;
    SVoxPool vp_RGB1;
    SVoxPool vp_DYNL;
    SVoxPool vp_RGB2;
    SVoxPool vp_RGB3;
    SVoxPool vp_NORM;
    SVoxPool vp_ALDI;

    PodArray<I3DEngine::SSvoNodeInfo> m_nodesForStaticUpdate;
    PodArray<I3DEngine::SSvoNodeInfo> m_nodesForDynamicUpdate;
    PodArray<I3DEngine::SLightTI> m_arrLightsStatic;
    PodArray<I3DEngine::SLightTI> m_arrLightsDynamic;
    static const int SVO_MAX_NODE_GROUPS = 4;
    float               m_arrNodesForUpdate[SVO_MAX_NODE_GROUPS][4][4];
    int                 m_nCurPropagationPassID;
    I3DEngine::SSvoStaticTexInfo m_texInfo;
    static CSvoRenderer* s_pInstance;
    PodArray<I3DEngine::SSvoNodeInfo> m_arrNodeInfo;
    AZStd::mutex m_renderMutex;

};
#endif
#endif
