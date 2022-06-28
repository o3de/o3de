/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AtomToolsFramework/Inspector/InspectorPropertyGroupWidget.h>
#include <AtomToolsFramework/SettingsDialog/SettingsDialog.h>
#include <AtomToolsFramework/Util/Util.h>

#include <QDialogButtonBox>
#include <QVBoxLayout>

namespace AtomToolsFramework
{
    SettingsDialog::SettingsDialog(AZStd::vector<AZStd::shared_ptr<DynamicPropertyGroup>> groups, QWidget* parent)
        : QDialog(parent)
        , m_groups(groups)
    {
        setWindowTitle("Settings");
        setFixedSize(800, 400);
        setLayout(new QVBoxLayout(this));
        setModal(true);

        m_inspectorWidget = new InspectorWidget(this);
        layout()->addWidget(m_inspectorWidget);

        m_inspectorWidget->AddGroupsBegin();
        for (auto group : m_groups)
        {
            m_inspectorWidget->AddGroup(
                group->m_name,
                group->m_displayName,
                group->m_description,
                new InspectorPropertyGroupWidget(group.get(), group.get(), azrtti_typeid<DynamicPropertyGroup>()));
        }
        m_inspectorWidget->AddGroupsEnd();

        // Create the bottom row of the dialog with action buttons
        auto buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok, this);
        QObject::connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
        QObject::connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
        layout()->addWidget(buttonBox);
    }

    AZStd::shared_ptr<DynamicPropertyGroup> CreateSettingsGroup(
        const AZStd::string& displayName,
        const AZStd::string& description,
        AZStd::vector<DynamicProperty> properties,
        AZStd::vector<AZStd::shared_ptr<DynamicPropertyGroup>> groups)
    {
        auto group = aznew DynamicPropertyGroup();
        group->m_name = displayName;
        group->m_displayName = displayName;
        group->m_description = description;
        group->m_properties = properties;
        group->m_groups = groups;
        return AZStd::shared_ptr<DynamicPropertyGroup>(group);
    }

    DynamicProperty CreatePropertyFromSetting(
        const AZStd::string& id, const AZStd::string& displayName, const AZStd::string& description, const AZStd::string& defaultValue)
    {
        DynamicPropertyConfig config;
        config.m_dataType = DynamicPropertyType::String;
        config.m_id = id;
        config.m_name = displayName;
        config.m_displayName = displayName;
        config.m_description = description;
        config.m_defaultValue = config.m_originalValue = config.m_parentValue = GetSettingsValue<AZStd::string>(id, defaultValue);
        config.m_dataChangeCallback = [id](const AZStd::any& value)
        {
            SetSettingsValue<AZStd::string>(id, AZStd::any_cast<AZStd::string>(value));
            return AZ::Edit::PropertyRefreshLevels::AttributesAndValues;
        };
        return DynamicProperty(config);
    }

    DynamicProperty CreatePropertyFromSetting(
        const AZStd::string& id, const AZStd::string& displayName, const AZStd::string& description, const bool& defaultValue)
    {
        DynamicPropertyConfig config;
        config.m_dataType = DynamicPropertyType::Bool;
        config.m_id = id;
        config.m_name = displayName;
        config.m_displayName = displayName;
        config.m_description = description;
        config.m_defaultValue = config.m_originalValue = config.m_parentValue = GetSettingsValue<bool>(id, defaultValue);
        config.m_dataChangeCallback = [id](const AZStd::any& value)
        {
            SetSettingsValue<bool>(id, AZStd::any_cast<bool>(value));
            return AZ::Edit::PropertyRefreshLevels::AttributesAndValues;
        };
        return DynamicProperty(config);
    }

    DynamicProperty CreatePropertyFromSetting(
        const AZStd::string& id, const AZStd::string& displayName, const AZStd::string& description, const double& defaultValue)
    {
        DynamicPropertyConfig config;
        config.m_dataType = DynamicPropertyType::Float;
        config.m_id = id;
        config.m_name = displayName;
        config.m_displayName = displayName;
        config.m_description = description;
        config.m_defaultValue = config.m_originalValue = config.m_parentValue = static_cast<float>(GetSettingsValue<double>(id, defaultValue));
        config.m_dataChangeCallback = [id](const AZStd::any& value)
        {
            SetSettingsValue<double>(id, static_cast<double>(AZStd::any_cast<float>(value)));
            return AZ::Edit::PropertyRefreshLevels::AttributesAndValues;
        };
        return DynamicProperty(config);
    }

    DynamicProperty CreatePropertyFromSetting(
        const AZStd::string& id, const AZStd::string& displayName, const AZStd::string& description, const uint64_t& defaultValue)
    {
        DynamicPropertyConfig config;
        config.m_dataType = DynamicPropertyType::UInt;
        config.m_id = id;
        config.m_name = displayName;
        config.m_displayName = displayName;
        config.m_description = description;
        config.m_defaultValue = config.m_originalValue = config.m_parentValue = static_cast<uint32_t>(GetSettingsValue<uint64_t>(id, defaultValue));
        config.m_dataChangeCallback = [id](const AZStd::any& value)
        {
            SetSettingsValue<uint64_t>(id, static_cast<uint64_t>(AZStd::any_cast<uint32_t>(value)));
            return AZ::Edit::PropertyRefreshLevels::AttributesAndValues;
        };
        return DynamicProperty(config);
    }

    DynamicProperty CreatePropertyFromSetting(
        const AZStd::string& id, const AZStd::string& displayName, const AZStd::string& description, const int64_t& defaultValue)
    {
        DynamicPropertyConfig config;
        config.m_dataType = DynamicPropertyType::Int;
        config.m_id = id;
        config.m_name = displayName;
        config.m_displayName = displayName;
        config.m_description = description;
        config.m_defaultValue = config.m_originalValue = config.m_parentValue = static_cast<int32_t>(GetSettingsValue<int64_t>(id, defaultValue));
        config.m_dataChangeCallback = [id](const AZStd::any& value)
        {
            SetSettingsValue<int64_t>(id, static_cast<int64_t>(AZStd::any_cast<int32_t>(value)));
            return AZ::Edit::PropertyRefreshLevels::AttributesAndValues;
        };
        return DynamicProperty(config);
    }
} // namespace AtomToolsFramework
