/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/RTTI/TypeInfo.h>
#include <AzCore/Serialization/EditContext.h>

#include <AzToolsFramework/Editor/Settings/EditorSettingsContext.h>

namespace AzToolsFramework
{
    using EditorSettingValueToPropertyMap = AZStd::unordered_map<const void*, EditorSettingProperty*>;

    class EditorSettingPropertyValue
    {
    public:
        AZ_RTTI(EditorSettingPropertyValue, "{F6149794-BEF4-4C99-BF6F-78F897964EDB}");

        EditorSettingPropertyValue() {}
        virtual ~EditorSettingPropertyValue() {};

        virtual void someFunction() = 0;
        void SetName(AZStd::string_view name);

    private:
        friend class EditorSettingsManager;
        friend struct EditorSettingPropertyGroup;

        AZStd::string m_name;
    };

    class EditorSettingPropertyInt
        : public EditorSettingPropertyValue
    {
    public:
        AZ_RTTI(EditorSettingPropertyInt, "{8BD35DEC-38AC-471C-B097-BD8699ACA8D7}", EditorSettingPropertyValue);

        EditorSettingPropertyInt() = default;
        explicit EditorSettingPropertyInt(int value)
            : m_value(value)
        {}

        void someFunction() override {};

        int GetValue()
        {
            return m_value;
        }

    private:
        friend class EditorSettingsManager;
        friend struct EditorSettingPropertyGroup;

        int m_value = 0;
    };

    struct EditorSettingPropertyGroup
    {
        AZ_TYPE_INFO(EditorSettingPropertyGroup, "{A6088F5B-E565-4A45-958F-2512AA7BA2FE}");

        friend class EditorSettingsManager;

        AZStd::string m_name;
        AZStd::vector<EditorSettingPropertyValue*> m_properties;
        AZStd::vector<EditorSettingPropertyGroup> m_groups;

        // Get the pointer to the specified group in m_groups. Returns nullptr if not found.
        EditorSettingPropertyGroup* GetGroup(const char* groupName);
        // Get the pointer to the specified property in m_properties. Returns nullptr if not found.
        EditorSettingPropertyValue* GetProperty(const char* propertyName);
        // Remove all properties and groups
        void Clear();

        EditorSettingPropertyGroup() = default;
        ~EditorSettingPropertyGroup();

        EditorSettingPropertyGroup(const EditorSettingPropertyGroup& rhs) = delete;
        EditorSettingPropertyGroup& operator=(EditorSettingPropertyGroup&) = delete;

    public:
        EditorSettingPropertyGroup(EditorSettingPropertyGroup&& rhs)
        {
            *this = AZStd::move(rhs);
        }
        EditorSettingPropertyGroup& operator=(EditorSettingPropertyGroup&& rhs);
    };

    class EditorSettingsBlock
    {
    public:
        AZ_RTTI(EditorSettingsBlock, "{6D0372C1-3D93-4A43-A030-85FE259D9DB2}");

        EditorSettingsBlock();
        EditorSettingsBlock(AZStd::string_view name);
        virtual ~EditorSettingsBlock();

        void AddProperty(EditorSettingProperty settingItem);

        static const AZ::Edit::ElementData* GetSettingPropertyEditData(
            const void* handlerPtr, const void* elementPtr, const AZ::Uuid& elementType);

    private:
        friend class EditorSettingsManager;

        AZStd::string                               m_name;
        AZStd::vector<EditorSettingProperty>        m_settingProperties;
        EditorSettingPropertyGroup                  m_settingValues;
        EditorSettingValueToPropertyMap             m_settingPropertyValueMap;

        EditorSettingsBlock(const EditorSettingsBlock& rhs) = delete;
        EditorSettingsBlock& operator=(EditorSettingsBlock&) = delete;

    public:
        EditorSettingsBlock(EditorSettingsBlock&& rhs)
        {
            *this = AZStd::move(rhs);
        }
        EditorSettingsBlock& operator=(EditorSettingsBlock&& rhs);
    };

} // namespace AzToolsFramework::Editor
