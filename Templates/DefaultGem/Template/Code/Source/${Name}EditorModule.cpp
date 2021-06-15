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

#include <${Name}ModuleInterface.h>
#include <${Name}EditorSystemComponent.h>

namespace ${SanitizedCppName}
{
    class ${SanitizedCppName}EditorModule
        : public ${SanitizedCppName}ModuleInterface
    {
    public:
        AZ_RTTI(${SanitizedCppName}EditorModule, "${ModuleClassId}", ${SanitizedCppName}ModuleInterface);
        AZ_CLASS_ALLOCATOR(${SanitizedCppName}EditorModule, AZ::SystemAllocator, 0);

        ${SanitizedCppName}EditorModule()
        {
            // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
            // Add ALL components descriptors associated with this gem to m_descriptors.
            // This will associate the AzTypeInfo information for the components with the the SerializeContext, BehaviorContext and EditContext.
            // This happens through the [MyComponent]::Reflect() function.
            m_descriptors.insert(m_descriptors.end(), {
                ${SanitizedCppName}EditorSystemComponent::CreateDescriptor(),
            });
        }

        /**
         * Add required SystemComponents to the SystemEntity.
         * Non-SystemComponents should not be added here
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList {
                azrtti_typeid<${SanitizedCppName}EditorSystemComponent>(),
            };
        }
    };
}// namespace ${SanitizedCppName}

AZ_DECLARE_MODULE_CLASS(Gem_${SanitizedCppName}, ${SanitizedCppName}::${SanitizedCppName}EditorModule)
