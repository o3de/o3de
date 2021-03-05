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

// Description : Generic device Buffer management


#include "RenderDll_precompiled.h"
#include <numeric>
#include <IMemory.h>

#include "DriverD3D.h"
#include "DeviceManager/Base.h"
#include "DeviceManager/PartitionTable.h"
#include "AzCore/std/parallel/mutex.h"
#include "Common/Memory/VRAMDrillerBus.h"


#if defined(AZ_RESTRICTED_PLATFORM)
#undef AZ_RESTRICTED_SECTION
#define DEVBUFFER_CPP_SECTION_1 1
#define DEVBUFFER_CPP_SECTION_2 2
#define DEVBUFFER_CPP_SECTION_3 3
#endif

#if defined(min)
# undef min
#endif
#if defined(max)
# undef max
#endif

void ReleaseD3DBuffer(D3DBuffer*& buffer)
{
    if (buffer)
    {
        EBUS_EVENT(Render::Debug::VRAMDrillerBus, UnregisterAllocation, static_cast<void*>(buffer));
        SAFE_RELEASE(buffer);
    }
}

namespace
{
    static inline bool CopyData(void* dst, const void* src, size_t size)
    {
        bool requires_flush = true;
#   if defined(_CPU_SSE)
        if ((((uintptr_t)dst | (uintptr_t)src | size) & 0xf) == 0u)
        {
            __m128* d = (__m128*)dst;
            const __m128* s = (const __m128*)src;
            const __m128* e = (const __m128*)src + (size >> 4);
            while (s < e)
            {
                _mm_stream_ps((float*)(d++), _mm_load_ps((const float*)(s++)));
            }
            _mm_sfence();
            requires_flush = false;
        }
        else
#   endif
        {
            cryMemcpy(dst, src, size, MC_CPU_TO_GPU);
        }
        return requires_flush;
    }

    struct PoolConfig
    {
        enum
        {
            POOL_STAGING_COUNT = 1,
            POOL_ALIGNMENT = 128,
            POOL_FRAME_QUERY_COUNT = 4,
#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION DEVBUFFER_CPP_SECTION_1
    #include AZ_RESTRICTED_FILE(DevBuffer_cpp)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#else
            POOL_MAX_ALLOCATION_SIZE = 64 << 20,
#endif
            POOL_FRAME_QUERY_MASK = POOL_FRAME_QUERY_COUNT - 1
        };

        size_t m_pool_bank_size;
        size_t m_transient_pool_size;
        size_t m_cb_bank_size;
        size_t m_cb_threshold;
        size_t m_pool_bank_mask;
        size_t m_pool_max_allocs;
        size_t m_pool_max_moves_per_update;
        bool m_pool_defrag_static;
        bool m_pool_defrag_dynamic;

        bool Configure()
        {
            m_pool_bank_size = (size_t)NextPower2(gRenDev->CV_r_buffer_banksize) << 20;
            m_transient_pool_size = (size_t)NextPower2(gRenDev->CV_r_transient_pool_size) << 20;
            m_cb_bank_size = NextPower2(gRenDev->CV_r_constantbuffer_banksize) << 20;
            m_cb_threshold = NextPower2(gRenDev->CV_r_constantbuffer_watermark) << 20;
            m_pool_bank_mask = m_pool_bank_size - 1;
            m_pool_max_allocs = gRenDev->CV_r_buffer_pool_max_allocs;
            m_pool_defrag_static = gRenDev->CV_r_buffer_pool_defrag_static != 0;
            m_pool_defrag_dynamic = gRenDev->CV_r_buffer_pool_defrag_dynamic != 0;
            if (m_pool_defrag_static | m_pool_defrag_dynamic)
            {
                m_pool_max_moves_per_update = gRenDev->CV_r_buffer_pool_defrag_max_moves;
            }
            else
            {
                m_pool_max_moves_per_update = 0;
            }
            return true;
        }
    };
    static PoolConfig s_PoolConfig;


    static const char* ConstantToString(BUFFER_USAGE usage)
    {
        switch (usage)
        {
        case BU_IMMUTABLE:
            return "IMMUTABLE";
        case BU_STATIC:
            return "STATIC";
        case BU_DYNAMIC:
            return "DYNAMIC";
        case BU_TRANSIENT:
            return "BU_TRANSIENT";
        case BU_TRANSIENT_RT:
            return "BU_TRANSIENT_RT";
        case BU_WHEN_LOADINGTHREAD_ACTIVE:
            return "BU_WHEN_LOADINGTHREAD_ACTIVE";
        }
        return NULL;
    }

    static const char* ConstantToString(BUFFER_BIND_TYPE type)
    {
        switch (type)
        {
        case BBT_VERTEX_BUFFER:
            return "VB";
        case BBT_INDEX_BUFFER:
            return "IB";
        }
        return NULL;
    }

    static inline int _GetThreadID()
    {
        return gRenDev->m_pRT->IsRenderThread() ? gRenDev->m_RP.m_nProcessThreadID : gRenDev->m_RP.m_nFillThreadID;
    }

    static inline void UnsetStreamSources(D3DBuffer* buffer)
    {
        gcpRendD3D->FX_UnbindStreamSource(buffer);
    }

    struct BufferPoolBank;
    struct BufferPool;
    struct BufferPoolItem;

    //////////////////////////////////////////////////////////////////////////////////////////
    // A backing device buffer serving as a memory bank from which further allocations can be sliced out
    //
    struct BufferPoolBank
    {
        // The pointer to backing device buffer.
        D3DBuffer* m_buffer;

        // Base pointer to buffer (used on platforms with unified memory)
        uint8* m_base_ptr;

        // Size of the backing buffer
        size_t m_capacity;

        // Number of allocated bytes from within the buffer
        size_t m_free_space;

        // Handle into the bank table
        size_t m_handle;

        BufferPoolBank(size_t handle)
            : m_buffer()
            , m_base_ptr(NULL)
            , m_capacity()
            , m_free_space()
            , m_handle(handle)
        {}

        ~BufferPoolBank()
        {
            UnsetStreamSources(m_buffer);
            ReleaseD3DBuffer(m_buffer);
        }
    };

    using BufferPoolBankTable = AzRHI::PartitionTable<BufferPoolBank>;

    //////////////////////////////////////////////////////////////////////////
    // An allocation within a pool bank is represented by this structure
    //
    // Note: In case the allocation request could not be satisfied by a pool
    // the pool item contains a pointer to the backing buffer directly.
    // On destruction the backing device buffer will be released.
    struct BufferPoolItem
    {
        // The pointer to the backing buffer
        D3DBuffer* m_buffer;

        // The pool that maintains this item (will be null if pool-less)
        BufferPool* m_pool;

        // Base pointer to buffer
        uint8* m_base_ptr;

        // The pointer to the defragging allocator if backed by one
        IDefragAllocator* m_defrag_allocator;

        // The intrusive list member for deferred unpinning/deletion
        // Note: only one list because deletion overrides unpinning
        util::list<BufferPoolItem> m_deferred_list;

        // The intrusive list member for deferred relocations
        // due to copy on writes performed on non-renderthreads
        util::list<BufferPoolItem> m_cow_list;

        // The table handle for this item
        item_handle_t m_handle;

        // If this item has been relocated on update, this is the item
        // handle of the new item (to be swapped)
        item_handle_t m_cow_handle;

        // The size of the item in bytes
        uint32 m_size;

        // The offset in bytes from the start of the buffer
        uint32 m_offset;

        // The bank index this item resides in
        uint32 m_bank;

        // The defrag allocation handle for this item
        IDefragAllocator::Hdl m_defrag_handle;

        // Set to one if the item was already used once
        uint8 m_used : 1;

        // Set to one if the item is backed by the defrag allocator (XXXX)
        uint8 m_defrag : 1;

        // Set to one if the item is backed by the defrag allocator (XXXX)
        uint8 m_cpu_flush : 1;

        // Set to one if the item is backed by the defrag allocator (XXXX)
        uint8 m_gpu_flush : 1;

        BufferPoolItem(size_t handle)
            : m_buffer()
            , m_pool()
            , m_size()
            , m_offset(~0u)
            , m_bank(~0u)
            , m_base_ptr(NULL)
            , m_defrag_allocator()
            , m_defrag_handle(IDefragAllocator::InvalidHdl)
            , m_handle(handle)
            , m_deferred_list()
            , m_cow_list()
            , m_cow_handle(~0u)
            , m_used()
            , m_defrag()
            , m_cpu_flush()
            , m_gpu_flush()
        {
        }

        ~BufferPoolItem()
        {
        #ifdef AZRHI_DEBUG
            m_offset = ~0u;
            m_bank = ~0u;
            m_base_ptr = (uint8*)-1;
            m_defrag_handle = IDefragAllocator::InvalidHdl;
        #endif
        }

        void Relocate(BufferPoolItem& item)
        {
            std::swap(m_buffer, item.m_buffer);
            AZRHI_ASSERT(m_pool == item.m_pool);
            AZRHI_ASSERT(m_size == item.m_size);
            std::swap(m_offset, item.m_offset);
            std::swap(m_bank, item.m_bank);
            std::swap(m_base_ptr, item.m_base_ptr);
            if (m_defrag)
            {
                AZRHI_ASSERT(m_defrag_allocator == item.m_defrag_allocator);
                AZRHI_ASSERT(item.m_defrag_handle != m_defrag_handle);
                m_defrag_allocator->ChangeContext(m_defrag_handle, reinterpret_cast<void*>(static_cast<uintptr_t>(item.m_handle)));
                m_defrag_allocator->ChangeContext(item.m_defrag_handle, reinterpret_cast<void*>(static_cast<uintptr_t>(m_handle)));
            }
            std::swap(m_defrag_allocator, item.m_defrag_allocator);
            std::swap(m_defrag_handle, item.m_defrag_handle);
            m_cpu_flush = item.m_cpu_flush;
            m_gpu_flush = item.m_gpu_flush;
        }
    };

    using BufferItemTable = AzRHI::PartitionTable<BufferPoolItem>;

    struct StagingResources
    {
        enum
        {
            WRITE = 0, READ = 1
        };

        D3DBuffer* m_staging_buffers[2];
        size_t m_staged_open[2];
        size_t m_staged_base;
        size_t m_staged_size;
        size_t m_staged_offset;
        D3DBuffer* m_staged_buffer;

        StagingResources() { memset(this, 0x0, sizeof(*this)); }
    };

    template<size_t BIND_FLAGS>
    class StaticBufferUpdaterBase
    {
    protected:
        StagingResources& m_resources;

    public:
        StaticBufferUpdaterBase(StagingResources& resources)
            : m_resources(resources)
        {}
        ~StaticBufferUpdaterBase() { }

        // Create the staging buffers if supported && enabled
        bool CreateResources()
        {
            if (!m_resources.m_staging_buffers[StagingResources::WRITE] && gRenDev->m_DevMan.CreateBuffer(
                s_PoolConfig.m_pool_bank_size
                , 1
                , CDeviceManager::USAGE_CPU_WRITE | CDeviceManager::USAGE_STAGING
                , BIND_FLAGS
                , &m_resources.m_staging_buffers[StagingResources::WRITE]) != S_OK)
            {
                CryLogAlways("SStaticBufferPool::CreateResources: could not create staging buffer");
                FreeResources();
                return false;
            }
            return true;
        }

        bool FreeResources()
        {
            for (size_t i = 0; i < 2; ++i)
            {
                UnsetStreamSources(m_resources.m_staging_buffers[i]);
                SAFE_RELEASE(m_resources.m_staging_buffers[i]);
                m_resources.m_staged_open[i] = 0;
            }
            m_resources.m_staged_base = 0;
            m_resources.m_staged_size = 0;
            m_resources.m_staged_offset = 0;
            m_resources.m_staged_buffer = 0;
            m_resources.m_staged_open[StagingResources::WRITE] = 1;
            return true;
        }

        void* BeginRead(D3DBuffer* buffer, size_t size, size_t offset)
        {
            AZRHI_ASSERT(buffer && size && offset);
            AZRHI_ASSERT(size <= s_PoolConfig.m_pool_bank_size);
            AZRHI_VERIFY(m_resources.m_staged_open[StagingResources::READ] == 0);

            D3D11_BOX contents;
            contents.left = offset;
            contents.right = offset + size;
            contents.top = 0;
            contents.bottom = 1;
            contents.front = 0;
            contents.back = 1;
            gcpRendD3D->GetDeviceContext().CopySubresourceRegion(
                m_resources.m_staging_buffers[StagingResources::READ]
                , 0
                , 0
                , 0
                , 0
                , buffer
                , 0
                , &contents);

            D3D11_MAPPED_SUBRESOURCE mapped_resource;
            D3D11_MAP map = D3D11_MAP_READ;
            HRESULT hr = gcpRendD3D->GetDeviceContext().Map(
                m_resources.m_staging_buffers[StagingResources::READ]
                , 0
                , map
                , 0
                , &mapped_resource);
            if (!CHECK_HRESULT(hr))
            {
                CryLogAlways("map of staging buffer for READING failed!");
                return NULL;
            }
            m_resources.m_staged_open[StagingResources::READ] = 1;
            return mapped_resource.pData;
        }

        void* BeginWrite(D3DBuffer* buffer, size_t size, size_t offset)
        {
            AZRHI_ASSERT(buffer && size);
            AZRHI_ASSERT(size <= s_PoolConfig.m_pool_bank_size);

            D3D11_MAPPED_SUBRESOURCE mapped_resource;
            D3D11_MAP map = D3D11_MAP_WRITE;
            HRESULT hr = gcpRendD3D->GetDeviceContext().Map(
                m_resources.m_staging_buffers[StagingResources::WRITE]
                , 0
                , map
                , 0
                , &mapped_resource);
            if (!CHECK_HRESULT(hr))
            {
                CryLogAlways("map of staging buffer for WRITING failed!");
                return NULL;
            }
            void* result = reinterpret_cast<uint8*>(mapped_resource.pData);
            m_resources.m_staged_size = size;
            m_resources.m_staged_offset = offset;
            m_resources.m_staged_buffer = buffer;
            m_resources.m_staged_open[StagingResources::WRITE] = 1;
            return result;
        }

        inline void EndRead()
        {
            if (m_resources.m_staged_open[StagingResources::READ])
            {
                gcpRendD3D->GetDeviceContext().Unmap(m_resources.m_staging_buffers[StagingResources::READ], 0);
                m_resources.m_staged_open[StagingResources::READ] = 0;
            }
        }

        void EndReadWrite()
        {
            D3D11_BOX contents;
            EndRead();
            if (m_resources.m_staged_open[StagingResources::WRITE])
            {
                AZRHI_ASSERT(m_resources.m_staged_buffer);
                gcpRendD3D->GetDeviceContext().Unmap(m_resources.m_staging_buffers[StagingResources::WRITE], 0);
                contents.left = 0;
                contents.right = m_resources.m_staged_size;
                contents.top = 0;
                contents.bottom = 1;
                contents.front = 0;
                contents.back = 1;
                gcpRendD3D->GetDeviceContext().CopySubresourceRegion(
                    m_resources.m_staged_buffer
                    , 0
                    , m_resources.m_staged_offset
                    , 0
                    , 0
                    , m_resources.m_staging_buffers[StagingResources::WRITE]
                    , 0
                    , &contents);
                m_resources.m_staged_size = 0;
                m_resources.m_staged_offset = 0;
                m_resources.m_staged_buffer = 0;
                m_resources.m_staged_open[StagingResources::WRITE] = 0;
            }
        }

        void Move(
            D3DBuffer* dst_buffer
            , [[maybe_unused]] size_t dst_size
            , size_t dst_offset
            , D3DBuffer* src_buffer
            , size_t src_size
            , size_t src_offset)

