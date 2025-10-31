/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/Prefab/Spawnable/PrefabProcessor.h>
#include <AzCore/Serialization/ObjectStream.h>

namespace AzToolsFramework::Prefab::PrefabConversionUtils
{
    class PrefabProcessorContext;
    class PrefabDocument;
}
   
namespace Multiplayer
{
    class NetworkPrefabProcessor : public AzToolsFramework::Prefab::PrefabConversionUtils::PrefabProcessor
    {
    public:
        AZ_CLASS_ALLOCATOR(NetworkPrefabProcessor, AZ::SystemAllocator);
        AZ_RTTI(
            Multiplayer::NetworkPrefabProcessor,
            "{AF6C36DA-CBB9-4DF4-AE2D-7BC6CCE65176}",
            AzToolsFramework::Prefab::PrefabConversionUtils::PrefabProcessor);

        NetworkPrefabProcessor();
        ~NetworkPrefabProcessor() override = default;

        void Process(AzToolsFramework::Prefab::PrefabConversionUtils::PrefabProcessorContext& context) override;

        static void Reflect(AZ::ReflectContext* context);
    protected:
        bool ProcessPrefab(
            AzToolsFramework::Prefab::PrefabConversionUtils::PrefabProcessorContext& context,
            AzToolsFramework::Prefab::PrefabConversionUtils::PrefabDocument& prefab);

        void PostProcessSpawnable(const AZStd::string& prefabName, AzFramework::Spawnable& spawnable);

        AZStd::unordered_set<AZStd::string> m_processedNetworkPrefabs;

        AzToolsFramework::Prefab::PrefabConversionUtils::PrefabSpawnablePostProcessEvent::Handler m_postProcessHandler;
    };
}
