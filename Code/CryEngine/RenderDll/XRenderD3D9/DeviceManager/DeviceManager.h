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


#if defined(AZ_RESTRICTED_PLATFORM)
#undef AZ_RESTRICTED_SECTION
#define DEVICEMANAGER_H_SECTION_1 1
#define DEVICEMANAGER_H_SECTION_2 2
#define DEVICEMANAGER_H_SECTION_3 3
#define DEVICEMANAGER_H_SECTION_4 4
#define DEVICEMANAGER_H_SECTION_5 5
#define DEVICEMANAGER_H_SECTION_6 6
#define DEVICEMANAGER_H_SECTION_7 7
#define DEVICEMANAGER_H_SECTION_8 8
#define DEVICEMANAGER_H_SECTION_9 9
#define DEVICEMANAGER_H_SECTION_10 10
#define DEVICEMANAGER_H_SECTION_11 11
#define DEVICEMANAGER_H_SECTION_12 12
#define DEVICEMANAGER_H_SECTION_13 13
#define DEVICEMANAGER_H_SECTION_14 14
#endif

#pragma once

#if !defined(NULL_RENDERER)
#define DEVMAN_USE_STAGING_POOL
#endif

#include "Enums.h"
#include <AzCore/PlatformDef.h>
#ifndef CRY_USE_DX12
#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION DEVICEMANAGER_H_SECTION_1
    #include AZ_RESTRICTED_FILE(DeviceManager_h)
#  endif
#endif

#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION DEVICEMANAGER_H_SECTION_2
    #include AZ_RESTRICTED_FILE(DeviceManager_h)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#else
#define DEVICE_MANAGER_IMMEDIATE_STATE_WRITE 0
#endif

class CDeviceTexture;
namespace AzRHI 
{
    class ConstantBuffer;
}

typedef CDeviceTexture* LPDEVICETEXTURE;
typedef UINT_PTR DeviceFenceHandle;

struct STextureInfoData
{
    const void* pSysMem;
    uint32 SysMemPitch;
    uint32 SysMemSlicePitch;
    ETEX_TileMode SysMemTileMode;
};

struct STextureInfo
{
    uint8 m_nMSAASamples;
    uint8 m_nMSAAQuality;
    STextureInfoData* m_pData;
    STextureInfo()
    {
        m_nMSAASamples = 1;
        m_nMSAAQuality = 0;
        m_pData = NULL;
    }
};

#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION DEVICEMANAGER_H_SECTION_3
    #include AZ_RESTRICTED_FILE(DeviceManager_h)
#endif

//===============================================================================================================

class CDeviceManager
{
public:
    enum
    {
        MAX_BOUND_VBS = 16,
        MAX_BOUND_SRVS = 128,
        MAX_BOUND_UAVS = 64,
        MAX_BOUND_SAMPLERS = 16,
        MAX_SRV_DIRTY = MAX_BOUND_SRVS / 32,
        MAX_UAV_DIRTY = MAX_BOUND_UAVS / 32,
        SRV_DIRTY_SHIFT = 5,
        SRV_DIRTY_MASK = 31,
        UAV_DIRTY_SHIFT = 5,
        UAV_DIRTY_MASK = 31
    };

private:
    friend class CD3DRenderer;
    friend class CDeviceTexture;

#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION DEVICEMANAGER_H_SECTION_4
    #include AZ_RESTRICTED_FILE(DeviceManager_h)
# endif

private:

# ifndef NULL_RENDERER

    struct ConstantBufferBindState
    {
        AZ::u64 m_constantBufferCodes = 0;
        AZ::u32 m_constantBufferBindOffset = 0;
    };

    ConstantBufferBindState m_constantBufferBindState[eHWSC_Num][eConstantBufferShaderSlot_Count];

# if !DEVICE_MANAGER_IMMEDIATE_STATE_WRITE
    // Constant Buffers
    struct
    {
        D3DBuffer* buffers[eConstantBufferShaderSlot_Count];
        D3DBuffer* buffers1[eConstantBufferShaderSlot_Count];
        uint32 offsets[eConstantBufferShaderSlot_Count];
        uint32 sizes[eConstantBufferShaderSlot_Count];
        uint32 dirty;
        uint32 dirty1;
    } m_CB[eHWSC_Num];

