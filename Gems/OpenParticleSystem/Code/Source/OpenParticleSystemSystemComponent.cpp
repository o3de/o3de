/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "OpenParticleSystemSystemComponent.h"

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/EditContextConstants.inl>

namespace OpenParticleSystem
{
    void OpenParticleSystemSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<OpenParticleSystemSystemComponent, AZ::Component>()->Version(0);

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<OpenParticleSystemSystemComponent>(
                      "OpenParticleSystem", "[Description of functionality provided by this System Component]")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System"))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true);
            }
        }
    }

    void OpenParticleSystemSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("OpenParticleSystemService"));
    }

    void OpenParticleSystemSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("OpenParticleSystemService"));
    }

    void OpenParticleSystemSystemComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
    {
    }

    void OpenParticleSystemSystemComponent::GetDependentServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
    }

    OpenParticleSystemSystemComponent::OpenParticleSystemSystemComponent()
    {
        if (OpenParticleSystemInterface::Get() == nullptr)
        {
            OpenParticleSystemInterface::Register(this);
        }
    }

    OpenParticleSystemSystemComponent::~OpenParticleSystemSystemComponent()
    {
        if (OpenParticleSystemInterface::Get() == this)
        {
            OpenParticleSystemInterface::Unregister(this);
        }
    }

    void OpenParticleSystemSystemComponent::Init()
    {
    }

    void OpenParticleSystemSystemComponent::Activate()
    {
        OpenParticleSystemRequestBus::Handler::BusConnect();
        AZ::TickBus::Handler::BusConnect();
    }

    void OpenParticleSystemSystemComponent::Deactivate()
    {
        AZ::TickBus::Handler::BusDisconnect();
        OpenParticleSystemRequestBus::Handler::BusDisconnect();
    }

    void OpenParticleSystemSystemComponent::OnTick([[maybe_unused]] float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint time)
    {
    }

} // namespace OpenParticleSystem
