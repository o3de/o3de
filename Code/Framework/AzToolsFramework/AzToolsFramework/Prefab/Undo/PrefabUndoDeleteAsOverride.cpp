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
#include <AzToolsFramework/Prefab/PrefabInstanceUtils.h>
#include <AzToolsFramework/Prefab/PrefabSystemComponentInterface.h>
#include <AzToolsFramework/Prefab/Undo/PrefabUndoDeleteAsOverride.h>
#include <AzToolsFramework/Prefab/Undo/PrefabUndoUtils.h>

namespace AzToolsFramework
{
    namespace Prefab
    {
        PrefabUndoDeleteAsOverride::PrefabUndoDeleteAsOverride(const AZStd::string& undoOperationName)
            : PrefabUndoUpdateLink(undoOperationName)
        {
        }

        void PrefabUndoDeleteAsOverride::Capture(
            const AZStd::vector<AZStd::string>& entityAliasPathList,
            const AZStd::vector<AZStd::string>& instanceAliasPathList,
            const AZStd::vector<const AZ::Entity*> parentEntityList,
            const Instance& owningInstance,
            Instance& focusedInstance)
        {
            m_templateId = focusedInstance.GetTemplateId();
            m_redoPatch.SetArray();
            m_undoPatch.SetArray();

            PrefabDom& focusedTempalteDom = m_prefabSystemComponentInterface->FindTemplateDom(m_templateId);

            PrefabDomReference cachedOwningInstanceDom = focusedInstance.GetCachedInstanceDom();

            // Generates override patch path to owning instance.
            InstanceClimbUpResult climbUpResult = PrefabInstanceUtils::ClimbUpToTargetOrRootInstance(owningInstance, &focusedInstance);
            AZ_Assert(
                climbUpResult.m_isTargetInstanceReached,
                "PrefabUndoDeleteAsOverride::Capture - "
                "Owning prefab instance should be owned by a descendant of the focused prefab instance.");

            const AZStd::string overridePatchPathToOwningInstance =
                PrefabInstanceUtils::GetRelativePathFromClimbedInstances(climbUpResult.m_climbedInstances, true);
            const LinkId instanceOverridePatchLinkId = climbUpResult.m_climbedInstances.back()->GetLinkId();
            AZ_Assert(
                instanceOverridePatchLinkId != InvalidLinkId,
                "PrefabUndoDeleteAsOverride::Capture - "
                "Could not get the link id between focused instance and top instance.");

            // Removes entity DOMs.
            for (const AZStd::string& entityAliasPath : entityAliasPathList)
            {
                if (entityAliasPath.empty())
                {
                    AZ_Error("Prefab", false, "PrefabUndoDeleteAsOverride::Capture - Entity alias path is empty.");
                    continue;
                }

                PrefabUndoUtils::AppendRemovePatch(m_redoPatch, overridePatchPathToOwningInstance + entityAliasPath);

                // Preemptively updates the cached DOM to prevent reloading instance.
                if (cachedOwningInstanceDom.has_value())
                {
                    PrefabUndoUtils::RemoveValueInInstanceDom(cachedOwningInstanceDom->get(), entityAliasPath);
                }
            }

            // Removes instance DOMs.
            for (const AZStd::string& instanceAliasPath : instanceAliasPathList)
            {
                PrefabUndoUtils::AppendRemovePatch(m_redoPatch, overridePatchPathToOwningInstance + instanceAliasPath);

                // Preemptively updates the cached DOM to prevent reloading instance.
                if (cachedOwningInstanceDom.has_value())
                {
                    PrefabUndoUtils::RemoveValueInInstanceDom(cachedOwningInstanceDom->get(), instanceAliasPath);
                }
            }

            // Updates parent entity DOMs.
            // For each parent entity, it includes one or more than one patches to update EditorEntitySortComponent.
            const AZStd::string owningInstanceAliasPathFromFocused = PrefabDomUtils::PathStartingWithInstances +
                climbUpResult.m_climbedInstances.back()->GetInstanceAlias() + overridePatchPathToOwningInstance;

            for (const AZ::Entity* parentEntity : parentEntityList)
            {
                if (!parentEntity)
                {
                    AZ_Error("Prefab", false, "PrefabUndoDeleteAsOverride::Capture - Parent entity cannot be nullptr.");
                    continue;
                }

                const AZStd::string parentEntityAliasPath = m_instanceToTemplateInterface->GenerateEntityAliasPath(
                    parentEntity->GetId());
                PrefabDomPath parentEntityAliasDomPathFromFocused((owningInstanceAliasPathFromFocused + parentEntityAliasPath).c_str());

                // This scope is added to limit usage and ensure DOM is not modified when it is being used.
                {
                    // DOM value pointers can't be relied upon if the original DOM gets modified after pointer creation.
                    const PrefabDomValue* parentEntityDomInFocusedTemplate = parentEntityAliasDomPathFromFocused.Get(focusedTempalteDom);
                    if (!parentEntityDomInFocusedTemplate)
                    {
                        AZ_Error("Prefab", false,
                            "PrefabUndoDeleteAsOverride::Capture - Cannot retrieve parent entity DOM from focused template.");
                        continue;
                    }

                    PrefabDom parentEntityDomAfterRemovingChildren;
                    m_instanceToTemplateInterface->GenerateDomForEntity(parentEntityDomAfterRemovingChildren, *parentEntity);

                    PrefabUndoUtils::AppendUpdateEntityPatch(
                        m_redoPatch,
                        *parentEntityDomInFocusedTemplate,
                        parentEntityDomAfterRemovingChildren,
                        overridePatchPathToOwningInstance + parentEntityAliasPath);

                    // Preemptively updates the cached DOM to prevent reloading instance.
                    if (cachedOwningInstanceDom.has_value())
                    {
                        PrefabUndoUtils::UpdateEntityInInstanceDom(
                            cachedOwningInstanceDom->get(), parentEntityDomAfterRemovingChildren, parentEntityAliasPath);
                    }
                }
            }

            // Updates override link.
            PrefabUndoUpdateLink::Capture(m_redoPatch, instanceOverridePatchLinkId);
        }
    } // namespace Prefab
} // namespace AzToolsFramework
