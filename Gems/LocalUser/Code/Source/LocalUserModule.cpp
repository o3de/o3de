/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Module/Module.h>

#include <LocalUserSystemComponent.h>

namespace LocalUser
{
    class LocalUserModule
        : public AZ::Module
    {
    public:
        AZ_RTTI(LocalUserModule, "{E154A426-0E21-4EB0-BE57-0947BB257D4D}", AZ::Module);
        AZ_CLASS_ALLOCATOR(LocalUserModule, AZ::SystemAllocator);

        LocalUserModule()
            : AZ::Module()
        {
            // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
            m_descriptors.insert(m_descriptors.end(), {
                LocalUserSystemComponent::CreateDescriptor(),
            });
        }

        /**
         * Add required SystemComponents to the SystemEntity.
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList{
                azrtti_typeid<LocalUserSystemComponent>(),
            };
        }
    };
}

#if defined(O3DE_GEM_NAME)
AZ_DECLARE_MODULE_CLASS(AZ_JOIN(Gem_, O3DE_GEM_NAME), LocalUser::LocalUserModule)
#else
AZ_DECLARE_MODULE_CLASS(Gem_LocalUser, LocalUser::LocalUserModule)
#endif
