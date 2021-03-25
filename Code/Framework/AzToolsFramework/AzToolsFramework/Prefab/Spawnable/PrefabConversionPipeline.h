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
#include <AzCore/Serialization/Json/JsonSerialization.h>
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
        AZ_CLASS_ALLOCATOR(PrefabConversionPipeline, AZ::SystemAllocator, 0);
        
        using PrefabProcessorListEntry = AZStd::unique_ptr<PrefabProcessor>;
        using PrefabProcessorList = AZStd::vector<PrefabProcessorListEntry>;

        bool LoadStackProfile(AZStd::string_view stackProfile);

        void ProcessPrefab(PrefabProcessorContext& context);

        static void Reflect(AZ::ReflectContext* context);

    private:
        PrefabProcessorList m_processors;
    };
} // namespace AzToolsFramework::Prefab::PrefabConversionUtils
