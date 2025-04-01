/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/BufferPoolDescriptor.h>
#include <Atom/RHI/DeviceBufferPool.h>
#include <Atom/RHI/Buffer.h>
#include <Atom/RHI/BufferPoolBase.h>

namespace AZ::RHI
{
    class Fence;

    //! A structure used as an argument to BufferPool::MapBuffer.
    struct BufferMapResponse
    {
        //! Will hold the mapped data for each device selected in the Buffer
        AZStd::unordered_map<int, void*> m_data;
    };

    //! A structure used as an argument to BufferPool::UpdateBufferDeviceMask.
    struct BufferDeviceMaskRequest
    {
        BufferDeviceMaskRequest() = default;

        BufferDeviceMaskRequest(
            Buffer& buffer, MultiDevice::DeviceMask deviceMask = MultiDevice::AllDevices, const void* initialData = nullptr)
            : m_buffer{ &buffer }
            , m_deviceMask{ deviceMask }
            , m_initialData{ initialData }
        {
        }

        /// The buffer to update the device mask of and (de)allocate device buffers.
        Buffer* m_buffer = nullptr;

        /// The new device mask used for the buffer.
        /// Note: Only devices in the mask of the buffer pool will be considered.
        MultiDevice::DeviceMask m_deviceMask = MultiDevice::AllDevices;

        /// [Optional] Initial data used to initialize new device buffers with.
        const void* m_initialData = nullptr;
    };

    //! A structure used as an argument to BufferPool::InitBuffer.
    struct BufferInitRequest : public BufferDeviceMaskRequest
    {
        BufferInitRequest() = default;

        BufferInitRequest(
            Buffer& buffer,
            const BufferDescriptor& descriptor,
            const void* initialData = nullptr,
            MultiDevice::DeviceMask deviceMask = MultiDevice::AllDevices)
            : BufferDeviceMaskRequest{ buffer, deviceMask, initialData }
            , m_descriptor{ descriptor }
        {
        }

        /// The descriptor used to initialize the buffer.
        BufferDescriptor m_descriptor;
    };

    using BufferMapRequest = BufferMapRequestTemplate<Buffer>;
    using BufferStreamRequest = BufferStreamRequestTemplate<Buffer, Fence>;

    //! Buffer pool provides backing storage and context for buffer instances. The BufferPoolDescriptor
    //! contains properties defining memory characteristics of buffer pools. All buffers created on a pool
    //! share the same backing heap and buffer bind flags.
    class BufferPool : public BufferPoolBase
    {
        friend class RayTracingBufferPools;

    public:
        AZ_CLASS_ALLOCATOR(BufferPool, AZ::SystemAllocator, 0);
        AZ_RTTI(BufferPool, "{547F1577-0AA3-4F0D-9656-8905DE5E9E8A}", BufferPoolBase)
        AZ_RHI_MULTI_DEVICE_OBJECT_GETTER(BufferPool);
        BufferPool() = default;
        virtual ~BufferPool() override = default;

        //! Initializes the buffer pool with a provided descriptor. The pool must be in an uninitialized
        //! state, or this call will fail. To re-use an existing pool, you must first call Shutdown
        //! before calling Init again.
        //!
        //!  @param descriptor The descriptor containing properties used to initialize the pool.
        //!  @return A result code denoting the status of the call. If successful, the pool is considered
        //!      initialized and is able to service buffer requests. If failure, the pool remains uninitialized.
        ResultCode Init(const BufferPoolDescriptor& descriptor);

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
        ResultCode InitBuffer(const BufferInitRequest& request);

        //! Updates the device mask of a buffer instance created from this pool. The buffer must be in an
        //! initialized state, or the call will fail, i.e., first call InitBuffer on the pool.
        //!
        //!  @param request The request used to update a buffer instance's device mask and device buffer
        //!      allocations.
        //!  @return A result code denoting the status of the call. If successful, the buffer device mask is
        //!      considered updated. If the pool fails to secure an allocation for the device buffers, it's
        //!      device mask may only partially change. If the initial data upload fails, the buffer will be
        //!      initialized, but will remain empty and the call will return ResultCode::OutOfMemory.
        ResultCode UpdateBufferDeviceMask(const BufferDeviceMaskRequest& request);

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
        ResultCode OrphanBuffer(Buffer& buffer);

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
        ResultCode MapBuffer(const BufferMapRequest& request, BufferMapResponse& response);

        //! Unmaps a buffer for CPU access. The mapped data pointer is considered invalid after this call and
        //! should not be accessed. This call unmaps the data region and unblocks the GPU for access.
        void UnmapBuffer(Buffer& buffer);

        //! Asynchronously streams buffer data up to the GPU. The operation is decoupled from the frame scheduler.
        //! It is not valid to use the buffer while the upload is running. The provided fence is signaled when the
        //! upload completes.
        ResultCode StreamBuffer(const BufferStreamRequest& request);

        //! Returns the buffer descriptor used to initialize the buffer pool. Descriptor contents
        //! are undefined for uninitialized pools.
        const BufferPoolDescriptor& GetDescriptor() const override final;

        //! Shuts down the pool. This method will shutdown all resources associated with the pool.
        void Shutdown() override final;

    private:
        using BufferPoolBase::InitBuffer;
        using ResourcePool::Init;

        //! Validates that the map operation succeeded by printing a warning otherwise. Increments
        //! the map reference counts for the buffer and the pool.
        void ValidateBufferMap(Buffer& buffer, bool isDataValid);

        bool ValidatePoolDescriptor(const BufferPoolDescriptor& descriptor) const;
        bool ValidateInitRequest(const BufferInitRequest& initRequest) const;
        bool ValidateIsHostHeap() const;
        bool ValidateMapRequest(const BufferMapRequest& request) const;

        BufferPoolDescriptor m_descriptor;
    };
} // namespace AZ::RHI
