/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <${Name}/${Name}TypeIds.h>
#include <${Name}ModuleInterface.h>
#include "${Name}EditorSystemComponent.h"
#include "Components/Editor${Name}Component.h"

namespace ${Name}
{
    class ${Name}EditorModule
        : public ${Name}ModuleInterface
    {
    public:
        AZ_RTTI(${Name}EditorModule, ${Name}EditorModuleTypeId, ${Name}ModuleInterface);
        AZ_CLASS_ALLOCATOR(${Name}EditorModule, AZ::SystemAllocator);

        ${Name}EditorModule()
        {
            // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
            // Add ALL components descriptors associated with this gem to m_descriptors.
            // This will associate the AzTypeInfo information for the components with the the SerializeContext, BehaviorContext and EditContext.
            // This happens through the [MyComponent]::Reflect() function.
            m_descriptors.insert(m_descriptors.end(), {
                ${Name}EditorSystemComponent::CreateDescriptor(),
                ${Name}Component::CreateDescriptor(),
                Editor${Name}Component::CreateDescriptor(),
            });
        }

        /**
         * Add required SystemComponents to the SystemEntity.
         * Non-SystemComponents should not be added here
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList {
                azrtti_typeid<${Name}EditorSystemComponent>(),
            };
        }
    };
}// namespace ${Name}

AZ_DECLARE_MODULE_CLASS(Gem_${Name}, ${Name}::${Name}EditorModule)
