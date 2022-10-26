/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Interface/Interface.h>
#include <AzToolsFramework/Prefab/PrefabSystemComponentInterface.h>
#include <AzToolsFramework/Prefab/Undo/PrefabUndoUpdateLink.h>
#include <AzToolsFramework/Prefab/Undo/PrefabUndoUtils.h>

namespace AzToolsFramework
{
    namespace Prefab
    {
        PrefabUndoUpdateLink::PrefabUndoUpdateLink(const AZStd::string& undoOperationName)
            : PrefabUndoBase(undoOperationName)
        {
            m_prefabSystemComponentInterface = AZ::Interface<PrefabSystemComponentInterface>::Get();
            AZ_Assert(m_prefabSystemComponentInterface,
                "PrefabUndoUpdateLink - Failed to grab PrefabSystemComponentInterface");
        }

        void PrefabUndoUpdateLink::Undo()
        {
            UpdateLink(m_undoPatch);
        }

        void PrefabUndoUpdateLink::Redo()
        {
            UpdateLink(m_redoPatch);
        }

        void PrefabUndoUpdateLink::SetLink(LinkId linkId)
        {
            m_link = m_prefabSystemComponentInterface->FindLink(linkId);
            AZ_Assert(m_link.has_value(),
                "PrefabUndoUpdateLink::Capture - Link with id '%llu' not found",
                static_cast<AZ::u64>(linkId));

            m_templateId = m_link->get().GetTargetTemplateId();
            AZ_Assert(m_templateId != InvalidTemplateId,
                "PrefabUndoUpdateLink::Capture - Link with id '%llu' contains incorrect target template id",
                static_cast<AZ::u64>(linkId));
        }

        void PrefabUndoUpdateLink::Capture(const PrefabDom& linkedInstancePatch, LinkId linkId)
        {
            SetLink(linkId);
            Capture(linkedInstancePatch);
        }

        void PrefabUndoUpdateLink::Capture(const PrefabDom& linkedInstancePatch)
        {
            AZ_Assert(m_link.has_value(),
                "PrefabUndoUpdateLink::Capture - m_link is still invalid");

            //Cache current link DOM for undo link update.
            m_undoPatch.CopyFrom(m_link->get().GetLinkDom(), m_undoPatch.GetAllocator());

            //Get DOM of the link's source template.
            TemplateId sourceTemplateId = m_link->get().GetSourceTemplateId();
            TemplateReference sourceTemplate = m_prefabSystemComponentInterface->FindTemplate(sourceTemplateId);
            AZ_Assert(sourceTemplate.has_value(),
                "PrefabUndoUpdateLink::GenerateUndoUpdateLinkPatches - Source template not found");
            PrefabDom& sourceDom = sourceTemplate->get().GetPrefabDom();

            //Get DOM of instance that the link points to.
            PrefabDomValueReference instanceDomRef = m_link->get().GetLinkedInstanceDom();
            AZ_Assert(instanceDomRef.has_value(),
                "PrefabUndoUpdateLink::GenerateUndoUpdateLinkPatches -Linked Instance DOM not found");
            PrefabDom instanceDom;
            instanceDom.CopyFrom(instanceDomRef->get(), instanceDom.GetAllocator());

            //Apply patch for adding new entity under parent entity to the instance.
            [[maybe_unused]] AZ::JsonSerializationResult::ResultCode result = PrefabDomUtils::ApplyPatches(
                instanceDom, instanceDom.GetAllocator(), linkedInstancePatch);
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

        void PrefabUndoUpdateLink::UpdateLink(const PrefabDom& linkDom)
        {
            AZ_Assert(m_link.has_value(),
                "PrefabUndoUpdateLink::UpdateLink - m_link is still invalid");

            m_link->get().SetLinkDom(linkDom);
            m_link->get().UpdateTarget();

            m_prefabSystemComponentInterface->PropagateTemplateChanges(m_templateId);
            m_prefabSystemComponentInterface->SetTemplateDirtyFlag(m_templateId, true);
        }
    }
}
