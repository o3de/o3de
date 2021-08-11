/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <ImplementationManager.h>

#include <AzFramework/Application/Application.h>

#include <ATLControlsModel.h>
#include <AudioControlsEditorPlugin.h>
#include <IAudioSystemEditor.h>


//-----------------------------------------------------------------------------------------------//
CImplementationManager::CImplementationManager()
{
}

//-----------------------------------------------------------------------------------------------//
bool CImplementationManager::LoadImplementation()
{
    AudioControls::CATLControlsModel* pATLModel = CAudioControlsEditorPlugin::GetATLModel();
    if (pATLModel)
    {
        pATLModel->ClearAllConnections();

        // release the loaded implementation (if any)
        Release();

        const char* engineRoot = nullptr;
        AzFramework::ApplicationRequests::Bus::BroadcastResult(engineRoot, &AzFramework::ApplicationRequests::GetEngineRoot);
        AZ_Assert(engineRoot != nullptr, "Unable to communicate with AzFramework::ApplicationRequests::Bus");

        AudioControlsEditor::EditorImplPluginEventBus::Broadcast(&AudioControlsEditor::EditorImplPluginEventBus::Events::InitializeEditorImplPlugin);
    }
    else
    {
        return false;
    }

    ImplementationChanged();
    return true;
}

//-----------------------------------------------------------------------------------------------//
void CImplementationManager::Release()
{
    AudioControlsEditor::EditorImplPluginEventBus::Broadcast(&AudioControlsEditor::EditorImplPluginEventBus::Events::ReleaseEditorImplPlugin);
}

//-----------------------------------------------------------------------------------------------//
AudioControls::IAudioSystemEditor* CImplementationManager::GetImplementation()
{
    AudioControls::IAudioSystemEditor* editor = nullptr;
    AudioControlsEditor::EditorImplPluginEventBus::BroadcastResult(editor, &AudioControlsEditor::EditorImplPluginEventBus::Events::GetEditorImplPlugin);
    return editor;
}

#include <Source/Editor/moc_ImplementationManager.cpp>
