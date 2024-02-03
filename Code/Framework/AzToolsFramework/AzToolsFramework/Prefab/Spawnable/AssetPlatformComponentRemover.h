/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Memory/SystemAllocator.h>
#include <AzToolsFramework/Prefab/Spawnable/PrefabProcessor.h>

namespace AZ
{
    class ReflectContext;
}

namespace AzToolsFramework::Prefab::PrefabConversionUtils
{
    class AssetPlatformComponentRemover
        : public PrefabProcessor
    {
    public:
        AZ_CLASS_ALLOCATOR(AssetPlatformComponentRemover, AZ::SystemAllocator);
        AZ_RTTI(AzToolsFramework::Prefab::PrefabConversionUtils::AssetPlatformComponentRemover,
            "{25D9A8A6-908F-4B26-A752-EBAF7DC074F8}", PrefabProcessor);

        static void Reflect(AZ::ReflectContext* context);

        void Process(PrefabProcessorContext& prefabProcessorContext) override;
        void RemoveComponentsBasedOnAssetPlatform(PrefabDocument& prefab, const AZStd::set<AZ::Uuid>& exludedComponents);

     private:
        AZStd::map<AZStd::string, AZStd::set<AZ::Uuid>> m_platformExcludedComponents;
    };
} // namespace AzToolsFramework::Prefab::PrefabConversionUtils
