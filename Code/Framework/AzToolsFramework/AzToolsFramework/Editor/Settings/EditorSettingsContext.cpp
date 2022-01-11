/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/EditContextConstants.inl>

#include <AzToolsFramework/Editor/Settings/EditorSettingsContext.h>

namespace AzToolsFramework
{
    EditorSettingProperty::EditorSettingProperty(
        AZ::Uuid typeId,
        AZStd::string_view category,
        AZStd::string_view subCategory,
        AZStd::string_view name,
        AZStd::string_view description
    )
        : m_typeId(typeId)
        , m_category(category)
        , m_subCategory(subCategory)
    {
        m_editData.m_name = name.data();
        m_editData.m_description = description.data();
        m_editData.m_elementId = AZ::Edit::UIHandlers::Default;
    }

    AZStd::string_view EditorSettingProperty::GetName()
    {
        return m_editData.m_name;
    }

    AZStd::string_view EditorSettingProperty::GetDescription()
    {
        return m_editData.m_description;
    }

    AZStd::string_view EditorSettingProperty::GetCategory()
    {
        return m_category;
    }

    AZStd::string_view EditorSettingProperty::GetSubCategory()
    {
        return m_subCategory;
    }

    const AZ::Edit::ElementData* EditorSettingProperty::GetEditData() const
    {
        return &m_editData;
    }

    AZ::Uuid EditorSettingProperty::GetTypeId()
    {
        return m_typeId;
    }

    void EditorSettingProperty::AddAttribute(AZ::AttributePair attributePair)
    {
        m_editData.m_attributes.emplace_back(AZStd::move(attributePair));
    }

    const EditorSettingsArray& EditorSettingsContext::GetSettingsArray() const
    {
        return m_settings;
    }

    EditorSettingsContext::~EditorSettingsContext()
    {
    }
}
