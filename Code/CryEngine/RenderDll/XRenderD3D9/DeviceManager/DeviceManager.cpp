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

#include "RenderDll_precompiled.h"
#include "../DriverD3D.h"

#include <Common/Memory/VRAMDrillerBus.h>
#include "Base.h"


#if defined(AZ_RESTRICTED_PLATFORM)
#undef AZ_RESTRICTED_SECTION
#define DEVICEMANAGER_CPP_SECTION_1 1
#define DEVICEMANAGER_CPP_SECTION_3 3
#define DEVICEMANAGER_CPP_SECTION_4 4
#define DEVICEMANAGER_CPP_SECTION_5 5
#define DEVICEMANAGER_CPP_SECTION_6 6
#define DEVICEMANAGER_CPP_SECTION_7 7
#define DEVICEMANAGER_CPP_SECTION_8 8
#define DEVICEMANAGER_CPP_SECTION_9 9
#define DEVICEMANAGER_CPP_SECTION_10 10
#define DEVICEMANAGER_CPP_SECTION_11 11
#define DEVICEMANAGER_CPP_SECTION_12 12
#define DEVICEMANAGER_CPP_SECTION_13 13
#define DEVICEMANAGER_CPP_SECTION_14 14
#define DEVICEMANAGER_CPP_SECTION_15 15
#define DEVICEMANAGER_CPP_SECTION_16 16
#define DEVICEMANAGER_CPP_SECTION_17 17
#define DEVICEMANAGER_CPP_SECTION_18 18
#define DEVICEMANAGER_CPP_SECTION_20 20
#endif

#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION DEVICEMANAGER_CPP_SECTION_1
    #include AZ_RESTRICTED_FILE(DeviceManager_cpp)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#elif defined(CRY_USE_DX12)
    #include "DeviceManager_D3D12.inl"
#else
    #include "DeviceManager_D3D11.inl"
#endif

CDeviceManager::CDeviceManager()
    : m_fence_handle()
{
}

CDeviceManager::~CDeviceManager()
{
    if (m_fence_handle != DeviceFenceHandle() && FAILED(ReleaseFence(m_fence_handle)))
    {
        CryWarning(VALIDATOR_MODULE_RENDERER, VALIDATOR_WARNING,
            "could not create sync fence");
    }
}

void CDeviceManager::Init()
{
#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION DEVICEMANAGER_CPP_SECTION_3
    #include AZ_RESTRICTED_FILE(DeviceManager_cpp)
#endif
#if !DEVICE_MANAGER_IMMEDIATE_STATE_WRITE
    memset(m_CB, 0x0, sizeof(m_CB));
    memset(m_SRV, 0x0, sizeof(m_SRV));
    memset(m_UAV, 0x0, sizeof(m_UAV));
    memset(m_Samplers, 0x0, sizeof(m_Samplers));
    memset(&m_VBs, 0x0, sizeof(m_VBs));
    memset(&m_IB, 0x0, sizeof(m_IB));
    memset(&m_VertexDecl, 0x0, sizeof(m_VertexDecl));
    memset(&m_Topology, 0x0, sizeof(m_Topology));
    memset(&m_Shaders, 0x0, sizeof(m_Shaders));
    memset(&m_RasterState, 0x0, sizeof(m_RasterState));
    memset(&m_DepthStencilState, 0x0, sizeof(m_DepthStencilState));
    memset(&m_BlendState, 0x0, sizeof(m_BlendState));
#endif
}

void CDeviceManager::RT_Tick()
{
    m_numInvalidDrawcalls = 0;

#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION DEVICEMANAGER_CPP_SECTION_4
    #include AZ_RESTRICTED_FILE(DeviceManager_cpp)
#endif
}

HRESULT CDeviceManager::CreateD3D11Texture2D(const D3D11_TEXTURE2D_DESC* desc, [[maybe_unused]] const FLOAT clearValue[4], const D3D11_SUBRESOURCE_DATA* initialData, ID3D11Texture2D** texture2D, const char* textureName)
{
    HRESULT hr = gcpRendD3D->GetDevice().CreateTexture2D(
        desc,
#ifdef CRY_USE_DX12
        clearValue,
#endif
        initialData, texture2D);

    if (hr == S_OK)
    {
#if !defined(RELEASE) && defined(WIN64)
        (*texture2D)->SetPrivateData(WKPDID_D3DDebugObjectName, strlen(textureName), textureName);
#endif
        void* address = static_cast<void*>(*texture2D);
        size_t byteSize = CDeviceTexture::TextureDataSize(desc->Width, desc->Height, 1, desc->MipLevels, 1, CTexture::TexFormatFromDeviceFormat(desc->Format));
        EBUS_EVENT(Render::Debug::VRAMDrillerBus, RegisterAllocation, address, byteSize, textureName, Render::Debug::VRAM_CATEGORY_TEXTURE, Render::Debug::VRAM_SUBCATEGORY_TEXTURE_TEXTURE);
    }
    else
    {
        AZ_Warning("Rendering", false, "CreateD3D11Texture2D for %s failed! [0x%08x]", textureName, hr);
    }

    return hr;
}

void CDeviceManager::ReleaseD3D11Texture2D(ID3D11Texture2D* texture2D)
{
    if (texture2D)
    {
        EBUS_EVENT(Render::Debug::VRAMDrillerBus, UnregisterAllocation, static_cast<void*>(texture2D));
        texture2D->Release();
    }
}

HRESULT CDeviceManager::CreateD3D11Buffer(const D3D11_BUFFER_DESC* desc, const D3D11_SUBRESOURCE_DATA* initialData, D3DBuffer** buffer, const char* bufferName)
{
    HRESULT hr = gcpRendD3D->GetDevice().CreateBuffer(desc, initialData, (ID3D11Buffer**)buffer);

    if (hr == S_OK)
    {
#if !defined(RELEASE) && defined(WIN64)
        (*buffer)->SetPrivateData(WKPDID_D3DDebugObjectName, strlen(bufferName), bufferName);
#endif

        Render::Debug::VRAMAllocationSubcategory subcategory = Render::Debug::VRAM_SUBCATEGORY_BUFFER_OTHER_BUFFER;
        if (desc->BindFlags & D3D11_BIND_VERTEX_BUFFER)
        {
            subcategory = Render::Debug::VRAM_SUBCATEGORY_BUFFER_VERTEX_BUFFER;
        }
        else if (desc->BindFlags & D3D11_BIND_INDEX_BUFFER)
        {
            subcategory = Render::Debug::VRAM_SUBCATEGORY_BUFFER_INDEX_BUFFER;
        }
        else if (desc->BindFlags & D3D11_BIND_CONSTANT_BUFFER)
        {
            subcategory = Render::Debug::VRAM_SUBCATEGORY_BUFFER_CONSTANT_BUFFER;
        }

        void* address = static_cast<void*>(*buffer);
        size_t byteSize = desc->ByteWidth;
        EBUS_EVENT(Render::Debug::VRAMDrillerBus, RegisterAllocation, address, byteSize, bufferName, Render::Debug::VRAM_CATEGORY_BUFFER, subcategory);
    }
    else
    {
        AZ_Warning("Rendering", false, "CreateD3D11Buffer for %s failed! [0x%08x]", bufferName, hr);
    }

    return hr;
}

