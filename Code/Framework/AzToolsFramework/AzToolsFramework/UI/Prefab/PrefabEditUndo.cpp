/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Interface/Interface.h>

#include <AzToolsFramework/Prefab/Instance/Instance.h>
#include <AzToolsFramework/UI/Prefab/PrefabEditInterface.h>
#include <AzToolsFramework/UI/Prefab/PrefabEditUndo.h>

namespace AzToolsFramework
{
    namespace Prefab
    {
        PrefabUndoEdit::PrefabUndoEdit(const AZStd::string& undoOperationName)
            : UndoSystem::URSequencePoint(undoOperationName)
            , m_changed(true)
        {
            m_prefabEditInterface = AZ::Interface<PrefabEditInterface>::Get();
            AZ_Assert(m_prefabEditInterface, "PrefabUndoEdit - Failed to grab prefab edit interface");
        }

        void PrefabUndoEdit::Capture(AZ::EntityId oldContainerEntityId, AZ::EntityId newContainerEntityId)
        {
            m_oldContainerEntityId = oldContainerEntityId;
            m_newContainerEntityId = newContainerEntityId;
        }

        void PrefabUndoEdit::Undo()
        {
            m_prefabEditInterface->EditPrefab(m_oldContainerEntityId);
        }

        void PrefabUndoEdit::Redo()
        {
            m_prefabEditInterface->EditPrefab(m_newContainerEntityId);
        }

    } // namespace Prefab
} // namespace AzToolsFramework
