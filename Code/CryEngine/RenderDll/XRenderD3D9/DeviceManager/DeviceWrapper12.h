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

#include <VertexFormats.h>
#include <ITexture.h>
#include <array>

#include "Enums.h"

class CHWShader_D3D;
class CShader;
class CTexture;
class CCryNameTSCRC;
namespace AzRHI { class ConstantBuffer; }
class CShaderResources;
struct SGraphicsPipelineStateDescription;

#if !defined(NULL_RENDERER)

/////////////////////////////////////////////////////////////////////////////////
//// include concrete device/context implementations
#if defined(CRY_USE_DX12)
#define CRY_USE_DX12_NATIVE
#include "XRenderD3D9/DX12/API/DX12CommandList.hpp"
#endif

////////////////////////////////////////////////////////////////////////////

class CDeviceResourceSet
{
    friend class CDeviceObjectFactory;
    friend class CDeviceResourceLayout;
    friend class CDeviceResourceLayout_DX12;

public:
    enum EFlags
    {
        EFlags_None = 0,
        EFlags_ForceSetAllState,
    };

    CDeviceResourceSet(EFlags flags);
    virtual ~CDeviceResourceSet();

    bool IsDirty() const { return m_bDirty; }
    void SetDirty(bool bDirty);

    static int  GetGlobalDirtyCount() { return s_dirtyCount; }
    static void ResetGlobalDirtyCount() { s_dirtyCount = 0; }

    void Clear();

    void SetTexture(ShaderSlot shaderSlot, _smart_ptr<CTexture> pTexture, SResourceView::KeyType resourceViewID = SResourceView::DefaultView, EShaderStage shaderStages = EShaderStage_Pixel);
    void SetSampler(ShaderSlot shaderSlot, int sampler, EShaderStage shaderStages = EShaderStage_Pixel);
    void SetConstantBuffer(ShaderSlot shaderSlot, _smart_ptr<AzRHI::ConstantBuffer> pBuffer, EShaderStage shaderStages = EShaderStage_Pixel);
    void SetBuffer(ShaderSlot shaderSlot, WrappedDX11Buffer buffer, EShaderStage shaderStages = EShaderStage_Pixel);

    EShaderStage GetShaderStages() const;
    EFlags GetFlags() const { return m_Flags; }

    bool Fill(CShader* pShader, CShaderResources* pResources, EShaderStage shaderStages = EShaderStage_Pixel);

    virtual void Prepare() = 0;
    virtual void Build() = 0;

protected:
    template<typename T>
    struct SResourceData
    {
        SResourceData()
            : resource()
            , shaderStages(EShaderStage_None) {}
        SResourceData(T _resource, ::EShaderStage _shaderStages)
            : resource(_resource)
            , shaderStages(_shaderStages) {}

        bool operator==(const SResourceData<T>& other) const
        {
            return resource == other.resource && shaderStages == other.shaderStages;
        }

        T resource;
        ::EShaderStage shaderStages;
    };

    typedef SResourceData< std::tuple<SResourceView::KeyType, _smart_ptr<CTexture> > > STextureData;
    typedef SResourceData< int >                                                       SSamplerData;
    typedef SResourceData< _smart_ptr<AzRHI::ConstantBuffer> >                               SConstantBufferData;
    typedef SResourceData< WrappedDX11Buffer >                                         SBufferData;

    CDeviceResourceSet(const CDeviceResourceSet&) {};
    void OnTextureChanged(uint32 dirtyFlags);

    VectorMap<ShaderSlot, STextureData> m_Textures;
    VectorMap<ShaderSlot, SSamplerData> m_Samplers;
    VectorMap<ShaderSlot, SBufferData>  m_Buffers;
    VectorMap<ShaderSlot, SConstantBufferData> m_ConstantBuffers;

    bool              m_bDirty : 1;
    EFlags m_Flags  : 8;

    static volatile int s_dirtyCount;
};

typedef AZStd::shared_ptr<CDeviceResourceSet> CDeviceResourceSetPtr;

////////////////////////////////////////////////////////////////////////////

class CDeviceResourceLayout
{
public:
    void Clear();
    void SetInlineConstants(uint32 numConstants);
    void SetResourceSet(uint32 bindSlot, CDeviceResourceSetPtr pResourceSet);
    void SetConstantBuffer(uint32 bindSlot, EConstantBufferShaderSlot shaderSlot, EShaderStage shaderStages);

    virtual bool Build() = 0;

protected:
    struct SConstantBufferShaderBinding
    {
        EConstantBufferShaderSlot shaderSlot;
        EShaderStage shaderStages;
    };

