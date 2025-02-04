/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#import <QuartzCore/CAMetalLayer.h>
#include <RHI/Buffer.h>
#include <RHI/BufferPool.h>
#include <RHI/MemoryView.h>
#include <RHI/MetalView_Platform.h>
#include <RHI/PhysicalDevice.h>
#include <Atom/RHI/RHIBus.h>

namespace Platform
{
    AZ::RHI::PhysicalDeviceType GetPhysicalDeviceType(id<MTLDevice> mtlDevice)
    {
        if (mtlDevice.lowPower)
        {
            return AZ::RHI::PhysicalDeviceType::GpuIntegrated;
        }
        else
        {
            return AZ::RHI::PhysicalDeviceType::GpuDiscrete;
        }
    }

    AZ::RHI::PhysicalDeviceList EnumerateDevices()
    {
        AZ::RHI::PhysicalDeviceList physicalDeviceList;
        @autoreleasepool
        {
            NSArray<id<MTLDevice>> * metalDevices = MTLCopyAllDevices();
            for (id device in metalDevices)
            {
                AZ::Metal::PhysicalDevice* physicalDevice = aznew AZ::Metal::PhysicalDevice;
                physicalDevice->Init(device);
                physicalDeviceList.emplace_back(physicalDevice);
            }
        }
        AZ::RHI::RHIRequirementRequestBus::Broadcast(
            &AZ::RHI::RHIRequirementsRequest::FilterSupportedPhysicalDevices, physicalDeviceList, AZ::RHI::APIIndex::Metal);
        return physicalDeviceList;
    }
    
    float GetRefreshRate()
    {
        CGDirectDisplayID display = CGMainDisplayID();
        CGDisplayModeRef currentMode = CGDisplayCopyDisplayMode(display);
        return CGDisplayModeGetRefreshRate(currentMode);
    }

    void PresentInternal(id <MTLCommandBuffer> mtlCommandBuffer, id<CAMetalDrawable> drawable, float syncInterval, float refreshRate)
    {
        bool framePresented = false;
        
        //seconds per frame (1/refreshrate) * num frames (sync interval)
        float presentAfterMinimumDuration = syncInterval / refreshRate;
        
#if defined(__MAC_10_15_4)
        if(@available(macOS 10.15.4, *))
        {
            if(presentAfterMinimumDuration > 0.0f)
            {
                [mtlCommandBuffer presentDrawable:drawable afterMinimumDuration:presentAfterMinimumDuration];
                framePresented = true;
            }
        }
#endif

        if(!framePresented)
        {
            [mtlCommandBuffer presentDrawable:drawable];
        }
    }

    CGRect GetScreenBounds(NativeWindowType* nativeWindow)
    {
        return [nativeWindow frame];
    }

    CGFloat GetScreenScale()
    {
        NativeScreenType* nativeScreen = [NativeScreenType mainScreen];
        return [nativeScreen backingScaleFactor];
    }

    void AttachViewController(NativeWindowType* nativeWindow, NativeViewControllerType* viewController, RHIMetalView* metalView)
    {
        nativeWindow.contentViewController = viewController;
        [nativeWindow makeFirstResponder: metalView];
        
        //Used for window close notification
        [[NSNotificationCenter defaultCenter] addObserver   :viewController
                                            selector        :@selector(windowWillClose:)
                                            name            :NSWindowWillCloseNotification
                                            object          :nativeWindow];
        
        //Used for window resizing notification
        [[NSNotificationCenter defaultCenter] addObserver   :viewController
                                            selector        :@selector(windowDidResize:)
                                            name            :NSWindowDidResizeNotification
                                            object          :nativeWindow];
    }

    void UnAttachViewController(NativeWindowType* nativeWindow, NativeViewControllerType* viewController)
    {
        if (nativeWindow.contentViewController == viewController)
        {
            nativeWindow.contentViewController = nil;
        }
    }

    void ResizeInternal(RHIMetalView* metalView, CGSize viewSize)
    {
        [metalView.metalLayer setDrawableSize: viewSize];
    }

    RHIMetalView* GetMetalView(NativeWindowType* nativeWindow)
    {
        return reinterpret_cast<RHIMetalView*>([nativeWindow.contentViewController view]);
    }

    void PublishBufferCpuChangeOnGpu(id<MTLBuffer> mtlBuffer, size_t bufferOffset, size_t bufferSize)
    {
        if(mtlBuffer.storageMode == MTLStorageModeManaged)
        {
            [mtlBuffer didModifyRange:NSMakeRange(bufferOffset, bufferSize)];
        }
    }

    void PublishBufferGpuChangeOnCpu(id<MTLBlitCommandEncoder> blitEncoder, id<MTLBuffer> mtlBuffer)
    {
        if(blitEncoder && mtlBuffer.storageMode == MTLStorageModeManaged)
        {
            [blitEncoder synchronizeResource:mtlBuffer];
        }
    }

    void PublishTextureGpuChangeOnCpu(id<MTLBlitCommandEncoder> blitEncoder, id<MTLTexture> mtlTexture)
    {
        if(blitEncoder && mtlTexture.storageMode == MTLStorageModeManaged)
        {
            [blitEncoder synchronizeResource:mtlTexture];
        }
    }

    AZ::RHI::ResultCode MapBufferInternal(const AZ::RHI::DeviceBufferMapRequest& request, AZ::RHI::DeviceBufferMapResponse& response)
    {
        AZ::Metal::Buffer& buffer = *static_cast<AZ::Metal::Buffer*>(request.m_buffer);
        MTLStorageMode mtlStorageMode = buffer.GetMemoryView().GetStorageMode();
        
        switch(mtlStorageMode)
        {
            case MTLStorageModeManaged:
            {
                void* systemAddress = buffer.GetMemoryView().GetCpuAddress();
                AZ::Metal::CpuVirtualAddress mappedData = static_cast<AZ::Metal::CpuVirtualAddress>(systemAddress);
                
                if (!mappedData)
                {
                    return AZ::RHI::ResultCode::Fail;
                }
                mappedData += request.m_byteOffset;                
                response.m_data = mappedData;
                buffer.SetMapRequestOffset(static_cast<uint32_t>(request.m_byteOffset));
                break;
            }
            default:
            {
                AZ_Assert(false, "Storage type not supported.");
                return AZ::RHI::ResultCode::InvalidArgument;
            }
        }
        return AZ::RHI::ResultCode::Success;
    }

    void UnMapBufferInternal(AZ::RHI::DeviceBuffer& bufferBase)
    {
        AZ::Metal::Buffer& buffer = static_cast<AZ::Metal::Buffer&>(bufferBase);
        //Ony need to handle MTLStorageModeManaged memory.
        PublishBufferCpuChangeOnGpu(buffer.GetMemoryView().GetGpuAddress<id<MTLBuffer>>(),
                               buffer.GetMemoryView().GetOffset() + buffer.GetMapRequestOffset(),
                               buffer.GetMemoryView().GetSize());
    }
}