        {
            AZRHI_ASSERT(dst_buffer && src_buffer && dst_size == src_size);
#if defined(DEVICE_SUPPORTS_D3D11_1)
            D3D11_BOX contents;
            contents.left = src_offset;
            contents.right = src_offset + src_size;
            contents.top = 0;
            contents.bottom = 1;
            contents.front = 0;
            contents.back = 1;
            gcpRendD3D->GetDeviceContext().CopySubresourceRegion1(
                dst_buffer
                , 0
                , dst_offset
                , 0
                , 0
                , src_buffer
                , 0
                , &contents
                , 0);
#    else
            D3D11_BOX contents;
            contents.left = src_offset;
            contents.right = src_offset + src_size;
            contents.top = 0;
            contents.bottom = 1;
            contents.front = 0;
            contents.back = 1;
            gcpRendD3D->GetDeviceContext().CopySubresourceRegion(
                m_resources.m_staging_buffers[StagingResources::READ]
                , 0
                , 0
                , 0
                , 0
                , src_buffer
                , 0
                , &contents);
#endif
            contents.left = 0;
            contents.right = src_size;
            contents.top = 0;
            contents.bottom = 1;
            contents.front = 0;
            contents.back = 1;
            gcpRendD3D->GetDeviceContext().CopySubresourceRegion(
                dst_buffer
                , 0
                , dst_offset
                , 0
                , 0
                , m_resources.m_staging_buffers[StagingResources::READ]
                , 0
                , &contents);
        }
    };

#ifdef CRY_USE_DX12
    //
    // Override staging path to perform writes over a dedicated upload buffer per bank. This allows
    // mapping as WRITE_NO_OVERWRITE.
    //
    template<size_t BIND_FLAGS>
    class StaticBufferUpdater
        : public StaticBufferUpdaterBase<BIND_FLAGS>
    {
        using Super = StaticBufferUpdaterBase<BIND_FLAGS>;
        D3DBuffer* m_uploadBuffer = nullptr;
    public:
        StaticBufferUpdater(StagingResources& resources)
            : Super(resources)
        {}

        void* BeginWrite(D3DBuffer* buffer, size_t size, size_t offset)
        {
            AZRHI_ASSERT(buffer && size);
            AZRHI_ASSERT(size <= s_PoolConfig.m_pool_bank_size);

            // Use dedicated upload buffer to do staging.
            D3DBuffer* uploadBuffer = static_cast<CCryDX12Buffer*>(buffer)->AcquireUploadBuffer();

            D3D11_MAPPED_SUBRESOURCE mapped_resource;
            HRESULT hr = gcpRendD3D->GetDeviceContext().Map(uploadBuffer, 0, D3D11_MAP_WRITE_NO_OVERWRITE, 0, &mapped_resource);
            if (!CHECK_HRESULT(hr))
            {
                CryLogAlways("map of staging buffer for WRITING failed!");
                return NULL;
            }
            m_uploadBuffer = uploadBuffer;
            this->m_resources.m_staged_size = size;
            this->m_resources.m_staged_offset = offset;
            this->m_resources.m_staged_buffer = buffer;
            this->m_resources.m_staged_open[StagingResources::WRITE] = 1;
            return reinterpret_cast<uint8*>(mapped_resource.pData) + offset;
        }

        void EndReadWrite()
        {
            Super::EndRead();

            D3D11_BOX contents;
            if (this->m_resources.m_staged_open[StagingResources::WRITE])
            {
                AZRHI_ASSERT(this->m_resources.m_staged_buffer);
                gcpRendD3D->GetDeviceContext().Unmap(m_uploadBuffer, 0);
                contents.left = this->m_resources.m_staged_offset;
                contents.right = this->m_resources.m_staged_offset + this->m_resources.m_staged_size;
                contents.top = 0;
                contents.bottom = 1;
                contents.front = 0;
                contents.back = 1;

                gcpRendD3D->GetDeviceContext().CopySubresourceRegion(
                    this->m_resources.m_staged_buffer, 0, this->m_resources.m_staged_offset, 0, 0, m_uploadBuffer, 0, &contents);

                this->m_resources.m_staged_size = 0;
                this->m_resources.m_staged_offset = 0;
                this->m_resources.m_staged_buffer = 0;
                this->m_resources.m_staged_open[StagingResources::WRITE] = 0;
                m_uploadBuffer = nullptr;
            }
        }
    };

#else
    template <size_t BIND_FLAGS>
    class StaticBufferUpdater : public StaticBufferUpdaterBase<BIND_FLAGS>
    {
        using Super = StaticBufferUpdaterBase<BIND_FLAGS>;
    public:
        StaticBufferUpdater(StagingResources& resources)
            : Super(resources)
        {}
    };
#endif

    //////////////////////////////////////////////////////////////////////////
    // Performs buffer updates over dynamic updates
    //
    template<size_t BIND_FLAGS>
    class DynamicBufferUpdater
    {
    private:
        StagingResources& m_resources;
        D3DBuffer* m_locked_buffer;

    public:
        DynamicBufferUpdater(StagingResources& resources)
            : m_resources(resources)
            , m_locked_buffer()
        {}
        ~DynamicBufferUpdater() { }

        bool CreateResources()
        {
            if (!m_resources.m_staging_buffers[StagingResources::READ] &&
                gRenDev->m_DevMan.CreateBuffer(
                    s_PoolConfig.m_pool_bank_size
                    , 1
                    , CDeviceManager::USAGE_DEFAULT
                    , BIND_FLAGS
                    , &m_resources.m_staging_buffers[StagingResources::READ]) != S_OK)
            {
                CryLogAlways("SStaticBufferPool::CreateResources: could not create temporary buffer");
                goto error;
            }
            if (false)
            {
error:
                FreeResources();
                return false;
            }
            return true;
        }

        bool FreeResources()
        {
            UnsetStreamSources(m_resources.m_staging_buffers[StagingResources::READ]);
            SAFE_RELEASE(m_resources.m_staging_buffers[StagingResources::READ]);
            return true;
        }

        void* BeginRead([[maybe_unused]] D3DBuffer* buffer, [[maybe_unused]] size_t size, [[maybe_unused]] size_t offset)
        {
            return NULL;
        }

        void* BeginWrite(D3DBuffer* buffer, [[maybe_unused]] size_t size, size_t offset)
        {
            AZRHI_ASSERT(buffer && size);
            D3D11_MAPPED_SUBRESOURCE mapped_resource;
            D3D11_MAP map = D3D11_MAP_WRITE_NO_OVERWRITE;
            m_locked_buffer = buffer;
#if defined(OPENGL) && !DXGL_FULL_EMULATION
            HRESULT hr = DXGLMapBufferRange(
                    &gcpRendD3D->GetDeviceContext()
                    , m_locked_buffer
                    , offset
                    , size
                    , map
                    , 0
                    , &mapped_resource);
#else
            HRESULT hr = gcpRendD3D->GetDeviceContext().Map(
                    m_locked_buffer
                    , 0
                    , map
                    , 0
                    , &mapped_resource);
#endif
            if (!CHECK_HRESULT(hr))
            {
                CryLogAlways("map of staging buffer for WRITING failed!");
                return NULL;
            }
#if defined(OPENGL) && !DXGL_FULL_EMULATION
            return reinterpret_cast<uint8*>(mapped_resource.pData);
#else
            return reinterpret_cast<uint8*>(mapped_resource.pData) + offset;
#endif
        }

        void EndReadWrite()
        {
            AZRHI_ASSERT(m_locked_buffer || CRenderer::CV_r_buffer_enable_lockless_updates == 1);
            if (m_locked_buffer)
            {
                gcpRendD3D->GetDeviceContext().Unmap(m_locked_buffer, 0);
                m_locked_buffer = NULL;
            }
        }

        void Move(
            D3DBuffer* dst_buffer
            , [[maybe_unused]] size_t dst_size
            , size_t dst_offset
            , D3DBuffer* src_buffer
            , size_t src_size
            , size_t src_offset)

        {
            AZRHI_ASSERT(dst_buffer && src_buffer && dst_size == src_size);
#if defined(DEVICE_SUPPORTS_D3D11_1)
            D3D11_BOX contents;
            contents.left = src_offset;
            contents.right = src_offset + src_size;
            contents.top = 0;
            contents.bottom = 1;
            contents.front = 0;
            contents.back = 1;
            gcpRendD3D->GetDeviceContext().CopySubresourceRegion1(
                dst_buffer
                , 0
                , dst_offset
                , 0
                , 0
                , src_buffer
                , 0
                , &contents
                , 0);
#    else
            D3D11_BOX contents;
            contents.left = src_offset;
            contents.right = src_offset + src_size;
            contents.top = 0;
            contents.bottom = 1;
            contents.front = 0;
            contents.back = 1;
            gcpRendD3D->GetDeviceContext().CopySubresourceRegion(
                m_resources.m_staging_buffers[StagingResources::READ]
                , 0
                , 0
                , 0
                , 0
                , src_buffer
                , 0
                , &contents);
#endif
            contents.left = 0;
            contents.right = src_size;
            contents.top = 0;
            contents.bottom = 1;
            contents.front = 0;
            contents.back = 1;
            gcpRendD3D->GetDeviceContext().CopySubresourceRegion(
                dst_buffer
                , 0
                , dst_offset
                , 0
                , 0
                , m_resources.m_staging_buffers[StagingResources::READ]
                , 0
                , &contents);
        }
    };

    template<size_t BIND_FLAGS>
    class DirectBufferUpdater
    {
    public:
        DirectBufferUpdater([[maybe_unused]] StagingResources& resources)
        {}

        bool CreateResources()
        {
            return true;
        }

        bool FreeResources()
        {
            return true;
        }

        void* BeginRead([[maybe_unused]] D3DBuffer* buffer, [[maybe_unused]] size_t size, [[maybe_unused]] size_t offset)
        {
            return NULL;
        }

        void* BeginWrite([[maybe_unused]] D3DBuffer* buffer, [[maybe_unused]] size_t size, [[maybe_unused]] size_t offset)
        {
            return NULL;
        }

        void EndReadWrite()
        {
        }

        void Move(
            D3DBuffer* dst_buffer
            , [[maybe_unused]] size_t dst_size
            , size_t dst_offset
            , D3DBuffer* src_buffer
            , size_t src_size
            , size_t src_offset)

        {
            AZRHI_ASSERT(dst_buffer && src_buffer && dst_size == src_size);
#if defined(DEVICE_SUPPORTS_D3D11_1)
            D3D11_BOX contents;
            contents.left = src_offset;
            contents.right = src_offset + src_size;
            contents.top = 0;
            contents.bottom = 1;
            contents.front = 0;
            contents.back = 1;
            gcpRendD3D->GetDeviceContext().CopySubresourceRegion1(
                dst_buffer
                , 0
                , dst_offset
                , 0
                , 0
                , src_buffer
                , 0
                , &contents
                , 0);
# else
            D3D11_BOX contents;
            contents.left = src_offset;
            contents.right = src_offset + src_size;
            contents.top = 0;
            contents.bottom = 1;
            contents.front = 0;
            contents.back = 1;
            gcpRendD3D->GetDeviceContext().CopySubresourceRegion(
                dst_buffer
                , 0
                , dst_offset
                , 0
                , 0
                , src_buffer
                , 0
                , &contents);
#endif
        }
    };

    struct DynamicDefragAllocator
    {
        // Instance of the defragging allocator
        IDefragAllocator* m_defrag_allocator;

        // Instance of the defragging allocator policy (if not set, do not perform defragging)
        IDefragAllocatorPolicy* m_defrag_policy;

        // Manages the item storage
        BufferItemTable& m_item_table;

        DynamicDefragAllocator(BufferItemTable& table)
            : m_defrag_allocator()
            , m_defrag_policy()
            , m_item_table(table)
        {}

        ~DynamicDefragAllocator() { AZRHI_VERIFY(m_defrag_allocator == NULL); }

        bool Initialize(IDefragAllocatorPolicy* policy, bool bestFit)
        {
            if (m_defrag_allocator = CryGetIMemoryManager()->CreateDefragAllocator())
            {
                IDefragAllocator::Policy pol;
                pol.pDefragPolicy = m_defrag_policy = policy;
                pol.maxAllocs = ((policy) ? s_PoolConfig.m_pool_max_allocs : 1024);
                pol.maxSegments = 256;
                pol.blockSearchKind = bestFit ? IDefragAllocator::eBSK_BestFit : IDefragAllocator::eBSK_FirstFit;
                m_defrag_allocator->Init(0, PoolConfig::POOL_ALIGNMENT, pol);
            }
            return m_defrag_allocator != NULL;
        }

        bool Shutdown()
        {
            SAFE_RELEASE(m_defrag_allocator);
            return m_defrag_allocator == NULL;
        }

        void GetStats(IDefragAllocatorStats& stats)
        {
            if (m_defrag_allocator)
            {
                stats = m_defrag_allocator->GetStats();
            }
        }

        item_handle_t Allocate(size_t size, BufferPoolItem*& item)
        {
            FUNCTION_PROFILER(gEnv->pSystem, PROFILE_RENDERER);
            AZRHI_VERIFY(size);
            IDefragAllocator::Hdl hdl = m_defrag_allocator->Allocate(size, NULL);
            if (hdl == IDefragAllocator::InvalidHdl)
            {
                return ~0u;
            }
            item_handle_t item_hdl = m_item_table.Allocate();
            item = &m_item_table[item_hdl];
            item->m_size   = size;
            item->m_offset = (uint32)m_defrag_allocator->WeakPin(hdl);
            item->m_defrag_allocator = m_defrag_allocator;
            item->m_defrag_handle = hdl;
            item->m_defrag = true;
            m_defrag_allocator->ChangeContext(hdl, reinterpret_cast<void*>(static_cast<uintptr_t>(item_hdl)));
            return item_hdl;
        }

        void Free(BufferPoolItem* item)
        {
            FUNCTION_PROFILER(gEnv->pSystem, PROFILE_RENDERER);
            
            IF (item->m_defrag_handle != IDefragAllocator::InvalidHdl, 1)
            {
                m_defrag_allocator->Free(item->m_defrag_handle);
            }
            m_item_table.Free(item->m_handle);
        }

        bool Extend(BufferPoolBank* bank) { return m_defrag_allocator->AppendSegment(bank->m_capacity); }

        void Update(uint32 inflight, [[maybe_unused]] uint32 frame_id, bool allow_defragmentation)
        {
            IF (m_defrag_policy && allow_defragmentation, 1)
            {
                m_defrag_allocator->DefragmentTick(s_PoolConfig.m_pool_max_moves_per_update - inflight, s_PoolConfig.m_pool_bank_size);
            }
        }

        void PinItem(BufferPoolItem* item)
        {
            AZRHI_VERIFY((m_defrag_allocator->Pin(item->m_defrag_handle) & s_PoolConfig.m_pool_bank_mask) == item->m_offset);
        }
        void UnpinItem(BufferPoolItem* item)
        {
            m_defrag_allocator->Unpin(item->m_defrag_handle);
        }
    };

    //////////////////////////////////////////////////////////////////////////
    // Partition based allocator for constant buffers of roughly the same size
    struct PartitionAllocator
    {
        D3DBuffer* m_buffer;
        void* m_base_ptr;
        uint32 m_page_size;
        uint32 m_bucket_size;
        uint32 m_partition;
        uint32 m_capacity;

        std::vector<uint32> m_table;
        std::vector<uint32> m_remap;

        PartitionAllocator(
            D3DBuffer* buffer, void* base_ptr, size_t page_size, size_t bucket_size)
            : m_buffer(buffer)
            , m_base_ptr(base_ptr)
            , m_page_size((uint32)page_size)
            , m_bucket_size((uint32)bucket_size)
            , m_partition(0)
            , m_capacity(page_size / bucket_size)
            , m_table()
            , m_remap()
        {
            m_table.resize(page_size / bucket_size);
            m_remap.resize(page_size / bucket_size);
            std::iota(m_table.begin(), m_table.end(), 0);
        }
        ~PartitionAllocator()
        {
            AZRHI_VERIFY(m_partition == 0);
            UnsetStreamSources(m_buffer);
            ReleaseD3DBuffer(m_buffer);
        }

