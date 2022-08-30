/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/FileReader.h>
#include <AzCore/IO/GenericStreams.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/IO/TextStreamWriters.h>
#include <AzCore/JSON/document.h>
#include <AzCore/JSON/pointer.h>
#include <AzCore/JSON/prettywriter.h>
#include <AzCore/JSON/writer.h>
#include <AzCore/PlatformId/PlatformDefaults.h>
#include <AzCore/Settings/SettingsRegistryImpl.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/Settings/SettingsRegistryVisitorUtils.h>
#include <AzCore/Settings/CommandLine.h>
#include <AzCore/std/string/conversions.h>
#include <AzCore/Utils/Utils.h>

#include <cinttypes>
#include <locale>


namespace AZ::Internal
{
    static constexpr AZStd::string_view EngineJsonFilename = "engine.json";
    static constexpr AZStd::string_view GemJsonFilename = "gem.json";
    static constexpr AZStd::string_view ProjectJsonFilename = "project.json";
    static constexpr AZStd::string_view O3DEManifestJsonFilename = "o3de_manifest.json";

    static constexpr const char* ProductCacheDirectoryName = "Cache";

    AZ::SettingsRegistryInterface::FixedValueString GetEngineMonikerForProject(
        SettingsRegistryInterface& settingsRegistry, const AZ::IO::FixedMaxPath& projectJsonPath)
    {
        // projectPath needs to be an absolute path here.
        using namespace AZ::SettingsRegistryMergeUtils;
        bool projectJsonMerged = settingsRegistry.MergeSettingsFile(
            projectJsonPath.Native(), AZ::SettingsRegistryInterface::Format::JsonMergePatch, ProjectSettingsRootKey);

        AZ::SettingsRegistryInterface::FixedValueString engineMoniker;
        if (projectJsonMerged)
        {
            // In project.json look for the "engine" key.
            auto engineMonikerKey = AZ::SettingsRegistryInterface::FixedValueString::format("%s/engine", ProjectSettingsRootKey);
            settingsRegistry.Get(engineMoniker, engineMonikerKey);
        }

        return engineMoniker;
    }

    AZ::IO::FixedMaxPath ReconcileEngineRootFromProjectPath(SettingsRegistryInterface& settingsRegistry, const AZ::IO::FixedMaxPath& projectPath)
    {
        // Find the engine root via the engine manifest file and project.json
        // Locate the engine manifest file and merge it to settings registry.
        // Visit over the engine paths list and merge the engine.json files to settings registry.
        // Merge project.json to settings registry.  That will give us an "engine" key.
        // When we find a match for "engine_name" value against the "engine" value from before, we can stop and use that engine root.
        // Finally set the BootstrapSettingsRootKey/engine_path setting so that subsequent calls to GetEngineRoot will use that
        // and avoid all this logic.

        using namespace AZ::SettingsRegistryMergeUtils;
        using FixedValueString = AZ::SettingsRegistryInterface::FixedValueString;

        AZ::IO::FixedMaxPath engineRoot;
        if (auto o3deManifestPath = AZ::Utils::GetO3deManifestPath(); !o3deManifestPath.empty())
        {
            bool manifestLoaded{false};

            if (AZ::IO::SystemFile::Exists(o3deManifestPath.c_str()))
            {
                manifestLoaded = settingsRegistry.MergeSettingsFile(
                    o3deManifestPath, AZ::SettingsRegistryInterface::Format::JsonMergePatch, O3deManifestSettingsRootKey);
            }

            struct EngineInfo
            {
                AZ::IO::FixedMaxPath m_path;
                FixedValueString m_moniker;
            };

            struct EnginePathsVisitor : public AZ::SettingsRegistryInterface::Visitor
            {
                using AZ::SettingsRegistryInterface::Visitor::Visit;
                void Visit(
                    const AZ::SettingsRegistryInterface::VisitArgs& visitArgs, AZStd::string_view value) override
                {
                    m_enginePaths.emplace_back(EngineInfo{ AZ::IO::FixedMaxPath{value}.LexicallyNormal(), FixedValueString{visitArgs.m_fieldName} });
                    // Make sure any engine paths read from the manifest are absolute
                    AZ::IO::FixedMaxPath& recentEnginePath = m_enginePaths.back().m_path;
                    if (recentEnginePath.IsRelative())
                    {
                        if (auto engineRootAbsPath = AZ::Utils::ConvertToAbsolutePath(recentEnginePath.Native());
                            engineRootAbsPath.has_value())
                        {
                            recentEnginePath = AZStd::move(*engineRootAbsPath);
                        }
                    }
                }

                AZStd::vector<EngineInfo> m_enginePaths{};
            };

            EnginePathsVisitor pathVisitor;
            if (manifestLoaded)
            {
                auto enginePathsKey = FixedValueString::format("%s/engines_path", O3deManifestSettingsRootKey);
                settingsRegistry.Visit(pathVisitor, enginePathsKey);
            }

            const auto engineMonikerKey = FixedValueString::format("%s/engine_name", EngineSettingsRootKey);

            AZStd::set<AZ::IO::FixedMaxPath> projectPathsNotFound;

            for (EngineInfo& engineInfo : pathVisitor.m_enginePaths)
            {
                if (auto engineSettingsPath = AZ::IO::FixedMaxPath{engineInfo.m_path} / EngineJsonFilename;
                    AZ::IO::SystemFile::Exists(engineSettingsPath.c_str()))
                {
                    if (settingsRegistry.MergeSettingsFile(
                            engineSettingsPath.Native(), AZ::SettingsRegistryInterface::Format::JsonMergePatch, EngineSettingsRootKey))
                    {
                        FixedValueString engineName;
                        settingsRegistry.Get(engineName, engineMonikerKey);
                        AZ_Warning("SettingsRegistryMergeUtils", engineInfo.m_moniker == engineName,
                            R"(The engine name key "%s" mapped to engine path "%s" within the global manifest of "%s")"
                            R"( does not match the "engine_name" field "%s" in the engine.json)" "\n"
                            "This engine should be re-registered.",
                            engineInfo.m_moniker.c_str(), engineInfo.m_path.c_str(), o3deManifestPath.c_str(),
                            engineName.c_str());
                        engineInfo.m_moniker = engineName;
                    }
                }

                if (auto projectJsonPath = (engineInfo.m_path / projectPath / ProjectJsonFilename).LexicallyNormal();
                    AZ::IO::SystemFile::Exists(projectJsonPath.c_str()))
                {
                    if (auto engineMoniker = Internal::GetEngineMonikerForProject(settingsRegistry, projectJsonPath);
                        !engineMoniker.empty() && engineMoniker == engineInfo.m_moniker)
                    {
                        engineRoot = engineInfo.m_path;
                        break;
                    }
                }
                else
                {
                    projectPathsNotFound.insert(projectJsonPath);
                }

                // Continue looking for candidates, remove the previous engine and project settings that were merged above.
                settingsRegistry.Remove(ProjectSettingsRootKey);
                settingsRegistry.Remove(EngineSettingsRootKey);
            }

            if (engineRoot.empty())
            {
                AZStd::string errorStr;
                if (!projectPathsNotFound.empty())
                {
                    // This case is usually encountered when a project path is given as a relative path,
                    // which is assumed to be relative to an engine root.
                    // When no project.json files are found this way, dump this error message about
                    // which project paths were checked.
                    AZStd::string projectPathsTested;
                    for (const auto& path : projectPathsNotFound)
                    {
                        projectPathsTested.append(AZStd::string::format("  %s\n", path.c_str()));
                    }
                    errorStr = AZStd::string::format("No valid project was found at these locations:\n%s"
                        "Please supply a valid --project-path to the application.",
                        projectPathsTested.c_str());
                }
                else
                {
                    // The other case is that a project.json was found, but after checking all the registered engines
                    // none of them matched the engine moniker.
                    AZStd::string enginePathsChecked;
                    for (const auto& engineInfo : pathVisitor.m_enginePaths)
                    {
                        enginePathsChecked.append(AZStd::string::format("  %s (%s)\n", engineInfo.m_path.c_str(), engineInfo.m_moniker.c_str()));
                    }
                    errorStr = AZStd::string::format(
                        "No engine was found in o3de_manifest.json with a name that matches the one set in the project.json.\n"
                        "Engines that were checked:\n%s"
                        "Please check that your engine and project have both been registered with scripts/o3de.py.", enginePathsChecked.c_str()
                    );
                }

                settingsRegistry.Set(FilePathKey_ErrorText, errorStr.c_str());
            }
        }

        return engineRoot;
    }

