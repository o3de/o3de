/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Module/Module.h>

#include <Builder/TestAssetBuilderComponent.h>

namespace TestAssetBuilder
{
    class TestAssetBuilderModule
        : public AZ::Module
    {
    public:
        AZ_RTTI(TestAssetBuilderModule, "{E1BD9AEE-8A56-4BA5-8FD7-7B9DD5DCBADB}", AZ::Module);
        AZ_CLASS_ALLOCATOR(TestAssetBuilderModule, AZ::SystemAllocator, 0);

        TestAssetBuilderModule()
            : AZ::Module()
        {
            m_descriptors.insert(m_descriptors.end(), {
                TestAssetBuilderComponent::CreateDescriptor(),
            });
        }
    };
}

// DO NOT MODIFY THIS LINE UNLESS YOU RENAME THE GEM
// The first parameter should be GemName_GemIdLower
// The second should be the fully qualified name of the class above
AZ_DECLARE_MODULE_CLASS(Gem_TestAssetBuilder, TestAssetBuilder::TestAssetBuilderModule)
