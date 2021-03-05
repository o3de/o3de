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

#ifndef CRYINCLUDE_CRY3DENGINE_MATERIAL_H
#define CRYINCLUDE_CRY3DENGINE_MATERIAL_H
#pragma once

#include <IMaterial.h>

#if !defined(CONSOLE)
#   define TRACE_MATERIAL_LEAKS
#   define SUPPORT_MATERIAL_EDITING
#endif

#ifndef _RELEASE
#define SUPPORT_MATERIAL_SKETCH
#endif

class CMaterialLayer
    : public IMaterialLayer
{
public:
    CMaterialLayer()
        : m_nRefCount(0)
        , m_nFlags(0)
    {
    }

    virtual ~CMaterialLayer()
    {
        SAFE_RELEASE(m_pShaderItem.m_pShader);
        SAFE_RELEASE(m_pShaderItem.m_pShaderResources);
    }

    virtual void AddRef()
    {
        m_nRefCount++;
    };

    virtual void Release()
    {
        if (--m_nRefCount <= 0)
        {
            delete this;
        }
    }

    virtual void Enable(bool bEnable = true)
    {
        m_nFlags |= (bEnable == false) ? MTL_LAYER_USAGE_NODRAW : 0;
    }

    virtual bool IsEnabled() const
    {
        return (m_nFlags & MTL_LAYER_USAGE_NODRAW) ? false : true;
    }

    virtual void FadeOut(bool bFadeOut = true)
    {
        m_nFlags |= (bFadeOut == false) ? MTL_LAYER_USAGE_FADEOUT : 0;
    }

    virtual bool DoesFadeOut() const
    {
        return (m_nFlags & MTL_LAYER_USAGE_FADEOUT) ? true : false;
    }

    virtual void SetShaderItem(const _smart_ptr<IMaterial> pParentMtl, const SShaderItem& pShaderItem);

    virtual const SShaderItem& GetShaderItem() const
    {
        return m_pShaderItem;
    }

    virtual SShaderItem& GetShaderItem()
    {
        return m_pShaderItem;
    }

    virtual void SetFlags(uint8 nFlags)
    {
        m_nFlags = nFlags;
    }

    virtual uint8 GetFlags() const
    {
        return m_nFlags;
    }

    void GetMemoryUsage(ICrySizer* pSizer);
    size_t GetResourceMemoryUsage(ICrySizer* pSizer);

private:
    uint8 m_nFlags;
    int m_nRefCount;
    SShaderItem m_pShaderItem;
};

