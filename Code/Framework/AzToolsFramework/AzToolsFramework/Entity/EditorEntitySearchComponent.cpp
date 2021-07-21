/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/Component/EditorComponentAPIBus.h>
#include <AzToolsFramework/Entity/EditorEntitySearchComponent.h>
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>
#include <AzToolsFramework/Entity/EditorEntityHelpers.h>
#include <AzToolsFramework/Entity/EditorEntityInfoBus.h>

#include <AzCore/RTTI/BehaviorContext.h>

#include <AzFramework/StringFunc/StringFunc.h>
#include <AzCore/std/string/wildcard.h>
#include <AzCore/Component/TransformBus.h>

namespace AzToolsFramework
{
    namespace Components
    {
        void EditorEntitySearchComponent::Reflect(AZ::ReflectContext* context)
        {
            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<EditorEntitySearchComponent, AZ::Component>();

                serializeContext->Class<EntitySearchFilter>()
                    ->Version(1)
                    ->Field("Names", &EntitySearchFilter::m_names)
                    ->Field("NamesCaseSensitive", &EntitySearchFilter::m_namesCaseSensitive)
                    ->Field("Components", &EntitySearchFilter::m_components)
                    ->Field("ComponentMatchAll", &EntitySearchFilter::m_mustMatchAllComponents)
                    ->Field("Roots", &EntitySearchFilter::m_roots)
                    ->Field("NameIsRootBased", &EntitySearchFilter::m_namesAreRootBased)
                    ->Field("Aabb", &EntitySearchFilter::m_aabb)
                    ;
            }

            if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
            {
                behaviorContext->Class<EntitySearchFilter>("SearchFilter")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                    ->Attribute(AZ::Script::Attributes::Category, "Entity")
                    ->Attribute(AZ::Script::Attributes::Module, "entity")
                    ->Property("Names", BehaviorValueProperty(&EntitySearchFilter::m_names))
                        ->Attribute(AZ::Script::Attributes::Alias, "names")
                    ->Property("NamesCaseSensitive", BehaviorValueProperty(&EntitySearchFilter::m_namesCaseSensitive))
                        ->Attribute(AZ::Script::Attributes::Alias, "names_case_sensitive")
                    ->Property("Components", BehaviorValueProperty(&EntitySearchFilter::m_components))
                        ->Attribute(AZ::Script::Attributes::Alias, "components")
                    ->Property("ComponentMatchAll", BehaviorValueProperty(&EntitySearchFilter::m_mustMatchAllComponents))
                        ->Attribute(AZ::Script::Attributes::Alias, "components_match_all")
                    ->Property("Roots", BehaviorValueProperty(&EntitySearchFilter::m_roots))
                        ->Attribute(AZ::Script::Attributes::Alias, "roots")
                    ->Property("NamesAreRootBased", BehaviorValueProperty(&EntitySearchFilter::m_namesAreRootBased))
                        ->Attribute(AZ::Script::Attributes::Alias, "names_are_root_based")
                    ->Property("Aabb", BehaviorValueProperty(&EntitySearchFilter::m_aabb))
                        ->Attribute(AZ::Script::Attributes::Alias, "aabb")
                    ;

                behaviorContext->EBus<EditorEntitySearchBus>("SearchBus")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                    ->Attribute(AZ::Script::Attributes::Category, "Entity")
                    ->Attribute(AZ::Script::Attributes::Module, "entity")
                    ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                    ->Event("SearchEntities", &EditorEntitySearchRequests::SearchEntities)
                    ->Event("GetRootEditorEntities", &EditorEntitySearchRequests::GetRootEditorEntities)
                    ;
            }
        }

        void EditorEntitySearchComponent::Activate()
        {
            EditorEntitySearchBus::Handler::BusConnect();
        }

        void EditorEntitySearchComponent::Deactivate()
        {
            EditorEntitySearchBus::Handler::BusDisconnect();
        }

