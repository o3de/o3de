/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Interface/Interface.h>
#include <AzToolsFramework/Prefab/Instance/InstanceToTemplateInterface.h>
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
        }

        void PrefabUndoUpdateLink::Undo()
        {
            UpdateLink(m_undoPatch);
        }

        void PrefabUndoUpdateLink::Redo()
        {
            UpdateLink(m_redoPatch);
        }

        void PrefabUndoUpdateLink::Redo(InstanceOptionalConstReference instanceToExclude)
        {
            UpdateLink(m_redoPatch, instanceToExclude);
        }

        void PrefabUndoUpdateLink::Capture(const PrefabDom& linkedInstancePatch, LinkId linkId)
        {
            m_linkId = linkId;
            LinkReference link = m_prefabSystemComponentInterface->FindLink(m_linkId);
            if (!link.has_value())
            {
                AZ_Error("Prefab", false, "PrefabUndoUpdateLink::Capture - Could not get the link.");
                return;
            }

            m_templateId = link->get().GetTargetTemplateId();

            //Cache current link DOM for undo link update.
            link->get().GetLinkDom(m_undoPatch, m_undoPatch.GetAllocator());

            //Get DOM of the link's source template.
            TemplateId sourceTemplateId = link->get().GetSourceTemplateId();
            TemplateReference sourceTemplate = m_prefabSystemComponentInterface->FindTemplate(sourceTemplateId);
            AZ_Assert(sourceTemplate.has_value(),
                "PrefabUndoUpdateLink::GenerateUndoUpdateLinkPatches - Source template not found");
            PrefabDom& sourceDom = sourceTemplate->get().GetPrefabDom();

            //Get DOM of instance that the link points to.
            PrefabDomValueReference instanceDomRef = link->get().GetLinkedInstanceDom();
            AZ_Assert(instanceDomRef.has_value(),
                "PrefabUndoUpdateLink::GenerateUndoUpdateLinkPatches -Linked Instance DOM not found");
            PrefabDom instanceDom;
            instanceDom.CopyFrom(instanceDomRef->get(), instanceDom.GetAllocator());

            //Apply given patch to the linked instance DOM.
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
            if (sourceDom.HasMember(PrefabDomUtils::LinkIdName))
            {
                AZ_Warning("Prefab", false, "Source template from link with id '%llu' shouldn't contains key 'LinkId'",
                    static_cast<AZ::u64>(linkId));
                sourceDom.RemoveMember(PrefabDomUtils::LinkIdName);
            }

            m_redoPatch = PrefabDom(); // deallocates all memory previously allocated, just in case.
            // the above line needs to be done before the GeneratePatch call so that the memory stays alive to be 'moved'

            //Diff the instance against the source template.
            PrefabDom linkPatch(&m_redoPatch.GetAllocator()); // allocations from GeneratePatch will be stored in the m_redoPatch allocator.
            m_instanceToTemplateInterface->GeneratePatch(linkPatch, sourceDom, instanceDom);
            
            m_redoPatch.CopyFrom(m_undoPatch, m_redoPatch.GetAllocator());
            if (auto patchesIter = m_redoPatch.FindMember(PrefabDomUtils::PatchesName); patchesIter == m_redoPatch.MemberEnd())
            {
                m_redoPatch.AddMember(
                    rapidjson::GenericStringRef(PrefabDomUtils::PatchesName), AZStd::move(linkPatch), m_redoPatch.GetAllocator());
            }
            else
            {
                patchesIter->value = AZStd::move(linkPatch.GetArray());
            }
        }

        void PrefabUndoUpdateLink::UpdateLink(const PrefabDom& linkDom, InstanceOptionalConstReference instanceToExclude)
        {
            LinkReference link = m_prefabSystemComponentInterface->FindLink(m_linkId);
            if (link.has_value())
            {
                link->get().SetLinkDom(linkDom);
                link->get().UpdateTarget();

                m_prefabSystemComponentInterface->PropagateTemplateChanges(m_templateId, instanceToExclude);
                m_prefabSystemComponentInterface->SetTemplateDirtyFlag(m_templateId, true);
            }
        }
    }
}
