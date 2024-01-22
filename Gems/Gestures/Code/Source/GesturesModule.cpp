/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Memory/SystemAllocator.h>

#include "GesturesSystemComponent.h"

#include <IGem.h>

namespace Gestures
{
    class GesturesModule
        : public CryHooksModule
    {
    public:
        AZ_RTTI(GesturesModule, "{5648A92C-04A3-4E30-B4E2-B0AEB280CA44}", CryHooksModule);
        AZ_CLASS_ALLOCATOR(GesturesModule, AZ::SystemAllocator);

        GesturesModule()
            : CryHooksModule()
        {
            // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
            m_descriptors.insert(m_descriptors.end(), {
                GesturesSystemComponent::CreateDescriptor(),
            });
        }

        /**
         * Add required SystemComponents to the SystemEntity.
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList{
                azrtti_typeid<GesturesSystemComponent>(),
            };
        }
    };
}

#if defined(O3DE_GEM_NAME)
AZ_DECLARE_MODULE_CLASS(AZ_JOIN(Gem_, O3DE_GEM_NAME), Gestures::GesturesModule)
#else
AZ_DECLARE_MODULE_CLASS(Gem_Gestures, Gestures::GesturesModule)
#endif
