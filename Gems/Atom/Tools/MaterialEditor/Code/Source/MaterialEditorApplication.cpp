/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/IO/Path/Path.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/Utils/Utils.h>
#include <AzFramework/Asset/AssetSystemComponent.h>
#include <AzFramework/IO/LocalFileIO.h>
#include <AzFramework/Network/AssetProcessorConnection.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzToolsFramework/API/EditorPythonConsoleBus.h>
#include <AzToolsFramework/API/EditorPythonRunnerRequestsBus.h>
#include <AzToolsFramework/Asset/AssetSystemComponent.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserComponent.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserEntry.h>
#include <AzToolsFramework/AzToolsFrameworkModule.h>
#include <AzToolsFramework/SourceControl/PerforceComponent.h>
#include <AzToolsFramework/SourceControl/SourceControlAPI.h>
#include <AzToolsFramework/Thumbnails/ThumbnailerComponent.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyManagerComponent.h>
#include <AzToolsFramework/UI/UICore/QTreeViewStateSaver.hxx>
#include <AzToolsFramework/UI/UICore/QWidgetSavedState.h>

#include <AtomToolsFramework/Util/Util.h>

#include <Atom/Document/MaterialDocumentSystemRequestBus.h>
#include <Atom/RPI.Edit/Common/AssetUtils.h>
#include <Atom/RPI.Public/RPISystemInterface.h>

#include <Atom/Document/MaterialDocumentModule.h>
#include <Atom/Viewport/MaterialViewportModule.h>
#include <Atom/Window/MaterialEditorWindowFactoryRequestBus.h>
#include <Atom/Window/MaterialEditorWindowModule.h>
#include <Atom/Window/MaterialEditorWindowRequestBus.h>

#include <MaterialEditorApplication.h>
#include <MaterialEditor_Traits_Platform.h>

AZ_PUSH_DISABLE_WARNING(4251 4800, "-Wunknown-warning-option") // disable warnings spawned by QT
#include <QMessageBox>
#include <QObject>
AZ_POP_DISABLE_WARNING

namespace MaterialEditor
{
    //! This function returns the build system target name of "MaterialEditor
    AZStd::string MaterialEditorApplication::GetBuildTargetName() const
    {
#if !defined(LY_CMAKE_TARGET)
#error "LY_CMAKE_TARGET must be defined in order to add this source file to a CMake executable target"
#endif
        return AZStd::string{ LY_CMAKE_TARGET };
    }

    const char* MaterialEditorApplication::GetCurrentConfigurationName() const
    {
#if defined(_RELEASE)
        return "ReleaseMaterialEditor";
#elif defined(_DEBUG)
        return "DebugMaterialEditor";
#else
        return "ProfileMaterialEditor";
#endif
    }

    MaterialEditorApplication::MaterialEditorApplication(int* argc, char*** argv)
        : AtomToolsApplication(argc, argv)
    {
        QApplication::setApplicationName("O3DE Material Editor");

        AZ::SettingsRegistryMergeUtils::MergeSettingsToRegistry_AddBuildSystemTargetSpecialization(
            *AZ::SettingsRegistry::Get(), GetBuildTargetName());
    }

    MaterialEditorApplication::~MaterialEditorApplication()
    {
        AzToolsFramework::AssetDatabase::AssetDatabaseRequestsBus::Handler::BusDisconnect();
        MaterialEditorWindowNotificationBus::Handler::BusDisconnect();
        AzToolsFramework::EditorPythonConsoleNotificationBus::Handler::BusDisconnect();
    }

    void MaterialEditorApplication::CreateStaticModules(AZStd::vector<AZ::Module*>& outModules)
    {
        Base::CreateStaticModules(outModules);
        outModules.push_back(aznew MaterialDocumentModule);
        outModules.push_back(aznew MaterialViewportModule);
        outModules.push_back(aznew MaterialEditorWindowModule);
    }

    void MaterialEditorApplication::OnMaterialEditorWindowClosing()
    {
        ExitMainLoop();
    }

    void MaterialEditorApplication::Destroy()
    {
        // before modules are unloaded, destroy UI to free up any assets it cached
        MaterialEditor::MaterialEditorWindowFactoryRequestBus::Broadcast(
            &MaterialEditor::MaterialEditorWindowFactoryRequestBus::Handler::DestroyMaterialEditorWindow);

        MaterialEditorWindowNotificationBus::Handler::BusDisconnect();

        Base::Destroy();
    }

    AZStd::vector<AZStd::string> MaterialEditorApplication::GetCriticalAssetFilters() const
    {
        return AZStd::vector<AZStd::string>({ "passes/", "config/", "MaterialEditor" });
    }

    void MaterialEditorApplication::ProcessCommandLine(const AZ::CommandLine& commandLine)
    {
        const AZStd::string activateWindowSwitchName = "activatewindow";
        if (commandLine.HasSwitch(activateWindowSwitchName))
        {
            MaterialEditor::MaterialEditorWindowRequestBus::Broadcast(
                &MaterialEditor::MaterialEditorWindowRequestBus::Handler::ActivateWindow);
        }

        // Process command line options for opening one or more material documents on startup
        size_t openDocumentCount = commandLine.GetNumMiscValues();
        for (size_t openDocumentIndex = 0; openDocumentIndex < openDocumentCount; ++openDocumentIndex)
        {
            const AZStd::string openDocumentPath = commandLine.GetMiscValue(openDocumentIndex);

            AZ_Printf(GetBuildTargetName().c_str(), "Opening document: %s", openDocumentPath.c_str());
            MaterialDocumentSystemRequestBus::Broadcast(&MaterialDocumentSystemRequestBus::Events::OpenDocument, openDocumentPath);
        }

        Base::ProcessCommandLine(commandLine);
    }

    void MaterialEditorApplication::StartInternal()
    {
        Base::StartInternal();

        MaterialEditorWindowNotificationBus::Handler::BusConnect();

        MaterialEditor::MaterialEditorWindowFactoryRequestBus::Broadcast(
            &MaterialEditor::MaterialEditorWindowFactoryRequestBus::Handler::CreateMaterialEditorWindow);
    }

    void MaterialEditorApplication::Stop()
    {
        MaterialEditor::MaterialEditorWindowFactoryRequestBus::Broadcast(
            &MaterialEditor::MaterialEditorWindowFactoryRequestBus::Handler::DestroyMaterialEditorWindow);

        Base::Stop();
    }
} // namespace MaterialEditor
