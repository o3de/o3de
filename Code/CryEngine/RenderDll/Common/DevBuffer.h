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

#include <XRenderD3D9/DeviceManager/Base.h>
#include <XRenderD3D9/DeviceManager/Enums.h>

////////////////////////////////////////////////////////////////////////////////////////
// Usage hints
enum BUFFER_USAGE
{
    BU_IMMUTABLE = 0, // For data that never, ever changes
    BU_STATIC,    // For long-lived data that changes infrequently (every n-frames)
    BU_DYNAMIC,   // For short-lived data that changes frequently (every frame)
    BU_TRANSIENT, // For very short-lived data that can be considered garbage after first usage
    BU_TRANSIENT_RT, // For very short-lived data that can be considered garbage after first usage
    BU_WHEN_LOADINGTHREAD_ACTIVE, // yes we can ... because renderloadingthread frames not synced with mainthread frames
    BU_MAX
};

////////////////////////////////////////////////////////////////////////////////////////
// Binding flags
enum BUFFER_BIND_TYPE
{
    BBT_VERTEX_BUFFER = 0,
    BBT_INDEX_BUFFER,
    BBT_MAX
};

typedef uintptr_t buffer_handle_t;
typedef uint32 item_handle_t;

// CRY DX12
////////////////////////////////////////////////////////////////////////////////////////
struct SDescriptorBlock
{
    SDescriptorBlock(uint32 id)
        : blockID(id)
        , pBuffer(NULL)
        , size(0)
        , offset(~0u)
    {}

    const uint32 blockID;

    void* pBuffer;
    uint32 size;
    uint32 offset;
};

class CDeviceBufferManager;
struct ConstantBufferAllocator;

namespace AzRHI
{
    enum class ConstantBufferUsage : AZ::u8
    {
        Static,
        Dynamic
    };

    enum class ConstantBufferFlags : AZ::u8
    {
        None = 0,
        DenyStreaming = BIT(1), // Used by OpenGL for constant buffer streaming
    };
    AZ_DEFINE_ENUM_BITWISE_OPERATORS(ConstantBufferFlags)

    class ConstantBuffer
    {
    public:
        ConstantBuffer(uint32 handle);
        virtual ~ConstantBuffer();

        inline D3DBuffer* GetPlatformBuffer() const
        {
            return m_buffer;
        }

        inline AzRHI::ConstantBufferUsage GetUsage() const
        {
            return m_usage;
        }

        inline AzRHI::ConstantBufferFlags GetFlags() const
        {
            return m_flags;
        }

        inline AZ::u32 GetByteOffset() const
        {
            return m_offset;
        }

        inline AZ::u32 GetByteCount() const
        {
            return m_size;
        }

#if !defined(NULL_RENDERER)
        inline AZ::u64 GetCode() const
        {
#if defined(AZ_RESTRICTED_PLATFORM)
    #include AZ_RESTRICTED_FILE(DevBuffer_h)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#else
            return reinterpret_cast<AZ::u64>(m_buffer) | ((AZ::u64)m_offset << 40);
#endif
        }
#endif // !NULL_RENDERER

        void AddRef();

        AZ::u32 Release();

        void* BeginWrite();

        void EndWrite();

        void UpdateBuffer(const void* data, AZ::u32 size);

    private:
        friend class ::CDeviceBufferManager;
        friend struct ::ConstantBufferAllocator;

        const char* m_name;
        D3DBuffer* m_buffer;
        item_handle_t m_handle;
        void* m_allocator;
        void* m_base_ptr;
        AZ::u32 m_offset;
        AZ::u32 m_size;
        ConstantBufferUsage m_usage;
        ConstantBufferFlags m_flags;
        AZ::u8 m_used : 1;
        AZ::u8 m_dynamic : 1;
        AZStd::atomic_uint m_refCount;

        int m_nHeapOffset;
        SDescriptorBlock* m_pDescriptorBlock;
    };

    using ConstantBufferPtr = _smart_ptr<AzRHI::ConstantBuffer>;

