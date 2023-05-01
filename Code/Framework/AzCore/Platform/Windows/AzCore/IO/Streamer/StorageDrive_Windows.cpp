/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <climits>

#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/Debug/Profiler.h>
#include <AzCore/IO/Streamer/FileRequest.h>
#include <AzCore/IO/Streamer/StreamerContext.h>
#include <AzCore/IO/Streamer/StorageDrive_Windows.h>
#include <AzCore/std/typetraits/decay.h>
#include <AzCore/std/typetraits/is_unsigned.h>
#include <AzCore/StringFunc/StringFunc.h>
#include <AzCore/std/string/conversions.h>

namespace AZ::IO
{
#if AZ_STREAMER_ADD_EXTRA_PROFILING_INFO
    static constexpr char FileSwitchesName[] = "File switches";
    static constexpr char SeeksName[] = "Seeks";
    static constexpr char DirectReadsName[] = "Direct reads (no internal alloc)";
#endif // AZ_STREAMER_ADD_EXTRA_PROFILING_INFO

    const AZStd::chrono::microseconds StorageDriveWin::s_averageSeekTime =
        AZStd::chrono::milliseconds(9) + // Common average seek time for desktop hdd drives.
        AZStd::chrono::milliseconds(3); // Rotational latency for a 7200RPM disk

    //
    // ConstructionOptions
    //

    StorageDriveWin::ConstructionOptions::ConstructionOptions()
        : m_hasSeekPenalty(true)
        , m_enableUnbufferedReads(true)
        , m_enableSharing(false)
        , m_minimalReporting(false)
    {}

    //
    // FileReadInformation
    //

    void StorageDriveWin::FileReadInformation::AllocateAlignedBuffer(size_t size, size_t sectorSize)
    {
        AZ_Assert(m_sectorAlignedOutput == nullptr, "Assign a sector aligned buffer when one is already assigned.");
        m_sectorAlignedOutput =  azmalloc(size, sectorSize, AZ::SystemAllocator);
    }

    void StorageDriveWin::FileReadInformation::Clear()
    {
        if (m_sectorAlignedOutput)
        {
            azfree(m_sectorAlignedOutput, AZ::SystemAllocator);
        }
        *this = FileReadInformation{};
    }

    //
    // StorageDriveWin
    //
    StorageDriveWin::StorageDriveWin(const AZStd::vector<AZStd::string_view>& drivePaths, u32 maxFileHandles, u32 maxMetaDataCacheEntries,
        size_t physicalSectorSize, size_t logicalSectorSize, u32 ioChannelCount, s32 overCommit, ConstructionOptions options)
        : m_maxFileHandles(maxFileHandles)
        , m_physicalSectorSize(physicalSectorSize)
        , m_logicalSectorSize(logicalSectorSize)
        , m_ioChannelCount(ioChannelCount)
        , m_overCommit(overCommit)
        , m_constructionOptions(options)
    {
        AZ_Assert(!drivePaths.empty(), "StorageDrive_win requires at least one drive path to work.");

        // Get drive paths
        m_drivePaths.reserve(drivePaths.size());
        for (AZStd::string_view drivePath : drivePaths)
        {
            AZStd::string path(drivePath);
            // Erase the slash as it's one less character to compare and avoids issues with forward and
            // backward slashes.
            const char lastChar = path.back();
            if (lastChar == AZ_CORRECT_FILESYSTEM_SEPARATOR || lastChar == AZ_WRONG_FILESYSTEM_SEPARATOR)
            {
                path.pop_back();
            }
            m_drivePaths.push_back(AZStd::move(path));
        }

        // Create name for statistics. The name will include all drive mappings on this physical device
        // for instance "Storage drive (F,G,H)". Colons and slashes are removed.
        m_name = "Storage drive (";
        m_name += m_drivePaths[0].substr(0, m_drivePaths[0].length()-1);
        for (size_t i = 1; i < m_drivePaths.size(); ++i)
        {
            m_name += ',';
            // Add the path, but don't include the slash as that might cause formating issues with some profilers.
            m_name += m_drivePaths[i].substr(0, m_drivePaths[i].length()-1);
        }
        m_name += ')';
        if (!m_constructionOptions.m_minimalReporting)
        {
            AZ_Printf("Streamer", "%s created.\n", m_name.c_str());
        }

        if (m_physicalSectorSize == 0)
        {
            m_physicalSectorSize = 16_kib;
            AZ_Error("StorageDriveWin", false,
                "Received physical sector size of 0 for %s. Picking a sector size of %zu instead.\n", m_name.c_str(), m_physicalSectorSize);
        }
        if (m_logicalSectorSize == 0)
        {
            m_logicalSectorSize = 4_kib;
            AZ_Error("StorageDriveWin", false,
                "Received logical sector size of 0 for %s. Picking a sector size of %zu instead.\n", m_name.c_str(), m_logicalSectorSize);
        }
        AZ_Error("StorageDriveWin", IStreamerTypes::IsPowerOf2(m_physicalSectorSize) && IStreamerTypes::IsPowerOf2(m_logicalSectorSize),
            "StorageDriveWin requires power-of-2 sector sizes. Received physical: %zu and logical: %zu",
            m_physicalSectorSize, m_logicalSectorSize);

        // Cap the IO channels to the maximum
        if (m_ioChannelCount == 0)
        {
            m_ioChannelCount = aznumeric_cast<u32>(AZ::Platform::StreamerContextThreadSync::MaxIoEvents);
            AZ_Warning("StorageDriveWin", false,
                "Received io channel count of 0 for %s. Picking a count of %zu instead.\n",
                m_name.c_str(), AZ::Platform::StreamerContextThreadSync::MaxIoEvents);
        }
        else
        {
            m_ioChannelCount = AZ::GetMin(m_ioChannelCount, aznumeric_cast<u32>(AZ::Platform::StreamerContextThreadSync::MaxIoEvents));
        }
        // Make sure that the overCommit isn't so small that no slots are ever reported.
        if (aznumeric_cast<s32>(m_ioChannelCount) + m_overCommit <= 0)
        {
            AZ_Error("StorageDriveWin", false,
                "Received overcommit (%i) for %s that subtracts more than the number of IO channels (%u). Setting combined count to 1.\n",
                m_overCommit, m_name.c_str(), m_ioChannelCount);
            m_overCommit = 1 - aznumeric_cast<s32>(m_ioChannelCount);
        }

        // Add initial dummy values to the stats to avoid division by zero later on and avoid needing branches.
        m_readSizeAverage.PushEntry(1);
        m_readTimeAverage.PushEntry(AZStd::chrono::microseconds(1));

        AZ_Assert(IStreamerTypes::IsPowerOf2(maxMetaDataCacheEntries),
            "StorageDriveWin requires a power-of-2 for maxMetaDataCacheEntries. Received %zu", maxMetaDataCacheEntries);
        m_metaDataCache_paths.resize(maxMetaDataCacheEntries);
        m_metaDataCache_fileSize.resize(maxMetaDataCacheEntries);
    }

