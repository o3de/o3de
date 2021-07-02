/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "WhiteBox_precompiled.h"

#include "Components/WhiteBoxColliderComponent.h"
#include "WhiteBoxAllocator.h"
#include "WhiteBoxComponent.h"
#include "WhiteBoxModule.h"
#include "WhiteBoxSystemComponent.h"

namespace WhiteBox
{
    AZ_CLASS_ALLOCATOR_IMPL(WhiteBoxModule, AZ::SystemAllocator, 0)

    WhiteBoxModule::WhiteBoxModule()
        : CryHooksModule()
    {
        AZ::AllocatorInstance<WhiteBoxAllocator>::Create();

        // push results of [MyComponent]::CreateDescriptor() into m_descriptors here
        m_descriptors.insert(
            m_descriptors.end(),
            {
                WhiteBoxSystemComponent::CreateDescriptor(),
                WhiteBoxColliderComponent::CreateDescriptor(),
                WhiteBoxComponent::CreateDescriptor(),
            });
    }

    WhiteBoxModule::~WhiteBoxModule()
    {
        AZ::AllocatorInstance<WhiteBoxAllocator>::Destroy();
    }

    AZ::ComponentTypeList WhiteBoxModule::GetRequiredSystemComponents() const
    {
        // add required SystemComponents to the SystemEntity
        return AZ::ComponentTypeList{azrtti_typeid<WhiteBoxSystemComponent>()};
    }
} // namespace WhiteBox

#if !defined(WHITE_BOX_EDITOR)
AZ_DECLARE_MODULE_CLASS(Gem_WhiteBox, WhiteBox::WhiteBoxModule)
#endif // !defined(WHITE_BOX_EDITOR)
