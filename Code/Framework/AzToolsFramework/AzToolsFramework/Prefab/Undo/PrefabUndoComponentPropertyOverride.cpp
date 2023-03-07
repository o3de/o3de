/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma optimize("", off)
#include <AzToolsFramework/Prefab/Instance/InstanceToTemplateInterface.h>
#include <AzToolsFramework/Prefab/Link/Link.h>
#include <AzToolsFramework/Prefab/Overrides/PrefabOverridePublicInterface.h>
#include <AzToolsFramework/Prefab/PrefabDomUtils.h>
#include <AzToolsFramework/Prefab/PrefabInstanceUtils.h>
#include <AzToolsFramework/Prefab/PrefabFocusInterface.h>
#include <AzToolsFramework/Prefab/PrefabSystemComponentInterface.h>
#include <AzToolsFramework/Prefab/Undo/PrefabUndoComponentPropertyOverride.h>
#include <AzToolsFramework/Prefab/Undo/PrefabUndoUtils.h>


namespace AzToolsFramework
{
    namespace Prefab
    {
        PrefabUndoComponentPropertyOverride::PrefabUndoComponentPropertyOverride(const AZStd::string& undoOperationName)
            : UndoSystem::URSequencePoint(undoOperationName)
            , m_linkId(InvalidLinkId)
        {
            m_prefabSystemComponentInterface = AZ::Interface<PrefabSystemComponentInterface>::Get();
            AZ_Assert(m_prefabSystemComponentInterface, "PrefabUndoComponentPropertyOverride - PrefabSystemComponentInterface not found.");

            m_instanceToTemplateInterface = AZ::Interface<InstanceToTemplateInterface>::Get();
            AZ_Assert(m_instanceToTemplateInterface, "PrefabUndoComponentPropertyOverride - InstanceToTemplateInterface not found.");

            m_prefabOverridePublicInterface = AZ::Interface<PrefabOverridePublicInterface>::Get();
            AZ_Assert(m_prefabOverridePublicInterface, "PrefabUndoComponentPropertyOverride - PrefabOverridePublicInterface not found.");

            m_prefabFocusInterface = AZ::Interface<Prefab::PrefabFocusInterface>::Get();
            AZ_Assert(
                m_prefabFocusInterface != nullptr,
                "PrefabUndoComponentPropertyOverride - PrefabFocusInterface not found.");
        }

        bool PrefabUndoComponentPropertyOverride::Changed() const
        {
            return true;
        }

