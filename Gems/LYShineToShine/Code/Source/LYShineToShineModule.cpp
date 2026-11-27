/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Module/Module.h>
#include "LYShineToShineSystemComponent.h"
#include "CanvasUpgradeBuilderComponent.h"

namespace LYShineToShine
{
    class LYShineToShineModule
        : public AZ::Module
    {
    public:
        AZ_RTTI(LYShineToShineModule, "{B8C4D2E1-F3A5-4D6B-9E7C-1A2B3C4D5E6F}", AZ::Module);
        AZ_CLASS_ALLOCATOR(LYShineToShineModule, AZ::SystemAllocator);

        LYShineToShineModule()
        {
            m_descriptors.insert(m_descriptors.end(), {
                LYShineToShineSystemComponent::CreateDescriptor(),
                CanvasUpgradeBuilderComponent::CreateDescriptor(),
            });
        }

        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList{
                azrtti_typeid<LYShineToShineSystemComponent>(),
            };
        }
    };
} // namespace LYShineToShine

#if defined(O3DE_GEM_NAME)
AZ_DECLARE_MODULE_CLASS(AZ_JOIN(Gem_, O3DE_GEM_NAME), LYShineToShine::LYShineToShineModule)
#else
AZ_DECLARE_MODULE_CLASS(Gem_LYShineToShine, LYShineToShine::LYShineToShineModule)
#endif
