/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

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
            , m_changed(false)
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
            return m_changed;
        }

        void PrefabUndoComponentPropertyOverride::CaptureAndRedo(
            Instance& owningInstance,
            const AZ::Dom::Path pathToPropertyFromOwningPrefab,
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

            // Configure paths from focused and top templates. The top template is the source template of the link.
            AZ::Dom::Path pathToPropertyFromTopPrefab =
                AZ::Dom::Path(PrefabInstanceUtils::GetRelativePathFromClimbedInstances(climbUpResult.m_climbedInstances, true)) /
                pathToPropertyFromOwningPrefab;
            AZ::Dom::Path pathToPropertyFromFocusedPrefab(PrefabDomUtils::InstancesName);
            pathToPropertyFromFocusedPrefab /= climbUpResult.m_climbedInstances.back()->GetInstanceAlias(); // top instance alias
            pathToPropertyFromFocusedPrefab /= pathToPropertyFromTopPrefab;

            // Get the current state of property value from focused template.
            const TemplateId focusedTemplateId = link->get().GetTargetTemplateId();
            const PrefabDom& focusedTemplateDom = m_prefabSystemComponentInterface->FindTemplateDom(focusedTemplateId);
            PrefabDomPath domPathToPropertyFromFocusedPrefab(pathToPropertyFromFocusedPrefab.ToString().c_str());
            const PrefabDomValue* currentPropertyDomValue = domPathToPropertyFromFocusedPrefab.Get(focusedTemplateDom);

            if (currentPropertyDomValue)
            {
                PrefabDom changePatches;
                m_instanceToTemplateInterface->GeneratePatch(changePatches, *currentPropertyDomValue, afterStateOfComponentProperty);
                m_changed = !(changePatches.GetArray().Empty());
            }
            else
            {
                // If the DOM value from focused template is not present, it means the value is new to the template.
                m_changed = true;
            }

            if (m_changed)
            {
                // Get the default state of property value from top template (direct child prefab of focused template).
                // Note: There could be the case where top prefab instance is not the owning prefab instance,
                // so we fetch data from the top prefab template for default values.
                const TemplateId topTemplateId = link->get().GetSourceTemplateId();
                const PrefabDom& topTemplateDom = m_prefabSystemComponentInterface->FindTemplateDom(topTemplateId);
                PrefabDomPath domPathToPropertyFromTopPrefab(pathToPropertyFromTopPrefab.ToString().c_str());
                const PrefabDomValue* defaultPropertyDomValue = domPathToPropertyFromTopPrefab.Get(topTemplateDom);

                PrefabDom overridePatches(rapidjson::kArrayType);

                // Generate override patches for the property.
                PatchType patchType = PatchType::Edit;
                if (!defaultPropertyDomValue)
                {
                    // If the default property DOM value is not present in template, we create a new 'add' patch for the new value.
                    patchType = PatchType::Add;
                }

                PrefabUndoUtils::AppendUpdateValuePatch(
                    overridePatches, afterStateOfComponentProperty, pathToPropertyFromTopPrefab.ToString(), patchType);

                // Remove the subtree and cache the subtree for undo.
                // Note: Depending on the path, this would remove all existing patches to individual values (e.g. vector).
                m_overriddenPropertySubTree = AZStd::move(link->get().RemoveOverrides(pathToPropertyFromTopPrefab));

                m_overriddenPropertyPath = pathToPropertyFromTopPrefab;

                // Redo - Add the override patches to the tree.
                link->get().AddOverrides(overridePatches);

                PrefabDomReference cachedOwningInstanceDom = owningInstance.GetCachedInstanceDom();

                // Preemptively updates the cached DOM to prevent reloading instance DOM.
                if (cachedOwningInstanceDom.has_value())
                {
                    PrefabUndoUtils::UpdateValueInPrefabDom(
                        cachedOwningInstanceDom, afterStateOfComponentProperty, pathToPropertyFromOwningPrefab.ToString());
                }

                // Redo - Update target template of the link.
                link->get().UpdateTarget();
                m_prefabSystemComponentInterface->SetTemplateDirtyFlag(link->get().GetTargetTemplateId(), true);
                m_prefabSystemComponentInterface->PropagateTemplateChanges(link->get().GetTargetTemplateId());
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
            if (m_changed)
            {
                LinkReference link = m_prefabSystemComponentInterface->FindLink(m_linkId);
                if (link.has_value())
                {
                    // In redo, after-state subtrees in map will be moved to the link tree.
                    // In undo, before-state subtrees in map will be moved to the link tree.
                    // The previous states of subtrees in link are moved back to the map for next undo/redo if any.
                    AZ_Assert(!m_overriddenPropertyPath.IsEmpty(), "PrefabUndoComponentPropertyOverride - Overriden property path is empty.");
                    PrefabOverridePrefixTree subtreeInLink = AZStd::move(link->get().RemoveOverrides(m_overriddenPropertyPath));
                    link->get().AddOverrides(m_overriddenPropertyPath, AZStd::move(m_overriddenPropertySubTree));
                    m_overriddenPropertySubTree = AZStd::move(subtreeInLink);

                    link->get().UpdateTarget();
                    m_prefabSystemComponentInterface->SetTemplateDirtyFlag(link->get().GetTargetTemplateId(), true);
                    m_prefabSystemComponentInterface->PropagateTemplateChanges(link->get().GetTargetTemplateId());
                }
            }
        }
    } // namespace Prefab
} // namespace AzToolsFramework
