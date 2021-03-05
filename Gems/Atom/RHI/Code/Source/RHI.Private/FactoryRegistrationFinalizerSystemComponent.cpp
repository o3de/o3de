/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/


#include <Atom/RHI/Factory.h>
#include <Atom/RHI/FactoryManagerBus.h>

#include <RHI.Private/FactoryRegistrationFinalizerSystemComponent.h>

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace AZ
{
    namespace RHI
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
    } // namespace RHI
} // namespace AZ
