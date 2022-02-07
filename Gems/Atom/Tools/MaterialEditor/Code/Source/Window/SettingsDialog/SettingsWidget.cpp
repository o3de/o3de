/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Window/SettingsDialog/SettingsWidget.h>
#include <AtomToolsFramework/Inspector/InspectorPropertyGroupWidget.h>

namespace MaterialEditor
{
    SettingsWidget::SettingsWidget(QWidget* parent)
        : AtomToolsFramework::InspectorWidget(parent)
    {
        m_documentSystemSettings = AZ::UserSettings::CreateFind<AtomToolsFramework::AtomToolsDocumentSystemSettings>(
            AZ_CRC_CE("AtomToolsDocumentSystemSettings"), AZ::UserSettings::CT_GLOBAL);
    }

    SettingsWidget::~SettingsWidget()
    {
        AtomToolsFramework::InspectorRequestBus::Handler::BusDisconnect();
    }

    void SettingsWidget::Populate()
    {
        AddGroupsBegin();
        AddDocumentSystemSettingsGroup();
        AddGroupsEnd();
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
                m_documentSystemSettings.get(), nullptr, m_documentSystemSettings->TYPEINFO_Uuid(), nullptr, this, saveStateKey));
    }

    void SettingsWidget::Reset()
    {
        AtomToolsFramework::InspectorRequestBus::Handler::BusDisconnect();
        AtomToolsFramework::InspectorWidget::Reset();
    }
} // namespace MaterialEditor

//#include <Window/SettingsWidget/moc_SettingsWidget.cpp>