    StorageDriveWin::~StorageDriveWin()
    {
        for (HANDLE file : m_fileCache_handles)
        {
            if (file != INVALID_HANDLE_VALUE)
            {
                ::CloseHandle(file);
            }
        }
        if (!m_constructionOptions.m_minimalReporting)
        {
            AZ_Printf("Streamer", "%s destroyed.\n", m_name.c_str());
        }
    }

    void StorageDriveWin::PrepareRequest(FileRequest* request)
    {
        AZ_PROFILE_FUNCTION(AzCore);
        AZ_Assert(request, "PrepareRequest was provided a null request.");

        if (AZStd::holds_alternative<Requests::ReadRequestData>(request->GetCommand()))
        {
            auto& readRequest = AZStd::get<Requests::ReadRequestData>(request->GetCommand());
            if (IsServicedByThisDrive(readRequest.m_path.GetAbsolutePath()))
            {
                FileRequest* read = m_context->GetNewInternalRequest();
                read->CreateRead(request, readRequest.m_output, readRequest.m_outputSize, readRequest.m_path,
                    readRequest.m_offset, readRequest.m_size);
                m_context->PushPreparedRequest(read);
                return;
            }
        }
        StreamStackEntry::PrepareRequest(request);
    }

    void StorageDriveWin::QueueRequest(FileRequest* request)
    {
        AZ_PROFILE_FUNCTION(AzCore);
        AZ_Assert(request, "QueueRequest was provided a null request.");

        AZStd::visit([this, request](auto&& args)
        {
            using Command = AZStd::decay_t<decltype(args)>;
            if constexpr (AZStd::is_same_v<Command, Requests::ReadData>)
            {
                if (IsServicedByThisDrive(args.m_path.GetAbsolutePath()))
                {
                    m_pendingReadRequests.push_back(request);
                    return;
                }
            }
            else if constexpr (AZStd::is_same_v<Command, Requests::FileExistsCheckData> ||
                AZStd::is_same_v<Command, Requests::FileMetaDataRetrievalData>)
            {
                if (IsServicedByThisDrive(args.m_path.GetAbsolutePath()))
                {
                    m_pendingRequests.push_back(request);
                    return;
                }
            }
            else if constexpr (AZStd::is_same_v<Command, Requests::CancelData>)
            {
                if (CancelRequest(request, args.m_target))
                {
                    // Only forward if this isn't part of the request chain, otherwise the storage device should
                    // be the last step as it doesn't forward any (sub)requests.
                    return;
                }
            }
            else if constexpr (AZStd::is_same_v<Command, Requests::FlushData>)
            {
                FlushCache(args.m_path);
            }
            else if constexpr (AZStd::is_same_v<Command, Requests::FlushAllData>)
            {
                FlushEntireCache();
            }
            else if constexpr (AZStd::is_same_v<Command, Requests::ReportData>)
            {
                Report(args);
            }
            StreamStackEntry::QueueRequest(request);
        }, request->GetCommand());
    }

    bool StorageDriveWin::ExecuteRequests()
    {
        bool hasFinalizedReads = FinalizeReads();
        bool hasWorked = false;

        if (!m_pendingReadRequests.empty())
        {
            FileRequest* request = m_pendingReadRequests.front();
            if (ReadRequest(request))
            {
                m_pendingReadRequests.pop_front();
                hasWorked = true;
            }
        }
        else if (!m_pendingRequests.empty())
        {
            FileRequest* request = m_pendingRequests.front();
            hasWorked = AZStd::visit(
                [this, request](auto&& args)
                {
                    using Command = AZStd::decay_t<decltype(args)>;
                    if constexpr (AZStd::is_same_v<Command, Requests::FileExistsCheckData>)
                    {
                        FileExistsRequest(request);
                        m_pendingRequests.pop_front();
                        return true;
                    }
                    else if constexpr (AZStd::is_same_v<Command, Requests::FileMetaDataRetrievalData>)
                    {
                        FileMetaDataRetrievalRequest(request);
                        m_pendingRequests.pop_front();
                        return true;
                    }
                    else
                    {
                        AZ_Assert(false, "A request was added to StorageDriveWin's pending queue that isn't supported.");
                        return false;
                    }
                },
                request->GetCommand());
        }

        return StreamStackEntry::ExecuteRequests() || hasFinalizedReads || hasWorked;
    }

    void StorageDriveWin::UpdateStatus(Status& status) const
    {
        StreamStackEntry::UpdateStatus(status);
        status.m_numAvailableSlots = AZStd::min(status.m_numAvailableSlots, CalculateNumAvailableSlots());
        status.m_isIdle = status.m_isIdle && m_pendingReadRequests.empty() && m_pendingRequests.empty() && (m_activeReads_Count == 0);
    }

    void StorageDriveWin::UpdateCompletionEstimates(AZStd::chrono::steady_clock::time_point now, AZStd::vector<FileRequest*>& internalPending,
        StreamerContext::PreparedQueue::iterator pendingBegin, StreamerContext::PreparedQueue::iterator pendingEnd)
    {
        StreamStackEntry::UpdateCompletionEstimates(now, internalPending, pendingBegin, pendingEnd);

        const RequestPath* activeFile = nullptr;
        if (m_activeCacheSlot != InvalidFileCacheIndex)
        {
            activeFile = &m_fileCache_paths[m_activeCacheSlot];
        }
        u64 activeOffset = m_activeOffset;

        // Determine the time of the first available slot
        AZStd::chrono::steady_clock::time_point earliestSlot = AZStd::chrono::steady_clock::time_point::max();
        for (size_t i = 0; i < m_readSlots_readInfo.size(); ++i)
        {
            if (m_readSlots_active[i])
            {
                FileReadInformation& read = m_readSlots_readInfo[i];
                u64 totalBytesRead = m_readSizeAverage.GetTotal();
                double totalReadTime = aznumeric_caster(m_readTimeAverage.GetTotal().count());
                auto readCommand = AZStd::get_if<Requests::ReadData>(&read.m_request->GetCommand());
                AZ_Assert(readCommand, "Request currently reading doesn't contain a read command.");
                AZStd::chrono::steady_clock::time_point endTime =
                    read.m_startTime + Statistic::TimeValue(aznumeric_cast<u64>((readCommand->m_size * totalReadTime) / totalBytesRead));
                earliestSlot = AZStd::min(earliestSlot, endTime);
                read.m_request->SetEstimatedCompletion(endTime);
            }
        }
        if (earliestSlot != AZStd::chrono::steady_clock::time_point::max())
        {
            now = earliestSlot;
        }

        // Estimate requests in this stack entry.
        for (FileRequest* request : m_pendingReadRequests)
        {
            EstimateCompletionTimeForRequest(request, now, activeFile, activeOffset);
        }
        for (FileRequest* request : m_pendingRequests)
        {
            EstimateCompletionTimeForRequest(request, now, activeFile, activeOffset);
        }

        // Estimate internally pending requests. Because this call will go from the top of the stack to the bottom,
        // but estimation is calculated from the bottom to the top, this list should be processed in reverse order.
        for (auto requestIt = internalPending.rbegin(); requestIt != internalPending.rend(); ++requestIt)
        {
            EstimateCompletionTimeForRequestChecked(*requestIt, now, activeFile, activeOffset);
        }

        // Estimate pending requests that have not been queued yet.
        for (auto requestIt = pendingBegin; requestIt != pendingEnd; ++requestIt)
        {
            EstimateCompletionTimeForRequestChecked(*requestIt, now, activeFile, activeOffset);
        }
    }

