/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/API/ViewPaneOptions.h>

#include <Include/IPlugin.h>

#include "ProjectSettingsToolWindow.h"
#include "../Editor/LyViewPaneNames.h"


class ProjectSettingsToolPlugin
    : public IPlugin
{
public:
    ProjectSettingsToolPlugin([[maybe_unused]] IEditor* editor)
    {
        AzToolsFramework::ViewPaneOptions options;
        options.showInMenu = false;
        AzToolsFramework::RegisterViewPane<ProjectSettingsTool::ProjectSettingsToolWindow>(LyViewPane::ProjectSettingsTool, LyViewPane::ProjectSettingsTool, options);
    }

    void Release() override
    {
        AzToolsFramework::UnregisterViewPane(LyViewPane::ProjectSettingsTool);
        delete this;
    }

    void ShowAbout() override {}

    const char* GetPluginGUID() override { return "{C5B96A1A-036A-46F9-B7F0-5DF93494F988}"; }
    DWORD GetPluginVersion() override { return 1; }
    const char* GetPluginName() override { return "ProjectSettingsTool"; }
    bool CanExitNow() override { return true; }
    void OnEditorNotify([[maybe_unused]] EEditorNotifyEvent aEventId) override {}
};

PLUGIN_API IPlugin* CreatePluginInstance(PLUGIN_INIT_PARAM* pInitParam)
{
    ISystem* pSystem = pInitParam->pIEditorInterface->GetSystem();
    ModuleInitISystem(pSystem, "ProjectSettingsTool");
    // the above line initializes the gEnv global variable if necessary, and also makes GetIEditor() and other similar functions work correctly.

    return new ProjectSettingsToolPlugin(GetIEditor());
}

#if defined(AZ_PLATFORM_WINDOWS)
HINSTANCE g_hInstance = 0;
BOOL __stdcall DllMain(HINSTANCE hinstDLL, ULONG fdwReason, [[maybe_unused]] LPVOID lpvReserved)
{
    if (fdwReason == DLL_PROCESS_ATTACH)
    {
        g_hInstance = hinstDLL;
    }

    return TRUE;
}
#endif
