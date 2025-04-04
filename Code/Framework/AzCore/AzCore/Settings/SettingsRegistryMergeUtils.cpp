/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/GenericStreams.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/IO/TextStreamWriters.h>
#include <AzCore/JSON/document.h>
#include <AzCore/JSON/pointer.h>
#include <AzCore/JSON/prettywriter.h>
#include <AzCore/JSON/writer.h>
#include <AzCore/PlatformId/PlatformDefaults.h>
#include <AzCore/Settings/CommandLine.h>
#include <AzCore/Settings/ConfigParser.h>
#include <AzCore/Settings/SettingsRegistryImpl.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/Settings/SettingsRegistryVisitorUtils.h>
#include <AzCore/std/string/conversions.h>
#include <AzCore/Utils/Utils.h>
#include <AzCore/Dependency/Dependency.h>
#include <AzCore/Outcome/Outcome.h>

#include <cinttypes>
#include <locale>


namespace AZ::Internal
{
    //! References the settings key to set the project path via *.setreg(patch) file
    //! Lowest Priority: Will be overridden by the Project Scan Up Key value
    //! This setting shouldn't be used be at all
    //! Reason: The path to the project is needed to determine where the project is located,
    //! which engine the project is using as well as which gems the project is using.
    //! Therefore this setting would need to be set within a registry file(.setreg)
    //! that is available without knowing the project root at all.
    //! The only location registry files can be reliably found while the project path is not
    //! available is in the `<userhome>/.o3de/Registry` folder
    //! This is C:\Users\<username>\.o3de\Registry on Windows = %USERPROFILE%
    //! This is /home/<username>/.o3de/Registry on Linux = $HOME
    //! This is /Users/<username>/.o3de/Registry on MacOS = $HOME
    static constexpr AZStd::string_view SetregFileProjectRootKey{ "/Amazon/AzCore/Bootstrap/project_path" };

    //! References the settings key to set the engine path via *.setreg(patch) file
    //! Lowest Priority: Will be overridden by the Engine Scan Up Key value
    //! This setting shouldn't be used be at all - see note on project path root key above
    static constexpr AZStd::string_view SetregFileEngineRootKey{ "/Amazon/AzCore/Bootstrap/engine_path" };

    //! Represents the settings key storing the value of locating the project path by scanning upwards
    //! Middle Priority: Overrides any project path set via .setreg(patch) file
    //!
    //! This setting is used when running in a project-centric workflow to locate the project root directory
    //! without the need to specify the --project-path argument.
    static constexpr AZStd::string_view ScanUpProjectRootKey{ "/O3DE/Runtime/Internal/project_root_scan_up_path" };

    //! Represents the settings key storing the value of locating the engine path by scanning upwards
    //! Middle Priority: Overrides any engine path set via .setreg(patch) file
    //!
    //! This setting is used when running in an engine-centric workflow to locate the engine root directory
    //! without the need to specify the --engine-path argument.
    static constexpr AZStd::string_view ScanUpEngineRootKey{ "/O3DE/Runtime/Internal/engine_root_scan_up_path" };

    //! References the settings key where the command line value for the --project-path option would be stored
    //! Highest Priority: Overrides any project paths specified in the "/Amazon/AzCore/Bootstrap/project_path" key
    //! or found via scanning upwards from the nearest executable directory to locate a project.json file
    //!
    //! This setting should be used when using running an O3DE application from a location where a project.json file
    //! cannot be found by scanning upwards.
    static constexpr AZStd::string_view CommandLineKey{ "/O3DE/Runtime/CommandLine" };

    static constexpr AZStd::string_view CommandLineEngineOptionName{ "engine-path" };
    static constexpr AZStd::string_view CommandLineProjectOptionName{ "project-path" };

    static constexpr AZStd::string_view EngineJsonFilename = "engine.json";
    static constexpr AZStd::string_view GemJsonFilename = "gem.json";
    static constexpr AZStd::string_view ProjectJsonFilename = "project.json";
    static constexpr AZStd::string_view O3DEManifestJsonFilename = "o3de_manifest.json";

    static constexpr const char* ProductCacheDirectoryName = "Cache";

    SettingsRegistryInterface::MergeSettingsResult MergeEngineAndProjectSettings(
        AZ::SettingsRegistryInterface& settingsRegistry,
        const AZ::IO::FixedMaxPath& engineJsonPath,
        const AZ::IO::FixedMaxPath& projectJsonPath,
        const AZ::IO::FixedMaxPath& projectUserJsonPath = {})
    {
        static constexpr AZStd::string_view InternalProjectJsonPathKey{ "/O3DE/Runtime/Internal/project_json_path" };

        using namespace AZ::SettingsRegistryMergeUtils;
        constexpr auto format = AZ::SettingsRegistryInterface::Format::JsonMergePatch;

        if (auto outcome = settingsRegistry.MergeSettingsFile(engineJsonPath.LexicallyNormal().c_str(), format, EngineSettingsRootKey);
            !outcome)
        {
            return outcome;
        }

        // Check if the currently merged project is the same
        AZ::IO::FixedMaxPath mergedProjectJsonPath;
        if (settingsRegistry.Get(mergedProjectJsonPath.Native(), InternalProjectJsonPathKey); mergedProjectJsonPath == projectJsonPath)
        {
            SettingsRegistryInterface::MergeSettingsResult result;
            result.m_returnCode = SettingsRegistryInterface::MergeSettingsReturnCode::Success;
            return result;
        }

        if (auto outcome = settingsRegistry.MergeSettingsFile(projectJsonPath.LexicallyNormal().c_str(), format, ProjectSettingsRootKey);
            !outcome)
        {
            return outcome;
        }

        // '<project-root>/user/project.json' file overrides are optional
        if (!projectUserJsonPath.empty())
        {
            settingsRegistry.MergeSettingsFile(projectUserJsonPath.LexicallyNormal().c_str(), format, ProjectSettingsRootKey);
        }

        // Set the InternalProjectJsonPathKey to avoid loading again
        settingsRegistry.Set(InternalProjectJsonPathKey, projectJsonPath.Native());

        SettingsRegistryInterface::MergeSettingsResult result;
        result.m_returnCode = SettingsRegistryInterface::MergeSettingsReturnCode::Success;
        return result;
    }

