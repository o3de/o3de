// {BEGIN_LICENSE}
/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
// {END_LICENSE}

#include <${SanitizedCppName}ModuleInterface.h>
#include <Tools/${SanitizedCppName}EditorSystemComponent.h>
#include <Tools/Components/Editor${SanitizedCppName}Component.h>

namespace ${SanitizedCppName}
{
    class ${SanitizedCppName}EditorModule
        : public ${SanitizedCppName}ModuleInterface
    {
    public:
        AZ_RTTI(${SanitizedCppName}EditorModule, ${SanitizedCppName}EditorModuleTypeId, ${SanitizedCppName}ModuleInterface);
        AZ_CLASS_ALLOCATOR(${SanitizedCppName}EditorModule, AZ::SystemAllocator);

        ${SanitizedCppName}EditorModule()
        {
            m_descriptors.insert(m_descriptors.end(), {
                ${SanitizedCppName}EditorSystemComponent::CreateDescriptor(),
                Editor${SanitizedCppName}Component::CreateDescriptor(),
            });
        }

        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList{
                azrtti_typeid<${SanitizedCppName}EditorSystemComponent>(),
            };
        }
    };
} // namespace ${SanitizedCppName}

AZ_DECLARE_MODULE_CLASS(AZ_JOIN(Gem_, O3DE_GEM_NAME, _Editor), ${SanitizedCppName}::${SanitizedCppName}EditorModule)
