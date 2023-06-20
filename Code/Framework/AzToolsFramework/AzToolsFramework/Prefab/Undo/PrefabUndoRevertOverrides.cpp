/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <AzToolsFramework/Prefab/PrefabSystemComponentInterface.h>
#include <AzToolsFramework/Prefab/Undo/PrefabUndoRevertOverrides.h>

namespace AzToolsFramework
{
    namespace Prefab
    {
        PrefabUndoRevertOverrides::PrefabUndoRevertOverrides(const AZStd::string& undoOperationName)
            : UndoSystem::URSequencePoint(undoOperationName)
            , m_changed(true)
        {
            m_prefabSystemComponentInterface = AZ::Interface<PrefabSystemComponentInterface>::Get();
            AZ_Assert(m_prefabSystemComponentInterface, "PrefabUndoRevertOverrides - Failed to grab prefab system component interface.");
        }

        bool PrefabUndoRevertOverrides::Changed() const
        {
            return m_changed;
        }

        void PrefabUndoRevertOverrides::Capture(
            const AZ::Dom::Path& pathToSubTree, PrefabOverridePrefixTree&& overrideSubTree, LinkId linkId)
        {
            m_pathToSubTree = pathToSubTree;
            m_overrideSubTree = AZStd::move(overrideSubTree);
            m_linkId = linkId;
        }

        void PrefabUndoRevertOverrides::Undo()
        {
            LinkReference link = m_prefabSystemComponentInterface->FindLink(m_linkId);
            if (link.has_value())
            {
                // This will move the sub-tree stored in the undo node and make it empty. It will be populated again on redo.
                link->get().AddOverrides(m_pathToSubTree, AZStd::move(m_overrideSubTree));
                link->get().UpdateTarget();
                m_prefabSystemComponentInterface->SetTemplateDirtyFlag(link->get().GetTargetTemplateId(), true);
                m_prefabSystemComponentInterface->PropagateTemplateChanges(link->get().GetTargetTemplateId());
            }
        }

        void PrefabUndoRevertOverrides::Redo()
        {
            LinkReference link = m_prefabSystemComponentInterface->FindLink(m_linkId);
            if (link.has_value())
            {
                m_overrideSubTree = AZStd::move(link->get().RemoveOverrides(m_pathToSubTree));
                link->get().UpdateTarget();
                m_prefabSystemComponentInterface->SetTemplateDirtyFlag(link->get().GetTargetTemplateId(), true);
                m_prefabSystemComponentInterface->PropagateTemplateChanges(link->get().GetTargetTemplateId());
            }
        }
    } // namespace Prefab
} // namespace AzToolsFramework