    AZ::IO::FixedMaxPath ScanUpRootLocator(AZStd::string_view rootFileToLocate)
    {
        AZ::IO::FixedMaxPath rootCandidate{ AZ::Utils::GetExecutableDirectory() };

        bool rootPathVisited = false;
        do
        {
            if (AZ::IO::SystemFile::Exists((rootCandidate / rootFileToLocate).c_str()))
            {
                return rootCandidate;
            }

            // Note for posix filesystems the parent directory of '/' is '/' and for windows
            // the parent directory of 'C:\\' is 'C:\\'

            // Validate that the parent directory isn't itself, that would imply
            // that it is the filesystem root path
            AZ::IO::PathView parentPath = rootCandidate.ParentPath();
            rootPathVisited = (rootCandidate == parentPath);
            // Recurse upwards one directory
            rootCandidate = AZStd::move(parentPath);

        } while (!rootPathVisited);

        return {};
    }

    enum class InjectLocation : bool
    {
        Front,
        Back
    };

    void InjectSettingToCommandLine(AZ::SettingsRegistryInterface& settingsRegistry,
        AZStd::string_view path, AZStd::string_view value,
        InjectLocation injectLocation = InjectLocation::Front)
    {
        AZ::CommandLine commandLine;
        AZ::SettingsRegistryMergeUtils::GetCommandLineFromRegistry(settingsRegistry, commandLine);
        AZ::CommandLine::ParamContainer paramContainer;
        commandLine.Dump(paramContainer);

        auto projectPathOverride = AZStd::string::format(R"(--regset="%.*s=%.*s")",
            aznumeric_cast<int>(path.size()), path.data(), aznumeric_cast<int>(value.size()), value.data());
        auto emplaceIter = injectLocation == InjectLocation::Front ? paramContainer.begin() : paramContainer.end();
        paramContainer.emplace(emplaceIter, AZStd::move(projectPathOverride));
        commandLine.Parse(paramContainer);
        AZ::SettingsRegistryMergeUtils::StoreCommandLineToRegistry(settingsRegistry, commandLine);
    }
} // namespace AZ::Internal

namespace AZ::SettingsRegistryMergeUtils
{
    //! The algorithm that is used to find the *directory* containing the o3de_manifest.json
    //! 1. It first checks the "{BootstrapSettingsRootKey}/o3de_manifest_path"
    //! 2. If that is not set it defaults to the user's home directory with the .o3de folder appended to it
    //!    equal to "~/.o3de"
    static AZ::IO::FixedMaxPath FindO3deManifestCachePath(SettingsRegistryInterface& settingsRegistry)
    {
        using FixedValueString = SettingsRegistryInterface::FixedValueString;

        constexpr auto o3deManifestPathKey = FixedValueString(BootstrapSettingsRootKey) + "/o3de_manifest_path";

        // Step 1 Check the o3de_manifest_path setting
        AZ::IO::FixedMaxPath o3deManifestPath;
        if (!settingsRegistry.Get(o3deManifestPath.Native(), o3deManifestPathKey))
        {
            // Step 2 Use the user's home directory
            o3deManifestPath = AZ::Utils::GetHomeDirectory();
            o3deManifestPath /= ".o3de";
        }

        if (o3deManifestPath.IsRelative())
        {
            if (auto o3deManifestAbsPath = AZ::Utils::ConvertToAbsolutePath(o3deManifestPath.Native());
                o3deManifestAbsPath.has_value())
            {
                o3deManifestPath = AZStd::move(*o3deManifestAbsPath);
            }
        }
        return o3deManifestPath;
    }

    AZ::IO::FixedMaxPath FindEngineRoot(SettingsRegistryInterface& settingsRegistry)
    {
        static constexpr AZStd::string_view InternalScanUpEngineRootKey{ "/O3DE/Runtime/Internal/engine_root_scan_up_path" };
        using FixedValueString = SettingsRegistryInterface::FixedValueString;
        using Type = SettingsRegistryInterface::Type;

        AZ::IO::FixedMaxPath engineRoot;
        // This is the 'external' engine root key, as in passed from command-line or .setreg files.
        constexpr auto engineRootKey = FixedValueString(BootstrapSettingsRootKey) + "/engine_path";

        // Step 1 Run the scan upwards logic once to find the location of the engine.json if it exist
        // Once this step is run the {InternalScanUpEngineRootKey} is set in the Settings Registry
        // to have this scan logic only run once InternalScanUpEngineRootKey the supplied registry
        if (settingsRegistry.GetType(InternalScanUpEngineRootKey) == Type::NoType)
        {
            // We can scan up from exe directory to find engine.json, use that for engine root if it exists.
            engineRoot = Internal::ScanUpRootLocator(Internal::EngineJsonFilename);
            // The Internal ScanUp Engine Root Key will be set as an absolute path
            if (!engineRoot.empty())
            {
                if (engineRoot.IsRelative())
                {
                    if (auto engineRootAbsPath = AZ::Utils::ConvertToAbsolutePath(engineRoot.Native());
                        engineRootAbsPath.has_value())
                    {
                        engineRoot = AZStd::move(*engineRootAbsPath);
                    }
                }
            }

            // Set the {InternalScanUpEngineRootKey} to make sure this code path isn't called again for this settings registry
            settingsRegistry.Set(InternalScanUpEngineRootKey, engineRoot.Native());
            if (!engineRoot.empty())
            {
                settingsRegistry.Set(engineRootKey, engineRoot.Native());
                // Inject the engine root to the front of the command line settings
                Internal::InjectSettingToCommandLine(settingsRegistry, engineRootKey, engineRoot.Native());
                return engineRoot;
            }
        }

        // Step 2 check if the engine_path key has been supplied
        if (settingsRegistry.Get(engineRoot.Native(), engineRootKey); !engineRoot.empty())
        {
            if (engineRoot.IsRelative())
            {
                if (auto engineRootAbsPath = AZ::Utils::ConvertToAbsolutePath(engineRoot.Native());
                    engineRootAbsPath.has_value())
                {
                    engineRoot = AZStd::move(*engineRootAbsPath);
                }
            }
            return engineRoot;
        }

        // Step 3 locate the project root and attempt to find the engine root using the registered engine
        // for the project in the project.json file
        AZ::IO::FixedMaxPath projectRoot;
        settingsRegistry.Get(projectRoot.Native(), FilePathKey_ProjectPath);
        if (projectRoot.empty())
        {
            return {};
        }

        // Use the project.json and engine manifest to locate the engine root.
        if (engineRoot = Internal::ReconcileEngineRootFromProjectPath(settingsRegistry, projectRoot); !engineRoot.empty())
        {
            settingsRegistry.Set(engineRootKey, engineRoot.c_str());
            return engineRoot;
        }

        // Fall back to using the project root as the engine root if the engine path could not be reconciled
        // by checking the project.json "engine" string within o3de_manifest.json "engine_paths" object
        return projectRoot;
    }

    AZ::IO::FixedMaxPath FindProjectRoot(SettingsRegistryInterface& settingsRegistry)
    {
        static constexpr AZStd::string_view InternalScanUpProjectRootKey{ "/O3DE/Runtime/Internal/project_root_scan_up_path" };
        using FixedValueString = SettingsRegistryInterface::FixedValueString;
        using Type = SettingsRegistryInterface::Type;

        AZ::IO::FixedMaxPath projectRoot;
        constexpr auto projectRootKey = FixedValueString(BootstrapSettingsRootKey) + "/project_path";

        // Step 1 Run the scan upwards logic once to find the location of the closest ancestor project.json
        // Once this step is run the {InternalScanUpProjectRootKey} is set in the Settings Registry
        // to have this scan logic only run once for the supplied registry
        // SettingsRegistryInterface::GetType is used to check if a key is set
        if (settingsRegistry.GetType(InternalScanUpProjectRootKey) == Type::NoType)
        {
            projectRoot = Internal::ScanUpRootLocator(Internal::ProjectJsonFilename);
            // Convert the path to an absolute path before adding it as a setting to the
            // InternalScanUpProjectRootKey
            if (!projectRoot.empty())
            {
                if (projectRoot.IsRelative())
                {
                    if (auto projectAbsPath = AZ::Utils::ConvertToAbsolutePath(projectRoot.Native());
                        projectAbsPath.has_value())
                    {
                        projectRoot = AZStd::move(*projectAbsPath);
                    }
                }
            }

            // Set the {InternalScanUpProjectRootKey} to make sure this code path isn't called again for this settings registry
            settingsRegistry.Set(InternalScanUpProjectRootKey, projectRoot.Native());
            if (!projectRoot.empty())
            {
                settingsRegistry.Set(projectRootKey, projectRoot.c_str());
                // Inject the project root at to the front of the command line settings
                Internal::InjectSettingToCommandLine(settingsRegistry, projectRootKey, projectRoot.Native());

                return projectRoot;
            }
        }

        // Step 2 Check the project-path key
        // This is the project path root key, as passed from command-line or *.setreg files.
        settingsRegistry.Get(projectRoot.Native(), projectRootKey);
        if (!projectRoot.empty())
        {
            if (projectRoot.IsRelative())
            {
                if (auto projectAbsPath = AZ::Utils::ConvertToAbsolutePath(projectRoot.Native());
                    projectAbsPath.has_value())
                {
                    projectRoot = AZStd::move(*projectAbsPath);
                }
            }
        }

        return projectRoot;
    }