    AZ::IO::FixedMaxPath GetCommandLineOption(
        AZ::SettingsRegistryInterface& settingsRegistry, AZStd::string_view optionName)
    {
        using FixedValueString = SettingsRegistryInterface::FixedValueString;
        AZ::IO::FixedMaxPath optionPath;

        //  Parse Command Line
        auto VisitCommandLineOptions = [&optionPath, optionName](const AZ::SettingsRegistryInterface::VisitArgs& visitArgs)
        {
            // Lookup the "/O3DE/Runtime/CommandLine/%u/Option" for each command line parameter to
            // see if the key and value are available, and if they are, retrieve the value.
            auto cmdPathKey = FixedValueString::format("%.*s/Option", AZ_STRING_ARG(visitArgs.m_jsonKeyPath));
            if (FixedValueString cmdOptionName;
                visitArgs.m_registry.Get(cmdOptionName, cmdPathKey) && cmdOptionName == optionName)
            {
                // Updated the existing cmdPathKey to read the value from the command line
                cmdPathKey = FixedValueString::format("%.*s/Value", AZ_STRING_ARG(visitArgs.m_jsonKeyPath));
                visitArgs.m_registry.Get(optionPath.Native(), cmdPathKey);
            }

            // Continue to visit command line parameters, in case there is a additional matching options
            return AZ::SettingsRegistryInterface::VisitResponse::Skip;
        };
        SettingsRegistryVisitorUtils::VisitArray(settingsRegistry, VisitCommandLineOptions, Internal::CommandLineKey);

        return optionPath;
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

    void SetScanUpRootKey(AZ::SettingsRegistryInterface& settingsRegistry, AZStd::string_view key, AZStd::string_view fileLocator)
    {
        using Type = SettingsRegistryInterface::Type;
        if (settingsRegistry.GetType(key) == Type::NoType)
        {
            // We can scan up from exe directory to find fileLocator file, use that for the root if it exists.
            AZ::IO::FixedMaxPath rootPath = Internal::ScanUpRootLocator(fileLocator);
            if (!rootPath.empty() && rootPath.IsRelative())
            {
                if (auto rootAbsPath = AZ::Utils::ConvertToAbsolutePath(rootPath.Native()); rootAbsPath.has_value())
                {
                    rootPath = AZStd::move(*rootAbsPath);
                }
            }

            settingsRegistry.Set(key, rootPath.Native());
        }
    }

    AZ::Outcome<void, AZStd::string> EngineIsCompatible(
        AZ::SettingsRegistryInterface& settingsRegistry,
        const AZ::IO::FixedMaxPath& engineJsonPath,
        const AZ::IO::FixedMaxPath& projectJsonPath,
        const AZ::IO::FixedMaxPath& projectUserJsonPath = {}
        )
    {
        using FixedValueString = AZ::SettingsRegistryInterface::FixedValueString;
        using namespace AZ::SettingsRegistryMergeUtils;

        if (auto outcome = MergeEngineAndProjectSettings(settingsRegistry, engineJsonPath,
            projectJsonPath, projectUserJsonPath);
            !outcome)
        {
            return AZ::Failure(outcome.m_operationMessages);
        }

        // In project.json look for the "engine" key.
        FixedValueString projectEngineMoniker;
        const auto projectEngineKey = FixedValueString::format("%s/engine", ProjectSettingsRootKey);
        if (!settingsRegistry.Get(projectEngineMoniker, projectEngineKey) || projectEngineMoniker.empty())
        {
            return AZ::Failure("Could not find an 'engine' key in 'project.json'.\n"
                "Please verify the project is registered with a compatible engine.");
        }

        FixedValueString engineName;
        const auto engineNameKey = FixedValueString::format("%s/engine_name", EngineSettingsRootKey);
        if (!settingsRegistry.Get(engineName, engineNameKey) || engineName.empty())
        {
            return AZ::Failure(AZStd::string::format(
                "Could not find an 'engine_name' key in '%s'.\n"
                "Please verify the 'engine.json' file is not corrupt "
                "and that the engine is installed and registered.",
                engineJsonPath.c_str()));
        }

        FixedValueString projectEngineMonikerWithSpecifier = projectEngineMoniker;

        // Extract dependency specifier from engine moniker
        AZ::Dependency<SemanticVersion::parts_count> engineDependency;
        if (engineDependency.ParseVersions({ projectEngineMoniker.c_str() }) &&
            !engineDependency.GetName().empty())
        {
            projectEngineMoniker = engineDependency.GetName();
        }

        if (projectEngineMoniker != engineName)
        {
            return AZ::Failure(AZStd::string::format(
                "Engine name '%s' in project.json does not match the engine_name '%s' found in '%s'.\n"
                "Please verify the project has been registered to the correct engine.",
                projectEngineMoniker.c_str(), engineName.c_str(), engineJsonPath.c_str()));
        }

        if (engineDependency.GetBounds().empty())
        {
            // There is no version specifier to satisfy
            return AZ::Success();
        }

        // If the engine has no version information or is not known incompatible then assume compatible
        const auto engineVersionKey = FixedValueString::format("%s/version", EngineSettingsRootKey);
        if(FixedValueString engineVersionValue; settingsRegistry.Get(engineVersionValue, engineVersionKey))
        {
            using SemanticSpecifier = AZ::Specifier<AZ::SemanticVersion::parts_count>;
            AZ::SemanticVersion engineVersion;
            if (auto parseOutcome = AZ::SemanticVersion::ParseFromString(engineVersionValue.c_str()); parseOutcome)
            {
                engineVersion = parseOutcome.TakeValue();
                if(!engineDependency.IsFullfilledBy(SemanticSpecifier(AZ::Uuid::CreateNull(), engineVersion)))
                {
                    return AZ::Failure(AZStd::string::format(
                        "Engine version '%s' in '%s' does not satisfy the project.json engine constraints '%s'.\n"
                        "Please verify you have a compatible engine installed and that the project is registered  "
                        " to the correct engine.",
                        engineVersionValue.c_str(), engineJsonPath.c_str(), projectEngineMonikerWithSpecifier.c_str()));
                }
            }
        }

        return AZ::Success();
    }

    AZ::Outcome<AZ::IO::FixedMaxPath, AZStd::string> ReconcileEngineRootFromProjectUserPath(
        SettingsRegistryInterface& settingsRegistry, const AZ::IO::FixedMaxPath& projectPath)
    {
        using namespace AZ::SettingsRegistryMergeUtils;
        using FixedValueString = AZ::SettingsRegistryInterface::FixedValueString;
        constexpr auto format = AZ::SettingsRegistryInterface::Format::JsonMergePatch;
        AZ::IO::FixedMaxPath engineRoot{};

        AZ::IO::FixedMaxPath projectUserPath;
        if (!settingsRegistry.Get(projectUserPath.Native(), FilePathKey_ProjectUserPath) ||
            projectUserPath.empty())
        {
            return AZ::Success(engineRoot);
        }

        const auto projectUserJsonPath = (projectUserPath / Internal::ProjectJsonFilename).LexicallyNormal();
        if (auto outcome = settingsRegistry.MergeSettingsFile(projectUserJsonPath.c_str(), format, ProjectSettingsRootKey);
            !outcome)
        {
            return AZ::Success(engineRoot);
        }

        settingsRegistry.Get(engineRoot.Native(), FixedValueString::format("%s/engine_path", ProjectSettingsRootKey));
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

            AZ::IO::FixedMaxPath engineJsonPath{ engineRoot / Internal::EngineJsonFilename };
            AZ::IO::FixedMaxPath projectJsonPath{ projectPath / Internal::ProjectJsonFilename };
            if (auto isCompatible = Internal::EngineIsCompatible(settingsRegistry, engineJsonPath,
                projectJsonPath, projectUserJsonPath);
                !isCompatible)
            {
                return AZ::Failure(isCompatible.GetError());
            }
        }

        return AZ::Success(engineRoot);
    }

