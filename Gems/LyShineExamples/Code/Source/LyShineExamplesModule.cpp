/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "LyShineExamples_precompiled.h"

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

// DO NOT MODIFY THIS LINE UNLESS YOU RENAME THE GEM
// The first parameter should be GemName_GemIdLower
// The second should be the fully qualified name of the class above
AZ_DECLARE_MODULE_CLASS(Gem_LyShineExamples, LyShineExamples::LyShineExamplesModule)
