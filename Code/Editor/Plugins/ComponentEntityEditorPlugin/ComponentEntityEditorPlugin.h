/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once
#include <IEditor.h>
#include <Include/IPlugin.h>

//------------------------------------------------------------------
class ComponentEntityEditorPlugin
    : public IPlugin
{
public:
    ComponentEntityEditorPlugin(IEditor* editor);
    virtual ~ComponentEntityEditorPlugin() = default;

    void Release() override;
    void ShowAbout() override {}
    const char* GetPluginGUID() override { return "{11B0041C-BC34-4827-A3E4-AB7458FFF678}"; }
    DWORD GetPluginVersion() override { return 1; }
    const char* GetPluginName() override { return "ComponentEntityEditor"; }
    bool CanExitNow() override { return true; }
    void OnEditorNotify([[maybe_unused]] EEditorNotifyEvent aEventId) override {}

private:
    bool m_registered;
    class SandboxIntegrationManager* m_appListener;
};