    // Shader Resource Views
    struct
    {
        D3DShaderResourceView* views[MAX_BOUND_SRVS];
        uint32 dirty[MAX_SRV_DIRTY];
    } m_SRV[eHWSC_Num];

    // Unordered Access Views
    struct
    {
        D3DUnorderedAccessView* views[MAX_BOUND_UAVS];
        uint32 counts[MAX_BOUND_UAVS];
        uint32 dirty[MAX_UAV_DIRTY];
    } m_UAV[eHWSC_Num];

    // SamplerStates
    struct
    {
        D3DSamplerState* samplers[MAX_BOUND_SAMPLERS];
        uint32 dirty;
    } m_Samplers[eHWSC_Num];

    // VertexBuffers
    struct
    {
        D3DBuffer* buffers[MAX_BOUND_VBS];
        uint32 offsets[MAX_BOUND_VBS];
        uint32 strides[MAX_BOUND_VBS];
        uint32 dirty;
    } m_VBs;

    // IndexBuffer
    struct
    {
        D3DBuffer* buffer;
        uint32 offset;
        int format;
        uint32 dirty;
    } m_IB;

    struct
    {
        D3DVertexDeclaration* decl;
        bool dirty;
    } m_VertexDecl;

    struct
    {
        D3D11_PRIMITIVE_TOPOLOGY topology;
        bool dirty;
    } m_Topology;

    struct
    {
        ID3D11DepthStencilState* dss;
        uint32 stencilref;
        bool dirty;
    } m_DepthStencilState;

    struct
    {
        ID3D11BlendState* pBlendState;
        float BlendFactor[4];
        uint32 SampleMask;
        bool dirty;
    } m_BlendState;

    struct
    {
        ID3D11RasterizerState* pRasterizerState;
        bool dirty;
    } m_RasterState;


    struct
    {
        ID3D11Resource* shader;
        bool dirty;
    } m_Shaders[eHWSC_Num];

    // The buffer invalidations
    struct SBufferInvalidation
    {
        D3DBuffer* buffer;
        void* base_ptr;
        size_t offset;
        size_t size;
        bool operator< (const SBufferInvalidation& other) const
        {
            if (buffer == other.buffer)
            {
                return offset < other.offset;
            }
            return buffer < other.buffer;
        }
        bool operator!= (const SBufferInvalidation& other) const
        {
            return buffer != other.buffer
#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION DEVICEMANAGER_H_SECTION_5
    #include AZ_RESTRICTED_FILE(DeviceManager_h)
#endif
            ;
        }
    };
    typedef std::vector<SBufferInvalidation> BufferInvalidationsT;
    BufferInvalidationsT m_buffer_invalidations[2];
# endif

    uint32 m_numInvalidDrawcalls;

    // Internal handle for debugging
    DeviceFenceHandle m_fence_handle;

    void BindConstantBuffers(EHWShaderClass type, D3DDeviceContext& rDeviceContext);
    void BindOffsetConstantBuffers(EHWShaderClass type, D3DDeviceContext& rDeviceContext);
    void BindSRVs(EHWShaderClass type, D3DDeviceContext& rDeviceContext);
    void BindUAVs(EHWShaderClass type, D3DDeviceContext& rDeviceContext);
    void BindSamplers(EHWShaderClass type, D3DDeviceContext& rDeviceContext);
    void BindShader(EHWShaderClass type, D3DDeviceContext& rDeviceContext);
    void BindState(D3DDeviceContext& rDeviceContext);
    void BindIA(D3DDeviceContext& rDeviceContext);

    bool ValidateDrawcall();
# endif

    void BindPlatformConstantBuffer(
        EHWShaderClass type,
        D3DBuffer* platformBuffer,
        AZ::u32 slot,
        AZ::u32 registerOffset,
        AZ::u32 registerCount);

