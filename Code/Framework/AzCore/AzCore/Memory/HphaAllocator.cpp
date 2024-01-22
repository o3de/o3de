/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/PlatformIncl.h>
#include <AzCore/Memory/HphaAllocator.h>
#include <AzCore/std/allocator_stateless.h>

#include <AzCore/Math/Random.h>
#include <AzCore/Memory/OSAllocator.h> // required by certain platforms
#include <AzCore/std/parallel/mutex.h>
#include <AzCore/std/parallel/lock.h>
#include <AzCore/std/containers/intrusive_list.h>
#include <AzCore/std/containers/intrusive_set.h>

#ifdef _DEBUG
//#define DEBUG_PTR_IN_BUCKET_CHECK // enabled this when NOT sure if PTR in bucket marker check is successfully
#endif

#include <AzCore/Debug/StackTracer.h>
#include <AzCore/std/containers/set.h>

// Enable if AZ_Assert is making things worse (since AZ_Assert may end up doing allocations)
//#include <assert.h>
//#define _HPHA_ASSERT1(_exp)         assert(_exp)
//#define _HPHA_ASSERT2(_exp, reason) assert(_exp)
#define _HPHA_ASSERT1(_exp) AZ_Assert(_exp, "HPHA Assert, expression: \"" #_exp "\"")
#define _HPHA_ASSERT2(_exp, _reason) AZ_Assert(_exp, "HPHA Assert, expression: \"" #_exp "\", possible reason: " #_reason)
#define _GET_MACRO12(_1, _2, NAME, ...) NAME
#define _EXPAND(x) x
#define HPPA_ASSERT(...) _EXPAND(_GET_MACRO12(__VA_ARGS__, _HPHA_ASSERT2, _HPHA_ASSERT1)(__VA_ARGS__))

#define _HPPA_ASSERT_PRINT_STACK2(_cond, _it)                                                                                              \
    {                                                                                                                                      \
        if (!_cond)                                                                                                                        \
            _it->print_stack();                                                                                                            \
        _HPHA_ASSERT1(_cond);                                                                                                              \
    }
#define _HPPA_ASSERT_PRINT_STACK3(_cond, _it, _reason)                                                                                     \
    {                                                                                                                                      \
        if (!_cond)                                                                                                                        \
            _it->print_stack();                                                                                                            \
        _HPHA_ASSERT2(_cond, _reason);                                                                                                     \
    }
#define _GET_MACRO23(_1, _2, _3, NAME, ...) NAME
#define HPPA_ASSERT_PRINT_STACK(...) _EXPAND(_GET_MACRO23(__VA_ARGS__, _HPPA_ASSERT_PRINT_STACK3, _HPPA_ASSERT_PRINT_STACK2)(__VA_ARGS__))

namespace AZ
{
    /// default windows virtual page size \todo Read this from the OS when we create the allocator)
#define OS_VIRTUAL_PAGE_SIZE AZ_PAGE_SIZE
    //////////////////////////////////////////////////////////////////////////

#define MULTITHREADED

#ifdef MULTITHREADED
#   define  SPIN_COUNT 4000
#endif

// Enabled mutex per bucket
#define USE_MUTEX_PER_BUCKET

    //////////////////////////////////////////////////////////////////////////

