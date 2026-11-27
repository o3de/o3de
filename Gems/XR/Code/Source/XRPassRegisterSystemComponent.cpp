/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <XR/XRPassRegisterSystemComponent.h>
#include <Atom/RPI.Public/Pass/PassSystemInterface.h>
#include <Passes/FoveatedImagePass.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace XR
{
    void PassRegisterSystemComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        // This module uses the PassSystem that is part of the RPI System Module.
        required.push_back(AZ_CRC_CE("RPISystem"));
    }

    void PassRegisterSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<PassRegisterSystemComponent, AZ::Component>()->Version(1);
        }
    }

    void PassRegisterSystemComponent::Activate()
    {
        auto* passSystem = AZ::RPI::PassSystemInterface::Get();
        AZ_Assert(passSystem, "Cannot get the pass system.");
        passSystem->AddPassCreator(AZ::Name(FoveatedImagePassClassName), &FoveatedImagePass::Create);

        // Setup handler for load pass template mappings
        m_loadTemplatesHandler = AZ::RPI::PassSystemInterface::OnReadyLoadTemplatesEvent::Handler([this]() { this->LoadPassTemplateMappings(); });
        passSystem->ConnectEvent(m_loadTemplatesHandler);
    }

    void PassRegisterSystemComponent::Deactivate()
    {
        m_loadTemplatesHandler.Disconnect();
    }

    void PassRegisterSystemComponent::LoadPassTemplateMappings() const
    {
        const char* passTemplatesFile = "Passes/OpenXRPassTemplates.azasset";
        AZ::RPI::PassSystemInterface::Get()->LoadPassTemplateMappings(passTemplatesFile);
    }
}