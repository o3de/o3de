/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/BufferPoolDescriptor.h>
#include <Atom/RHI/DeviceBufferPoolBase.h>

namespace AZ::RHI
{
    class DeviceFence;

    //! A structure used as an argument to DeviceBufferPool::InitBuffer.
    struct DeviceBufferInitRequest
    {
        DeviceBufferInitRequest() = default;

        DeviceBufferInitRequest(DeviceBuffer& buffer, const BufferDescriptor& descriptor, const void* initialData = nullptr)
            : m_buffer{ &buffer }
            , m_descriptor{ descriptor }
            , m_initialData{ initialData }
        {}

        /// The buffer to initialize. The buffer must be in an uninitialized state.
        DeviceBuffer* m_buffer = nullptr;

        /// The descriptor used to initialize the buffer.
        BufferDescriptor m_descriptor;

        /// [Optional] Initial data used to initialize the buffer.
        const void* m_initialData = nullptr;
    };

    //! A structure used as an argument to DeviceBufferPool::MapBuffer.
    template <typename BufferClass>
    struct BufferMapRequestTemplate
    {
        BufferMapRequestTemplate() = default;

        BufferMapRequestTemplate(BufferClass& buffer, size_t byteOffset, size_t byteCount)
            : m_buffer{&buffer}
            , m_byteOffset{byteOffset}
            , m_byteCount{byteCount}
            {}

        /// The buffer instance to map for CPU access.
        BufferClass* m_buffer = nullptr;

        /// The number of bytes offset from the base of the buffer to map for access.
        size_t m_byteOffset = 0;

        /// The number of bytes beginning from the offset to map for access.
        size_t m_byteCount = 0;
    };

    //! A structure used as an argument to DeviceBufferPool::MapBuffer.
    struct DeviceBufferMapResponse
    {
        void* m_data = nullptr;
    };

    //! A structure used as an argument to DeviceBufferPool::StreamBuffer.
    template <typename BufferClass, typename FenceClass>
    struct BufferStreamRequestTemplate
    {
        /// A fence to signal on completion of the upload operation.
        FenceClass* m_fenceToSignal = nullptr;

        /// The buffer instance to stream up to.
        BufferClass* m_buffer = nullptr;

        /// The number of bytes offset from the base of the buffer to start the upload.
        size_t m_byteOffset = 0;

        /// The number of bytes to upload beginning from m_byteOffset.
        size_t m_byteCount = 0;

        /// A pointer to the source data to upload. The source data must remain valid
        /// for the duration of the upload operation (i.e. until m_callbackFunction
        /// is invoked).
        const void* m_sourceData = nullptr;
    };

    using DeviceBufferMapRequest = BufferMapRequestTemplate<DeviceBuffer>;
    using DeviceBufferStreamRequest = BufferStreamRequestTemplate<DeviceBuffer, DeviceFence>;

