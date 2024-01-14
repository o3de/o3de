/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Module/Module.h>
#include <AzCore/RTTI/RTTI.h>

#include "AtomRenderOptionsSystemComponent.h"

namespace AZ::Render
{
    class AtomRenderOptionsModule : public AZ::Module
    {
    public:
        AZ_RTTI(AtomRenderOptionsModule, "{B6129302-BBC5-4020-BF21-5CC95FAB7941}", AZ::Module);
        AZ_CLASS_ALLOCATOR(AtomRenderOptionsModule, AZ::SystemAllocator);

        AtomRenderOptionsModule()
            : AZ::Module()
        {
            m_descriptors.insert(
                m_descriptors.end(),
                {
                    AtomRenderOptionsSystemComponent::CreateDescriptor(),
                });
        }

        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList{
                azrtti_typeid<AtomRenderOptionsSystemComponent>(),
            };
        }
    };
} // namespace AZ::Render

#if defined(O3DE_GEM_NAME)
AZ_DECLARE_MODULE_CLASS(AZ_JOIN(Gem_, O3DE_GEM_NAME), AZ::Render::AtomRenderOptionsModule)
#else
AZ_DECLARE_MODULE_CLASS(Gem_AtomViewportDisplayInfo, AZ::Render::AtomRenderOptionsModule)
#endif
