/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Console/IConsole.h>
#include <AzCore/Math/Crc.h>
#include <AzCore/Memory/AllocatorManager.h>
#include <AzCore/Memory/Memory.h>
#include <AzCore/Memory/IAllocator.h>

#include <AzCore/Memory/OSAllocator.h>
#include <AzCore/Memory/AllocationRecords.h>
#include <AzCore/Debug/StackTracer.h>

#include <AzCore/std/parallel/lock.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/std/containers/array.h>

namespace AZ
{
    static void sys_DumpAllocators([[maybe_unused]] const AZ::ConsoleCommandContainer& arguments)
    {
        AllocatorManager::Instance().DumpAllocators();
    }
    AZ_CONSOLEFREEFUNC(sys_DumpAllocators, AZ::ConsoleFunctorFlags::Null, "Print memory allocator statistics.");

    // Provides a range of allocations to dump. The min value is inclusive and the max value is exclusive
    // Theerefore the range is [min, max)
    struct AllocationDumpRange
    {
        size_t m_min = 0;
        size_t m_max = AZStd::numeric_limits<size_t>::max();
    };

    // Add console command for outputting the Allocation records for all or a specified set of allocators with the provided names
    static void DumpAllocationsForAllocatorHelper(AZStd::span<AZStd::string_view const> allocatorNameArguments, const AllocationDumpRange& allocationDumpRange = {})
    {
        auto& allocatorManager = AZ::AllocatorManager::Instance();
        const int numAllocators = allocatorManager.GetNumAllocators();
        // Garbage collect the allocations before dumping them
        allocatorManager.GarbageCollect();

        AZStd::vector<IAllocator*> allocatorsToDump;
        for (int i = 0; i < numAllocators; i++)
        {
            auto* allocator = allocatorManager.GetAllocator(i);
            const AZ::Debug::AllocationRecords* records = allocator->GetRecords();
            if (records)
            {
                // If no allocator name arguments to the Console command has been supplied
                // dump all allocators that record allocation records
                if (allocatorNameArguments.empty())
                {
                    allocatorsToDump.emplace_back(allocator);
                    // Skip back to the next allocator
                    continue;
                }

                // If the allocator has records and it matches the name of one of the allocators
                auto IsAllocatorInNameSet = [allocator](AZStd::string_view searchName)
                {
                    return searchName == allocator->GetName();
                };
                if (AZStd::any_of(allocatorNameArguments.begin(), allocatorNameArguments.end(), IsAllocatorInNameSet))
                {
                    allocatorsToDump.emplace_back(allocator);
                }
            }
        }

        using AllocationString = AZStd::fixed_string<1024>;

        static constexpr const char* MemoryTag = "mem";

        size_t allocationCount{};
        size_t totalAllocations{};

        constexpr bool includeAllocationLineAndCallstack = true;
        constexpr bool includeAllocatorNameAndSourceName = true;
        auto PrintAllocations = [MemoryTag = MemoryTag, includeAllocationLineAndCallstack, includeAllocatorNameAndSourceName,
            &allocationDumpRange, &allocationCount, &totalAllocations](
                                    void* address, const Debug::AllocationInfo& info, unsigned int numStackLevels, size_t numRecords)
        {
            totalAllocations = numRecords;
            auto& tracer = AZ::Debug::Trace::Instance();


            // Only dump allocations in specified index range
            if (allocationCount >= allocationDumpRange.m_min && allocationCount < allocationDumpRange.m_max)
            {
                if (includeAllocatorNameAndSourceName && info.m_name)
                {
                    tracer.Output(
                        MemoryTag,
                        AllocationString::format(
                            R"(Allocation Name: "%s" Addr: 0%p Size: %d Alignment: %d)"
                            "\n",
                            info.m_name,
                            address,
                            info.m_byteSize,
                            info.m_alignment)
                        .c_str());
                }
                else
                {
                    tracer.Output(
                        MemoryTag,
                        AllocationString::format("Allocation Addr: 0%p Size: %d Alignment: %d\n", address, info.m_byteSize, info.m_alignment).c_str());
                }

                if (includeAllocationLineAndCallstack)
                {
                    // If there is no stack frame records, output the location where the allocation took place
                    if (!info.m_stackFrames)
                    {
                        // For the callstack use the NoWindow constant to have the stack frames be visually aligned beneath the previous
                        // "mem" window entry
                        tracer.Output(
                            AZ::Debug::Trace::GetNoWindow(),
                            AllocationString::format(
                                R"( "%s" (%d))"
                                "\n",
                                info.m_fileName,
                                info.m_lineNum)
                                .c_str());
                    }
                    else
                    {
                        constexpr unsigned int MaxStackFramesToDecode = 30;
                        Debug::SymbolStorage::StackLine decodedStackLines[MaxStackFramesToDecode];

                        for (size_t currentStackFrame{}; numStackLevels > 0;)
                        {
                            // Decode up to lower of MaxStackFramesToDecode and the remaining number of stack frames per loop iteration
                            unsigned int stackFramesToDecode = AZStd::GetMin(MaxStackFramesToDecode, static_cast<unsigned int>(numStackLevels));
                            Debug::SymbolStorage::DecodeFrames(info.m_stackFrames + currentStackFrame, stackFramesToDecode, decodedStackLines);
                            for (unsigned int i = 0; i < stackFramesToDecode; ++i)
                            {
                                if (info.m_stackFrames[currentStackFrame + i].IsValid())
                                {
                                    tracer.Output(AZ::Debug::Trace::GetNoWindow(), AllocationString::format(" %s\n", decodedStackLines[i]).c_str());
                                }
                            }
                            numStackLevels -= stackFramesToDecode;
                            currentStackFrame += stackFramesToDecode;
                        }
                    }
                }
            }

            ++allocationCount;

            return true;
        };

        AZ::Debug::Trace::Instance().Output(
            MemoryTag, AllocationString("Dumping allocation records. This may take awhile\n").c_str());
        // Iterate over each allocator to dump and print their allocation records
        for (AZ::IAllocator* allocator : allocatorsToDump)
        {
            AZ::Debug::AllocationRecords* records = allocator->GetRecords();
            AZ::Debug::Trace::Instance().Output(
                MemoryTag, AllocationString::format("Dumping allocation records for allocator %s\n", allocator->GetName()).c_str());
            allocationCount = 0;
            records->EnumerateAllocations(PrintAllocations);
            AZ::Debug::Trace::Instance().Output(
                MemoryTag, AllocationString::format("Allocator %s contains %zu allocations \n", allocator->GetName(), totalAllocations).c_str());
        }
    }

