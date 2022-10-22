/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AssetImporterPlugin.h>
#include <AssetImporterWindow.h>
#include <AzCore/Component/ComponentApplication.h>
#include <ImporterRootDisplay.h>
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

    if (AZ::ReflectionEnvironment::GetReflectionManager())
    {
        AZ::ReflectionEnvironment::GetReflectionManager()->Reflect(
            SceneSettingsAssetImporterForScriptRequestHandler::RTTI_Type(),
            [](AZ::ReflectContext* context)
            {
                SceneSettingsAssetImporterForScriptRequestHandler::Reflect(context);
                SceneSettingsRootDisplayScriptRequestHandler::Reflect(context);
            });
    }

    m_requestHandler = AZStd::make_shared<SceneSettingsAssetImporterForScriptRequestHandler>();
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
                (*init)();
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

QMainWindow* AssetImporterPlugin::EditImportSettings(const AZStd::string& sourceFilePath)
{
    const QtViewPane* assetImporterPane = GetIEditor()->OpenView(m_toolName.c_str());

    if(!assetImporterPane)
    {
        return nullptr;
    }

    AssetImporterWindow* assetImporterWindow = qobject_cast<AssetImporterWindow*>(assetImporterPane->Widget());
    if (!assetImporterWindow)
    {
        return nullptr;
    }

    assetImporterWindow->OpenFile(sourceFilePath);
    return assetImporterWindow;
}

SceneSettingsAssetImporterForScriptRequestHandler::SceneSettingsAssetImporterForScriptRequestHandler()
{
    SceneSettingsAssetImporterForScriptRequestBus::Handler::BusConnect();
}

SceneSettingsAssetImporterForScriptRequestHandler ::~SceneSettingsAssetImporterForScriptRequestHandler()
{
    SceneSettingsAssetImporterForScriptRequestBus::Handler::BusDisconnect();
}

void SceneSettingsAssetImporterForScriptRequestHandler::Reflect(AZ::ReflectContext* context)
{
    if (auto* serialize = azrtti_cast<AZ::SerializeContext*>(context))
    {
        serialize->Class<SceneSettingsAssetImporterForScriptRequestHandler>()->Version(0);
    }

    if (AZ::BehaviorContext* behavior = azrtti_cast<AZ::BehaviorContext*>(context))
    {
        behavior->EBus<SceneSettingsAssetImporterForScriptRequestBus>("SceneSettingsAssetImporterForScriptRequestBus")
            ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
            ->Attribute(AZ::Script::Attributes::Module, "qt")
            ->Event("EditImportSettings", &SceneSettingsAssetImporterForScriptRequestBus::Events::EditImportSettings);
    }
}

AZ::u64 SceneSettingsAssetImporterForScriptRequestHandler::EditImportSettings(const AZStd::string& sourceFilePath)
{
    QMainWindow* importSettingsWindow = AssetImporterPlugin::GetInstance()->EditImportSettings(sourceFilePath);

    if (!importSettingsWindow)
    {
        // There doesn't seem to be a defined invalid Qt WId, so this matches
        // the behavior of the QtForPythonSystemComponent.cpp, which uses a window ID of 0 if it
        // can't find the real window ID.
        return 0;
    }

    // This is a helper function, to let Python invoke the scene settings tool.
    // Qt objects can't be passed back to Python, so pass the ID of the window.
    return aznumeric_cast<AZ::u64>(importSettingsWindow->winId());
}
