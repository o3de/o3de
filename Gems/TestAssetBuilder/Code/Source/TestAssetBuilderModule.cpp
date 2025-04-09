/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Module/Module.h>

#include <Builder/TestAssetBuilderComponent.h>
#include <Builder/TestIntermediateAssetBuilderComponent.h>
#include <Builder/TestDependencyBuilderComponent.h>

namespace TestAssetBuilder
{
    class TestAssetBuilderModule
        : public AZ::Module
    {
    public:
        AZ_RTTI(TestAssetBuilderModule, "{E1BD9AEE-8A56-4BA5-8FD7-7B9DD5DCBADB}", AZ::Module);
        AZ_CLASS_ALLOCATOR(TestAssetBuilderModule, AZ::SystemAllocator);

        TestAssetBuilderModule()
            : AZ::Module()
        {
            m_descriptors.insert(m_descriptors.end(), {
                TestAssetBuilderComponent::CreateDescriptor(),
                TestIntermediateAssetBuilderComponent::CreateDescriptor(),
                TestDependencyBuilderComponent::CreateDescriptor(),
            });
        }
    };
}

#if defined(O3DE_GEM_NAME)
AZ_DECLARE_MODULE_CLASS(AZ_JOIN(Gem_, O3DE_GEM_NAME, _Editor), TestAssetBuilder::TestAssetBuilderModule)
#else
AZ_DECLARE_MODULE_CLASS(Gem_TestAssetBuilder_Editor, TestAssetBuilder::TestAssetBuilderModule)
#endif
