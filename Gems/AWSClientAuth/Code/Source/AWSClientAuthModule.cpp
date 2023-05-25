/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AWSClientAuthModule.h>
#include <AWSClientAuthSystemComponent.h>

#if AWSCLIENTAUTH_EDITOR
#include <AWSClientAuthEditorSystemComponent.h>
#endif

namespace AWSClientAuth
{

    AWSClientAuthModule::AWSClientAuthModule()
            : AZ::Module()
    {
        // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
        m_descriptors.insert(m_descriptors.end(), {
#if defined(AWSCLIENTAUTH_EDITOR)
                        AWSClientAuthEditorSystemComponent::CreateDescriptor()
#else
                        AWSClientAuthSystemComponent::CreateDescriptor()
#endif
        });
    }

    /**
    * Add required SystemComponents to the SystemEntity.
    */
    AZ::ComponentTypeList AWSClientAuthModule::GetRequiredSystemComponents() const
    {
        return AZ::ComponentTypeList{
#if defined(AWSCLIENTAUTH_EDITOR)
            azrtti_typeid<AWSClientAuthEditorSystemComponent>(),
#else
            azrtti_typeid<AWSClientAuthSystemComponent>(),
#endif
        };
    }

}

// DO NOT MODIFY THIS LINE UNLESS YOU RENAME THE GEM
// The first parameter should be GemName_GemIdLower
// The second should be the fully qualified name of the class above
AZ_DECLARE_MODULE_CLASS(Gem_AWSClientAuth, AWSClientAuth::AWSClientAuthModule)
