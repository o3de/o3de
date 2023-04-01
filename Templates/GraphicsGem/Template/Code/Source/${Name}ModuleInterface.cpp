/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "${Name}ModuleInterface.h"
#include <AzCore/Memory/Memory.h>

#include <${Name}/${Name}TypeIds.h>

#include <Clients/${Name}SystemComponent.h>

namespace ${Name}
{
    AZ_TYPE_INFO_WITH_NAME_IMPL(${Name}ModuleInterface,
        "${Name}ModuleInterface", ${Name}ModuleInterfaceTypeId);
    AZ_RTTI_NO_TYPE_INFO_IMPL(${Name}ModuleInterface, AZ::Module);
    AZ_CLASS_ALLOCATOR_IMPL(${Name}ModuleInterface, AZ::SystemAllocator);

    ${Name}ModuleInterface::${Name}ModuleInterface()
    {
        // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
        // Add ALL components descriptors associated with this gem to m_descriptors.
        // This will associate the AzTypeInfo information for the components with the the SerializeContext, BehaviorContext and EditContext.
        // This happens through the [MyComponent]::Reflect() function.
        m_descriptors.insert(m_descriptors.end(), {
            ${Name}SystemComponent::CreateDescriptor(),
            });
    }

    AZ::ComponentTypeList ${Name}ModuleInterface::GetRequiredSystemComponents() const
    {
        return AZ::ComponentTypeList{
            azrtti_typeid<${Name}SystemComponent>(),
        };
    }
} // namespace ${Name}
