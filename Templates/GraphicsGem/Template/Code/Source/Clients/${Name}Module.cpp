/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <${Name}/${Name}TypeIds.h>
#include <${Name}ModuleInterface.h>
#include "${Name}SystemComponent.h"

#include <AzCore/RTTI/RTTI.h>

#include <Components/${Name}Component.h>

namespace ${Name}
{
    class ${Name}Module
        : public ${Name}ModuleInterface
    {
    public:
        AZ_RTTI(${Name}Module, ${Name}ModuleTypeId, ${Name}ModuleInterface);
        AZ_CLASS_ALLOCATOR(${Name}Module, AZ::SystemAllocator);

        ${Name}Module()
        {
            m_descriptors.insert(m_descriptors.end(),
                {
                    ${Name}SystemComponent::CreateDescriptor(),
                    ${Name}Component::CreateDescriptor(),
                });
        }

        AZ::ComponentTypeList GetRequiredSystemComponents() const
        {
            return AZ::ComponentTypeList{ azrtti_typeid<${Name}SystemComponent>() };
        }
    };
}// namespace ${Name}

AZ_DECLARE_MODULE_CLASS(Gem_${Name}, ${Name}::${Name}Module)