    //! The algorithm that is used to find the project cache is as follows
    //! 1. The "{BootstrapSettingsRootKey}/project_cache_path" is checked for the path
    //! 2. Otherwise append the ProductCacheDirectoryName constant to the <project-path>
    static AZ::IO::FixedMaxPath FindProjectCachePath(SettingsRegistryInterface& settingsRegistry, const AZ::IO::FixedMaxPath& projectPath)
    {
        using FixedValueString = SettingsRegistryInterface::FixedValueString;

        constexpr auto projectCachePathKey = FixedValueString(BootstrapSettingsRootKey) + "/project_cache_path";

        // Step 1 Check the project-cache-path key
        AZ::IO::FixedMaxPath projectCachePath;
        if (!settingsRegistry.Get(projectCachePath.Native(), projectCachePathKey))
        {
            // Step 2 Append the "Cache" directory to the project-path
            projectCachePath = projectPath / Internal::ProductCacheDirectoryName;
        }

        if (projectCachePath.IsRelative())
        {
            if (auto projectCacheAbsPath = AZ::Utils::ConvertToAbsolutePath(projectCachePath.Native());
                projectCacheAbsPath.has_value())
            {
                projectCachePath = AZStd::move(*projectCacheAbsPath);
            }
        }
        return projectCachePath;
    }

    //! Set the user directory with the provided path or using <project-path>/user as default
    static AZ::IO::FixedMaxPath FindProjectUserPath(SettingsRegistryInterface& settingsRegistry,
        const AZ::IO::FixedMaxPath& projectPath)
    {
        using FixedValueString = SettingsRegistryInterface::FixedValueString;

        // User: root - same as the @user@ alias, this is the starting path for transient data and log files.
        constexpr auto projectUserPathKey = FixedValueString(BootstrapSettingsRootKey) + "/project_user_path";

        // Step 1 Check the project-user-path key
        AZ::IO::FixedMaxPath projectUserPath;
        if (!settingsRegistry.Get(projectUserPath.Native(), projectUserPathKey))
        {
            // Step 2 Append the "User" directory to the project-path
            projectUserPath = projectPath / "user";
        }

        if (projectUserPath.IsRelative())
        {
            if (auto projectUserAbsPath = AZ::Utils::ConvertToAbsolutePath(projectUserPath.Native());
                projectUserAbsPath.has_value())
            {
                projectUserPath = AZStd::move(*projectUserAbsPath);
            }
        }
        return projectUserPath;
    }

    //! Set the log directory using the settings registry path or using <project-user-path>/log as default
    static AZ::IO::FixedMaxPath FindProjectLogPath(SettingsRegistryInterface& settingsRegistry,
        const AZ::IO::FixedMaxPath& projectUserPath)
    {
        using FixedValueString = SettingsRegistryInterface::FixedValueString;

        // User: root - same as the @log@ alias, this is the starting path for transient data and log files.
        constexpr auto projectLogPathKey = FixedValueString(BootstrapSettingsRootKey) + "/project_log_path";

        // Step 1 Check the project-user-path key
        AZ::IO::FixedMaxPath projectLogPath;
        if (!settingsRegistry.Get(projectLogPath.Native(), projectLogPathKey))
        {
            // Step 2 Append the "Log" directory to the project-user-path
            projectLogPath = projectUserPath / "log";
        }

        if (projectLogPath.IsRelative())
        {
            if (auto projectLogAbsPath = AZ::Utils::ConvertToAbsolutePath(projectLogPath.Native()))
            {
                projectLogPath = AZStd::move(*projectLogAbsPath);
            }
        }

        return projectLogPath;
    }

    // check for a default write storage path, fall back to the <project-user-path> if not
    static AZ::IO::FixedMaxPath FindDevWriteStoragePath(const AZ::IO::FixedMaxPath& projectUserPath)
    {
        AZStd::optional<AZ::IO::FixedMaxPathString> devWriteStorage = Utils::GetDevWriteStoragePath();
        AZ::IO::FixedMaxPath devWriteStoragePath = devWriteStorage.has_value() ? *devWriteStorage : projectUserPath;
        if (devWriteStoragePath.IsRelative())
        {
            if (auto devWriteStorageAbsPath = AZ::Utils::ConvertToAbsolutePath(devWriteStoragePath.Native()))
            {
                devWriteStoragePath = AZStd::move(*devWriteStorageAbsPath);
            }
        }
        return devWriteStoragePath;
    }

    // check for the project build path, which is a relative path from the project root
    // that specifies where the build directory is located
    static void SetProjectBuildPath(SettingsRegistryInterface& settingsRegistry,
        const AZ::IO::FixedMaxPath& projectPath)
    {
        if (AZ::IO::FixedMaxPath projectBuildPath; settingsRegistry.Get(projectBuildPath.Native(), ProjectBuildPath))
        {
            settingsRegistry.Remove(FilePathKey_ProjectBuildPath);
            settingsRegistry.Remove(FilePathKey_ProjectConfigurationBinPath);
            AZ::IO::FixedMaxPath buildConfigurationPath = (projectPath / projectBuildPath).LexicallyNormal();
            if (IO::SystemFile::Exists(buildConfigurationPath.c_str()))
            {
                settingsRegistry.Set(FilePathKey_ProjectBuildPath, buildConfigurationPath.Native());
            }

            // Add the specific build configuration paths to the Settings Registry
            // First try <project-build-path>/bin/$<CONFIG> and if that path doesn't exist
            // try <project-build-path>/bin/$<PLATFORM>/$<CONFIG>
            buildConfigurationPath /= "bin";
            if (IO::SystemFile::Exists((buildConfigurationPath / AZ_BUILD_CONFIGURATION_TYPE).c_str()))
            {
                settingsRegistry.Set(FilePathKey_ProjectConfigurationBinPath,
                    (buildConfigurationPath / AZ_BUILD_CONFIGURATION_TYPE).Native());
            }
            else if (IO::SystemFile::Exists((buildConfigurationPath / AZ_TRAIT_OS_PLATFORM_CODENAME / AZ_BUILD_CONFIGURATION_TYPE).c_str()))
            {
                settingsRegistry.Set(FilePathKey_ProjectConfigurationBinPath,
                    (buildConfigurationPath / AZ_TRAIT_OS_PLATFORM_CODENAME / AZ_BUILD_CONFIGURATION_TYPE).Native());
            }
        }
    }

    // Sets the project name within the Settings Registry by looking up the "project_name"
    // within the project.json file
    static void SetProjectName(SettingsRegistryInterface& settingsRegistry,
        const AZ::IO::FixedMaxPath& projectPath)
    {
        using FixedValueString = SettingsRegistryInterface::FixedValueString;
        // Project name - if it was set via merging project.json use that value, otherwise use the project path's folder name.
        constexpr auto projectNameKey = FixedValueString(ProjectSettingsRootKey) + "/project_name";

        // Read the project name from the project.json file if it exists
        if (AZ::IO::FixedMaxPath projectJsonPath = projectPath / Internal::ProjectJsonFilename;
            AZ::IO::SystemFile::Exists(projectJsonPath.c_str()))
        {
            settingsRegistry.MergeSettingsFile(projectJsonPath.Native(),
                AZ::SettingsRegistryInterface::Format::JsonMergePatch, AZ::SettingsRegistryMergeUtils::ProjectSettingsRootKey);
        }
        // If a project name isn't set the default will be set to the final path segment of the project path
        if (FixedValueString projectName; !settingsRegistry.Get(projectName, projectNameKey))
        {
            projectName = projectPath.Filename().Native();
            settingsRegistry.Set(projectNameKey, projectName);
        }
    }