    template<bool DebugAllocatorEnable>
    class HphaSchemaBase<DebugAllocatorEnable>::HpAllocator
        : public IAllocator
    {
    public:
        // the guard size controls how many extra bytes are stored after
        // every allocation payload to detect memory stomps
        static constexpr size_t MEMORY_GUARD_SIZE = DebugAllocatorEnable ? 16UL : 0;

        // minimum allocation size, must be a power of two
        // and it needs to be able to fit a pointer
        static const size_t MIN_ALLOCATION_LOG2 = 3UL;
        static const size_t MIN_ALLOCATION = 1UL << MIN_ALLOCATION_LOG2;

        // the maximum small allocation, anything larger goes to the tree HpAllocator
        // must be a power of two
        static const size_t MAX_SMALL_ALLOCATION_LOG2 = 9UL;
        static const size_t MAX_SMALL_ALLOCATION = 1UL << MAX_SMALL_ALLOCATION_LOG2;

        // default alignment, must be a power of two
        static const size_t DEFAULT_ALIGNMENT = sizeof(double);

        static const size_t NUM_BUCKETS = (MAX_SMALL_ALLOCATION / MIN_ALLOCATION);

        static inline bool is_small_allocation(size_t s)
        {
            return s + MEMORY_GUARD_SIZE <= MAX_SMALL_ALLOCATION;
        }
        static inline size_t clamp_small_allocation(size_t s)
        {
            return (s + MEMORY_GUARD_SIZE < MIN_ALLOCATION) ? MIN_ALLOCATION - MEMORY_GUARD_SIZE : s;
        }

        // bucket spacing functions control how the size-space is divided between buckets
        // currently we use linear spacing, could be changed to logarithmic etc
        static inline unsigned bucket_spacing_function(size_t size)
        {
            return (unsigned)((size + (MIN_ALLOCATION - 1)) >> MIN_ALLOCATION_LOG2) - 1;
        }
        static inline unsigned bucket_spacing_function_aligned(size_t size)
        {
            return (unsigned)(size >> MIN_ALLOCATION_LOG2) - 1;
        }
        static inline size_t bucket_spacing_function_inverse(unsigned index)
        {
            return (size_t)(index + 1) << MIN_ALLOCATION_LOG2;
        }

        // block header is where the large HpAllocator stores its book-keeping information
        // it is always located in front of the payload block
        class block_header
        {
            enum block_flags : uint64_t
            {
                BL_USED = 1,
                BL_FLAG_MASK = 0x3
            };
            block_header* mPrev;
            // 16 bits of tag, 46 bits of size, 2 bits of flags (used or not)
            uint64_t mSizeAndFlags;

        public:
            using block_ptr = block_header*;
            size_t size() const
            {
                return mSizeAndFlags & ~BL_FLAG_MASK;
            }
            block_ptr next() const
            {
                return (block_ptr)((char*)mem() + size());
            }
            block_ptr prev() const
            {
                return mPrev;
            }
            void* mem() const
            {
                return (void*)((char*)this + sizeof(block_header));
            }
            bool used() const
            {
                return (mSizeAndFlags & BL_USED) != 0;
            }
            void set_used()
            {
                mSizeAndFlags |= BL_USED;
            }
            void set_unused()
            {
                mSizeAndFlags &= ~BL_USED;
            }
            void unlink()
            {
                next()->prev(prev());
                prev()->next(next());
            }
            void link_after(block_ptr link)
            {
                prev(link);
                next(link->next());
                next()->prev(this);
                prev()->next(this);
            }
            void size(size_t size)
            {
                HPPA_ASSERT((size & BL_FLAG_MASK) == 0);
                mSizeAndFlags = (mSizeAndFlags & BL_FLAG_MASK) | size;
            }
            void next(block_ptr next)
            {
                HPPA_ASSERT(next >= mem());
                size((char*)next - (char*)mem());
            }
            void prev(block_ptr prev)
            {
                mPrev = prev;
            }
        };

        struct block_header_proxy
        {
            unsigned char _proxy_size[sizeof(block_header)];
        };

        // the page structure is where the small HpAllocator stores all its book-keeping information
        // it is always located at the front of a OS page
        struct free_link
        {
            free_link* mNext;
        };
        struct page
            : public block_header_proxy /* must be first */
            , public AZStd::list_base_hook<page>::node_type
        {
            page(size_t elemSize, size_t pageSize, size_t marker)
                : mBucketIndex((unsigned short)bucket_spacing_function_aligned(elemSize))
                , mUseCount(0)
            {
                mMarker = marker ^ ((size_t)this);

                // build the free list inside the new page
                // the page info sits at the front of the page
                size_t numElements = (pageSize - sizeof(page)) / elemSize;
                char* endMem = (char*)this + pageSize;
                char* currentMem = endMem - numElements * elemSize;
                char* nextMem = currentMem + elemSize;
                mFreeList = (free_link*)currentMem;
                while (nextMem != endMem)
                {
                    ((free_link*)currentMem)->mNext = (free_link*)(nextMem);
                    currentMem = nextMem;
                    nextMem += elemSize;
                }
                ((free_link*)currentMem)->mNext = nullptr;
            }

            inline void setInvalid() volatile
            {
                mMarker = 0;
            }

            free_link* mFreeList = nullptr;
            size_t mMarker = 0;
            unsigned short mBucketIndex = 0;
            unsigned short mUseCount = 0;

            size_t elem_size() const
            {
                return bucket_spacing_function_inverse(mBucketIndex);
            }
            unsigned bucket_index() const
            {
                return mBucketIndex;
            }
            size_t count() const
            {
                return mUseCount;
            }
            bool empty() const
            {
                return mUseCount == 0;
            }
            void inc_ref()
            {
                mUseCount++;
            }
            void dec_ref()
            {
                HPPA_ASSERT(mUseCount > 0);
                mUseCount--;
            }
            bool check_marker(size_t marker) const
            {
                return mMarker == (marker ^ ((size_t)this));
            }
        };
        using page_list = AZStd::intrusive_list<page, AZStd::list_base_hook<page>>;

#if defined(MULTITHREADED) && defined(USE_MUTEX_PER_BUCKET)
        static constexpr size_t BucketAlignment = AlignUpToPowerOfTwo(sizeof(page_list) + sizeof(AZStd::mutex) + sizeof(size_t));
#else
        static constexpr size_t BucketAlignment = AlignUpToPowerOfTwo(sizeof(page_list) + sizeof(size_t));
#endif
        AZ_PUSH_DISABLE_WARNING_MSVC(4324)
        class alignas(BucketAlignment) bucket
        {
            page_list mPageList;
#if defined(MULTITHREADED) && defined(USE_MUTEX_PER_BUCKET)
            mutable AZStd::mutex mLock;
#endif
            size_t mMarker;

        public:
            bucket();
#if defined(MULTITHREADED) && defined(USE_MUTEX_PER_BUCKET)
            AZStd::mutex& get_lock() const
            {
                return mLock;
            }
#endif
            size_t marker() const
            {
                return mMarker;
            }
            auto page_list_begin() const
            {
                return mPageList.begin();
            }
            auto page_list_begin()
            {
                return mPageList.begin();
            }
            auto page_list_end() const
            {
                return mPageList.end();
            }
            auto page_list_end()
            {
                return mPageList.end();
            }
            bool page_list_empty() const
            {
                return mPageList.empty();
            }
            void add_free_page(page* p)
            {
                mPageList.push_front(*p);
            }
            page* get_free_page();
            const page* get_free_page() const;
            void* alloc(page* p);
            void free(page* p, void* ptr);
            void unlink(page* p);
        };

        AZ_POP_DISABLE_WARNING_MSVC
        void* bucket_system_alloc();
        void bucket_system_free(void* ptr);
        page* bucket_grow(size_t elemSize, size_t marker);
        AllocateAddress bucket_alloc(size_t size);
        AllocateAddress bucket_alloc_direct(unsigned bi);
        AllocateAddress bucket_realloc(void* ptr, size_t size);
        AllocateAddress bucket_realloc_aligned(void* ptr, size_t size, size_t alignment);
        size_type bucket_free(void* ptr);
        size_type bucket_free_direct(void* ptr, unsigned bi);
        /// return the block size for the pointer.
        size_t bucket_ptr_size(void* ptr) const;
        size_t bucket_get_max_allocation() const;
        size_t bucket_get_unused_memory(bool isPrint) const;
        void bucket_purge();

        // locate the page information from a pointer
        inline page* ptr_get_page(void* ptr) const
        {
            return (page*)AZ::PointerAlignDown((char*)ptr, m_poolPageSize);
        }

        bool ptr_in_bucket(void* ptr) const
        {
            bool result = false;
            page* p = ptr_get_page(ptr);
            unsigned bi = p->bucket_index();
            if (bi < NUM_BUCKETS)
            {
                result = p->check_marker(mBuckets[bi].marker());
                // there's a minimal chance the marker check is not sufficient
                // due to random data that happens to match the marker
                // the exhaustive search below will catch this case
                // and that will indicate that more secure measures are needed
#ifdef DEBUG_PTR_IN_BUCKET_CHECK
#ifdef MULTITHREADED
                AZStd::lock_guard<AZStd::mutex> lock(mBuckets[bi].get_lock());
#endif
                const page* pe = mBuckets[bi].page_list_end();
                const page* pb = mBuckets[bi].page_list_begin();
                for (; pb != pe && pb != p; pb = pb->next())
                {
                }
                HPPA_ASSERT(result == (pb == p));
#endif
            }
            return result;
        }

        // free node is what the large HpAllocator uses to find free space
        // it's stored next to the block header when a block is not in use
        struct free_node : public AZStd::intrusive_multiset_node<free_node>
        {
            block_header* get_block() const
            {
                return (block_header*)((char*)this - sizeof(block_header));
            }
            operator size_t() const
            {
                return get_block()->size();
            }

            friend bool operator<(const free_node& lhs, const free_node& rhs)
            {
                return static_cast<size_t>(lhs) < static_cast<size_t>(rhs);
            }
            friend bool operator<(const free_node& lhs, size_t rhs)
            {
                return static_cast<size_t>(lhs) < rhs;
            }
            friend bool operator<(size_t rhs, const free_node& lhs)
            {
                return rhs < static_cast<size_t>(lhs);
            }
        };

        using free_node_tree = AZStd::intrusive_multiset<free_node, AZStd::intrusive_multiset_base_hook<free_node>, AZStd::less<>>;

        static inline block_header* ptr_get_block_header(void* ptr)
        {
            return (block_header*)((char*)ptr - sizeof(block_header));
        }

        // helper functions to manipulate block headers
        void split_block(block_header* bl, size_t size);
        block_header* coalesce_block(block_header* bl);

        void* tree_system_alloc(size_t size);
        void tree_system_free(void* ptr, size_t size);
        block_header* tree_extract(size_t size);
        block_header* tree_extract_aligned(size_t size, size_t alignment);
        block_header* tree_extract_bucket_page();
        block_header* tree_add_block(void* mem, size_t size);
        block_header* tree_grow(size_t size);
        block_header* tree_grow_aligned(size_t size, size_t alignment);
        void tree_attach(block_header* bl);
        void tree_detach(block_header* bl);
        void tree_purge_block(block_header* bl);

        AllocateAddress tree_alloc(size_t size);
        AllocateAddress tree_alloc_aligned(size_t size, size_t alignment);
        AllocateAddress tree_realloc(void* ptr, size_t size);
        AllocateAddress tree_realloc_aligned(void* ptr, size_t size, size_t alignment);
        size_t tree_resize(void* ptr, size_t size);
        size_type tree_free(void* ptr);
        size_t tree_ptr_size(void* ptr) const;
        size_t tree_get_max_allocation() const;
        size_t tree_get_unused_memory(bool isPrint) const;
        void tree_purge();

        bucket mBuckets[NUM_BUCKETS];
        free_node_tree mFreeTree;
#ifdef MULTITHREADED
        mutable AZStd::recursive_mutex mTreeMutex;
#endif

        enum debug_source
        {
            DEBUG_SOURCE_BUCKETS = 0,
            DEBUG_SOURCE_TREE = 1,
            DEBUG_SOURCE_SIZE = 2,
            DEBUG_SOURCE_INVALID = DEBUG_SOURCE_SIZE
        };
        enum memory_debugging_flags
        {
            DEBUG_FLAGS_NONE = 0,
            DEBUG_FLAGS_FILLING = 1,
            DEBUG_FLAGS_GUARD_CHECK = 2,

            DEBUG_FLAGS_ALL = DEBUG_FLAGS_FILLING | DEBUG_FLAGS_GUARD_CHECK
        };
        static constexpr size_t DEBUG_UNKNOWN_SIZE = (size_t)-1;

        // debug record stores all debugging information for every allocation
        class debug_record
        {
        public:
            static const unsigned MAX_CALLSTACK_DEPTH = 16;
            debug_record()
                : mPtr(nullptr)
                , mSize(0)
                , mSource(DEBUG_SOURCE_INVALID)
                , mGuardByte(0)
                , mCallStack()
            {
            }

            debug_record(void* ptr) // used for searching based on the pointer
                : mPtr(ptr)
                , mSize(0)
                , mSource(DEBUG_SOURCE_INVALID)
                , mGuardByte(0)
                , mCallStack()
            {
            }

            debug_record(void* ptr, size_t size, debug_source source)
                : mPtr(ptr)
                , mSize(size)
                , mSource(source)
                , mCallStack()
            {
            }

            void* ptr() const
            {
                return mPtr;
            }
            void set_ptr(void* ptr)
            {
                mPtr = ptr;
            }

            size_t size() const
            {
                return mSize;
            }
            void set_size(size_t size)
            {
                mSize = size;
            }

            debug_source source() const
            {
                return mSource;
            }

            void print_stack() const;
            void record_stack();

            void write_guard();
            bool check_guard() const;

            // required for the multiset comparator
            bool operator<(const debug_record& other) const
            {
                return mPtr < other.mPtr;
            }

        private:
            void* mPtr;
            size_t mSize;
            debug_source mSource;
            unsigned char mGuardByte;
            AZ::Debug::StackFrame mCallStack[MAX_CALLSTACK_DEPTH];
        };

        // helper structure for returning debug record information
        struct debug_info
        {
            size_t size;
            debug_source source;
            debug_info(size_t _size, debug_source _source)
                : size(_size)
                , source(_source)
            {
            }
        };

        // record map that keeps all debug records in a set, sorted by memory address of the allocation
        class debug_record_map : public AZStd::set<debug_record, AZStd::less<debug_record>, AZStd::stateless_allocator>
        {
            using base = AZStd::set<debug_record, AZStd::less<debug_record>, AZStd::stateless_allocator>;

            static void memory_fill(void* ptr, size_t size);

        public:
            using base::base;

            using const_iterator = typename base::const_iterator;
            using iterator = typename base::iterator;
            void add(void* ptr, size_t size, debug_source source, memory_debugging_flags flags);
            debug_info remove(void* ptr, size_t size, debug_source source, memory_debugging_flags flags);

            void check(void* ptr) const;
        };

        // Packs all the debug data members into a struct
        // that is added as a member of the HpAllocator<true> specialization
        // but not the HpAllocator<false> specialization
        struct DebugAllocatorData
        {
            debug_record_map m_debugMap;
        #ifdef MULTITHREADED
            mutable AZStd::mutex m_debugMutex;
        #endif
            size_t m_totalDebugRequestedSize[DEBUG_SOURCE_SIZE];
        };

        struct EmptyClass {};

        using DebugAllocatorData_t = AZStd::conditional_t<DebugAllocatorEnable, DebugAllocatorData, EmptyClass>;
        AZ_NO_UNIQUE_ADDRESS DebugAllocatorData_t m_debugData;


        void debug_add(void* ptr, size_t size, debug_source source, memory_debugging_flags flags = DEBUG_FLAGS_ALL);
        void debug_remove(void* ptr, size_t size, debug_source source, memory_debugging_flags flags = DEBUG_FLAGS_ALL);

        void debug_check(void*) const;


        // Bucket-dependent counters need to atomic since the locks that protect bucket allocations are per bucket
        // So multiple threads could be updating these counters
        AZStd::atomic<size_t> mTotalAllocatedSizeBuckets = 0;
        AZStd::atomic<size_t> mTotalCapacitySizeBuckets = 0;
        // In the case of tree allocations, there is a lock on the tree, so these counters are protected from multiple
        // threads through that lock
        size_t mTotalAllocatedSizeTree = 0;
        size_t mTotalCapacitySizeTree = 0;
    public:
        HpAllocator();
        ~HpAllocator() override;

        AllocateAddress allocate(size_type byteSize, align_type alignment = 1) override;
        size_type deallocate(pointer ptr, size_type byteSize = 0, align_type alignment = 0) override;
        AllocateAddress reallocate(pointer ptr, size_type newSize, align_type alignment = 1) override;
        size_type get_allocated_size(pointer ptr, align_type alignment = 1) const override;

        // allocate memory using DEFAULT_ALIGNMENT
        // size == 0 returns NULL
        inline AllocateAddress alloc(size_t size)
        {
            if (size == 0)
            {
                return AllocateAddress{};
            }
            if (is_small_allocation(size))
            {
                size = clamp_small_allocation(size);
                AllocateAddress ptr = bucket_alloc_direct(bucket_spacing_function(size + MEMORY_GUARD_SIZE));
                debug_add(ptr, size, DEBUG_SOURCE_BUCKETS);
                return ptr;
            }
            else
            {
                AllocateAddress ptr = tree_alloc(size + MEMORY_GUARD_SIZE);
                debug_add(ptr, size, DEBUG_SOURCE_TREE);
                return ptr;
            }
        }

        // allocate memory with a specific alignment
        // size == 0 returns NULL
        // alignment <= DEFAULT_ALIGNMENT acts as alloc
        inline AllocateAddress alloc(size_t size, size_t alignment)
        {
            HPPA_ASSERT((alignment & (alignment - 1)) == 0);
            if (alignment <= DEFAULT_ALIGNMENT)
            {
                return alloc(size);
            }
            if (size == 0)
            {
                return AllocateAddress{};
            }
            if (is_small_allocation(size) && alignment <= MAX_SMALL_ALLOCATION)
            {
                size = clamp_small_allocation(size);
                AllocateAddress ptr = bucket_alloc_direct(bucket_spacing_function(AZ::SizeAlignUp(size + MEMORY_GUARD_SIZE, alignment)));
                debug_add(ptr, size, DEBUG_SOURCE_BUCKETS);
                return ptr;
            }
            else
            {
                AllocateAddress ptr = tree_alloc_aligned(size + MEMORY_GUARD_SIZE, alignment);
                debug_add(ptr, size, DEBUG_SOURCE_TREE);
                return ptr;
            }
        }

        // realloc the memory block using DEFAULT_ALIGNMENT
        // ptr == NULL acts as alloc
        // size == 0 acts as free
        AllocateAddress realloc(void* ptr, size_t size)
        {
            if (ptr == nullptr)
            {
                return alloc(size);
            }
            if (size == 0)
            {
                free(ptr);
                return AllocateAddress{};
            }
            debug_check(ptr);
            AllocateAddress newPtr;
            if (ptr_in_bucket(ptr))
            {
                if (is_small_allocation(size))
                {
                    size = clamp_small_allocation(size);
                    debug_remove(
                        ptr, DEBUG_UNKNOWN_SIZE, DEBUG_SOURCE_BUCKETS, DEBUG_FLAGS_GUARD_CHECK); // do not fill since the memory is reused
                    newPtr = bucket_realloc(ptr, size + MEMORY_GUARD_SIZE);
                    debug_add(newPtr, size, DEBUG_SOURCE_BUCKETS, DEBUG_FLAGS_NONE); // do not fill since the memory is reused
                }
                else
                {
                    newPtr = tree_alloc(size + MEMORY_GUARD_SIZE);
                    if (newPtr)
                    {
                        debug_add(newPtr, size, DEBUG_SOURCE_TREE);
                        HPPA_ASSERT(size >= (ptr_get_page(ptr)->elem_size() - MEMORY_GUARD_SIZE));
                        memcpy(newPtr, ptr, ptr_get_page(ptr)->elem_size() - MEMORY_GUARD_SIZE);
                        debug_remove(ptr, DEBUG_UNKNOWN_SIZE, DEBUG_SOURCE_BUCKETS);
                        bucket_free(ptr);
                    }
                }
            }
            else
            {
                if (is_small_allocation(size))
                {
                    size = clamp_small_allocation(size);
                    newPtr = bucket_alloc(size + MEMORY_GUARD_SIZE);
                    if (newPtr)
                    {
                        debug_add(newPtr, size, DEBUG_SOURCE_BUCKETS);
                        HPPA_ASSERT(size <= ptr_get_block_header(ptr)->size() - MEMORY_GUARD_SIZE);
                        memcpy(newPtr, ptr, size);
                        debug_remove(ptr, DEBUG_UNKNOWN_SIZE, DEBUG_SOURCE_TREE);
                        tree_free(ptr);
                    }
                }
                else
                {
                    debug_remove(
                        ptr, DEBUG_UNKNOWN_SIZE, DEBUG_SOURCE_TREE, DEBUG_FLAGS_GUARD_CHECK); // do not fill since the memory is reused
                    newPtr = tree_realloc(ptr, size + MEMORY_GUARD_SIZE);
                    debug_add(newPtr, size, DEBUG_SOURCE_TREE, DEBUG_FLAGS_NONE); // do not fill since the memory is reused
                }
            }
            return newPtr;
        }

        // realloc the memory block using a specific alignment
        // ptr == NULL acts as alloc
        // size == 0 acts as free
        // alignment <= DEFAULT_ALIGNMENT acts as realloc
        AllocateAddress realloc(void* ptr, size_t size, size_t alignment)
        {
            HPPA_ASSERT((alignment & (alignment - 1)) == 0);
            if (alignment <= DEFAULT_ALIGNMENT)
            {
                return realloc(ptr, size);
            }
            if (ptr == nullptr)
            {
                return alloc(size, alignment);
            }
            if (size == 0)
            {
                free(ptr);
                return AllocateAddress{};
            }
            if ((size_t)ptr & (alignment - 1))
            {
                AllocateAddress newPtr = alloc(size, alignment);
                if (!newPtr)
                {
                    return AllocateAddress{};
                }
                size_t count = this->size(ptr);
                if (count > size)
                {
                    count = size;
                }
                memcpy(newPtr, ptr, count);
                free(ptr);
                return newPtr;
            }
            debug_check(ptr);
            AllocateAddress newPtr;
            if (ptr_in_bucket(ptr))
            {
                if (is_small_allocation(size) && alignment <= MAX_SMALL_ALLOCATION)
                {
                    size = clamp_small_allocation(size);
                    debug_remove(
                        ptr, DEBUG_UNKNOWN_SIZE, DEBUG_SOURCE_BUCKETS, DEBUG_FLAGS_GUARD_CHECK); // do not fill since the memory is reused
                    newPtr = bucket_realloc_aligned(ptr, size + MEMORY_GUARD_SIZE, alignment);
                    debug_add(newPtr, size, DEBUG_SOURCE_BUCKETS, DEBUG_FLAGS_NONE); // do not fill since the memory is reused
                }
                else
                {
                    newPtr = tree_alloc_aligned(size + MEMORY_GUARD_SIZE, alignment);
                    if (newPtr)
                    {
                        debug_add(newPtr, size, DEBUG_SOURCE_TREE);
                        HPPA_ASSERT(size >= (ptr_get_page(ptr)->elem_size() - MEMORY_GUARD_SIZE));
                        memcpy(newPtr, ptr, ptr_get_page(ptr)->elem_size() - MEMORY_GUARD_SIZE);
                        debug_remove(ptr, DEBUG_UNKNOWN_SIZE, DEBUG_SOURCE_BUCKETS);
                        bucket_free(ptr);
                    }
                }
            }
            else
            {
                if (is_small_allocation(size) && alignment <= MAX_SMALL_ALLOCATION)
                {
                    size = clamp_small_allocation(size);
                    newPtr = bucket_alloc_direct(bucket_spacing_function(AZ::SizeAlignUp(size + MEMORY_GUARD_SIZE, alignment)));
                    if (newPtr)
                    {
                        debug_add(newPtr, size, DEBUG_SOURCE_BUCKETS);
                        HPPA_ASSERT(size <= ptr_get_block_header(ptr)->size() - MEMORY_GUARD_SIZE);
                        memcpy(newPtr, ptr, size);
                        debug_remove(ptr, DEBUG_UNKNOWN_SIZE, DEBUG_SOURCE_TREE);
                        tree_free(ptr);
                    }
                }
                else
                {
                    debug_remove(
                        ptr, DEBUG_UNKNOWN_SIZE, DEBUG_SOURCE_TREE, DEBUG_FLAGS_GUARD_CHECK); // do not fill since the memory is reused
                    newPtr = tree_realloc_aligned(ptr, size + MEMORY_GUARD_SIZE, alignment);
                    debug_add(newPtr, size, DEBUG_SOURCE_TREE, DEBUG_FLAGS_NONE); // do not fill since the memory is reused
                }
            }
            return newPtr;
        }

        // resize the memory block to the extent possible
        // returns the size of the resulting memory block
        inline size_t resize(void* ptr, size_t size)
        {
            if (ptr == nullptr)
            {
                return 0;
            }
            HPPA_ASSERT(size > 0);
            debug_check(ptr);
            if (ptr_in_bucket(ptr))
            {
                size = ptr_get_page(ptr)->elem_size() - MEMORY_GUARD_SIZE;
                debug_remove(
                    ptr, DEBUG_UNKNOWN_SIZE, DEBUG_SOURCE_BUCKETS, DEBUG_FLAGS_GUARD_CHECK); // do not fill since the memory is reused
                debug_add(ptr, size, DEBUG_SOURCE_BUCKETS, DEBUG_FLAGS_NONE); // do not fill since the memory is reused
                return size;
            }

            // from the tree we should allocate only >= MAX_SMALL_ALLOCATION
            size = AZStd::GetMax(size, MAX_SMALL_ALLOCATION + MIN_ALLOCATION);

            size = tree_resize(ptr, size + MEMORY_GUARD_SIZE) - MEMORY_GUARD_SIZE;
            debug_remove(ptr, DEBUG_UNKNOWN_SIZE, DEBUG_SOURCE_TREE, DEBUG_FLAGS_GUARD_CHECK); // do not fill since the memory is reused
            debug_add(ptr, size, DEBUG_SOURCE_TREE, DEBUG_FLAGS_NONE); // do not fill since the memory is reused
            return size;
        }

        // query the size of the memory block
        inline size_t size(void* ptr) const
        {
            if (ptr == nullptr)
            {
                return 0;
            }
            if (ptr_in_bucket(ptr))
            {
                debug_check(ptr);
                return ptr_get_page(ptr)->elem_size() - MEMORY_GUARD_SIZE;
            }
            block_header* bl = ptr_get_block_header(ptr);
            debug_check(ptr);
            size_t blockSize = bl->size() - MEMORY_GUARD_SIZE;
            return blockSize;
        }

        // Similar method as above, except this one receives an iterator to the debug record. It avoids a lookup and will not lock
        inline size_t size(typename debug_record_map::const_iterator it) const
        {
            if constexpr (DebugAllocatorEnable)
            {
                if (it == m_debugData.m_debugMap.end())
                {
                    return 0;
                }
                if (ptr_in_bucket(it->ptr()))
                {
                    return ptr_get_page(it->ptr())->elem_size() - MEMORY_GUARD_SIZE;
                }
                block_header* bl = ptr_get_block_header(it->ptr());
                size_t blockSize = bl->size() - MEMORY_GUARD_SIZE;
                return blockSize;
            }
            else
            {
                return {};
            }
        }

        // free the memory block
        inline size_type free(void* ptr)
        {
            if (ptr == nullptr)
            {
                return 0;
            }
            if (ptr_in_bucket(ptr))
            {
                debug_remove(ptr, DEBUG_UNKNOWN_SIZE, DEBUG_SOURCE_BUCKETS);
                return bucket_free(ptr);
            }
            debug_remove(ptr, DEBUG_UNKNOWN_SIZE, DEBUG_SOURCE_TREE);
            return tree_free(ptr);
        }

        // free the memory block supplying the original size with DEFAULT_ALIGNMENT
        inline size_type free(void* ptr, size_t origSize)
        {
            if (ptr == nullptr)
            {
                return 0;
            }
            if (is_small_allocation(origSize))
            {
                // if this asserts probably the original alloc used alignment
                HPPA_ASSERT(ptr_in_bucket(ptr));
                debug_remove(ptr, origSize, DEBUG_SOURCE_BUCKETS);
                return bucket_free_direct(ptr, bucket_spacing_function(origSize + MEMORY_GUARD_SIZE));
            }
            debug_remove(ptr, origSize, DEBUG_SOURCE_TREE);
            return tree_free(ptr);
        }

        // free the memory block supplying the original size and alignment
        inline size_type free(void* ptr, size_t origSize, size_t oldAlignment)
        {
            if (ptr == nullptr)
            {
                return 0;
            }
            if (is_small_allocation(origSize) && oldAlignment <= MAX_SMALL_ALLOCATION)
            {
                HPPA_ASSERT(ptr_in_bucket(ptr), "small object ptr not in a bucket");
                debug_remove(ptr, origSize, DEBUG_SOURCE_BUCKETS);
                return bucket_free_direct(ptr, bucket_spacing_function(AZ::SizeAlignUp(origSize + MEMORY_GUARD_SIZE, oldAlignment)));
            }
            debug_remove(ptr, origSize, DEBUG_SOURCE_TREE);
            return tree_free(ptr);
        }

        // return all unused memory pages to the OS
        // this function doesn't need to be called unless needed
        // it can be called periodically when large amounts of memory can be reclaimed
        // in all cases memory is never automatically returned to the OS
        void purge()
        {
            // Purge buckets first since they use tree pages
            bucket_purge();
            tree_purge();
        }

        // print HpAllocator statistics
        void report();

        // check memory integrity
        void check();

        // return the total number of allocated memory
        inline size_t allocated() const
        {
            return mTotalAllocatedSizeBuckets + mTotalAllocatedSizeTree;
        }

        /// returns allocation size for the pointer if it belongs to the allocator. result is undefined if the pointer doesn't belong to the allocator.
        size_t  AllocationSize(void* ptr);
        size_t  GetMaxAllocationSize() const;
        size_t  GetMaxContiguousAllocationSize() const;
        size_t  GetUnAllocatedMemory(bool isPrint) const;

        void*   SystemAlloc(size_t size, size_t align);
        void    SystemFree(void* ptr);

        const size_t m_treePageSize;
        const size_t m_treePageAlignment;
        const size_t m_poolPageSize;

#if !defined (USE_MUTEX_PER_BUCKET)
        mutable AZStd::mutex m_mutex;
#endif
    };

#ifdef DEBUG_MULTI_RBTREE
    unsigned intrusive_multi_rbtree_base::check_height(node_base* node) const
    {
        if (node == &mHead)
        {
            return 0;
        }
        if (node->black())
        {
            return check_height(node->child(LEFT)) + check_height(node->child(RIGHT)) + 1;
        }
        HPPA_ASSERT(node->child(LEFT)->black() && node->child(RIGHT)->black());
        unsigned lh = check_height(node->child(LEFT));
        unsigned rh = check_height(node->child(RIGHT));
        HPPA_ASSERT(lh == rh);
        return lh;
    }

