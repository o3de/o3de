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

#include <Atom/RPI.Edit/Common/AssetUtils.h>
#include <Atom/RPI.Public/RPISystemInterface.h>
#include <AtomToolsFramework/Util/Util.h>

#include <ShaderManagementConsoleApplication.h>
#include <ShaderManagementConsole_Traits_Platform.h>

#include <Atom/Document/ShaderManagementConsoleDocumentModule.h>
#include <Atom/Document/ShaderManagementConsoleDocumentSystemRequestBus.h>
#include <Atom/Window/ShaderManagementConsoleWindowModule.h>
#include <Atom/Window/ShaderManagementConsoleWindowRequestBus.h>

AZ_PUSH_DISABLE_WARNING(4251 4800, "-Wunknown-warning-option") // disable warnings spawned by QT
#include <QFileInfo>
#include <QMessageBox>
#include <QObject>
AZ_POP_DISABLE_WARNING

namespace ShaderManagementConsole
{
    AZStd::string ShaderManagementConsoleApplication::GetBuildTargetName() const
    {
#if !defined(LY_CMAKE_TARGET)
#error "LY_CMAKE_TARGET must be defined in order to add this source file to a CMake executable target"
#endif
        return AZStd::string_view{ LY_CMAKE_TARGET };
    }

    const char* ShaderManagementConsoleApplication::GetCurrentConfigurationName() const
    {
#if defined(_RELEASE)
        return "ReleaseShaderManagementConsole";
#elif defined(_DEBUG)
        return "DebugShaderManagementConsole";
#else
        return "ProfileShaderManagementConsole";
#endif
    }

    ShaderManagementConsoleApplication::ShaderManagementConsoleApplication(int* argc, char*** argv)
        : AtomToolsApplication(argc, argv)
    {
        QApplication::setApplicationName("O3DE Shader Management Console");

        // The settings registry has been created at this point, so add the CMake target
        AZ::SettingsRegistryMergeUtils::MergeSettingsToRegistry_AddBuildSystemTargetSpecialization(
            *AZ::SettingsRegistry::Get(), GetBuildTargetName());
    }

    void ShaderManagementConsoleApplication::CreateStaticModules(AZStd::vector<AZ::Module*>& outModules)
    {
        Base::CreateStaticModules(outModules);
        outModules.push_back(aznew ShaderManagementConsoleDocumentModule);
        outModules.push_back(aznew ShaderManagementConsoleWindowModule);
    }

    void ShaderManagementConsoleApplication::OnShaderManagementConsoleWindowClosing()
    {
        ExitMainLoop();
        ShaderManagementConsoleWindowNotificationBus::Handler::BusDisconnect();
        AzToolsFramework::EditorPythonConsoleNotificationBus::Handler::BusDisconnect();
    }

    void ShaderManagementConsoleApplication::Destroy()
    {
        // before modules are unloaded, destroy UI to free up any assets it cached
        ShaderManagementConsole::ShaderManagementConsoleWindowRequestBus::Broadcast(
            &ShaderManagementConsole::ShaderManagementConsoleWindowRequestBus::Handler::DestroyShaderManagementConsoleWindow);

        ShaderManagementConsoleWindowNotificationBus::Handler::BusDisconnect();

        Base::Destroy();
    }

    AZStd::vector<AZStd::string> ShaderManagementConsoleApplication::GetCriticalAssetFilters() const
    {
        return AZStd::vector<AZStd::string>({ "passes/", "config/" });
    }

    void ShaderManagementConsoleApplication::ProcessCommandLine()
    {
        // Process command line options for running one or more python scripts on startup
        const AZStd::string runPythonScriptSwitchName = "runpython";
        size_t runPythonScriptCount = m_commandLine.GetNumSwitchValues(runPythonScriptSwitchName);
        for (size_t runPythonScriptIndex = 0; runPythonScriptIndex < runPythonScriptCount; ++runPythonScriptIndex)
        {
            const AZStd::string runPythonScriptPath = m_commandLine.GetSwitchValue(runPythonScriptSwitchName, runPythonScriptIndex);
            AZStd::vector<AZStd::string_view> runPythonArgs;
            AzToolsFramework::EditorPythonRunnerRequestBus::Broadcast(
                &AzToolsFramework::EditorPythonRunnerRequestBus::Events::ExecuteByFilenameWithArgs, runPythonScriptPath, runPythonArgs);
        }

        // Process command line options for opening one or more documents on startup
        size_t openDocumentCount = m_commandLine.GetNumMiscValues();
        for (size_t openDocumentIndex = 0; openDocumentIndex < openDocumentCount; ++openDocumentIndex)
        {
            const AZStd::string openDocumentPath = m_commandLine.GetMiscValue(openDocumentIndex);
            ShaderManagementConsoleDocumentSystemRequestBus::Broadcast(
                &ShaderManagementConsoleDocumentSystemRequestBus::Events::OpenDocument, openDocumentPath);
        }
    }

    void ShaderManagementConsoleApplication::StartInternal()
    {
        Base::StartInternal();

        ShaderManagementConsoleWindowNotificationBus::Handler::BusConnect();

        ShaderManagementConsole::ShaderManagementConsoleWindowRequestBus::Broadcast(
            &ShaderManagementConsole::ShaderManagementConsoleWindowRequestBus::Handler::CreateShaderManagementConsoleWindow);
    }
} // namespace ShaderManagementConsole
