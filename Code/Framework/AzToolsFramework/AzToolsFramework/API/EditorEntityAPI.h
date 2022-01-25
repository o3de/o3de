/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/EntityId.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace AzToolsFramework
{
    using EntityIdList = AZStd::vector<AZ::EntityId>;

    /*!
    * EditorEntityAPI
    * Handles basic Entity operations
    */
    class EditorEntityAPI
    {
    public:
        AZ_RTTI(EditorEntityAPI, "{3E217E21-046F-462E-8FA2-1347FBDDFDE7}");

        /**
         * Delete all currently-selected entities.
         */
        virtual void DeleteSelected() = 0;

        /**
        * Deletes the specified entity.
        */
        virtual void DeleteEntityById(AZ::EntityId entityId) = 0;

        /**
         * Deletes all specified entities.
         */
        virtual void DeleteEntities(const EntityIdList& entities) = 0;

        /**
        * Deletes the specified entity, as well as any transform descendants.
        */
        virtual void DeleteEntityAndAllDescendants(AZ::EntityId entityId) = 0;

        /**
        * Deletes all entities in the provided list, as well as their transform descendants.
        */
        virtual void DeleteEntitiesAndAllDescendants(const EntityIdList& entities) = 0;

        /**
          * Duplicate all currently-selected entities.
          */
        virtual void DuplicateSelected() = 0;

        /**
         * Duplicates the specified entity.
         */
        virtual void DuplicateEntityById(AZ::EntityId entityId) = 0;

        /**
         * Duplicates all specified entities.
         */
        virtual void DuplicateEntities(const EntityIdList& entities) = 0;
    };

} // namespace AzToolsFramework