    void intrusive_multi_rbtree_base::dump(node_base* node) const
    {
        if (node == nullptr)
        {
            node = mHead.child(LEFT);
        }
        if (!node->nil())
        {
            AZ_TracePrintf(
                "HphaAllocator", "%p(%s|%s|%p|%p|%p)\n", node, node->red() ? "red" : "black", node->parent_side() == LEFT ? "L" : "R",
                reinterpret_cast<void*>(reinterpret_cast<int>(node->parent()) & ~3), node->left()->nil() ? nullptr : node->left(),
                node->right()->nil() ? nullptr : node->right());
            dump(node->left());
            dump(node->right());
        }
    }

    void intrusive_multi_rbtree_base::check() const
    {
        HPPA_ASSERT(mHead.black());
        HPPA_ASSERT(mHead.child(RIGHT) == &mHead);
        HPPA_ASSERT(mHead.child(LEFT) == &mHead || mHead.child(LEFT)->black());
        check_height(mHead.child(LEFT));
    }
#endif

    //////////////////////////////////////////////////////////////////////////
    template<bool DebugAllocatorEnable>
    HphaSchemaBase<DebugAllocatorEnable>::HpAllocator::HpAllocator()
        // We will use the os for direct allocations if memoryBlock == NULL
        // If m_systemChunkSize is specified, use that size for allocating tree blocks from the OS
        // m_treePageAlignment should be OS_VIRTUAL_PAGE_SIZE in all cases with this trait as we work
        // with virtual memory addresses when the tree grows and we cannot specify an alignment in all cases
        : m_treePageSize(OS_VIRTUAL_PAGE_SIZE)
        , m_treePageAlignment(OS_VIRTUAL_PAGE_SIZE)
        , m_poolPageSize(OS_VIRTUAL_PAGE_SIZE)
    {
        if constexpr (DebugAllocatorEnable)
        {
            m_debugData.m_totalDebugRequestedSize[DEBUG_SOURCE_BUCKETS] = 0;
            m_debugData.m_totalDebugRequestedSize[DEBUG_SOURCE_TREE] = 0;
        }
        mTotalAllocatedSizeBuckets = 0;
        mTotalAllocatedSizeTree = 0;

#if AZ_TRAIT_OS_HAS_CRITICAL_SECTION_SPIN_COUNT
#if defined(MULTITHREADED)
        // For some platforms we can use an actual spin lock, test and profile. We don't expect much contention there
        SetCriticalSectionSpinCount((LPCRITICAL_SECTION)mTreeMutex.native_handle(), SPIN_COUNT);
#endif // MULTITHREADED
#endif // AZ_TRAIT_OS_HAS_CRITICAL_SECTION_SPIN_COUNT

#if !defined(USE_MUTEX_PER_BUCKET)
#if AZ_TRAIT_OS_HAS_CRITICAL_SECTION_SPIN_COUNT
#if defined(MULTITHREADED)
        // For some platforms we can use an actual spin lock, test and profile. We don't expect much contention there
        SetCriticalSectionSpinCount(m_mutex.native_handle(), SPIN_COUNT);
#endif // MULTITHREADED
#endif // AZ_TRAIT_OS_HAS_CRITICAL_SECTION_SPIN_COUNT
#endif
    }

