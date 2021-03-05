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

#include <AzCore/Component/Entity.h>
#include <AzCore/StringFunc/StringFunc.h>
#include <AzToolsFramework/Prefab/Instance/Instance.h>
#include <AzToolsFramework/Prefab/Instance/InstanceEntityIdMapper.h>
#include <AzToolsFramework/Prefab/Instance/InstanceEntityMapper.h>

namespace AzToolsFramework
{
    namespace Prefab
    {
        AZ::JsonSerializationResult::Result InstanceEntityIdMapper::MapJsonToId(AZ::EntityId& outputValue,
            const rapidjson::Value& inputValue, AZ::JsonDeserializerContext& context)
        {
            namespace JSR = AZ::JsonSerializationResult;

            if (!m_loadingInstance)
            {
                AZ_Assert(false,
                    "Attempted to map an EntityId in Prefab Instance without setting the Loading Prefab Instance");

                return context.Report(JSR::Tasks::WriteValue, JSR::Outcomes::DefaultsUsed,
                    "Attempted to map an EntityId in Prefab Instance without setting the Loading Prefab Instance");
            }

            AZ::EntityId mappedValue(AZ::EntityId::InvalidEntityId);
            EntityAlias inputAlias;

            if (inputValue.IsString())
            {
                inputAlias = EntityAlias(inputValue.GetString(), inputValue.GetStringLength());
            }

            if(!inputAlias.empty())
            {
                auto entityIdMapIter = m_loadingInstance->m_templateToInstanceEntityIdMap.find(inputAlias);

                if (entityIdMapIter != m_loadingInstance->m_templateToInstanceEntityIdMap.end())
                {
                    mappedValue = entityIdMapIter->second;
                }
                else
                {
                    if (inputAlias[0] != ReferencePathDelimiter)
                    {
                        mappedValue = AZ::Entity::MakeId();
                    }
                    else
                    {
                        mappedValue = ResolveEntityReferencePath(inputAlias);
                    }

                    if (!m_loadingInstance->RegisterEntity(mappedValue, inputAlias))
                    {
                        mappedValue = AZ::EntityId(AZ::EntityId::InvalidEntityId);
                        context.Report(JSR::Tasks::ReadField, JSR::Outcomes::DefaultsUsed,
                            "Unable to register entity Id with prefab instance during load. Using default invalid id");
                    }
                }
            }

            outputValue = mappedValue;

            return context.Report(JSR::Tasks::ReadField, JSR::Outcomes::Success, "Succesfully mapped Entity Id For Prefab Instance load");
        }

        AZ::JsonSerializationResult::Result InstanceEntityIdMapper::MapIdToJson(rapidjson::Value& outputValue,
            const AZ::EntityId& inputValue, AZ::JsonSerializerContext& context)
        {
            namespace JSR = AZ::JsonSerializationResult;

            if (!m_storingInstance)
            {
                AZ_Assert(false,
                    "Attempted to map an EntityId in Prefab Instance without setting the Storing Prefab Instance");

                return context.Report(JSR::Tasks::WriteValue, JSR::Outcomes::DefaultsUsed,
                    "Attempted to map an EntityId in Prefab Instance without setting the Storing Prefab Instance");
            }

            EntityAlias mappedValue;
            if (inputValue.IsValid())
            {
                auto idMapIter = m_storingInstance->m_instanceToTemplateEntityIdMap.find(inputValue);

                if (idMapIter != m_storingInstance->m_instanceToTemplateEntityIdMap.end())
                {
                    mappedValue = idMapIter->second;
                }
                else
                {
                    mappedValue = ResolveEntityId(inputValue);
                }
            }

            outputValue.SetString(rapidjson::StringRef(mappedValue.c_str(), mappedValue.length()), context.GetJsonAllocator());

            return context.Report(JSR::Tasks::WriteValue, JSR::Outcomes::Success, "Succesfully mapped Entity Id For Prefab Instance store");
        }

        void InstanceEntityIdMapper::SetStoringInstance(const Instance& storingInstance)
        {
            m_storingInstance = &storingInstance;
        }

        void InstanceEntityIdMapper::SetLoadingInstance(Instance& loadingInstance)
        {
            m_loadingInstance = &loadingInstance;
        }