void CDeviceManager::ReleaseD3D11Buffer(D3DBuffer* buffer)
{
    if (buffer)
    {
        EBUS_EVENT(Render::Debug::VRAMDrillerBus, UnregisterAllocation, static_cast<void*>(buffer));
        buffer->Release();
    }
}

void* CDeviceManager::GetBackingStorage([[maybe_unused]] D3DBuffer* buffer)
{
#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION DEVICEMANAGER_CPP_SECTION_5
    #include AZ_RESTRICTED_FILE(DeviceManager_cpp)
# endif
    return NULL;
}
void CDeviceManager::FreebackingStorage([[maybe_unused]] void* base_ptr)
{
    FUNCTION_PROFILER(gEnv->pSystem, PROFILE_RENDERER);
#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION DEVICEMANAGER_CPP_SECTION_6
    #include AZ_RESTRICTED_FILE(DeviceManager_cpp)
# endif
}

HRESULT CDeviceManager::CreateFence(DeviceFenceHandle& query)
{
    HRESULT hr = S_FALSE;
# if CRY_USE_DX12
    query = reinterpret_cast<DeviceFenceHandle>(new UINT64);
    hr = query ? S_OK : S_FALSE;
#   define AZ_RESTRICTED_SECTION_IMPLEMENTED
#elif defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION DEVICEMANAGER_CPP_SECTION_7
    #include AZ_RESTRICTED_FILE(DeviceManager_cpp)
# endif
# if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#   undef AZ_RESTRICTED_SECTION_IMPLEMENTED
# else
    D3D11_QUERY_DESC QDesc;
    QDesc.Query = D3D11_QUERY_EVENT;
    QDesc.MiscFlags = 0;
    D3DQuery* d3d_query;
    hr = gcpRendD3D->GetDevice().CreateQuery(&QDesc, &d3d_query);
    if (CHECK_HRESULT(hr))
    {
        query = reinterpret_cast<DeviceFenceHandle>(d3d_query);
    }
# endif
    if (!FAILED(hr))
    {
        IssueFence(query);
    }
    return hr;
}
HRESULT CDeviceManager::ReleaseFence(DeviceFenceHandle query)
{
    HRESULT hr = S_FALSE;
# if CRY_USE_DX12
    delete reinterpret_cast<UINT64*>(query);
    hr = S_OK;
#   define AZ_RESTRICTED_SECTION_IMPLEMENTED
#elif defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION DEVICEMANAGER_CPP_SECTION_8
    #include AZ_RESTRICTED_FILE(DeviceManager_cpp)
# endif
# if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#   undef AZ_RESTRICTED_SECTION_IMPLEMENTED
# else
    D3DQuery* d3d_query = reinterpret_cast<D3DQuery*>(query);
    SAFE_RELEASE(d3d_query);
    hr = S_OK;
# endif
    return hr;
}
HRESULT CDeviceManager::IssueFence(DeviceFenceHandle query)
{
    HRESULT hr = S_FALSE;
# if CRY_USE_DX12
    UINT64* handle = reinterpret_cast<UINT64*>(query);
    if (handle)
    {
        *handle = gcpRendD3D->GetDeviceContext().InsertFence();
        //  gcpRendD3D->GetDeviceContext().Flush();
    }
#   define AZ_RESTRICTED_SECTION_IMPLEMENTED
#elif defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION DEVICEMANAGER_CPP_SECTION_9
    #include AZ_RESTRICTED_FILE(DeviceManager_cpp)
# endif
# if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#   undef AZ_RESTRICTED_SECTION_IMPLEMENTED
# else
    D3DQuery* d3d_query = reinterpret_cast<D3DQuery*>(query);
    if (d3d_query)
    {
        gcpRendD3D->GetDeviceContext().End(d3d_query);
        hr = S_OK;
    }
# endif
    return hr;
}

HRESULT CDeviceManager::SyncFence(DeviceFenceHandle query, bool block, [[maybe_unused]] bool flush)
{
    HRESULT hr = S_FALSE;
# if CRY_USE_DX12
    UINT64* handle = reinterpret_cast<UINT64*>(query);
    if (handle)
    {
        hr = gcpRendD3D->GetDeviceContext().TestForFence(*handle);
        if (hr != S_OK)
        {
            if (flush)
            {
                AZ_Assert(GetCurrentThreadId() == gRenDev->m_pRT->m_nRenderThread, "Must flush in render thread!");
                gcpRendD3D->GetDeviceContext().Flush();
            }

            if (block)
            {
                hr = gcpRendD3D->GetDeviceContext().WaitForFence(*handle);
            }
        }
    }
#   define AZ_RESTRICTED_SECTION_IMPLEMENTED
#elif defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION DEVICEMANAGER_CPP_SECTION_10
    #include AZ_RESTRICTED_FILE(DeviceManager_cpp)
# endif
# if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#   undef AZ_RESTRICTED_SECTION_IMPLEMENTED
# else
    D3DQuery* d3d_query = reinterpret_cast<D3DQuery*>(query);
    if (d3d_query)
    {
        BOOL bQuery = false;
        do
        {
            hr = gcpRendD3D->GetDeviceContext().GetData(d3d_query, (void*)&bQuery, sizeof(BOOL), flush ? 0 : D3D11_ASYNC_GETDATA_DONOTFLUSH);
#     if !defined(_RELEASE)
            if (hr != S_OK && hr != S_FALSE)
            {
                CHECK_HRESULT(hr);

                if (hr == DXGI_ERROR_DEVICE_REMOVED)
                {
                    // If the device has been removed we will be stuck here if this is a blocking sync (i.e. block == true) ).
                    // It's a critical error, though, so we have to bail out and handle it elsewhere.
                    ID3D11Device* device = nullptr;
                    gcpRendD3D->GetDeviceContext().GetDevice(&device);
                    if (device)
                    {
                        const HRESULT removedReason = device->GetDeviceRemovedReason();
                        CryWarning(VALIDATOR_MODULE_RENDERER, VALIDATOR_WARNING, "Graphical device was removed for the following reason: %x", removedReason);
                    }
                    return hr;
                }
            }
#     endif
        } while (block && hr != S_OK && hr != E_FAIL);
    }
# endif
    return hr;
}