    template<bool DebugAllocatorEnable>
    HphaSchemaBase<DebugAllocatorEnable>::HpAllocator::~HpAllocator()
    {
        if constexpr (DebugAllocatorEnable)
        {
            // Check if there are not-freed allocations
            report();
            check();
        }

        purge();

        if constexpr (DebugAllocatorEnable)
        {
            // Check if all the memory was returned to the OS
            for (unsigned i = 0; i < NUM_BUCKETS; i++)
            {
                HPPA_ASSERT(mBuckets[i].page_list_empty(), "small object leak");
            }

#ifdef AZ_ENABLE_TRACING
            if (!mFreeTree.empty())
            {
                auto node = mFreeTree.begin();
                auto end = mFreeTree.end();
                while (node != end)
                {
                    block_header* cur = node->get_block();
                    AZ_TracePrintf("HPHA", "Block in free tree: %p, size=%zi bytes", cur, cur->size());
                    ++node;
                }
            }
#endif
            HPPA_ASSERT(mFreeTree.empty());
        }
    }

    template<bool DebugAllocatorEnable>
    HphaSchemaBase<DebugAllocatorEnable>::HpAllocator::bucket::bucket()
    {
#if AZ_TRAIT_OS_HAS_CRITICAL_SECTION_SPIN_COUNT
#if defined(MULTITHREADED)
#if defined(USE_MUTEX_PER_BUCKET)
        // For some platforms we can use an actual spin lock, test and profile. We don't expect much contention there
        SetCriticalSectionSpinCount((LPCRITICAL_SECTION)mLock.native_handle(), SPIN_COUNT);
#endif
#endif // MULTITHREADED
#endif // AZ_TRAIT_OS_HAS_CRITICAL_SECTION_SPIN_COUNT

        // Initializing Sfmt causes a file read which causes an allocation which prevents us from initializing the allocators before we need
        // malloc overridden. Thats why we use SimpleLcgRandom here
        AZ::SimpleLcgRandom randGenerator = AZ::SimpleLcgRandom(reinterpret_cast<u64>(static_cast<void*>(this)));
        mMarker = size_t(randGenerator.Getu64Random());
    }

    template<bool DebugAllocatorEnable>
    auto HphaSchemaBase<DebugAllocatorEnable>::HpAllocator::bucket::get_free_page() -> page*
    {
        if (!mPageList.empty())
        {
            page* p = &mPageList.front();
            if (p->mFreeList)
            {
                return p;
            }
        }
        return nullptr;
    }

    template<bool DebugAllocatorEnable>
    auto HphaSchemaBase<DebugAllocatorEnable>::HpAllocator::bucket::get_free_page() const -> const page*
    {
        if (!mPageList.empty())
        {
            const page* p = &mPageList.front();
            if (p->mFreeList)
            {
                return p;
            }
        }
        return nullptr;
    }

    template<bool DebugAllocatorEnable>
    void* HphaSchemaBase<DebugAllocatorEnable>::HpAllocator::bucket::alloc(page* p)
    {
        // get an element from the free list
        HPPA_ASSERT(p && p->mFreeList);
        p->inc_ref();
        free_link* free = p->mFreeList;
        free_link* next = free->mNext;
        p->mFreeList = next;
        if (!next)
        {
            // if full, auto sort to back
            mPageList.erase(*p);
            mPageList.push_back(*p);
        }
        return (void*)free;
    }

    template<bool DebugAllocatorEnable>
    void HphaSchemaBase<DebugAllocatorEnable>::HpAllocator::bucket::free(page* p, void* ptr)
    {
        // add the element back to the free list
        free_link* free = p->mFreeList;
        free_link* lnk = (free_link*)ptr;
        lnk->mNext = free;
        p->mFreeList = lnk;
        p->dec_ref();
        if (!free)
        {
            // if the page was previously full, auto sort to front
            mPageList.erase(*p);
            mPageList.push_front(*p);
        }
    }

    template<bool DebugAllocatorEnable>
    void HphaSchemaBase<DebugAllocatorEnable>::HpAllocator::bucket::unlink(page* p)
    {
        mPageList.erase(*p);
    }

    template<bool DebugAllocatorEnable>
    void* HphaSchemaBase<DebugAllocatorEnable>::HpAllocator::bucket_system_alloc()
    {
        void* ptr;
        ptr = SystemAlloc(m_poolPageSize, m_poolPageSize);
        mTotalCapacitySizeBuckets += m_poolPageSize;
        HPPA_ASSERT(((size_t)ptr & (m_poolPageSize - 1)) == 0);
        return ptr;
    }

    template<bool DebugAllocatorEnable>
    void HphaSchemaBase<DebugAllocatorEnable>::HpAllocator::bucket_system_free(void* ptr)
    {
        HPPA_ASSERT(ptr);
        SystemFree(ptr);
        mTotalCapacitySizeBuckets -= m_poolPageSize;
    }

    template<bool DebugAllocatorEnable>
    auto HphaSchemaBase<DebugAllocatorEnable>::HpAllocator::bucket_grow(size_t elemSize, size_t marker) -> page*
    {
        // make sure mUseCount won't overflow
        HPPA_ASSERT((m_poolPageSize - sizeof(page)) / elemSize <= AZStd::numeric_limits<unsigned short>::max());
        if (void* mem = bucket_system_alloc())
        {
            return new (mem) page((unsigned short)elemSize, m_poolPageSize, marker);
        }
        return nullptr;
    }

    template<bool DebugAllocatorEnable>
    AllocateAddress HphaSchemaBase<DebugAllocatorEnable>::HpAllocator::bucket_alloc(size_t size)
    {
        HPPA_ASSERT(size <= MAX_SMALL_ALLOCATION);
        unsigned bi = bucket_spacing_function(size);
        HPPA_ASSERT(bi < NUM_BUCKETS);
#ifdef MULTITHREADED
#if defined(USE_MUTEX_PER_BUCKET)
        AZStd::lock_guard<AZStd::mutex> lock(mBuckets[bi].get_lock());
#else
        AZStd::lock_guard<AZStd::mutex> lock(m_mutex);
#endif
#endif
        // get the page info and check if there's any available elements
        page* p = mBuckets[bi].get_free_page();
        if (!p)
        {
            // get a page from the OS, initialize it and add it to the list
            size_t bsize = bucket_spacing_function_inverse(bi);
            p = bucket_grow(bsize, mBuckets[bi].marker());
            if (!p)
            {
                return AllocateAddress{};
            }
            mBuckets[bi].add_free_page(p);
        }
        HPPA_ASSERT(p->elem_size() >= size);
        mTotalAllocatedSizeBuckets += p->elem_size();
        return AllocateAddress{ mBuckets[bi].alloc(p), p->elem_size() };
    }

    template<bool DebugAllocatorEnable>
    AllocateAddress HphaSchemaBase<DebugAllocatorEnable>::HpAllocator::bucket_alloc_direct(unsigned bi)
    {
        HPPA_ASSERT(bi < NUM_BUCKETS);
#ifdef MULTITHREADED
#if defined(USE_MUTEX_PER_BUCKET)
        AZStd::lock_guard<AZStd::mutex> lock(mBuckets[bi].get_lock());
#else
        AZStd::lock_guard<AZStd::mutex> lock(m_mutex);
#endif
#endif
        page* p = mBuckets[bi].get_free_page();
        if (!p)
        {
            size_t bsize = bucket_spacing_function_inverse(bi);
            p = bucket_grow(bsize, mBuckets[bi].marker());
            if (!p)
            {
                return AllocateAddress{};
            }
            mBuckets[bi].add_free_page(p);
        }
        mTotalAllocatedSizeBuckets += p->elem_size();
        return AllocateAddress(mBuckets[bi].alloc(p), p->elem_size());
    }

    template<bool DebugAllocatorEnable>
    AllocateAddress HphaSchemaBase<DebugAllocatorEnable>::HpAllocator::bucket_realloc(void* ptr, size_t size)
    {
        page* p = ptr_get_page(ptr);
        size_t elemSize = p->elem_size();
        // if we do this and we use the bucket_free with size hint we will crash, as the hit will not be the real index/bucket size
        // if (size <= elemSize)
        if (size == elemSize)
        {
            return AllocateAddress{ ptr, size };
        }
        AllocateAddress newPtr = bucket_alloc(size);
        if (newPtr.GetAddress() == nullptr)
        {
            return AllocateAddress{};
        }
        memcpy(newPtr, ptr, AZStd::GetMin(elemSize - MEMORY_GUARD_SIZE, size - MEMORY_GUARD_SIZE));
        bucket_free(ptr);
        return newPtr;
    }

    template<bool DebugAllocatorEnable>
    AllocateAddress HphaSchemaBase<DebugAllocatorEnable>::HpAllocator::bucket_realloc_aligned(void* ptr, size_t size, size_t alignment)
    {
        page* p = ptr_get_page(ptr);
        size_t elemSize = p->elem_size();
        // if we do this and we use the bucket_free with size hint we will crash, as the hit will not be the real index/bucket size
        // if (size <= elemSize && (elemSize&(alignment-1))==0)
        if (size == elemSize && (elemSize & (alignment - 1)) == 0)
        {
            return AllocateAddress{ ptr, size };
        }
        AllocateAddress newPtr = bucket_alloc_direct(bucket_spacing_function(AZ::SizeAlignUp(size, alignment)));
        if (newPtr.GetAddress() == nullptr)
        {
            return AllocateAddress{};
        }
        memcpy(newPtr, ptr, AZStd::GetMin(elemSize - MEMORY_GUARD_SIZE, size - MEMORY_GUARD_SIZE));
        bucket_free(ptr);
        return newPtr;
    }

    template<bool DebugAllocatorEnable>
    auto HphaSchemaBase<DebugAllocatorEnable>::HpAllocator::bucket_free(void* ptr) -> size_type
    {
        page* p = ptr_get_page(ptr);
        unsigned bi = p->bucket_index();
        HPPA_ASSERT(bi < NUM_BUCKETS);
#ifdef MULTITHREADED
#if defined(USE_MUTEX_PER_BUCKET)
        AZStd::lock_guard<AZStd::mutex> lock(mBuckets[bi].get_lock());
#else
        AZStd::lock_guard<AZStd::mutex> lock(m_mutex);
#endif
#endif
        size_type allocatedByteCount = p->elem_size();
        mTotalAllocatedSizeBuckets -= allocatedByteCount;
        mBuckets[bi].free(p, ptr);

        return allocatedByteCount;
    }

    template<bool DebugAllocatorEnable>
    auto HphaSchemaBase<DebugAllocatorEnable>::HpAllocator::bucket_free_direct(void* ptr, unsigned bi) -> size_type
    {
        HPPA_ASSERT(bi < NUM_BUCKETS);
        page* p = ptr_get_page(ptr);
        // if this asserts, the free size doesn't match the allocated size
        // most likely a class needs a base virtual destructor
        HPPA_ASSERT(bi == p->bucket_index());
#ifdef MULTITHREADED
#if defined(USE_MUTEX_PER_BUCKET)
        AZStd::lock_guard<AZStd::mutex> lock(mBuckets[bi].get_lock());
#else
        AZStd::lock_guard<AZStd::mutex> lock(m_mutex);
#endif
#endif
        size_type allocatedByteCount = p->elem_size();
        mTotalAllocatedSizeBuckets -= allocatedByteCount;
        mBuckets[bi].free(p, ptr);

        return allocatedByteCount;
    }

