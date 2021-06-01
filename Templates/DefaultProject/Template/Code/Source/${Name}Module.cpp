// {BEGIN_LICENSE}
/*
 * All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
 * its licensors.
 *
 * For complete copyright and license terms please see the LICENSE at the root of this
 * distribution (the "License"). All use of this software is governed by the License,
 * or, if provided, by the license below or the license accompanying this file. Do not
 * remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *
 */
 // {END_LICENSE}

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Module/Module.h>

#include "${Name}SystemComponent.h"

namespace ${Name}
{
    class ${Name}Module
        : public AZ::Module
    {
    public:
        AZ_RTTI(${Name}Module, "${ModuleClassId}", AZ::Module);
        AZ_CLASS_ALLOCATOR(${Name}Module, AZ::SystemAllocator, 0);

        ${Name}Module()
            : AZ::Module()
        {
            // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
            m_descriptors.insert(m_descriptors.end(), {
                ${Name}SystemComponent::CreateDescriptor(),
            });
        }

        /**
         * Add required SystemComponents to the SystemEntity.
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList{
                azrtti_typeid<${Name}SystemComponent>(),
            };
        }
    };
}// namespace ${Name}

AZ_DECLARE_MODULE_CLASS(Gem_${Name}, ${Name}::${Name}Module)
