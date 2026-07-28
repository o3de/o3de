/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/Prefab/PrefabFocusUndo.h>

#include <AzCore/Interface/Interface.h>
#include <AzToolsFramework/Prefab/PrefabFocusInterface.h>

namespace AzToolsFramework::Prefab
{
    PrefabFocusUndo::PrefabFocusUndo(const AZStd::string& undoOperationName)
        : UndoSystem::URSequencePoint(undoOperationName)
    {
        m_prefabFocusInterface = AZ::Interface<PrefabFocusInterface>::Get();
        AZ_Assert(m_prefabFocusInterface, "PrefabFocusUndo - Failed to grab prefab focus interface");
    }

    bool PrefabFocusUndo::Changed() const
    {
        return true;
    }

    void PrefabFocusUndo::Capture(AZ::EntityId entityId)
    {
        // The focus being replaced is the one of the world the entity lives in, not the one of the world being viewed.
        const InstanceOptionalReference focusedInstance = m_prefabFocusInterface->GetFocusedPrefabInstanceForEntity(entityId);

        m_beforeEntityId = focusedInstance.has_value() ? focusedInstance->get().GetContainerEntityId() : AZ::EntityId();
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
