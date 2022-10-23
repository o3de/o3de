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
#include <Prefab/Undo/PrefabUndoAddEntity.h>

namespace AzToolsFramework
{
    namespace Prefab
    {
        PrefabUndoAddEntity::PrefabUndoAddEntity(const AZStd::string& undoOperationName)
            : PrefabUndoBase(undoOperationName)
        {
            m_instanceEntityMapperInterface = AZ::Interface<InstanceEntityMapperInterface>::Get();
            AZ_Assert(m_instanceEntityMapperInterface, "Failed to grab InstanceEntityMapperInterface");

            m_prefabSystemComponentInterface = AZ::Interface<PrefabSystemComponentInterface>::Get();
            AZ_Assert(m_prefabSystemComponentInterface, "Failed to grab PrefabSystemComponentInterface");
        }

        void PrefabUndoAddEntity::Capture(
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

            InstanceOptionalReference findOwningInstanceResult =
                m_instanceEntityMapperInterface->FindOwningInstance(parentEntityId);
            AZ_Assert(findOwningInstanceResult.has_value(),
                "Can't find owning instance of parent entity with id '%llu'.",
                static_cast<AZ::u64>(parentEntityId));
            Instance& owningInstance = findOwningInstanceResult->get();

            // Paths
            InstanceClimbUpResult climbUpResult =
                PrefabInstanceUtils::ClimbUpToTargetOrRootInstance(owningInstance, &focusedInstance);
            AZ_Assert(climbUpResult.m_isTargetInstanceReached,
                "Owning prefab instance should be owned by a descendant of the focused prefab instance.");

            const bool updateLinkDomNeeded = &owningInstance != &focusedInstance;
            if (updateLinkDomNeeded)
            {
                LinkId linkId = climbUpResult.m_climbedInstances.back()->GetLinkId();
                m_link = m_prefabSystemComponentInterface->FindLink(linkId);
                AZ_Assert(m_link.has_value(), "Link with id '%llu' not found",
                    static_cast<AZ::u64>(linkId));
            }

            const AZStd::string entityAliasPathPrefixForPatches =
                PrefabInstanceUtils::GetRelativePathFromClimbedInstances(climbUpResult.m_climbedInstances, true);

            const AZStd::string parentEntityAliasPathForPatches =
                entityAliasPathPrefixForPatches + parentEntityAliasPath;

            const AZStd::string newEntityAliasPathForPatches =
                entityAliasPathPrefixForPatches + newEntityAliasPath;

            const AZStd::string parentEntityAliasPathInFocusedTemplate = updateLinkDomNeeded?
                (PrefabDomUtils::PathStartingWithInstances +
                climbUpResult.m_climbedInstances.back()->GetInstanceAlias() +
                parentEntityAliasPathForPatches) :
                parentEntityAliasPathForPatches;

            PrefabDom parentEntityDomAfterAddingEntity;
            m_instanceToTemplateInterface->GenerateDomForEntity(parentEntityDomAfterAddingEntity, parentEntity);
            GeneratePatchesForUpdateParentEntity(parentEntityDomAfterAddingEntity,
                parentEntityAliasPathForPatches,
                parentEntityAliasPathInFocusedTemplate,
                !updateLinkDomNeeded);

            PrefabDom newEntityDom;
            m_instanceToTemplateInterface->GenerateDomForEntity(newEntityDom, newEntity);
            GeneratePatchesForAddNewEntity(newEntityDom,
                newEntityAliasPathForPatches,
                !updateLinkDomNeeded);

            if (updateLinkDomNeeded)
            {
                GeneratePatchesForLinkUpdate();
            }
            
            // Preemptively updates the cached DOM to prevent reloading instance DOM.
            PrefabDomReference cachedOwningInstanceDom = owningInstance.GetCachedInstanceDom();
            if (cachedOwningInstanceDom.has_value())
            {
                UpdateCachedOwningInstanceDOM(cachedOwningInstanceDom,
                    parentEntityDomAfterAddingEntity, parentEntityAliasPath);
                UpdateCachedOwningInstanceDOM(cachedOwningInstanceDom,
                    newEntityDom, newEntityAliasPath);
            }
        }