    CDeviceResourceLayout();
    virtual ~CDeviceResourceLayout() = default;

    bool IsValid();

    uint32 m_InlineConstantCount;
    VectorMap<uint32, SConstantBufferShaderBinding> m_ConstantBuffers;
    VectorMap<uint32, CDeviceResourceSetPtr> m_ResourceSets;
};

typedef AZStd::shared_ptr<CDeviceResourceLayout> CDeviceResourceLayoutPtr;

////////////////////////////////////////////////////////////////////////////
class CDeviceGraphicsPSODesc
{
public:
    CDeviceGraphicsPSODesc(const CDeviceGraphicsPSODesc& other);
    CDeviceGraphicsPSODesc(CDeviceResourceLayout* pResourceLayout, const SGraphicsPipelineStateDescription& pipelineDesc);
    CDeviceGraphicsPSODesc(CDeviceResourceLayout* pResourceLayout, CShader* pShader, const CCryNameTSCRC& technique, uint64 rtFlags, uint32 mdFlags, uint32 mdvFlags, bool bAllowTessellation);

    AZ::u32 GetHash() const;

public:
    void InitWithDefaults();

    void  FillDescs(D3D11_RASTERIZER_DESC& rasterizerDesc, D3D11_BLEND_DESC& blendDesc, D3D11_DEPTH_STENCIL_DESC& depthStencilDesc) const;
    uint8 CombineVertexStreamMasks(uint8 fromShader, uint8 fromObject) const;

    void Build();

public:
    CShader*                              m_pShader;
    CCryNameTSCRC                         m_technique;
    bool                                  m_bAllowTesselation;
    uint64                                m_ShaderFlags_RT;
    uint32                                m_ShaderFlags_MD;
    uint32                                m_ShaderFlags_MDV;
    uint32                                m_RenderState;
    uint32                                m_StencilState;
    uint8                                 m_StencilReadMask;
    uint8                                 m_StencilWriteMask;
    uint8                                 m_ObjectStreamMask;
    std::array<ETEX_Format, 8>            m_RenderTargetFormats;
    ETEX_Format                           m_DepthStencilFormat;
    ECull                                 m_CullMode;
    int32                                 m_DepthBias;
    f32                                   m_DepthBiasClamp;
    f32                                   m_SlopeScaledDepthBias;
    eRenderPrimitiveType                  m_PrimitiveType;
    CDeviceResourceLayout*                m_pResourceLayout;
    AZ::Vertex::Format                    m_VertexFormat;

private:
    AZ::u32 m_Hash;
};

namespace std
{
    template<>
    struct hash<CDeviceGraphicsPSODesc>
    {
        size_t operator()(const CDeviceGraphicsPSODesc& psoDesc) const
        {
            return psoDesc.GetHash();
        }
    };

    template<>
    struct equal_to<CDeviceGraphicsPSODesc>
    {
        bool operator()(const CDeviceGraphicsPSODesc& psoDesc1, const CDeviceGraphicsPSODesc& psoDesc2) const
        {
            return psoDesc1.GetHash() == psoDesc2.GetHash();
        }
    };
}


class CDeviceGraphicsPSO
{
public:
    std::array<void*, eHWSC_Num>          m_pHwShaderInstances; // TODO: remove once we don't need shader reflection anymore
    std::array<CHWShader_D3D*, eHWSC_Num> m_pHwShaders;         // TODO: remove once we don't need shader reflection anymore

#if defined(ENABLE_PROFILING_CODE)
    eRenderPrimitiveType m_PrimitiveTypeForProfiling;
#endif
};

class CDeviceComputePSO
{
    friend class CDeviceComputeCommandList;
    friend struct SDeviceObjectHelpers;

public:
    virtual ~CDeviceComputePSO() {};

    void SetShader(CShader* pShader, const CCryNameTSCRC& technique, uint64 rtFlags, uint32 mdFlags, uint32 mdvFlags);

    virtual bool Build() = 0;

protected:
    CDeviceComputePSO();
    CDeviceComputePSO(const CDeviceComputePSO&) {}

    std::array<CHWShader_D3D*, eHWSC_Num> m_pHwShaders;
    std::array<void*, eHWSC_Num>          m_pHwShaderInstances;
    std::array<void*, eHWSC_Num>          m_pDeviceShaders;
};

typedef CDeviceGraphicsPSO*                 CDeviceGraphicsPSOPtr;
typedef AZStd::unique_ptr<CDeviceGraphicsPSO> CDeviceGraphicsPSOUPtr;
typedef AZStd::shared_ptr<CDeviceComputePSO>  CDeviceComputePSOPtr;

