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

    void SettingsWidget::Reset()
    {
        AtomToolsFramework::InspectorWidget::Reset();
    }
} // namespace MaterialEditor

//#include <Window/SettingsWidget/moc_SettingsWidget.cpp>
