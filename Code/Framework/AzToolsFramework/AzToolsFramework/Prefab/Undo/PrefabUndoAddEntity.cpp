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
#include <AzToolsFramework/Prefab/Undo/PrefabUndoAddEntity.h>
#include <AzToolsFramework/Prefab/Undo/PrefabUndoUtils.h>

namespace AzToolsFramework
{
    namespace Prefab
    {
        PrefabUndoAddEntity::PrefabUndoAddEntity(const AZStd::string& undoOperationName)
            : PrefabUndoBase(undoOperationName)
        {
        }

        void PrefabUndoAddEntity::Capture(
            const AZ::Entity& parentEntity,
            const AZ::Entity& newEntity,
            Instance& focusedInstance)
        {
            m_templateId = focusedInstance.GetTemplateId();
            const AZ::EntityId parentEntityId = parentEntity.GetId();
            const AZ::EntityId newEntityId = newEntity.GetId();

            const AZStd::string parentEntityAliasPath =
                m_instanceToTemplateInterface->GenerateEntityAliasPath(parentEntityId);
            AZ_Assert(!parentEntityAliasPath.empty(),
                "Alias path of parent entity with id '%llu' shouldn't be empty.",
                static_cast<AZ::u64>(parentEntityId));

            const AZStd::string newEntityAliasPath =
                m_instanceToTemplateInterface->GenerateEntityAliasPath(newEntityId);
            AZ_Assert(!newEntityAliasPath.empty(),
                "Alias path of new entity with id '%llu' shouldn't be empty.",
                static_cast<AZ::u64>(newEntityId));

            PrefabDom parentEntityDomAfterAddingEntity;
            m_instanceToTemplateInterface->GenerateEntityDomBySerializing(parentEntityDomAfterAddingEntity, parentEntity);

            // If the parent is the focused container entity, we need to fetch from root template rather than
            // retrieving from the focused template. The focused containter entity DOM in focused template does
            // not contain transform data seen from root, but after-state entity DOM from serialization always
            // contain the transform data, which may generate unexpected patches to update transform.
            if (parentEntityId == focusedInstance.GetContainerEntityId())
            {
                PrefabDom parentEntityDomBeforeAddingEntity;
                m_instanceDomGeneratorInterface->GetEntityDomFromTemplate(parentEntityDomBeforeAddingEntity, parentEntity);

                if (parentEntityDomBeforeAddingEntity.IsNull())
                {
                    AZ_Error("Prefab", false, "PrefabUndoAddEntity::Capture - "
                        "Cannot retrieve parent container entity DOM from root template via instance DOM generator.");
                }
                else
                {
                    PrefabUndoUtils::GenerateAndAppendPatch(
                        m_redoPatch, parentEntityDomBeforeAddingEntity, parentEntityDomAfterAddingEntity, parentEntityAliasPath);
                    PrefabUndoUtils::GenerateAndAppendPatch(
                        m_undoPatch, parentEntityDomAfterAddingEntity, parentEntityDomBeforeAddingEntity, parentEntityAliasPath);
                }
            }
            else
            {
                PrefabDom& focusedTemplateDom = m_prefabSystemComponentInterface->FindTemplateDom(m_templateId);
                PrefabDomPath entityPathInFocusedTemplate(parentEntityAliasPath.c_str());
                // This scope is added to limit their usage and ensure DOM is not modified when it is being used.
                {
                    // DOM value pointers can't be relied upon if the original DOM gets modified after pointer creation.
                    const PrefabDomValue* parentEntityDomInFocusedTemplate = entityPathInFocusedTemplate.Get(focusedTemplateDom);

                    if (!parentEntityDomInFocusedTemplate)
                    {
                        AZ_Error("Prefab", false, "PrefabUndoAddEntity::Capture - "
                            "Cannot retrieve parent entity DOM from focused template.");
                    }
                    else
                    {
                        PrefabUndoUtils::GenerateAndAppendPatch(
                            m_redoPatch, *parentEntityDomInFocusedTemplate, parentEntityDomAfterAddingEntity, parentEntityAliasPath);
                        PrefabUndoUtils::GenerateAndAppendPatch(
                            m_undoPatch, parentEntityDomAfterAddingEntity, *parentEntityDomInFocusedTemplate, parentEntityAliasPath);
                    }
                }
            }

            PrefabDom newEntityDom;
            m_instanceToTemplateInterface->GenerateEntityDomBySerializing(newEntityDom, newEntity);
            PrefabUndoUtils::AppendAddEntityPatch(m_redoPatch, newEntityDom, newEntityAliasPath);
            PrefabUndoUtils::AppendRemovePatch(m_undoPatch, newEntityAliasPath);

            // Preemptively updates the cached DOM to prevent reloading instance DOM.
            PrefabDomReference cachedOwningInstanceDom = focusedInstance.GetCachedInstanceDom();
            if (cachedOwningInstanceDom.has_value())
            {
                PrefabUndoUtils::UpdateEntityInPrefabDom(cachedOwningInstanceDom, parentEntityDomAfterAddingEntity, parentEntityAliasPath);
                PrefabUndoUtils::UpdateEntityInPrefabDom(cachedOwningInstanceDom, newEntityDom, newEntityAliasPath);
            }
        }
    }
}
