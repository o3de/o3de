/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/PlatformIncl.h>
#include <AzCore/Memory/HphaSchema.h>

#include <AzCore/Math/Random.h>
#include <AzCore/Memory/OSAllocator.h> // required by certain platforms
#include <AzCore/std/parallel/mutex.h>
#include <AzCore/std/parallel/lock.h>
#include <AzCore/std/containers/intrusive_set.h>

#ifdef _DEBUG
//#define DEBUG_ALLOCATOR
//#define DEBUG_PTR_IN_BUCKET_CHECK // enabled this when NOT sure if PTR in bucket marker check is successfully
#endif

#ifdef DEBUG_ALLOCATOR
#include <AzCore/Debug/StackTracer.h>
#include <AzCore/std/containers/set.h>
#endif

// Enable if AZ_Assert is making things worse (since AZ_Assert may end up doing allocations)
//#include <assert.h>
//#define _HPHA_ASSERT1(_exp)         assert(_exp)
//#define _HPHA_ASSERT2(_exp, reason) assert(_exp)
#define _HPHA_ASSERT1(_exp)          AZ_Assert(_exp, "HPHA Assert, expression: \"" #_exp "\"")
#define _HPHA_ASSERT2(_exp, _reason) AZ_Assert(_exp, "HPHA Assert, expression: \"" #_exp "\", possible reason: " #_reason)
#define _GET_MACRO12(_1, _2, NAME, ...) NAME
#define _EXPAND( x ) x
#define HPPA_ASSERT(...) _EXPAND(_GET_MACRO12(__VA_ARGS__, _HPHA_ASSERT2, _HPHA_ASSERT1)(__VA_ARGS__))

#define _HPPA_ASSERT_PRINT_STACK2(_cond, _it) { if (!_cond) _it->print_stack(); _HPHA_ASSERT1(_cond); }
#define _HPPA_ASSERT_PRINT_STACK3(_cond, _it, _reason) { if (!_cond) _it->print_stack(); _HPHA_ASSERT2(_cond, _reason); }
#define _GET_MACRO23(_1, _2, _3, NAME, ...) NAME
#define HPPA_ASSERT_PRINT_STACK(...) _EXPAND(_GET_MACRO23(__VA_ARGS__, _HPPA_ASSERT_PRINT_STACK3, _HPPA_ASSERT_PRINT_STACK2)(__VA_ARGS__))  


namespace AZ {

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
    // TODO: Replace with AZStd::intrusive_list
    class intrusive_list_base
    {
    public:
        class node_base
        {
            node_base* mPrev;
            node_base* mNext;
        public:
            node_base* next() const {return mNext; }
            node_base* prev() const {return mPrev; }
            void reset()
            {
                mPrev = this;
                mNext = this;
            }
            void unlink()
            {
                mNext->mPrev = mPrev;
                mPrev->mNext = mNext;
            }
            void link(node_base* node)
            {
                mPrev = node->mPrev;
                mNext = node;
                node->mPrev = this;
                mPrev->mNext = this;
            }
        };
        intrusive_list_base()
        {
            mHead.reset();
        }
        intrusive_list_base(const intrusive_list_base&)
        {
            mHead.reset();
        }
        bool empty() const {return mHead.next() == &mHead; }
        void swap(intrusive_list_base& other)
        {
            node_base* node = &other.mHead;
            if (!empty())
            {
                node = mHead.next();
                mHead.unlink();
                mHead.reset();
            }
            node_base* other_node = &mHead;
            if (!other.empty())
            {
                other_node = other.mHead.next();
                other.mHead.unlink();
                other.mHead.reset();
            }
            mHead.link(other_node);
            other.mHead.link(node);
        }
    protected:
        node_base mHead;
    };

    //////////////////////////////////////////////////////////////////////////
    // TODO: Replace with AZStd::intrusive_list
    template<class T>
    class intrusive_list
        : public intrusive_list_base
    {
        intrusive_list(const intrusive_list& rhs);
        intrusive_list& operator=(const intrusive_list& rhs);
    public:
        class node
            : public node_base
        {
        public:
            T* next() const {return static_cast<T*>(node_base::next()); }
            T* prev() const {return static_cast<T*>(node_base::prev()); }
            const T& data() const {return *static_cast<const T*>(this); }
            T& data() {return *static_cast<T*>(this); }
        };

        class const_iterator;
        class iterator
        {
            typedef T& reference;
            typedef T* pointer;
            friend class const_iterator;
            T* mPtr;
        public:
            iterator()
                : mPtr(0) {}
            explicit iterator(T* ptr)
                : mPtr(ptr) {}
            reference operator*() const {return mPtr->data(); }
            pointer operator->() const {return &mPtr->data(); }
            operator pointer() const {
                return &mPtr->data();
            }
            iterator& operator++()
            {
                mPtr = mPtr->next();
                return *this;
            }
            iterator& operator--()
            {
                mPtr = mPtr->prev();
                return *this;
            }
            bool operator==(const iterator& rhs) const {return mPtr == rhs.mPtr; }
            bool operator!=(const iterator& rhs) const {return mPtr != rhs.mPtr; }
            T* ptr() const {return mPtr; }
        };

        class const_iterator
        {
            typedef const T& reference;
            typedef const T* pointer;
            const T* mPtr;
        public:
            const_iterator()
                : mPtr(0) {}
            explicit const_iterator(const T* ptr)
                : mPtr(ptr) {}
            const_iterator(const iterator& it)
                : mPtr(it.mPtr) {}
            reference operator*() const {return mPtr->data(); }
            pointer operator->() const {return &mPtr->data(); }
            operator pointer() const {
                return &mPtr->data();
            }
            const_iterator& operator++()
            {
                mPtr = mPtr->next();
                return *this;
            }
            const_iterator& operator--()
            {
                mPtr = mPtr->prev();
                return *this;
            }
            bool operator==(const const_iterator& rhs) const {return mPtr == rhs.mPtr; }
            bool operator!=(const const_iterator& rhs) const {return mPtr != rhs.mPtr; }
            const T* ptr() const {return mPtr; }
        };

        intrusive_list()
            : intrusive_list_base() {}
        ~intrusive_list() {clear(); }

        const_iterator begin() const {return const_iterator((const T*)mHead.next()); }
        iterator begin() {return iterator((T*)mHead.next()); }
        const_iterator end() const {return const_iterator((const T*)&mHead); }
        iterator end() {return iterator((T*)&mHead); }

        const T& front() const
        {
            HPPA_ASSERT(!empty());
            return *begin();
        }
        T& front()
        {
            HPPA_ASSERT(!empty());
            return *begin();
        }
        const T& back() const
        {
            HPPA_ASSERT(!empty());
            return *(--end());
        }
        T& back()
        {
            HPPA_ASSERT(!empty());
            return *(--end());
        }

        void push_front(T* v) {insert(this->begin(), v); }
        void pop_front() {erase(this->begin()); }
        void push_back(T* v) {insert(this->end(), v); }
        void pop_back() {erase(--(this->end())); }

        iterator insert(iterator where, T* node)
        {
            T* newLink = node;
            newLink->link(where.ptr());
            return iterator(newLink);
        }
        iterator erase(iterator where)
        {
            T* node = where.ptr();
            ++where;
            node->unlink();
            return where;
        }
        void erase(T* node)
        {
            node->unlink();
        }
        void clear()
        {
            while (!this->empty())
            {
                this->pop_back();
            }
        }
    };

    //////////////////////////////////////////////////////////////////////////
    class HpAllocator
    {
    public:
        // the guard size controls how many extra bytes are stored after
        // every allocation payload to detect memory stomps
#ifdef DEBUG_ALLOCATOR
        static const size_t MEMORY_GUARD_SIZE  = 16UL;
#else
        static const size_t MEMORY_GUARD_SIZE  = 0UL;
#endif

        // minimum allocation size, must be a power of two
        // and it needs to be able to fit a pointer
        static const size_t MIN_ALLOCATION_LOG2 = 3UL;
        static const size_t MIN_ALLOCATION  = 1UL << MIN_ALLOCATION_LOG2;

        // the maximum small allocation, anything larger goes to the tree HpAllocator
        // must be a power of two
        static const size_t MAX_SMALL_ALLOCATION_LOG2 = 9UL;
        static const size_t MAX_SMALL_ALLOCATION  = 1UL << MAX_SMALL_ALLOCATION_LOG2;

        // default alignment, must be a power of two
        static const size_t DEFAULT_ALIGNMENT  = sizeof(double);

