/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/base.h>
#include <AzCore/std/algorithm.h>
#include <AzCore/std/allocator_stateless.h>

#include <AzCore/std/function/function_fwd.h> // for callbacks

#include <AzCore/Memory/OSAllocator.h>
#include <AzCore/std/containers/unordered_map.h>

namespace AZ
{
    namespace Debug
    {
        struct StackFrame;

        /**
        * Allocation tracking information.
        */
        struct AllocationInfo
        {
            size_t          m_byteSize{};
            const char*     m_name{};
            const char*     m_fileName{};
            int             m_lineNum{};
            unsigned int    m_alignment{};
            void*           m_namesBlock{}; ///< Memory block if m_name and m_fileName have been allocated specifically for this allocation record
            size_t          m_namesBlockSize{};

            AZ::Debug::StackFrame*  m_stackFrames{};
            unsigned int m_stackFramesCount{};

            AZ::u64         m_timeStamp{}; ///< Timestamp for sorting/tracking allocations
        };

        // We use OSAllocator which uses system calls to allocate memory, they are not recorded or tracked!
        using AllocationRecordsType = AZStd::unordered_map<void*, AllocationInfo, AZStd::hash<void*>, AZStd::equal_to<void*>, AZStd::stateless_allocator>;

        /**
         * Records enumeration callback
         * \param void* allocation address
         * \param const AllocationInfo& reference to the allocation record.
         * \param unsigned char number of stack records/levels, if AllocationInfo::m_stackFrames != NULL.
         * \param size_t total number of AllocationRecord objects being enumerated . This will remain the same throughout enumeration
         * \returns true if you want to continue traverse of the records and false if you want to stop.
         */
        using AllocationInfoCBType = AZStd::function<bool (void*, const AllocationInfo&, unsigned char, size_t numRecords)>;
        /**
         * Example of records enumeration callback.
         */
        struct PrintAllocationsCB
        {
            PrintAllocationsCB(bool isDetailed = false, bool includeNameAndFilename = false)
                : m_isDetailed(isDetailed), m_includeNameAndFilename(includeNameAndFilename) {}

            bool operator()(void* address, const AllocationInfo& info, unsigned char numStackLevels, size_t numRecords);

            bool m_isDetailed;      ///< True to print allocation line and allocation callstack, otherwise false.
            bool m_includeNameAndFilename;  /// < True to print the source name and source filename, otherwise skip
        };

        /**
         * Guard value is used to guard different memory allocations for stomping.
         */
        struct GuardValue
        {
            static const u32 m_defValue = 0xbad0babe;
            GuardValue()    { m_value = (m_defValue ^ (u32)((size_t) this)); }
            ~GuardValue()   { m_value = 0xf00d8bad; }
            bool Validate() const { return m_value == (m_defValue ^ (u32)((size_t) this)); }
            void ValidateWithAssert() const { AZ_Assert(m_value == m_defValue, "Guard value doesn't match! Value: 0x%08x should be: 0x%08x", m_value, m_defValue); }
        private:
            u32     m_value;
        };

        // Using the AZ_ENUM* macro to allow an AZ EnumTraits structure
        // to be associated with the AllocationRecord Mode enum
        // This is used by the ConsoleTypeHelpers to allow
        // conversion from an enum Value string to an AllocationRecordMode
        AZ_ENUM_WITH_UNDERLYING_TYPE(AllocationRecordMode, int,
            RECORD_NO_RECORDS,              ///< Never record any information.
            RECORD_STACK_NEVER,             ///< Never record stack traces. All other info is stored.
            RECORD_STACK_IF_NO_FILE_LINE,   ///< Record stack if fileName and lineNum are not available. (default)
            RECORD_FULL,                    ///< Always record the full stack.

            RECORD_MAX                      ///< Must be last
        );

        /**
        * Container for debug allocation records. This
        * records can be thread safe or not depending on your
        * needs. When you set the thread safe flag all
        * functions will be thread safe unless explicitly noted.
        *
        * IMPORTANT: If you enable isAllocationGuard (true), you will
        * need to make sure every every has \ref MemoryGuardSize() bytes at end.
        * This is where the memory guard will be located. Failure to do so will cause failed memory stomps
        * and possible memory corruption.
        */
        class AllocationRecords
        {
        public:
            using Mode = AllocationRecordMode;

            /**
             * IMPORTANT: if isAllocationGuard
             */

