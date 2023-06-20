/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
#include <Prefab/PrefabDomUtils.h>

namespace AzToolsFramework
{
    namespace Prefab
    {
        AZ_CLASS_ALLOCATOR_IMPL(JsonInstanceSerializer, AZ::SystemAllocator);

        namespace Internal
        {
            static void AddEntitiesToScrub(
                const AZStd::span<AZ::Entity*>& entitiesModified, AZ::JsonDeserializerContext& jsonDeserializerContext)
            {
                InstanceEntityScrubber* instanceEntityScrubber = jsonDeserializerContext.GetMetadata().Find<InstanceEntityScrubber>();
                if (instanceEntityScrubber)
                {
                    instanceEntityScrubber->AddEntitiesToScrub(entitiesModified);
                }
            }
        }

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

                PrefabDomValueReference entitiesDom = PrefabDomUtils::FindPrefabDomValue(outputValue, "Entities");
                if (entitiesDom.has_value() && context.ShouldKeepDefaults() && entitiesDom->get().IsArray() && entitiesDom->get().Empty())
                {
                    entitiesDom->get().SetObject();
                }

                result.Combine(resultEntities);
            }

            {
                AZ::ScopedContextPath subPathInstances(context, "m_nestedInstances");
                const Instance::AliasToInstanceMap* instances = &instance->m_nestedInstances;
                const Instance::AliasToInstanceMap* defaultInstances = defaultInstance ? &defaultInstance->m_nestedInstances : nullptr;

                JSR::ResultCode resultInstances = ContinueStoringToJsonObjectField(
                    outputValue, "Instances",
                        instances, defaultInstances, azrtti_typeid<Instance::AliasToInstanceMap>(), context);

                PrefabDomValueReference instancesDom = PrefabDomUtils::FindPrefabDomValue(outputValue, "Instances");
                if (instancesDom.has_value() && context.ShouldKeepDefaults() && instancesDom->get().IsArray() && instancesDom->get().Empty())
                {
                    instancesDom->get().SetObject();
                }

                result.Combine(resultInstances);
            }

            PrefabDomUtils::LinkIdMetadata* subPathLinkId = context.GetMetadata().Find<PrefabDomUtils::LinkIdMetadata>();
            if (subPathLinkId)
            {
                AZ::ScopedContextPath subPathSource(context, "m_linkId");

                result = ContinueStoringToJsonObjectField(
                    outputValue, "LinkId", &(instance->m_linkId), &InvalidLinkId, azrtti_typeid<decltype(instance->m_linkId)>(), context);
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
                result.Combine(
                    ContinueLoadingFromJsonObjectField(&instance->m_linkId, azrtti_typeid<LinkId>(), inputValue, "LinkId", context));
            }

            PrefabDomUtils::InstanceDomMetadata* instanceDomMetadata = context.GetMetadata().Find<PrefabDomUtils::InstanceDomMetadata>();
            PrefabDomReference cachedInstanceDom = instance->GetCachedInstanceDom();

            if (instanceDomMetadata == nullptr || cachedInstanceDom == AZStd::nullopt)
            {
                ClearAndLoadInstances(inputValue, context, instance, result);

                if (idMapper && *idMapper)
                {
                    (*idMapper)->SetLoadingInstance(*instance);
                }

                {
                    instance->DetachContainerEntity();
                    JSR::ResultCode containerEntityResult = ContinueLoadingFromJsonObjectField(
                        &instance->m_containerEntity, azrtti_typeid<decltype(instance->m_containerEntity)>(), inputValue, "ContainerEntity",
                        context);

                    result.Combine(containerEntityResult);

                    if (instance->m_containerEntity && instance->m_containerEntity->GetId().IsValid())
                    {
                        EntityList containerEntity{ instance->m_containerEntity.get() };
                        Internal::AddEntitiesToScrub(containerEntity, context);
                    }
                }

                ClearAndLoadEntities(inputValue, context, instance, result);
            }
            else
            {
                PrefabDom jsonPatch;
                AZ::JsonSerialization::CreatePatch(
                    jsonPatch, jsonPatch.GetAllocator(), cachedInstanceDom->get(), inputValue, AZ::JsonMergeApproach::JsonPatch);

                if (jsonPatch.IsArray() && jsonPatch.GetArray().Empty())
                {
                    JSR::ResultCode skippedResult(JSR::Tasks::CreatePatch, JSR::Outcomes::Skipped);
                    result.Combine(skippedResult);
                }
                else
                {
                    Reload(inputValue, context, instance, AZStd::move(jsonPatch), result);
                }
            }

