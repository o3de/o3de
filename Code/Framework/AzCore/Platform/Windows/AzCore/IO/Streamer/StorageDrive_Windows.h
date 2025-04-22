/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/PlatformIncl.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/IO/Streamer/RequestPath.h>
#include <AzCore/IO/Streamer/Statistics.h>
#include <AzCore/IO/Streamer/StreamerConfiguration.h>
#include <AzCore/IO/Streamer/StreamStackEntry.h>
#include <AzCore/std/containers/deque.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/chrono/chrono.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/string/string_view.h>
#include <AzCore/Statistics/RunningStatistic.h>

namespace AZ::IO::Requests
{
    struct ReadData;
    struct ReportData;
}

namespace AZ::IO
{
    class StorageDriveWin
        : public StreamStackEntry
    {
    public:
        struct ConstructionOptions
        {
            ConstructionOptions();

            //! Whether or not the device has a cost for seeking, such as happens on platter disks. This
            //! will be accounted for when predicting file reads.
            u8 m_hasSeekPenalty : 1;
            //! Use unbuffered reads for the fastest possible read speeds by bypassing the Windows
            //! file cache. This results in a faster read the first time a file is read, but subsequent reads will possibly be
            //! slower as those could have been serviced from the faster OS cache. During development or for games that reread
            //! files frequently it's recommended to set this option to false, but generally it's best to be turned on.
            //! Unbuffered reads have alignment restrictions. Many of the other stream stack entry are (optionally) aware and
            //! make adjustments. For the most optimal performance align read buffers to the physicalSectorSize.
            u8 m_enableUnbufferedReads : 1;
            //! Globally enable file sharing. This allows files to used outside AZ::IO::Streamer, including other applications
            //! while in use by AZ::IO::Streamer.
            u8 m_enableSharing : 1;
            //! If true, only information that's explicitly requested or issues are reported. If false, status information
            //! such as when drives are created and destroyed is reported as well.
            u8 m_minimalReporting : 1;
        };

        //! Creates an instance of a storage device that's optimized for use on Windows.
        //! @param drivePaths The paths to the drives that are supported by this device. A single device can have multiple
        //!     logical disks.
        //! @param maxFileHandles The maximum number of file handles that are cached. Only a small number are needed when
        //!     running from archives, but it's recommended that a larger number are kept open when reading from loose files.
        //! @param maxMetaDataCacheEntires The maximum number of files to keep meta data, such as the file size, to cache. Only
        //!     a small number are needed when running from archives, but it's recommended that a larger number are kept open
        //!     when reading from loose files.
        //! @param physicalSectorSize The minimal sector size as instructed by the device. When unbuffered reads are used the output
        //!     buffer needs to be aligned to this value.
        //! @param logicalSectorSize The minimal sector size as instructed by the device. When unbuffered reads are used the
        //!     file size and read offset need to be aligned to this value.
        //! @param ioChannelCount The maximum number of requests that the IO controller driving the device supports. This value
        //!     will be capped by the maximum number of parallel requests that can be issued per thread.
        //! @param overCommit The number of additional slots that will be reported as available. This makes sure that there are
        //!     always a few requests pending to avoid starvation. An over-commit that is too large can negatively impact the
        //!     scheduler's ability to re-order requests for optimal read order. A negative value will under-commit and will
        //!     avoid saturating the IO controller which can be needed if the drive is used by other applications.
        //! @param options Additional configuration options. See ConstructionOptions for more details.
        StorageDriveWin(const AZStd::vector<AZStd::string_view>& drivePaths, u32 maxFileHandles, u32 maxMetaDataCacheEntries,
            size_t physicalSectorSize, size_t logicalSectorSize, u32 ioChannelCount, s32 overCommit, ConstructionOptions options);
        ~StorageDriveWin() override;

        void PrepareRequest(FileRequest* request) override;
        void QueueRequest(FileRequest* request) override;
        bool ExecuteRequests() override;

        void UpdateStatus(Status& status) const override;
        void UpdateCompletionEstimates(AZStd::chrono::steady_clock::time_point now, AZStd::vector<FileRequest*>& internalPending,
            StreamerContext::PreparedQueue::iterator pendingBegin, StreamerContext::PreparedQueue::iterator pendingEnd) override;

        void CollectStatistics(AZStd::vector<Statistic>& statistics) const override;

    protected:
        static const AZStd::chrono::microseconds s_averageSeekTime;

        inline static constexpr size_t InvalidFileCacheIndex = std::numeric_limits<size_t>::max();
        inline static constexpr size_t InvalidReadSlotIndex = std::numeric_limits<size_t>::max();
        inline static constexpr size_t InvalidMetaDataCacheIndex = std::numeric_limits<size_t>::max();

        struct FileReadStatus
        {
            OVERLAPPED m_overlapped{};
            size_t m_fileHandleIndex{ InvalidFileCacheIndex };
        };

        struct FileReadInformation
        {
            AZStd::chrono::steady_clock::time_point m_startTime;
            FileRequest* m_request{ nullptr };
            void* m_sectorAlignedOutput{ nullptr };    // Internally allocated buffer that is sector aligned.
            size_t m_copyBackOffset{ 0 };

            void AllocateAlignedBuffer(size_t size, size_t sectorSize);
            void Clear();
        };

        enum class OpenFileResult
        {
            FileOpened,
            RequestForwarded,
            CacheFull
        };

        OpenFileResult OpenFile(HANDLE& fileHandle, size_t& cacheSlot, FileRequest* request, const Requests::ReadData& data);
        bool ReadRequest(FileRequest* request);
        bool ReadRequest(FileRequest* request, size_t readSlot);
        bool CancelRequest(FileRequest* cancelRequest, FileRequestPtr& target);
        void FileExistsRequest(FileRequest* request);
        void FileMetaDataRetrievalRequest(FileRequest* request);
        size_t FindInFileHandleCache(const RequestPath& filePath) const;
        size_t FindAvailableFileHandleCacheIndex() const;
        size_t FindAvailableReadSlot();
        size_t FindInMetaDataCache(const RequestPath& filePath) const;
        size_t GetNextMetaDataCacheSlot();
        bool IsServicedByThisDrive(AZ::IO::PathView filePath) const;

        void EstimateCompletionTimeForRequest(FileRequest* request, AZStd::chrono::steady_clock::time_point& startTime,
            const RequestPath*& activeFile, u64& activeOffset) const;
        void EstimateCompletionTimeForRequestChecked(FileRequest* request,
            AZStd::chrono::steady_clock::time_point startTime, const RequestPath*& activeFile, u64& activeOffset) const;
        s32 CalculateNumAvailableSlots() const;

        void FlushCache(const RequestPath& filePath);
        void FlushEntireCache();

        bool FinalizeReads();
        void FinalizeSingleRequest(FileReadStatus& status, size_t readSlot, DWORD numBytesTransferred,
            bool isCanceled, bool encounteredError);

        void Report(const Requests::ReportData& data) const;

        TimedAverageWindow<s_statisticsWindowSize> m_fileOpenCloseTimeAverage;
        TimedAverageWindow<s_statisticsWindowSize> m_getFileExistsTimeAverage;
        TimedAverageWindow<s_statisticsWindowSize> m_getFileMetaDataRetrievalTimeAverage;
        TimedAverageWindow<s_statisticsWindowSize> m_readTimeAverage;
        AverageWindow<u64, float, s_statisticsWindowSize> m_readSizeAverage;
#if AZ_STREAMER_ADD_EXTRA_PROFILING_INFO
        AZ::Statistics::RunningStatistic m_fileSwitchPercentageStat;
        AZ::Statistics::RunningStatistic m_seekPercentageStat;
        AZ::Statistics::RunningStatistic m_directReadsPercentageStat;
#endif
        AZStd::chrono::steady_clock::time_point m_activeReads_startTime;

        AZStd::deque<FileRequest*> m_pendingReadRequests;
        AZStd::deque<FileRequest*> m_pendingRequests;

        AZStd::vector<FileReadInformation> m_readSlots_readInfo;
        AZStd::vector<FileReadStatus> m_readSlots_statusInfo;
        AZStd::vector<bool> m_readSlots_active;

        AZStd::vector<AZStd::chrono::steady_clock::time_point> m_fileCache_lastTimeUsed;
        AZStd::vector<RequestPath> m_fileCache_paths;
        AZStd::vector<HANDLE> m_fileCache_handles;
        AZStd::vector<u16> m_fileCache_activeReads;

        AZStd::vector<RequestPath> m_metaDataCache_paths;
        AZStd::vector<u64> m_metaDataCache_fileSize;

        AZStd::vector<AZStd::string> m_drivePaths;

        size_t m_activeReads_ByteCount{ 0 };

        size_t m_physicalSectorSize{ 0 };
        size_t m_logicalSectorSize{ 0 };
        size_t m_activeCacheSlot{ InvalidFileCacheIndex };
        size_t m_metaDataCache_front{ 0 };
        u64 m_activeOffset{ 0 };
        u32 m_maxFileHandles{ 1 };
        u32 m_ioChannelCount{ 1 };
        s32 m_overCommit{ 0 };

        u16 m_activeReads_Count{ 0 };

        ConstructionOptions m_constructionOptions;
        bool m_cachesInitialized{ false };
    };
} // namespace AZ::IO