    AZStd::string_view ConfigParserSettings::DefaultCommentPrefixFilter(AZStd::string_view line)
    {
        constexpr AZStd::string_view commentPrefixes = ";#";
        for (char commentPrefix : commentPrefixes)
        {
            if (size_t commentOffset = line.find(commentPrefix); commentOffset != AZStd::string_view::npos)
            {
                return line.substr(0, commentOffset);
            }
        }

        return line;
    }

    AZStd::string_view ConfigParserSettings::DefaultSectionHeaderFilter(AZStd::string_view line)
    {
        AZStd::string_view sectionName;
        constexpr char sectionHeaderStart = '[';
        constexpr char sectionHeaderEnd = ']';
        if (line.starts_with(sectionHeaderStart) && line.ends_with(sectionHeaderEnd))
        {
            // View substring of line between the section '[' and ']' characters
            sectionName = line.substr(1);
            sectionName = sectionName.substr(0, sectionName.size() - 1);
            // strip any leading and trailing whitespace from the section name
            size_t startIndex = 0;
            for (; startIndex < sectionName.size() && isspace(sectionName[startIndex]); ++startIndex);
            sectionName = sectionName.substr(startIndex);
            size_t endIndex = sectionName.size();
            for (; endIndex > 0 && isspace(sectionName[endIndex - 1]); --endIndex);
            sectionName = sectionName.substr(0, endIndex);
        }

        return sectionName;
    }

    void QuerySpecializationsFromRegistry(SettingsRegistryInterface& registry, SettingsRegistryInterface::Specializations& specializations)
    {
        // Append any specializations stored in the registry
        struct SpecializationsVisitor
            : AZ::SettingsRegistryInterface::Visitor
        {
            SpecializationsVisitor(SettingsRegistryInterface::Specializations& specializations)
                : m_settingsSpecialization{ specializations }
            {}

            using AZ::SettingsRegistryInterface::Visitor::Visit;
            void Visit(const AZ::SettingsRegistryInterface::VisitArgs& visitArgs, bool value) override
            {
                if (value)
                {
                    // The specialization is the key itself.
                    // The value is just used to determine if the specialization should be added
                    m_settingsSpecialization.Append(visitArgs.m_fieldName);
                }
            }
            SettingsRegistryInterface::Specializations& m_settingsSpecialization;
        };

        SpecializationsVisitor visitor{ specializations };
        registry.Visit(visitor, SpecializationsRootKey);
    }

    void MergeSettingsToRegistry_AddBuildSystemTargetSpecialization(SettingsRegistryInterface& registry, AZStd::string_view targetName)
    {
        registry.Set(BuildTargetNameKey, targetName);

        // Add specializations to the target registry based on the name of the Build System Target
        auto targetSpecialization = AZ::SettingsRegistryInterface::FixedValueString::format("%s/%.*s",
            SpecializationsRootKey, aznumeric_cast<int>(targetName.size()), targetName.data());
        registry.Set(targetSpecialization, true);
    }

    bool MergeSettingsToRegistry_ConfigFile(SettingsRegistryInterface& registry, AZStd::string_view filePath,
        const ConfigParserSettings& configParserSettings)
    {
        auto configPath = FindProjectRoot(registry) / filePath;
        IO::FileReader configFile;
        bool configFileOpened{};
        switch (configParserSettings.m_fileReaderClass)
        {
        case ConfigParserSettings::FileReaderClass::UseFileIOIfAvailableFallbackToSystemFile:
        {
            auto fileIo = AZ::IO::FileIOBase::GetInstance();
            configFileOpened = configFile.Open(fileIo, configPath.c_str());
            break;
        }
        case ConfigParserSettings::FileReaderClass::UseSystemFileOnly:
        {
            configFileOpened = configFile.Open(nullptr, configPath.c_str());
            break;
        }
        case ConfigParserSettings::FileReaderClass::UseFileIOOnly:
        {
            auto fileIo = AZ::IO::FileIOBase::GetInstance();
            if (fileIo == nullptr)
            {
                return false;
            }
            configFileOpened = configFile.Open(fileIo, configPath.c_str());
            break;
        }
        default:
            AZ_Error("SettingsRegistryMergeUtils", false, "An Invalid FileReaderClass enum value has been supplied");
            return false;
        }
        if (!configFileOpened)
        {
            // While all parsing and formatting errors are actual errors, config files that are not present
            // are not an error as they are always optional.  In this case, show a brief trace message
            // that indicates the location the file could be placed at in order to run it.
            AZ_TracePrintf("SettingsRegistryMergeUtils", "Optional config file \"%s\" not found.\n", configPath.c_str());
            return false;
        }

        bool configFileParsed = true;
        // The ConfigFile is parsed using into a fixed_vector of ConfigBufferMaxSize below
        // which avoids performing any dynamic memory allocations during parsing
        // The Settings Registry might still allocate memory though in the MergeCommandLineArgument() function
        size_t remainingFileLength = configFile.Length();
        if (remainingFileLength == 0)
        {
            AZ_TracePrintf("SettingsRegistryMergeUtils", R"(Configuration file "%s" is empty, nothing to merge)" "\n", configPath.c_str());
            return true;
        }
        constexpr size_t ConfigBufferMaxSize = 4096;
        AZStd::fixed_vector<char, ConfigBufferMaxSize> configBuffer;
        size_t readSize = (AZStd::min)(configBuffer.max_size(), remainingFileLength);
        configBuffer.resize_no_construct(readSize);

        size_t bytesRead = configFile.Read(configBuffer.size(), configBuffer.data());
        remainingFileLength -= bytesRead;

        AZ::SettingsRegistryInterface::FixedValueString currentJsonPointerPath(configParserSettings.m_registryRootPointerPath);
        size_t rootPointerPathSize = configParserSettings.m_registryRootPointerPath.size();
        do
        {
            decltype(configBuffer)::iterator frontIter{};
            for (frontIter = configBuffer.begin(); frontIter != configBuffer.end();)
            {
                if (std::isspace(*frontIter, {}))
                {
                    ++frontIter;
                    continue;
                }

                // Find end of line
                auto lineStartIter = frontIter;
                auto lineEndIter = AZStd::find(frontIter, configBuffer.end(), '\n');
                bool foundNewLine = lineEndIter != configBuffer.end();
                if (!foundNewLine && remainingFileLength > 0)
                {
                    // No newline has been found in the remaining characters in the buffer,
                    // Read the next chunk(up to ConfigBufferMaxSize) of the config file and look for a new line
                    // if the file has remaining content
                    // Otherwise if the file has no more remaining content, then it is improperly terminated
                    // text file according to the posix standard
                    // https://stackoverflow.com/questions/729692/why-should-text-files-end-with-a-newline
                    // Therefore the final text after the final newline will be parsed
                    break;
                }

                AZStd::string_view line(lineStartIter, lineEndIter);

                // Remove any trailing carriage returns from the line
                if (size_t lastValidLineCharIndex = line.find_last_not_of('\r'); lastValidLineCharIndex != AZStd::string_view::npos)
                {
                    line = line.substr(0, lastValidLineCharIndex + 1);
                }
                // Retrieve non-commented portion of line
                if (configParserSettings.m_commentPrefixFunc)
                {
                    line = configParserSettings.m_commentPrefixFunc(line);
                }
                // Lookup any new section names from the line
                if (configParserSettings.m_sectionHeaderFunc)
                {
                    AZStd::string_view sectionName = configParserSettings.m_sectionHeaderFunc(line);
                    if (!sectionName.empty())
                    {
                        currentJsonPointerPath.erase(rootPointerPathSize);
                        currentJsonPointerPath += '/';
                        currentJsonPointerPath += sectionName;
                    }
                }

                registry.MergeCommandLineArgument(line, currentJsonPointerPath, configParserSettings.m_commandLineSettings);

                // Skip past the newline character if found
                frontIter = lineEndIter + (foundNewLine ? 1 : 0);
            }

            // Read in more data from the config file if available.
            // If the parsing was in the middle of a parsing line, then move the remaining data of that line to the beginning
            // of the fixed_vector buffer
            if (frontIter == configBuffer.begin())
            {
                AZ_Error("SettingsRegistryMergeUtils", false,
                    R"(The config file "%s" contains a line which is longer than the max line length of %zu.)" "\n"
                    R"(Parsing will halt. The line content so far is:)" "\n"
                    R"("%.*s")" "\n", configPath.c_str(), configBuffer.max_size(),
                    aznumeric_cast<int>(configBuffer.size()), configBuffer.data());
                configFileParsed = false;
                break;
            }
            const size_t readOffset = AZStd::distance(frontIter, configBuffer.end());
            if (readOffset > 0)
            {
                memmove(configBuffer.begin(), frontIter, readOffset);
            }
            readSize = (AZStd::min)(configBuffer.max_size() - readOffset, remainingFileLength);
            bytesRead = configFile.Read(readSize, configBuffer.data() + readOffset);
            configBuffer.resize_no_construct(readOffset + readSize);
            remainingFileLength -= bytesRead;
        } while (bytesRead > 0);

        configFile.Close();

        return configFileParsed;
    }

