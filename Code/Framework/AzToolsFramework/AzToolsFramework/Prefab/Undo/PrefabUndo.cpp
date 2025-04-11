/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/Prefab/Instance/InstanceToTemplateInterface.h>
#include <AzToolsFramework/Prefab/PrefabDomUtils.h>
#include <AzToolsFramework/Prefab/PrefabSystemComponentInterface.h>
#include <AzToolsFramework/Prefab/Undo/PrefabUndo.h>
#include <AzToolsFramework/Prefab/Undo/PrefabUndoUtils.h>

namespace AzToolsFramework
{
    namespace Prefab
    {
        // PrefabInstanceUndo

        PrefabUndoInstance::PrefabUndoInstance(const AZStd::string& undoOperationName)
            : PrefabUndoBase(undoOperationName)
        {
        }

        void PrefabUndoInstance::Capture(
            const PrefabDom& initialState,
            const PrefabDom& endState,
            TemplateId templateId)
        {
            m_templateId = templateId;

            m_instanceToTemplateInterface->GeneratePatch(m_redoPatch, initialState, endState);
            m_instanceToTemplateInterface->GeneratePatch(m_undoPatch, endState, initialState);
        }

        // PrefabUndoAddEntityDoms

        PrefabUndoAddEntityDoms::PrefabUndoAddEntityDoms(const AZStd::string& undoOperationName)
            : PrefabUndoBase(undoOperationName)
        {
        }

        void PrefabUndoAddEntityDoms::Capture(const AZStd::vector<const AZ::Entity*>& entityList, TemplateId templateId)
        {
            m_templateId = templateId;

            for (const AZ::Entity* entity : entityList)
            {
                if (entity)
                {
                    PrefabDom entityDom;
                    m_instanceToTemplateInterface->GenerateEntityDomBySerializing(entityDom, *entity);

                    const AZStd::string& entityAliasPath = m_instanceToTemplateInterface->GenerateEntityAliasPath(entity->GetId());
                    PrefabUndoUtils::AppendAddEntityPatch(m_redoPatch, entityDom, entityAliasPath);
                    PrefabUndoUtils::AppendRemovePatch(m_undoPatch, entityAliasPath);
                }
            }
        }

        // PrefabUndoRemoveEntityDoms

        PrefabUndoRemoveEntityDoms::PrefabUndoRemoveEntityDoms(const AZStd::string& undoOperationName)
            : PrefabUndoBase(undoOperationName)
        {
        }

        void PrefabUndoRemoveEntityDoms::Capture(
            const AZStd::vector<AZStd::pair<const PrefabDomValue*, AZStd::string>>& entityDomAndPathList, TemplateId templateId)
        {
            m_templateId = templateId;

            for (const auto& entityDomAndPath : entityDomAndPathList)
            {
                const PrefabDomValue* entityDomValue = entityDomAndPath.first;
                if (entityDomValue)
                {
                    const AZStd::string& entityAliasPath = entityDomAndPath.second;
                    PrefabUndoUtils::AppendRemovePatch(m_redoPatch, entityAliasPath);
                    PrefabUndoUtils::AppendAddEntityPatch(m_undoPatch, *entityDomValue, entityAliasPath);
                }
            }
        }

        // PrefabUndoEntityUpdate

        PrefabUndoEntityUpdate::PrefabUndoEntityUpdate(const AZStd::string& undoOperationName)
            : PrefabUndoBase(undoOperationName)
        {
            m_instanceEntityMapperInterface = AZ::Interface<InstanceEntityMapperInterface>::Get();
            AZ_Assert(m_instanceEntityMapperInterface, "Failed to grab instance entity mapper interface");
        }

        void PrefabUndoEntityUpdate::Capture(
            const PrefabDomValue& initialState, const PrefabDomValue& endState, AZ::EntityId entityId, bool updateCache)
        {
            //get the entity alias for future undo/redo
            auto instanceReference = m_instanceEntityMapperInterface->FindOwningInstance(entityId);
            AZ_Error("Prefab", instanceReference,
                "Failed to find an owning instance for the entity with id %llu.", static_cast<AZ::u64>(entityId));
            Instance& instance = instanceReference->get();
            m_templateId = instance.GetTemplateId();
            auto aliasReference = instance.GetEntityAlias(entityId);
            if (!aliasReference.has_value())
            {
                AZ_Error(
                    "Prefab", aliasReference.has_value(), "Failed to find the entity alias for entity %s.", entityId.ToString().c_str());
                return;
            }

            m_entityAlias = aliasReference.value();

            //generate undo/redo patches
            const AZStd::string& entityAliasPath = m_instanceToTemplateInterface->GenerateEntityAliasPath(entityId);
            PrefabUndoUtils::GenerateAndAppendPatch(m_redoPatch, initialState, endState, entityAliasPath);
            PrefabUndoUtils::GenerateAndAppendPatch(m_undoPatch, endState, initialState, entityAliasPath);

            // Preemptively updates the cached DOM to prevent reloading instance DOM.
            if (updateCache)
            {
                PrefabDomReference cachedOwningInstanceDom = instance.GetCachedInstanceDom();
                if (cachedOwningInstanceDom.has_value())
                {
                    PrefabUndoUtils::UpdateEntityInPrefabDom(cachedOwningInstanceDom, endState, entityAliasPath);
                }
            }
        }

