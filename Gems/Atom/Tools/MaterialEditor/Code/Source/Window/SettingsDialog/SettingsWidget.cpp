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
    }

    SettingsWidget::~SettingsWidget()
    {
        AtomToolsFramework::InspectorRequestBus::Handler::BusDisconnect();
    }

    void SettingsWidget::Populate()
    {
        AddGroupsBegin();
        AddGroupsEnd();
    }

    void SettingsWidget::Reset()
    {
        AtomToolsFramework::InspectorRequestBus::Handler::BusDisconnect();
        AtomToolsFramework::InspectorWidget::Reset();
    }
} // namespace MaterialEditor

//#include <Window/SettingsWidget/moc_SettingsWidget.cpp>