HRESULT CDeviceManager::InvalidateCpuCache([[maybe_unused]] void* buffer_ptr, [[maybe_unused]] size_t size, [[maybe_unused]] size_t offset)
{
#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION DEVICEMANAGER_CPP_SECTION_11
    #include AZ_RESTRICTED_FILE(DeviceManager_cpp)
# endif
    return S_OK;
}

HRESULT CDeviceManager::InvalidateGpuCache([[maybe_unused]] D3DBuffer* buffer, [[maybe_unused]] void* buffer_ptr, [[maybe_unused]] size_t size, [[maybe_unused]] size_t offset)
{
#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION DEVICEMANAGER_CPP_SECTION_12
    #include AZ_RESTRICTED_FILE(DeviceManager_cpp)
# endif
    return S_OK;
}

HRESULT CDeviceManager::CreateDirectAccessBuffer(uint32 nSize, uint32 elemSize, int32 nBindFlags, D3DBuffer** ppBuff)
{
    int32 nUsage =
        CDeviceManager::USAGE_CPU_WRITE
        | CDeviceManager::USAGE_DIRECT_ACCESS
        | CDeviceManager::USAGE_DIRECT_ACCESS_CPU_COHERENT
        | CDeviceManager::USAGE_DIRECT_ACCESS_GPU_COHERENT;

    // under dx12 there is direct access, but through the dynamic-usage flag
#ifdef CRY_USE_DX12
    nUsage |= CDeviceManager::USAGE_DYNAMIC;
    nUsage |= CDeviceManager::USAGE_CPU_WRITE;
#endif

    // if no direct access is available, let the driver handle preventing writing to VMEM while it is in use
#if BUFFER_ENABLE_DIRECT_ACCESS == 0
    nUsage |= CDeviceManager::USAGE_DYNAMIC;
#endif
#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION DEVICEMANAGER_CPP_SECTION_13
    #include AZ_RESTRICTED_FILE(DeviceManager_cpp)
#endif

    return CreateBuffer(nSize, elemSize, nUsage, nBindFlags, ppBuff);
}

HRESULT CDeviceManager::DestroyDirectAccessBuffer(D3DBuffer* ppBuff)
{
    void* address = static_cast<void*>(ppBuff);
    EBUS_EVENT(Render::Debug::VRAMDrillerBus, UnregisterAllocation, address);

    return ppBuff->Release();
}

HRESULT CDeviceManager::LockDirectAccessBuffer(D3DBuffer* pBuff, [[maybe_unused]] int32 nBindFlags, void** pBuffer)
{
    HRESULT hr = D3D_OK;
    // get correct locking flags
#if BUFFER_ENABLE_DIRECT_ACCESS == 1
    uint8* ptr;
    ExtractBasePointer(pBuff, ptr);
    *pBuffer = reinterpret_cast<void*>(ptr);
# else
    D3D11_MAP nLockFlags = D3D11_MAP_WRITE_DISCARD;
    D3D11_MAPPED_SUBRESOURCE mappedResource;
    memset(&mappedResource, 0x0, sizeof(mappedResource));

    hr = gcpRendD3D->GetDeviceContext().Map(pBuff, 0, nLockFlags, 0, &mappedResource);
    IF (FAILED(hr), 0)
    {
        CHECK_HRESULT(hr);
        return 0;
    }
    *pBuffer = mappedResource.pData;
# endif
    return hr;
}

void CDeviceManager::UnlockDirectAccessBuffer([[maybe_unused]] D3DBuffer* pBuff, [[maybe_unused]] int32 nBindFlags)
{
# if BUFFER_ENABLE_DIRECT_ACCESS == 0
    gcpRendD3D->GetDeviceContext().Unmap(pBuff, 0);
# endif
}

void CDeviceManager::InvalidateBuffer([[maybe_unused]] D3DBuffer* buffer, [[maybe_unused]] void* base_ptr, [[maybe_unused]] size_t offset, [[maybe_unused]] size_t size, [[maybe_unused]] uint32 id)
{
#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION DEVICEMANAGER_CPP_SECTION_14
    #include AZ_RESTRICTED_FILE(DeviceManager_cpp)
# endif
}

ILINE bool CDeviceManager::ValidateDrawcall()
{
    if (CHWShader_D3D::s_nActivationFailMask != 0)
    {
        /*
            A draw call is allowed to fail if we're currently processing
            shader compilation asynchronously. If shader jobs are running
            we don't want to count an "invalid" draw call because this is 
            expected behavior. A draw call is only invalid if there are no 
            shader jobs running.
        */
        if (gRenDev->m_cEF.m_ShaderCacheStats.m_nNumShaderAsyncCompiles == 0)
        {
            ++m_numInvalidDrawcalls;
        }
        return false;
    }

    return true;
}

void CDeviceManager::Draw(uint32 nVerticesCount, uint32 nStartVertex)
{
    if (!ValidateDrawcall())
    {
        return;
    }

    CommitDeviceStates();
    gcpRendD3D->GetDeviceContext().Draw(nVerticesCount, nStartVertex);
    SyncToGPU();
}

void CDeviceManager::DrawInstanced(uint32 nInstanceVerts, uint32 nInstances, uint32 nStartVertex, uint32 nStartInstance)
{
    if (!ValidateDrawcall())
    {
        return;
    }

    CommitDeviceStates();
    gcpRendD3D->GetDeviceContext().DrawInstanced(nInstanceVerts, nInstances, nStartVertex, nStartInstance);
    SyncToGPU();
}

void CDeviceManager::DrawIndexedInstanced(uint32 numIndices, uint32 nInsts, uint32 startIndex, uint32 v0, uint32 v1)
{
    if (!ValidateDrawcall())
    {
        return;
    }

    CommitDeviceStates();
    gcpRendD3D->GetDeviceContext().DrawIndexedInstanced(numIndices, nInsts, startIndex, v0, v1);
    SyncToGPU();
}

void CDeviceManager::DrawIndexed(uint32 numIndices, uint32 startIndex, uint32 vbOffset)
{
    if (!ValidateDrawcall())
    {
        return;
    }

    CommitDeviceStates();
    gcpRendD3D->GetDeviceContext().DrawIndexed(numIndices, startIndex, vbOffset);
    SyncToGPU();
}


void CDeviceManager::Dispatch(uint32 dX, uint32 dY, uint32 dZ)
{
    if (!ValidateDrawcall())
    {
        return;
    }

    CommitDeviceStates();
    gcpRendD3D->GetDeviceContext().Dispatch(dX, dY, dZ);
    SyncToGPU();
}

