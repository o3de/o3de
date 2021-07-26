/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/Editor/Settings/EditorSettingsContext.h>

namespace AzToolsFramework
{
    EditorSettingItem::EditorSettingItem(
        AZStd::string_view category, AZStd::string_view subCategory,
        AZStd::string_view name, AZStd::string_view description)
        : m_category(category)
        , m_subCategory(subCategory)
        , m_name(name)
        , m_description(description)
    {
    }

    AZStd::string_view EditorSettingItem::GetCategory()
    {
        return m_category;
    }

    void EditorSettingItem::SetCategory(AZStd::string_view category)
    {
        m_category = category;
    }

    AZStd::string_view EditorSettingItem::GetSubCategory()
    {
        return m_subCategory;
    }

    void EditorSettingItem::SetSubCategory(AZStd::string_view subCategory)
    {
        m_subCategory = subCategory;
    }

    AZStd::string_view EditorSettingItem::GetName()
    {
        return m_name;
    }

    AZStd::string_view EditorSettingItem::GetDescription()
    {
        return m_description;
    }

    void EditorSettingItem::AddAttribute(AttributePair attributePair)
    {
        m_attributes.emplace_back(AZStd::move(attributePair));
    }

    const EditorSettingsArray& EditorSettingsContext::GetSettingsArray() const
    {
        return m_settings;
    }

    EditorSettingsContext::~EditorSettingsContext()
    {
    }
}
