// {BEGIN_LICENSE}
/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
// {END_LICENSE}

#include <Clients/${SanitizedCppName}SystemComponent.h>
#include <${Name}/${SanitizedCppName}TypeIds.h>

#include <AzCore/Serialization/SerializeContext.h>

namespace ${SanitizedCppName}
{
    AZ_COMPONENT_IMPL(${SanitizedCppName}SystemComponent, "${SanitizedCppName}SystemComponent", ${SanitizedCppName}SystemComponentTypeId);

    void ${SanitizedCppName}SystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<${SanitizedCppName}SystemComponent, AZ::Component>()
                ->Version(1)
                ;
        }
    }

    void ${SanitizedCppName}SystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("${SanitizedCppName}SystemService"));
    }

    void ${SanitizedCppName}SystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("${SanitizedCppName}SystemService"));
    }

    void ${SanitizedCppName}SystemComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
    {
    }

    void ${SanitizedCppName}SystemComponent::GetDependentServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
    }

    void ${SanitizedCppName}SystemComponent::Init()
    {
    }

    void ${SanitizedCppName}SystemComponent::Activate()
    {
    }

    void ${SanitizedCppName}SystemComponent::Deactivate()
    {
    }
} // namespace ${SanitizedCppName}
