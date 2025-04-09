/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Console/IConsole.h>
#include <AzCore/Date/DateFormat.h>
#include <AzCore/Debug/StackTracer.h>
#include <AzCore/IO/GenericStreams.h>
#include <AzCore/Math/Crc.h>
#include <AzCore/Memory/AllocationRecords.h>
#include <AzCore/Memory/AllocatorManager.h>
#include <AzCore/Memory/ChildAllocatorSchema.h>
#include <AzCore/Memory/Memory.h>
#include <AzCore/Memory/IAllocator.h>
#include <AzCore/Memory/OSAllocator.h>
#include <AzCore/Memory/SimpleSchemaAllocator.h>

#include <AzCore/Platform.h>

#include <AzCore/std/parallel/lock.h>
#include <AzCore/std/containers/array.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/Utils/Utils.h>

namespace AZ
{
    static void sys_DumpAllocators([[maybe_unused]] const AZ::ConsoleCommandContainer& arguments)
    {
        AllocatorManager::Instance().DumpAllocators();
    }
    AZ_CONSOLEFREEFUNC(sys_DumpAllocators, AZ::ConsoleFunctorFlags::Null, "Print memory allocator statistics.");

    // Provides a range of allocations to dump. The min value is inclusive and the max value is exclusive
    // Therefore the range is [min, max)
    struct AllocationDumpRange
    {
        size_t m_min = 0;
        size_t m_max = AZStd::numeric_limits<size_t>::max();
    };

