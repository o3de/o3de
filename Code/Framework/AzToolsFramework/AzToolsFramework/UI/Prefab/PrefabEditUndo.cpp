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
            m_prefabEditInterface->EditOwningPrefab(m_oldContainerEntityId);
        }

        void PrefabUndoEdit::Redo()
        {
            m_prefabEditInterface->EditOwningPrefab(m_newContainerEntityId);
        }



    } // namespace Prefab
} // namespace AzToolsFramework