    void StorageDriveWin::EstimateCompletionTimeForRequest(FileRequest* request, AZStd::chrono::steady_clock::time_point& startTime,
        const RequestPath*& activeFile, u64& activeOffset) const
    {
        u64 readSize = 0;
        u64 offset = 0;
        const RequestPath* targetFile = nullptr;

        AZStd::visit([&](auto&& args)
        {
            using Command = AZStd::decay_t<decltype(args)>;
            if constexpr (AZStd::is_same_v<Command, Requests::ReadData>)
            {
                targetFile = &args.m_path;
                readSize = args.m_size;
                offset = args.m_offset;
            }
            else if constexpr (AZStd::is_same_v<Command, Requests::CompressedReadData>)
            {
                targetFile = &args.m_compressionInfo.m_archiveFilename;
                readSize = args.m_compressionInfo.m_compressedSize;
                offset = args.m_compressionInfo.m_offset;
            }
            else if constexpr (AZStd::is_same_v<Command, Requests::FileExistsCheckData>)
            {
                readSize = 0;
                AZStd::chrono::microseconds getFileExistsTimeAverage = m_getFileExistsTimeAverage.CalculateAverage();
                startTime += getFileExistsTimeAverage;
            }
            else if constexpr (AZStd::is_same_v<Command, Requests::FileMetaDataRetrievalData>)
            {
                readSize = 0;
                AZStd::chrono::microseconds getFileExistsTimeAverage = m_getFileMetaDataRetrievalTimeAverage.CalculateAverage();
                startTime += getFileExistsTimeAverage;
            }
        }, request->GetCommand());

        if (readSize > 0)
        {
            if (activeFile && activeFile != targetFile)
            {
                if (FindInFileHandleCache(*targetFile) == InvalidFileCacheIndex)
                {
                    AZStd::chrono::microseconds fileOpenCloseTimeAverage = m_fileOpenCloseTimeAverage.CalculateAverage();
                    startTime += fileOpenCloseTimeAverage;
                }
                activeOffset = std::numeric_limits<u64>::max();
            }

            if (activeOffset != offset && m_constructionOptions.m_hasSeekPenalty)
            {
                startTime += s_averageSeekTime;
            }

            u64 totalBytesRead = m_readSizeAverage.GetTotal();
            double totalReadTime = aznumeric_caster(m_readTimeAverage.GetTotal().count());
            startTime += Statistic::TimeValue(aznumeric_cast<u64>((readSize * totalReadTime) / totalBytesRead));
            activeOffset = offset + readSize;
        }
        request->SetEstimatedCompletion(startTime);
    }

    void StorageDriveWin::EstimateCompletionTimeForRequestChecked(FileRequest* request,
        AZStd::chrono::steady_clock::time_point startTime, const RequestPath*& activeFile, u64& activeOffset) const
    {
        AZStd::visit([&, this](auto&& args)
        {
            using Command = AZStd::decay_t<decltype(args)>;
            if constexpr (AZStd::is_same_v<Command, Requests::ReadData> ||
                          AZStd::is_same_v<Command, Requests::FileExistsCheckData>)
            {
                if (IsServicedByThisDrive(args.m_path.GetAbsolutePath()))
                {
                    EstimateCompletionTimeForRequest(request, startTime, activeFile, activeOffset);
                }
            }
            else if constexpr (AZStd::is_same_v<Command, Requests::CompressedReadData>)
            {
                if (IsServicedByThisDrive(args.m_compressionInfo.m_archiveFilename.GetAbsolutePath()))
                {
                    EstimateCompletionTimeForRequest(request, startTime, activeFile, activeOffset);
                }
            }
        }, request->GetCommand());
    }

    s32 StorageDriveWin::CalculateNumAvailableSlots() const
    {
        return (m_overCommit + aznumeric_cast<s32>(m_ioChannelCount)) - aznumeric_cast<s32>(m_pendingReadRequests.size()) -
            aznumeric_cast<s32>(m_pendingRequests.size()) - m_activeReads_Count;
    }

    auto StorageDriveWin::OpenFile(HANDLE& fileHandle, size_t& cacheSlot, FileRequest* request, const Requests::ReadData& data) -> OpenFileResult
    {
        HANDLE file = INVALID_HANDLE_VALUE;

        // If the file is already opened for use, use that file handle and update it's last touched time.
        size_t cacheIndex = FindInFileHandleCache(data.m_path);
        if (cacheIndex != InvalidFileCacheIndex)
        {
            file = m_fileCache_handles[cacheIndex];
            AZ_Assert(file != INVALID_HANDLE_VALUE, "Found the file '%s' in cache, but file handle is invalid.\n",
                data.m_path.GetRelativePath());
        }
        else
        {
            // If the file is not already found in the cache, attempt to claim an available cache entry.
            cacheIndex = FindAvailableFileHandleCacheIndex();
            if (cacheIndex == InvalidFileCacheIndex)
            {
                // No files ready to be evicted.
                return OpenFileResult::CacheFull;
            }

            // Adding explicit scope here for profiling file Open & Close
            {
                AZ_PROFILE_SCOPE(AzCore, "StorageDriveWin::ReadRequest OpenFile %s", m_name.c_str());
                TIMED_AVERAGE_WINDOW_SCOPE(m_fileOpenCloseTimeAverage);

                // All reads are overlapped (asynchronous).
                // Depending on configuration, reads can also be unbuffered.
                // Unbuffered means Windows does not use its own file cache for these files.
                DWORD createFlags = m_constructionOptions.m_enableUnbufferedReads ? (FILE_FLAG_OVERLAPPED | FILE_FLAG_NO_BUFFERING) : FILE_FLAG_OVERLAPPED;
                DWORD shareMode = (m_constructionOptions.m_enableSharing || data.m_sharedRead) ? FILE_SHARE_READ: 0;

                AZStd::wstring filenameW;
                AZStd::to_wstring(filenameW, data.m_path.GetAbsolutePathCStr());
                file = ::CreateFileW(
                    filenameW.c_str(),              // file name
                    FILE_GENERIC_READ,              // desired access
                    shareMode,                      // share mode
                    nullptr,                        // security attributes
                    OPEN_EXISTING,                  // creation disposition
                    createFlags,                    // flags and attributes
                    nullptr);                       // template file

                if (file == INVALID_HANDLE_VALUE)
                {
                    // Failed to open the file, so let the next entry in the stack try.
                    StreamStackEntry::QueueRequest(request);
                    return OpenFileResult::RequestForwarded;
                }

                // Remove any alertable IO completion notifications that could be queued by the IO Manager.
                if (!::SetFileCompletionNotificationModes(file, FILE_SKIP_SET_EVENT_ON_HANDLE | FILE_SKIP_COMPLETION_PORT_ON_SUCCESS))
                {
                    AZ_Warning("StorageDriveWin", false, "Failed to remove alertable IO completion notifications. (Error: %u)\n", ::GetLastError());
                }

                if (m_fileCache_handles[cacheIndex] != INVALID_HANDLE_VALUE)
                {
                    ::CloseHandle(m_fileCache_handles[cacheIndex]);
                }
            }

            // Fill the cache entry with data about the new file.
            m_fileCache_handles[cacheIndex] = file;
            m_fileCache_activeReads[cacheIndex] = 0;
            m_fileCache_paths[cacheIndex] = data.m_path;
        }

        AZ_Assert(file != INVALID_HANDLE_VALUE, "While searching for file '%s' in StorageDeviceWin::OpenFile failed to detect a problem.",
            data.m_path.GetRelativePath());

        // Set the current request and update timestamp, regardless of cache hit or miss.
        m_fileCache_lastTimeUsed[cacheIndex] = AZStd::chrono::steady_clock::now();
        fileHandle = file;
        cacheSlot = cacheIndex;
        return OpenFileResult::FileOpened;
    }