        D3DBuffer* buffer() const { return m_buffer; }
        void* base_ptr() const { return m_base_ptr; }
        bool empty() const { return m_partition == 0; }

        uint32 allocate()
        {
            size_t key = ~0u;
            IF (m_partition + 1 >= m_capacity, 0)
            {
                return ~0u;
            }
            uint32 storage_index = m_table[key = m_partition++];
            m_remap[storage_index] = key;
            return storage_index;
        }

        void deallocate(size_t key)
        {
            AZRHI_ASSERT(m_partition && key < m_remap.size());
            uint32 roster_index = m_remap[key];
            std::swap(m_table[roster_index], m_table[--m_partition]);
            std::swap(m_remap[key], m_remap[m_table[roster_index]]);
        }
    };

} // namespace

    //////////////////////////////////////////////////////////////////////////
    // Special Allocator for constant buffers
    //
# if  CONSTANT_BUFFER_ENABLE_DIRECT_ACCESS
    struct ConstantBufferAllocator
    {
        // The page buckets
        typedef std::vector<PartitionAllocator*> PageBucketsT;
        PageBucketsT m_page_buckets[18];

        // The retired allocations
        typedef std::pair<PartitionAllocator*, uint16> RetiredSlot;
        std::vector<RetiredSlot> m_retired_slots[PoolConfig::POOL_FRAME_QUERY_COUNT];

        // Device fences issues at the end of a frame
        DeviceFenceHandle m_fences[PoolConfig::POOL_FRAME_QUERY_COUNT];

        // Current frameid
        uint32 m_frameid;

        // The number of allocate pages
        uint32 m_pages;

        ConstantBufferAllocator()
            : m_frameid()
            , m_pages()
        {
            memset(m_fences, 0, sizeof(m_fences));
        }
        ~ConstantBufferAllocator() {}

        void ReleaseEmptyBanks()
        {
            if (m_pages * s_PoolConfig.m_cb_bank_size <= s_PoolConfig.m_cb_threshold)
            {
                return;
            }
            FUNCTION_PROFILER_RENDERER;
            for (size_t i = 0; i < 16; ++i)
            {
                for (PageBucketsT::iterator j = m_page_buckets[i].begin(),
                     end = m_page_buckets[i].end(); j != end; )
                {
                    if ((*j)->empty())
                    {
                        delete *j;
                        --m_pages;
                        j = m_page_buckets[i].erase(j);
                        end = m_page_buckets[i].end();
                    }
                    else
                    {
                        ++j;
                    }
                }
            }
        }

        bool Initialize() { return true; }

        bool Shutdown()
        {
            for (size_t i = 0; i < PoolConfig::POOL_FRAME_QUERY_COUNT; ++i)
            {
                for (size_t j = 0; j < m_retired_slots[i].size(); ++j)
                {
                    const RetiredSlot& slot = m_retired_slots[i][j];
                    slot.first->deallocate(slot.second);
                }
                m_retired_slots[i].clear();
            }

            for (size_t i = 0; i < 16; ++i)
            {
                for (size_t j = 0; j < m_page_buckets[i].size(); ++j)
                {
                    delete m_page_buckets[i][j];
                }
                m_page_buckets[i].clear();
            }
            return true;
        }

        bool Allocate(AzRHI::ConstantBuffer* cbuffer)
        {
            FUNCTION_PROFILER(gEnv->pSystem, PROFILE_RENDERER);
            const unsigned size = cbuffer->m_size;
            const unsigned nsize = NextPower2(size);
            const unsigned bucket = IntegerLog2(nsize) - 8;
            bool failed = false;
retry:
            for (size_t i = m_page_buckets[bucket].size(); i > 0; --i)
            {
                unsigned key = m_page_buckets[bucket][i - 1]->allocate();
                if (key != ~0u)
                {
                    cbuffer->m_buffer = m_page_buckets[bucket][i - 1]->buffer();
                    cbuffer->m_base_ptr = m_page_buckets[bucket][i - 1]->base_ptr();
                    cbuffer->m_offset = key * nsize;
                    cbuffer->m_allocator = reinterpret_cast<void*>(m_page_buckets[bucket][i - 1]);
                    return true;
                }
            }
            if (!failed)
            {
                uint8* base_ptr;
                ++m_pages;
                D3DBuffer* buffer = NULL;
                if (gRenDev->m_DevMan.CreateBuffer(
                        s_PoolConfig.m_cb_bank_size
                        , 1
                        , CDeviceManager::USAGE_DIRECT_ACCESS
                        | CDeviceManager::USAGE_DIRECT_ACCESS_CPU_COHERENT
                        | CDeviceManager::USAGE_DIRECT_ACCESS_GPU_COHERENT
#ifdef CRY_USE_DX12
                        // under dx12 there is direct access, but through the dynamic-usage flag
                        | CDeviceManager::USAGE_DYNAMIC
                        | CDeviceManager::USAGE_CPU_WRITE
#endif
                        , CDeviceManager::BIND_CONSTANT_BUFFER
                        , &buffer) != S_OK)
                {
                    CryLogAlways("failed to create constant buffer pool");
                    return false;
                }
                CDeviceManager::ExtractBasePointer(buffer, base_ptr);
                m_page_buckets[bucket].push_back(
                    new PartitionAllocator(buffer, base_ptr, s_PoolConfig.m_cb_bank_size, nsize));
                failed = true;
                goto retry;
            }
            return false;
        }

        void Free(AzRHI::ConstantBuffer* cbuffer)
        {
            const unsigned size = cbuffer->m_size;
            const unsigned nsize = NextPower2(size);
            const unsigned bucket = IntegerLog2(nsize) - 8;
            PartitionAllocator* allocator =
                reinterpret_cast<PartitionAllocator*>(cbuffer->m_allocator);
            m_retired_slots[m_frameid].push_back(
                std::make_pair(allocator, (uint16)(cbuffer->m_offset >> (bucket + 8))));
        }

        void Update(uint32 frame_id, DeviceFenceHandle fence, [[maybe_unused]] bool allow_defragmentation)
        {
            m_frameid = frame_id & PoolConfig::POOL_FRAME_QUERY_MASK;
            for (size_t i = m_frameid; i < m_frameid + PoolConfig::POOL_FRAME_QUERY_COUNT; ++i)
            {
                size_t idx = i & PoolConfig::POOL_FRAME_QUERY_MASK;
                if (m_fences[idx] && gRenDev->m_DevMan.SyncFence(m_fences[idx], false, false) == S_OK)
                {
                    for (size_t j = 0, end = m_retired_slots[idx].size(); j < end; ++j)
                    {
                        const RetiredSlot& slot = m_retired_slots[idx][j];
                        slot.first->deallocate(slot.second);
                    }
                    m_retired_slots[idx].clear();
                }
            }
            m_fences[m_frameid] = fence;
        }
    };
# endif

namespace
{
    struct BufferPool
    {
    protected:
        BufferItemTable m_item_table;
        BufferPoolBankTable m_bank_table;

    public:
        // This lock must be held when operating on the buffers
        SRecursiveSpinLock m_lock;

        BufferPool()
            : m_item_table()
            , m_bank_table()
        {}
        virtual ~BufferPool() {}

        virtual item_handle_t Allocate(size_t) { return ~0u; }
        virtual void Free([[maybe_unused]] BufferPoolItem* item) { }
        virtual bool CreateResources(bool, bool) { return false; }
        virtual bool FreeResources() { return false; }
        virtual bool GetStats(SDeviceBufferPoolStats&) { return false; }
        virtual bool DebugRender() { return false; }
        virtual void Sync() {}
        virtual void Update([[maybe_unused]] uint32 frameId, [[maybe_unused]] DeviceFenceHandle fence, [[maybe_unused]] bool allow_defragmentation) {}
        virtual void ReleaseEmptyBanks() {}
        virtual void* BeginRead([[maybe_unused]] BufferPoolItem* item) { return NULL; }
        virtual void* BeginWrite([[maybe_unused]] BufferPoolItem* item) { return NULL; }
        virtual void  EndReadWrite([[maybe_unused]] BufferPoolItem* item, [[maybe_unused]] bool requires_flush) {}
        virtual void Write([[maybe_unused]] BufferPoolItem* item, [[maybe_unused]] const void* src, [[maybe_unused]] size_t size) { __debugbreak(); }
        BufferPoolItem* Resolve(item_handle_t handle) { return &m_item_table[handle]; }
    };