    void BindPlatformConstantBuffer(
        EHWShaderClass type,
        D3DBuffer* platformBuffer,
        AZ::u32 slot);

public:
    enum
    {
        USAGE_DIRECT_ACCESS              = BIT(0),
        USAGE_DIRECT_ACCESS_CPU_COHERENT = BIT(1),
        USAGE_DIRECT_ACCESS_GPU_COHERENT = BIT(2),
        USAGE_TRANSIENT                  = BIT(5), //This forces Metal runtime to create a special mode buffer. Mapped data is valid during a single frame only and until next map.
        USAGE_TEXTURE_COMPATIBLE         = BIT(15),
        USAGE_MEMORYLESS                 = BIT(16), //Used to tag memoryless textures on ios
        USAGE_DEPTH_STENCIL              = BIT(17),
        USAGE_RENDER_TARGET              = BIT(18),
        USAGE_DYNAMIC                    = BIT(19),
        USAGE_AUTOGENMIPS                = BIT(20),
        USAGE_STREAMING                  = BIT(21),
        USAGE_STAGE_ACCESS               = BIT(22),
        USAGE_STAGING                    = BIT(23),
        USAGE_IMMUTABLE                  = BIT(24),
        USAGE_DEFAULT                    = BIT(25),
        USAGE_CPU_READ                   = BIT(26),
        USAGE_CPU_WRITE                  = BIT(27),
        USAGE_CPU_CACHED_MEMORY          = BIT(28), // This flag is now redundant
        USAGE_UNORDERED_ACCESS           = BIT(29),
        USAGE_ETERNAL                    = BIT(30),
        USAGE_UAV_RWTEXTURE              = BIT(31),

        USAGE_CUSTOM = (USAGE_DEPTH_STENCIL | USAGE_RENDER_TARGET | USAGE_DYNAMIC | USAGE_AUTOGENMIPS)
    };

    enum
    {
        BIND_VERTEX_BUFFER       = BIT(0),
        BIND_INDEX_BUFFER        = BIT(1),
        BIND_CONSTANT_BUFFER     = BIT(2),
        BIND_SHADER_RESOURCE     = BIT(3),
        BIND_STREAM_OUTPUT       = BIT(4),
        BIND_RENDER_TARGET       = BIT(5),
        BIND_DEPTH_STENCIL       = BIT(6),
        BIND_UNORDERED_ACCESS    = BIT(7)
    };


#ifndef NULL_RENDERER
    CDeviceManager();
    ~CDeviceManager();

    void SyncToGPU();

    void Init();
    void RT_Tick();

    HRESULT Create2DTexture(const string& textureName, uint32 nWidth, uint32 nHeight, uint32 nMips, uint32 nArraySize, uint32 nUsage, const ColorF& cClearValue, D3DFormat Format, D3DPOOL Pool, LPDEVICETEXTURE* ppDevTexture, STextureInfo* pTI = NULL, bool bShouldBeCreated = false, int32 nESRAMOffset = -1);
    HRESULT CreateCubeTexture(const string& textureName, uint32 nSize, uint32 nMips, uint32 nArraySize, uint32 nUsage, const ColorF& cClearValue, D3DFormat Format, D3DPOOL Pool, LPDEVICETEXTURE* ppDevTexture, STextureInfo* pTI = NULL, bool bShouldBeCreated = false);
    HRESULT CreateVolumeTexture(const string& textureName, uint32 nWidth, uint32 nHeight, uint32 nDepth, uint32 nMips, uint32 nUsage, const ColorF& cClearValue, D3DFormat Format, D3DPOOL Pool, LPDEVICETEXTURE* ppDevTexture, STextureInfo* pTI = NULL);

    //// Wrappers for legacy code that directly access the DX11 device.  Wrappers manage VRAM tracking as well resource creation/deletion. ////
    HRESULT CreateD3D11Texture2D(const D3D11_TEXTURE2D_DESC* desc, const FLOAT clearValue[4], const D3D11_SUBRESOURCE_DATA* initialData, ID3D11Texture2D** texture2D, const char* textureName);
    void ReleaseD3D11Texture2D(ID3D11Texture2D* texture2D);

    HRESULT CreateD3D11Buffer(const D3D11_BUFFER_DESC* desc, const D3D11_SUBRESOURCE_DATA* initialData, D3DBuffer** buffer, const char* bufferName);
    void ReleaseD3D11Buffer(D3DBuffer* buffer);
    //// End legacy wrappers ////