        static const size_t NUM_BUCKETS  = (MAX_SMALL_ALLOCATION / MIN_ALLOCATION);

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
            typedef block_header* block_ptr;
            size_t size() const { return mSizeAndFlags & ~BL_FLAG_MASK; }
            block_ptr next() const {return (block_ptr)((char*)mem() + size()); }
            block_ptr prev() const {return mPrev; }
            void* mem() const {return (void*)((char*)this + sizeof(block_header)); }
            bool used() const {return (mSizeAndFlags & BL_USED) != 0; }
            void set_used() {mSizeAndFlags |= BL_USED; }
            void set_unused() {mSizeAndFlags &= ~BL_USED; }
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
            : public block_header_proxy     /* must be first */
            , public intrusive_list<page>::node
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

            inline void setInvalid()                { mMarker = 0; }

            free_link*      mFreeList = nullptr;
            size_t          mMarker = 0;
            unsigned short  mBucketIndex = 0;
            unsigned short  mUseCount = 0;

            size_t elem_size() const                { return bucket_spacing_function_inverse(mBucketIndex); }
            unsigned bucket_index() const           { return mBucketIndex; }
            size_t count() const                    { return mUseCount; }
            bool empty() const                      { return mUseCount == 0; }
            void inc_ref()                          { mUseCount++; }
            void dec_ref()                          { HPPA_ASSERT(mUseCount > 0); mUseCount--; }
            bool check_marker(size_t marker) const  { return mMarker == (marker ^ ((size_t)this)); }
        };
        typedef intrusive_list<page> page_list;
        class bucket
        {
            page_list mPageList;
#ifdef MULTITHREADED 
    #if defined (USE_MUTEX_PER_BUCKET)
            mutable AZStd::mutex mLock;
    #endif
#endif
            size_t          mMarker;
#ifdef MULTITHREADED
    #if defined (USE_MUTEX_PER_BUCKET)
            unsigned char _padding[sizeof(void*) * 16 - sizeof(page_list) - sizeof(AZStd::mutex) - sizeof(size_t)];
    #else
            unsigned char _padding[sizeof(void*) * 16 - sizeof(page_list) - sizeof(size_t)];
    #endif
#else
            unsigned char _padding[sizeof(void*) * 4 - sizeof(page_list) - sizeof(size_t)];
#endif
        public:
            bucket();
#ifdef MULTITHREADED
    #if defined (USE_MUTEX_PER_BUCKET)
            AZStd::mutex& get_lock() const {return mLock; }
    #endif
#endif
            size_t marker() const {return mMarker; }
            const page* page_list_begin() const {return mPageList.begin(); }
            page* page_list_begin() {return mPageList.begin(); }
            const page* page_list_end() const {return mPageList.end(); }
            page* page_list_end() {return mPageList.end(); }
            bool page_list_empty() const {return mPageList.empty(); }
            void add_free_page(page* p) {mPageList.push_front(p); }
            page* get_free_page();
            const page* get_free_page() const;
            void* alloc(page* p);
            void free(page* p, void* ptr);
        };
        void* bucket_system_alloc();
        void bucket_system_free(void* ptr);
        page* bucket_grow(size_t elemSize, size_t marker);
        void* bucket_alloc(size_t size);
        void* bucket_alloc_direct(unsigned bi);
        void* bucket_realloc(void* ptr, size_t size);
        void* bucket_realloc_aligned(void* ptr, size_t size, size_t alignment);
        void bucket_free(void* ptr);
        void bucket_free_direct(void* ptr, unsigned bi);
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
            if (m_isPoolAllocations)
            {
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
            }
            return result;
        }

        // free node is what the large HpAllocator uses to find free space
        // it's stored next to the block header when a block is not in use
        struct free_node
            : public AZStd::intrusive_multiset_node<free_node>
        {
            block_header* get_block() const { return (block_header*)((char*)this - sizeof(block_header)); }
            operator size_t() const {
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

        using free_node_tree = AZStd::intrusive_multiset<free_node, AZStd::intrusive_multiset_base_hook<free_node>, AZStd::less<> >;

        static inline block_header* ptr_get_block_header(void* ptr)
        {
            return (block_header*)((char*)ptr - sizeof(block_header));
        }

        // helper functions to manipulate block headers
        void split_block(block_header* bl, size_t size);
        block_header* shift_block(block_header* bl, size_t offs);
        block_header* coalesce_block(block_header* bl);

        void* tree_system_alloc(size_t size);
        void tree_system_free(void* ptr, size_t size);
        block_header* tree_extract(size_t size);
        block_header* tree_extract_aligned(size_t size, size_t alignment);
        block_header* tree_extract_bucket_page();
        block_header* tree_add_block(void* mem, size_t size);
        block_header* tree_grow(size_t size);
        void tree_attach(block_header* bl);
        void tree_detach(block_header* bl);
        void tree_purge_block(block_header* bl);

        void* tree_alloc(size_t size);
        void* tree_alloc_aligned(size_t size, size_t alignment);
        void* tree_alloc_bucket_page();
        void* tree_realloc(void* ptr, size_t size);
        void* tree_realloc_aligned(void* ptr, size_t size, size_t alignment);
        size_t tree_resize(void* ptr, size_t size);
        void tree_free(void* ptr);
        void tree_free_bucket_page(void* ptr); // only allocations with tree_alloc_bucket_page should be passed here, we don't have any guards.
        size_t tree_ptr_size(void* ptr) const;
        size_t tree_get_max_allocation() const;
        size_t tree_get_unused_memory(bool isPrint) const;
        void tree_purge();

        bucket mBuckets[NUM_BUCKETS];
        free_node_tree mFreeTree;
#ifdef MULTITHREADED
        // TODO rbbaklov: switched to recursive_mutex from mutex for Linux support.
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

#ifdef DEBUG_ALLOCATOR
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
            {}
            
            debug_record(void* ptr) // used for searching based on the pointer
                : mPtr(ptr)
                , mSize(0)
                , mSource(DEBUG_SOURCE_INVALID)
                , mGuardByte(0)
                , mCallStack()
            {}

            debug_record(void* ptr, size_t size, debug_source source)
                : mPtr(ptr)
                , mSize(size)
                , mSource(source)
                , mCallStack()
            {}

            void* ptr() const {return mPtr; }
            void set_ptr(void* ptr) {mPtr = ptr; }

            size_t size() const {return mSize; }
            void set_size(size_t size) { mSize = size; }

            debug_source source() const { return mSource; }

            const void print_stack() const;
            void record_stack();

            void write_guard();
            bool check_guard() const;

            // required for the multiset comparator
            bool operator<(const debug_record& other) const { return mPtr < other.mPtr; }

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
                , source(_source) {}
        };

        class DebugAllocator
        {
        public:
            typedef void* pointer_type;
            typedef AZStd::size_t       size_type;
            typedef AZStd::ptrdiff_t    difference_type;
            typedef AZStd::false_type   allow_memory_leaks;         ///< Regular allocators should not leak.

            AZ_FORCE_INLINE pointer_type allocate(size_t byteSize, size_t alignment, int = 0)
            {
                return AZ_OS_MALLOC(byteSize, alignment);
            }
            AZ_FORCE_INLINE size_type resize(pointer_type, size_type)
            {
                return 0;
            }
            AZ_FORCE_INLINE void deallocate(pointer_type ptr, size_type, size_type)
            {
                AZ_OS_FREE(ptr);
            }
        };

        // record map that keeps all debug records in a set, sorted by memory address of the allocation
        class debug_record_map
            : public AZStd::set<debug_record, AZStd::less<debug_record>, DebugAllocator >
        {
            typedef AZStd::set<debug_record, AZStd::less<debug_record>, DebugAllocator > base;

            static void memory_fill(void* ptr, size_t size);

        public:
            
            debug_record_map()
            {
            }
            ~debug_record_map()
            {
            }

            typedef base::const_iterator const_iterator;
            typedef base::iterator iterator;
            void add(void* ptr, size_t size, debug_source source, memory_debugging_flags flags);
            debug_info remove(void* ptr, size_t size, debug_source source, memory_debugging_flags flags);
  
            void check(void* ptr) const;
        };
        debug_record_map mDebugMap;
#ifdef MULTITHREADED
        mutable AZStd::mutex mDebugMutex;
#endif

        size_t mTotalDebugRequestedSize[DEBUG_SOURCE_SIZE];

        void debug_add(void* ptr, size_t size, debug_source source, memory_debugging_flags flags = DEBUG_FLAGS_ALL);
        void debug_remove(void* ptr, size_t size, debug_source source, memory_debugging_flags flags = DEBUG_FLAGS_ALL);

        void debug_check(void*) const;

#else // !DEBUG_ALLOCATOR

        void debug_add(void*, size_t, debug_source, memory_debugging_flags = DEBUG_FLAGS_ALL) {}
        void debug_remove(void*, size_t, debug_source, memory_debugging_flags = DEBUG_FLAGS_ALL) {}

        void debug_check(void*) const {}

#endif // DEBUG_ALLOCATOR

        size_t mTotalAllocatedSizeBuckets = 0;
        size_t mTotalCapacitySizeBuckets = 0;
        size_t mTotalAllocatedSizeTree = 0;
        size_t mTotalCapacitySizeTree = 0;
    public:
        HpAllocator(AZ::HphaSchema::Descriptor desc);
        ~HpAllocator();

        // allocate memory using DEFAULT_ALIGNMENT
        // size == 0 returns NULL
        inline void* alloc(size_t size)
        {
            if (size == 0)
            {
                return nullptr;
            }
            if (m_isPoolAllocations && is_small_allocation(size))
            {
                size = clamp_small_allocation(size);
                void* ptr = bucket_alloc_direct(bucket_spacing_function(size + MEMORY_GUARD_SIZE));
                debug_add(ptr, size, DEBUG_SOURCE_BUCKETS);
                return ptr;
            }
            else
            {
                void* ptr = tree_alloc(size + MEMORY_GUARD_SIZE);
                debug_add(ptr, size, DEBUG_SOURCE_TREE);
                return ptr;
            }
        }

        // allocate memory with a specific alignment
        // size == 0 returns NULL
        // alignment <= DEFAULT_ALIGNMENT acts as alloc
        inline void* alloc(size_t size, size_t alignment)
        {
            HPPA_ASSERT((alignment & (alignment - 1)) == 0);
            if (alignment <= DEFAULT_ALIGNMENT)
            {
                return alloc(size);
            }
            if (size == 0)
            {
                return nullptr;
            }
            if (m_isPoolAllocations && is_small_allocation(size) && alignment <= MAX_SMALL_ALLOCATION)
            {
                size = clamp_small_allocation(size);
                void* ptr = bucket_alloc_direct(bucket_spacing_function(AZ::SizeAlignUp(size + MEMORY_GUARD_SIZE, alignment)));
                debug_add(ptr, size, DEBUG_SOURCE_BUCKETS);
                return ptr;
            }
            else
            {
                void* ptr = tree_alloc_aligned(size + MEMORY_GUARD_SIZE, alignment);
                debug_add(ptr, size, DEBUG_SOURCE_TREE);
                return ptr;
            }
        }

        // realloc the memory block using DEFAULT_ALIGNMENT
        // ptr == NULL acts as alloc
        // size == 0 acts as free
        void* realloc(void* ptr, size_t size)
        {
            if (ptr == nullptr)
            {
                return alloc(size);
            }
            if (size == 0)
            {
                free(ptr);
                return nullptr;
            }
            debug_check(ptr);
            void* newPtr = nullptr;
            if (ptr_in_bucket(ptr))
            {
                if (is_small_allocation(size)) // no point to check m_isPoolAllocations as if it's false pointer can't be in a bucket.
                {
                    size = clamp_small_allocation(size);
                    debug_remove(ptr, DEBUG_UNKNOWN_SIZE, DEBUG_SOURCE_BUCKETS, DEBUG_FLAGS_GUARD_CHECK); // do not fill since the memory is reused
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
                if (m_isPoolAllocations && is_small_allocation(size))
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
                    debug_remove(ptr, DEBUG_UNKNOWN_SIZE, DEBUG_SOURCE_TREE, DEBUG_FLAGS_GUARD_CHECK); // do not fill since the memory is reused
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
        void* realloc(void* ptr, size_t size, size_t alignment)
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
                return nullptr;
            }
            if ((size_t)ptr & (alignment - 1))
            {
                void* newPtr = alloc(size, alignment);
                if (!newPtr)
                {
                    return nullptr;
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
            void* newPtr = nullptr;
            if (ptr_in_bucket(ptr))
            {
                if (is_small_allocation(size) && alignment <= MAX_SMALL_ALLOCATION) // no point to check m_isPoolAllocations as if it was false, pointer can't be in a bucket
                {
                    size = clamp_small_allocation(size);
                    debug_remove(ptr, DEBUG_UNKNOWN_SIZE, DEBUG_SOURCE_BUCKETS, DEBUG_FLAGS_GUARD_CHECK); // do not fill since the memory is reused
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
                if (m_isPoolAllocations && is_small_allocation(size) && alignment <= MAX_SMALL_ALLOCATION)
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
                    debug_remove(ptr, DEBUG_UNKNOWN_SIZE, DEBUG_SOURCE_TREE, DEBUG_FLAGS_GUARD_CHECK); // do not fill since the memory is reused
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
                debug_remove(ptr, DEBUG_UNKNOWN_SIZE, DEBUG_SOURCE_BUCKETS, DEBUG_FLAGS_GUARD_CHECK); // do not fill since the memory is reused
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

#ifdef DEBUG_ALLOCATOR
        // Similar method as above, except this one receives an iterator to the debug record. It avoids a lookup and will not lock
        inline size_t size(debug_record_map::const_iterator it) const
        {
            if (it == mDebugMap.end())
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
#endif

        // free the memory block
        inline void free(void* ptr)
        {
            if (ptr == nullptr)
            {
                return;
            }
            if (ptr_in_bucket(ptr))
            {
                debug_remove(ptr, DEBUG_UNKNOWN_SIZE, DEBUG_SOURCE_BUCKETS);
                return bucket_free(ptr);
            }
            debug_remove(ptr, DEBUG_UNKNOWN_SIZE, DEBUG_SOURCE_TREE);
            tree_free(ptr);
        }

        // free the memory block supplying the original size with DEFAULT_ALIGNMENT
        inline void free(void* ptr, size_t origSize)
        {
            if (ptr == nullptr)
            {
                return;
            }
            if (m_isPoolAllocations && is_small_allocation(origSize))
            {
                // if this asserts probably the original alloc used alignment
                HPPA_ASSERT(ptr_in_bucket(ptr));
                debug_remove(ptr, origSize, DEBUG_SOURCE_BUCKETS);
                return bucket_free_direct(ptr, bucket_spacing_function(origSize + MEMORY_GUARD_SIZE));
            }
            debug_remove(ptr, origSize, DEBUG_SOURCE_TREE);
            tree_free(ptr);
        }

        // free the memory block supplying the original size and alignment
        inline void free(void* ptr, size_t origSize, size_t oldAlignment)
        {
            if (ptr == nullptr)
            {
                return;
            }
            if (m_isPoolAllocations && is_small_allocation(origSize) && oldAlignment <= MAX_SMALL_ALLOCATION)
            {
                HPPA_ASSERT(ptr_in_bucket(ptr), "small object ptr not in a bucket");
                debug_remove(ptr, origSize, DEBUG_SOURCE_BUCKETS);
                return bucket_free_direct(ptr, bucket_spacing_function(AZ::SizeAlignUp(origSize + MEMORY_GUARD_SIZE, oldAlignment)));
            }
            debug_remove(ptr, origSize, DEBUG_SOURCE_TREE);
            tree_free(ptr);
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

#ifdef DEBUG_ALLOCATOR
        // print HpAllocator statistics
        void report();
        
        // check memory integrity
        void check();
#endif

        // return the total number of allocated memory
        inline  size_t allocated() const
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

        void*        m_fixedBlock;
        size_t       m_fixedBlockSize;
        const size_t m_treePageSize;
        const size_t m_treePageAlignment;
        const size_t m_poolPageSize;
        bool         m_isPoolAllocations;
        IAllocatorAllocate* m_subAllocator;

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
            AZ_TracePrintf("HphaAllocator", "%p(%s|%s|%p|%p|%p)\n", node, node->red() ? "red" : "black", node->parent_side() == LEFT ? "L" : "R", reinterpret_cast<void*>(reinterpret_cast<int>(node->parent()) & ~3), node->left()->nil() ? nullptr : node->left(), node->right()->nil() ? nullptr : node->right());
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
    HpAllocator::HpAllocator(AZ::HphaSchema::Descriptor desc)
        // We will use the os for direct allocations if memoryBlock == NULL
        // If m_systemChunkSize is specified, use that size for allocating tree blocks from the OS
        // m_treePageAlignment should be OS_VIRTUAL_PAGE_SIZE in all cases with this trait as we work 
        // with virtual memory addresses when the tree grows and we cannot specify an alignment in all cases
        : m_treePageSize(desc.m_fixedMemoryBlock != nullptr ? desc.m_pageSize :
            desc.m_systemChunkSize != 0 ? desc.m_systemChunkSize : OS_VIRTUAL_PAGE_SIZE)
        , m_treePageAlignment(desc.m_pageSize)
        , m_poolPageSize(desc.m_fixedMemoryBlock != nullptr ? desc.m_poolPageSize : OS_VIRTUAL_PAGE_SIZE)
        , m_subAllocator(desc.m_subAllocator)
    {
#ifdef DEBUG_ALLOCATOR
        mTotalDebugRequestedSize[DEBUG_SOURCE_BUCKETS] = 0;
        mTotalDebugRequestedSize[DEBUG_SOURCE_TREE] = 0;
#endif
        mTotalAllocatedSizeBuckets = 0;
        mTotalAllocatedSizeTree = 0;

        m_fixedBlock = desc.m_fixedMemoryBlock;
        m_fixedBlockSize = desc.m_fixedMemoryBlockByteSize;
        m_isPoolAllocations = desc.m_isPoolAllocations;
        if (desc.m_fixedMemoryBlock)
        {
            block_header* bl = tree_add_block(m_fixedBlock, m_fixedBlockSize);
            tree_attach(bl);
        }

#if AZ_TRAIT_OS_HAS_CRITICAL_SECTION_SPIN_COUNT
#   if  defined(MULTITHREADED)
        // For some platforms we can use an actual spin lock, test and profile. We don't expect much contention there
        SetCriticalSectionSpinCount((LPCRITICAL_SECTION)mTreeMutex.native_handle(), SPIN_COUNT);
#   endif // MULTITHREADED
#endif // AZ_TRAIT_OS_HAS_CRITICAL_SECTION_SPIN_COUNT

#if !defined (USE_MUTEX_PER_BUCKET)
    #if AZ_TRAIT_OS_HAS_CRITICAL_SECTION_SPIN_COUNT
    #   if  defined(MULTITHREADED)
            // For some platforms we can use an actual spin lock, test and profile. We don't expect much contention there
            SetCriticalSectionSpinCount(m_mutex.native_handle(), SPIN_COUNT);
    #   endif // MULTITHREADED
    #endif // AZ_TRAIT_OS_HAS_CRITICAL_SECTION_SPIN_COUNT
#endif
    }

    HpAllocator::~HpAllocator()
    {
#ifdef DEBUG_ALLOCATOR
        // Check if there are not-freed allocations
        report();
        check();
#endif
        
        purge();

#ifdef DEBUG_ALLOCATOR 
        // Check if all the memory was returned to the OS
        for (unsigned i = 0; i < NUM_BUCKETS; i++)
        {
            HPPA_ASSERT(mBuckets[i].page_list_empty(), "small object leak");
        }

        if (!m_fixedBlock)
        {
#ifdef AZ_ENABLE_TRACING
            if (!mFreeTree.empty())
            {
                free_node_tree::iterator node = mFreeTree.begin();
                free_node_tree::iterator end = mFreeTree.end();
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
        else
        {
            // Verify that the last block in the free tree is the m_fixedBlock
            HPPA_ASSERT(mFreeTree.size() == 1);
            HPPA_ASSERT(mFreeTree.begin()->get_block()->prev() == m_fixedBlock);
            HPPA_ASSERT((mFreeTree.begin()->get_block()->size() + sizeof(block_header) * 3) == m_fixedBlockSize);
        }
#endif
    }

    HpAllocator::bucket::bucket()
    {
#if AZ_TRAIT_OS_HAS_CRITICAL_SECTION_SPIN_COUNT
#   if  defined(MULTITHREADED)
    #if defined (USE_MUTEX_PER_BUCKET)
        // For some platforms we can use an actual spin lock, test and profile. We don't expect much contention there
        SetCriticalSectionSpinCount((LPCRITICAL_SECTION)mLock.native_handle(), SPIN_COUNT);
    #endif
#   endif // MULTITHREADED
#endif // AZ_TRAIT_OS_HAS_CRITICAL_SECTION_SPIN_COUNT

        // Initializing Sfmt causes a file read which causes an allocation which prevents us from initializing the allocators before we need malloc overridden.
        // Thats why we use SimpleLcgRandom here
        AZ::SimpleLcgRandom randGenerator = AZ::SimpleLcgRandom(reinterpret_cast<u64>(static_cast<void*>(this)));
        mMarker = size_t(randGenerator.Getu64Random());

        (void)_padding;
    }

    HpAllocator::page* HpAllocator::bucket::get_free_page()
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

    const HpAllocator::page* HpAllocator::bucket::get_free_page() const
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

    void* HpAllocator::bucket::alloc(page* p)
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
            p->unlink();
            mPageList.push_back(p);
        }
        return (void*)free;
    }

    void HpAllocator::bucket::free(page* p, void* ptr)
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
            p->unlink();
            mPageList.push_front(p);
        }
    }

    void* HpAllocator::bucket_system_alloc()
    {
        void* ptr;
        if (m_fixedBlock)
        {
            ptr = tree_alloc_bucket_page();
            // mTotalCapacitySizeBuckets memory is part of the tree allocations
        }
        else
        {
            ptr = SystemAlloc(m_poolPageSize, m_poolPageSize);
            mTotalCapacitySizeBuckets += m_poolPageSize;
        }
        HPPA_ASSERT(((size_t)ptr & (m_poolPageSize - 1)) == 0);
        return ptr;
    }

    void HpAllocator::bucket_system_free(void* ptr)
    {
        HPPA_ASSERT(ptr);
        if (m_fixedBlock)
        {
            tree_free_bucket_page(ptr);
            // mTotalAllocatedSizeBuckets memory is part of the tree allocations
        }
        else
        {
            SystemFree(ptr);
            mTotalCapacitySizeBuckets -= m_poolPageSize;
        }
    }

    HpAllocator::page* HpAllocator::bucket_grow(size_t elemSize, size_t marker)
    {
        // make sure mUseCount won't overflow
        HPPA_ASSERT((m_poolPageSize - sizeof(page)) / elemSize <= USHRT_MAX);
        if (void* mem = bucket_system_alloc())
        {
            return new (mem) page((unsigned short)elemSize, m_poolPageSize, marker);
        }
        return nullptr;
    }

    void* HpAllocator::bucket_alloc(size_t size)
    {
        HPPA_ASSERT(size <= MAX_SMALL_ALLOCATION);
        unsigned bi = bucket_spacing_function(size);
        HPPA_ASSERT(bi < NUM_BUCKETS);
#ifdef MULTITHREADED
    #if defined (USE_MUTEX_PER_BUCKET)
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
                return nullptr;
            }
            mBuckets[bi].add_free_page(p);
        }
        HPPA_ASSERT(p->elem_size() >= size);
        mTotalAllocatedSizeBuckets += p->elem_size();
        return mBuckets[bi].alloc(p);
    }

    void* HpAllocator::bucket_alloc_direct(unsigned bi)
    {
        HPPA_ASSERT(bi < NUM_BUCKETS);
#ifdef MULTITHREADED
    #if defined (USE_MUTEX_PER_BUCKET)
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
                return nullptr;
            }
            mBuckets[bi].add_free_page(p);
        }
        mTotalAllocatedSizeBuckets += p->elem_size();
        return mBuckets[bi].alloc(p);
    }

    void* HpAllocator::bucket_realloc(void* ptr, size_t size)
    {
        page* p = ptr_get_page(ptr);
        size_t elemSize = p->elem_size();
        //if we do this and we use the bucket_free with size hint we will crash, as the hit will not be the real index/bucket size
        //if (size <= elemSize)
        if (size == elemSize)
        {
            return ptr;
        }
        void* newPtr = bucket_alloc(size);
        if (!newPtr)
        {
            return nullptr;
        }
        memcpy(newPtr, ptr, AZStd::GetMin(elemSize - MEMORY_GUARD_SIZE, size - MEMORY_GUARD_SIZE));
        bucket_free(ptr);
        return newPtr;
    }

    void* HpAllocator::bucket_realloc_aligned(void* ptr, size_t size, size_t alignment)
    {
        page* p = ptr_get_page(ptr);
        size_t elemSize = p->elem_size();
        //if we do this and we use the bucket_free with size hint we will crash, as the hit will not be the real index/bucket size
        //if (size <= elemSize && (elemSize&(alignment-1))==0)
        if (size == elemSize && (elemSize & (alignment - 1)) == 0)
        {
            return ptr;
        }
        void* newPtr = bucket_alloc_direct(bucket_spacing_function(AZ::SizeAlignUp(size, alignment)));
        if (!newPtr)
        {
            return nullptr;
        }
        memcpy(newPtr, ptr, AZStd::GetMin(elemSize - MEMORY_GUARD_SIZE, size - MEMORY_GUARD_SIZE));
        bucket_free(ptr);
        return newPtr;
    }

    void HpAllocator::bucket_free(void* ptr)
    {
        page* p = ptr_get_page(ptr);
        unsigned bi = p->bucket_index();
        HPPA_ASSERT(bi < NUM_BUCKETS);
#ifdef MULTITHREADED
    #if defined (USE_MUTEX_PER_BUCKET)
        AZStd::lock_guard<AZStd::mutex> lock(mBuckets[bi].get_lock());
    #else
        AZStd::lock_guard<AZStd::mutex> lock(m_mutex);
    #endif
#endif
        mTotalAllocatedSizeBuckets -= p->elem_size();
        mBuckets[bi].free(p, ptr);
    }

    void HpAllocator::bucket_free_direct(void* ptr, unsigned bi)
    {
        HPPA_ASSERT(bi < NUM_BUCKETS);
        page* p = ptr_get_page(ptr);
        // if this asserts, the free size doesn't match the allocated size
        // most likely a class needs a base virtual destructor
        HPPA_ASSERT(bi == p->bucket_index());
#ifdef MULTITHREADED
    #if defined (USE_MUTEX_PER_BUCKET)
        AZStd::lock_guard<AZStd::mutex> lock(mBuckets[bi].get_lock());
    #else
        AZStd::lock_guard<AZStd::mutex> lock(m_mutex);
    #endif
#endif
        mTotalAllocatedSizeBuckets -= p->elem_size();
        mBuckets[bi].free(p, ptr);
    }

    size_t HpAllocator::bucket_ptr_size(void* ptr) const
    {
        page* p = ptr_get_page(ptr);
        HPPA_ASSERT(p->bucket_index() < NUM_BUCKETS);

#ifdef MULTITHREADED
    #if defined (USE_MUTEX_PER_BUCKET)
        unsigned bi = p->bucket_index();
        AZStd::lock_guard<AZStd::mutex> lock(mBuckets[bi].get_lock());
    #else
        AZStd::lock_guard<AZStd::mutex> lock(m_mutex);
    #endif
#endif
        return p->elem_size() - MEMORY_GUARD_SIZE;
    }

    size_t HpAllocator::bucket_get_max_allocation() const
    {
        for (int i = (int)NUM_BUCKETS - 1; i > 0; i--)
        {
#ifdef MULTITHREADED
    #if defined (USE_MUTEX_PER_BUCKET)
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

    size_t HpAllocator::bucket_get_unused_memory(bool isPrint) const
    {
        size_t unusedMemory = 0;
        size_t availablePageMemory = m_poolPageSize - sizeof(page);
        for (int i = (int)NUM_BUCKETS - 1; i > 0; i--)
        {
#ifdef MULTITHREADED
    #if defined (USE_MUTEX_PER_BUCKET)
            AZStd::lock_guard<AZStd::mutex> lock(mBuckets[i].get_lock());
    #else
            AZStd::lock_guard<AZStd::mutex> lock(m_mutex);
    #endif
#endif
            const page* pageEnd = mBuckets[i].page_list_end();
            for (const page* p = mBuckets[i].page_list_begin(); p != pageEnd; )
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
                    AZ_TracePrintf("System", "Unused Bucket %d page %p elementSize: %d available: %d elements\n", i, p, elementSize, availableMemory / elementSize);
                }
                p = p->next();
            }
        }
        return unusedMemory;
    }

    void HpAllocator::bucket_purge()
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
            page* pageEnd = mBuckets[i].page_list_end();
            for (page* p = mBuckets[i].page_list_begin(); p != pageEnd; )
            {
                // early out if we reach fully occupied page (the remaining should all be full)
                if (p->mFreeList == nullptr)
                {
                    break;
                }
                page* next = p->next();
                if (p->empty())
                {
                    HPPA_ASSERT(p->mFreeList);
                    p->unlink();
                    p->setInvalid();
                    bucket_system_free(p);
                }
                p = next;
            }
        }
    }

    void HpAllocator::split_block(block_header* bl, size_t size)
    {
        HPPA_ASSERT(size + sizeof(block_header) + sizeof(free_node) <= bl->size());
        block_header* newBl = (block_header*)((char*)bl + size + sizeof(block_header));
        newBl->link_after(bl);
        newBl->set_unused();
    }

    HpAllocator::block_header* HpAllocator::shift_block(block_header* bl, size_t offs)
    {
        HPPA_ASSERT(offs > 0);
        block_header* prev = bl->prev();
        bl->unlink();
        bl = (block_header*)((char*)bl + offs);
        bl->link_after(prev);
        bl->set_unused();
        return bl;
    }

    HpAllocator::block_header* HpAllocator::coalesce_block(block_header* bl)
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

    void* HpAllocator::tree_system_alloc(size_t size)
    {
        if (m_fixedBlock)
        {
            return nullptr; // we ran out of memory in our fixed block
        }
        size_t allocSize = AZ::SizeAlignUp(size, OS_VIRTUAL_PAGE_SIZE);
        mTotalCapacitySizeTree += allocSize;
        return SystemAlloc(size, m_treePageAlignment);
    }

    void HpAllocator::tree_system_free(void* ptr, size_t size)
    {
        HPPA_ASSERT(ptr);
        (void)size;

        if (m_fixedBlock)
        {
            return; // no need to free the fixed block
        }
        size_t allocSize = AZ::SizeAlignUp(size, OS_VIRTUAL_PAGE_SIZE);
        mTotalCapacitySizeTree -= allocSize;
        SystemFree(ptr);
    }

    HpAllocator::block_header* HpAllocator::tree_add_block(void* mem, size_t size)
    {
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

    HpAllocator::block_header* HpAllocator::tree_grow(size_t size)
    {
        const size_t sizeWithBlockHeaders = size + 3 * sizeof(block_header); // two fences plus one fake
        const size_t newSize = (sizeWithBlockHeaders < m_treePageSize) ? AZ::SizeAlignUp(sizeWithBlockHeaders, m_treePageSize) : sizeWithBlockHeaders;
        HPPA_ASSERT(newSize >= sizeWithBlockHeaders);

        if (void* mem = tree_system_alloc(newSize))
        {
            return tree_add_block(mem, newSize);
        }
        return nullptr;
    }

    HpAllocator::block_header* HpAllocator::tree_extract(size_t size)
    {
        // search the tree and get the smallest fitting block
        free_node_tree::iterator it = mFreeTree.lower_bound(size);
        if (it == mFreeTree.end())
        {
            return nullptr;
        }
        free_node* bestNode = it->next(); // improves removal time
        block_header* bestBlock = bestNode->get_block();
        tree_detach(bestBlock);
        return bestBlock;
    }

    HpAllocator::block_header* HpAllocator::tree_extract_aligned(size_t size, size_t alignment)
    {
        // get the sequence of nodes from size to (size + alignment - 1) including
        size_t sizeUpper = size + alignment;
        free_node_tree::iterator bestNode = mFreeTree.lower_bound(size);
        free_node_tree::iterator lastNode = mFreeTree.upper_bound(sizeUpper);
        while (bestNode != lastNode)
        {
            free_node* node = &*bestNode;
            size_t alignmentOffs = AZ::PointerAlignUp((char*)node, alignment) - (char*)node;
            if (node->get_block()->size() >= size + alignmentOffs)
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
        block_header* bestBlock = bestNode->get_block();
        tree_detach(bestBlock);
        return bestBlock;
    }

    HpAllocator::block_header* HpAllocator::tree_extract_bucket_page()
    {
        block_header* bestBlock = nullptr;
        size_t alignment = m_poolPageSize;
        size_t size = m_poolPageSize;

        // get the sequence of nodes from size to (size + alignment - 1) including
        size_t sizeUpper = size + alignment;
        free_node_tree::iterator bestNode = mFreeTree.lower_bound(size);
        free_node_tree::iterator lastNode = mFreeTree.upper_bound(sizeUpper);
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

    void HpAllocator::tree_attach(block_header* bl)
    {
        mFreeTree.insert((free_node*)bl->mem());
    }

    void HpAllocator::tree_detach(block_header* bl)
    {
        mFreeTree.erase((free_node*)bl->mem());
    }

    void* HpAllocator::tree_alloc(size_t size)
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
                return nullptr;
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
        return newBl->mem();
    }

    void* HpAllocator::tree_alloc_aligned(size_t size, size_t alignment)
    {
#ifdef MULTITHREADED
        AZStd::lock_guard<AZStd::recursive_mutex> lock(mTreeMutex);
#endif
        if (size < sizeof(free_node))
        {
            size = sizeof(free_node);
        }
        size = AZ::SizeAlignUp(size, sizeof(block_header));
        block_header* newBl = tree_extract_aligned(size, alignment);
        if (!newBl)
        {
            newBl = tree_grow(size + alignment);
            if (!newBl)
            {
                return nullptr;
            }
        }
        HPPA_ASSERT(newBl && newBl->size() >= size);
        size_t alignmentOffs = AZ::PointerAlignUp((char*)newBl->mem(), alignment) - (char*)newBl->mem();
        HPPA_ASSERT(newBl->size() >= size + alignmentOffs);
        if (alignmentOffs >= sizeof(block_header) + sizeof(free_node))
        {
            split_block(newBl, alignmentOffs - sizeof(block_header));
            tree_attach(newBl);
            newBl = newBl->next();
        }
        else if (alignmentOffs > 0)
        {
            newBl = shift_block(newBl, alignmentOffs);
        }
        if (newBl->size() >= size + sizeof(block_header) + sizeof(free_node))
        {
            split_block(newBl, size);
            tree_attach(newBl->next());
        }
        newBl->set_used();
        HPPA_ASSERT(((size_t)newBl->mem() & (alignment - 1)) == 0);
        mTotalAllocatedSizeTree += newBl->size();
        return newBl->mem();
    }

    void* HpAllocator::tree_alloc_bucket_page()
    {
#ifdef MULTITHREADED
        AZStd::lock_guard<AZStd::recursive_mutex> lock(mTreeMutex);
#endif
        // We are allocating pool pages m_poolPageSize aligned at m_poolPageSize
        // what is special is that we are keeping the block_header at the beginning of the
        // memory and we return the offset pointer.
        size_t size = m_poolPageSize;
        size_t alignment = m_poolPageSize;
        block_header* newBl = tree_extract_bucket_page();
        if (!newBl)
        {
            newBl = tree_grow(size + alignment);
            if (!newBl)
            {
                return nullptr;
            }
        }
        HPPA_ASSERT(newBl && (newBl->size() + sizeof(block_header)) >= size);
        size_t alignmentOffs = AZ::PointerAlignUp((char*)newBl, alignment) - (char*)newBl;
        HPPA_ASSERT(newBl->size() + sizeof(block_header) >= size + alignmentOffs);
        if (alignmentOffs >= sizeof(block_header) + sizeof(free_node))
        {
            split_block(newBl, alignmentOffs - sizeof(block_header));
            tree_attach(newBl);
            newBl = newBl->next();
        }
        else if (alignmentOffs > 0)
        {
            newBl = shift_block(newBl, alignmentOffs);
        }
        if (newBl->size() >= (size - sizeof(block_header)) + sizeof(block_header) + sizeof(free_node))
        {
            split_block(newBl, size - sizeof(block_header));
            tree_attach(newBl->next());
        }
        newBl->set_used();
        HPPA_ASSERT(((size_t)newBl & (alignment - 1)) == 0);
        return newBl;
    }

    void* HpAllocator::tree_realloc(void* ptr, size_t size)
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
            return ptr;
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
            return ptr;
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
            return newPtr;
        }
        // fall back to alloc/copy/free
        void* newPtr = tree_alloc(size);
        if (newPtr)
        {
            memcpy(newPtr, ptr, blSize - MEMORY_GUARD_SIZE);
            tree_free(ptr);
            return newPtr;
        }
        return nullptr;
    }

    void* HpAllocator::tree_realloc_aligned(void* ptr, size_t size, size_t alignment)
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
            return ptr;
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
            return ptr;
        }
        block_header* prev = bl->prev();
        size_t prevSize = prev->used() ? 0 : prev->size() + sizeof(block_header);
        size_t alignmentOffs = prev->used() ? 0 : AZ::PointerAlignUp((char*)prev->mem(), alignment) - (char*)prev->mem();
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
            if (alignmentOffs >= sizeof(block_header) + sizeof(free_node))
            {
                split_block(prev, alignmentOffs - sizeof(block_header));
                tree_attach(prev);
                prev = prev->next();
            }
            else if (alignmentOffs > 0)
            {
                prev = shift_block(prev, alignmentOffs);
            }
            bl = prev;
            bl->set_used();
            HPPA_ASSERT(bl->size() >= size && ((size_t)bl->mem() & (alignment - 1)) == 0);
            void* newPtr = bl->mem();
            memmove(newPtr, ptr, blSize - MEMORY_GUARD_SIZE);
            if (bl->size() >= size + sizeof(block_header) + sizeof(free_node))
            {
                split_block(bl, size);
                tree_attach(bl->next());
            }
            mTotalAllocatedSizeTree += bl->size() - blSize;
            return newPtr;
        }
        void* newPtr = tree_alloc_aligned(size, alignment);
        if (newPtr)
        {
            memcpy(newPtr, ptr, blSize - MEMORY_GUARD_SIZE);
            tree_free(ptr);
            return newPtr;
        }
        return nullptr;
    }

    size_t HpAllocator::tree_resize(void* ptr, size_t size)
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

    void HpAllocator::tree_free(void* ptr)
    {
#ifdef MULTITHREADED
        AZStd::lock_guard<AZStd::recursive_mutex> lock(mTreeMutex);
#endif
        block_header* bl = ptr_get_block_header(ptr);
        // This assert detects a double-free of ptr
        HPPA_ASSERT(bl->used());
        mTotalAllocatedSizeTree -= bl->size();
        bl->set_unused();
        bl = coalesce_block(bl);
        tree_attach(bl);
    }

    void HpAllocator::tree_free_bucket_page(void* ptr)
    {
#ifdef MULTITHREADED
        AZStd::lock_guard<AZStd::recursive_mutex> lock(mTreeMutex);
#endif
        HPPA_ASSERT(AZ::PointerAlignDown(ptr, m_poolPageSize) == ptr);
        block_header* bl = (block_header*)ptr;
        HPPA_ASSERT(bl->size() >= m_poolPageSize - sizeof(block_header));
        bl->set_unused();
        bl = coalesce_block(bl);
        tree_attach(bl);
    }

    void HpAllocator::tree_purge_block(block_header* bl)
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
        //else if( m_fixedBlock )
        //{
        //  size_t requiredFrontSize = 2*sizeof(block_header) + sizeof(free_node); // 1 fake start (prev()->prev()) and one free block
        //  size_t requiredEndSize = 2*sizeof(block_header) + sizeof(free_node); // 1 fake start (prev()->prev()) and one free block
        //  if( bl->prev()->prev() == NULL )
        //  {
        //      char* alignedEnd = AZ::PointerAlignDown((char*)bl->next()-requiredFrontSize,m_treePageSize);
        //      char* memStart = (char*)bl->prev();
        //      if( alignedEnd > memStart )
        //      {
        //          tree_detach(bl);
        //          block_header* frontFence = (block_header*)alignedEnd;
        //          frontFence->prev(0);
        //          frontFence->size(0);
        //          frontFence->set_used();
        //          block_header* frontFreeBlock = (block_header*)frontFence->mem();
        //          frontFreeBlock->prev(frontFence);
        //          frontFreeBlock->set_unused();
        //          frontFreeBlock->next(bl->next()); // set's the size
        //          bl->next()->prev(frontFreeBlock);
        //          tree_attach(frontFreeBlock);

        //          memset(memStart,0xff,sizeof(block_header));
        //          tree_system_free(memStart, alignedEnd - memStart);
        //      }
        //  }
        //  else if( bl->next()->size() == 0)
        //  {
        //      char* alignedStart = AZ::PointerAlignUp((char*)bl+requiredEndSize,m_treePageSize);
        //      char* memEnd = (char*)bl->next()+sizeof(block_header);
        //      if( alignedStart < memEnd )
        //      {
        //          tree_detach(bl);
        //          block_header* backFence = (block_header*)(alignedStart-sizeof(block_header));
        //          block_header* backFreeBlock = bl;
        //          backFreeBlock->next(backFence); // set's the size
        //
        //          backFence->prev(backFreeBlock);
        //          backFence->size(0);
        //          backFence->set_used();

        //          tree_attach(backFreeBlock);
        //
        //          memset(alignedStart,0xff,sizeof(block_header));
        //          tree_system_free(alignedStart, memEnd - alignedStart);
        //      }
        //  }
        //  else
        //  {
        //      char* alignedStart = AZ::PointerAlignUp((char*)bl+requiredEndSize,m_treePageSize);
        //      char* alignedEnd = AZ::PointerAlignDown((char*)bl->next()-requiredFrontSize,m_treePageSize);
        //      if( alignedStart < alignedEnd )
        //      {
        //          tree_detach(bl);

        //          //
        //          block_header* frontFence = (block_header*)alignedEnd;
        //          frontFence->prev(0);
        //          frontFence->size(0);
        //          frontFence->set_used();
        //          block_header* frontFreeBlock = (block_header*)frontFence->mem();
        //          frontFreeBlock->prev(frontFence);
        //          frontFreeBlock->set_unused();
        //          frontFreeBlock->next(bl->next()); // set's the size
        //          bl->next()->prev(frontFreeBlock);
        //          tree_attach(frontFreeBlock);

        //          //
        //          block_header* backFence = (block_header*)(alignedStart-sizeof(block_header));
        //          block_header* backFreeBlock = bl;
        //          backFreeBlock->next(backFence); // set's the size

        //          backFence->prev(backFreeBlock);
        //          backFence->size(0);
        //          backFence->set_used();
        //          tree_attach(backFreeBlock);

        //          memset(alignedStart,0xff,sizeof(block_header));
        //          tree_system_free(alignedStart, alignedEnd - alignedStart);
        //      }
        //  }
        //}
    }

    size_t HpAllocator::tree_ptr_size(void* ptr) const
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

    size_t HpAllocator::tree_get_max_allocation() const
    {
        return mFreeTree.maximum()->get_block()->size();
    }

    size_t HpAllocator::tree_get_unused_memory(bool isPrint) const
    {
        size_t unusedMemory = 0;
        for (free_node_tree::const_iterator it = mFreeTree.begin(); it != mFreeTree.end(); ++it)
        {
            unusedMemory += it->get_block()->size();
            if (isPrint)
            {
                AZ_TracePrintf("System", "Unused Treenode %p size: %zu\n", it->get_block()->mem(), it->get_block()->size());
            }
        }
        return unusedMemory;
    }

    void HpAllocator::tree_purge()
    {
#ifdef MULTITHREADED
        AZStd::lock_guard<AZStd::recursive_mutex> lock(mTreeMutex);
#endif
        free_node_tree::iterator node = mFreeTree.begin();
        free_node_tree::iterator end = mFreeTree.end();
        while (node != end)
        {
            block_header* cur = node->get_block();
            ++node;
            if (cur->prev() != m_fixedBlock) // check we are not purging the fixed block
            {
                tree_purge_block(cur);
            }
        }
    }

    //=========================================================================
    // allocationSize
    // [2/22/2011]
    //=========================================================================
    size_t
    HpAllocator::AllocationSize(void* ptr)
    {
        if (m_fixedBlock)
        {
            if (ptr < m_fixedBlock && ptr >= (char*)m_fixedBlock + m_fixedBlockSize)
            {
                return 0;
            }
        }
        return size(ptr);
    }

    //=========================================================================
    // GetMaxAllocationSize
    // [2/22/2011]
    //=========================================================================
    size_t
    HpAllocator::GetMaxAllocationSize() const
    {
        const_cast<HpAllocator*>(this)->purge(); // slow

        size_t maxSize = 0;
        maxSize = AZStd::GetMax(bucket_get_max_allocation(), maxSize);
        maxSize = AZStd::GetMax(tree_get_max_allocation(), maxSize);
        return maxSize;
    }

    size_t HpAllocator::GetMaxContiguousAllocationSize() const
    {
        return AZ_CORE_MAX_ALLOCATOR_SIZE;
    }

    //=========================================================================
    // GetUnAllocatedMemory
    // [9/30/2013]
    //=========================================================================
    size_t
    HpAllocator::GetUnAllocatedMemory(bool isPrint) const
    {
        return bucket_get_unused_memory(isPrint) + tree_get_unused_memory(isPrint);
    }

    //=========================================================================
    // SystemAlloc
    // [2/22/2011]
    //=========================================================================
    void*
    HpAllocator::SystemAlloc(size_t size, size_t align)
    {
        if (m_subAllocator)
        {
            return m_subAllocator->Allocate(size, align, 0, "HphaSchema sub allocation", __FILE__, __LINE__);
        }
        AZ_Assert(align % OS_VIRTUAL_PAGE_SIZE == 0, "Invalid allocation/page alignment %d should be a multiple of %d!", size, OS_VIRTUAL_PAGE_SIZE);
        return AZ_OS_MALLOC(size, align);
    }

    //=========================================================================
    // SystemFree
    // [2/22/2011]
    //=========================================================================
    void
    HpAllocator::SystemFree(void* ptr)
    {
        if (m_subAllocator)
        {
            m_subAllocator->DeAllocate(ptr);
            return;
        }
        AZ_OS_FREE(ptr);
    }