        void PrefabUndoEntityUpdate::Undo()
        {
            [[maybe_unused]] bool isPatchApplicationSuccessful =
                m_instanceToTemplateInterface->PatchTemplate(m_undoPatch, m_templateId);

            AZ_Error(
                "Prefab", isPatchApplicationSuccessful,
                "Applying the undo patch on the entity with alias '%s' in template with id '%llu' was unsuccessful", m_entityAlias.c_str(),
                m_templateId);
        }

        void PrefabUndoEntityUpdate::Redo()
        {
            Redo(AZStd::nullopt);
        }

        void PrefabUndoEntityUpdate::Redo(InstanceOptionalConstReference instance)
        {
            [[maybe_unused]] bool isPatchApplicationSuccessful =
                m_instanceToTemplateInterface->PatchTemplate(m_redoPatch, m_templateId, instance);

            AZ_Error(
                "Prefab", isPatchApplicationSuccessful,
                "Applying the patch on the entity with alias '%s' in template with id '%llu' was unsuccessful", m_entityAlias.c_str(),
                m_templateId);
        }

        //PrefabInstanceLinkUndo
        PrefabUndoInstanceLink::PrefabUndoInstanceLink(const AZStd::string& undoOperationName)
            : PrefabUndoBase(undoOperationName)
            , m_targetId(InvalidTemplateId)
            , m_sourceId(InvalidTemplateId)
            , m_instanceAlias("")
            , m_linkId(InvalidLinkId)
            , m_linkPatches(PrefabDom())
            , m_linkStatus(LinkStatus::LINKSTATUS)
        {
        }

        void PrefabUndoInstanceLink::Capture(
            TemplateId targetId,
            TemplateId sourceId,
            const InstanceAlias& instanceAlias,
            PrefabDom linkPatches,
            const LinkId linkId)
        {
            m_targetId = targetId;
            m_sourceId = sourceId;
            m_instanceAlias = instanceAlias;
            m_linkId = linkId;

            m_linkPatches = AZStd::move(linkPatches);

            //if linkId is invalid, set as ADD
            if (m_linkId == InvalidLinkId)
            {
                m_linkStatus = LinkStatus::ADD;
            }
            else
            {
                m_linkStatus = LinkStatus::REMOVE;
            }
        }

        void PrefabUndoInstanceLink::Undo()
        {
            switch (m_linkStatus)
            {
            case LinkStatus::ADD:
                RemoveLink();
                break;

            case LinkStatus::REMOVE:
                AddLink();
                break;

            default:
                break;
            }

            m_prefabSystemComponentInterface->PropagateTemplateChanges(m_targetId);
        }

        void PrefabUndoInstanceLink::Redo()
        {
            Redo(AZStd::nullopt);
        }

        void PrefabUndoInstanceLink::Redo(InstanceOptionalConstReference instanceToExclude)
        {
            switch (m_linkStatus)
            {
            case LinkStatus::ADD:
                AddLink();
                break;

            case LinkStatus::REMOVE:
                RemoveLink();
                break;

            default:
                break;
            }

            m_prefabSystemComponentInterface->PropagateTemplateChanges(m_targetId, instanceToExclude);
        }

        LinkId PrefabUndoInstanceLink::GetLinkId()
        {
            return m_linkId;
        }

        void PrefabUndoInstanceLink::AddLink()
        {
            m_linkId = m_prefabSystemComponentInterface->CreateLink(m_targetId, m_sourceId, m_instanceAlias, m_linkPatches, m_linkId);
        }

        void PrefabUndoInstanceLink::RemoveLink()
        {
            m_prefabSystemComponentInterface->RemoveLink(m_linkId);
        }
    }
}
