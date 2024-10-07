/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
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

#if defined(O3DE_GEM_NAME)
AZ_DECLARE_MODULE_CLASS(AZ_JOIN(Gem_, O3DE_GEM_NAME), AWSCore::AWSCoreModule)
#else
AZ_DECLARE_MODULE_CLASS(Gem_AWSCore, AWSCore::AWSCoreModule)
#endif
