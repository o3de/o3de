/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Module/Module.h>

#include <GameStateSystemComponent.h>

namespace GameState
{
    class GameStateModule
        : public AZ::Module
    {
    public:
        AZ_RTTI(GameStateModule, "{33EF4B1E-388F-4F05-B73A-6560FE0CF4E3}", AZ::Module);
        AZ_CLASS_ALLOCATOR(GameStateModule, AZ::SystemAllocator);

        GameStateModule()
            : AZ::Module()
        {
            // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
            m_descriptors.insert(m_descriptors.end(), {
                GameStateSystemComponent::CreateDescriptor(),
            });
        }

        /**
         * Add required SystemComponents to the SystemEntity.
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList{
                azrtti_typeid<GameStateSystemComponent>(),
            };
        }
    };
}

#if defined(O3DE_GEM_NAME)
AZ_DECLARE_MODULE_CLASS(AZ_JOIN(Gem_, O3DE_GEM_NAME), GameState::GameStateModule)
#else
AZ_DECLARE_MODULE_CLASS(Gem_GameState, GameState::GameStateModule)
#endif
