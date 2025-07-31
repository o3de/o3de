/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "PerforceEditorModule.h"
#include "EditorPerforceSystemComponent.h"

namespace Perforce
{
    AZ_CLASS_ALLOCATOR_IMPL(PerforceEditorModule, AZ::SystemAllocator)

    PerforceEditorModule::PerforceEditorModule()
        : CryHooksModule()
    {
        // push results of [MyComponent]::CreateDescriptor() into m_descriptors here
         m_descriptors.insert(
             m_descriptors.end(),
             {
                 EditorPerforceSystemComponent::CreateDescriptor(),
             });
    }

    PerforceEditorModule::~PerforceEditorModule()
    {
    }

    AZ::ComponentTypeList PerforceEditorModule::GetRequiredSystemComponents() const
    {
        // add required SystemComponents to the SystemEntity
        return AZ::ComponentTypeList{azrtti_typeid<EditorPerforceSystemComponent>()};
    }
} // namespace Perforce

#if defined(O3DE_GEM_NAME)
AZ_DECLARE_MODULE_CLASS(AZ_JOIN(Gem_, O3DE_GEM_NAME), Perforce::PerforceEditorModule)
#else
AZ_DECLARE_MODULE_CLASS(Gem_Perfoce_Editor, Perforce::PerforceEditorModule)
#endif
