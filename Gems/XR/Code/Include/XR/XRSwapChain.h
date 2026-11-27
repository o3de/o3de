/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/smart_ptr/intrusive_ptr.h>
#include <Atom/RHI/ValidationLayer.h>
#include <Atom/RHI.Reflect/Format.h>
#include <XR/XRBase.h>
#include <XR/XRDevice.h>
#include <XR/XRObject.h>
#include <XR/XRSession.h>

namespace XR
{
    //! This class will be responsible for creating multiple XR::SwapChain::ViewSwapchains
    //! (one per view). Each XR::SwapChain::ViewSwapchain will then be responsible
    //! for manging and synchronizing multiple swap chain images
    class SwapChain
        : public XR::Object
    {
    public:
        AZ_CLASS_ALLOCATOR(SwapChain, AZ::SystemAllocator);
        AZ_RTTI(SwapChain, "{0C666E76-E4B7-4097-8D14-713DC2C446EF}");

        class Image
            : public AZStd::intrusive_base
        {
        public:
            AZ_CLASS_ALLOCATOR(Image, AZ::SystemAllocator);
            AZ_RTTI(Image, "{4037835D-F1BB-4407-BC98-2299CC7BE0A3}");

            Image() = default;
            ~Image() override = default;
        };

        class View
            : public AZStd::intrusive_base
        {
        public:
            AZ_CLASS_ALLOCATOR(View, AZ::SystemAllocator);
            AZ_RTTI(View, "{774EB724-8261-4684-AA78-EDF6BBECD48A}");

            View() = default;
            ~View() override = default;

            virtual void Shutdown() = 0;
            virtual AZ::u32 GetCurrentImageIndex() const = 0;
            //! All the images associated with this View
            AZStd::vector<Ptr<SwapChain::Image>> m_images;

            //! The active image index
            AZ::u32 m_activeImageIndex = 0;

            //! Number of swapchain images associated with this view.
            AZ::u32 m_numImages = 0;

            //! Returns if an image acquired for this view.
            bool m_isImageAcquired = false;

            //! Width of the swap chain view.
            AZ::u32 m_width = 0;

            //! Height of the swap chain view.
            AZ::u32 m_height = 0;
        };

        struct Descriptor
        {
            AZ::RHI::ValidationMode m_validationMode = AZ::RHI::ValidationMode::Disabled;
            Ptr<Instance> m_instance;
            Ptr<Session> m_session;
            Ptr<Device> m_device;
        };

        //! Returns the view swap chain related to the index.
        SwapChain::View* GetView(AZ::u32 swapChainIndex) const;

        //! Returns the image associated with the provided image
        //! index and view swap chain index.
        SwapChain::Image* GetImage(AZ::u32 imageIndex, AZ::u32 swapChainIndex) const;

        //! Initialize the XR swapchain.
        AZ::RHI::ResultCode Init(const Descriptor& descriptor);

        //! Get the descriptor.
        const Descriptor& GetDescriptor() const;

        //! Get the number of Xr views
        AZ::u32 GetNumViews() const;

        //! Api to allow the back end object to return the requested native swapchain image
        virtual AZ::RHI::ResultCode GetSwapChainImage(AZ::RHI::XRSwapChainDescriptor* swapchainDescriptor) const = 0;

        //! Api to allow the back end to report the recommended swapchain width
        virtual AZ::u32 GetSwapChainWidth(AZ::u32 viewIndex) const = 0;

        //! Api to allow the back end to report the recommended swapchain height
        virtual AZ::u32 GetSwapChainHeight(AZ::u32 viewIndex) const = 0;

        //! Api to allow the back end to report the swapchain format.
        virtual AZ::RHI::Format GetSwapChainFormat(AZ::u32 viewIndex) const = 0;

    protected:
        //! Number of Xr views
        AZ::u32 m_numViews = 0;

        //! Vector to hold all the SwapChain View objects
        AZStd::vector<Ptr<SwapChain::View>> m_viewSwapchains;

    private:
        ///////////////////////////////////////////////////////////////////
        // XR::Object override
        void Shutdown() override;
        ///////////////////////////////////////////////////////////////////

        //! Called when the swapchain is being shutdown.
        virtual void ShutdownInternal() = 0;

        //! Api to allow the back end object to initialize
        virtual AZ::RHI::ResultCode InitInternal() = 0;

        Descriptor m_descriptor;
    };
} // namespace XR
