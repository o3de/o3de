/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <RecastNavigationSystemComponent.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace RecastNavigation
{
    void RecastNavigationSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<RecastNavigationSystemComponent, AZ::Component>()
                ->Version(0)
                ;

            if (AZ::EditContext* editContext = serialize->GetEditContext())
            {
                editContext->Class<RecastNavigationSystemComponent>("RecastNavigation", "[System Component for the Recast Navigation gem]")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }
    }

    void RecastNavigationSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("RecastNavigationService"));
    }

    void RecastNavigationSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("RecastNavigationService"));
    }

    void RecastNavigationSystemComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
    {
    }

    void RecastNavigationSystemComponent::GetDependentServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
    }

    RecastNavigationSystemComponent::RecastNavigationSystemComponent()
    {
        if (RecastNavigationInterface::Get() == nullptr)
        {
            RecastNavigationInterface::Register(this);
        }
    }

    RecastNavigationSystemComponent::~RecastNavigationSystemComponent()
    {
        if (RecastNavigationInterface::Get() == this)
        {
            RecastNavigationInterface::Unregister(this);
        }
    }

    void RecastNavigationSystemComponent::Init()
    {
    }

    void RecastNavigationSystemComponent::Activate()
    {
        RecastNavigationRequestBus::Handler::BusConnect();
        AZ::TickBus::Handler::BusConnect();
    }

    void RecastNavigationSystemComponent::Deactivate()
    {
        AZ::TickBus::Handler::BusDisconnect();
        RecastNavigationRequestBus::Handler::BusDisconnect();
    }

    void RecastNavigationSystemComponent::OnTick([[maybe_unused]] float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint time)
    {
    }

} // namespace RecastNavigation