    AZ::IO::FixedMaxPath ReconcileEngineRootFromProjectPath(SettingsRegistryInterface& settingsRegistry, const AZ::IO::FixedMaxPath& projectPath)
    {
        // Find the engine root via '<project-root>/user/project.json', engine manifest and project.json
        // Locate the engine manifest file and merge it to settings registry.
        // Visit over the engine paths list and merge the engine.json files to settings registry.
        // Merge project.json to settings registry.  The "engine" key contains the engine name and optional version specifier.
        // When we find a match for "engine_name" value against the "engine" we check if the engine "version" is compatible
        // with any version specifier in the project's "engine" key.
        // If the engine is compatible we check if it is more compatible than the previously found most compatible engine.
        // Finally, merge in the engine and project settings for the most compatible engine into the registry and
        // return the path of the most compatible engine

        using namespace AZ::SettingsRegistryMergeUtils;
        using FixedValueString = AZ::SettingsRegistryInterface::FixedValueString;

        AZ::IO::FixedMaxPath engineRoot;
        if (auto o3deManifestPath = AZ::Utils::GetO3deManifestPath(&settingsRegistry); !o3deManifestPath.empty())
        {
            const auto manifestLoaded = settingsRegistry.MergeSettingsFile(o3deManifestPath,
                    AZ::SettingsRegistryInterface::Format::JsonMergePatch, O3deManifestSettingsRootKey);

            struct EngineInfo
            {
                AZ::IO::FixedMaxPath m_path;
                FixedValueString m_name;
                AZ::SemanticVersion m_version;
            };

            AZStd::set<AZ::IO::FixedMaxPath> missingProjectJsonPaths;
            AZStd::vector<EngineInfo> searchedEngineInfo;

            if (manifestLoaded)
            {
                const auto engineVersionKey = FixedValueString::format("%s/version", EngineSettingsRootKey);
                const auto engineNameKey = FixedValueString::format("%s/engine_name", EngineSettingsRootKey);
                const auto enginesKey = FixedValueString::format("%s/engines", O3deManifestSettingsRootKey);

                // Avoid modifying the SettingsRegistry while visiting which may invalidate iterators and cause crashes
                AZ::SettingsRegistryVisitorUtils::VisitArray(settingsRegistry,
                    [&](const AZ::SettingsRegistryInterface::VisitArgs& visitArgs)
                    {
                        EngineInfo engineInfo;
                        visitArgs.m_registry.Get(engineInfo.m_path.Native(), visitArgs.m_jsonKeyPath);
                        if (engineInfo.m_path.IsRelative())
                        {
                            if (auto engineRootAbsPath = AZ::Utils::ConvertToAbsolutePath(engineInfo.m_path.Native());
                                engineRootAbsPath.has_value())
                            {
                                engineInfo.m_path = AZStd::move(*engineRootAbsPath);
                            }
                        }
                        searchedEngineInfo.emplace_back(engineInfo);
                        return AZ::SettingsRegistryInterface::VisitResponse::Continue;
                    },
                    enginesKey);

                EngineInfo mostCompatibleEngineInfo;
                AZ::IO::FixedMaxPath projectUserPath;
                settingsRegistry.Get(projectUserPath.Native(), FilePathKey_ProjectUserPath);

                AZ::SettingsRegistryImpl scratchSettingsRegistry;

                // Look through the manifest engines for the most compatible engine
                for (auto& engineInfo : searchedEngineInfo)
                {
                    AZ::IO::FixedMaxPath projectJsonPath{projectPath / ProjectJsonFilename};
                    AZ::IO::FixedMaxPath projectUserJsonPath{projectUserPath / ProjectJsonFilename};
                    AZ::IO::FixedMaxPath engineJsonPath{engineInfo.m_path  / EngineJsonFilename};

                    auto isCompatible = Internal::EngineIsCompatible(scratchSettingsRegistry, engineJsonPath, projectJsonPath, projectUserJsonPath);

                    // get the engine name and version
                    scratchSettingsRegistry.Get(engineInfo.m_name, engineNameKey);

                    FixedValueString engineVersion;
                    if (scratchSettingsRegistry.Get(engineVersion, engineVersionKey); !engineVersion.empty())
                    {
                        if (auto parseOutcome = AZ::SemanticVersion::ParseFromString(engineVersion.c_str());
                            parseOutcome)
                        {
                            engineInfo.m_version = parseOutcome.TakeValue();
                        }
                    }

                    if (isCompatible)
                    {
                        if (mostCompatibleEngineInfo.m_path.empty())
                        {
                            mostCompatibleEngineInfo = engineInfo;
                        }
                        else if (!engineInfo.m_version.IsZero())
                        {
                            AZ_Warning("SettingsRegistryMergeUtils", engineInfo.m_version != mostCompatibleEngineInfo.m_version,
                                "Not using the engine at '%s' because another engine with the same name '%s' and version '%s' was already found at '%s'",
                                engineInfo.m_path.c_str(), engineInfo.m_name.c_str(), engineInfo.m_version.ToString().c_str(),
                                mostCompatibleEngineInfo.m_path.c_str());

                            // is it more compatible?
                            if (engineInfo.m_version > mostCompatibleEngineInfo.m_version)
                            {
                                mostCompatibleEngineInfo = engineInfo;
                            }
                        }
                    }

                    // Remove engine settings which will be different for each engine but keep the
                    // project settings (ProjectSettingsRootKey) in case the project path is the
                    // same to avoid re-loading them.
                    scratchSettingsRegistry.Remove(EngineSettingsRootKey);

                    if (!AZ::IO::SystemFile::Exists(projectJsonPath.c_str()))
                    {
                        // add the project path where we looked for 'project.json'
                        missingProjectJsonPaths.insert((projectPath).LexicallyNormal());
                    }
                }

                if (!mostCompatibleEngineInfo.m_path.empty())
                {
                    engineRoot = mostCompatibleEngineInfo.m_path;

                    // merge in the engine and project settings with overrides
                    Internal::MergeEngineAndProjectSettings(settingsRegistry,
                        (engineRoot  / EngineJsonFilename),
                        (projectPath / ProjectJsonFilename),
                        (engineRoot / projectUserPath / ProjectJsonFilename)
                        );
                }
            }

            if (engineRoot.empty())
            {
                AZStd::string errorStr;
                if (!missingProjectJsonPaths.empty())
                {
                    // This case is usually encountered when a project path is given as a relative path,
                    // which is assumed to be relative to an engine root.
                    // When no project.json files are found this way, dump this error message about
                    // which project paths were checked.
                    AZStd::string projectPathsTested;
                    for (const auto& path : missingProjectJsonPaths)
                    {
                        projectPathsTested.append(AZStd::string::format("  %s\n", path.c_str()));
                    }
                    errorStr = AZStd::string::format(
                        "No valid project was found (missing 'project.json') at these locations:\n%s"
                        "Please supply a valid --project-path to the application.",
                        projectPathsTested.c_str());
                }
                else
                {
                    FixedValueString projectEngineMoniker;
                    const auto projectEngineKey = FixedValueString::format("%s/engine", ProjectSettingsRootKey);
                    if (!settingsRegistry.Get(projectEngineMoniker, projectEngineKey) || projectEngineMoniker.empty())
                    {
                        projectEngineMoniker = "missing";
                    }

                    // The other case is that a project.json was found, but after checking all the registered engines
                    // none of them matched the engine moniker.
                    AZStd::string enginePathsChecked;
                    for (const auto& engineInfo : searchedEngineInfo)
                    {
                        enginePathsChecked.append(AZStd::string::format("  %s (%s %s)\n", engineInfo.m_path.c_str(),
                            engineInfo.m_name.c_str(), engineInfo.m_version.ToString().c_str()));
                    }
                    errorStr = AZStd::string::format(
                        "No engine was found in o3de_manifest.json that is compatible with the one set in the project.json '%s'\n"
                        "If that's not the engine qualifier you expected, please check if the engine field is being overridden in 'user/project.json'.\n\n"
                        "Engines that were checked:\n%s\n"
                        "Please check that your engine and project have both been registered with scripts/o3de.py.",
                        projectEngineMoniker.c_str(), enginePathsChecked.c_str()
                    );
                }

                settingsRegistry.Set(FilePathKey_ErrorText, errorStr.c_str());
            }
        }

        return engineRoot;
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
        // Step 1 Run the scan upwards logic once to find the location of the engine.json if it exist
        // Once this step is run the {InternalScanUpEngineRootKey} is set in the Settings Registry
        // and this logic will not run again for this Settings Registry instance
        Internal::SetScanUpRootKey(settingsRegistry, Internal::ScanUpEngineRootKey, Internal::EngineJsonFilename);

        // Check for the engine path to use in priority of
        // 1. command line
        // 2. <project-root>/user/project.json "engine_path"
        // 3. first compatible engine based on project.json "engine"
        // 4. First engine.json found by scanning upwards from the current executable directory
        // 5. Bootstrap engine_path from .setreg file
        AZ::IO::FixedMaxPath engineRoot = Internal::GetCommandLineOption(settingsRegistry, Internal::CommandLineEngineOptionName);

        // Note: the projectRoot should be absolute because of FindProjectRoot
        AZ::IO::FixedMaxPath projectRoot;
        settingsRegistry.Get(projectRoot.Native(), FilePathKey_ProjectPath);

        if (engineRoot.empty() && !projectRoot.empty())
        {
            // Step 2 Check for alternate 'engine_path' setting in '<project-root>/user/project.json'
            if (auto outcome = Internal::ReconcileEngineRootFromProjectUserPath(settingsRegistry, projectRoot); !outcome)
            {
                // An error occurred that needs to be shown the the user, possibly an invalid engine name or path
                settingsRegistry.Set(FilePathKey_ErrorText, outcome.GetError().c_str());
                return {};
            }
            else
            {
                engineRoot = outcome.TakeValue();
            }
        }

        if (engineRoot.empty() && !projectRoot.empty())
        {
            // 3. Locate the project root and attempt to find the most compatible engine
            // using the engine name and optional version in project.json
            engineRoot = Internal::ReconcileEngineRootFromProjectPath(settingsRegistry, projectRoot);
        }

        if (engineRoot.empty())
        {
            // 3. Use the engine scan up result
            settingsRegistry.Get(engineRoot.Native(), Internal::ScanUpEngineRootKey);
        }

        if (engineRoot.empty())
        {
            // 4. Use the bootstrap setting
            settingsRegistry.Get(engineRoot.Native(), Internal::SetregFileEngineRootKey);
        }

        // Make the engine root an absolute path if it is not empty
        if (!engineRoot.empty() && engineRoot.IsRelative())
         {
            if (auto engineRootAbsPath = AZ::Utils::ConvertToAbsolutePath(engineRoot.Native());
                engineRootAbsPath.has_value())
            {
                engineRoot = AZStd::move(*engineRootAbsPath);
            }
        }

        return engineRoot;
    }

