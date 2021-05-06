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
#include <Prefab/PrefabDomUtils.h>

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
            const PrefabDom& initialState,
            const PrefabDom& endState,
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
            m_instanceToTemplateInterface->AppendEntityAliasToPatchPaths(m_redoPatch, entityId);
            m_instanceToTemplateInterface->GeneratePatch(m_undoPatch, endState, initialState);
            m_instanceToTemplateInterface->AppendEntityAliasToPatchPaths(m_undoPatch, entityId);
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
            [[maybe_unused]] bool isPatchApplicationSuccessful =
                m_instanceToTemplateInterface->PatchTemplate(m_redoPatch, m_templateId);

            AZ_Error(
                "Prefab", isPatchApplicationSuccessful,
                "Applying the redo patch on the entity with alias '%s' in template with id '%llu' was unsuccessful", m_entityAlias.c_str(),
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
            m_prefabSystemComponentInterface = AZ::Interface<PrefabSystemComponentInterface>::Get();
            AZ_Assert(m_instanceToTemplateInterface, "Failed to grab interface");
        }

        void PrefabUndoInstanceLink::Capture(
            const TemplateId& targetId,
            const TemplateId& sourceId,
            const InstanceAlias& instanceAlias,
            PrefabDomReference linkPatches,
            const LinkId linkId)
        {
            m_targetId = targetId;
            m_sourceId = sourceId;
            m_instanceAlias = instanceAlias;
            m_linkId = linkId;

            if (linkPatches.has_value())
            {
                m_linkPatches = AZStd::move(linkPatches->get());
            }

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

        //PrefabUndoLinkUpdate
        PrefabUndoLinkUpdate::PrefabUndoLinkUpdate(const AZStd::string& undoOperationName)
            : PrefabUndoBase(undoOperationName)
            , m_linkId(InvalidLinkId)
            , m_linkDomNext(PrefabDom())
            , m_linkDomPrevious(PrefabDom())
        {
            m_prefabSystemComponentInterface = AZ::Interface<PrefabSystemComponentInterface>::Get();
            AZ_Assert(m_instanceToTemplateInterface, "Failed to grab interface");
        }

        void PrefabUndoLinkUpdate::Capture(
            const PrefabDom& patch,
            const LinkId linkId)
        {
            m_linkId = linkId;

            //acquire link and existing values
            LinkReference link = m_prefabSystemComponentInterface->FindLink(m_linkId);
            if (link == AZStd::nullopt)
            {
                AZ_Error("Prefab", false, "PrefabUndoLinkUpdate: Link not found");
                return;
            }

            if (link.has_value())
            {
                m_linkDomPrevious = AZStd::move(link->get().GetLinkDom());
            }

            //get source templateDom
            TemplateReference sourceTemplate = m_prefabSystemComponentInterface->FindTemplate(link->get().GetSourceTemplateId());

            if (sourceTemplate == AZStd::nullopt)
            {
                AZ_Error("Prefab", false, "PrefabUndoLinkUpdate: Source template not found");
                return;
            }

            PrefabDomReference sourceDom = sourceTemplate->get().GetPrefabDom();

            //use instance pointer to reach position
            PrefabDomValueReference instanceDomRef = link->get().GetLinkedInstanceDom();

            //copy the target instance the link is pointing to
            PrefabDom instanceDom;
            instanceDom.CopyFrom(instanceDomRef->get(), instanceDom.GetAllocator());

            //apply the patch to the template within the target
            AZ::JsonSerializationResult::ResultCode result = AZ::JsonSerialization::ApplyPatch(instanceDom,
                instanceDom.GetAllocator(), patch, AZ::JsonMergeApproach::JsonPatch);

            //remove the link id placed into the instance
            auto linkIdIter = instanceDom.FindMember(PrefabDomUtils::LinkIdName);
            if (linkIdIter != instanceDom.MemberEnd())
            {
                instanceDom.RemoveMember(PrefabDomUtils::LinkIdName);
            }

            //we use this to diff our copy against the vanilla template (source template)
            PrefabDom patchLink;
            m_instanceToTemplateInterface->GeneratePatch(patchLink, sourceDom->get(), instanceDom);

            // Create a copy of patchLink by providing the allocator of m_linkDomNext so that the patch doesn't become invalid when
            // the patch goes out of scope in this function.
            PrefabDom patchLinkCopy;
            patchLinkCopy.CopyFrom(patchLink, m_linkDomNext.GetAllocator());

            m_linkDomNext.CopyFrom(m_linkDomPrevious, m_linkDomNext.GetAllocator());
            auto patchesIter = m_linkDomNext.FindMember(PrefabDomUtils::PatchesName);

            if (patchesIter == m_linkDomNext.MemberEnd())
            {
                m_linkDomNext.AddMember(
                    rapidjson::GenericStringRef(PrefabDomUtils::PatchesName), patchLinkCopy, m_linkDomNext.GetAllocator());
            }
            else
            {
                patchesIter->value = AZStd::move(patchLinkCopy.GetArray());
            }
        }

        void PrefabUndoLinkUpdate::Undo()
        {
            UpdateLink(m_linkDomPrevious);
        }

        void PrefabUndoLinkUpdate::Redo()
        {
            UpdateLink(m_linkDomNext);
        }

        void PrefabUndoLinkUpdate::UpdateLink(PrefabDom& linkDom)
        {
            LinkReference link = m_prefabSystemComponentInterface->FindLink(m_linkId);

            if (link == AZStd::nullopt)
            {
                AZ_Error("Prefab", false, "PrefabUndoLinkUpdate: Link not found");
                return;
            }

            PrefabDom moveLink;
            moveLink.CopyFrom(linkDom, linkDom.GetAllocator());
            link->get().GetLinkDom() = AZStd::move(moveLink);

            //propagate the link changes
            link->get().UpdateTarget();
            m_prefabSystemComponentInterface->PropagateTemplateChanges(link->get().GetTargetTemplateId());

            //mark as dirty
            m_prefabSystemComponentInterface->SetTemplateDirtyFlag(link->get().GetTargetTemplateId(), true);
        }
    }
}