    bool StorageDriveWin::ReadRequest(FileRequest* request)
    {
        AZ_PROFILE_SCOPE(AzCore, "StorageDriveWin::ReadRequest %s", m_name.c_str());

        if (!m_cachesInitialized)
        {
            m_fileCache_lastTimeUsed.resize(m_maxFileHandles, AZStd::chrono::steady_clock::time_point::min());
            m_fileCache_paths.resize(m_maxFileHandles);
            m_fileCache_handles.resize(m_maxFileHandles, INVALID_HANDLE_VALUE);
            m_fileCache_activeReads.resize(m_maxFileHandles, 0);

            m_readSlots_readInfo.resize(m_ioChannelCount);
            m_readSlots_statusInfo.resize(m_ioChannelCount);
            m_readSlots_active.resize(m_ioChannelCount);

            m_cachesInitialized = true;
        }

        if (m_activeReads_Count >= m_ioChannelCount)
        {
            return false;
        }

        size_t readSlot = FindAvailableReadSlot();
        AZ_Assert(readSlot != InvalidReadSlotIndex, "Active read slot count indicates there's a read slot available, but no read slot was found.");

        return ReadRequest(request, readSlot);
    }

    bool StorageDriveWin::ReadRequest(FileRequest* request, size_t readSlot)
    {
        AZ_PROFILE_SCOPE(AzCore, "StorageDriveWin::ReadRequest %s", m_name.c_str());

        if (!m_context->GetStreamerThreadSynchronizer().AreEventHandlesAvailable())
        {
            // There are no more events handles available so delay executing this request until events become available.
            return false;
        }

        auto data = AZStd::get_if<Requests::ReadData>(&request->GetCommand());
        AZ_Assert(data, "Read request in StorageDriveWin doesn't contain read data.");

        HANDLE file = INVALID_HANDLE_VALUE;
        size_t fileCacheSlot = InvalidFileCacheIndex;
        switch (OpenFile(file, fileCacheSlot, request, *data))
        {
        case OpenFileResult::FileOpened:
            break;
        case OpenFileResult::RequestForwarded:
            return true;
        case OpenFileResult::CacheFull:
            return false;
        default:
            AZ_Assert(false, "Unsupported OpenFileRequest returned.");
        }

        DWORD readSize = aznumeric_cast<DWORD>(data->m_size);
        u64 readOffs = data->m_offset;
        void* output = data->m_output;

        FileReadInformation& readInfo = m_readSlots_readInfo[readSlot];
        readInfo.m_request = request;

        if (m_constructionOptions.m_enableUnbufferedReads)
        {
            // Check alignment of the file read information: size, offset, and address.
            // If any are unaligned to the sector sizes, make adjustments and allocate an aligned buffer.
            const bool alignedAddr = IStreamerTypes::IsAlignedTo(data->m_output, aznumeric_caster(m_physicalSectorSize));
            const bool alignedOffs = IStreamerTypes::IsAlignedTo(data->m_offset, aznumeric_caster(m_logicalSectorSize));

            // Adjust the offset if it's misaligned.
            // Align the offset down to next lowest sector.
            // Change the size to compensate.
            //
            // Before:
            // +---------------+---------------+
            // |   XXXXXXXXXXXX|XXXXXXX        |
            // +---------------+---------------+
            //     ^--- offset
            //     <--- size --------->
            //
            // After:
            // +---------------+---------------+
            // |###XXXXXXXXXXXX|XXXXXXX        |
            // +---------------+---------------+
            // ^--- offset
            // <--- size ------------->
            // <--> copyBackOffset
            //
            // Store the size of the adjustment in copyBackOffset, which will be used
            // later to copy only the X's and not the #'s (from diagram above).
            if (!alignedOffs)
            {
                readOffs = AZ_SIZE_ALIGN_DOWN(readOffs, m_logicalSectorSize);
                u64 offsetCorrection = data->m_offset - readOffs;
                readInfo.m_copyBackOffset = offsetCorrection;
                readSize = aznumeric_cast<DWORD>(data->m_size + offsetCorrection);
            }

            bool alignedSize = IStreamerTypes::IsAlignedTo(readSize, aznumeric_caster(m_logicalSectorSize));
            if (!alignedSize)
            {
                DWORD alignedReadSize = aznumeric_caster(AZ_SIZE_ALIGN_UP(readSize, m_logicalSectorSize));
                if (alignedReadSize <= data->m_outputSize)
                {
                    alignedSize = true;
                    readSize = alignedReadSize;
                }
            }

            // Adjust the size again if the end is misaligned.
            // Aligns the new read size up to be a multiple of the the sector size.
            //
            // Before:
            // +---------------+---------------+
            // |XXXXXXXXXXXXXXX|XXXXXXX        |
            // +---------------+---------------+
            // ^--- offset
            // <--- size ------------->
            //
            // After:
            // +---------------+---------------+
            // |XXXXXXXXXXXXXXX|XXXXXXX########|
            // +---------------+---------------+
            // ^--- offset
            // <--- size ---------------------->
            //
            // Once everything is aligned, allocate the temporary buffer.
            // Again, when read completes from OS, only copy back the X's and not
            // the #'s (from diagram above).
            const bool isAligned = (alignedAddr && alignedSize && alignedOffs);
            if (!isAligned)
            {
                readSize = aznumeric_cast<DWORD>(AZ_SIZE_ALIGN_UP(readSize, m_logicalSectorSize));
                readInfo.AllocateAlignedBuffer(readSize, m_physicalSectorSize);
                output = readInfo.m_sectorAlignedOutput;
            }
#if AZ_STREAMER_ADD_EXTRA_PROFILING_INFO
            m_directReadsPercentageStat.PushSample(isAligned ? 1.0 : 0.0);
            Statistic::PlotImmediate(m_name, DirectReadsName, m_directReadsPercentageStat.GetMostRecentSample());
#endif // AZ_STREAMER_ADD_EXTRA_PROFILING_INFO
        }

        FileReadStatus& readStatus = m_readSlots_statusInfo[readSlot];
        LPOVERLAPPED overlapped = &readStatus.m_overlapped;
        overlapped->Offset = aznumeric_caster(readOffs);
        overlapped->OffsetHigh = aznumeric_caster(readOffs >> (sizeof(overlapped->Offset) << 3));
        overlapped->hEvent = m_context->GetStreamerThreadSynchronizer().CreateEventHandle();
        readStatus.m_fileHandleIndex = fileCacheSlot;

        bool result = false;
        {
            AZ_PROFILE_SCOPE(AzCore, "StorageDriveWin::ReadRequest ::ReadFile");
            result = ::ReadFile(file, output, readSize, nullptr, overlapped);
        }

        if (!result)
        {
            DWORD error = ::GetLastError();
            if (error != ERROR_IO_PENDING)
            {
                AZ_Warning("StorageDriveWin", false, "::ReadFile failed with error: %u\n", error);

                m_context->GetStreamerThreadSynchronizer().DestroyEventHandle(overlapped->hEvent);

                // Finish the request since this drive opened the file handle but the read failed.
                request->SetStatus(IStreamerTypes::RequestStatus::Failed);
                m_context->MarkRequestAsCompleted(request);
                readInfo.Clear();
                return true;
            }
        }
        else
        {
            // If this scope is reached, it means that ::ReadFile processed the read synchronously.  This can happen if
            // the OS already had the file in the cache.  The OVERLAPPED struct will still be filled out so we proceed as
            // if the read was fully asynchronous.
        }

        auto now = AZStd::chrono::steady_clock::now();
        if (m_activeReads_Count++ == 0)
        {
            m_activeReads_startTime = now;
        }
        readInfo.m_startTime = now;
        m_readSlots_active[readSlot] = true;

#if AZ_STREAMER_ADD_EXTRA_PROFILING_INFO
        if (m_activeCacheSlot == fileCacheSlot)
        {
            m_fileSwitchPercentageStat.PushSample(0.0);
            m_seekPercentageStat.PushSample(m_activeOffset == data->m_offset ? 0.0 : 1.0);
        }
        else
        {
            m_fileSwitchPercentageStat.PushSample(1.0);
            m_seekPercentageStat.PushSample(0.0);
        }

        Statistic::PlotImmediate(m_name, FileSwitchesName, m_fileSwitchPercentageStat.GetMostRecentSample());
        Statistic::PlotImmediate(m_name, SeeksName, m_seekPercentageStat.GetMostRecentSample());
#endif // AZ_STREAMER_ADD_EXTRA_PROFILING_INFO

        m_fileCache_activeReads[fileCacheSlot]++;
        m_activeCacheSlot = fileCacheSlot;
        m_activeOffset = readOffs + readSize;

        return true;
    }

