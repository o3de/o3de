/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/Module/Module.h>

#include "AtomViewportDisplayIconsSystemComponent.h"

namespace AZ
{
    namespace Render
    {
        class AtomViewportDisplayInfoModule
            : public AZ::Module
        {
        public:
            AZ_RTTI(AtomViewportDisplayInfoModule, "{8D72F14E-958D-4225-B3BC-C5C87BDDD426}", AZ::Module);
            AZ_CLASS_ALLOCATOR(AtomViewportDisplayInfoModule, AZ::SystemAllocator);

            AtomViewportDisplayInfoModule()
                : AZ::Module()
            {
                m_descriptors.insert(m_descriptors.end(), {
                        AtomViewportDisplayIconsSystemComponent::CreateDescriptor(),
                    });
            }

            AZ::ComponentTypeList GetRequiredSystemComponents() const override
            {
                return AZ::ComponentTypeList{
                    azrtti_typeid<AtomViewportDisplayIconsSystemComponent>(),
                };
            }
        };
    } // namespace Render
} // namespace AZ

#if defined(O3DE_GEM_NAME)
AZ_DECLARE_MODULE_CLASS(AZ_JOIN(Gem_, O3DE_GEM_NAME), AZ::Render::AtomViewportDisplayInfoModule)
#else
AZ_DECLARE_MODULE_CLASS(Gem_AtomViewportDisplayInfo, AZ::Render::AtomViewportDisplayInfoModule)
#endif