    HRESULT CreateBuffer(uint32 nSize, uint32 elemSize, int32 nUsage, int32 nBindFlags, D3DBuffer** ppBuff);

    HRESULT CreateFence(DeviceFenceHandle& query);
    HRESULT ReleaseFence(DeviceFenceHandle query);
    HRESULT IssueFence(DeviceFenceHandle query);
    // CRY DX12 temporarily set flush default to true
    HRESULT SyncFence(DeviceFenceHandle query, bool block, bool flush = true);

    static HRESULT InvalidateGpuCache(D3DBuffer* buffer, void* base_ptr, size_t size, size_t offset);
    static HRESULT InvalidateCpuCache(void* base_ptr, size_t size, size_t offset);

    HRESULT CreateDirectAccessBuffer(uint32 nSize, uint32 elemSize, int32 nBindFlags, D3DBuffer** ppBuff);
    HRESULT DestroyDirectAccessBuffer(D3DBuffer* ppBuff);
    HRESULT LockDirectAccessBuffer(D3DBuffer* pBuff, int32 nBindFlags, void** ppBuffer);
    void UnlockDirectAccessBuffer(D3DBuffer* ppBuff, int32 nBindFlags);

    void InvalidateBuffer(D3DBuffer* buffer, void* base_ptr, size_t offset, size_t size, uint32 id);

    void BindConstantBuffer(
        EHWShaderClass type,
        AzRHI::ConstantBuffer* Buffer,
        uint32 slot);

    void BindConstantBuffer(
        EHWShaderClass type,
        AzRHI::ConstantBuffer* Buffer,
        uint32 slot,
        uint32 offset,
        uint32 size);

    // Unbind a constant buffer if it is bound to an active Constant buffer slot in hardware
    void UnbindConstantBuffer(AzRHI::ConstantBuffer* constantBuffer);
    void UnbindSRV(D3DShaderResourceView* shaderResourceView);

    inline void BindSRV(EHWShaderClass type, D3DShaderResourceView* SRV, uint32 slot);
    inline void BindSRV(EHWShaderClass type, D3DShaderResourceView** SRV, uint32 start_slot, uint32 count);
    inline void BindUAV(EHWShaderClass type, D3DUnorderedAccessView* UAV, uint32 counts, uint32 slot);
    inline void BindUAV(EHWShaderClass type, D3DUnorderedAccessView** UAV, const uint32* counts, uint32 start_slot, uint32 count);
    inline void BindSampler(EHWShaderClass type, D3DSamplerState* Sampler, uint32 slot);
    inline void BindSampler(EHWShaderClass type, D3DSamplerState** Samplers, uint32 start_slot, uint32 count);
    inline void BindVB(D3DBuffer* VB, uint32 slot, uint32 offset, uint32 stride);
    inline void BindVB(uint32 start, uint32 count, D3DBuffer** Buffers, uint32* offset, uint32* stride);
    inline void BindIB(D3DBuffer* Buffer, uint32 offset, DXGI_FORMAT format);
    inline void BindVtxDecl(D3DVertexDeclaration* decl);
    inline void BindTopology(D3D11_PRIMITIVE_TOPOLOGY top);
    inline void BindShader(EHWShaderClass type, ID3D11Resource* shader);
    inline void SetDepthStencilState(ID3D11DepthStencilState* dss, uint32 stencilref);
    inline void SetBlendState(ID3D11BlendState* pBlendState, float* BlendFactor, uint32 SampleMask);
    inline void SetRasterState(ID3D11RasterizerState* pRasterizerState);
    void CommitDeviceStates();
    void Draw(uint32 nVerticesCount, uint32 nStartVertex);
    void DrawInstanced(uint32 nInstanceVerts, uint32 nInsts, uint32 nStartVertex, uint32 nStartInstance);
    void DrawIndexed(uint32, uint32, uint32);
    void DrawIndexedInstanced(uint32 numIndices, uint32 nInsts, uint32 startIndex, uint32 v0, uint32 v1);
    void DrawIndexedInstancedIndirect(D3DBuffer*, uint32);
    void Dispatch(uint32, uint32, uint32);
    void DispatchIndirect(D3DBuffer*, uint32);

#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION DEVICEMANAGER_H_SECTION_6
    #include AZ_RESTRICTED_FILE(DeviceManager_h)
#endif
    uint32 GetNumInvalidDrawcalls() const
    {
        return m_numInvalidDrawcalls;
    }