    bool StorageDriveWin::CancelRequest(FileRequest* cancelRequest, FileRequestPtr& target)
    {
        bool ownsRequestChain = false;
        for (auto it = m_pendingReadRequests.begin(); it != m_pendingReadRequests.end();)
        {
            if ((*it)->WorksOn(target))
            {
                (*it)->SetStatus(IStreamerTypes::RequestStatus::Canceled);
                m_context->MarkRequestAsCompleted(*it);
                it = m_pendingReadRequests.erase(it);
                ownsRequestChain = true;
            }
            else
            {
                ++it;
            }
        }

        // Pending requests have been accounted for, now address any active reads and issue a Cancel call to the OS.

        size_t fileCacheIndex = InvalidFileCacheIndex;

        for (size_t readSlot = 0; readSlot < m_readSlots_active.size(); ++readSlot)
        {
            if (m_readSlots_active[readSlot] && m_readSlots_readInfo[readSlot].m_request->WorksOn(target))
            {
                if (fileCacheIndex == InvalidFileCacheIndex)
                {
                    fileCacheIndex = m_readSlots_statusInfo[readSlot].m_fileHandleIndex;
                }
                AZ_Assert(fileCacheIndex == m_readSlots_statusInfo[readSlot].m_fileHandleIndex,
                    "Active file reads for a target read request have mismatched file cache indexes.");

                ownsRequestChain = true;
                if (!::CancelIoEx(m_fileCache_handles[fileCacheIndex], &m_readSlots_statusInfo[readSlot].m_overlapped))
                {
                    DWORD error = ::GetLastError();
                    if (error != ERROR_NOT_FOUND)
                    {
                        AZ_Error("StorageDriveWin", false, "::CancelIoEx failed with error: %u\n", error);
                    }
                }
            }
        }

        if (ownsRequestChain)
        {
            cancelRequest->SetStatus(IStreamerTypes::RequestStatus::Completed);
            m_context->MarkRequestAsCompleted(cancelRequest);
        }

        return ownsRequestChain;
    }

