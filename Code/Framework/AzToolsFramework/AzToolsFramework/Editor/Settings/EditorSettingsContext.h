/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/std/string/string.h>

#include <AzToolsFramework/Editor/Settings/EditorSettingsContextConstants.inl>

namespace AzToolsFramework
{
    using AttributePair = AZ::AttributePair;
    using AttributeArray = AZ::AttributeArray;

    class EditorSettingItem
    {
    public:
        EditorSettingItem(
            AZStd::string_view category, AZStd::string_view subCategory,
            AZStd::string_view name, AZStd::string_view description);

        AZStd::string_view GetCategory();
        void SetCategory(AZStd::string_view category);
        AZStd::string_view GetSubCategory();
        void SetSubCategory(AZStd::string_view subCategory);
        AZStd::string_view GetName();
        AZStd::string_view GetDescription();

        void AddAttribute(AttributePair attributePair);
    private:
        AZStd::string       m_category;
        AZStd::string       m_subCategory;
        AZStd::string       m_name;
        AZStd::string       m_description;
        // TODO - store the type!
        AttributeArray      m_attributes;
    };
    using EditorSettingsArray = AZStd::vector<EditorSettingItem>;

    class EditorSettingsContext
        : public AZ::ReflectContext
    {
    public:
        /// @cond EXCLUDE_DOCS
        class SettingBuilder;
        /// @endcond

        AZ_RTTI(EditorSettingsContext, "{DEBEEB16-1EFA-490B-A743-F1B764130D58}", ReflectContext);

        ~EditorSettingsContext() override;

        const EditorSettingsArray& GetSettingsArray() const;

        template<class T>
        SettingBuilder Setting(const char* category, const char* subCategory, const char* name, const char* description);

        class SettingBuilder
        {
            friend EditorSettingsContext;
            SettingBuilder(EditorSettingsContext* context, EditorSettingItem& setting)
                : m_context(context)
                , m_setting(setting)
            {
                AZ_Assert(context, "context cannot be null");
            }

            EditorSettingsContext* m_context;
            EditorSettingItem& m_setting;

        public:
            ~SettingBuilder()
            {
            }
            SettingBuilder* operator->()
            {
                return this;
            }

            // template<class T>
            // SettingBuilder* Attribute(const char* id, T value);

            template<class T>
            SettingBuilder* Attribute(AZ::Crc32 idCrc, T value);
        };

    private:
        EditorSettingsArray m_settings;
    };

    template<class T>
    EditorSettingsContext::SettingBuilder EditorSettingsContext::Setting(const char* category, const char* subCategory, const char* name, const char* description)
    {
        // Add the setting to the settings array
        m_settings.emplace_back(EditorSettingItem(category, subCategory, name, description));

        return SettingBuilder(this, m_settings.back());
    }

    template<class T>
    EditorSettingsContext::SettingBuilder* EditorSettingsContext::SettingBuilder::Attribute(AZ::Crc32 idCrc, T value)
    {
        // TODO - add 100 checks...
        /*
        if (!IsValid())
        {
            return this;
        }
        */

        using ContainerType = AZ::AttributeContainerType<T>;
        m_setting.AddAttribute(AttributePair(idCrc, aznew ContainerType(value)));
        
        return this;
    }
} // namespace AzToolsFramework
