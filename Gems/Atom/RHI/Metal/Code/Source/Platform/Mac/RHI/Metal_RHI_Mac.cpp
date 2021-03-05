/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates, or
* a third party where indicated.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include "Atom_RHI_Metal_precompiled.h"

#import <QuartzCore/CAMetalLayer.h>
#include <RHI/Buffer.h>
#include <RHI/BufferPool.h>
#include <RHI/MemoryView.h>
#include <RHI/MetalView_Platform.h>
#include <RHI/PhysicalDevice.h>

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
        return physicalDeviceList;
    }

    void PresentInternal(id <MTLCommandBuffer> mtlCommandBuffer, id<CAMetalDrawable> drawable, float syncInterval)
    {
        AZ_UNUSED(syncInterval);
        [mtlCommandBuffer presentDrawable:drawable];
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
        [metalView resizeSubviewsWithOldSize:viewSize];
    }

    RHIMetalView* GetMetalView(NativeWindowType* nativeWindow)
    {
        return reinterpret_cast<RHIMetalView*>([nativeWindow.contentViewController view]);
    }

    void ApplyTileDimentions(MTLRenderPassDescriptor* mtlRenderPassDescriptor)
    {
        AZ_UNUSED(mtlRenderPassDescriptor);
    }

    void SynchronizeBufferOnCPU(id<MTLBuffer> mtlBuffer, size_t bufferOffset, size_t bufferSize)
    {
        if(mtlBuffer.storageMode == MTLStorageModeManaged)
        {
            [mtlBuffer didModifyRange:NSMakeRange(bufferOffset, bufferSize)];
        }
    }

    void SynchronizeBufferOnGPU(id<MTLBlitCommandEncoder> blitEncoder, id<MTLBuffer> mtlBuffer)
    {
        if(blitEncoder && mtlBuffer.storageMode == MTLStorageModeManaged)
        {
            [blitEncoder synchronizeResource:mtlBuffer];
        }
    }

    void SynchronizeTextureOnGPU(id<MTLBlitCommandEncoder> blitEncoder, id<MTLTexture> mtlTexture)
    {
        if(blitEncoder && mtlTexture.storageMode == MTLStorageModeManaged)
        {
            [blitEncoder synchronizeResource:mtlTexture];
        }
    }

    AZ::RHI::ResultCode MapBufferInternal(const AZ::RHI::BufferMapRequest& request, AZ::RHI::BufferMapResponse& response)
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

    void UnMapBufferInternal(AZ::RHI::Buffer& bufferBase)
    {
        AZ::Metal::Buffer& buffer = static_cast<AZ::Metal::Buffer&>(bufferBase);
        //Ony need to handle MTLStorageModeManaged memory.
        SynchronizeBufferOnCPU(buffer.GetMemoryView().GetGpuAddress<id<MTLBuffer>>(), buffer.GetMemoryView().GetOffset(), buffer.GetMemoryView().GetSize());
    }
}
