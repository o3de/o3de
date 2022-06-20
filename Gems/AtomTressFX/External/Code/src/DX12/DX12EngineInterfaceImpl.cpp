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

#include "EngineInterface.h"

#include <DirectXMath.h>
#include <pix_win.h>
using namespace DirectX;
#include "d3dcompiler.h"

#include <cassert>
#include "Base/ShaderCompilerHelper.h"
#include "Base/Helper.h"
#include "GLTF/GltfHelpers.h"
#include "GLTF/GltfCommon.h"
#include "GLTF/GLTFTexturesAndBuffers.h"
#include "GLTF/GltfPbrPass.h"
#include "GLTF/GltfDepthPass.h"
#include "Misc/Error.h"
#include "TressFXLayouts.h"

// Inline operators for general state data
inline D3D12_COMPARISON_FUNC operator* (EI_CompareFunc Enum)
{
    if (Enum == EI_CompareFunc::Never) return D3D12_COMPARISON_FUNC_NEVER;
    if (Enum == EI_CompareFunc::Less) return D3D12_COMPARISON_FUNC_LESS;
    if (Enum == EI_CompareFunc::Equal) return D3D12_COMPARISON_FUNC_EQUAL;
    if (Enum == EI_CompareFunc::LessEqual) return D3D12_COMPARISON_FUNC_LESS_EQUAL;
    if (Enum == EI_CompareFunc::Greater) return D3D12_COMPARISON_FUNC_GREATER;
    if (Enum == EI_CompareFunc::NotEqual) return D3D12_COMPARISON_FUNC_NOT_EQUAL;
    if (Enum == EI_CompareFunc::GreaterEqual) return D3D12_COMPARISON_FUNC_GREATER_EQUAL;
    if (Enum == EI_CompareFunc::Always) return D3D12_COMPARISON_FUNC_ALWAYS;
    assert(false && "Using an EI_CompareFunc that has not been mapped to DX12 yet!");
    return D3D12_COMPARISON_FUNC_NEVER;
}

inline D3D12_BLEND_OP operator* (EI_BlendOp Enum)
{
    if (Enum == EI_BlendOp::Add) return D3D12_BLEND_OP_ADD;
    if (Enum == EI_BlendOp::Subtract) return D3D12_BLEND_OP_SUBTRACT;
    if (Enum == EI_BlendOp::ReverseSubtract) return D3D12_BLEND_OP_REV_SUBTRACT;
    if (Enum == EI_BlendOp::Min) return D3D12_BLEND_OP_MIN;
    if (Enum == EI_BlendOp::Max) return D3D12_BLEND_OP_MAX;
    assert(false && "Using an EI_BlendOp that has not been mapped to DX12 yet!");
    return D3D12_BLEND_OP_ADD;
};

inline D3D12_STENCIL_OP operator* (EI_StencilOp Enum)
{
    if (Enum == EI_StencilOp::Keep) return D3D12_STENCIL_OP_KEEP;
    if (Enum == EI_StencilOp::Zero) return D3D12_STENCIL_OP_ZERO;
    if (Enum == EI_StencilOp::Replace) return D3D12_STENCIL_OP_REPLACE;
    if (Enum == EI_StencilOp::IncrementClamp) return D3D12_STENCIL_OP_INCR_SAT;
    if (Enum == EI_StencilOp::DecrementClamp) return D3D12_STENCIL_OP_DECR_SAT;
    if (Enum == EI_StencilOp::Invert) return D3D12_STENCIL_OP_INVERT;
    if (Enum == EI_StencilOp::IncrementWrap) return D3D12_STENCIL_OP_INCR;
    if (Enum == EI_StencilOp::DecrementWrap) return D3D12_STENCIL_OP_DECR;
    assert(false && "Using an EI_StencilOp that has not been mapped to DX12 yet!");
    return D3D12_STENCIL_OP_KEEP;
}

inline D3D12_BLEND operator* (EI_BlendFactor Enum)
{
    if (Enum == EI_BlendFactor::Zero) return D3D12_BLEND_ZERO;
    if (Enum == EI_BlendFactor::One) return D3D12_BLEND_ONE;
    if (Enum == EI_BlendFactor::SrcColor) return D3D12_BLEND_SRC_COLOR;
    if (Enum == EI_BlendFactor::InvSrcColor) return D3D12_BLEND_INV_SRC_COLOR;
    if (Enum == EI_BlendFactor::DstColor) return D3D12_BLEND_DEST_COLOR;
    if (Enum == EI_BlendFactor::InvDstColor) return D3D12_BLEND_INV_DEST_COLOR;
    if (Enum == EI_BlendFactor::SrcAlpha) return D3D12_BLEND_SRC_ALPHA;
    if (Enum == EI_BlendFactor::InvSrcAlpha) return D3D12_BLEND_INV_SRC_ALPHA;
    if (Enum == EI_BlendFactor::DstAlpha) return D3D12_BLEND_DEST_ALPHA;
    if (Enum == EI_BlendFactor::InvDstAlpha) return D3D12_BLEND_INV_DEST_ALPHA;
    assert(false && "Using an EI_BlendFactor that has not been mapped to DX12 yet!");
    return D3D12_BLEND_ZERO;
};

inline D3D12_PRIMITIVE_TOPOLOGY operator* (EI_Topology Enum)
{
    if (Enum == EI_Topology::TriangleList) return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    if (Enum == EI_Topology::TriangleStrip) return D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
    assert(false && "Using an EI_Topology that has not been mapped to DX12 yet!");
    return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
}

EI_Device::EI_Device() :
    m_currentImageIndex(0),
    m_pFullscreenIndexBuffer()
{
}

EI_Device::~EI_Device()
{
}

void EI_Device::AllocateCPUVisibleView(CAULDRON_DX12::ResourceView* pResourceView)
{
    // Check that both heaps can be allocated into in general (worst case, we allocate 2 descriptors on a heap)
    if (m_CPUDescriptorIndex + 1 >= 256)
    {
        assert(false && "AllocateResourceView: heap ran of memory, increase its size");
    }

    D3D12_CPU_DESCRIPTOR_HANDLE CPUView = m_CPUDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
    CPUView.ptr += m_CPUDescriptorIndex * m_DescriptorSize;

    D3D12_GPU_DESCRIPTOR_HANDLE GPUView = m_CPUDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
    GPUView.ptr += m_CPUDescriptorIndex * m_DescriptorSize;

    m_CPUDescriptorIndex++;

    // Override the resource view internal pointers
    // This is a bit of a hack, but Cauldron doesn't currently support the ability to allocate descriptor handles from different heaps.
    pResourceView->SetResourceView(1, m_DescriptorSize, CPUView, GPUView);
}

struct DX12Resource
{
    DX12Resource(CAULDRON_DX12::Device* pDevice) : pDevice(pDevice) {}

    void CreateTex2D(DXGI_FORMAT Format, const int width, const int height, const int depthOrArray, const unsigned int flags, const char* name)
    {
        CD3DX12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Tex2D(Format, width, height, depthOrArray, 1);
        CreateResource(desc, flags, name);
    }
    void CreateBuffer(const int structSize, const int structCount, const unsigned int flags, const char* name)
    {
        m_structSize = structSize;
        m_structCount = structCount;
        m_totalMemSize = m_structSize * m_structCount;

        if (flags & EI_BF_UNIFORMBUFFER)
        {
            //size of DX12 constant buffers must be multiple of 256
            if (m_totalMemSize % 256 != 0)
                m_totalMemSize += 256 - m_totalMemSize % 256;
        }

        CD3DX12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Buffer(m_totalMemSize);
        CreateResource(desc, flags, name);
    }

    void CreateResource(CD3DX12_RESOURCE_DESC desc, const unsigned int flags, const char* name)
    {
        D3D12_RESOURCE_FLAGS resourceFlags = D3D12_RESOURCE_FLAG_NONE;
        D3D12_RESOURCE_STATES initialState = (D3D12_RESOURCE_STATES)D3D12_RESOURCE_STATE_COPY_DEST;

        
        if (flags & EI_BF_NEEDSUAV)
        {
            resourceFlags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
            initialState = (D3D12_RESOURCE_STATES)D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
        }

        desc.Flags = resourceFlags;
        m_ResourceDesc = desc;

        wchar_t  uniName[1024];
        swprintf(uniName, 1024, L"%S", name);

        if (flags & EI_BF_NEEDSCPUMEMORY) {
            CD3DX12_RESOURCE_DESC cpuDesc = desc;
            cpuDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
            ThrowIfFailed(pDevice->GetDevice()->CreateCommittedResource(
                &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
                D3D12_HEAP_FLAG_NONE,
                &cpuDesc,
                D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr,
                IID_PPV_ARGS(&cpuBuffer)));

            cpuBuffer->SetName(uniName);
        }

        ThrowIfFailed(pDevice->GetDevice()->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
            D3D12_HEAP_FLAG_NONE,
            &desc,
            initialState,
            nullptr,
            IID_PPV_ARGS(&gpuBuffer)));

        gpuBuffer->SetName(uniName);

