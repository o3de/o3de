// {BEGIN_LICENSE}
/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
// {END_LICENSE}

#include "${SanitizedCppName}ModuleInterface.h"
#include <Clients/${SanitizedCppName}SystemComponent.h>
#include <Components/${SanitizedCppName}Component.h>

namespace ${SanitizedCppName}
{
    ${SanitizedCppName}ModuleInterface::${SanitizedCppName}ModuleInterface()
    {
        m_descriptors.insert(m_descriptors.end(), {
            ${SanitizedCppName}SystemComponent::CreateDescriptor(),
            ${SanitizedCppName}Component::CreateDescriptor(),
        });
    }

    AZ::ComponentTypeList ${SanitizedCppName}ModuleInterface::GetRequiredSystemComponents() const
    {
        return AZ::ComponentTypeList{
            azrtti_typeid<${SanitizedCppName}SystemComponent>(),
        };
    }
} // namespace ${SanitizedCppName}
