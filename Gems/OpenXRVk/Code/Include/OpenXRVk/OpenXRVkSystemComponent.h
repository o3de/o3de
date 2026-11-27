/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <XR/XRFactory.h>
#include <OpenXRVk/OpenXRVkInstance.h>
#include <AzCore/Component/Component.h>

#include <OpenXRVk/OpenXRVkInteractionProfilesAsset.h>
#include <OpenXRVk/OpenXRVkActionSetsAsset.h>

namespace OpenXRVk
{
    //! This class is the component related to the vulkan backend of XR.
    class SystemComponent final
        : public AZ::Component
        , public XR::Factory
    {
    public:
        AZ_COMPONENT(SystemComponent, "{C0ABD1CE-FD3C-48C3-8AE8-C098BCCFC604}");

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void Reflect(AZ::ReflectContext* context);

        SystemComponent() = default;
        ~SystemComponent() = default;

        //////////////////////////////////////////////////////////////////////////
        // Component
        void Activate() override;
        void Deactivate() override;
        //////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////
        // XR::Factory overrides
        //! Create OpenXRVk::Instance object
        XR::Ptr<XR::Instance> CreateInstance() override;

        //! Create OpenXRVk::Device object
        XR::Ptr<XR::Device> CreateDevice() override;

        //! Create XR::Session object.
        XR::Ptr<XR::Session> CreateSession() override;

        //! Create XR::Input object.
        XR::Ptr<XR::Input> CreateInput() override;

        //! Create XR::Space object.
        XR::Ptr<XR::Space> CreateSpace() override;

        //! Create XR::Swapchain object.
        XR::Ptr<XR::SwapChain> CreateSwapChain() override;

        //! Create XR::Swapchain::View object.
        XR::Ptr<XR::SwapChain::View> CreateSwapChainView() override;

        //! Create XR::Swapchain::Image object.
        XR::Ptr<XR::SwapChain::Image> CreateSwapChainImage() override;
        ///////////////////////////////////////////////////////////////////

    private:
        XR::Ptr<OpenXRVk::Instance> m_instance;

        AZStd::unique_ptr<OpenXRInteractionProfilesAssetHandler> m_interactionProfilesAssetHandler;
        AZStd::unique_ptr<OpenXRActionSetsAssetHandler> m_actionSetsAssetHandler;
    };
}