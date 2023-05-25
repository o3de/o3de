/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Archive/ArchiveTypeIds.h>
#include <ArchiveModuleInterface.h>
#include "ArchiveEditorSystemComponent.h"

namespace Archive
{
    class ArchiveEditorModule
        : public ArchiveModuleInterface
    {
    public:
        AZ_RTTI(ArchiveEditorModule, ArchiveEditorModuleTypeId, ArchiveModuleInterface);
        AZ_CLASS_ALLOCATOR(ArchiveEditorModule, AZ::SystemAllocator);

        ArchiveEditorModule()
        {
            // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
            // Add ALL components descriptors associated with this gem to m_descriptors.
            // This will associate the AzTypeInfo information for the components with the the SerializeContext, BehaviorContext and EditContext.
            // This happens through the [MyComponent]::Reflect() function.
            m_descriptors.insert(m_descriptors.end(), {
                ArchiveEditorSystemComponent::CreateDescriptor(),
            });
        }

        /**
         * Add required SystemComponents to the SystemEntity.
         * Non-SystemComponents should not be added here
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList {
                azrtti_typeid<ArchiveEditorSystemComponent>(),
            };
        }
    };
}// namespace Archive

AZ_DECLARE_MODULE_CLASS(Gem_Archive, Archive::ArchiveEditorModule)
