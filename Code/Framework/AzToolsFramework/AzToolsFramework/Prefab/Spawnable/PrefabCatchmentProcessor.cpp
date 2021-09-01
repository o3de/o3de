/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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

        bool result = SpawnableUtils::CreateSpawnable(*spawnable, prefab, object.GetReferencedAssets());
        if (result)
        {
            AzFramework::Spawnable::EntityList& entities = spawnable->GetEntities();
            for (auto it = entities.begin(); it != entities.end(); )
            {
                if (*it)
                {
                    (*it)->InvalidateDependencies();
                    AZ::Entity::DependencySortOutcome evaluation = (*it)->EvaluateDependenciesGetDetails();
                    if (evaluation.IsSuccess())
                    {
                        ++it;
                    }
                    else
                    {
                        AZ_Error(
                            "Prefabs", false, "Entity '%s' %s cannot be activated for the following reason: %s", (*it)->GetName().c_str(),
                            (*it)->GetId().ToString().c_str(), evaluation.GetError().m_message.c_str());
                        it = entities.erase(it);
                    }
                }
                else
                {
                    it = entities.erase(it);
                }
            }
            SpawnableUtils::SortEntitiesByTransformHierarchy(*spawnable);
            context.GetProcessedObjects().push_back(AZStd::move(object));
        }
        else
        {
            AZ_Error("Prefabs", false, "Failed to convert prefab '%.*s' to a spawnable.", AZ_STRING_ARG(prefabName));
            context.ErrorEncountered();
        }
    }
} // namespace AzToolsFramework::Prefab::PrefabConversionUtils