        EntityIdList EditorEntitySearchComponent::SearchEntities(const EntitySearchFilter& filter)
        {
            SearchConditions conditions = SearchConditions(filter);
            EntityIdSet searchResults;

            if (!conditions.m_filter.m_roots.empty() && conditions.m_filter.m_namesAreRootBased)
            {
                // Roots specified, Name is root based
                // Search from roots, only based on children
                // (paths starting from children are still matched)

                for (const AZ::EntityId& rootId : conditions.m_filter.m_roots)
                {
                    EntityIdList rootChildren;
                    EditorEntityInfoRequestBus::EventResult(rootChildren, rootId, &EditorEntityInfoRequestBus::Events::GetChildren);

                    for (AZ::EntityId entityId : rootChildren)
                    {
                        FilterEntity(entityId, conditions, searchResults);
                    }
                }
            }
            else if (conditions.m_filter.m_roots.empty() && conditions.m_filter.m_namesAreRootBased)
            {
                // Roots not specified, Name is root based
                // Search children of level root only
                // (paths starting from root are still matched)

                EntityIdList rootEntities = GetRootEditorEntities();

                for (AZ::EntityId entityId : rootEntities)
                {
                    FilterEntity(entityId, conditions, searchResults);
                }
            }
            else if (!conditions.m_filter.m_roots.empty() && !conditions.m_filter.m_namesAreRootBased)
            {
                // Roots specified, Name is not root based
                // Only search subtrees of the roots
                // (Paths starting from anywhere in the subtree will be matched)

                for (const AZ::EntityId& rootId : conditions.m_filter.m_roots)
                {
                    EntityIdList subTreeEntityList;
                    GetAllEntitiesInSubtree(rootId, subTreeEntityList);

                    for (AZ::EntityId entityId : subTreeEntityList)
                    {
                        FilterEntity(entityId, conditions, searchResults);
                    }
                }
            }
            else
            {
                // Search every Entity

                auto searchFunction = [this, &searchResults, &conditions](AZ::Entity* entity)
                {
                    FilterEntity(entity->GetId(), conditions, searchResults);
                };

                AZ::ComponentApplicationBus::Broadcast(&AZ::ComponentApplicationRequests::EnumerateEntities, searchFunction);
            }

            return EntityIdList(searchResults.begin(), searchResults.end());
        }

        EntityIdList EditorEntitySearchComponent::GetRootEditorEntities()
        {
            EntityIdList rootEntities;
            EditorEntityInfoRequestBus::EventResult(rootEntities, AZ::EntityId(), &EditorEntityInfoRequestBus::Events::GetChildren);
            return rootEntities;
        }

        EditorEntitySearchComponent::SearchConditions::SearchConditions(const EntitySearchFilter& filter) : m_filter(filter)
        {
            if (!m_filter.m_names.empty())
            {
                m_tokenizedPaths.resize(m_filter.m_names.size());

                size_t i = 0;
                for (const AZStd::string& name : m_filter.m_names)
                {
                    AzFramework::StringFunc::Tokenize(name.c_str(), m_tokenizedPaths[i], "|");
                    ++i;
                }
            }
        }

        void EditorEntitySearchComponent::GetAllEntitiesInSubtree(AZ::EntityId rootId, EntityIdList& subTreeEntityList)
        {
            subTreeEntityList.clear();
            subTreeEntityList.push_back(rootId);

            for (size_t index = 0; index < subTreeEntityList.size(); ++index)
            {
                EntityIdList children;
                EditorEntityInfoRequestBus::EventResult(children, subTreeEntityList[index], &EditorEntityInfoRequestBus::Events::GetChildren);

                if (!children.empty())
                {
                    subTreeEntityList.insert(subTreeEntityList.end(), children.begin(), children.end());
                }
            }
        }

        void EditorEntitySearchComponent::FilterEntity(AZ::EntityId entityId, const SearchConditions& conditions, EntityIdSet& filteredEntities)
        {
            EntityIdSet tempEntities;
            
            // Ignore root entity
            if (!entityId.IsValid() || entityId == AZ::EntityId(0))
            {
                return;
            }

            // Only return Editor Entities (for now)
            bool isEditorEntity = false;
            EditorEntityContextRequestBus::BroadcastResult(isEditorEntity, &EditorEntityContextRequests::IsEditorEntity, entityId);
            if (!isEditorEntity)
            {
                return;
            }

            // Name/Path match
            if (!conditions.m_tokenizedPaths.empty())
            {
                for (const AZStd::vector<AZStd::string>& tokenizedPath : conditions.m_tokenizedPaths)
                {
                    if (!tokenizedPath.empty())
                    {
                        FollowPathHelper(EntityIdList{ entityId }, tokenizedPath, &tempEntities, conditions.m_filter.m_namesCaseSensitive, tokenizedPath.size());
                    }
                }
            }
            else
            {
                tempEntities.insert(entityId);
            }

            // Run all entities that matched names/paths through the rest of the conditions
            // Note: it's early out, so order checks from least to most expensive
            for (AZ::EntityId tempEntityId : tempEntities)
            {
                if (IsPositionContained(tempEntityId, conditions.m_filter.m_aabb) // AABB
                    && AreComponentsMatched(tempEntityId, conditions.m_filter.m_components, conditions.m_filter.m_mustMatchAllComponents) // Component Types and Property Values
                    )
                {
                    filteredEntities.insert(tempEntityId);
                }
            }
        }

