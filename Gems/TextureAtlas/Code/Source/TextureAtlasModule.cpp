/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include "TextureAtlas_precompiled.h"

#include <AzCore/Memory/SystemAllocator.h>

#include "TextureAtlasSystemComponent.h"

#include <IGem.h>

namespace TextureAtlasNamespace
{
    class TextureAtlasModule
        : public CryHooksModule
    {
    public:
        AZ_RTTI(TextureAtlasModule, "{D3997F41-8117-4E0F-9BFE-937C4AE7E71F}", CryHooksModule);
        AZ_CLASS_ALLOCATOR(TextureAtlasModule, AZ::SystemAllocator, 0);

        TextureAtlasModule()
            : CryHooksModule()
        {
            // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
            m_descriptors.insert(m_descriptors.end(), {
                TextureAtlasSystemComponent::CreateDescriptor(),
            });
        }

        /**
         * Add required SystemComponents to the SystemEntity.
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList{
                azrtti_typeid<TextureAtlasSystemComponent>(),
            };
        }
    };
}

// DO NOT MODIFY THIS LINE UNLESS YOU RENAME THE GEM
// The first parameter should be GemName_GemIdLower
// The second should be the fully qualified name of the class above
AZ_DECLARE_MODULE_CLASS(Gem_TextureAtlas, TextureAtlasNamespace::TextureAtlasModule)