    // On UMA system, return the pointer to the start of the buffer
    static void ExtractBasePointer(D3DBuffer* buffer, uint8*& base_ptr);

    static void* GetBackingStorage(D3DBuffer* buffer);
    static void FreebackingStorage(void* base_ptr);

    void DisplayMemoryUsage();

#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION DEVICEMANAGER_H_SECTION_7
    #include AZ_RESTRICTED_FILE(DeviceManager_h)
#endif
#endif

#if defined(DEVMAN_USE_STAGING_POOL)
    D3DResource* AllocateStagingResource(D3DResource* pForTex, bool bUpload);
    void ReleaseStagingResource(D3DResource* pStagingTex);

#if !defined(CRY_USE_DX12)
private:
    struct StagingTextureDef
    {
        D3D11_TEXTURE2D_DESC desc;
        D3DTexture* pStagingTexture;

        friend bool operator == (const StagingTextureDef& a, const D3D11_TEXTURE2D_DESC& b)
        {
            return memcmp(&a.desc, &b, sizeof(b)) == 0;
        }
    };

    typedef std::vector<StagingTextureDef> StagingPoolVec;

private:
    StagingPoolVec m_stagingPool;
#endif
#endif

#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION DEVICEMANAGER_H_SECTION_8
    #include AZ_RESTRICTED_FILE(DeviceManager_h)
#endif
};

class CDeviceTexture
{
    friend class CDeviceManager;

#ifndef NULL_RENDERER
    D3DBaseTexture* m_pD3DTexture;
    // for native hand-made textures
    size_t m_nBaseAllocatedSize;
#endif
    // Keep track of the number of subresources we have, for validation purposes and because
    // it can affect our allocation flags (whether or not we need to support partial writes).
    uint32 m_numSubResources;
    bool m_bNoDelete;
    bool m_bCube;
    bool m_isTracked = false;
#ifdef DEVMAN_USE_STAGING_POOL
    bool m_bStagingTextureAllocedOnLock;
    D3DResource* m_pStagingResourceDownload;
    void* m_pStagingMemoryDownload;
    // For uploads, we use a ring buffer so that we can write new resources without blocking the GPU.
    static constexpr int NUM_UPLOAD_STAGING_RES = 3;
    D3DResource* m_pStagingResourceUpload[NUM_UPLOAD_STAGING_RES];
    void* m_pStagingMemoryUpload[NUM_UPLOAD_STAGING_RES];
#endif

#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION DEVICEMANAGER_H_SECTION_9
    #include AZ_RESTRICTED_FILE(DeviceManager_h)
#endif

public:
#ifndef NULL_RENDERER

    void free();

    inline D3DBaseTexture* GetBaseTexture()
    {
        return m_pD3DTexture;
    }

    D3DTexture* Get2DTexture()
    {
        return (D3DTexture*)GetBaseTexture();
    }

    D3DCubeTexture* GetCubeTexture()
    {
        return (D3DCubeTexture*)GetBaseTexture();
    }

    D3DVolumeTexture* GetVolumeTexture()
    {
        return (D3DVolumeTexture*)GetBaseTexture();
    }

    bool IsCube() const
    {
        return m_bCube;
    }

    void Unbind();

