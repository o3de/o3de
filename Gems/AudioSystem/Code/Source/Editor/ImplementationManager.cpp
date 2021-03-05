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

#include <ImplementationManager.h>

#include <AzFramework/Application/Application.h>

#include <ATLControlsModel.h>
#include <AudioControlsEditorPlugin.h>
#include <IAudioSystemEditor.h>
#include <IConsole.h>
#include <IEditor.h>


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
