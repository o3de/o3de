/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzCore/Module/Module.h>

#include <AWSCoreModule.h>
#include <AWSCoreSystemComponent.h>
#include <ScriptCanvas/AWSScriptBehaviorsComponent.h>


namespace AWSCore
{
    AWSCoreModule::AWSCoreModule()
            : AZ::Module()
    {
        // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
        m_descriptors.insert(m_descriptors.end(), {
            AWSCoreSystemComponent::CreateDescriptor(),
            AWSScriptBehaviorsComponent::CreateDescriptor(),
        });
    }

    /**
    * Add required SystemComponents to the SystemEntity.
    */
    AZ::ComponentTypeList AWSCoreModule::GetRequiredSystemComponents() const
    {
        return AZ::ComponentTypeList{
            azrtti_typeid<AWSCoreSystemComponent>(),
            azrtti_typeid<AWSScriptBehaviorsComponent>()
        };
    }

}

// DO NOT MODIFY THIS LINE UNLESS YOU RENAME THE GEM
// The first parameter should be GemName_GemIdLower
// The second should be the fully qualified name of the class above
AZ_DECLARE_MODULE_CLASS(Gem_AWSCore, AWSCore::AWSCoreModule)
