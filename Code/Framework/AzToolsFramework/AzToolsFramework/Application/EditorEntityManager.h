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

#include <AzToolsFramework/API/EditorEntityAPI.h>
#include <AzToolsFramework/Prefab/PrefabPublicInterface.h>

namespace AzToolsFramework
{
    class EditorEntityManager
        : public EditorEntityAPI
    {
    public:
        ~EditorEntityManager();

        void Start();

        // EditorEntityAPI...
        void DeleteSelected() override;
        void DeleteEntityById(AZ::EntityId entityId) override;
        void DeleteEntities(const EntityIdList& entities) override;
        void DeleteEntityAndAllDescendants(AZ::EntityId entityId) override;
        void DeleteEntitiesAndAllDescendants(const EntityIdList& entities) override;

    private:
        Prefab::PrefabPublicInterface* m_prefabPublicInterface = nullptr;
    };

}