    template<bool DebugAllocatorEnable>
    size_t HphaSchemaBase<DebugAllocatorEnable>::HpAllocator::bucket_ptr_size(void* ptr) const
    {
        page* p = ptr_get_page(ptr);
        HPPA_ASSERT(p->bucket_index() < NUM_BUCKETS);

#ifdef MULTITHREADED
#if defined(USE_MUTEX_PER_BUCKET)
        unsigned bi = p->bucket_index();
        AZStd::lock_guard<AZStd::mutex> lock(mBuckets[bi].get_lock());
#else
        AZStd::lock_guard<AZStd::mutex> lock(m_mutex);
#endif
#endif
        return p->elem_size() - MEMORY_GUARD_SIZE;
    }

    template<bool DebugAllocatorEnable>
    size_t HphaSchemaBase<DebugAllocatorEnable>::HpAllocator::bucket_get_max_allocation() const
    {
        for (int i = (int)NUM_BUCKETS - 1; i > 0; i--)
        {
#ifdef MULTITHREADED
#if defined(USE_MUTEX_PER_BUCKET)
            AZStd::lock_guard<AZStd::mutex> lock(mBuckets[i].get_lock());
#else
            AZStd::lock_guard<AZStd::mutex> lock(m_mutex);
#endif
#endif
            const page* p = mBuckets[i].get_free_page();
            if (p)
            {
                return p->elem_size();
            }
        }

        return 0;
    }

    template<bool DebugAllocatorEnable>
    size_t HphaSchemaBase<DebugAllocatorEnable>::HpAllocator::bucket_get_unused_memory(bool isPrint) const
    {
        size_t unusedMemory = 0;
        size_t availablePageMemory = m_poolPageSize - sizeof(page);
        for (int i = (int)NUM_BUCKETS - 1; i > 0; i--)
        {
#ifdef MULTITHREADED
#if defined(USE_MUTEX_PER_BUCKET)
            AZStd::lock_guard<AZStd::mutex> lock(mBuckets[i].get_lock());
#else
            AZStd::lock_guard<AZStd::mutex> lock(m_mutex);
#endif
#endif
            auto pageEnd = mBuckets[i].page_list_end();
            for (auto p = mBuckets[i].page_list_begin(); p != pageEnd; )
            {
                // early out if we reach fully occupied page (the remaining should all be full)
                if (p->mFreeList == nullptr)
                {
                    break;
                }
                size_t elementSize = p->elem_size();
                size_t availableMemory = availablePageMemory - (elementSize * p->count());
                unusedMemory += availableMemory;
                if (isPrint)
                {
                    AZ_TracePrintf(
                        "System", "Unused Bucket %d page %p elementSize: %d available: %d elements\n", i, p, elementSize,
                        availableMemory / elementSize);
                }
                p = p->m_next;
            }
        }
        return unusedMemory;
    }

    template<bool DebugAllocatorEnable>
    void HphaSchemaBase<DebugAllocatorEnable>::HpAllocator::bucket_purge()
    {
        for (unsigned i = 0; i < NUM_BUCKETS; i++)
        {
#ifdef MULTITHREADED
    #if defined (USE_MUTEX_PER_BUCKET)
            AZStd::lock_guard<AZStd::mutex> lock(mBuckets[i].get_lock());
    #else
            AZStd::lock_guard<AZStd::mutex> lock(m_mutex);
    #endif
#endif
            auto pageEnd = mBuckets[i].page_list_end();
            for (auto p = mBuckets[i].page_list_begin(); p != pageEnd;)
            {
                // early out if we reach fully occupied page (the remaining should all be full)
                if (p->mFreeList == nullptr)
                {
                    break;
                }
                page* next = p->m_next;
                if (p->empty())
                {
                    HPPA_ASSERT(p->mFreeList);
                    mBuckets[i].unlink(AZStd::to_address(p));
                    p->setInvalid();
                    bucket_system_free(AZStd::to_address(p));
                }
                p = next;
            }
        }
    }

    template<bool DebugAllocatorEnable>
    void HphaSchemaBase<DebugAllocatorEnable>::HpAllocator::split_block(block_header* bl, size_t size)
    {
        HPPA_ASSERT(size + sizeof(block_header) + sizeof(free_node) <= bl->size());
        block_header* newBl = (block_header*)((char*)bl + size + sizeof(block_header));
        newBl->link_after(bl);
        newBl->set_unused();
    }

    template<bool DebugAllocatorEnable>
    auto HphaSchemaBase<DebugAllocatorEnable>::HpAllocator::coalesce_block(block_header* bl) -> block_header*
    {
        HPPA_ASSERT(!bl->used());
        block_header* next = bl->next();
        if (!next->used())
        {
            tree_detach(next);
            next->unlink();
        }
        block_header* prev = bl->prev();
        if (!prev->used())
        {
            tree_detach(prev);
            bl->unlink();
            bl = prev;
        }
        return bl;
    }

    template<bool DebugAllocatorEnable>
    void* HphaSchemaBase<DebugAllocatorEnable>::HpAllocator::tree_system_alloc(size_t size)
    {
        size_t allocSize = AZ::SizeAlignUp(size, OS_VIRTUAL_PAGE_SIZE);
        mTotalCapacitySizeTree += allocSize;
        return SystemAlloc(size, m_treePageAlignment);
    }

    template<bool DebugAllocatorEnable>
    void HphaSchemaBase<DebugAllocatorEnable>::HpAllocator::tree_system_free(void* ptr, size_t size)
    {
        HPPA_ASSERT(ptr);
        (void)size;

        size_t allocSize = AZ::SizeAlignUp(size, OS_VIRTUAL_PAGE_SIZE);
        mTotalCapacitySizeTree -= allocSize;
        SystemFree(ptr);
    }

    template<bool DebugAllocatorEnable>
    auto HphaSchemaBase<DebugAllocatorEnable>::HpAllocator::tree_add_block(void* mem, size_t size) -> block_header*
    {
        memset(mem, 0, sizeof(page));

        // create a dummy block to avoid prev() NULL checks and allow easy block shifts
        // potentially this dummy block might grow (due to shift_block) but not more than sizeof(free_node)
        block_header* front = (block_header*)mem;
        front->prev(nullptr);
        front->size(0);
        front->set_used();
        block_header* back = (block_header*)front->mem();
        back->prev(front);
        back->size(0);
        // now the real free block
        front = back;
        back = (block_header*)((char*)mem + size - sizeof(block_header));
        back->size(0);
        back->set_used();
        front->set_unused();
        front->next(back);
        back->prev(front);
        return front;
    }

    template<bool DebugAllocatorEnable>
    auto HphaSchemaBase<DebugAllocatorEnable>::HpAllocator::tree_grow(size_t size) -> block_header*
    {
        const size_t sizeWithBlockHeaders = size + 3 * sizeof(block_header); // two fences plus one fake
        const size_t newSize =
            (sizeWithBlockHeaders < m_treePageSize) ? AZ::SizeAlignUp(sizeWithBlockHeaders, m_treePageSize) : sizeWithBlockHeaders;
        HPPA_ASSERT(newSize >= sizeWithBlockHeaders);

        if (void* mem = tree_system_alloc(newSize))
        {
            return tree_add_block(mem, newSize);
        }
        return nullptr;
    }

    template<bool DebugAllocatorEnable>
    auto HphaSchemaBase<DebugAllocatorEnable>::HpAllocator::tree_grow_aligned(size_t size, size_t alignment) -> block_header*
    {
        const size_t sizeWithBlockHeaders = size + 3 * sizeof(block_header); // two fences plus one fake

        // tree_system_alloc() will return an offset that is a multiple of OS_VIRTUAL_PAGE_SIZE which is 64KiB
        // The following tree_add_block() call will add a dummy block at the beginning of the allocated offset
        // which has a size of 0 which including the block header would put the offset of the real block header at
        // a multiple of 16(sizeof block_header) and the actual memory address for use at a multiple of 32
        //
        // So tree_system_alloc will return an address of the form (0x10000 * N) where is N some integer
        // mem_offset = (0x10000 * N) (multiple of 64KiB)
        //
        // tree_add_block() then creates a dummy_block of size 0, followed by a real_block of the actual size
        // dummy_block_header_offset = (0x10000 * N) (multiple of 64KiB and also a multiple of 16 as well)
        // real_block_header_offset = (0x10000 * N) + 16 (multiple of 16)
        // real_block_memory_offset = (0x10000 * N) + 32 (multiple of 32)
        //
        // This means the return result tree_add_block() is a 32-byte aligned memory address
        // So any alignment <= 32 doesn't need any additional padding memory to be supplied to tree_system_alloc to work

        // Now if the alignment is 64, then a real_block_memory_offset needs to be adjusted to be a multiple of 64
        // In order for that to occur another block(a free block) needs to be inserted between the dummy block and the real block
        // to be at an offset that is a multiple of 64 - sizeof(block_header)
        // The first candidate for the real block would be to have a block header at offset 48
        // free_block_header_offset = (0x10000 * N) + 16
        // free_block_memory_offset = (0x10000 * N) + 32
        // real_block_header_offset = (0x10000 * N) + 48
        // real_block_memory_offset = (0x10000 * N) + 64
        // However there is a problem here where a free block is re-used as a free node in an intrusive_multiset
        // to make it easier to find the memory allocation free blocks when not in use.
        // Now because a node in an intrusive_multiset is 40 bytes(see `sizeof free_node`) then this enforces
        // that the minimum distance between the free_block_header_offset and real_block_header_offset must be
        // 56 bytes (sizeof(block_header) + sizeof(free_node))
        // But as can be seen the distance from the `free_block_memory_offset` to the `real_block_header_offset`
        // is only 16 bytes.
        // 
        // So the next candidate for the real block would be to have a block header at offset 112 (128 - sizeof(block_header))
        // free_block_header_offset = (0x10000 * N) + 112
        // free_block_memory_offset = (0x10000 * N) + 128
        // This allow the actual real memory address to be to be 64 byte aligned, while making sure the free block
        // has enough space(>= 40 bytes) to store a free node
        //
        // Now once for alignments >=128, the same amount of padding memory that is used for an alignment of 64 would work
        // since the free_block_memory_offset would have 80 bytes of space which is greater than the 40 bytes that is required
        // Ex. alignment=128
        // free_block_header_offset = (0x10000 * N) + 16
        // free_block_memory_offset = (0x10000 * N) + 32
        // real_block_header_offset = (0x10000 * N) + 112
        // real_block_memory_offset = (0x10000 * N) + 128
        // Ex. alignment=256
        // free_block_header_offset = (0x10000 * N) + 16
        // free_block_memory_offset = (0x10000 * N) + 32
        // real_block_header_offset = (0x10000 * N) + 244
        // real_block_memory_offset = (0x10000 * N) + 256
        //
        // Now these *_aligned version of functions are only called when the alignment is > DEFAULT_ALIGNMENT (8)
        // So when the alignment 16 or 32 the `sizeWithBlockHeaders` variable above is sufficient
        // If the alignment is 64, then `sizeWithBlockHeaders` variable needs to make sure it account for at least an alignment of
        // `alignment + 2 * sizeof(block header)` which is 96 bytes to allow a free_block to be inserted
        // before the aligned offset
        // If the alignment >= 128 , then `sizeWithBlockHeaders` variables needs to account for `alignment - 2 *sizeof(block_header)`
        size_t sizeWithBlockHeadersAndAlignmentPadding = sizeWithBlockHeaders;
        if (alignment == 64)
        {
            sizeWithBlockHeadersAndAlignmentPadding += alignment + 2 * sizeof(block_header);
        }
        else if (alignment > 64)
        {
            sizeWithBlockHeadersAndAlignmentPadding += alignment - 2 * sizeof(block_header);
        }

        const size_t newSize =
            (sizeWithBlockHeadersAndAlignmentPadding < m_treePageSize)
            ? AZ::SizeAlignUp(sizeWithBlockHeadersAndAlignmentPadding, m_treePageSize)
            : sizeWithBlockHeadersAndAlignmentPadding;
        HPPA_ASSERT(newSize >= sizeWithBlockHeaders);

        if (void* mem = tree_system_alloc(newSize))
        {
            constexpr size_t MinRequiredSize = sizeof(block_header) + sizeof(free_node);
            block_header* newBl = tree_add_block(mem, newSize);
            // new block memory is only 32-byte aligned at this point
            size_t alignmentOffs = AZ::PointerAlignUp((char*)newBl->mem(), alignment) - (char*)newBl->mem();
            if (alignmentOffs > 0 && alignmentOffs < MinRequiredSize)
            {
                // Make sure the alignment offset is large enough, that a free block
                // can be created between the new block and the previous block
                alignmentOffs = AZ::PointerAlignUp((char*)newBl->mem() + MinRequiredSize, alignment) - (char*)newBl->mem();
            }

            if (alignmentOffs >= MinRequiredSize)
            {
                split_block(newBl, alignmentOffs - sizeof(block_header));
                tree_attach(newBl);
                newBl = newBl->next();
            }

            AZ_Assert(
                reinterpret_cast<uintptr_t>(newBl->mem()) % alignment == 0,
                "The allocated memory address %p is not aligned to %zu",
                newBl->mem(),
                alignment);
            AZ_Assert(newBl->size() >= size, "The allocated memory size %zu is less than %zu", newBl->size(), size);

            return newBl;
        }
        return nullptr;
    }

