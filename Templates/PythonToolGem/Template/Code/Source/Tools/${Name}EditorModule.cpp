// {BEGIN_LICENSE}
/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
// {END_LICENSE}

#include <${Name}/${Name}TypeIds.h>
#include <${Name}ModuleInterface.h>
#include "${Name}EditorSystemComponent.h"
#include <AzToolsFramework/API/PythonLoader.h>

#include <QtGlobal>

void Init${SanitizedCppName}Resources()
{
    // We must register our Qt resources (.qrc file) since this is being loaded from a separate module (gem)
    Q_INIT_RESOURCE(${SanitizedCppName});
}

namespace ${SanitizedCppName}
{
    class ${SanitizedCppName}EditorModule
        : public ${SanitizedCppName}ModuleInterface
        , public AzToolsFramework::EmbeddedPython::PythonLoader
    {
    public:
        AZ_RTTI(${SanitizedCppName}EditorModule, ${SanitizedCppName}EditorModuleTypeId, ${SanitizedCppName}ModuleInterface);
        AZ_CLASS_ALLOCATOR(${SanitizedCppName}EditorModule, AZ::SystemAllocator);

        ${SanitizedCppName}EditorModule()
        {
            Init${SanitizedCppName}Resources();

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

#if defined(O3DE_GEM_NAME)
AZ_DECLARE_MODULE_CLASS(AZ_JOIN(Gem_, O3DE_GEM_NAME, _Editor), ${SanitizedCppName}::${SanitizedCppName}EditorModule)
#else
AZ_DECLARE_MODULE_CLASS(Gem_${SanitizedCppName}_Editor, ${SanitizedCppName}::${SanitizedCppName}EditorModule)
#endif