    template<
        size_t BIND_FLAGS,
        size_t USAGE_FLAGS,
        typename Allocator,
        template<size_t> class Updater,
        size_t ALIGNMENT = PoolConfig::POOL_ALIGNMENT>
    struct BufferPoolImpl
        : public BufferPool
        , private IDefragAllocatorPolicy
    {
        typedef Allocator allocator_t;
        typedef Updater<BIND_FLAGS> updater_t;

        // The item allocator backing this storage
        allocator_t m_allocator;

        // The update strategy implementation
        updater_t m_updater;

        // The list of banks this pool uses
        std::vector<size_t, stl::STLGlobalAllocator<size_t> > m_banks;

        // Deferred items for unpinning && deletion
        struct SDeferredItems
        {
            DeviceFenceHandle m_fence;
            util::list<BufferPoolItem> m_deleted_items;
            SDeferredItems()
                : m_fence()
                , m_deleted_items()
            {}
            ~SDeferredItems()
            {
                AZRHI_ASSERT(m_deleted_items.empty());
            }
        };
        SDeferredItems m_deferred_items[PoolConfig::POOL_FRAME_QUERY_COUNT];

        // The relocation list of all items that need to be relocated at the
        // beginning of the next frame
        util::list<BufferPoolItem> m_cow_relocation_list;

        // The current frame id
        uint32 m_current_frame;

        // The current fence of the device
        DeviceFenceHandle m_current_fence;

        // The current fence of the device
        DeviceFenceHandle m_lockstep_fence;

        // Syncs to gpu should (debugging only)
        void SyncToGPU([[maybe_unused]] bool block)
        {
#     if !defined(_RELEASE)
            if (m_lockstep_fence && block)
            {
                gRenDev->m_DevMan.IssueFence(m_lockstep_fence);
                gRenDev->m_DevMan.SyncFence(m_lockstep_fence, true);
            }
#     endif
        }

        // The list of moves we need to perform
        struct SPendingMove
        {
            IDefragAllocatorCopyNotification* m_notification;
            item_handle_t m_item_handle;
            UINT_PTR m_src_offset;
            UINT_PTR m_dst_offset;
            UINT_PTR m_size;
            DeviceFenceHandle m_copy_fence;
            DeviceFenceHandle m_relocate_fence;
            bool m_moving : 1;
            bool m_relocating : 1;
            bool m_relocated : 1;
            bool m_canceled : 1;

            SPendingMove()
                : m_notification()
                , m_item_handle(~0u)
                , m_src_offset(-1)
                , m_dst_offset(-1)
                , m_size()
                , m_copy_fence()
                , m_relocate_fence()
                , m_moving()
                , m_relocating()
                , m_relocated()
                , m_canceled()
            {}
            ~SPendingMove()
            {
                if (m_copy_fence)
                {
                    gRenDev->m_DevMan.ReleaseFence(m_copy_fence);
                }
                if (m_relocate_fence)
                {
                    gRenDev->m_DevMan.ReleaseFence(m_relocate_fence);
                }
            }
        };
        std::vector<SPendingMove, stl::STLGlobalAllocator<SPendingMove> > m_pending_moves;

        void ProcessPendingMove(SPendingMove& move, bool block)
        {
            bool done = false;
            // Should have finished by now ... soft-sync to fence, if not done, don't finish
            if (move.m_moving)
            {
                if (gRenDev->m_DevMan.SyncFence(move.m_copy_fence, block, block) == S_OK)
                {
                    move.m_notification->bDstIsValid = true;
                    move.m_moving = false;
                }
            }
            // Only finish the relocation by informing the defragger if the gpu has caught up to the
            // point where the new destination has been considered valid
            else if (move.m_relocating)
            {
                if (gRenDev->m_DevMan.SyncFence(move.m_relocate_fence, block, block) == S_OK)
                {
                    move.m_notification->bSrcIsUnneeded = true;
                    move.m_relocating = false;
                    done = true;
                }
            }
            else if (move.m_canceled)
            {
                move.m_notification->bSrcIsUnneeded = true;
                done = true;
            }
            if (done)
            {
                UINT_PTR nDecOffs = move.m_canceled && !move.m_relocated
                    ? move.m_dst_offset
                    : move.m_src_offset;

                {
                    int nSrcBank = nDecOffs / s_PoolConfig.m_pool_bank_size;
                    BufferPoolBank* bank = &m_bank_table[m_banks[nSrcBank]];
                    bank->m_free_space += move.m_size;
                }

                move.m_moving = false;
                move.m_relocating = false;
                move.m_relocated = false;
                move.m_canceled = false;
                move.m_notification = NULL;
            }
        }

        // Creates a new bank for the buffer
        BufferPoolBank* CreateBank()
        {
            FUNCTION_PROFILER_RENDERER;
            // Allocate a new bank
            size_t bank_index = ~0u;
            D3DBuffer* buffer;
            BufferPoolBank* bank = NULL;
            {
                if (gRenDev->m_DevMan.CreateBuffer(
                        s_PoolConfig.m_pool_bank_size
                        , 1
                        , USAGE_FLAGS | CDeviceManager::USAGE_DIRECT_ACCESS
                        , BIND_FLAGS
                        , &buffer) != S_OK)
                {
                    CryLogAlways("SBufferPoolImpl::Allocate: could not allocate additional bank of size %" PRISIZE_T, s_PoolConfig.m_pool_bank_size);
                    return NULL;
                }
            }
            bank = &m_bank_table[bank_index = m_bank_table.Allocate()];
            bank->m_buffer     = buffer;
            bank->m_capacity   = s_PoolConfig.m_pool_bank_size;
            bank->m_free_space = s_PoolConfig.m_pool_bank_size;
            CDeviceManager::ExtractBasePointer(buffer, bank->m_base_ptr);
            m_banks.push_back(bank_index);
            return bank;
        }

        void PrintDebugStats()
        {
            SDeviceBufferPoolStats stats;
            stats.bank_size = s_PoolConfig.m_pool_bank_size;
            for (size_t i = 0, end = m_banks.size(); i < end; ++i)
            {
                const BufferPoolBank& bank = m_bank_table[m_banks[i]];
                stats.num_banks += bank.m_buffer ? 1 : 0;
            }
            m_allocator.GetStats(stats.allocator_stats);
            stats.num_allocs = stats.allocator_stats.nInUseBlocks;

            CryLogAlways("SBufferPoolImpl Stats : %04" PRISIZE_T " num_banks %06" PRISIZE_T " allocations"
                , stats.num_banks, stats.num_allocs);
        }

        // Recreates a previously freed bank
        bool RecreateBank(BufferPoolBank* bank)
        {
            FUNCTION_PROFILER_RENDERER;
            {
                if (gRenDev->m_DevMan.CreateBuffer(
                        s_PoolConfig.m_pool_bank_size
                        , 1
                        , USAGE_FLAGS | CDeviceManager::USAGE_DIRECT_ACCESS
                        , BIND_FLAGS
                        , &bank->m_buffer) != S_OK)
                {
                    CryLogAlways("SBufferPoolImpl::Allocate: could not re-allocate freed bank of size %" PRISIZE_T, s_PoolConfig.m_pool_bank_size);
                    return false;
                }
            }
            CDeviceManager::ExtractBasePointer(bank->m_buffer, bank->m_base_ptr);
            return true;
        }

        void RetireEmptyBanks()
        {
            for (size_t i = 0, end = m_banks.size(); i < end; ++i)
            {
                BufferPoolBank& bank = m_bank_table[m_banks[i]];
                IF (bank.m_capacity != bank.m_free_space, 1)
                {
                    continue;
                }
                UnsetStreamSources(bank.m_buffer);
                ReleaseD3DBuffer(bank.m_buffer);
                bank.m_base_ptr = NULL;
            }
        }

        void RetirePendingFrees(SDeferredItems& deferred)
        {
            for (util::list<BufferPoolItem>* iter = deferred.m_deleted_items.next;
                 iter != &deferred.m_deleted_items; iter = iter->next)
            {
                BufferPoolItem* item = iter->item<& BufferPoolItem::m_deferred_list>();
                BufferPoolBank& bank = m_bank_table[m_banks[item->m_bank]];
                bank.m_free_space += item->m_size;
                m_allocator.Free(item);
            }
            deferred.m_deleted_items.erase();
        }

        void PerformPendingCOWRelocations()
        {
            for (util::list<BufferPoolItem>* iter = m_cow_relocation_list.next;
                 iter != &m_cow_relocation_list; iter = iter->next)
            {
                BufferPoolItem* item = iter->item<& BufferPoolItem::m_deferred_list>();
                BufferPoolItem* new_item = &m_item_table[item->m_cow_handle];

                item->Relocate(*new_item);
                Free(new_item);
                item->m_cow_handle = ~0u;
            }
            m_cow_relocation_list.erase();
        }

        // Implementation of IDefragAllocatorPolicy below
        uint32 BeginCopy(
            void* pContext
            , UINT_PTR dstOffset
            , UINT_PTR srcOffset
            , UINT_PTR size
            , IDefragAllocatorCopyNotification* pNotification)
        {
#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION DEVBUFFER_CPP_SECTION_2
    #include AZ_RESTRICTED_FILE(DevBuffer_cpp)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#elif defined(APPLE)
            // using a C-cast here breaks
            item_handle_t handle = reinterpret_cast<TRUNCATE_PTR>(pContext);
#elif defined(LINUX)
            // using a C-cast here breaks
            item_handle_t handle = reinterpret_cast<TRUNCATE_PTR>(pContext);
#else
            item_handle_t handle = static_cast<item_handle_t>(reinterpret_cast<uintptr_t>(pContext));
#endif

            BufferPoolItem* old_item = &m_item_table[handle];
            BufferPoolBank* bank = NULL;
            size_t pm = ~0u, bank_index;
            for (size_t i = 0; i < m_pending_moves.size(); ++i)
            {
                if (m_pending_moves[i].m_notification != NULL)
                {
                    continue;
                }
                pm = i;
                break;
            }
            if (pm == ~0u)
            {
                return 0;
            }
            old_item = &m_item_table[handle];
            bank_index = (dstOffset / s_PoolConfig.m_pool_bank_size);
            AZRHI_ASSERT(bank_index < m_banks.size());
            bank = &m_bank_table[m_banks[bank_index]];
            // The below should never happen in practice, but who knows for sure, so to be
            // on the safe side we account for the fact that the allocator might want to move
            // an allocation onto an empty bank.
            IF (bank->m_buffer == NULL, 0)
            {
                if (RecreateBank(bank) == false)
                {
                    CryLogAlways("SBufferPoolImpl::Allocate: could not re-allocate freed bank of size %" PRISIZE_T, s_PoolConfig.m_pool_bank_size);
                    return 0;
                }
            }
            bank->m_free_space -= size;

            SPendingMove& pending = m_pending_moves[pm];
            pending.m_notification = pNotification;
            pending.m_item_handle = handle;
            pending.m_src_offset = srcOffset;
            pending.m_dst_offset = dstOffset;
            pending.m_size = size;

            // Perform the actual move in (hopefully) hardware
            m_updater.Move(
                bank->m_buffer
                , size
                , dstOffset & s_PoolConfig.m_pool_bank_mask
                , old_item->m_buffer
                , old_item->m_size
                , old_item->m_offset);

            // Issue a fence so that the copy can be synced
            gRenDev->m_DevMan.IssueFence(pending.m_copy_fence);
            pending.m_moving = true;
            // The move will be considered "done" (bDstIsValid) on the next Update call
            // thanks to r_flush being one, this is always true!
            return pm + 1;
        }
        void Relocate(uint32 userMoveId, [[maybe_unused]] void* pContext, [[maybe_unused]] UINT_PTR newOffset, [[maybe_unused]] UINT_PTR oldOffset, [[maybe_unused]] UINT_PTR size)
        {
            // Swap both items. The previous item will be the new item and will get freed upon
            // the next update loop
            SPendingMove& move = m_pending_moves[userMoveId - 1];
            AZRHI_ASSERT(move.m_relocating == false);
            BufferPoolItem& item = m_item_table[move.m_item_handle];
            BufferPoolBank* bank = &m_bank_table[m_banks[item.m_bank]];
            uint8* old_offset = bank->m_base_ptr + item.m_offset;
            item.m_bank   = move.m_dst_offset / s_PoolConfig.m_pool_bank_size;
            item.m_offset = move.m_dst_offset & s_PoolConfig.m_pool_bank_mask;
            bank = &m_bank_table[m_banks[item.m_bank]];
            item.m_buffer = bank->m_buffer;
            // Issue a fence so that the previous location will only be able
            // to be shelled after this point in terms of gpu execution
            gRenDev->m_DevMan.IssueFence(move.m_relocate_fence);
            move.m_relocating = true;
            move.m_relocated = true;
        }
        void CancelCopy(uint32 userMoveId, [[maybe_unused]] void* pContext, [[maybe_unused]] bool bSync)
        {
            // Remove the move from the list of pending moves, free the destination item
            // as it's not going to be used anymore
            SPendingMove& move = m_pending_moves[userMoveId - 1];
            move.m_canceled = true;
        }
        void SyncCopy([[maybe_unused]] void* pContext, [[maybe_unused]] UINT_PTR dstOffset, [[maybe_unused]] UINT_PTR srcOffset, [[maybe_unused]] UINT_PTR size) { __debugbreak(); }

    public:
        BufferPoolImpl(StagingResources& resources)
            : m_allocator(m_item_table)
            , m_updater(resources)
            , m_banks()
            , m_current_frame()
            , m_current_fence()
            , m_lockstep_fence()
            , m_pending_moves()
        {}
        virtual ~BufferPoolImpl() {}

        bool GetStats(SDeviceBufferPoolStats& stats)
        {
            stats.bank_size = s_PoolConfig.m_pool_bank_size;
            for (size_t i = 0, end = m_banks.size(); i < end; ++i)
            {
                const BufferPoolBank& bank = m_bank_table[m_banks[i]];
                stats.num_banks += bank.m_buffer ? 1 : 0;
            }
            m_allocator.GetStats(stats.allocator_stats);
            stats.num_allocs = stats.allocator_stats.nInUseBlocks;
            return true;
        }

        // Try to satisfy an allocation of a given size from within the pool
        // allocating a new bank if all previously created banks are full
        item_handle_t Allocate(size_t size)
        {
            D3DBuffer* buffer = NULL;
            BufferPoolItem* item = NULL;
            BufferPoolBank* bank = NULL;
            size_t offset = 0u, bank_index = 0u;
            item_handle_t handle;
            bool failed = false;

            // Align the allocation size up to the configured allocation alignment
            size = (max(size, size_t(1u)) + (ALIGNMENT - 1)) & ~(ALIGNMENT - 1);

            // Handle the case where an allocation cannot be satisfied by a pool bank
            // as the size is too large and create a free standing buffer therefore.
            // Note: Care should be taken to reduce the amount of unpooled items!
            IF (size > s_PoolConfig.m_pool_bank_size, 0)
            {
freestanding:
                if (gRenDev->m_DevMan.CreateBuffer(
                        size
                        , 1
                        , USAGE_FLAGS | CDeviceManager::USAGE_DIRECT_ACCESS
                        , BIND_FLAGS
                        , &buffer) != S_OK)
                {
                    CryLogAlways("SBufferPoolImpl::Allocate: could not allocate buffer of size %" PRISIZE_T, size);
                    gEnv->bIsOutOfVideoMemory = true;
                    return ~0u;
                }
                item = &m_item_table[handle = m_item_table.Allocate()];
                item->m_buffer = buffer;
                item->m_pool   = this;
                item->m_offset = 0u;
                item->m_bank   = ~0u;
                item->m_size   = size;
                item->m_defrag_handle = IDefragAllocator::InvalidHdl;
                CDeviceManager::ExtractBasePointer(buffer, item->m_base_ptr);
                return handle;
            }

            // Find a bank that can satisfy the allocation. If none could be found,
            // add an additional bank and retry, if allocations still fail, flag error
retry:
            if ((handle = m_allocator.Allocate(size, item)) != ~0u)
            {
                item->m_pool   = this;
                item->m_bank   = (bank_index = (item->m_offset / s_PoolConfig.m_pool_bank_size));
                item->m_offset &= s_PoolConfig.m_pool_bank_mask;
                AZRHI_ASSERT(bank_index < m_banks.size());
                bank = &m_bank_table[m_banks[bank_index]];
                IF (bank->m_buffer == NULL, 0)
                {
                    if (RecreateBank(bank) == false)
                    {
                        m_allocator.Free(item);
                        return ~0u;
                    }
                }
                item->m_buffer = bank->m_buffer;
                bank->m_free_space -= size;
                return handle;
            }
            if (failed) // already tried once
            {
                CryLogAlways("SBufferPoolImpl::Allocate: could not allocate pool item of size %" PRISIZE_T, size);
                // Try to allocate a free standing buffer now ... fingers crossed
                goto freestanding;
            }
            if ((bank = CreateBank()) == NULL)
            {
                gEnv->bIsOutOfVideoMemory = true;
                return ~0u;
            }
            if (!m_allocator.Extend(bank))
            {
#       ifndef _RELEASE
                CryLogAlways("SBufferPoolImpl::Allocate: WARNING: "
                    "could not extend allocator segment. Performing a free standing allocation!"
                    "(backing allocator might have run out of handles, please check)");
                PrintDebugStats();
#       endif

                // Extending the allocator failed, so the newly created bank is rolled back
                UnsetStreamSources(bank->m_buffer);
                ReleaseD3DBuffer(bank->m_buffer);
                m_bank_table.Free(bank->m_handle);
                m_banks.erase(m_banks.end() - 1);
                // Try to allocate a free standing buffer now ... fingers crossed
                goto freestanding;
            }

            failed = true; // Prevents an infinite loop
            goto retry;
        }

        // Free a previously made allocation
        void Free(BufferPoolItem* item)
        {
            AZRHI_ASSERT(item);
            // Handle un pooled buffers
            IF ((item->m_bank) == ~0u, 0)
            {
                UnsetStreamSources(item->m_buffer);
                ReleaseD3DBuffer(item->m_buffer);
                m_item_table.Free(item->m_handle);
                return;
            }
            item->m_deferred_list.relink_tail(m_deferred_items[m_current_frame].m_deleted_items);
        }

        bool CreateResources(bool enable_defragging, bool best_fit)
        {
            IDefragAllocatorPolicy* defrag_policy = enable_defragging ? this : NULL;
            if (!m_allocator.Initialize(defrag_policy, best_fit))
            {
                CryLogAlways("buffer pool allocator failed to create resources");
                return false;
            }
            if (!m_updater.CreateResources())
            {
                CryLogAlways("Buffer pool updater failed to create resources");
                return false;
            }
            m_pending_moves.resize(s_PoolConfig.m_pool_max_moves_per_update);
            for (size_t i = 0; i < s_PoolConfig.m_pool_max_moves_per_update; ++i)
            {
                if (gRenDev->m_DevMan.CreateFence(m_pending_moves[i].m_copy_fence) != S_OK)
                {
                    CryLogAlways("Could not create buffer pool copy gpu fence");
                    return false;
                }
                if (gRenDev->m_DevMan.CreateFence(m_pending_moves[i].m_relocate_fence) != S_OK)
                {
                    CryLogAlways("Could not create buffer pool relocate fence");
                    return false;
                }
            }
            if (gRenDev->m_DevMan.CreateFence(m_lockstep_fence) != S_OK)
            {
                CryLogAlways("Could not create lockstep debugging fence");
                return false;
            }
            return true;
        }

        bool FreeResources()
        {
            Sync();
            if (m_updater.FreeResources() == false)
            {
                return false;
            }
            if (m_allocator.Shutdown() == false)
            {
                return false;
            }
            for (size_t i = 0, end = m_banks.size(); i < end; ++i)
            {
                m_bank_table.Free((item_handle_t)m_banks[i]);
            }
            if (m_lockstep_fence && gRenDev->m_DevMan.ReleaseFence(m_lockstep_fence) != S_OK)
            {
                return false;
            }
            stl::free_container(m_banks);
            stl::free_container(m_pending_moves);
            return true;
        }

        void ReleaseEmptyBanks() { RetireEmptyBanks(); }

        void Sync()
        {
            for (size_t i = 0, end = m_pending_moves.size(); i < end; ++i)
            {
                SPendingMove& move = m_pending_moves[i];
                if (move.m_notification == NULL)
                {
                    continue;
                }
                ProcessPendingMove(move, true);
            }

            // Update all deferred items
            for (int32 i = 0; i < PoolConfig::POOL_FRAME_QUERY_COUNT; ++i)
            {
                RetirePendingFrees(m_deferred_items[i]);
            }

            PerformPendingCOWRelocations();

            // Free any banks that remained free until now
            RetireEmptyBanks();
        }

        void Update(uint32 frame_id, DeviceFenceHandle fence, bool allow_defragmentation)
        {
            // Loop over the pending moves and update their state accordingly
            uint32 inflight = 0;
            for (size_t i = 0, end = m_pending_moves.size(); i < end; ++i)
            {
                SPendingMove& move = m_pending_moves[i];
                if (move.m_notification == NULL)
                {
                    continue;
                }
                ProcessPendingMove(move, false);
                ++inflight;
            }

            // Update the current deferred items
            m_current_frame = (frame_id + 1) & PoolConfig::POOL_FRAME_QUERY_MASK;
            for (uint32 i = m_current_frame; i < m_current_frame + PoolConfig::POOL_FRAME_QUERY_COUNT; ++i)
            {
                SDeferredItems& deferred = m_deferred_items[i & PoolConfig::POOL_FRAME_QUERY_MASK];
                if (deferred.m_fence && gRenDev->m_DevMan.SyncFence(deferred.m_fence, false, false) != S_OK)
                {
                    continue;
                }
                RetirePendingFrees(deferred);
            }
            m_deferred_items[m_current_frame & PoolConfig::POOL_FRAME_QUERY_MASK].m_fence = fence;
            m_current_fence = fence;

            PerformPendingCOWRelocations();

            // Let the allocator free the items that were retired
            m_allocator.Update(min(inflight, (uint32)s_PoolConfig.m_pool_max_moves_per_update)
                , frame_id, allow_defragmentation);
        }
////////
        // Buffer IO methods
        void* BeginRead(BufferPoolItem* item)
        {
            SyncToGPU(CRenderer::CV_r_enable_full_gpu_sync != 0);

            AZRHI_VERIFY(item->m_used);
            IF (item->m_bank != ~0u, 1)
            {
                m_allocator.PinItem(item);
            }
            IF (item->m_bank != ~0u, 1)
            {
                BufferPoolBank& bank = m_bank_table[m_banks[item->m_bank]];
                IF (bank.m_base_ptr != NULL && CRenderer::CV_r_buffer_enable_lockless_updates, 1)
                {
                    return bank.m_base_ptr + item->m_offset;
                }
            }
            return m_updater.BeginRead(item->m_buffer, item->m_size, item->m_offset);
        }

        void* BeginWrite(BufferPoolItem* item)
        {
            SyncToGPU(CRenderer::CV_r_enable_full_gpu_sync != 0);
            // In case item was previously used and the current last fence can not be
            // synced already we allocate a new item and swap it with the existing one
            // to make sure that we do not contend with the gpu on an already
            // used item's buffer update.
            size_t item_handle = item->m_handle;
            IF (item->m_bank != ~0u, 1)
            {
                m_allocator.PinItem(item);
            }
            IF (item->m_bank != ~0u && item->m_used /*&& gRenDev->m_DevMan.SyncFence(m_current_fence, false) != S_OK*/, 0)
            {
                item_handle_t handle = Allocate(item->m_size);
                if (handle == ~0u)
                {
                    CryLogAlways("failed to allocate new slot on write");
                    return NULL;
                }
                item->m_cow_handle = handle;

                BufferPoolItem* new_item = &m_item_table[handle];
                // Pin the item so that the defragger does not come up with
                // the idea of moving this item because it will be invalidated
                // soon as we are moving the allocation to a pristine location (not used by the gpu).
                // Relocate the old item to the new pristine allocation
                IF (new_item->m_bank != ~0u, 1)
                {
                    m_allocator.PinItem(new_item);
                }

                // Return the memory of the newly allocated item
                item = new_item;
            }
            item->m_used = 1u;
            PREFAST_SUPPRESS_WARNING(6326)
            if constexpr ((USAGE_FLAGS& CDeviceManager::USAGE_DIRECT_ACCESS_CPU_COHERENT) == 0)
            {
                item->m_cpu_flush = 1;
            }
            if constexpr ((USAGE_FLAGS& CDeviceManager::USAGE_DIRECT_ACCESS_GPU_COHERENT) == 0)
            {
                item->m_gpu_flush = 1;
            }
            IF (item->m_bank != ~0u, 1)
            {
                BufferPoolBank& bank = m_bank_table[m_banks[item->m_bank]];
                IF (bank.m_base_ptr != NULL && CRenderer::CV_r_buffer_enable_lockless_updates, 1)
                {
                    return bank.m_base_ptr + item->m_offset;
                }
            }
            return m_updater.BeginWrite(item->m_buffer, item->m_size, item->m_offset);
        }

        void EndReadWrite(BufferPoolItem* item, [[maybe_unused]] bool requires_flush)
        {
            IF (item->m_cow_handle != ~0u, 0)
            {
                BufferPoolItem* new_item = &m_item_table[item->m_cow_handle];
                IF (gRenDev->m_pRT->IsRenderThread(), 1)
                {
                    // As we are now relocating the allocation, we also need
                    // to free the previous allocation
                    item->Relocate(*new_item);
                    Free(new_item);

                    item->m_cow_handle = ~0u;
                }
                else
                {
                    item->m_cow_list.relink_tail(m_cow_relocation_list);
                    item = new_item;
                }
            }
            IF (item->m_bank != ~0u, 1)
            {
                m_allocator.UnpinItem(item);
                if IsCVarConstAccess(constexpr) (CRenderer::CV_r_buffer_enable_lockless_updates)
                {
#         if BUFFER_ENABLE_DIRECT_ACCESS
                    BufferPoolBank* bank = &m_bank_table[m_banks[item->m_bank]];
                    if (item->m_cpu_flush)
                    {
                        if (requires_flush)
                        {
                            CDeviceManager::InvalidateCpuCache(
                                bank->m_base_ptr, item->m_size, item->m_offset);
                        }
                        item->m_cpu_flush = 0;
                    }
                    if (item->m_gpu_flush)
                    {
                        gRenDev->m_DevMan.InvalidateBuffer(
                            bank->m_buffer
                            , bank->m_base_ptr
                            , item->m_offset
                            , item->m_size
                            , _GetThreadID());
                        item->m_gpu_flush = 0;
                    }
#         endif
                }
            }
            m_updater.EndReadWrite();

            SyncToGPU(CRenderer::CV_r_enable_full_gpu_sync != 0);
        }

        void Write(BufferPoolItem* item, const void* src, size_t size)
        {
            AZRHI_ASSERT(size <= item->m_size);

            if (item->m_size <= s_PoolConfig.m_pool_bank_size)
            {
                void* const dst = BeginWrite(item);
                IF (dst, 1)
                {
                    const size_t csize = min((size_t)item->m_size, size);
                    const bool requires_flush = CopyData(dst, src, csize);
                    EndReadWrite(item, requires_flush);
                }
                return;
            }

            AZRHI_ASSERT(item->m_bank == ~0u);
            AZRHI_ASSERT(item->m_cow_handle == ~0u);

            SyncToGPU(gRenDev->CV_r_enable_full_gpu_sync != 0);

            item->m_used = 1u;

            for (size_t offset = 0; offset < size; )
            {
                const size_t sz = min(size - offset, s_PoolConfig.m_pool_bank_size);
                void* const dst = m_updater.BeginWrite(item->m_buffer, sz, item->m_offset + offset);
                IF (dst, 1)
                {
                    const bool requires_flush = CopyData(dst, ((char*)src) + offset, sz);
                }
                m_updater.EndReadWrite();
                offset += sz;
            }

            SyncToGPU(gRenDev->CV_r_enable_full_gpu_sync != 0);
        }
    };

