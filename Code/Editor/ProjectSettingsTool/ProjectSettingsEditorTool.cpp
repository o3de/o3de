/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ProjectSettingsEditorTool.h"

#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/API/ViewPaneOptions.h>

#include "ProjectSettingsToolWindow.h"
#include <LyViewPaneNames.h>

ProjectSettingsEditorTool::ProjectSettingsEditorTool()
{
    AzToolsFramework::ViewPaneOptions options;
    options.showInMenu = false;
    AzToolsFramework::RegisterViewPane<ProjectSettingsTool::ProjectSettingsToolWindow>(LyViewPane::ProjectSettingsTool, LyViewPane::ProjectSettingsTool, options);
}

ProjectSettingsEditorTool::~ProjectSettingsEditorTool()
{
    AzToolsFramework::UnregisterViewPane(LyViewPane::ProjectSettingsTool);
}
