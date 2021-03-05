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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#pragma once

#include <AzCore/std/containers/set.h>
#include <AzCore/std/string/string_view.h>

#include <ATLControlsModel.h>
#include <IAudioInterfacesCommonData.h>
#include <IAudioSystemEditor.h>
#include <QATLControlsTreeModel.h>

#include <IEditor.h>
#include <Include/IPlugin.h>

#include <QStandardItem>

namespace Audio
{
    struct IAudioProxy;
} // namespace Audio


class CImplementationManager;

//-------------------------------------------------------------------------------------------//
class CAudioControlsEditorPlugin
    : public IPlugin
    , public ISystemEventListener
{
public:
    explicit CAudioControlsEditorPlugin(IEditor* editor);
    ~CAudioControlsEditorPlugin() override;

    void Release() override;
    void ShowAbout() override {}
    const char* GetPluginGUID() override { return "{DDD96BF3-892E-4A75-ABF7-BBAE446972DA}"; }
    DWORD GetPluginVersion() override { return 2; }
    const char* GetPluginName() override { return "AudioControlsEditor"; }
    bool CanExitNow() override { return true; }
    void OnEditorNotify([[maybe_unused]] EEditorNotifyEvent aEventId) override {}

    static void SaveModels();
    static void ReloadModels();
    static void ReloadScopes();
    static AudioControls::CATLControlsModel* GetATLModel();
    static AudioControls::QATLTreeModel* GetControlsTree();
    static CImplementationManager* GetImplementationManager();
    static AudioControls::IAudioSystemEditor* GetAudioSystemEditorImpl();
    static void ExecuteTrigger(const AZStd::string_view sTriggerName);
    static void StopTriggerExecution();

    ///////////////////////////////////////////////////////////////////////////
    // ISystemEventListener
    void OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam) override;
    ///////////////////////////////////////////////////////////////////////////

private:
    static AudioControls::CATLControlsModel ms_ATLModel;
    static AudioControls::QATLTreeModel ms_layoutModel;
    static AudioControls::FilepathSet ms_currentFilenames;
    static Audio::IAudioProxy* ms_pIAudioProxy;
    static Audio::TAudioControlID ms_nAudioTriggerID;

    static CImplementationManager ms_implementationManager;
};