#ifdef DEBUG_ALLOCATOR

    const void HpAllocator::debug_record::print_stack() const
    {
        AZ::Debug::SymbolStorage::StackLine stackLines[MAX_CALLSTACK_DEPTH] = { { 0 } };
        AZ::Debug::SymbolStorage::DecodeFrames(mCallStack, MAX_CALLSTACK_DEPTH, stackLines);
        
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
                AZ::Debug::Trace::Output("HpAllocator", stackLines[i]);
            }
        }
    }

    void HpAllocator::debug_record::record_stack()
    {
        AZ::Debug::StackRecorder::Record(mCallStack, MAX_CALLSTACK_DEPTH, 6);
    }

    void HpAllocator::debug_record::write_guard()
    {
        unsigned char guardByte = (unsigned char)rand();
        unsigned char* guard = (unsigned char*)mPtr + mSize;
        mGuardByte = guardByte;
        for (unsigned i = 0; i < MEMORY_GUARD_SIZE; i++)
        {
            guard[i] = guardByte++;
        }
    }

    bool HpAllocator::debug_record::check_guard() const
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
        return true;
    }

    void HpAllocator::debug_record_map::memory_fill(void* ptr, size_t size)
    {
        unsigned char sFiller[] = {0xFF, 0xC0, 0xC0, 0xFF}; // QNAN (little OR big endian)
        unsigned char* p = (unsigned char*)ptr;
        for (size_t s = 0; s < size; s++)
        {
            p[s] = sFiller[s % sizeof(sFiller) / sizeof(sFiller[0])];
        }
    }

    void HpAllocator::debug_record_map::add(void* ptr, size_t size, debug_source source, memory_debugging_flags flags)
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

    HpAllocator::debug_info HpAllocator::debug_record_map::remove(void* ptr, size_t size, debug_source source, memory_debugging_flags flags)
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

    void HpAllocator::debug_record_map::check(void* ptr) const
    {
        const_iterator it = this->find(debug_record(ptr));
        // if this asserts most likely the pointer was already deleted
        HPPA_ASSERT(it != this->end());
        // if this asserts then the memory was corrupted past the end of the block
        HPPA_ASSERT_PRINT_STACK(it->check_guard(), it, "overflow");
    }

    void HpAllocator::debug_add(void* ptr, size_t size, debug_source source, memory_debugging_flags flags)
    {
        if (ptr)
        {
            {
#ifdef MULTITHREADED
                AZStd::lock_guard<AZStd::mutex> lock(mDebugMutex);
#endif
                mDebugMap.add(ptr, size, source, flags);
                mTotalDebugRequestedSize[source] += size + MEMORY_GUARD_SIZE;
            }
            HPPA_ASSERT(size <= this->size(ptr));
        }
    }

    void HpAllocator::debug_remove(void* ptr, size_t size, debug_source source, memory_debugging_flags flags)
    {
    #ifdef MULTITHREADED
        AZStd::lock_guard<AZStd::mutex> lock(mDebugMutex);
    #endif
        const debug_info info = mDebugMap.remove(ptr, size, source, flags);
        mTotalDebugRequestedSize[source] -= info.size + MEMORY_GUARD_SIZE;
    }

    void HpAllocator::debug_check(void* ptr) const
    {
#ifdef MULTITHREADED
        AZStd::lock_guard<AZStd::mutex> lock(mDebugMutex);
#endif
        mDebugMap.check(ptr);
    }

    void HpAllocator::check()
    {
#ifdef MULTITHREADED
        AZStd::lock_guard<AZStd::mutex> lock(mDebugMutex);
#endif
        for (debug_record_map::const_iterator it = mDebugMap.begin(); it != mDebugMap.end(); ++it)
        {
            HPPA_ASSERT(it->size() <= size(it));
            HPPA_ASSERT(it->check_guard(), "leaky overflow");
        }
    }

    void HpAllocator::report()
    {
    #ifdef MULTITHREADED
        AZStd::lock_guard<AZStd::mutex> lock(mDebugMutex);
    #endif
        AZ_TracePrintf("HPHA", "REPORT =================================================\n");
        AZ_TracePrintf("HPHA", "Total requested size=%zi bytes\n", mTotalDebugRequestedSize[DEBUG_SOURCE_BUCKETS] + mTotalDebugRequestedSize[DEBUG_SOURCE_TREE]);
        AZ_TracePrintf("HPHA", "Total allocated size=%zi bytes\n", mTotalDebugRequestedSize[DEBUG_SOURCE_BUCKETS] + mTotalDebugRequestedSize[DEBUG_SOURCE_TREE]);
        AZ_TracePrintf("HPHA", "Currently allocated blocks:\n");
        for (debug_record_map::const_iterator it = mDebugMap.begin(); it != mDebugMap.end(); ++it)
        {
            AZ_TracePrintf("HPHA", "ptr=%zX, size=%zi\n", (size_t)it->ptr(), it->size());
            it->print_stack();
        }
        AZ_TracePrintf("HPHA", "===========================================================\n");
    }
