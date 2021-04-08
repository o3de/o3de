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

#pragma once

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
    };

} // namespace AzToolsFramework

