// Copyright(c) 2019 Advanced Micro Devices, Inc.All rights reserved.
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#ifndef DX12_ENGINE_INTERFACE_IMPL
#define DX12_ENGINE_INTERFACE_IMPL

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers

// Windows Header Files:
#include <windows.h>
#include <windowsx.h>
// C RunTime Header Files
#include <malloc.h>
#include <map>
#include <vector>
#include <mutex>
#include <fstream>
#include <cassert>

#include "Base/Device.h"
#include "Base/SwapChain.h"
#include "Base/Texture.h"
#include "Base/StaticBufferPool.h"
#include "Base/DynamicBufferRing.h"
#include "Base/CommandListRing.h"
#include "Base/GPUTimestamps.h"
#include "Base/Imgui.h"
#include "Base/UserMarkers.h"

#include "PostProc/Tonemapping.h"

#include "EngineInterface.h"
#include "TressFXCommon.h"

#include <wrl.h>
#include <d3d12.h>

// Enable to have access to debug UAV
//#define TRESSFX_DEBUG_UAV 1
#define USE_VID_MEM true

typedef DXGI_FORMAT EI_ResourceFormat;

typedef CAULDRON_DX12::CBV_SRV_UAV EI_UAV;
typedef CAULDRON_DX12::CBV_SRV_UAV EI_SRV;
typedef CAULDRON_DX12::RTV EI_RTV;
typedef CAULDRON_DX12::DSV EI_DSV;
typedef CAULDRON_DX12::RTV EI_RTV;

struct EI_BindEntry
{
    int Binding = 0;
    EI_ResourceTypeEnum ResourceType = EI_ResourceTypeEnum::EI_RESOURCETYPE_UNDEFINED;
    EI_Resource* pResource;
};

class EI_BindSet
{
public:
    ~EI_BindSet();
    CAULDRON_DX12::ResourceView  descriptorTable;
};

class EI_CommandContext
{
public:
    void SubmitBarrier(int numBarriers, EI_Barrier* barriers);
    void BindPSO(EI_PSO* pso);
    void BindSets(EI_PSO* pso, int numBindSets, EI_BindSet** bindSets);
    void Dispatch(int numGroups);
    void UpdateBuffer(EI_Resource* res, void* data);
    void ClearUint32Image(EI_Resource* res, uint32_t value);
    void DrawIndexedInstanced(EI_PSO& pso, EI_IndexedDrawParams& drawParams);
    void DrawInstanced(EI_PSO& pso, EI_DrawParams& drawParams);
    void PushConstants(EI_PSO* pso, int size, void* data);
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandBuffer = nullptr;
};

namespace CAULDRON_DX12
{
    class GLTFTexturesAndBuffers;
    class GltfPbrPass;
    class GltfDepthPass;
}

typedef CAULDRON_DX12::GLTFTexturesAndBuffers EI_GLTFTexturesAndBuffers;
typedef CAULDRON_DX12::GltfPbrPass EI_GltfPbrPass;
typedef CAULDRON_DX12::GltfDepthPass EI_GltfDepthPass;

struct EI_RenderTargetSet;
class GLTFCommon;

class EI_Marker
{
public:
    EI_Marker(EI_CommandContext& ctx, const char * string) : marker(ctx.commandBuffer.Get(), string) {}
private:
    CAULDRON_DX12::UserMarker marker;
};