    static void DumpAllocationsForAllocator(const AZ::ConsoleCommandContainer& arguments)
    {
        DumpAllocationsForAllocatorHelper(arguments);
    }
    AZ_CONSOLEFREEFUNC("sys_DumpAllocationRecordsSlow", DumpAllocationsForAllocator, AZ::ConsoleFunctorFlags::Null,
        "Print ALL individual allocations for the specified allocator.\n"
        " If no allocator is specified, then all allocations are dumped\n"
        "NOTE: This can be very slow depending on the number of allocations\n"
        R"(For better control of which allocations get printed, use the "sys_DumpAllocationRecordInRange" command)" "\n"
        "usage: sys_DumpAllocationsRecords <allocator name...>\n"
        "Ex. `sys_DumpAllocationsRecords SystemAllocator`");

    static void DumpAllocationsForAllocatorInRange(const AZ::ConsoleCommandContainer& arguments)
    {
        using AllocationString = AZStd::fixed_string<1024>;
        static constexpr const char* MemoryTag = "mem";

        constexpr size_t rangeArgumentCount = 2;

        if (arguments.size() < rangeArgumentCount)
        {
            AZ_Error(
                MemoryTag,
                false,
                AllocationString(
                    R"("sys_DumpAllocationRecordsInRange" command requires the first two arguments to specify a range of allocation records to dump.)"
                    "\n")
                    .c_str());
            return;
        }

        // Try to convert the first argument to a number
        AllocationDumpRange dumpRange;
        if (!ConsoleTypeHelpers::ToValue(dumpRange.m_min, arguments[0]))
        {
            AZ_Error(
                MemoryTag,
                false,
                AllocationString::format(
                    R"(Unable to convert the min argument of "%.*s" to an integer .)"
                    "\n")
                .c_str());
            return;
        }
        if (!ConsoleTypeHelpers::ToValue(dumpRange.m_max, arguments[1]))
        {
            AZ_Error(
                MemoryTag,
                false,
                AllocationString::format(
                    R"(Unable to convert the max argument of "%.*s" to an integer .)"
                    "\n")
                .c_str());
            return;
        }
        // Create a subspan of the list of arguments where the allocator name starts from
        // The first two arguments represent the range of allocations to print
        AZStd::span<AZStd::string_view const> allocatorNameArguments{ AZStd::next(arguments.begin(), rangeArgumentCount), arguments.end() };
        DumpAllocationsForAllocatorHelper(allocatorNameArguments, dumpRange);
    }