//////////////////////////////////////////////////////////////////////
class CMatInfo
    : public IMaterial
    , public Cry3DEngineBase
{
public:
    CMatInfo();
    ~CMatInfo();

    void ShutDown();

    virtual void AddRef();
    virtual void Release();

    virtual int GetNumRefs() { return m_nRefCount; };

    //////////////////////////////////////////////////////////////////////////
    // IMaterial implementation
    //////////////////////////////////////////////////////////////////////////
    int Size();

    virtual IMaterialHelpers& GetMaterialHelpers();
    virtual IMaterialManager* GetMaterialManager();

    virtual void SetName(const char* pName);
    virtual const char* GetName() const { return m_sMaterialName; };

    virtual void SetFlags(int flags) { m_Flags = flags; };
    virtual int GetFlags() const { return m_Flags; };
    virtual void UpdateFlags();

    bool IsMaterialGroup() const override;
    bool IsSubMaterial() const override;

    // Returns true if this is the default material.
    virtual bool IsDefault();

    virtual int GetSurfaceTypeId() { return m_nSurfaceTypeId; };

    virtual void SetSurfaceType(const char* sSurfaceTypeName);
    virtual ISurfaceType* GetSurfaceType();

    void SetShaderName(const char* pName) override
    {
        m_shaderName = pName;
    };
    const char* GetShaderName() const override
    {
        return m_shaderName.c_str();
    };

    // shader item
    virtual void ReleaseCurrentShaderItem();
    virtual void SetShaderItem(const SShaderItem& _ShaderItem);
    // [Alexey] EF_LoadShaderItem return value with RefCount = 1, so if you'll use SetShaderItem after EF_LoadShaderItem use Assign function
    virtual void AssignShaderItem(const SShaderItem& _ShaderItem);

    virtual SShaderItem& GetShaderItem();
    virtual const SShaderItem& GetShaderItem() const;

    /**
    * Retrieves the shader item of the sub-material of the given index
    *
    * If the material has no sub-materials or is not flagged as having any sub-materials
    * this will return its own shader item. If no shader item is found for the given
    * sub-material the default material's shader item will be returned instead.
    *
    * @param nSubMtlSlot The index to the requested sub-material
    *
    * @return A reference to the sub-material's shader item
    */
    virtual SShaderItem& GetShaderItem(int nSubMtlSlot);

    /**
    * Retrieves the shader item of the sub-material of the given index
    *
    * If the material has no sub-materials or is not flagged as having any sub-materials
    * this will return its own shader item. If no shader item is found for the given
    * sub-material the default material's shader item will be returned instead.
    *
    * @param nSubMtlSlot The index to the requested sub-material
    *
    * @return A reference to the sub-material's shader item
    */
    virtual const SShaderItem& GetShaderItem(int nSubMtlSlot) const;

    virtual bool IsStreamedIn(const int nMinPrecacheRoundIds[MAX_STREAM_PREDICTION_ZONES], IRenderMesh* pRenderMesh) const;
    bool AreChunkTexturesStreamedIn(CRenderChunk* pChunk, const int nMinPrecacheRoundIds[MAX_STREAM_PREDICTION_ZONES]) const;
    bool AreTexturesStreamedIn(const int nMinPrecacheRoundIds[MAX_STREAM_PREDICTION_ZONES]) const;

    //////////////////////////////////////////////////////////////////////////
    // Functions to set param into the material/material group
    // If this is a material group, materialIndex specifies the index of the sub-material to be modified
    //////////////////////////////////////////////////////////////////////////
    bool SetGetMaterialParamFloat(const char* sParamName, float& v, bool bGet, bool allowShaderParam = false, int materialIndex = 0) override;
    bool SetGetMaterialParamVec3(const char* sParamName, Vec3& v, bool bGet, bool allowShaderParam = false, int materialIndex = 0) override;
    bool SetGetMaterialParamVec4(const char* sParamName, Vec4& v, bool bGet, bool allowShaderParam = false, int materialIndex = 0) override;
    
    void SetDirty(bool dirty = true) override;
    bool IsDirty() const override;

    //////////////////////////////////////////////////////////////////////////
    // Sub materials.
    //////////////////////////////////////////////////////////////////////////
    virtual void SetSubMtlCount(int numSubMtl);
    virtual int GetSubMtlCount(){return m_subMtls.size(); }
    virtual _smart_ptr<IMaterial> GetSubMtl(int nSlot)
    {
        if (m_subMtls.empty() || !(m_Flags & MTL_FLAG_MULTI_SUBMTL))
        {
            return 0; // Not Multi material.
        }
        if (nSlot >= 0 && nSlot < (int)m_subMtls.size())
        {
            return m_subMtls[nSlot];
        }
        else
        {
            return 0;
        }
    }
    virtual void SetSubMtl(int nSlot, _smart_ptr<IMaterial> pMtl);
    virtual void SetUserData(void* pUserData);
    virtual void* GetUserData() const;

    virtual _smart_ptr<IMaterial> GetSafeSubMtl(int nSlot);
    virtual _smart_ptr<CMatInfo> Clone();
    virtual void Copy(_smart_ptr<IMaterial> pMtlDest, EMaterialCopyFlags flags);

    //////////////////////////////////////////////////////////////////////////
    // Layers
    //////////////////////////////////////////////////////////////////////////
    virtual void SetLayerCount(uint32 nCount);
    virtual uint32 GetLayerCount() const;
    virtual void SetLayer(uint32 nSlot, IMaterialLayer* pLayer);
    virtual const IMaterialLayer* GetLayer(uint8 nLayersMask, uint8 nLayersUsageMask) const;
    virtual const IMaterialLayer* GetLayer(uint32 nSlot) const;
    virtual IMaterialLayer* CreateLayer();

    // Fill int table with surface ids of sub materials.
    // Return number of filled items.
    int FillSurfaceTypeIds(int pSurfaceIdsTable[]);

    virtual void GetMemoryUsage(ICrySizer* pSizer) const;

    virtual size_t GetResourceMemoryUsage(ICrySizer* pSizer);

    void UpdateShaderItems() override;
    void RefreshShaderResourceConstants();

    //////////////////////////////////////////////////////////////////////////
    void SetSketchMode(int mode);
    void SetTexelDensityDebug(int mode);

    // Check for specific rendering conditions (forward rendering/nearest cubemap requirement)
    bool IsForwardRenderingRequired();
    bool IsNearestCubemapRequired();

    //////////////////////////////////////////////////////////////////////////
    // Debug routines
    //////////////////////////////////////////////////////////////////////////
    virtual const char* GetLoadingCallstack();  // trace leaking materials by callstack

    void DisableTextureStreaming() override;
    virtual void RequestTexturesLoading(const float fMipFactor);

    virtual void PrecacheMaterial(const float fEntDistance, struct IRenderMesh* pRenderMesh, bool bFullUpdate, bool bDrawNear = false);
    void PrecacheTextures(const float fMipFactor, const int nFlags, bool bFullUpdate);
    void PrecacheChunkTextures(const float fInstanceDistance, const int nFlags, CRenderChunk* pRenderChunk, bool bFullUpdate);

    virtual int GetTextureMemoryUsage(ICrySizer* pSizer, int nSubMtlSlot = -1);
    virtual void SetKeepLowResSysCopyForDiffTex();

    virtual void SetMaterialLinkName(const char* name);
    virtual const char* GetMaterialLinkName() const;

    uint32 GetDccMaterialHash() const override { return m_dccMaterialHash; }
    void SetDccMaterialHash(uint32 hash) override { m_dccMaterialHash = hash; }

    virtual CryCriticalSection& GetSubMaterialResizeLock();
public:
    //////////////////////////////////////////////////////////////////////////
    // for debug purposes
    //////////////////////////////////////////////////////////////////////////
#ifdef TRACE_MATERIAL_LEAKS
    string  m_sLoadingCallstack;
#endif

private:
    friend class CMatMan;
    friend class CMaterialLayer;

    //////////////////////////////////////////////////////////////////////////
    string m_sMaterialName;
    string m_sUniqueMaterialName;

    // Id of surface type assigned to this material.
    int m_nSurfaceTypeId;

    //! Number of references to this material.
    int m_nRefCount;
    //! Material flags.
    //! @see EMatInfoFlags
    int m_Flags;

    uint32 m_dccMaterialHash;

    SShaderItem m_shaderItem;

    //! shader full name
    AZStd::string m_shaderName;

#ifdef SUPPORT_MATERIAL_SKETCH
    _smart_ptr<IShader> m_pPreSketchShader;
    int m_nPreSketchTechnique;
#endif

    //! Array of Sub materials.
    typedef DynArray<_smart_ptr<CMatInfo> > SubMtls;
    SubMtls m_subMtls;

#ifdef SUPPORT_MATERIAL_EDITING
    // User data used by Editor.
    void* m_pUserData;

    string m_sMaterialLinkName;
#endif

    //! Material layers
    typedef std::vector< _smart_ptr< CMaterialLayer > > MatLayers;
    MatLayers* m_pMaterialLayers;

    //! Used for material layers
    mutable CMaterialLayer* m_pActiveLayer;

    struct SStreamingPredictionZone
    {
        int nRoundId : 31;
        int bHighPriority : 1;
        float fMinMipFactor;
    } m_streamZoneInfo[2];

    bool m_isDirty;
};

#endif // CRYINCLUDE_CRY3DENGINE_MATERIAL_H