    AZ::u32 GetConstantRegisterCountMax(EHWShaderClass shaderClass);
}

////////////////////////////////////////////////////////////////////////////////////////
// Pool statistics
struct SDeviceBufferPoolStats
    : private NoCopy
{
    string buffer_descr;
    size_t  bank_size;     // size of a pool bank in bytes
    size_t  num_banks;     // number of banks currently allocated
    size_t  num_allocs;    // number of allocs present in the device pool
    IDefragAllocatorStats allocator_stats; // backing allocator statistics

    SDeviceBufferPoolStats()
        : buffer_descr()
        , bank_size()
        , num_banks()
        , num_allocs()
        , allocator_stats()
    { memset(&allocator_stats, 0x0, sizeof(allocator_stats)); }

    ~SDeviceBufferPoolStats() {}
};

class CVertexBuffer;
class CIndexBuffer;

class IDeviceBufferManager
{
    friend class CGuardedDeviceBufferManager;
    friend class CDeviceManager;
public:

# ifndef NULL_RENDERER
    virtual D3DBuffer* GetD3D(buffer_handle_t handle, size_t* outOffset) = 0;
#endif
    virtual void LockDevMan() = 0;
    virtual void UnlockDevMan() = 0;

private:
    virtual buffer_handle_t Create_Locked(BUFFER_BIND_TYPE, BUFFER_USAGE, size_t) = 0;
    virtual void Destroy_Locked(buffer_handle_t) = 0;
    virtual void* BeginRead_Locked(buffer_handle_t handle) = 0;
    virtual void* BeginWrite_Locked(buffer_handle_t handle) = 0;
    virtual void  EndReadWrite_Locked(buffer_handle_t handle) = 0;
    virtual bool  UpdateBuffer_Locked(buffer_handle_t handle, const void*, size_t) = 0;
    virtual size_t Size_Locked(buffer_handle_t) = 0;
};

class CDeviceBufferManager : public IDeviceBufferManager
{
    buffer_handle_t Create_Locked(BUFFER_BIND_TYPE, BUFFER_USAGE, size_t) override;
    void Destroy_Locked(buffer_handle_t) override;
    void* BeginRead_Locked(buffer_handle_t handle) override;
    void* BeginWrite_Locked(buffer_handle_t handle) override;
    void  EndReadWrite_Locked(buffer_handle_t handle) override;
    bool  UpdateBuffer_Locked(buffer_handle_t handle, const void*, size_t) override;
    size_t Size_Locked(buffer_handle_t) override;

public:
    CDeviceBufferManager();
    ~CDeviceBufferManager();

    ////////////////////////////////////////////////////////////////////////////////////////
    // Initialization and destruction and high level update funcationality
    bool Init();
    void Update(uint32 frameId, bool called_during_loading);
    void ReleaseEmptyBanks(uint32 frameId);
    void Sync(uint32 frameId);
    bool Shutdown();

    AzRHI::ConstantBuffer* CreateConstantBuffer(
        const char* name,
        AZ::u32 size,
        AzRHI::ConstantBufferUsage usage,
        AzRHI::ConstantBufferFlags flags = AzRHI::ConstantBufferFlags::None);

    ////////////////////////////////////////////////////////////////////////////////////////
    // Descriptor blocks
    SDescriptorBlock* CreateDescriptorBlock(size_t size);
    void ReleaseDescriptorBlock(SDescriptorBlock* pBlock);

    ////////////////////////////////////////////////////////////////////////////////////////
    // Locks the global devicebuffer lock
    void LockDevMan() override;
    void UnlockDevMan() override;

    // Returns the size in bytes of the allocation
    size_t Size(buffer_handle_t);

    ////////////////////////////////////////////////////////////////////////////////////////
    // Buffer Resource creation methods
    //
    buffer_handle_t Create(BUFFER_BIND_TYPE, BUFFER_USAGE, size_t);
    void Destroy(buffer_handle_t);

