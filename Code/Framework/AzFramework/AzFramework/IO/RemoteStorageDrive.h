/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/IO/Streamer/Statistics.h>
#include <AzCore/IO/Streamer/StreamerConfiguration.h>
#include <AzCore/IO/Streamer/StreamStackEntry.h>
#include <AzCore/std/containers/deque.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/chrono/chrono.h>
#include <AzFramework/IO/RemoteFileIO.h>

namespace AZ::IO
{
    class RequestPath;
    namespace Requests
    {
        struct ReportData;
    }
}

namespace AzFramework
{
    struct RemoteStorageDriveConfig final :
        public AZ::IO::IStreamerStackConfig
    {
        AZ_RTTI(AzFramework::RemoteStorageDriveConfig, "{A37AC957-AC1F-4CAC-BB7B-0CAD5ABAD416}", AZ::IO::IStreamerStackConfig);
        AZ_CLASS_ALLOCATOR(RemoteStorageDriveConfig, AZ::SystemAllocator);

        ~RemoteStorageDriveConfig() override = default;
        AZStd::shared_ptr<AZ::IO::StreamStackEntry> AddStreamStackEntry(
            const AZ::IO::HardwareInformation& hardware, AZStd::shared_ptr<AZ::IO::StreamStackEntry> parent) override;
        static void Reflect(AZ::ReflectContext* context);

        AZ::u32 m_maxFileHandles{ 1024 };
    };

    class RemoteStorageDrive
        : public AZ::IO::StreamStackEntry
    {
    public:
        explicit RemoteStorageDrive(AZ::u32 maxFileHandles);
        ~RemoteStorageDrive() override;

        void PrepareRequest(AZ::IO::FileRequest* request) override;
        void QueueRequest(AZ::IO::FileRequest* request) override;
        bool ExecuteRequests() override;

        void UpdateStatus(Status& status) const override;
        void UpdateCompletionEstimates(AZStd::chrono::steady_clock::time_point now,
            AZStd::vector<AZ::IO::FileRequest*>& internalPending,
            AZ::IO::StreamerContext::PreparedQueue::iterator pendingBegin,
            AZ::IO::StreamerContext::PreparedQueue::iterator pendingEnd) override;

        void CollectStatistics(AZStd::vector<AZ::IO::Statistic>& statistics) const override;

    protected:
        static constexpr AZ::s32 s_maxRequests = 1;

        void ReadFile(AZ::IO::FileRequest* request);
        bool CancelRequest(AZ::IO::FileRequest* cancelRequest, AZ::IO::FileRequestPtr& target);
        void FileExistsRequest(AZ::IO::FileRequest* request);
        void FileMetaDataRetrievalRequest(AZ::IO::FileRequest* request);
        size_t FindFileInCache(const AZ::IO::RequestPath& filePath) const;
        void EstimateCompletionTimeForRequest(AZ::IO::FileRequest* request, AZStd::chrono::steady_clock::time_point& startTime,
            const AZ::IO::RequestPath*& activeFile) const;
        void FlushCache(const AZ::IO::RequestPath& filePath);
        void FlushEntireCache();
        void Report(const AZ::IO::Requests::ReportData& data) const;

        AZ::IO::RemoteFileIO m_fileIO;
        AZ::IO::TimedAverageWindow<AZ::IO::s_statisticsWindowSize> m_fileOpenCloseTimeAverage;
        AZ::IO::TimedAverageWindow<AZ::IO::s_statisticsWindowSize> m_getFileExistsTimeAverage;
        AZ::IO::TimedAverageWindow<AZ::IO::s_statisticsWindowSize> m_getFileMetaDataTimeAverage;
        AZ::IO::TimedAverageWindow<AZ::IO::s_statisticsWindowSize> m_readTimeAverage;
        AZ::IO::AverageWindow<AZ::u64, float, AZ::IO::s_statisticsWindowSize> m_readSizeAverage;
        AZStd::deque<AZ::IO::FileRequest*> m_pendingRequests;

        AZStd::vector<AZStd::chrono::steady_clock::time_point> m_fileLastUsed;
        AZStd::vector<AZ::IO::RequestPath> m_filePaths;
        AZStd::vector<AZ::IO::HandleType> m_fileHandles;

        size_t m_activeCacheSlot = AZ::IO::s_fileNotFound;
    };
} // namespace AzFramework
