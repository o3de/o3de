/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if defined(CARBONATED)
#include <thread>
#endif
#include <AzCore/base.h>
#include <AzCore/Memory/AllocationRecords.h>
#include <AzCore/std/algorithm.h>
#include <AzCore/std/parallel/mutex.h>
#include <AzCore/std/string/string.h>

#include <AzCore/std/functional.h> // for callbacks

#include <AzCore/std/smart_ptr/shared_ptr.h>


namespace AZ
{
    class IAllocator;

    /**
    * Global allocation manager. It has access to all
    * created allocators IAllocator interface. And control
    * some global allocations. This manager is NOT thread safe, so all allocators
    * should be created on the same thread (we can change that if needed, we just need a good reason).
    */
    class AllocatorManager
    {
        friend IAllocator;
        friend class AllocatorBase;
        friend class Debug::AllocationRecords;

    public:

        AllocatorManager();
        ~AllocatorManager();

        typedef AZStd::function<void (IAllocator* allocator, size_t /*byteSize*/, size_t /*alignment*/)>    OutOfMemoryCBType;

        static AllocatorManager& Instance();
        static bool IsReady();
        static void Destroy();

        class AllocatorLock
        {
        public:
            virtual ~AllocatorLock();
        };

        /// Locks the allocator manager so you can safely iterate over its contents. The manager will unlock
        /// when the returned object goes out of scope.
        ///
        /// Allocators typically don't change during the lifetime of the app, but some systems may despawn
        /// or respawn allocators on a more frequent basis, e.g. when changing levels. So it is recommended
        /// to lock the manager whenever iterating over allocators.
        AZStd::shared_ptr<AllocatorLock> LockAllocators();

        AZ_FORCE_INLINE  int            GetNumAllocators() const    { return m_numAllocators; }
        AZ_FORCE_INLINE  IAllocator*    GetAllocator(int index)
        {
            AZ_Assert(index < m_numAllocators, "Invalid allocator index %d [0,%d]!", index, m_numAllocators - 1);
            return m_allocators[index];
        }
        /// Calls all registered allocators garbage collect function.
        void    GarbageCollect();
        /// Add out of memory listener. True if add was successful, false if listener was already added.
        bool    AddOutOfMemoryListener(const OutOfMemoryCBType& cb);
        /// Remove out of memory listener.
        void    RemoveOutOfMemoryListener();

        /// Set default memory track mode for all allocators created after this point.
        void    SetDefaultTrackingMode(AZ::Debug::AllocationRecords::Mode mode) { m_defaultTrackingRecordMode = mode; }
        AZ::Debug::AllocationRecords::Mode  GetDefaultTrackingMode() const      { return m_defaultTrackingRecordMode; }

        /// Set memory track mode for all allocators already created.
        void    SetTrackingMode(AZ::Debug::AllocationRecords::Mode mode);

        /// Especially for great code and engines...
        void    SetAllocatorLeaking(bool allowLeaking)  { m_isAllocatorLeaking = allowLeaking; }

        /// Enter or exit profiling mode; calls to Enter must be matched with calls to Exit
        void EnterProfilingMode();
        void ExitProfilingMode();

        // Controls if profiling should be enabled for newly created allocators
        void SetDefaultProfilingState(bool newState) { m_defaultProfilingState = newState; }
        bool GetDefaultProfilingState() const { return m_defaultProfilingState; }

        /// Outputs allocator usage to the console, and also stores the values in m_dumpInfo for viewing in the crash dump
        void DumpAllocators();

        void SetTrackingForAllocator(AZStd::string_view allocatorName, AZ::Debug::AllocationRecords::Mode recordMode);
        bool RemoveTrackingForAllocator(AZStd::string_view allocatorName);

        struct DumpInfo
        {
            // Must contain only POD types
            const char* m_name;
            size_t m_used;
            size_t m_reserved;
            size_t m_consumed;
        };

        struct AllocatorStats
        {
            AllocatorStats(const char* name, size_t allocatedBytes, size_t capacityBytes)
                : m_name(name)
                , m_allocatedBytes(allocatedBytes)
                , m_capacityBytes(capacityBytes)
            {}

            AZStd::string m_name;
            size_t m_allocatedBytes;
            size_t m_capacityBytes;
        };

        void GetAllocatorStats(size_t& usedBytes, size_t& reservedBytes, AZStd::vector<AllocatorStats>* outStats = nullptr);

        //////////////////////////////////////////////////////////////////////////
        // Debug support
        static const int MaxNumMemoryBreaks = 5;
        struct MemoryBreak
        {
            MemoryBreak();
            void*           addressStart;
            void*           addressEnd;
            size_t          byteSize;
            unsigned int    alignment;
            const char*     name;

            const char*     fileName;
            int             lineNum;
        };
        /// Installs a memory break on a specific slot (0 to MaxNumMemoryBreaks). Code will trigger and DebugBreak.
        void SetMemoryBreak(int slot, const MemoryBreak& mb);
        /// Reset a memory break. -1 all slots, otherwise (0 to MaxNumMemoryBreaks)
        void ResetMemoryBreak(int slot = -1);
        //////////////////////////////////////////////////////////////////////////

#if defined(CARBONATED)
        struct CodePoint
        {
            const char* m_name;
            const char* m_file;
            int m_line;
            bool m_isLiteral;

            CodePoint()
                : m_name(nullptr)
                , m_file(nullptr)
            {
            }

            CodePoint(const char* name, const char* file, int line, bool isLiteral)
                : m_name(name)
                , m_file(file)
                , m_line(line)
                , m_isLiteral(isLiteral)
            {
            }
        };

