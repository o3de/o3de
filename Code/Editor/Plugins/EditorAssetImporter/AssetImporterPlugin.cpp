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
    , m_assetBrowserContextProvider()
    , m_sceneSerializationHandler()
{
    s_instance = this;

    m_sceneUIModule = LoadSceneLibrary("SceneUI", true);

    m_sceneSerializationHandler.Activate();

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
    if (m_assetImporterWindow)
    {
        delete m_assetImporterWindow;
    }

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
        module->Load();

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
    if (!m_assetImporterWindow)
    {
        return nullptr;
    }

    AssetImporterWindow* assetImporterWindow = qobject_cast<AssetImporterWindow*>(m_assetImporterWindow);
    if (!assetImporterWindow)
    {
        return nullptr;
    }

    assetImporterWindow->OpenFile(sourceFilePath);
    return m_assetImporterWindow;
}

QMainWindow* AssetImporterPlugin::OpenImportSettings()
{
    // Can only be one asset importer window at a time
    if (m_assetImporterWindow)
    {
        return nullptr;
    }

    m_assetImporterWindow = new AssetImporterWindow();
    return m_assetImporterWindow;
}

bool AssetImporterPlugin::SaveBeforeClosing()
{
    if (!m_assetImporterWindow)
    {
        return false;
    }

    AssetImporterWindow* assetImporterWindow = qobject_cast<AssetImporterWindow*>(m_assetImporterWindow);
    if (!assetImporterWindow)
    {
        return false;
    }

    bool canClose = assetImporterWindow->CanClose();
    if (canClose)
    {
        delete m_assetImporterWindow;
    }

    return !canClose;
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
