/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Interface/Interface.h>
#include <AzToolsFramework/Prefab/Instance/InstanceToTemplateInterface.h>
#include <AzToolsFramework/Prefab/PrefabDomUtils.h>
#include <AzToolsFramework/Prefab/PrefabSystemComponentInterface.h>
#include <AzToolsFramework/Prefab/Undo/PrefabUndo.h>
#include <AzToolsFramework/Prefab/Undo/PrefabUndoDelete.h>
#include <AzToolsFramework/Prefab/Undo/PrefabUndoUtils.h>

namespace AzToolsFramework
{
    namespace Prefab
    {
        PrefabUndoDelete::PrefabUndoDelete(const AZStd::string& undoOperationName)
            : PrefabUndoBase(undoOperationName)
        {
        }

        void PrefabUndoDelete::Capture(
            const AZStd::vector<AZStd::string>& entityAliasPathList,
            const AZStd::vector<const Instance*>& nestedInstanceList,
            const AZStd::vector<const AZ::Entity*> parentEntityList,
            Instance& focusedInstance)
        {
            m_templateId = focusedInstance.GetTemplateId();
            m_redoPatch.SetArray();
            m_undoPatch.SetArray();

            PrefabDom& focusedTempalteDom = m_prefabSystemComponentInterface->FindTemplateDom(m_templateId);

            PrefabDomReference cachedOwningInstanceDom = focusedInstance.GetCachedInstanceDom();

            // Removes entity DOMs.
            for (const AZStd::string& entityAliasPath : entityAliasPathList)
            {
                if (entityAliasPath.empty())
                {
                    AZ_Error("Prefab", false, "PrefabUndoDelete::Capture - Entity alias path is empty. Skips removal.");
                    continue;
                }

                PrefabUndoUtils::AppendRemovePatch(m_redoPatch, entityAliasPath);

                PrefabDomPath entityDomPathInFocusedTemplate(entityAliasPath.c_str());
                const PrefabDomValue* entityDomInFocusedTemplate = entityDomPathInFocusedTemplate.Get(focusedTempalteDom);
                PrefabUndoUtils::AppendAddEntityPatch(m_undoPatch, *entityDomInFocusedTemplate, entityAliasPath);

                // Preemptively updates the cached DOM to prevent reloading instance.
                if (cachedOwningInstanceDom.has_value())
                {
                    PrefabUndoUtils::UpdateCachedOwningInstanceDomWithRemoval(cachedOwningInstanceDom, entityAliasPath);
                }
            }

            // Removes instance DOMs and links.
            for (const Instance* nestedInstance : nestedInstanceList)
            {
                LinkReference nestedInstanceLink = m_prefabSystemComponentInterface->FindLink(nestedInstance->GetLinkId());
                if (!nestedInstanceLink.has_value())
                {
                    AZ_Error("Prefab", false, "PrefabUndoDelete::Capture - Nested instance link could not be found.");
                    continue;
                }

                PrefabDom patchesCopyForUndoSupport;
                PrefabDomReference nestedInstanceLinkDom = nestedInstanceLink->get().GetLinkDom();
                if (nestedInstanceLinkDom.has_value())
                {
                    PrefabDomValueReference nestedInstanceLinkPatches =
                        PrefabDomUtils::FindPrefabDomValue(nestedInstanceLinkDom->get(), PrefabDomUtils::PatchesName);
                    if (nestedInstanceLinkPatches.has_value())
                    {
                        patchesCopyForUndoSupport.CopyFrom(nestedInstanceLinkPatches->get(), patchesCopyForUndoSupport.GetAllocator());
                    }
                }

                PrefabUndoInstanceLink* removeLinkUndo = aznew PrefabUndoInstanceLink("Remove Link");
                removeLinkUndo->Capture(
                    focusedInstance.GetTemplateId(),
                    nestedInstance->GetTemplateId(),
                    nestedInstance->GetInstanceAlias(),
                    AZStd::move(patchesCopyForUndoSupport),
                    nestedInstance->GetLinkId());
                removeLinkUndo->SetParent(GetParent());
                removeLinkUndo->Redo();
            }

            // Updates parent entity DOMs.
            for (const AZ::Entity* parentEntity : parentEntityList)
            {
                // This is actually a from-focused path because focused instance is the owning instance in the current workflow.
                const AZStd::string parentEntityAliasPath = m_instanceToTemplateInterface->GenerateEntityAliasPath(parentEntity->GetId());
                PrefabDomPath parentEntityAliasDomPathFromFocused(parentEntityAliasPath.c_str());

                // This scope is added to limit usage and ensure DOM is not modified when it is being used.
                {
                    // DOM value pointers can't be relied upon if the original DOM gets modified after pointer creation.
                    const PrefabDomValue* parentEntityDomInFocusedTemplate = parentEntityAliasDomPathFromFocused.Get(focusedTempalteDom);
                    if (!parentEntityDomInFocusedTemplate)
                    {
                        AZ_Error("Prefab", false, "PrefabUndoDelete::Capture - Cannot retrieve parent entity DOM from focused template.");
                        continue;
                    }

                    PrefabDom parentEntityDomAfterRemovingChildren;
                    m_instanceToTemplateInterface->GenerateDomForEntity(parentEntityDomAfterRemovingChildren, *parentEntity);

                    PrefabUndoUtils::AppendUpdateEntityPatch(
                        m_redoPatch, *parentEntityDomInFocusedTemplate, parentEntityDomAfterRemovingChildren, parentEntityAliasPath);
                    PrefabUndoUtils::AppendUpdateEntityPatch(
                        m_undoPatch, parentEntityDomAfterRemovingChildren, *parentEntityDomInFocusedTemplate, parentEntityAliasPath);

                    // Preemptively updates the cached DOM to prevent reloading instance.
                    if (cachedOwningInstanceDom.has_value())
                    {
                        PrefabUndoUtils::UpdateCachedOwningInstanceDom(
                            cachedOwningInstanceDom->get(), parentEntityDomAfterRemovingChildren, parentEntityAliasPath);
                    }
                }
            }
        }
    } // namespace Prefab
} // namespace AzToolsFramework