void CDeviceManager::DispatchIndirect(D3DBuffer* pBufferForArgs, uint32 AlignedOffsetForArgs)
{
    if (!ValidateDrawcall())
    {
        return;
    }

    CommitDeviceStates();
    gcpRendD3D->GetDeviceContext().DispatchIndirect(pBufferForArgs, AlignedOffsetForArgs);
    SyncToGPU();
}

void CDeviceManager::DrawIndexedInstancedIndirect(D3DBuffer* pBufferForArgs, uint32 AlignedOffsetForArgs)
{
    if (!ValidateDrawcall())
    {
        return;
    }

    CommitDeviceStates();
    gcpRendD3D->GetDeviceContext().DrawIndexedInstancedIndirect(pBufferForArgs, AlignedOffsetForArgs);
    SyncToGPU();
}

void CDeviceManager::BindPlatformConstantBuffer(
    EHWShaderClass type,
    D3DBuffer* platformBuffer,
    AZ::u32 slot)
{
#if DEVICE_MANAGER_IMMEDIATE_STATE_WRITE
    D3DDeviceContext& rDeviceContext = gcpRendD3D->GetDeviceContext();
    switch (type)
    {
    case eHWSC_Vertex:
        rDeviceContext.VSSetConstantBuffers(slot, 1, &platformBuffer);
        break;
    case eHWSC_Pixel:
        rDeviceContext.PSSetConstantBuffers(slot, 1, &platformBuffer);
        break;
    case eHWSC_Geometry:
        rDeviceContext.GSSetConstantBuffers(slot, 1, &platformBuffer);
        break;
    case eHWSC_Domain:
        rDeviceContext.DSSetConstantBuffers(slot, 1, &platformBuffer);
        break;
    case eHWSC_Hull:
        rDeviceContext.HSSetConstantBuffers(slot, 1, &platformBuffer);
        break;
    case eHWSC_Compute:
        rDeviceContext.CSSetConstantBuffers(slot, 1, &platformBuffer);
        break;
    }
#else
    m_CB[type].buffers[slot] = platformBuffer;
    m_CB[type].dirty |= 1 << slot;
#endif
}

void CDeviceManager::BindPlatformConstantBuffer(
    EHWShaderClass type,
    ID3D11Buffer* platformBuffer,
    AZ::u32 slot,
    AZ::u32 registerOffset,
    AZ::u32 registerCount)
{
# if DEVICE_MANAGER_IMMEDIATE_STATE_WRITE
    D3DDeviceContext& rDeviceContext = gcpRendD3D->GetDeviceContext();
    switch (type)
    {
    case eHWSC_Vertex:
        rDeviceContext.VSSetConstantBuffers1(slot, 1, &platformBuffer, &registerOffset, &registerCount);
        break;
    case eHWSC_Pixel:
        rDeviceContext.PSSetConstantBuffers1(slot, 1, &platformBuffer, &registerOffset, &registerCount);
        break;
    case eHWSC_Geometry:
        rDeviceContext.GSSetConstantBuffers1(slot, 1, &platformBuffer, &registerOffset, &registerCount);
        break;
    case eHWSC_Domain:
        rDeviceContext.DSSetConstantBuffers1(slot, 1, &platformBuffer, &registerOffset, &registerCount);
        break;
    case eHWSC_Hull:
        rDeviceContext.HSSetConstantBuffers1(slot, 1, &platformBuffer, &registerOffset, &registerCount);
        break;
    case eHWSC_Compute:
        rDeviceContext.CSSetConstantBuffers1(slot, 1, &platformBuffer, &registerOffset, &registerCount);
        break;
    }
#else
    m_CB[type].buffers1[slot] = platformBuffer;
    m_CB[type].offsets[slot] = registerOffset;
    m_CB[type].sizes[slot] = registerCount;
    m_CB[type].dirty1 |= 1 << slot;
#endif
}

void CDeviceManager::UnbindConstantBuffer(AzRHI::ConstantBuffer* constantBuffer)
{
    if(!constantBuffer || !constantBuffer->GetPlatformBuffer())
    {
        return;
    }

    AZ::u64 myCode = constantBuffer->GetCode();
    for (AZ::u32 shaderStage = 0; shaderStage < eHWSC_Num; ++shaderStage)
    {
        for (AZ::u32 shaderSlot = 0; shaderSlot < eConstantBufferShaderSlot_Count; ++shaderSlot)
        {
            const CDeviceManager::ConstantBufferBindState& bindState = m_constantBufferBindState[shaderStage][shaderSlot];
            if (bindState.m_constantBufferCodes == myCode)
            {
                BindConstantBuffer(static_cast<EHWShaderClass>(shaderStage), nullptr, shaderSlot);
            }
        }
    }

    // Commit device state to immediately unbind the resource because we may be deleting it
    CommitDeviceStates();
}


void CDeviceManager::UnbindSRV(D3DShaderResourceView* shaderResourceView)
{
# if !DEVICE_MANAGER_IMMEDIATE_STATE_WRITE
    if (!shaderResourceView)
    {
        return;
    }

    for (int type = 0; type < EHWShaderClass::eHWSC_Num; ++type)
    {
        for (uint32 slot = 0; slot < MAX_BOUND_SRVS; ++slot)
        {
            if (m_SRV[type].views[slot] == shaderResourceView)
            {
                BindSRV(static_cast<EHWShaderClass>(type), nullptr, slot);
            }
        }
    }
#endif
}

void CDeviceManager::BindConstantBuffer(
    EHWShaderClass type,
    AzRHI::ConstantBuffer* constantBuffer,
    AZ::u32 slot)
{
    BindConstantBuffer(
        type, constantBuffer, slot, 0,
        constantBuffer ? constantBuffer->GetByteCount() : 0);
}

void CDeviceManager::BindConstantBuffer(
    EHWShaderClass type,
    AzRHI::ConstantBuffer* constantBuffer,
    AZ::u32 slot,
    AZ::u32 byteOffset,
    [[maybe_unused]] AZ::u32 byteCount)
{
    AZ::u64 code = 0;
    D3DBuffer* platformBuffer = nullptr;

    if (constantBuffer)
    {
        platformBuffer = constantBuffer->GetPlatformBuffer();
        byteOffset += constantBuffer->GetByteOffset();
        code = constantBuffer->GetCode();
    }

# if defined(DEVICE_SUPPORTS_D3D11_1)
    if ( (m_constantBufferBindState[type][slot].m_constantBufferCodes != code) || (m_constantBufferBindState[type][slot].m_constantBufferBindOffset != byteOffset) )
    {
        m_constantBufferBindState[type][slot].m_constantBufferCodes = code;
        m_constantBufferBindState[type][slot].m_constantBufferBindOffset = byteOffset;

        // Convert bytes to registers
        AZ_Assert((byteOffset & 0xF) == 0, "16 byte alignment required");
        const AZ::u32 registerOffset = byteOffset >> 4;
        const AZ::u32 registerCount = Align(byteCount, 16) >> 4;

        BindPlatformConstantBuffer(type, platformBuffer, slot, registerOffset, registerCount);
    }
#else
    if(m_constantBufferBindState[type][slot].m_constantBufferCodes != code)
    {
        m_constantBufferBindState[type][slot].m_constantBufferCodes = code;

        AZ_Assert(byteOffset == 0, "Offset not supported");
        BindPlatformConstantBuffer(type, platformBuffer, slot);
    }
# endif
}