    AZ_CONSOLEFREEFUNC(
        "sys_DumpAllocationRecordsInRange",
        DumpAllocationsForAllocatorInRange,
        AZ::ConsoleFunctorFlags::Null,
        "Print allocations records in the specified index range of min to max for any allocations.\n"
        " If no allocator is specified, then all registered allocator allocations records are dumped in the specified range\n"
        "usage: sys_DumpAllocationsRecords <min-inclusive-index> <max-exclusive-index> <allocator name...>\n"
        "Ex. Dump the first 100 allocations of the System Allocator\n"
        "`sys_DumpAllocationsRecords 0 100 SystemAllocator'\n"
        "Ex. Dump all but first 100 records of the OSAllocator\n"
        "`sys_DumpAllocationsRecords 100 18446744073709552000 OSAllocator'\n"
        "NOTE: smaller values for the max index can be specified and still print out all the allocations, as long as it larger than the "
        "total number of allocation records\n");

    static EnvironmentVariable<AllocatorManager>& GetAllocatorManagerEnvVar()
    {
        static EnvironmentVariable<AllocatorManager> s_allocManager;
        return s_allocManager;
    }

    static AllocatorManager* s_allocManagerDebug; // For easier viewing in crash dumps

    //////////////////////////////////////////////////////////////////////////
    bool AllocatorManager::IsReady()
    {
        return GetAllocatorManagerEnvVar().IsConstructed();
    }
    //////////////////////////////////////////////////////////////////////////
    void AllocatorManager::Destroy()
    {
    }

    //////////////////////////////////////////////////////////////////////////
    // The only allocator manager instance.
    AllocatorManager& AllocatorManager::Instance()
    {
        auto& allocatorManager = GetAllocatorManagerEnvVar();
        if (!allocatorManager)
        {
            allocatorManager = Environment::CreateVariable<AllocatorManager>(AZ_CRC_CE("AZ::AllocatorManager::s_allocManager"));

            s_allocManagerDebug = &(*allocatorManager);
        }

        return *allocatorManager;
    }
    //////////////////////////////////////////////////////////////////////////

    //=========================================================================
    // AllocatorManager
    // [9/11/2009]
    //=========================================================================
    AllocatorManager::AllocatorManager()
        : m_profilingRefcount(0)
    {
        m_numAllocators = 0;
        m_isAllocatorLeaking = false;
        m_defaultTrackingRecordMode = Debug::AllocationRecords::Mode::RECORD_NO_RECORDS;
    }

    //=========================================================================
    // ~AllocatorManager
    // [9/11/2009]
    //=========================================================================
    AllocatorManager::~AllocatorManager()
    {
        InternalDestroy();
    }