        void PrefabUndoComponentPropertyOverride::CaptureAndRedo(
            Instance& owningInstance,
            AZ::Dom::Path relativePathFromOwningPrefab,
            const PrefabDomValue& afterStateOfComponentProperty)
        {
            InstanceOptionalReference focusedInstance =
                m_prefabFocusInterface->GetFocusedPrefabInstance(AzFramework::EntityContextId::CreateNull());

            AZ_Assert(focusedInstance.has_value(), "PrefabUndoComponentPropertyOverride::CaptureAndRedo - Focused instance not found.");

            InstanceClimbUpResult climbUpResult = PrefabInstanceUtils::ClimbUpToTargetOrRootInstance(owningInstance, &focusedInstance->get());
            AZ_Assert(
                climbUpResult.m_isTargetInstanceReached,
                "PrefabUndoComponentPropertyOverride::CaptureAndRedo - "
                "Owning prefab instance should be a descendant of the focused prefab instance.");

            m_linkId = climbUpResult.m_climbedInstances.back()->GetLinkId();
            AZ_Assert(
                m_linkId != InvalidLinkId,
                "PrefabUndoComponentPropertyOverride::CaptureAndRedo - "
                "Could not get the link id between focused instance and top instance.");

            LinkReference link = m_prefabSystemComponentInterface->FindLink(m_linkId);
            if (!link.has_value())
            {
                AZ_Error("Prefab", false, "PrefabUndoComponentPropertyOverride::CaptureAndRedo - Could not get the link for override editing.");
                return;
            }

            m_overriddenPropertyPathFromFocusedPrefab =
                AZ::Dom::Path(PrefabInstanceUtils::GetRelativePathFromClimbedInstances(climbUpResult.m_climbedInstances, true));

            m_overriddenPropertyPathFromFocusedPrefab /= AZ::Dom::Path(relativePathFromOwningPrefab);

            const TemplateId topTemplateId = link->get().GetTargetTemplateId();
            const PrefabDom& topTemplateDom = m_prefabSystemComponentInterface->FindTemplateDom(topTemplateId);

            {
                AZ::Dom::Path pathFromFocusedPrefab(PrefabDomUtils::InstancesName);
                pathFromFocusedPrefab /= climbUpResult.m_climbedInstances.back()->GetInstanceAlias();
                pathFromFocusedPrefab /= m_overriddenPropertyPathFromFocusedPrefab;

                // This scope is added to limit their usage and ensure DOM is not modified when it is being used.
                // DOM value pointers can't be relied upon if the original DOM gets modified after pointer creation.
                PrefabDomPath overriddenPropertyDomPath(pathFromFocusedPrefab.ToString().c_str());
                const PrefabDomValue* overriddenPropertyDomInTopTemplate = overriddenPropertyDomPath.Get(topTemplateDom);

                
                PrefabDom overridePatches;
                if (overriddenPropertyDomInTopTemplate)
                {
                    PrefabUndoUtils::GenerateUpdateEntityPatch(
                        overridePatches,
                        *overriddenPropertyDomInTopTemplate,
                        afterStateOfComponentProperty,
                        m_overriddenPropertyPathFromFocusedPrefab.ToString());

                    // Remove the subtree and cache the subtree for undo.
                    m_componentPropertyOverrideSubTree =
                        AZStd::move(link->get().RemoveOverrides(m_overriddenPropertyPathFromFocusedPrefab));

                    // Redo - Add the override patches to the tree.
                    link->get().AddOverrides(overridePatches);

                    PrefabDomReference cachedFocusedInstanceDom = focusedInstance->get().GetCachedInstanceDom();

                    // Preemptively updates the cached DOM to prevent reloading instance DOM.
                    if (cachedFocusedInstanceDom.has_value())
                    {
                        PrefabUndoUtils::UpdateEntityInInstanceDom(
                            cachedFocusedInstanceDom, afterStateOfComponentProperty, pathFromFocusedPrefab.ToString());
                    }

                    // Redo - Update target template of the link.
                    link->get().UpdateTarget();
                    m_prefabSystemComponentInterface->SetTemplateDirtyFlag(link->get().GetTargetTemplateId(), true);
                    m_prefabSystemComponentInterface->PropagateTemplateChanges(link->get().GetTargetTemplateId());
                }
                else
                {
                    AZ_Warning(
                        "Prefab",
                        false,
                        "PrefabUndoComponentPropertyOverride::CaptureAndRedo - Cannot reach overridden property value from the DOM of the prefab being edited.");
                }
            }

            
        }

        void PrefabUndoComponentPropertyOverride::Undo()
        {
            UpdateLink();
        }

        void PrefabUndoComponentPropertyOverride::Redo()
        {
            UpdateLink();
        }

        void PrefabUndoComponentPropertyOverride::UpdateLink()
        {
            LinkReference link = m_prefabSystemComponentInterface->FindLink(m_linkId);
            if (link.has_value())
            {
                // In redo, after-state subtrees in map will be moved to the link tree.
                // In undo, before-state subtrees in map will be moved to the link tree.
                // The previous states of subtrees in link are moved back to the map for next undo/redo if any.
                PrefabOverridePrefixTree subtreeInLink =
                    AZStd::move(link->get().RemoveOverrides(m_overriddenPropertyPathFromFocusedPrefab));
                link->get().AddOverrides(m_overriddenPropertyPathFromFocusedPrefab, AZStd::move(m_componentPropertyOverrideSubTree));
                m_componentPropertyOverrideSubTree = AZStd::move(subtreeInLink);

                link->get().UpdateTarget();
                m_prefabSystemComponentInterface->SetTemplateDirtyFlag(link->get().GetTargetTemplateId(), true);
                m_prefabSystemComponentInterface->PropagateTemplateChanges(link->get().GetTargetTemplateId());
            }
        }
    } // namespace Prefab
} // namespace AzToolsFramework
#pragma optimize("", on)
