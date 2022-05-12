/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/UI/Prefab/Procedural/ProceduralPrefabReadOnlyHandler.h>

#include <AzToolsFramework/Entity/EditorEntityContextBus.h>
#include <AzToolsFramework/Entity/ReadOnly/ReadOnlyEntityInterface.h>
#include <AzToolsFramework/Prefab/PrefabPublicInterface.h>
#include <AzToolsFramework/Prefab/PrefabFocusPublicInterface.h>

namespace AzToolsFramework
{
    namespace Prefab
    {
        ProceduralPrefabReadOnlyHandler::ProceduralPrefabReadOnlyHandler()
        {
            m_prefabPublicInterface = AZ::Interface<PrefabPublicInterface>::Get();
            AZ_Assert(
                m_prefabPublicInterface != nullptr,
                "ProceduralPrefabReadOnlyHandler requires a PrefabPublicInterface instance on Initialize.");

            m_prefabFocusPublicInterface = AZ::Interface<PrefabFocusPublicInterface>::Get();
            AZ_Assert(
                m_prefabFocusPublicInterface != nullptr,
                "ProceduralPrefabReadOnlyHandler requires a PrefabFocusPublicInterface instance on Initialize.");

            AzFramework::EntityContextId editorEntityContextId = AzFramework::EntityContextId::CreateNull();
            EditorEntityContextRequestBus::BroadcastResult(editorEntityContextId, &EditorEntityContextRequests::GetEditorEntityContextId);

            ReadOnlyEntityQueryRequestBus::Handler::BusConnect(editorEntityContextId);

             // Refresh the whole read-only cache
            if (auto readOnlyEntityQueryInterface = AZ::Interface<ReadOnlyEntityQueryInterface>::Get())
            {
                readOnlyEntityQueryInterface->RefreshReadOnlyStateForAllEntities();
            }
        }

        ProceduralPrefabReadOnlyHandler ::~ProceduralPrefabReadOnlyHandler()
        {
            ReadOnlyEntityQueryRequestBus::Handler::BusDisconnect();
        }

        void ProceduralPrefabReadOnlyHandler::IsReadOnly(const AZ::EntityId& entityId, bool& isReadOnly)
        {
            if(m_prefabPublicInterface->IsOwnedByProceduralPrefabInstance(entityId))
            {
                // All entities nested inside a procedural prefabs should always be marked as read-only.
                if (!m_prefabPublicInterface->IsInstanceContainerEntity(entityId))
                {
                    isReadOnly = true;
                }

                // The container entity of a procedural prefab should only be marked as read-only when the prefab is being edited.
                if (m_prefabFocusPublicInterface->IsOwningPrefabBeingFocused(entityId))
                {
                    isReadOnly = true;
                }
            }
        }

    } // namespace Prefab
} // namespace AzToolsFramework