        if (flags & EI_BF_INDEXBUFFER)
        {
            indexBufferView.BufferLocation = gpuBuffer->GetGPUVirtualAddress();
            indexBufferView.Format = DXGI_FORMAT_R32_UINT;
            indexBufferView.SizeInBytes = m_totalMemSize;
        }
    }

    void FreeCPUMemory()
    {
        if (cpuBuffer)
            cpuBuffer->Release();
    }

    void Free() {
        FreeCPUMemory();
        if (gpuBuffer)
            gpuBuffer->Release();
    }

    void CreateCBV(uint32_t index, CAULDRON_DX12::CBV_SRV_UAV* pRV)
    {
        // Describe and create a constant buffer view (CBV).
        D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
        cbvDesc.BufferLocation = gpuBuffer->GetGPUVirtualAddress();
        cbvDesc.SizeInBytes = m_totalMemSize;
        pDevice->GetDevice()->CreateConstantBufferView(&cbvDesc, pRV->GetCPU(index));
    }

    void CreateSRV(uint32_t index, CAULDRON_DX12::ResourceView* pRV)
    {
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = m_ResourceDesc.Format;
        srvDesc.ViewDimension = (m_ResourceDesc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER) ?
            D3D12_SRV_DIMENSION_BUFFER :
            (m_ResourceDesc.DepthOrArraySize > 1) ? D3D12_SRV_DIMENSION_TEXTURE2DARRAY : D3D12_SRV_DIMENSION_TEXTURE2D;

        if (srvDesc.ViewDimension == D3D12_UAV_DIMENSION_BUFFER)
        {
            srvDesc.Buffer.FirstElement = 0;
            srvDesc.Buffer.NumElements = m_structCount;
            srvDesc.Buffer.StructureByteStride = m_structSize;
            srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
            srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        }
        else
        {
            if (m_ResourceDesc.DepthOrArraySize > 1)
            {
                srvDesc.Texture2DArray.ArraySize = m_ResourceDesc.DepthOrArraySize;
                srvDesc.Texture2DArray.FirstArraySlice = 0;
                srvDesc.Texture2DArray.MipLevels = m_ResourceDesc.MipLevels;
                srvDesc.Texture2DArray.MostDetailedMip = 0;
            }
            else
            {
                srvDesc.Texture2D.MipLevels = m_ResourceDesc.MipLevels;
                srvDesc.Texture2D.MostDetailedMip = 0;
            }
        }
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

        // For SRV, we can use the existing mapping that was made from Cauldron
        GetDevice()->GetDX12Device()->CreateShaderResourceView(gpuBuffer, &srvDesc, pRV->GetCPU(index));
    }

    void CreateUAV(uint32_t index, CAULDRON_DX12::ResourceView* pRV)
    {
        // Allocate CPU/GPU handles for the UAV
        m_pUnorderedAccessView = std::unique_ptr<CAULDRON_DX12::ResourceView>(new CAULDRON_DX12::ResourceView);
        GetDevice()->AllocateCPUVisibleView(m_pUnorderedAccessView.get());

        D3D12_UNORDERED_ACCESS_VIEW_DESC UAViewDesc = {};
        UAViewDesc.Format = m_ResourceDesc.Format;
        UAViewDesc.ViewDimension = (m_ResourceDesc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER) ?
            D3D12_UAV_DIMENSION_BUFFER :
            (m_ResourceDesc.DepthOrArraySize > 1) ? D3D12_UAV_DIMENSION_TEXTURE2DARRAY : D3D12_UAV_DIMENSION_TEXTURE2D;

        if (UAViewDesc.ViewDimension == D3D12_UAV_DIMENSION_BUFFER)
        {
            UAViewDesc.Buffer.FirstElement = 0;
            UAViewDesc.Buffer.NumElements = m_structCount;
            UAViewDesc.Buffer.StructureByteStride = m_structSize;
            UAViewDesc.Buffer.CounterOffsetInBytes = 0;
            UAViewDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;
        }
        else
        {
            if (m_ResourceDesc.DepthOrArraySize > 1)
            {
                UAViewDesc.Texture2DArray.ArraySize = m_ResourceDesc.DepthOrArraySize;
                UAViewDesc.Texture2DArray.FirstArraySlice = 0;
                UAViewDesc.Texture2DArray.MipSlice = 0;
            }
            else
            {
                UAViewDesc.Texture2D.MipSlice = 0;
            }
        }

        // For UAVs, we need one we can use to clear with that is CPU read/write, and one that will map back to what Cauldron expects (CPU write only)
        pDevice->GetDevice()->CreateUnorderedAccessView(gpuBuffer, nullptr, &UAViewDesc, m_pUnorderedAccessView->GetCPU());
        pDevice->GetDevice()->CreateUnorderedAccessView(gpuBuffer, nullptr, &UAViewDesc, pRV->GetCPU(index));
    }

    int m_totalMemSize = 0;
    int m_structCount = 0;
    int m_structSize = 0;
    CAULDRON_DX12::Device* pDevice = nullptr;
    ID3D12Resource* cpuBuffer = nullptr;
    ID3D12Resource* gpuBuffer = nullptr;
    D3D12_INDEX_BUFFER_VIEW indexBufferView;

    CD3DX12_RESOURCE_DESC   m_ResourceDesc;
    std::unique_ptr<CAULDRON_DX12::ResourceView> m_pUnorderedAccessView;
};

void EI_Device::DrawFullScreenQuad(EI_CommandContext& commandContext, EI_PSO& pso, EI_BindSet** bindSets, uint32_t numBindSets)
{
    // Set everything
    commandContext.BindSets(&pso, numBindSets, bindSets);

    EI_IndexedDrawParams drawParams;
    drawParams.pIndexBuffer = m_pFullscreenIndexBuffer.get();
    drawParams.numIndices = 4;
    drawParams.numInstances = 1;

    commandContext.DrawIndexedInstanced(pso, drawParams);
}

std::unique_ptr<EI_Resource> EI_Device::CreateBufferResource(int structSize, const int structCount, const unsigned int flags, const char* name)
{
    EI_Resource* res = new EI_Resource;
    res->m_ResourceType = EI_ResourceType::Buffer;
    res->m_pBuffer = new DX12Resource(&m_device);
    res->m_pBuffer->CreateBuffer(structSize, structCount, flags | EI_BF_NEEDSCPUMEMORY, name);

    return std::unique_ptr<EI_Resource>(res);
}

std::unique_ptr<EI_Resource> EI_Device::CreateUint32Resource(const int width, const int height, const int arraySize, const char* name, uint32_t ClearValue /*= 0*/)
{
    EI_Resource* res = new EI_Resource;
    res->m_ResourceType = EI_ResourceType::Buffer;
    res->m_pBuffer = new DX12Resource(&m_device);

    res->m_pBuffer->CreateTex2D(DXGI_FORMAT_R32_UINT, width, height, arraySize, EI_BF_NEEDSUAV, name);

    return std::unique_ptr<EI_Resource>(res);
}

#ifdef TRESSFX_DEBUG_UAV
std::unique_ptr<EI_Resource> EI_Device::CreateDebugUAVResource(const int width, const int height, const size_t channels, const int arraySize, const char* name, float ClearValue /*= 0.f*/)
{
    EI_Resource* res = new EI_Resource;
    res->m_ResourceType = EI_ResourceType::Buffer;
    res->m_pBuffer = new DX12Resource(&m_device);

    DXGI_FORMAT format = DXGI_FORMAT_R32_UINT;
    switch (channels)
    {
    case 4:
        format = DXGI_FORMAT_R32G32B32A32_FLOAT;
        break;

    default:
        // Unsupported ... add whatever you need
        assert(false);
        break;
    }

    res->m_pBuffer->CreateTex2D(format, width, height, arraySize, EI_BF_NEEDSUAV, name);

    return std::unique_ptr<EI_Resource>(res);
}
#endif // TRESSFX_DEBUG_UAV

std::unique_ptr<EI_Resource> EI_Device::CreateRenderTargetResource(const int width, const int height, const size_t channels, const size_t channelSize, const char* name, AMD::float4* ClearValues /*= nullptr*/)
{
    EI_Resource* res = new EI_Resource;

    res->m_ResourceType = EI_ResourceType::Texture;
    res->m_pTexture = new CAULDRON_DX12::Texture();

    CD3DX12_RESOURCE_DESC resourceDesc = {};
    resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    resourceDesc.Alignment = 0;
    resourceDesc.Width = width;
    resourceDesc.Height = height;
    resourceDesc.DepthOrArraySize = 1;
    resourceDesc.MipLevels = 1;
    resourceDesc.SampleDesc.Count = 1;
    resourceDesc.SampleDesc.Quality = 0;
    resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;

    if (channels == 1)
    {
        resourceDesc.Format = DXGI_FORMAT_R16_FLOAT;
    }
    else if (channels == 2)
    {
        resourceDesc.Format = DXGI_FORMAT_R16G16_FLOAT;
    }
    else if (channels == 4)
    {
        if (channelSize == 1)
            resourceDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        else
            resourceDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
    }

    if (ClearValues)
    {
        D3D12_CLEAR_VALUE ClearParams;
        ClearParams.Format = resourceDesc.Format;
        ClearParams.Color[0] = ClearValues->x;
        ClearParams.Color[1] = ClearValues->y;
        ClearParams.Color[2] = ClearValues->z;
        ClearParams.Color[3] = ClearValues->w;

        // Makes initial barriers easier to deal with
        res->m_pTexture->Init(&m_device, name, &resourceDesc, D3D12_RESOURCE_STATE_COMMON, &ClearParams);
    }
    else
        res->m_pTexture->InitRenderTarget(&m_device, name, &resourceDesc, D3D12_RESOURCE_STATE_RENDER_TARGET);

    res->RTView = new CAULDRON_DX12::RTV();
    m_resourceViewHeaps.AllocRTVDescriptor(1, res->RTView);
    res->m_pTexture->CreateRTV(0, res->RTView);

    res->SRView = new CAULDRON_DX12::CBV_SRV_UAV();
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = resourceDesc.Format;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.TextureCube.MostDetailedMip = 0;
    srvDesc.TextureCube.ResourceMinLODClamp = 0;
    srvDesc.TextureCube.MipLevels = 1;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    m_resourceViewHeaps.AllocCBV_SRV_UAVDescriptor(1, res->SRView);
    res->m_pTexture->CreateSRV(0, res->SRView, &srvDesc);

    return std::unique_ptr<EI_Resource>(res);
}