////////////////////////////////////////////////////////////////////////////

class CDeviceCommandList
{
    friend class CDeviceObjectFactory;

public:
    CDeviceCommandList() {};
    virtual ~CDeviceCommandList() = default;

    void  SpecifyResourceUsage(/* resource, state */); // set => begin+end
    void AnnounceResourceUsage(/* resource, state */); // begin
    void  ApproveResourceUsage(/* resource, state */); // end

    virtual void LockToThread() = 0;
    virtual void Build() = 0;
};

class CDeviceCopyCommandList
    : public CDeviceCommandList
{
    friend class CDeviceObjectFactory;

public:
    enum ECopyType
    {
        eCT_GraphicsResources, // RenderTarget && DepthStencil && SwapChain -> Direct
        eCT_GenericResources,  // ShaderResource && UnorderedAccess -> Compute
        eCT_OffCardResources   // everything crossing PCIe -> XDMA
    };

    static ECopyType DetermineCopyType(ECopyType eCurrent, D3DResource* pResource);

public:
    CDeviceCopyCommandList([[maybe_unused]] ECopyType eType = eCT_OffCardResources) {};
    ~CDeviceCopyCommandList() override = default;

    void CopySubresourceRegion(D3DResource* pDstResource, UINT DstSubresource, UINT DstX, UINT DstY, UINT DstZ, D3DResource* pSrcResource, UINT SrcSubresource, const D3D11_BOX* pSrcBox, UINT CopyFlags);
    void CopyResource(D3DResource* pDstResource, ID3D11Resource* pSrcResource, UINT CopyFlags);
    void UpdateSubresource(D3DResource* pDstResource, UINT DstSubresource, const D3D11_BOX* pDstBox, const void* pSrcData, UINT SrcRowPitch, UINT SrcDepthPitch, UINT CopyFlags);

    virtual void LockToThread() override;
    virtual void Build() override;
};

class CDeviceGraphicsCommandList
    : public CDeviceCopyCommandList
{
    friend class CDeviceObjectFactory;

public:
    CDeviceGraphicsCommandList()
        : CDeviceCopyCommandList(eCT_GraphicsResources) {}
    ~CDeviceGraphicsCommandList() override = default;

    void SetRenderTargets(uint32 targetCount, CTexture** pTargets, SDepthTexture* pDepthTarget);
    void SetViewports(uint32 vpCount, const D3DViewPort* pViewports);
    void SetScissorRects(uint32 rcCount, const D3DRectangle* pRects);
    void SetPipelineState(CDeviceGraphicsPSOPtr devicePSO);
    void SetResourceLayout(CDeviceResourceLayout* pResourceLayout);
    void SetResources(uint32 bindSlot, CDeviceResourceSet* pResources);
    void SetInlineConstantBuffer(uint32 bindSlot, AzRHI::ConstantBuffer* pBuffer, EConstantBufferShaderSlot shaderSlot, EHWShaderClass shaderClass);
    void SetInlineConstantBuffer(uint32 bindSlot, AzRHI::ConstantBuffer* pBuffer, EConstantBufferShaderSlot shaderSlot, EShaderStage shaderStages);
    void SetVertexBuffers(uint32 bufferCount, D3DBuffer** pBuffers, size_t* offsets, uint32* strides);
    void SetVertexBuffers(uint32 streamCount, const SStreamInfo* streams);
    void SetIndexBuffer(const SStreamInfo& indexStream); // TODO: IMPLEMENT. NOTE: take care with PSO strip cut value and 32/16 bit indices
    void SetInlineConstants(uint32 bindSlot, uint32 constantCount, float* pConstants);
    void SetStencilRef(uint8 stencilRefValue);

    void Draw(uint32 VertexCountPerInstance, uint32 InstanceCount, uint32 StartVertexLocation, uint32 StartInstanceLocation);
    void DrawIndexed(uint32 IndexCountPerInstance, uint32 InstanceCount, uint32 StartIndexLocation, int BaseVertexLocation, uint32 StartInstanceLocation);

    void ClearSurface(D3DSurface* pView, const FLOAT Color[4], UINT NumRects, const D3D11_RECT* pRect);
    void ClearDepthSurface(D3DDepthSurface* pView, UINT ClearFlags, FLOAT Depth, UINT8 Stencil, UINT NumRects, const D3D11_RECT* pRect);

    virtual void LockToThread() final;
    virtual void Build() final;

    void Reset();

#if defined(AZ_RESTRICTED_PLATFORM)
    #include AZ_RESTRICTED_FILE(DeviceWrapper12_h)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#else
    inline void SwitchToNewGraphicsPipeline() {}
#endif

protected:
    void SetPipelineStateImpl(CDeviceGraphicsPSOPtr devicePSO);

    void DrawImpl(uint32 VertexCountPerInstance, uint32 InstanceCount, uint32 StartVertexLocation, uint32 StartInstanceLocation);
    void DrawIndexedImpl(uint32 IndexCountPerInstance, uint32 InstanceCount, uint32 StartIndexLocation, int BaseVertexLocation, uint32 StartInstanceLocation);
    void SetResourcesImpl(uint32 bindSlot, CDeviceResourceSet* pResources);
    void SetStencilRefImpl(uint8 stencilRefValue);

    void ResetImpl();

    CDeviceGraphicsPSOPtr                                      m_pCurrentPipelineState;
    std::array<CDeviceResourceSet*, EResourceLayoutSlot_Count> m_pCurrentResources;
    int32                                                      m_CurrentStencilRef;
};

