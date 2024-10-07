/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

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
        AZ_CLASS_ALLOCATOR(MessagePopupModule, AZ::SystemAllocator);

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

#if defined(O3DE_GEM_NAME)
AZ_DECLARE_MODULE_CLASS(AZ_JOIN(Gem_, O3DE_GEM_NAME), MessagePopup::MessagePopupModule)
#else
AZ_DECLARE_MODULE_CLASS(Gem_MessagePopup, MessagePopup::MessagePopupModule)
#endif