    //////////////////////////////////////////////////////////////////////////////////////
    // SStaticBufferPool A buffer pool for geometry that change infrequently and have a
    // significant lifetime
    //
    // Use this pool for example for :
    //    - streamed static geometry
    //    - geometry that rarely changes
    //
    // Corresponding D3D_USAGE : USAGE_DEFAULT
    // Corresponding update strategy : d3d11 staging buffers (CopySubResource)
    //
    typedef BufferPoolImpl<
        CDeviceManager::BIND_VERTEX_BUFFER
        , CDeviceManager::USAGE_DEFAULT | CDeviceManager::USAGE_DIRECT_ACCESS_CPU_COHERENT
        , DynamicDefragAllocator
  #     if BUFFER_USE_STAGED_UPDATES
        , StaticBufferUpdater
  #     else
        , DirectBufferUpdater
  #     endif
        > StaticBufferPoolVB;
    typedef BufferPoolImpl<
        CDeviceManager::BIND_INDEX_BUFFER
        , CDeviceManager::USAGE_DEFAULT | CDeviceManager::USAGE_DIRECT_ACCESS_CPU_COHERENT
        , DynamicDefragAllocator
  #     if BUFFER_USE_STAGED_UPDATES
        , StaticBufferUpdater
  #     else
        , DirectBufferUpdater
  #     endif
        > StaticBufferPoolIB;

    //////////////////////////////////////////////////////////////////////////////////////
    // SDynamicBufferPool A buffer pool for geometry that can change frequently but rarely
    // changes topology
    //
    // Use this pool for example for :
    //    - deforming geometry that is updated on the CPU
    //    - characters skinned in software
    //
    // Corresponding D3D_USAGE : USAGE_DYNAMIC
    // Corresponding update strategy : NO_OVERWRITE direct map of the buffer
    typedef BufferPoolImpl<
        CDeviceManager::BIND_VERTEX_BUFFER
        , CDeviceManager::USAGE_DYNAMIC
        | CDeviceManager::USAGE_CPU_WRITE
        | CDeviceManager::USAGE_DIRECT_ACCESS_CPU_COHERENT
        | CDeviceManager::USAGE_DIRECT_ACCESS_GPU_COHERENT
        , DynamicDefragAllocator
#     if BUFFER_USE_STAGED_UPDATES
        , DynamicBufferUpdater
#     else
        , DirectBufferUpdater
#     endif
        > DynamicBufferPoolVB;
    typedef BufferPoolImpl<
        CDeviceManager::BIND_INDEX_BUFFER
        , CDeviceManager::USAGE_DYNAMIC
        | CDeviceManager::USAGE_CPU_WRITE
        | CDeviceManager::USAGE_DIRECT_ACCESS_CPU_COHERENT
        | CDeviceManager::USAGE_DIRECT_ACCESS_GPU_COHERENT
        , DynamicDefragAllocator
#     if BUFFER_USE_STAGED_UPDATES
        , DynamicBufferUpdater
#     else
        , DirectBufferUpdater
#     endif
        > DynamicBufferPoolIB;

# if BUFFER_SUPPORT_TRANSIENT_POOLS
    template<size_t BIND_FLAGS, size_t ALIGNMENT = PoolConfig::POOL_ALIGNMENT>
    class TransientBufferPool
        : public BufferPool
    {
        BufferPoolBank m_backing_buffer;
        size_t m_allocation_count;
        D3D11_MAP m_map_type;

    public:
        TransientBufferPool()
            : m_backing_buffer(~0u)
            , m_allocation_count()
            , m_map_type(D3D11_MAP_WRITE_NO_OVERWRITE)
        {
        }

        item_handle_t Allocate(size_t size)
        {
            // Align the allocation size up to the configured allocation alignment
            size = (max(size, size_t(1u)) + (ALIGNMENT - 1)) & ~(ALIGNMENT - 1);

            AZRHI_ASSERT(size <= m_backing_buffer.m_capacity);

            if (m_backing_buffer.m_free_space + size >= m_backing_buffer.m_capacity)
            {
                m_map_type = D3D11_MAP_WRITE_DISCARD;
                m_backing_buffer.m_free_space = 0;
            }

            BufferPoolItem* item = &m_item_table[m_item_table.Allocate()];
            item->m_buffer = m_backing_buffer.m_buffer;
            item->m_pool   = this;
            item->m_offset = m_backing_buffer.m_free_space;
            item->m_bank   = ~0u;
            item->m_size   = size;
            item->m_defrag_handle = IDefragAllocator::InvalidHdl;
            CDeviceManager::ExtractBasePointer(m_backing_buffer.m_buffer, item->m_base_ptr);

            m_backing_buffer.m_free_space += size;
            ++m_allocation_count;

            return item->m_handle;
        }
        void Free(BufferPoolItem* item)
        {
            m_item_table.Free(item->m_handle);
            --m_allocation_count;
        }
        bool CreateResources(bool, bool)
        {
            if (gRenDev->m_DevMan.CreateBuffer(
                    s_PoolConfig.m_transient_pool_size
                    , 1
                    , CDeviceManager::USAGE_CPU_WRITE | CDeviceManager::USAGE_DYNAMIC | CDeviceManager::USAGE_TRANSIENT
                    , BIND_FLAGS
                    , &m_backing_buffer.m_buffer) != S_OK)
            {
                CryLogAlways(
                    "TransientBufferPool::CreateResources: could not allocate backing buffer of size %" PRISIZE_T
                    , s_PoolConfig.m_transient_pool_size);
                return false;
            }
            m_backing_buffer.m_capacity   = s_PoolConfig.m_transient_pool_size;
            m_backing_buffer.m_free_space = 0;
            m_backing_buffer.m_handle = ~0u;
            CDeviceManager::ExtractBasePointer(m_backing_buffer.m_buffer, m_backing_buffer.m_base_ptr);
            return true;
        }
        bool FreeResources()
        {
            UnsetStreamSources(m_backing_buffer.m_buffer);
            ReleaseD3DBuffer(m_backing_buffer.m_buffer);
            m_backing_buffer.m_capacity   = 0;
            m_backing_buffer.m_free_space = 0;
            m_backing_buffer.m_handle = ~0u;
            return true;
        }
        bool GetStats(SDeviceBufferPoolStats&) { return false; }
        bool DebugRender() { return false; }
        void Sync() {}
        void Update([[maybe_unused]] uint32 frameId, [[maybe_unused]] DeviceFenceHandle fence, [[maybe_unused]] bool allow_defragmentation)
        {
            if (m_allocation_count)
            {
                CryFatalError(
                    "TransientBufferPool::Update %" PRISIZE_T " allocations still in transient pool!"
                    , m_allocation_count);
            }
            m_map_type = D3D11_MAP_WRITE_DISCARD;
            m_backing_buffer.m_free_space = 0;
        }
        void ReleaseEmptyBanks() {}
        void* BeginRead([[maybe_unused]] BufferPoolItem* item) { return NULL; }
        void* BeginWrite(BufferPoolItem* item)
        {
            D3DBuffer* buffer = m_backing_buffer.m_buffer;
            size_t size = item->m_size;
            D3D11_MAPPED_SUBRESOURCE mapped_resource;
            D3D11_MAP map = m_map_type;
#if defined(OPENGL) && !DXGL_FULL_EMULATION
            HRESULT hr = DXGLMapBufferRange(
                    &gcpRendD3D->GetDeviceContext()
                    , buffer
                    , item->m_offset
                    , item->m_size
                    , map
                    , 0
                    , &mapped_resource);
#else
            HRESULT hr = gcpRendD3D->GetDeviceContext().Map(
                    buffer
                    , 0
                    , map
                    , 0
                    , &mapped_resource);
#endif
            if (!CHECK_HRESULT(hr))
            {
                CryLogAlways("map of staging buffer for WRITING failed!");
                return NULL;
            }
#if defined(OPENGL) && !DXGL_FULL_EMULATION
            return reinterpret_cast<uint8*>(mapped_resource.pData);
#else
            return reinterpret_cast<uint8*>(mapped_resource.pData) + item->m_offset;
#endif
        }
        void EndReadWrite([[maybe_unused]] BufferPoolItem* item, [[maybe_unused]] bool requires_flush)
        {
            gcpRendD3D->GetDeviceContext().Unmap(m_backing_buffer.m_buffer, 0);
            m_map_type = D3D11_MAP_WRITE_NO_OVERWRITE;
        }
        void Write(BufferPoolItem* item, const void* src, size_t size)
        {
            AZRHI_ASSERT(size <= item->m_size);
            AZRHI_ASSERT(item->m_size <= m_backing_buffer.m_capacity);
            void* const dst = BeginWrite(item);
            IF (dst, 1)
            {
                const size_t csize = min((size_t)item->m_size, size);
                const bool requires_flush = CopyData(dst, src, csize);
                EndReadWrite(item, requires_flush);
            }
        }
    };

    //////////////////////////////////////////////////////////////////////////////////////
    // TransientBufferPool is a buffer pool for geometry that can change frequently and
    // is only valid for a single frame (fire&forgot geometry).
    //
    // Corresponding D3D_USAGE : USAGE_DYNAMIC
    // Corresponding update strategy : DISCARD + NO_OVERWRITE direct map of the buffer
    typedef TransientBufferPool<CDeviceManager::BIND_VERTEX_BUFFER> TransientBufferPoolVB;
    typedef TransientBufferPool<CDeviceManager::BIND_INDEX_BUFFER> TransientBufferPoolIB;
# endif

