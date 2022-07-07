/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
        Template::Template(const AZ::IO::Path& filePath, PrefabDom prefabDom)
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
            if (m_prefabDom.IsNull() || m_filePath.empty())
            {
                return false;
            }
            else if (!m_prefabDom.IsObject())
            {
                return false;
            }
            auto source = m_prefabDom.FindMember(PrefabDomUtils::SourceName);
            return (source != m_prefabDom.MemberEnd());
        }

        bool Template::IsLoadedWithErrors() const
        {
            return m_isLoadedWithErrors;
        }

        void Template::MarkAsLoadedWithErrors(bool loadedWithErrors)
        {
            m_isLoadedWithErrors = loadedWithErrors;
        }

        bool Template::IsDirty() const
        {
            return m_isDirty;
        }

        void Template::MarkAsDirty(bool dirty)
        {
            m_isDirty = dirty;
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

        bool Template::IsProcedural() const
        {
            if (m_isProcedural.has_value())
            {
                return m_isProcedural.value();
            }
            else if (!IsValid())
            {
                return false;
            }
            auto source = m_prefabDom.FindMember(PrefabDomUtils::SourceName);
            if (!source->value.IsString())
            {
                return false;
            }
            AZ::IO::PathView path(source->value.GetString());
            m_isProcedural = AZStd::make_optional(path.Extension().Match(".procprefab"));
            return m_isProcedural.value();
        }

        const AZ::IO::Path& Template::GetFilePath() const
        {
            return m_filePath;
        }

        void Template::SetFilePath(const AZ::IO::PathView& path)
        {
            m_filePath = path;
        }
    } // namespace Prefab
} // namespace AzToolsFramework
