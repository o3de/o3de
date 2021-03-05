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

#pragma once

#include <AzCore/std/containers/map.h>
#include "DeviceManager/Enums.h"

//==============================================================================
//! This class provide all necessary resources to the shader extracted from material definition.
//------------------------------------------------------------------------------
class CShaderResources
    : public IRenderShaderResources
    , public SBaseShaderResources
{
private:
    // Dynamically managed vector of all required shaders constants  - 
    // This array will be copied to the (Per Material) constant buffer when ready.
    AZStd::vector<Vec4>                 m_Constants;

    // The actual constant buffer to be binded as the Per material CB
    AzRHI::ConstantBuffer*              m_ConstantBuffer;       

public:
    TexturesResourcesMap                m_TexturesResourcesMap;          // A map of texture used by the shader
    SDeformInfo*                        m_pDeformInfo;          // 4 bytes - Per Texture modulator info
    TArray<struct SHRenderTarget*>      m_RTargets;             // 4
    SSkyInfo*                           m_pSky;                 // [Shader System TO DO] - disconnect and remove!
    uint16                              m_Id;                   // Id of the shader resource in the frame's SR list - s_ShaderResources_known
    uint16                              m_IdGroup;              // Id of the SR group for this SR in the frame's SR list.  Starts at 20,000

    /////////////////////////////////////////////////////
    float                               m_fMinMipFactorLoad;
    int                                 m_nRefCounter;
    int                                 m_nFrameLoad;

    // Only do expensive DX12 resource set building for PC DX12
#if defined(CRY_USE_DX12)
    // Compiled resource set.
    // For DX12 will prepare list of textures in the global heap.
    AZStd::shared_ptr<class CDeviceResourceSet>               m_pCompiledResourceSet;
    AZStd::shared_ptr<class CGraphicsPipelineStateLocalCache> m_pipelineStateCache;
#endif

    uint8 m_nMtlLayerNoDrawFlags;

public:

    bool TextureSlotExists(ResourceSlotIndex slotId) const
    {
        auto    iter = m_TexturesResourcesMap.find(slotId);
        return (iter != m_TexturesResourcesMap.end()) ? true : false;
    }

    SEfResTexture* GetTextureResource(ResourceSlotIndex slotId)
    {
        auto    iter = m_TexturesResourcesMap.find(slotId);
        return (iter != m_TexturesResourcesMap.end()) ? &iter->second : nullptr;
    }

    TexturesResourcesMap* GetTexturesResourceMap()
    {
        return &m_TexturesResourcesMap;
    }

    inline AzRHI::ConstantBuffer* GetConstantBuffer() 
    {
        return m_ConstantBuffer;
    }

    int Size() const
    {
        int     nSize = sizeof(CShaderResources);
        for ( auto iter=m_TexturesResourcesMap.begin() ; iter != m_TexturesResourcesMap.end() ; ++iter )
        {
            nSize += iter->second.Size();
        }

        nSize += sizeofVector(m_Constants);
        nSize += m_RTargets.GetMemoryUsage();
        if (m_pDeformInfo)
        {
            nSize += m_pDeformInfo->Size();
        }
        return nSize;
    }

    void GetMemoryUsage(ICrySizer* pSizer) const final
    {
        pSizer->AddObject(this, sizeof(*this));

        for ( auto iter=m_TexturesResourcesMap.begin() ; iter != m_TexturesResourcesMap.end() ; ++iter )
        {
            pSizer->AddObject(iter->second);
        }


        pSizer->AddObject(m_Constants);
        pSizer->AddObject(m_RTargets);
        pSizer->AddObject(m_pDeformInfo);
        SBaseShaderResources::GetMemoryUsage(pSizer);
    }

    CShaderResources();
    CShaderResources& operator=(const CShaderResources& src);
    CShaderResources(struct SInputShaderResources* pSrc);

    void PostLoad(CShader* pSH);
    void AdjustForSpec();
    void CreateModifiers(SInputShaderResources* pInRes);
    virtual void UpdateConstants(IShader* pSH) final;
    virtual void CloneConstants(const IRenderShaderResources* pSrc) final;
    virtual int GetResFlags() final { return m_ResFlags; }
    virtual void SetMaterialName(const char* szName) final { m_szMaterialName = szName; }
    virtual SSkyInfo* GetSkyInfo() final { return m_pSky; }
    virtual const float& GetAlphaRef() const final { return m_AlphaRef; }
    virtual void SetAlphaRef(float alphaRef) final { m_AlphaRef = alphaRef; }

    virtual AZStd::vector<SShaderParam>& GetParameters() final { return m_ShaderParams; }
    virtual ColorF GetFinalEmittance() final
    {
        const float kKiloScale = 1000.0f;
        return GetColorValue(EFTT_EMITTANCE) * GetStrengthValue(EFTT_EMITTANCE) * (kKiloScale / RENDERER_LIGHT_UNIT_SCALE);
    }
    virtual float GetVoxelCoverage() final { return ((float)m_VoxelCoverage) * (1.0f / 255.0f); }

    virtual void SetMtlLayerNoDrawFlags(uint8 nFlags) final { m_nMtlLayerNoDrawFlags = nFlags; }
    virtual uint8 GetMtlLayerNoDrawFlags() const final { return m_nMtlLayerNoDrawFlags; }

    void Rebuild(IShader* pSH, AzRHI::ConstantBufferUsage usage = AzRHI::ConstantBufferUsage::Dynamic);
    void ReleaseConstants();

    void Reset();

    bool IsDeforming() const
    {
        return (m_pDeformInfo && m_pDeformInfo->m_fDividerX != 0);
    }

    bool HasLMConstants() const { return (m_Constants.size() > 0); }
    virtual void SetInputLM(const CInputLightMaterial& lm) final;
    virtual void ToInputLM(CInputLightMaterial& lm) final;

    virtual ColorF GetColorValue(EEfResTextures slot) const final;
    virtual float GetStrengthValue(EEfResTextures slot) const final;

    virtual void SetColorValue(EEfResTextures slot, const ColorF& color) final;
    virtual void SetStrengthValue(EEfResTextures slot, float value) final;

    ~CShaderResources();
    virtual void Release() final;
    virtual void AddRef() final { CryInterlockedIncrement(&m_nRefCounter); }
    virtual void ConvertToInputResource(SInputShaderResources* pDst) final;
    virtual CShaderResources* Clone() const final;
    virtual void SetShaderParams(SInputShaderResources* pDst, IShader* pSH) final;

    virtual size_t GetResourceMemoryUsage(ICrySizer* pSizer) final;

    void Cleanup();
};
typedef _smart_ptr<CShaderResources> CShaderResourcesPtr;