    AZ::IO::FixedMaxPath FindProjectRoot(SettingsRegistryInterface& settingsRegistry)
    {
        // Run the scan upwards logic one time only for the supplied Settings Registry instance
        // to find the location of the closest ancestor project.json
        // Once this step is run the {ScanUpProjectRootKey} is set in the Settings Registry
        // and this logic will not run again for this Settings Registry instance
        Internal::SetScanUpRootKey(settingsRegistry, Internal::ScanUpProjectRootKey, Internal::ProjectJsonFilename);

        // Check for the project path to use in priority of
        // 1. command-line
        // 2. First project.json found by scanning upwards from the current executable directory
        // 3. "/Amazon/AzCore/Bootstrap/project_path" key set in .setreg file
        AZ::IO::FixedMaxPath projectRoot = Internal::GetCommandLineOption(settingsRegistry, Internal::CommandLineProjectOptionName);

        if (projectRoot.empty())
        {
            // 2. Check result of Scanning upwards for project.json
            settingsRegistry.Get(projectRoot.Native(), Internal::ScanUpProjectRootKey);
        }
        if (projectRoot.empty())
        {
            // 3. Check "/Amazon/AzCore/Bootstrap/project_path" key set from .setreg(patch) file
            settingsRegistry.Get(projectRoot.Native(), Internal::SetregFileProjectRootKey);
        }

        // Make the project root an absolute path if it is not empty
        if (!projectRoot.empty() && projectRoot.IsRelative())
        {
            if (auto projectAbsPath = AZ::Utils::ConvertToAbsolutePath(projectRoot.Native());
                projectAbsPath.has_value())
            {
                projectRoot = AZStd::move(*projectAbsPath);
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
        AZStd::optional<AZ::IO::FixedMaxPathString> devWriteStorage = Utils::GetDefaultDevWriteStoragePath();
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
                line = line.substr(0, commentOffset);
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

    void MergeSettingsToRegistry_AddSpecialization(SettingsRegistryInterface& registry, AZStd::string_view value)
    {
        auto targetSpecialization = AZ::SettingsRegistryInterface::FixedValueString::format("%s/%.*s",
            SpecializationsRootKey, AZ_STRING_ARG(value));
        registry.Set(targetSpecialization, true);
    }

    void MergeSettingsToRegistry_AddBuildSystemTargetSpecialization(SettingsRegistryInterface& registry,
        AZStd::string_view targetName)
    {
        registry.Set(BuildTargetNameKey, targetName);
        // Add specializations to the target registry based on the name of the Build System Target
        MergeSettingsToRegistry_AddSpecialization(registry, targetName);
    }

    bool MergeSettingsToRegistry_ConfigFile(SettingsRegistryInterface& registry, AZStd::string_view filePath,
        const ConfigParserSettings& configParserSettings)
    {
        auto configPath = FindProjectRoot(registry) / filePath;
        AZ::IO::FileIOStream fileIoStream;
        AZ::IO::SystemFileStream systemFileStream;

        const auto fileIo = AZ::IO::FileIOBase::GetInstance();

        IO::GenericStream* configFile{};
        switch (configParserSettings.m_fileReaderClass)
        {
        case ConfigParserSettings::FileReaderClass::UseFileIOIfAvailableFallbackToSystemFile:
        {
            if (fileIo != nullptr && fileIoStream.Open(configPath.c_str(), AZ::IO::OpenMode::ModeRead))
            {
                configFile = &fileIoStream;
                break;
            }
            [[fallthrough]];
        }
        case ConfigParserSettings::FileReaderClass::UseSystemFileOnly:
        {
            // If the file path is a dash, read from stdin instead
            if (filePath == "-")
            {
                systemFileStream = AZ::IO::SystemFileStream(AZ::IO::SystemFile::GetStdin());
                configFile = &systemFileStream;
            }
            if (systemFileStream.Open(configPath.c_str(), AZ::IO::OpenMode::ModeRead))
            {
                configFile = &systemFileStream;
            }
            break;
        }
        case ConfigParserSettings::FileReaderClass::UseFileIOOnly:
        {
            if (fileIo != nullptr && fileIoStream.Open(configPath.c_str(), AZ::IO::OpenMode::ModeRead))
            {
                configFile = &fileIoStream;
            }
            break;
        }
        default:
            AZ_Error("SettingsRegistryMergeUtils", false, "An Invalid FileReaderClass enum value has been supplied");
            return false;
        }
        if (configFile == nullptr)
        {
            // While all parsing and formatting errors are actual errors, config files that are not present
            // is not an error as they are always optional.  In this case, show a brief trace message
            // that indicates the location the file could be placed at in order to run it.
            AZ_Trace("SettingsRegistryMergeUtils", "Optional config file \"%s\" not found.\n", configPath.c_str());
            return false;
        }

        AZ::Settings::ConfigParserSettings parserSettings;

        parserSettings.m_parseConfigEntryFunc = [&registry, &configParserSettings]
        (const AZ::Settings::ConfigParserSettings::ConfigEntry& configEntry)
        {
            AZ::SettingsRegistryInterface::FixedValueString argumentLine(configEntry.m_sectionHeader);
            if (!argumentLine.empty())
            {
                argumentLine += '/';
            }

            argumentLine += configEntry.m_keyValuePair.m_key;
            argumentLine += '=';
            argumentLine += configEntry.m_keyValuePair.m_value;

            return registry.MergeCommandLineArgument(argumentLine, configParserSettings.m_registryRootPointerPath, configParserSettings.m_commandLineSettings);
        };

        parserSettings.m_commentPrefixFunc = configParserSettings.m_commentPrefixFunc;
        parserSettings.m_sectionHeaderFunc = configParserSettings.m_sectionHeaderFunc;
        const auto parseOutcome = AZ::Settings::ParseConfigFile(*configFile, parserSettings);

        return parseOutcome.has_value();
    }

    void MergeSettingsToRegistry_ManifestGemsPaths(SettingsRegistryInterface& registry)
    {
        // cache a vector so that we don't mutate the registry while inside visitor iteration.
        AZStd::vector<AZStd::pair<AZStd::string, AZ::IO::FixedMaxPath>> collectedGems;
        auto CollectManifestGems =
            [&collectedGems](AZStd::string_view manifestKey, AZStd::string_view gemName, AZ::IO::PathView gemRootPath)
        {
            if (manifestKey == GemNameKey)
            {
                collectedGems.push_back(AZStd::make_pair(AZStd::string(gemName), AZ::IO::FixedMaxPath(gemRootPath)));
            }
        };

        VisitAllManifestGems(registry, CollectManifestGems);

        for (const auto& [gemName, gemRootPath] : collectedGems)
        {
            using FixedValueString = SettingsRegistryInterface::FixedValueString;
            const auto manifestGemJsonPath = FixedValueString::format("%s/%.*s/Path", ManifestGemsRootKey, AZ_STRING_ARG(gemName));
            registry.Set(manifestGemJsonPath, gemRootPath.LexicallyNormal().Native());
        }
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
            projectPath = exePath;
            registry.Set(FilePathKey_ProjectPath, exePath.Native());
        }

        // User folder
        AZ::IO::FixedMaxPath projectUserPath = FindProjectUserPath(registry, projectPath);
        if (!projectUserPath.empty())
        {
            projectUserPath = projectUserPath.LexicallyNormal();
            registry.Set(FilePathKey_ProjectUserPath, projectUserPath.Native());
        }

        // Engine root folder - corresponds to the @engroot@ alias
        AZ::IO::FixedMaxPath engineRoot = FindEngineRoot(registry);
        if (engineRoot.empty())
        {
            // Use the project path if the engine root wasn't found, this can happen
            // when running the game launcher with bundled assets which are not loaded
            // until after the SettingsRegistry has determined file paths
            engineRoot = projectPath;
        }

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
        if (AZStd::optional<AZ::IO::FixedMaxPathString> devWriteStorage = Utils::GetDefaultDevWriteStoragePath();
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

    auto MergeSettingsToRegistry_TargetBuildDependencyRegistry(SettingsRegistryInterface& registry, const AZStd::string_view platform,
        const SettingsRegistryInterface::Specializations& specializations, AZStd::vector<char>* scratchBuffer)
        -> SettingsRegistryInterface::MergeSettingsResult
    {
        SettingsRegistryInterface::MergeSettingsResult aggregateMergeResult;

        AZ::IO::FixedMaxPath mergePath = AZ::Utils::GetExecutableDirectory();
        if (!mergePath.empty())
        {
            aggregateMergeResult.Combine(registry.MergeSettingsFolder((mergePath / SettingsRegistryInterface::RegistryFolder).Native(),
                specializations, platform, "", scratchBuffer));
        }

        // Look within the Cache Root directory for target build dependency .setreg files
        AZ::SettingsRegistryInterface::FixedValueString cacheRootPath;
        if (registry.Get(cacheRootPath, FilePathKey_CacheRootFolder))
        {
            mergePath = AZStd::move(cacheRootPath);
            AZStd::fixed_string<32> registryFolderLower(SettingsRegistryInterface::RegistryFolder);
            AZStd::to_lower(registryFolderLower.begin(), registryFolderLower.end());
            mergePath /= registryFolderLower;
            aggregateMergeResult.Combine(registry.MergeSettingsFolder(mergePath.Native(), specializations, platform, "", scratchBuffer));
        }

        AZ::IO::FixedMaxPath projectBinPath;
        if (registry.Get(projectBinPath.Native(), FilePathKey_ProjectConfigurationBinPath))
        {
            // Append the project build path path to the project root
            projectBinPath /= SettingsRegistryInterface::RegistryFolder;
            aggregateMergeResult.Combine(registry.MergeSettingsFolder(projectBinPath.Native(), specializations, platform, "", scratchBuffer));
        }

        return aggregateMergeResult;
    }

    auto MergeSettingsToRegistry_EngineRegistry(SettingsRegistryInterface& registry, const AZStd::string_view platform,
        const SettingsRegistryInterface::Specializations& specializations, AZStd::vector<char>* scratchBuffer)
        -> SettingsRegistryInterface::MergeSettingsResult
    {
        SettingsRegistryInterface::MergeSettingsResult mergeResult;
        AZ::SettingsRegistryInterface::FixedValueString engineRootPath;
        if (registry.Get(engineRootPath, FilePathKey_EngineRootFolder))
        {
            AZ::IO::FixedMaxPath mergePath{ AZStd::move(engineRootPath) };
            mergePath /= SettingsRegistryInterface::RegistryFolder;
            mergeResult.Combine(registry.MergeSettingsFolder(mergePath.Native(), specializations, platform, "", scratchBuffer));
        }

        return mergeResult;
    }

    auto MergeSettingsToRegistry_GemRegistries(SettingsRegistryInterface& registry, const AZStd::string_view platform,
        const SettingsRegistryInterface::Specializations& specializations, AZStd::vector<char>* scratchBuffer)
        -> SettingsRegistryInterface::MergeSettingsResult
    {
        // collect the paths first, then mutate the registry, so that we do not do any registry modifications while visiting it.
        AZStd::vector<AZ::IO::FixedMaxPath> gemPaths;
        auto CollectRegistryFolders = [&gemPaths]
            (AZStd::string_view, AZ::IO::FixedMaxPath gemPath)
        {
            gemPaths.push_back(gemPath);
        };

        VisitActiveGems(registry, CollectRegistryFolders);

        SettingsRegistryInterface::MergeSettingsResult aggregateMergeResult;
        for (const auto& gemPath : gemPaths)
        {
            aggregateMergeResult.Combine(registry.MergeSettingsFolder(
                (gemPath / SettingsRegistryInterface::RegistryFolder).Native(), specializations, platform, "", scratchBuffer));
        }

        return aggregateMergeResult;
    }

    auto MergeSettingsToRegistry_ProjectRegistry(SettingsRegistryInterface& registry, const AZStd::string_view platform,
        const SettingsRegistryInterface::Specializations& specializations, AZStd::vector<char>* scratchBuffer)
        -> SettingsRegistryInterface::MergeSettingsResult
    {
        SettingsRegistryInterface::MergeSettingsResult mergeResult;
        AZ::SettingsRegistryInterface::FixedValueString projectPath;
        if (registry.Get(projectPath, FilePathKey_ProjectPath))
        {
            AZ::IO::FixedMaxPath mergePath{ projectPath };
            mergePath /= SettingsRegistryInterface::RegistryFolder;
            registry.MergeSettingsFolder(mergePath.Native(), specializations, platform, "", scratchBuffer);
        }

        return mergeResult;
    }

    auto MergeSettingsToRegistry_ProjectUserRegistry(SettingsRegistryInterface& registry, const AZStd::string_view platform,
        const SettingsRegistryInterface::Specializations& specializations, AZStd::vector<char>* scratchBuffer)
        -> SettingsRegistryInterface::MergeSettingsResult
    {
        SettingsRegistryInterface::MergeSettingsResult mergeResult;
        // Unlike other paths, the path can't be overwritten by the user settings because that would create a circular dependency.
        AZ::IO::FixedMaxPath projectUserPath;
        if (registry.Get(projectUserPath.Native(), FilePathKey_ProjectPath))
        {
            projectUserPath /= SettingsRegistryInterface::DevUserRegistryFolder;
            mergeResult.Combine(registry.MergeSettingsFolder(projectUserPath.Native(), specializations, platform, "", scratchBuffer));
        }

        return mergeResult;
    }

    auto MergeSettingsToRegistry_O3deUserRegistry(SettingsRegistryInterface& registry, const AZStd::string_view platform,
        const SettingsRegistryInterface::Specializations& specializations, AZStd::vector<char>* scratchBuffer)
        -> SettingsRegistryInterface::MergeSettingsResult
    {
        SettingsRegistryInterface::MergeSettingsResult mergeResult;
        if (AZ::IO::FixedMaxPath o3deUserPath = AZ::Utils::GetO3deManifestDirectory(); !o3deUserPath.empty())
        {
            o3deUserPath /= SettingsRegistryInterface::RegistryFolder;
            mergeResult.Combine(registry.MergeSettingsFolder(o3deUserPath.Native(), specializations, platform, "", scratchBuffer));
        }

        return mergeResult;
    }

    // This function intentionally copies `commandLine`. It looks like it only uses it as a const reference, but the
    // code in the loop makes calls that mutates the `commandLine` instance, invalidating the iterators. Making a copy
    // ensures that the iterators remain valid.
    // NOLINTNEXTLINE(performance-unnecessary-value-param)
    void MergeSettingsToRegistry_CommandLine(SettingsRegistryInterface& registry, AZ::CommandLine commandLine, const CommandsToParse& commandsToParse)
    {
        // Iterate over all the command line options in order to parse the --regset and --regremove
        // arguments in the order they were supplied
        for (const CommandLine::CommandArgument& commandArgument : commandLine)
        {
            if (commandsToParse.m_parseRegsetCommands && commandArgument.m_option == "regset")
            {
                if (!registry.MergeCommandLineArgument(commandArgument.m_value, AZStd::string_view{}))
                {
                    AZ_Warning("SettingsRegistryMergeUtils", false, "Unable to parse argument for --regset with value of %s.",
                        commandArgument.m_value.c_str());
                    continue;
                }
            }
            // Only merge the regset-file when executeReg
            else if (commandsToParse.m_parseRegsetFileCommands && commandArgument.m_option == "regset-file")
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
                    if (auto mergeResult = registry.MergeSettingsFile(filePath.Native(), mergeFormat, jsonAnchorPath);
                        !mergeResult)
                    {
                        AZ_Warning("SettingsRegistryMergeUtils", false,
                            R"(Merging of file "%.*s" to the Settings Registry has failed at anchor "%.*s".)" "\n"
                            "Error message is %s",
                            AZ_STRING_ARG(filePath.Native()), AZ_STRING_ARG(jsonAnchorPath)
                            , mergeResult.GetMessages().c_str());
                        continue;
                    }
                }
            }
            else if (commandsToParse.m_parseRegremoveCommands && commandArgument.m_option == "regremove")
            {
                if (!registry.Remove(commandArgument.m_value))
                {
                    AZ_Warning("SettingsRegistryMergeUtils", false, "Unable to remove value at JSON Pointer %s for --regremove.",
                        commandArgument.m_value.c_str());
                    continue;
                }
            }
        }

        if (commandsToParse.m_parseRegdumpCommands)
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

        // Provide mappings for the engine root directroy, project product directory(<project-root>/Cache/<asset-platform>),
        // project user directory (<project-root>/user), project log directory (<project-root>/user/log)
        // command line options to regset options
        //
        // A mapping for the project-build-path option which represents the CMake binary directory
        // supplied during configure is also available to be mapped to a regset setting
        //
        // The o3de-manifest-path option maps to a regset option that allows users to override
        // the directory containing their o3de_manifest.json file.
        AZStd::array commandOptions = {
            OptionKeyToRegsetKey{
                "engine-path", AZStd::string::format("%s/engine_path", BootstrapSettingsRootKey)},
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