    void MergeSettingsToRegistry_ManifestGemsPaths(SettingsRegistryInterface& registry)
    {
        auto MergeGemPathToRegistry = [&registry](AZStd::string_view manifestKey,
            AZStd::string_view gemName,
            AZ::IO::PathView gemRootPath)
        {
            using FixedValueString = SettingsRegistryInterface::FixedValueString;
            if (manifestKey == GemNameKey)
            {
                const auto manifestGemJsonPath = FixedValueString::format("%s/%.*s/Path",
                    ManifestGemsRootKey, AZ_STRING_ARG(gemName));
                registry.Set(manifestGemJsonPath, gemRootPath.LexicallyNormal().Native());
            }
        };

        VisitAllManifestGems(registry, MergeGemPathToRegistry);
    }

    void MergeSettingsToRegistry_AddRuntimeFilePaths(SettingsRegistryInterface& registry)
    {
        using FixedValueString = AZ::SettingsRegistryInterface::FixedValueString;

        // Binary folder - corresponds to the @exefolder@ alias
        AZ::IO::FixedMaxPath exePath = AZ::Utils::GetExecutableDirectory();
        registry.Set(FilePathKey_BinaryFolder, exePath.LexicallyNormal().Native());

        // O3de manifest folder
        if (AZ::IO::FixedMaxPath o3deManifestPath = FindO3deManifestCachePath(registry);
            !o3deManifestPath.empty())
        {
            registry.Set(FilePathKey_O3deManifestRootFolder, o3deManifestPath.LexicallyNormal().Native());
        }

        // Project path - corresponds to the @projectroot@ alias
        // NOTE: We make the project-path in the BootstrapSettingsRootKey absolute first

        AZ::IO::FixedMaxPath projectPath = FindProjectRoot(registry);
        if ([[maybe_unused]] constexpr auto projectPathKey = FixedValueString(BootstrapSettingsRootKey) + "/project_path";
            !projectPath.empty())
        {
            projectPath = projectPath.LexicallyNormal();
            AZ_Warning("SettingsRegistryMergeUtils", AZ::IO::SystemFile::Exists(projectPath.c_str()),
                R"(Project path "%s" does not exist. Is the "%.*s" registry setting set to a valid absolute path?)"
                , projectPath.c_str(), AZ_STRING_ARG(projectPathKey));

            registry.Set(FilePathKey_ProjectPath, projectPath.Native());
        }
        else
        {
            AZ_TracePrintf("SettingsRegistryMergeUtils",
                R"(Project path isn't set in the Settings Registry at "%.*s".)"
                " Project-related filepaths will be set relative to the executable directory\n",
                AZ_STRING_ARG(projectPathKey));
            projectPath = exePath;
            registry.Set(FilePathKey_ProjectPath, exePath.Native());
        }

        // Engine root folder - corresponds to the @engroot@ alias
        AZ::IO::FixedMaxPath engineRoot = FindEngineRoot(registry);
        if (!engineRoot.empty())
        {
            engineRoot = engineRoot.LexicallyNormal();
            registry.Set(FilePathKey_EngineRootFolder, engineRoot.Native());
        }

        // Cache folder
        AZ::IO::FixedMaxPath projectCachePath = FindProjectCachePath(registry, projectPath).LexicallyNormal();
        if (!projectCachePath.empty())
        {
            projectCachePath = projectCachePath.LexicallyNormal();
            registry.Set(FilePathKey_CacheProjectRootFolder, projectCachePath.Native());

            // Cache/<asset-platform> folder
            // Get the name of the asset platform assigned by the bootstrap. First check for platform version such as "windows_assets"
            // and if that's missing just get "assets".
            FixedValueString assetPlatform;
            if (auto assetPlatformKey = FixedValueString::format("%s/%s_assets", BootstrapSettingsRootKey, AZ_TRAIT_OS_PLATFORM_CODENAME_LOWER);
                !registry.Get(assetPlatform, assetPlatformKey))
            {
                assetPlatformKey = FixedValueString::format("%s/assets", BootstrapSettingsRootKey);
                registry.Get(assetPlatform, assetPlatformKey);
            }
            if (assetPlatform.empty())
            {
                // Use the platform codename to retrieve the default asset platform value
                assetPlatform = AZ::OSPlatformToDefaultAssetPlatform(AZ_TRAIT_OS_PLATFORM_CODENAME);
            }

            // Make sure the asset platform is set before setting cache path for the asset platform.
            if (!assetPlatform.empty())
            {
                registry.Set(FilePathKey_CacheRootFolder, (projectCachePath / assetPlatform).Native());
            }
        }

        // User folder
        AZ::IO::FixedMaxPath projectUserPath = FindProjectUserPath(registry, projectPath);
        if (!projectUserPath.empty())
        {
            projectUserPath = projectUserPath.LexicallyNormal();
            registry.Set(FilePathKey_ProjectUserPath, projectUserPath.Native());
        }

        // Log folder
        if (AZ::IO::FixedMaxPath projectLogPath = FindProjectLogPath(registry, projectUserPath); !projectLogPath.empty())
        {
            projectLogPath = projectLogPath.LexicallyNormal();
            registry.Set(FilePathKey_ProjectLogPath, projectLogPath.Native());
        }

        // Developer Write Storage folder
        if (AZ::IO::FixedMaxPath devWriteStoragePath = FindDevWriteStoragePath(projectUserPath); !devWriteStoragePath.empty())
        {
            devWriteStoragePath = devWriteStoragePath.LexicallyNormal();
            registry.Set(FilePathKey_DevWriteStorage, devWriteStoragePath.Native());
        }

        // Set the project in-memory build path if the ProjectBuildPath key has been supplied
        SetProjectBuildPath(registry, projectPath);
        // Set the project name using the "project_name" key
        SetProjectName(registry, projectPath);

        // Read every registry Gem Path into the `ManifestGemsRootKey` object
        MergeSettingsToRegistry_ManifestGemsPaths(registry);

#if !AZ_TRAIT_OS_IS_HOST_OS_PLATFORM
        // Setup the cache, user, and log paths to platform specific locations when running on non-host platforms
        if (AZStd::optional<AZ::IO::FixedMaxPathString> nonHostCacheRoot = Utils::GetDefaultAppRootPath();
            nonHostCacheRoot)
        {
            registry.Set(FilePathKey_CacheProjectRootFolder, *nonHostCacheRoot);
            registry.Set(FilePathKey_CacheRootFolder, *nonHostCacheRoot);
        }
        else
        {
            registry.Set(FilePathKey_CacheProjectRootFolder, projectPath.Native());
            registry.Set(FilePathKey_CacheRootFolder, projectPath.Native());
        }
        if (AZStd::optional<AZ::IO::FixedMaxPathString> devWriteStorage = Utils::GetDevWriteStoragePath();
            devWriteStorage)
        {
            const auto devWriteStoragePath = AZ::IO::PathView(*devWriteStorage).LexicallyNormal();
            registry.Set(FilePathKey_DevWriteStorage, devWriteStoragePath.Native());
            registry.Set(FilePathKey_ProjectUserPath, (devWriteStoragePath / "user").Native());
            registry.Set(FilePathKey_ProjectLogPath, (devWriteStoragePath / "user" / "log").Native());
        }
        else
        {
            registry.Set(FilePathKey_DevWriteStorage, projectPath.Native());
            registry.Set(FilePathKey_ProjectUserPath, (projectPath / "user").Native());
            registry.Set(FilePathKey_ProjectLogPath, (projectPath / "user" / "log").Native());
        }
#endif // AZ_TRAIT_OS_IS_HOST_OS_PLATFORM
    }

