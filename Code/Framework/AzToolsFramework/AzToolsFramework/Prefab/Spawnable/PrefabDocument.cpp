/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/Prefab/PrefabDomUtils.h>
#include <AzToolsFramework/Prefab/Spawnable/PrefabDocument.h>

namespace AzToolsFramework::Prefab::PrefabConversionUtils
{
    PrefabDocument::PrefabDocument(AZStd::string name)
        : m_name(AZStd::move(name))
        , m_instance(AZStd::make_unique<AzToolsFramework::Prefab::Instance>())
    {
        m_instance->SetTemplateSourcePath(AZ::IO::Path("InMemory") / name);
    }

    bool PrefabDocument::SetPrefabDom(const PrefabDom& prefab)
    {
        if (ConstructInstanceFromPrefabDom(prefab))
        {
            constexpr bool copyConstStrings = true;
            m_dom.CopyFrom(prefab, m_dom.GetAllocator(), copyConstStrings);
            return true;
        }
        else
        {
            return false;
        }
    }

    bool PrefabDocument::SetPrefabDom(PrefabDom&& prefab)
    {
        if (ConstructInstanceFromPrefabDom(prefab))
        {
            m_dom = AZStd::move(prefab);
            return true;
        }
        else
        {
            return false;
        }
    }

    const AZStd::string& PrefabDocument::GetName() const
    {
        return m_name;
    }

    const PrefabDom& PrefabDocument::GetDom() const
    {
        if (m_isDirty)
        {
            m_isDirty = !PrefabDomUtils::StoreInstanceInPrefabDom(*m_instance, m_dom);
        }
        return m_dom;
    }

    PrefabDom&& PrefabDocument::TakeDom()
    {
        if (m_isDirty)
        {
            m_isDirty = !PrefabDomUtils::StoreInstanceInPrefabDom(*m_instance, m_dom);
        }
        return AZStd::move(m_dom);
    }

    AzToolsFramework::Prefab::Instance& PrefabDocument::GetInstance()
    {
        // Assume that changes will be made to the instance.
        m_isDirty = true;
        return *m_instance;
    }

    const AzToolsFramework::Prefab::Instance& PrefabDocument::GetInstance() const
    {
        return *m_instance;
    }

    bool PrefabDocument::ConstructInstanceFromPrefabDom(const PrefabDom& prefab)
    {
        using namespace AzToolsFramework::Prefab;

        m_instance->Reset();
        if (PrefabDomUtils::LoadInstanceFromPrefabDom(*m_instance, prefab, PrefabDomUtils::LoadFlags::AssignRandomEntityId))
        {
            return true;
        }
        else
        {
            AZStd::string errorMessage("Failed to construct Prefab instance from given PrefabDOM");

            PrefabDomValueConstReference sourceReference = PrefabDomUtils::FindPrefabDomValue(prefab, PrefabDomUtils::SourceName);
            if (sourceReference.has_value() && sourceReference->get().IsString() && sourceReference->get().GetStringLength() != 0)
            {
                errorMessage += " (Source: ";
                errorMessage += AZStd::string_view(sourceReference->get().GetString(), sourceReference->get().GetStringLength());
                errorMessage += ')';
            }

            errorMessage += '.';
            AZ_Error("PrefabDocument", false, errorMessage.c_str());
            return false;
        }
    }
} // namespace AzToolsFramework::Prefab::PrefabConversionUtils
