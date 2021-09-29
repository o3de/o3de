/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Component/ComponentApplicationBus.h>
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
        bool result = registry->GetObject(m_processors, registryKey);
        if (!result)
        {
            m_processors.clear();
        }

        AZ::SerializeContext* serializeContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);

        if (serializeContext)
        {
            m_fingerprint = CalculateProcessorFingerprint(serializeContext);
        }
        else
        {
            AZ_Error("PrefabConversionPipeline", false, "Failed to get serialization context");
        }

        return result;
    }

    bool PrefabConversionPipeline::IsLoaded() const
    {
        return !m_processors.empty();
    }

    void PrefabConversionPipeline::ProcessPrefab(PrefabProcessorContext& context)
    {
        for (auto& processor : m_processors)
        {
            processor->Process(context);
        }
    }
    size_t PrefabConversionPipeline::CalculateProcessorFingerprint(AZ::SerializeContext* context)
    {
        size_t fingerprint = 0;

        for (const auto& processor : m_processors)
        {
            const AZ::SerializeContext::ClassData* classData = context->FindClassData(processor->RTTI_GetType());

            if (!classData)
            {
                AZ_Warning("PrefabConversionPipeline", false, "Class data for processor type %s not found.  Cannot get version for fingerprinting", processor->RTTI_GetType().ToString<AZStd::string>().c_str());
                continue;
            }

            AZStd::hash_combine(fingerprint, processor->RTTI_GetType());
            AZStd::hash_combine(fingerprint, classData->m_version);
        }

        return fingerprint;
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

    size_t PrefabConversionPipeline::GetFingerprint() const
    {
        return m_fingerprint;
    }
} // namespace AzToolsFramework::Prefab::PrefabConversionUtils
