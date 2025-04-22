/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/std/containers/vector.h>

#include <AzToolsFramework/Application/ToolsApplication.h>

namespace AzToolsFramework
{
    //! Class storing the match conditions for an editor entity search
    class EntitySearchFilter
    {
    public:
        AZ_TYPE_INFO(EntitySearchFilter, "{48A94382-72BE-457B-BB43-0E6C245824D2}")

        //! List of names (matches if any match); can contain wildcards in the name.
        AZStd::vector<AZStd::string> m_names;

        //! Determines if the name matching should be case sensitive.
        bool m_namesCaseSensitive = false;

        using ComponentProperties = AZStd::unordered_map<AZStd::string, AZStd::any>;
        using ComponentProperty = AZStd::pair<AZStd::string, AZStd::any>;
        using Components = AZStd::unordered_map<AZ::Uuid, ComponentProperties>;

        //! Map of component type ids with values as their properties if needed.
        //! Matches if any component's property values match. If no property value given, matches if any component types match.
        Components m_components;

        //! Determines if the filter should match all component type ids (AND).
        bool m_mustMatchAllComponents = false;

        //! Specifies the entities that act as roots of the search.
        AZStd::vector<AZ::EntityId> m_roots;

        //! Determines if the names are relative to the root or should be searched in children too.
        bool m_namesAreRootBased = false;

        //! Determines if entities' position is inside the given valid AABB. 
        AZ::Aabb m_aabb = AZ::Aabb::CreateNull();
    };

    //! Provides an API to search editor entity that match some conditions in the currently open level.
    class EditorEntitySearchRequests : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;

        //! Iterates through all entities in the current level, and returns a list of the ones that match the conditions.
        virtual EntityIdList SearchEntities(const EntitySearchFilter& conditions) = 0;

        //! Returns a list of all editor entities at the root level in the current level.
        virtual EntityIdList GetRootEditorEntities() = 0;

    protected:
        ~EditorEntitySearchRequests() = default;
    };
    using EditorEntitySearchBus = AZ::EBus<EditorEntitySearchRequests>;

} // namespace AzToolsFramework
