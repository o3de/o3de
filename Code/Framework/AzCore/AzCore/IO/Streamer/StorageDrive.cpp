/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <limits>
#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/Debug/Profiler.h>
#include <AzCore/IO/Streamer/FileRequest.h>
#include <AzCore/IO/Streamer/StreamerContext.h>
#include <AzCore/IO/Streamer/StorageDrive.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/std/typetraits/decay.h>

namespace AZ::IO
{
    AZStd::shared_ptr<StreamStackEntry> StorageDriveConfig::AddStreamStackEntry(
        [[maybe_unused]] const HardwareInformation& hardware, [[maybe_unused]] AZStd::shared_ptr<StreamStackEntry> parent)
    {
        return AZStd::make_shared<StorageDrive>(m_maxFileHandles);
    }

    void StorageDriveConfig::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context); serializeContext != nullptr)
        {
            serializeContext->Class<StorageDriveConfig, IStreamerStackConfig>()
                ->Version(1)
                ->Field("MaxFileHandles", &StorageDriveConfig::m_maxFileHandles);
        }
    }

    const AZStd::chrono::microseconds StorageDrive::s_averageSeekTime =
        AZStd::chrono::milliseconds(9) + // Common average seek time for desktop hdd drives.
        AZStd::chrono::milliseconds(3); // Rotational latency for a 7200RPM disk

    StorageDrive::StorageDrive(u32 maxFileHandles)
        : StreamStackEntry("Storage drive (generic)")
    {
        m_fileLastUsed.resize(maxFileHandles, AZStd::chrono::steady_clock::time_point::min());
        m_filePaths.resize(maxFileHandles);
        m_fileHandles.resize(maxFileHandles);

        // Add initial dummy values to the stats to avoid division by zero later on and avoid needing branches.
        m_readSizeAverage.PushEntry(1);
        m_readTimeAverage.PushEntry(AZStd::chrono::microseconds(1));
    }

    void StorageDrive::SetNext(AZStd::shared_ptr<StreamStackEntry> /*next*/)
    {
        AZ_Assert(false, "StorageDrive isn't allowed to have a node to forward requests to.");
    }

    void StorageDrive::PrepareRequest(FileRequest* request)
    {
        AZ_PROFILE_FUNCTION(AzCore);
        AZ_Assert(request, "PrepareRequest was provided a null request.");

        if (AZStd::holds_alternative<Requests::ReadRequestData>(request->GetCommand()))
        {
            auto& readRequest = AZStd::get<Requests::ReadRequestData>(request->GetCommand());

            FileRequest* read = m_context->GetNewInternalRequest();
            read->CreateRead(
                request, readRequest.m_output, readRequest.m_outputSize, readRequest.m_path, readRequest.m_offset, readRequest.m_size);
            m_context->PushPreparedRequest(read);
            return;
        }
        StreamStackEntry::PrepareRequest(request);
    }

    void StorageDrive::QueueRequest(FileRequest* request)
    {
        AZ_Assert(request, "QueueRequest was provided a null request.");
        AZStd::visit([this, request](auto&& args)
        {
            using Command = AZStd::decay_t<decltype(args)>;
            if constexpr (AZStd::is_same_v<Command, Requests::ReadData> ||
                AZStd::is_same_v<Command, Requests::FileExistsCheckData> ||
                AZStd::is_same_v<Command, Requests::FileMetaDataRetrievalData>)
            {
                m_pendingRequests.push_back(request);
                return;
            }
            else if constexpr (AZStd::is_same_v<Command, Requests::CancelData>)
            {
                CancelRequest(request, args.m_target);
                return;
            }
            else
            {
                if constexpr (AZStd::is_same_v<Command, Requests::FlushData>)
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
            }
        }, request->GetCommand());
    }

    bool StorageDrive::ExecuteRequests()
    {
        if (!m_pendingRequests.empty())
        {
            FileRequest* request = m_pendingRequests.front();
            AZStd::visit([this, request](auto&& args)
            {
                using Command = AZStd::decay_t<decltype(args)>;
                if constexpr (AZStd::is_same_v<Command, Requests::ReadData>)
                {
                    ReadFile(request);
                }
                else if constexpr (AZStd::is_same_v<Command, Requests::FileExistsCheckData>)
                {
                    FileExistsRequest(request);
                }
                else if constexpr (AZStd::is_same_v<Command, Requests::FileMetaDataRetrievalData>)
                {
                    FileMetaDataRetrievalRequest(request);
                }
            }, request->GetCommand());
            m_pendingRequests.pop_front();
            return true;
        }
        else
        {
            return false;
        }
    }

    void StorageDrive::UpdateStatus(Status& status) const
    {
        // Only participate if there are actually any reads done.
        if (m_fileOpenCloseTimeAverage.GetNumRecorded() > 0)
        {
            s32 availableSlots = s_maxRequests - aznumeric_cast<s32>(m_pendingRequests.size());
            StreamStackEntry::UpdateStatus(status);
            status.m_numAvailableSlots = AZStd::min(status.m_numAvailableSlots, availableSlots);
            status.m_isIdle = status.m_isIdle && m_pendingRequests.empty();
        }
        else
        {
            status.m_numAvailableSlots = AZStd::min(status.m_numAvailableSlots, s_maxRequests);
        }
    }

    void StorageDrive::UpdateCompletionEstimates(AZStd::chrono::steady_clock::time_point now,
        AZStd::vector<FileRequest*>& internalPending, StreamerContext::PreparedQueue::iterator pendingBegin,
        StreamerContext::PreparedQueue::iterator pendingEnd)
    {
        StreamStackEntry::UpdateCompletionEstimates(now, internalPending, pendingBegin, pendingEnd);

        const RequestPath* activeFile = nullptr;
        if (m_activeCacheSlot != s_fileNotFound)
        {
            activeFile = &m_filePaths[m_activeCacheSlot];
        }
        u64 activeOffset = m_activeOffset;

        // Estimate requests in this stack entry.
        for (FileRequest* request : m_pendingRequests)
        {
            EstimateCompletionTimeForRequest(request, now, activeFile, activeOffset);
        }

        // Estimate internally pending requests. Because this call will go from the top of the stack to the bottom,
        // but estimation is calculated from the bottom to the top, this list should be processed in reverse order.
        for (auto requestIt = internalPending.rbegin(); requestIt != internalPending.rend(); ++requestIt)
        {
            EstimateCompletionTimeForRequest(*requestIt, now, activeFile, activeOffset);
        }

        // Estimate pending requests that have not been queued yet.
        for (auto requestIt = pendingBegin; requestIt != pendingEnd; ++requestIt)
        {
            EstimateCompletionTimeForRequest(*requestIt, now, activeFile, activeOffset);
        }
    }

    void StorageDrive::EstimateCompletionTimeForRequest(FileRequest* request, AZStd::chrono::steady_clock::time_point& startTime,
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
                AZStd::chrono::microseconds averageTime = AZStd::chrono::duration_cast<AZStd::chrono::microseconds>(m_getFileExistsTimeAverage.CalculateAverage());
                startTime += averageTime;
            }
            else if constexpr (AZStd::is_same_v<Command, Requests::FileMetaDataRetrievalData>)
            {
                readSize = 0;
                AZStd::chrono::microseconds averageTime = AZStd::chrono::duration_cast<AZStd::chrono::microseconds>(m_getFileMetaDataTimeAverage.CalculateAverage());
                startTime += averageTime;
            }
        }, request->GetCommand());

        if (readSize > 0)
        {
            if (activeFile && activeFile != targetFile)
            {
                if (FindFileInCache(*targetFile) == s_fileNotFound)
                {
                    AZStd::chrono::microseconds fileOpenCloseTimeAverage = AZStd::chrono::duration_cast<AZStd::chrono::microseconds>(m_fileOpenCloseTimeAverage.CalculateAverage());
                    startTime += fileOpenCloseTimeAverage;
                }
                startTime += s_averageSeekTime;
                activeOffset = std::numeric_limits<u64>::max();
            }
            else if (activeOffset != offset)
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

    void StorageDrive::ReadFile(FileRequest* request)
    {
        AZ_PROFILE_FUNCTION(AzCore);

        auto data = AZStd::get_if<Requests::ReadData>(&request->GetCommand());
        AZ_Assert(data, "FileRequest queued on StorageDrive to be read didn't contain read data.");

        SystemFile* file = nullptr;

        // If the file is already open, use that file handle and update it's last touched time.
        size_t cacheIndex = FindFileInCache(data->m_path);
        if (cacheIndex != s_fileNotFound)
        {
            file = m_fileHandles[cacheIndex].get();
            m_fileLastUsed[cacheIndex] = AZStd::chrono::steady_clock::now();
        }

        // If the file is not open, eject the entry from the cache that hasn't been used for the longest time
        // and open the file for reading.
        if (!file)
        {
            AZStd::chrono::steady_clock::time_point oldest = m_fileLastUsed[0];
            cacheIndex = 0;
            size_t numFiles = m_filePaths.size();
            for (size_t i = 1; i < numFiles; ++i)
            {
                if (m_fileLastUsed[i] < oldest)
                {
                    oldest = m_fileLastUsed[i];
                    cacheIndex = i;
                }
            }

            TIMED_AVERAGE_WINDOW_SCOPE(m_fileOpenCloseTimeAverage);
            AZStd::unique_ptr<SystemFile> newFile = AZStd::make_unique<SystemFile>();
            bool isOpen = newFile->Open(data->m_path.GetAbsolutePathCStr(), SystemFile::OpenMode::SF_OPEN_READ_ONLY);
            if (!isOpen)
            {
                request->SetStatus(IStreamerTypes::RequestStatus::Failed);
                m_context->MarkRequestAsCompleted(request);
                return;
            }

            file = newFile.get();
            m_fileLastUsed[cacheIndex] = AZStd::chrono::steady_clock::now();
            m_fileHandles[cacheIndex] = AZStd::move(newFile);
            m_filePaths[cacheIndex] = data->m_path;
        }

        AZ_Assert(file, "While searching for file '%s' StorageDevice::ReadFile failed to detect a problem.", data->m_path.GetRelativePath());
        u64 bytesRead = 0;
        {
            TIMED_AVERAGE_WINDOW_SCOPE(m_readTimeAverage);
            if (file->Tell() != data->m_offset)
            {
                file->Seek(data->m_offset, SystemFile::SeekMode::SF_SEEK_BEGIN);
            }
            bytesRead = file->Read(data->m_size, data->m_output);
        }
        m_readSizeAverage.PushEntry(bytesRead);

        m_activeCacheSlot = cacheIndex;
        m_activeOffset = data->m_offset + bytesRead;

        request->SetStatus(bytesRead == data->m_size ? IStreamerTypes::RequestStatus::Completed : IStreamerTypes::RequestStatus::Failed);
        m_context->MarkRequestAsCompleted(request);
    }

    void StorageDrive::CancelRequest(FileRequest* cancelRequest, FileRequestPtr& target)
    {
        for (auto it = m_pendingRequests.begin(); it != m_pendingRequests.end();)
        {
            if ((*it)->WorksOn(target))
            {
                (*it)->SetStatus(IStreamerTypes::RequestStatus::Canceled);
                m_context->MarkRequestAsCompleted(*it);
                it = m_pendingRequests.erase(it);
            }
            else
            {
                ++it;
            }
        }
        cancelRequest->SetStatus(IStreamerTypes::RequestStatus::Completed);
        m_context->MarkRequestAsCompleted(cancelRequest);
    }

    void StorageDrive::FileExistsRequest(FileRequest* request)
    {
        AZ_PROFILE_FUNCTION(AzCore);
        TIMED_AVERAGE_WINDOW_SCOPE(m_getFileExistsTimeAverage);

        auto& fileExists = AZStd::get<Requests::FileExistsCheckData>(request->GetCommand());
        size_t cacheIndex = FindFileInCache(fileExists.m_path);
        if (cacheIndex != s_fileNotFound)
        {
            fileExists.m_found = true;
        }
        else
        {
            fileExists.m_found = SystemFile::Exists(fileExists.m_path.GetAbsolutePathCStr());
        }
        m_context->MarkRequestAsCompleted(request);
    }

    void StorageDrive::FileMetaDataRetrievalRequest(FileRequest* request)
    {
        AZ_PROFILE_FUNCTION(AzCore);
        TIMED_AVERAGE_WINDOW_SCOPE(m_getFileMetaDataTimeAverage);

        auto& command = AZStd::get<Requests::FileMetaDataRetrievalData>(request->GetCommand());
        // If the file is already open, use the file handle which usually is cheaper than asking for the file by name.
        size_t cacheIndex = FindFileInCache(command.m_path);
        if (cacheIndex != s_fileNotFound)
        {
            AZ_Assert(m_fileHandles[cacheIndex],
                "File path '%s' doesn't have an associated file handle.", m_filePaths[cacheIndex].GetRelativePathCStr());
            command.m_fileSize = m_fileHandles[cacheIndex]->Length();
            command.m_found = true;
            request->SetStatus(IStreamerTypes::RequestStatus::Completed);
        }
        else
        {
            // The file is not open yet, so try to get the file size by name.
            u64 size = SystemFile::Length(command.m_path.GetAbsolutePathCStr());
            if (size != 0) // SystemFile::Length doesn't allow telling a zero-sized file apart from a invalid path.
            {
                command.m_fileSize = size;
                command.m_found = true;
                request->SetStatus(IStreamerTypes::RequestStatus::Completed);
            }
            else
            {
                request->SetStatus(IStreamerTypes::RequestStatus::Failed);
            }
        }

        m_context->MarkRequestAsCompleted(request);
    }

    void StorageDrive::FlushCache(const RequestPath& filePath)
    {
        size_t cacheIndex = FindFileInCache(filePath);
        if (cacheIndex != s_fileNotFound)
        {
            m_fileLastUsed[cacheIndex] = AZStd::chrono::steady_clock::time_point();
            m_fileHandles[cacheIndex].reset();
            m_filePaths[cacheIndex].Clear();
        }
    }

    void StorageDrive::FlushEntireCache()
    {
        size_t numFiles = m_filePaths.size();
        for (size_t i = 0; i < numFiles; ++i)
        {
            m_fileLastUsed[i] = AZStd::chrono::steady_clock::time_point();
            m_fileHandles[i].reset();
            m_filePaths[i].Clear();
        }
    }

    size_t StorageDrive::FindFileInCache(const RequestPath& filePath) const
    {
        size_t numFiles = m_filePaths.size();
        for (size_t i = 0; i < numFiles; ++i)
        {
            if (m_filePaths[i] == filePath)
            {
                return i;
            }
        }
        return s_fileNotFound;
    }

    void StorageDrive::CollectStatistics(AZStd::vector<Statistic>& statistics) const
    {
        using DoubleSeconds = AZStd::chrono::duration<double>;

        if (m_readSizeAverage.GetTotal() > 1) // A default value is always added.
        {
            u64 totalBytesRead = m_readSizeAverage.GetTotal();
            double totalReadTimeSec = AZStd::chrono::duration_cast<DoubleSeconds>(m_readTimeAverage.GetTotal()).count();
            statistics.push_back(Statistic::CreateBytesPerSecond(
                m_name, "Read Speed", totalBytesRead / totalReadTimeSec,
                "The average read speed this drive achieved. This is the maximum achievable speed for reading from "
                "disk. If this is lower than expected it may indicate that there's an overhead from the operating system, the drive has "
                "seen a lot of use, other applications are using the same drive and/or anti-virus scans are slowing down reads."));
        }

        if (m_fileOpenCloseTimeAverage.GetNumRecorded() > 0)
        {
            statistics.push_back(Statistic::CreateTimeRange(
                m_name, "File Open & Close", m_fileOpenCloseTimeAverage.CalculateAverage(), m_fileOpenCloseTimeAverage.GetMinimum(),
                m_fileOpenCloseTimeAverage.GetMaximum(),
                "The average amount of time needed to open and close file handles. This is a fixed cost from the operating "
                "system. This can be mitigated running from archives."));
            statistics.push_back(Statistic::CreateTimeRange(
                m_name, "Get file exists", m_getFileExistsTimeAverage.CalculateAverage(), m_getFileExistsTimeAverage.GetMinimum(),
                m_getFileExistsTimeAverage.GetMaximum(),
                "The average amount of time in microseconds needed to check if a file exists. This is a fixed cost from the operating "
                "system. This can be mitigated running from archives."));
            statistics.push_back(Statistic::CreateTimeRange(
                m_name, "Get file meta data", m_getFileMetaDataTimeAverage.CalculateAverage(), m_getFileMetaDataTimeAverage.GetMinimum(),
                m_getFileMetaDataTimeAverage.GetMaximum(),
                "The average amount of time in microseconds needed to retrieve file information. This is a fixed cost from the operating "
                "system. This can be mitigated running from archives."));
            statistics.push_back(Statistic::CreateInteger(
                m_name, "Available slots", s64{ s_maxRequests } - m_pendingRequests.size(),
                "The total number of available slots to queue requests on. The lower this number, the more active this node is. A small "
                "negative number is ideal as it means there are a few requests available for immediate processing next once a request "
                "completes."));
        }
    }

    void StorageDrive::Report(const Requests::ReportData& data) const
    {
        switch (data.m_reportType)
        {
        case IStreamerTypes::ReportType::Config:
            data.m_output.push_back(Statistic::CreateInteger(
                m_name, "Max file handles", m_fileHandles.size(),
                "The maximum number of file handles this drive node will cache. Increasing this will allow files that are read "
                "multiple times to be processed faster. It's recommended to have this set to at least the largest number of archives "
                "that can be in use at the same time."));
            data.m_output.push_back(Statistic::CreateReferenceString(
                m_name, "Next node", m_next ? AZStd::string_view(m_next->GetName()) : AZStd::string_view("<None>"),
                "The name of the node that follows this node or none."));
            break;
        case IStreamerTypes::ReportType::FileLocks:
            for (u32 i = 0; i < m_fileHandles.size(); ++i)
            {
                if (m_fileHandles[i] != nullptr)
                {
                    data.m_output.push_back(
                        Statistic::CreatePersistentString(m_name, "File lock", m_filePaths[i].GetRelativePath().Native()));
                }
            }
            break;
        default:
            break;
        }
    }
} // namespace AZ::IO
