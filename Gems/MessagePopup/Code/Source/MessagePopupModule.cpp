/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <MessagePopup_precompiled.h>

#include <AzCore/Memory/SystemAllocator.h>
#include <MessagePopupSystemComponent.h>
#include <LyShineMessagePopup.h>
#include <IGem.h>

namespace MessagePopup
{
    class MessagePopupModule
        : public CryHooksModule
    {
    public:
        AZ_RTTI(MessagePopupModule, "{79CE538B-D7D1-4066-8C0E-5794121BE8A8}", CryHooksModule);
        AZ_CLASS_ALLOCATOR(MessagePopupModule, AZ::SystemAllocator, 0);

        MessagePopupModule()
            : CryHooksModule()
        {
            // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
            m_descriptors.insert(m_descriptors.end(), {
                MessagePopupSystemComponent::CreateDescriptor(),
            });

            m_descriptors.insert(m_descriptors.end(), {
                LyShineMessagePopup::CreateDescriptor(),
            });
        }

        /**
         * Add required SystemComponents to the SystemEntity.
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList{
                azrtti_typeid<MessagePopupSystemComponent>(),
                azrtti_typeid<LyShineMessagePopup>(),
            };
        }
    };
}

// DO NOT MODIFY THIS LINE UNLESS YOU RENAME THE GEM
// The first parameter should be GemName_GemIdLower
// The second should be the fully qualified name of the class above
AZ_DECLARE_MODULE_CLASS(Gem_MessagePopup, MessagePopup::MessagePopupModule)
