#pragma once

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
#include <QPointer>

class AssetImporterWindow;

//! Python interface for scene settings
class SceneSettingsAssetImporterForScriptRequests
    : public AZ::EBusTraits
{
public:
    //////////////////////////////////////////////////////////////////////////
    // EBusTraits overrides
    static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
    static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
    //////////////////////////////////////////////////////////////////////////

    //! Opens the scene settings tool to the specified source asset path.
    //! Returns the window ID of the viewpane, because Python can't have QObjects sent to it.
    virtual AZ::u64 EditImportSettings(const AZStd::string& sourceFilePath) = 0;
};
using SceneSettingsAssetImporterForScriptRequestBus = AZ::EBus<SceneSettingsAssetImporterForScriptRequests>;

class SceneSettingsAssetImporterForScriptRequestHandler
    : protected SceneSettingsAssetImporterForScriptRequestBus::Handler
{
public:
    AZ_RTTI(SceneSettingsAssetImporterForScriptRequestHandler, "{C3B9DCFC-CD41-4130-B295-485905A7CECB}");
    SceneSettingsAssetImporterForScriptRequestHandler();
    ~SceneSettingsAssetImporterForScriptRequestHandler();

    static void Reflect(AZ::ReflectContext* context);
    AZ::u64 EditImportSettings(const AZStd::string& sourceFilePath) override;
};

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

    const AZStd::string& GetToolName() const
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

    QMainWindow* EditImportSettings(const AZStd::string& sourceFilePath);
    QMainWindow* OpenImportSettings();
    bool SaveBeforeClosing();

private:
    AZStd::unique_ptr<AZ::DynamicModuleHandle> LoadSceneLibrary(const char* name, bool explicitInit);
    
    // Singleton instance
    static AssetImporterPlugin* s_instance;

    // The asset importer window
    QPointer<QMainWindow> m_assetImporterWindow;

    // Dependency DLL Handles
    AZStd::unique_ptr<AZ::DynamicModuleHandle> m_sceneUIModule;
    
    // The editor used to construct the plugin
    IEditor* const m_editor;

    // Tool name
    AZStd::string m_toolName;

    // Context provider for the Asset Browser
    AZ::AssetBrowserContextProvider m_assetBrowserContextProvider;
    AZ::SceneSerializationHandler m_sceneSerializationHandler;
    AZStd::shared_ptr<SceneSettingsAssetImporterForScriptRequestHandler> m_requestHandler;
};
