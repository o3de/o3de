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

#include "AtomFontSystemComponent.h"

namespace AZ
{
    namespace Render
    {
        class AtomFontModule
            : public AZ::Module
        {
        public:
            AZ_RTTI(AtomFontModule, "{E5EDF3B2-F85D-441B-8D0B-21D44D177799}", AZ::Module);
            AZ_CLASS_ALLOCATOR(AtomFontModule, AZ::SystemAllocator);

            AtomFontModule()
                : AZ::Module()
            {
                m_descriptors.insert(m_descriptors.end(), {
                        AtomFontSystemComponent::CreateDescriptor(),
                    });
            }

            AZ::ComponentTypeList GetRequiredSystemComponents() const override
            {
                return AZ::ComponentTypeList{
                    azrtti_typeid<AtomFontSystemComponent>(),
                };
            }
        };
    } // namespace Render
} // namespace AZ

#if defined(O3DE_GEM_NAME)
AZ_DECLARE_MODULE_CLASS(AZ_JOIN(Gem_, O3DE_GEM_NAME), AZ::Render::AtomFontModule)
#else
AZ_DECLARE_MODULE_CLASS(Gem_AtomFont, AZ::Render::AtomFontModule)
#endif
