/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Interface/Interface.h>
#include <AzToolsFramework/Prefab/PrefabSystemComponentInterface.h>
#include <Prefab/Undo/PrefabUndoAddEntity.h>

namespace AzToolsFramework
{
    namespace Prefab
    {
        PrefabUndoAddEntity::PrefabUndoAddEntity(const AZStd::string& undoOperationName)
            : PrefabUndoBase(undoOperationName)
        {
            m_prefabSystemComponentInterface = AZ::Interface<PrefabSystemComponentInterface>::Get();
            AZ_Assert(m_prefabSystemComponentInterface, "Failed to grab PrefabSystemComponentInterface");
        }

        void PrefabUndoAddEntity::Capture(
            const AZ::Entity& parentEntity,
            const AZ::Entity& newEntity,
            TemplateId focusedTemplateId,
            const AZStd::string& focusedToOwningInstancePath,
            PrefabDomReference cachedInstanceDom)
        {
            m_templateId = focusedTemplateId;

            // Create redo/undo patches for parent entity updated while adding new entity under it.
            AZStd::string parentEntityAliasPath =
                m_instanceToTemplateInterface->GenerateEntityAliasPath(parentEntity.GetId());

            PrefabDom parentEntityDomAfterAddingEntity;
            m_instanceToTemplateInterface->GenerateDomForEntity(parentEntityDomAfterAddingEntity, parentEntity);

            GenerateUpdateParentEntityUndoPatches(parentEntityDomAfterAddingEntity,
                focusedToOwningInstancePath + parentEntityAliasPath);

            // Create redo/undo patches for new entity.
            AZStd::string newEntityAliasPath =
                m_instanceToTemplateInterface->GenerateEntityAliasPath(newEntity.GetId());

            PrefabDom newEntityDom;
            m_instanceToTemplateInterface->GenerateDomForEntity(newEntityDom, newEntity);

            GenerateAddNewEntityUndoPatches(newEntityDom,
                focusedToOwningInstancePath + newEntityAliasPath);

            // Preemptively updates the cached DOM to prevent reloading instance DOM.
            if (cachedInstanceDom.has_value())
            {
                // Create a copy of the DOM of the end state so that it shares the lifecycle of the cached DOM.
                if (!parentEntityAliasPath.empty())
                {
                    PrefabDom endStateCopy;
                    endStateCopy.CopyFrom(parentEntityDomAfterAddingEntity, cachedInstanceDom->get().GetAllocator());
                    PrefabDomPath parentEntityPathInDom(parentEntityAliasPath.c_str());

                    // Update the cached instance DOM corresponding to the entity so that the same modified entity isn't reloaded again.
                    parentEntityPathInDom.Set(cachedInstanceDom->get(), AZStd::move(endStateCopy));
                }

                if (!newEntityAliasPath.empty())
                {
                    PrefabDom endStateCopy;
                    endStateCopy.CopyFrom(newEntityDom, cachedInstanceDom->get().GetAllocator());
                    PrefabDomPath newEntityPathInDom(newEntityAliasPath.c_str());

                    newEntityPathInDom.Set(cachedInstanceDom->get(), AZStd::move(endStateCopy));
                }
            }
        }

        void PrefabUndoAddEntity::Undo()
        {
            m_instanceToTemplateInterface->PatchTemplate(m_undoPatch, m_templateId);
        }

        void PrefabUndoAddEntity::Redo()
        {
            m_instanceToTemplateInterface->PatchTemplate(m_redoPatch, m_templateId);
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
                AZ_Assert(parentEntityDomInFocusedTemplate, "Could not load parent entity's DOM from the focused template's DOM.");

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
    }
}
