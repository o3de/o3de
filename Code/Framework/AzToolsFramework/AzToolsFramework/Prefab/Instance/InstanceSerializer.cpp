/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Component/Entity.h>
#include <AzCore/Serialization/Json/RegistrationContext.h>
#include <AzToolsFramework/Prefab/Instance/Instance.h>
#include <AzToolsFramework/Prefab/Instance/InstanceEntityScrubber.h>
#include <AzToolsFramework/Prefab/Instance/InstanceSerializer.h>
#include <AzToolsFramework/Prefab/Instance/InstanceEntityIdMapper.h>
#include <AzToolsFramework/Prefab/Instance/InstanceEntityMapperInterface.h>
#include <AzToolsFramework/Prefab/PrefabLoaderInterface.h>
#include <AzToolsFramework/Prefab/PrefabSystemComponentInterface.h>

namespace AzToolsFramework
{
    namespace Prefab
    {
        AZ_CLASS_ALLOCATOR_IMPL(JsonInstanceSerializer, AZ::SystemAllocator, 0);

        AZ::JsonSerializationResult::Result JsonInstanceSerializer::Store(rapidjson::Value& outputValue, const void* inputValue, const void* defaultValue,
            [[maybe_unused]] const AZ::Uuid& valueTypeId, AZ::JsonSerializerContext& context)
        {
            namespace JSR = AZ::JsonSerializationResult;

            AZ_Assert(azrtti_typeid<Instance>() == valueTypeId, "Unable to Serialize Instance because the provided type is %s",
                valueTypeId.ToString<AZStd::string>().c_str());

            const Instance* instance = reinterpret_cast<const Instance*>(inputValue);
            AZ_Assert(instance, "Input value for JsonInstanceSerializer can't be null.");
            const Instance* defaultInstance = reinterpret_cast<const Instance*>(defaultValue);

            InstanceEntityIdMapper** idMapper = context.GetMetadata().Find<InstanceEntityIdMapper*>();

            if (idMapper && *idMapper)
            {
                (*idMapper)->SetStoringInstance(*instance);
            }

            JSR::ResultCode result(JSR::Tasks::WriteValue);
            {
                AZ::ScopedContextPath subPathSource(context, "m_templateSourcePath");
                const AZStd::string* sourcePath = &(instance->GetTemplateSourcePath().Native());
                const AZStd::string* defaultSourcePath = defaultInstance ? &defaultInstance->GetTemplateSourcePath().Native() : nullptr;

                result = ContinueStoringToJsonObjectField(outputValue, "Source", sourcePath, defaultSourcePath, azrtti_typeid<AZStd::string>(), context);
            }
            
            {
                AZ::ScopedContextPath subPathContainerEntity(context, "m_containerEntity");
                JSR::ResultCode resultContainerEntity = ContinueStoringToJsonObjectField(outputValue, "ContainerEntity",
                    &instance->m_containerEntity, nullptr, azrtti_typeid<decltype(instance->m_containerEntity)>(), context);

                result.Combine(resultContainerEntity);
            }

            {
                AZ::ScopedContextPath subPathEntities(context, "m_entities");
                const Instance::AliasToEntityMap* entities = &instance->m_entities;
                const Instance::AliasToEntityMap* defaultEntities = defaultInstance ? &defaultInstance->m_entities : nullptr;
                JSR::ResultCode resultEntities =
                    ContinueStoringToJsonObjectField(outputValue, "Entities",
                        entities, defaultEntities, azrtti_typeid<Instance::AliasToEntityMap>(), context);

                result.Combine(resultEntities);
            }

            {
                AZ::ScopedContextPath subPathInstances(context, "m_nestedInstances");
                const Instance::AliasToInstanceMap* instances = &instance->m_nestedInstances;
                const Instance::AliasToInstanceMap* defaultInstances = defaultInstance ? &defaultInstance->m_nestedInstances : nullptr;

                JSR::ResultCode resultInstances =
                    ContinueStoringToJsonObjectField(outputValue, "Instances",
                        instances, defaultInstances, azrtti_typeid<Instance::AliasToInstanceMap>(), context);

                result.Combine(resultInstances);
            }

            return context.Report(result,
                result.GetProcessing() == JSR::Processing::Completed ? "Successfully stored Instance information for Prefab." :
                "Failed to store Instance information for Prefab.");
        }