    void MergeSettingsToRegistry_TargetBuildDependencyRegistry(SettingsRegistryInterface& registry, const AZStd::string_view platform,
        const SettingsRegistryInterface::Specializations& specializations, AZStd::vector<char>* scratchBuffer)
    {
        AZ::IO::FixedMaxPath mergePath = AZ::Utils::GetExecutableDirectory();
        if (!mergePath.empty())
        {
            registry.MergeSettingsFolder((mergePath / SettingsRegistryInterface::RegistryFolder).Native(),
                specializations, platform, "", scratchBuffer);
        }

        // Look within the Cache Root directory for target build dependency .setreg files
        AZ::SettingsRegistryInterface::FixedValueString cacheRootPath;
        if (registry.Get(cacheRootPath, FilePathKey_CacheRootFolder))
        {
            mergePath = AZStd::move(cacheRootPath);
            AZStd::fixed_string<32> registryFolderLower(SettingsRegistryInterface::RegistryFolder);
            AZStd::to_lower(registryFolderLower.begin(), registryFolderLower.end());
            mergePath /= registryFolderLower;
            registry.MergeSettingsFolder(mergePath.Native(), specializations, platform, "", scratchBuffer);
        }

        AZ::IO::FixedMaxPath projectBinPath;
        if (registry.Get(projectBinPath.Native(), FilePathKey_ProjectConfigurationBinPath))
        {
            // Append the project build path path to the project root
            projectBinPath /= SettingsRegistryInterface::RegistryFolder;
            registry.MergeSettingsFolder(projectBinPath.Native(), specializations, platform, "", scratchBuffer);
        }
    }

    void MergeSettingsToRegistry_EngineRegistry(SettingsRegistryInterface& registry, const AZStd::string_view platform,
        const SettingsRegistryInterface::Specializations& specializations, AZStd::vector<char>* scratchBuffer)
    {
        AZ::SettingsRegistryInterface::FixedValueString engineRootPath;
        if (registry.Get(engineRootPath, FilePathKey_EngineRootFolder))
        {
            AZ::IO::FixedMaxPath mergePath{ AZStd::move(engineRootPath) };
            mergePath /= SettingsRegistryInterface::RegistryFolder;
            registry.MergeSettingsFolder(mergePath.Native(), specializations, platform, "", scratchBuffer);
        }
    }

    void MergeSettingsToRegistry_GemRegistries(SettingsRegistryInterface& registry, const AZStd::string_view platform,
        const SettingsRegistryInterface::Specializations& specializations, AZStd::vector<char>* scratchBuffer)
    {
        auto MergeGemRootRegistryFolder = [&registry, &platform, &specializations, &scratchBuffer]
        (AZStd::string_view, AZ::IO::FixedMaxPath gemPath)
        {
            registry.MergeSettingsFolder((gemPath / SettingsRegistryInterface::RegistryFolder).Native(),
                specializations, platform, "", scratchBuffer);
        };
        VisitActiveGems(registry, MergeGemRootRegistryFolder);
    }

    void MergeSettingsToRegistry_ProjectRegistry(SettingsRegistryInterface& registry, const AZStd::string_view platform,
        const SettingsRegistryInterface::Specializations& specializations, AZStd::vector<char>* scratchBuffer)
    {
        AZ::SettingsRegistryInterface::FixedValueString sourceGamePath;
        if (registry.Get(sourceGamePath, FilePathKey_ProjectPath))
        {
            AZ::IO::FixedMaxPath mergePath{ sourceGamePath };
            mergePath /= SettingsRegistryInterface::RegistryFolder;
            registry.MergeSettingsFolder(mergePath.Native(), specializations, platform, "", scratchBuffer);
        }
    }

    void MergeSettingsToRegistry_ProjectUserRegistry(SettingsRegistryInterface& registry, const AZStd::string_view platform,
        const SettingsRegistryInterface::Specializations& specializations, AZStd::vector<char>* scratchBuffer)
    {
        // Unlike other paths, the path can't be overwritten by the dev settings because that would create a circular dependency.
        AZ::IO::FixedMaxPath projectUserPath;
        if (registry.Get(projectUserPath.Native(), FilePathKey_ProjectPath))
        {
            projectUserPath /= SettingsRegistryInterface::DevUserRegistryFolder;
            registry.MergeSettingsFolder(projectUserPath.Native(), specializations, platform, "", scratchBuffer);
        }
    }

    void MergeSettingsToRegistry_O3deUserRegistry(SettingsRegistryInterface& registry, const AZStd::string_view platform,
        const SettingsRegistryInterface::Specializations& specializations, AZStd::vector<char>* scratchBuffer)
    {
        if (AZ::IO::FixedMaxPath o3deUserPath = AZ::Utils::GetO3deManifestDirectory(); !o3deUserPath.empty())
        {
            o3deUserPath /= SettingsRegistryInterface::RegistryFolder;
            registry.MergeSettingsFolder(o3deUserPath.Native(), specializations, platform, "", scratchBuffer);
        }
    }

    // This function intentionally copies `commandLine`. It looks like it only uses it as a const reference, but the
    // code in the loop makes calls that mutates the `commandLine` instance, invalidating the iterators. Making a copy
    // ensures that the iterators remain valid.
    // NOLINTNEXTLINE(performance-unnecessary-value-param)
    void MergeSettingsToRegistry_CommandLine(SettingsRegistryInterface& registry, AZ::CommandLine commandLine, bool executeRegdumpCommands)
    {
        // Iterate over all the command line options in order to parse the --regset and --regremove
        // arguments in the order they were supplied
        for (const CommandLine::CommandArgument& commandArgument : commandLine)
        {
            if (commandArgument.m_option == "regset")
            {
                if (!registry.MergeCommandLineArgument(commandArgument.m_value, AZStd::string_view{}))
                {
                    AZ_Warning("SettingsRegistryMergeUtils", false, "Unable to parse argument for --regset with value of %s.",
                        commandArgument.m_value.c_str());
                    continue;
                }
            }
            else if (commandArgument.m_option == "regset-file")
            {
                AZStd::string_view fileArg(commandArgument.m_value);
                AZStd::string_view jsonAnchorPath;
                // double colons is treated as the separator for an anchor path
                // single colon cannot be used as it is used in Windows paths
                if (auto anchorPathIndex = AZ::StringFunc::Find(fileArg, "::");
                    anchorPathIndex != AZStd::string_view::npos)
                {
                    jsonAnchorPath = fileArg.substr(anchorPathIndex + 2);
                    fileArg = fileArg.substr(0, anchorPathIndex);
                }
                if (!fileArg.empty())
                {
                    AZ::IO::PathView filePath(fileArg);
                    const auto mergeFormat = filePath.Extension() != ".setregpatch"
                        ? AZ::SettingsRegistryInterface::Format::JsonMergePatch
                        : AZ::SettingsRegistryInterface::Format::JsonPatch;
                    if (!registry.MergeSettingsFile(filePath.Native(), mergeFormat, jsonAnchorPath))
                    {
                        AZ_Warning("SettingsRegistryMergeUtils", false, R"(Merging of file "%.*s" to the Settings Registry has failed at anchor  "%.*s".)",
                            AZ_STRING_ARG(filePath.Native()), AZ_STRING_ARG(jsonAnchorPath));
                        continue;
                    }
                }
            }
            else if (commandArgument.m_option == "regremove")
            {
                if (!registry.Remove(commandArgument.m_value))
                {
                    AZ_Warning("SettingsRegistryMergeUtils", false, "Unable to remove value at JSON Pointer %s for --regremove.",
                        commandArgument.m_value.c_str());
                    continue;
                }
            }
        }

        if (executeRegdumpCommands)
        {
            constexpr bool prettifyOutput = true;
            const size_t regdumpSwitchValues = commandLine.GetNumSwitchValues("regdump");
            for (size_t regdumpIndex = 0; regdumpIndex < regdumpSwitchValues; ++regdumpIndex)
            {
                AZStd::string_view regdumpValue = commandLine.GetSwitchValue("regdump", regdumpIndex);
                if (regdumpValue.empty())
                {
                    AZ_Warning("SettingsRegistryMergeUtils", false, "Unable to parse argument for --regdump with value of %.*s.",
                        aznumeric_cast<int>(regdumpValue.size()), regdumpValue.data());
                    continue;
                }

                DumperSettings dumperSettings{ prettifyOutput };
                AZ::IO::StdoutStream outputStream;
                DumpSettingsRegistryToStream(registry, regdumpValue, outputStream, dumperSettings);
            }

            if (commandLine.HasSwitch("regdumpall"))
            {
                DumperSettings dumperSettings{ prettifyOutput };
                AZ::IO::StdoutStream outputStream;
                DumpSettingsRegistryToStream(registry, "", outputStream, dumperSettings);
            }
        }
    }

