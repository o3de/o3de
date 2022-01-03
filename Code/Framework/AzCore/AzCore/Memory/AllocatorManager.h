/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

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
    class MallocSchema;

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
        template<typename T, typename... Args> friend constexpr auto AZStd::construct_at(T*, Args&&... args)
            ->AZStd::enable_if_t<AZStd::is_void_v<AZStd::void_t<decltype(new (AZStd::declval<void*>()) T(AZStd::forward<Args>(args)...))>>, T*>;
        template<typename T> constexpr friend void AZStd::destroy_at(T*);

    public:
        typedef AZStd::function<void (IAllocator* allocator, size_t /*byteSize*/, size_t /*alignment*/, int/* flags*/, const char* /*name*/, const char* /*fileName*/, int lineNum /*=0*/)>    OutOfMemoryCBType;

        static void PreRegisterAllocator(IAllocator* allocator);  // Only call if the environment is not yet attached
        static IAllocator* CreateLazyAllocator(size_t size, size_t alignment, IAllocator*(*creationFn)(void*));  // May be called at any time before shutdown
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

        /// Set an override allocator
        /// All allocators registered with the AllocatorManager will automatically redirect to this allocator
        /// if set.
        void    SetOverrideAllocatorSource(IAllocatorAllocate* source, bool overrideExistingAllocators = true);

        /// Retrieve the override schema
        IAllocatorAllocate* GetOverrideAllocatorSource() const  { return m_overrideSource; }

        void AddAllocatorRemapping(const char* fromName, const char* toName);
        void FinalizeConfiguration();

        /// Enter or exit profiling mode; calls to Enter must be matched with calls to Exit
        void EnterProfilingMode();
        void ExitProfilingMode();

        /// Outputs allocator useage to the console, and also stores the values in m_dumpInfo for viewing in the crash dump
        void DumpAllocators();

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
            AllocatorStats(const char* name, const char* aliasOrDescription, size_t allocatedBytes, size_t capacityBytes, bool isAlias)
                : m_name(name)
                , m_aliasOrDescription(aliasOrDescription)
                , m_allocatedBytes(allocatedBytes)
                , m_capacityBytes(capacityBytes)
                , m_isAlias(isAlias)
            {}

            AZStd::string m_name;
            AZStd::string m_aliasOrDescription;
            size_t m_allocatedBytes;
            size_t m_capacityBytes;
            bool   m_isAlias;
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

        // Called from IAllocator
        void RegisterAllocator(IAllocator* alloc);
        void UnRegisterAllocator(IAllocator* alloc);

    private:
        void InternalDestroy();
        void ConfigureAllocatorOverrides(IAllocator* alloc);
        void DebugBreak(void* address, const Debug::AllocationInfo& info);
        AZ::MallocSchema* CreateMallocSchema();

        AllocatorManager(const AllocatorManager&);
        AllocatorManager& operator=(const AllocatorManager&);

        static const int m_maxNumAllocators = 100;
        IAllocator*         m_allocators[m_maxNumAllocators];
        volatile int        m_numAllocators;
        OutOfMemoryCBType   m_outOfMemoryListener;
        bool                m_isAllocatorLeaking;
        MemoryBreak         m_memoryBreak[MaxNumMemoryBreaks];
        char                m_activeBreaks;
        AZStd::mutex        m_allocatorListMutex;
        IAllocatorAllocate* m_overrideSource;

        DumpInfo            m_dumpInfo[m_maxNumAllocators];

        struct InternalData;

        InternalData*       m_data;
        bool                m_configurationFinalized;
        AZStd::atomic<int>  m_profilingRefcount;

        AZ::Debug::AllocationRecords::Mode m_defaultTrackingRecordMode;
        AZStd::unique_ptr<AZ::MallocSchema, void(*)(AZ::MallocSchema*)> m_mallocSchema;

        AllocatorManager();
        ~AllocatorManager();

        static AllocatorManager g_allocMgr;    ///< The single instance of the allocator manager
    };
}
