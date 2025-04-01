/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Asset/AssetDataStream.h>
#include <AzCore/Memory/Memory.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Debug/Profiler.h>
#include <AzCore/IO/IStreamer.h>

#include <AzCore/IO/Streamer/FileRequest.h>
#include <AzCore/std/parallel/condition_variable.h>

namespace AZ::Data
{
    namespace DataStreamInternal
    {
        struct AssetDataStreamPrivate
        {
            AZ_CLASS_ALLOCATOR(AssetDataStreamPrivate, SystemAllocator);
            //! Optional data buffer that's been directly passed in through Open(), instead of reading data from a file.
            AZStd::vector<AZ::u8> m_preloadedData;
            //! The current active streamer read request - tracked in case we need to cancel it prematurely
            AZ::IO::FileRequestPtr m_curReadRequest{ nullptr };

            //! Synchronization for the read request, so that it's possible to block until completion.
            AZStd::mutex m_readRequestMutex;
            AZStd::condition_variable m_readRequestActive;

            void SetReadRequest(AZ::IO::FileRequestPtr&& req)
            {
                AZStd::scoped_lock<AZStd::mutex> lock(m_readRequestMutex);
                // The read request finished, so stop tracking it.
                m_curReadRequest = AZStd::move(req);
            }
            void BlockUntilReadComplete()
            {
                AZStd::unique_lock lock(m_readRequestMutex);
                m_readRequestActive.wait(
                    lock,
                    [this]
                    {
                        return m_curReadRequest == nullptr;
                    });
                lock.unlock();
            }
            void CancelRequest()
            {
                AZStd::scoped_lock<AZStd::mutex> lock(m_readRequestMutex);
                if (m_curReadRequest)
                {
                    auto streamer = Interface<IO::IStreamer>::Get();
                    m_curReadRequest = streamer->Cancel(m_curReadRequest);
                }
            }
        };
    } // namespace Internal

    AssetDataStream::AssetDataStream(AZ::IO::IStreamerTypes::RequestMemoryAllocator* bufferAllocator)
        : m_privateData(AZStd::make_unique<DataStreamInternal::AssetDataStreamPrivate>())
        , m_bufferAllocator(bufferAllocator ? bufferAllocator : &m_defaultAllocator)
    {
        ClearInternalStateData();
    }

    AssetDataStream::~AssetDataStream()
    {
        if (m_isOpen)
        {
            Close();
        }
    }


    void AssetDataStream::Open(const AZStd::vector<AZ::u8>& data)
    {
        AZ_PROFILE_FUNCTION(AzCore);

        AZ_Assert(!m_isOpen, "Attempting to open the stream when it is already open.");

        // Initialize the state variables and start tracking the overall load timings
        OpenInternal(data.size(), "(mem buffer)");

        // Create the asset buffer
        auto result = m_bufferAllocator->Allocate(data.size(), data.size(), AZCORE_GLOBAL_NEW_ALIGNMENT);
        m_buffer = result.m_address;
        m_loadedSize = result.m_size;

        // "Load" the asset buffer by copying the provided data buffer
        memcpy(m_buffer, data.data(), m_loadedSize);
    }

    void AssetDataStream::Open(AZStd::vector<AZ::u8>&& data)
    {
        AZ_PROFILE_FUNCTION(AzCore);

        AZ_Assert(!m_isOpen, "Attempting to open the stream when it is already open.");

        // Initialize the state variables and start tracking the overall load timings
        OpenInternal(data.size(), "(mem buffer)");

        // Directly take ownership of the provided buffer
        m_privateData->m_preloadedData = AZStd::move(data);
        m_buffer = m_privateData->m_preloadedData.data();
        m_loadedSize = m_privateData->m_preloadedData.size();
    }

