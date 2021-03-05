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

#include "SliceFavorites_precompiled.h"

#include <AzCore/Memory/SystemAllocator.h>

#include "SliceFavoritesSystemComponent.h"

#include <IGem.h>

namespace SliceFavorites
{
    class SliceFavoritesModule
        : public AZ::Module
    {
    public:
        AZ_RTTI(SliceFavoritesModule, "{2D4B6591-2274-4556-B61C-98518D4C8CCE}", AZ::Module);
        AZ_CLASS_ALLOCATOR(SliceFavoritesModule, AZ::SystemAllocator, 0);

        SliceFavoritesModule()
            : AZ::Module()
        {
            // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
            m_descriptors.insert(m_descriptors.end(), {
                SliceFavoritesSystemComponent::CreateDescriptor(),
            });
        }

        /**
         * Add required SystemComponents to the SystemEntity.
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList{
                azrtti_typeid<SliceFavoritesSystemComponent>(),
            };
        }
    };
}

// DO NOT MODIFY THIS LINE UNLESS YOU RENAME THE GEM
// The first parameter should be GemName_GemIdLower
// The second should be the fully qualified name of the class above
AZ_DECLARE_MODULE_CLASS(Gem_SliceFavorites, SliceFavorites::SliceFavoritesModule)
