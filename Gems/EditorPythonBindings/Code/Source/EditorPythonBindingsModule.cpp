/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Module/Module.h>

#include <AzToolsFramework/API/PythonLoader.h>

#include <PythonSystemComponent.h>
#include <PythonReflectionComponent.h>
#include <PythonMarshalComponent.h>
#include <PythonLogSymbolsComponent.h>

namespace EditorPythonBindings
{
    class EditorPythonBindingsModule
        : public AZ::Module
        , public AzToolsFramework::EmbeddedPython::PythonLoader
    {
    public:
        AZ_RTTI(EditorPythonBindingsModule, "{851B9E35-4FD5-49B1-8207-E40D4BBA36CC}", AZ::Module);
        AZ_CLASS_ALLOCATOR(EditorPythonBindingsModule, AZ::SystemAllocator, 0);

        EditorPythonBindingsModule()
            : AZ::Module()
        {
            m_descriptors.insert(m_descriptors.end(), 
            {
                PythonSystemComponent::CreateDescriptor(),
                PythonReflectionComponent::CreateDescriptor(),
                PythonMarshalComponent::CreateDescriptor(),
                PythonLogSymbolsComponent::CreateDescriptor(),
            });
        }

        /**
         * Add required SystemComponents to the SystemEntity.
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList
            {
                azrtti_typeid<PythonSystemComponent>(),
                azrtti_typeid<PythonReflectionComponent>(),
                azrtti_typeid<PythonMarshalComponent>(),
                azrtti_typeid<PythonLogSymbolsComponent>()
            };
        }
    };
}

// DO NOT MODIFY THIS LINE UNLESS YOU RENAME THE GEM
// The first parameter should be GemName_GemIdLower
// The second should be the fully qualified name of the class above
AZ_DECLARE_MODULE_CLASS(Gem_EditorPythonBindings, EditorPythonBindings::EditorPythonBindingsModule)
