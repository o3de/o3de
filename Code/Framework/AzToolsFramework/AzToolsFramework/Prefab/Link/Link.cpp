/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Debug/Profiler.h>
#include <AzToolsFramework/Prefab/Link/Link.h>
#include <AzToolsFramework/Prefab/PrefabDomUtils.h>
#include <AzToolsFramework/Prefab/PrefabSystemComponentInterface.h>
#include <AzToolsFramework/Prefab/Template/Template.h>

namespace AzToolsFramework
{
    namespace Prefab
    {
        Link::Link()
            : Link(InvalidLinkId)
        {
        }

        Link::Link(LinkId linkId)
        {
            m_id = linkId;
            m_prefabSystemComponentInterface = AZ::Interface<PrefabSystemComponentInterface>::Get();
            AZ_Assert(m_prefabSystemComponentInterface != nullptr,
                "Prefab System Component Interface could not be found. "
                "It is a requirement for the Link class. Check that it is being correctly initialized.");
        }

        Link::Link(Link&& other) noexcept
            : m_id(AZStd::move(other.m_id))
            , m_sourceTemplateId(AZStd::move(other.m_sourceTemplateId))
            , m_targetTemplateId(AZStd::move(other.m_targetTemplateId))
            , m_instanceName(AZStd::move(other.m_instanceName))
            , m_prefabSystemComponentInterface(AZStd::move(other.m_prefabSystemComponentInterface))
            , m_linkPatchesTree(AZStd::move(other.m_linkPatchesTree))
        {
            other.m_prefabSystemComponentInterface = nullptr;
        }


        Link& Link::operator=(Link&& other) noexcept
        {
            if (this != &other)
            {
                m_id = AZStd::move(other.m_id);
                m_sourceTemplateId = AZStd::move(other.m_targetTemplateId);
                m_targetTemplateId = AZStd::move(other.m_sourceTemplateId);
                m_instanceName = AZStd::move(other.m_instanceName);
                m_prefabSystemComponentInterface = AZStd::move(other.m_prefabSystemComponentInterface);
                AZ_Assert(m_prefabSystemComponentInterface != nullptr,
                    "Prefab System Component Interface could not be found. "
                    "It is a requirement for the Link class. Check that it is being correctly initialized.");
                other.m_prefabSystemComponentInterface = nullptr;
                m_linkPatchesTree = AZStd::move(other.m_linkPatchesTree);
            }
            return *this;
        }

        void Link::SetSourceTemplateId(TemplateId id)
        {
            m_sourceTemplateId = id;
        }

        void Link::SetTargetTemplateId(TemplateId id)
        {
            m_targetTemplateId = id;
        }

        void Link::SetLinkDom(const PrefabDomValue& linkDom)
        {
            AZ_PROFILE_FUNCTION(PrefabSystem);
            m_linkPatchesTree.Clear();
            PrefabDomValueConstReference patchesReference = PrefabDomUtils::FindPrefabDomValue(linkDom, PrefabDomUtils::PatchesName);
            if (patchesReference.has_value())
            {
                RebuildLinkPatchesTree(patchesReference->get());
            }
        }

        void Link::AddPatchesToLink(const PrefabDom& patches)
        {
            RebuildLinkPatchesTree(patches);
        }

        void Link::SetInstanceName(const char* instanceName)
        {
            m_instanceName = instanceName;
        }

        bool Link::IsValid() const
        {
            return
                m_targetTemplateId != InvalidTemplateId &&
                m_sourceTemplateId != InvalidTemplateId &&
                !m_instanceName.empty();
        }

        TemplateId Link::GetSourceTemplateId() const
        {
            return m_sourceTemplateId;
        }

        TemplateId Link::GetTargetTemplateId() const
        {
            return m_targetTemplateId;
        }

        LinkId Link::GetId() const
        {
            return m_id;
        }

        void Link::GetLinkDom(PrefabDomValue& linkDom, PrefabDomAllocator& allocator) const
        {
            AZ_PROFILE_FUNCTION(PrefabSystem);
            return ConstructLinkDomFromPatches(linkDom, allocator);
        }

        bool Link::AreOverridesPresent(AZ::Dom::Path path, AZ::Dom::PrefixTreeTraversalFlags prefixTreeTraversalFlags)
        {
            bool areOverridesPresent = false;
            auto visitorFn = [&areOverridesPresent](AZ::Dom::Path, const PrefabOverrideMetadata&)
            {
                areOverridesPresent = true;
                // We just need to check if at least one override is present at the path.
                // Return false here so that we don't keep looking for all patches at the path.
                return false;
            };

            m_linkPatchesTree.VisitPath(path, visitorFn, prefixTreeTraversalFlags);
            return areOverridesPresent;
        }

