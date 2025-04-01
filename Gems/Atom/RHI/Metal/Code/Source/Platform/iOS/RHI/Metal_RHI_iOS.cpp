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
#include <RHI/MetalView_Platform.h>
#include <RHI/PhysicalDevice.h>

namespace Platform
{
    AZ::RHI::PhysicalDeviceType GetPhysicalDeviceType(id<MTLDevice> mtlDevice)
    {
        return AZ::RHI::PhysicalDeviceType::GpuIntegrated;
    }

    AZ::RHI::PhysicalDeviceList EnumerateDevices()
    {
        AZ::RHI::PhysicalDeviceList physicalDeviceList;
        AZ::Metal::PhysicalDevice* physicalDevice = aznew AZ::Metal::PhysicalDevice;
        physicalDevice->Init(MTLCreateSystemDefaultDevice());
        physicalDeviceList.emplace_back(physicalDevice);
        return physicalDeviceList;
    }

    float GetRefreshRate()
    {
        NativeScreenType* nativeScreen = [NativeScreenType mainScreen];
        return [nativeScreen maximumFramesPerSecond];
    }

    void PresentInternal(id <MTLCommandBuffer> mtlCommandBuffer, id<CAMetalDrawable> drawable, float syncInterval, float refreshRate)
    {
        //seconds per frame (1/refreshrate) * num frames (sync interval)
        float presentAfterMinimumDuration = syncInterval / refreshRate;
        if (presentAfterMinimumDuration > 0.0f)
        {
            [mtlCommandBuffer presentDrawable:drawable afterMinimumDuration:presentAfterMinimumDuration];
        }
        else
        {
            [mtlCommandBuffer presentDrawable:drawable];
        }
    }

    CGRect GetScreenBounds(NativeWindowType* nativeWindow)
    {
        AZ_UNUSED(nativeWindow);
        NativeScreenType* nativeScreen = [NativeScreenType mainScreen];
        return [nativeScreen bounds];
    }

    CGFloat GetScreenScale()
    {
        NativeScreenType* nativeScreen = [NativeScreenType mainScreen];
        return [nativeScreen scale];
    }

    void AttachViewController(NativeWindowType* nativeWindow, NativeViewControllerType* viewController, RHIMetalView* metalView)
    {
        AZ_UNUSED(metalView);
        nativeWindow.rootViewController = viewController;
    }

    void UnAttachViewController(NativeWindowType* nativeWindow, NativeViewControllerType* viewController)
    {
        if (nativeWindow.rootViewController == viewController)
        {
            nativeWindow.rootViewController = nil;
        }
        [viewController setView : nil];
    }

    void ResizeInternal(RHIMetalView* metalView, CGSize viewSize)
    {
        [metalView.metalLayer setDrawableSize: viewSize];
    }

    RHIMetalView* GetMetalView(NativeWindowType* nativeWindow)
    {
        return reinterpret_cast<RHIMetalView*>([nativeWindow.rootViewController view]);
    }

    void PublishBufferCpuChangeOnGpu(id<MTLBuffer> mtlBuffer, size_t bufferOffset, size_t bufferSize)
    {
        //No synchronization needed as ios uses shared memory and does not support MTLStorageModeManaged
        AZ_UNUSED(mtlBuffer);
        AZ_UNUSED(bufferOffset);
        AZ_UNUSED(bufferSize);
    }

    void PublishBufferGpuChangeOnCpu(id<MTLBlitCommandEncoder> blitEncoder, id<MTLBuffer> mtlBuffer)
    {
        //No synchronization needed as ios uses shared memory and does not support MTLStorageModeManaged
        AZ_UNUSED(blitEncoder);
        AZ_UNUSED(mtlBuffer);
    }

    void PublishTextureGpuChangeOnCpu(id<MTLBlitCommandEncoder> blitEncoder, id<MTLTexture> mtlTexture)
    {
        //No synchronization needed as ios uses shared memory and does not support MTLStorageModeManaged
        AZ_UNUSED(blitEncoder);
        AZ_UNUSED(mtlTexture);
    }

    AZ::RHI::ResultCode MapBufferInternal(const AZ::RHI::DeviceBufferMapRequest& request, AZ::RHI::DeviceBufferMapResponse& response)
    {
        //No need to do anything here as ios does not support MTLStorageModeManaged
        AZ_UNUSED(request);
        AZ_UNUSED(response);
        return AZ::RHI::ResultCode::Success;
    }
    
    void UnMapBufferInternal(AZ::RHI::DeviceBuffer& bufferBase)
    {
        //No need to do anything here as ios does not support MTLStorageModeManaged
        AZ_UNUSED(bufferBase);
    }
}


