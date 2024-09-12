/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <RHI/WebGPU.h>
#include <RHI/AsyncUploadQueue.h>
#include <RHI/Buffer.h>
#include <RHI/CommandQueue.h>
#include <RHI/CommandQueueContext.h>
#include <RHI/Device.h>
#include <RHI/Fence.h>
#include <RHI/Image.h>
#include <Atom/RHI/DeviceStreamingImagePool.h>
#include <AzCore/Component/TickBus.h>

namespace AZ::WebGPU
{
    RHI::ResultCode AsyncUploadQueue::Init(Device& device)
    {
        Base::Init(device);
        RHI::ResultCode result = RHI::ResultCode::Success;

        m_queue = &device.GetCommandQueueContext().GetCommandQueue(RHI::HardwareQueueClass::Copy);
        m_asyncWaitQueue.Init();

        return result;
    }

    void AsyncUploadQueue::Shutdown()
    {
        m_asyncWaitQueue.ShutDown();
        m_callbackList.clear();
    }

    RHI::AsyncWorkHandle AsyncUploadQueue::QueueUpload(const RHI::DeviceBufferStreamRequest& request)
    {
        auto& device = static_cast<Device&>(GetDevice());

        const uint8_t* sourceData = reinterpret_cast<const uint8_t*>(request.m_sourceData);
        const size_t byteCount = request.m_byteCount;
        auto* buffer = static_cast<Buffer*>(request.m_buffer);
        if (byteCount == 0)
        {
            AZ_Assert(false, "Trying to upload 0 bytes to buffer");
            return RHI::AsyncWorkHandle::Null;
        }

        RHI::Ptr<Fence> uploadFence = Fence::Create();
        uploadFence->Init(device, RHI::FenceState::Reset);
        m_queue->WriteBuffer(*buffer, request.m_byteOffset, AZStd::span<const uint8_t>(sourceData, byteCount));
        m_queue->Signal(*uploadFence);
        if (request.m_fenceToSignal)
        {
            m_queue->Signal(static_cast<Fence&>(*request.m_fenceToSignal));
        }

        auto waitEvent = [buffer]()
        {
            buffer->SetUploadHandle(RHI::AsyncWorkHandle::Null);
        };

        // Add the wait event so we can wait for upload if necessary.
        RHI::AsyncWorkHandle handle = CreateAsyncWork(uploadFence, waitEvent);
        buffer->SetUploadHandle(handle);
        m_asyncWaitQueue.UnlockAsyncWorkQueue();
        return handle;
    }

