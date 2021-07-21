/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/Prefab/Spawnable/PrefabProcessor.h>

namespace AzToolsFramework::Prefab::PrefabConversionUtils
{
    class PrefabProcessorContext;
}
   
namespace Multiplayer
{
    using AzToolsFramework::Prefab::PrefabConversionUtils::PrefabProcessor;
    using AzToolsFramework::Prefab::PrefabConversionUtils::PrefabProcessorContext;
    using AzToolsFramework::Prefab::PrefabDom;

    class NetworkPrefabProcessor : public PrefabProcessor
    {
    public:
        AZ_CLASS_ALLOCATOR(NetworkPrefabProcessor, AZ::SystemAllocator, 0);
        AZ_RTTI(Multiplayer::NetworkPrefabProcessor, "{AF6C36DA-CBB9-4DF4-AE2D-7BC6CCE65176}", PrefabProcessor);

        ~NetworkPrefabProcessor() override = default;

        void Process(PrefabProcessorContext& context) override;

        static void Reflect(AZ::ReflectContext* context);

    protected:
        static void ProcessPrefab(PrefabProcessorContext& context, AZStd::string_view prefabName, PrefabDom& prefab);
    };
}
