/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include "${Name}EditorSystemComponent.h"

#include <${Name}/${Name}TypeIds.h>

namespace ${Name}
{
    AZ_COMPONENT_IMPL(${Name}EditorSystemComponent, "${Name}EditorSystemComponent",
        ${Name}EditorSystemComponentTypeId, BaseSystemComponent);

    void ${Name}EditorSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<${Name}EditorSystemComponent, ${Name}SystemComponent>()
                ->Version(0);
        }
    }

    ${Name}EditorSystemComponent::${Name}EditorSystemComponent() = default;

    ${Name}EditorSystemComponent::~${Name}EditorSystemComponent() = default;

    void ${Name}EditorSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        BaseSystemComponent::GetProvidedServices(provided);
        provided.push_back(AZ_CRC_CE("${Name}SystemEditorService"));
    }

    void ${Name}EditorSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        BaseSystemComponent::GetIncompatibleServices(incompatible);
        incompatible.push_back(AZ_CRC_CE("${Name}SystemEditorService"));
    }

    void ${Name}EditorSystemComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        BaseSystemComponent::GetRequiredServices(required);
    }

    void ${Name}EditorSystemComponent::GetDependentServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        BaseSystemComponent::GetDependentServices(dependent);
    }

    void ${Name}EditorSystemComponent::Activate()
    {
        ${Name}SystemComponent::Activate();
        AzToolsFramework::EditorEvents::Bus::Handler::BusConnect();
    }

    void ${Name}EditorSystemComponent::Deactivate()
    {
        AzToolsFramework::EditorEvents::Bus::Handler::BusDisconnect();
        ${Name}SystemComponent::Deactivate();
    }

} // namespace ${Name}