    //! Buffer pool provides backing storage and context for buffer instances. The BufferPoolDescriptor
    //! contains properties defining memory characteristics of buffer pools. All buffers created on a pool
    //! share the same backing heap and buffer bind flags.
    class DeviceBufferPool
        : public DeviceBufferPoolBase
    {
    public:
        AZ_RTTI(DeviceBufferPool, "{6C7A657E-3940-465D-BC15-569741D9BBDF}", DeviceBufferPoolBase)
        virtual ~DeviceBufferPool() override = default;

        //! Initializes the buffer pool with a provided descriptor. The pool must be in an uninitialized
        //! state, or this call will fail. To re-use an existing pool, you must first call Shutdown
        //! before calling Init again.
        //!
        //!  @param descriptor The descriptor containing properties used to initialize the pool.
        //!  @return A result code denoting the status of the call. If successful, the pool is considered
        //!      initialized and is able to service buffer requests. If failure, the pool remains uninitialized.
        ResultCode Init(Device& device, const BufferPoolDescriptor& descriptor);


        //! Initializes a buffer instance created from this pool. The buffer must be in an uninitialized
        //! state, or the call will fail. To re-use an existing buffer instance, first call Shutdown
        //! on the buffer prior to calling InitBuffer on the pool.
        //!
        //!  @param request The request used to initialize a buffer instance.
        //!  @return A result code denoting the status of the call. If successful, the buffer is considered
        //!      initialized and 'registered' with the pool. If the pool fails to secure an allocation for the
        //!      buffer, it remain in a shutdown state. If the initial data upload fails, the buffer will be
        //!      initialized, but will remain empty and the call will return ResultCode::OutOfMemory. Checking
        //!      this amounts to seeing if buffer.IsInitialized() is true.
        ResultCode InitBuffer(const DeviceBufferInitRequest& request);

        //! NOTE: Only applicable to 'Host' pools. Device pools will fail with ResultCode::InvalidOperation.
        //!
        //! Instructs the pool to allocate a new backing allocation for the buffer. This enables the user to
        //! ignore tracking hazards between the CPU and GPU timelines. Call this method if the entire buffer contents
        //! are being overwritten for a new frame.
        //!
        //! The user may instead do hazard tracking manually by not overwriting regions in-flight on the GPU. To ensure
        //! that a region has flushed through the GPU, either use Fences to track when a Scope has completed, or rely
        //! on RHI::Limits::Device::FrameCountMax (for example, by N-buffering the data in a round-robin
        //! fashion).
        //!
        //! If the new allocation is small enough to be page-allocated, the buffer's debug name will be lost.
        //! If the allocation is large enough to create a new buffer object, it will call SetName() with
        //! the old name.
        //!
        //!  @param buffer The buffer whose backing allocation to orphan. The buffer must be registered and
        //!      initialized with this pool.
        //!  @return On success, the buffer is considered to have a new backing allocation. On failure, the existing
        //!      buffer allocation remains intact.
        ResultCode OrphanBuffer(DeviceBuffer& buffer);

        //! Maps a buffer region for CPU access. The type of access (read or write) is dictated by the type of
        //! buffer pool. Host pools with host read access may read from the buffer--the contents of which
        //! are written by the GPU. All other modes only expose write-only access by the CPU.
        //!
        //! Is it safe to nest Map operations if the regions are disjoint. Calling Map is reference counted, so calling
        //! Unmap is required for each Map call. Map operations will block the frame scheduler from recording staging
        //! operations to the command lists. To avoid this, unmap all buffer regions before the frame execution phase.
        //!
        //!  @param request The map request structure holding properties for the map operation.
        //!  @param response The map response structure holding the mapped data pointer (if successful), or null.
        //!  @return Returns a result code specifying whether the call succeeded, or a failure code specifying
        //!      why the call failed.
        ResultCode MapBuffer(const DeviceBufferMapRequest& request, DeviceBufferMapResponse& response);

        //! Unmaps a buffer for CPU access. The mapped data pointer is considered invalid after this call and
        //! should not be accessed. This call unmaps the data region and unblocks the GPU for access.
        void UnmapBuffer(DeviceBuffer& buffer);

        //! Asynchronously streams buffer data up to the GPU. The operation is decoupled from the frame scheduler.
        //! It is not valid to use the buffer while the upload is running. The provided fence is signaled when the
        //! upload completes.
        ResultCode StreamBuffer(const DeviceBufferStreamRequest& request);

        //! Returns the buffer descriptor used to initialize the buffer pool. Descriptor contents
        //! are undefined for uninitialized pools.
        const BufferPoolDescriptor& GetDescriptor() const override final;

    protected:
        DeviceBufferPool() = default;

        ///////////////////////////////////////////////////////////////////
        // FrameEventBus::Handler
        void OnFrameBegin() override;
        ///////////////////////////////////////////////////////////////////

        bool ValidateNotProcessingFrame() const;

    private:
        using DeviceResourcePool::Init;
        using DeviceBufferPoolBase::InitBuffer;

        bool ValidatePoolDescriptor(const BufferPoolDescriptor& descriptor) const;
        bool ValidateInitRequest(const DeviceBufferInitRequest& initRequest) const;
        bool ValidateIsHostHeap() const;
        bool ValidateMapRequest(const DeviceBufferMapRequest& request) const;

        //////////////////////////////////////////////////////////////////////////
        // Platform API

        /// Called when the pool is being initialized.
        virtual ResultCode InitInternal(Device& device, const RHI::BufferPoolDescriptor& descriptor) = 0;

        /// Called when a buffer is being initialized onto the pool.
        virtual ResultCode InitBufferInternal(DeviceBuffer& buffer, const BufferDescriptor& descriptor) = 0;

        /// Called when the buffer is being orphaned.
        virtual ResultCode OrphanBufferInternal(DeviceBuffer& buffer) = 0;

        /// Called when a buffer is being mapped.
        virtual ResultCode MapBufferInternal(const DeviceBufferMapRequest& request, DeviceBufferMapResponse& response) = 0;

        /// Called when a buffer is being unmapped.
        virtual void UnmapBufferInternal(DeviceBuffer& buffer) = 0;

        /// Called when a buffer is being streamed asynchronously.
        virtual ResultCode StreamBufferInternal(const DeviceBufferStreamRequest& request);

        //Called in order to do a simple mem copy allowing Null rhi to opt out
        virtual void BufferCopy(void* destination, const void* source, size_t num);

        //////////////////////////////////////////////////////////////////////////

        BufferPoolDescriptor m_descriptor;
    };
}