    // Add console command for outputting the Allocation records for all or a specified set of allocators with the provided names
    static void DumpAllocationsForAllocatorHelper(
        AZStd::span<AZStd::string_view const> allocatorNameArguments,
        AZ::IO::GenericStream& printStream,
        const AllocationDumpRange& allocationDumpRange = {})
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
                // If no allocator name arguments to the Console command have been supplied
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
                    return AZ::StringFunc::Equal(searchName, allocator->GetName());
                };
                if (AZStd::any_of(allocatorNameArguments.begin(), allocatorNameArguments.end(), IsAllocatorInNameSet))
                {
                    allocatorsToDump.emplace_back(allocator);
                }
            }
        }

        // Note that AllocationString and MemoryTag may be unused in release builds.
        using AllocationString [[maybe_unused]] = AZStd::fixed_string<1024>;
        [[maybe_unused]] static constexpr const char* MemoryTag = "mem";

        size_t allocationCount{};
        size_t totalAllocations{};

        constexpr bool includeAllocationLineAndCallstack = true;
        constexpr bool includeAllocatorNameAndSourceName = true;
        auto PrintAllocations =
            [&printStream,
             includeAllocationLineAndCallstack = includeAllocationLineAndCallstack,
             includeAllocatorNameAndSourceName = includeAllocatorNameAndSourceName,
             &allocationDumpRange,
             &allocationCount,
             &totalAllocations](void* address, const Debug::AllocationInfo& info, unsigned int numStackLevels, size_t numRecords)
        {
            totalAllocations = numRecords;
            AllocationString writeString;

            // Only dump allocations in specified index range
            if (allocationCount >= allocationDumpRange.m_min && allocationCount < allocationDumpRange.m_max)
            {
                if (includeAllocatorNameAndSourceName && info.m_name)
                {
                    writeString = AllocationString::format(
                        R"(Allocation Name: "%s" Addr: 0%p Size: %zu Alignment: %u)"
                        "\n",
                        info.m_name,
                        address,
                        info.m_byteSize,
                        info.m_alignment);
                    printStream.Write(writeString.size(), writeString.data());
                }
                else
                {
                    writeString = AllocationString::format(
                        "Allocation Addr: 0%p Size: %zu Alignment: %u\n", address, info.m_byteSize, info.m_alignment);
                    printStream.Write(writeString.size(), writeString.c_str());
                }

                if (includeAllocationLineAndCallstack)
                {
                    // If there is no stack frame records, output the location where the allocation took place
                    if (!info.m_stackFrames)
                    {
                        writeString = AllocationString::format(
                            R"( "%s" (%d))"
                            "\n",
                            info.m_fileName,
                            info.m_lineNum);
                        printStream.Write(writeString.size(), writeString.c_str());
                    }
                    else
                    {
                        constexpr unsigned int MaxStackFramesToDecode = 30;
                        Debug::SymbolStorage::StackLine decodedStackLines[MaxStackFramesToDecode];

                        for (size_t currentStackFrame{}; numStackLevels > 0;)
                        {
                            // Decode up to lower of MaxStackFramesToDecode and the remaining number of stack frames per loop iteration
                            unsigned int stackFramesToDecode =
                                AZStd::GetMin(MaxStackFramesToDecode, static_cast<unsigned int>(numStackLevels));
                            Debug::SymbolStorage::DecodeFrames(
                                info.m_stackFrames + currentStackFrame, stackFramesToDecode, decodedStackLines);
                            for (unsigned int i = 0; i < stackFramesToDecode; ++i)
                            {
                                if (info.m_stackFrames[currentStackFrame + i].IsValid())
                                {
                                    writeString = ' ';
                                    writeString += decodedStackLines[i];
                                    writeString += "\n";
                                    printStream.Write(writeString.size(), writeString.c_str());
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

        // GetAllocationCount runs EnumerateAllocations for the only the first allocation record
        // in order to get the current allocation count at the time of the call
        size_t estimateAllocationCount{};
        auto GetAllocationCount = [&estimateAllocationCount](void*, const Debug::AllocationInfo&, unsigned int, size_t numRecords)
        {
            estimateAllocationCount = numRecords;
            return false;
        };

        // Stores an estimate of how many allocation records can be output per second
        // This is based on empirical data of dumping the SystemAllocator in the Editor
        // About 10000 records can be printed per second
        constexpr size_t AllocationRecordsPerSecondEstimate = 10000;

        // Iterate over each allocator to dump and print their allocation records
        for (AZ::IAllocator* allocator : allocatorsToDump)
        {
            AZ::Debug::AllocationRecords* records = allocator->GetRecords();

            estimateAllocationCount = 0;
            // Get the allocation count at the time of the first EnumerateAllocations call
            // NOTE: This is only an estimation of the count as the number of allocations
            // can change between this call and the next call to EnumerationAllocations to print the records
            records->EnumerateAllocations(GetAllocationCount);

            using SecondsAsDouble = AZStd::chrono::duration<double>;
            auto allocationRecordsSecondsEstimate = AZStd::chrono::ceil<AZStd::chrono::seconds>(SecondsAsDouble(estimateAllocationCount / double(AllocationRecordsPerSecondEstimate)));

            AllocationString printString = AllocationString::format(
                R"(Printing allocation records for allocator %s. Estimated time to print all records is %lld seconds)"
                "\n",
                allocator->GetName(),
                static_cast<AZ::s64>(allocationRecordsSecondsEstimate.count()));
            printStream.Write(printString.size(), printString.c_str());
            // Also write the print string above to the Trace system
            AZ::Debug::Trace::Instance().Output(MemoryTag, printString.c_str());

            // Reset allocationCount and totalAllocationsCount to zero
            // The PrintAllocations lambda is not called in the case where there are no recorded allocations
            allocationCount = 0;
            totalAllocations = 0;
            auto startTime = AZStd::chrono::steady_clock::now();
            records->EnumerateAllocations(PrintAllocations);
            auto endTime = AZStd::chrono::steady_clock::now();
            auto durationInMilliseconds = AZStd::chrono::duration_cast<AZStd::chrono::milliseconds>(endTime - startTime);
            auto durationInSeconds = AZStd::chrono::ceil<AZStd::chrono::seconds>(durationInMilliseconds);

            AllocationString allocationsPerSecond = u8"\u221e";
            if (durationInSeconds.count() != 0)
            {
                // Use a double for the seconds duration in order to allow decimal values
                AZStd::to_string(allocationsPerSecond, static_cast<AZ::s64>(totalAllocations / durationInSeconds.count()));
            }

            printString = AllocationString::format(
                R"(Printed %zu allocations in %lld seconds for allocator "%s" (%s records per seconds))"
                "\n",
                totalAllocations,
                static_cast<AZ::s64>(durationInSeconds.count()),
                allocator->GetName(),
                allocationsPerSecond.c_str());
            printStream.Write(printString.size(), printString.c_str());
            // Also write the amount of time taken to the console window
            AZ::Debug::Trace::Instance().Output(MemoryTag, printString.c_str());
        }
    }

    static void DumpAllocationsForAllocatorToStdout(const AZ::ConsoleCommandContainer& arguments)
    {
        // Dump allocations to stdout by default
        AZ::IO::SystemFileStream printStream(AZ::IO::SystemFile::GetStdout());
        DumpAllocationsForAllocatorHelper(arguments, printStream);
    }
    AZ_CONSOLEFREEFUNC("sys_DumpAllocationRecordsToStdout", DumpAllocationsForAllocatorToStdout, AZ::ConsoleFunctorFlags::Null,
        "Print ALL individual allocations for the specified allocator to stdout.\n"
        " If no allocator is specified, then all allocations are dumped\n"
        "NOTE: This can be slow depending on the number of allocations\n"
        R"(For better control of which allocations get printed, use the "sys_DumpAllocationRecordInRange" command)" "\n"
        "usage: sys_DumpAllocationRecordsToStdout [<allocator name...>]\n"
        "Ex. `sys_DumpAllocationRecordsToStdout SystemAllocator`");

    static void DumpAllocationsForAllocatorToFile(const AZ::ConsoleCommandContainer& arguments)
    {
        // Note that AllocationString and MemoryTag may be unused in release builds.
        using AllocationString [[maybe_unused]] = AZStd::fixed_string<1024>;
        [[maybe_unused]] static constexpr const char* MemoryTag = "mem";

        constexpr size_t DumpToFileMinArgumentCount = 1;

        if (arguments.size() < DumpToFileMinArgumentCount)
        {
            AZ_Error(
                MemoryTag,
                false,
                AllocationString(
                    R"("sys_DumpAllocationRecordsToFile" command requires the first argument to specify the file path where the allocation records will be written.)"
                    "\n")
                    .c_str());
            return;
        }

        // Get the file path from the first argument
        AZStd::string_view filePath = arguments[0];
        // Create a subrange to the indices that would contain the allocator name arguments
        AZStd::span<AZStd::string_view const> allocatorNameArguments{ AZStd::next(arguments.begin(), DumpToFileMinArgumentCount), arguments.end() };

        // Open the file stream
        // if the file path is '-', then a stream to stdout is opened
        constexpr auto openMode = AZ::IO::OpenMode::ModeCreatePath | AZ::IO::OpenMode::ModeWrite;

        AZ::IO::SystemFileStream printStream;
        if (filePath != "-")
        {
            printStream = AZ::IO::SystemFileStream(AZ::IO::FixedMaxPathString(filePath).c_str(), openMode);
        }
        else
        {
            printStream = AZ::IO::SystemFileStream(AZ::IO::SystemFile::GetStdout());
        }

        if (!printStream.IsOpen())
        {
            AZ_Error(
                MemoryTag,
                false,
                AllocationString::format(
                    R"("sys_DumpAllocationRecordsToFile" command could not open file path of "%s".)"
                    "\n",
                    printStream.GetFilename())
                    .c_str());
            return;
        }
        DumpAllocationsForAllocatorHelper(arguments, printStream);
    }
    AZ_CONSOLEFREEFUNC("sys_DumpAllocationRecordsToFile", DumpAllocationsForAllocatorToFile, AZ::ConsoleFunctorFlags::Null,
        "Write ALL individual allocations for the specified allocator to the user specified file path.\n"
        "The path is relative to the current working directory of the running application.\n"
        "If no allocator is specified, then all allocations are dumped\n"
        "NOTE: This can be slow depending on the number of allocations\n"
        R"(For better control of which allocations get printed, use the "sys_DumpAllocationRecordInRange" command)" "\n"
        "usage: sys_DumpAllocationRecordsToFile <file-path> [<allocator name...>]\n"
        "Ex. `sys_DumpAllocationRecordsToFile /home/user/allocation_records.log SystemAllocator`");

    static void DumpAllocationsForAllocatorToDevWriteStorage(const AZ::ConsoleCommandContainer& arguments)
    {
        // Note that AllocationString and MemoryTag may be unused in release builds.
        using AllocationString [[maybe_unused]] = AZStd::fixed_string<1024>;
        [[maybe_unused]] static constexpr const char* MemoryTag = "mem";

        // Dump allocation records to <dev-write-storage>/allocation_records/records.<iso8601-timestamp>.<process-id>.log
        // Use a ISO8601 timestamp + the process ID to provide uniqueness to the metrics json files
        AZ::Date::Iso8601TimestampString utcTimestampString;
        AZ::Date::GetFilenameCompatibleFormatNow(utcTimestampString);
        // append process id
        AZStd::fixed_string<32> processIdString;
        AZStd::to_string(processIdString, AZ::Platform::GetCurrentProcessId());

        // Append the relative file name portion to the <project-root>/user directory
        const auto filePath = AZ::IO::FixedMaxPath{ AZ::Utils::GetDevWriteStoragePath() } / "allocation_records" /
            AZ::IO::FixedMaxPathString::format("records.%s.%s.log", utcTimestampString.c_str(), processIdString.c_str());
        constexpr auto openMode = AZ::IO::OpenMode::ModeCreatePath | AZ::IO::OpenMode::ModeWrite;
        AZ::IO::SystemFileStream printStream(filePath.c_str(), openMode);
        if (!printStream.IsOpen())
        {
            AZ_Error(
                MemoryTag,
                false,
                AllocationString::format(
                    R"("sys_DumpAllocationRecordsToFile" command could not open file path of "%s".)"
                    "\n",
                    printStream.GetFilename())
                .c_str());
            return;
        }
        DumpAllocationsForAllocatorHelper(arguments, printStream);
    }
    AZ_CONSOLEFREEFUNC("sys_DumpAllocationRecordsToDevWriteStorage", DumpAllocationsForAllocatorToDevWriteStorage, AZ::ConsoleFunctorFlags::Null,
        "Write ALL individual allocations for the specified allocator to <dev-write-storage>/allocation_records/records.<iso8601-timestamp>.<process-id>.log.\n"
        "On host plaforms such as Windows/Linux/MacOS, <dev-write-storage> is equivalent to <project-root>/user directory.\n"
        "On non-host platforms such as Android/iOS this folder is a writable directory based on those operating systems' Data container/storage APIs\n"
        "If no allocator is specified, then all allocations are dumped\n"
        "NOTE: This can be slow depending on the number of allocations\n"
        R"(For better control of which allocations get printed, use the "sys_DumpAllocationRecordInRange" command)" "\n"
        "usage: sys_DumpAllocationRecordsToDevWriteStorage [<allocator name...>]\n"
        "Ex. `sys_DumpAllocationRecordsToDevWriteStorage SystemAllocator`");

    static void DumpAllocationsForAllocatorInRange(const AZ::ConsoleCommandContainer& arguments)
    {
        // Note that AllocationString and MemoryTag may be unused in release builds.
        using AllocationString [[maybe_unused]] = AZStd::fixed_string<1024>;
        [[maybe_unused]] static constexpr const char* MemoryTag = "mem";

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
                    "\n", AZ_STRING_ARG(arguments[0]))
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
                    "\n", AZ_STRING_ARG(arguments[1]))
                .c_str());
            return;
        }

        // Create a subspan of the list of arguments where the allocator name starts from
        // The first two arguments represent the range of allocations to print
        AZStd::span<AZStd::string_view const> allocatorNameArguments{ AZStd::next(arguments.begin(), rangeArgumentCount), arguments.end() };

        // Dump allocations to stdout by default
        AZ::IO::SystemFileStream printStream(AZ::IO::SystemFile::GetStdout());
        DumpAllocationsForAllocatorHelper(allocatorNameArguments, printStream, dumpRange);
    }

    AZ_CONSOLEFREEFUNC(
        "sys_DumpAllocationRecordsInRange",
        DumpAllocationsForAllocatorInRange,
        AZ::ConsoleFunctorFlags::Null,
        "Print allocations records in the specified index range of min to max for any allocations.\n"
        " If no allocator is specified, then all registered allocator allocations records are dumped in the specified range\n"
        "usage: sys_DumpAllocationsRecords <min-inclusive-index> <max-exclusive-index> [<allocator name...>]\n"
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

        // If there is an allocator tracking config entry for the allocator store
        // use its recording mode option to turn on allocation tracking
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
                // The ProfileAllocation function requires that the allocator has profiling turned on to record allocations
                // Therefore profiling is turned on if the Allocation Record mode is set to record any kind of records
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
        AZStd::lock_guard<AZStd::mutex> lock(m_allocatorListMutex);
        size_t totalUsedBytes = 0;
        size_t totalReservedBytes = 0;
        size_t totalConsumedBytes = 0;

        memset(m_dumpInfo, 0, sizeof(m_dumpInfo));

        AZ_Printf(AZ::Debug::NoWindow, "Index,Name,Used KiB,Reserved KiB,Consumed KiB,Parent Allocator\n");

        for (int i = 0; i < m_numAllocators; i++)
        {
            IAllocator* allocator = GetAllocator(i);
            const char* name = allocator->GetName();
            size_t usedBytes = allocator->NumAllocatedBytes();
            size_t reservedBytes = allocator->Capacity();
            size_t consumedBytes = reservedBytes;
            const char* parentName = "";
            if (auto childAllocatorSchema = azrtti_cast<AZ::ChildAllocatorSchemaBase*>(allocator);
                childAllocatorSchema != nullptr)
            {
                auto parentAllocator = childAllocatorSchema->GetParentAllocator();
                parentName = parentAllocator != nullptr ? parentAllocator->GetName() : "";
            }

            totalUsedBytes += usedBytes;
            totalReservedBytes += reservedBytes;
            totalConsumedBytes += consumedBytes;
            m_dumpInfo[i].m_name = name;
            m_dumpInfo[i].m_used = usedBytes;
            m_dumpInfo[i].m_reserved = reservedBytes;
            m_dumpInfo[i].m_consumed = consumedBytes;
            AZ_Printf(
                AZ::Debug::NoWindow,
                "%d,%s,%.2f,%.2f,%.2f,%s\n",
                i,
                name,
                usedBytes / 1024.0f,
                reservedBytes / 1024.0f,
                consumedBytes / 1024.0f,
                parentName);
        }

        AZ_Printf(AZ::Debug::NoWindow, "-,Totals,%.2f,%.2f,%.2f,\n", totalUsedBytes / 1024.0f, totalReservedBytes / 1024.0f, totalConsumedBytes / 1024.0f);
        AZ_Printf(AZ::Debug::NoWindow, "%d allocators active\n", m_numAllocators);
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
                const char* parentName = "";
                if (auto childAllocatorSchema = azrtti_cast<AZ::ChildAllocatorSchemaBase*>(allocator); childAllocatorSchema != nullptr)
                {
                    auto parentAllocator = childAllocatorSchema->GetParentAllocator();
                    parentName = parentAllocator != nullptr ? parentAllocator->GetName() : "";
                }
                outStats->emplace(outStats->end(), allocator->GetName(), parentName, allocator->NumAllocatedBytes(), allocator->Capacity());
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
