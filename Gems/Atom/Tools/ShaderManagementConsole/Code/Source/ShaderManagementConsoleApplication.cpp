/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/Document/ShaderManagementConsoleDocumentModule.h>
#include <Atom/Document/ShaderManagementConsoleDocumentSystemRequestBus.h>
#include <Atom/RPI.Edit/Common/AssetUtils.h>
#include <Atom/RPI.Public/RPISystemInterface.h>
#include <Atom/Window/ShaderManagementConsoleWindowModule.h>

#include <AtomToolsFramework/Util/Util.h>

#include <AzCore/IO/Path/Path.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/Utils/Utils.h>

#include <AzFramework/StringFunc/StringFunc.h>
#include <AzFramework/IO/LocalFileIO.h>
#include <AzFramework/Network/AssetProcessorConnection.h>

#include <AzToolsFramework/AzToolsFrameworkModule.h>
#include <AzToolsFramework/API/EditorPythonConsoleBus.h>
#include <AzToolsFramework/API/EditorPythonRunnerRequestsBus.h>
#include <AzToolsFramework/UI/UICore/QWidgetSavedState.h>
#include <AzToolsFramework/UI/UICore/QTreeViewStateSaver.hxx>
#include <AzToolsFramework/AssetBrowser/AssetBrowserEntry.h>
#include <AzToolsFramework/SourceControl/PerforceComponent.h>
#include <AzToolsFramework/Asset/AssetSystemComponent.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserComponent.h>
#include <AzToolsFramework/Thumbnails/ThumbnailerComponent.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyManagerComponent.h>
#include <AzToolsFramework/SourceControl/SourceControlAPI.h>

#include <Source/ShaderManagementConsoleApplication.h>
#include <ShaderManagementConsole_Traits_Platform.h>

AZ_PUSH_DISABLE_WARNING(4251 4800, "-Wunknown-warning-option") // disable warnings spawned by QT
#include <QObject>
#include <QMessageBox>
AZ_POP_DISABLE_WARNING

namespace ShaderManagementConsole
{
    //! This function returns the build system target name of "ShaderManagementConsole"
    AZStd::string ShaderManagementConsoleApplication::GetBuildTargetName() const
    {
#if !defined (LY_CMAKE_TARGET)
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

    AZStd::vector<AZStd::string> ShaderManagementConsoleApplication::GetCriticalAssetFilters() const
    {
        return AZStd::vector<AZStd::string>({ "passes/", "config/" });
    }

    void ShaderManagementConsoleApplication::ProcessCommandLine(const AZ::CommandLine& commandLine)
    {
        // Process command line options for opening one or more documents on startup
        size_t openDocumentCount = m_commandLine.GetNumMiscValues();
        for (size_t openDocumentIndex = 0; openDocumentIndex < openDocumentCount; ++openDocumentIndex)
        {
            const AZStd::string openDocumentPath = commandLine.GetMiscValue(openDocumentIndex);

            AZ_Printf(GetBuildTargetName().c_str(), "Opening document: %s", openDocumentPath.c_str());
            ShaderManagementConsoleDocumentSystemRequestBus::Broadcast(
                &ShaderManagementConsoleDocumentSystemRequestBus::Events::OpenDocument, openDocumentPath);
        }

        Base::ProcessCommandLine(commandLine);
    }
} // namespace ShaderManagementConsole
