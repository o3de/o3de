/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "LyShineExamplesSystemComponent.h"
#include "UiTestScrollBoxDataProviderComponent.h"
#include "UiCustomImageComponent.h"

#include <IGem.h>

namespace LyShineExamples
{
    class LyShineExamplesModule
        : public CryHooksModule
    {
    public:
        AZ_RTTI(LyShineExamplesModule, "{BC028F50-D2C4-4A71-84D1-F1BDC727019A}", CryHooksModule);

        LyShineExamplesModule()
            : CryHooksModule()
        {
            // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
            m_descriptors.insert(m_descriptors.end(), {
                LyShineExamplesSystemComponent::CreateDescriptor(),
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
                azrtti_typeid<LyShineExamplesSystemComponent>(),
            };
        }
    };
}

#if defined(O3DE_GEM_NAME)
AZ_DECLARE_MODULE_CLASS(AZ_JOIN(Gem_, O3DE_GEM_NAME), LyShineExamples::LyShineExamplesModule)
#else
AZ_DECLARE_MODULE_CLASS(Gem_LyShineExamples, LyShineExamples::LyShineExamplesModule)
#endif
