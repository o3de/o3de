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

#include <AzCore/Memory/SystemAllocator.h>
#include <AzToolsFramework/Prefab/Spawnable/PrefabProcessor.h>

namespace AZ
{
    class ReflectContext;
}

namespace AzToolsFramework::Prefab::PrefabConversionUtils
{
    class PrefabCatchmentProcessor
        : public PrefabProcessor
    {
    public:
        AZ_CLASS_ALLOCATOR(PrefabCatchmentProcessor, AZ::SystemAllocator, 0);
        AZ_RTTI(AzToolsFramework::Prefab::PrefabConversionUtils::PrefabCatchmentProcessor,
            "{F71E2FBA-22ED-44C7-B4C8-D2CF4B2C7B97}", PrefabProcessor);

        ~PrefabCatchmentProcessor() override = default;

        void Process(PrefabProcessorContext& context) override;

        static void Reflect(AZ::ReflectContext* context);

    protected:
        static void ProcessPrefab(PrefabProcessorContext& context, AZStd::string_view prefabName, PrefabDom& prefab);
    };
} // namespace AzToolsFramework::Prefab::PrefabConversionUtils