#if !DEVICE_MANAGER_IMMEDIATE_STATE_WRITE
void CDeviceManager::BindConstantBuffers(EHWShaderClass type, D3DDeviceContext& rDeviceContext)
{
    while (m_CB[type].dirty)
    {
        const unsigned lsb   = m_CB[type].dirty & ~(m_CB[type].dirty - 1);
        const unsigned lbit  = AzRHI::ScanBitsReverse(lsb);
        const unsigned hbit  = AzRHI::ScanBitsForward(~(m_CB[type].dirty) & ~(lsb - 1));
        const unsigned base  = lbit;
        const unsigned count = hbit - lbit;
        switch (type)
        {
        case eHWSC_Vertex:
            rDeviceContext.VSSetConstantBuffers(base, count, &m_CB[eHWSC_Vertex].buffers[base]);
            break;
        case eHWSC_Pixel:
            rDeviceContext.PSSetConstantBuffers(base, count, &m_CB[eHWSC_Pixel].buffers[base]);
            break;
        case eHWSC_Geometry:
            rDeviceContext.GSSetConstantBuffers(base, count, &m_CB[eHWSC_Geometry].buffers[base]);
            break;
        case eHWSC_Domain:
            rDeviceContext.DSSetConstantBuffers(base, count, &m_CB[eHWSC_Domain].buffers[base]);
            break;
        case eHWSC_Hull:
            rDeviceContext.HSSetConstantBuffers(base, count, &m_CB[eHWSC_Hull].buffers[base]);
            break;
        case eHWSC_Compute:
            rDeviceContext.CSSetConstantBuffers(base, count, &m_CB[eHWSC_Compute].buffers[base]);
            break;
        }
        const signed mask = iszero(32 - (signed)hbit) - 1;
        m_CB[type].dirty &= (~((1 << hbit) - (1 << lbit)) & mask);
    }
}

void CDeviceManager::BindOffsetConstantBuffers([[maybe_unused]] EHWShaderClass type, [[maybe_unused]] D3DDeviceContext& rDeviceContext)
{
#   if defined(DEVICE_SUPPORTS_D3D11_1)
    while (m_CB[type].dirty1)
    {
        const unsigned lsb   = m_CB[type].dirty1 & ~(m_CB[type].dirty1 - 1);
        const unsigned lbit  = AzRHI::ScanBitsReverse(lsb);
        const unsigned hbit  = AzRHI::ScanBitsForward(~(m_CB[type].dirty1) & ~(lsb - 1));
        const unsigned base  = lbit;
        const unsigned count = hbit - lbit;
        switch (type)
        {
        case eHWSC_Vertex:
            rDeviceContext.VSSetConstantBuffers1(base, count, &m_CB[eHWSC_Vertex].buffers1[base], &m_CB[eHWSC_Vertex].offsets[base], &m_CB[eHWSC_Vertex].sizes[base]);
            break;
        case eHWSC_Pixel:
            rDeviceContext.PSSetConstantBuffers1(base, count, &m_CB[eHWSC_Pixel].buffers1[base], &m_CB[eHWSC_Pixel].offsets[base], &m_CB[eHWSC_Pixel].sizes[base]);
            break;
        case eHWSC_Geometry:
            rDeviceContext.GSSetConstantBuffers1(base, count, &m_CB[eHWSC_Geometry].buffers1[base], &m_CB[eHWSC_Geometry].offsets[base], &m_CB[eHWSC_Geometry].sizes[base]);
            break;
        case eHWSC_Domain:
            rDeviceContext.DSSetConstantBuffers1(base, count, &m_CB[eHWSC_Domain].buffers1[base], &m_CB[eHWSC_Domain].offsets[base], &m_CB[eHWSC_Domain].sizes[base]);
            break;
        case eHWSC_Hull:
            rDeviceContext.HSSetConstantBuffers1(base, count, &m_CB[eHWSC_Hull].buffers1[base], &m_CB[eHWSC_Hull].offsets[base], &m_CB[eHWSC_Hull].sizes[base]);
            break;
        case eHWSC_Compute:
            rDeviceContext.CSSetConstantBuffers1(base, count, &m_CB[eHWSC_Compute].buffers1[base], &m_CB[eHWSC_Compute].offsets[base], &m_CB[eHWSC_Compute].sizes[base]);
            break;
        }
        const signed mask = iszero(32 - (signed)hbit) - 1;
        m_CB[type].dirty1 &= (~((1 << hbit) - (1 << lbit)) & mask);
    }
# endif // DEVICE_SUPPORTS_D3D11_1
}
void CDeviceManager::BindSamplers(EHWShaderClass type, D3DDeviceContext& rDeviceContext)
{
    while (m_Samplers[type].dirty)
    {
        const unsigned lsb   = m_Samplers[type].dirty & ~(m_Samplers[type].dirty - 1);
        const unsigned lbit  = AzRHI::ScanBitsReverse(lsb);
        const unsigned hbit  = AzRHI::ScanBitsForward(~(m_Samplers[type].dirty) & ~(lsb - 1));
        const unsigned base  = lbit;
        const unsigned count = hbit - lbit;
        switch (type)
        {
        case eHWSC_Vertex:
            rDeviceContext.VSSetSamplers(base, count, &m_Samplers[eHWSC_Vertex].samplers[base]);
            break;
        case eHWSC_Pixel:
            rDeviceContext.PSSetSamplers(base, count, &m_Samplers[eHWSC_Pixel].samplers[base]);
            break;
        case eHWSC_Geometry:
            rDeviceContext.GSSetSamplers(base, count, &m_Samplers[eHWSC_Geometry].samplers[base]);
            break;
        case eHWSC_Domain:
            rDeviceContext.DSSetSamplers(base, count, &m_Samplers[eHWSC_Domain].samplers[base]);
            break;
        case eHWSC_Hull:
            rDeviceContext.HSSetSamplers(base, count, &m_Samplers[eHWSC_Hull].samplers[base]);
            break;
        case eHWSC_Compute:
            rDeviceContext.CSSetSamplers(base, count, &m_Samplers[eHWSC_Compute].samplers[base]);
            break;
        }
        ;
        const signed mask = iszero(32 - (signed)hbit) - 1;
        m_Samplers[type].dirty &= (~((1 << hbit) - (1 << lbit)) & mask);
    }
}