    void StoreCommandLineToRegistry(SettingsRegistryInterface& registry, const AZ::CommandLine& commandLine)
    {
        // Clear out any existing CommandLine settings
        registry.Remove(CommandLineRootKey);

        AZ::SettingsRegistryInterface::FixedValueString commandLinePath{ CommandLineRootKey };
        const size_t commandLineRootSize = commandLinePath.size();
        size_t argumentIndex{};
        for (const CommandLine::CommandArgument& commandArgument : commandLine)
        {
            commandLinePath += AZ::SettingsRegistryInterface::FixedValueString::format("/%zu", argumentIndex);
            registry.Set(commandLinePath + "/Option", commandArgument.m_option);
            registry.Set(commandLinePath + "/Value", commandArgument.m_value);
            ++argumentIndex;
            commandLinePath.resize(commandLineRootSize);
        }

        // This key is used allow Notification Handlers to know when the command line has been updated within the
        // registry. The value itself is meaningless. The JSON path of {CommandLineValueChangedKey}
        // being passed to the Notification Event Handler indicates that the command line has be updated
        registry.Set(CommandLineValueChangedKey, true);
    }

    bool GetCommandLineFromRegistry(SettingsRegistryInterface& registry, AZ::CommandLine& commandLine)
    {
        struct CommandLineVisitor
            : AZ::SettingsRegistryInterface::Visitor
        {
            using AZ::SettingsRegistryInterface::Visitor::Visit;
            void Visit(const AZ::SettingsRegistryInterface::VisitArgs& visitArgs,
                AZStd::string_view value) override
            {
                if (visitArgs.m_fieldName == "Option" && !value.empty())
                {
                    m_arguments.push_back(AZStd::string::format("--%.*s", AZ_STRING_ARG(value)));
                }
                else if (visitArgs.m_fieldName == "Value" && !value.empty())
                {
                    // Make sure value types are in quotes in case they start with a command option prefix
                    m_arguments.push_back(QuoteArgument(value));
                }
            }

            AZStd::string QuoteArgument(AZStd::string_view arg)
            {
                return !arg.empty() ? AZStd::string::format(R"("%.*s")", AZ_STRING_ARG(arg)) : AZStd::string{arg};
            }

            // The first parameter is skipped by the ComamndLine::Parse function so initialize
            // the container with one empty element
            AZ::CommandLine::ParamContainer m_arguments{ 1 };
        };

        CommandLineVisitor commandLineVisitor;
        if (!registry.Visit(commandLineVisitor, AZ::SettingsRegistryMergeUtils::CommandLineRootKey))
        {
            return false;
        }
        commandLine.Parse(commandLineVisitor.m_arguments);
        return true;
    }

    void ParseCommandLine(AZ::CommandLine& commandLine)
    {
        struct OptionKeyToRegsetKey
        {
            AZStd::string_view m_optionKey;
            AZStd::string m_regsetKey;
        };

        // Provide overrides for the engine root, the project root and the project cache root
        AZStd::array commandOptions = {
            OptionKeyToRegsetKey{
                "engine-path", AZStd::string::format("%s/engine_path", BootstrapSettingsRootKey)},
            OptionKeyToRegsetKey{
                "project-path", AZStd::string::format("%s/project_path", BootstrapSettingsRootKey)},
            OptionKeyToRegsetKey{
                "project-cache-path",
                AZStd::string::format("%s/project_cache_path", BootstrapSettingsRootKey)},
            OptionKeyToRegsetKey{
                "project-user-path",
                AZStd::string::format("%s/project_user_path", BootstrapSettingsRootKey)},
            OptionKeyToRegsetKey{
                "project-log-path",
                AZStd::string::format("%s/project_log_path", BootstrapSettingsRootKey)},
            OptionKeyToRegsetKey{"project-build-path", ProjectBuildPath},
            OptionKeyToRegsetKey{
                "o3de-manifest-path", AZStd::string::format("%s/o3de_manifest_path", BootstrapSettingsRootKey)},
        };

        AZStd::fixed_vector<AZStd::string, commandOptions.size()> overrideArgs;

        for (auto&& [optionKey, regsetKey] : commandOptions)
        {
            if (size_t optionCount = commandLine.GetNumSwitchValues(optionKey); optionCount > 0)
            {
                // Use the last supplied command option value to override previous values
                auto overrideArg = AZStd::string::format(
                    R"(--regset="%s=%s")", regsetKey.c_str(), commandLine.GetSwitchValue(optionKey, optionCount - 1).c_str());
                overrideArgs.emplace_back(AZStd::move(overrideArg));
            }
        }

        if (!overrideArgs.empty())
        {
            // Dump the input command line, add the additional option overrides
            // and Parse the new command line args (write back) into the input command line.
            AZ::CommandLine::ParamContainer commandLineArgs;
            commandLine.Dump(commandLineArgs);
            commandLineArgs.insert(
                commandLineArgs.end(), AZStd::make_move_iterator(overrideArgs.begin()), AZStd::make_move_iterator(overrideArgs.end()));
            commandLine.Parse(commandLineArgs);
        }
    }

    bool DumpSettingsRegistryToStream(SettingsRegistryInterface& registry, AZStd::string_view key,
        AZ::IO::GenericStream& stream, const DumperSettings& dumperSettings)
    {
        struct SettingsExportVisitor
            : SettingsRegistryInterface::Visitor
        {
            SettingsExportVisitor(AZ::IO::GenericStream& stream, const DumperSettings& dumperSettings)
                : m_rapidJsonStream{ &stream }
                , m_dumperSettings{ dumperSettings }
                , m_writer{ m_stringBuffer }
            {
            }
            bool WriteName(AZStd::string_view name)
            {
                // Write the Key if the include name stack the top element is true
                if (!m_includeNameStack.empty() && m_includeNameStack.top())
                {
                    return m_writer.Key(name.data(), aznumeric_caster(name.size()), false);
                }

                return true;
            }
            AZ::SettingsRegistryInterface::VisitResponse Traverse(
                const AZ::SettingsRegistryInterface::VisitArgs& visitArgs, AZ::SettingsRegistryInterface::VisitAction action) override
            {
                // Pass the pointer path to the inclusion filter if available
                if (m_dumperSettings.m_includeFilter && !m_dumperSettings.m_includeFilter(visitArgs.m_jsonKeyPath))
                {
                    return AZ::SettingsRegistryInterface::VisitResponse::Skip;
                }

                if (action == AZ::SettingsRegistryInterface::VisitAction::Begin)
                {
                    m_result = m_result && WriteName(visitArgs.m_fieldName);
                    if (visitArgs.m_type.m_type == AZ::SettingsRegistryInterface::Type::Object)
                    {
                        m_result = m_result && m_writer.StartObject();
                        if (m_result)
                        {
                            m_includeNameStack.push(true);
                        }
                    }
                    else
                    {
                        m_result = m_result && m_writer.StartArray();
                        if (m_result)
                        {
                            m_includeNameStack.push(false);
                        }
                    }
                }
                else if (action == AZ::SettingsRegistryInterface::VisitAction::End)
                {
                    if (visitArgs.m_type.m_type == AZ::SettingsRegistryInterface::Type::Object)
                    {
                        m_result = m_result && m_writer.EndObject();
                    }
                    else
                    {
                        m_result = m_result && m_writer.EndArray();
                    }
                    AZ_Assert(!m_includeNameStack.empty(), "Attempting to close a json array or object that wasn't started.");
                    m_includeNameStack.pop();
                }
                else
                {
                    if (visitArgs.m_type.m_type == AZ::SettingsRegistryInterface::Type::Null)
                    {
                        m_result = m_result && WriteName(visitArgs.m_fieldName) && m_writer.Null();
                    }
                }

                return m_result ?
                    AZ::SettingsRegistryInterface::VisitResponse::Continue :
                    AZ::SettingsRegistryInterface::VisitResponse::Done;
            }

            void Visit(const AZ::SettingsRegistryInterface::VisitArgs& visitArgs, bool value) override
            {
                m_result = m_result && WriteName(visitArgs.m_fieldName) && m_writer.Bool(value);
            }

            void Visit(const AZ::SettingsRegistryInterface::VisitArgs& visitArgs, AZ::s64 value) override
            {
                m_result = m_result && WriteName(visitArgs.m_fieldName) && m_writer.Int64(value);
            }

            void Visit(const AZ::SettingsRegistryInterface::VisitArgs& visitArgs, AZ::u64 value) override
            {
                m_result = m_result && WriteName(visitArgs.m_fieldName) && m_writer.Uint64(value);
            }

            void Visit(const AZ::SettingsRegistryInterface::VisitArgs& visitArgs, double value) override
            {
                m_result = m_result && WriteName(visitArgs.m_fieldName) && m_writer.Double(value);
            }

            void Visit(const AZ::SettingsRegistryInterface::VisitArgs& visitArgs, AZStd::string_view value) override
            {
                m_result = m_result && WriteName(visitArgs.m_fieldName) && m_writer.String(value.data(), aznumeric_caster(value.size()));
            }

            bool Finalize()
            {
                if (!m_includeNameStack.empty())
                {
                    AZ_Assert(false, "m_includeNameStack is expected to be empty. This means that there was an object or array what wasn't closed.");
                    return false;
                }

                // Root the JSON document underneath the JSON pointer prefix if non-empty
                // Parse non-anchored JSON data into a document and then re-root
                // that document under the prefix value
                rapidjson::Document document;
                document.Parse(m_stringBuffer.GetString(), m_stringBuffer.GetSize());

                rapidjson::Document rootDocument;
                AZStd::string_view jsonPrefix{ m_dumperSettings.m_jsonPointerPrefix };
                if (!jsonPrefix.empty())
                {
                    rapidjson::Pointer rootPointer(jsonPrefix.data(), jsonPrefix.size());
                    rapidjson::SetValueByPointer(rootDocument, rootPointer, document);
                }
                else
                {
                    rootDocument = AZStd::move(document);
                }

                if (m_dumperSettings.m_prettifyOutput)
                {
                    rapidjson::PrettyWriter settingsWriter(m_rapidJsonStream);
                    rootDocument.Accept(settingsWriter);
                }
                else
                {
                    rapidjson::Writer settingsWriter(m_rapidJsonStream);
                    rootDocument.Accept(settingsWriter);
                }

                return true;
            }

            rapidjson::StringBuffer m_stringBuffer;
            rapidjson::Writer<rapidjson::StringBuffer> m_writer;

            AZ::IO::RapidJSONStreamWriter m_rapidJsonStream;
            DumperSettings m_dumperSettings;
            AZStd::stack<bool> m_includeNameStack;
            bool m_result{ true };
        };

        SettingsExportVisitor visitor(stream, dumperSettings);

        if (!registry.Visit(visitor, key))
        {
            AZ_Warning("SettingsRegistryMergeUtils", false, "Unable to visit Settings Registry at key %.*s.",
                aznumeric_cast<int>(key.size()), key.data());
            return false;
        }

        return visitor.Finalize();
    }