    //////////////////////////////////////////////////////////////////////////
    // Freestanding buffer implementation
    template<
        size_t BIND_FLAGS,
        size_t USAGE_FLAGS,
        typename Allocator,
        template<size_t> class Updater>
    struct FreeBufferPoolImpl
        : public BufferPool
    {
        typedef Updater<BIND_FLAGS> updater_t;

        BufferPoolBank m_backing_buffer;
        size_t m_allocation_size;
        size_t m_item_handle;
        updater_t m_updater;

    public:
        FreeBufferPoolImpl(StagingResources& resources, size_t size)
            : m_backing_buffer(~0u)
            , m_allocation_size((max(size, size_t(1u)) + (PoolConfig::POOL_ALIGNMENT - 1)) & ~(PoolConfig::POOL_ALIGNMENT - 1))
            , m_item_handle(~0u)
            , m_updater(resources)
        {
            if (!CreateResources(true, true))
            {
                CryLogAlways("DEVBUFFER WARNING: could not create free standing buffer");
            }
        }

        virtual ~FreeBufferPoolImpl()  { FreeResources();  }

        item_handle_t Allocate(size_t size)
        {
            // Align the allocation size up to the configured allocation alignment
            size = (max(size, size_t(1u)) + (PoolConfig::POOL_ALIGNMENT - 1)) & ~(PoolConfig::POOL_ALIGNMENT - 1);
            if (m_item_handle != ~0u || size != m_allocation_size)
            {
                CryFatalError("free standing buffer allocated twice?!");
                return ~0u;
            }

            BufferPoolItem* item = &m_item_table[m_item_table.Allocate()];
            item->m_buffer = m_backing_buffer.m_buffer;
            item->m_pool   = this;
            item->m_offset = 0u;
            item->m_bank   = ~0u;
            item->m_size   = size;
            item->m_defrag_handle = IDefragAllocator::InvalidHdl;
            CDeviceManager::ExtractBasePointer(m_backing_buffer.m_buffer, item->m_base_ptr);

            m_backing_buffer.m_free_space += size;
            return (m_item_handle = item->m_handle);
        }

        void Free(BufferPoolItem* item)
        {
            m_item_table.Free(item->m_handle);

            // We can do this safely here as only the item has a reference to
            // this instance.
            delete this;
        }

        bool CreateResources(bool, bool)
        {
            if (gRenDev->m_DevMan.CreateBuffer(
                    m_allocation_size
                    , 1
                    , USAGE_FLAGS
                    , BIND_FLAGS
                    , &m_backing_buffer.m_buffer) != S_OK)
            {
                CryLogAlways(
                    "FreeStandingBuffer::CreateResources: could not allocate backing buffer of size %" PRISIZE_T
                    , s_PoolConfig.m_transient_pool_size);
                return false;
            }
            m_backing_buffer.m_capacity = m_allocation_size;
            m_backing_buffer.m_free_space = 0;
            m_backing_buffer.m_handle = ~0u;
            CDeviceManager::ExtractBasePointer(m_backing_buffer.m_buffer, m_backing_buffer.m_base_ptr);
            return true;
        }
        bool FreeResources()
        {
            UnsetStreamSources(m_backing_buffer.m_buffer);
            ReleaseD3DBuffer(m_backing_buffer.m_buffer);
            m_backing_buffer.m_capacity   = 0;
            m_backing_buffer.m_free_space = 0;
            m_backing_buffer.m_handle = ~0u;
            return true;
        }
        bool GetStats(SDeviceBufferPoolStats&) { return false; }
        bool DebugRender() { return false; }
        void Sync() {}
        void Update([[maybe_unused]] uint32 frameId, [[maybe_unused]] DeviceFenceHandle fence, [[maybe_unused]] bool allow_defragmentation) {}
        void ReleaseEmptyBanks() {}
        void* BeginRead([[maybe_unused]] BufferPoolItem* item) { return NULL; }
        void* BeginWrite(BufferPoolItem* item)
        {
            return m_updater.BeginWrite(item->m_buffer, item->m_size, item->m_offset);
        }
        void EndReadWrite([[maybe_unused]] BufferPoolItem* item, [[maybe_unused]] bool requires_flush)
        {
            m_updater.EndReadWrite();
        }
        static BufferPool* Create(StagingResources& resources, size_t size)
        { return new FreeBufferPoolImpl(resources, size); }
    };
    typedef BufferPool* (* BufferCreateFnc)(StagingResources&, size_t);

    //////////////////////////////////////////////////////////////////////////////////////
    // A freestanding buffer for geometry that change infrequently and have a
    // significant lifetime
    //
    // Use this pool for example for :
    //    - streamed static geometry
    //    - geometry that rarely changes
    //
    // Corresponding D3D_USAGE : USAGE_DEFAULT
    // Corresponding update strategy : d3d11 staging buffers (CopySubResource)
    //
    typedef FreeBufferPoolImpl<
        CDeviceManager::BIND_VERTEX_BUFFER
        , CDeviceManager::USAGE_DEFAULT | CDeviceManager::USAGE_DIRECT_ACCESS_CPU_COHERENT
        , DynamicDefragAllocator
  #     if BUFFER_USE_STAGED_UPDATES
        , StaticBufferUpdater
  #     else
        , DirectBufferUpdater
  #     endif
        > SStaticFreeBufferVB;
    typedef FreeBufferPoolImpl<
        CDeviceManager::BIND_INDEX_BUFFER
        , CDeviceManager::USAGE_DEFAULT
        , DynamicDefragAllocator
  #     if BUFFER_USE_STAGED_UPDATES
        , StaticBufferUpdater
  #     else
        , DirectBufferUpdater
  #     endif
        > SStaticFreeBufferIB;



    //////////////////////////////////////////////////////////////////////////////////////
    // A free standing buffer for geometry that can change frequently but rarely
    // changes topology
    //
    // Use this pool for example for :
    //    - deforming geometry that is updated on the CPU
    //    - characters skinned in software
    //
    // Corresponding D3D_USAGE : USAGE_DYNAMIC
    // Corresponding update strategy : NO_OVERWRITE direct map of the buffer
    typedef FreeBufferPoolImpl<
        CDeviceManager::BIND_VERTEX_BUFFER
        , CDeviceManager::USAGE_DYNAMIC
        | CDeviceManager::USAGE_CPU_WRITE
        | CDeviceManager::USAGE_DIRECT_ACCESS_CPU_COHERENT
        | CDeviceManager::USAGE_DIRECT_ACCESS_GPU_COHERENT
        , DynamicDefragAllocator
#     if BUFFER_USE_STAGED_UPDATES
        , DynamicBufferUpdater
#     else
        , DirectBufferUpdater
#     endif
        > SDynamicFreeBufferVB;
    typedef FreeBufferPoolImpl<
        CDeviceManager::BIND_INDEX_BUFFER
        , CDeviceManager::USAGE_DYNAMIC
        | CDeviceManager::USAGE_CPU_WRITE
        | CDeviceManager::USAGE_DIRECT_ACCESS_CPU_COHERENT
        | CDeviceManager::USAGE_DIRECT_ACCESS_GPU_COHERENT
        , DynamicDefragAllocator
#     if BUFFER_USE_STAGED_UPDATES
        , DynamicBufferUpdater
#     else
        , DirectBufferUpdater
#     endif
        > SDynamicFreeBufferIB;

    //===============================================================================

#if defined(CRY_USE_DX12)
    class CDescriptorPool
    {
        struct SDescriptorBlockList
        {
            AzRHI::PartitionTable<SDescriptorBlock> items;
            std::vector<DX12::DescriptorBlock> blocks;

            SDescriptorBlockList() {}
            SDescriptorBlockList(SDescriptorBlockList&& other)
            {
                items = std::move(other.items);
                blocks = std::move(other.blocks);
            }
        };

        struct SRetiredBlock
        {
            uint32 listIndex;
            item_handle_t itemHandle;
        };

        std::unordered_map<uint32_t, SDescriptorBlockList > m_DescriptorBlocks;
        std::array<std::vector<SRetiredBlock>, PoolConfig::POOL_FRAME_QUERY_COUNT> m_RetiredBlocks;
        std::array<DeviceFenceHandle, PoolConfig::POOL_FRAME_QUERY_COUNT> m_fences;

        uint32 m_frameID;
        CryCriticalSection m_lock;

    public:
        CDescriptorPool()
            : m_frameID(0)
        {
            m_fences.fill(0);
        }

        SDescriptorBlock* Allocate(size_t size)
        {
            AUTO_LOCK(m_lock);

            SDescriptorBlockList& blockList = m_DescriptorBlocks[size];
            item_handle_t itemHandle = blockList.items.Allocate();

            if (blockList.blocks.size() < blockList.items.Capacity())
            {
                blockList.blocks.resize(blockList.items.Capacity());
            }

            DX12::DescriptorBlock& block = blockList.blocks[itemHandle];
            if (block.GetCapacity() == 0)
            {
                DX12::Device* pDevice = reinterpret_cast<CCryDX12Device&>(gcpRendD3D->GetDevice()).GetDX12Device();
                block = pDevice->GetGlobalDescriptorBlock(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, size);
            }

            SDescriptorBlock& item = blockList.items[itemHandle];
            item.offset = block.GetStartOffset();
            item.size = size;
            item.pBuffer = block.GetDescriptorHeap();

            return &item;
        }

        void Free(SDescriptorBlock* pItem)
        {
            AUTO_LOCK(m_lock);
            SRetiredBlock retiredBlock = { pItem->size, pItem->blockID };
            m_RetiredBlocks[m_frameID].push_back(retiredBlock);
        }

        void Update(uint32 frameId, DeviceFenceHandle fence)
        {
            m_frameID = frameId & PoolConfig::POOL_FRAME_QUERY_MASK;

            for (auto& retiredBlockList : m_RetiredBlocks)
            {
                if (S_OK == gRenDev->m_DevMan.SyncFence(m_fences[frameId & PoolConfig::POOL_FRAME_QUERY_MASK], false, false))
                {
                    AUTO_LOCK(m_lock);
                    for (auto& block : retiredBlockList)
                    {
                        m_DescriptorBlocks[block.listIndex].items.Free(block.itemHandle);
                    }

                    retiredBlockList.clear();
                }
            }

            m_fences[m_frameID] = fence;
        }

        void FreeResources()
        {
            for (auto& retiredBlockList : m_RetiredBlocks)
            {
                retiredBlockList.clear();
            }

            m_DescriptorBlocks.clear();
        }
    };
#endif

    //////////////////////////////////////////////////////////////////////////////////////
    // Manages all pool - in anonymous namespace to reduce recompiles
    struct PoolManager
    {
        AZStd::mutex m_constantBufferLock;

        // Storage for constant buffer wrapper instances
        AzRHI::PartitionTable<AzRHI::ConstantBuffer> m_constant_buffers;

        // The allocator for constant buffers
#   if CONSTANT_BUFFER_ENABLE_DIRECT_ACCESS
        ConstantBufferAllocator m_constant_allocator;
#   endif

#if defined(CRY_USE_DX12)
        CDescriptorPool m_ResourceDescriptorPool;
#endif

        // The pools segregated by usage and binding
        BufferPool* m_pools[BBT_MAX][BU_MAX];

        // Freestanding buffer creator functions
        BufferCreateFnc m_buffer_creators[BBT_MAX][BU_MAX];

        // The pools fences
        DeviceFenceHandle m_fences[PoolConfig::POOL_FRAME_QUERY_COUNT];

        // The resources used for updating buffers
        StagingResources m_staging_resources[BU_MAX];

        // This lock must be held when operating on the buffers
        SRecursiveSpinLock m_lock;

        static PoolManager& GetInstance()
        {
            static PoolManager s_Instance;
            return s_Instance;
        }

        bool m_initialized;

        PoolManager()
            : m_initialized()
        {
            memset(m_pools, 0x0, sizeof(m_pools));
            memset(m_fences, 0x0, sizeof(m_fences));
            memset(m_buffer_creators, 0x0, sizeof(m_buffer_creators));
        }

        ~PoolManager() {}

        bool CreatePool(BUFFER_BIND_TYPE type, BUFFER_USAGE usage, bool enable_defragging, bool best_fit, BufferPool* pool)
        {
            if ((m_pools[type][usage] = pool)->CreateResources(enable_defragging, best_fit) == false)
            {
                CryLogAlways("SPoolManager::Initialize: could not initialize buffer pool of type '%s|%s'"
                    , ConstantToString(type)
                    , ConstantToString(usage));
                return false;
            }
            return true;
        }

        bool Initialize()
        {
            bool success = true;
            if (!s_PoolConfig.Configure())
            {
                goto error;
            }

            for (size_t i = 0; i < PoolConfig::POOL_FRAME_QUERY_COUNT; ++i)
            {
                if (gRenDev->m_DevMan.CreateFence(m_fences[i]) != S_OK)
                {
                    CryLogAlways("SPoolManager::Initialize: could not create per-frame gpu fence");
                    goto error;
                }
            }

#     if CONSTANT_BUFFER_ENABLE_DIRECT_ACCESS
            m_constant_allocator.Initialize();
#     endif
            success &= CreatePool(BBT_VERTEX_BUFFER, BU_STATIC, gRenDev->CV_r_buffer_pool_defrag_static > 0  && gRenDev->GetActiveGPUCount() == 1, true, new StaticBufferPoolVB(m_staging_resources[BU_STATIC]));
            success &= CreatePool(BBT_INDEX_BUFFER, BU_STATIC, gRenDev->CV_r_buffer_pool_defrag_static > 0  && gRenDev->GetActiveGPUCount() == 1, true, new StaticBufferPoolIB(m_staging_resources[BU_STATIC]));


#     if CRY_USE_DX12
            success &= CreatePool(BBT_VERTEX_BUFFER, BU_DYNAMIC, gRenDev->CV_r_buffer_pool_defrag_dynamic > 0 && gRenDev->GetActiveGPUCount() == 1, true, new StaticBufferPoolVB(m_staging_resources[BU_DYNAMIC]));
            success &= CreatePool(BBT_INDEX_BUFFER, BU_DYNAMIC, gRenDev->CV_r_buffer_pool_defrag_dynamic > 0 && gRenDev->GetActiveGPUCount() == 1, true, new StaticBufferPoolIB(m_staging_resources[BU_DYNAMIC]));
#     else
            success &= CreatePool(BBT_VERTEX_BUFFER, BU_DYNAMIC, gRenDev->CV_r_buffer_pool_defrag_dynamic > 0 && gRenDev->GetActiveGPUCount() == 1, true, new DynamicBufferPoolVB(m_staging_resources[BU_DYNAMIC]));
            success &= CreatePool(BBT_INDEX_BUFFER, BU_DYNAMIC, gRenDev->CV_r_buffer_pool_defrag_dynamic > 0 && gRenDev->GetActiveGPUCount() == 1, true, new DynamicBufferPoolIB(m_staging_resources[BU_DYNAMIC]));
#     endif
            success &= CreatePool(BBT_VERTEX_BUFFER, BU_TRANSIENT, false, false, new DynamicBufferPoolVB(m_staging_resources[BU_TRANSIENT]));
            success &= CreatePool(BBT_INDEX_BUFFER, BU_TRANSIENT, false, false, new DynamicBufferPoolIB(m_staging_resources[BU_TRANSIENT]));

#     if BUFFER_SUPPORT_TRANSIENT_POOLS
            success &= CreatePool(BBT_VERTEX_BUFFER, BU_TRANSIENT_RT, false, false, new TransientBufferPoolVB());
            success &= CreatePool(BBT_INDEX_BUFFER, BU_TRANSIENT_RT, false, false, new TransientBufferPoolIB());
            success &= CreatePool(BBT_VERTEX_BUFFER, BU_WHEN_LOADINGTHREAD_ACTIVE, false, false, new TransientBufferPoolVB());
            success &= CreatePool(BBT_INDEX_BUFFER, BU_WHEN_LOADINGTHREAD_ACTIVE, false, false, new TransientBufferPoolIB());
#     else
            success &= CreatePool(BBT_VERTEX_BUFFER, BU_TRANSIENT_RT, false, false, new DynamicBufferPoolVB(m_staging_resources[BU_TRANSIENT]));
            success &= CreatePool(BBT_INDEX_BUFFER, BU_TRANSIENT_RT, false, false, new DynamicBufferPoolIB(m_staging_resources[BU_TRANSIENT]));
            success &= CreatePool(BBT_VERTEX_BUFFER, BU_WHEN_LOADINGTHREAD_ACTIVE, false, false, new DynamicBufferPoolVB(m_staging_resources[BU_TRANSIENT]));
            success &= CreatePool(BBT_INDEX_BUFFER, BU_WHEN_LOADINGTHREAD_ACTIVE, false, false, new DynamicBufferPoolIB(m_staging_resources[BU_TRANSIENT]));
#     endif

            if (!success)
            {
                CryLogAlways("SPoolManager::Initialize: could not initialize a buffer pool");
                goto error;
            }

            m_buffer_creators[BBT_VERTEX_BUFFER][BU_STATIC] = &SStaticFreeBufferVB::Create;
            m_buffer_creators[BBT_INDEX_BUFFER ][BU_STATIC] = &SStaticFreeBufferIB::Create;
            m_buffer_creators[BBT_VERTEX_BUFFER][BU_DYNAMIC] = &SDynamicFreeBufferVB::Create;
            m_buffer_creators[BBT_INDEX_BUFFER ][BU_DYNAMIC] = &SDynamicFreeBufferIB::Create;
            m_buffer_creators[BBT_VERTEX_BUFFER][BU_TRANSIENT] = &SDynamicFreeBufferVB::Create;
            m_buffer_creators[BBT_INDEX_BUFFER ][BU_TRANSIENT] = &SDynamicFreeBufferIB::Create;
            m_buffer_creators[BBT_VERTEX_BUFFER][BU_TRANSIENT_RT] = &SDynamicFreeBufferVB::Create;
            m_buffer_creators[BBT_INDEX_BUFFER ][BU_TRANSIENT_RT] = &SDynamicFreeBufferIB::Create;

            if (false)
            {
error:
                Shutdown();
                return false;
            }
            m_initialized = true;
            return true;
        }

