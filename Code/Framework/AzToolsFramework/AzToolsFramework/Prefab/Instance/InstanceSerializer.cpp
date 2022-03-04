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
        AZ_CLASS_ALLOCATOR_IMPL(JsonInstanceSerializer, AZ::SystemAllocator, 0);

        namespace Internal
        {
            static constexpr AZStd::string_view PathStartingWithEntities = "/Entities/";
            static constexpr AZStd::string_view PathMatchingContainerEntity = "/ContainerEntity";

            //! Identifies the instance members to reload by parsing through the patches provided.
            static void IdentifyInstanceMembersToReload(
                PrefabDom& patches, AZStd::unordered_set<EntityAlias>& entitiesToReload, bool& shouldReloadContainerEntity)
            {
                for (const PrefabDomValue& patchEntry : patches.GetArray())
                {
                    PrefabDomValue::ConstMemberIterator patchEntryIterator = patchEntry.FindMember("path");
                    if (patchEntryIterator != patchEntry.MemberEnd())
                    {
                        AZStd::string_view patchPath = patchEntryIterator->value.GetString();
                        if (patchPath.starts_with(PathStartingWithEntities))
                        {
                            patchPath.remove_prefix(PathStartingWithEntities.size());
                            AZStd::size_t pathSeparatorIndex = patchPath.find('/');
                            if (pathSeparatorIndex != AZStd::string::npos)
                            {
                                entitiesToReload.emplace(patchPath.substr(0, pathSeparatorIndex));
                            }
                            else
                            {
                                patchEntryIterator = patchEntry.FindMember("op");
                                if (patchEntryIterator != patchEntry.MemberEnd())
                                {
                                    AZStd::string opPath = patchEntryIterator->value.GetString();
                                    if (opPath != "remove") // Removal of entity needs to be addressed later.
                                    {
                                        // Could be an add or change from empty->full. The later case is rare but not impossible.
                                        entitiesToReload.emplace(AZStd::move(patchPath));
                                    }
                                }
                            }
                        }
                        else if (patchPath.starts_with(PathMatchingContainerEntity))
                        {
                            shouldReloadContainerEntity = true;
                        }
                    }
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

            if (idMapper && *idMapper)
            {
                (*idMapper)->SetLoadingInstance(*instance);
            }

            {
                result.Combine(
                    ContinueLoadingFromJsonObjectField(&instance->m_linkId, azrtti_typeid<LinkId>(), inputValue, "LinkId", context));
            }

            PrefabDomUtils::InstanceDomMetadata* instanceDomMetadata = context.GetMetadata().Find<PrefabDomUtils::InstanceDomMetadata>();
            PrefabDomValueConstReference cachedInstanceDom = instance->GetCachedInstanceDom();

            if (instanceDomMetadata == nullptr || cachedInstanceDom == AZStd::nullopt)
            {
                // An already filled instance should be cleared if inputValue's Entities member is empty
                // The Json serializer will not do this by default as it will not attempt to load a missing member
                instance->ClearEntities();
                {
                    JSR::ResultCode containerEntityResult = ContinueLoadingFromJsonObjectField(
                        &instance->m_containerEntity, azrtti_typeid<decltype(instance->m_containerEntity)>(), inputValue, "ContainerEntity",
                        context);

                    result.Combine(containerEntityResult);

                    if (instance->m_containerEntity && instance->m_containerEntity->GetId().IsValid())
                    {
                        AddEntitiesToScrub(EntityList{ instance->m_containerEntity.get() }, context);
                    }
                }

                {
                    JSR::ResultCode entitiesResult = ContinueLoadingFromJsonObjectField(
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
                    AddEntitiesToScrub(AZStd::move(entitiesLoaded), context);
                }
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

        void JsonInstanceSerializer::Reload(
            const rapidjson::Value& inputValue,
            AZ::JsonDeserializerContext& context,
            Instance* instance,
            PrefabDom patches,
            AZ::JsonSerializationResult::ResultCode& result)
        {
            AZStd::unordered_set<EntityAlias> entitiesToReload;
            bool shouldReloadContainerEntity = false;

            Internal::IdentifyInstanceMembersToReload(patches, entitiesToReload, shouldReloadContainerEntity);

            if (shouldReloadContainerEntity)
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
                    AddEntitiesToScrub(EntityList{ instance->m_containerEntity.get() }, context);
                }
            }

            {
                auto entitiesMemberIterator = inputValue.FindMember("Entities");
                if (entitiesMemberIterator != inputValue.MemberEnd() && entitiesMemberIterator->value.IsObject())
                {
                    EntityList entitiesLoaded;
                    entitiesLoaded.reserve(entitiesToReload.size());

                    for (AZStd::string entityAlias : entitiesToReload)
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

                    AddEntitiesToScrub(AZStd::move(entitiesLoaded), context);
                }
            }
        }

        void JsonInstanceSerializer::AddEntitiesToScrub(EntityList entitiesModified, AZ::JsonDeserializerContext& jsonDeserializerContext)
        {
            InstanceEntityScrubber* instanceEntityScrubber = jsonDeserializerContext.GetMetadata().Find<InstanceEntityScrubber>();
            if (instanceEntityScrubber)
            {
                instanceEntityScrubber->AddEntitiesToScrub(entitiesModified);
            }
        }

    } // namespace Prefab
} // namespace AzToolsFramework
