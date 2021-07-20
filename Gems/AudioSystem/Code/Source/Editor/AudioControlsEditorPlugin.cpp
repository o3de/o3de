/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <AudioControlsEditorPlugin.h>

#include <AudioControl.h>
#include <AudioControlsEditorWindow.h>
#include <AudioControlsLoader.h>
#include <AudioControlsWriter.h>

#include <CryFile.h>
#include <CryPath.h>
#include <Cry_Camera.h>
#include <Include/IResourceSelectorHost.h>

#include <IAudioSystem.h>
#include <IAudioSystemEditor.h>
#include <ImplementationManager.h>

#include <MathConversion.h>
#include <QtViewPaneManager.h>


using namespace AudioControls;
using namespace PathUtil;

CATLControlsModel CAudioControlsEditorPlugin::ms_ATLModel;
QATLTreeModel CAudioControlsEditorPlugin::ms_layoutModel;
FilepathSet CAudioControlsEditorPlugin::ms_currentFilenames;
Audio::IAudioProxy* CAudioControlsEditorPlugin::ms_pIAudioProxy = nullptr;
Audio::TAudioControlID CAudioControlsEditorPlugin::ms_nAudioTriggerID = INVALID_AUDIO_CONTROL_ID;
CImplementationManager CAudioControlsEditorPlugin::ms_implementationManager;

//-----------------------------------------------------------------------------------------------//
CAudioControlsEditorPlugin::CAudioControlsEditorPlugin(IEditor* editor)
{
    QtViewOptions options;
    options.canHaveMultipleInstances = true;
    RegisterQtViewPane<CAudioControlsEditorWindow>(editor, LyViewPane::AudioControlsEditor, LyViewPane::CategoryOther, options);
    RegisterModuleResourceSelectors(GetIEditor()->GetResourceSelectorHost());

    Audio::AudioSystemRequestBus::BroadcastResult(ms_pIAudioProxy, &Audio::AudioSystemRequestBus::Events::GetFreeAudioProxy);

    if (ms_pIAudioProxy)
    {
        ms_pIAudioProxy->Initialize("AudioControlsEditor-Preview");
        ms_pIAudioProxy->SetObstructionCalcType(Audio::eAOOCT_IGNORE);
    }

    ms_implementationManager.LoadImplementation();
    ReloadModels();
    ms_layoutModel.Initialize(&ms_ATLModel);
    GetISystem()->GetISystemEventDispatcher()->RegisterListener(this);
}

//-----------------------------------------------------------------------------------------------//
CAudioControlsEditorPlugin::~CAudioControlsEditorPlugin()
{
    Release();
}

//-----------------------------------------------------------------------------------------------//
void CAudioControlsEditorPlugin::Release()
{
    UnregisterQtViewPane<CAudioControlsEditorWindow>();
    // clear connections before releasing the implementation since they hold pointers to data
    // instantiated from the implementation dll.
    CUndoSuspend suspendUndo;
    ms_ATLModel.ClearAllConnections();
    ms_implementationManager.Release();
    if (ms_pIAudioProxy)
    {
        StopTriggerExecution();
        ms_pIAudioProxy->Release();
    }
    GetISystem()->GetISystemEventDispatcher()->RemoveListener(this);
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
    if (!sTriggerName.empty() && ms_pIAudioProxy)
    {
        StopTriggerExecution();
        Audio::AudioSystemRequestBus::BroadcastResult(ms_nAudioTriggerID, &Audio::AudioSystemRequestBus::Events::GetAudioTriggerID, sTriggerName.data());
        if (ms_nAudioTriggerID != INVALID_AUDIO_CONTROL_ID)
        {
            const CCamera& camera = GetIEditor()->GetSystem()->GetViewCamera();

            Audio::SAudioRequest request;
            request.nFlags = Audio::eARF_PRIORITY_NORMAL;

            const AZ::Matrix3x4 cameraMatrix = LYTransformToAZMatrix3x4(camera.GetMatrix());

            Audio::SAudioListenerRequestData<Audio::eALRT_SET_POSITION> requestData(cameraMatrix);
            requestData.oNewPosition.NormalizeForwardVec();
            requestData.oNewPosition.NormalizeUpVec();
            request.pData = &requestData;
            Audio::AudioSystemRequestBus::Broadcast(&Audio::AudioSystemRequestBus::Events::PushRequest, request);

            ms_pIAudioProxy->SetPosition(cameraMatrix);
            ms_pIAudioProxy->ExecuteTrigger(ms_nAudioTriggerID);
        }
    }
}

//-----------------------------------------------------------------------------------------------//
void CAudioControlsEditorPlugin::StopTriggerExecution()
{
    if (ms_pIAudioProxy && ms_nAudioTriggerID != INVALID_AUDIO_CONTROL_ID)
    {
        ms_pIAudioProxy->StopTrigger(ms_nAudioTriggerID);
        ms_nAudioTriggerID = INVALID_AUDIO_CONTROL_ID;
    }
}

//-----------------------------------------------------------------------------------------------//
void CAudioControlsEditorPlugin::OnSystemEvent(ESystemEvent event, [[maybe_unused]] UINT_PTR wparam, [[maybe_unused]] UINT_PTR lparam)
{
    switch (event)
    {
    case ESYSTEM_EVENT_AUDIO_IMPLEMENTATION_LOADED:
        GetIEditor()->SuspendUndo();
        ms_implementationManager.LoadImplementation();
        GetIEditor()->ResumeUndo();
        break;
    }
}

//-----------------------------------------------------------------------------------------------//
CImplementationManager* CAudioControlsEditorPlugin::GetImplementationManager()
{
    return &ms_implementationManager;
}

//-----------------------------------------------------------------------------------------------//
template<>
REFGUID CQtViewClass<AudioControls::CAudioControlsEditorWindow>::GetClassID()
{
    // {82AD1635-38A6-4642-A801-EAB7A829411B}
    static const GUID guid =
    {
        0x82AD1635, 0x38A6, 0x4642, { 0xA8, 0x01, 0xEA, 0xB7, 0xA8, 0x29, 0x41, 0x1B }
    };
    return guid;
}
