/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Module/Module.h>

#include <PresenceSystemComponent.h>

namespace Presence
{
    class PresenceModule
        : public AZ::Module
    {
    public:
        AZ_RTTI(PresenceModule, "{FAFD5AC3-26EC-446B-A444-ADFFC06BCD3D}", AZ::Module);
        AZ_CLASS_ALLOCATOR(PresenceModule, AZ::SystemAllocator);

        PresenceModule()
            : AZ::Module()
        {
            // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
            m_descriptors.insert(m_descriptors.end(), {
                PresenceSystemComponent::CreateDescriptor(),
            });
        }

        /**
         * Add required SystemComponents to the SystemEntity.
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList{
                azrtti_typeid<PresenceSystemComponent>(),
            };
        }
    };
}

#if defined(O3DE_GEM_NAME)
AZ_DECLARE_MODULE_CLASS(AZ_JOIN(Gem_, O3DE_GEM_NAME), Presence::PresenceModule)
#else
AZ_DECLARE_MODULE_CLASS(Gem_Presence, Presence::PresenceModule)
#endif
