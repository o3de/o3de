/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <AudioControlsEditorPlugin.h>

#include <AudioControl.h>
#include <AudioControlsEditorWindow.h>
#include <AudioControlsLoader.h>
#include <AudioControlsWriter.h>

#include <IAudioSystem.h>
#include <IAudioSystemEditor.h>
#include <ImplementationManager.h>

#include <MathConversion.h>
#include <QtViewPaneManager.h>

#include <AzFramework/Components/CameraBus.h>


using namespace AudioControls;

CATLControlsModel CAudioControlsEditorPlugin::ms_ATLModel;
QATLTreeModel CAudioControlsEditorPlugin::ms_layoutModel;
FilepathSet CAudioControlsEditorPlugin::ms_currentFilenames;
CImplementationManager CAudioControlsEditorPlugin::ms_implementationManager;
Audio::TAudioControlID CAudioControlsEditorPlugin::ms_audioTriggerId = INVALID_AUDIO_CONTROL_ID;

//-----------------------------------------------------------------------------------------------//
CAudioControlsEditorPlugin::CAudioControlsEditorPlugin(IEditor* editor)
{
    QtViewOptions options;
    options.canHaveMultipleInstances = true;
    RegisterQtViewPane<CAudioControlsEditorWindow>(editor, LyViewPane::AudioControlsEditor, LyViewPane::CategoryOther, options);

    ms_implementationManager.LoadImplementation();
    ReloadModels();
    ms_layoutModel.Initialize(&ms_ATLModel);
}

//-----------------------------------------------------------------------------------------------//
CAudioControlsEditorPlugin::~CAudioControlsEditorPlugin()
{
    Release();
}

//-----------------------------------------------------------------------------------------------//
void CAudioControlsEditorPlugin::Release()
{
    // clear connections before releasing the implementation since they hold pointers to data
    // instantiated from the implementation dll.
    CUndoSuspend suspendUndo;
    ms_ATLModel.ClearAllConnections();
    ms_implementationManager.Release();
}

//-----------------------------------------------------------------------------------------------//
void CAudioControlsEditorPlugin::SaveModels()
{
    IAudioSystemEditor* pImpl = GetAudioSystemEditorImpl();
    if (pImpl)
    {
        CAudioControlsWriter writer(&ms_ATLModel, &ms_layoutModel, pImpl, ms_currentFilenames);
    }
}

//-----------------------------------------------------------------------------------------------//
void CAudioControlsEditorPlugin::ReloadModels()
{
    GetIEditor()->SuspendUndo();
    ms_ATLModel.SetSuppressMessages(true);

    IAudioSystemEditor* pImpl = GetAudioSystemEditorImpl();
    if (pImpl)
    {
        ms_layoutModel.clear();
        ms_ATLModel.Clear();
        pImpl->Reload();
        CAudioControlsLoader ATLLoader(&ms_ATLModel, &ms_layoutModel, pImpl);
        ATLLoader.LoadAll();
        ms_currentFilenames = ATLLoader.GetLoadedFilenamesList();
    }

    ms_ATLModel.SetSuppressMessages(false);
    GetIEditor()->ResumeUndo();
}

//-----------------------------------------------------------------------------------------------//
void CAudioControlsEditorPlugin::ReloadScopes()
{
    IAudioSystemEditor* pImpl = GetAudioSystemEditorImpl();
    if (pImpl)
    {
        ms_ATLModel.ClearScopes();
        CAudioControlsLoader ATLLoader(&ms_ATLModel, &ms_layoutModel, pImpl);
        ATLLoader.LoadScopes();
    }
}

//-----------------------------------------------------------------------------------------------//
CATLControlsModel* CAudioControlsEditorPlugin::GetATLModel()
{
    return &ms_ATLModel;
}

//-----------------------------------------------------------------------------------------------//
IAudioSystemEditor* CAudioControlsEditorPlugin::GetAudioSystemEditorImpl()
{
    return ms_implementationManager.GetImplementation();
}

//-----------------------------------------------------------------------------------------------//
QATLTreeModel* CAudioControlsEditorPlugin::GetControlsTree()
{
    return &ms_layoutModel;
}

//-----------------------------------------------------------------------------------------------//
void CAudioControlsEditorPlugin::ExecuteTrigger(const AZStd::string_view sTriggerName)
{
    if (!sTriggerName.empty())
    {
        auto audioSystem = AZ::Interface<Audio::IAudioSystem>::Get();
        if (!audioSystem)
        {
            return;
        }

        StopTriggerExecution();
        if (ms_audioTriggerId = audioSystem->GetAudioTriggerID(sTriggerName.data());
            ms_audioTriggerId != INVALID_AUDIO_CONTROL_ID)
        {
            Audio::ObjectRequest::ExecuteTrigger execTrigger;
            execTrigger.m_triggerId = ms_audioTriggerId;
            audioSystem->PushRequest(AZStd::move(execTrigger));
        }
    }
}

//-----------------------------------------------------------------------------------------------//
void CAudioControlsEditorPlugin::StopTriggerExecution()
{
    if (ms_audioTriggerId != INVALID_AUDIO_CONTROL_ID)
    {
        if (auto audioSystem = AZ::Interface<Audio::IAudioSystem>::Get();
            audioSystem != nullptr)
        {
            Audio::ObjectRequest::StopTrigger stopTrigger;
            stopTrigger.m_triggerId = ms_audioTriggerId;
            audioSystem->PushRequest(AZStd::move(stopTrigger));
            ms_audioTriggerId = INVALID_AUDIO_CONTROL_ID;
        }
    }
}

//-----------------------------------------------------------------------------------------------//
CImplementationManager* CAudioControlsEditorPlugin::GetImplementationManager()
{
    return &ms_implementationManager;
}
