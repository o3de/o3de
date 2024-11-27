/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <TwitchSystemComponent.h>
#include <AzCore/Module/Module.h>

namespace Twitch
{
    class TwitchModule
        : public AZ::Module
    {
    public:
        AZ_RTTI(TwitchModule, "{A7BA4662-18FE-4538-8B88-6F66C43AB319}", AZ::Module);

        TwitchModule()
            : AZ::Module()
        {
            // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
            m_descriptors.insert(m_descriptors.end(), {
                TwitchSystemComponent::CreateDescriptor(),
            });
        }

        /**
         * Add required SystemComponents to the SystemEntity.
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList{
                azrtti_typeid<TwitchSystemComponent>(),
            };
        }
    };
}

#if defined(O3DE_GEM_NAME)
AZ_DECLARE_MODULE_CLASS(AZ_JOIN(Gem_, O3DE_GEM_NAME), Twitch::TwitchModule)
#else
AZ_DECLARE_MODULE_CLASS(Gem_Twitch, Twitch::TwitchModule)
#endif