        bool Shutdown()
        {
            bool success = true;
            for (size_t i = 0; i < BBT_MAX; ++i)
            {
                for (size_t j = 0; j < BU_MAX; ++j)
                {
                    if (m_pools[i][j] && !m_pools[i][j]->FreeResources())
                    {
                        CryLogAlways("SPoolManager::Initialize: could not shutdown buffer pool of type '%s|%s'"
                            , ConstantToString((BUFFER_USAGE)i)
                            , ConstantToString((BUFFER_USAGE)j));
                        success = false;
                    }
                    SAFE_DELETE(m_pools[i][j]);
                }
            }

#     if CONSTANT_BUFFER_ENABLE_DIRECT_ACCESS
            m_constant_allocator.Shutdown();
#     endif

            m_constant_buffers.Clear();

#if defined(CRY_USE_DX12)
            m_ResourceDescriptorPool.FreeResources();
#endif

            for (size_t i = 0; i < PoolConfig::POOL_FRAME_QUERY_COUNT; ++i)
            {
                if (gRenDev->m_DevMan.ReleaseFence(m_fences[i]) != S_OK)
                {
                    CryLogAlways("SPoolManager::Initialize: could not releasefence");
                    success = false;
                }
                m_fences[i] = DeviceFenceHandle();
            }

            m_initialized = false;
            return success;
        }
    };
}

CDeviceBufferManager::CDeviceBufferManager()
{
}

CDeviceBufferManager::~CDeviceBufferManager()
{
}

void CDeviceBufferManager::LockDevMan()
{
    PoolManager::GetInstance().m_lock.Lock();
}

void CDeviceBufferManager::UnlockDevMan()
{
    PoolManager::GetInstance().m_lock.Unlock();
}

bool CDeviceBufferManager::Init()
{
    PoolManager& poolManager = PoolManager::GetInstance();

    LOADING_TIME_PROFILE_SECTION;
    SREC_AUTO_LOCK(poolManager.m_lock);
    if (poolManager.m_initialized == true)
    {
        return true;
    }

    // Initialize the pool manager
    if (!poolManager.Initialize())
    {
        CryFatalError("CDeviceBufferManager::Init(): pool manager failed to initialize");
        return false;
    }

    return true;
}

bool CDeviceBufferManager::Shutdown()
{
    PoolManager& poolManager = PoolManager::GetInstance();

    SREC_AUTO_LOCK(poolManager.m_lock);
    if (poolManager.m_initialized == false)
    {
        return true;
    }

    // Initialize the pool manager
    if (!poolManager.Shutdown())
    {
        CryFatalError("CDeviceBufferManager::Init(): pool manager failed during shutdown");
        return false;
    }

    return true;
}

void CDeviceBufferManager::Sync(uint32 frameId)
{
    PoolManager& poolManager = PoolManager::GetInstance();

    FUNCTION_PROFILER_RENDERER;
    SREC_AUTO_LOCK(poolManager.m_lock);

    for (int i = 0; i < PoolConfig::POOL_FRAME_QUERY_COUNT; ++i)
    {
        gRenDev->m_DevMan.SyncFence(poolManager.m_fences[i], true);
    }

    for (size_t i = 0; i < BBT_MAX; ++i)
    {
        for (size_t j = 0; j < BU_MAX; ++j)
        {
            IF (poolManager.m_pools[i][j] == NULL, 0)
            {
                continue;
            }
            SREC_AUTO_LOCK(poolManager.m_pools[i][j]->m_lock);
            poolManager.m_pools[i][j]->Sync();
        }
    }

    // Note: Issue the fence now for COPY_ON_WRITE. If the GPU has caught up to this point, no previous drawcall
    // will be pending and therefore it is safe to just reuse the previous allocation.
    gRenDev->m_DevMan.IssueFence(poolManager.m_fences[frameId & PoolConfig::POOL_FRAME_QUERY_MASK]);
}

void CDeviceBufferManager::ReleaseEmptyBanks(uint32 frameId)
{
    PoolManager& poolManager = PoolManager::GetInstance();

    FUNCTION_PROFILER_RENDERER;
    
    SREC_AUTO_LOCK(poolManager.m_lock);

    for (size_t i = 0; i < BBT_MAX; ++i)
    {
        for (size_t j = 0; j < BU_MAX; ++j)
        {
            IF (poolManager.m_pools[i][j] == NULL, 0)
            {
                continue;
            }
            SREC_AUTO_LOCK(poolManager.m_pools[i][j]->m_lock);
            poolManager.m_pools[i][j]->ReleaseEmptyBanks();
        }
    }

    // Release empty constant buffers
# if CONSTANT_BUFFER_ENABLE_DIRECT_ACCESS
    poolManager.m_constant_allocator.ReleaseEmptyBanks();
# endif

    // Note: Issue the current fence for retiring allocations. This is the same fence shelled out
    // to the pools during the update stage for COW, now we are reusing it to ensure the gpu caught
    // up to this point and therefore give out reclaimed memory again.
    gRenDev->m_DevMan.IssueFence(poolManager.m_fences[frameId & PoolConfig::POOL_FRAME_QUERY_MASK]);
}

void CDeviceBufferManager::Update(uint32 frameId, bool called_during_loading)
{
    PoolManager& poolManager = PoolManager::GetInstance();

    FUNCTION_PROFILER_RENDERER;
    LOADING_TIME_PROFILE_SECTION;
    
    SREC_AUTO_LOCK(poolManager.m_lock);

    gRenDev->m_DevMan.SyncFence(poolManager.m_fences[frameId & PoolConfig::POOL_FRAME_QUERY_MASK], true);

    for (size_t i = 0; i < BBT_MAX; ++i)
    {
        for (size_t j = 0; j < BU_MAX; ++j)
        {
            IF (poolManager.m_pools[i][j] == NULL, 0)
            {
                continue;
            }
            SREC_AUTO_LOCK(poolManager.m_pools[i][j]->m_lock);
            poolManager.m_pools[i][j]->Update(frameId
                , poolManager.m_fences[frameId & PoolConfig::POOL_FRAME_QUERY_MASK]
                , called_during_loading == false);
        }
    }

    // Update the constant buffers
# if CONSTANT_BUFFER_ENABLE_DIRECT_ACCESS
    {
        AZStd::lock_guard<AZStd::mutex> lock(poolManager.m_constantBufferLock);
        poolManager.m_constant_allocator.Update(frameId
            , poolManager.m_fences[frameId & PoolConfig::POOL_FRAME_QUERY_MASK]
            , called_during_loading == false);
    }
# endif

#if defined(CRY_USE_DX12)
    poolManager.m_ResourceDescriptorPool.Update(frameId,
        poolManager.m_fences[frameId & PoolConfig::POOL_FRAME_QUERY_MASK]);
#endif

    // Note: Issue the fence now for COPY_ON_WRITE. If the GPU has caught up to this point, no previous drawcall
    // will be pending and therefore it is safe to just reuse the previous allocation.
    gRenDev->m_DevMan.IssueFence(poolManager.m_fences[frameId & PoolConfig::POOL_FRAME_QUERY_MASK]);
}

AzRHI::ConstantBuffer* CDeviceBufferManager::CreateConstantBuffer(
    const char* name,
    AZ::u32 size,
    AzRHI::ConstantBufferUsage usage,
    AzRHI::ConstantBufferFlags flags)
{
    PoolManager& poolManager = PoolManager::GetInstance();

    size = (max(size, AZ::u32(1)) + (255)) & ~(255);

    AZStd::lock_guard<AZStd::mutex> lock(poolManager.m_constantBufferLock);
    AZ::u32 handle = poolManager.m_constant_buffers.Allocate();

    AzRHI::ConstantBuffer* buffer = &poolManager.m_constant_buffers[handle];
    buffer->m_name = name;
    buffer->m_usage = usage;
    buffer->m_flags = flags;
    buffer->m_size = size;
    buffer->m_dynamic = (usage == AzRHI::ConstantBufferUsage::Dynamic);
    return buffer;
}

#if defined(CRY_USE_DX12)
SDescriptorBlock* CDeviceBufferManager::CreateDescriptorBlock(size_t size)
{
    return PoolManager::GetInstance().m_ResourceDescriptorPool.Allocate(size);
}

void CDeviceBufferManager::ReleaseDescriptorBlock(SDescriptorBlock* pBlock)
{
    CRY_ASSERT(pBlock != NULL);
    PoolManager::GetInstance().m_ResourceDescriptorPool.Free(pBlock);
}


#else
SDescriptorBlock* CDeviceBufferManager::CreateDescriptorBlock([[maybe_unused]] size_t size) { return NULL; }
void CDeviceBufferManager::ReleaseDescriptorBlock([[maybe_unused]] SDescriptorBlock* pBlock) {}
#endif

buffer_handle_t CDeviceBufferManager::Create_Locked(
    BUFFER_BIND_TYPE type
    , BUFFER_USAGE usage
    , size_t size)
{
    PoolManager& poolManager = PoolManager::GetInstance();

    AZRHI_ASSERT((type >= BBT_VERTEX_BUFFER && type < BBT_MAX));
    AZRHI_ASSERT((usage >= BU_IMMUTABLE && usage < BU_MAX));
    AZRHI_ASSERT(poolManager.m_pools[type][usage] != NULL);

    // Workaround for NVIDIA SLI issues with latest drivers. GFE should disable the cvar below when fixed
    // Disabled for now
# if (defined(WIN32) || defined(WIN64))
    if (poolManager.m_buffer_creators[type][usage])
    {
        if (gRenDev->GetActiveGPUCount() > 1
            && gRenDev->m_bVendorLibInitialized
            && gRenDev->CV_r_buffer_sli_workaround
            && (usage == BU_DYNAMIC || usage == BU_TRANSIENT))
        {
            BufferPool* pool = poolManager.m_buffer_creators[type][usage](
                    poolManager.m_staging_resources[usage],
                    size
                    );
            item_handle_t item_handle = pool->Allocate(size);
            return item_handle == ~0u
                   ? (buffer_handle_t) ~0u
                   : (buffer_handle_t)pool->Resolve(item_handle);
        }
    }
# endif

    item_handle_t item_handle = poolManager.m_pools[type][usage]->Allocate(size);
    return item_handle == ~0u ? (buffer_handle_t) ~0u : (buffer_handle_t)poolManager.m_pools[type][usage]->Resolve(item_handle);
}

buffer_handle_t CDeviceBufferManager::Create(
    BUFFER_BIND_TYPE type
    , BUFFER_USAGE usage
    , size_t size)
{
    PoolManager& poolManager = PoolManager::GetInstance();

    FUNCTION_PROFILER(gEnv->pSystem, PROFILE_RENDERER);
    if (!poolManager.m_pools[type][usage])
    {
        return ~0u;
    }
# if (defined(WIN32) || defined(WIN64))
    SRecursiveSpinLocker __lock(&poolManager.m_lock);
# endif
    SREC_AUTO_LOCK(poolManager.m_pools[type][usage]->m_lock);
    return Create_Locked(type, usage, size);
}

void CDeviceBufferManager::Destroy_Locked(buffer_handle_t handle)
{
    FUNCTION_PROFILER_LEGACYONLY(gEnv->pSystem, PROFILE_RENDERER);
    AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::RendererDetailed);
    
    AZRHI_ASSERT(handle != 0);
    BufferPoolItem& item = *reinterpret_cast<BufferPoolItem*>(handle);
    item.m_pool->Free(&item);
}

void CDeviceBufferManager::Destroy(buffer_handle_t handle)
{
    PoolManager& poolManager = PoolManager::GetInstance();

# if (defined(WIN32) || defined(WIN64))
    SRecursiveSpinLocker __lock(&poolManager.m_lock);
# endif
    BufferPoolItem& item = *reinterpret_cast<BufferPoolItem*>(handle);
    SREC_AUTO_LOCK(item.m_pool->m_lock);
    Destroy_Locked(handle);
}

void* CDeviceBufferManager::BeginRead_Locked(buffer_handle_t handle)
{
    FUNCTION_PROFILER(gEnv->pSystem, PROFILE_RENDERER);
    
    AZRHI_ASSERT(handle != 0);
    BufferPoolItem& item = *reinterpret_cast<BufferPoolItem*>(handle);
    return item.m_pool->BeginRead(&item);
}
void* CDeviceBufferManager::BeginRead(buffer_handle_t handle)
{
    FUNCTION_PROFILER(gEnv->pSystem, PROFILE_RENDERER);
# if (defined(WIN32) || defined(WIN64))
    SRecursiveSpinLocker __lock(&PoolManager::GetInstance().m_lock);
# endif
    BufferPoolItem& item = *reinterpret_cast<BufferPoolItem*>(handle);
    SREC_AUTO_LOCK(item.m_pool->m_lock);
    return BeginRead_Locked(handle);
}

size_t CDeviceBufferManager::Size_Locked(buffer_handle_t handle)
{
    BufferPoolItem& item = *reinterpret_cast<BufferPoolItem*>(handle);
    return item.m_size;
}
size_t CDeviceBufferManager::Size(buffer_handle_t handle)
{
    return Size_Locked(handle);
}

void* CDeviceBufferManager::BeginWrite_Locked(buffer_handle_t handle)
{
    FUNCTION_PROFILER_LEGACYONLY(gEnv->pSystem, PROFILE_RENDERER);
    AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::RendererDetailed);
    
    AZRHI_ASSERT(handle != 0);
    BufferPoolItem& item = *reinterpret_cast<BufferPoolItem*>(handle);
    return item.m_pool->BeginWrite(&item);
}
void* CDeviceBufferManager::BeginWrite(buffer_handle_t handle)
{
# if (defined(WIN32) || defined(WIN64))
    SRecursiveSpinLocker __lock(&PoolManager::GetInstance().m_lock);
# endif
    BufferPoolItem& item = *reinterpret_cast<BufferPoolItem*>(handle);
    SREC_AUTO_LOCK(item.m_pool->m_lock);
    return BeginWrite_Locked(handle);
}

void CDeviceBufferManager::EndReadWrite_Locked(buffer_handle_t handle)
{
    FUNCTION_PROFILER_LEGACYONLY(gEnv->pSystem, PROFILE_RENDERER);
    AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::RendererDetailed);
    
    AZRHI_ASSERT(handle != 0);
    BufferPoolItem& item = *reinterpret_cast<BufferPoolItem*>(handle);
    item.m_pool->EndReadWrite(&item, true);
}
void CDeviceBufferManager::EndReadWrite(buffer_handle_t handle)
{
# if (defined(WIN32) || defined(WIN64))
    SRecursiveSpinLocker __lock(&PoolManager::GetInstance().m_lock);
# endif
    BufferPoolItem& item = *reinterpret_cast<BufferPoolItem*>(handle);
    SREC_AUTO_LOCK(item.m_pool->m_lock);
    return EndReadWrite_Locked(handle);
}

bool CDeviceBufferManager::UpdateBuffer_Locked(
    buffer_handle_t handle, const void* src, size_t size)
{
    FUNCTION_PROFILER_LEGACYONLY(gEnv->pSystem, PROFILE_RENDERER);
    AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::RendererDetailed);
    
    AZRHI_ASSERT(handle != 0);
    BufferPoolItem& item = *reinterpret_cast<BufferPoolItem*>(handle);
    item.m_pool->Write(&item, src, size);
    return true;
}
bool CDeviceBufferManager::UpdateBuffer
    (buffer_handle_t handle, const void* src, size_t size)
{
# if (defined(WIN32) || defined(WIN64))
    SRecursiveSpinLocker __lock(&PoolManager::GetInstance().m_lock);
# endif
    BufferPoolItem& item = *reinterpret_cast<BufferPoolItem*>(handle);
    SREC_AUTO_LOCK(item.m_pool->m_lock);
    return UpdateBuffer_Locked(handle, src, size);
}
D3DBuffer* CDeviceBufferManager::GetD3D(buffer_handle_t handle, size_t* offset)
{
    
    AZRHI_ASSERT(handle != 0);
    BufferPoolItem& item = *reinterpret_cast<BufferPoolItem*>(handle);
    *offset = item.m_offset;
    AZRHI_ASSERT(item.m_buffer);
    return item.m_buffer;
}