#endif // DEBUG_ALLOCATOR


    //=========================================================================
    // HphaScema
    // [2/22/2011]
    //=========================================================================
    HphaSchema::HphaSchema(const Descriptor& desc)
    {
        (void)m_pad; 
        m_capacity = 0;

        m_desc = desc;
        m_ownMemoryBlock = false;

        if (m_desc.m_fixedMemoryBlockByteSize > 0)
        {
            AZ_Assert((m_desc.m_fixedMemoryBlockByteSize & (m_desc.m_pageSize - 1)) == 0, "Memory block size %d MUST be multiples of the of the page size %d!", m_desc.m_fixedMemoryBlockByteSize, m_desc.m_pageSize);
            if (m_desc.m_fixedMemoryBlock == nullptr)
            {
                AZ_Assert(m_desc.m_subAllocator != nullptr, "Sub allocator must point to a valid allocator if m_fixedMemoryBlock is NOT allocated (NULL)!");
                m_desc.m_fixedMemoryBlock = m_desc.m_subAllocator->Allocate(m_desc.m_fixedMemoryBlockByteSize, m_desc.m_fixedMemoryBlockAlignment, 0, "HphaSchema", __FILE__, __LINE__, 1);
                AZ_Assert(m_desc.m_fixedMemoryBlock != nullptr, "Failed to allocate %d bytes!", m_desc.m_fixedMemoryBlockByteSize);
                m_ownMemoryBlock = true;
            }
            AZ_Assert((reinterpret_cast<size_t>(m_desc.m_fixedMemoryBlock) & static_cast<size_t>(desc.m_fixedMemoryBlockAlignment - 1)) == 0, "Memory block must be page size (%d bytes) aligned!", desc.m_fixedMemoryBlockAlignment);
            m_capacity = m_desc.m_fixedMemoryBlockByteSize;
        }
        else
        {
            m_capacity = desc.m_capacity;
        }

        AZ_Assert(sizeof(HpAllocator) <= sizeof(m_hpAllocatorBuffer), "Increase the m_hpAllocatorBuffer, we need %d bytes but we have %d bytes!", sizeof(HpAllocator), sizeof(m_hpAllocatorBuffer));
        m_allocator = new (&m_hpAllocatorBuffer) HpAllocator(m_desc);
    }

    //=========================================================================
    // ~HphaSchema
    // [2/22/2011]
    //=========================================================================
    HphaSchema::~HphaSchema()
    {
        m_capacity = 0;
        m_allocator->~HpAllocator();

        if (m_ownMemoryBlock)
        {
            m_desc.m_subAllocator->DeAllocate(m_desc.m_fixedMemoryBlock, m_desc.m_fixedMemoryBlockByteSize, m_desc.m_fixedMemoryBlockAlignment);
            m_desc.m_fixedMemoryBlock = nullptr;
        }
    }

    //=========================================================================
    // Allocate
    // [2/22/2011]
    //=========================================================================
    HphaSchema::pointer_type
    HphaSchema::Allocate(size_type byteSize, size_type alignment, int flags, const char* name, const char* fileName, int lineNum, unsigned int suppressStackRecord)
    {
        (void)flags;
        (void)name;
        (void)fileName;
        (void)lineNum;
        (void)suppressStackRecord;
        pointer_type address = m_allocator->alloc(byteSize, alignment);
        if (address == nullptr)
        {
            GarbageCollect();
            address = m_allocator->alloc(byteSize, alignment);
        }
        return address;
    }

    //=========================================================================
    // pointer_type
    // [2/22/2011]
    //=========================================================================
    HphaSchema::pointer_type
    HphaSchema::ReAllocate(pointer_type ptr, size_type newSize, size_type newAlignment)
    {
        pointer_type address = m_allocator->realloc(ptr, newSize, newAlignment);
        if (address == nullptr && newSize > 0)
        {
            GarbageCollect();
            address = m_allocator->realloc(ptr, newSize, newAlignment);
        }
        return address;
    }

    //=========================================================================
    // DeAllocate(pointer_type ptr,size_type size,size_type alignment)
    // [2/22/2011]
    //=========================================================================
    void
    HphaSchema::DeAllocate(pointer_type ptr, size_type size, size_type alignment)
    {
        if (ptr == nullptr)
        {
            return;
        }
        if (size == 0)
        {
            m_allocator->free(ptr);
        }
        else if (alignment == 0)
        {
            m_allocator->free(ptr, size);
        }
        else
        {
            m_allocator->free(ptr, size, alignment);
        }
    }

    //=========================================================================
    // Resize
    // [2/22/2011]
    //=========================================================================
    HphaSchema::size_type
    HphaSchema::Resize(pointer_type ptr, size_type newSize)
    {
        return m_allocator->resize(ptr, newSize);
    }

    //=========================================================================
    // AllocationSize
    // [2/22/2011]
    //=========================================================================
    HphaSchema::size_type
    HphaSchema::AllocationSize(pointer_type ptr)
    {
        return m_allocator->AllocationSize(ptr);
    }

    //=========================================================================
    // NumAllocatedBytes
    // [2/22/2011]
    //=========================================================================
    HphaSchema::size_type
    HphaSchema::NumAllocatedBytes() const
    {
        return m_allocator->allocated();
    }


    //=========================================================================
    // GetMaxAllocationSize
    // [2/22/2011]
    //=========================================================================
    HphaSchema::size_type
    HphaSchema::GetMaxAllocationSize() const
    {
        return m_allocator->GetMaxAllocationSize();
    }

    auto HphaSchema::GetMaxContiguousAllocationSize() const -> size_type
    {
        return m_allocator->GetMaxContiguousAllocationSize();
    }

    //=========================================================================
    // GetUnAllocatedMemory
    // [9/30/2013]
    //=========================================================================
    HphaSchema::size_type
    HphaSchema::GetUnAllocatedMemory(bool isPrint) const
    {
        return m_allocator->GetUnAllocatedMemory(isPrint);
    }

    //=========================================================================
    // GarbageCollect
    // [2/22/2011]
    //=========================================================================
    void
    HphaSchema::GarbageCollect()
    {
        m_allocator->purge();
    }
        
    size_t
    HphaSchema::Capacity() const
    { 
        // Do not return m_capacity if it was never initialized.  Instead return raw tracked numbers of how much the tree and buckets have grown
        if (m_capacity == AZ_CORE_MAX_ALLOCATOR_SIZE)
        {
            return m_allocator->mTotalCapacitySizeBuckets + m_allocator->mTotalCapacitySizeTree;
        }
        return m_capacity;
    }

} // namspace AZ
