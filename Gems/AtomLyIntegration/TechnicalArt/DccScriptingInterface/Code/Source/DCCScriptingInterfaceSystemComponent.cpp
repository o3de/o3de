/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <DCCScriptingInterfaceSystemComponent.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/EditContextConstants.inl>

namespace DCCScriptingInterface
{
    void DCCScriptingInterfaceSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<DCCScriptingInterfaceSystemComponent, AZ::Component>()
                ->Version(0)
                ;

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<DCCScriptingInterfaceSystemComponent>("DCCScriptingInterface", "[Description of functionality provided by this System Component]")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System"))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }
    }

    void DCCScriptingInterfaceSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("DCCScriptingInterfaceService"));
    }

    void DCCScriptingInterfaceSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("DCCScriptingInterfaceService"));
    }

    void DCCScriptingInterfaceSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        AZ_UNUSED(required);
    }

    void DCCScriptingInterfaceSystemComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        AZ_UNUSED(dependent);
    }

    void DCCScriptingInterfaceSystemComponent::Init()
    {
    }

    void DCCScriptingInterfaceSystemComponent::Activate()
    {
        DCCScriptingInterfaceRequestBus::Handler::BusConnect();
    }

    void DCCScriptingInterfaceSystemComponent::Deactivate()
    {
        DCCScriptingInterfaceRequestBus::Handler::BusDisconnect();
    }

} // namespace DCCScriptingInterface
