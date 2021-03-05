/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates, or 
* a third party where indicated.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,  
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  
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
