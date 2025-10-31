/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AtomToolsFramework/DynamicProperty/DynamicPropertyGroup.h>
#include <AtomToolsFramework/Inspector/InspectorPropertyGroupWidget.h>
#include <AtomToolsFramework/Inspector/InspectorWidget.h>
#include <AtomToolsFramework/Util/Util.h>
#include <QDialog>
#endif

namespace AtomToolsFramework
{
    //! Modal dialog for displaying a list of property groups wrapping registry settings
    class SettingsDialog : public QDialog
    {
        Q_OBJECT
    public:
        SettingsDialog(QWidget* parent = nullptr);
        ~SettingsDialog() = default;
        InspectorWidget* GetInspector();

    private:
        InspectorWidget* m_inspectorWidget = {};
    };

    // Create a basic dynamic property group with display name and description configured
    AZStd::shared_ptr<DynamicPropertyGroup> CreateSettingsPropertyGroup(
        const AZStd::string& displayName,
        const AZStd::string& description,
        AZStd::vector<DynamicProperty> properties,
        AZStd::vector<AZStd::shared_ptr<DynamicPropertyGroup>> groups = {});

    // Helper function to create and bind a string registry setting to a dynamic property
    DynamicProperty CreateSettingsPropertyValue(
        const AZStd::string& id, const AZStd::string& displayName, const AZStd::string& description, const AZStd::string& defaultValue);

    // Helper function to create and bind a bool registry setting to a dynamic property
    DynamicProperty CreateSettingsPropertyValue(
        const AZStd::string& id, const AZStd::string& displayName, const AZStd::string& description, const bool& defaultValue);

    // Helper function to create and bind a numeric registry setting to a dynamic property
    DynamicProperty CreateSettingsPropertyValue(
        const AZStd::string& id,
        const AZStd::string& displayName,
        const AZStd::string& description,
        const double& defaultValue,
        const double& minValue = AZStd::numeric_limits<double>::lowest(),
        const double& maxValue = AZStd::numeric_limits<double>::max());

    // Helper function to create and bind a numeric registry setting to a dynamic property
    DynamicProperty CreateSettingsPropertyValue(
        const AZStd::string& id,
        const AZStd::string& displayName,
        const AZStd::string& description,
        const AZ::u64& defaultValue,
        const AZ::u64& minValue = AZStd::numeric_limits<AZ::u64>::lowest(),
        const AZ::u64& maxValue = AZStd::numeric_limits<AZ::u64>::max());

    // Helper function to create and bind a numeric registry setting to a dynamic property
    DynamicProperty CreateSettingsPropertyValue(
        const AZStd::string& id,
        const AZStd::string& displayName,
        const AZStd::string& description,
        const AZ::s64& defaultValue,
        const AZ::s64& minValue = AZStd::numeric_limits<AZ::s64>::lowest(),
        const AZ::s64& maxValue = AZStd::numeric_limits<AZ::s64>::max());

    // Helper function to create and bind a numeric registry setting to a dynamic property
    template<typename T>
    DynamicProperty CreateSettingsPropertyObject(
        const AZStd::string& id, const AZStd::string& displayName, const AZStd::string& description, const T& defaultValue)
    {
        DynamicPropertyConfig config;
        config.m_id = id;
        config.m_name = displayName;
        config.m_displayName = displayName;
        config.m_description = description;
        config.m_defaultValue = config.m_originalValue = config.m_parentValue = GetSettingsObject<T>(id, defaultValue);
        config.m_dataChangeCallback = [id](const AZStd::any& value)
        {
            SetSettingsObject<T>(id, AZStd::any_cast<T>(value));
            return AZ::Edit::PropertyRefreshLevels::AttributesAndValues;
        };
        return DynamicProperty(config);
    }
} // namespace AtomToolsFramework