    CDeviceTexture ()
        : m_pD3DTexture(NULL)
        , m_nBaseAllocatedSize(0)
        , m_bNoDelete(false)
        , m_bCube(false)
        , m_numSubResources(0)
#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION DEVICEMANAGER_H_SECTION_10
    #include AZ_RESTRICTED_FILE(DeviceManager_h)
#endif
    {
#ifdef DEVMAN_USE_STAGING_POOL
        m_pStagingResourceDownload = nullptr;
        m_pStagingMemoryDownload = nullptr;
        for (int i = 0; i < NUM_UPLOAD_STAGING_RES; i++)
        {
            m_pStagingResourceUpload[i] = nullptr;
            m_pStagingMemoryUpload[i] = nullptr;
        }
#endif
    }
    CDeviceTexture (D3DBaseTexture* pBaseTexture)
        : m_pD3DTexture(pBaseTexture)
        , m_nBaseAllocatedSize(0)
        , m_bNoDelete(false)
        , m_bCube(false)
        , m_numSubResources(0)
#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION DEVICEMANAGER_H_SECTION_11
    #include AZ_RESTRICTED_FILE(DeviceManager_h)
#endif
    {
#ifdef DEVMAN_USE_STAGING_POOL
        m_pStagingResourceDownload = nullptr;
        m_pStagingMemoryDownload = nullptr;
        for (int i = 0; i < NUM_UPLOAD_STAGING_RES; i++)
        {
            m_pStagingResourceUpload[i] = nullptr;
            m_pStagingMemoryUpload[i] = nullptr;
        }
#endif
    }
    CDeviceTexture (D3DCubeTexture* pBaseTexture)
        : m_pD3DTexture(pBaseTexture)
        , m_nBaseAllocatedSize(0)
        , m_bNoDelete(false)
        , m_bCube(true)
        , m_numSubResources(0)
#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION DEVICEMANAGER_H_SECTION_12
    #include AZ_RESTRICTED_FILE(DeviceManager_h)
#endif
    {
#ifdef DEVMAN_USE_STAGING_POOL
        m_pStagingResourceDownload = nullptr;
        m_pStagingMemoryDownload = nullptr;
        for (int i = 0; i < NUM_UPLOAD_STAGING_RES; i++)
        {
            m_pStagingResourceUpload[i] = nullptr;
            m_pStagingMemoryUpload[i] = nullptr;
        }
#endif
    }

    ~CDeviceTexture();

    using StagingHook = ITexture::StagingHook;
    void DownloadToStagingResource(uint32 nSubRes, StagingHook cbTransfer);
    void DownloadToStagingResource(uint32 nSubRes);
    void DownloadToStagingResource();
    void UploadFromStagingResource(uint32 nSubRes, StagingHook cbTransfer);
    void UploadFromStagingResource(uint32 nSubRes);
    void UploadFromStagingResource();
    void AccessCurrStagingResource(uint32 nSubRes, bool forUpload, StagingHook cbTransfer);

    size_t GetDeviceSize() const
    {
        return m_nBaseAllocatedSize;
    }

    static uint32 TextureDataSize(D3DBaseView* pView);
    static uint32 TextureDataSize(D3DBaseView* pView, const uint numRects, const RECT* pRects);

#endif

#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION DEVICEMANAGER_H_SECTION_13
    #include AZ_RESTRICTED_FILE(DeviceManager_h)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#else
    static uint32 TextureDataSize(uint32 nWidth, uint32 nHeight, uint32 nDepth, uint32 nMips, uint32 nSlices, const ETEX_Format eTF);
#endif

#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION DEVICEMANAGER_H_SECTION_14
    #include AZ_RESTRICTED_FILE(DeviceManager_h)
#endif

    void GetMemoryUsage(ICrySizer* pSizer) const
    {
        pSizer->AddObject(this, sizeof(*this));
    }

    int32 Release();
    void SetNoDelete(bool noDelete)
    {
        m_bNoDelete = noDelete;
    }

    void TrackTextureMemory(uint32 usageFlags, const char* name);
    void RemoveFromTextureMemoryTracking();

private:
    CDeviceTexture(const CDeviceTexture&);
    CDeviceTexture& operator = (const CDeviceTexture&);
    int Cleanup();

#ifdef DEVMAN_USE_STAGING_POOL
    D3DResource* GetCurrUploadStagingResource();
    D3DResource* GetCurrDownloadStagingResource();
    D3DResource* GetCurrStagingResource(bool forUpload)
    {
        return forUpload ? GetCurrUploadStagingResource() : GetCurrDownloadStagingResource();
    }

    void** GetCurrUploadStagingMemoryPtr();
    void** GetCurrDownloadStagingMemoryPtr();
    void** GetCurrStagingMemoryPtr(bool forUpload)
    {
        return forUpload ? GetCurrUploadStagingMemoryPtr() : GetCurrDownloadStagingMemoryPtr();
    }
#endif

};