class CDeviceComputeCommandList
    : public CDeviceCopyCommandList
{
    friend class CDeviceObjectFactory;

public:
    CDeviceComputeCommandList()
        : CDeviceCopyCommandList(eCT_GenericResources) {}
    ~CDeviceComputeCommandList() override = default;

    void SetPipelineState(CDeviceComputePSOPtr devicePSO);
    void SetResourceLayout(CDeviceResourceLayout* pResourceLayout);
    void SetResources(uint32 bindSlot, CDeviceResourceSet* pResources);
    void SetConstantBuffer(uint32 bindSlot, AzRHI::ConstantBuffer* pBuffer, EConstantBufferShaderSlot shaderSlot, EHWShaderClass shaderStage);
    void SetInlineConstants(uint32 bindSlot, uint32 constantCount, float* pConstants);

    void Dispatch(uint32 X, uint32 Y, uint32 Z);

    void ClearUAV(D3DUnorderedAccessView* pView, const FLOAT Values[4], UINT NumRects, const D3D11_RECT* pRect);
    void ClearUAV(D3DUnorderedAccessView* pView, const UINT Values[4], UINT NumRects, const D3D11_RECT* pRect);

    virtual void LockToThread() final;
    virtual void Build() final;

protected:
    void SetPipelineStateImpl(CDeviceComputePSOPtr devicePSO);

    void DispatchImpl(uint32 X, uint32 Y, uint32 Z);

    CDeviceComputePSOPtr m_pCurrentPipelineState;
};

typedef AZStd::shared_ptr<CDeviceGraphicsCommandList> CDeviceGraphicsCommandListPtr;
typedef AZStd::shared_ptr<CDeviceComputeCommandList> CDeviceComputeCommandListPtr;
typedef AZStd::shared_ptr<CDeviceCopyCommandList> CDeviceCopyCommandListPtr;

typedef CDeviceGraphicsCommandList& CDeviceGraphicsCommandListRef;
typedef CDeviceComputeCommandList& CDeviceComputeCommandListRef;
typedef CDeviceCopyCommandList& CDeviceCopyCommandListRef;

typedef AZStd::unique_ptr<CDeviceGraphicsCommandList> CDeviceGraphicsCommandListUPtr;
typedef AZStd::unique_ptr<CDeviceComputeCommandList> CDeviceComputeCommandListUPtr;
typedef AZStd::unique_ptr<CDeviceCopyCommandList> CDeviceCopyCommandListUPtr;

////////////////////////////////////////////////////////////////////////////
// Device Object Factory

class CDeviceObjectFactory
{
public:
    CDeviceObjectFactory();

    static CDeviceObjectFactory& GetInstance();

    CDeviceResourceSetPtr CreateResourceSet(CDeviceResourceSet::EFlags flags = CDeviceResourceSet::EFlags_None) const;
    CDeviceResourceLayoutPtr CreateResourceLayout() const;
    CDeviceGraphicsPSOPtr CreateGraphicsPSO(const CDeviceGraphicsPSODesc& psoDesc);
    CDeviceComputePSOPtr CreateComputePSO(CDeviceResourceLayoutPtr pResourceLayout) const;

    /**
    * Clears the PSO cache forcing all PSOs to be re-built
    *
    * Only call this when re-loading shaders
    */
    inline void InvalidatePSOCache() { m_PsoCache.clear(); }

    // Get a pointer to the core graphics command-list, which runs on the command-queue assigned to Present().
    // Only the allocating thread is allowed to call functions on this command-list (DX12 restriction).
    CDeviceGraphicsCommandListPtr GetCoreGraphicsCommandList() const;