            if (instanceDomMetadata)
            {
                instance->SetCachedInstanceDom(inputValue);
            }

            return context.Report(
                result,
                result.GetProcessing() == JSR::Processing::Completed ? "Successfully loaded instance information for prefab."
                                                                     : "Failed to load instance information for prefab");
        }

        void JsonInstanceSerializer::ClearAndLoadEntities(
            const rapidjson::Value& inputValue,
            AZ::JsonDeserializerContext& context,
            Instance* instance,
            AZ::JsonSerializationResult::ResultCode& result)
        {
            for (auto& [entityAlias, entity] :instance->m_entities)
            {
                instance->UnregisterEntity(entity->GetId());
            }
            instance->m_entities.clear();

            AZ::JsonSerializationResult::ResultCode entitiesResult = ContinueLoadingFromJsonObjectField(
                &instance->m_entities, azrtti_typeid<Instance::AliasToEntityMap>(), inputValue, "Entities", context);

            EntityList entitiesLoaded;
            entitiesLoaded.reserve(instance->m_entities.size());
            for (const auto& [entityAlias, entity] : instance->m_entities)
            {
                if (entity != nullptr)
                {
                    entitiesLoaded.emplace_back(entity.get());
                }
            }

            result.Combine(entitiesResult);
            Internal::AddEntitiesToScrub(entitiesLoaded, context);
        }

        void JsonInstanceSerializer::ClearAndLoadInstances(
            const rapidjson::Value& inputValue,
            AZ::JsonDeserializerContext& context,
            Instance* instance,
            AZ::JsonSerializationResult::ResultCode& result)
        {
            instance->m_nestedInstances.clear();

            // Load nested instances iteratively
            // We want to first create the nested instance object and assign its alias and parent pointer
            // These values are needed for the idmapper to properly resolve alias paths
            auto instancesMemberIter = inputValue.FindMember(PrefabDomUtils::InstancesName);
            if (instancesMemberIter != inputValue.MemberEnd() && instancesMemberIter->value.IsObject())
            {
                for (auto& instanceIter : instancesMemberIter->value.GetObject())
                {
                    const rapidjson::Value& instanceAliasValue = instanceIter.name;

                    InstanceAlias instanceAlias(instanceAliasValue.GetString(), instanceAliasValue.GetStringLength());

                    AZStd::unique_ptr<Instance> nestedInstance = AZStd::make_unique<Instance>();

                    nestedInstance->m_alias = instanceAlias;
                    nestedInstance->m_parent = instance;

                    result.Combine(
                        ContinueLoading(&nestedInstance, azrtti_typeid<decltype(nestedInstance)>(), instanceIter.value, context));

                    instance->m_nestedInstances.emplace(instanceAlias, AZStd::move(nestedInstance));
                }
            }
        }

        void JsonInstanceSerializer::Reload(
            const rapidjson::Value& inputValue,
            AZ::JsonDeserializerContext& context,
            Instance* instance,
            PrefabDom patches,
            AZ::JsonSerializationResult::ResultCode& result)
        {
            PrefabDomUtils::PatchesMetadata patchesMetadata = PrefabDomUtils::IdentifyModifiedInstanceMembers(patches);

            if (patchesMetadata.m_clearAndLoadAllInstances)
            {
                ClearAndLoadInstances(inputValue, context, instance, result);
            }
            else
            {
                auto instancesMemberIterator = inputValue.FindMember(PrefabDomUtils::InstancesName);
                if (instancesMemberIterator != inputValue.MemberEnd() && instancesMemberIterator->value.IsObject())
                {
                    // Remove nested instances identified in patches metadata.
                    for (const AZStd::string& instanceAlias : patchesMetadata.m_instancesToRemove)
                    {
                        instance->DetachNestedInstance(instanceAlias);
                    }

                    // Add nested instances identified in patches metadata.
                    for (const AZStd::string& instanceAlias : patchesMetadata.m_instancesToAdd)
                    {
                        AZStd::unique_ptr<Instance> detachedInstance = instance->DetachNestedInstance(instanceAlias);
                        detachedInstance.reset();

                        AZStd::unique_ptr<Instance> nestedInstance = AZStd::make_unique<Instance>(instance->m_entityIdInstanceRelationship);

                        nestedInstance->m_alias = instanceAlias;
                        nestedInstance->m_parent = instance;

                        auto instanceIter = instancesMemberIterator->value.FindMember(instanceAlias.c_str());
                        result.Combine(
                            ContinueLoading(&nestedInstance, azrtti_typeid<decltype(nestedInstance)>(), instanceIter->value, context));

                        instance->m_nestedInstances.emplace(instanceAlias, AZStd::move(nestedInstance));

                        PrefabDomUtils::InstanceDomMetadata* instanceDomMetadata =
                            context.GetMetadata().Find<PrefabDomUtils::InstanceDomMetadata>();
                        if (instanceDomMetadata)
                        {
                            instance->SetCachedInstanceDom(instanceIter->value);
                        }
                    }

                    // Reload nested instances identified in patches metadata. This will trigger further instance loads recursively.
                    for (const AZStd::string& instanceAlias : patchesMetadata.m_instancesToReload)
                    {
                        InstanceOptionalReference nestedInstance = instance->FindNestedInstance(instanceAlias);
                        if (nestedInstance.has_value())
                        {
                            auto instanceIter = instancesMemberIterator->value.FindMember(instanceAlias.c_str());
                            result.Combine(ContinueLoading(
                                &nestedInstance->get(), azrtti_typeid<decltype(nestedInstance->get())>(), instanceIter->value, context));
                        }
                    }
                }
            }

            InstanceEntityIdMapper** idMapper = context.GetMetadata().Find<InstanceEntityIdMapper*>();
            if (idMapper && *idMapper)
            {
                (*idMapper)->SetLoadingInstance(*instance);
            }

            if (patchesMetadata.m_shouldReloadContainerEntity)
            {
                if (instance->m_containerEntity)
                {
                    instance->UnregisterEntity(instance->m_containerEntity->GetId());
                    instance->m_containerEntity.reset();
                }

                auto instancesMemberIter = inputValue.FindMember("ContainerEntity");
                if (instancesMemberIter != inputValue.MemberEnd() && instancesMemberIter->value.IsObject())
                {
                    AZ::JsonSerializationResult::ResultCode containerEntityResult = ContinueLoadingFromJsonObjectField(
                        &instance->m_containerEntity, azrtti_typeid<decltype(instance->m_containerEntity)>(), inputValue, "ContainerEntity",
                        context);

                    result.Combine(containerEntityResult);
                    EntityList containerEntity{ instance->m_containerEntity.get() };
                    Internal::AddEntitiesToScrub(containerEntity, context);
                }
            }

            if (patchesMetadata.m_clearAndLoadAllEntities)
            {
                ClearAndLoadEntities(inputValue, context, instance, result);
            }
            else
            {
                auto entitiesMemberIterator = inputValue.FindMember("Entities");
                if (entitiesMemberIterator != inputValue.MemberEnd() && entitiesMemberIterator->value.IsObject())
                {
                    // Remove entities identified in patches metadata.
                    for (AZStd::string entityAlias : patchesMetadata.m_entitiesToRemove)
                    {
                        EntityOptionalReference existingEntity = instance->GetEntity(entityAlias);

                        if (existingEntity != AZStd::nullopt)
                        {
                            instance->DetachEntity(existingEntity->get().GetId());
                        }
                    }

                    EntityList entitiesLoaded;
                    entitiesLoaded.reserve(patchesMetadata.m_entitiesToReload.size());

                    // Reload entities identified in patches metadata. This includes addition of new entities too.
                    for (AZStd::string entityAlias : patchesMetadata.m_entitiesToReload)
                    {
                        EntityOptionalReference existingEntity = instance->GetEntity(entityAlias);

                        if (existingEntity != AZStd::nullopt)
                        {
                            instance->DetachEntity(existingEntity->get().GetId());
                        }

                        AZStd::unique_ptr<AZ::Entity> entity = AZStd::make_unique<AZ::Entity>();
                        auto entityIterator = entitiesMemberIterator->value.FindMember(entityAlias.c_str());
                        result.Combine(ContinueLoading(&entity, azrtti_typeid<decltype(entity)>(), entityIterator->value, context));
                        entitiesLoaded.emplace_back(entity.get());
                        instance->m_entities.emplace(entityAlias, AZStd::move(entity));
                    }

                    Internal::AddEntitiesToScrub(entitiesLoaded, context);
                }
            }
        }
    } // namespace Prefab
} // namespace AzToolsFramework

