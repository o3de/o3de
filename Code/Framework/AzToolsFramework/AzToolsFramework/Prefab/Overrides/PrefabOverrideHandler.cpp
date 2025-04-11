/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/Prefab/Overrides/PrefabOverrideHandler.h>

#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/Prefab/PrefabDomUtils.h>
#include <AzToolsFramework/Prefab/PrefabSystemComponentInterface.h>
#include <AzToolsFramework/Prefab/Undo/PrefabUndoApplyOverrides.h>
#include <AzToolsFramework/Prefab/Undo/PrefabUndoComponentPropertyEdit.h>
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

        bool PrefabOverrideHandler::RevertOverrides(AZ::Dom::Path path, LinkId linkId) const
        {
            LinkReference link = m_prefabSystemComponentInterface->FindLink(linkId);
            if (!link.has_value())
            {
                return false;
            }

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

        bool PrefabOverrideHandler::PushOverridesToPrefab(
            const AZ::Dom::Path& path,
            AZStd::string_view relativePath,
            InstanceOptionalReference targetInstance
        ) const
        {
            // Retrieve the link between the instance holding the overrides (focused) and the one receiving them (targetInstance).
            LinkId linkId = targetInstance->get().GetLinkId();
            LinkReference link = m_prefabSystemComponentInterface->FindLink(linkId);

            // Start an undo batch.
            ScopedUndoBatch undoBatch("Apply Overrides to Prefab");

            // Remove Overrides in the link.
            PrefabOverridePrefixTree subTree = link->get().RemoveOverrides(path);
            if (subTree.IsEmpty())
            {
                AZ_Warning("Prefab", false, "PrefabOverrideHandler::PushOverrides is called on a path that has no overrides.");
                return false;
            }

            // Visit all nodes to apply patches as edits one by one, with the correct path.
            subTree.VisitPath(
                AZ::Dom::Path(""),
                [&undoBatch, &targetInstance, relativePath](
                    [[maybe_unused]] const AZ::Dom::Path& path, [[maybe_unused]] Link::PrefabOverrideMetadata& metaData) -> bool
                {
                    const auto& overridePath = metaData.m_patch.FindMember("path")->value;

                    AZStd::string overridePathStr = overridePath.GetString();
                    if (overridePathStr.starts_with(relativePath))
                    {
                        overridePathStr = overridePathStr.substr(relativePath.length());
                    }

                    // Apply individual overrides to target prefab template.
                    PrefabUndoComponentPropertyEdit* state = aznew PrefabUndoComponentPropertyEdit("Apply Override");
                    state->SetParent(undoBatch.GetUndoBatch());
                    state->Capture(targetInstance->get(), AZ::Dom::Path(overridePathStr).ToString(), metaData.m_patch.FindMember("value")->value);
                    state->Redo();

                    return true;
                },
                AZ::Dom::PrefixTreeTraversalFlags::None
            );

            // Create node for overrides reversal so that they get restored on undo.
            PrefabUndoRevertOverrides* state = new Prefab::PrefabUndoRevertOverrides("Capture Override SubTree");
            state->Capture(path, AZStd::move(subTree), linkId);
            state->SetParent(undoBatch.GetUndoBatch());

            // Queue a refresh of the property display.
            AzToolsFramework::ToolsApplicationEvents::Bus::Broadcast(
                &AzToolsFramework::ToolsApplicationEvents::Bus::Events::InvalidatePropertyDisplay,
                AzToolsFramework::Refresh_AttributesAndValues);

            return true;
        }

        bool PrefabOverrideHandler::PushOverridesToLink(
            const AZ::Dom::Path& path,
            AZStd::string_view relativePath,
            LinkId sourceLinkId,
            LinkId targetLinkId
        ) const
        {
            // Retrieve link.
            LinkReference sourceLink = m_prefabSystemComponentInterface->FindLink(sourceLinkId);
            LinkReference targetLink = m_prefabSystemComponentInterface->FindLink(targetLinkId);

            // Remove relativePath from pathStr so that the patches from sourceLink can be applied to targetLink.
            AZStd::string pathStr = path.ToString();
            if (pathStr.starts_with(relativePath))
            {
                pathStr = pathStr.substr(relativePath.length());
            }

            // Start an undo batch.
            ScopedUndoBatch undoBatch("Apply Overrides to Prefab");

            // Remove Overrides in sourceLink.
            PrefabOverridePrefixTree subTree = sourceLink->get().RemoveOverrides(path);
            if (subTree.IsEmpty())
            {
                AZ_Warning("Prefab", false, "PrefabOverrideHandler::PushOverrides is called on a path that has no overrides.");
                return false;
            }

            // Create a new overrides PrefabOverridePrefixTree that will be altered and passed to the target link.
            PrefabOverridePrefixTree newOverrides;

            subTree.VisitPath(
                AZ::Dom::Path(""),
                [&newOverrides, relativePath](
                    const AZ::Dom::Path& path, Link::PrefabOverrideMetadata& metaData) -> bool
                {
                    // Patches in subTree were retrieved from the source link, so they will contain a path property
                    // with the relative path from source link to the property being overridden.
                    // Given that we now want to move these to the target path, we should alter the path so that it is
                    // pointing from target link to the property being overridden.
                    // Since the target link is always a descendant of the source link, we are able to do this
                    // by simply removing the relative path from source to target link from the beginning of the path.

                    // Copy the patch so that subTree is untouched and can be used for undo/redo.
                    PrefabDom newPatch;
                    newPatch.CopyFrom(metaData.m_patch, newPatch.GetAllocator());

                    auto& overridePath = newPatch.FindMember("path")->value;
                    AZStd::string overridePathStr = overridePath.GetString();
                    if (overridePathStr.starts_with(relativePath))
                    {
                        overridePathStr = overridePathStr.substr(relativePath.length());
                    }

                    overridePath.SetString(
                        overridePathStr.c_str(),
                        static_cast<rapidjson::SizeType>(overridePathStr.length()),
                        newPatch.GetAllocator()
                    );

                    // We copy the paths to newOverrides, which is what will be added to the target link.
                    newOverrides.SetValue<Link::PrefabOverrideMetadata>(
                        path,
                        Link::PrefabOverrideMetadata(
                            AZStd::move(newPatch),
                            metaData.m_patchIndex
                        )
                    );

                    return true;
                },
                AZ::Dom::PrefixTreeTraversalFlags::None
            );

            // Create node for overrides reversal on source link so that they get restored on undo.
            PrefabUndoRevertOverrides* sourceState = new Prefab::PrefabUndoRevertOverrides("Capture Override SubTree");
            sourceState->Capture(path, AZStd::move(subTree), sourceLinkId);
            sourceState->SetParent(undoBatch.GetUndoBatch());

            // Create node for overrides addition on target link so that they get removed on undo.
            PrefabUndoApplyOverrides* targetState = new Prefab::PrefabUndoApplyOverrides("Apply Overrides");
            targetState->Capture(path, AZStd::move(newOverrides), targetLinkId);
            targetState->SetParent(undoBatch.GetUndoBatch());

            // Actually apply the overrides.
            targetState->Redo();

            // Correctly update both links.
            sourceLink->get().UpdateTarget();
            m_prefabSystemComponentInterface->SetTemplateDirtyFlag(sourceLink->get().GetTargetTemplateId(), true);
            m_prefabSystemComponentInterface->PropagateTemplateChanges(sourceLink->get().GetTargetTemplateId());

            targetLink->get().UpdateTarget();
            m_prefabSystemComponentInterface->SetTemplateDirtyFlag(targetLink->get().GetTargetTemplateId(), true);
            m_prefabSystemComponentInterface->PropagateTemplateChanges(targetLink->get().GetTargetTemplateId());

            // Queue a refresh of the property display in case the overrides contain only default values.
            // Otherwise, when overrides don't trigger changes in the template dom, the entity won't be
            // recreated on propagation and the inspector won't be updated to reflect override icon changes
            AzToolsFramework::ToolsApplicationEvents::Bus::Broadcast(
                &AzToolsFramework::ToolsApplicationEvents::Bus::Events::InvalidatePropertyDisplay,
                AzToolsFramework::Refresh_AttributesAndValues);

            return true;
        }

    } // namespace Prefab
} // namespace AzToolsFramework