    // Acquire one or more command-lists which are independent of the core command-list
    // Only one thread is allowed to call functions on this command-list (DX12 restriction).
    // The thread that gets the permition is the one calling Begin() on it AFAICS
    CDeviceGraphicsCommandListUPtr AcquireGraphicsCommandList();
    CDeviceComputeCommandListUPtr AcquireComputeCommandList();
    CDeviceCopyCommandListUPtr AcquireCopyCommandList(CDeviceCopyCommandList::ECopyType eType);

    std::vector<CDeviceGraphicsCommandListUPtr> AcquireGraphicsCommandLists(uint32 listCount);
    std::vector<CDeviceComputeCommandListUPtr> AcquireComputeCommandLists(uint32 listCount);
    std::vector<CDeviceCopyCommandListUPtr> AcquireCopyCommandLists(uint32 listCount, CDeviceCopyCommandList::ECopyType eType);

    // Command-list sinks, will automatically submit command-lists in [global] allocation-order
    void ForfeitGraphicsCommandList(CDeviceGraphicsCommandListUPtr pCommandList);
    void ForfeitComputeCommandList(CDeviceComputeCommandListUPtr pCommandList);
    void ForfeitCopyCommandList(CDeviceCopyCommandListUPtr pCommandList);

    void ForfeitGraphicsCommandLists(std::vector<CDeviceGraphicsCommandListUPtr> pCommandLists);
    void ForfeitComputeCommandLists(std::vector<CDeviceComputeCommandListUPtr> pCommandLists);
    void ForfeitCopyCommandLists(std::vector<CDeviceCopyCommandListUPtr> pCommandLists);

private:
    CDeviceGraphicsPSOUPtr CreateGraphicsPSOImpl(const CDeviceGraphicsPSODesc& psoDesc) const;

    CDeviceGraphicsCommandListPtr m_pCoreCommandList;

    std::unordered_map<CDeviceGraphicsPSODesc, CDeviceGraphicsPSOUPtr> m_PsoCache;
};

struct SDeviceObjectHelpers
{
    struct SShaderInstanceInfo
    {
        SShaderInstanceInfo()
            : pHwShader(NULL)
            , pHwShaderInstance(NULL)
            , pDeviceShader(NULL) {}

        CHWShader_D3D* pHwShader;
        CCryNameTSCRC technique; // temp
        void* pHwShaderInstance;
        void* pDeviceShader;
    };

    struct SConstantBufferBindInfo
    {
        EConstantBufferShaderSlot shaderSlot;
        int vectorCount;
        EHWShaderClass shaderClass;
        _smart_ptr<AzRHI::ConstantBuffer> pBuffer;
        _smart_ptr<AzRHI::ConstantBuffer> pPreviousBuffer;
        SShaderInstanceInfo shaderInfo;
    };

    // Get shader instances for each shader stage
    static bool GetShaderInstanceInfo(std::array<SShaderInstanceInfo, eHWSC_Num>& shaderInstanceInfos, CShader * pShader, const CCryNameTSCRC&technique, uint64 rtFlags, uint32 mdFlags, uint32 mdvFlags, UPipelineState pipelineState[eHWSC_Num], bool bAllowTesselation);

    // get constant buffers required by shader. NOTE: only CB_PER_BATCH, CB_PER_INSTANCE and CB_PER_FRAME supported currently
    static bool GetConstantBuffersFromShader(std::vector<SConstantBufferBindInfo>& constantBufferInfos, CShader* pShader, const CCryNameTSCRC& technique, uint64 rtFlags, uint32 mdFlags, uint32 mdvFlags);

    // set up constant buffers and fill via reflection NOTE: only per batch, per instance, per frame and per camera supported
    static void BeginUpdateConstantBuffers(std::vector<SConstantBufferBindInfo>& constantBuffers);
    static void EndUpdateConstantBuffers(std::vector<SConstantBufferBindInfo>& constantBuffers);
};
#else
typedef struct D3D11_VIEWPORT
{
    FLOAT TopLeftX;
    FLOAT TopLeftY;
    FLOAT Width;
    FLOAT Height;
    FLOAT MinDepth;
    FLOAT MaxDepth;
}   D3D11_VIEWPORT;
typedef AZStd::shared_ptr<uint32> CDeviceResourceSetPtr;
typedef void*                 CDeviceGraphicsPSOPtr;
typedef AZStd::unique_ptr<uint32> CDeviceGraphicsPSOUPtr;
typedef AZStd::shared_ptr<uint32>  CDeviceComputePSOPtr;
typedef uint32& CDeviceGraphicsCommandListRef;
typedef uint32& CDeviceComputeCommandListRef;
typedef uint32& CDeviceCopyCommandListRef;
#endif
