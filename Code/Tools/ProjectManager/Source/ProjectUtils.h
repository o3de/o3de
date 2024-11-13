/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <ScreenDefs.h>
#include <ProjectInfo.h>
#include <ProjectManagerDefs.h>

#if !defined(Q_MOC_RUN)
#include <QWidget>
#include <QMessageBox>
#include <QProcessEnvironment>
#endif

#include <AzCore/Dependency/Dependency.h>
#include <AzCore/IO/Path/Path_fwd.h>
#include <AzCore/Outcome/Outcome.h>
#include <AzCore/std/string/string_view.h>

namespace O3DE::ProjectManager
{
    namespace ProjectUtils
    {
        static constexpr AZStd::string_view EngineJsonFilename = "engine.json";
        static constexpr AZStd::string_view ProjectJsonFilename = "project.json";

        bool RegisterProject(const QString& path, QWidget* parent = nullptr);
        bool UnregisterProject(const QString& path, QWidget* parent = nullptr);
        bool CopyProjectDialog(const QString& origPath, ProjectInfo& newProjectInfo, QWidget* parent = nullptr);
        bool CopyProject(const QString& origPath, const QString& newPath, QWidget* parent, bool skipRegister = false, bool showProgress = true);
        bool DeleteProjectFiles(const QString& path, bool force = false);
        bool MoveProject(QString origPath, QString newPath, QWidget* parent, bool skipRegister = false, bool showProgress = true);

        bool ReplaceProjectFile(const QString& origFile, const QString& newFile, QWidget* parent = nullptr, bool interactive = true);

        bool FindSupportedCompiler(const ProjectInfo& projectInfo, QWidget* parent = nullptr);
        AZ::Outcome<QString, QString> FindSupportedCompilerForPlatform(const ProjectInfo& projectInfo);

        //! Detect if cmake is installed
        //! Does NOT detect if the version of cmake required to run O3DE
        //! The cmake exeuctable is only tool suitable for detecting the minimum cmake version
        //! required, so it is left up to it to detect the version and error out.
        AZ::Outcome<QString, QString> FindSupportedCMake();

        //! Detect if Ninja-build.Ninja is installed
        //! This is only required if the project is a Script-Only project.
        AZ::Outcome<QString, QString> FindSupportedNinja();

        ProjectManagerScreen GetProjectManagerScreen(const QString& screen);

        /**
         * Execute a console command and return the result.
         * @param cmd the command
         * @param arguments the command argument list
         * @param processEnv the environment
         * @param commandTimeoutSeconds the amount of time in seconds to let the command run before terminating it
         * @return AZ::Outcome with the command result on success
         */
        AZ::Outcome<QString, QString> ExecuteCommandResult(
            const QString& cmd,
            const QStringList& arguments,
            int commandTimeoutSeconds = ProjectCommandLineTimeoutSeconds);

        /**
         * Execute a console command, display the progress in a modal dialog and return the result.
         * @param cmd the command
         * @param arguments the command argument list
         * @param processEnv the environment
         * @param commandTimeoutSeconds the amount of time in seconds to let the command run before terminating it
         * @return AZ::Outcome with the command result on success
         */
        AZ::Outcome<QString, QString> ExecuteCommandResultModalDialog(
            const QString& cmd,
            const QStringList& arguments,
            const QString& title);

        AZ::Outcome<void, QString> SetupCommandLineProcessEnvironment();
        AZ::Outcome<QString, QString> GetProjectBuildPath(const QString& projectPath);
        AZ::Outcome<void, QString> OpenCMakeGUI(const QString& projectPath);
        AZ::Outcome<QString, QString> RunGetPythonScript(const QString& enginePath);
        QString GetPythonExecutablePath(const QString& enginePath);

        QString GetDefaultProjectPath();
        QString GetDefaultTemplatePath();

        /**
         * Create a desktop shortcut.
         * @param filename the name of the desktop shorcut file
         * @param target the path to the target to run
         * @param arguments the argument list to provide to the target
         * @return AZ::Outcome with the command result on success
         */
        AZ::Outcome<QString, QString> CreateDesktopShortcut(const QString& filename, const QString& targetPath, const QStringList& arguments);

