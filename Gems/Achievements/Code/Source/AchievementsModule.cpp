/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Module/Module.h>

#include <AchievementsSystemComponent.h>

namespace Achievements
{
    class AchievementsModule
        : public AZ::Module
    {
    public:
        AZ_RTTI(AchievementsModule, "{67B7EBC3-69DE-447C-B006-776C2C1A4583}", AZ::Module);
        AZ_CLASS_ALLOCATOR(AchievementsModule, AZ::SystemAllocator);

        AchievementsModule()
            : AZ::Module()
        {
            // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
            m_descriptors.insert(m_descriptors.end(), {
                AchievementsSystemComponent::CreateDescriptor(),
            });
        }

        /**
         * Add required SystemComponents to the SystemEntity.
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList{
                azrtti_typeid<AchievementsSystemComponent>(),
            };
        }
    };
}

#if defined(O3DE_GEM_NAME)
AZ_DECLARE_MODULE_CLASS(AZ_JOIN(Gem_, O3DE_GEM_NAME), Achievements::AchievementsModule)
#else
AZ_DECLARE_MODULE_CLASS(Gem_Achievements, Achievements::AchievementsModule)
#endif
