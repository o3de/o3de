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

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Settings/SettingsRegistry.h>
#include <AzToolsFramework/Prefab/Spawnable/PrefabConversionPipeline.h>

namespace AzToolsFramework::Prefab::PrefabConversionUtils
{
    bool PrefabConversionPipeline::LoadStackProfile(AZStd::string_view stackProfile)
    {
        m_processors.clear();

        AZStd::string registryKey = "/Amazon/Tools/Prefab/Processing/Stack/";
        registryKey += stackProfile;

        auto registry = AZ::SettingsRegistry::Get();
        AZ_Assert(registry, "PrefabConversionPipeline is created before the Settings Registry is available.");
        return registry->GetObject(m_processors, registryKey);
    }

    void PrefabConversionPipeline::ProcessPrefab(PrefabProcessorContext& context)
    {
        for (auto& processor : m_processors)
        {
            processor->Process(context);
        }
    }

    void PrefabConversionPipeline::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context); serializeContext != nullptr)
        {
            serializeContext->Class<PrefabProcessor>()->Version(1);
            serializeContext->RegisterGenericType<PrefabProcessorList>();
            serializeContext->RegisterGenericType<PrefabProcessorListEntry>();
        }  
    }
} // namespace AzToolsFramework::Prefab::PrefabConversionUtils
