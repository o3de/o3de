/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Module/Module.h>
#include <PrefabDependencyViewerEditorSystemComponent.h>
#include <AzToolsFramework/Prefab/PrefabSystemComponent.h>

namespace PrefabDependencyViewer
{
    class PrefabDependencyViewerEditorModule
        : public AZ::Module
    {
    public:
        AZ_RTTI(PrefabDependencyViewerEditorModule, "{094b3252-c64d-4c24-96e8-57c64725e6e1}", AZ::Module);
        AZ_CLASS_ALLOCATOR(PrefabDependencyViewerEditorModule, AZ::SystemAllocator, 0);

        PrefabDependencyViewerEditorModule()
        {
            // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
            // Add ALL components descriptors associated with this gem to m_descriptors.
            // This will associate the AzTypeInfo information for the components with the the SerializeContext, BehaviorContext and EditContext.
            // This happens through the [MyComponent]::Reflect() function.
            m_descriptors.insert(m_descriptors.end(), {
                PrefabDependencyViewerEditorSystemComponent::CreateDescriptor(),
            });
        }

        /**
         * Add required SystemComponents to the SystemEntity.
         * Non-SystemComponents should not be added here
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList {
                azrtti_typeid<PrefabDependencyViewerEditorSystemComponent>(),
                azrtti_typeid<AzToolsFramework::Prefab::PrefabSystemComponent>()
            };
        }
    };
}// namespace PrefabDependencyViewer

AZ_DECLARE_MODULE_CLASS(Gem_PrefabDependencyViewer, PrefabDependencyViewer::PrefabDependencyViewerEditorModule)