        void PrefabUndoAddEntity::Undo()
        {
            if (m_link.has_value())
            {
                UpdateLink(m_undoPatch);
            }
            else
            {
                PrefabUndoBase::Undo();
            }
        }

        void PrefabUndoAddEntity::Redo()
        {
            if (m_link.has_value())
            {
                UpdateLink(m_redoPatch);
            }
            else
            {
                PrefabUndoBase::Redo();
            }
        }

        void PrefabUndoAddEntity::GeneratePatchesForUpdateParentEntity(
            PrefabDom& parentEntityDomAfterAddingEntity,
            const AZStd::string& parentEntityAliasPathForPatches,
            const AZStd::string& parentEntityAliasPathInFocusedTemplate,
            bool updateUndoPatchNeeded)
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

                m_instanceToTemplateInterface->AppendEntityAliasPathToPatchPaths(m_redoPatch,
                    parentEntityAliasPathForPatches);

                if (updateUndoPatchNeeded)
                {
                    m_instanceToTemplateInterface->GeneratePatch(m_undoPatch,
                        parentEntityDomAfterAddingEntity, *parentEntityDomInFocusedTemplate);

                    m_instanceToTemplateInterface->AppendEntityAliasPathToPatchPaths(m_undoPatch,
                        parentEntityAliasPathForPatches);
                }
            }
        }

        void PrefabUndoAddEntity::GeneratePatchesForAddNewEntity(
            PrefabDom& newEntityDom,
            const AZStd::string& newEntityAliasPathForPatches,
            bool updateUndoPatchNeeded)
        {
            PrefabDomValue addNewEntityRedoPatch(rapidjson::kObjectType);
            rapidjson::Value path = rapidjson::Value(newEntityAliasPathForPatches.data(),
                aznumeric_caster(newEntityAliasPathForPatches.length()), m_redoPatch.GetAllocator());
            rapidjson::Value patchValue;
            patchValue.CopyFrom(newEntityDom, m_redoPatch.GetAllocator(), true);
            addNewEntityRedoPatch.AddMember(rapidjson::StringRef("op"), rapidjson::StringRef("add"), m_redoPatch.GetAllocator())
                .AddMember(rapidjson::StringRef("path"), AZStd::move(path), m_redoPatch.GetAllocator())
                .AddMember(rapidjson::StringRef("value"), AZStd::move(patchValue), m_redoPatch.GetAllocator());
            m_redoPatch.PushBack(addNewEntityRedoPatch.Move(), m_redoPatch.GetAllocator());

            if (updateUndoPatchNeeded)
            {
                PrefabDomValue addNewEntityUndoPatch(rapidjson::kObjectType);
                path = rapidjson::Value(newEntityAliasPathForPatches.data(),
                    aznumeric_caster(newEntityAliasPathForPatches.length()), m_undoPatch.GetAllocator());
                addNewEntityUndoPatch.AddMember(rapidjson::StringRef("op"), rapidjson::StringRef("remove"), m_undoPatch.GetAllocator())
                    .AddMember(rapidjson::StringRef("path"), AZStd::move(path), m_undoPatch.GetAllocator());
                m_undoPatch.PushBack(addNewEntityUndoPatch.Move(), m_undoPatch.GetAllocator());
            }
        }

        void PrefabUndoAddEntity::GeneratePatchesForLinkUpdate()
        {
            AZ_Assert(m_link.has_value(), "Link not found");

            //Cache current link DOM for undo link update.
            m_undoPatch.CopyFrom(m_link->get().GetLinkDom(), m_undoPatch.GetAllocator());

            //Get DOM of the link's source template.
            TemplateId sourceTemplateId = m_link->get().GetSourceTemplateId();
            TemplateReference sourceTemplate = m_prefabSystemComponentInterface->FindTemplate(sourceTemplateId);
            AZ_Assert(sourceTemplate.has_value(), "Source template not found");
            PrefabDom& sourceDom = sourceTemplate->get().GetPrefabDom();

            //Get DOM of instance that the link points to.
            PrefabDomValueReference instanceDomRef = m_link->get().GetLinkedInstanceDom();
            AZ_Assert(instanceDomRef.has_value(), "Linked Instance DOM not found");
            PrefabDom instanceDom;
            instanceDom.CopyFrom(instanceDomRef->get(), instanceDom.GetAllocator());

            //Apply patch for adding new entity under parent entity to the instance.
            [[maybe_unused]] AZ::JsonSerializationResult::ResultCode result = PrefabDomUtils::ApplyPatches(
                instanceDom, instanceDom.GetAllocator(), m_redoPatch);
            AZ_Warning(
                "Prefab",
                (result.GetOutcome() != AZ::JsonSerializationResult::Outcomes::Skipped) &&
                (result.GetOutcome() != AZ::JsonSerializationResult::Outcomes::PartialSkip),
                "Some of the patches are not successfully applied:\n%s.", PrefabDomUtils::PrefabDomValueToString(m_redoPatch).c_str());

            // Remove the link ids if present in the DOMs. We don't want any overrides to be created on top of linkIds because
            // linkIds are not persistent and will be created dynamically when prefabs are loaded into the editor.
            instanceDom.RemoveMember(PrefabDomUtils::LinkIdName);
            sourceDom.RemoveMember(PrefabDomUtils::LinkIdName);

            //Diff the instance against the source template.
            PrefabDom linkPatch;
            m_instanceToTemplateInterface->GeneratePatch(linkPatch, sourceDom, instanceDom);

            // Create a copy of linkPatch by providing the allocator of m_redoPatch so that the patch doesn't become invalid when
            // the patch goes out of scope in this function.
            PrefabDom linkPatchCopy;
            linkPatchCopy.CopyFrom(linkPatch, m_redoPatch.GetAllocator());
            m_redoPatch.CopyFrom(m_undoPatch, m_redoPatch.GetAllocator());
            if (auto patchesIter = m_redoPatch.FindMember(PrefabDomUtils::PatchesName); patchesIter == m_redoPatch.MemberEnd())
            {
                m_redoPatch.AddMember(
                    rapidjson::GenericStringRef(PrefabDomUtils::PatchesName), AZStd::move(linkPatchCopy), m_redoPatch.GetAllocator());
            }
            else
            {
                patchesIter->value = AZStd::move(linkPatchCopy.GetArray());
            }
        }

        void PrefabUndoAddEntity::UpdateLink(PrefabDom& linkDom)
        {
            AZ_Assert(m_link.has_value(), "Link not found");

            m_link->get().SetLinkDom(linkDom);
            m_link->get().UpdateTarget();

            m_prefabSystemComponentInterface->PropagateTemplateChanges(m_templateId);
            m_prefabSystemComponentInterface->SetTemplateDirtyFlag(m_templateId, true);
        }

        void PrefabUndoAddEntity::UpdateCachedOwningInstanceDOM(
            PrefabDomReference cachedOwningInstanceDom,
            const PrefabDom& entityDom,
            const AZStd::string& entityAliasPath)
        {
            // Create a copy of the DOM of the end state so that it shares the lifecycle of the cached DOM.
            PrefabDom endStateCopy;
            endStateCopy.CopyFrom(entityDom, cachedOwningInstanceDom->get().GetAllocator());
            PrefabDomPath entityPathInDom(entityAliasPath.c_str());

            // Update the cached instance DOM corresponding to the entity so that the same modified entity isn't reloaded again.
            entityPathInDom.Set(cachedOwningInstanceDom->get(), AZStd::move(endStateCopy));
        }
    }
}