void CDeviceManager::BindSRVs(EHWShaderClass type, D3DDeviceContext& rDeviceContext)
{
    for (size_t j = 0; j < MAX_SRV_DIRTY; ++j)
    {
        while (m_SRV[type].dirty[j])
        {
            const unsigned lsb   = m_SRV[type].dirty[j] & ~(m_SRV[type].dirty[j] - 1);
            const unsigned lbit  = AzRHI::ScanBitsReverse(lsb);
            const unsigned hbit  = AzRHI::ScanBitsForward(~(m_SRV[type].dirty[j]) & ~(lsb - 1));
            const unsigned base  = j * 32 + lbit;
            const unsigned count = hbit - lbit;
            switch (type)
            {
            case eHWSC_Vertex:
                rDeviceContext.VSSetShaderResources(base, count, &m_SRV[type].views[base]);
                break;
            case eHWSC_Pixel:
                rDeviceContext.PSSetShaderResources(base, count, &m_SRV[type].views[base]);
                break;
            case eHWSC_Geometry:
                rDeviceContext.GSSetShaderResources(base, count, &m_SRV[type].views[base]);
                break;
            case eHWSC_Domain:
                rDeviceContext.DSSetShaderResources(base, count, &m_SRV[type].views[base]);
                break;
            case eHWSC_Hull:
                rDeviceContext.HSSetShaderResources(base, count, &m_SRV[type].views[base]);
                break;
            case eHWSC_Compute:
                rDeviceContext.CSSetShaderResources(base, count, &m_SRV[type].views[base]);
                break;
            }
            const signed mask = iszero(32 - (signed)hbit) - 1;
            m_SRV[type].dirty[j] &= (~((1 << hbit) - (1 << lbit)) & mask);
        }
    }
}
void CDeviceManager::BindUAVs(EHWShaderClass type, D3DDeviceContext& rDeviceContext)
{
    for (size_t j = 0; j < MAX_UAV_DIRTY; ++j)
    {
        while (m_UAV[type].dirty[j])
        {
            const unsigned lsb   = m_UAV[type].dirty[j] & ~(m_UAV[type].dirty[j] - 1);
            const unsigned lbit  = AzRHI::ScanBitsReverse(lsb);
            const unsigned hbit  = AzRHI::ScanBitsForward(~(m_UAV[type].dirty[j]) & ~(lsb - 1));
            const unsigned base  = j * 32 + lbit;
            const unsigned count = hbit - lbit;
            switch (type)
            {
            case eHWSC_Vertex:
                assert(0 && "NOT IMPLEMENTED ON D3D11.0");
                break;
            case eHWSC_Pixel:
                rDeviceContext.OMSetRenderTargetsAndUnorderedAccessViews(D3D11_KEEP_RENDER_TARGETS_AND_DEPTH_STENCIL, NULL, NULL, base, count, &m_UAV[type].views[base], &m_UAV[type].counts[base]);
                break;
            case eHWSC_Geometry:
                assert(0 && "NOT IMPLEMENTED ON D3D11.0");
                break;
            case eHWSC_Domain:
                assert(0 && "NOT IMPLEMENTED ON D3D11.0");
                break;
            case eHWSC_Hull:
                assert(0 && "NOT IMPLEMENTED ON D3D11.0");
                break;
            case eHWSC_Compute:
                rDeviceContext.CSSetUnorderedAccessViews(base, count, &m_UAV[type].views[base], &m_UAV[type].counts[base]);
                break;
            }
            const signed mask = iszero(32 - (signed)hbit) - 1;
            m_UAV[type].dirty[j] &= (~((1 << hbit) - (1 << lbit)) & mask);
        }
    }
}
void CDeviceManager::BindIA(D3DDeviceContext& rDeviceContext)
{
    while (m_VBs.dirty)
    {
        const unsigned lsb   = m_VBs.dirty & ~(m_VBs.dirty - 1);
        const unsigned lbit  = AzRHI::ScanBitsReverse(lsb);
        const unsigned hbit  = AzRHI::ScanBitsForward(~(m_VBs.dirty) & ~(lsb - 1));
        const unsigned base  = lbit;
        const unsigned count = hbit - lbit;
        const signed mask = iszero(32 - (signed)hbit) - 1;
        rDeviceContext.IASetVertexBuffers(base, count, &m_VBs.buffers[base], &m_VBs.strides[base], &m_VBs.offsets[base]);
        m_VBs.dirty &= (~((1 << hbit) - (1 << lbit)) & mask);
    }
    if (m_IB.dirty)
    {
        rDeviceContext.IASetIndexBuffer(m_IB.buffer, (DXGI_FORMAT)m_IB.format, m_IB.offset);
        m_IB.dirty = 0;
    }
    if (m_VertexDecl.dirty)
    {
        rDeviceContext.IASetInputLayout(m_VertexDecl.decl);
        m_VertexDecl.dirty = false;
    }
    if (m_Topology.dirty)
    {
        rDeviceContext.IASetPrimitiveTopology(m_Topology.topology);
        m_Topology.dirty = false;
    }
}
void CDeviceManager::BindState(D3DDeviceContext& rDeviceContext)
{
    if (m_RasterState.dirty)
    {
        rDeviceContext.RSSetState(m_RasterState.pRasterizerState);
        m_RasterState.dirty = false;
    }
    if (m_BlendState.dirty)
    {
        rDeviceContext.OMSetBlendState(m_BlendState.pBlendState, m_BlendState.BlendFactor, m_BlendState.SampleMask);
        m_BlendState.dirty = false;
    }
    if (m_DepthStencilState.dirty)
    {
        rDeviceContext.OMSetDepthStencilState(m_DepthStencilState.dss, m_DepthStencilState.stencilref);
        m_DepthStencilState.dirty = false;
    }
}