bool CDeviceBufferManager::GetStats(BUFFER_BIND_TYPE type, BUFFER_USAGE usage, SDeviceBufferPoolStats& stats)
{
    PoolManager& poolManager = PoolManager::GetInstance();

    FUNCTION_PROFILER(gEnv->pSystem, PROFILE_RENDERER);
    stats.buffer_descr =  string(ConstantToString(type));
    stats.buffer_descr += "_";
    stats.buffer_descr += string(ConstantToString(usage));
    stats.buffer_descr += "_";
    if (!poolManager.m_pools[type][usage])
    {
        return false;
    }
    SREC_AUTO_LOCK(poolManager.m_pools[type][usage]->m_lock);
    return poolManager.m_pools[type][usage]->GetStats(stats);
}

/////////////////////////////////////////////////////////////
// Legacy interface
//
// Use with care, can be removed at any point!
void CDeviceBufferManager::ReleaseVBuffer(CVertexBuffer* pVB)
{
    SAFE_DELETE(pVB);
}
void CDeviceBufferManager::ReleaseIBuffer(CIndexBuffer* pIB)
{
    SAFE_DELETE(pIB);
}
CVertexBuffer* CDeviceBufferManager::CreateVBuffer(size_t nVerts, const AZ::Vertex::Format& vertexFormat, [[maybe_unused]] const char* szName, BUFFER_USAGE usage)
{
    CVertexBuffer* pVB = new CVertexBuffer(NULL, vertexFormat);
    pVB->m_nVerts = nVerts;
    pVB->m_VS.m_BufferHdl = Create(BBT_VERTEX_BUFFER, usage, nVerts * vertexFormat.GetStride());

    return pVB;
}

CIndexBuffer* CDeviceBufferManager::CreateIBuffer(size_t nInds, [[maybe_unused]] const char* szNam, BUFFER_USAGE usage)
{
    CIndexBuffer* pIB = new CIndexBuffer(NULL);
    pIB->m_nInds = nInds;
    pIB->m_VS.m_BufferHdl = Create(BBT_INDEX_BUFFER, usage, nInds * sizeof(uint16));
    return pIB;
}

bool CDeviceBufferManager::UpdateVBuffer(CVertexBuffer* pVB, void* pVerts, size_t nVerts)
{
    AZRHI_ASSERT(pVB->m_VS.m_BufferHdl != ~0u);
    return UpdateBuffer(pVB->m_VS.m_BufferHdl, pVerts, nVerts * pVB->m_vertexFormat.GetStride());
}

bool CDeviceBufferManager::UpdateIBuffer(CIndexBuffer* pIB, void* pInds, size_t nInds)
{
    AZRHI_ASSERT(pIB->m_VS.m_BufferHdl != ~0u);
    return UpdateBuffer(pIB->m_VS.m_BufferHdl, pInds, nInds * sizeof(uint16));
}

namespace AzRHI
{
    ConstantBuffer::ConstantBuffer(uint32 handle)
        : m_buffer()
        , m_allocator()
        , m_base_ptr()
        , m_handle(handle)
        , m_offset()
        , m_size()
        , m_used()
        , m_usage{ ConstantBufferUsage::Dynamic }
        , m_flags{ ConstantBufferFlags::None }
        , m_refCount{ 1 }
    {}

    void ConstantBuffer::AddRef()
    {
        ++m_refCount;
    }

    AZ::u32 ConstantBuffer::Release()
    {
        AZ::u32 refCount = --m_refCount;
        if (!refCount)
        {
            PoolManager& poolManager = PoolManager::GetInstance();
            AZStd::lock_guard<AZStd::mutex> lock(poolManager.m_constantBufferLock);

#   if CONSTANT_BUFFER_ENABLE_DIRECT_ACCESS
            if (m_used)
            {
                poolManager.m_constant_allocator.Free(this);
                m_used = 0;
            }
#   endif
            poolManager.m_constant_buffers.Free(m_handle);
            return 0;
        }
        return refCount;
    }

    ConstantBuffer::~ConstantBuffer()
    {
#   if CONSTANT_BUFFER_ENABLE_DIRECT_ACCESS == 0
        gcpRendD3D->m_DevMan.UnbindConstantBuffer(this);
        gcpRendD3D->m_DevMan.ReleaseD3D11Buffer(m_buffer);
        m_buffer = nullptr;
#   endif
    }

    void* ConstantBuffer::BeginWrite()
    {
        PoolManager& poolManager = PoolManager::GetInstance();

# if CONSTANT_BUFFER_ENABLE_DIRECT_ACCESS
        if (m_used)
        {
            poolManager.m_constant_allocator.Free(this);
        }
        if (poolManager.m_constant_allocator.Allocate(this))
        {
            m_used = 1;
            return (void*)((uintptr_t)m_base_ptr + m_offset);
        }
# else
        if (!m_used)
        {
            HRESULT hr;
            D3D11_BUFFER_DESC bd;
            ZeroStruct(bd);
            bd.Usage = m_dynamic ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_DEFAULT;
            bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
            bd.CPUAccessFlags = m_dynamic ? D3D11_CPU_ACCESS_WRITE : 0;
            bd.MiscFlags = 0;
#   if defined(OPENGL) && !(CRY_USE_METAL)
            bd.MiscFlags |= AZ::u8(m_flags & ConstantBufferFlags::DenyStreaming) != 0 ? D3D11_RESOURCE_MISC_DXGL_NO_STREAMING : 0;
#   endif
            bd.ByteWidth = m_size;
            hr = gcpRendD3D->m_DevMan.CreateD3D11Buffer(&bd, NULL, &m_buffer, "ConstantBuffer");
            CHECK_HRESULT(hr);

            m_used = (hr == S_OK);
        }
        if (m_dynamic)
        {
            if (m_used && m_buffer)
            {
                AZ_Assert(m_base_ptr == nullptr, "Already mapped when mapping");
                D3D11_MAPPED_SUBRESOURCE mappedResource;
                HRESULT hr = gcpRendD3D->GetDeviceContext().Map(m_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
                AZ_Assert(hr == S_OK, "Map buffer failed");
                m_base_ptr = mappedResource.pData;
                return mappedResource.pData;
            }
        }
        else
        {
            return m_base_ptr = new char[m_size];
        }
# endif
        return nullptr;
    }

    void ConstantBuffer::EndWrite()
    {
# if CONSTANT_BUFFER_ENABLE_DIRECT_ACCESS == 0
        if (m_dynamic)
        {
            AZ_Assert(m_base_ptr != nullptr, "Not mapped when unmapping");
            gcpRendD3D->GetDeviceContext().Unmap(m_buffer, 0);
        }
        else
        {
            gcpRendD3D->GetDeviceContext().UpdateSubresource(m_buffer, 0, nullptr, m_base_ptr, m_size, 0);
            delete[] alias_cast<char*>(m_base_ptr);
        }
        m_base_ptr = nullptr;
# endif
    }

    void ConstantBuffer::UpdateBuffer(const void* src, AZ::u32 size)
    {
        if (void* dst = BeginWrite())
        {
            CopyData(dst, src, std::min(m_size, size));
            EndWrite();
        }
    }

    AZ::u32 GetConstantRegisterCountMax(EHWShaderClass shaderClass)
    {
        switch (shaderClass)
        {
        case eHWSC_Pixel:
        case eHWSC_Vertex:
        case eHWSC_Geometry:
        case eHWSC_Domain:
        case eHWSC_Hull:
        case eHWSC_Compute:
            return 512;
        default:
            assert(0);
            return 0;
        }
    }
}

CVertexBuffer::~CVertexBuffer()
{
    if (m_VS.m_BufferHdl != ~0u)
    {
        if (gRenDev)
        {
            gRenDev->m_DevBufMan.Destroy(m_VS.m_BufferHdl);
        }
        m_VS.m_BufferHdl = ~0u;
    }
}

CIndexBuffer::~CIndexBuffer()
{    
    if (m_VS.m_BufferHdl != ~0u)
    {
        if (gRenDev)
        {
            gRenDev->m_DevBufMan.Destroy(m_VS.m_BufferHdl);
        }
        m_VS.m_BufferHdl = ~0u;
    }
}

WrappedDX11Buffer::WrappedDX11Buffer(const WrappedDX11Buffer& src)
{
    
    m_pBuffer = src.m_pBuffer;
    if (m_pBuffer)
    {
        m_pBuffer->AddRef();
    }

    for (uint32_t i = 0; i < MAX_VIEW_COUNT; ++i)
    {
        m_pSRV[i] = src.m_pSRV[i];
        if (m_pSRV[i])
        {
            m_pSRV[i]->AddRef();
        }

        m_pUAV[i] = src.m_pUAV[i];
        if (m_pUAV[i])
        {
            m_pUAV[i]->AddRef();
        }
    }

    m_numElements = src.m_numElements;
    m_flags = src.m_flags;
}

WrappedDX11Buffer& WrappedDX11Buffer::operator=(const WrappedDX11Buffer& rhs)
{    
    ID3D11Buffer* pOldBuffer = m_pBuffer;

    m_pBuffer = rhs.m_pBuffer;
    if (m_pBuffer)
    {
        m_pBuffer->AddRef();
    }

    for (uint32_t i = 0; i < MAX_VIEW_COUNT; ++i)
    {
        ID3D11ShaderResourceView* pOldSRV = m_pSRV[i];
        m_pSRV[i] = rhs.m_pSRV[i];
        if (m_pSRV[i])
        {
            m_pSRV[i]->AddRef();
        }
        SAFE_RELEASE(pOldSRV);

        ID3D11UnorderedAccessView* pOldUAV = m_pUAV[i];
        m_pUAV[i] = rhs.m_pUAV[i];
        if (m_pUAV[i])
        {
            m_pUAV[i]->AddRef();
        }
        SAFE_RELEASE(pOldUAV);
    }

    SAFE_RELEASE(pOldBuffer);

    m_numElements = rhs.m_numElements;
    m_flags = rhs.m_flags;

    return *this;
}

bool WrappedDX11Buffer::operator==(const WrappedDX11Buffer& other) const
{
    return memcmp(this, &other, sizeof(*this)) != 0;
}

WrappedDX11Buffer::~WrappedDX11Buffer()
{
    Release();
}

void WrappedDX11Buffer::Release()
{    
    for (uint32_t i = 0; i < MAX_VIEW_COUNT; ++i)
    {
        SAFE_RELEASE(m_pSRV[i]);
        SAFE_RELEASE(m_pUAV[i]);
    }
    gcpRendD3D->m_DevMan.ReleaseD3D11Buffer(m_pBuffer);
    m_pBuffer = nullptr;
    m_numElements = 0;
    m_flags = 0;
}

void WrappedDX11Buffer::Create(uint32 numElements, uint32 elementSize, DXGI_FORMAT elementFormat, uint32 flags, const void* pData, [[maybe_unused]] int32 nESRAMOffset /*=-1*/)
{
    assert(pData != NULL || (flags & (DX11BUF_DYNAMIC | DX11BUF_BIND_UAV | DX11BUF_STAGING)));
    assert((flags & (DX11BUF_DYNAMIC | DX11BUF_BIND_UAV)) != (DX11BUF_DYNAMIC | DX11BUF_BIND_UAV));

    Release();

    const uint32 bufferCount = (flags & DX11BUF_DYNAMIC) ? 3 : 1;

    D3D11_BUFFER_DESC Desc;
    Desc.BindFlags = ((flags & DX11BUF_BIND_SRV) ? D3D11_BIND_SHADER_RESOURCE : 0) |
        ((flags & DX11BUF_BIND_UAV) ? D3D11_BIND_UNORDERED_ACCESS : 0);
    Desc.ByteWidth = numElements * elementSize * bufferCount;
    Desc.CPUAccessFlags = (flags & DX11BUF_DYNAMIC) ? D3D11_CPU_ACCESS_WRITE : ((flags & DX11BUF_STAGING) ? D3D11_CPU_ACCESS_READ : 0);
    Desc.MiscFlags = ((flags & DX11BUF_STRUCTURED) ? D3D11_RESOURCE_MISC_BUFFER_STRUCTURED : 0) |
        ((flags & DX11BUF_DRAWINDIRECT) ? D3D11_RESOURCE_MISC_DRAWINDIRECT_ARGS : 0);
    Desc.StructureByteStride = elementSize;
    Desc.Usage = (flags & DX11BUF_DYNAMIC) ? D3D11_USAGE_DYNAMIC : (flags & DX11BUF_BIND_UAV) ? D3D11_USAGE_DEFAULT : (flags & DX11BUF_STAGING) ? D3D11_USAGE_STAGING : D3D11_USAGE_IMMUTABLE;
    D3D11_SUBRESOURCE_DATA Data;
    Data.pSysMem = pData;
    Data.SysMemPitch = Desc.ByteWidth;
    Data.SysMemSlicePitch = Desc.ByteWidth;

#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION DEVBUFFER_CPP_SECTION_3
    #include AZ_RESTRICTED_FILE(DevBuffer_cpp)
#endif

    gcpRendD3D->m_DevMan.CreateD3D11Buffer(&Desc, (pData != NULL) ? &Data : NULL, &m_pBuffer, "WrappedDX11Buffer");

    if (flags & DX11BUF_BIND_SRV)
    {
        for (uint32 i = 0; i < bufferCount; ++i)
        {
            D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc;
            SRVDesc.Format = elementFormat;
            SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
            SRVDesc.Buffer.ElementOffset = i * numElements;
            SRVDesc.Buffer.ElementWidth = numElements;
            gcpRendD3D->GetDevice().CreateShaderResourceView(m_pBuffer, &SRVDesc, &m_pSRV[i]);
        }
    }

    if (flags & DX11BUF_BIND_UAV)
    {
        for (uint32 i = 0; i < bufferCount; ++i)
        {
            D3D11_UNORDERED_ACCESS_VIEW_DESC UAVDesc;
            UAVDesc.Format = elementFormat;
            UAVDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
            UAVDesc.Buffer.FirstElement = i * numElements;
            UAVDesc.Buffer.Flags = (flags & DX11BUF_UAV_APPEND) ? D3D11_BUFFER_UAV_FLAG_APPEND : 0;
            UAVDesc.Buffer.NumElements = numElements;
            gcpRendD3D->GetDevice().CreateUnorderedAccessView(m_pBuffer, &UAVDesc, &m_pUAV[i]);
        }
    }

    m_numElements = numElements;
    m_elementSize = elementSize;
    m_elementFormat = elementFormat;
    m_flags = flags;
}

void WrappedDX11Buffer::UpdateBufferContent(void* pData, size_t nSize)
{
    if (!m_pBuffer || !pData || nSize == 0)
    {
        return;
    }

    assert(m_flags & DX11BUF_DYNAMIC);

    m_currentBuffer = (m_currentBuffer + 1) % MAX_VIEW_COUNT;

    D3D11_MAPPED_SUBRESOURCE mappedRes;
#ifdef CRY_USE_DX12
    gcpRendD3D->GetDeviceContext().Map(m_pBuffer, 0, D3D11_MAP_WRITE_NO_OVERWRITE, 0, &mappedRes);
#else
    // D3D11_MAP_WRITE_NO_OVERWRITE with buffers other than vertex and index buffer is not supported on os prior to windows 8 dx11.1.
    gcpRendD3D->GetDeviceContext().Map(m_pBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedRes);
#endif
    uint8_t* memory = reinterpret_cast<uint8_t*>(mappedRes.pData) + m_currentBuffer * m_elementSize * m_numElements;
    memcpy(memory, pData, nSize);
    gcpRendD3D->GetDeviceContext().Unmap(m_pBuffer, 0);
}
