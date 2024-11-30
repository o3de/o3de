/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <RecastNavigationModuleInterface.h>
#include <RecastNavigationEditorSystemComponent.h>
#include <EditorComponents/EditorDetourNavigationComponent.h>
#include <EditorComponents/EditorRecastNavigationMeshComponent.h>
#include <EditorComponents/EditorRecastNavigationPhysXProviderComponent.h>

namespace RecastNavigation
{
    class RecastNavigationEditorModule
        : public RecastNavigationModuleInterface
    {
    public:
        AZ_RTTI(RecastNavigationEditorModule, "{a8fb0082-78ab-4ca6-8f63-68c98f1a6a6d}", RecastNavigationModuleInterface);
        AZ_CLASS_ALLOCATOR(RecastNavigationEditorModule, AZ::SystemAllocator);

        RecastNavigationEditorModule()
        {
            // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
            // Add ALL components descriptors associated with this gem to m_descriptors.
            // This will associate the AzTypeInfo information for the components with the the SerializeContext, BehaviorContext and EditContext.
            // This happens through the [MyComponent]::Reflect() function.
            m_descriptors.insert(m_descriptors.end(), {
                RecastNavigationEditorSystemComponent::CreateDescriptor(),
                EditorDetourNavigationComponent::CreateDescriptor(),
                EditorRecastNavigationMeshComponent::CreateDescriptor(),
                EditorRecastNavigationPhysXProviderComponent::CreateDescriptor(),
            });
        }

        //! Add required SystemComponents to the SystemEntity.
        //! Non-SystemComponents should not be added here.
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList {
                azrtti_typeid<RecastNavigationEditorSystemComponent>(),
            };
        }
    };
}// namespace RecastNavigation

#if defined(O3DE_GEM_NAME)
AZ_DECLARE_MODULE_CLASS(AZ_JOIN(Gem_, O3DE_GEM_NAME, _Editor), RecastNavigation::RecastNavigationEditorModule)
#else
AZ_DECLARE_MODULE_CLASS(Gem_RecastNavigation_Editor, RecastNavigation::RecastNavigationEditorModule)
#endif
