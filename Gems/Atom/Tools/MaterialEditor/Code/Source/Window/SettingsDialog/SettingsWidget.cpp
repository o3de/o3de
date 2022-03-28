/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AtomToolsFramework/Inspector/InspectorPropertyGroupWidget.h>
#include <Window/SettingsDialog/SettingsWidget.h>

namespace MaterialEditor
{
    SettingsWidget::SettingsWidget(QWidget* parent)
        : AtomToolsFramework::InspectorWidget(parent)
    {
        SetGroupSettingsPrefix("/O3DE/Atom/MaterialEditor/SettingsWidget");
    }

    SettingsWidget::~SettingsWidget()
    {
    }

    void SettingsWidget::Populate()
    {
        AddGroupsBegin();
        AddGroupsEnd();
    }

<<<<<<< HEAD
    void SettingsWidget::AddDocumentSettingsGroup()
    {
        const AZStd::string groupName = "documentSettings";
        const AZStd::string groupDisplayName = "Document Settings";
        const AZStd::string groupDescription = "Document Settings";

        const AZ::Crc32 saveStateKey(AZStd::string::format("SettingsWidget::DocumentSettingsGroup"));
        AddGroup(
            groupName, groupDisplayName, groupDescription,
            new AtomToolsFramework::InspectorPropertyGroupWidget(
                m_documentSettings.get(), nullptr, m_documentSettings->TYPEINFO_Uuid(), this, this, saveStateKey));
    }

    void SettingsWidget::AddDocumentSystemSettingsGroup()
    {
        const AZStd::string groupName = "documentSystemSettings";
        const AZStd::string groupDisplayName = "Document System Settings";
        const AZStd::string groupDescription = "Document System Settings";

        const AZ::Crc32 saveStateKey(AZStd::string::format("SettingsWidget::DocumentSystemSettingsGroup"));
        AddGroup(
            groupName, groupDisplayName, groupDescription,
            new AtomToolsFramework::InspectorPropertyGroupWidget(
                m_documentSystemSettings.get(), nullptr, m_documentSystemSettings->TYPEINFO_Uuid(), this, this, saveStateKey));
    }

=======
>>>>>>> development
    void SettingsWidget::Reset()
    {
        AtomToolsFramework::InspectorWidget::Reset();
    }
} // namespace MaterialEditor

//#include <Window/SettingsWidget/moc_SettingsWidget.cpp>
