/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/containers/map.h>
#include <AzCore/std/containers/set.h>
#include <AzToolsFramework/Prefab/Spawnable/PrefabProcessor.h>

namespace AZ
{
    class ReflectContext;
}

// Predefinition for unit test friend class
namespace UnitTest
{
    class PrefabProcessingTestFixture;
}

namespace AzToolsFramework::Prefab::PrefabConversionUtils
{
    //! A prefab processor for removing a spawnable's components based on asset tags.
    //! For example, a headless server shouldn't need to load a mesh or material asset,
    //!   so remove any MeshComponents from the server asset spawnable.
    //! Excluded components are defined inside of Registry/prefab.tools.setreg
    class AssetPlatformComponentRemover
        : public PrefabProcessor
    {
        friend class UnitTest::PrefabProcessingTestFixture;

    public:
        AZ_CLASS_ALLOCATOR(AssetPlatformComponentRemover, AZ::SystemAllocator);
        AZ_RTTI(AzToolsFramework::Prefab::PrefabConversionUtils::AssetPlatformComponentRemover,
            "{25D9A8A6-908F-4B26-A752-EBAF7DC074F8}", PrefabProcessor);

        static void Reflect(AZ::ReflectContext* context);
        ~AssetPlatformComponentRemover() override = default;

        void Process(PrefabProcessorContext& prefabProcessorContext) override;

    private:
        AZStd::map<AZStd::string, AZStd::set<AZ::Uuid>> m_platformExcludedComponents;
    };
} // namespace AzToolsFramework::Prefab::PrefabConversionUtils
