/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Interface/Interface.h>
#include <AzToolsFramework/Prefab/PrefabSystemComponentInterface.h>
#include <Prefab/PrefabUndoAddEntity.h>

namespace AzToolsFramework
{
    namespace Prefab
    {
        PrefabUndoAddEntity::ParentEntityInfo::ParentEntityInfo(
            AZ::EntityId parentEntityId,
            const PrefabDomValue& parentEntityDomBeforeAddingEntity,
            const PrefabDomValue& parentEntityDomAfterAddingEntity)
            : m_parentEntityId(parentEntityId)
            , m_parentEntityDomBeforeAddingEntity(parentEntityDomBeforeAddingEntity)
            , m_parentEntityDomAfterAddingEntity(parentEntityDomAfterAddingEntity)
        {
        }

        PrefabUndoAddEntity::NewEntityInfo::NewEntityInfo(
            AZ::EntityId newEntityId,
            const PrefabDomValue& newEntityDom)
            : m_newEntityId(newEntityId)
            , m_newEntityDom(newEntityDom)
        {
        }

        PrefabUndoAddEntity::PrefabUndoAddEntity(const AZStd::string& undoOperationName)
            : PrefabUndoBase(undoOperationName)
        {
            m_prefabSystemComponentInterface = AZ::Interface<PrefabSystemComponentInterface>::Get();
            AZ_Assert(m_prefabSystemComponentInterface, "Failed to grab interface");
        }

        void PrefabUndoAddEntity::Capture(
            ParentEntityInfo parentEntityInfo,
            NewEntityInfo newEntityInfo,
            PrefabDomReference cachedInstanceDom,
            TemplateId templateId,
            AZStd::string entityAliasPathPrefix)
        {
            m_templateId = templateId;

            AZStd::string newEntityAliasPath =
                m_instanceToTemplateInterface->GenerateEntityAliasPath(newEntityInfo.m_newEntityId, entityAliasPathPrefix);
            AZStd::string parentEntityAliasPath =
                m_instanceToTemplateInterface->GenerateEntityAliasPath(parentEntityInfo.m_parentEntityId, AZStd::move(entityAliasPathPrefix));

            // Create redo patch.
            m_redoPatch.SetArray();

            PrefabDomValue addNewEntityRedoPatch(rapidjson::kObjectType);
            rapidjson::Value path = rapidjson::Value(newEntityAliasPath.data(), aznumeric_caster(newEntityAliasPath.length()), m_redoPatch.GetAllocator());
            rapidjson::Value patchValue;
            patchValue.CopyFrom(newEntityInfo.m_newEntityDom, m_redoPatch.GetAllocator(), true);
            addNewEntityRedoPatch.AddMember(rapidjson::StringRef("op"), rapidjson::StringRef("add"), m_redoPatch.GetAllocator())
                .AddMember(rapidjson::StringRef("path"), AZStd::move(path), m_redoPatch.GetAllocator())
                .AddMember(rapidjson::StringRef("value"), AZStd::move(patchValue), m_redoPatch.GetAllocator());
            m_redoPatch.PushBack(addNewEntityRedoPatch.Move(), m_redoPatch.GetAllocator());

            PrefabDom updateParentEntityRedoPatch(rapidjson::kObjectType);
            m_instanceToTemplateInterface->GeneratePatch(
                updateParentEntityRedoPatch, parentEntityInfo.m_parentEntityDomBeforeAddingEntity, parentEntityInfo.m_parentEntityDomAfterAddingEntity);
            m_instanceToTemplateInterface->AppendEntityAliasPathToPatchPaths(updateParentEntityRedoPatch, parentEntityAliasPath);
            m_redoPatch.PushBack(updateParentEntityRedoPatch.Move(), m_redoPatch.GetAllocator());

            // Create undo patch.
            m_undoPatch.SetArray();

            PrefabDomValue addNewEntityUndoPatch(rapidjson::kObjectType);
            path = rapidjson::Value(newEntityAliasPath.data(), aznumeric_caster(newEntityAliasPath.length()), m_undoPatch.GetAllocator());
            addNewEntityUndoPatch.AddMember(rapidjson::StringRef("op"), rapidjson::StringRef("remove"), m_undoPatch.GetAllocator())
                .AddMember(rapidjson::StringRef("path"), AZStd::move(path), m_undoPatch.GetAllocator());
            m_undoPatch.PushBack(addNewEntityUndoPatch.Move(), m_undoPatch.GetAllocator());

            PrefabDom updateParentEntityUndoPatch(rapidjson::kObjectType);
            m_instanceToTemplateInterface->GeneratePatch(updateParentEntityUndoPatch,
                parentEntityInfo.m_parentEntityDomAfterAddingEntity, parentEntityInfo.m_parentEntityDomBeforeAddingEntity);
            m_instanceToTemplateInterface->AppendEntityAliasPathToPatchPaths(updateParentEntityUndoPatch, parentEntityAliasPath);
            m_undoPatch.PushBack(updateParentEntityUndoPatch.Move(), m_undoPatch.GetAllocator());

            // Preemptively updates the cached DOM to prevent reloading instance DOM.
            if (cachedInstanceDom.has_value())
            {
                // Create a copy of the DOM of the end state so that it shares the lifecycle of the cached DOM.
                if (!parentEntityAliasPath.empty())
                {
                    PrefabDom endStateCopy;
                    endStateCopy.CopyFrom(parentEntityInfo.m_parentEntityDomAfterAddingEntity, cachedInstanceDom->get().GetAllocator());
                    Prefab::PrefabDomPath parentEntityPathInDom(parentEntityAliasPath.c_str());

                    // Update the cached instance DOM corresponding to the entity so that the same modified entity isn't reloaded again.
                    parentEntityPathInDom.Set(cachedInstanceDom->get(), AZStd::move(endStateCopy));
                }

                if (!newEntityAliasPath.empty())
                {
                    PrefabDom endStateCopy;
                    endStateCopy.CopyFrom(newEntityInfo.m_newEntityDom, cachedInstanceDom->get().GetAllocator());
                    Prefab::PrefabDomPath newEntityPathInDom(newEntityAliasPath.c_str());

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
    }
}
