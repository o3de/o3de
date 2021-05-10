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

#include <AzCore/IO/ByteContainerStream.h>
#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/RTTI/TypeInfo.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/Utils.h>
#include <AzFramework/Spawnable/Spawnable.h>
#include <AzToolsFramework/Entity/EditorEntityHelpers.h>
#include <AzToolsFramework/Prefab/Spawnable/PrefabCatchmentProcessor.h>
#include <AzToolsFramework/Prefab/Spawnable/SpawnableUtils.h>

#include <AzToolsFramework/Prefab/PrefabDomUtils.h>

namespace AzToolsFramework::Prefab::PrefabConversionUtils
{
    void PrefabCatchmentProcessor::Process(PrefabProcessorContext& context)
    {
        AZ::DataStream::StreamType serializationFormat = m_serializationFormat == SerializationFormats::Binary ?
            AZ::DataStream::StreamType::ST_BINARY : AZ::DataStream::StreamType::ST_XML;
        context.ListPrefabs([&context, serializationFormat](AZStd::string_view prefabName, PrefabDom& prefab)
            {
                ProcessPrefab(context, prefabName, prefab, serializationFormat);
            });
    }

    void PrefabCatchmentProcessor::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context); serializeContext != nullptr)
        {
            serializeContext->Enum<SerializationFormats>()
                ->Value("Binary", SerializationFormats::Binary)
                ->Value("Text", SerializationFormats::Text);

            serializeContext->Class<PrefabCatchmentProcessor, PrefabProcessor>()
                ->Version(2)
                ->Field("SerializationFormat", &PrefabCatchmentProcessor::m_serializationFormat);
        }
    }

    void PrefabCatchmentProcessor::ProcessPrefab(PrefabProcessorContext& context, AZStd::string_view prefabName, PrefabDom& prefab,
        AZ::DataStream::StreamType serializationFormat)
    {
        AZStd::string uniqueName = prefabName;
        uniqueName += AzFramework::Spawnable::DotFileExtension;

        auto serializer = [serializationFormat](AZStd::vector<uint8_t>& output, const ProcessedObjectStore& object) -> bool
        {
            AZ::IO::ByteContainerStream stream(&output);
            auto& asset = object.GetAsset();
            return AZ::Utils::SaveObjectToStream(stream, serializationFormat, &asset, asset.GetType());
        };

        auto&& [object, spawnable] = ProcessedObjectStore::Create<AzFramework::Spawnable>(
            AZStd::move(uniqueName), context.GetSourceUuid(), AZStd::move(serializer));
        AZ_Assert(spawnable, "Failed to create a new spawnable.");

        Prefab::PrefabDomUtils::PrintPrefabDomValue("Prefab used for spawnable", prefab);
        bool result = SpawnableUtils::CreateSpawnable(*spawnable, prefab, object.GetReferencedAssets());
        if (result)
        {
            AzFramework::Spawnable::EntityList& entities = spawnable->GetEntities();
            for (auto it = entities.begin(); it != entities.end(); )
            {
                (*it)->InvalidateDependencies();
                AZ::Entity::DependencySortOutcome evaluation = (*it)->EvaluateDependenciesGetDetails();
                if (evaluation.IsSuccess())
                {
                    ++it;
                }
                else
                {
                    AZ_Error("Prefabs", false, "Entity '%s' %s cannot be activated for the following reason: %s",
                        (*it)->GetName().c_str(), (*it)->GetId().ToString().c_str(), evaluation.GetError().m_message.c_str());
                    it = entities.erase(it);
                }
            }
            SpawnableUtils::SortEntitiesByTransformHierarchy(*spawnable);
            context.GetProcessedObjects().push_back(AZStd::move(object));

            context.RemovePrefab(prefabName);
        }
        else
        {
            AZ_Error("Prefabs", false, "Failed to convert prefab '%.*s' to a spawnable.", AZ_STRING_ARG(prefabName));
            context.ErrorEncountered();
        }
    }
} // namespace AzToolsFramework::Prefab::PrefabConversionUtils
