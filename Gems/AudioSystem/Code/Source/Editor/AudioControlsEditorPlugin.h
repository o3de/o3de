/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#pragma once

#include <AzCore/std/containers/set.h>
#include <AzCore/std/string/string_view.h>

#include <ATLControlsModel.h>
#include <AudioResourceSelectors.h>
#include <IAudioInterfacesCommonData.h>
#include <IAudioSystemEditor.h>
#include <QATLControlsTreeModel.h>

#include <IEditor.h>
#include <Include/IPlugin.h>

#include <QStandardItem>


class CImplementationManager;

//-------------------------------------------------------------------------------------------//
class CAudioControlsEditorPlugin
    : public IPlugin
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

private:
    static AudioControls::CATLControlsModel ms_ATLModel;
    static AudioControls::QATLTreeModel ms_layoutModel;
    static AudioControls::FilepathSet ms_currentFilenames;
    static CImplementationManager ms_implementationManager;
    static Audio::TAudioControlID ms_audioTriggerId;

    AudioControls::AudioControlSelectorHandler m_controlSelector;
};
