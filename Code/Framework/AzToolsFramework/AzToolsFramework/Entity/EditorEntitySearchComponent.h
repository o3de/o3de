/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/Component.h>
#include <AzToolsFramework/Entity/EditorEntitySearchBus.h>

namespace AzToolsFramework
{
    namespace Components
    {
        //! A System Component to reflect Editor operations on Components to Behavior Context
        class EditorEntitySearchComponent final
            : public AZ::Component
            , public EditorEntitySearchBus::Handler
        {
        public:
            AZ_COMPONENT(EditorEntitySearchComponent, "{BD1E6D92-58D5-4364-A3CE-D9BE63C0D9C8}");

            EditorEntitySearchComponent() = default;
            ~EditorEntitySearchComponent() override = default;

            static void Reflect(AZ::ReflectContext* context);

            // Component ...
            void Activate() override;
            void Deactivate() override;

            // EditorEntitySearchBus ...
            EntityIdList SearchEntities(const EntitySearchFilter& filter) override;
            EntityIdList GetRootEditorEntities() override;
            
        private:
            struct SearchConditions
            {
                AZ_TYPE_INFO(EntitySearchFilter, "{B5B6438D-D864-4CC2-83F4-BE18FCAB744B}")

                SearchConditions(const EntitySearchFilter& filter);

                const EntitySearchFilter& m_filter;
                AZStd::vector<AZStd::vector<AZStd::string>> m_tokenizedPaths;
            };

            void GetAllEntitiesInSubtree(AZ::EntityId rootId, EntityIdList& subTreeEntityList);

            void FilterEntity(AZ::EntityId entityId, const SearchConditions& conditions, EntityIdSet& filteredEntities);

            void FollowPathHelper(
                const AZStd::vector<AZ::EntityId>& entities, const AZStd::vector<AZStd::string>& tokenizedPath,
                EntityIdSet* result, bool caseSensitive, size_t pathSize, size_t currentIndex = 0);

            bool IsPositionContained(
                AZ::EntityId entityId,
                const AZ::Aabb& aabb) const;

            bool AreComponentsMatched(
                AZ::EntityId entityId,
                const EntitySearchFilter::Components& components,
                bool mustMatchAllComponents) const;

            bool ArePropertyValuesOfComponentMatched(
                AZ::EntityId entityId,
                AZ::Uuid componentTypeId,
                const EntitySearchFilter::ComponentProperties& componentProperties,
                bool mustMatchAllComponents) const;
        };

    } // Components
} // AzToolsFramework
