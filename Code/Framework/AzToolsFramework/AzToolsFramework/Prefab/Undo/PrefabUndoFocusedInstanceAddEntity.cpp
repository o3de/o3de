/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Interface/Interface.h>
#include <AzToolsFramework/Prefab/Instance/InstanceEntityMapperInterface.h>
#include <AzToolsFramework/Prefab/PrefabInstanceUtils.h>
#include <AzToolsFramework/Prefab/PrefabSystemComponentInterface.h>
#include <Prefab/Undo/PrefabUndoFocusedInstanceAddEntity.h>
#include <Prefab/Undo/PrefabUndoUtils.h>

namespace AzToolsFramework
{
    namespace Prefab
    {
        PrefabUndoFocusedInstanceAddEntity::PrefabUndoFocusedInstanceAddEntity(const AZStd::string& undoOperationName)
            : PrefabUndoBase(undoOperationName)
        {
            m_prefabSystemComponentInterface = AZ::Interface<PrefabSystemComponentInterface>::Get();
            AZ_Assert(m_prefabSystemComponentInterface, "Failed to grab PrefabSystemComponentInterface");
        }

        void PrefabUndoFocusedInstanceAddEntity::Capture(
            const AZ::Entity& parentEntity,
            const AZ::Entity& newEntity,
            Instance& focusedInstance)
        {
            m_templateId = focusedInstance.GetTemplateId();
            const AZ::EntityId parentEntityId = parentEntity.GetId();
            const AZ::EntityId newEntityId = newEntity.GetId();

            AZStd::string parentEntityAliasPath =
                m_instanceToTemplateInterface->GenerateEntityAliasPath(parentEntityId);
            AZ_Assert(!parentEntityAliasPath.empty(),
                "Alias path of parent entity with id '%llu' shouldn't be empty.",
                static_cast<AZ::u64>(parentEntityId));

            AZStd::string newEntityAliasPath =
                m_instanceToTemplateInterface->GenerateEntityAliasPath(newEntityId);
            AZ_Assert(!newEntityAliasPath.empty(),
                "Alias path of new entity with id '%llu' shouldn't be empty.",
                static_cast<AZ::u64>(newEntityId));

            PrefabDom parentEntityDomAfterAddingEntity;
            m_instanceToTemplateInterface->GenerateDomForEntity(parentEntityDomAfterAddingEntity, parentEntity);
            GeneratePatchesForUpdateParentEntity(parentEntityDomAfterAddingEntity,
                parentEntityAliasPath);

            PrefabDom newEntityDom;
            m_instanceToTemplateInterface->GenerateDomForEntity(newEntityDom, newEntity);
            PrefabUndoUtils::GenerateAddEntityPatch(m_redoPatch, newEntityDom, newEntityAliasPath);
            PrefabUndoUtils::GenerateRemoveEntityPatch(m_undoPatch, newEntityAliasPath);

            // Preemptively updates the cached DOM to prevent reloading instance DOM.
            PrefabDomReference cachedOwningInstanceDom = focusedInstance.GetCachedInstanceDom();
            if (cachedOwningInstanceDom.has_value())
            {
                PrefabUndoUtils::UpdateCachedOwningInstanceDom(cachedOwningInstanceDom,
                    parentEntityDomAfterAddingEntity, parentEntityAliasPath);
                PrefabUndoUtils::UpdateCachedOwningInstanceDom(cachedOwningInstanceDom,
                    newEntityDom, newEntityAliasPath);
            }
        }

        void PrefabUndoFocusedInstanceAddEntity::GeneratePatchesForUpdateParentEntity(
            PrefabDom& parentEntityDomAfterAddingEntity,
            const AZStd::string& parentEntityAliasPath)
        {
            PrefabDom& focusedTemplateDom =
                m_prefabSystemComponentInterface->FindTemplateDom(m_templateId);

            PrefabDomPath entityPathInFocusedTemplate(parentEntityAliasPath.c_str());

            // This scope is added to limit their usage and ensure DOM is not modified when it is being used.
            {
                // DOM value pointers can't be relied upon if the original DOM gets modified after pointer creation.
                const PrefabDomValue* parentEntityDomInFocusedTemplate = entityPathInFocusedTemplate.Get(focusedTemplateDom);
                AZ_Assert(parentEntityDomInFocusedTemplate,
                    "Could not load parent entity's DOM from the focused template's DOM. "
                    "Focused template id: '%llu'.", static_cast<AZ::u64>(m_templateId));

                m_instanceToTemplateInterface->GeneratePatch(m_redoPatch,
                    *parentEntityDomInFocusedTemplate, parentEntityDomAfterAddingEntity);
                m_instanceToTemplateInterface->GeneratePatch(m_undoPatch,
                    parentEntityDomAfterAddingEntity, *parentEntityDomInFocusedTemplate);
            }

            m_instanceToTemplateInterface->AppendEntityAliasPathToPatchPaths(m_redoPatch,
                parentEntityAliasPath);
            m_instanceToTemplateInterface->AppendEntityAliasPathToPatchPaths(m_undoPatch,
                parentEntityAliasPath);
        }
    }
}
