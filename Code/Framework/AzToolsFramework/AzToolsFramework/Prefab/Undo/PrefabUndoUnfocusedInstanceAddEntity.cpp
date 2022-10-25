/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Interface/Interface.h>
#include <AzToolsFramework/Prefab/PrefabInstanceUtils.h>
#include <AzToolsFramework/Prefab/PrefabSystemComponentInterface.h>
#include <Prefab/Undo/PrefabUndoUnfocusedInstanceAddEntity.h>
#include <Prefab/Undo/PrefabUndoUtils.h>

namespace AzToolsFramework
{
    namespace Prefab
    {
        PrefabUndoUnfocusedInstanceAddEntity::PrefabUndoUnfocusedInstanceAddEntity(const AZStd::string& undoOperationName)
            : PrefabUndoUpdateLinkBase(undoOperationName)
        {
        }

        void PrefabUndoUnfocusedInstanceAddEntity::Capture(
            const AZ::Entity& parentEntity,
            const AZ::Entity& newEntity,
            Instance& owningInstance,
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

            // Paths
            InstanceClimbUpResult climbUpResult =
                PrefabInstanceUtils::ClimbUpToTargetOrRootInstance(owningInstance, &focusedInstance);
            AZ_Assert(climbUpResult.m_isTargetInstanceReached,
                "Owning prefab instance should be owned by a descendant of the focused prefab instance.");

            LinkId linkId = climbUpResult.m_climbedInstances.back()->GetLinkId();
            m_link = m_prefabSystemComponentInterface->FindLink(linkId);
            AZ_Assert(m_link.has_value(), "Link with id '%llu' not found",
                static_cast<AZ::u64>(linkId));

            const AZStd::string entityAliasPathPrefixForPatches =
                PrefabInstanceUtils::GetRelativePathFromClimbedInstances(climbUpResult.m_climbedInstances, true);

            const AZStd::string parentEntityAliasPathForPatches =
                entityAliasPathPrefixForPatches + parentEntityAliasPath;

            const AZStd::string newEntityAliasPathForPatches =
                entityAliasPathPrefixForPatches + newEntityAliasPath;

            const AZStd::string parentEntityAliasPathInFocusedTemplate =
                PrefabDomUtils::PathStartingWithInstances +
                climbUpResult.m_climbedInstances.back()->GetInstanceAlias() +
                parentEntityAliasPathForPatches;

            PrefabDom parentEntityDomAfterAddingEntity;
            m_instanceToTemplateInterface->GenerateDomForEntity(parentEntityDomAfterAddingEntity, parentEntity);
            GeneratePatchesForUpdateParentEntity(parentEntityDomAfterAddingEntity,
                parentEntityAliasPathForPatches,
                parentEntityAliasPathInFocusedTemplate);

            PrefabDom newEntityDom;
            m_instanceToTemplateInterface->GenerateDomForEntity(newEntityDom, newEntity);
            PrefabUndoUtils::GenerateAddEntityPatch(m_redoPatch, newEntityDom, newEntityAliasPathForPatches);

            GenerateUndoUpdateLinkPatches(m_redoPatch);
            
            // Preemptively updates the cached DOM to prevent reloading instance DOM.
            PrefabDomReference cachedOwningInstanceDom = owningInstance.GetCachedInstanceDom();
            if (cachedOwningInstanceDom.has_value())
            {
                PrefabUndoUtils::UpdateCachedOwningInstanceDom(cachedOwningInstanceDom,
                    parentEntityDomAfterAddingEntity, parentEntityAliasPath);
                PrefabUndoUtils::UpdateCachedOwningInstanceDom(cachedOwningInstanceDom,
                    newEntityDom, newEntityAliasPath);
            }
        }

        void PrefabUndoUnfocusedInstanceAddEntity::GeneratePatchesForUpdateParentEntity(
            PrefabDom& parentEntityDomAfterAddingEntity,
            const AZStd::string& parentEntityAliasPathForPatches,
            const AZStd::string& parentEntityAliasPathInFocusedTemplate)
        {
            PrefabDom& focusedTemplateDom =
                m_prefabSystemComponentInterface->FindTemplateDom(m_templateId);

            PrefabDomPath entityPathInFocusedTemplate(parentEntityAliasPathInFocusedTemplate.c_str());

            // This scope is added to limit their usage and ensure DOM is not modified when it is being used.
            {
                // DOM value pointers can't be relied upon if the original DOM gets modified after pointer creation.
                const PrefabDomValue* parentEntityDomInFocusedTemplate = entityPathInFocusedTemplate.Get(focusedTemplateDom);
                AZ_Assert(parentEntityDomInFocusedTemplate,
                    "Could not load parent entity's DOM from the focused template's DOM. "
                    "Focused template id: '%llu'.", static_cast<AZ::u64>(m_templateId));

                m_instanceToTemplateInterface->GeneratePatch(m_redoPatch,
                    *parentEntityDomInFocusedTemplate, parentEntityDomAfterAddingEntity);
            }

            m_instanceToTemplateInterface->AppendEntityAliasPathToPatchPaths(m_redoPatch,
                parentEntityAliasPathForPatches);
        }
    }
}