std::unique_ptr<EI_Resource> EI_Device::CreateDepthResource(const int width, const int height, const char* name)
{
    EI_Resource* res = new EI_Resource;
    res->m_ResourceType = EI_ResourceType::Texture;
    res->m_pTexture = new CAULDRON_DX12::Texture();

    res->m_pTexture->InitDepthStencil(&m_device, name, &CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R32_TYPELESS, width, height, 1, 1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL));

    res->DSView = new CAULDRON_DX12::DSV();
    m_resourceViewHeaps.AllocDSVDescriptor(1, res->DSView);
    res->m_pTexture->CreateDSV(0, res->DSView);

    res->SRView = new CAULDRON_DX12::CBV_SRV_UAV();
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.TextureCube.MostDetailedMip = 0;
    srvDesc.TextureCube.ResourceMinLODClamp = 0;
    srvDesc.TextureCube.MipLevels = 1;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    m_resourceViewHeaps.AllocCBV_SRV_UAVDescriptor(1, res->SRView);
    res->m_pTexture->CreateSRV(0, res->SRView, &srvDesc);

    return std::unique_ptr<EI_Resource>(res);
}

std::unique_ptr<EI_Resource> EI_Device::CreateResourceFromFile(const char* szFilename, bool useSRGB/*= false*/)
{
    EI_Resource* res = new EI_Resource;
    res->m_ResourceType = EI_ResourceType::Texture;
    res->m_pTexture = new CAULDRON_DX12::Texture();

    res->m_pTexture->InitFromFile(GetDevice()->GetCauldronDevice(), &m_uploadHeap, szFilename, useSRGB);

    m_uploadHeap.FlushAndFinish();

    return std::unique_ptr<EI_Resource>(res);
}

std::unique_ptr<EI_Resource> EI_Device::CreateSampler(EI_Filter MinFilter, EI_Filter MaxFilter, EI_Filter MipFilter, EI_AddressMode AddressMode)
{
    EI_Resource* res = new EI_Resource;
    res->m_ResourceType = EI_ResourceType::Sampler;

    D3D12_FILTER Filter;

    if (MinFilter == EI_Filter::Linear)
    {
        if (MaxFilter == EI_Filter::Linear)
        {
            if (MipFilter == EI_Filter::Linear)
                Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
            else
                Filter = D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT;
        }
        else
        {
            if (MipFilter == EI_Filter::Linear)
                Filter = D3D12_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR;
            else
                Filter = D3D12_FILTER_MIN_LINEAR_MAG_MIP_POINT;
        }
    }
    else
    {
        if (MaxFilter == EI_Filter::Linear)
        {
            if (MipFilter == EI_Filter::Linear)
                Filter = D3D12_FILTER_MIN_POINT_MAG_MIP_LINEAR;
            else
                Filter = D3D12_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT;
        }
        else
        {
            if (MipFilter == EI_Filter::Linear)
                Filter = D3D12_FILTER_MIN_MAG_POINT_MIP_LINEAR;
            else
                Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
        }
    }

    D3D12_SAMPLER_DESC SamplerDesc = {};
    SamplerDesc.Filter = Filter;
    SamplerDesc.AddressU = (AddressMode == EI_AddressMode::Wrap) ? D3D12_TEXTURE_ADDRESS_MODE_WRAP : D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    SamplerDesc.AddressV = (AddressMode == EI_AddressMode::Wrap) ? D3D12_TEXTURE_ADDRESS_MODE_WRAP : D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    SamplerDesc.AddressW = (AddressMode == EI_AddressMode::Wrap) ? D3D12_TEXTURE_ADDRESS_MODE_WRAP : D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    SamplerDesc.MipLODBias = 0;
    SamplerDesc.MaxAnisotropy = 1;
    SamplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
    SamplerDesc.BorderColor[0] = 0.f;
    SamplerDesc.BorderColor[1] = 0.f;
    SamplerDesc.BorderColor[2] = 0.f;
    SamplerDesc.BorderColor[3] = 0.f;
    SamplerDesc.MinLOD = 0.f;
    SamplerDesc.MaxLOD = D3D12_FLOAT32_MAX;

    res->m_pSampler = new CAULDRON_DX12::SAMPLER();
    m_resourceViewHeaps.AllocSamplerDescriptor(1, res->m_pSampler);
        
    m_device.GetDevice()->CreateSampler(&SamplerDesc, res->m_pSampler->GetCPU());
    res->samplerDesc = SamplerDesc;

    return std::unique_ptr<EI_Resource>(res);
}

std::unique_ptr<EI_BindSet> EI_Device::CreateBindSet(EI_BindLayout* layout, EI_BindSetDescription& bindSet)
{
    EI_BindSet* result = new EI_BindSet;
    assert(layout->layoutBindings.size() == bindSet.resources.size());

    UINT numTotalSlotsNeededInDeScriptorTable = 0;
    int numBindings = (int)layout->layoutBindings.size();

    // On DX12, we can't mix samplers with other resource types, so validate that all resources in the bind set are all samplers, or all other
    bool IsSamplerBindSet = (bindSet.resources[0]->m_ResourceType == EI_ResourceType::Sampler);

    for (int i = 0; i < numBindings; i++)
    {
        UINT numSlotsNeededInDeScriptorTable = layout->layoutBindings[i].BaseShaderRegister + layout->layoutBindings[i].NumDescriptors;
        if (numSlotsNeededInDeScriptorTable > numTotalSlotsNeededInDeScriptorTable)
            numTotalSlotsNeededInDeScriptorTable = numSlotsNeededInDeScriptorTable;
        assert(!IsSamplerBindSet || (IsSamplerBindSet && bindSet.resources[i]->m_ResourceType == EI_ResourceType::Sampler) && "Samplers cannot be mixed with other resource types. Please re-organize your layout/bindset");
    }

    if (!IsSamplerBindSet)
        m_resourceViewHeaps.AllocCBV_SRV_UAVDescriptor(numTotalSlotsNeededInDeScriptorTable, static_cast<CAULDRON_DX12::CBV_SRV_UAV*>(&result->descriptorTable));
    else
        m_resourceViewHeaps.AllocSamplerDescriptor(numTotalSlotsNeededInDeScriptorTable, static_cast<CAULDRON_DX12::SAMPLER*>(&result->descriptorTable));

    for (int i = 0; i < numBindings; i++)
    {
        assert(layout->layoutBindings[i].NumDescriptors == 1);
        UINT descriptorIdx = i;
        switch (layout->description.resources[i].type)
        {
        case EI_RESOURCETYPE_BUFFER_RW:
            bindSet.resources[i]->m_pBuffer->CreateUAV(descriptorIdx, static_cast<CAULDRON_DX12::CBV_SRV_UAV*>(&result->descriptorTable));
            break;
        case EI_RESOURCETYPE_BUFFER_RO:
            bindSet.resources[i]->m_pBuffer->CreateSRV(descriptorIdx, static_cast<CAULDRON_DX12::CBV_SRV_UAV*>(&result->descriptorTable));
            break;
        case EI_RESOURCETYPE_IMAGE_RW:
            if (bindSet.resources[i]->m_ResourceType == EI_ResourceType::Buffer)
                bindSet.resources[i]->m_pBuffer->CreateUAV(descriptorIdx, &result->descriptorTable);  // Override the descriptor pointers used with our own which are properly allocated for clearing/writing
            else
                bindSet.resources[i]->m_pTexture->CreateUAV(descriptorIdx, static_cast<CAULDRON_DX12::CBV_SRV_UAV*>(&result->descriptorTable));
            break;
        case EI_RESOURCETYPE_IMAGE_RO:
            if (bindSet.resources[i]->m_ResourceType == EI_ResourceType::Buffer)
                bindSet.resources[i]->m_pBuffer->CreateSRV(descriptorIdx, &result->descriptorTable);  // Override the descriptor pointers used with our own which are properly allocated for clearing/writing
            else
                bindSet.resources[i]->m_pTexture->CreateSRV(descriptorIdx, static_cast<CAULDRON_DX12::CBV_SRV_UAV*>(&result->descriptorTable), 0);
            break;
        case EI_RESOURCETYPE_UNIFORM:
            bindSet.resources[i]->m_pBuffer->CreateCBV(descriptorIdx, static_cast<CAULDRON_DX12::CBV_SRV_UAV*>(&result->descriptorTable));
            break;
        case EI_RESOURCETYPE_SAMPLER:
            m_device.GetDevice()->CreateSampler(&bindSet.resources[i]->samplerDesc, static_cast<CAULDRON_DX12::SAMPLER*>(&result->descriptorTable)->GetCPU());
            break;
        default:
            assert(0);
            break;
        }
    }

    return std::unique_ptr<EI_BindSet>(result);
}

EI_BindSet::~EI_BindSet()
{
    // Should we move the descriptor allocation/deallocation to the bindset?
    //GetDevice()->GetResourceViewHeaps().FreeDescriptor(m_descriptorSet);
}

