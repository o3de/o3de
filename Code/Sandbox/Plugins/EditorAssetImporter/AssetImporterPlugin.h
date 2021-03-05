#pragma once

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

/////////////////////////////////////////////////////////////////////////////
//
// Asset Importer Sandbox Plugin
//
/////////////////////////////////////////////////////////////////////////////

#include <IEditor.h>
#include <Include/IPlugin.h>
#include <AzCore/Module/DynamicModuleHandle.h>
#include <AssetBrowserContextProvider.h>
#include <SceneSerializationHandler.h>

class AssetImporterPlugin
    : public IPlugin
{
    // Create plugin instance, only accessible to CreatePluginInstance
    // If you need instance, use static GetInstance()
    friend IPlugin* ::CreatePluginInstance(PLUGIN_INIT_PARAM* initParam);
    AssetImporterPlugin(IEditor* editor);

public:
    // Get the singleton instance of the plugin
    static AssetImporterPlugin* GetInstance()
    {
        return s_instance;
    }

    // Get the editor used to create this plugin
    IEditor* GetIEditor()
    {
        return m_editor;
    }

    const string& GetToolName() const
    {
        return m_toolName;
    }

    /////////////////////////////////////////////////////////////////////////////
    // IPlugin implementation
    void Release() override;
    void ShowAbout() override
    {
    }
    const char* GetPluginGUID() override
    {
        return "{0abf28f2-ef56-4ac9-a459-175abb40d649}";
    }
    DWORD GetPluginVersion() override
    {
        return 1;
    }
    const char* GetPluginName() override
    {
        return "QtAssetImporter";
    }
    bool CanExitNow() override
    {
        return true;
    }
    void OnEditorNotify([[maybe_unused]] EEditorNotifyEvent aEventId) override
    {
    }
    /////////////////////////////////////////////////////////////////////////////

    void EditImportSettings(const AZStd::string& sourceFilePath);

private:
    AZStd::unique_ptr<AZ::DynamicModuleHandle> LoadSceneLibrary(const char* name, bool explicitInit);
    
    // Singleton instance
    static AssetImporterPlugin* s_instance;

    // Dependency DLL Handles
    AZStd::unique_ptr<AZ::DynamicModuleHandle> m_sceneUIModule;
    
    // The editor used to construct the plugin
    IEditor* const m_editor;

    // Tool name
    string m_toolName;

    // Context provider for the Asset Browser
    AZ::AssetBrowserContextProvider m_assetBrowserContextProvider;
    AZ::SceneSerializationHandler m_sceneSerializationHandler;
};