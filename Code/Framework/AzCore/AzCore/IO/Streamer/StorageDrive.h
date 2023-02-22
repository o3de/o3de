/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/IO/Streamer/RequestPath.h>
#include <AzCore/IO/Streamer/Statistics.h>
#include <AzCore/IO/Streamer/StreamerConfiguration.h>
#include <AzCore/IO/Streamer/StreamStackEntry.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/std/containers/deque.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/chrono/chrono.h>

namespace AZ::IO::Requests
{
    struct ReportData;
}

namespace AZ::IO
{
    struct StorageDriveConfig final :
        public IStreamerStackConfig
    {
        AZ_RTTI(AZ::IO::StorageDriveConfig, "{3D568902-6C09-4E9E-A4DB-8B561481D298}", IStreamerStackConfig);
        AZ_CLASS_ALLOCATOR(StorageDriveConfig, AZ::SystemAllocator);

        ~StorageDriveConfig() override = default;
        AZStd::shared_ptr<StreamStackEntry> AddStreamStackEntry(
            const HardwareInformation& hardware, AZStd::shared_ptr<StreamStackEntry> parent) override;
        static void Reflect(AZ::ReflectContext* context);

        u32 m_maxFileHandles{1024};
    };

    //! Platform agnostic version of a storage drive, such as hdd, ssd, dvd, etc.
    //! This stream stack entry is responsible for accessing a storage drive to
    //! retrieve file information and data.
    //! This entry is designed as a catch-all for any reads that weren't handled
    //! by platform specific implementations or the virtual file system. It should
    //! by the last entry in the stack as it will not forward calls to the next entry.
    class StorageDrive
        : public StreamStackEntry
    {
    public:
        explicit StorageDrive(u32 maxFileHandles);
        ~StorageDrive() override = default;

        void SetNext(AZStd::shared_ptr<StreamStackEntry> next) override;

        void PrepareRequest(FileRequest* request) override;
        void QueueRequest(FileRequest* request) override;
        bool ExecuteRequests() override;

        void UpdateStatus(Status& status) const override;
        void UpdateCompletionEstimates(AZStd::chrono::steady_clock::time_point now, AZStd::vector<FileRequest*>& internalPending,
            StreamerContext::PreparedQueue::iterator pendingBegin, StreamerContext::PreparedQueue::iterator pendingEnd) override;

        void CollectStatistics(AZStd::vector<Statistic>& statistics) const override;

    protected:
        static const AZStd::chrono::microseconds s_averageSeekTime;
        static constexpr s32 s_maxRequests = 1;

        size_t FindFileInCache(const RequestPath& filePath) const;
        void ReadFile(FileRequest* request);
        void CancelRequest(FileRequest* cancelRequest, FileRequestPtr& target);
        void FileExistsRequest(FileRequest* request);
        void FileMetaDataRetrievalRequest(FileRequest* request);
        void FlushCache(const RequestPath& filePath);
        void FlushEntireCache();

        void EstimateCompletionTimeForRequest(FileRequest* request, AZStd::chrono::steady_clock::time_point& startTime,
            const RequestPath*& activeFile, u64& activeOffset) const;

        void Report(const Requests::ReportData& data) const;

        TimedAverageWindow<s_statisticsWindowSize> m_fileOpenCloseTimeAverage;
        TimedAverageWindow<s_statisticsWindowSize> m_getFileExistsTimeAverage;
        TimedAverageWindow<s_statisticsWindowSize> m_getFileMetaDataTimeAverage;
        TimedAverageWindow<s_statisticsWindowSize> m_readTimeAverage;
        AverageWindow<u64, float, s_statisticsWindowSize> m_readSizeAverage;
        //! File requests that are queued for processing.
        AZStd::deque<FileRequest*> m_pendingRequests;

        //! The last time a file handle was used to access a file. The handle is stored in m_fileHandles.
        AZStd::vector<AZStd::chrono::steady_clock::time_point> m_fileLastUsed;
        //! The file path to the file handle. The handle is stored in m_fileHandles.
        AZStd::vector<RequestPath> m_filePaths;
        //! A list of file handles that's being cached in case they're needed again in the future.
        AZStd::vector<AZStd::unique_ptr<SystemFile>> m_fileHandles;

        //! The offset into the file that's cached by the active cache slot.
        u64 m_activeOffset = 0;
        //! The index into m_fileHandles for the file that's currently being read.
        size_t m_activeCacheSlot = s_fileNotFound;
    };
} // namespace AZ::IO
