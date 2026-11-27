/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <AzCore/Serialization/SerializeContext.h>

#include <Atom/RHI/FactoryManagerBus.h>

#include <OpenXRVk/OpenXRVkDevice.h>
#include <OpenXRVk/OpenXRVkInput.h>
#include <OpenXRVk/OpenXRVkInstance.h>
#include <OpenXRVk/OpenXRVkSession.h>
#include <OpenXRVk/OpenXRVkSpace.h>
#include <OpenXRVk/OpenXRVkSwapChain.h>
#include <OpenXRVk/OpenXRVkSystemComponent.h>

#include "OpenXRVkBehaviorReflection.h"

#include <XR/XRUtils.h>

namespace OpenXRVk
{
    void SystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(XR::Factory::GetPlatformService());
        provided.push_back(AZ_CRC_CE("VulkanRequirementsService"));
    }

    void SystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<SystemComponent, AZ::Component>()
                ->Version(1);
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            OpenXRBehaviorReflect(*behaviorContext);
        }

        AzFramework::InputDeviceXRController::Reflect(context);
        OpenXRInteractionProfilesAsset::Reflect(context);
        OpenXRActionSetsAsset::Reflect(context);
    }

    XR::Ptr<XR::Instance> SystemComponent::CreateInstance()
    {
        return Instance::Create();
    }

    XR::Ptr<XR::Device> SystemComponent::CreateDevice()
    {
        return Device::Create();
    }

    XR::Ptr<XR::Session> SystemComponent::CreateSession()
    {
        return Session::Create();
    }
    
    XR::Ptr<XR::Input> SystemComponent::CreateInput()
    {
        return Input::Create();
    }

    XR::Ptr<XR::Space> SystemComponent::CreateSpace()
    {
        return Space::Create();
    }

    XR::Ptr<XR::SwapChain> SystemComponent::CreateSwapChain()
    {
        return SwapChain::Create();
    }

    XR::Ptr<XR::SwapChain::View> SystemComponent::CreateSwapChainView()
    {
        return SwapChain::View::Create();
    }

    XR::Ptr<XR::SwapChain::Image> SystemComponent::CreateSwapChainImage()
    {
        return SwapChain::Image::Create();
    }

    void SystemComponent::Activate()
    {
        m_actionSetsAssetHandler = AZStd::make_unique<OpenXRActionSetsAssetHandler>();
        m_actionSetsAssetHandler->Register();

        m_interactionProfilesAssetHandler = AZStd::make_unique<OpenXRInteractionProfilesAssetHandler>();
        m_interactionProfilesAssetHandler->Register();

        if (XR::IsOpenXREnabled())
        {
            m_instance = AZStd::static_pointer_cast<OpenXRVk::Instance>(CreateInstance());
            //Get the validation mode
            AZ::RHI::ValidationMode validationMode = AZ::RHI::ValidationMode::Disabled;
            AZ::RHI::FactoryManagerBus::BroadcastResult(validationMode, &AZ::RHI::FactoryManagerRequest::DetermineValidationMode);

            if (m_instance->Init(validationMode) == AZ::RHI::ResultCode::Success)
            {
                XR::Factory::Register(this);
                AZ::Interface<XR::Instance>::Register(m_instance.get());
            }
            else
            {
                AZ_Warning("OpenXRVK", false, "OpenXRVK is not supported on this platform");
                m_instance = nullptr;
            }
        }
    }

    void SystemComponent::Deactivate()
    {
        if (m_instance)
        {
            XR::Factory::Unregister(this);
            AZ::Interface<XR::Instance>::Unregister(m_instance.get());
            m_instance = nullptr;
        }

        m_actionSetsAssetHandler->Unregister();
        m_interactionProfilesAssetHandler->Unregister();
    }

}