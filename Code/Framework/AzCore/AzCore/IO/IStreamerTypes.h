/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/base.h>
#include <AzCore/Memory/IAllocator.h>
#include <AzCore/Memory/Memory.h>
#include <AzCore/std/chrono/chrono.h>
#include <AzCore/std/parallel/atomic.h>

 // The user defined literals have to be in the header because there were linking issues with CrySystem.

constexpr AZ::u64 operator"" _kib(AZ::u64 value);
constexpr AZ::u64 operator"" _mib(AZ::u64 value);
constexpr AZ::u64 operator"" _gib(AZ::u64 value);

namespace AZ::IO::IStreamerTypes
{
    // Default value for commonly used priorities.
    inline constexpr static AZStd::chrono::microseconds s_deadlineNow = AZStd::chrono::microseconds::zero();
    inline constexpr static AZStd::chrono::microseconds s_noDeadline = AZStd::chrono::microseconds::max();

    // Default values for commonly used priorities
    using Priority = u8;
    inline constexpr static Priority s_priorityHighest = 255;
    inline constexpr static Priority s_priorityHigh = 191;
    inline constexpr static Priority s_priorityMedium = 127;
    inline constexpr static Priority s_priorityLow = 63;
    inline constexpr static Priority s_priorityLowest = 0;

    //! Provides configuration recommendations for using the file streaming system.
    struct Recommendations
    {
        //! The minimal memory alignment that's required to avoid intermediate buffers. If the memory
        //! provided to the read request isn't aligned to this size it may require a temporary or cached buffer
        //! to first read to and copy the result from to the provided memory.
        AZ::u64 m_memoryAlignment{ AZCORE_GLOBAL_NEW_ALIGNMENT };
        //! The minimal size alignment that's required to avoid intermediate buffers. If the size and/or offset
        //! provided to the read request isn't aligned to this size it may require a temporary or cached buffer
        //! to first read to and copy the result from to the provided memory.
        AZ::u64 m_sizeAlignment{ 1 };
        //! The recommended size for partial reads. It's recommended to read entire files at once, but for
        //! streaming systems such as video and audio this is not always practical. The granularity will
        //! give the most optimal size for partial file reads. Note for partial reads it's also recommended
        //! to store the data uncompressed and to align the offset of the rest to the granularity.
        AZ::u64 m_granularity{ 1_mib };
        //! The number of requests that the scheduler will try to keep active in the stack. Additional requests
        //! are considered pending and are subject to scheduling. There are no restrictions on the number of
        //! requests that can be send and generally there is no need to throttle the number of requests. The
        //! exception is for streaming systems such as video and audio that could flood the scheduler with
        //! requests in a short amount of time if not capped. For those systems it's recommended that no more
        //! than the provided number of requests are issued.
        AZ::u16 m_maxConcurrentRequests{ 1 };

        //! Calculates the recommended size to allocate for the read buffer in order to avoid the need for
        //! temporary buffers. The returned size will always be at least as big as readSize, but can be bigger.
        //! @param readSize The intended amount of bytes to be read.
        //! @param readOffset The number of bytes to offset into the file to start reading from.
        //! @return The recommended amount of memory to reserve, taking size, offset and alignment into account.
        AZ::u64 CalculateRecommendedMemorySize(u64 readSize, u64 readOffset = 0);
    };

    enum class RequestStatus
    {
        Pending, //!< The request has been registered but is waiting to be processed.
        Queued, //!< The request is queued somewhere on in the stack and waiting further processing.
        Processing, //!< The request is being processed.
        Completed, //!< The request has successfully completed.
        Canceled, //!< The request was canceled. This can still mean the request was partially completed.
        Failed //!< The request has failed to complete and can't be processed any further.
    };

    enum class MemoryType : u8
    {
        ReadWrite, //!< General purpose memory.
        WriteCombined //!< Reading back from memory with this flag will be avoided. This may require additional temporary buffers.
    };

