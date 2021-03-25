/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include <AzCore/Interface/Interface.h>
#include <AzToolsFramework/Prefab/Instance/Instance.h>

#include <Prefab/PrefabUndo.h>

namespace AzToolsFramework
{
    namespace Prefab
    {
        PrefabUndoBase::PrefabUndoBase(const AZStd::string& undoOperationName)
            : UndoSystem::URSequencePoint(undoOperationName)
            , m_changed(true)
            , m_templateId(InvalidTemplateId)
        {
            m_instanceToTemplateInterface = AZ::Interface<InstanceToTemplateInterface>::Get();
            AZ_Assert(m_instanceToTemplateInterface, "Failed to grab instance to template interface");
        }


        //PrefabInstanceUndo
        PrefabUndoInstance::PrefabUndoInstance(const AZStd::string& undoOperationName)
            : PrefabUndoBase(undoOperationName)
        {
        }

        void PrefabUndoInstance::Capture(
            PrefabDom& initialState,
            PrefabDom& endState,
            const TemplateId& templateId)
        {
            m_templateId = templateId;

            m_instanceToTemplateInterface->GeneratePatch(m_redoPatch, initialState, endState);
            m_instanceToTemplateInterface->GeneratePatch(m_undoPatch, endState, initialState);
        }

        void PrefabUndoInstance::Undo()
        {
            m_instanceToTemplateInterface->PatchTemplate(m_undoPatch, m_templateId);
        }

        void PrefabUndoInstance::Redo()
        {
            m_instanceToTemplateInterface->PatchTemplate(m_redoPatch, m_templateId);
        }


        //PrefabEntityUpdateUndo
        PrefabUndoEntityUpdate::PrefabUndoEntityUpdate(const AZStd::string& undoOperationName)
            : PrefabUndoBase(undoOperationName)
        {
            m_instanceEntityMapperInterface = AZ::Interface<InstanceEntityMapperInterface>::Get();
            AZ_Assert(m_instanceEntityMapperInterface, "Failed to grab instance entity mapper interface");
        }

        void PrefabUndoEntityUpdate::Capture(
            PrefabDom& initialState,
            PrefabDom& endState,
            const AZ::EntityId& entityId)
        {
            //get the entity alias for future undo/redo
            InstanceOptionalReference instanceOptionalReference = m_instanceEntityMapperInterface->FindOwningInstance(entityId);
            AZ_Error("Prefab", instanceOptionalReference,
                "Failed to find an owning instance for the entity with id %llu.", static_cast<AZ::u64>(entityId));
            Instance& instance = instanceOptionalReference->get();
            m_templateId = instance.GetTemplateId();
            m_entityAlias = (instance.GetEntityAlias(entityId)).value();

            //generate undo/redo patches
            m_instanceToTemplateInterface->GeneratePatch(m_redoPatch, initialState, endState);
            m_instanceToTemplateInterface->GeneratePatch(m_undoPatch, endState, initialState);
        }

        void PrefabUndoEntityUpdate::Undo()
        {
            m_instanceToTemplateInterface->PatchEntityInTemplate(m_undoPatch, m_entityAlias, m_templateId);
        }

        void PrefabUndoEntityUpdate::Redo()
        {
            m_instanceToTemplateInterface->PatchEntityInTemplate(m_redoPatch, m_entityAlias, m_templateId);
        }

        //PrefabInstanceLinkUndo
        PrefabUndoInstanceLink::PrefabUndoInstanceLink(const AZStd::string& undoOperationName)
            : PrefabUndoBase(undoOperationName)
            , m_targetId(InvalidTemplateId)
            , m_sourceId(InvalidTemplateId)
            , m_instanceAlias("")
            , m_linkId(InvalidLinkId)
            , m_link(Link())
            , m_linkStatus(LinkStatus::LINKSTATUS)
        {
            m_prefabSystemComponentInterface = AZ::Interface<PrefabSystemComponentInterface>::Get();
            AZ_Assert(m_instanceToTemplateInterface, "Failed to grab interface");
        }

        void PrefabUndoInstanceLink::Capture(
            const TemplateId& targetId,
            const TemplateId& sourceId,
            const InstanceAlias& instanceAlias,
            const LinkId& linkId,
            const Link& link)
        {
            m_targetId = targetId;
            m_sourceId = sourceId;
            m_instanceAlias = instanceAlias;
            m_linkId = linkId;
            m_link = link;

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

            m_prefabSystemComponentInterface->PropagateTemplateChanges(m_targetId);
        }

        void PrefabUndoInstanceLink::AddLink()
        {
            m_linkId = m_prefabSystemComponentInterface->CreateLink(m_targetId, m_sourceId, m_instanceAlias, m_linkId);

            //if data already exists, repopulate
            if (m_linkStatus == LinkStatus::REMOVE)
            {
                LinkReference link = m_prefabSystemComponentInterface->FindLink(m_linkId);
                link = m_link;
            }         
        }

        void PrefabUndoInstanceLink::RemoveLink()
        {
            m_prefabSystemComponentInterface->RemoveLink(m_linkId);
        }
    }
}