        PrefabOverridePrefixTree Link::RemoveOverrides(AZ::Dom::Path path)
        {
            return m_linkPatchesTree.DetachSubTree(path);
        }

        bool Link::AddOverrides(const AZ::Dom::Path& path, AZ::Dom::DomPrefixTree<PrefabOverrideMetadata>&& subTree)
        {
            return m_linkPatchesTree.AttachSubTree(path, AZStd::move(subTree));
        }

        PrefabDomPath Link::GetInstancePath() const
        {
            return PrefabDomUtils::GetPrefabDomInstancePath(m_instanceName.c_str());
        }

        const AZStd::string& Link::GetInstanceName() const
        {
            return m_instanceName;
        }

        bool Link::UpdateTarget()
        {
            PrefabDomValue& linkedInstanceDom = GetLinkedInstanceDom();
            PrefabDom& targetTemplatePrefabDom = m_prefabSystemComponentInterface->FindTemplateDom(m_targetTemplateId);
            PrefabDom& sourceTemplatePrefabDom = m_prefabSystemComponentInterface->FindTemplateDom(m_sourceTemplateId);

            // Copy the source template dom so that the actual template DOM does not change and only the linked instance DOM does.
            PrefabDom sourceTemplateDomCopy;
            sourceTemplateDomCopy.CopyFrom(sourceTemplatePrefabDom, sourceTemplatePrefabDom.GetAllocator());

            
            PrefabDom patchesDom;
            ConstructLinkDomFromPatches(patchesDom, patchesDom.GetAllocator());
            PrefabDomValueReference patchesReference = PrefabDomUtils::FindPrefabDomValue(patchesDom, PrefabDomUtils::PatchesName);
            if (!patchesReference.has_value())
            {
                if (AZ::JsonSerialization::Compare(linkedInstanceDom, sourceTemplateDomCopy) != AZ::JsonSerializerCompareResult::Equal)
                {
                    linkedInstanceDom.CopyFrom(sourceTemplateDomCopy, targetTemplatePrefabDom.GetAllocator());
                }
            }
            else
            {
                AZ::JsonSerializationResult::ResultCode applyPatchResult =
                    PrefabDomUtils::ApplyPatches(sourceTemplateDomCopy, targetTemplatePrefabDom.GetAllocator(), patchesReference->get());
                linkedInstanceDom.CopyFrom(sourceTemplateDomCopy, targetTemplatePrefabDom.GetAllocator());

                [[maybe_unused]] PrefabDomValueReference sourceTemplateName =
                    PrefabDomUtils::FindPrefabDomValue(sourceTemplateDomCopy, PrefabDomUtils::SourceName);
                AZ_Assert(sourceTemplateName && sourceTemplateName->get().IsString(), "A valid source template name couldn't be found");
                [[maybe_unused]] PrefabDomValueReference targetTemplateName =
                    PrefabDomUtils::FindPrefabDomValue(targetTemplatePrefabDom, PrefabDomUtils::SourceName);
                AZ_Assert(targetTemplateName && targetTemplateName->get().IsString(), "A valid target template name couldn't be found");

                if (applyPatchResult.GetProcessing() != AZ::JsonSerializationResult::Processing::Completed)
                {
                    AZ_Error(
                        "Prefab", false,
                        "Link::UpdateTarget - ApplyPatches failed for Prefab DOM from source Template '%u' and target Template '%u'.",
                        m_sourceTemplateId, m_targetTemplateId);
                    return false;
                }
                if (applyPatchResult.GetOutcome() == AZ::JsonSerializationResult::Outcomes::PartialSkip ||
                    applyPatchResult.GetOutcome() == AZ::JsonSerializationResult::Outcomes::Skipped)
                {
                    AZ_Warning(
                        "Prefab", false,
                        "Link::UpdateTarget - Some of the patches couldn't be applied on the source template '%s' present under the  "
                        "target Template '%s'.",
                        sourceTemplateName->get().GetString(), targetTemplateName->get().GetString());
                }
            }

            // This is a guardrail to ensure the linked instance dom always has the LinkId value
            // in case the template copy or the patch application removed it.
            AddLinkIdToInstanceDom(linkedInstanceDom, targetTemplatePrefabDom.GetAllocator());
            return true;
        }