std::unique_ptr<EI_RenderTargetSet> EI_Device::CreateRenderTargetSet(const EI_ResourceFormat* pResourceFormats, const uint32_t numResources, const EI_AttachmentParams* AttachmentParams, float* clearValues)
{
    assert(numResources < MaxRenderAttachments && "Number of resources exceeds maximum allowable. Please grow MaxRenderAttachments value.");

    // Create the render pass set
    EI_RenderTargetSet* pNewRenderTargetSet = new EI_RenderTargetSet;

    int currentClearValueRef = 0;
    for (uint32_t i = 0; i < numResources; i++)
    {
        // Check size consistency
        assert(!(AttachmentParams[i].Flags & EI_RenderPassFlags::Depth && (i != (numResources - 1))) && "Only the last attachment can be specified as depth target");

        // Setup a clear value if needed
        if (AttachmentParams[i].Flags & EI_RenderPassFlags::Clear)
        {
            if (AttachmentParams[i].Flags & EI_RenderPassFlags::Depth)
            {
                pNewRenderTargetSet->m_ClearValues[i].DepthStencil.Depth = clearValues[currentClearValueRef++];
                pNewRenderTargetSet->m_ClearValues[i].DepthStencil.Stencil = (uint32_t)clearValues[currentClearValueRef++];
                pNewRenderTargetSet->m_ClearValues[i].Format = DXGI_FORMAT_D32_FLOAT;
                pNewRenderTargetSet->m_HasDepth = true;
                pNewRenderTargetSet->m_ClearDepth = true;
            }
            else
            {
                pNewRenderTargetSet->m_ClearValues[i].Color[0] = clearValues[currentClearValueRef++];
                pNewRenderTargetSet->m_ClearValues[i].Color[1] = clearValues[currentClearValueRef++];
                pNewRenderTargetSet->m_ClearValues[i].Color[2] = clearValues[currentClearValueRef++];
                pNewRenderTargetSet->m_ClearValues[i].Color[3] = clearValues[currentClearValueRef++];
                pNewRenderTargetSet->m_ClearValues[i].Format = pResourceFormats[i];
                pNewRenderTargetSet->m_ClearColor[i] = true;
            }
        }
        else if (AttachmentParams[i].Flags & EI_RenderPassFlags::Depth)
            pNewRenderTargetSet->m_HasDepth = true;

        pNewRenderTargetSet->m_RenderResourceFormats[i] = pResourceFormats[i];
    }

    // Tag the number of resources this render pass set is setting/clearing
    pNewRenderTargetSet->m_NumResources = numResources;

    return std::unique_ptr<EI_RenderTargetSet>(pNewRenderTargetSet);
}

std::unique_ptr<EI_RenderTargetSet> EI_Device::CreateRenderTargetSet(const EI_Resource** pResourcesArray, const uint32_t numResources, const EI_AttachmentParams* AttachmentParams, float* clearValues)
{
    std::vector<EI_ResourceFormat> FormatArray(numResources);

    for (uint32_t i = 0; i < numResources; ++i)
    {
        assert(pResourcesArray[i]->m_ResourceType == EI_ResourceType::Texture);
        FormatArray[i] = pResourcesArray[i]->m_pTexture->GetFormat();
        if (FormatArray[i] == DXGI_FORMAT_R32_TYPELESS)
        {
            FormatArray[i] = DXGI_FORMAT_D32_FLOAT;
        }
    }
    std::unique_ptr<EI_RenderTargetSet> result = CreateRenderTargetSet(FormatArray.data(), numResources, AttachmentParams, clearValues);
    result->SetResources(pResourcesArray);
    return result;
}

std::unique_ptr<EI_GLTFTexturesAndBuffers> EI_Device::CreateGLTFTexturesAndBuffers(GLTFCommon* pGLTFCommon)
{
    EI_GLTFTexturesAndBuffers* GLTFBuffersAndTextures = new CAULDRON_DX12::GLTFTexturesAndBuffers();
    GLTFBuffersAndTextures->OnCreate(GetCauldronDevice(), pGLTFCommon, &m_uploadHeap, &m_vidMemBufferPool, &m_constantBufferRing);

    return std::unique_ptr<EI_GLTFTexturesAndBuffers>(GLTFBuffersAndTextures);
}

std::unique_ptr<EI_GltfPbrPass> EI_Device::CreateGLTFPbrPass(EI_GLTFTexturesAndBuffers* pGLTFTexturesAndBuffers, EI_RenderTargetSet* renderTargetSet)
{
    EI_GltfPbrPass* GLTFPbr = new CAULDRON_DX12::GltfPbrPass();
    GLTFPbr->OnCreate(GetCauldronDevice(), &m_uploadHeap, &m_resourceViewHeaps, &m_constantBufferRing, &m_vidMemBufferPool,
                      pGLTFTexturesAndBuffers, nullptr, false, GetColorBufferFormat(), 1);

    return std::unique_ptr<EI_GltfPbrPass>(GLTFPbr);
}

std::unique_ptr<EI_GltfDepthPass> EI_Device::CreateGLTFDepthPass(EI_GLTFTexturesAndBuffers* pGLTFTexturesAndBuffers, EI_RenderTargetSet* renderTargetSet)
{
    EI_GltfDepthPass* GLTFDepth = new CAULDRON_DX12::GltfDepthPass();
    assert(renderTargetSet);

    GLTFDepth->OnCreate(GetCauldronDevice(), &m_uploadHeap, &m_resourceViewHeaps, &m_constantBufferRing, &m_vidMemBufferPool, 
                        pGLTFTexturesAndBuffers);

    return std::unique_ptr<EI_GltfDepthPass>(GLTFDepth);
}

void EI_RenderTargetSet::SetResources(const EI_Resource** pResourcesArray)
{
    for (uint32_t i = 0; i < m_NumResources; ++i)
        m_RenderResources[i] = pResourcesArray[i];
}

EI_RenderTargetSet::~EI_RenderTargetSet()
{
    // Nothing to clean up
}

void EI_Device::BeginRenderPass(EI_CommandContext& commandContext, const EI_RenderTargetSet* pRenderTargetSet, const wchar_t* pPassName, uint32_t width /*= 0*/, uint32_t height /*= 0*/)
{
    D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle[MaxRenderAttachments];
    D3D12_CPU_DESCRIPTOR_HANDLE* depthCpuHandle = nullptr;
    uint32_t numRenderTargets = 0;
    const D3D12_CLEAR_VALUE* pDepthClearValue = nullptr;

    using namespace DirectX::Detail;
    UINT size = static_cast<UINT>((wcslen(pPassName) + 1) * sizeof(wchar_t));
    commandContext.commandBuffer.Get()->BeginEvent(PIX_EVENT_UNICODE_VERSION, pPassName, size);

    assert(pRenderTargetSet->m_NumResources == 1 || (pRenderTargetSet->m_NumResources == 2 && pRenderTargetSet->m_HasDepth) && "Currently only support 1 render target with (or without) depth");

    // This is a depth render
    if (pRenderTargetSet->m_NumResources == 1 && pRenderTargetSet->m_HasDepth)
    {
        cpuHandle[0] = pRenderTargetSet->m_RenderResources[0]->DSView->GetCPU();
        depthCpuHandle = &cpuHandle[0];
        if (pRenderTargetSet->m_ClearDepth)
            pDepthClearValue = &pRenderTargetSet->m_ClearValues[0];
    }
    else
    {
        cpuHandle[0] = pRenderTargetSet->m_RenderResources[0]->RTView->GetCPU();
        ++numRenderTargets;
    }

    if (pRenderTargetSet->m_HasDepth && pRenderTargetSet->m_NumResources > 1)
    {
        cpuHandle[1] = pRenderTargetSet->m_RenderResources[1]->DSView->GetCPU();
        depthCpuHandle = &cpuHandle[1];
        if (pRenderTargetSet->m_ClearDepth)
            pDepthClearValue = &pRenderTargetSet->m_ClearValues[1];
    }

    commandContext.commandBuffer->OMSetRenderTargets(numRenderTargets, numRenderTargets? cpuHandle : nullptr, FALSE, depthCpuHandle);

    // Do we need to clear?
    if (numRenderTargets && pRenderTargetSet->m_ClearColor[0])
        commandContext.commandBuffer->ClearRenderTargetView(cpuHandle[0], pRenderTargetSet->m_ClearValues[0].Color, 0, nullptr);

    if (depthCpuHandle && pDepthClearValue)
        commandContext.commandBuffer->ClearDepthStencilView(*depthCpuHandle, D3D12_CLEAR_FLAG_DEPTH, pDepthClearValue->DepthStencil.Depth, pDepthClearValue->DepthStencil.Stencil, 0, nullptr);

    SetViewportAndScissor(commandContext, 0, 0, width ? width : m_width, height ? height : m_height);
}

void EI_Device::EndRenderPass(EI_CommandContext& commandContext)
{
    // End of tracing event 
    commandContext.commandBuffer.Get()->EndEvent();

    // Unset all OMS RenderTargets
    GetDevice()->EndRenderPass();
}

void EI_Device::SetViewportAndScissor(EI_CommandContext& commandContext, uint32_t topX, uint32_t topY, uint32_t width, uint32_t height)
{
    CAULDRON_DX12::SetViewportAndScissor(commandContext.commandBuffer.Get(), topX, topY, width, height);
}