class EI_Device
{
public:
    EI_Device();
    ~EI_Device();
    // interface
    EI_CommandContext& GetCurrentCommandContext() { return m_currentCommandBuffer; }
    std::unique_ptr<EI_Resource>    CreateBufferResource(const int structSize, const int structCount, const unsigned int flags, const char* name);
    std::unique_ptr<EI_Resource>    CreateUint32Resource(const int width, const int height, const int arraySize, const char* name, uint32_t ClearValue = 0);
#ifdef TRESSFX_DEBUG_UAV
    std::unique_ptr<EI_Resource>    CreateDebugUAVResource(const int width, const int height, const size_t channels, const int arraySize, const char* name, float ClearValue = 0.f);
#endif // TRESSFX_DEBUG_UAV
    std::unique_ptr<EI_Resource>    CreateRenderTargetResource(const int width, const int height, const size_t channels, const size_t channelSize, const char* name, AMD::float4* ClearValues = nullptr);
    std::unique_ptr<EI_Resource>    CreateDepthResource(const int width, const int height, const char* name);
    std::unique_ptr<EI_Resource>    CreateResourceFromFile(const char* szFilename, bool useSRGB = false);
    std::unique_ptr<EI_Resource>    CreateSampler(EI_Filter MinFilter, EI_Filter MaxFilter, EI_Filter MipFilter, EI_AddressMode AddressMode);

    std::unique_ptr<EI_BindLayout>  CreateLayout(const EI_LayoutDescription& description);

    std::unique_ptr <EI_BindSet>    CreateBindSet(EI_BindLayout* layout, EI_BindSetDescription& bindSet);

    std::unique_ptr<EI_RenderTargetSet>   CreateRenderTargetSet(const EI_ResourceFormat* pResourceFormats, const uint32_t numResources, const EI_AttachmentParams* AttachmentParams, float* clearValues);
    std::unique_ptr<EI_RenderTargetSet>   CreateRenderTargetSet(const EI_Resource** pResourcesArray, const uint32_t numResources, const EI_AttachmentParams* AttachmentParams, float* clearValues);

    std::unique_ptr<EI_GLTFTexturesAndBuffers> CreateGLTFTexturesAndBuffers(GLTFCommon* pGLTFCommon);
    std::unique_ptr<EI_GltfPbrPass> CreateGLTFPbrPass(EI_GLTFTexturesAndBuffers* pGLTFTexturesAndBuffers, EI_RenderTargetSet* renderTargetSet);
    std::unique_ptr<EI_GltfDepthPass> CreateGLTFDepthPass(EI_GLTFTexturesAndBuffers* pGLTFTexturesAndBuffers, EI_RenderTargetSet* renderTargetSet);

    void			BeginRenderPass(EI_CommandContext& commandContext, const EI_RenderTargetSet* pRenderPassSet, const wchar_t* pPassName, uint32_t width = 0, uint32_t height = 0);
    void			EndRenderPass(EI_CommandContext& commandContext);
    void            SetViewportAndScissor(EI_CommandContext& commandContext, uint32_t topX, uint32_t topY, uint32_t width, uint32_t height);

    std::unique_ptr<EI_PSO> CreateComputeShaderPSO(const char* shaderName, const char* entryPoint, EI_BindLayout** layouts, int numLayouts);
    std::unique_ptr<EI_PSO> CreateGraphicsPSO(const char* vertexShaderName, const char* vertexEntryPoint, const char* fragmentShaderName, const char* fragmentEntryPoint, EI_PSOParams& psoParams);

    /* async compute */
    EI_CommandContext& GetComputeCommandContext() { return m_currentComputeCommandBuffer; }
    void WaitForCompute();
    void SignalComputeStart();
    void WaitForLastFrameGraphics();
    void SubmitComputeCommandList();
    /* /async compute */

    // internals
    void            OnCreate(HWND hWnd, uint32_t numBackBuffers, bool enableValidation, const char* appName);
    void            OnResize(uint32_t width, uint32_t height);
    void            FlushGPU() { m_device.GPUFlush(); }
    void            OnDestroy();
    void            OnBeginFrame(bool bDoAsync);
    void            OnEndFrame();
    void            BeginNewCommandBuffer();
    void            BeginNewComputeCommandBuffer();
    void            EndAndSubmitCommandBuffer();
    void            BeginBackbufferRenderPass();
    void			EndRenderPass();
    void            RenderUI();

    void            SetVSync(bool vSync) { m_vSync = vSync; }

    CAULDRON_DX12::Device* GetCauldronDevice() { return &m_device; }

