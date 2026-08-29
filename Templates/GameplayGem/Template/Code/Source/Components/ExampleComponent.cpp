// {BEGIN_LICENSE}
/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
// {END_LICENSE}

#include <Components/${SanitizedCppName}Component.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace ${SanitizedCppName}
{
    AZ_COMPONENT_IMPL(${SanitizedCppName}Component, "${SanitizedCppName}Component", ${SanitizedCppName}ComponentTypeId);

    ${SanitizedCppName}Component::${SanitizedCppName}Component(const ${SanitizedCppName}ComponentConfig& config)
        : m_controller(config)
    {
    }

    void ${SanitizedCppName}Component::Reflect(AZ::ReflectContext* context)
    {
        ${SanitizedCppName}ComponentController::Reflect(context);

        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<${SanitizedCppName}Component, AZ::Component>()
                ->Version(1)
                ->Field("Controller", &${SanitizedCppName}Component::m_controller)
                ;
        }
    }

    void ${SanitizedCppName}Component::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("${SanitizedCppName}Service"));
    }

    void ${SanitizedCppName}Component::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("${SanitizedCppName}Service"));
    }

    void ${SanitizedCppName}Component::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
    {
    }

    void ${SanitizedCppName}Component::GetDependentServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
    }

    void ${SanitizedCppName}Component::Activate()
    {
        m_controller.Activate(GetEntityId());
    }

    void ${SanitizedCppName}Component::Deactivate()
    {
        m_controller.Deactivate();
    }
} // namespace ${SanitizedCppName}