void EI_Device::OnCreate(HWND hWnd, uint32_t numBackBuffers, bool enableValidation, const char* appName)
{
    // Create Device
    m_device.OnCreate(appName, "TressFX 4.1 (DX12)", enableValidation, hWnd);
    m_device.CreatePipelineCache();

    //init the shader compiler
    CAULDRON_DX12::CreateShaderCache();

    // Create Swap chain
    m_swapChain.OnCreate(&m_device, numBackBuffers, hWnd, CAULDRON_DX12::DISPLAYMODE_SDR);

    m_resourceViewHeaps.OnCreate(&m_device, 256, 256, 256, 256, 256, 256);

    // Create our own resource heaps (needed for more complex UAV behaviors)
    m_CPUDescriptorIndex = 0;
    m_DescriptorSize = m_device.GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    D3D12_DESCRIPTOR_HEAP_DESC descHeap;
    descHeap.NumDescriptors = 256;
    descHeap.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    descHeap.NodeMask = 0;
        
    // CPU read/write Descriptor heap
    descHeap.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    ThrowIfFailed(m_device.GetDevice()->CreateDescriptorHeap(&descHeap, IID_PPV_ARGS(m_CPUDescriptorHeap.GetAddressOf())));
    m_CPUDescriptorHeap->SetName(L"DX12EngineInterface_CPUDescriptorHeap");

    // Create a command list ring for the Direct queue
    D3D12_COMMAND_QUEUE_DESC CommandQueueDesc = {};
    CommandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    CommandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    CommandQueueDesc.NodeMask = 0;
    m_commandListRing.OnCreate(&m_device, numBackBuffers, 8, CommandQueueDesc);
    // async compute
    // Cauldron doesn't currently support a compute queue (TODO), so reuse the direct queue for these
    CommandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_COMPUTE;
    m_computeCommandListRing.OnCreate(&m_device, numBackBuffers, 8, CommandQueueDesc);
    BeginNewCommandBuffer();
    
    // Create fences
    m_computeDoneFence.OnCreate(&m_device, "Compute Done Fence");
    m_lastFrameGraphicsCommandBufferFence.OnCreate(&m_device, "Last Frame Graphics Command Buffer Fence");

    // Create a 'dynamic' constant buffers ring
    m_constantBufferRing.OnCreate(&m_device, numBackBuffers, 20 * 1024 * 1024, &m_resourceViewHeaps);

    // Create a 'static' constant buffer pool
    m_vidMemBufferPool.OnCreate(&m_device, 128 * 1024 * 1024, USE_VID_MEM, "StaticGeom");
    m_sysMemBufferPool.OnCreate(&m_device, 32 * 1024, false, "PostProcGeom");

    // initialize the GPU time stamps module
    m_gpuTimer.OnCreate(&m_device, numBackBuffers);

    // Quick helper to upload resources, it has it's own commandList and uses sub-allocation.
    // for 4K textures we'll need 100Megs
    m_uploadHeap.OnCreate(&m_device, 100 * 1024 * 1024);

    // Create tonemapping pass
    m_toneMapping.OnCreate(&m_device, &m_resourceViewHeaps, &m_constantBufferRing, &m_vidMemBufferPool, m_swapChain.GetFormat());

    // Initialize UI rendering resources
    m_ImGUI.OnCreate(&m_device, &m_uploadHeap, &m_resourceViewHeaps, &m_constantBufferRing, DXGI_FORMAT_R8G8B8A8_UNORM);

    // Create a render pass for our main buffer as it's needed earlier than other stuff is created
    //VkFormat backbufferFormats[] = { VK_FORMAT_R8G8B8A8_SRGB, VK_FORMAT_D32_SFLOAT };
    
    // Create index buffer for full screen passes
    m_pFullscreenIndexBuffer = CreateBufferResource(sizeof(uint32_t), 4, EI_BF_INDEXBUFFER, "FullScreenIndexBuffer");

    // Create shadow buffer. Because GLTF only allows us 1 buffer, we are going to create a HUGE one and divy it up as needed.
    m_shadowBuffer = CreateDepthResource(4096, 4096, "Shadow Buffer");

    // Create layout and PSO for resolve to swap chain
    EI_LayoutDescription desc = { { {"ColorTexture", 0, EI_RESOURCETYPE_IMAGE_RO }, }, EI_PS };
    m_pEndFrameResolveBindLayout = CreateLayout(desc);

    // Recreate a PSO for full screen resolve to swap chain
    EI_PSOParams psoParams;
    psoParams.primitiveTopology = EI_Topology::TriangleStrip;
    psoParams.colorWriteEnable = true;
    psoParams.depthTestEnable = false;
    psoParams.depthWriteEnable = false;
    psoParams.depthCompareOp = EI_CompareFunc::Always;

    psoParams.colorBlendParams.colorBlendEnabled = false;
    psoParams.colorBlendParams.colorBlendOp = EI_BlendOp::Add;
    psoParams.colorBlendParams.colorSrcBlend = EI_BlendFactor::Zero;
    psoParams.colorBlendParams.colorDstBlend = EI_BlendFactor::One;
    psoParams.colorBlendParams.alphaBlendOp = EI_BlendOp::Add;
    psoParams.colorBlendParams.alphaSrcBlend = EI_BlendFactor::One;
    psoParams.colorBlendParams.alphaDstBlend = EI_BlendFactor::Zero;

    EI_BindLayout* layouts[] = { m_pEndFrameResolveBindLayout.get() };
    psoParams.layouts = layouts;
    psoParams.numLayouts = 1;
    psoParams.renderTargetSet = nullptr; // Will go to swap chain
    m_pEndFrameResolvePSO = CreateGraphicsPSO("FullScreenRender.hlsl", "FullScreenVS", "FullScreenRender.hlsl", "FullScreenPS", psoParams);

    // Create default white texture to use
    m_DefaultWhiteTexture = CreateResourceFromFile("DefaultWhite.png", true);

    // Create some samplers to use
    m_LinearWrapSampler = CreateSampler(EI_Filter::Linear, EI_Filter::Linear, EI_Filter::Linear, EI_AddressMode::Wrap);

    // finish creating the index buffer
    uint32_t indexArray[] = { 0, 1, 2, 3 };
    m_currentCommandBuffer.UpdateBuffer(m_pFullscreenIndexBuffer.get(), indexArray);

    EI_Barrier copyToResource[] = {
        { m_pFullscreenIndexBuffer.get(), EI_STATE_COPY_DEST, EI_STATE_INDEX_BUFFER }
    };
    m_currentCommandBuffer.SubmitBarrier(AMD_ARRAY_SIZE(copyToResource), copyToResource);

}

void EI_Device::OnResize(uint32_t width, uint32_t height)
{
    m_width = width;
    m_height = height;

    // if a previous resize event from this frame hasnt already opened a command buffer
    if (!m_recording)
    {
        BeginNewCommandBuffer();
    }

    // If resizing but no minimizing
    if (width > 0 && height > 0)
    {
        // Re/Create color buffer
        m_colorBuffer = CreateRenderTargetResource(width, height, 4, 1, "Color Buffer");

        m_depthBuffer = CreateDepthResource(width, height, "Depth Buffer");
        m_swapChain.OnCreateWindowSizeDependentResources(width, height, m_vSync, CAULDRON_DX12::DISPLAYMODE_SDR);

#ifdef TRESSFX_DEBUG_UAV
        m_pDebugUAV = CreateDebugUAVResource(width, height, 4, 2, "DebugUAV", 0.f);
#endif // TRESSFX_DEBUG_UAV

        // Create resources we need to resolve out render target back to swap chain
        EI_BindSetDescription bindSet = { { m_colorBuffer.get() } };
        m_pEndFrameResolveBindSet = CreateBindSet(m_pEndFrameResolveBindLayout.get(), bindSet);

        // Create a bind set for any samplers we need (Doing it here because the layouts aren't yet initialized during OnCreate() call)
        EI_BindSetDescription bindSetDesc = { { m_LinearWrapSampler.get(), } };
        m_pSamplerBindSet = CreateBindSet(GetSamplerLayout(), bindSetDesc);

        // update tonemapping
        m_toneMapping.UpdatePipelines(m_swapChain.GetFormat());
    }
}

void EI_Device::OnDestroy()
{
    m_device.GPUFlush();

    // Remove linear wrap sampler
    m_LinearWrapSampler.reset();

    // Remove default white texture
    m_DefaultWhiteTexture.reset();

    // Wipe all the local resources we were using
    m_pSamplerBindSet.reset();
    m_pEndFrameResolveBindSet.reset();
    m_pEndFrameResolvePSO.reset();
    m_pEndFrameResolveBindLayout.reset();

    m_pFullscreenIndexBuffer.reset();

    m_depthBuffer.reset();
    m_colorBuffer.reset();

#if TRESSFX_DEBUG_UAV
    m_pDebugUAV.reset();
#endif // TRESSFX_DEBUG_UAV

    m_toneMapping.OnDestroy();
    m_ImGUI.OnDestroy();

    m_uploadHeap.OnDestroy();
    m_gpuTimer.OnDestroy();
    m_vidMemBufferPool.OnDestroy();
    m_sysMemBufferPool.OnDestroy();
    m_constantBufferRing.OnDestroy();
    m_resourceViewHeaps.OnDestroy();
    m_commandListRing.OnDestroy();
    m_computeCommandListRing.OnDestroy();

    // Full screen state should always be false before exiting the app.
    m_swapChain.SetFullScreen(false);
    m_swapChain.OnDestroyWindowSizeDependentResources();
    m_swapChain.OnDestroy();

    // shut down the shader compiler
    DestroyShaderCache(&m_device);
    m_device.DestroyPipelineCache();
    m_device.OnDestroy();
}

D3D12_SHADER_VISIBILITY GetShaderVisibility(EI_ShaderStage stage)
{
    switch (stage)
    {
    case EI_VS:
        return D3D12_SHADER_VISIBILITY_VERTEX;
    case EI_PS:
        return D3D12_SHADER_VISIBILITY_PIXEL;
    case EI_CS:
        return D3D12_SHADER_VISIBILITY_ALL;
    case EI_ALL:
        return D3D12_SHADER_VISIBILITY_ALL;
    default:
        return D3D12_SHADER_VISIBILITY_ALL;
    }
}

