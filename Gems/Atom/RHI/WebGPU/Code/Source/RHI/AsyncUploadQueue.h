/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/DeviceBufferPool.h>
#include <Atom/RHI/DeviceStreamingImagePool.h>
#include <AzCore/std/parallel/mutex.h>
#include <AzCore/std/containers/unordered_map.h>
#include <Atom/RHI/AsyncWorkQueue.h>
#include <RHI/CommandQueue.h>

namespace AZ::WebGPU
{
    class Buffer;
    class CommandList;
    class Queue;
    class Fence;
    struct QueueId;

    /**
        * This class implements a dedicate upload queue for uploading data to device resources.
    */
    class AsyncUploadQueue final
        : public RHI::DeviceObject
    {
        using Base = RHI::DeviceObject;

    public:
        AZ_CLASS_ALLOCATOR(AsyncUploadQueue, AZ::SystemAllocator);

        AZ_DISABLE_COPY_MOVE(AsyncUploadQueue);

        AsyncUploadQueue() = default;
        ~AsyncUploadQueue() = default;

        RHI::ResultCode Init(Device& device);
        void Shutdown();

        RHI::AsyncWorkHandle QueueUpload(const RHI::DeviceBufferStreamRequest& request);
        RHI::AsyncWorkHandle QueueUpload(const RHI::DeviceStreamingImageExpandRequest& request, uint32_t residentMip);

        void WaitForUpload(const RHI::AsyncWorkHandle& workHandle);

    private:
        RHI::Ptr<CommandQueue> m_queue;
        RHI::AsyncWorkHandle CreateAsyncWork(RHI::Ptr<Fence> fence, RHI::DeviceFence::SignalCallback callback = nullptr);
        void ProcessCallback(const RHI::AsyncWorkHandle& handle);

        // Async queue used for waiting for an upload event to complete.
        RHI::AsyncWorkQueue m_asyncWaitQueue;

        AZStd::mutex m_callbackListMutex;
        AZStd::unordered_map<RHI::AsyncWorkHandle, RHI::CompleteCallback> m_callbackList;
    };    
}
