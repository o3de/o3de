// {BEGIN_LICENSE}
/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
// {END_LICENSE}

#include "${Name}ModuleInterface.h"
#include <AzCore/Memory/Memory.h>

#include <${Name}/${Name}TypeIds.h>

#include <Unified/${Name}SystemComponent.h>

namespace ${SanitizedCppName}
{
    AZ_TYPE_INFO_WITH_NAME_IMPL(${SanitizedCppName}ModuleInterface,
        "${SanitizedCppName}ModuleInterface", ${SanitizedCppName}ModuleInterfaceTypeId);
    AZ_RTTI_NO_TYPE_INFO_IMPL(${SanitizedCppName}ModuleInterface, AZ::Module);
    AZ_CLASS_ALLOCATOR_IMPL(${SanitizedCppName}ModuleInterface, AZ::SystemAllocator);

    ${SanitizedCppName}ModuleInterface::${SanitizedCppName}ModuleInterface()
    {
        // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
        // Add ALL components descriptors associated with this gem to m_descriptors.
        // This will associate the AzTypeInfo information for the components with the the SerializeContext, BehaviorContext and EditContext.
        // This happens through the [MyComponent]::Reflect() function.
        m_descriptors.insert(m_descriptors.end(), {
            ${SanitizedCppName}SystemComponent::CreateDescriptor(),
            });
    }

    AZ::ComponentTypeList ${SanitizedCppName}ModuleInterface::GetRequiredSystemComponents() const
    {
        return AZ::ComponentTypeList{
            azrtti_typeid<${SanitizedCppName}SystemComponent>(),
        };
    }
} // namespace ${SanitizedCppName}