std::unique_ptr<EI_PSO> EI_Device::CreateComputeShaderPSO(const char* shaderName, const char* entryPoint, EI_BindLayout** layouts, int numLayouts)
{
    EI_PSO* result = new EI_PSO;

    DefineList defines;
    defines["AMD_TRESSFX_MAX_NUM_BONES"] = std::to_string(AMD_TRESSFX_MAX_NUM_BONES);
    defines["AMD_TRESSFX_MAX_HAIR_GROUP_RENDER"] = std::to_string(AMD_TRESSFX_MAX_HAIR_GROUP_RENDER);
    defines["AMD_TRESSFX_DX12"] = "1";

#ifdef TRESSFX_DEBUG_UAV
    defines["TRESSFX_DEBUG_UAV"] = "1";
#endif // TRESSFX_DEBUG_UAV

    D3D12_SHADER_BYTECODE computeShader;
    CAULDRON_DX12::CompileShaderFromFile(shaderName, &defines, entryPoint, "cs_6_0", D3DCOMPILE_DEBUG | D3DCOMPILE_OPTIMIZATION_LEVEL0 | D3DCOMPILE_SKIP_OPTIMIZATION, &computeShader);

    CD3DX12_ROOT_PARAMETER descSetLayouts[16];
    assert(numLayouts < 16);
    for (int i = 0; i < numLayouts; ++i)
    {
        // i dont like this since it has a side effect on the layout that gets passed - i'd just get rid of the spaces
        for (int j = 0; j < layouts[i]->layoutBindings.size(); ++j)
        {
            layouts[i]->layoutBindings[j].RegisterSpace = i;
        }
        descSetLayouts[i].InitAsDescriptorTable((uint32_t)layouts[i]->layoutBindings.size(), layouts[i]->layoutBindings.data(), GetShaderVisibility(layouts[i]->description.stage));
    }

    CD3DX12_ROOT_SIGNATURE_DESC descRootSignature = CD3DX12_ROOT_SIGNATURE_DESC();
    descRootSignature.NumParameters = numLayouts;
    descRootSignature.pParameters = descSetLayouts;
    descRootSignature.NumStaticSamplers = 0;
    descRootSignature.pStaticSamplers = 0;

    // deny uneccessary access to certain pipeline stages
    descRootSignature.Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE
        | D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
        | D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS
        | D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS
        | D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

    ID3DBlob* signature;
    ID3DBlob* error;
    HRESULT hr = D3D12SerializeRootSignature(&descRootSignature, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);
    m_device.GetDevice()->CreateRootSignature(
        0,
        signature->GetBufferPointer(),
        signature->GetBufferSize(),
        __uuidof(ID3D12RootSignature),
        (void**)&result->m_pipelineLayout);

    signature->Release();
    if (error)
        error->Release();

    // Describe and create the compute pipeline state object (PSO).
    D3D12_COMPUTE_PIPELINE_STATE_DESC computePsoDesc = {};
    computePsoDesc.pRootSignature = result-> m_pipelineLayout.Get();
    computePsoDesc.CS = CD3DX12_SHADER_BYTECODE(computeShader);
    ThrowIfFailed(m_device.GetDevice()->CreateComputePipelineState(&computePsoDesc, IID_PPV_ARGS(&result->m_pipeline)));

    wchar_t  uniName[1024];
    swprintf(uniName, 1024, L"%hs%hs", shaderName, entryPoint);
    result->m_pipelineLayout->SetName(uniName);
    result->m_pipeline->SetName(uniName);

    result->m_bp = EI_BP_COMPUTE;
    return std::unique_ptr<EI_PSO>(result);
}

EI_PSO::~EI_PSO()
{
    // Everything will auto release when going out of scope
}

std::unique_ptr<EI_PSO> EI_Device::CreateGraphicsPSO(const char* vertexShaderName, const char* vertexEntryPoint, const char* fragmentShaderName, const char* fragmentEntryPoint, EI_PSOParams& psoParams)
{
    EI_PSO* result = new EI_PSO;

    DefineList defines;
    defines["AMD_TRESSFX_MAX_NUM_BONES"] = std::to_string(AMD_TRESSFX_MAX_NUM_BONES);
    defines["AMD_TRESSFX_MAX_HAIR_GROUP_RENDER"] = std::to_string(AMD_TRESSFX_MAX_HAIR_GROUP_RENDER);
    defines["AMD_TRESSFX_DX12"] = "1";

#ifdef TRESSFX_DEBUG_UAV
    defines["TRESSFX_DEBUG_UAV"] = "1";
#endif // TRESSFX_DEBUG_UAV

    // Compile and create shaders
    D3D12_SHADER_BYTECODE vertexShader, fragmentShader;

    CAULDRON_DX12::CompileShaderFromFile(vertexShaderName, &defines, vertexEntryPoint, "vs_6_0", D3DCOMPILE_DEBUG | D3DCOMPILE_OPTIMIZATION_LEVEL0 | D3DCOMPILE_SKIP_OPTIMIZATION, &vertexShader);
    CAULDRON_DX12::CompileShaderFromFile(fragmentShaderName, &defines, fragmentEntryPoint, "ps_6_0", D3DCOMPILE_DEBUG | D3DCOMPILE_OPTIMIZATION_LEVEL0 | D3DCOMPILE_SKIP_OPTIMIZATION, &fragmentShader);
    
    CD3DX12_ROOT_PARAMETER descSetLayouts[16];
    for (int i = 0; i < psoParams.numLayouts; ++i)
    {
        // i dont like this since it has a side effect on the layout that gets passed - i'd just get rid of the spaces
        for (int j = 0; j < psoParams.layouts[i]->layoutBindings.size(); ++j)
        {
            psoParams.layouts[i]->layoutBindings[j].RegisterSpace = i;
        }
        descSetLayouts[i].InitAsDescriptorTable((uint32_t)psoParams.layouts[i]->layoutBindings.size(), psoParams.layouts[i]->layoutBindings.data(), GetShaderVisibility(psoParams.layouts[i]->description.stage));
    }

    CD3DX12_ROOT_SIGNATURE_DESC descRootSignature = CD3DX12_ROOT_SIGNATURE_DESC();
    descRootSignature.NumParameters = psoParams.numLayouts;
    descRootSignature.pParameters = descSetLayouts;
    descRootSignature.NumStaticSamplers = 0;
    descRootSignature.pStaticSamplers = 0;

    // deny uneccessary access to certain pipeline stages
    descRootSignature.Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE
        | D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
        | D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS
        | D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS
        | D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

    ID3DBlob* signature;
    ID3DBlob* error;
    HRESULT hr = D3D12SerializeRootSignature(&descRootSignature, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);
    m_device.GetDevice()->CreateRootSignature(
        0,
        signature->GetBufferPointer(),
        signature->GetBufferSize(),
        __uuidof(ID3D12RootSignature),
        (void**)&result->m_pipelineLayout);

    signature->Release();
    if (error)
        error->Release();

    // Setup blending
    D3D12_BLEND_DESC blendDesc;
    blendDesc.AlphaToCoverageEnable = false;
    blendDesc.IndependentBlendEnable = false;
    blendDesc.RenderTarget[0].LogicOpEnable = false;
    blendDesc.RenderTarget[0].BlendEnable = psoParams.colorBlendParams.colorBlendEnabled;
    blendDesc.RenderTarget[0].SrcBlend = *psoParams.colorBlendParams.colorSrcBlend;
    blendDesc.RenderTarget[0].DestBlend = *psoParams.colorBlendParams.colorDstBlend;
    blendDesc.RenderTarget[0].BlendOp = *psoParams.colorBlendParams.colorBlendOp;
    blendDesc.RenderTarget[0].SrcBlendAlpha = *psoParams.colorBlendParams.alphaSrcBlend;
    blendDesc.RenderTarget[0].DestBlendAlpha = *psoParams.colorBlendParams.alphaDstBlend;
    blendDesc.RenderTarget[0].BlendOpAlpha = *psoParams.colorBlendParams.alphaBlendOp;
    blendDesc.RenderTarget[0].LogicOp = D3D12_LOGIC_OP_NOOP;
    blendDesc.RenderTarget[0].RenderTargetWriteMask =  psoParams.colorWriteEnable ? D3D12_COLOR_WRITE_ENABLE_ALL : 0;

    bool depthOnly = (psoParams.renderTargetSet && psoParams.renderTargetSet->m_NumResources == 1 && psoParams.renderTargetSet->m_HasDepth);
    bool hasDepth = (psoParams.renderTargetSet && psoParams.renderTargetSet->m_HasDepth);

    // Describe and create the graphics pipeline state object (PSO).
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.InputLayout = { nullptr, 0 };  // Don't even need a vertex buffer right now
    psoDesc.pRootSignature = result->m_pipelineLayout.Get();
    psoDesc.VS = vertexShader;
    psoDesc.PS = fragmentShader;
    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.RasterizerState.FrontCounterClockwise = true;
    // hack, the fullscreen quad doesnt show up without this even if i reverse the index order
    if (psoParams.primitiveTopology == EI_Topology::TriangleStrip)
    {
        psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
    }
    psoDesc.BlendState = blendDesc;
    psoDesc.DepthStencilState.DepthEnable = psoParams.depthTestEnable;
    psoDesc.DepthStencilState.StencilEnable = psoParams.stencilTestEnable;
    psoDesc.DepthStencilState.DepthFunc = *psoParams.depthCompareOp;
    psoDesc.DepthStencilState.DepthWriteMask = psoParams.depthWriteEnable ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;
    psoDesc.DepthStencilState.StencilReadMask = psoParams.stencilReadMask;
    psoDesc.DepthStencilState.StencilWriteMask = psoParams.stencilWriteMask;
    psoDesc.DepthStencilState.BackFace.StencilDepthFailOp = *psoParams.backDepthFailOp;
    psoDesc.DepthStencilState.BackFace.StencilFailOp = *psoParams.backFailOp;
    psoDesc.DepthStencilState.BackFace.StencilFunc = *psoParams.backCompareOp;
    psoDesc.DepthStencilState.BackFace.StencilPassOp = *psoParams.backPassOp;
    psoDesc.DepthStencilState.FrontFace.StencilDepthFailOp = *psoParams.frontDepthFailOp;
    psoDesc.DepthStencilState.FrontFace.StencilFailOp = *psoParams.frontFailOp;
    psoDesc.DepthStencilState.FrontFace.StencilFunc = *psoParams.frontCompareOp;
    psoDesc.DepthStencilState.FrontFace.StencilPassOp = *psoParams.frontPassOp;
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = depthOnly ? 0 : 1;
    psoDesc.RTVFormats[0] = (psoParams.renderTargetSet) ? ( depthOnly ? DXGI_FORMAT_UNKNOWN : psoParams.renderTargetSet->m_RenderResourceFormats[0] ) : m_swapChain.GetFormat();
    psoDesc.DSVFormat = hasDepth ? psoParams.renderTargetSet->m_RenderResourceFormats[psoParams.renderTargetSet->m_NumResources-1] : DXGI_FORMAT_UNKNOWN;
    psoDesc.SampleDesc.Count = 1;
    ThrowIfFailed(m_device.GetDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&result->m_pipeline)));

    // Store the prim type as well
    result->m_primitiveTopology = *psoParams.primitiveTopology;

    wchar_t  uniName[1024];
    swprintf(uniName, 1024, L"%hs%hs", vertexShaderName, vertexEntryPoint);
    result->m_pipelineLayout->SetName(uniName);
    result->m_pipeline->SetName(uniName);

    result->m_bp = EI_BP_GRAPHICS;
    return std::unique_ptr<EI_PSO>(result);
}

