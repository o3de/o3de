/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Interface/Interface.h>
#include <AzToolsFramework/Prefab/Instance/InstanceToTemplateInterface.h>
#include <AzToolsFramework/Prefab/PrefabInstanceUtils.h>
#include <AzToolsFramework/Prefab/PrefabSystemComponentInterface.h>
#include <AzToolsFramework/Prefab/Undo/PrefabUndoAddEntityAsOverride.h>
#include <AzToolsFramework/Prefab/Undo/PrefabUndoUtils.h>

namespace AzToolsFramework
{
    namespace Prefab
    {
        PrefabUndoAddEntityAsOverride::PrefabUndoAddEntityAsOverride(const AZStd::string& undoOperationName)
            : PrefabUndoUpdateLink(undoOperationName)
        {
        }

        void PrefabUndoAddEntityAsOverride::Capture(
            const AZ::Entity& parentEntity,
            const AZ::Entity& newEntity,
            Instance& owningInstance,
            Instance& focusedInstance)
        {
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

            InstanceClimbUpResult climbUpResult =
                PrefabInstanceUtils::ClimbUpToTargetOrRootInstance(owningInstance, &focusedInstance);
            AZ_Assert(climbUpResult.m_isTargetInstanceReached,
                "Owning prefab instance should be owned by a descendant of the focused prefab instance.");

            const AZStd::string entityAliasPathPrefixForPatch =
                PrefabInstanceUtils::GetRelativePathFromClimbedInstances(climbUpResult.m_climbedInstances, true);

            const AZStd::string parentEntityAliasPathForPatch =
                entityAliasPathPrefixForPatch + parentEntityAliasPath;

            const AZStd::string parentEntityAliasPathInFocusedTemplate =
                PrefabDomUtils::PathStartingWithInstances +
                climbUpResult.m_climbedInstances.back()->GetInstanceAlias() +
                parentEntityAliasPathForPatch;

            PrefabDom parentEntityDomAfterAddingEntity;
            m_instanceToTemplateInterface->GenerateEntityDomBySerializing(parentEntityDomAfterAddingEntity, parentEntity);

            TemplateId focusedTemplateId = focusedInstance.GetTemplateId();
            PrefabDom& focusedTemplateDom =
                m_prefabSystemComponentInterface->FindTemplateDom(focusedTemplateId);
            PrefabDomPath entityPathInFocusedTemplate(parentEntityAliasPathInFocusedTemplate.c_str());
            // This scope is added to limit their usage and ensure DOM is not modified when it is being used.
            {
                // DOM value pointers can't be relied upon if the original DOM gets modified after pointer creation.
                const PrefabDomValue* parentEntityDomInFocusedTemplate = entityPathInFocusedTemplate.Get(focusedTemplateDom);
                AZ_Assert(parentEntityDomInFocusedTemplate,
                    "Could not load parent entity's DOM from the focused template's DOM. "
                    "Focused template id: '%llu'.", static_cast<AZ::u64>(focusedTemplateId));

                PrefabUndoUtils::GenerateAndAppendPatch(
                    m_redoPatch, *parentEntityDomInFocusedTemplate, parentEntityDomAfterAddingEntity, parentEntityAliasPathForPatch);
            }

            const AZStd::string newEntityAliasPathForPatch =
                entityAliasPathPrefixForPatch + newEntityAliasPath;

            PrefabDom newEntityDom;
            m_instanceToTemplateInterface->GenerateEntityDomBySerializing(newEntityDom, newEntity);
            PrefabUndoUtils::AppendAddEntityPatch(m_redoPatch, newEntityDom, newEntityAliasPathForPatch);

            const LinkId linkId = climbUpResult.m_climbedInstances.back()->GetLinkId();
            PrefabUndoUpdateLink::Capture(m_redoPatch, linkId);
            
            // Preemptively updates the cached DOM to prevent reloading instance DOM.
            PrefabDomReference cachedOwningInstanceDom = owningInstance.GetCachedInstanceDom();
            if (cachedOwningInstanceDom.has_value())
            {
                PrefabUndoUtils::UpdateEntityInPrefabDom(cachedOwningInstanceDom, parentEntityDomAfterAddingEntity, parentEntityAliasPath);
                PrefabUndoUtils::UpdateEntityInPrefabDom(cachedOwningInstanceDom, newEntityDom, newEntityAliasPath);
            }
        }
    }
}