    enum class ClaimMemory : bool
    {
        No, //!< If not claimed, the memory will be automatically released once the request has no more references.
        Yes //!< Ownership is transfered to the caller and memory will no longer be automatically released. It's recommended to use
            //!< the allocator provided to request to later release this memory.
    };

    struct RequestMemoryAllocatorResult
    {
        void* m_address; //!< The address to the reserved memory.
        size_t m_size; //!< The actual amount of memory that was reserved.
        MemoryType m_type; //!< The type of memory used.
    };

    //! RequestMemoryAllocators can be used to provide a mechanic to allow the Streamer to allocate memory right before the request is
    //! processed. This can be useful in avoiding having to track memory buffers as these are managed by Streamer until the buffer is
    //! claimed. It can also help reduce peak memory if processing data requires temporary buffers as these do not all have to be
    //! created upfront.
    //! The RequestMemoryAllocator typically outlives the request and a completed request does not mean the allocator isn't still in use as
    //! internal processing may still need the allocator. Calls to LockAllocator and UnlockAllocator can be used to keep track of the
    //! number of requests that still require processing.
    //! The functions in the RequestMemoryAllocator can be called from various threads and may prevent the Streamer from continuing
    //! processing. (Similar to the completion callbacks in the requests). It's recommended to spend as little time as possible in these
    //! callbacks and defer work to jobs where possible.
    class RequestMemoryAllocator
    {
    public:
        virtual ~RequestMemoryAllocator() = default;

        //! Called when the Streamer is about to start allocating memory for a request. This will be called multiple times.
        virtual void LockAllocator() = 0;
        //! Called when the Streamer no longer needs to manage memory for a request. This will be called multiple times. If the number of
        //! unlocks matches the number of locks it means the memory allocator is not actively being used by the Streamer.
        virtual void UnlockAllocator() = 0;

        //! Allocate memory for a request.
        //! @param minimalSize The minimal amount of memory to reserve. More can be reserved, but not less.
        //! @param recommendedSize The recommended amount of memory to reserve. If this size or more is reserved temporary buffers can be avoided.
        //! @param alignment The minimal recommended memory alignment. If the alignment is less than the recommendation it may force
        //!     the Streamer to create temporary buffers and do extra processing.
        //! @return A description of the allocated memory. If allocation fails, return a size of 0 and a null pointer for the address.
        virtual RequestMemoryAllocatorResult Allocate(u64 minimalSize, u64 recommendeSize, size_t alignment) = 0;
        //! Releases memory previously allocated by Allocate.
        //! @param address The address previously provided by Allocate.
        virtual void Release(void* address) = 0;
    };

    //! Default memory allocator for file requests. This allocator is a wrapper around the standard memory allocator and can be used
    //! if only delayed memory allocations are needed but no special memory requirements.
    class DefaultRequestMemoryAllocator final
        : public RequestMemoryAllocator
    {
    public:
        //! DefaultRequestMemoryAllocator wraps around the AZ::SystemAllocator by default.
        DefaultRequestMemoryAllocator();
        explicit DefaultRequestMemoryAllocator(AZ::IAllocatorAllocate& allocator);
        ~DefaultRequestMemoryAllocator() override;

        void LockAllocator() override;
        void UnlockAllocator() override;

        RequestMemoryAllocatorResult Allocate(u64 minimalSize, u64 recommendeSize, size_t alignment) override;
        void Release(void* address) override;

        int GetNumLocks() const;

    private:
        AZStd::atomic_int m_lockCounter{ 0 };
        AZStd::atomic_int m_allocationCounter{ 0 };
        AZ::IAllocatorAllocate& m_allocator;
    };

    // The following alignment functions are put here until they're available in AzCore's math library.

    constexpr bool IsPowerOf2(AZ::u64 value);
    constexpr bool IsAlignedTo(AZ::u64 value, AZ::u64 alignment);
    inline bool IsAlignedTo(void* address, AZ::u64 alignment);
    inline AZ::u64 GetAlignment(AZ::u64 value);
    inline AZ::u64 GetAlignment(void* address);

} // namespace AZ::IO::IStreamer

#include <AzCore/IO/IStreamerTypes.inl>