void CDeviceManager::BindShader(EHWShaderClass type, D3DDeviceContext& rDeviceContext)
{
    if (m_Shaders[type].dirty)
    {
        switch (type)
        {
        case eHWSC_Vertex:
            rDeviceContext.VSSetShader((D3DVertexShader*)m_Shaders[type].shader, NULL, 0);
            break;
        case eHWSC_Pixel:
            rDeviceContext.PSSetShader((D3DPixelShader*)m_Shaders[type].shader, NULL, 0);
            break;
        case eHWSC_Hull:
            rDeviceContext.HSSetShader((ID3D11HullShader*)m_Shaders[type].shader, NULL, 0);
            break;
        case eHWSC_Geometry:
            rDeviceContext.GSSetShader((ID3D11GeometryShader*)m_Shaders[type].shader, NULL, 0);
            break;
        case eHWSC_Domain:
            rDeviceContext.DSSetShader((ID3D11DomainShader*)m_Shaders[type].shader, NULL, 0);
            break;
        case eHWSC_Compute:
            rDeviceContext.CSSetShader((ID3D11ComputeShader*)m_Shaders[type].shader, NULL, 0);
            break;
        }
        m_Shaders[type].dirty = false;
    }
}


void CDeviceManager::CommitDeviceStates()
{
    FUNCTION_PROFILER_LEGACYONLY(gEnv->pSystem, PROFILE_RENDERER);
    AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::RendererDetailed);
    DETAILED_PROFILE_MARKER("CommitDeviceStates");
    {
        DETAILED_PROFILE_MARKER("InvalidateBuffers");
        const uint32 threadid = gRenDev->m_RP.m_nProcessThreadID;
        BufferInvalidationsT& invalidated = m_buffer_invalidations[threadid];

        for (size_t i = 0, end = invalidated.size(); i < end; ++i)
        {
            CDeviceManager::InvalidateGpuCache(
                invalidated[i].buffer,
                invalidated[i].base_ptr,
                invalidated[i].size,
                invalidated[i].offset);
        }

        invalidated.clear();
    }

    {
        DETAILED_PROFILE_MARKER("BindDeviceResources");
        D3DDeviceContext& rDeviceContext = gcpRendD3D->GetDeviceContext();
        BindIA(rDeviceContext);
        BindState(rDeviceContext);
        for (signed i = 0; i < eHWSC_Num; ++i)
        {
            BindShader(static_cast<EHWShaderClass>(i), rDeviceContext);
        }
        for (signed i = 0; i < eHWSC_Num; ++i)
        {
            BindConstantBuffers(static_cast<EHWShaderClass>(i), rDeviceContext);
        }
        for (signed i = 0; i < eHWSC_Num; ++i)
        {
            BindOffsetConstantBuffers(static_cast<EHWShaderClass>(i), rDeviceContext);
        }
        for (signed i = 0; i < eHWSC_Num; ++i)
        {
            BindSRVs(static_cast<EHWShaderClass>(i), rDeviceContext);
        }
        for (signed i = 0; i < eHWSC_Num; ++i)
        {
            BindUAVs((EHWShaderClass)i, rDeviceContext);
        }
        for (signed i = 0; i < eHWSC_Num; ++i)
        {
            BindSamplers(static_cast<EHWShaderClass>(i), rDeviceContext);
        }
    }
}
#endif

void CDeviceManager::SyncToGPU()
{
    if IsCVarConstAccess(constexpr) (CRenderer::CV_r_enable_full_gpu_sync)
    {
        if (m_fence_handle == DeviceFenceHandle() && FAILED(CreateFence(m_fence_handle)))
        {
            CryWarning(VALIDATOR_MODULE_RENDERER, VALIDATOR_WARNING,
                "could not create sync fence");
        }
        if (m_fence_handle)
        {
            IssueFence(m_fence_handle);
            SyncFence(m_fence_handle, true);
        }
    }
}

void CDeviceManager::DisplayMemoryUsage()
{
#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION DEVICEMANAGER_CPP_SECTION_20
    #include AZ_RESTRICTED_FILE(DeviceManager_cpp)
#endif
}

//=============================================================================

int CDeviceTexture::Cleanup()
{
    Unbind();

    // Unregister the VRAM allocation with the VRAM driller
    RemoveFromTextureMemoryTracking();

    int32 nRef = -1;
    if (m_pD3DTexture)
    {
#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION DEVICEMANAGER_CPP_SECTION_18
    #include AZ_RESTRICTED_FILE(DeviceManager_cpp)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#else
        nRef = m_pD3DTexture->Release();
#endif
        m_pD3DTexture = NULL;
    }

#ifdef DEVMAN_USE_STAGING_POOL
    if (m_pStagingResourceDownload)
    {
        gcpRendD3D->m_DevMan.ReleaseStagingResource(m_pStagingResourceDownload);
        m_pStagingResourceDownload = nullptr;
    }

    for (int i = 0; i < NUM_UPLOAD_STAGING_RES; i++)
    {
        if (m_pStagingResourceUpload[i])
        {
            gcpRendD3D->m_DevMan.ReleaseStagingResource(m_pStagingResourceUpload[i]);
            m_pStagingResourceUpload[i] = nullptr;
        }
    }
#endif

#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION DEVICEMANAGER_CPP_SECTION_16
    #include AZ_RESTRICTED_FILE(DeviceManager_cpp)
#endif

    return nRef;
}

CDeviceTexture::~CDeviceTexture()
{
    Cleanup();
}

#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION DEVICEMANAGER_CPP_SECTION_17
    #include AZ_RESTRICTED_FILE(DeviceManager_cpp)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#else
uint32 CDeviceTexture::TextureDataSize(uint32 nWidth, uint32 nHeight, uint32 nDepth, uint32 nMips, uint32 nSlices, const ETEX_Format eTF)
{
    return CTexture::TextureDataSize(nWidth, nHeight, nDepth, nMips, nSlices, eTF);
}
#endif

uint32 CDeviceTexture::TextureDataSize(D3DBaseView* pView)
{
    D3DResource* pResource = nullptr;
    if (pView)
    {
        pView->GetResource(&pResource);
        if (pResource)
        {
            uint32 nWidth = 0;
            uint32 nHeight = 0;
            uint32 nDepth = 0;
            uint32 nMips = 1;
            uint32 nSlices = 1;
            ETEX_Format eTF = eTF_Unknown;

            D3D11_RESOURCE_DIMENSION eResourceDimension = D3D11_RESOURCE_DIMENSION_UNKNOWN;
            pResource->GetType(&eResourceDimension);
            pResource->Release();
            if (eResourceDimension == D3D11_RESOURCE_DIMENSION_BUFFER)
            {
                D3D11_BUFFER_DESC sDesc;
                ((D3DBuffer*)pResource)->GetDesc(&sDesc);
                nWidth = sDesc.ByteWidth;
                nHeight = 1;
                nDepth = 1;
                eTF = eTF_R8;
            }
            else if (eResourceDimension == D3D11_RESOURCE_DIMENSION_TEXTURE1D)
            {
                D3D11_TEXTURE1D_DESC sDesc;
                ((ID3D11Texture1D*)pResource)->GetDesc(&sDesc);
                nWidth = sDesc.Width;
                nHeight = 1;
                nDepth = 1;
                nMips = sDesc.MipLevels;
                nSlices = sDesc.ArraySize;
                eTF = CTexture::TexFormatFromDeviceFormat(sDesc.Format);
            }
            else if (eResourceDimension == D3D11_RESOURCE_DIMENSION_TEXTURE2D)
            {
                D3D11_TEXTURE2D_DESC sDesc;
                ((ID3D11Texture2D*)pResource)->GetDesc(&sDesc);
                nWidth = sDesc.Width;
                nHeight = sDesc.Height;
                nDepth = 1;
                nMips = sDesc.MipLevels;
                nSlices = sDesc.ArraySize;
                eTF = CTexture::TexFormatFromDeviceFormat(sDesc.Format);
            }
            else if (eResourceDimension == D3D11_RESOURCE_DIMENSION_TEXTURE3D)
            {
                D3D11_TEXTURE3D_DESC sDesc;
                ((ID3D11Texture3D*)pResource)->GetDesc(&sDesc);
                nWidth = sDesc.Width;
                nHeight = sDesc.Height;
                nDepth = sDesc.Depth;
                nMips = sDesc.MipLevels;
                eTF = CTexture::TexFormatFromDeviceFormat(sDesc.Format);
            }


            return CTexture::TextureDataSize(nWidth, nHeight, nDepth, nMips, nSlices, eTF);
        }
    }

    return 0;
}

