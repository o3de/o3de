/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/Application/EditorEntityManager.h>

#include <AzToolsFramework/API/ToolsApplicationAPI.h>

namespace AzToolsFramework
{
    void EditorEntityManager::Start()
    {
        m_prefabPublicInterface = AZ::Interface<Prefab::PrefabPublicInterface>::Get();
        AZ_Assert(m_prefabPublicInterface, "EditorEntityManager - Could not retrieve instance of PrefabPublicInterface");

        AZ::Interface<EditorEntityAPI>::Register(this);
    }

    EditorEntityManager::~EditorEntityManager()
    {
        // Attempting to Unregister if we never registerd (e.g. if Start() was never called) throws an error, so check that first
        EditorEntityAPI* editorEntityInterface = AZ::Interface<EditorEntityAPI>::Get();
        if (editorEntityInterface == this)
        {
            AZ::Interface<EditorEntityAPI>::Unregister(this);
        }
    }

    void EditorEntityManager::DeleteSelected()
    {
        EntityIdList selectedEntities;
        ToolsApplicationRequestBus::BroadcastResult(selectedEntities, &ToolsApplicationRequests::GetSelectedEntities);
        m_prefabPublicInterface->DeleteEntitiesInInstance(selectedEntities);
    }

    void EditorEntityManager::DeleteEntityById(AZ::EntityId entityId)
    {
        DeleteEntities(EntityIdList{ entityId });
    }

    void EditorEntityManager::DeleteEntities(const EntityIdList& entities)
    {
        m_prefabPublicInterface->DeleteEntitiesInInstance(entities);
    }

    void EditorEntityManager::DeleteEntityAndAllDescendants(AZ::EntityId entityId)
    {
        DeleteEntitiesAndAllDescendants(EntityIdList{ entityId });
    }

    void EditorEntityManager::DeleteEntitiesAndAllDescendants(const EntityIdList& entities)
    {
        m_prefabPublicInterface->DeleteEntitiesAndAllDescendantsInInstance(entities);
    }

    void EditorEntityManager::DuplicateSelected()
    {
        EntityIdList selectedEntities;
        ToolsApplicationRequestBus::BroadcastResult(selectedEntities, &ToolsApplicationRequests::GetSelectedEntities);

        m_prefabPublicInterface->DuplicateEntitiesInInstance(selectedEntities);
    }

    void EditorEntityManager::DuplicateEntityById(AZ::EntityId entityId)
    {
        DuplicateEntities(EntityIdList{ entityId });
    }

    void EditorEntityManager::DuplicateEntities(const EntityIdList& entities)
    {
        m_prefabPublicInterface->DuplicateEntitiesInInstance(entities);
    }
}

