/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
 
#include <DccScriptingInterfaceModuleInterface.h>
#include "DccScriptingInterfaceEditorSystemComponent.h"
#include <AzToolsFramework/API/PythonLoader.h>

#include <QtGlobal>

void InitDccScriptingInterfaceResources()
{
    // We must register our Qt resources (.qrc file) since this is being loaded from a separate module (gem)
    Q_INIT_RESOURCE(DccScriptingInterface);
}

namespace DccScriptingInterface
{
    class DccScriptingInterfaceEditorModule
        : public DccScriptingInterfaceModuleInterface
        , public AzToolsFramework::EmbeddedPython::PythonLoader
    {
    public:
        AZ_RTTI(DccScriptingInterfaceEditorModule, "{F6CEC69D-14DB-48F8-9AFC-D56D0602D79F}", DccScriptingInterfaceModuleInterface);
        AZ_CLASS_ALLOCATOR(DccScriptingInterfaceEditorModule, AZ::SystemAllocator);

        DccScriptingInterfaceEditorModule()
        {
            InitDccScriptingInterfaceResources();

            // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
            // Add ALL components descriptors associated with this gem to m_descriptors.
            // This will associate the AzTypeInfo information for the components with the the SerializeContext, BehaviorContext and EditContext.
            // This happens through the [MyComponent]::Reflect() function.
            m_descriptors.insert(m_descriptors.end(), {
                DccScriptingInterfaceEditorSystemComponent::CreateDescriptor(),
            });
        }

        /**
         * Add required SystemComponents to the SystemEntity.
         * Non-SystemComponents should not be added here
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList {
                azrtti_typeid<DccScriptingInterfaceEditorSystemComponent>(),
            };
        }
    };
}// namespace DccScriptingInterface

#if defined(O3DE_GEM_NAME)
AZ_DECLARE_MODULE_CLASS(AZ_JOIN(Gem_, O3DE_GEM_NAME), DccScriptingInterface::DccScriptingInterfaceEditorModule)
#else
AZ_DECLARE_MODULE_CLASS(Gem_DccScriptingInterface, DccScriptingInterface::DccScriptingInterfaceEditorModule)
#endif
