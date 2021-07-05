/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
        m_documentSettings =
            AZ::UserSettings::CreateFind<MaterialDocumentSettings>(AZ::Crc32("MaterialDocumentSettings"), AZ::UserSettings::CT_GLOBAL);
    }

    SettingsWidget::~SettingsWidget()
    {
        AtomToolsFramework::InspectorRequestBus::Handler::BusDisconnect();
    }

    void SettingsWidget::Populate()
    {
        AddGroupsBegin();
        AddDocumentGroup();
        AddGroupsEnd();
    }

    void SettingsWidget::AddDocumentGroup()
    {
        const AZStd::string groupNameId = "documentSettings";
        const AZStd::string groupDisplayName = "Document Settings";
        const AZStd::string groupDescription = "Document Settings";

        const AZ::Crc32 saveStateKey(AZStd::string::format("SettingsWidget::DocumentGroup"));
        AddGroup(
            groupNameId, groupDisplayName, groupDescription,
            new AtomToolsFramework::InspectorPropertyGroupWidget(
                m_documentSettings.get(), nullptr, m_documentSettings->TYPEINFO_Uuid(), this, this, saveStateKey));
    }

    void SettingsWidget::Reset()
    {
        AtomToolsFramework::InspectorRequestBus::Handler::BusDisconnect();
        AtomToolsFramework::InspectorWidget::Reset();
    }

    void SettingsWidget::BeforePropertyModified(AzToolsFramework::InstanceDataNode* pNode)
    {
        AZ_UNUSED(pNode);
    }

    void SettingsWidget::AfterPropertyModified(AzToolsFramework::InstanceDataNode* pNode)
    {
        AZ_UNUSED(pNode);
    }

    void SettingsWidget::SetPropertyEditingComplete(AzToolsFramework::InstanceDataNode* pNode)
    {
        AZ_UNUSED(pNode);
    }
} // namespace MaterialEditor

//#include <Window/SettingsWidget/moc_SettingsWidget.cpp>