uint32 CDeviceTexture::TextureDataSize(D3DBaseView* pView, const uint numRects, const RECT* pRects)
{
    uint32 fullSize = TextureDataSize(pView);
    if (!numRects)
    {
        return fullSize;
    }

    D3DResource* pResource = nullptr;
    pView->GetResource(&pResource);
    if (pResource)
    {
        uint32 nWidth = 0;
        uint32 nHeight = 0;

        D3D11_RESOURCE_DIMENSION eResourceDimension = D3D11_RESOURCE_DIMENSION_UNKNOWN;
        pResource->GetType(&eResourceDimension);
        pResource->Release();
        if (eResourceDimension == D3D11_RESOURCE_DIMENSION_BUFFER)
        {
            D3D11_BUFFER_DESC sDesc;
            ((D3DBuffer*)pResource)->GetDesc(&sDesc);
            nWidth = sDesc.ByteWidth;
            nHeight = 1;
        }
        else if (eResourceDimension == D3D11_RESOURCE_DIMENSION_TEXTURE1D)
        {
            D3D11_TEXTURE1D_DESC sDesc;
            ((ID3D11Texture1D*)pResource)->GetDesc(&sDesc);
            nWidth = sDesc.Width;
            nHeight = 1;
        }
        else if (eResourceDimension == D3D11_RESOURCE_DIMENSION_TEXTURE2D)
        {
            D3D11_TEXTURE2D_DESC sDesc;
            ((ID3D11Texture2D*)pResource)->GetDesc(&sDesc);
            nWidth = sDesc.Width;
            nHeight = sDesc.Height;
        }
        else if (eResourceDimension == D3D11_RESOURCE_DIMENSION_TEXTURE3D)
        {
            D3D11_TEXTURE3D_DESC sDesc;
            ((ID3D11Texture3D*)pResource)->GetDesc(&sDesc);
            nWidth = sDesc.Width;
            nHeight = sDesc.Height;
        }

        // If overlapping rectangles are cleared multiple times is ambiguous
        uint32 fullDim = nWidth * nHeight;
        uint32 rectSize = 0;

        for (uint i = 0; i < numRects; ++i)
        {
            uint32 nW = max(0, int32(min(uint32(pRects->right), nWidth)) - int32(max(0U, uint32(pRects->left))));
            uint32 nH = max(0, int32(min(uint32(pRects->bottom), nHeight)) - int32(max(0U, uint32(pRects->top))));

            uint32 rectDim = nW * nH;

            rectSize += (uint64(fullSize) * uint64(rectDim)) / fullDim;
        }

        return rectSize;
    }

    return 0;
}

void CDeviceTexture::TrackTextureMemory(uint32 usageFlags, const char* name)
{
    AZ_Warning("Rendering", !m_isTracked, "Texture %s already being tracked by the VRAMDriller", name);

    Render::Debug::VRAMAllocationSubcategory subcategory = Render::Debug::VRAM_SUBCATEGORY_TEXTURE_TEXTURE;
    if (usageFlags & (CDeviceManager::USAGE_DEPTH_STENCIL | CDeviceManager::USAGE_RENDER_TARGET | CDeviceManager::USAGE_UNORDERED_ACCESS))
    {
        subcategory = Render::Debug::VRAM_SUBCATEGORY_TEXTURE_RENDERTARGET;
    }
    else if (usageFlags & (CDeviceManager::USAGE_DYNAMIC | CDeviceManager::USAGE_STAGING))
    {
        subcategory = Render::Debug::VRAM_SUBCATEGORY_TEXTURE_DYNAMIC;            
    }
    EBUS_EVENT(Render::Debug::VRAMDrillerBus, RegisterAllocation, this, m_nBaseAllocatedSize, name, Render::Debug::VRAM_CATEGORY_TEXTURE, subcategory);
    m_isTracked = true;
}

void CDeviceTexture::RemoveFromTextureMemoryTracking()
{
    // We cannot naively remove the texture from tracking because dummy device textures are created at times that do not have a memory backing
    if (m_isTracked)
    {
        EBUS_EVENT(Render::Debug::VRAMDrillerBus, UnregisterAllocation, static_cast<void*>(this));
        m_isTracked = false;
    }
} 

D3DResource* CDeviceTexture::GetCurrUploadStagingResource()
{
#ifdef DEVMAN_USE_STAGING_POOL
    int resourceIndex = gcpRendD3D->GetFrameID() % NUM_UPLOAD_STAGING_RES;
    return m_pStagingResourceUpload[resourceIndex];
#else
    return nullptr;
#endif
}

D3DResource* CDeviceTexture::GetCurrDownloadStagingResource()
{
#ifdef DEVMAN_USE_STAGING_POOL
    return m_pStagingResourceDownload;
#else
    return nullptr;
#endif
}

void** CDeviceTexture::GetCurrUploadStagingMemoryPtr()
{
#ifdef DEVMAN_USE_STAGING_POOL
    int resourceIndex = gcpRendD3D->GetFrameID() % NUM_UPLOAD_STAGING_RES;
    return &m_pStagingMemoryUpload[resourceIndex];
#else
    return nullptr;
#endif
}

void** CDeviceTexture::GetCurrDownloadStagingMemoryPtr()
{
#ifdef DEVMAN_USE_STAGING_POOL
    return &m_pStagingMemoryDownload;
#else
    return nullptr;
#endif
}

