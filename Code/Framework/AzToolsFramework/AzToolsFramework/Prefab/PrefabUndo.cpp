/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
        {
            m_instanceToTemplateInterface = AZ::Interface<InstanceToTemplateInterface>::Get();
            AZ_Assert(m_instanceToTemplateInterface, "Failed to grab instance to template interface");
        }

        //PrefabInstanceUndo
        PrefabUndoInstance::PrefabUndoInstance(const AZStd::string& undoOperationName, bool useImmediatePropagation)
            : PrefabUndoBase(undoOperationName)
        {
            m_useImmediatePropagation = useImmediatePropagation;
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

        void PrefabUndoInstance::Undo()
        {
            m_instanceToTemplateInterface->PatchTemplate(m_undoPatch, m_templateId, m_useImmediatePropagation);
        }

        void PrefabUndoInstance::Redo()
        {
            m_instanceToTemplateInterface->PatchTemplate(m_redoPatch, m_templateId, m_useImmediatePropagation);
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
            m_instanceToTemplateInterface->GeneratePatch(m_redoPatch, initialState, endState);
            m_instanceToTemplateInterface->AppendEntityAliasToPatchPaths(m_redoPatch, entityId);
            m_instanceToTemplateInterface->GeneratePatch(m_undoPatch, endState, initialState);
            m_instanceToTemplateInterface->AppendEntityAliasToPatchPaths(m_undoPatch, entityId);
        }

        void PrefabUndoEntityUpdate::Undo()
        {
            [[maybe_unused]] bool isPatchApplicationSuccessful =
                m_instanceToTemplateInterface->PatchTemplate(m_undoPatch, m_templateId, true);

            AZ_Error(
                "Prefab", isPatchApplicationSuccessful,
                "Applying the undo patch on the entity with alias '%s' in template with id '%llu' was unsuccessful", m_entityAlias.c_str(),
                m_templateId);
        }

        void PrefabUndoEntityUpdate::Redo()
        {
            [[maybe_unused]] bool isPatchApplicationSuccessful =
                m_instanceToTemplateInterface->PatchTemplate(m_redoPatch, m_templateId, true);

            AZ_Error(
                "Prefab", isPatchApplicationSuccessful,
                "Applying the redo patch on the entity with alias '%s' in template with id '%llu' was unsuccessful", m_entityAlias.c_str(),
                m_templateId);
        }

        void PrefabUndoEntityUpdate::Redo(InstanceOptionalReference instanceToExclude)
        {
            [[maybe_unused]] bool isPatchApplicationSuccessful =
                m_instanceToTemplateInterface->PatchTemplate(m_redoPatch, m_templateId, false, instanceToExclude);

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
            m_prefabSystemComponentInterface = AZ::Interface<PrefabSystemComponentInterface>::Get();
            AZ_Assert(m_instanceToTemplateInterface, "Failed to grab interface");
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
                m_linkDomPrevious.CopyFrom(link->get().GetLinkDom(), m_linkDomPrevious.GetAllocator());
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
            [[maybe_unused]] AZ::JsonSerializationResult::ResultCode result = PrefabDomUtils::ApplyPatches(instanceDom, instanceDom.GetAllocator(), patch);

            AZ_Error(
                "Prefab",
                (result.GetOutcome() != AZ::JsonSerializationResult::Outcomes::Skipped) &&
                (result.GetOutcome() != AZ::JsonSerializationResult::Outcomes::PartialSkip),
                "Some of the patches are not successfully applied.");

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
                    rapidjson::GenericStringRef(PrefabDomUtils::PatchesName), AZStd::move(patchLinkCopy), m_linkDomNext.GetAllocator());
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

        void PrefabUndoLinkUpdate::Redo(InstanceOptionalReference instanceToExclude)
        {
            UpdateLink(m_linkDomNext, instanceToExclude);
        }

        void PrefabUndoLinkUpdate::UpdateLink(PrefabDom& linkDom, InstanceOptionalReference instanceToExclude)
        {
            LinkReference link = m_prefabSystemComponentInterface->FindLink(m_linkId);

            if (link == AZStd::nullopt)
            {
                AZ_Error("Prefab", false, "PrefabUndoLinkUpdate: Link not found");
                return;
            }

            link->get().SetLinkDom(linkDom);

            //propagate the link changes
            link->get().UpdateTarget();
            m_prefabSystemComponentInterface->PropagateTemplateChanges(link->get().GetTargetTemplateId(), false, instanceToExclude);

            //mark as dirty
            m_prefabSystemComponentInterface->SetTemplateDirtyFlag(link->get().GetTargetTemplateId(), true);
        }
    }
}