    void AssetDataStream::Open(const AZStd::string& filePath, size_t fileOffset, size_t assetSize,
        AZ::IO::IStreamerTypes::Deadline deadline, AZ::IO::IStreamerTypes::Priority priority, OnCompleteCallback loadCallback)
    {
        AZ_PROFILE_FUNCTION(AzCore);

        AZ_Assert(!m_isOpen, "Attempting to open the stream when it is already open.");
        AZ_Assert(!m_privateData->m_curReadRequest, "Queueing an asset stream load while one is still in progress.");
        AZ_Assert(!filePath.empty(), "AssetDataStream::Open called without a valid file name.");

        // Initialize the state variables and start tracking the overall load timings
        OpenInternal(assetSize, filePath.c_str());

        m_filePath = filePath;
        m_fileOffset = fileOffset;

        // If the asset load is requesting more than 0 bytes of data, queue it up with the file streamer.
        if (m_requestedAssetSize > 0)
        {
            // Set up the callback that will process the asset data once the raw file load is finished.
            auto streamerCallback = [this, loadCallback](AZ::IO::FileRequestHandle fileHandle)
            {
                AZ_PROFILE_SCOPE(AzCore, "AZ::Data::LoadAssetDataStreamCallback %s",
                    m_filePath.c_str());

                // Get the results
                auto streamer = AZ::Interface<AZ::IO::IStreamer>::Get();
                AZ::u64 bytesRead = 0;
                streamer->GetReadRequestResult(fileHandle, m_buffer, bytesRead,
                                               AZ::IO::IStreamerTypes::ClaimMemory::Yes);
                auto status = streamer->GetRequestStatus(fileHandle);
                m_loadedSize = aznumeric_cast<size_t>(bytesRead);

                // Validate that our read request generated expected results.
                AZ_Assert(m_buffer, "Streamer provided a null buffer in the file read callback for %s.", m_filePath.c_str());
                AZ_Error("AssetDataStream", m_loadedSize == m_requestedAssetSize,
                         "Buffer for %s was expected to be %zu bytes, but is %zu bytes.",
                         m_filePath.c_str(), m_requestedAssetSize, m_loadedSize);

                // The read request finished, so stop tracking it.
                m_privateData->SetReadRequest(nullptr);

                // Call the load callback to start processing the loaded data.
                if (loadCallback)
                {
                    loadCallback(status);
                }
                else
                {
                    AZ_Error("AssetDataStream", status == AZ::IO::IStreamerTypes::RequestStatus::Completed,
                        "AssetDataStream failed to load %s", m_filePath.c_str());
                }

                // Notify that the load is complete, in case anyone is using BlockUntilLoadComplete to block.
                m_privateData->m_readRequestActive.notify_one();
            };

            // Queue the raw file load with the file streamer.
            auto streamer = AZ::Interface<AZ::IO::IStreamer>::Get();
            m_privateData->m_curReadRequest =
                streamer->Read(
                m_filePath,
                *m_bufferAllocator,
                m_requestedAssetSize,
                deadline, priority, m_fileOffset);
            m_curDeadline = deadline;
            m_curPriority = priority;
            streamer->SetRequestCompleteCallback(m_privateData->m_curReadRequest, streamerCallback);

            streamer->QueueRequest(m_privateData->m_curReadRequest);
        }
        else
        {
            // If 0 bytes are requested, skip the file streamer entirely, and just directly call the load callback.
            if (loadCallback)
            {
                loadCallback(AZ::IO::IStreamerTypes::RequestStatus::Completed);
            }

            m_privateData->m_readRequestActive.notify_one();
        }
    }

    void AssetDataStream::Reschedule(AZ::IO::IStreamerTypes::Deadline newDeadline, AZ::IO::IStreamerTypes::Priority newPriority)
    {
        if (m_privateData->m_curReadRequest && (newDeadline < m_curDeadline || newPriority > m_curPriority))
        {
            auto deadline = AZStd::GetMin(m_curDeadline, newDeadline);
            auto priority = AZStd::GetMax(m_curPriority, newPriority);

            auto streamer = Interface<IO::IStreamer>::Get();
            m_privateData->m_curReadRequest = streamer->RescheduleRequest(m_privateData->m_curReadRequest, deadline, priority);
            m_curDeadline = deadline;
            m_curPriority = priority;
        }
    }