            AllocationRecords(unsigned char stackRecordLevels, bool isMemoryGuard, bool isMarkUnallocatedMemory, const char* allocatorName);

            unsigned int  MemoryGuardSize() const               { return m_memoryGuardSize; }

            //////////////////////////////////////////////////////////////////////////
            // lock interface (can be used with std/AZStd::lock)
            void    lock();
            bool    try_lock();
            void    unlock();
            //////////////////////////////////////////////////////////////////////////

            /// Enabling too much stack recording may cause performance issues. Use wisely.
            void    SetMode(Mode mode);
            Mode    GetMode() const                             { return m_mode; }
            void    SetSaveNames(bool saveNames)                { m_saveNames = saveNames; }
            void    SetDecodeImmediately(bool decodeImmediately) { m_decodeImmediately = decodeImmediately; }

            /// Returns number of stack levels that will captured for each allocation when requested (depending on the \ref Mode)
            unsigned char   GetNumStackLevels() const           { return m_numStackLevels; }

            /// Not thread safe!!! Make sure you lock/unlock while you work with the records.
            AZ_FORCE_INLINE Debug::AllocationRecordsType& GetMap()  { return m_records; }

            /// Enumerates all allocations in a thread safe manner.
            void    EnumerateAllocations(AllocationInfoCBType cb) const;

            /// If marking is enabled it will set all memory we deallocate with 0xcd
            void    MarkUallocatedMemory(bool isMark)           { m_isMarkUnallocatedMemory = isMark; }
            bool    IsMarkUnallocatedMemory() const             { return m_isMarkUnallocatedMemory; }
            int     GetUnallocatedMarkValue() const             { return 0xcd; }

            /// Checks the integrity of the allocator. Enabled if isMemoryGuard is set to true. This can be a slow operation.
            void    IntegrityCheck() const;
            /// Enables IntergrityCheck on allocation and deallocation. Enabled only if isMemoryGuard is set to true. This will cause allocation/deallocation to be VERY SLOW!
            void    AutoIntegrityCheck(bool enable)             { m_isAutoIntegrityCheck = enable; }

            /// Returns peak of requested memory. IMPORTANT: This is user requested memory! Any allocator overhead is NOT included.
            size_t  RequestedBytesPeak() const                  { return m_requestedBytesPeak; }
            /// Reset the peak allocation to the current requested memory.
            void    ResetPeakBytes()                            { m_requestedBytesPeak.store(m_requestedBytes); }
            /// Return requested user bytes. IMPORTANT: This is user requested memory! Any allocator overhead is NOT included.
            size_t  RequestedBytes() const                      { return m_requestedBytes; }
            /// Returns total number of requested allocations.
            size_t  RequestedAllocs() const                     { return m_requestedAllocs; }

            const char* GetAllocatorName() const                { return m_allocatorName; }

            // @{ Allocation tracking management - we assume this functions are called with the lock locked.
            const AllocationInfo*   RegisterAllocation(void* address, size_t byteSize, size_t alignment, unsigned int stackSuppressCount);
            void    UnregisterAllocation(void* address, size_t byteSize, size_t alignment, AllocationInfo* info);
            // the address of the variable will not change we are just updating the statistics.
            void    ResizeAllocation(void* address, size_t newSize);

            void RegisterReallocation(void* address, void* newAddress, size_t byteSize, size_t alignment, unsigned int stackSuppressCount);
            // @}

        protected:
            Debug::AllocationRecordsType    m_records;
            mutable AZStd::spin_mutex       m_recordsMutex;
            Mode                            m_mode;
            bool                            m_isAutoIntegrityCheck;
            bool                            m_isMarkUnallocatedMemory;      ///< True if we want to set value 0xcd in unallocated memory.
            bool                            m_saveNames;
            bool                            m_decodeImmediately;
            unsigned char                   m_numStackLevels;
            unsigned int                    m_memoryGuardSize;
            AZStd::atomic<size_t>           m_requestedAllocs;
            AZStd::atomic<size_t>           m_requestedBytes;
            AZStd::atomic<size_t>           m_requestedBytesPeak;

            const char*                     m_allocatorName;
        };
    }

    AZ_TYPE_INFO_SPECIALIZE(Debug::AllocationRecords::Mode, "{C007B46A-3827-42DC-B56D-0484BC9942A9}");
}




