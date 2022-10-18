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

            InstanceOptionalReference findOwningInstanceResult =
                m_instanceEntityMapperInterface->FindOwningInstance(parentEntityId);
            AZ_Assert(findOwningInstanceResult.has_value(),
                "Can't find owning instance of parent entity with id '%llu'.",
                static_cast<AZ::u64>(parentEntityId));
            Instance& owningInstance = findOwningInstanceResult->get();

            const AZStd::string focusedToOwningInstancePath =
                GenerateFocusedToOwningInstanceRelativePath(focusedInstance, owningInstance);

            // Create redo/undo patches for parent entity updated while adding new entity under it.
            AZStd::string parentEntityAliasPath =
                m_instanceToTemplateInterface->GenerateEntityAliasPath(parentEntityId);
            AZ_Assert(!parentEntityAliasPath.empty(),
                "Alias path of parent entity with id '%llu' shouldn't be empty.",
                static_cast<AZ::u64>(parentEntityId));

            PrefabDom parentEntityDomAfterAddingEntity;
            m_instanceToTemplateInterface->GenerateDomForEntity(parentEntityDomAfterAddingEntity, parentEntity);

            GenerateUpdateParentEntityUndoPatches(parentEntityDomAfterAddingEntity,
                focusedToOwningInstancePath + parentEntityAliasPath);

            // Create redo/undo patches for new entity.
            AZStd::string newEntityAliasPath =
                m_instanceToTemplateInterface->GenerateEntityAliasPath(newEntityId);
            AZ_Assert(!newEntityAliasPath.empty(),
                "Alias path of new entity with id '%llu' shouldn't be empty.",
                static_cast<AZ::u64>(newEntityId));

            PrefabDom newEntityDom;
            m_instanceToTemplateInterface->GenerateDomForEntity(newEntityDom, newEntity);

            GenerateAddNewEntityUndoPatches(newEntityDom,
                focusedToOwningInstancePath + newEntityAliasPath);

            // Preemptively updates the cached DOM to prevent reloading instance DOM.
            PrefabDomReference cachedOwningInstanceDom = owningInstance.GetCachedInstanceDom();
            AZ_Assert(cachedOwningInstanceDom.has_value(),
                "Can't find cached DOM of parent entity's owning instance. "
                "Parent entity id: '%llu'.", static_cast<AZ::u64>(parentEntityId));

            UpdateCachedOwningInstanceDOM(cachedOwningInstanceDom,
                parentEntityDomAfterAddingEntity, parentEntityAliasPath);
            UpdateCachedOwningInstanceDOM(cachedOwningInstanceDom,
                newEntityDom, newEntityAliasPath);
        }

        void PrefabUndoAddEntity::GenerateUpdateParentEntityUndoPatches(
            const PrefabDom& parentEntityDomAfterAddingEntity,
            const AZStd::string& parentEntityAliasPath)
        {
            // Get the alias of the parent in the focused template's DOM.
            PrefabDomPath entityPathInFocusedTemplate(parentEntityAliasPath.c_str());
            PrefabDom& focusedTemplateDom =
                m_prefabSystemComponentInterface->FindTemplateDom(m_templateId);

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

            m_instanceToTemplateInterface->AppendEntityAliasPathToPatchPaths(m_redoPatch, parentEntityAliasPath);
            m_instanceToTemplateInterface->AppendEntityAliasPathToPatchPaths(m_undoPatch, parentEntityAliasPath);
        }

        void PrefabUndoAddEntity::GenerateAddNewEntityUndoPatches(
            const PrefabDom& newEntityDom,
            const AZStd::string& newEntityAliasPath)
        {
            PrefabDomValue addNewEntityRedoPatch(rapidjson::kObjectType);
            rapidjson::Value path = rapidjson::Value(newEntityAliasPath.data(), aznumeric_caster(newEntityAliasPath.length()), m_redoPatch.GetAllocator());
            rapidjson::Value patchValue;
            patchValue.CopyFrom(newEntityDom, m_redoPatch.GetAllocator(), true);
            addNewEntityRedoPatch.AddMember(rapidjson::StringRef("op"), rapidjson::StringRef("add"), m_redoPatch.GetAllocator())
                .AddMember(rapidjson::StringRef("path"), AZStd::move(path), m_redoPatch.GetAllocator())
                .AddMember(rapidjson::StringRef("value"), AZStd::move(patchValue), m_redoPatch.GetAllocator());
            m_redoPatch.PushBack(addNewEntityRedoPatch.Move(), m_redoPatch.GetAllocator());

            PrefabDomValue addNewEntityUndoPatch(rapidjson::kObjectType);
            path = rapidjson::Value(newEntityAliasPath.data(), aznumeric_caster(newEntityAliasPath.length()), m_undoPatch.GetAllocator());
            addNewEntityUndoPatch.AddMember(rapidjson::StringRef("op"), rapidjson::StringRef("remove"), m_undoPatch.GetAllocator())
                .AddMember(rapidjson::StringRef("path"), AZStd::move(path), m_undoPatch.GetAllocator());
            m_undoPatch.PushBack(addNewEntityUndoPatch.Move(), m_undoPatch.GetAllocator());
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

        AZStd::string PrefabUndoAddEntity::GenerateFocusedToOwningInstanceRelativePath(
            const Instance& focusedInstance, const Instance& owningInstance)
        {
            InstanceClimbUpResult climbUpResult =
                PrefabInstanceUtils::ClimbUpToTargetOrRootInstance(owningInstance, &focusedInstance);
            AZ_Assert(climbUpResult.m_isTargetInstanceReached,
                "Owning prefab instance should be owned by a descendant of the focused prefab instance.");
            
            return PrefabInstanceUtils::GetRelativePathFromClimbedInstances(climbUpResult.m_climbedInstances);
        }
    }
}
