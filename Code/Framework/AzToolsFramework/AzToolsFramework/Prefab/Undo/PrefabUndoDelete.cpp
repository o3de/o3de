/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Interface/Interface.h>
#include <AzToolsFramework/Prefab/Instance/InstanceDomGeneratorInterface.h>
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
        PrefabUndoDeleteEntity::PrefabUndoDeleteEntity(const AZStd::string& undoOperationName)
            : PrefabUndoBase(undoOperationName)
        {
        }

        void PrefabUndoDeleteEntity::Capture(
            const AZStd::vector<AZStd::string>& entityAliasPathList,
            const AZStd::vector<const AZ::Entity*> parentEntityList,
            Instance& focusedInstance)
        {
            m_templateId = focusedInstance.GetTemplateId();

            PrefabDom& focusedTempalteDom = m_prefabSystemComponentInterface->FindTemplateDom(m_templateId);

            PrefabDomReference cachedOwningInstanceDom = focusedInstance.GetCachedInstanceDom();

            // Removes entity DOMs.
            for (const AZStd::string& entityAliasPath : entityAliasPathList)
            {
                if (entityAliasPath.empty())
                {
                    AZ_Error("Prefab", false, "PrefabUndoDeleteEntity::Capture - Entity alias path is empty. Skips removal.");
                    continue;
                }

                PrefabUndoUtils::AppendRemovePatch(m_redoPatch, entityAliasPath);

                PrefabDomPath entityDomPathInFocusedTemplate(entityAliasPath.c_str());
                const PrefabDomValue* entityDomInFocusedTemplate = entityDomPathInFocusedTemplate.Get(focusedTempalteDom);
                PrefabUndoUtils::AppendAddEntityPatch(m_undoPatch, *entityDomInFocusedTemplate, entityAliasPath);

                // Preemptively updates the cached DOM to prevent reloading instance.
                if (cachedOwningInstanceDom.has_value())
                {
                    PrefabUndoUtils::RemoveValueInPrefabDom(cachedOwningInstanceDom, entityAliasPath);
                }
            }

            // Updates parent entity DOMs.
            // For each parent entity, it includes one or more than one patches to update EditorEntitySortComponent.
            for (const AZ::Entity* parentEntity : parentEntityList)
            {
                if (!parentEntity)
                {
                    AZ_Error("Prefab", false, "PrefabUndoDelete::Capture - Parent entity cannot be nullptr.");
                    continue;
                }

                // This is actually a from-focused path because focused instance is the owning instance in the current workflow.
                const AZStd::string parentEntityAliasPath = m_instanceToTemplateInterface->GenerateEntityAliasPath(parentEntity->GetId());

                PrefabDom parentEntityDomAfterRemovingChildren;
                m_instanceToTemplateInterface->GenerateEntityDomBySerializing(parentEntityDomAfterRemovingChildren, *parentEntity);

                // If the parent is the focused container entity, we need to fetch from root template rather than
                // retrieving from the focused template. The focused containter entity DOM in focused template does
                // not contain transform data seen from root, but after-state entity DOM from serialization always
                // contain the transform data, which may generate unexpected patches to update transform.
                if (parentEntity->GetId() == focusedInstance.GetContainerEntityId())
                {
                    PrefabDom parentEntityDomBeforeRemoving;
                    m_instanceDomGeneratorInterface->GetEntityDomFromTemplate(parentEntityDomBeforeRemoving, *parentEntity);

                    if (parentEntityDomBeforeRemoving.IsNull())
                    {
                        AZ_Error("Prefab", false, "PrefabUndoDeleteEntity::Capture - "
                            "Cannot retrieve parent container entity DOM from root template via instance DOM generator.");
                        continue;
                    }

                    PrefabUndoUtils::GenerateAndAppendPatch(
                        m_redoPatch, parentEntityDomBeforeRemoving, parentEntityDomAfterRemovingChildren, parentEntityAliasPath);
                    PrefabUndoUtils::GenerateAndAppendPatch(
                        m_undoPatch, parentEntityDomAfterRemovingChildren, parentEntityDomBeforeRemoving, parentEntityAliasPath);
                }
                else
                {
                    PrefabDomPath parentEntityAliasDomPathFromFocused(parentEntityAliasPath.c_str());

                    // This scope is added to limit usage and ensure DOM is not modified when it is being used.
                    {
                        // DOM value pointers can't be relied upon if the original DOM gets modified after pointer creation.
                        const PrefabDomValue* parentEntityDomInFocusedTemplate =
                            parentEntityAliasDomPathFromFocused.Get(focusedTempalteDom);

                        if (!parentEntityDomInFocusedTemplate)
                        {
                            AZ_Error("Prefab", false, "PrefabUndoDeleteEntity::Capture - "
                                "Cannot retrieve parent entity DOM from focused template.");
                            continue;
                        }

                        PrefabUndoUtils::GenerateAndAppendPatch(
                            m_redoPatch, *parentEntityDomInFocusedTemplate, parentEntityDomAfterRemovingChildren, parentEntityAliasPath);
                        PrefabUndoUtils::GenerateAndAppendPatch(
                            m_undoPatch, parentEntityDomAfterRemovingChildren, *parentEntityDomInFocusedTemplate, parentEntityAliasPath);
                    }
                }

                // Preemptively updates the cached DOM to prevent reloading instance.
                if (cachedOwningInstanceDom.has_value())
                {
                    PrefabUndoUtils::UpdateEntityInPrefabDom(
                        cachedOwningInstanceDom->get(), parentEntityDomAfterRemovingChildren, parentEntityAliasPath);
                }
            }
        }
    } // namespace Prefab
} // namespace AzToolsFramework