        /**
         * Lookup the location of an Editor executable executable that can be used with the
         * supplied project path
         * First the method attempts to locate a build directory with the project path
         * via querying the <project-path>/user/Registry/Platform/<platform>/build_path.setreg
         * Once that is done a path is formed to locate the Editor executable within the that build
         * directory.
         * Two paths will checked for the existence of an Editor
         * - "<project-build-directory>/bin/$<CONFIG>/Editor"
         * - "<project-build-directory>/bin/<platform>/$<CONFIG>/Editor"
         * Where <platform> is the current platform the O3DE executable is running on and $<CONFIG> is the
         * current build configuration the O3DE executable
         *
         * If neiether of the above paths contain an Editor application, then a path to the Editor
         * is formed by combinding the O3DE executable directory with the filename of Editor
         * - "<executable-directory>/Editor"
         *
         * @param projectPath Path to the root of the project
         * @return path of the Editor Executable if found or an empty path if not
         */
        AZ::IO::FixedMaxPath GetEditorExecutablePath(const AZ::IO::PathView& projectPath);

        /**
         * Compare two version strings.  Invalid version strings will be treated as 0.0.0
         * If you need to validate version strings, use AZ::Validate::ParseVersion()
         * @param version1 The first version string
         * @param version2 The first version string
         * @return 0 if a == b, <0 if a < b, and >0 if a > b on success, or failure
         */
        int VersionCompare(const QString& a, const QString&b);

        /**
         * Return a human readable dependency
         * @param dependency The dependency, e.g. o3de==1.2.3
         * @return a human readable string e.g. o3de 1.2.3, or o3de greater than or equal to 2.3.4
         */
        QString GetDependencyString(const QString& dependency);


        using Dependency = AZ::Dependency<AZ::SemanticVersion::parts_count>;
        using Comparison = AZ::Dependency<AZ::SemanticVersion::parts_count>::Bound::Comparison;

        /**
         * Helper to parse a dependency, e.g. o3de==1.2.3 into an object name, comparator version 
         * @param dependency The dependency, e.g. o3de==1.2.3
         * @param objectName The parsed object name, e.g. o3de 
         * @param comparator The parsed comparator, e.g. ==
         * @param version The parsed version, e.g. 1.2.3 
         */
        void GetDependencyNameAndVersion(const QString& dependency, QString& objectName, Comparison& comparator, QString& version);

        /**
         * Helper to parse a dependency, e.g. o3de==1.2.3 into an object name 
         * @param dependency The dependency, e.g. o3de==1.2.3
         * @return The parsed object name, e.g. o3de 
         */
        QString GetDependencyName(const QString& dependency);

        /**
         * Display a dialog with general and detailed sections for the given AZ::Outcome
         * @param title Dialog title
         * @param outcome The AZ::Outcome with general and detailed error messages
         * @param parent Optional QWidget parent
         * @return the QMessageBox result
         */
        int DisplayDetailedError(
            const QString& title, const AZ::Outcome<void, AZStd::pair<AZStd::string, AZStd::string>>& outcome, QWidget* parent = nullptr);

        /**
         * Display a dialog with general and detailed sections for the given ErrorPair
         * @param title Dialog title
         * @param errorPair The general and detailed error messages pair
         * @param parent Optional QWidget parent
         * @param buttons Optional buttons to show
         * @return the QMessageBox result
         */
        int DisplayDetailedError(
            const QString& title,
            const AZStd::string& generalError,
            const AZStd::string& detailedError,
            QWidget* parent = nullptr,
            QMessageBox::StandardButtons buttons = QMessageBox::Ok);
    } // namespace ProjectUtils

    namespace ErrorMessages
    {
        static const char * ExportCancelled =   "Export Cancelled.";
        static const char * LogOpenFailureMsg = "Failed to open log file.";
        static const char * LogPathFailureMsg = "Failed to retrieve log file path.";
    }
} // namespace O3DE::ProjectManager