    //=========================================================================
    // RegisterAllocator
    // [9/17/2009]
    //=========================================================================
    void AllocatorManager::RegisterAllocator(class IAllocator* alloc)
    {
        AZStd::lock_guard<AZStd::mutex> lock(m_allocatorListMutex);
        AZ_Assert(m_numAllocators < m_maxNumAllocators, "Too many allocators %d! Max is %d", m_numAllocators, m_maxNumAllocators);

        for (size_t i = 0; i < m_numAllocators; i++)
        {
            AZ_Assert(m_allocators[i] != alloc, "Allocator %s (%s) registered twice!", alloc->GetName());
        }

        m_allocators[m_numAllocators++] = alloc;
        alloc->SetProfilingActive(m_defaultProfilingState);

        // If there is an allocator tracking config entry for the allocator
        // stored, use it's recording mode option to turn on allocation tracking
        auto FindAllocatorTrackingConfigByName = [allocatorName = alloc->GetName()](const AllocatorTrackingConfig& trackingConfig)
        {
            // performs a case-insensitive compare of the allocator name against the tracking config value
            return AZ::StringFunc::Equal(trackingConfig.m_allocatorName, allocatorName);
        };

        if (auto foundTrackingConfigIt =
                AZStd::find_if(m_allocatorTrackingConfigs.begin(), m_allocatorTrackingConfigs.end(), FindAllocatorTrackingConfigByName);
            foundTrackingConfigIt != m_allocatorTrackingConfigs.end())
        {
            ConfigureTrackingForAllocator(alloc, *foundTrackingConfigIt);
        }
    }

    void AllocatorManager::ConfigureTrackingForAllocator(IAllocator* alloc, const AllocatorTrackingConfig& allocatorTrackingConfig)
    {
        if (AZ::Debug::AllocationRecords* records = alloc->GetRecords(); records != nullptr)
        {
            records->SetMode(allocatorTrackingConfig.m_recordMode);
            if (allocatorTrackingConfig.m_recordMode != AZ::Debug::AllocationRecords::Mode::RECORD_NO_RECORDS)
            {
                // The AllocatorBase::ProfileAllocation function requires that the allocator has profiling turned on to record
                // allocations
                // Profiling is turned if Allocation Record mode is set to record any kind record
                constexpr bool profileAllocations = true;
                alloc->SetProfilingActive(profileAllocations);
            }
        }
    }

    //=========================================================================
    // InternalDestroy
    // [11/7/2018]
    //=========================================================================
    void AllocatorManager::InternalDestroy()
    {
        while (m_numAllocators > 0)
        {
            IAllocator* allocator = m_allocators[m_numAllocators - 1];
            (void)allocator;
            m_allocators[--m_numAllocators] = nullptr;
            // Do not actually destroy the lazy allocator as it may have work to do during non-deterministic shutdown
        }

        if (!m_isAllocatorLeaking)
        {
            AZ_Assert(m_numAllocators == 0, "There are still %d registered allocators!", m_numAllocators);
        }
    }

    //=========================================================================
    // UnRegisterAllocator
    // [9/17/2009]
    //=========================================================================
    void AllocatorManager::UnRegisterAllocator(class IAllocator* alloc)
    {
        AZStd::lock_guard<AZStd::mutex> lock(m_allocatorListMutex);

        for (int i = 0; i < m_numAllocators; ++i)
        {
            if (m_allocators[i] == alloc)
            {
                --m_numAllocators;
                m_allocators[i] = m_allocators[m_numAllocators];
            }
        }
    }

    //=========================================================================
    // LockAllocators
    // [11/21/2018]
    //=========================================================================
    AllocatorManager::AllocatorLock::~AllocatorLock()
    {
    }

    AZStd::shared_ptr<AllocatorManager::AllocatorLock> AllocatorManager::LockAllocators()
    {
        class AllocatorLockImpl : public AllocatorLock
        {
        public:
            AllocatorLockImpl(AZStd::mutex& mutex)
                : m_lock(mutex)
            {
            }

            AZStd::unique_lock<AZStd::mutex> m_lock;
        };

        AZStd::shared_ptr<AllocatorLock> result = AZStd::allocate_shared<AllocatorLockImpl>(OSStdAllocator(), m_allocatorListMutex);

        return result;
    }

