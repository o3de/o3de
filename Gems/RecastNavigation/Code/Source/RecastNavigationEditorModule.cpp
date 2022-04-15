/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <RecastNavigationModuleInterface.h>

namespace RecastNavigation
{
    class RecastNavigationEditorModule
        : public RecastNavigationModuleInterface
    {
    public:
        AZ_RTTI(RecastNavigationEditorModule, "{8178c5eb-97fa-4fe6-a0b6-b7678ed205e4}", RecastNavigationModuleInterface);
        AZ_CLASS_ALLOCATOR(RecastNavigationEditorModule, AZ::SystemAllocator, 0);

        RecastNavigationEditorModule()
        {
            // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
            // Add ALL components descriptors associated with this gem to m_descriptors.
            // This will associate the AzTypeInfo information for the components with the the SerializeContext, BehaviorContext and EditContext.
            // This happens through the [MyComponent]::Reflect() function.
            m_descriptors.insert(m_descriptors.end(), {
            });
        }
    };
}// namespace RecastNavigation

AZ_DECLARE_MODULE_CLASS(Gem_RecastNavigation, RecastNavigation::RecastNavigationEditorModule)