    template<bool DebugAllocatorEnable>
    auto HphaSchemaBase<DebugAllocatorEnable>::HpAllocator::tree_extract(size_t size) -> block_header*
    {
        // search the tree and get the smallest fitting block
        auto it = mFreeTree.lower_bound(size);
        if (it == mFreeTree.end())
        {
            return nullptr;
        }
        free_node* bestNode = it->next(); // improves removal time
        block_header* bestBlock = bestNode->get_block();
        tree_detach(bestBlock);
        return bestBlock;
    }

    template<bool DebugAllocatorEnable>
    auto HphaSchemaBase<DebugAllocatorEnable>::HpAllocator::tree_extract_aligned(size_t size, size_t alignment) -> block_header*
    {
        // get the sequence of nodes from size to (size + sizeof(block_header) + sizeof(free_node) + (alignment - 1)) including
        constexpr size_t MinRequiredSize = sizeof(block_header) + sizeof(free_node);

        size_t sizeUpper = size + alignment + MinRequiredSize;
        auto bestNode = mFreeTree.lower_bound(size);
        auto lastNode = mFreeTree.upper_bound(sizeUpper);
        while (bestNode != lastNode)
        {
            free_node* node = &*bestNode;
            block_header* candidateBlock = bestNode->get_block();

            size_t  alignmentOffs = AZ::PointerAlignUp((char*)node, alignment) - (char*)node;
            if (alignmentOffs > 0 && alignmentOffs < MinRequiredSize)
            {
                // Make sure the alignment offset is large enough, that a free block
                // can be created between the new block and the previous block
                alignmentOffs = AZ::PointerAlignUp((char*)node + MinRequiredSize, alignment) - (char*)node;
            }
            // The alignmentOffs at this point is either 0 or >= sizeof(block_header) + sizeof(free_node)
            if (candidateBlock->size() >= size + alignmentOffs)
            {
                break;
            }
            // keep walking the sequence till we find a match or reach the end
            // the larger the alignment the more linear searching will be done
            ++bestNode;
        }
        if (bestNode == mFreeTree.end())
        {
            return nullptr;
        }
        if (bestNode == lastNode)
        {
            bestNode = bestNode->next(); // improves removal time
        }

        // Re-query the alignment offset needed to get to a block that is aligned
        free_node* bestNodeAddress = &*bestNode;
        size_t alignmentOffs = AZ::PointerAlignUp((char*)bestNodeAddress, alignment) - (char*)bestNodeAddress;
        if (alignmentOffs > 0 && alignmentOffs < MinRequiredSize)
        {
            // Make sure the alignment offset is large enough, that a free block
            // can be created between the new block and the previous block
            alignmentOffs = AZ::PointerAlignUp((char*)bestNodeAddress + MinRequiredSize, alignment) - (char*)bestNodeAddress;
        }

        block_header* bestBlock = bestNode->get_block();
        tree_detach(bestBlock);

        // Return an block_header that points to aligned block that can be used
        // split_block is used to create a free block for the section
        // between the best block current memory address and the aligned memory address
        if (alignmentOffs >= MinRequiredSize)
        {
            split_block(bestBlock, alignmentOffs - sizeof(block_header));
            tree_attach(bestBlock);
            bestBlock = bestBlock->next();
        }
        return bestBlock;
    }

    template<bool DebugAllocatorEnable>
    auto HphaSchemaBase<DebugAllocatorEnable>::HpAllocator::tree_extract_bucket_page() -> block_header*
    {
        block_header* bestBlock = nullptr;
        size_t alignment = m_poolPageSize;
        size_t size = m_poolPageSize;

        // get the sequence of nodes from size to (size + alignment - 1) including
        size_t sizeUpper = size + alignment;
        auto bestNode = mFreeTree.lower_bound(size);
        auto lastNode = mFreeTree.upper_bound(sizeUpper);
        while (bestNode != lastNode)
        {
            bestBlock = bestNode->get_block();
            size_t alignmentOffs = AZ::PointerAlignUp((char*)bestBlock, alignment) - (char*)bestBlock;
            if (bestBlock->size() + sizeof(block_header) >= size + alignmentOffs)
            {
                break;
            }
            // keep walking the sequence till we find a match or reach the end
            // the larger the alignment the more linear searching will be done
            ++bestNode;
        }
        if (bestNode == mFreeTree.end())
        {
            return nullptr;
        }
        if (bestNode == lastNode)
        {
            bestNode = bestNode->next(); // improves removal time
        }
        bestBlock = bestNode->get_block();
        tree_detach(bestBlock);
        return bestBlock;
    }

    template<bool DebugAllocatorEnable>
    void HphaSchemaBase<DebugAllocatorEnable>::HpAllocator::tree_attach(block_header* bl)
    {
        mFreeTree.insert((free_node*)bl->mem());
    }

    template<bool DebugAllocatorEnable>
    void HphaSchemaBase<DebugAllocatorEnable>::HpAllocator::tree_detach(block_header* bl)
    {
        mFreeTree.erase((free_node*)bl->mem());
    }

    template<bool DebugAllocatorEnable>
    AllocateAddress HphaSchemaBase<DebugAllocatorEnable>::HpAllocator::tree_alloc(size_t size)
    {
#ifdef MULTITHREADED
        AZStd::lock_guard<AZStd::recursive_mutex> lock(mTreeMutex);
#endif
        // modify the size to make sure we can fit the block header and free node
        if (size < sizeof(free_node))
        {
            size = sizeof(free_node);
        }
        size = AZ::SizeAlignUp(size, sizeof(block_header));
        // extract a block from the tree if found
        block_header* newBl = tree_extract(size);
        if (!newBl)
        {
            // ask the OS for more memory
            newBl = tree_grow(size);
            if (!newBl)
            {
                return AllocateAddress{};
            }
        }
        HPPA_ASSERT(!newBl->used());
        HPPA_ASSERT(newBl && newBl->size() >= size);
        // chop off any remainder
        if (newBl->size() >= size + sizeof(block_header) + sizeof(free_node))
        {
            split_block(newBl, size);
            tree_attach(newBl->next());
        }
        newBl->set_used();
        mTotalAllocatedSizeTree += newBl->size();
        return AllocateAddress{ newBl->mem(), newBl->size() };
    }

    template<bool DebugAllocatorEnable>
    AllocateAddress HphaSchemaBase<DebugAllocatorEnable>::HpAllocator::tree_alloc_aligned(size_t size, size_t alignment)
    {
#ifdef MULTITHREADED
        AZStd::lock_guard<AZStd::recursive_mutex> lock(mTreeMutex);
#endif
        if (size < sizeof(free_node))
        {
            size = sizeof(free_node);
        }
        size = AZ::SizeAlignUp(size, sizeof(block_header));

        constexpr size_t MinRequiredSize = sizeof(block_header) + sizeof(free_node);

        // The block header + free_node size is the minimum amount
        // of bytes needed for a block
        block_header* alignedBlockHeader = tree_extract_aligned(size, alignment);
        if (alignedBlockHeader == nullptr)
        {
            alignedBlockHeader = tree_grow_aligned(size, alignment);
            if (alignedBlockHeader == nullptr)
            {
                return AllocateAddress{};
            }
        }

        block_header* newBl = alignedBlockHeader;

        HPPA_ASSERT(newBl && newBl->size() >= size);

        if (newBl->size() >= size + MinRequiredSize)
        {
            split_block(newBl, size);
            tree_attach(newBl->next());
        }
        newBl->set_used();
        HPPA_ASSERT(((size_t)newBl->mem() & (alignment - 1)) == 0);
        mTotalAllocatedSizeTree += newBl->size();
        return AllocateAddress{ newBl->mem(), newBl->size() };
    }

    template<bool DebugAllocatorEnable>
    AllocateAddress HphaSchemaBase<DebugAllocatorEnable>::HpAllocator::tree_realloc(void* ptr, size_t size)
    {
#ifdef MULTITHREADED
        AZStd::lock_guard<AZStd::recursive_mutex> lock(mTreeMutex);
#endif
        if (size < sizeof(free_node))
        {
            size = sizeof(free_node);
        }
        size = AZ::SizeAlignUp(size, sizeof(block_header));
        block_header* bl = ptr_get_block_header(ptr);
        size_t blSize = bl->size();
        if (blSize >= size)
        {
            // the block is being shrunk, truncate and mark the remainder as free
            if (blSize >= size + sizeof(block_header) + sizeof(free_node))
            {
                split_block(bl, size);
                block_header* next = bl->next();
                next = coalesce_block(next);
                tree_attach(next);
                mTotalAllocatedSizeTree += bl->size() - blSize;
            }
            HPPA_ASSERT(bl->size() >= size);
            return AllocateAddress{ ptr, bl->size() };
        }
        // check if the following block is free and if it can accommodate the requested size
        block_header* next = bl->next();
        size_t nextSize = next->used() ? 0 : next->size() + sizeof(block_header);
        if (blSize + nextSize >= size)
        {
            HPPA_ASSERT(!next->used());
            tree_detach(next);
            next->unlink();
            HPPA_ASSERT(bl->size() >= size);
            if (bl->size() >= size + sizeof(block_header) + sizeof(free_node))
            {
                split_block(bl, size);
                tree_attach(bl->next());
            }
            mTotalAllocatedSizeTree += bl->size() - blSize;
            return AllocateAddress{ ptr, bl->size() };
        }
        // check if the previous block can be used to avoid searching
        block_header* prev = bl->prev();
        size_t prevSize = prev->used() ? 0 : prev->size() + sizeof(block_header);
        if (blSize + prevSize + nextSize >= size)
        {
            HPPA_ASSERT(!prev->used());
            tree_detach(prev);
            bl->unlink();
            if (!next->used())
            {
                tree_detach(next);
                next->unlink();
            }
            bl = prev;
            bl->set_used();
            HPPA_ASSERT(bl->size() >= size);
            void* newPtr = bl->mem();
            // move the memory before we split the block
            memmove(newPtr, ptr, blSize - MEMORY_GUARD_SIZE);
            if (bl->size() >= size + sizeof(block_header) + sizeof(free_node))
            {
                split_block(bl, size);
                tree_attach(bl->next());
            }
            mTotalAllocatedSizeTree += bl->size() - blSize;
            return AllocateAddress{ newPtr, bl->size() };
        }
        // fall back to alloc/copy/free
        AllocateAddress newPtr = tree_alloc(size);
        if (newPtr)
        {
            memcpy(newPtr, ptr, blSize - MEMORY_GUARD_SIZE);
            tree_free(ptr);
            return newPtr;
        }
        return AllocateAddress{};
    }