        void EditorEntitySearchComponent::FollowPathHelper(
            const AZStd::vector<AZ::EntityId>& entities, const AZStd::vector<AZStd::string>& tokenizedPath,
            EntityIdSet* result, bool caseSensitive, size_t pathSize, size_t currentIndex)
        {
            EntityIdSet matches;
            EntityIdSet* resultPtr;

            if (currentIndex + 1 == pathSize)
            {
                // End of path - matching entities are our result
                resultPtr = result;
            }
            else
            {
                // Save entities for next step
                resultPtr = &matches;
            }

            for (AZ::EntityId entityId : entities)
            {
                if((!caseSensitive && wildcard_match(tokenizedPath[currentIndex], GetEntityById(entityId)->GetName())) || 
                    (caseSensitive && wildcard_match_case(tokenizedPath[currentIndex], GetEntityById(entityId)->GetName())))
                {
                    resultPtr->insert(entityId);
                }
            }

            if (matches.size() > 0 && currentIndex + 1 < pathSize)
            {
                // Get children of nextStep
                AZStd::vector<AZ::EntityId> nextStep;

                for (AZ::EntityId& entityId : matches)
                {
                    AZStd::vector<AZ::EntityId> children;
                    EditorEntityInfoRequestBus::EventResult(children, entityId, &EditorEntityInfoRequestBus::Events::GetChildren);

                    nextStep.insert(nextStep.end(), children.begin(), children.end());
                }

                if (nextStep.size() > 0)
                {
                    FollowPathHelper(nextStep, tokenizedPath, result, caseSensitive, pathSize, ++currentIndex);
                }
            }
        }

        bool EditorEntitySearchComponent::IsPositionContained(
            AZ::EntityId entityId,
            const AZ::Aabb& aabb) const
        {
            if (!aabb.IsValid())
            {
                return true;
            }

            AZ::Vector3 entityWorldPosition;
            AZ::TransformBus::EventResult(entityWorldPosition, entityId, &AZ::TransformBus::Events::GetWorldTranslation);
            return aabb.Contains(entityWorldPosition);
        }

        bool EditorEntitySearchComponent::AreComponentsMatched(
            AZ::EntityId entityId,
            const EntitySearchFilter::Components& components,
            bool mustMatchAllComponents) const
        {
            if (components.empty())
            {
                return true;
            }

            bool doesMatch = !mustMatchAllComponents;

            // check each component type/property values filter
            for (auto& componentProperties : components)
            {
                // check if entity has any component matching the type       
                bool hasComponent = false;
                AZ::Uuid componentTypeId = componentProperties.first;
                EditorComponentAPIBus::BroadcastResult(hasComponent, &EditorComponentAPIRequests::HasComponentOfType, entityId, componentTypeId);

                // If must match all components, return false if any component's type/property values are not matched.
                // If not must match all components, return true if any component's type/property value is matched.
                doesMatch = hasComponent
                    && (componentProperties.second.empty()
                        || ArePropertyValuesOfComponentMatched(entityId, componentTypeId, componentProperties.second, mustMatchAllComponents));

                if (doesMatch != mustMatchAllComponents)
                {
                    break;
                }
            }

            return doesMatch;
        }

        bool EditorEntitySearchComponent::ArePropertyValuesOfComponentMatched(
            AZ::EntityId entityId,
            AZ::Uuid componentTypeId,
            const EntitySearchFilter::ComponentProperties& componentProperties,
            bool mustMatchAllComponents) const
        {
            EditorComponentAPIRequests::GetComponentsOutcome getComponentsOutcome;
            EditorComponentAPIBus::BroadcastResult(getComponentsOutcome, &EditorComponentAPIRequests::GetComponentsOfType, entityId, componentTypeId);
            const AZStd::vector<AZ::EntityComponentIdPair>& componentInstances = getComponentsOutcome.GetValueOr(AZStd::vector<AZ::EntityComponentIdPair>());

            bool doesMatch = true;

            // If must match all components, return false if one component's property value is not matched.
            // If not must match all components, return true if one component's property value is matched.
            for (auto& componentProperty : componentProperties)
            {
                doesMatch = componentInstances.end() != AZStd::find_if(componentInstances.begin(), componentInstances.end(),
                    [&componentProperty](auto& componentInstance)
                    {
                        bool propertyMatched = false;
                        EditorComponentAPIBus::BroadcastResult(propertyMatched, &EditorComponentAPIRequests::CompareComponentProperty,
                            componentInstance, componentProperty.first.c_str(), componentProperty.second);
                        return propertyMatched;
                    }
                );

                if (doesMatch != mustMatchAllComponents)
                {
                    break;
                }
            }

            return doesMatch;            
        }

    } // Components
} // AzToolsFramework
