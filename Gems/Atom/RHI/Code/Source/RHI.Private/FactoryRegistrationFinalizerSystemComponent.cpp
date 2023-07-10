/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <Atom/RHI/Factory.h>
#include <Atom/RHI/FactoryManagerBus.h>

#include <RHI.Private/FactoryRegistrationFinalizerSystemComponent.h>

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace AZ::RHI
{
    void FactoryRegistrationFinalizerSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context))
        {
            serializeContext->Class<FactoryRegistrationFinalizerSystemComponent, AZ::Component>()
                ->Version(0);
        }
    }

    void FactoryRegistrationFinalizerSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(RHI::Factory::GetComponentService());
    }

    void FactoryRegistrationFinalizerSystemComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        dependent.push_back(RHI::Factory::GetPlatformService());
    }

    void FactoryRegistrationFinalizerSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(RHI::Factory::GetComponentService());
    }

    void FactoryRegistrationFinalizerSystemComponent::Activate()
    {
        // This is the only job of this system component.
        FactoryManagerBus::Broadcast(&FactoryManagerRequest::FactoryRegistrationFinalize);
    }

    void FactoryRegistrationFinalizerSystemComponent::Deactivate()
    {
    }
}