    //=========================================================================
    // GarbageCollect
    // [9/11/2009]
    //=========================================================================
    void AllocatorManager::GarbageCollect()
    {
        AZStd::lock_guard<AZStd::mutex> lock(m_allocatorListMutex);

        // Allocators can use other allocators. When this happens, the dependent
        // allocators are registered first. By running garbage collect on the
        // allocators in reverse order, the ones with no dependencies are garbage
        // collected first, which frees up more allocations for the dependent
        // allocators to release.
        for (int i = m_numAllocators - 1; i >= 0; --i)
        {
            m_allocators[i]->GarbageCollect();
        }
    }

    //=========================================================================
    // AddOutOfMemoryListener
    // [12/2/2010]
    //=========================================================================
    bool AllocatorManager::AddOutOfMemoryListener(const OutOfMemoryCBType& cb)
    {
        AZ_Warning("Memory", !m_outOfMemoryListener, "Out of memory listener was already installed!");
        if (!m_outOfMemoryListener)
        {
            m_outOfMemoryListener = cb;
            return true;
        }

        return false;
    }

    //=========================================================================
    // RemoveOutOfMemoryListener
    // [12/2/2010]
    //=========================================================================
    void AllocatorManager::RemoveOutOfMemoryListener()
    {
        m_outOfMemoryListener = nullptr;
    }

    //=========================================================================
    // SetTrackingMode
    // [9/16/2011]
    //=========================================================================
    void AllocatorManager::SetTrackingMode(Debug::AllocationRecords::Mode mode)
    {
        AZStd::lock_guard<AZStd::mutex> lock(m_allocatorListMutex);
        for (int i = 0; i < m_numAllocators; ++i)
        {
            Debug::AllocationRecords* records = m_allocators[i]->GetRecords();
            if (records)
            {
                records->SetMode(mode);
            }
        }
    }

    void AllocatorManager::SetTrackingForAllocator(AZStd::string_view allocatorName, AZ::Debug::AllocationRecords::Mode recordMode)
    {
        AZStd::lock_guard<AZStd::mutex> lock(m_allocatorListMutex);
        auto FindAllocatorTrackingConfigByName = [&allocatorName](const AllocatorTrackingConfig& trackingConfig)
        {
            // performs a case-insensitive compare of the allocator name against the tracking config value
            return AZ::StringFunc::Equal(trackingConfig.m_allocatorName, allocatorName);
        };

        // Store a pointer to the allocator tracking config that was either found
        // or inserted into the allocator tracking config container
        AllocatorTrackingConfig* trackingConfig{};

        auto foundTrackingConfigIt =
            AZStd::find_if(m_allocatorTrackingConfigs.begin(), m_allocatorTrackingConfigs.end(), FindAllocatorTrackingConfigByName);
        if (foundTrackingConfigIt != m_allocatorTrackingConfigs.end())
        {
            // Update the recording mode
            foundTrackingConfigIt->m_recordMode = recordMode;
            trackingConfig = AZStd::to_address(foundTrackingConfigIt);
        }
        else
        {
            trackingConfig = &m_allocatorTrackingConfigs.emplace_back(AllocatorTrackingConfig{ AllocatorName(allocatorName), recordMode });
        }

        // If the allocator is already registered with the Allocator Manager update the allocation record tracking
        // to either start or stop tracking
        auto FindAllocatorByName = [&allocatorName](IAllocator* allocator)
        {
            // performs a case-insensitive compare of the allocator name against the tracking config value
            return AZ::StringFunc::Equal(allocator->GetName(), allocatorName);
        };

        auto allocatorsEnd = m_allocators + m_numAllocators;
        if (auto foundAllocatorIt = AZStd::find_if(m_allocators, allocatorsEnd, FindAllocatorByName);
            foundAllocatorIt != allocatorsEnd)
        {
            ConfigureTrackingForAllocator(*foundAllocatorIt, *trackingConfig);
        }
    }