    template<bool DebugAllocatorEnable>
    AllocateAddress HphaSchemaBase<DebugAllocatorEnable>::HpAllocator::tree_realloc_aligned(void* ptr, size_t size, size_t alignment)
    {
        HPPA_ASSERT(((size_t)ptr & (alignment - 1)) == 0);
#ifdef MULTITHREADED
        AZStd::lock_guard<AZStd::recursive_mutex> lock(mTreeMutex);
#endif
        if (size < sizeof(free_node))
        {
            size = sizeof(free_node);
        }
        size = AZ::SizeAlignUp(size, sizeof(block_header));
        block_header* bl = ptr_get_block_header(ptr);
        size_t blSize = bl->size();
        if (blSize >= size)
        {
            if (blSize >= size + sizeof(block_header) + sizeof(free_node))
            {
                split_block(bl, size);
                block_header* next = bl->next();
                next = coalesce_block(next);
                tree_attach(next);
            }
            mTotalAllocatedSizeTree += bl->size() - blSize;
            HPPA_ASSERT(bl->size() >= size);
            return AllocateAddress{ ptr, bl->size() };
        }
        block_header* next = bl->next();
        size_t nextSize = next->used() ? 0 : next->size() + sizeof(block_header);
        if (blSize + nextSize >= size)
        {
            HPPA_ASSERT(!next->used());
            tree_detach(next);
            next->unlink();
            HPPA_ASSERT(bl->size() >= size);
            if (bl->size() >= size + sizeof(block_header) + sizeof(free_node))
            {
                split_block(bl, size);
                tree_attach(bl->next());
            }
            mTotalAllocatedSizeTree += bl->size() - blSize;
            return AllocateAddress{ ptr, bl->size() };
        }
        block_header* prev = bl->prev();
        size_t prevSize = prev->used() ? 0 : prev->size() + sizeof(block_header);
        size_t alignmentOffs = prev->used() ? 0 : AZ::PointerAlignUp((char*)prev->mem(), alignment) - (char*)prev->mem();

        constexpr size_t MinRequiredSize = sizeof(free_node) + sizeof(block_header);
        if (alignmentOffs > 0 && alignmentOffs < MinRequiredSize)
        {
            // Make sure the alignment offset is large enough, that a free block
            // can be created between the new block and the previous block
            alignmentOffs = AZ::PointerAlignUp((char*)prev->mem() + MinRequiredSize, alignment) - (char*)prev->mem();
        }
        if (blSize + prevSize + nextSize >= size + alignmentOffs)
        {
            HPPA_ASSERT(!prev->used());
            tree_detach(prev);
            bl->unlink();
            if (!next->used())
            {
                tree_detach(next);
                next->unlink();
            }
            if (alignmentOffs >= MinRequiredSize)
            {
                split_block(prev, alignmentOffs - sizeof(block_header));
                tree_attach(prev);
                prev = prev->next();
            }
            bl = prev;
            bl->set_used();
            HPPA_ASSERT(bl->size() >= size && ((size_t)bl->mem() & (alignment - 1)) == 0);
            void* newPtr = bl->mem();
            memmove(newPtr, ptr, blSize - MEMORY_GUARD_SIZE);
            if (bl->size() >= size + MinRequiredSize)
            {
                split_block(bl, size);
                tree_attach(bl->next());
            }
            mTotalAllocatedSizeTree += bl->size() - blSize;
            return AllocateAddress{ newPtr, bl->size() };
        }
        AllocateAddress newPtr = tree_alloc_aligned(size, alignment);
        if (newPtr)
        {
            memcpy(newPtr, ptr, blSize - MEMORY_GUARD_SIZE);
            tree_free(ptr);
            return newPtr;
        }
        return AllocateAddress{};
    }

    template<bool DebugAllocatorEnable>
    size_t HphaSchemaBase<DebugAllocatorEnable>::HpAllocator::tree_resize(void* ptr, size_t size)
    {
#ifdef MULTITHREADED
        AZStd::lock_guard<AZStd::recursive_mutex> lock(mTreeMutex);
#endif
        if (size < sizeof(free_node))
        {
            size = sizeof(free_node);
        }
        size = AZ::SizeAlignUp(size, sizeof(block_header));
        block_header* bl = ptr_get_block_header(ptr);
        size_t blSize = bl->size();
        if (blSize >= size)
        {
            if (blSize >= size + sizeof(block_header) + sizeof(free_node))
            {
                split_block(bl, size);
                block_header* next = bl->next();
                next = coalesce_block(next);
                tree_attach(next);
            }
            HPPA_ASSERT(bl->size() >= size);
        }
        else
        {
            block_header* next = bl->next();
            if (!next->used() && blSize + next->size() + sizeof(block_header) >= size)
            {
                tree_detach(next);
                next->unlink();
                if (bl->size() >= size + sizeof(block_header) + sizeof(free_node))
                {
                    split_block(bl, size);
                    tree_attach(bl->next());
                }
                HPPA_ASSERT(bl->size() >= size);
            }
        }
        mTotalAllocatedSizeTree += bl->size() - blSize;
        return bl->size();
    }

    template<bool DebugAllocatorEnable>
    auto HphaSchemaBase<DebugAllocatorEnable>::HpAllocator::tree_free(void* ptr) -> size_type
    {
#ifdef MULTITHREADED
        AZStd::lock_guard<AZStd::recursive_mutex> lock(mTreeMutex);
#endif
        block_header* bl = ptr_get_block_header(ptr);
        // This assert detects a double-free of ptr
        HPPA_ASSERT(bl->used());
        size_type allocatedBytes = bl->size();
        mTotalAllocatedSizeTree -= allocatedBytes;
        bl->set_unused();
        bl = coalesce_block(bl);
        tree_attach(bl);

        return allocatedBytes;
    }


    template<bool DebugAllocatorEnable>
    void HphaSchemaBase<DebugAllocatorEnable>::HpAllocator::tree_purge_block(block_header* bl)
    {
        HPPA_ASSERT(!bl->used());
        HPPA_ASSERT(bl->prev() && bl->prev()->used());
        HPPA_ASSERT(bl->next() && bl->next()->used());
        if (bl->prev()->prev() == nullptr && bl->next()->size() == 0)
        {
            tree_detach(bl);
            char* memStart = (char*)bl->prev();
            char* memEnd = (char*)bl->mem() + bl->size() + sizeof(block_header);
            void* mem = memStart;
            size_t size = memEnd - memStart;
            HPPA_ASSERT(((size_t)mem & (m_treePageAlignment - 1)) == 0);
            memset(mem, 0xff, sizeof(block_header));
            tree_system_free(mem, size);
        }
    }

    template<bool DebugAllocatorEnable>
    size_t HphaSchemaBase<DebugAllocatorEnable>::HpAllocator::tree_ptr_size(void* ptr) const
    {
#ifdef MULTITHREADED
        AZStd::lock_guard<AZStd::recursive_mutex> lock(mTreeMutex);
#endif
        block_header* bl = ptr_get_block_header(ptr);
        if (bl->used()) // add a magic number to avoid better bad pointer data.
        {
            return bl->size();
        }
        else
        {
            return 0;
        }
    }

    template<bool DebugAllocatorEnable>
    size_t HphaSchemaBase<DebugAllocatorEnable>::HpAllocator::tree_get_max_allocation() const
    {
#ifdef MULTITHREADED
        AZStd::lock_guard<AZStd::recursive_mutex> lock(mTreeMutex);
#endif
        return mFreeTree.maximum()->get_block()->size();
    }

    template<bool DebugAllocatorEnable>
    size_t HphaSchemaBase<DebugAllocatorEnable>::HpAllocator::tree_get_unused_memory(bool isPrint) const
    {
#ifdef MULTITHREADED
        AZStd::lock_guard<AZStd::recursive_mutex> lock(mTreeMutex);
#endif
        size_t unusedMemory = 0;
        for (auto it = mFreeTree.begin(); it != mFreeTree.end(); ++it)
        {
            unusedMemory += it->get_block()->size();
            if (isPrint)
            {
                AZ_TracePrintf("System", "Unused Treenode %p size: %zu\n", it->get_block()->mem(), it->get_block()->size());
            }
        }
        return unusedMemory;
    }

    template<bool DebugAllocatorEnable>
    void HphaSchemaBase<DebugAllocatorEnable>::HpAllocator::tree_purge()
    {
#ifdef MULTITHREADED
        AZStd::lock_guard<AZStd::recursive_mutex> lock(mTreeMutex);
#endif
        auto node = mFreeTree.begin();
        auto end = mFreeTree.end();
        while (node != end)
        {
            block_header* cur = node->get_block();
            ++node;
            tree_purge_block(cur);
        }
    }

    template<bool DebugAllocatorEnable>
    AllocateAddress HphaSchemaBase<DebugAllocatorEnable>::HpAllocator::allocate(size_type byteSize, align_type alignment)
    {
        AllocateAddress address = alloc(byteSize, static_cast<size_t>(alignment));
        if (address == nullptr)
        {
            purge();
            address = alloc(byteSize, static_cast<size_t>(alignment));
        }
        return address;
    }

    template<bool DebugAllocatorEnable>
    auto HphaSchemaBase<DebugAllocatorEnable>::HpAllocator::deallocate(pointer ptr, size_type byteSize, align_type alignment)
        -> size_type
    {
        if (ptr == nullptr)
        {
            return 0;
        }
        if (byteSize == 0)
        {
            return free(ptr);
        }
        else if (alignment == 0)
        {
            return free(ptr, byteSize);
        }
        else
        {
            return free(ptr, byteSize, alignment);
        }
    }

    template<bool DebugAllocatorEnable>
    AllocateAddress HphaSchemaBase<DebugAllocatorEnable>::HpAllocator::reallocate(pointer ptr, size_type newSize, align_type alignment)
    {
        AllocateAddress address = realloc(ptr, newSize, static_cast<size_t>(alignment));
        if (address.GetAddress() == nullptr && newSize > 0)
        {
            purge();
            address = realloc(ptr, newSize, static_cast<size_t>(alignment));
        }
        return address;
    }

    template<bool DebugAllocatorEnable>
    auto HphaSchemaBase<DebugAllocatorEnable>::HpAllocator::get_allocated_size(pointer ptr, [[maybe_unused]] align_type alignment) const
        -> size_type
    {
        return size(ptr);
    }

    //=========================================================================
    // allocationSize
    // [2/22/2011]
    //=========================================================================
    template<bool DebugAllocatorEnable>
    size_t HphaSchemaBase<DebugAllocatorEnable>::HpAllocator::AllocationSize(void* ptr)
    {
        return size(ptr);
    }

    //=========================================================================
    // GetMaxAllocationSize
    // [2/22/2011]
    //=========================================================================
    template<bool DebugAllocatorEnable>
    size_t HphaSchemaBase<DebugAllocatorEnable>::HpAllocator::GetMaxAllocationSize() const
    {
        const_cast<HpAllocator*>(this)->purge(); // slow

        size_t maxSize = 0;
        maxSize = AZStd::GetMax(bucket_get_max_allocation(), maxSize);
        maxSize = AZStd::GetMax(tree_get_max_allocation(), maxSize);
        return maxSize;
    }

    template<bool DebugAllocatorEnable>
    size_t HphaSchemaBase<DebugAllocatorEnable>::HpAllocator::GetMaxContiguousAllocationSize() const
    {
        return AZ_CORE_MAX_ALLOCATOR_SIZE;
    }

    //=========================================================================
    // GetUnAllocatedMemory
    // [9/30/2013]
    //=========================================================================
    template<bool DebugAllocatorEnable>
    size_t HphaSchemaBase<DebugAllocatorEnable>::HpAllocator::GetUnAllocatedMemory(bool isPrint) const
    {
        return bucket_get_unused_memory(isPrint) + tree_get_unused_memory(isPrint);
    }

    //=========================================================================
    // SystemAlloc
    // [2/22/2011]
    //=========================================================================
    template<bool DebugAllocatorEnable>
    void* HphaSchemaBase<DebugAllocatorEnable>::HpAllocator::SystemAlloc(size_t size, size_t align)
    {
        AZ_Assert(align % OS_VIRTUAL_PAGE_SIZE == 0, "Invalid allocation/page alignment %d should be a multiple of %d!", size, OS_VIRTUAL_PAGE_SIZE);
        return AZ_OS_MALLOC(size, align);
    }

    //=========================================================================
    // SystemFree
    // [2/22/2011]
    //=========================================================================
    template<bool DebugAllocatorEnable>
    void HphaSchemaBase<DebugAllocatorEnable>::HpAllocator::SystemFree(void* ptr)
    {
        AZ_OS_FREE(ptr);
    }