    ////////////////////////////////////////////////////////////////////////////////////////
    // Manual IO operations
    //
    // Note: it's an error to NOT end an IO operation with EndReadWrite!!!
    //
    // Note: If you are writing (updating) a buffer only partially, please be aware that the
    //       the contents of the untouched areas might be undefined as a copy-on-write semantic
    //       ensures that the updating of buffers does not synchronize with the GPU at any cost.
    //
    void* BeginRead(buffer_handle_t handle);
    void* BeginWrite(buffer_handle_t handle);
    void  EndReadWrite(buffer_handle_t handle);
    bool  UpdateBuffer(buffer_handle_t handle, const void*, size_t);

    ////////////////////////////////////////////////////////////////////////////////////////
    // Get Stats back from the devbuffer
    bool GetStats(BUFFER_BIND_TYPE, BUFFER_USAGE, SDeviceBufferPoolStats&);

# ifndef NULL_RENDERER
    D3DBuffer* GetD3D(buffer_handle_t handle, size_t* outOffset);
# endif

    /////////////////////////////////////////////////////////////
    // Legacy interface
    //
    // Use with care, can be removed at any point!
    CVertexBuffer* CreateVBuffer(size_t, const AZ::Vertex::Format&, const char*, BUFFER_USAGE usage = BU_STATIC); //waltont given that this is a legacy interface, does it make more sense to just remove it? What is the alternative, new interface? The only place that uses it is BreakableGlass.
    void ReleaseVBuffer(CVertexBuffer*);

    CIndexBuffer* CreateIBuffer(size_t, const char*, BUFFER_USAGE usage = BU_STATIC);
    void ReleaseIBuffer(CIndexBuffer*);

    bool UpdateVBuffer(CVertexBuffer*, void*, size_t);
    bool UpdateIBuffer(CIndexBuffer*, void*, size_t);
};

class CGuardedDeviceBufferManager
    : public NoCopy
{
private:
    IDeviceBufferManager* m_pDevMan;

public:
    explicit CGuardedDeviceBufferManager(IDeviceBufferManager* pDevMan)
        : m_pDevMan(pDevMan)
    {
        m_pDevMan->LockDevMan();
    }

    ~CGuardedDeviceBufferManager()
    {
        m_pDevMan->UnlockDevMan();
    }

    inline buffer_handle_t Create(BUFFER_BIND_TYPE type, BUFFER_USAGE usage, size_t size)
    {
        return m_pDevMan->Create_Locked(type, usage, size);
    }

    inline void Destroy(buffer_handle_t handle)
    {
        return m_pDevMan->Destroy_Locked(handle);
    }

    inline void* BeginRead(buffer_handle_t handle)
    {
        return m_pDevMan->BeginRead_Locked(handle);
    }

    inline void* BeginWrite(buffer_handle_t handle)
    {
        return m_pDevMan->BeginWrite_Locked(handle);
    }

    inline void EndReadWrite(buffer_handle_t handle)
    {
        m_pDevMan->EndReadWrite_Locked(handle);
    }

    inline bool UpdateBuffer(buffer_handle_t handle, const void* src, size_t size)
    {
        return m_pDevMan->UpdateBuffer_Locked(handle, src, size);
    }

# ifndef NULL_RENDERER
    inline D3DBuffer* GetD3D(buffer_handle_t handle, size_t* offset)
    {
        return m_pDevMan->GetD3D(handle, offset);
    }
# endif
};


class SRecursiveSpinLock
{
    volatile LONG m_lock;
    volatile threadID m_owner;
    volatile uint16 m_counter;

    enum
    {
        SPIN_COUNT = 10
    };

public:

    SRecursiveSpinLock()
        : m_lock()
        , m_owner()
        , m_counter()
    {}

    ~SRecursiveSpinLock() {}

    void Lock()
    {
        threadID threadId = CryGetCurrentThreadId();
        int32 iterations = 0;
retry:
        IF (CryInterlockedCompareExchange(&(this->m_lock), 1L, 0L) == 0L, 1)
        {
            assert (m_owner == 0u && m_counter == 0u);
            m_owner = threadId;
            m_counter = 1;
        }
        else
        {
            IF (m_owner == threadId, 1)
            {
                ++m_counter;
            }
            else
            {
                Sleep((1 & isneg(SPIN_COUNT - iterations++)));
                goto retry;
            }
        }
    }

