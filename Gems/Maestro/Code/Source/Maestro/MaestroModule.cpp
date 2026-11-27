/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <IGem.h>

#include <Maestro/Components/SequenceAgentComponent.h>
#include <Maestro/Components/SequenceComponent.h>
#include <Maestro/MaestroSystemComponent.h>

namespace Maestro
{
    class MaestroModule
        : public CryHooksModule
    {
    public:
        AZ_RTTI(MaestroModule, "{ED1C74E6-BB73-4AC5-BD4B-91EFB400BAF4}", CryHooksModule);

        MaestroModule()
            : CryHooksModule()
        {
            // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
            m_descriptors.insert(m_descriptors.end(), {
                MaestroAllocatorComponent::CreateDescriptor(),
                MaestroSystemComponent::CreateDescriptor(),
                SequenceComponent::CreateDescriptor(),
                SequenceAgentComponent::CreateDescriptor(),
            });
        }

        /**
         * Add required SystemComponents to the SystemEntity.
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList{
                azrtti_typeid<MaestroAllocatorComponent>(),
                azrtti_typeid<MaestroSystemComponent>()
            };
        }
    };
}

#if defined(O3DE_GEM_NAME)
AZ_DECLARE_MODULE_CLASS(AZ_JOIN(Gem_, O3DE_GEM_NAME), Maestro::MaestroModule)
#else
AZ_DECLARE_MODULE_CLASS(Gem_Maestro, Maestro::MaestroModule)
#endif
