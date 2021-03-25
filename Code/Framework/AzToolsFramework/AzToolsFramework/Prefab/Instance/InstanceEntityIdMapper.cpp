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

            if (!inputAlias.empty())
            {
                if (m_isEntityReference)
                {
                    m_unresolvedEntityAliases[m_loadingInstance].emplace_back(inputAlias, &outputValue);
                }
                else
                {
                    mappedValue = AZ::Entity::MakeId();

                    if (m_loadingInstance->RegisterEntity(mappedValue, inputAlias))
                    {
                        m_resolvedEntityAliases[m_loadingInstance].emplace_back(inputAlias, mappedValue);
                    }
                    else
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
                if (m_isEntityReference)
                {
                    mappedValue = ResolveReferenceId(inputValue);
                }
                else
                {
                    auto idMapIter = m_storingInstance->m_instanceToTemplateEntityIdMap.find(inputValue);

                    if (idMapIter != m_storingInstance->m_instanceToTemplateEntityIdMap.end())
                    {
                        mappedValue = idMapIter->second;
                    }
                    else
                    {
                        AZStd::string defaultErrorMessage =
                            "Entity with Id " + inputValue.ToString() +
                            " could not be found within its owning instance. Defaulting to invalid Id for Store.";

                        AZ_Assert(false, defaultErrorMessage.c_str());
                        context.Report(JSR::Tasks::WriteValue, JSR::Outcomes::DefaultsUsed, defaultErrorMessage);
                    }
                }
            }

            outputValue.SetString(rapidjson::StringRef(mappedValue.c_str(), mappedValue.length()), context.GetJsonAllocator());

            return context.Report(JSR::Tasks::WriteValue, JSR::Outcomes::Success, "Succesfully mapped Entity Id For Prefab Instance store");
        }

        void InstanceEntityIdMapper::SetStoringInstance(const Instance& storingInstance)
        {
            m_storingInstance = &storingInstance;
            GetAbsoluteInstanceAliasPath(m_storingInstance, m_instanceAbsolutePath);
        }

        void InstanceEntityIdMapper::SetLoadingInstance(Instance& loadingInstance)
        {
            m_loadingInstance = &loadingInstance;
        }

        void InstanceEntityIdMapper::FixUpUnresolvedEntityReferences()
        {
            // Nothing to resolve
            if (m_unresolvedEntityAliases.empty())
            {
                return;
            }

            // Calculate the absolute path to each instance based on its position in the hierarchy
            // We'll use this to generate the absolute paths of resolved and unresolved aliases
            AZStd::unordered_map<Instance*, AliasPath> absoluteInstanceAliasPaths;
            absoluteInstanceAliasPaths.reserve(m_resolvedEntityAliases.bucket_count());

            // Also calculate the absolute path to each resolved entity based on its position in the hierarchy
            AZStd::unordered_map<AliasPath, AZ::EntityId> resolvedAbsoluteEntityAliasPaths;

            for (auto& [instance, resolvedAliasIdList] : m_resolvedEntityAliases)
            {
                AliasPath absoluteInstancePath;
                GetAbsoluteInstanceAliasPath(instance, absoluteInstancePath);

                absoluteInstanceAliasPaths.emplace(instance, absoluteInstancePath);

                for (auto& [entityAlias, entityId] : resolvedAliasIdList)
                {
                    resolvedAbsoluteEntityAliasPaths.emplace(absoluteInstancePath / entityAlias, entityId);
                }
            }

            // Using the absolute paths of the instances containing the unresolved aliases
            // Attempt to find a match within the resolved paths
            // A match will allow us to update the unresolved reference id to match the id it's referencing
            for (auto& [instance, unresolvedAliasIdList] : m_unresolvedEntityAliases)
            {
                auto findResolvedInstance = absoluteInstanceAliasPaths.find(instance);

                if (findResolvedInstance != absoluteInstanceAliasPaths.end())
                {
                    AliasPath& absoluteInstancePath = findResolvedInstance->second;

                    for (auto& [entityAlias, entityPointer] : unresolvedAliasIdList)
                    {
                        AliasPath absoluteEntityReferencePath = (absoluteInstancePath / entityAlias).LexicallyNormal();

                        auto foundResolvedPath =
                            resolvedAbsoluteEntityAliasPaths.find(absoluteEntityReferencePath);

                        if (foundResolvedPath != resolvedAbsoluteEntityAliasPaths.end())
                        {
                            *entityPointer = foundResolvedPath->second;
                        }
                        else
                        {
                            AZ_Warning("Prefabs", false,
                                "Unable to resolve entity reference alias path [%s] while loading Prefab instance. "
                                "The reference was likely made on a parent or sibling prefab without the use of an override. "
                                "Defaulting the reference to an invalid EntityId.",
                                absoluteEntityReferencePath.String().c_str());
                        }
                    }
                }
                else
                {
                    AZ_Assert(false, "Prefabs - "
                        "Attempted to resolve entity alias path(s) but the instance owning the unresolved reference has no entities. "
                        "An Entity Reference can only come from an entity/component property.");
                }
            }

            return;
        }

        EntityAlias InstanceEntityIdMapper::ResolveReferenceId(const AZ::EntityId& entityId)
        {
            // Acquire the owning instance of our entity
            InstanceOptionalReference owningInstanceReference = m_storingInstance->m_instanceEntityMapper->FindOwningInstance(entityId);

            // Start with an empty alias to build out our reference path
            // If we can't resolve this id we'll return a random new alias instead of a reference path
            AliasPath relativeEntityAliasPath;
            if (!owningInstanceReference)
            {
                AZ_Assert(false,
                    "Prefab - EntityIdMapper: Entity with Id %s has no registered owning instance",
                    entityId.ToString().c_str());

                return Instance::GenerateEntityAlias();
            }

            Instance* owningInstance = &(owningInstanceReference->get());

            // Build out the absolute path of this alias
            // so we can compare it to the absolute path of our currently scoped instance
            GetAbsoluteInstanceAliasPath(owningInstance, relativeEntityAliasPath);
            relativeEntityAliasPath.Append(owningInstance->GetEntityAlias(entityId)->get());

            return relativeEntityAliasPath.LexicallyRelative(m_instanceAbsolutePath).String();
        }

        void InstanceEntityIdMapper::GetAbsoluteInstanceAliasPath(const Instance* instance, AliasPath& aliasPathResult)
        {
            // Reset the path using our preferred seperator
            aliasPathResult = AliasPath(m_aliasPathSeperator);
            const Instance* currentInstance = instance;

            // If no parent instance we are a root instance and our absolute path is empty
            while (currentInstance->m_parent)
            {
                aliasPathResult.Append(currentInstance->m_alias);
                currentInstance = currentInstance->m_parent;
            }
        }
    }
}
