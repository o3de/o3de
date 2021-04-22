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
        AZ_RTTI(NetworkPrefabProcessor, "{AF6C36DA-CBB9-4DF4-AE2D-7BC6CCE65176}", PrefabProcessor);

        ~NetworkPrefabProcessor() override = default;

        void Process(PrefabProcessorContext& context) override;

        static void Reflect(AZ::ReflectContext* context);

    protected:
        static void ProcessPrefab(PrefabProcessorContext& context, AZStd::string_view prefabName, PrefabDom& prefab);
    };
}
