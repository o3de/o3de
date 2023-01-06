/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/Prefab/Instance/InstanceToTemplateInterface.h>
#include <AzToolsFramework/Prefab/Link/Link.h>
#include <AzToolsFramework/Prefab/Undo/PrefabUndoEntityOverrides.h>
#include <AzToolsFramework/Prefab/Undo/PrefabUndoUtils.h>
#include <AzToolsFramework/Prefab/PrefabInstanceUtils.h>
#include <AzToolsFramework/Prefab/PrefabSystemComponentInterface.h>

namespace AzToolsFramework
{
    namespace Prefab
    {
        PrefabUndoEntityOverrides::PrefabUndoEntityOverrides(const AZStd::string& undoOperationName)
            : UndoSystem::URSequencePoint(undoOperationName)
            , m_linkId(InvalidLinkId)
            , m_changed(true)
        {
            m_prefabSystemComponentInterface = AZ::Interface<PrefabSystemComponentInterface>::Get();
            AZ_Assert(m_prefabSystemComponentInterface, "PrefabUndoEntityOverrides - Failed to grab prefab system component interface.");

            m_instanceToTemplateInterface = AZ::Interface<InstanceToTemplateInterface>::Get();
            AZ_Assert(m_instanceToTemplateInterface, "PrefabUndoEntityOverrides - Could not get instance to template interface.");
        }

        bool PrefabUndoEntityOverrides::Changed() const
        {
            return m_changed;
        }

        void PrefabUndoEntityOverrides::Capture(
            const AZStd::vector<const AZ::Entity*>& entityList,
            Instance& owningInstance,
            const Instance& focusedInstance)
        {
            AZ_Assert(
                &owningInstance != &focusedInstance,
                "PrefabUndoEntityOverrides::Capture - Owning instance could not be the focused instance for override edit node.");

            InstanceClimbUpResult climbUpResult = PrefabInstanceUtils::ClimbUpToTargetOrRootInstance(owningInstance, &focusedInstance);
            AZ_Assert(climbUpResult.m_isTargetInstanceReached, "PrefabUndoEntityOverrides::Capture - "
                "Owning prefab instance should be a descendant of the focused prefab instance.");

            m_linkId = climbUpResult.m_climbedInstances.back()->GetLinkId();
            AZ_Assert(m_linkId != InvalidLinkId, "PrefabUndoEntityOverrides::Capture - "
                "Could not get the link id between focused instance and top instance.");

            LinkReference link = m_prefabSystemComponentInterface->FindLink(m_linkId);
            if (!link.has_value())
            {
                AZ_Error("Prefab", false, "PrefabUndoEntityOverrides::Capture - Could not get the link for override editing.");
                return;
            }

            PrefabDomReference cachedOwningInstanceDom = owningInstance.GetCachedInstanceDom();

            const AZStd::string overridePatchPathToOwningInstance = PrefabInstanceUtils::GetRelativePathFromClimbedInstances(
                climbUpResult.m_climbedInstances, true); // skip top instance

            for (const AZ::Entity* entity : entityList)
            {
                if (!entity)
                {
                    AZ_Warning("Prefab", false, "PrefabUndoEntityOverrides::Capture - Found a null entity. Skipping it.");
                    continue;
                }

                const AZStd::string entityAliasPath = m_instanceToTemplateInterface->GenerateEntityAliasPath(entity->GetId());
                const AZStd::string entityPathFromTopInstance = overridePatchPathToOwningInstance + entityAliasPath;
                PrefabDomPath entityDomPathFromTopInstance(entityPathFromTopInstance.c_str());

                PrefabDom entityDomAfterUpdate;
                m_instanceToTemplateInterface->GenerateDomForEntity(entityDomAfterUpdate, *entity);

                // Get the before-state entity DOM inside the source template DOM of the top instance.
                const TemplateId topTemplateId = link->get().GetSourceTemplateId();
                const PrefabDom& topTemplateDom = m_prefabSystemComponentInterface->FindTemplateDom(topTemplateId);

                PrefabDom overridePatchesWithSubpaths; // subpath is path pointing from entity to component value
                {
                    // This scope is added to limit their usage and ensure DOM is not modified when it is being used.
                    // DOM value pointers can't be relied upon if the original DOM gets modified after pointer creation.
                    const PrefabDomValue* entityDomInTopTemplate = entityDomPathFromTopInstance.Get(topTemplateDom);

                    if (entityDomInTopTemplate)
                    {
                        m_instanceToTemplateInterface->GeneratePatch(
                            overridePatchesWithSubpaths, *entityDomInTopTemplate, entityDomAfterUpdate);
                    }
                    else
                    {
                        // If entity DOM is not present in template, it indicates that the entity itself was added as an override.
                        // In this case, additional override patches on this entity would be merged into the entity DOM value that is
                        // stored in the existing add-entity override patch.

                        overridePatchesWithSubpaths.SetArray();
                        // Note that for add-entity patch, there is no subpath that points from entity to component value.
                        // Entity path passing to the util function is empty such that the generated JSON patch won't contain any path.
                        // The path inside JSON patch will be set correctly below when generating the override subtree.
                        PrefabUndoUtils::AppendAddEntityPatch(overridePatchesWithSubpaths, entityDomAfterUpdate, "");
                    }
                }

                // Subtrees in map are initialized as after-state subtrees for next redo.
                PrefabOverridePrefixTree redoSubtree = link->get().GenerateOverrideSubTreeForEntity(
                    overridePatchesWithSubpaths, entityPathFromTopInstance);
                m_subtreeStates[AZ::Dom::Path(entityPathFromTopInstance)] = AZStd::move(redoSubtree);

                // Preemptively updates the cached DOM to prevent reloading instance DOM.
                if (cachedOwningInstanceDom.has_value())
                {
                    PrefabUndoUtils::UpdateEntityInInstanceDom(cachedOwningInstanceDom, entityDomAfterUpdate, entityAliasPath);
                }
            }
        }

        void PrefabUndoEntityOverrides::Undo()
        {
            UpdateLink();
        }

        void PrefabUndoEntityOverrides::Redo()
        {
            UpdateLink();
        }

        void PrefabUndoEntityOverrides::UpdateLink()
        {
            LinkReference link = m_prefabSystemComponentInterface->FindLink(m_linkId);
            if (link.has_value())
            {
                for (auto& [pathToSubtree, subtreeState] : m_subtreeStates)
                {
                    // In redo, after-state subtrees in map will be moved to the link tree.
                    // In undo, before-state subtrees in map will be moved to the link tree.
                    // The previous states of subtrees in link are moved back to the map for next undo/redo if any.
                    PrefabOverridePrefixTree subtreeInLink = AZStd::move(link->get().RemoveOverrides(pathToSubtree));
                    link->get().AddOverrides(pathToSubtree, AZStd::move(subtreeState));
                    subtreeState = AZStd::move(subtreeInLink);
                }

                link->get().UpdateTarget();
                m_prefabSystemComponentInterface->SetTemplateDirtyFlag(link->get().GetTargetTemplateId(), true);
                m_prefabSystemComponentInterface->PropagateTemplateChanges(link->get().GetTargetTemplateId());
            }
        }
    } // namespace Prefab
} // namespace AzToolsFramework
