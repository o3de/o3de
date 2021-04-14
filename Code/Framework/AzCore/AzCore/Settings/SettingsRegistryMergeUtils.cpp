/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include <AzCore/IO/GenericStreams.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/IO/TextStreamWriters.h>
#include <AzCore/JSON/document.h>
#include <AzCore/JSON/pointer.h>
#include <AzCore/JSON/prettywriter.h>
#include <AzCore/JSON/writer.h>
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
        SettingsRegistryInterface& settingsRegistry, const AZ::IO::FixedMaxPath& projectPath)
    {
        // projectPath needs to be an absolute path here.
        using namespace AZ::SettingsRegistryMergeUtils;
        bool projectJsonMerged = false;
        auto projectJsonPath = projectPath / "project.json";
        if (AZ::IO::SystemFile::Exists(projectJsonPath.c_str()))
        {
            projectJsonMerged = settingsRegistry.MergeSettingsFile(
                projectJsonPath.Native(), AZ::SettingsRegistryInterface::Format::JsonMergePatch, ProjectSettingsRootKey);
        }

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
                AZ::SettingsRegistryInterface::FixedValueString m_moniker;
            };

            struct EnginePathsVisitor : public AZ::SettingsRegistryInterface::Visitor
            {
                void Visit(
                    [[maybe_unused]] AZStd::string_view path, [[maybe_unused]] AZStd::string_view valueName,
                    [[maybe_unused]] AZ::SettingsRegistryInterface::Type type, AZStd::string_view value) override
                {
                    m_enginePaths.emplace_back(EngineInfo{AZ::IO::FixedMaxPath{value}.LexicallyNormal(), {}});
                }

                AZStd::vector<EngineInfo> m_enginePaths{};
            };

            EnginePathsVisitor pathVisitor;
            if (manifestLoaded)
            {
                auto enginePathsKey = AZ::SettingsRegistryInterface::FixedValueString::format("%s/engines", EngineManifestRootKey);
                settingsRegistry.Visit(pathVisitor, enginePathsKey);
            }

            const auto engineMonikerKey = AZ::SettingsRegistryInterface::FixedValueString::format("%s/engine_name", EngineSettingsRootKey);

            for (EngineInfo& engineInfo : pathVisitor.m_enginePaths)
            {
                AZ::IO::FixedMaxPath engineSettingsPath{engineInfo.m_path};
                engineSettingsPath /= "engine.json";

                if (AZ::IO::SystemFile::Exists(engineSettingsPath.c_str()))
                {
                    if (settingsRegistry.MergeSettingsFile(
                            engineSettingsPath.Native(), AZ::SettingsRegistryInterface::Format::JsonMergePatch, EngineSettingsRootKey))
                    {
                        settingsRegistry.Get(engineInfo.m_moniker, engineMonikerKey);
                    }
                }

                auto engineMoniker = Internal::GetEngineMonikerForProject(settingsRegistry, engineInfo.m_path / projectPath);
                if (!engineMoniker.empty() && engineMoniker == engineInfo.m_moniker)
                {
                    engineRoot = engineInfo.m_path;
                    break;
                }
            }
        }

        return engineRoot;
    }

    AZ::IO::FixedMaxPath ScanUpRootLocator(AZStd::string_view rootFileToLocate)
    {

        AZStd::fixed_string<AZ::IO::MaxPathLength> executableDir;
        if (AZ::Utils::GetExecutableDirectory(executableDir.data(), executableDir.capacity()) == Utils::ExecutablePathResult::Success)
        {
            // Update the size value of the executable directory fixed string to correctly be the length of the null-terminated string
            // stored within it
            executableDir.resize_no_construct(AZStd::char_traits<char>::length(executableDir.data()));
        }

        AZ::IO::FixedMaxPath engineRootCandidate{ executableDir };

        bool rootPathVisited = false;
        do
        {
            if (AZ::IO::SystemFile::Exists((engineRootCandidate / rootFileToLocate).c_str()))
            {
                return engineRootCandidate;
            }

            // Note for posix filesystems the parent directory of '/' is '/' and for windows
            // the parent directory of 'C:\\' is 'C:\\'

            // Validate that the parent directory isn't itself, that would imply
            // that it is the filesystem root path
            AZ::IO::PathView parentPath = engineRootCandidate.ParentPath();
            rootPathVisited = (engineRootCandidate == parentPath);
            // Recurse upwards one directory
            engineRootCandidate = AZStd::move(parentPath);

        } while (!rootPathVisited);

        return {};
    }

} // namespace AZ::Internal

