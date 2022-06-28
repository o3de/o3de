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
#include <AtomToolsFramework/Inspector/InspectorWidget.h>
#include <QDialog>
#endif

namespace AtomToolsFramework
{
    //! Modal dialog for displaying a list of property groups wrapping registry settings
    class SettingsDialog : public QDialog
    {
        Q_OBJECT
    public:
        SettingsDialog(AZStd::vector<AZStd::shared_ptr<DynamicPropertyGroup>> groups, QWidget* parent = nullptr);
        ~SettingsDialog() = default;

    private:
        InspectorWidget* m_inspectorWidget = {};
        AZStd::vector<AZStd::shared_ptr<DynamicPropertyGroup>> m_groups;
    };

    // Create a basic dynamic property group with display name and description configured
    AZStd::shared_ptr<DynamicPropertyGroup> CreateSettingsGroup(
        const AZStd::string& displayName,
        const AZStd::string& description,
        AZStd::vector<DynamicProperty> properties,
        AZStd::vector<AZStd::shared_ptr<DynamicPropertyGroup>> groups = {});

    // Helper function to create and bind a string registry setting to a dynamic property
    DynamicProperty CreatePropertyFromSetting(
        const AZStd::string& id, const AZStd::string& displayName, const AZStd::string& description, const AZStd::string& defaultValue);

    // Helper function to create and bind a bool registry setting to a dynamic property
    DynamicProperty CreatePropertyFromSetting(
        const AZStd::string& id, const AZStd::string& displayName, const AZStd::string& description, const bool& defaultValue);

    // Helper function to create and bind a numeric registry setting to a dynamic property
    DynamicProperty CreatePropertyFromSetting(
        const AZStd::string& id, const AZStd::string& displayName, const AZStd::string& description, const double& defaultValue);

    // Helper function to create and bind a numeric registry setting to a dynamic property
    DynamicProperty CreatePropertyFromSetting(
        const AZStd::string& id, const AZStd::string& displayName, const AZStd::string& description, const AZ::u64& defaultValue);

    // Helper function to create and bind a numeric registry setting to a dynamic property
    DynamicProperty CreatePropertyFromSetting(
        const AZStd::string& id, const AZStd::string& displayName, const AZStd::string& description, const AZ::s64& defaultValue);
} // namespace AtomToolsFramework
