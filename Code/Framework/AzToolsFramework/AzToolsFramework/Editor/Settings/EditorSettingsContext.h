/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/std/string/string.h>

#include <AzToolsFramework/Editor/Settings/EditorSettingsContextConstants.inl>

namespace AzToolsFramework
{

    class EditorSettingProperty
    {
    public:
        EditorSettingProperty(
            AZ::Uuid typeId,
            AZStd::string_view category,
            AZStd::string_view subCategory,
            AZStd::string_view name,
            AZStd::string_view description
        );

        AZStd::string_view GetName();
        AZStd::string_view GetDescription();
        AZStd::string_view GetCategory();
        AZStd::string_view GetSubCategory();
        const AZ::Edit::ElementData* GetEditData() const;
        AZ::Uuid GetTypeId();
        void AddAttribute(AZ::AttributePair attributePair);

    private:
        AZ::Uuid                m_typeId;
        AZStd::string           m_category;
        AZStd::string           m_subCategory;
        AZ::Edit::ElementData   m_editData;
    };
    using EditorSettingsArray = AZStd::vector<EditorSettingProperty>;

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
            SettingBuilder(EditorSettingsContext* context, EditorSettingProperty& setting)
                : m_context(context)
                , m_setting(setting)
            {
                AZ_Assert(context, "context cannot be null");
            }

            EditorSettingsContext* m_context;
            EditorSettingProperty& m_setting;

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
        m_settings.emplace_back(EditorSettingProperty(azrtti_typeid<T>(), category, subCategory, name, description));
        
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
        m_setting.AddAttribute(AZ::AttributePair(idCrc, aznew ContainerType(value)));
        
        return this;
    }
} // namespace AzToolsFramework