    bool AllocatorManager::RemoveTrackingForAllocator(AZStd::string_view allocatorName)
    {
        auto FindAllocatorByName = [&allocatorName](const AllocatorTrackingConfig& trackingConfig)
        {
            // performs a case-insensitive compare of the allocator name against the tracking config value
            return AZ::StringFunc::Equal(trackingConfig.m_allocatorName, allocatorName);
        };
        return AZStd::erase_if(m_allocatorTrackingConfigs, FindAllocatorByName) != 0;
    }

    void AllocatorManager::EnterProfilingMode()
    {
        AZStd::lock_guard<AZStd::mutex> lock(m_allocatorListMutex);
        for (int i = 0; i < m_numAllocators; ++i)
        {
            m_allocators[i]->SetProfilingActive(true);
        }
    }

    void AllocatorManager::ExitProfilingMode()
    {
        AZStd::lock_guard<AZStd::mutex> lock(m_allocatorListMutex);
        for (int i = 0; i < m_numAllocators; ++i)
        {
            m_allocators[i]->SetProfilingActive(false);
        }
    }

    void AllocatorManager::DumpAllocators()
    {
        static constexpr const char* TAG = "mem";

        AZStd::lock_guard<AZStd::mutex> lock(m_allocatorListMutex);
        size_t totalUsedBytes = 0;
        size_t totalReservedBytes = 0;
        size_t totalConsumedBytes = 0;

        memset(m_dumpInfo, 0, sizeof(m_dumpInfo));

        AZ_Printf(TAG, "%d allocators active\n", m_numAllocators);
        AZ_Printf(TAG, "Index,Name,Used kb,Reserved kb,Consumed kb\n");

        for (int i = 0; i < m_numAllocators; i++)
        {
            IAllocator* allocator = GetAllocator(i);
            const char* name = allocator->GetName();
            size_t usedBytes = allocator->NumAllocatedBytes();
            size_t reservedBytes = allocator->Capacity();
            size_t consumedBytes = reservedBytes;

            totalUsedBytes += usedBytes;
            totalReservedBytes += reservedBytes;
            totalConsumedBytes += consumedBytes;
            m_dumpInfo[i].m_name = name;
            m_dumpInfo[i].m_used = usedBytes;
            m_dumpInfo[i].m_reserved = reservedBytes;
            m_dumpInfo[i].m_consumed = consumedBytes;
            AZ_Printf(TAG, "%d,%s,%.2f,%.2f,%.2f\n", i, name, usedBytes / 1024.0f, reservedBytes / 1024.0f, consumedBytes / 1024.0f);
        }

        AZ_Printf(TAG, "-,Totals,%.2f,%.2f,%.2f\n", totalUsedBytes / 1024.0f, totalReservedBytes / 1024.0f, totalConsumedBytes / 1024.0f);
    }
    void AllocatorManager::GetAllocatorStats(size_t& allocatedBytes, size_t& capacityBytes, AZStd::vector<AllocatorStats>* outStats)
    {
        allocatedBytes = 0;
        capacityBytes = 0;

        AZStd::lock_guard<AZStd::mutex> lock(m_allocatorListMutex);
        const int allocatorCount = GetNumAllocators();

        // Build a mapping of original allocator sources to their allocators
        for (int i = 0; i < allocatorCount; ++i)
        {
            IAllocator* allocator = GetAllocator(i);
            allocatedBytes += allocator->NumAllocatedBytes();
            capacityBytes += allocator->Capacity();

            if (outStats)
            {
                outStats->emplace(outStats->end(), allocator->GetName(), allocator->NumAllocatedBytes(), allocator->Capacity());
            }
        }
    }

    //=========================================================================
    // MemoryBreak
    // [2/24/2011]
    //=========================================================================
    AllocatorManager::MemoryBreak::MemoryBreak()
    {
        addressStart = nullptr;
        addressEnd = nullptr;
        byteSize = 0;
        alignment = static_cast<size_t>(0xffffffff);
        name = nullptr;

        fileName = nullptr;
        lineNum = -1;
    }

