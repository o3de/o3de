/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/API/EditorEntityAPI.h>
#include <AzToolsFramework/Prefab/PrefabPublicInterface.h>

namespace AzToolsFramework
{
    class EditorEntityManager
        : public EditorEntityAPI
    {
    public:
        virtual ~EditorEntityManager();

        void Start();

        // EditorEntityAPI...
        void DeleteSelected() override;
        void DeleteEntityById(AZ::EntityId entityId) override;
        void DeleteEntities(const EntityIdList& entities) override;
        void DeleteEntityAndAllDescendants(AZ::EntityId entityId) override;
        void DeleteEntitiesAndAllDescendants(const EntityIdList& entities) override;
        void DuplicateSelected() override;
        void DuplicateEntityById(AZ::EntityId entityId) override;
        void DuplicateEntities(const EntityIdList& entities) override;

    private:
        Prefab::PrefabPublicInterface* m_prefabPublicInterface = nullptr;
    };

}