        /// Returns current thread's top stack registered code point
        AZStd::tuple<const CodePoint*, uint64_t, unsigned int, const char*> GetCodePointMaskTagAsset();

        /// Pushes memory marker to stack, this speeds up memory tracking by dropping callstack symbolication, see MEMORY_ALLOCATION_MARKER_NAME macro
        void PushMemoryMarker(const CodePoint& point);

        /// Pop memory marker from stack
        void PopMemoryMarker();

        /// Pushes memory tag to stack, an alternative memory marking way, see MEMORY_TAG macro
        void PushMemoryTag(unsigned int tag);

        /// Pop memory tag from stack
        void PopMemoryTag();

        /// Pushes asset memory tag to stack, see ASSET_TAG macro
        void PushAssetMemoryTag(const char* name);

        /// Pop asset memory tag from stack
        void PopAssetMemoryTag();

        /// Protection from a recursive call by the same thread
        bool IsRecursive()
        {
            return m_recursive;
        }
        //////////////////////////////////////////////////////////////////////////
#endif  // CARBONATED

        // Called from IAllocator
        void RegisterAllocator(IAllocator* alloc);
        void UnRegisterAllocator(IAllocator* alloc);

    private:
        void InternalDestroy();
        void DebugBreak(void* address, const Debug::AllocationInfo& info);

        AllocatorManager(const AllocatorManager&);
        AllocatorManager& operator=(const AllocatorManager&);

        static constexpr int m_maxNumAllocators = 100;
        IAllocator*         m_allocators[m_maxNumAllocators];
        volatile int        m_numAllocators;
        OutOfMemoryCBType   m_outOfMemoryListener;
        bool                m_isAllocatorLeaking;
        bool                m_defaultProfilingState = false;
        MemoryBreak         m_memoryBreak[MaxNumMemoryBreaks];
        char                m_activeBreaks;
        AZStd::mutex        m_allocatorListMutex;

        DumpInfo            m_dumpInfo[m_maxNumAllocators];

        AZStd::atomic<int>  m_profilingRefcount;

        AZ::Debug::AllocationRecords::Mode m_defaultTrackingRecordMode;

        using AllocatorName = AZStd::fixed_string<128>;
        //! Stores the name
        struct AllocatorTrackingConfig
        {
            AllocatorName m_allocatorName;
            AZ::Debug::AllocationRecords::Mode m_recordMode{ AZ::Debug::AllocationRecords::Mode::RECORD_NO_RECORDS };
        };
        AZStd::fixed_vector<AllocatorTrackingConfig, m_maxNumAllocators> m_allocatorTrackingConfigs;

        static AllocatorManager g_allocMgr;    ///< The single instance of the allocator manager

    private:
        void ConfigureTrackingForAllocator(IAllocator* alloc, const AllocatorTrackingConfig& foundTrackingConfigIt);


#if defined(CARBONATED)
    private:
        // this is a specfic stack that can grow beyond its capacity not storing values
        template<typename Data, int Size>
        class DataStack
        {
        public:
            void Push(const Data& d)
            {
                AZ_Assert(!IsFull(), "Push, but full");
                m_stack[m_numItems++] = d;
            }
            void SimulatePush()  // stack grows beyond the capacity
            {
                AZ_Assert(IsFull(), "Simulate push, but not full");
                m_numItems++;
            }
            void Pop()
            {
                AZ_Assert(!IsEmpty(), "Pop, but empty");
                m_numItems--;
            }

            const Data& Get() const
            {
                AZ_Assert(!IsEmpty(), "Get, but empty");
                AZ_Assert(!IsOverflow(), "Get, but overflow");
                return m_stack[m_numItems - 1];
            }

            bool IsFull() const
            {
                return m_numItems >= Size;
            }
            bool IsOverflow() const
            {
                return m_numItems > Size;
            }
            bool IsEmpty() const
            {
                return m_numItems == 0;
            }

        private:
            Data m_stack[Size];
            int m_numItems = 0;
        };

        class AssetMemoryItem
        {
        public:
            AssetMemoryItem(const char* name)
            {
                const size_t size = strlen(name) + 1;
                mName = aznew char[size];
                memcpy(mName, name, size);
            }

            const char* GetName() const
            {
                return mName;
            }

        private:
            char* mName;
        };

        struct ThreadLocalData
        {
            DataStack<CodePoint, 64> m_allocationMarkers;
            DataStack<unsigned int, 64> m_allocationTags;
            uint64_t m_tagMask = 0;

            DataStack<AssetMemoryItem*, 64> m_assetItems;
        };
        AZStd::unordered_map<std::thread::id, ThreadLocalData, std::hash<std::thread::id>, AZStd::equal_to<std::thread::id>, AZStd::stateless_allocator>
            m_threadData;
        volatile bool m_recursive = false;
        AZStd::mutex m_threadDataLock;

        class hash_string
        {
        public:
            size_t operator()(const char* p) const
            {
                // FNV hash 64 (maybe tryMurmurHash3 ?)
                size_t h = 14695981039346656037llu;  // 32bit 2166136261u;
                while (*p)
                {
                    h ^= *(p++);
                    h *= 1099511628211llu;  // 32bit 16777619u;
                }
                return h;
            }
        };
        AZStd::unordered_map<const char*, AssetMemoryItem*, hash_string> m_assetMap;
        AZStd::mutex m_assetMapLock;

        ThreadLocalData& FindThreadData();
#endif // CARBONATED
    };
}
