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
// Asset Importer Tool
//
/////////////////////////////////////////////////////////////////////////////

#include <IEditor.h>
#include <AzCore/Module/DynamicModuleHandle.h>
#include <AssetImporter/AssetBrowserContextProvider.h>
#include <AssetImporter/SceneSerializationHandler.h>
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

class AssetImporterTool
{
public:
    AssetImporterTool(IEditor* editor);
    ~AssetImporterTool();

    // Get the singleton instance of the plugin
    static AssetImporterTool* GetInstance()
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

    QMainWindow* EditImportSettings(const AZStd::string& sourceFilePath);
    QMainWindow* OpenImportSettings();
    bool SaveBeforeClosing();

private:
    AZStd::unique_ptr<AZ::DynamicModuleHandle> LoadSceneLibrary(const char* name, bool explicitInit);
    
    // Singleton instance
    static AssetImporterTool* s_instance;

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
