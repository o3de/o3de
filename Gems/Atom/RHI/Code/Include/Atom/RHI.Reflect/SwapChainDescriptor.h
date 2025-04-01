/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/AttachmentId.h>
#include <Atom/RHI.Reflect/Handle.h>
#include <Atom/RHI.Reflect/Format.h>
#include <Atom/RHI.Reflect/ResourcePoolDescriptor.h>

namespace AZ
{
    class ReflectContext;
}

namespace AZ::RHI
{
    using WindowHandle = Handle<uint64_t, class Window>;

    struct SwapChainDimensions
    {
        AZ_TYPE_INFO(SwapChainDimensions, "{1B1D266F-15FA-4EA6-B28C-B87467844617}");

        /// Number of images in the swap chain.
        uint32_t m_imageCount = 0;

        /// Pixel width of the images in the swap chain.
        uint32_t m_imageWidth = 0;

        /// Pixel height of the images in the swap chain.
        uint32_t m_imageHeight = 0;

        /// Pixel format of the images in the swap chain.
        Format m_imageFormat = Format::Unknown;
    };

    class SwapChainDescriptor
        : public ResourcePoolDescriptor
    {
    public:
        virtual ~SwapChainDescriptor() = default;
        AZ_RTTI(SwapChainDescriptor, "{214C7DD0-C380-45B6-8021-FD0C43CF5C05}", ResourcePoolDescriptor);
        AZ_CLASS_ALLOCATOR(SwapChainDescriptor, SystemAllocator)
        static void Reflect(AZ::ReflectContext* context);

        // The dimensions and format of the swap chain images.
        SwapChainDimensions m_dimensions;

        // 0: disable VSync. >= 1: sync N vertical blanks.
        uint32_t m_verticalSyncInterval = 0;

        // [Not Reflected] API dependent handle to the OS window to attach the swap chain.
        WindowHandle m_window;

        // ID for the SwapChain's attachment.
        RHI::AttachmentId m_attachmentId;

        // Dictates if it is a XR swapchain.
        bool m_isXrSwapChain = false;

        // Return the index of a XR swapchain
        // as you can have multiple XR swapchains (one per view).
        uint32_t m_xrSwapChainIndex = 0;

        // The scaling mode to use when presenting the swapchain's back buffer to the target
        // Note: not all platforms support stretch or stretch with aspect ratio.
        // Use DeviceFeature::m_swapChainScalingFlags to find out supported stretch modes
        Scaling m_scalingMode = RHI::Scaling::None;
    };
}
