// {BEGIN_LICENSE}
/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
// {END_LICENSE}

#include "${SanitizedCppName}SystemComponent.h"

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/RTTI/BehaviorContext.h>

namespace ${GemName}
{
    AZ_COMPONENT_IMPL(${SanitizedCppName}SystemComponent, "${SanitizedCppName}SystemComponent", "{${Random_Uuid}}");

    void ${SanitizedCppName}SystemComponent::Activate()
    {
        ${SanitizedCppName}RequestBus::Handler::BusConnect(GetEntityId());
    }

    void ${SanitizedCppName}SystemComponent::Deactivate()
    {
        ${SanitizedCppName}RequestBus::Handler::BusDisconnect(GetEntityId());
    }

    void ${SanitizedCppName}SystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<${SanitizedCppName}SystemComponent, AZ::Component>()
                ->Version(1)
                ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<${SanitizedCppName}SystemComponent>("${SanitizedCppName}SystemComponent", "[Description of functionality provided by this component]")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "ComponentCategory")
                    ->Attribute(AZ::Edit::Attributes::Icon, "Icons/Components/Component_Placeholder.svg")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
                    ;
            }
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<${SanitizedCppName}SystemComponent>("${SanitizedCppName} Component Group")
                ->Attribute(AZ::Script::Attributes::Category, "${GemName} Gem Group")
                ;
        }
    }

    void ${SanitizedCppName}SystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("${SanitizedCppName}SystemComponentService"));
    }

    void ${SanitizedCppName}SystemComponent::GetIncompatibleServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
    }

    void ${SanitizedCppName}SystemComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
    {
    }

    void ${SanitizedCppName}SystemComponent::GetDependentServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
    }
} // namespace ${GemName}
