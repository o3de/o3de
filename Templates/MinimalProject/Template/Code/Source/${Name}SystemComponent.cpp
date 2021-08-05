// {BEGIN_LICENSE}
/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
 // {END_LICENSE}

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/EditContextConstants.inl>

#include "${Name}SystemComponent.h"

namespace ${SanitizedCppName}
{
    void ${SanitizedCppName}SystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<${SanitizedCppName}SystemComponent, AZ::Component>()
                ->Version(0)
                ;

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<${SanitizedCppName}SystemComponent>("${SanitizedCppName}", "[Description of functionality provided by this System Component]")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System"))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }
    }

    void ${SanitizedCppName}SystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("${SanitizedCppName}Service"));
    }

    void ${SanitizedCppName}SystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("${SanitizedCppName}Service"));
    }

    void ${SanitizedCppName}SystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        AZ_UNUSED(required);
    }

    void ${SanitizedCppName}SystemComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        AZ_UNUSED(dependent);
    }
    
    ${SanitizedCppName}SystemComponent::${SanitizedCppName}SystemComponent()
    {
        if (${SanitizedCppName}Interface::Get() == nullptr)
        {
            ${SanitizedCppName}Interface::Register(this);
        }
    }

    ${SanitizedCppName}SystemComponent::~${SanitizedCppName}SystemComponent()
    {
        if (${SanitizedCppName}Interface::Get() == this)
        {
            ${SanitizedCppName}Interface::Unregister(this);
        }
    }

    void ${SanitizedCppName}SystemComponent::Init()
    {
    }

    void ${SanitizedCppName}SystemComponent::Activate()
    {
        ${SanitizedCppName}RequestBus::Handler::BusConnect();
    }

    void ${SanitizedCppName}SystemComponent::Deactivate()
    {
        ${SanitizedCppName}RequestBus::Handler::BusDisconnect();
    }
}
