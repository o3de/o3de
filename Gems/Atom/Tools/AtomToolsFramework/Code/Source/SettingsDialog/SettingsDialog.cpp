/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AtomToolsFramework/SettingsDialog/SettingsDialog.h>

#include <QDialogButtonBox>
#include <QVBoxLayout>

namespace AtomToolsFramework
{
    SettingsDialog::SettingsDialog(QWidget* parent)
        : QDialog(parent)
    {
        setWindowTitle("Settings");
        setLayout(new QVBoxLayout(this));
        setModal(true);

        m_inspectorWidget = new InspectorWidget(this);
        layout()->addWidget(m_inspectorWidget);

        // Create the bottom row of the dialog with action buttons
        auto buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok, this);
        QObject::connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
        QObject::connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
        layout()->addWidget(buttonBox);
    }

    InspectorWidget* SettingsDialog::GetInspector()
    {
        return m_inspectorWidget;
    }

    AZStd::shared_ptr<DynamicPropertyGroup> CreateSettingsPropertyGroup(
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

    DynamicProperty CreateSettingsPropertyValue(
        const AZStd::string& id, const AZStd::string& displayName, const AZStd::string& description, const AZStd::string& defaultValue)
    {
        DynamicPropertyConfig config;
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

    DynamicProperty CreateSettingsPropertyValue(
        const AZStd::string& id, const AZStd::string& displayName, const AZStd::string& description, const bool& defaultValue)
    {
        DynamicPropertyConfig config;
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

    DynamicProperty CreateSettingsPropertyValue(
        const AZStd::string& id,
        const AZStd::string& displayName,
        const AZStd::string& description,
        const double& defaultValue,
        const double& minValue,
        const double& maxValue)
    {
        DynamicPropertyConfig config;
        config.m_id = id;
        config.m_name = displayName;
        config.m_displayName = displayName;
        config.m_description = description;
        config.m_min = aznumeric_cast<float>(minValue);
        config.m_max = aznumeric_cast<float>(maxValue);
        config.m_defaultValue = config.m_originalValue = config.m_parentValue =
            aznumeric_cast<float>(GetSettingsValue<double>(id, defaultValue));
        config.m_dataChangeCallback = [id](const AZStd::any& value)
        {
            SetSettingsValue<double>(id, aznumeric_cast<double>(AZStd::any_cast<float>(value)));
            return AZ::Edit::PropertyRefreshLevels::AttributesAndValues;
        };
        return DynamicProperty(config);
    }

    DynamicProperty CreateSettingsPropertyValue(
        const AZStd::string& id,
        const AZStd::string& displayName,
        const AZStd::string& description,
        const AZ::u64& defaultValue,
        const AZ::u64& minValue,
        const AZ::u64& maxValue)
    {
        DynamicPropertyConfig config;
        config.m_id = id;
        config.m_name = displayName;
        config.m_displayName = displayName;
        config.m_description = description;
        config.m_min = aznumeric_cast<AZ::u32>(minValue);
        config.m_max = aznumeric_cast<AZ::u32>(maxValue);
        config.m_defaultValue = config.m_originalValue = config.m_parentValue =
            aznumeric_cast<AZ::u32>(GetSettingsValue<AZ::u64>(id, defaultValue));
        config.m_dataChangeCallback = [id](const AZStd::any& value)
        {
            SetSettingsValue<AZ::u64>(id, aznumeric_cast<AZ::u64>(AZStd::any_cast<AZ::u32>(value)));
            return AZ::Edit::PropertyRefreshLevels::AttributesAndValues;
        };
        return DynamicProperty(config);
    }

    DynamicProperty CreateSettingsPropertyValue(
        const AZStd::string& id,
        const AZStd::string& displayName,
        const AZStd::string& description,
        const AZ::s64& defaultValue,
        const AZ::s64& minValue,
        const AZ::s64& maxValue)
    {
        DynamicPropertyConfig config;
        config.m_id = id;
        config.m_name = displayName;
        config.m_displayName = displayName;
        config.m_description = description;
        config.m_min = aznumeric_cast<AZ::s32>(minValue);
        config.m_max = aznumeric_cast<AZ::s32>(maxValue);
        config.m_defaultValue = config.m_originalValue = config.m_parentValue =
            aznumeric_cast<AZ::s32>(GetSettingsValue<AZ::s64>(id, defaultValue));
        config.m_dataChangeCallback = [id](const AZStd::any& value)
        {
            SetSettingsValue<AZ::s64>(id, aznumeric_cast<AZ::s64>(AZStd::any_cast<AZ::s32>(value)));
            return AZ::Edit::PropertyRefreshLevels::AttributesAndValues;
        };
        return DynamicProperty(config);
    }
} // namespace AtomToolsFramework