    RHI::AsyncWorkHandle AsyncUploadQueue::QueueUpload(const RHI::DeviceStreamingImageExpandRequest& request, uint32_t residentMip)
    {
        if (residentMip < request.m_mipSlices.size() || residentMip < 1)
        {
            AZ_Assert(false, "Wrong input parameter");
        }

        auto* image = static_cast<Image*>(request.m_image);
        auto& device = static_cast<Device&>(GetDevice());

        const uint16_t startMip = static_cast<uint16_t>(residentMip - 1);
        const uint16_t endMip = static_cast<uint16_t>(residentMip - request.m_mipSlices.size());

        RHI::Ptr<Fence> uploadFence = Fence::Create();
        uploadFence->Init(device, RHI::FenceState::Reset);

        CommandQueue::Command command = [=](void* queue)
        {
            AZ_PROFILE_SCOPE(RHI, "Upload Image");

            wgpu::Queue* wgpuQueue = static_cast<wgpu::Queue*>(queue);
            for (uint16_t curMip = endMip; curMip <= startMip; ++curMip)
            {
                const size_t sliceIndex = curMip - endMip;
                const RHI::DeviceImageSubresourceLayout& subresourceLayout = request.m_mipSlices[sliceIndex].m_subresourceLayout;

                // ImageHeight must be bigger than or equal to the Image's row count. Images with a RowCount that is less than the ImageHeight indicates a block compression.
                // Images with a RowCount which is higher than the ImageHeight indicates a planar image, which is not supported for streaming images.
                AZ_Error("StreamingImage", subresourceLayout.m_size.m_height >= subresourceLayout.m_rowCount, "AsyncUploadQueue::QueueUpload expects ImageHeight '%d' to be bigger than or equal to the image's RowCount '%d'.", subresourceLayout.m_size.m_height, subresourceLayout.m_rowCount);

                auto& subresources = request.m_mipSlices[sliceIndex].m_subresources;
                bool isContinousRange =
                    (static_cast<const uint8_t*>(subresources.front().m_data) + subresources.size() * subresourceLayout.m_bytesPerImage) ==
                    (static_cast<const uint8_t*>(subresources.back().m_data) + subresourceLayout.m_bytesPerImage);

                wgpu::Extent3D size = {};
                size.width = subresourceLayout.m_size.m_width;
                size.height = subresourceLayout.m_size.m_height;
                size.depthOrArrayLayers = subresourceLayout.m_size.m_depth;
                wgpu::TextureDataLayout dataLayout = {};
                dataLayout.offset = 0;
                dataLayout.bytesPerRow = subresourceLayout.m_bytesPerRow;
                dataLayout.rowsPerImage = subresourceLayout.m_rowCount;

                wgpu::ImageCopyTexture copyDescriptor = {};
                copyDescriptor.texture = static_cast<Image*>(request.m_image)->GetNativeTexture();
                copyDescriptor.mipLevel = curMip;
                copyDescriptor.origin = {};
                copyDescriptor.aspect = wgpu::TextureAspect::All;
                if (isContinousRange)
                {
                    wgpuQueue->WriteTexture(
                        &copyDescriptor,
                        request.m_mipSlices[sliceIndex].m_subresources.front().m_data,
                        subresourceLayout.m_bytesPerImage,
                        &dataLayout,
                        &size);
                }
                else
                {
                    for (uint32_t arraySlice = 0; arraySlice < request.m_mipSlices[sliceIndex].m_subresources.size(); ++arraySlice)
                    {
                        const RHI::StreamingImageSubresourceData& subresourceData =
                            request.m_mipSlices[sliceIndex].m_subresources[arraySlice];

                        copyDescriptor.origin.z = arraySlice;
                        wgpuQueue->WriteTexture(
                            &copyDescriptor,
                            subresourceData.m_data,
                            subresourceLayout.m_bytesPerImage,
                            &dataLayout,
                            &size);
                    }
                }
            }  
        };

        m_queue->QueueCommand(AZStd::move(command));
        m_queue->Signal(*uploadFence);

        auto waitEvent = [this, request, image]()
        {
            RHI::AsyncWorkHandle uploadHandle = image->GetUploadHandle();
            if (request.m_completeCallback)
            {
                if (request.m_waitForUpload)
                {
                    request.m_completeCallback();
                }
                else
                {
                    // Add the callback so it can be processed from the main thread.                       
                    {
                        AZStd::unique_lock<AZStd::mutex> lock(m_callbackListMutex);
                        m_callbackList.insert(AZStd::make_pair(uploadHandle, [request, image]() {
                            image->SetUploadHandle(RHI::AsyncWorkHandle::Null);
                            request.m_completeCallback();
                        }));
                    }
                    // We could just add a lambda that calls the request.m_completeCallback() but that could create a crash
                    // if the image is destroyed before the callback is triggered from the TickBus. Because of this we save the callbacks in this
                    // class and when and image is destroy, we just execute any pending callback for that image.
                    AZ::TickBus::QueueFunction([this, uploadHandle]() { ProcessCallback(uploadHandle); });
                }
            }
        };

        if (request.m_waitForUpload)
        {
            // No need to add wait event.
            uploadFence->WaitOnCpu();
            waitEvent();
        }
        else
        {
            auto uploadHandle = CreateAsyncWork(uploadFence, waitEvent);
            image->SetUploadHandle(uploadHandle);
            m_asyncWaitQueue.UnlockAsyncWorkQueue();
            return uploadHandle;
        }

        return RHI::AsyncWorkHandle::Null;
    }

    void AsyncUploadQueue::WaitForUpload(const RHI::AsyncWorkHandle& workHandle)
    {
        m_asyncWaitQueue.WaitToFinish(workHandle);
        ProcessCallback(workHandle);
    }

    RHI::AsyncWorkHandle AsyncUploadQueue::CreateAsyncWork(RHI::Ptr<Fence> fence, RHI::DeviceFence::SignalCallback callback /* = nullptr */)
    {
        return m_asyncWaitQueue.CreateAsyncWork([fence, callback]()
        {
            fence->WaitOnCpu();
            if (callback)
            {
                callback();
            }
        });
    }

    void AsyncUploadQueue::ProcessCallback(const RHI::AsyncWorkHandle& handle)
    {
        AZ_PROFILE_SCOPE(RHI, "AsyncUploadQueue: ProcessCallback");
        AZStd::unique_lock<AZStd::mutex> lock(m_callbackListMutex);
        auto findIter = m_callbackList.find(handle);
        if (findIter != m_callbackList.end())
        {
            findIter->second();
            m_callbackList.erase(findIter);
        }
    }
}
