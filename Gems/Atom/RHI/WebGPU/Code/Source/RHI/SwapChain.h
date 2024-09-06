/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/DeviceSwapChain.h>

namespace AZ
{
    namespace WebGPU
    {
        class SwapChain
            : public RHI::DeviceSwapChain
        {
            using Base = RHI::DeviceSwapChain;
        public:
            AZ_RTTI(SwapChain, "{DF9FB230-1D9E-4865-96F9-3F04795BCE07}", Base);
            AZ_CLASS_ALLOCATOR(SwapChain, AZ::SystemAllocator);

            static RHI::Ptr<SwapChain> Create();
            
        private:
            SwapChain() = default;
            
            //////////////////////////////////////////////////////////////////////////
            // RHI::DeviceSwapChain
            RHI::ResultCode InitInternal(
                RHI::Device& deviceBase,
                const RHI::SwapChainDescriptor& descriptor,
                RHI::SwapChainDimensions* nativeDimensions) override;
            void ShutdownInternal() override;
            uint32_t PresentInternal() override;
            RHI::ResultCode InitImageInternal(const InitImageRequest& request) override;
            RHI::ResultCode ResizeInternal([[maybe_unused]] const RHI::SwapChainDimensions& dimensions, [[maybe_unused]] RHI::SwapChainDimensions* nativeDimensions) override { return RHI::ResultCode::Success;}
            //////////////////////////////////////////////////////////////////////////

            static wgpu::Surface BuildNativeSurface(const RHI::SwapChainDescriptor& descriptor);
            wgpu::SurfaceCapabilities GetSurfaceCapabilities(RHI::Device& deviceBase);
            wgpu::TextureFormat GetSupportedSurfaceFormat(const RHI::Format rhiFormat) const;
            wgpu::PresentMode GetSupportedPresentMode(uint32_t verticalSyncInterval) const;
            wgpu::CompositeAlphaMode GetSupportedCompositeAlpha() const;
            void AcquireNextImage(uint32_t imageIndex);

            //! The native surface object
            wgpu::Surface m_wgpuSurface = nullptr;
            //! Capabilities of the surface
            wgpu::SurfaceCapabilities m_wgpuSurfaceCapabilities = {};
            //! Selected format for the surface
            wgpu::TextureFormat m_wgpuSurfaceFormat = wgpu::TextureFormat::Undefined;
            //! Selected presentation mode
            wgpu::PresentMode m_wgpuPresentMode = wgpu::PresentMode::Fifo;
            wgpu::CompositeAlphaMode m_wgpuCompositeAlphaMode = wgpu::CompositeAlphaMode::Auto;
        };
    }
}