        AZ::EntityId InstanceEntityIdMapper::ResolveEntityReferencePath(const AZStd::string& entityIdReferencePath)
        {
            // If we can't resolve the path treat the entire path as one alias.
            // Assigning a random id instead of an invalid id will save off a unique mapping.
            // Allowing the ability to store the instance back to this full alias again.
            // This prevents the replacement of the path with an empty alias and avoids data loss.
            AZ::EntityId resolvedId = AZ::Entity::MakeId();

            AZStd::string_view referencePathView = entityIdReferencePath;

            // Trim the starting '/' from the path view as tokenizing it would result in an empty token
            referencePathView.remove_prefix(1);

            AZStd::optional<AZStd::string_view> currentPathToken = AZ::StringFunc::TokenizeNext(referencePathView, ReferencePathDelimiter);

            const Instance* currentInstance = m_loadingInstance;

            // Walk down the instance hierarchy using the Alias path
            while(!referencePathView.empty() && currentPathToken.has_value())
            {
                auto aliasedInstanceIter = currentInstance->m_nestedInstances.find(currentPathToken.value());
                if (aliasedInstanceIter != currentInstance->m_nestedInstances.end())
                {
                    AZ_Assert(aliasedInstanceIter->second,
                        "Prefab - EntityIdMapper: An instance of %s contained a null instance using alias %.*s",
                        currentInstance->GetTemplateSourcePath().c_str(),
                        aznumeric_cast<int>(currentPathToken.value().size()), currentPathToken.value().data());

                    currentInstance = aliasedInstanceIter->second.get();
                }
                else
                {
                    // If there's ever a path mismatch then there's no valid entity to reference
                    return resolvedId;
                }

                currentPathToken = AZ::StringFunc::TokenizeNext(referencePathView, ReferencePathDelimiter);
            }

            // If we ever recieved an invalid token then our path was empty or we walked too far
            if (!currentPathToken.has_value())
            {
                return resolvedId;
            }

            // Our current alias should be an entity in our current instance
            auto aliasedEntityIter = currentInstance->m_entities.find(currentPathToken.value());
            if (aliasedEntityIter != currentInstance->m_entities.end())
            {
                const AZStd::unique_ptr<AZ::Entity>& aliasedEntity = aliasedEntityIter->second;

                AZ_Assert(aliasedEntity,
                    "Prefab - EntityIdMapper: An instance of %s contained a null entity using alias %s",
                     currentInstance->GetTemplateSourcePath().c_str(),
                    aznumeric_cast<int>(currentPathToken.value().size()), currentPathToken.value().data());

                resolvedId = aliasedEntity->GetId();
            }

            return resolvedId;
        }

        EntityAlias InstanceEntityIdMapper::ResolveEntityId(const AZ::EntityId& entityId)
        {
            // Acquire the owning instance of our entity
            InstanceOptionalReference currentInstanceOptionalReference = m_storingInstance->m_instanceEntityMapper->FindOwningInstance(entityId);

            // Start with an empty alias to build out our reference path
            // If we can't resolve this id we'll return a random new alias instead of a reference path
            EntityAlias resolvedAlias;
            if (!currentInstanceOptionalReference)
            {
                AZ_Assert(false,
                    "Prefab - EntityIdMapper: Entity with Id %s has no registered owning instance",
                    entityId.ToString().c_str());

                return Instance::GenerateEntityAlias();
            }
            Instance* currentInstance = &(currentInstanceOptionalReference->get());

            // This entity should have an alias in its owning instance
            // If not
            auto entityAliasIter = currentInstance->m_instanceToTemplateEntityIdMap.find(entityId);
            if (entityAliasIter == currentInstance->m_instanceToTemplateEntityIdMap.end())
            {
                AZ_Assert(false,
                    "Prefab - EntityIdMapper: Instance of Prefab %s was registered to own entity with Id %s. "
                    "However the entity has no alias within the instance",
                    currentInstance->GetTemplateSourcePath().c_str(), entityId.ToString().c_str());

                return Instance::GenerateEntityAlias();
            }

            // Start off the reference path with /EntityAlias
            resolvedAlias = ReferencePathDelimiter + entityAliasIter->second;

            // Walk up the instance hierarchy
            while (currentInstance)
            {
                if (currentInstance == m_storingInstance)
                {
                    // Path is resolved if we intersect with the instance resolving this id
                    return resolvedAlias;
                }

                // Continue building out the reference path
                // /InstanceAlias/InstanceAlias/EntityAlias
                resolvedAlias = ReferencePathDelimiter + currentInstance->m_alias + resolvedAlias;
                currentInstance = currentInstance->m_parent;
            }

            // If we hit a null instance we failed to resolve the id
            return Instance::GenerateEntityAlias();
        }
    }
}
