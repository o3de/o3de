/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/Prefab/Overrides/PrefabOverrideHandler.h>
#include <AzToolsFramework/Prefab/PrefabSystemComponentInterface.h>
#include <AzToolsFramework/Prefab/Undo/PrefabUndoRevertOverrides.h>

 namespace AzToolsFramework
{
    namespace Prefab
    {
        PrefabOverrideHandler::PrefabOverrideHandler()
        {
            m_prefabSystemComponentInterface = AZ::Interface<PrefabSystemComponentInterface>::Get();
            AZ_Assert(m_prefabSystemComponentInterface, "PrefabOverrideHandler - PrefabSystemComponentInterface could not be found.");
        }

        PrefabOverrideHandler::~PrefabOverrideHandler()
        {
            m_prefabSystemComponentInterface = nullptr;
        }

        bool PrefabOverrideHandler::AreOverridesPresent(AZ::Dom::Path path, LinkId linkId) const
        {
            LinkReference link = m_prefabSystemComponentInterface->FindLink(linkId);
            if (link.has_value())
            {
                return link->get().AreOverridesPresent(path);
            }
            return false;
        }

        bool PrefabOverrideHandler::RevertOverrides(AZ::Dom::Path path, LinkId linkId) const
        {
            LinkReference link = m_prefabSystemComponentInterface->FindLink(linkId);
            if (link.has_value())
            {
                auto subTree = link->get().RemoveOverrides(path);
                if (subTree.IsEmpty())
                {
                    AZ_Warning("Prefab", false, "PrefabOverrideHandler::RevertOverrides is called on a path that has no overrides.");
                    return false;
                }

                ScopedUndoBatch undoBatch("Revert Prefab Overrides");
                PrefabUndoRevertOverrides* state = new Prefab::PrefabUndoRevertOverrides("Capture Override SubTree");
                state->Capture(path, AZStd::move(subTree), linkId);
                state->SetParent(undoBatch.GetUndoBatch());

                link->get().UpdateTarget();
                m_prefabSystemComponentInterface->SetTemplateDirtyFlag(link->get().GetTargetTemplateId(), true);
                m_prefabSystemComponentInterface->PropagateTemplateChanges(link->get().GetTargetTemplateId());
                return true;
            }
            return false;
        }
    }
} // namespace AzToolsFramework
