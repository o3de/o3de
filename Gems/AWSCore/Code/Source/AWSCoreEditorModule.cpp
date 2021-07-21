/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AWSCoreEditorModule.h>
#include <AWSCoreEditorSystemComponent.h>
#include <Editor/Attribution/AWSCoreAttributionSystemComponent.h>

namespace AWSCore
{
    AWSCoreEditorModule::AWSCoreEditorModule()
    {
        // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
        m_descriptors.insert(m_descriptors.end(), {
            AWSCoreEditorSystemComponent::CreateDescriptor(),
            AWSAttributionSystemComponent::CreateDescriptor()
        });
    }

    /**
    * Add required SystemComponents to the SystemEntity.
    */
    AZ::ComponentTypeList AWSCoreEditorModule::GetRequiredSystemComponents() const
    {
        return AZ::ComponentTypeList{
            azrtti_typeid<AWSCoreEditorSystemComponent>(),
            azrtti_typeid<AWSAttributionSystemComponent>()
        };
    }

}

// DO NOT MODIFY THIS LINE UNLESS YOU RENAME THE GEM
// The first parameter should be GemName_GemIdLower
// The second should be the fully qualified name of the class above
AZ_DECLARE_MODULE_CLASS(Gem_AWSCore_Editor, AWSCore::AWSCoreEditorModule)
