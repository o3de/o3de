/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <inttypes.h>
#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/Debug/Profiler.h>
#include <AzCore/IO/Streamer/FileRequest.h>
#include <AzCore/IO/Streamer/Statistics.h>
#include <AzCore/IO/Streamer/StreamerContext.h>
#include <AzCore/Settings/SettingsRegistry.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/std/typetraits/decay.h>
#include <AzFramework/IO/RemoteStorageDrive.h>

namespace AzFramework
{
    AZStd::shared_ptr<AZ::IO::StreamStackEntry> RemoteStorageDriveConfig::AddStreamStackEntry(
        [[maybe_unused]] const AZ::IO::HardwareInformation& hardware, AZStd::shared_ptr<AZ::IO::StreamStackEntry> parent)
    {
        AZ::SettingsRegistryInterface* settingsRegistry = AZ::SettingsRegistry::Get();
        AZ::s64 allowRemoteFilesystem{};

        AZ::SettingsRegistryMergeUtils::PlatformGet(*settingsRegistry, allowRemoteFilesystem,
            AZ::SettingsRegistryMergeUtils::BootstrapSettingsRootKey, "remote_filesystem");
        if (allowRemoteFilesystem != 0)
        {
            auto result = AZStd::make_shared<AzFramework::RemoteStorageDrive>(m_maxFileHandles);
            result->SetNext(AZStd::move(parent));
            return result;
        }
        else
        {
            return parent;
        }
    }