    void AssetDataStream::BlockUntilLoadComplete()
    {
        m_privateData->BlockUntilReadComplete();
    }

    void AssetDataStream::ClearInternalStateData()
    {
        // Clear all our internal state data.
        m_privateData->m_preloadedData.resize(0);
        m_buffer = nullptr;
        m_loadedSize = 0;
        m_requestedAssetSize = 0;
        m_curOffset = 0;
        m_filePath.clear();
        m_fileOffset = 0;
        m_isOpen = false;
    }

    void AssetDataStream::OpenInternal(size_t assetSize, [[maybe_unused]] const char* streamName)
    {
        // Due to a bug, we need to create a superfluous profile interval here, because for some reason
        // the real interval we want to record below won't show up unless this is here.
        /**/
        {
            AZ_PROFILE_INTERVAL_START(AzCore, this + 1, "AssetDataStream: %s", streamName);
            AZ_PROFILE_INTERVAL_END(AzCore, this + 1);
        }
        /**/

        // Start a timespan marker to track the full load time for the requested asset.
        AZ_PROFILE_INTERVAL_START(AzCore, this, "AssetLoad: %s", streamName);

        // Lock the allocator to ensure it remains active from Open to Close.
        m_bufferAllocator->LockAllocator();

        // Init all the tracking variables.
        ClearInternalStateData();

        m_requestedAssetSize = assetSize;
        m_isOpen = true;
    }

    void AssetDataStream::Close()
    {
        AZ_Assert(m_isOpen, "Attempting to close a stream that hasn't been opened.");
        AZ_Assert(m_privateData->m_curReadRequest == nullptr, "Attempting to close a stream with a read request in flight.");

        // Destroy the asset buffer and unlock the allocator, so the allocator itself knows that it is no longer needed.
        if (m_buffer != m_privateData->m_preloadedData.data())
        {
            m_bufferAllocator->Release(m_buffer);
        }
        m_bufferAllocator->UnlockAllocator();

        ClearInternalStateData();

        // End the load time timespan marker for this asset.
        AZ_PROFILE_INTERVAL_END(AzCore, this);
    }

    void AssetDataStream::RequestCancel()
    {
        m_privateData->CancelRequest();
    }

    void AssetDataStream::Seek(AZ::IO::OffsetType bytes, AZ::IO::GenericStream::SeekMode mode)
    {
        AZ::IO::OffsetType requestedOffset = 0;

        switch (mode)
        {
            case ST_SEEK_BEGIN:
                requestedOffset = bytes;
                break;
            case ST_SEEK_CUR:
                requestedOffset = aznumeric_cast<AZ::IO::OffsetType>(m_curOffset) + bytes;
                break;
            case ST_SEEK_END:
                requestedOffset = aznumeric_cast<AZ::IO::OffsetType>(m_loadedSize) + bytes;
                break;
        }

        size_t calculatedOffset = aznumeric_cast<size_t>(AZ::GetMax(aznumeric_cast<AZ::IO::OffsetType>(0), requestedOffset));
        if (calculatedOffset >= m_curOffset)
        {
            m_curOffset = calculatedOffset;
        }
        else
        {
            AZ_Assert(false, "Backwards seeking is not allowed in AssetDataStream, since previously-read data might be paged out "
                      "of memory.  Current stream offset is %zu, requested offset is %zu.", m_curOffset, calculatedOffset);
        }
    }

    AZ::IO::SizeType AssetDataStream::Read(AZ::IO::SizeType bytes, void* oBuffer)
    {
        if (m_curOffset >= m_loadedSize)
        {
            return 0;
        }

        bytes = AZ::GetMin(bytes, aznumeric_cast<AZ::IO::SizeType>(m_loadedSize - m_curOffset));
        if (bytes)
        {
            memcpy(oBuffer, reinterpret_cast<AZ::u8*>(m_buffer) + m_curOffset, aznumeric_cast<size_t>(bytes));
            m_curOffset += aznumeric_cast<size_t>(bytes);
        }
        return bytes;
    }

} // AZ::Data