    CAULDRON_DX12::UploadHeap* GetUploadHeap() { return &m_uploadHeap; }
    CAULDRON_DX12::StaticBufferPool* GetVidMemBufferPool() { return &m_vidMemBufferPool; }
    CAULDRON_DX12::DynamicBufferRing* GetConstantBufferRing() { return &m_constantBufferRing; }

    // Find a better place to put this ...
    EI_Resource* GetDepthBufferResource() { return m_depthBuffer.get(); }
    EI_ResourceFormat   GetDepthBufferFormat() { return DXGI_FORMAT_D32_FLOAT; }
    EI_Resource* GetColorBufferResource() { return m_colorBuffer.get(); }
    EI_ResourceFormat   GetColorBufferFormat() { return DXGI_FORMAT_R8G8B8A8_UNORM; }
    EI_Resource* GetShadowBufferResource() { return m_shadowBuffer.get(); }
    EI_ResourceFormat   GetShadowBufferFormat() { return GetDepthBufferFormat(); }
    EI_Resource* GetDefaultWhiteTexture() { return m_DefaultWhiteTexture.get(); }
    EI_BindSet* GetSamplerBindSet() { return m_pSamplerBindSet.get(); }

#ifdef TRESSFX_DEBUG_UAV
    EI_Resource* GetDebugUAVResource() { return m_pDebugUAV.get(); }
#endif // TRESSFX_DEBUG_UAV

    // for the client code to set timestamps
    void        GetTimeStamp(char* name);
    int         GetNumTimeStamps() { return (int)m_sortedTimeStamps.size(); }
    const char* GetTimeStampName(const int i) { return m_sortedTimeStamps[i].m_label.c_str(); }
    int         GetTimeStampValue(const int i) { return (int)m_sortedTimeStamps[i].m_microseconds; }
    void        DrawFullScreenQuad(EI_CommandContext& commandContext, EI_PSO& pso, EI_BindSet** bindSets, uint32_t numBindSets);
    float       GetAverageGpuTime() const { return m_averageGpuTime; }

    // only to call by implementation internals
    ID3D12Device*    GetDX12Device() { return m_device.GetDevice(); }
    CAULDRON_DX12::ResourceViewHeaps& GetResourceViewHeaps() { return m_resourceViewHeaps; }

    void        AllocateCPUVisibleView(CAULDRON_DX12::ResourceView* pResourceView);
private:
    void            EndAndSubmitCommandBufferWithFence();
    CAULDRON_DX12::Device		m_device;
    CAULDRON_DX12::SwapChain	m_swapChain;
    int m_currentImageIndex;

    // We need to be able to get access to the depth buffer from within the demo
    // so store as an agnostic resource. We will also store a color target for all our sample's works
    std::unique_ptr<EI_Resource> m_depthBuffer;
    std::unique_ptr<EI_Resource> m_colorBuffer;
    std::unique_ptr<EI_Resource> m_shadowBuffer;

#ifdef TRESSFX_DEBUG_UAV
    std::unique_ptr<EI_Resource> m_pDebugUAV;	// Debug UAV
#endif // TRESSFX_DEBUG_UAV

    // Default resource to use when a resource is missing
    std::unique_ptr<EI_Resource> m_DefaultWhiteTexture;

    std::unique_ptr<EI_BindLayout>  m_pEndFrameResolveBindLayout = nullptr;
    std::unique_ptr<EI_BindSet>     m_pEndFrameResolveBindSet = nullptr;
    std::unique_ptr<EI_BindSet>     m_pSamplerBindSet = nullptr;
    std::unique_ptr<EI_PSO>         m_pEndFrameResolvePSO = nullptr;
    std::unique_ptr<EI_Resource>    m_pFullscreenIndexBuffer;

    int m_width;
    int m_height;
    bool m_vSync = false;

    bool                            m_recording = false;

    CAULDRON_DX12::ToneMapping        m_toneMapping;

