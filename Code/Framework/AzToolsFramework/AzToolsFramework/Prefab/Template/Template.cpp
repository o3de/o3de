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

#include <AzToolsFramework/Prefab/Template/Template.h>

#include <AzToolsFramework/Prefab/Link/Link.h>
#include <AzToolsFramework/Prefab/PrefabDomUtils.h>
#include <AzToolsFramework/Prefab/PrefabSystemComponentInterface.h>

namespace AzToolsFramework
{
    namespace Prefab
    {
        Template::Template(const AZStd::string& filePath, PrefabDom prefabDom)
            : m_filePath(filePath)
            , m_prefabDom(AZStd::move(prefabDom))
        {
        }

        Template::Template(const Template& other)
            : m_links(other.m_links)
            , m_filePath(other.m_filePath)
            , m_isLoadedWithErrors(other.m_isLoadedWithErrors)
        {
            m_prefabDom.CopyFrom(other.m_prefabDom, m_prefabDom.GetAllocator());
        }

        Template& Template::operator=(const Template& other)
        {
            if (this != &other)
            {
                m_links = other.m_links;
                m_filePath = other.m_filePath;
                m_isLoadedWithErrors = other.m_isLoadedWithErrors;
                m_prefabDom.CopyFrom(other.m_prefabDom, m_prefabDom.GetAllocator());
            }
            
            return *this;
        }

        Template::Template(Template&& other) noexcept
            : m_links(AZStd::move(other.m_links))
            , m_filePath(AZStd::move(other.m_filePath))
            , m_isLoadedWithErrors(AZStd::move(other.m_isLoadedWithErrors))
        {
            m_prefabDom.Swap(other.m_prefabDom);
        }

        Template& Template::operator=(Template&& other) noexcept
        {
            if (this != &other)
            {
                m_links = AZStd::move(other.m_links);
                m_filePath = AZStd::move(other.m_filePath);
                m_isLoadedWithErrors = AZStd::move(other.m_isLoadedWithErrors);
                m_prefabDom.Swap(other.m_prefabDom);
            }
            
            return *this;
        }

        bool Template::IsValid() const
        {
            return !m_prefabDom.IsNull() && !m_filePath.empty();
        }

        bool Template::IsLoadedWithErrors() const
        {
            return m_isLoadedWithErrors;
        }

        void Template::MarkAsLoadedWithErrors(bool loadedWithErrors)
        {
            m_isLoadedWithErrors = loadedWithErrors;
        }

        bool Template::AddLink(LinkId newLinkId)
        {
            if (newLinkId == InvalidLinkId)
            {
                AZ_Error("Prefab", false,
                    "Template::AddLink - "
                    "Attempted to add a link with an invalid id");

                return false;
            }

            if (HasLink(newLinkId))
            {
                AZ_Error("Prefab", false,
                    "Template::AddLink - "
                    "Link with id %u is already stored in this Template.",
                    newLinkId);

                return false;
            }

            m_links.insert(newLinkId);
            return true;
        }

        bool Template::RemoveLink(LinkId linkId)
        {
            return m_links.erase(linkId) != 0;
        }

        bool Template::HasLink(LinkId linkId) const
        {
            return m_links.count(linkId) > 0;
        }

        const Template::Links& Template::GetLinks() const
        {
            return m_links;
        }

        PrefabDom& Template::GetPrefabDom()
        {
            return m_prefabDom;
        }

        const PrefabDom& Template::GetPrefabDom() const
        {
            return m_prefabDom;
        }

        bool Template::CopyTemplateIntoPrefabFileFormat(PrefabDom& output)
        {
            // Start by making a copy of our dom
            output.CopyFrom(m_prefabDom, m_prefabDom.GetAllocator());

            PrefabSystemComponentInterface* prefabSystemComponentInterface =
                AZ::Interface<PrefabSystemComponentInterface>::Get();

            AZ_Assert(prefabSystemComponentInterface,
                "Prefab - Prefab System Component Interface is null while attempting "
                "to copy Template associated with Prefab file %s into Prefab File format",
                m_filePath.c_str());

            for (const LinkId& linkId : m_links)
            {
                AZStd::optional<AZStd::reference_wrapper<Link>> findLinkResult =
                    prefabSystemComponentInterface->FindLink(linkId);

                if (!findLinkResult.has_value())
                {
                    AZ_Error("Prefab", false,
                        "Link with id %llu could not be found while attempting to store "
                        "Prefab Template with source path %s in Prefab File format. "
                        "Unable to proceed.",
                        linkId, m_filePath.c_str());

                    return false;
                }

                if (!findLinkResult->get().IsValid())
                {
                    AZ_Error("Prefab", false,
                        "Link with id %llu and is invalid during attempt to store "
                        "Prefab Template with source path %s in Prefab File format. "
                        "Unable to Proceed.",
                        linkId, m_filePath.c_str());

                    return false;
                }

                Link& link = findLinkResult->get();

                PrefabDomPath instancePath = link.GetInstancePath();
                PrefabDom& linkDom = link.GetLinkDom();

                // Get the instance value of the Template copy
                // This currently stores a fully realized nested Template Dom
                PrefabDomValue* instanceValue = instancePath.Get(output);

                if (!instanceValue)
                {
                    AZ_Error("Prefab", false,
                        "Template::CopyTemplateIntoPrefabFileFormat: Unable to recover nested instance Dom value from link with id %llu "
                        "while attempting to store a collapsed version of a Prefab Template with source path %s. Unable to proceed.",
                        linkId, m_filePath.c_str());

                    return false;
                }

                // Copy the contents of the Link to overwrite our Template Dom copies Instance
                // The instance is now "collapsed" as it contains the file reference and patches from the link
                instanceValue->CopyFrom(linkDom, m_prefabDom.GetAllocator());
            }

            return true;
        }

        PrefabDomValueReference Template::GetInstancesValue()
        {
            if (!IsValid())
            {
                return AZStd::nullopt;
            }

            AZStd::optional<AZStd::reference_wrapper<PrefabDomValue>> findInstancesResult;
            findInstancesResult = PrefabDomUtils::FindPrefabDomValue(
                m_prefabDom, PrefabDomUtils::InstancesName);
            if (!findInstancesResult.has_value() || !(findInstancesResult->get().IsObject()))
            {
                return AZStd::nullopt;
            }

            return findInstancesResult->get();
        }

        PrefabDomValueConstReference Template::GetInstancesValue() const
        {
            if (!IsValid())
            {
                return AZStd::nullopt;
            }

            AZStd::optional<AZStd::reference_wrapper<const PrefabDomValue>> findInstancesResult;
            findInstancesResult = PrefabDomUtils::FindPrefabDomValue(
                m_prefabDom, PrefabDomUtils::InstancesName);
            if (!findInstancesResult.has_value() || !(findInstancesResult->get().IsObject()))
            {
                return AZStd::nullopt;
            }

            return findInstancesResult->get();
        }

        const AZStd::string& Template::GetFilePath() const
        {
            return m_filePath;
        }

    } // namespace Prefab
} // namespace AzToolsFramework