void EI_Device::WaitForCompute()
{
    m_computeDoneFence.GpuWaitForFence(m_device.GetGraphicsQueue());
}

void EI_Device::SignalComputeStart()
{
}

void EI_Device::WaitForLastFrameGraphics()
{
    m_lastFrameGraphicsCommandBufferFence.CpuWaitForFence(1);
}

void EI_Device::SubmitComputeCommandList()
{
    m_currentComputeCommandBuffer.commandBuffer->Close();

    // Execute the command list.
    ID3D12CommandList* ppCommandLists[] = { m_currentComputeCommandBuffer.commandBuffer.Get() };
    m_device.GetComputeQueue()->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
    m_computeDoneFence.IssueFence(m_device.GetComputeQueue());
}

void EI_CommandContext::DrawIndexedInstanced(EI_PSO& pso, EI_IndexedDrawParams& drawParams)
{
    commandBuffer->IASetIndexBuffer(&drawParams.pIndexBuffer->m_pBuffer->indexBufferView);
    commandBuffer->IASetPrimitiveTopology(pso.m_primitiveTopology);
    commandBuffer->SetPipelineState(pso.m_pipeline.Get());
    commandBuffer->DrawIndexedInstanced(drawParams.numIndices, 1, 0, 0, 0);
}

void EI_CommandContext::DrawInstanced(EI_PSO& pso, EI_DrawParams& drawParams)
{
    commandBuffer->SetPipelineState(pso.m_pipeline.Get());
    commandBuffer->IASetPrimitiveTopology(pso.m_primitiveTopology);
    commandBuffer->DrawInstanced(drawParams.numVertices, drawParams.numInstances, 0, 0);
}

void EI_CommandContext::PushConstants(EI_PSO* pso, int size, void* data)
{
    assert(false && "Not yet implemented!");
    //vkCmdPushConstants(commandBuffer, pso->m_pipelineLayout, VK_SHADER_STAGE_ALL, 0, size, data);
}

void EI_Device::BeginNewCommandBuffer()
{
    m_currentCommandBuffer.commandBuffer = m_commandListRing.GetNewCommandList();
    m_recording = true;
}

void EI_Device::BeginNewComputeCommandBuffer()
{
    m_currentComputeCommandBuffer.commandBuffer = m_computeCommandListRing.GetNewCommandList();
}

void EI_Device::EndAndSubmitCommandBuffer()
{
    m_currentCommandBuffer.commandBuffer->Close();

    // Execute the command list.
    ID3D12CommandList* ppCommandLists[] = { m_currentCommandBuffer.commandBuffer.Get() };
    m_device.GetGraphicsQueue()->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
    m_recording = false;
}

void EI_Device::EndAndSubmitCommandBufferWithFence()
{
    m_currentCommandBuffer.commandBuffer->Close();

    // Execute the command list.
    ID3D12CommandList* ppCommandLists[] = { m_currentCommandBuffer.commandBuffer.Get() };
    m_device.GetGraphicsQueue()->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
    m_lastFrameGraphicsCommandBufferFence.IssueFence(m_device.GetGraphicsQueue());
    m_recording = false;
}

