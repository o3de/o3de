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
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/Settings/CommandLine.h>
#include <AzCore/std/string/conversions.h>
#include <AzCore/std/string/wildcard.h>
#include <AzCore/std/tuple.h>
#include <AzCore/std/containers/variant.h>
#include <AzCore/Utils/Utils.h>

#include <cinttypes>
#include <locale>

namespace AZ::Internal
{
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
        if (auto engineManifestPath = AZ::Utils::GetEngineManifestPath(); !engineManifestPath.empty())
        {
            bool manifestLoaded{false};

            if (AZ::IO::SystemFile::Exists(engineManifestPath.c_str()))
            {
                manifestLoaded = settingsRegistry.MergeSettingsFile(
                    engineManifestPath, AZ::SettingsRegistryInterface::Format::JsonMergePatch, EngineManifestRootKey);
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
                    [[maybe_unused]] AZStd::string_view path, AZStd::string_view valueName,
                    [[maybe_unused]] AZ::SettingsRegistryInterface::Type type, AZStd::string_view value) override
                {
                    m_enginePaths.emplace_back(EngineInfo{ AZ::IO::FixedMaxPath{value}.LexicallyNormal(), FixedValueString{valueName} });
                }

                AZStd::vector<EngineInfo> m_enginePaths{};
            };

            EnginePathsVisitor pathVisitor;
            if (manifestLoaded)
            {
                auto enginePathsKey = FixedValueString::format("%s/engines_path", EngineManifestRootKey);
                settingsRegistry.Visit(pathVisitor, enginePathsKey);
            }

            const auto engineMonikerKey = FixedValueString::format("%s/engine_name", EngineSettingsRootKey);

            AZStd::set<AZ::IO::FixedMaxPath> projectPathsNotFound;

            for (EngineInfo& engineInfo : pathVisitor.m_enginePaths)
            {
                if (auto engineSettingsPath = AZ::IO::FixedMaxPath{engineInfo.m_path} / "engine.json";
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
                            engineInfo.m_moniker.c_str(), engineInfo.m_path.c_str(), engineManifestPath.c_str(),
                            engineName.c_str());
                        engineInfo.m_moniker = engineName;
                    }
                }

                if (auto projectJsonPath = (engineInfo.m_path / projectPath / "project.json").LexicallyNormal();
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

    void InjectSettingToCommandLineBack(AZ::SettingsRegistryInterface& settingsRegistry,
        AZStd::string_view path, AZStd::string_view value)
    {
        AZ::CommandLine commandLine;
        AZ::SettingsRegistryMergeUtils::GetCommandLineFromRegistry(settingsRegistry, commandLine);
        AZ::CommandLine::ParamContainer paramContainer;
        commandLine.Dump(paramContainer);

        auto projectPathOverride = AZStd::string::format(R"(--regset="%.*s=%.*s")",
            aznumeric_cast<int>(path.size()), path.data(), aznumeric_cast<int>(value.size()), value.data());
        paramContainer.emplace(paramContainer.end(), AZStd::move(projectPathOverride));
        commandLine.Parse(paramContainer);
        AZ::SettingsRegistryMergeUtils::StoreCommandLineToRegistry(settingsRegistry, commandLine);
    }
} // namespace AZ::Internal

namespace AZ::SettingsRegistryMergeUtils
{
    constexpr AZStd::string_view InternalScanUpEngineRootKey{ "/O3DE/Settings/Internal/engine_root_scan_up_path" };
    constexpr AZStd::string_view InternalScanUpProjectRootKey{ "/O3DE/Settings/Internal/project_root_scan_up_path" };