        AZ::JsonSerializationResult::Result JsonInstanceSerializer::Load(void* outputValue, [[maybe_unused]] const AZ::Uuid& outputValueTypeId,
            const rapidjson::Value& inputValue, AZ::JsonDeserializerContext& context)
        {
            namespace JSR = AZ::JsonSerializationResult;

            AZ_Assert(azrtti_typeid<Instance>() == outputValueTypeId,
                "Unable to deserialize Prefab Instance from json because the provided type is %s.",
                outputValueTypeId.ToString<AZStd::string>().c_str());

            Instance* instance = reinterpret_cast<Instance*>(outputValue);
            AZ_Assert(instance, "Output value for JsonInstanceSerializer can't be null");

            InstanceEntityIdMapper** idMapper = context.GetMetadata().Find<InstanceEntityIdMapper*>();

            JSR::ResultCode result(JSR::Tasks::ReadField);
            {
                JSR::ResultCode sourceLoadResult =
                    ContinueLoadingFromJsonObjectField(&instance->m_templateSourcePath, azrtti_typeid<AZStd::string>(), inputValue, "Source", context);

                if (sourceLoadResult.GetOutcome() == JSR::Outcomes::Success)
                {
                    PrefabSystemComponentInterface* prefabSystemComponentInterface =
                        AZ::Interface<PrefabSystemComponentInterface>::Get();

                    AZ_Assert(prefabSystemComponentInterface,
                        "PrefabSystemComponentInterface could not be found. It is required to load Prefab Instances");

                    PrefabLoaderInterface* loaderInterface = AZ::Interface<PrefabLoaderInterface>::Get();

                    AZ_Assert(
                        loaderInterface,
                        "PrefabLoaderInterface could not be found. It is required to load Prefab Instances");

                    // Make sure we have a relative path
                    instance->m_templateSourcePath = loaderInterface->GenerateRelativePath(instance->m_templateSourcePath);

                    TemplateId templateId = prefabSystemComponentInterface->GetTemplateIdFromFilePath(instance->GetTemplateSourcePath());

                    instance->SetTemplateId(templateId);
                }

                result.Combine(sourceLoadResult);
            }

            {
                instance->m_nestedInstances.clear();

                // Load nested instances iteratively
                // We want to first create the nested instance object and assign its alias and parent pointer
                // These values are needed for the idmapper to properly resolve alias paths
                auto instancesMemberIter = inputValue.FindMember("Instances");
                if (instancesMemberIter != inputValue.MemberEnd() && instancesMemberIter->value.IsObject())
                {
                    for (auto& instanceIter : instancesMemberIter->value.GetObject())
                    {
                        const rapidjson::Value& instanceAliasValue = instanceIter.name;

                        InstanceAlias instanceAlias(instanceAliasValue.GetString(),
                            instanceAliasValue.GetStringLength());

                        AZStd::unique_ptr<Instance> nestedInstance = AZStd::make_unique<Instance>();

                        nestedInstance->m_alias = instanceAlias;
                        nestedInstance->m_parent = instance;

                        result.Combine(
                            ContinueLoading(&nestedInstance, azrtti_typeid<decltype(nestedInstance)>(),
                            instanceIter.value, context));

                        instance->m_nestedInstances.emplace(
                            instanceAlias,
                            AZStd::move(nestedInstance));
                    }
                }
            }

            // An already filled instance should be cleared if inputValue's Entities member is empty
            // The Json serializer will not do this by default as it will not attempt to load a missing member
            instance->ClearEntities();

            if (instance->m_containerEntity)
            {
                instance->m_instanceEntityMapper->UnregisterEntity(instance->m_containerEntity->GetId());
                instance->m_containerEntity.reset();
            }

            if (idMapper && *idMapper)
            {
                (*idMapper)->SetLoadingInstance(*instance);
            }

            {
                JSR::ResultCode containerEntityResult = ContinueLoadingFromJsonObjectField(
                    &instance->m_containerEntity, azrtti_typeid<decltype(instance->m_containerEntity)>(), inputValue, "ContainerEntity", context);

                result.Combine(containerEntityResult);
            }

            {
                JSR::ResultCode entitiesResult = ContinueLoadingFromJsonObjectField(
                    &instance->m_entities, azrtti_typeid<Instance::AliasToEntityMap>(), inputValue, "Entities", context);
                AddEntitiesToScrub(instance, context);
                result.Combine(entitiesResult);
            }

            {
                result.Combine(ContinueLoadingFromJsonObjectField(&instance->m_linkId, azrtti_typeid<LinkId>(), inputValue, "LinkId", context));
            }

            return context.Report(result,
                result.GetProcessing() == JSR::Processing::Completed ? "Successfully loaded instance information for prefab." :
                "Failed to load instance information for prefab");
        }

        void JsonInstanceSerializer::AddEntitiesToScrub(const Instance* instance, AZ::JsonDeserializerContext& jsonDeserializerContext)
        {
            EntityList entitiesInInstance;
            entitiesInInstance.reserve(instance->m_entities.size() + 1);

            if (instance->m_containerEntity && instance->m_containerEntity->GetId().IsValid())
            {
                entitiesInInstance.emplace_back(instance->m_containerEntity.get());
            }

            for (const auto& [entityAlias, entity] : instance->m_entities)
            {
                entitiesInInstance.emplace_back(entity.get());
            }

            InstanceEntityScrubber* instanceEntityScrubber = jsonDeserializerContext.GetMetadata().Find<InstanceEntityScrubber>();
            if (instanceEntityScrubber)
            {
                instanceEntityScrubber->AddEntitiesToScrub(entitiesInInstance);
            }
        }

    } // namespace Prefab
}
