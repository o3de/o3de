/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ShineExamplesSystemComponent.h"
#include "UiTestScrollBoxDataProviderComponent.h"
#include "UiCustomImageComponent.h"

#include <IGem.h>

namespace ShineExamples
{
    class ShineExamplesModule
        : public CryHooksModule
    {
    public:
        AZ_RTTI(ShineExamplesModule, "{BC028F50-D2C4-4A71-84D1-F1BDC727019A}", CryHooksModule);

        ShineExamplesModule()
            : CryHooksModule()
        {
            // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
            m_descriptors.insert(m_descriptors.end(), {
                ShineExamplesSystemComponent::CreateDescriptor(),
                UiTestScrollBoxDataProviderComponent::CreateDescriptor(),
                UiCustomImageComponent::CreateDescriptor(),
            });
        }

        /**
         * Add required SystemComponents to the SystemEntity.
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList{
                azrtti_typeid<ShineExamplesSystemComponent>(),
            };
        }
    };
}

#if defined(O3DE_GEM_NAME)
AZ_DECLARE_MODULE_CLASS(AZ_JOIN(Gem_, O3DE_GEM_NAME), ShineExamples::ShineExamplesModule)
#else
AZ_DECLARE_MODULE_CLASS(Gem_ShineExamples, ShineExamples::ShineExamplesModule)
#endif
