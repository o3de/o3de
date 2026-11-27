/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <XR/XRSystemComponent.h>
#include <XR/XRUtils.h>
#include <XR/XRFactory.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <Atom/RPI.Public/XR/XRRenderingInterface.h>
#include <Atom/RHI/ValidationLayer.h>
#include <Atom/RHI/FactoryManagerBus.h>

namespace XR
{
    void SystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("XRSystemService"));
    }

    void SystemComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC_CE("RHIService"));
    }

    void SystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<SystemComponent, AZ::Component>()->Version(1);
        }
    }

    void SystemComponent::Activate()
    {
        // Register XR system interface if openxr is enabled via command line or settings registry
        if (IsOpenXREnabled())
        {
            if (!Factory::IsReady())
            {
                AZ_Error("OpenXR", false, "OpenXR is enabled but no XR implementations are available. Unable to initialize XR system.");
                return;
            }

            //Get the validation mode
            AZ::RHI::ValidationMode validationMode = AZ::RHI::ValidationMode::Disabled;
            AZ::RHI::FactoryManagerBus::BroadcastResult(validationMode, &AZ::RHI::FactoryManagerRequest::DetermineValidationMode);

            //Init the XRSystem
            System::Descriptor descriptor;
            descriptor.m_validationMode = validationMode;
            m_xrSystem = aznew System();
            m_xrSystem->Init(descriptor);

            //Register xr system with RPI
            AZ::RPI::XRRegisterInterface::Get()->RegisterXRInterface(m_xrSystem.get());
        }
    }

    void SystemComponent::Deactivate()
    {
        if (m_xrSystem)
        {
            m_xrSystem->Shutdown();
            m_xrSystem.reset();
        }
    }
}