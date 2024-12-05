/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/DeviceSwapChain.h>
#include <RHI/Device.h>
#import <QuartzCore/CAMetalLayer.h>

namespace AZ
{
    namespace Metal
    {
        class Device;
        class Image;

        class SwapChain
            : public RHI::DeviceSwapChain
        {
            using Base = RHI::DeviceSwapChain;
        public:
            AZ_RTTI(SwapChain, "{2ECD01DB-BD24-4FD1-BA21-370B20071F02}", Base);
            AZ_CLASS_ALLOCATOR(SwapChain, AZ::SystemAllocator);

            static RHI::Ptr<SwapChain> Create();
            Device& GetDevice() const;
            void SetCommandBuffer(id <MTLCommandBuffer> mtlCommandBuffer);
            
            id<MTLTexture> RequestDrawable(bool isFrameCaptureEnabled); 
            
        private:
            SwapChain() = default;
            
            //////////////////////////////////////////////////////////////////////////
            // RHI::DeviceSwapChain
            RHI::ResultCode InitInternal(RHI::Device& deviceBase, const RHI::SwapChainDescriptor& descriptor, RHI::SwapChainDimensions* nativeDimensions) override;
            void ShutdownInternal() override;
            uint32_t PresentInternal() override;
            RHI::ResultCode InitImageInternal(const InitImageRequest& request) override;
            void ShutdownResourceInternal(RHI::DeviceResource& resourceBase) override;
            RHI::ResultCode ResizeInternal(const RHI::SwapChainDimensions& dimensions, RHI::SwapChainDimensions* nativeDimensions) override;
            //////////////////////////////////////////////////////////////////////////
            
            void AddSubView();
            
            id <MTLCommandBuffer>   m_mtlCommandBuffer;
            RHIMetalView* m_metalView = nullptr;
            NativeViewControllerType* m_viewController = nullptr;
            id<MTLDevice> m_mtlDevice = nil;
            NativeWindowType* m_nativeWindow = nullptr;
            AZStd::vector<id<CAMetalDrawable>> m_drawables;
            uint32_t m_refreshRate = 0;
            CGSize m_drawableSize;
            mutable AZStd::mutex m_drawablesMutex;
        };
    }
}
