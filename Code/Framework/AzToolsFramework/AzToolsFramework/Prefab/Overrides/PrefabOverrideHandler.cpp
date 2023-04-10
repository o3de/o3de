/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/Prefab/Overrides/PrefabOverrideHandler.h>
#include <AzToolsFramework/Prefab/PrefabDomUtils.h>
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

                // Queue a refresh of the property display in case the overrides contain only default values.
                // Otherwise, when overrides don't trigger changes in the template dom, the entity won't be
                // recreated on propagation and the inspector won't be updated to reflect override icon changes
                AzToolsFramework::ToolsApplicationEvents::Bus::Broadcast(
                    &AzToolsFramework::ToolsApplicationEvents::Bus::Events::InvalidatePropertyDisplay,
                    AzToolsFramework::Refresh_AttributesAndValues);

                return true;
            }
            return false;
        }

        AZStd::optional<PatchType> PrefabOverrideHandler::GetPatchType(AZ::Dom::Path path, LinkId linkId) const
        {
            AZStd::optional<PatchType> patchType = {};

            LinkReference link = m_prefabSystemComponentInterface->FindLink(linkId);
            if (link.has_value())
            {
                // Look for an override at the exact provided path
                if (PrefabDomConstReference overridePatch = link->get().GetOverridePatchAtExactPath(path); overridePatch.has_value())
                {
                    PrefabDomValue::ConstMemberIterator patchEntryIterator = overridePatch->get().FindMember("op");
                    if (patchEntryIterator != overridePatch->get().MemberEnd())
                    {
                        AZStd::string opPath = patchEntryIterator->value.GetString();
                        if (opPath == "remove")
                        {
                            patchType = PatchType::Remove;
                        }
                        else if (opPath == "add")
                        {
                            patchType = PatchType::Add;
                        }
                        else if (opPath == "replace")
                        {
                            patchType = PatchType::Edit;
                        }
                    }
                }
                else if (link->get().AreOverridesPresent(path))
                {
                    // Any overrides on descendant paths are edits
                    patchType = PatchType::Edit;
                }
            }

            return patchType;
        }
    } // namespace Prefab
} // namespace AzToolsFramework
