/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/IO/GenericStreams.h>
#include <AzCore/IO/IStreamerTypes.h>
#include <AzCore/IO/IStreamer.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/parallel/condition_variable.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Debug/Profiler.h>

namespace AZ::Data
{
    class AssetDataStream : public AZ::IO::GenericStream
    {
    public:
        // The default Generic Stream APIs in this class will only allow for a single sequential pass
        // through the data, no seeking.  Reads will block when pages aren't available yet, and
        // pages will be marked for recycling once reading has progressed beyond them.

        //! Construct a new AssetDataStream
        explicit AssetDataStream(AZ::IO::IStreamerTypes::RequestMemoryAllocator* bufferAllocator = nullptr);
        ~AssetDataStream() override;

        // Open the AssetDataStream and make a copy of the provided memory buffer.
        void Open(const AZStd::vector<AZ::u8>& data);

        // Open the AssetDataStream and directly take ownership of a pre-populated memory buffer.
        void Open(AZStd::vector<AZ::u8>&& data);

        // Open the AssetDataStream and load it via file streaming
        using OnCompleteCallback = AZStd::function<void(AZ::IO::IStreamerTypes::RequestStatus)>;
        void Open(const AZStd::string& filePath, size_t fileOffset, size_t assetSize,
            AZStd::chrono::milliseconds deadline = AZ::IO::IStreamerTypes::s_noDeadline,
            AZ::IO::IStreamerTypes::Priority priority = AZ::IO::IStreamerTypes::s_priorityMedium,
            OnCompleteCallback loadCallback = {});

        // Reschedule the outstanding request.  Will only update with shorter deadline values or higher priority values
        void Reschedule(AZStd::chrono::milliseconds newDeadline, AZ::IO::IStreamerTypes::Priority newPriority);

        // Optionally block until the Open and data load has completed.
        void BlockUntilLoadComplete();

        // GenericStream APIs

        bool IsOpen() const override { return m_isOpen && IsFullyLoaded(); }
        bool CanSeek() const override { return false; }
        bool CanRead() const override { return true; }
        bool CanWrite() const override { return false; }

        void Seek(AZ::IO::OffsetType bytes, AZ::IO::GenericStream::SeekMode mode) override;

        AZ::IO::SizeType Write([[maybe_unused]] AZ::IO::SizeType bytes, [[maybe_unused]] const void* iBuffer) override
        {
            AZ_Assert(false, "Writing is not supported in AssetDataStream.");
            return 0;
        }

        AZ::IO::SizeType Read(AZ::IO::SizeType bytes, void* oBuffer) override;

        AZ::IO::SizeType GetCurPos() const override { return m_curOffset; }
        AZ::IO::SizeType GetLength() const override { return m_requestedAssetSize; }
        void Close() override;

        const char* GetFilename() const override { return m_filePath.c_str(); }

        // AssetDataStream specific APIs

        //! Whether or not all data has been loaded.
        bool IsFullyLoaded() const { return m_isOpen && (m_loadedSize == m_requestedAssetSize); }

        //! Gets the size of data loaded (so far).
        size_t GetLoadedSize() const { return m_loadedSize; }

        //! Request a cancellation of any current IO streamer requests.
        //! Note: This is asynchronous and not guaranteed to cancel if the request is already in-process.
        void RequestCancel();

    private:
        //! Perform any operations needed by all variants of Open()
        void OpenInternal(size_t assetSize, const char* streamName);

        void ClearInternalStateData();

        //! The allocator to use for allocating / deallocating asset buffers
        AZ::IO::IStreamerTypes::RequestMemoryAllocator* m_bufferAllocator{ nullptr };

        //! The default allocator to use if no specialized allocators are passed in.
        AZ::IO::IStreamerTypes::DefaultRequestMemoryAllocator m_defaultAllocator;

        //! The path and file name of the asset being loaded
        AZStd::string m_filePath;

        //! The offset into the file to start loading at.  
        size_t m_fileOffset{ 0 };

        //! The amount of data that's expected to be loaded.
        size_t m_requestedAssetSize{ 0 };

        //! Optional data buffer that's been directly passed in through Open(), instead of reading data from a file.
        AZStd::vector<AZ::u8> m_preloadedData;

        //! The buffer that will hold the raw data after it's loaded from the file.
        void* m_buffer{ nullptr };

        //! The amount of data that's been loaded. This can differ from the requested size if for
        //! instance a problem was encountered during loading.
        size_t m_loadedSize{ 0 };

        //! The current offset representing how far we've read into the buffer.
        size_t m_curOffset{ 0 };

        //! The current active streamer read request - tracked in case we need to cancel it prematurely
        AZ::IO::FileRequestPtr m_curReadRequest{ nullptr };

        //! The current request deadline.  Used to avoid requesting a reschedule to the same (current) deadline.
        AZStd::chrono::milliseconds m_curDeadline{ AZ::IO::IStreamerTypes::s_noDeadline };

        //! The current request priority.  Used to avoid requesting a reschedule to the same (current) priority.
        AZ::IO::IStreamerTypes::Priority m_curPriority{ AZ::IO::IStreamerTypes::s_priorityMedium };

        //! Synchronization for the read request, so that it's possible to block until completion.
        AZStd::mutex m_readRequestMutex;
        AZStd::condition_variable m_readRequestActive;

        //! Track whether or not the stream is currently open
        bool m_isOpen{ false };

    };

} // AZ::Data