    //=========================================================================
    // SetMemoryBreak
    // [2/24/2011]
    //=========================================================================
    void AllocatorManager::SetMemoryBreak(int slot, const MemoryBreak& mb)
    {
        AZ_Assert(slot < MaxNumMemoryBreaks, "Invalid slot index");
        m_activeBreaks |= 1 << slot;
        m_memoryBreak[slot] = mb;
    }

    //=========================================================================
    // ResetMemoryBreak
    // [2/24/2011]
    //=========================================================================
    void AllocatorManager::ResetMemoryBreak(int slot)
    {
        if (slot == -1)
        {
            m_activeBreaks = 0;
        }
        else
        {
            AZ_Assert(slot < MaxNumMemoryBreaks, "Invalid slot index");
            m_activeBreaks &= ~(1 << slot);
        }
    }

    //=========================================================================
    // DebugBreak
    // [2/24/2011]
    //=========================================================================
    void AllocatorManager::DebugBreak(void* address, const Debug::AllocationInfo& info)
    {
        (void)address;
        (void)info;
        if (m_activeBreaks)
        {
            void* addressEnd = (char*)address + info.m_byteSize;
            (void)addressEnd;
            for (int i = 0; i < MaxNumMemoryBreaks; ++i)
            {
                if ((m_activeBreaks & (1 << i)) != 0)
                {
                    // check address range
                    AZ_Assert(
                        !((address <= m_memoryBreak[i].addressStart && m_memoryBreak[i].addressStart < addressEnd) ||
                          (address < m_memoryBreak[i].addressEnd && m_memoryBreak[i].addressEnd <= addressEnd) ||
                          (address >= m_memoryBreak[i].addressStart && addressEnd <= m_memoryBreak[i].addressEnd)),
                        "User triggered breakpoint - address overlap [%p,%p] with [%p,%p]",
                        address,
                        addressEnd,
                        m_memoryBreak[i].addressStart,
                        m_memoryBreak[i].addressEnd);

                    AZ_Assert(
                        !(m_memoryBreak[i].addressStart <= address && address < m_memoryBreak[i].addressEnd),
                        "User triggered breakpoint - address overlap [%p,%p] with [%p,%p]",
                        address,
                        addressEnd,
                        m_memoryBreak[i].addressStart,
                        m_memoryBreak[i].addressEnd);
                    AZ_Assert(
                        !(m_memoryBreak[i].addressStart < addressEnd && addressEnd <= m_memoryBreak[i].addressEnd),
                        "User triggered breakpoint - address overlap [%p,%p] with [%p,%p]",
                        address,
                        addressEnd,
                        m_memoryBreak[i].addressStart,
                        m_memoryBreak[i].addressEnd);

                    AZ_Assert(
                        !(m_memoryBreak[i].alignment == info.m_alignment), "User triggered breakpoint - alignment (%d)", info.m_alignment);
                    AZ_Assert(
                        !(m_memoryBreak[i].byteSize == info.m_byteSize),
                        "User triggered breakpoint - allocation size (%d)",
                        info.m_byteSize);
                    AZ_Assert(
                        !(info.m_name != nullptr && m_memoryBreak[i].name != nullptr && strcmp(m_memoryBreak[i].name, info.m_name) == 0),
                        "User triggered breakpoint - name \"%s\"",
                        info.m_name);
                    if (m_memoryBreak[i].lineNum != 0)
                    {
                        AZ_Assert(
                            !(info.m_fileName != nullptr && m_memoryBreak[i].fileName != nullptr &&
                              strcmp(m_memoryBreak[i].fileName, info.m_fileName) == 0 && m_memoryBreak[i].lineNum == info.m_lineNum),
                            "User triggered breakpoint - file/line number : %s(%d)",
                            info.m_fileName,
                            info.m_lineNum);
                    }
                    else
                    {
                        AZ_Assert(
                            !(info.m_fileName != nullptr && m_memoryBreak[i].fileName != nullptr &&
                              strcmp(m_memoryBreak[i].fileName, info.m_fileName) == 0),
                            "User triggered breakpoint - file name \"%s\"",
                            info.m_fileName);
                    }
                }
            }
        }
    }

} // namespace AZ
