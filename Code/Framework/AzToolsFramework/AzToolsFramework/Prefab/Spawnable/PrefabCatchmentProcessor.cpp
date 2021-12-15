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
#include <AzToolsFramework/Prefab/Instance/Instance.h>
#include <AzToolsFramework/Prefab/PrefabDomUtils.h>
#include <AzToolsFramework/Prefab/Spawnable/PrefabCatchmentProcessor.h>
#include <AzToolsFramework/Prefab/Spawnable/SpawnableUtils.h>


namespace AzToolsFramework::Prefab::PrefabConversionUtils
{
    void PrefabCatchmentProcessor::Process(PrefabProcessorContext& context)
    {
        AZ::DataStream::StreamType serializationFormat = m_serializationFormat == SerializationFormats::Binary ?
            AZ::DataStream::StreamType::ST_BINARY : AZ::DataStream::StreamType::ST_XML;
        context.ListPrefabs([&context, serializationFormat](PrefabDocument& prefab)
            {
                ProcessPrefab(context, prefab, serializationFormat);
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
                ->Version(3)
                ->Field("SerializationFormat", &PrefabCatchmentProcessor::m_serializationFormat);
        }
    }

    void PrefabCatchmentProcessor::ProcessPrefab(PrefabProcessorContext& context, PrefabDocument& prefab,
        AZ::DataStream::StreamType serializationFormat)
    {
        using namespace AzToolsFramework::Prefab::SpawnableUtils;

        AZStd::string uniqueName = prefab.GetName();
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

        Instance& instance = prefab.GetInstance();
        // Resolve entity aliases that store PrefabDOM information to use the spawnable instead. This is done before the entities are
        // moved from the instance as they'd otherwise can't be found.
        context.ResolveSpawnableEntityAliases(prefab.GetName(), *spawnable, instance);

        AzFramework::Spawnable::EntityList& entities = spawnable->GetEntities();
        instance.DetachAllEntitiesInHierarchy(
            [&entities, &context](AZStd::unique_ptr<AZ::Entity> entity)
            {
                if (entity)
                {
                    entity->InvalidateDependencies();
                    AZ::Entity::DependencySortOutcome evaluation = entity->EvaluateDependenciesGetDetails();
                    if (evaluation.IsSuccess())
                    {
                        entities.emplace_back(AZStd::move(entity));
                    }
                    else
                    {
                        AZ_Error(
                            "Prefabs", false, "Entity '%s' %s cannot be activated for the following reason: %s",
                            entity->GetName().c_str(), entity->GetId().ToString().c_str(), evaluation.GetError().m_message.c_str());
                        context.ErrorEncountered();
                    }
                }
            });

        SpawnableUtils::SortEntitiesByTransformHierarchy(*spawnable);
        context.GetProcessedObjects().push_back(AZStd::move(object));
    }
} // namespace AzToolsFramework::Prefab::PrefabConversionUtils