        PrefabDomValue& Link::GetLinkedInstanceDom()
        {
            AZ_Assert(IsValid(), "Link::GetLinkedInstanceDom - Trying to get DOM of an invalid link.");
            PrefabDom& targetTemplatePrefabDom = m_prefabSystemComponentInterface->FindTemplateDom(m_targetTemplateId);
            PrefabDomPath instancePath = GetInstancePath();
            PrefabDomValue* instanceValue = instancePath.Get(targetTemplatePrefabDom);
            AZ_Assert(instanceValue,"Link::GetLinkedInstanceDom - Invalid value for instance pointed by the link in template with id '%u'.",
                    m_targetTemplateId);
            return *instanceValue;
        }

        void Link::AddLinkIdToInstanceDom(PrefabDomValue& instanceDom)
        {
            PrefabDom& targetTemplatePrefabDom = m_prefabSystemComponentInterface->FindTemplateDom(m_targetTemplateId);
            AddLinkIdToInstanceDom(instanceDom, targetTemplatePrefabDom.GetAllocator());
        }

        void Link::AddLinkIdToInstanceDom(PrefabDomValue& instanceDom, PrefabDomAllocator& allocator)
        {
            PrefabDomValueReference linkIdReference = PrefabDomUtils::FindPrefabDomValue(instanceDom, PrefabDomUtils::LinkIdName);
            if (!linkIdReference.has_value())
            {
                AZ_Assert(instanceDom.IsObject(), "Link Id '%u' cannot be added because the DOM of the instance is not an object.", m_id);
                instanceDom.AddMember(rapidjson::StringRef(PrefabDomUtils::LinkIdName), rapidjson::Value().SetUint64(m_id), allocator);
            }
            else
            {
                linkIdReference->get().SetUint64(m_id);
            }
        }

        void Link::ConstructLinkDomFromPatches(PrefabDomValue& linkDom, PrefabDomAllocator& allocator) const
        {
            linkDom.SetObject();

            TemplateReference sourceTemplate = m_prefabSystemComponentInterface->FindTemplate(m_sourceTemplateId);
            if (!sourceTemplate.has_value())
            {
                AZ_Assert(false, "Failed to fetch source template from link");
                return;
            }

            linkDom.AddMember(
                rapidjson::StringRef(PrefabDomUtils::SourceName),
                PrefabDomValue(sourceTemplate->get().GetFilePath().c_str(), allocator),
                allocator);

            PrefabDomValue patchesArray;

            GetLinkPatches(patchesArray, allocator);

            if (patchesArray.Size() != 0)
            {
                linkDom.AddMember(rapidjson::StringRef(PrefabDomUtils::PatchesName), AZStd::move(patchesArray), allocator);
            }
        }

        void Link::RebuildLinkPatchesTree(const PrefabDomValue& patches)
        {
            m_linkPatchesTree.Clear();
            if (patches.IsArray())
            {
                rapidjson::GenericArray patchesArray = patches.GetArray();
                for (rapidjson::SizeType i = 0; i < patchesArray.Size(); i++)
                {
                    PrefabDom patchEntry;
                    patchEntry.CopyFrom(patchesArray[i], patchEntry.GetAllocator());

                    auto path = patchEntry.FindMember("path");
                    if (path != patchEntry.MemberEnd())
                    {
                        AZ::Dom::Path domPath(path->value.GetString());
                        PrefabOverrideMetadata overrideMetadata(AZStd::move(patchEntry), i);
                        m_linkPatchesTree.SetValue(domPath, AZStd::move(overrideMetadata));
                    }
                }
            }
        }

        void Link::GetLinkPatches(PrefabDomValue& patchesDom, PrefabDomAllocator& allocator) const
        {
            auto cmp = [](const PrefabOverrideMetadata* a, const PrefabOverrideMetadata* b)
            {
                return (a->m_patchIndex < b->m_patchIndex);
            };

            // Use a set to sort the patches based on their patch indices. This will make sure that entities are
            // retrieved from the tree in the same order as they are inserted in.
            AZStd::set<const PrefabOverrideMetadata*, decltype(cmp)> patchesSet(cmp);

            auto visitorFn = [&patchesSet](const AZ::Dom::Path&, const PrefabOverrideMetadata& overrideMetadata)
            {
                patchesSet.emplace(&overrideMetadata);
                return true;
            };

            patchesDom.SetArray();
            m_linkPatchesTree.VisitPath(AZ::Dom::Path(), visitorFn, AZ::Dom::PrefixTreeTraversalFlags::ExcludeExactPath);

            for (auto patchesSetIterator = patchesSet.begin(); patchesSetIterator != patchesSet.end(); ++patchesSetIterator)
            {
                PrefabDomValue patch((*patchesSetIterator)->m_patch, allocator);
                patchesDom.PushBack(patch.Move(), allocator);
            }
        }

    } // namespace Prefab
} // namespace AzToolsFramework
