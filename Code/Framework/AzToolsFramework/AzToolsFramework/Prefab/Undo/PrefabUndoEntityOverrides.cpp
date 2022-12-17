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
                    AZ_Error("Prefab", false, "PrefabUndoEntityOverrides::Capture - Found a null entity. Skipping it.");
                    continue;
                }

                // Hierarchy Example: Level (focused) -> Car (top) -> ... -> Wheel (owning) -> Tire (override)
                // Path seen from focused: /Instances/Instance_[Car]/.../Instances/Instance_[Wheel]/Entities/Entity_[Tire]
                //                                   (top instance)               (owning instance)         (entity to update)
                // Note: Override patch path is the path seen from top without top instance alias path.
                const AZStd::string entityAliasPath = m_instanceToTemplateInterface->GenerateEntityAliasPath(entity->GetId());
                const AZStd::string entityPathFromTopInstance = overridePatchPathToOwningInstance + entityAliasPath;
                PrefabDomPath entityDomPathFromTopInstance(entityPathFromTopInstance.c_str());

                // Get the after-state entity DOM.
                PrefabDom entityDomAfterUpdate;
                m_instanceToTemplateInterface->GenerateDomForEntity(entityDomAfterUpdate, *entity);

                // Get the before-state entity DOM inside the source template DOM of the top instance.
                const TemplateId topTemplateId = link->get().GetSourceTemplateId();
                const PrefabDom& topTemplateDom = m_prefabSystemComponentInterface->FindTemplateDom(topTemplateId);

                PrefabDom overridePatches;
                {
                    // This scope is added to limit their usage and ensure DOM is not modified when it is being used.
                    // DOM value pointers can't be relied upon if the original DOM gets modified after pointer creation.
                    const PrefabDomValue* entityDomInTopTemplate = entityDomPathFromTopInstance.Get(topTemplateDom);

                    if (entityDomInTopTemplate)
                    {
                        m_instanceToTemplateInterface->GeneratePatch(overridePatches, *entityDomInTopTemplate, entityDomAfterUpdate);
                    }
                    else
                    {
                        // Merge patches to an entity DOM as add-entity override.
                        // Path is empty because there is no subpath for add-entity override patch like "/Components/Component_[123]".
                        overridePatches.SetArray();
                        PrefabUndoUtils::AppendAddEntityPatch(overridePatches, entityDomAfterUpdate, "");
                    }
                }

                PrefabOverridePrefixTree overrideSubTree = link->get().GenerateOverrideSubTreeForEntity(
                    overridePatches, entityPathFromTopInstance);
                m_overrideSubTrees[AZ::Dom::Path(entityPathFromTopInstance)] = AZStd::move(overrideSubTree);

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
                for (auto& [pathToSubTree, overrideSubTree] : m_overrideSubTrees)
                {
                    PrefabOverridePrefixTree prevSubTree = AZStd::move(link->get().RemoveOverrides(pathToSubTree));
                    link->get().AddOverrides(pathToSubTree, AZStd::move(overrideSubTree));
                    overrideSubTree = AZStd::move(prevSubTree);
                }

                link->get().UpdateTarget();
                m_prefabSystemComponentInterface->SetTemplateDirtyFlag(link->get().GetTargetTemplateId(), true);
                m_prefabSystemComponentInterface->PropagateTemplateChanges(link->get().GetTargetTemplateId());
            }
        }
    } // namespace Prefab
} // namespace AzToolsFramework
