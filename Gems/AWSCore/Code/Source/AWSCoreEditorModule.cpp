/*
* All or portions of this file Copyright(c) Amazon.com, Inc.or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*/

#include <AWSCoreEditorModule.h>
#include <AWSCoreEditorSystemComponent.h>

namespace AWSCore
{
    AWSCoreEditorModule::AWSCoreEditorModule()
        : AWSCoreModule()
    {
        // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
        m_descriptors.insert(m_descriptors.end(), {
            AWSCoreEditorSystemComponent::CreateDescriptor(),
        });
    }

    /**
    * Add required SystemComponents to the SystemEntity.
    */
    AZ::ComponentTypeList AWSCoreEditorModule::GetRequiredSystemComponents() const
    {
        AZ::ComponentTypeList requiredComponents = AWSCoreModule::GetRequiredSystemComponents();
        requiredComponents.push_back(azrtti_typeid<AWSCoreEditorSystemComponent>());

        return requiredComponents;
    }

}

// DO NOT MODIFY THIS LINE UNLESS YOU RENAME THE GEM
// The first parameter should be GemName_GemIdLower
// The second should be the fully qualified name of the class above
AZ_DECLARE_MODULE_CLASS(Gem_AWSCore_Editor, AWSCore::AWSCoreEditorModule)