    template<bool DebugAllocatorEnable>
    void HphaSchemaBase<DebugAllocatorEnable>::HpAllocator::debug_record::print_stack() const
    {
        if constexpr (DebugAllocatorEnable)
        {
            AZ::Debug::SymbolStorage::StackLine stackLines[MAX_CALLSTACK_DEPTH] = { { 0 } };
            auto recordFrameCount = static_cast<unsigned int>(AZStd::GetMin(AZ_ARRAY_SIZE(mCallStack), AZ_ARRAY_SIZE(stackLines)));
            AZ::Debug::SymbolStorage::DecodeFrames(mCallStack, recordFrameCount, stackLines);

            for (int i = 0; i < MAX_CALLSTACK_DEPTH; ++i)
            {
                if (stackLines[i][0] != 0)
                {
                    const size_t lineLength = strlen(stackLines[i]);
                    // Append a line return to the stack line if we have enough room
                    if ((lineLength + 1) < AZ_ARRAY_SIZE(stackLines[i]))
                    {
                        stackLines[i][lineLength] = '\n';
                        stackLines[i][lineLength + 1] = 0;
                    }
                    AZ::Debug::Trace::Instance().Output("HpAllocator", stackLines[i]);
                }
            }
        }
    }

    template<bool DebugAllocatorEnable>
    void HphaSchemaBase<DebugAllocatorEnable>::HpAllocator::debug_record::record_stack()
    {
        if constexpr (DebugAllocatorEnable)
        {
            AZ::Debug::StackRecorder::Record(mCallStack, MAX_CALLSTACK_DEPTH, 6);
        }
    }

    template<bool DebugAllocatorEnable>
    void HphaSchemaBase<DebugAllocatorEnable>::HpAllocator::debug_record::write_guard()
    {
        if constexpr (DebugAllocatorEnable)
        {
            unsigned char guardByte = (unsigned char)rand();
            unsigned char* guard = (unsigned char*)mPtr + mSize;
            mGuardByte = guardByte;
            for (unsigned i = 0; i < MEMORY_GUARD_SIZE; i++)
            {
                guard[i] = guardByte++;
            }
        }
    }

    template<bool DebugAllocatorEnable>
    bool HphaSchemaBase<DebugAllocatorEnable>::HpAllocator::debug_record::check_guard() const
    {
        if constexpr (DebugAllocatorEnable)
        {
            unsigned char guardByte = mGuardByte;
            unsigned char* guard = (unsigned char*)mPtr + mSize;
            for (unsigned i = 0; i < MEMORY_GUARD_SIZE; i++)
            {
                if (guardByte++ != guard[i])
                {
                    return false;
                }
            }
        }
        return true;
    }

    template<bool DebugAllocatorEnable>
    void HphaSchemaBase<DebugAllocatorEnable>::HpAllocator::debug_record_map::memory_fill(void* ptr, size_t size)
    {
        if constexpr (DebugAllocatorEnable)
        {
            unsigned char sFiller[] = { 0xFF, 0xC0, 0xC0, 0xFF }; // QNAN (little OR big endian)
            unsigned char* p = (unsigned char*)ptr;
            for (size_t s = 0; s < size; s++)
            {
                p[s] = sFiller[s % sizeof(sFiller) / sizeof(sFiller[0])];
            }
        }
    }

    template<bool DebugAllocatorEnable>
    void HphaSchemaBase<DebugAllocatorEnable>::HpAllocator::debug_record_map::add(void* ptr, size_t size, debug_source source, memory_debugging_flags flags)
    {
        if constexpr (DebugAllocatorEnable)
        {
            HPPA_ASSERT(size != DEBUG_UNKNOWN_SIZE);
            const auto it = this->emplace(ptr, size, source);

            // Make sure this record is unique
            HPPA_ASSERT_PRINT_STACK(it.second, it.first);

            it.first->write_guard();
            it.first->record_stack();

            if (flags & DEBUG_FLAGS_FILLING)
            {
                memory_fill(ptr, size);
            }
        }
    }

    template<bool DebugAllocatorEnable>
    auto HphaSchemaBase<DebugAllocatorEnable>::HpAllocator::debug_record_map::remove(void* ptr, size_t size, debug_source source, memory_debugging_flags flags)
        -> debug_info
    {
        if constexpr (DebugAllocatorEnable)
        {
            const_iterator it = this->find(debug_record(ptr));
            // if this asserts most likely the pointer was already deleted or that the allocation happens in another allocator
            if (it == this->end())
            {
                HPPA_ASSERT(false, "double delete or pointer not in this allocator");
                return debug_info(0, DEBUG_SOURCE_INVALID);
            }

            HPPA_ASSERT_PRINT_STACK(it->source() == source, it);

            const debug_info debugInfo(it->size(), it->source());
            if (size != DEBUG_UNKNOWN_SIZE)
            {
                HPPA_ASSERT_PRINT_STACK((debugInfo.size <= size), it);
            }
            if (flags & DEBUG_FLAGS_GUARD_CHECK)
            {
                // if this asserts then the memory was corrupted past the end of the block
                HPPA_ASSERT_PRINT_STACK(it->check_guard(), it, "overflow");
            }
            if (flags & DEBUG_FLAGS_FILLING)
            {
                memory_fill(ptr, debugInfo.size);
            }
            this->erase(it);

            return debugInfo;
        }
        else
        {
            return debug_info(0, DEBUG_SOURCE_INVALID);
        }
    }

    template<bool DebugAllocatorEnable>
    void HphaSchemaBase<DebugAllocatorEnable>::HpAllocator::debug_record_map::check(void* ptr) const
    {
        if constexpr (DebugAllocatorEnable)
        {
            const_iterator it = this->find(debug_record(ptr));
            // if this asserts most likely the pointer was already deleted
            HPPA_ASSERT(it != this->end());
            // if this asserts then the memory was corrupted past the end of the block
            HPPA_ASSERT_PRINT_STACK(it->check_guard(), it, "overflow");
        }
    }

    template<bool DebugAllocatorEnable>
    void HphaSchemaBase<DebugAllocatorEnable>::HpAllocator::debug_add(void* ptr, size_t size, debug_source source, memory_debugging_flags flags)
    {
        if constexpr (DebugAllocatorEnable)
        {
            if (ptr)
            {
                {
#ifdef MULTITHREADED
                    AZStd::lock_guard<AZStd::mutex> lock(m_debugData.m_debugMutex);
#endif
                    m_debugData.m_debugMap.add(ptr, size, source, flags);
                    m_debugData.m_totalDebugRequestedSize[source] += size + MEMORY_GUARD_SIZE;
                }
                HPPA_ASSERT(size <= this->size(ptr));
            }
        }
    }

    template<bool DebugAllocatorEnable>
    void HphaSchemaBase<DebugAllocatorEnable>::HpAllocator::debug_remove(void* ptr, size_t size, debug_source source, memory_debugging_flags flags)
    {
        if constexpr (DebugAllocatorEnable)
        {
#ifdef MULTITHREADED
            AZStd::lock_guard<AZStd::mutex> lock(m_debugData.m_debugMutex);
#endif
            const debug_info info = m_debugData.m_debugMap.remove(ptr, size, source, flags);
            m_debugData.m_totalDebugRequestedSize[source] -= info.size + MEMORY_GUARD_SIZE;
        }
    }

    template<bool DebugAllocatorEnable>
    void HphaSchemaBase<DebugAllocatorEnable>::HpAllocator::debug_check(void* ptr) const
    {
        if constexpr (DebugAllocatorEnable)
        {
#ifdef MULTITHREADED
            AZStd::lock_guard<AZStd::mutex> lock(m_debugData.m_debugMutex);
#endif
            m_debugData.m_debugMap.check(ptr);
        }
    }

    template<bool DebugAllocatorEnable>
    void HphaSchemaBase<DebugAllocatorEnable>::HpAllocator::check()
    {
        if constexpr (DebugAllocatorEnable)
        {
#ifdef MULTITHREADED
            AZStd::lock_guard<AZStd::mutex> lock(m_debugData.m_debugMutex);
#endif
            for (auto it = m_debugData.m_debugMap.begin(); it != m_debugData.m_debugMap.end(); ++it)
            {
                HPPA_ASSERT(it->size() <= size(it));
                HPPA_ASSERT(it->check_guard(), "leaky overflow");
            }
        }
    }

    template<bool DebugAllocatorEnable>
    void HphaSchemaBase<DebugAllocatorEnable>::HpAllocator::report()
    {
        if constexpr (DebugAllocatorEnable)
        {
#ifdef MULTITHREADED
        AZStd::lock_guard<AZStd::mutex> lock(m_debugData.m_debugMutex);
#endif
        AZ_TracePrintf("HPHA", "REPORT =================================================\n");
        AZ_TracePrintf(
            "HPHA", "Total requested size=%zi bytes\n",
            m_debugData.m_totalDebugRequestedSize[DEBUG_SOURCE_BUCKETS] + m_debugData.m_totalDebugRequestedSize[DEBUG_SOURCE_TREE]);
        AZ_TracePrintf(
            "HPHA", "Total allocated size=%zi bytes\n",
            m_debugData.m_totalDebugRequestedSize[DEBUG_SOURCE_BUCKETS] + m_debugData.m_totalDebugRequestedSize[DEBUG_SOURCE_TREE]);
        AZ_TracePrintf("HPHA", "Currently allocated blocks:\n");
        for (auto it = m_debugData.m_debugMap.begin(); it != m_debugData.m_debugMap.end(); ++it)
        {
            AZ_TracePrintf("HPHA", "ptr=%zX, size=%zi\n", (size_t)it->ptr(), it->size());
            it->print_stack();
        }
        AZ_TracePrintf("HPHA", "===========================================================\n");
        }
    }


    //=========================================================================
    // HphaScema
    // [2/22/2011]
    //=========================================================================
    template<bool DebugAllocator>
    HphaSchemaBase<DebugAllocator>::HphaSchemaBase()
    {
        static_assert(sizeof(HpAllocator) <= sizeof(m_hpAllocatorBuffer), "Increase the m_hpAllocatorBuffer, it needs to be at least the sizeof(HpAllocator)");
        m_allocator = new (&m_hpAllocatorBuffer) HpAllocator();
    }

    //=========================================================================
    // ~HphaSchema
    // [2/22/2011]
    //=========================================================================
    template<bool DebugAllocator>
    HphaSchemaBase<DebugAllocator>::~HphaSchemaBase()
    {
        m_allocator->~HpAllocator();
    }

    //=========================================================================
    // Allocate
    // [2/22/2011]
    //=========================================================================
    template<bool DebugAllocator>
    AllocateAddress HphaSchemaBase<DebugAllocator>::allocate(size_type byteSize, size_type alignment)
    {
        return m_allocator->allocate(byteSize, alignment);
    }

    //=========================================================================
    // pointer
    // [2/22/2011]
    //=========================================================================
    template<bool DebugAllocator>
    AllocateAddress HphaSchemaBase<DebugAllocator>::reallocate(pointer ptr, size_type newSize, size_type newAlignment)
    {
        return m_allocator->reallocate(ptr, newSize, newAlignment);
    }

    //=========================================================================
    // DeAllocate(pointer ptr,size_type size,size_type alignment)
    // [2/22/2011]
    //=========================================================================
    template<bool DebugAllocator>
    auto HphaSchemaBase<DebugAllocator>::deallocate(pointer ptr, size_type size, size_type alignment) -> size_type
    {
        return m_allocator->deallocate(ptr, size, alignment);
    }

    //=========================================================================
    // AllocationSize
    // [2/22/2011]
    //=========================================================================
    template<bool DebugAllocator>
    auto HphaSchemaBase<DebugAllocator>::get_allocated_size(pointer ptr, align_type alignment) const -> size_type
    {
        return m_allocator->get_allocated_size(ptr, alignment);
    }

    //=========================================================================
    // NumAllocatedBytes
    // [2/22/2011]
    //=========================================================================
    template<bool DebugAllocator>
    auto HphaSchemaBase<DebugAllocator>::NumAllocatedBytes() const -> size_type
    {
        return m_allocator->allocated();
    }

    //=========================================================================
    // GarbageCollect
    // [2/22/2011]
    //=========================================================================
    template<bool DebugAllocator>
    void HphaSchemaBase<DebugAllocator>::GarbageCollect()
    {
        m_allocator->purge();
    }

    template<bool DebugAllocator>
    size_t HphaSchemaBase<DebugAllocator>::GetMemoryGuardSize()
    {
        return HphaSchemaBase<DebugAllocator>::HpAllocator::MEMORY_GUARD_SIZE;
    }

    template<bool DebugAllocator>
    size_t HphaSchemaBase<DebugAllocator>::GetFreeLinkSize()
    {
        return sizeof(typename HphaSchemaBase<DebugAllocator>::HpAllocator::free_link);
    }

    // explicitly instantiate both the non-debug and debug schema classes
    template class HphaSchemaBase<false>;
    template class HphaSchemaBase<true>;
} // namespace AZ