void EI_Device::BeginBackbufferRenderPass()
{
    using namespace DirectX::Detail;
    const wchar_t pPassName[] = L"BackBufferRenderPass";
    UINT size = static_cast<UINT>((wcslen(pPassName) + 1) * sizeof(wchar_t));
    m_currentCommandBuffer.commandBuffer.Get()->BeginEvent(PIX_EVENT_UNICODE_VERSION, pPassName, size);

    // First transition the current back buffer to render target from present resource
    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_swapChain.GetCurrentBackBufferResource(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
    m_currentCommandBuffer.commandBuffer->ResourceBarrier(1, &barrier);

    D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle[MaxRenderAttachments];

    cpuHandle[0] = *m_swapChain.GetCurrentBackBufferRTV();
    cpuHandle[1] = m_depthBuffer->DSView->GetCPU();

    m_currentCommandBuffer.commandBuffer->OMSetRenderTargets(1, cpuHandle, FALSE, &cpuHandle[1]);

    // Setup fast clear
    D3D12_CLEAR_VALUE clearValues[2];
    clearValues[0].Format = m_swapChain.GetFormat();
    clearValues[0].Color[0] = 0.0f;
    clearValues[0].Color[1] = 0.0f;
    clearValues[0].Color[2] = 0.0f;
    clearValues[0].Color[3] = 1.0f;
    
    clearValues[1].Format = DXGI_FORMAT_D32_FLOAT;
    clearValues[1].DepthStencil.Depth = 1.0f;
    clearValues[1].DepthStencil.Stencil = 0;

    // Do a clear before rendering everything out to the swap chain buffer
    m_currentCommandBuffer.commandBuffer->ClearRenderTargetView(cpuHandle[0], clearValues[0].Color, 0, nullptr);
    // Only clear depth as we don't currently have a stencil
    m_currentCommandBuffer.commandBuffer->ClearDepthStencilView(cpuHandle[1], D3D12_CLEAR_FLAG_DEPTH, clearValues[1].DepthStencil.Depth, clearValues[1].DepthStencil.Stencil, 0, nullptr);

    CAULDRON_DX12::SetViewportAndScissor(m_currentCommandBuffer.commandBuffer.Get(), 0, 0, m_width, m_height);
}

void EI_Device::EndRenderPass()
{
    // Unbind all render targets/depth/stencil buffers
    m_currentCommandBuffer.commandBuffer->OMSetRenderTargets(0, nullptr, FALSE, nullptr);
}

void EI_Device::GetTimeStamp(char* name)
{	
    m_gpuTimer.GetTimeStamp(m_currentCommandBuffer.commandBuffer.Get(), name);
}

D3D12_DESCRIPTOR_RANGE_TYPE GetDescriptorRangeType(EI_ResourceTypeEnum type)
{
    switch (type)
    {
    case EI_RESOURCETYPE_BUFFER_RW:
        return D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
    case EI_RESOURCETYPE_BUFFER_RO:
        return D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    case EI_RESOURCETYPE_IMAGE_RW:
        return D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
    case EI_RESOURCETYPE_IMAGE_RO:
        return D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    case EI_RESOURCETYPE_UNIFORM:
        return D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
    case EI_RESOURCETYPE_SAMPLER:
        return D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
    default:
        assert(0);
    }
    return D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
}

CD3DX12_DESCRIPTOR_RANGE DX12DescriptorSetBinding(int binding, EI_ShaderStage stage, EI_ResourceTypeEnum type) {

    CD3DX12_DESCRIPTOR_RANGE b;
    D3D12_DESCRIPTOR_RANGE_TYPE rangeType;

    if (type == EI_RESOURCETYPE_BUFFER_RO)
    {
        rangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    }
    if (type == EI_RESOURCETYPE_BUFFER_RW)
    {
        rangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
    }
    if (type == EI_RESOURCETYPE_IMAGE_RW)
    {
        rangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
    }
    if (type == EI_RESOURCETYPE_IMAGE_RO)
    {
        rangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    }
    if (type == EI_RESOURCETYPE_UNIFORM)
    {
        rangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
    }
    if (type == EI_RESOURCETYPE_SAMPLER)
    {
        rangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
    }
    b.Init(rangeType, 1, binding, 0);
    return b;
}

std::unique_ptr<EI_BindLayout> EI_Device::CreateLayout(const EI_LayoutDescription& description)
{
    std::vector<CD3DX12_DESCRIPTOR_RANGE> layoutBindings;

    EI_BindLayout* pLayout = new EI_BindLayout;

    int numResources = (int)description.resources.size();

    for (int i = 0; i < numResources; ++i)
    {
        int binding = description.resources[i].binding;
        if (binding >= 0) {
            layoutBindings.push_back(DX12DescriptorSetBinding(binding, description.stage, description.resources[i].type));
        }
    }

    pLayout->layoutBindings = layoutBindings;
    pLayout->description = description;
    return std::unique_ptr<EI_BindLayout>(pLayout);
}

EI_BindLayout::~EI_BindLayout()
{
    // Nothing to do here ...
}

void EI_Device::OnBeginFrame(bool bDoAsync)
{
    // This needs to be called prior to getting a command list because it's done differently on DX12 than Vulkan
    if (bDoAsync)
        m_computeCommandListRing.OnBeginFrame();

    // Let our resource managers do some house keeping 
    m_constantBufferRing.OnBeginFrame();

    // Timing values
    UINT64 gpuTicksPerSecond;
    m_device.GetGraphicsQueue()->GetTimestampFrequency(&gpuTicksPerSecond);
    m_gpuTimer.OnBeginFrame(gpuTicksPerSecond, &m_timeStamps);

    // if a resize event already started a the command buffer - we need to do it this way,
    // because multiple resizes in one frame could overflow the command buffer pool if we open a new
    // command buffer everytime we resize
    if (m_recording)
    {
        EndAndSubmitCommandBuffer();
        FlushGPU();
    }

    WaitForLastFrameGraphics();
    BeginNewCommandBuffer();

    if (bDoAsync)
        BeginNewComputeCommandBuffer();

    std::map<std::string, float> timeStampMap;
    for (int i = 0; i < (int)m_timeStamps.size() - 1; ++i)
    {
        timeStampMap[m_timeStamps[i + 1].m_label] += m_timeStamps[i + 1].m_microseconds - m_timeStamps[i].m_microseconds;
    }
    int i = 0;
    m_sortedTimeStamps.resize(timeStampMap.size());
    for (std::map<std::string, float>::iterator it = timeStampMap.begin(); it != timeStampMap.end(); ++it)
    {
        m_sortedTimeStamps[i].m_label = it->first;
        m_sortedTimeStamps[i].m_microseconds = it->second;
        ++i;
    }
    if (m_timeStamps.size() > 0)
    {
        //scrolling data and average computing
        static float values[128];
        values[127] = (float)(m_timeStamps.back().m_microseconds - m_timeStamps.front().m_microseconds);
        float average = values[0];
        for (uint32_t i = 0; i < 128 - 1; i++) { values[i] = values[i + 1]; average += values[i]; }
        average /= 128;
        m_averageGpuTime = average;
    }
}

void EI_Device::OnEndFrame()
{
    // Transition the color buffer to read before rendering into the swapchain
    EI_Barrier barrier = { m_colorBuffer.get(), EI_STATE_RENDER_TARGET, EI_STATE_SRV };
    m_currentCommandBuffer.SubmitBarrier(1, &barrier);

    EndAndSubmitCommandBuffer();

    m_swapChain.WaitForSwapChain();

    m_commandListRing.OnBeginFrame();

    BeginNewCommandBuffer();
    // Start by resolving render to swap chain
    BeginBackbufferRenderPass();

    // Tonemapping
    {
        m_currentCommandBuffer.commandBuffer->OMSetRenderTargets(1, m_swapChain.GetCurrentBackBufferRTV(), true, NULL);

        float exposure = 1.0f;
        int toneMapper = 0;
        m_toneMapping.Draw(m_currentCommandBuffer.commandBuffer.Get(), m_colorBuffer.get()->SRView, exposure, toneMapper);
        GetTimeStamp("Tone mapping");
    }

    // Do UI render over top
    RenderUI();

    // Wrap up
    EndRenderPass();

    // When we are done, transition it back for the next frame
    barrier = { m_colorBuffer.get(), EI_STATE_SRV, EI_STATE_RENDER_TARGET };
    m_currentCommandBuffer.SubmitBarrier(1, &barrier);


    // Make swap chain buffer presentable again
    CD3DX12_RESOURCE_BARRIER presentBarrier = CD3DX12_RESOURCE_BARRIER::Transition(m_swapChain.GetCurrentBackBufferResource(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
    m_currentCommandBuffer.commandBuffer->ResourceBarrier(1, &presentBarrier);

    m_gpuTimer.OnEndFrame();
    // Get the stats from the GPU
    m_gpuTimer.CollectTimings(m_currentCommandBuffer.commandBuffer.Get());

    EndAndSubmitCommandBufferWithFence();

    m_swapChain.Present();
}

void EI_Device::RenderUI()
{
    m_ImGUI.Draw(m_currentCommandBuffer.commandBuffer.Get());
}

static EI_Device* g_device = NULL;

EI_Device* GetDevice() {
    if (g_device == NULL) {
        g_device = new EI_Device();
    }
    return g_device;
}

void EI_CommandContext::BindSets(EI_PSO* pso, int numBindSets, EI_BindSet** bindSets)
{
    assert(numBindSets < 8);
    ID3D12DescriptorHeap* heaps[] = { GetDevice()->GetResourceViewHeaps().GetCBV_SRV_UAVHeap(), GetDevice()->GetResourceViewHeaps().GetSamplerHeap() };
    commandBuffer->SetDescriptorHeaps(2, heaps);

    if (pso->m_bp == EI_BP_GRAPHICS)
    {
        commandBuffer->SetGraphicsRootSignature(pso->m_pipelineLayout.Get());
        for (int i = 0; i < numBindSets; i++)
            commandBuffer->SetGraphicsRootDescriptorTable(i, bindSets[i]->descriptorTable.GetGPU());
    }
    else
    {
        commandBuffer->SetComputeRootSignature(pso->m_pipelineLayout.Get());

        for (int i = 0; i < numBindSets; i++)
            commandBuffer->SetComputeRootDescriptorTable(i, bindSets[i]->descriptorTable.GetGPU());
    }
}

EI_Resource::EI_Resource() :
    m_ResourceType(EI_ResourceType::Undefined),
    m_pTexture(nullptr)
{
}

EI_Resource::~EI_Resource()
{
    if (m_ResourceType == EI_ResourceType::Buffer && m_pBuffer != nullptr)
    {
        m_pBuffer->Free();
        delete m_pBuffer;
    }
    else if (m_ResourceType == EI_ResourceType::Texture)
    {
        m_pTexture->OnDestroy();
        delete m_pTexture;
    }
    else if (m_ResourceType == EI_ResourceType::Sampler)
    {
        delete m_pSampler;
    }
    else
    {
        assert(false && "Trying to destroy an undefined resource");
    }
}

D3D12_RESOURCE_STATES DX12AccessFlags(EI_ResourceState state)
{
    switch (state)
    {
    case EI_STATE_UNDEFINED:
        return D3D12_RESOURCE_STATE_COMMON;
    case EI_STATE_SRV:
        return D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    case EI_STATE_UAV:
        return D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    case EI_STATE_COPY_DEST:
        return D3D12_RESOURCE_STATE_COPY_DEST;
    case EI_STATE_COPY_SOURCE:
        return D3D12_RESOURCE_STATE_COPY_SOURCE;
    case EI_STATE_RENDER_TARGET:
        return D3D12_RESOURCE_STATE_RENDER_TARGET;
    case EI_STATE_DEPTH_STENCIL:
        return D3D12_RESOURCE_STATE_DEPTH_WRITE;
    case EI_STATE_INDEX_BUFFER:
        return D3D12_RESOURCE_STATE_INDEX_BUFFER;
    case EI_STATE_CONSTANT_BUFFER:
        return D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
    default:
        assert(0);
        return D3D12_RESOURCE_STATE_COMMON;
    }
}

void EI_CommandContext::SubmitBarrier(int numBarriers, EI_Barrier* barriers)
{
    assert(numBarriers < 16);
    CD3DX12_RESOURCE_BARRIER b[16];
    int actualNumBarriers = 0;
    for (int i = 0; i < numBarriers; ++i)
    {
        ID3D12Resource* pResource = nullptr;
        if (barriers[i].pResource->m_ResourceType == EI_ResourceType::Buffer)
            pResource = barriers[i].pResource->m_pBuffer->gpuBuffer;
        else
            pResource = barriers[i].pResource->m_pTexture->GetResource();
        D3D12_RESOURCE_STATES from = DX12AccessFlags(barriers[i].from);
        D3D12_RESOURCE_STATES to = DX12AccessFlags(barriers[i].to);
        if (from != to)
        {
            b[actualNumBarriers++] = CD3DX12_RESOURCE_BARRIER::Transition(pResource, from, to);
        }
        else if (from == D3D12_RESOURCE_STATE_UNORDERED_ACCESS && to == D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
        {
            b[actualNumBarriers++] = CD3DX12_RESOURCE_BARRIER::UAV(pResource);
        }
    }
    if (actualNumBarriers != 0)
    {
        commandBuffer->ResourceBarrier(actualNumBarriers, b);
    }
}

void EI_CommandContext::BindPSO(EI_PSO* pso)
{
    commandBuffer->SetPipelineState(pso->m_pipeline.Get());
    if (pso->m_bp == EI_BP_GRAPHICS)
    {
        commandBuffer->IASetPrimitiveTopology(pso->m_primitiveTopology);
    }
}

void EI_CommandContext::Dispatch(int numGroups)
{
    commandBuffer->Dispatch(numGroups, 1, 1);
}

void EI_CommandContext::UpdateBuffer(EI_Resource* res, void* data)
{
    D3D12_SUBRESOURCE_DATA subResData = {};
    subResData.pData = data;
    subResData.RowPitch = res->m_pBuffer->m_totalMemSize;
    subResData.SlicePitch = res->m_pBuffer->m_totalMemSize;
    UpdateSubresources<1>(commandBuffer.Get(), res->m_pBuffer->gpuBuffer, res->m_pBuffer->cpuBuffer, 0, 0, 1, &subResData);
}

void EI_CommandContext::ClearUint32Image(EI_Resource* res, uint32_t value)
{
    assert(res->m_ResourceType == EI_ResourceType::Buffer && "Trying to clear a non-UAV resource");

    UINT32 values[4] = { value, value, value, value };
    commandBuffer->ClearUnorderedAccessViewUint(res->m_pBuffer->m_pUnorderedAccessView->GetGPU(), res->m_pBuffer->m_pUnorderedAccessView->GetCPU(), res->m_pBuffer->gpuBuffer, values, 0, nullptr);
 }
