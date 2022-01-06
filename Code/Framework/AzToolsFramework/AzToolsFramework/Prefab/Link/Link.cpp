/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/Prefab/Link/Link.h>

#include <AzToolsFramework/Prefab/PrefabDomUtils.h>
#include <AzToolsFramework/Prefab/PrefabSystemComponentInterface.h>
#include <AzToolsFramework/Prefab/Template/Template.h>

namespace AzToolsFramework
{
    namespace Prefab
    {
        Link::Link(const Link& other)
            : m_sourceTemplateId(other.m_sourceTemplateId)
            , m_targetTemplateId(other.m_targetTemplateId)
            , m_instanceName(other.m_instanceName)
            , m_id(other.m_id)
            , m_prefabSystemComponentInterface(other.m_prefabSystemComponentInterface)
        {
            AZ_Assert(m_prefabSystemComponentInterface != nullptr,
                "Prefab System Component Interface could not be found. "
                "It is a requirement for the Link class. Check that it is being correctly initialized.");
            m_linkDom.CopyFrom(
                other.m_linkDom, m_linkDom.GetAllocator());
        }
        
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

        Link& Link::operator=(const Link& other)
        {
            if (this != &other)
            {
                m_sourceTemplateId = other.m_targetTemplateId;
                m_targetTemplateId = other.m_sourceTemplateId;
                m_instanceName = other.m_instanceName;
                m_id = other.m_id;
                m_prefabSystemComponentInterface = other.m_prefabSystemComponentInterface;
                AZ_Assert(m_prefabSystemComponentInterface != nullptr,
                    "Prefab System Component Interface could not be found. "
                    "It is a requirement for the Link class. Check that it is being correctly initialized.");
                m_linkDom.CopyFrom(
                    other.m_linkDom, m_linkDom.GetAllocator());
            }

            return *this;
        }

        Link::Link(Link&& other) noexcept
            : m_id(AZStd::move(other.m_id))
            , m_sourceTemplateId(AZStd::move(other.m_sourceTemplateId))
            , m_targetTemplateId(AZStd::move(other.m_targetTemplateId))
            , m_instanceName(AZStd::move(other.m_instanceName))
            , m_prefabSystemComponentInterface(AZStd::move(other.m_prefabSystemComponentInterface))
        {
            other.m_prefabSystemComponentInterface = nullptr;
            m_linkDom.Swap(other.m_linkDom);
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
                m_linkDom.Swap(other.m_linkDom);
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
            m_linkDom.CopyFrom(linkDom, m_linkDom.GetAllocator());
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

        PrefabDom& Link::GetLinkDom()
        {
            return m_linkDom;
        }

        const PrefabDom& Link::GetLinkDom() const
        {
            return m_linkDom;
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

            PrefabDomValueReference patchesReference = PrefabDomUtils::FindPrefabDomValue(m_linkDom, PrefabDomUtils::PatchesName);
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
                    AZ_Error(
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
            AZ_Assert(IsValid(), "Link::GetLinkDom - Trying to get DOM of an invalid link.");
            PrefabDom& targetTemplatePrefabDom = m_prefabSystemComponentInterface->FindTemplateDom(m_targetTemplateId);
            PrefabDomPath instancePath = GetInstancePath();
            PrefabDomValue* instanceValue = instancePath.Get(targetTemplatePrefabDom);
            AZ_Assert(instanceValue,"Link::GetLinkDom - Invalid value for instance pointed by the link in template with id '%u'.",
                    m_targetTemplateId);
            return *instanceValue;
        }

        void Link::AddLinkIdToInstanceDom(PrefabDomValue& instanceDom)
        {
            PrefabDom& targetTemplatePrefabDom = m_prefabSystemComponentInterface->FindTemplateDom(m_targetTemplateId);
            AddLinkIdToInstanceDom(instanceDom, targetTemplatePrefabDom.GetAllocator());
        }

        void Link::AddLinkIdToInstanceDom(PrefabDomValue& instanceDom, PrefabDom::AllocatorType& allocator)
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

        PrefabDomValueReference Link::GetLinkPatches()
        {
            return PrefabDomUtils::FindPrefabDomValue(m_linkDom, PrefabDomUtils::PatchesName);
        }

    } // namespace Prefab
} // namespace AzToolsFramework