    bool IsPathAncestorDescendantOrEqual(AZStd::string_view candidatePath, AZStd::string_view inputPath)
    {
        const AZ::IO::PathView candidateView{ candidatePath, AZ::IO::PosixPathSeparator };
        const AZ::IO::PathView inputView{ inputPath, AZ::IO::PosixPathSeparator };
        return inputView.empty() || candidateView.IsRelativeTo(inputView) || inputView.IsRelativeTo(candidateView);
    }

    bool IsPathDescendantOrEqual(AZStd::string_view candidatePath, AZStd::string_view inputPath)
    {
        const AZ::IO::PathView candidateView{ candidatePath, AZ::IO::PosixPathSeparator };
        const AZ::IO::PathView inputView{ inputPath, AZ::IO::PosixPathSeparator };
        return inputView.IsRelativeTo(candidateView);
    }

    void VisitActiveGems(SettingsRegistryInterface& registry, const GemCallback& activeGemCallback)
    {
        using FixedValueString = AZ::SettingsRegistryInterface::FixedValueString;

        auto VisitGem = [&registry, &activeGemCallback](const AZ::SettingsRegistryInterface::VisitArgs& visitArgs)
        {
            // Lookup the Gem Path underneath the `ManifestGemsRootKey/<gem-name>` field
            const auto gemPathKey = FixedValueString::format("%s/%.*s/Path", ManifestGemsRootKey, AZ_STRING_ARG(visitArgs.m_fieldName));

            if (AZ::IO::FixedMaxPath gemPath; registry.Get(gemPath.Native(), gemPathKey))
            {
                activeGemCallback(visitArgs.m_fieldName, gemPath.Native());
            }
            return AZ::SettingsRegistryInterface::VisitResponse::Skip;
        };
        SettingsRegistryVisitorUtils::VisitObject(registry, VisitGem, ActiveGemsRootKey);
    }


    bool VisitManifestJson(const ManifestCallback& gemManifestCallback, AZStd::string_view manifestPath,
        AZStd::string_view manifestObjectKey)
    {
        // Make a local settings registry to load the manifest json data
        AZ::SettingsRegistryImpl manifestJsonRegistry;
        if (!manifestJsonRegistry.MergeSettingsFile(manifestPath, AZ::SettingsRegistryInterface::Format::JsonMergePatch))
        {
            AZ_Warning("SettingsRegistryMergeUtils", false, "Failed to merge manifest file of %.*s to local registry",
                AZ_STRING_ARG(manifestPath));
            return false;
        }

        // Read the O3DE object name field to validate that it is valid O3DE manifest object
        using FixedValueString = AZ::SettingsRegistryInterface::FixedValueString;
        FixedValueString manifestObjectName;
        if (!manifestJsonRegistry.Get(manifestObjectName, manifestObjectKey))
        {
            AZ_Warning("SettingsRegistryMergeUtils", false, "Failed to read manifest key of \"%.*s\" from manifest file of %.*s",
                AZ_STRING_ARG(manifestObjectKey), AZ_STRING_ARG(manifestPath));
            return false;
        }

        // Invoke the visitor callback with the tuple of gem name, gem path
        auto manifestRootDirView = AZ::IO::PathView(manifestPath).ParentPath();
        gemManifestCallback(manifestObjectKey, manifestObjectName, manifestRootDirView.Native());

        // Visit children external subdirectories
        auto VisitExternalSubdirectories = [&gemManifestCallback, &manifestJsonRegistry, manifestRootDirView]
        (const AZ::SettingsRegistryInterface::VisitArgs& visitArgs)
        {
            if (FixedValueString externalSubdirectoryPath;
                manifestJsonRegistry.Get(externalSubdirectoryPath, visitArgs.m_jsonKeyPath))
            {
                auto gemManifestPath = (AZ::IO::FixedMaxPath(manifestRootDirView)
                    / externalSubdirectoryPath / Internal::GemJsonFilename).LexicallyNormal();
                if (AZ::IO::SystemFile::Exists(gemManifestPath.c_str()))
                {
                    VisitManifestJson(gemManifestCallback, gemManifestPath.Native(), GemNameKey);
                }
            }
            return AZ::SettingsRegistryInterface::VisitResponse::Skip;
        };

        AZ::SettingsRegistryVisitorUtils::VisitArray(manifestJsonRegistry, VisitExternalSubdirectories, ExternalSubdirectoriesKey);

        return true;
    }

    bool VisitO3deManifestGems(AZ::SettingsRegistryInterface& registry, const ManifestCallback& gemManifestCallback)
    {
        AZ::IO::FixedMaxPath o3deManifestPath;
        if (!registry.Get(o3deManifestPath.Native(), FilePathKey_O3deManifestRootFolder))
        {
            o3deManifestPath = AZ::Utils::GetO3deManifestDirectory();
        }
        o3deManifestPath /= Internal::O3DEManifestJsonFilename;
        return AZ::IO::SystemFile::Exists(o3deManifestPath.c_str())
            && VisitManifestJson(gemManifestCallback, o3deManifestPath.Native(), O3DEManifestNameKey);
    }

    bool VisitEngineGems(AZ::SettingsRegistryInterface& registry, const ManifestCallback& gemManifestCallback)
    {
        AZ::IO::FixedMaxPath engineManifestPath;
        registry.Get(engineManifestPath.Native(), FilePathKey_EngineRootFolder);
        engineManifestPath /= Internal::EngineJsonFilename;
        return AZ::IO::SystemFile::Exists(engineManifestPath.c_str())
            && VisitManifestJson(gemManifestCallback, engineManifestPath.Native(), EngineNameKey);
    }

    bool VisitProjectGems(AZ::SettingsRegistryInterface& registry, const ManifestCallback& gemManifestCallback)
    {
        AZ::IO::FixedMaxPath projectManifestPath;
        registry.Get(projectManifestPath.Native(), FilePathKey_ProjectPath);
        projectManifestPath /= Internal::ProjectJsonFilename;
        return AZ::IO::SystemFile::Exists(projectManifestPath.c_str())
            && VisitManifestJson(gemManifestCallback, projectManifestPath.Native(), ProjectNameKey);
    }

    bool VisitAllManifestGems(AZ::SettingsRegistryInterface& registry, const ManifestCallback& gemManifestCallback)
    {
        bool visitedAll = VisitO3deManifestGems(registry, gemManifestCallback);
        visitedAll = VisitEngineGems(registry, gemManifestCallback) && visitedAll;
        visitedAll = VisitProjectGems(registry, gemManifestCallback) && visitedAll;
        return visitedAll;
    }
}
