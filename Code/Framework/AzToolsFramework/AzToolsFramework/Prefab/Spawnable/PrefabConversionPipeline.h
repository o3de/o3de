/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Serialization/Json/JsonSerialization.h>
#include <AzCore/Settings/ConfigurableStack.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/string/string_view.h>
#include <AzToolsFramework/Prefab/Spawnable/PrefabProcessor.h>
#include <AzToolsFramework/Prefab/Spawnable/SpawnableUtils.h>

namespace AzToolsFramework::Prefab::PrefabConversionUtils
{
    class PrefabConversionPipeline final
    {
    public:
        AZ_CLASS_ALLOCATOR(PrefabConversionPipeline, AZ::SystemAllocator);
        
        using PrefabProcessorStack = AZ::ConfigurableStack<PrefabProcessor>;

        bool LoadStackProfile(AZStd::string_view stackProfile);
        bool IsLoaded() const;

        void ProcessPrefab(PrefabProcessorContext& context);

        size_t GetFingerprint() const;

        static void Reflect(AZ::ReflectContext* context);
        
    private:
        size_t CalculateProcessorFingerprint(AZ::SerializeContext* context);

        PrefabProcessorStack m_processors;
        size_t m_fingerprint{};
    };
} // namespace AzToolsFramework::Prefab::PrefabConversionUtils
