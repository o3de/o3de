/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AssetImporterPlugin.h>
#include <AssetImporterWindow.h>
#include <QtViewPaneManager.h>
#include <SceneAPI/SceneCore/Events/AssetImportRequest.h>
#include <SceneAPI/SceneCore/Utilities/Reporting.h>
#include <LyViewPaneNames.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>

AssetImporterPlugin* AssetImporterPlugin::s_instance;

AssetImporterPlugin::AssetImporterPlugin(IEditor* editor)
    : m_editor(editor)
    , m_toolName(LyViewPane::SceneSettings)
    , m_assetBrowserContextProvider()
    , m_sceneSerializationHandler()
{
    s_instance = this;

    m_sceneUIModule = LoadSceneLibrary("SceneUI", true);

    m_sceneSerializationHandler.Activate();
    
    AzToolsFramework::ViewPaneOptions opt;
    opt.isPreview = true;
    opt.showInMenu = false; // this view pane is used to display scene settings, but the user never opens it directly through the Tools menu
    opt.saveKeyName = "Scene Settings (PREVIEW)"; // user settings for this pane were originally saved with PREVIEW, so ensure that's how they are loaded as well, even after the PREVIEW is removed from the name
    AzToolsFramework::RegisterViewPane<AssetImporterWindow>(m_toolName.c_str(), LyViewPane::CategoryTools, opt);

    AzToolsFramework::ToolsApplicationRequestBus::Broadcast(
        &AzToolsFramework::ToolsApplicationRequests::CreateAndAddEntityFromComponentTags,
        AZStd::vector<AZ::Crc32>({ AZ::SceneAPI::Events::AssetImportRequest::GetAssetImportRequestComponentTag() }), "AssetImportersEntity");
}

void AssetImporterPlugin::Release()
{
    AzToolsFramework::UnregisterViewPane(m_toolName.c_str());
    m_sceneSerializationHandler.Deactivate();

    auto uninit = m_sceneUIModule->GetFunction<AZ::UninitializeDynamicModuleFunction>(AZ::UninitializeDynamicModuleFunctionName);
    if (uninit)
    {
        (*uninit)();
    }
    m_sceneUIModule.reset();
}

AZStd::unique_ptr<AZ::DynamicModuleHandle> AssetImporterPlugin::LoadSceneLibrary(const char* name, bool explicitInit)
{
    using ReflectFunc = void(*)(AZ::SerializeContext*);

    AZStd::unique_ptr<AZ::DynamicModuleHandle> module = AZ::DynamicModuleHandle::Create(name);
    if (module)
    {
        module->Load(false);

        if (explicitInit)
        {
            // Explicitly initialize all modules. Because we're loading them twice (link time, and now-time), we need to explicitly uninit them.
            auto init = module->GetFunction<AZ::InitializeDynamicModuleFunction>(AZ::InitializeDynamicModuleFunctionName);
            if (init)
            {
                (*init)(AZ::Environment::GetInstance());
            }
        }

        ReflectFunc reflect = module->GetFunction<ReflectFunc>("Reflect");
        if (reflect)
        {
            (*reflect)(nullptr);
        }
        return module;
    }
    else
    {
        AZ_TracePrintf(AZ::SceneAPI::Utilities::ErrorWindow, "Failed to initialize library '%s'", name);
        return nullptr;
    }
}

void AssetImporterPlugin::EditImportSettings(const AZStd::string& sourceFilePath)
{
    const QtViewPane* assetImporterPane = GetIEditor()->OpenView(m_toolName.c_str());

    if(!assetImporterPane)
    {
        return;
    }

    AssetImporterWindow* assetImporterWindow = qobject_cast<AssetImporterWindow*>(assetImporterPane->Widget());
    if (!assetImporterWindow)
    {
        return;
    }

    assetImporterWindow->OpenFile(sourceFilePath);
}
