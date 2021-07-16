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
#include <${Name}EditorSystemComponent.h>

namespace ${SanitizedCppName}
{
    void ${SanitizedCppName}EditorSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<${SanitizedCppName}EditorSystemComponent, ${SanitizedCppName}SystemComponent>()
                ->Version(0);
        }
    }

    ${SanitizedCppName}EditorSystemComponent::${SanitizedCppName}EditorSystemComponent() = default;

    ${SanitizedCppName}EditorSystemComponent::~${SanitizedCppName}EditorSystemComponent() = default;

    void ${SanitizedCppName}EditorSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        BaseSystemComponent::GetProvidedServices(provided);
        provided.push_back(AZ_CRC_CE("${SanitizedCppName}EditorService"));
    }

    void ${SanitizedCppName}EditorSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        BaseSystemComponent::GetIncompatibleServices(incompatible);
        incompatible.push_back(AZ_CRC_CE("${SanitizedCppName}EditorService"));
    }

    void ${SanitizedCppName}EditorSystemComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        BaseSystemComponent::GetRequiredServices(required);
    }

    void ${SanitizedCppName}EditorSystemComponent::GetDependentServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        BaseSystemComponent::GetDependentServices(dependent);
    }

    void ${SanitizedCppName}EditorSystemComponent::Activate()
    {
        ${SanitizedCppName}SystemComponent::Activate();
        AzToolsFramework::EditorEvents::Bus::Handler::BusConnect();
    }

    void ${SanitizedCppName}EditorSystemComponent::Deactivate()
    {
        AzToolsFramework::EditorEvents::Bus::Handler::BusDisconnect();
        ${SanitizedCppName}SystemComponent::Deactivate();
    }

} // namespace ${SanitizedCppName}
