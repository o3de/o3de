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

#include "AtomViewportDisplayInfoSystemComponent.h"

namespace AZ
{
    namespace Render
    {
        class AtomViewportDisplayInfoModule
            : public AZ::Module
        {
        public:
            AZ_RTTI(AtomViewportDisplayInfoModule, "{B10C0E55-03A1-4A46-AE3E-D3615AEAA659}", AZ::Module);
            AZ_CLASS_ALLOCATOR(AtomViewportDisplayInfoModule, AZ::SystemAllocator);

            AtomViewportDisplayInfoModule()
                : AZ::Module()
            {
                m_descriptors.insert(m_descriptors.end(), {
                        AtomViewportDisplayInfoSystemComponent::CreateDescriptor(),
                    });
            }

            AZ::ComponentTypeList GetRequiredSystemComponents() const override
            {
                return AZ::ComponentTypeList{
                    azrtti_typeid<AtomViewportDisplayInfoSystemComponent>(),
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