    void RemoteStorageDriveConfig::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context); serializeContext != nullptr)
        {
            serializeContext->Class<RemoteStorageDriveConfig, AZ::IO::IStreamerStackConfig>()
                ->Version(1)
                ->Field("MaxFileHandles", &RemoteStorageDriveConfig::m_maxFileHandles);
        }
    }

    RemoteStorageDrive::RemoteStorageDrive(AZ::u32 maxFileHandles)
        : StreamStackEntry("Storage drive(VFS)")
    {
        m_fileLastUsed.resize(maxFileHandles, AZStd::chrono::steady_clock::time_point::min());
        m_filePaths.resize(maxFileHandles);
        m_fileHandles.resize(maxFileHandles, AZ::IO::InvalidHandle);

        // Add initial dummy values to the stats to avoid division by zero later on and avoid needing branches.
        m_readSizeAverage.PushEntry(1);
        m_readTimeAverage.PushEntry(AZStd::chrono::microseconds(1));
    }

    RemoteStorageDrive::~RemoteStorageDrive()
    {
        using namespace AZ::IO;

        for (HandleType handle : m_fileHandles)
        {
            if (handle != InvalidHandle)
            {
                m_fileIO.Close(handle);
            }
        }
    }

    void RemoteStorageDrive::PrepareRequest(AZ::IO::FileRequest* request)
    {
        using namespace AZ::IO;

        AZ_PROFILE_FUNCTION(AzCore);
        AZ_Assert(request, "PrepareRequest was provided a null request.");

        if (AZStd::holds_alternative<Requests::ReadRequestData>(request->GetCommand()))
        {
            auto& readRequest = AZStd::get<Requests::ReadRequestData>(request->GetCommand());

            FileRequest* read = m_context->GetNewInternalRequest();
            read->CreateRead(request, readRequest.m_output, readRequest.m_outputSize, readRequest.m_path,
                readRequest.m_offset, readRequest.m_size);
            m_context->PushPreparedRequest(read);
            return;
        }
        StreamStackEntry::PrepareRequest(request);
    }

    void RemoteStorageDrive::QueueRequest(AZ::IO::FileRequest* request)
    {
        using namespace AZ::IO;

        AZ_Assert(request, "QueueRequest was provided a null request.");

        AZStd::visit([this, request](auto&& args)
        {
            using namespace AZ::IO;
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
                if (CancelRequest(request, args.m_target))
                {
                    // Only forward if this isn't part of the request chain, otherwise the storage device should
                    // be the last step as it doesn't forward any (sub)requests.
                    return;
                }
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

    bool RemoteStorageDrive::ExecuteRequests()
    {
        using namespace AZ::IO;

        if (!m_pendingRequests.empty())
        {
            FileRequest* request = m_pendingRequests.front();
            AZStd::visit([this, request](auto&& args)
            {
                using namespace AZ::IO;
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
            return StreamStackEntry::ExecuteRequests();
        }
    }

    void RemoteStorageDrive::UpdateStatus(Status& status) const
    {
        // Only participate if there are actually any reads done.
        if (m_fileOpenCloseTimeAverage.GetNumRecorded() > 0)
        {
            AZ::s32 availableSlots = s_maxRequests - aznumeric_cast<AZ::s32>(m_pendingRequests.size());
            StreamStackEntry::UpdateStatus(status);
            status.m_numAvailableSlots = AZStd::min(status.m_numAvailableSlots, availableSlots);
            status.m_isIdle = status.m_isIdle && m_pendingRequests.empty();
        }
    }

    void RemoteStorageDrive::UpdateCompletionEstimates(AZStd::chrono::steady_clock::time_point now,
        AZStd::vector<AZ::IO::FileRequest*>& internalPending, AZ::IO::StreamerContext::PreparedQueue::iterator pendingBegin,
        AZ::IO::StreamerContext::PreparedQueue::iterator pendingEnd)
    {
        using namespace AZ::IO;

        StreamStackEntry::UpdateCompletionEstimates(now, internalPending, pendingBegin, pendingEnd);

        const RequestPath* activeFile = nullptr;
        if (m_activeCacheSlot != s_fileNotFound)
        {
            activeFile = &m_filePaths[m_activeCacheSlot];
        }

        // Estimate requests in this stack entry.
        for (FileRequest* request : m_pendingRequests)
        {
            EstimateCompletionTimeForRequest(request, now, activeFile);
        }

        // Estimate internally pending requests. Because this call will go from the top of the stack to the bottom,
        // but estimation is calculated from the bottom to the top, this list should be processed in reverse order.
        for (auto requestIt = internalPending.rbegin(); requestIt != internalPending.rend(); ++requestIt)
        {
            EstimateCompletionTimeForRequest(*requestIt, now, activeFile);
        }

        // Estimate pending requests that have not been queued yet.
        for (auto requestIt = pendingBegin; requestIt != pendingEnd; ++requestIt)
        {
            EstimateCompletionTimeForRequest(*requestIt, now, activeFile);
        }
    }

    void RemoteStorageDrive::EstimateCompletionTimeForRequest(AZ::IO::FileRequest* request,
        AZStd::chrono::steady_clock::time_point& startTime, const AZ::IO::RequestPath*& activeFile) const
    {
        using namespace AZ::IO;

        AZ::u64 readSize = 0;
        const RequestPath* targetFile = nullptr;

        AZStd::visit([&](auto&& args)
        {
            using namespace AZ::IO;
            using Command = AZStd::decay_t<decltype(args)>;
            if constexpr (AZStd::is_same_v<Command, Requests::ReadData>)
            {
                targetFile = &args.m_path;
                readSize = args.m_size;
            }
            else if constexpr (AZStd::is_same_v<Command, Requests::CompressedReadData>)
            {
                targetFile = &args.m_compressionInfo.m_archiveFilename;
                readSize = args.m_compressionInfo.m_compressedSize;
            }
            else if constexpr (AZStd::is_same_v<Command, Requests::FileExistsCheckData>)
            {
                readSize = 0;
                AZStd::chrono::microseconds averageTime = m_getFileExistsTimeAverage.CalculateAverage();
                startTime += averageTime;
            }
            else if constexpr (AZStd::is_same_v<Command, Requests::FileMetaDataRetrievalData>)
            {
                readSize = 0;
                AZStd::chrono::microseconds averageTime = m_getFileMetaDataTimeAverage.CalculateAverage();
                startTime += averageTime;
            }
        }, request->GetCommand());

        if (readSize > 0)
        {
            if (activeFile && activeFile != targetFile)
            {
                if (FindFileInCache(*targetFile) == s_fileNotFound)
                {
                    AZStd::chrono::microseconds fileOpenCloseTimeAverage = m_fileOpenCloseTimeAverage.CalculateAverage();
                    startTime += fileOpenCloseTimeAverage;
                }
            }

            AZ::u64 totalBytesRead = m_readSizeAverage.GetTotal();
            double totalReadTime =
                aznumeric_caster(m_readTimeAverage.GetTotal().count());
            startTime += Statistic::TimeValue(aznumeric_cast<AZ::u64>((readSize * totalReadTime) / totalBytesRead));
        }
        request->SetEstimatedCompletion(startTime);
    }

    void RemoteStorageDrive::ReadFile(AZ::IO::FileRequest* request)
    {
        using namespace AZ::IO;

        AZ_PROFILE_FUNCTION(AzCore);

        auto data = AZStd::get_if<Requests::ReadData>(&request->GetCommand());
        AZ_Assert(data, "Request doing reading in the RemoteStorageDrive didn't contain read data.");

        HandleType file = InvalidHandle;

        // If the file is already open, use that file handle and update it's last touched time.
        size_t cacheIndex = FindFileInCache(data->m_path);
        if (cacheIndex != s_fileNotFound)
        {
            file = m_fileHandles[cacheIndex];
            m_fileLastUsed[cacheIndex] = AZStd::chrono::steady_clock::now();
        }

        // If the file is not open, eject the oldest entry from the cache and open the file for reading.
        if (file == InvalidHandle)
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
            if (!m_fileIO.Open(data->m_path.GetRelativePathCStr(), OpenMode::ModeRead, file))
            {
                StreamStackEntry::QueueRequest(request);
                return;
            }

            m_fileLastUsed[cacheIndex] = AZStd::chrono::steady_clock::now();
            if (m_fileHandles[cacheIndex] != InvalidHandle)
            {
                m_fileIO.Close(m_fileHandles[cacheIndex]);
            }
            m_fileHandles[cacheIndex] = file;
            m_filePaths[cacheIndex] = data->m_path;
        }
        m_activeCacheSlot = cacheIndex;

        AZ_Assert(
            file != InvalidHandle,
            "While searching for file '%s' RemoteStorageDevice::ReadFile encountered a problem that wasn't reported.",
            data->m_path.GetRelativePathCStr());
        {
            TIMED_AVERAGE_WINDOW_SCOPE(m_readTimeAverage);
            AZ::u64 currentOffset = 0;
            if (!m_fileIO.Tell(file, currentOffset))
            {
                AZ_Warning(
                    "IO", false, "RemoteIO failed to tell the offset for a valid file handle for file '%s'.",
                    data->m_path.GetRelativePathCStr());
                m_readSizeAverage.PushEntry(0);
                request->SetStatus(IStreamerTypes::RequestStatus::Failed);
                m_context->MarkRequestAsCompleted(request);
            }
            if (currentOffset != data->m_offset)
            {
                if (!m_fileIO.Seek(file, data->m_offset, SeekType::SeekFromStart))
                {
                    AZ_Warning(
                        "IO", false, "RemoteIO failed to tell to seek to %" PRIu64 " in '%s'.", data->m_offset,
                        data->m_path.GetRelativePathCStr());
                    m_readSizeAverage.PushEntry(0);
                    request->SetStatus(IStreamerTypes::RequestStatus::Failed);
                    m_context->MarkRequestAsCompleted(request);
                }
            }
            if (!m_fileIO.Read(file, data->m_output, data->m_size, true))
            {
                AZ_Warning("IO", false, "RemoteIO failed to read %i bytes at offset %" PRIu64 " from '%s'.",
                    data->m_size, data->m_offset, data->m_path.GetRelativePathCStr());
                m_readSizeAverage.PushEntry(0);
                request->SetStatus(IStreamerTypes::RequestStatus::Failed);
                m_context->MarkRequestAsCompleted(request);
            }
        }
        m_readSizeAverage.PushEntry(data->m_size);

        request->SetStatus(IStreamerTypes::RequestStatus::Completed);
        m_context->MarkRequestAsCompleted(request);
    }

    bool RemoteStorageDrive::CancelRequest(AZ::IO::FileRequest* cancelRequest, AZ::IO::FileRequestPtr& target)
    {
        using namespace AZ::IO;

        bool ownsRequestChain = false;
        for (auto it = m_pendingRequests.begin(); it != m_pendingRequests.end();)
        {
            if ((*it)->WorksOn(target))
            {
                (*it)->SetStatus(IStreamerTypes::RequestStatus::Canceled);
                m_context->MarkRequestAsCompleted(*it);
                it = m_pendingRequests.erase(it);
                ownsRequestChain = true;
            }
            else
            {
                ++it;
            }
        }

        if (ownsRequestChain)
        {
            cancelRequest->SetStatus(IStreamerTypes::RequestStatus::Completed);
            m_context->MarkRequestAsCompleted(cancelRequest);
        }

        return ownsRequestChain;
    }

    void RemoteStorageDrive::FileExistsRequest(AZ::IO::FileRequest* request)
    {
        using namespace AZ::IO;

        TIMED_AVERAGE_WINDOW_SCOPE(m_getFileExistsTimeAverage);

        auto& fileExists = AZStd::get<Requests::FileExistsCheckData>(request->GetCommand());
        size_t cacheIndex = FindFileInCache(fileExists.m_path);
        if (cacheIndex != s_fileNotFound)
        {
            fileExists.m_found = true;
            m_context->MarkRequestAsCompleted(request);
        }
        else
        {
            if (m_fileIO.Exists(fileExists.m_path.GetAbsolutePathCStr()))
            {
                fileExists.m_found = true;
                request->SetStatus(IStreamerTypes::RequestStatus::Completed);
                m_context->MarkRequestAsCompleted(request);
            }
            else
            {
                // File couldn't be found through VFS so let the next node try locally.
                StreamStackEntry::QueueRequest(request);
            }
        }
    }

    void RemoteStorageDrive::FileMetaDataRetrievalRequest(AZ::IO::FileRequest* request)
    {
        using namespace AZ::IO;

        AZ_PROFILE_FUNCTION(AzCore);
        TIMED_AVERAGE_WINDOW_SCOPE(m_getFileMetaDataTimeAverage);

        AZ::u64 fileSize = 0;
        bool found = false;

        auto& command = AZStd::get<Requests::FileMetaDataRetrievalData>(request->GetCommand());
        // If the file is already open, use the file handle which usually is cheaper than asking for the file by name.
        size_t cacheIndex = FindFileInCache(command.m_path);
        if (cacheIndex != s_fileNotFound)
        {
            AZ_Assert(
                m_fileHandles[cacheIndex] != InvalidHandle, "File path '%s' doesn't have an associated file handle.",
                m_filePaths[cacheIndex].GetRelativePathCStr());
            found = m_fileIO.Size(m_fileHandles[cacheIndex], fileSize);
        }
        else
        {
            // The file is not open yet, so try to get the file size by name.
            m_fileIO.Size(command.m_path.GetAbsolutePathCStr(), fileSize);
        }

        if (found)
        {
            command.m_fileSize = fileSize;
            command.m_found = true;
            request->SetStatus(IStreamerTypes::RequestStatus::Completed);
            m_context->MarkRequestAsCompleted(request);
        }
        else
        {
            StreamStackEntry::QueueRequest(request);
        }
    }

    void RemoteStorageDrive::FlushCache(const AZ::IO::RequestPath& filePath)
    {
        using namespace AZ::IO;

        size_t cacheIndex = FindFileInCache(filePath);
        if (cacheIndex != s_fileNotFound)
        {
            m_fileLastUsed[cacheIndex] = AZStd::chrono::steady_clock::time_point();
            m_filePaths[cacheIndex].Clear();
            AZ_Assert(
                m_fileHandles[cacheIndex] != AZ::IO::InvalidHandle, "File path '%s' doesn't have an associated file handle.",
                m_filePaths[cacheIndex].GetRelativePathCStr());
            m_fileIO.Close(m_fileHandles[cacheIndex]);
            m_fileHandles[cacheIndex] = InvalidHandle;
        }
    }

    void RemoteStorageDrive::FlushEntireCache()
    {
        size_t numFiles = m_filePaths.size();
        for (size_t i = 0; i < numFiles; ++i)
        {
            m_fileLastUsed[i] = AZStd::chrono::steady_clock::time_point();
            m_filePaths[i].Clear();
            if (m_fileHandles[i] != AZ::IO::InvalidHandle)
            {
                m_fileIO.Close(m_fileHandles[i]);
                m_fileHandles[i] = AZ::IO::InvalidHandle;
            }
        }
    }

    size_t RemoteStorageDrive::FindFileInCache(const AZ::IO::RequestPath& filePath) const
    {
        size_t numFiles = m_filePaths.size();
        for (size_t i = 0; i < numFiles; ++i)
        {
            if (m_filePaths[i] == filePath)
            {
                return i;
            }
        }
        return AZ::IO::s_fileNotFound;
    }

    void RemoteStorageDrive::CollectStatistics(AZStd::vector<AZ::IO::Statistic>& statistics) const
    {
        using namespace AZ::IO;

        using DoubleSeconds = AZStd::chrono::duration<double>;

        AZ::u64 totalBytesRead = m_readSizeAverage.GetTotal();
        double totalReadTimeSec = AZStd::chrono::duration_cast<DoubleSeconds>(m_readTimeAverage.GetTotal()).count();
        if (m_readSizeAverage.GetTotal() > 1) // A default is always added.
        {
            statistics.push_back(Statistic::CreateBytesPerSecond(m_name, "Read Speed", totalBytesRead / totalReadTimeSec,
                "The average read speed this node achieved. This is the maximum achievable speed for reading through "
                "the network. If this is lower than expected it may indicate that there's an overhead from the operating system, poor network "
                "connection or poor drive performance on the host."));
        }

        if (m_fileOpenCloseTimeAverage.GetNumRecorded() > 0)
        {
            statistics.push_back(Statistic::CreateTimeRange(
                m_name, "File Open & Close", m_fileOpenCloseTimeAverage.CalculateAverage(), m_fileOpenCloseTimeAverage.GetMinimum(),
                m_fileOpenCloseTimeAverage.GetMaximum(),
                "The average amount of time needed to open and close file handles. This is a fixed cost from the operating "
                "system. This can be mitigated running from archives."));
            statistics.push_back(Statistic::CreateTimeRange(
                m_name, "Get file exists (avg. us)", m_getFileExistsTimeAverage.CalculateAverage(),
                m_getFileExistsTimeAverage.GetMinimum(), m_getFileExistsTimeAverage.GetMaximum(),
                "The average amount of time needed to check if a file exists. This is a fixed cost from the operating "
                "system. This can be mitigated running from archives."));
            statistics.push_back(Statistic::CreateTimeRange(
                m_name, "Get file meta data", m_getFileMetaDataTimeAverage.CalculateAverage(), m_getFileMetaDataTimeAverage.GetMinimum(),
                m_getFileMetaDataTimeAverage.GetMaximum(),
                "The average amount of time needed to retrieve file information. This is a fixed cost from the operating "
                "system. This can be mitigated running from archives."));
            statistics.push_back(Statistic::CreateInteger(m_name, "Available slots", AZ::s64{ s_maxRequests } - m_pendingRequests.size(),
                "The total number of available slots to queue requests on. The lower this number, the more active this node is. A small "
                "negative number is ideal as it means there are a few requests available for immediate processing next once a request "
                "completes."));
        }

        StreamStackEntry::CollectStatistics(statistics);
    }

    void RemoteStorageDrive::Report(const AZ::IO::Requests::ReportData& data) const
    {
        using namespace AZ::IO;

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
            for (AZ::u32 i = 0; i < m_fileHandles.size(); ++i)
            {
                if (m_fileHandles[i] != InvalidHandle)
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
} // namespace AzFramework