    void StorageDriveWin::FileExistsRequest(FileRequest* request)
    {
        auto& fileExists = AZStd::get<Requests::FileExistsCheckData>(request->GetCommand());

        AZ_PROFILE_SCOPE(AzCore, "StorageDriveWin::FileExistsRequest %s : %s",
            m_name.c_str(), fileExists.m_path.GetRelativePath());
        TIMED_AVERAGE_WINDOW_SCOPE(m_getFileExistsTimeAverage);

        AZ_Assert(IsServicedByThisDrive(fileExists.m_path.GetAbsolutePath()),
            "FileExistsRequest was queued on a StorageDriveWin that doesn't service files on the given path '%s'.",
            fileExists.m_path.GetRelativePath());

        size_t cacheIndex = FindInFileHandleCache(fileExists.m_path);
        if (cacheIndex != InvalidFileCacheIndex)
        {
            fileExists.m_found = true;
            request->SetStatus(IStreamerTypes::RequestStatus::Completed);
            m_context->MarkRequestAsCompleted(request);
            return;
        }

        cacheIndex = FindInMetaDataCache(fileExists.m_path);
        if (cacheIndex != InvalidMetaDataCacheIndex)
        {
            fileExists.m_found = true;
            request->SetStatus(IStreamerTypes::RequestStatus::Completed);
            m_context->MarkRequestAsCompleted(request);
            return;
        }

        WIN32_FILE_ATTRIBUTE_DATA attributes;
        AZStd::wstring filenameW;
        AZStd::to_wstring(filenameW, fileExists.m_path.GetAbsolutePathCStr());
        if (::GetFileAttributesExW(filenameW.c_str(), GetFileExInfoStandard, &attributes))
        {
            if ((attributes.dwFileAttributes != INVALID_FILE_ATTRIBUTES) &&
                ((attributes.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0))
            {
                LARGE_INTEGER fileSize;
                fileSize.LowPart = attributes.nFileSizeLow;
                fileSize.HighPart = attributes.nFileSizeHigh;

                cacheIndex = GetNextMetaDataCacheSlot();
                m_metaDataCache_paths[cacheIndex] = fileExists.m_path;
                m_metaDataCache_fileSize[cacheIndex] = aznumeric_caster(fileSize.QuadPart);
                fileExists.m_found = true;

                request->SetStatus(IStreamerTypes::RequestStatus::Completed);
                m_context->MarkRequestAsCompleted(request);
            }
            return;
        }

        StreamStackEntry::QueueRequest(request);
    }

    void StorageDriveWin::FileMetaDataRetrievalRequest(FileRequest* request)
    {
        auto& command = AZStd::get<Requests::FileMetaDataRetrievalData>(request->GetCommand());

        AZ_PROFILE_SCOPE(AzCore, "StorageDriveWin::FileMetaDataRetrievalRequest %s : %s",
            m_name.c_str(), command.m_path.GetRelativePath());
        TIMED_AVERAGE_WINDOW_SCOPE(m_getFileMetaDataRetrievalTimeAverage);

        size_t cacheIndex = FindInMetaDataCache(command.m_path);
        if (cacheIndex != InvalidMetaDataCacheIndex)
        {
            command.m_fileSize = m_metaDataCache_fileSize[cacheIndex];
            command.m_found = true;
            request->SetStatus(IStreamerTypes::RequestStatus::Completed);
            m_context->MarkRequestAsCompleted(request);
            return;
        }

        LARGE_INTEGER fileSize{};

        cacheIndex = FindInFileHandleCache(command.m_path);
        if (cacheIndex != InvalidFileCacheIndex)
        {
            AZ_Assert(m_fileCache_handles[cacheIndex] != INVALID_HANDLE_VALUE,
                "File path '%s' doesn't have an associated file handle.", m_fileCache_paths[cacheIndex].GetRelativePath());
            if (::GetFileSizeEx(m_fileCache_handles[cacheIndex], &fileSize) == FALSE)
            {
                StreamStackEntry::QueueRequest(request);
                return;
            }
        }
        else
        {
            WIN32_FILE_ATTRIBUTE_DATA attributes;
            AZStd::wstring filenameW;
            AZStd::to_wstring(filenameW, command.m_path.GetAbsolutePathCStr());
            if (::GetFileAttributesExW(filenameW.c_str(), GetFileExInfoStandard, &attributes) &&
                (attributes.dwFileAttributes != INVALID_FILE_ATTRIBUTES) && ((attributes.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0))
            {
                fileSize.LowPart = attributes.nFileSizeLow;
                fileSize.HighPart = attributes.nFileSizeHigh;
            }
            else
            {
                StreamStackEntry::QueueRequest(request);
                return;
            }
        }

        command.m_fileSize = aznumeric_caster(fileSize.QuadPart);
        command.m_found = true;

        cacheIndex = GetNextMetaDataCacheSlot();

        m_metaDataCache_paths[cacheIndex] = command.m_path;
        m_metaDataCache_fileSize[cacheIndex] = aznumeric_caster(fileSize.QuadPart);

        request->SetStatus(IStreamerTypes::RequestStatus::Completed);
        m_context->MarkRequestAsCompleted(request);
    }

    void StorageDriveWin::FlushCache(const RequestPath& filePath)
    {
        if (m_cachesInitialized)
        {
            size_t cacheIndex = FindInFileHandleCache(filePath);
            if (cacheIndex != InvalidFileCacheIndex)
            {
                if (m_fileCache_handles[cacheIndex] != INVALID_HANDLE_VALUE)
                {
                    AZ_Assert(m_fileCache_activeReads[cacheIndex] == 0, "Flushing '%s' but it has %u active reads\n",
                        filePath.GetRelativePath(), m_fileCache_activeReads[cacheIndex]);
                    ::CloseHandle(m_fileCache_handles[cacheIndex]);
                    m_fileCache_handles[cacheIndex] = INVALID_HANDLE_VALUE;
                }
                m_fileCache_activeReads[cacheIndex] = 0;
                m_fileCache_lastTimeUsed[cacheIndex] = AZStd::chrono::steady_clock::time_point();
                m_fileCache_paths[cacheIndex].Clear();
            }

            cacheIndex = FindInMetaDataCache(filePath);
            if (cacheIndex != InvalidMetaDataCacheIndex)
            {
                m_metaDataCache_paths[cacheIndex].Clear();
                m_metaDataCache_fileSize[cacheIndex] = 0;
            }
        }
    }

    void StorageDriveWin::FlushEntireCache()
    {
        if (m_cachesInitialized)
        {
            // Clear file handle cache
            for (size_t cacheIndex = 0; cacheIndex < m_maxFileHandles; ++cacheIndex)
            {
                if (m_fileCache_handles[cacheIndex] != INVALID_HANDLE_VALUE)
                {
                    AZ_Assert(m_fileCache_activeReads[cacheIndex] == 0, "Flushing '%s' but it has %u active reads\n",
                        m_fileCache_paths[cacheIndex].GetRelativePath(), m_fileCache_activeReads[cacheIndex]);
                    ::CloseHandle(m_fileCache_handles[cacheIndex]);
                    m_fileCache_handles[cacheIndex] = INVALID_HANDLE_VALUE;
                }
                m_fileCache_activeReads[cacheIndex] = 0;
                m_fileCache_lastTimeUsed[cacheIndex] = AZStd::chrono::steady_clock::time_point();
                m_fileCache_paths[cacheIndex].Clear();
            }

            // Clear meta data cache
            auto metaDataCacheSize = m_metaDataCache_paths.size();
            m_metaDataCache_paths.clear();
            m_metaDataCache_fileSize.clear();
            m_metaDataCache_front = 0;
            m_metaDataCache_paths.resize(metaDataCacheSize);
            m_metaDataCache_fileSize.resize(metaDataCacheSize);
        }
    }

    bool StorageDriveWin::FinalizeReads()
    {
        AZ_PROFILE_FUNCTION(AzCore);

        bool hasWorked = false;
        for (size_t readSlot = 0; readSlot < m_readSlots_active.size(); ++readSlot)
        {
            if (m_readSlots_active[readSlot])
            {
                FileReadStatus& status = m_readSlots_statusInfo[readSlot];

                if (HasOverlappedIoCompleted(&status.m_overlapped))
                {
                    DWORD bytesTransferred = 0;
                    BOOL result = ::GetOverlappedResult(m_fileCache_handles[status.m_fileHandleIndex],
                        &status.m_overlapped, &bytesTransferred, FALSE);
                    DWORD error = ::GetLastError();

                    if (result || error == ERROR_OPERATION_ABORTED)
                    {
                        hasWorked = true;
                        constexpr bool encounteredError = false;
                        FinalizeSingleRequest(status, readSlot, bytesTransferred, error == ERROR_OPERATION_ABORTED, encounteredError);
                    }
                    else if (error != ERROR_IO_PENDING && error != ERROR_IO_INCOMPLETE)
                    {
                        AZ_Error("StorageDriveWin", false, "Async file read operation completed with extended error code %u\n", error);
                        hasWorked = true;
                        constexpr bool encounteredError = true;
                        FinalizeSingleRequest(status, readSlot, bytesTransferred, false, encounteredError);
                    }
                }
            }
        }
        return hasWorked;
    }

    void StorageDriveWin::FinalizeSingleRequest(FileReadStatus& status, size_t readSlot, DWORD numBytesTransferred,
        bool isCanceled, bool encounteredError)
    {
        m_activeReads_ByteCount += numBytesTransferred;
        if (--m_activeReads_Count == 0)
        {
            // Update read stats now that the operation is done.
            m_readSizeAverage.PushEntry(m_activeReads_ByteCount);
            m_readTimeAverage.PushEntry(AZStd::chrono::duration_cast<AZStd::chrono::microseconds>(
                AZStd::chrono::steady_clock::now() - m_activeReads_startTime));

            m_activeReads_ByteCount = 0;
        }

        FileReadInformation& fileReadInfo = m_readSlots_readInfo[readSlot];

        auto readCommand = AZStd::get_if<Requests::ReadData>(&fileReadInfo.m_request->GetCommand());
        AZ_Assert(readCommand != nullptr, "Request stored with the overlapped I/O call did not contain a read request.");

        if (fileReadInfo.m_sectorAlignedOutput && !encounteredError)
        {
            auto offsetAddress = reinterpret_cast<u8*>(fileReadInfo.m_sectorAlignedOutput) + fileReadInfo.m_copyBackOffset;
            ::memcpy(readCommand->m_output, offsetAddress, readCommand->m_size);
        }

        // The request could be reading more due to alignment requirements. It should however never read less that the amount of
        // requested data.
        bool isSuccess = !encounteredError && (readCommand->m_size <= numBytesTransferred);

        fileReadInfo.m_request->SetStatus(
            isCanceled
                ? IStreamerTypes::RequestStatus::Canceled
                : isSuccess
                    ? IStreamerTypes::RequestStatus::Completed
                    : IStreamerTypes::RequestStatus::Failed
        );
        m_context->MarkRequestAsCompleted(fileReadInfo.m_request);

        m_fileCache_activeReads[status.m_fileHandleIndex]--;
        m_readSlots_active[readSlot] = false;
        m_context->GetStreamerThreadSynchronizer().DestroyEventHandle(status.m_overlapped.hEvent);
        fileReadInfo.Clear();

        // There's now a slot available to queue the next request, if there is one.
        if (!m_pendingReadRequests.empty())
        {
            FileRequest* request = m_pendingReadRequests.front();
            if (ReadRequest(request, readSlot))
            {
                m_pendingReadRequests.pop_front();
            }
        }
    }

    size_t StorageDriveWin::FindInFileHandleCache(const RequestPath& filePath) const
    {
        size_t numFiles = m_fileCache_paths.size();
        for (size_t i = 0; i < numFiles; ++i)
        {
            if (m_fileCache_paths[i] == filePath)
            {
                return i;
            }
        }
        return InvalidFileCacheIndex;
    }

    size_t StorageDriveWin::FindAvailableFileHandleCacheIndex() const
    {
        AZ_Assert(m_cachesInitialized, "Using file cache before it has been (lazily) initialized\n");

        // This needs to look for files with no active reads, and the oldest file among those.
        size_t cacheIndex = InvalidFileCacheIndex;
        AZStd::chrono::steady_clock::time_point oldest = AZStd::chrono::steady_clock::time_point::max();
        for (size_t index = 0; index < m_maxFileHandles; ++index)
        {
            if (m_fileCache_activeReads[index] == 0 && m_fileCache_lastTimeUsed[index] < oldest)
            {
                oldest = m_fileCache_lastTimeUsed[index];
                cacheIndex = index;
            }
        }

        return cacheIndex;
    }

    size_t StorageDriveWin::FindAvailableReadSlot()
    {
        for (size_t i = 0; i < m_readSlots_active.size(); ++i)
        {
            if (!m_readSlots_active[i])
            {
                return i;
            }
        }
        return InvalidReadSlotIndex;
    }

    size_t StorageDriveWin::FindInMetaDataCache(const RequestPath& filePath) const
    {
        size_t numFiles = m_metaDataCache_paths.size();
        for (size_t i = 0; i < numFiles; ++i)
        {
            if (m_metaDataCache_paths[i] == filePath)
            {
                return i;
            }
        }
        return InvalidMetaDataCacheIndex;
    }

    size_t StorageDriveWin::GetNextMetaDataCacheSlot()
    {
        m_metaDataCache_front = (m_metaDataCache_front + 1) & (m_metaDataCache_paths.size() - 1);
        return m_metaDataCache_front;
    }

    bool StorageDriveWin::IsServicedByThisDrive(AZ::IO::PathView filePath) const
    {
        // This approach doesn't allow paths to be resolved to the correct drive when junctions are used or when a drive
        // is mapped as folder of another drive. To do this correctly "GetVolumePathName" should be used, but this takes
        // about 1 to 1.5 ms per request, so this introduces unacceptably large overhead particularly when the user has
        // multiple disks.
        for (const AZStd::string& drivePath : m_drivePaths)
        {
            if (filePath.RootName().Compare(drivePath) == 0)
            {
                return true;
            }
        }
        return false;
    }

    void StorageDriveWin::CollectStatistics(AZStd::vector<Statistic>& statistics) const
    {
        if (m_cachesInitialized)
        {
            using DoubleSeconds = AZStd::chrono::duration<double>;

            u64 totalBytesRead = m_readSizeAverage.GetTotal();
            double totalReadTimeSec = AZStd::chrono::duration_cast<DoubleSeconds>(m_readTimeAverage.GetTotal()).count();
            statistics.push_back(Statistic::CreateBytesPerSecond(m_name, "Read Speed", totalBytesRead / totalReadTimeSec,
                "The average read speed in megabytes per second this drive achieved. This is the maximum achievable speed for reading from "
                "disk. If this is lower than expected it may indicate that there's an overhead from the operating system, the drive has "
                "seen a lot of use, other applications are using the same drive and/or anti-virus scans are slowing down reads. Enabling "
                "buffered reads through the Settings Registry can increase the read speeds as the operating system can cache files, but "
                "this will typically only accelerate files that are read multiple times and will be slower for the first read. Artificial "
                "can therefore be misleading if the same files are repeatedly loaded."));
            statistics.push_back(Statistic::CreateTimeRange(
                m_name, "File Open & Close", m_fileOpenCloseTimeAverage.CalculateAverage(), m_fileOpenCloseTimeAverage.GetMinimum(),
                m_fileOpenCloseTimeAverage.GetMaximum(),
                "The average amount of time needed to open and close file handles. This is a fixed cost from the operating "
                "system. This can be mitigated running from archives."));
            statistics.push_back(Statistic::CreateTimeRange(
                m_name, "Get file exists", m_getFileExistsTimeAverage.CalculateAverage(),
                m_getFileExistsTimeAverage.GetMinimum(), m_getFileExistsTimeAverage.GetMaximum(),
                "The average amount of time needed to check if a file exists. This is a fixed cost from the operating "
                "system. This can be mitigated running from archives."));
            statistics.push_back(Statistic::CreateTimeRange(
                m_name, "Get file meta data", m_getFileMetaDataRetrievalTimeAverage.CalculateAverage(),
                m_getFileMetaDataRetrievalTimeAverage.GetMinimum(), m_getFileMetaDataRetrievalTimeAverage.GetMaximum(),
                "The average amount of time in microseconds needed to retrieve file information. This is a fixed cost from the operating "
                "system. This can be mitigated running from archives."));

            statistics.push_back(Statistic::CreateInteger(m_name, "Available slots", CalculateNumAvailableSlots(),
                "The total number of available slots to queue requests on. The lower this number, the more active this node is. A small "
                "number is ideal as it means there are a few requests available for immediate processing next once a request "
                "completes. If this is value is often negative then increasing the over-commit value, but keep in mind that too many "
                "over-committed reduces the ability of scheduler to order requests."));

#if AZ_STREAMER_ADD_EXTRA_PROFILING_INFO
            statistics.push_back(Statistic::CreatePercentageRange(
                m_name, FileSwitchesName, m_fileSwitchPercentageStat.GetAverage(), m_fileSwitchPercentageStat.GetMinimum(),
                m_fileSwitchPercentageStat.GetMaximum(),
                "The percentage of file requests that required switching to a different file. When running from loose file this should be "
                "close to 100% as that would indicate mostly full file reads. When running from archives this should be as close to 0 as "
                "possible as that would indicate efficiently running from archives."));
            statistics.push_back(Statistic::CreatePercentageRange(
                m_name, SeeksName, m_seekPercentageStat.GetAverage(), m_seekPercentageStat.GetMinimum(), m_seekPercentageStat.GetMaximum(),
                "The percentage of file reads that required seeking within a file. For loose files this should be lose to zero to indicate "
                "no partial file reads. For archives this value is typically high, which is not a problem, but lower values indicate more "
                "efficient scheduling and archive layout which will result in better hardware cache utilization."));
            statistics.push_back(Statistic::CreatePercentageRange(
                m_name, DirectReadsName, m_directReadsPercentageStat.GetAverage(), m_directReadsPercentageStat.GetMinimum(),
                m_directReadsPercentageStat.GetMaximum(),
                "The percentage of reads that did not require any additional aligning. If this number isn't close to 100 percent "
                "performance will suffer as temporary buffers need to be allocated and freed. The best way to avoid this is by adding a "
                "block cache and/or read splitter in front of this node."));
#endif
        }
        StreamStackEntry::CollectStatistics(statistics);
    }

    void StorageDriveWin::Report(const Requests::ReportData& data) const
    {
        switch (data.m_reportType)
        {
        case IStreamerTypes::ReportType::Config:
            {
                AZStd::string drivePaths;
                AZ::StringFunc::Join(drivePaths, m_drivePaths, ' ');
                data.m_output.push_back(Statistic::CreatePersistentString(
                    m_name, "Drive paths", AZStd::move(drivePaths), "The drive paths this node monitors."));
                data.m_output.push_back(Statistic::CreateInteger(
                    m_name, "Max file handles", m_maxFileHandles,
                    "The maximum number of file handles this drive node will cache. Increasing this will allow files that are read "
                    "multiple times to be processed faster. It's recommended to have this set to at least the largest number of archives "
                    "that can be in use at the same time."));
                data.m_output.push_back(Statistic::CreateInteger(
                    m_name, "Max meta data cache", m_metaDataCache_paths.size(),
                    "The maximum number of meta data like file sizes this drive node will cache."));
                data.m_output.push_back(Statistic::CreateByteSize(
                    m_name, "Physical sector size", m_physicalSectorSize,
                    "The sector size used by the hardware. For optimal performance memory alignment and read sizes need to be multiples of "
                    "this value."));
                data.m_output.push_back(Statistic::CreateByteSize(
                    m_name, "Logical sector size", m_logicalSectorSize,
                    "The sector size used by the operating system. This is typically the same or smaller than the physical sector size. If "
                    "the physical sector size alignment can't be met, this is the next best size to align to."));
                data.m_output.push_back(Statistic::CreateInteger(
                    m_name, "IO channel count", m_ioChannelCount, "The amount of requests the hardware can process in parallel."));
                data.m_output.push_back(Statistic::CreateInteger(
                    m_name, "Overcommit", m_overCommit,
                    "The number of additional requests this node will accept. Higher numbers means that drives don't have to wait for the "
                    "scheduler to provide new request to process and the next request can immediately start reading. If this value is too "
                    "high though it will negatively impact the scheduler's ability to order and prioritize requests, which can lead to "
                    "poorer hardware and software cache performance and slower cancellations, among others."));
                data.m_output.push_back(Statistic::CreateBoolean(
                    m_name, "Has seek penalty", m_constructionOptions.m_hasSeekPenalty,
                    "Whether or not the hardware has a penalty for seeking. This refers to drives that need to physically position a read "
                    "head to retrieve data, which can cause additional seek times for non-consecutive reads. This does not refer to seeks "
                    "impacting hardware cache performance."));
                data.m_output.push_back(Statistic::CreateBoolean(
                    m_name, "Unbuffered reads enabled", m_constructionOptions.m_enableUnbufferedReads,
                    "Whether or not this drive will use the operating system's cache (buffered) or not (unbuffered). Buffered reads are "
                    "beneficial when reading the same file frequently, which happens during development. Unbuffered typically is faster "
                    "when reading the initial file as there's much less the operating system has to do, but subsequential reads are "
                    "slower. Unbuffered is optimal for released games as these don't often read the same file. Please keep in mind that "
                    "artificial tests may show an improvement with buffered enabled but this can be due to repeatedly reading the same "
                    "files and often doesn't reflect a real-world scenario."));
                data.m_output.push_back(Statistic::CreateBoolean(
                    m_name, "Enable sharing", m_constructionOptions.m_enableSharing,
                    "Whether or not file read sharing is enabled. When enabled this enabled other applications can continue to read the "
                    "files, though not write to them. This doesn't have any noticeable impact on performance, but may be a security "
                    "concern."));
                data.m_output.push_back(Statistic::CreateBoolean(
                    m_name, "Minimal reporting", m_constructionOptions.m_minimalReporting,
                    "Whether or not this node only reports issues or reports all information."));
                data.m_output.push_back(Statistic::CreateReferenceString(
                    m_name, "Next node", m_next ? AZStd::string_view(m_next->GetName()) : AZStd::string_view("<None>"),
                    "The name of the node that follows this node or none."));
            }
            break;
        case IStreamerTypes::ReportType::FileLocks:
            if (m_cachesInitialized)
            {
                for (u32 i = 0; i < m_maxFileHandles; ++i)
                {
                    if (m_fileCache_handles[i] != INVALID_HANDLE_VALUE)
                    {
                        data.m_output.push_back(
                            Statistic::CreatePersistentString(m_name, "File lock", m_fileCache_paths[i].GetRelativePath().Native()));
                    }
                }
            }
            break;
        default:
            break;
        }
    }
} // namespace AZ::IO