    AZ::IO::FixedMaxPath FindEngineRoot(SettingsRegistryInterface& settingsRegistry)
    {
        AZ::IO::FixedMaxPath engineRoot;
        // This is the 'external' engine root key, as in passed from command-line or .setreg files.
        auto engineRootKey = SettingsRegistryInterface::FixedValueString::format("%s/engine_path", BootstrapSettingsRootKey);

        // Step 1 Run the scan upwards logic once to find the location of the engine.json if it exist
        // Once this step is run the {InternalScanUpEngineRootKey} is set in the Settings Registry
        // to have this scan logic only run once InternalScanUpEngineRootKey the supplied registry
        if (settingsRegistry.GetType(InternalScanUpEngineRootKey) == SettingsRegistryInterface::Type::NoType)
        {
            // We can scan up from exe directory to find engine.json, use that for engine root if it exists.
            engineRoot = Internal::ScanUpRootLocator("engine.json");
            // Set the {InternalScanUpEngineRootKey} to make sure this code path isn't called again for this settings registry
            settingsRegistry.Set(InternalScanUpEngineRootKey, engineRoot.Native());
            if (!engineRoot.empty())
            {
                settingsRegistry.Set(engineRootKey, engineRoot.Native());
                // Inject the engine root at the end of the command line settings
                Internal::InjectSettingToCommandLineBack(settingsRegistry, engineRootKey, engineRoot.Native());
                return engineRoot;
            }
        }

        // Step 2 check if the engine_path key has been supplied
        if (settingsRegistry.Get(engineRoot.Native(), engineRootKey); !engineRoot.empty())
        {
            return engineRoot;
        }

        // Step 3 locate the project root and attempt to find the engine root using the registered engine
        // for the project in the project.json file
        AZ::IO::FixedMaxPath projectRoot = FindProjectRoot(settingsRegistry);
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
        AZ::IO::FixedMaxPath projectRoot;
        const auto projectRootKey = SettingsRegistryInterface::FixedValueString::format("%s/project_path", BootstrapSettingsRootKey);

        // Step 1 Run the scan upwards logic once to find the location of the project.json if it exist
        // Once this step is run the {InternalScanUpProjectRootKey} is set in the Settings Registry
        // to have this scan logic only run once for the supplied registry
        // SettingsRegistryInterface::GetType is used to check if a key is set
        if (settingsRegistry.GetType(InternalScanUpProjectRootKey) == SettingsRegistryInterface::Type::NoType)
        {
            projectRoot = Internal::ScanUpRootLocator("project.json");
            // Set the {InternalScanUpProjectRootKey} to make sure this code path isn't called again for this settings registry
            settingsRegistry.Set(InternalScanUpProjectRootKey, projectRoot.Native());
            if (!projectRoot.empty())
            {
                settingsRegistry.Set(projectRootKey, projectRoot.c_str());
                // Inject the project root at the end of the command line settings
                Internal::InjectSettingToCommandLineBack(settingsRegistry, projectRootKey, projectRoot.Native());
                return projectRoot;
            }
        }

        // Step 2 Check the project-path key
        // This is the project path root key, as in passed from command-line or .setreg files.
        if (settingsRegistry.Get(projectRoot.Native(), projectRootKey))
        {
            return projectRoot;
        }

        // Step 3 Check for a "Cache" directory by scanning upwards from the executable directory
        if (auto candidateRoot = Internal::ScanUpRootLocator("Cache");
            !candidateRoot.empty() && AZ::IO::SystemFile::IsDirectory(candidateRoot.c_str()))
        {
            projectRoot = AZStd::move(candidateRoot);
        }
        return projectRoot;
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
            void Visit([[maybe_unused]] AZStd::string_view path, AZStd::string_view valueName,
                [[maybe_unused]] AZ::SettingsRegistryInterface::Type type, bool value) override
            {
                if (value)
                {
                    // The specialization is the key itself.
                    // The value is just used to determine if the specialization should be added
                    m_settingsSpecialization.Append(valueName);
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
        auto configPath = FindEngineRoot(registry) / filePath;
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
            AZ_Warning("SettingsRegistryMergeUtils", false, R"(Unable to open file "%s")", configPath.c_str());
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

    void MergeSettingsToRegistry_AddRuntimeFilePaths(SettingsRegistryInterface& registry)
    {
        using FixedValueString = AZ::SettingsRegistryInterface::FixedValueString;
        // Binary folder
        AZ::IO::FixedMaxPath path = AZ::Utils::GetExecutableDirectory();
        registry.Set(FilePathKey_BinaryFolder, path.LexicallyNormal().Native());

        // Engine root folder - corresponds to the @engroot@ and @engroot@ aliases
        AZ::IO::FixedMaxPath engineRoot = FindEngineRoot(registry);
        registry.Set(FilePathKey_EngineRootFolder, engineRoot.LexicallyNormal().Native());

        auto projectPathKey = FixedValueString::format("%s/project_path", BootstrapSettingsRootKey);
        SettingsRegistryInterface::FixedValueString projectPathValue;
        if (registry.Get(projectPathValue, projectPathKey))
        {
            // Cache folder
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

            // Project path - corresponds to the @projectroot@ alias
            // NOTE: Here we append to engineRoot, but if projectPathValue is absolute then engineRoot is discarded.
            path = engineRoot / projectPathValue;

            AZ_Warning("SettingsRegistryMergeUtils", AZ::IO::SystemFile::Exists(path.c_str()),
                R"(Project path "%s" does not exist. Is the "%.*s" registry setting set to valid absolute path?)"
                , path.c_str(), aznumeric_cast<int>(projectPathKey.size()), projectPathKey.data());

            AZ::IO::FixedMaxPath normalizedProjectPath = path.LexicallyNormal();
            registry.Set(FilePathKey_ProjectPath, normalizedProjectPath.Native());

            // Set the user directory with the provided path or using project/user as default
            auto projectUserPathKey = FixedValueString::format("%s/project_user_path", BootstrapSettingsRootKey);
            AZ::IO::FixedMaxPath projectUserPath;
            if (!registry.Get(projectUserPath.Native(), projectUserPathKey))
            {
                projectUserPath = (normalizedProjectPath / "user").LexicallyNormal();
            }
            registry.Set(FilePathKey_ProjectUserPath, projectUserPath.Native());

            // Set the log directory with the provided path or using project/user/log as default
            auto projectLogPathKey = FixedValueString::format("%s/project_log_path", BootstrapSettingsRootKey);
            AZ::IO::FixedMaxPath projectLogPath;
            if (!registry.Get(projectLogPath.Native(), projectLogPathKey))
            {
                projectLogPath = (projectUserPath / "log").LexicallyNormal();
            }
            registry.Set(FilePathKey_ProjectLogPath, projectLogPath.Native());

            // check for a default write storage path, fall back to the project's user/ directory if not
            AZStd::optional<AZ::IO::FixedMaxPathString> devWriteStorage = Utils::GetDevWriteStoragePath();
            registry.Set(FilePathKey_DevWriteStorage, devWriteStorage.has_value()
                ? devWriteStorage.value()
                : projectUserPath.Native());

            // Set the project in-memory build path if the ProjectBuildPath key has been supplied
            if (AZ::IO::FixedMaxPath projectBuildPath; registry.Get(projectBuildPath.Native(), ProjectBuildPath))
            {
                registry.Remove(FilePathKey_ProjectBuildPath);
                registry.Remove(FilePathKey_ProjectConfigurationBinPath);
                AZ::IO::FixedMaxPath buildConfigurationPath = normalizedProjectPath / projectBuildPath;
                if (IO::SystemFile::Exists(buildConfigurationPath.c_str()))
                {
                    registry.Set(FilePathKey_ProjectBuildPath, buildConfigurationPath.LexicallyNormal().Native());
                }

                // Add the specific build configuration paths to the Settings Registry
                // First try <project-build-path>/bin/$<CONFIG> and if that path doesn't exist
                // try <project-build-path>/bin/$<PLATFORM>/$<CONFIG>
                buildConfigurationPath /= "bin";
                if (IO::SystemFile::Exists((buildConfigurationPath / AZ_BUILD_CONFIGURATION_TYPE).c_str()))
                {
                    registry.Set(FilePathKey_ProjectConfigurationBinPath,
                        (buildConfigurationPath / AZ_BUILD_CONFIGURATION_TYPE).LexicallyNormal().Native());
                }
                else if (IO::SystemFile::Exists((buildConfigurationPath / AZ_TRAIT_OS_PLATFORM_CODENAME / AZ_BUILD_CONFIGURATION_TYPE).c_str()))
                {
                    registry.Set(FilePathKey_ProjectConfigurationBinPath,
                        (buildConfigurationPath / AZ_TRAIT_OS_PLATFORM_CODENAME / AZ_BUILD_CONFIGURATION_TYPE).LexicallyNormal().Native());
                }

            }

            // Project name - if it was set via merging project.json use that value, otherwise use the project path's folder name.
            auto projectNameKey =
                AZ::SettingsRegistryInterface::FixedValueString(AZ::SettingsRegistryMergeUtils::ProjectSettingsRootKey)
                + "/project_name";

            AZ::SettingsRegistryInterface::FixedValueString projectName;
            if (!registry.Get(projectName, projectNameKey))
            {
                projectName = path.Filename().Native();
                registry.Set(projectNameKey, projectName);
            }

            // Cache folders - sets up various paths in registry for the cache.
            // Make sure the asset platform is set before setting these cache paths.
            if (!assetPlatform.empty())
            {
                // Cache: project root - no corresponding fileIO alias, but this is where the asset database lives.
                // A registry override is accepted using the "project_cache_path" key.
                auto projectCacheRootOverrideKey = FixedValueString::format("%s/project_cache_path", BootstrapSettingsRootKey);
                // Clear path to make sure that the `project_cache_path` value isn't concatenated to the project path
                path.clear();
                if (registry.Get(path.Native(), projectCacheRootOverrideKey))
                {
                    registry.Set(FilePathKey_CacheProjectRootFolder, path.LexicallyNormal().Native());
                    path /= assetPlatform;
                    registry.Set(FilePathKey_CacheRootFolder, path.LexicallyNormal().Native());
                }
                else
                {
                    // Cache: root - same as the @products@ alias, this is the starting path for cache files.
                    path = normalizedProjectPath / "Cache";
                    registry.Set(FilePathKey_CacheProjectRootFolder, path.LexicallyNormal().Native());
                    path /= assetPlatform;
                    registry.Set(FilePathKey_CacheRootFolder, path.LexicallyNormal().Native());
                }
            }
        }
        else
        {
            // Set the default ProjectUserPath to the <engine-root>/user directory
            registry.Set(FilePathKey_ProjectUserPath, (engineRoot / "user").LexicallyNormal().Native());
            AZ_TracePrintf("SettingsRegistryMergeUtils",
                R"(Project path isn't set in the Settings Registry at "%.*s". Project-related filepaths will not be set)" "\n",
                aznumeric_cast<int>(projectPathKey.size()), projectPathKey.data());
        }

#if !AZ_TRAIT_OS_IS_HOST_OS_PLATFORM
        // Setup the cache, user, and log paths to platform specific locations when running on non-host platforms
        path = engineRoot;
        if (AZStd::optional<AZ::IO::FixedMaxPathString> nonHostCacheRoot = Utils::GetDefaultAppRootPath();
            nonHostCacheRoot)
        {
            registry.Set(FilePathKey_CacheProjectRootFolder, *nonHostCacheRoot);
            registry.Set(FilePathKey_CacheRootFolder, *nonHostCacheRoot);
        }
        else
        {
            registry.Set(FilePathKey_CacheProjectRootFolder, path.LexicallyNormal().Native());
            registry.Set(FilePathKey_CacheRootFolder, path.LexicallyNormal().Native());
        }
        if (AZStd::optional<AZ::IO::FixedMaxPathString> devWriteStorage = Utils::GetDevWriteStoragePath();
            devWriteStorage)
        {
            const AZ::IO::FixedMaxPath devWriteStoragePath(*devWriteStorage);
            registry.Set(FilePathKey_DevWriteStorage, devWriteStoragePath.LexicallyNormal().Native());
            registry.Set(FilePathKey_ProjectUserPath, (devWriteStoragePath / "user").LexicallyNormal().Native());
            registry.Set(FilePathKey_ProjectLogPath, (devWriteStoragePath / "user/log").LexicallyNormal().Native());
        }
        else
        {
            registry.Set(FilePathKey_DevWriteStorage, path.LexicallyNormal().Native());
            registry.Set(FilePathKey_ProjectUserPath, (path / "user").LexicallyNormal().Native());
            registry.Set(FilePathKey_ProjectLogPath, (path / "user/log").LexicallyNormal().Native());
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
        auto pathBuffer = AZ::SettingsRegistryInterface::FixedValueString::format("%s/Gems", OrganizationRootKey);
        AZStd::string_view gemListPath(pathBuffer);

        AZ::SettingsRegistryInterface::FixedValueString engineRootPath;
        if (registry.GetType(gemListPath) == SettingsRegistryInterface::Type::Object &&
            registry.Get(engineRootPath, FilePathKey_EngineRootFolder))
        {
            struct Visitor
                : public SettingsRegistryInterface::Visitor
            {
                AZ::IO::FixedMaxPath m_gemPath;
                SettingsRegistryInterface& m_registry;
                const AZStd::string_view m_platform;
                const SettingsRegistryInterface::Specializations& m_specializations;
                AZStd::vector<char>* m_scratchBuffer{};
                bool processingSourcePathKey{};

                Visitor(AZ::SettingsRegistryInterface::FixedValueString rootFolder, SettingsRegistryInterface& registry, const AZStd::string_view platform,
                    const SettingsRegistryInterface::Specializations& specializations, AZStd::vector<char>* scratchBuffer)
                    : m_gemPath(AZStd::move(rootFolder))
                    , m_registry(registry)
                    , m_platform(platform)
                    , m_specializations(specializations)
                    , m_scratchBuffer(scratchBuffer)
                {
                }

                SettingsRegistryInterface::VisitResponse Traverse([[maybe_unused]] AZStd::string_view path, AZStd::string_view valueName,
                    SettingsRegistryInterface::VisitAction action, [[maybe_unused]] SettingsRegistryInterface::Type type) override
                {
                    if (valueName == "SourcePaths")
                    {
                        if (action == SettingsRegistryInterface::VisitAction::Begin)
                        {
                            // Allows merging of the registry folders within the gem source path array
                            // via the Visit function
                            processingSourcePathKey = true;
                        }
                        else if (action == SettingsRegistryInterface::VisitAction::End)
                        {
                            // The end of the gem source path array has been reached
                            processingSourcePathKey = false;
                        }
                    }

                    return SettingsRegistryInterface::VisitResponse::Continue;
                }

                using AZ::SettingsRegistryInterface::Visitor::Visit;
                void Visit(AZStd::string_view, [[maybe_unused]] AZStd::string_view valueName, SettingsRegistryInterface::Type, AZStd::string_view value) override
                {
                    if (processingSourcePathKey)
                    {
                        m_registry.MergeSettingsFolder((m_gemPath / value / SettingsRegistryInterface::RegistryFolder).Native(),
                            m_specializations, m_platform, "", m_scratchBuffer);
                    }
                }
            };

            Visitor visitor(AZStd::move(engineRootPath), registry, platform, specializations, scratchBuffer);
            registry.Visit(visitor, gemListPath);
        }
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
    void MergeSettingsToRegistry_CommandLine(SettingsRegistryInterface& registry, AZ::CommandLine commandLine, bool executeCommands)
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
            else if (commandArgument.m_option == "regremove")
            {
                if (!registry.Remove(commandArgument.m_value))
                {
                    AZ_Warning("SettingsRegistryMergeUtils", false, "Unable to remove value at JSON Pointer %s for --regremove.",
                        commandArgument.m_value.data());
                    continue;
                }
            }
        }

        if (executeCommands)
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
            void Visit(AZStd::string_view, AZStd::string_view valueName, AZ::SettingsRegistryInterface::Type
                , AZStd::string_view value) override
            {
                if (valueName == "Option" && !value.empty())
                {
                    m_arguments.push_back(AZStd::string::format("--%.*s", aznumeric_cast<int>(value.size()), value.data()));
                }
                else if (valueName == "Value" && !value.empty())
                {
                    // Make sure value types are in quotes in case they start with a command option prefix
                    m_arguments.push_back(QuoteArgument(value));
                }
            }

            AZStd::string QuoteArgument(AZStd::string_view arg)
            {
                return !arg.empty() ? AZStd::string::format(R"("%.*s")", aznumeric_cast<int>(arg.size()), arg.data()) : AZStd::string{ arg };
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
                "engine-path", AZStd::string::format("%s/engine_path", AZ::SettingsRegistryMergeUtils::BootstrapSettingsRootKey)},
            OptionKeyToRegsetKey{
                "project-path", AZStd::string::format("%s/project_path", AZ::SettingsRegistryMergeUtils::BootstrapSettingsRootKey)},
            OptionKeyToRegsetKey{
                "project-cache-path",
                AZStd::string::format("%s/project_cache_path", AZ::SettingsRegistryMergeUtils::BootstrapSettingsRootKey)},
            OptionKeyToRegsetKey{
                "project-user-path",
                AZStd::string::format("%s/project_user_path", AZ::SettingsRegistryMergeUtils::BootstrapSettingsRootKey)},
            OptionKeyToRegsetKey{
                "project-log-path",
                AZStd::string::format("%s/project_log_path", AZ::SettingsRegistryMergeUtils::BootstrapSettingsRootKey)},
            OptionKeyToRegsetKey{"project-build-path", ProjectBuildPath},
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
                AZStd::string_view path, AZStd::string_view valueName, AZ::SettingsRegistryInterface::VisitAction action,
                AZ::SettingsRegistryInterface::Type type) override
            {
                // Pass the pointer path to the inclusion filter if available
                if (m_dumperSettings.m_includeFilter && !m_dumperSettings.m_includeFilter(path))
                {
                    return AZ::SettingsRegistryInterface::VisitResponse::Skip;
                }

                if (action == AZ::SettingsRegistryInterface::VisitAction::Begin)
                {
                    m_result = m_result && WriteName(valueName);
                    if (type == AZ::SettingsRegistryInterface::Type::Object)
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
                    if (type == AZ::SettingsRegistryInterface::Type::Object)
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
                    if (type == AZ::SettingsRegistryInterface::Type::Null)
                    {
                        m_result = m_result && WriteName(valueName) && m_writer.Null();
                    }
                }

                return m_result ?
                    AZ::SettingsRegistryInterface::VisitResponse::Continue :
                    AZ::SettingsRegistryInterface::VisitResponse::Done;
            }

            void Visit(AZStd::string_view, AZStd::string_view valueName, AZ::SettingsRegistryInterface::Type, bool value) override
            {
                m_result = m_result && WriteName(valueName) && m_writer.Bool(value);
            }

            void Visit(AZStd::string_view, AZStd::string_view valueName, AZ::SettingsRegistryInterface::Type, AZ::s64 value) override
            {
                m_result = m_result && WriteName(valueName) && m_writer.Int64(value);
            }

            void Visit(AZStd::string_view, AZStd::string_view valueName, AZ::SettingsRegistryInterface::Type, AZ::u64 value) override
            {
                m_result = m_result && WriteName(valueName) && m_writer.Uint64(value);
            }

            void Visit(AZStd::string_view, AZStd::string_view valueName, AZ::SettingsRegistryInterface::Type, double value) override
            {
                m_result = m_result && WriteName(valueName) && m_writer.Double(value);
            }

            void Visit(AZStd::string_view, AZStd::string_view valueName, AZ::SettingsRegistryInterface::Type, AZStd::string_view value) override
            {
                m_result = m_result && WriteName(valueName) && m_writer.String(value.data(), aznumeric_caster(value.size()));
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
        AZ::IO::PathView candidateView{ candidatePath, AZ::IO::PosixPathSeparator };
        AZ::IO::PathView inputView{ inputPath, AZ::IO::PosixPathSeparator };
        return inputView.empty() || candidateView.IsRelativeTo(inputView) || inputView.IsRelativeTo(candidateView);
    }

}