namespace AZ::SettingsRegistryMergeUtils
{
    AZ::IO::FixedMaxPath FindEngineRoot(SettingsRegistryInterface& settingsRegistry)
    {
        AZ::IO::FixedMaxPath engineRoot;

        // This is the 'external' engine root key, as in passed from command-line or .setreg files.
        auto engineRootKey = SettingsRegistryInterface::FixedValueString::format("%s/engine_path", BootstrapSettingsRootKey);
        if (settingsRegistry.Get(engineRoot.Native(), engineRootKey); !engineRoot.empty())
        {
            return engineRoot;
        }

        // We can scan up from exe directory to find engine.json, use that for engine root if it exists.
        if (engineRoot = Internal::ScanUpRootLocator("engine.json"); !engineRoot.empty())
        {
            settingsRegistry.Set(engineRootKey, engineRoot.c_str());
            return engineRoot;
        }

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

        return {};
    }

    AZ::IO::FixedMaxPath FindProjectRoot(SettingsRegistryInterface& settingsRegistry)
    {
        AZ::IO::FixedMaxPath projectRoot;
        // This is the 'external' project root key, as in passed from command-line or .setreg files.
        auto projectRootKey = SettingsRegistryInterface::FixedValueString::format("%s/project_path", BootstrapSettingsRootKey);
        if (settingsRegistry.Get(projectRoot.Native(), projectRootKey))
        {
            return projectRoot;
        }

        if (projectRoot = Internal::ScanUpRootLocator("project.json"); !projectRoot.empty())
        {
            settingsRegistry.Set(projectRootKey, projectRoot.c_str());
            return projectRoot;
        }

        return {};
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

    // Encodes a key, value delimited line such that the entire "key" can be stored as a single
    // JSON Pointer key by escaping the tilde(~) and forward slash(/)
    template<size_t BufferSize>
    static AZStd::fixed_string<BufferSize> EncodeLineForJsonPointer(AZStd::string_view token,
        const AZ::SettingsRegistryInterface::CommandLineArgumentSettings::DelimiterFunc& delimiterFunc)
    {
        if (!delimiterFunc)
        {
            // Since the delimiter function is not valid, return the token unchanged
            return AZStd::fixed_string<BufferSize>{ token };
        }
        // Iterate over the line and escape the '~' and '/' values
        AZStd::fixed_string<BufferSize> encodedToken;
        size_t chIndex = 0;
        for (; chIndex < token.size(); ++chIndex)
        {
            const char ch = token[chIndex];
            if (delimiterFunc(ch))
            {
                // If the delimiter is found, this indicates that the end of the key has been found
                break;
            }
            switch (ch)
            {
            case '~':
                encodedToken += "~0";
                break;
            case '/':
                encodedToken += "~1";
                break;
            default:
                encodedToken += ch;
            }
        }

        // Copy over the rest of the post delimited line to the encoded token
        encodedToken.append(token.data() + chIndex, token.data() + token.size());
        return encodedToken;
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
        IO::SystemFile configFile;
        if (!configFile.Open(configPath.c_str(), IO::SystemFile::OpenMode::SF_OPEN_READ_ONLY))
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

                // Check if the "key" portion of the line has '~' or '/' as the SettingsRegistry uses JSON Pointer
                // to set the "value" portion. Those characters need to be escaped with ~0 and ~1 respectively
                // to allow them to be embedded in a single json key
                // Iterate over the line and escape the '~' and '/' values
                AZStd::fixed_string<ConfigBufferMaxSize> escapedLine = EncodeLineForJsonPointer<ConfigBufferMaxSize>(line,
                    configParserSettings.m_commandLineSettings.m_delimiterFunc);

                registry.MergeCommandLineArgument(escapedLine, currentJsonPointerPath, configParserSettings.m_commandLineSettings);

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
                    R"("%.*s")" "\n", configFile.Name(), configBuffer.max_size(),
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

    void MergeSettingsToRegistry_Bootstrap(SettingsRegistryInterface& registry)
    {
        ConfigParserSettings parserSettings;
        parserSettings.m_commentPrefixFunc = [](AZStd::string_view line) -> AZStd::string_view
        {
            constexpr AZStd::string_view commentPrefixes[]{ "--", ";","#" };
            for (AZStd::string_view commentPrefix : commentPrefixes)
            {
                if (size_t commentOffset = line.find(commentPrefix); commentOffset != AZStd::string_view::npos)
                {
                    return line.substr(0, commentOffset);
                }
            }
            return line;
        };
        parserSettings.m_registryRootPointerPath = BootstrapSettingsRootKey;
        MergeSettingsToRegistry_ConfigFile(registry, "bootstrap.cfg", parserSettings);
    }

    void MergeSettingsToRegistry_AddRuntimeFilePaths(SettingsRegistryInterface& registry)
    {
        // Binary folder
        AZ::IO::FixedMaxPath path = AZ::Utils::GetExecutableDirectory();
        registry.Set(FilePathKey_BinaryFolder, path.LexicallyNormal().Native());

        // Engine root folder - corresponds to the @engroot@ and @devroot@ aliases
        AZ::IO::FixedMaxPath engineRoot = FindEngineRoot(registry);
        registry.Set(FilePathKey_EngineRootFolder, engineRoot.LexicallyNormal().Native());

        constexpr size_t bufferSize = 64;
        auto buffer = AZStd::fixed_string<bufferSize>::format("%s/project_path", BootstrapSettingsRootKey);

        AZ::SettingsRegistryInterface::FixedValueString projectPathKey(buffer);
        SettingsRegistryInterface::FixedValueString projectPathValue;
        if (registry.Get(projectPathValue, projectPathKey))
        {
            // Cache folder
            // Get the name of the asset platform assigned by the bootstrap. First check for platform version such as "windows_assets"
            // and if that's missing just get "assets".
            constexpr char platformName[] = AZ_TRAIT_OS_PLATFORM_CODENAME_LOWER;

            SettingsRegistryInterface::FixedValueString assetPlatform;
            buffer = AZStd::fixed_string<bufferSize>::format("%s/%s_assets", BootstrapSettingsRootKey, platformName);
            AZStd::string_view assetPlatformKey(buffer);
            if (!registry.Get(assetPlatform, assetPlatformKey))
            {
                buffer = AZStd::fixed_string<bufferSize>::format("%s/assets", BootstrapSettingsRootKey);
                assetPlatformKey = AZStd::string_view(buffer);
                registry.Get(assetPlatform, assetPlatformKey);
            }

            // Project path - corresponds to the @devassets@ alias
            // NOTE: Here we append to engineRoot, but if projectPathValue is absolute then engineRoot is discarded.
            path = engineRoot / projectPathValue;

            AZ_Warning("SettingsRegistryMergeUtils", AZ::IO::SystemFile::Exists(path.c_str()),
                R"(Project path "%s" does not exist. Is the "%.*s" registry setting set to valid absolute path?)"
                , path.c_str(), aznumeric_cast<int>(projectPathKey.size()), projectPathKey.data());

            AZ::IO::FixedMaxPath normalizedProjectPath = path.LexicallyNormal();
            registry.Set(FilePathKey_ProjectPath, normalizedProjectPath.Native());

            // Add an alias to the project "user" directory
            AZ::IO::FixedMaxPath projectUserPath = (normalizedProjectPath / "user").LexicallyNormal();
            registry.Set(FilePathKey_ProjectUserPath, projectUserPath.Native());
            // check for a default write storage path, fall back to the project's user/ directory if not
            AZStd::optional<AZ::IO::FixedMaxPathString> devWriteStorage = Utils::GetDevWriteStoragePath();
            registry.Set(FilePathKey_DevWriteStorage, devWriteStorage.has_value()
                ? devWriteStorage.value()
                : projectUserPath.Native());

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
                buffer = AZStd::fixed_string<bufferSize>::format("%s/project_cache_path", BootstrapSettingsRootKey);
                AZStd::string_view projectCacheRootOverrideKey(buffer);
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
                    // Cache: root - same as the @root@ alias, this is the starting path for cache files.
                    path = normalizedProjectPath / "Cache";
                    registry.Set(FilePathKey_CacheProjectRootFolder, path.LexicallyNormal().Native());
                    path /= assetPlatform;
                    registry.Set(FilePathKey_CacheRootFolder, path.LexicallyNormal().Native());
                }
            }
        }
        else
        {
            AZ_TracePrintf("SettingsRegistryMergeUtils",
                R"(Project path isn't set in the Settings Registry at "%.*s". Project-related filepaths will not be set)" "\n",
                aznumeric_cast<int>(projectPathKey.size()), projectPathKey.data());
        }

#if !AZ_TRAIT_USE_ASSET_CACHE_FOLDER
        // Setup the cache and user paths for Platforms where the Asset Cache Folder isn't used
        path = engineRoot;
        registry.Set(FilePathKey_CacheProjectRootFolder, path.LexicallyNormal().Native());
        registry.Set(FilePathKey_CacheRootFolder, path.LexicallyNormal().Native());
        registry.Set(FilePathKey_DevWriteStorage, path.LexicallyNormal().Native());
        registry.Set(FilePathKey_ProjectUserPath, (path / "user").LexicallyNormal().Native());
#endif // AZ_TRAIT_USE_ASSET_CACHE_FOLDER
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
            mergePath /= SettingsRegistryInterface::RegistryFolder;
            registry.MergeSettingsFolder(mergePath.Native(), specializations, platform, "", scratchBuffer);
        }
    }

    void MergeSettingsToRegistry_EngineRegistry(SettingsRegistryInterface& registry, const AZStd::string_view platform,
        const SettingsRegistryInterface::Specializations& specializations, AZStd::vector<char>* scratchBuffer)
    {
        AZ::SettingsRegistryInterface::FixedValueString engineRootPath;
        if (registry.Get(engineRootPath, FilePathKey_EngineRootFolder))
        {
            AZ::IO::FixedMaxPath mergePath{ AZStd::move(engineRootPath) };
            mergePath /= "Engine";
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

    void MergeSettingsToRegistry_CommandLine(SettingsRegistryInterface& registry, const AZ::CommandLine& commandLine, bool executeCommands)
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
            if (commandArgument.m_option == "regremove")
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
    }

    bool GetCommandLineFromRegistry(SettingsRegistryInterface& registry, AZ::CommandLine& commandLine)
    {
        struct CommandLineVisitor
            : AZ::SettingsRegistryInterface::Visitor
        {
            void Visit(AZStd::string_view, AZStd::string_view valueName, AZ::SettingsRegistryInterface::Type
                , AZStd::string_view value) override
            {
                if (valueName == "Option" && !value.empty())
                {
                    m_arguments.push_back(AZStd::string::format("--%.*s", aznumeric_cast<int>(value.size()), value.data()));
                }
                else if (valueName == "Value" && !value.empty())
                {
                    m_arguments.push_back(value);
                }
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
                AZ::SettingsRegistryInterface::Type type)
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

            void Visit(AZStd::string_view, AZStd::string_view valueName, AZ::SettingsRegistryInterface::Type, bool value)
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

            void Visit(AZStd::string_view, AZStd::string_view valueName, AZ::SettingsRegistryInterface::Type, double value)
            {
                m_result = m_result && WriteName(valueName) && m_writer.Double(value);
            }

            void Visit(AZStd::string_view, AZStd::string_view valueName, AZ::SettingsRegistryInterface::Type, AZStd::string_view value)
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
}
