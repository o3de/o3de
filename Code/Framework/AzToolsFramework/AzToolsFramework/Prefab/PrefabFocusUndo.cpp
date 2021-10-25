/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/Prefab/PrefabFocusUndo.h>

#include <AzCore/Interface/Interface.h>
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>
#include <AzToolsFramework/Prefab/PrefabFocusInterface.h>
#include <AzToolsFramework/Prefab/PrefabFocusPublicInterface.h>

namespace AzToolsFramework::Prefab
{
    PrefabFocusUndo::PrefabFocusUndo(const AZStd::string& undoOperationName)
        : UndoSystem::URSequencePoint(undoOperationName)
    {
        m_prefabFocusInterface = AZ::Interface<PrefabFocusInterface>::Get();
        AZ_Assert(m_prefabFocusInterface, "PrefabFocusUndo - Failed to grab prefab focus interface");

        m_prefabFocusPublicInterface = AZ::Interface<PrefabFocusPublicInterface>::Get();
        AZ_Assert(m_prefabFocusPublicInterface, "PrefabFocusUndo - Failed to grab prefab focus public interface");
    }

    bool PrefabFocusUndo::Changed() const
    {
        return true;
    }

    void PrefabFocusUndo::Capture(AZ::EntityId entityId)
    {
        auto entityContextId = AzFramework::EntityContextId::CreateNull();
        EditorEntityContextRequestBus::BroadcastResult(entityContextId, &EditorEntityContextRequests::GetEditorEntityContextId);

        m_beforeEntityId = m_prefabFocusPublicInterface->GetFocusedPrefabContainerEntityId(entityContextId);
        m_afterEntityId = entityId;
    }

    void PrefabFocusUndo::Undo()
    {
        m_prefabFocusInterface->FocusOnPrefabInstanceOwningEntityId(m_beforeEntityId);
    }

    void PrefabFocusUndo::Redo()
    {
        m_prefabFocusInterface->FocusOnPrefabInstanceOwningEntityId(m_afterEntityId);
    }

} // namespace AzToolsFramework::Prefab
