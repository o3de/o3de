/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/EditContextConstants.inl>
#include <Source/AutoGen/AutoComponentTypes.h>
#include <Multiplayer/IMultiplayer.h>
#include <AzCore/Console/ILogger.h>

#include <AutomatedTestingSystemComponent.h>
#include <Multiplayer/ReplicationWindows/IReplicationWindow.h>

namespace AutomatedTesting
{
    void AutomatedTestingSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<AutomatedTestingSystemComponent, AZ::Component>()
                ->Version(0)
                ;

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<AutomatedTestingSystemComponent>("AutomatedTesting", "[Description of functionality provided by this System Component]")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }
    }

    void AutomatedTestingSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("AutomatedTestingService"));
    }

    void AutomatedTestingSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("AutomatedTestingService"));
    }

    void AutomatedTestingSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC_CE("MultiplayerService"));
    }

    void AutomatedTestingSystemComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        AZ_UNUSED(dependent);
    }

    void AutomatedTestingSystemComponent::Init()
    {
    }

    void AutomatedTestingSystemComponent::Activate()
    {
        AutomatedTestingRequestBus::Handler::BusConnect();
        RegisterMultiplayerComponents(); //< Register AutomatedTesting's multiplayer components to assign NetComponentIds
    }

    void AutomatedTestingSystemComponent::Deactivate()
    {
        AutomatedTestingRequestBus::Handler::BusDisconnect();
    }
}
