/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
