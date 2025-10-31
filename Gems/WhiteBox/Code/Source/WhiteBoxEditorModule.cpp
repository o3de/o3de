/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "Components/EditorWhiteBoxColliderComponent.h"
#include "EditorWhiteBoxComponent.h"
#include "EditorWhiteBoxSystemComponent.h"
#include "WhiteBoxEditorModule.h"

namespace WhiteBox
{
    AZ_CLASS_ALLOCATOR_IMPL(WhiteBoxEditorModule, AZ::SystemAllocator)

    WhiteBoxEditorModule::WhiteBoxEditorModule()
        : WhiteBoxModule()
    {
        // push results of [MyComponent]::CreateDescriptor() into m_descriptors here
        m_descriptors.insert(
            m_descriptors.end(),
            {
                EditorWhiteBoxSystemComponent::CreateDescriptor(),
                EditorWhiteBoxComponent::CreateDescriptor(),
                EditorWhiteBoxColliderComponent::CreateDescriptor(),
            });
    }

    WhiteBoxEditorModule::~WhiteBoxEditorModule() = default;

    AZ::ComponentTypeList WhiteBoxEditorModule::GetRequiredSystemComponents() const
    {
        // add required SystemComponents to the SystemEntity
        return AZ::ComponentTypeList{azrtti_typeid<EditorWhiteBoxSystemComponent>()};
    }
} // namespace WhiteBox

#if defined(O3DE_GEM_NAME)
AZ_DECLARE_MODULE_CLASS(AZ_JOIN(Gem_, O3DE_GEM_NAME, _Editor), WhiteBox::WhiteBoxEditorModule)
#else
AZ_DECLARE_MODULE_CLASS(Gem_WhiteBox_Editor, WhiteBox::WhiteBoxEditorModule)
#endif