    bool TryLock()
    {
        threadID threadId = CryGetCurrentThreadId();
        IF (CryInterlockedCompareExchange(&m_lock, 1L, 0L) == 0L, 1)
        {
            assert (m_owner == 0u && m_counter == 0u);
            m_owner = threadId;
            m_counter = 1;
            return true;
        }
        else
        {
            IF (m_owner == threadId, 1)
            {
                ++m_counter;
                return true;
            }
            else
            {
                return false;
            }
        }
    }

    void Unlock()
    {
        assert (m_owner == CryGetCurrentThreadId() && m_counter != 0u);
        IF ((m_counter -= 1) == 0u, 1)
        {
            m_owner = 0u;
            m_lock = 0L;
            MemoryBarrier();
        }
    }
};

class SRecursiveSpinLocker
{
    SRecursiveSpinLock* lock;
public:
    SRecursiveSpinLocker(SRecursiveSpinLock* _lock)
        : lock(_lock)
    {
        lock->Lock();
    }
    ~SRecursiveSpinLocker() { lock->Unlock(); }
};
#define SREC_AUTO_LOCK(x) SRecursiveSpinLocker AZ_JOIN(_lock, __LINE__)(&(x))

class CConditonalDevManLock
{
    CDeviceBufferManager* m_pDevBufMan;
    int m_Active;
public:
    explicit CConditonalDevManLock(CDeviceBufferManager* DevMan, int active)
        : m_pDevBufMan(DevMan)
        , m_Active(active)
    {
        if (m_Active)
        {
            m_pDevBufMan->LockDevMan();
        }
    }

    ~CConditonalDevManLock()
    {
        if (m_Active)
        {
            m_pDevBufMan->UnlockDevMan();
        }
    }
};

// WrappedDX11Buffer Flags
#define DX11BUF_DYNAMIC       BIT(0)
#define DX11BUF_STRUCTURED    BIT(1)
#define DX11BUF_BIND_SRV      BIT(2)
#define DX11BUF_BIND_UAV      BIT(3)
#define DX11BUF_UAV_APPEND    BIT(4)
#define DX11BUF_DRAWINDIRECT  BIT(5)
#define DX11BUF_STAGING       BIT(6)

#if !defined(NULL_RENDERER)
struct WrappedDX11Buffer
{
    WrappedDX11Buffer()
        : m_pBuffer{}
        , m_pSRV{}
        , m_pUAV{}
        , m_numElements{}
        , m_elementSize{}
        , m_elementFormat{DXGI_FORMAT_UNKNOWN}
        , m_flags(0)
        , m_currentBuffer{}
    {
    }

    ~WrappedDX11Buffer();

    WrappedDX11Buffer(const WrappedDX11Buffer& src);
    WrappedDX11Buffer& operator=(const WrappedDX11Buffer& rhs);

    bool operator==(const WrappedDX11Buffer& other) const;

    D3DUnorderedAccessView* GetUnorderedAccessView() const
    {
        return m_pUAV[m_currentBuffer];
    }

    D3DShaderResourceView* GetShaderResourceView() const
    {
        return m_pSRV[m_currentBuffer];
    }

    void Create(uint32 numElements, uint32 elementSize, DXGI_FORMAT elementFormat, uint32 flags, const void* pData, int32 nESRAMOffset = -1);
    void Release();
    void UpdateBufferContent(void* pData, size_t nSize);

    static const uint32_t MAX_VIEW_COUNT = 3;

    D3DBuffer* m_pBuffer;
    D3DShaderResourceView* m_pSRV[MAX_VIEW_COUNT];
    D3DUnorderedAccessView* m_pUAV[MAX_VIEW_COUNT];
    uint32 m_elementSize;
    uint32 m_numElements;
    DXGI_FORMAT m_elementFormat;
    uint32 m_flags;
    uint32_t m_currentBuffer;
};
#endif