    // vulkan specific imgui stuff
    CAULDRON_DX12::ImGUI			  m_ImGUI;

    // resource allocators
    CAULDRON_DX12::ResourceViewHeaps  m_resourceViewHeaps;
    CAULDRON_DX12::UploadHeap         m_uploadHeap;
    CAULDRON_DX12::StaticBufferPool   m_vidMemBufferPool;
    CAULDRON_DX12::StaticBufferPool   m_sysMemBufferPool;
    CAULDRON_DX12::DynamicBufferRing  m_constantBufferRing; // "dynamic" uniform buffers
    CAULDRON_DX12::CommandListRing    m_commandListRing;

    CAULDRON_DX12::GPUTimestamps          m_gpuTimer;
    std::vector<CAULDRON_DX12::TimeStamp> m_timeStamps;
    std::vector<CAULDRON_DX12::TimeStamp> m_sortedTimeStamps;
    float                                 m_averageGpuTime;

    EI_CommandContext m_currentCommandBuffer;

    // async compute
    CAULDRON_DX12::CommandListRing  m_computeCommandListRing;
    EI_CommandContext               m_currentComputeCommandBuffer;

    CAULDRON_DX12::Fence            m_computeDoneFence;
    CAULDRON_DX12::Fence            m_lastFrameGraphicsCommandBufferFence;
    std::unique_ptr<EI_Resource>    m_LinearWrapSampler;

    // Need to manage discreet heaps ourselves
    uint32_t                                        m_DescriptorSize = 0;
    Microsoft::WRL::ComPtr< ID3D12DescriptorHeap>   m_CPUDescriptorHeap = nullptr;
    uint32_t                                        m_CPUDescriptorIndex = 0;    
};

struct DX12Resource;

class EI_Resource
{
public:
    // need this for automatic destruction
    EI_Resource();
    ~EI_Resource();

    int GetHeight() const { return m_ResourceType == EI_ResourceType::Texture ? m_pTexture->GetHeight() : 0; }
    int GetWidth() const { return m_ResourceType == EI_ResourceType::Texture ? m_pTexture->GetWidth() : 0; }

    EI_ResourceType m_ResourceType = EI_ResourceType::Undefined;

    union
    {
        DX12Resource* m_pBuffer;
        CAULDRON_DX12::Texture* m_pTexture;
        CAULDRON_DX12::SAMPLER* m_pSampler;
    };

    // Needed since we will allocate samplers directly in the table range
    D3D12_SAMPLER_DESC  samplerDesc = {};

    EI_RTV* RTView = nullptr;
    EI_DSV* DSView = nullptr;
    EI_SRV* SRView = nullptr;
private:
};

struct EI_BindLayout
{
    ~EI_BindLayout();

    EI_LayoutDescription description;
    std::vector<CD3DX12_DESCRIPTOR_RANGE> layoutBindings;
};

const static int MaxRenderAttachments = 5;
struct EI_RenderTargetSet
{
    ~EI_RenderTargetSet();
    void SetResources(const EI_Resource** pResourcesArray);

    const EI_Resource*  m_RenderResources[MaxRenderAttachments] = { nullptr };
    EI_ResourceFormat   m_RenderResourceFormats[MaxRenderAttachments];  // Needed for PS0 creation when we don't have resources yet (i.e gltf)
    D3D12_CLEAR_VALUE 	m_ClearValues[MaxRenderAttachments];
    bool                m_ClearColor[MaxRenderAttachments] = { false };
    uint32_t		    m_NumResources = 0;
    bool                m_HasDepth = false;
    bool                m_ClearDepth = false;
};

class EI_PSO
{
public:
    ~EI_PSO();

    Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_pipeline;
    Microsoft::WRL::ComPtr <ID3D12RootSignature>    m_pipelineLayout;
    D3D12_PRIMITIVE_TOPOLOGY    m_primitiveTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    EI_BindPoint                m_bp;
};

EI_Device* GetDevice();

#endif