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
    AZ::IO::FixedMaxPath GetExecutableDirectory()
    {
        AZStd::fixed_string<AZ::IO::MaxPathLength> value;

        // Binary folder
        AZ::Utils::ExecutablePathResult pathResult = Utils::GetExecutableDirectory(value.data(), value.capacity());
        // Update the size value of the executable directory fixed string to correctly be the length of the null-terminated string stored within it
        value.resize_no_construct(AZStd::char_traits<char>::length(value.data()));

        return value;
    }
}

namespace AZ::SettingsRegistryMergeUtils
{
    AZ::IO::FixedMaxPath GetAppRoot(SettingsRegistryInterface* settingsRegistry)
    {
        constexpr size_t MaxAppRootPathsToScan = 16;
        AZStd::fixed_vector<AZStd::string_view, MaxAppRootPathsToScan> appRootScanPaths;

        // Check the commandLine for --app-root parameter
        // If it exist use that instead
        auto appRootOverrideKey = AZ::SettingsRegistryInterface::FixedValueString::format("%s/app-root", AZ::SettingsRegistryMergeUtils::CommandLineSwitchRootKey);
        if (settingsRegistry)
        {
            AZ::IO::FixedMaxPath appRootOverridePath;
            if (settingsRegistry->Get(appRootOverridePath.Native(), appRootOverrideKey))
            {
                return appRootOverridePath;
            }
        }

        AZStd::optional<AZStd::fixed_string<AZ::IO::MaxPathLength>> appRootPath = Utils::GetDefaultAppRootPath();
        if (appRootPath.has_value())
        {
            appRootScanPaths.push_back(*appRootPath);
        }
        AZStd::fixed_string<AZ::IO::MaxPathLength> executableDir;
        AZ::Utils::ExecutablePathResult pathResult = Utils::GetExecutableDirectory(executableDir.data(), executableDir.capacity());
        if (pathResult == Utils::ExecutablePathResult::Success)
        {
            // Update the size value of the executable directory fixed string to correctly be the length of the null-terminated string stored within it
            executableDir.resize_no_construct(AZStd::char_traits<char>::length(executableDir.data()));
            if (auto foundIt = AZStd::find(appRootScanPaths.begin(), appRootScanPaths.end(), executableDir);
                foundIt == appRootScanPaths.end())
            {
                appRootScanPaths.push_back(executableDir);
            }
        }
        for (AZStd::string_view appRootScanPath : appRootScanPaths)
        {
            AZ::IO::FixedMaxPath appRootCandidate{ appRootScanPath };

            // Search for the application root
            bool rootPathVisited = false;
            do
            {
                if (AZ::IO::SystemFile::Exists((appRootCandidate / "bootstrap.cfg").c_str()))
                {
                    return appRootCandidate;
                }

                // Validate that the parent directory isn't itself, that would imply
                // that it is the root path
                AZ::IO::PathView parentPath = appRootCandidate.ParentPath();
                rootPathVisited = appRootCandidate == parentPath;
                // Recurse upwards one directory
                appRootCandidate = AZStd::move(parentPath);

            } while (!rootPathVisited);
            // Note for posix filesystems the parent directory of '/' is '/' and for windows
            // the parent directory of 'C:\\' is 'C:\\'
        }
        return {};
    }

    AZStd::string_view ConfigParserSettings::DefaultCommentPrefixFilter(AZStd::string_view line)
    {
        constexpr AZStd::string_view commentPrefixes = ";#";
        for (char commentPrefix : commentPrefixes)
        {
            if (line.starts_with(commentPrefix))
            {
                return {};
            }
        }

        return line;
    }

    AZStd::string_view ConfigParserSettings::DefaultSectionHeaderFilter(AZStd::string_view line)
    {
        AZStd::string_view sectionName;
        constexpr char sectionHeaderStart= '[';
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
        auto configPath = GetAppRoot(&registry) / filePath;
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
            if (line.starts_with("--") || line.starts_with(';') || line.starts_with('#'))
            {
                return {};
            }
            return line;
        };
        parserSettings.m_registryRootPointerPath = BootstrapSettingsRootKey;
        MergeSettingsToRegistry_ConfigFile(registry, "bootstrap.cfg", parserSettings);
    }

    void MergeSettingsToRegistry_AddRuntimeFilePaths(SettingsRegistryInterface& registry)
    {
        // Binary folder
        AZ::IO::FixedMaxPath path = Internal::GetExecutableDirectory();
        registry.Set(FilePathKey_BinaryFolder, path.LexicallyNormal().Native());

        // Engine root folder - corresponds to the @engroot@ and @devroot@ aliases
        AZ::IO::FixedMaxPath appRoot = GetAppRoot(&registry);
        registry.Set(FilePathKey_EngineRootFolder, appRoot.LexicallyNormal().Native());

        constexpr size_t bufferSize = 64;
        auto buffer = AZStd::fixed_string<bufferSize>::format("%s/sys_game_folder", BootstrapSettingsRootKey);

        AZStd::string_view projectPath(buffer);
        SettingsRegistryInterface::FixedValueString projectPathValue;
        if (registry.Get(projectPathValue, projectPath))
        {
            // Cache folder
            // Get the name of the asset platform assigned by the bootstrap. First check for platform version such as "windows_assets"
            // and if that's missing just get "asset".
            constexpr char platformName[] = AZ_TRAIT_OS_PLATFORM_CODENAME_LOWER;

            SettingsRegistryInterface::FixedValueString assetPlatform;
            buffer = AZStd::fixed_string<bufferSize>::format("%s/%s_assets", BootstrapSettingsRootKey, platformName);
            AZStd::string_view assetPlatformatPath(buffer);
            if (!registry.Get(assetPlatform, assetPlatformatPath))
            {
                buffer = AZStd::fixed_string<bufferSize>::format("%s/assets", BootstrapSettingsRootKey);
                assetPlatformatPath = AZStd::string_view(buffer);
                if (!registry.Get(assetPlatform, assetPlatformatPath))
                {
                    return;
                }
            }

            // Source game folder - corresponds to the @devassets@ alias
            path = appRoot / projectPathValue;
            AZ_Warning("SettingsRegistryMergeUtils", AZ::IO::SystemFile::Exists(path.c_str()),
                R"(Project Path "%s" does not exist. Is the "sys_game_folder" entry set to valid project folder?)"
                , path.c_str());
            registry.Set(FilePathKey_SourceGameFolder, path.LexicallyNormal().Native());

            // Source game name - The filename of the project directory and the name of the project
            registry.Set(FilePathKey_SourceGameName, path.Filename().Native());

            // Cache root folder - corresponds to the @root@ alias
#if AZ_TRAIT_USE_ASSET_CACHE_FOLDER
            path = appRoot / "Cache" / projectPathValue / assetPlatform;
#else
            // Use the Engine Root/App Root as the Asset root directory in this case
            path = appRoot;
#endif
            registry.Set(FilePathKey_CacheRootFolder, path.LexicallyNormal().Native());

            // check for a default write storage path, fall back to the cache root if not
            AZStd::optional<AZ::IO::FixedMaxPathString> devWriteStorage = Utils::GetDevWriteStoragePath();
            registry.Set(FilePathKey_DevWriteStorage,
                devWriteStorage.has_value() ?
                    devWriteStorage.value() :
                    path.LexicallyNormal().Native());

            // Cache game folder - corresponds to the @assets@ alias
            AZStd::to_lower(projectPathValue.begin(), projectPathValue.end());
            path /= projectPathValue;
            registry.Set(FilePathKey_CacheGameFolder, path.LexicallyNormal().Native());
        }
        else
        {
            AZ_TracePrintf("SettingsRegistryMergeUtils",
                R"(Current Project game folder "%.*s" isn't set in the Settings Registry. Project-related filepaths will not be set)" "\n",
                aznumeric_cast<int>(projectPath.size()), projectPath.data());
        }
    }

    void MergeSettingsToRegistry_TargetBuildDependencyRegistry(SettingsRegistryInterface& registry, const AZStd::string_view platform,
        const SettingsRegistryInterface::Specializations& specializations, AZStd::vector<char>* scratchBuffer)
    {
        AZ::IO::FixedMaxPath mergePath = Internal::GetExecutableDirectory();
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
        if (registry.Get(sourceGamePath, FilePathKey_SourceGameFolder))
        {
            AZ::IO::FixedMaxPath mergePath{ sourceGamePath };
            mergePath /= SettingsRegistryInterface::RegistryFolder;
            registry.MergeSettingsFolder(mergePath.Native(), specializations, platform, "", scratchBuffer);
        }
    }

    void MergeSettingsToRegistry_DevRegistry(SettingsRegistryInterface& registry, const AZStd::string_view platform,
        const SettingsRegistryInterface::Specializations& specializations, AZStd::vector<char>* scratchBuffer)
    {
        // Unlike other paths, the path can't be overwritten by the dev settings because that would create a circular dependency.
        auto mergePath = GetAppRoot(&registry);
        mergePath /= SettingsRegistryInterface::DevUserRegistryFolder;
        registry.MergeSettingsFolder(mergePath.Native(), specializations, platform, "", scratchBuffer);
    }

    void MergeSettingsToRegistry_CommandLine(SettingsRegistryInterface& registry, const AZ::CommandLine& commandLine, bool executeCommands)
    {
        const size_t regsetSwitchValues = commandLine.GetNumSwitchValues("regset");
        for (size_t regsetIndex = 0; regsetIndex < regsetSwitchValues; ++regsetIndex)
        {
            AZStd::string_view regsetValue = commandLine.GetSwitchValue("regset", regsetIndex);
            if (!registry.MergeCommandLineArgument(regsetValue, AZStd::string_view{}))
            {
                AZ_Warning("SettingsRegistryMergeUtils", false, "Unable to parse argument for --regset with value of %.*s.",
                    aznumeric_cast<int>(regsetValue.size()), regsetValue.data());
                continue;
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

            const size_t regdumpallSwitchValues = commandLine.GetNumSwitchValues("regdumpall");
            if (regdumpallSwitchValues > 0)
            {
                AZStd::string_view regdumpallValue = commandLine.GetSwitchValue("regdumpall", 0);
                if (regdumpallValue.empty())
                {
                    AZ_Warning("SettingsRegistryMergeUtils", false, "Unable to parse argument for --regdumpall with value of %.*s.",
                        aznumeric_cast<int>(regdumpallValue.size()), regdumpallValue.data());
                }

                DumperSettings dumperSettings{ prettifyOutput };
                AZ::IO::StdoutStream outputStream;
                DumpSettingsRegistryToStream(registry, "", outputStream, dumperSettings);
            }
        }
    }

    void MergeSettingsToRegistry_StoreCommandLine(SettingsRegistryInterface& registry, const AZ::CommandLine& commandLine)
    {
        // Clear out any existing CommandLine settings
        registry.Remove(CommandLineRootKey);
        // Add the positional arguments into the Settings Registry
        AZ::SettingsRegistryInterface::FixedValueString miscValueKey{ CommandLineMiscValuesRootKey };
        size_t miscKeyRootSize = miscValueKey.size();
        for (size_t miscIndex = 0; miscIndex < commandLine.GetNumMiscValues(); ++miscIndex)
        {
            // Push an array for each positional entry
            miscValueKey += AZ::SettingsRegistryInterface::FixedValueString::format("/%zu", miscIndex);
            registry.Set(miscValueKey, commandLine.GetMiscValue(miscIndex));
            miscValueKey.resize(miscKeyRootSize);
        }
        // Add the option arguments into the SettingsRegistry
        for (const auto& [commandOption, value] : commandLine.GetSwitchList())
        {
            AZ::SettingsRegistryInterface::FixedValueString switchKey{ CommandLineSwitchRootKey };
            switchKey += '/';
            switchKey += commandOption;
            size_t switchKeyRootSize = switchKey.size();
            // Associate an empty array with the commandOption by default
            rapidjson::Document commandSwitchDocument;
            rapidjson::Pointer pointer(switchKey.c_str(), switchKey.length());
            pointer.Set(commandSwitchDocument, rapidjson::Value(rapidjson::kArrayType));;
            rapidjson::StringBuffer documentBuffer;
            rapidjson::Writer documentWriter(documentBuffer);
            commandSwitchDocument.Accept(documentWriter);
            registry.MergeSettings(AZStd::string_view{ documentBuffer.GetString(), documentBuffer.GetSize() },
                AZ::SettingsRegistryInterface::Format::JsonMergePatch);

            for (size_t switchIndex = 0; switchIndex < value.size(); ++switchIndex)
            {
                // Push an array for each positional entry
                switchKey += AZ::SettingsRegistryInterface::FixedValueString::format("/%zu", switchIndex);
                registry.Set(switchKey, value[switchIndex]);
                switchKey.resize(switchKeyRootSize);
            }
        }
    }

    bool DumpSettingsRegistryToStream(SettingsRegistryInterface& registry, AZStd::string_view key,
        AZ::IO::GenericStream& stream, const DumperSettings& dumperSettings)
    {
        struct SettingsExportVisitor
            : SettingsRegistryInterface::Visitor
        {
            using SettingsWriter = AZStd::variant<
                rapidjson::PrettyWriter<AZ::IO::RapidJSONStreamWriter>,
                rapidjson::Writer<AZ::IO::RapidJSONStreamWriter>>;
            SettingsWriter m_writer;
            AZ::IO::RapidJSONStreamWriter m_rapidJsonStream;
            DumperSettings m_dumperSettings;
            AZStd::stack<bool> m_includeNameStack;
            bool m_includeName{};
            bool m_result{ true };

            SettingsExportVisitor(AZ::IO::GenericStream& stream, const DumperSettings& dumperSettings)
                : m_rapidJsonStream{ &stream }
                , m_dumperSettings{ dumperSettings }
            {
                if (m_dumperSettings.m_prettifyOutput)
                {
                    m_writer.emplace<rapidjson::PrettyWriter<AZ::IO::RapidJSONStreamWriter>>(m_rapidJsonStream);
                }
                else
                {
                    m_writer.emplace<rapidjson::Writer<AZ::IO::RapidJSONStreamWriter>>(m_rapidJsonStream);
                }
            }
            void WriteName(AZStd::string_view name)
            {
                if (m_includeName)
                {
                    AZStd::visit([&name](auto&& writer)
                    {
                        writer.Key(name.data(), aznumeric_caster(name.size()));
                    }, m_writer);
                }
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
                    AZ_Assert(type == AZ::SettingsRegistryInterface::Type::Object || type == AZ::SettingsRegistryInterface::Type::Array,
                        "Unexpected type visited: %i.", type);
                    WriteName(valueName);
                    if (type == AZ::SettingsRegistryInterface::Type::Object)
                    {
                        auto StartObject = [](auto&& writer)
                        {
                            return writer.StartObject();
                        };
                        m_result = m_result && AZStd::visit(StartObject, m_writer);
                        m_includeNameStack.push(true);
                        m_includeName = true;
                    }
                    else
                    {
                        auto StartArray = [](auto&& writer)
                        {
                            return writer.StartArray();
                        };
                        m_result = m_result && AZStd::visit(StartArray, m_writer);
                        m_includeNameStack.push(false);
                        m_includeName = false;
                    }
                }
                else if (action == AZ::SettingsRegistryInterface::VisitAction::End)
                {
                    if (type == AZ::SettingsRegistryInterface::Type::Object)
                    {
                        auto EndObject = [](auto&& writer)
                        {
                            return writer.EndObject();
                        };
                        m_result = m_result && AZStd::visit(EndObject, m_writer);
                    }
                    else
                    {
                        auto EndArray = [](auto&& writer)
                        {
                            return writer.EndArray();
                        };
                        m_result = m_result && AZStd::visit(EndArray, m_writer);
                    }
                    AZ_Assert(!m_includeNameStack.empty(), "Attempting to close a json array or object that wasn't started.");
                    m_includeNameStack.pop();
                    m_includeName = !m_includeNameStack.empty() ? m_includeNameStack.top() : true;
                }
                else if (type == AZ::SettingsRegistryInterface::Type::Null)
                {
                    WriteName(valueName);
                    auto WriteNull = [](auto&& writer)
                    {
                        return writer.Null();
                    };
                    m_result = m_result && AZStd::visit(WriteNull, m_writer);
                }

                return m_result ?
                    AZ::SettingsRegistryInterface::VisitResponse::Continue :
                    AZ::SettingsRegistryInterface::VisitResponse::Done;
            }

            void Visit(AZStd::string_view, AZStd::string_view valueName, AZ::SettingsRegistryInterface::Type, bool value)
            {
                WriteName(valueName);
                auto WriteBool = [value](auto&& writer)
                {
                    return writer.Bool(value);
                };
                m_result = m_result && AZStd::visit(WriteBool, m_writer);
            }

            void Visit(AZStd::string_view, AZStd::string_view valueName, AZ::SettingsRegistryInterface::Type, AZ::s64 value)
            {
                WriteName(valueName);
                auto WriteInt64 = [value](auto&& writer)
                {
                    return writer.Int64(value);
                };
                m_result = m_result && AZStd::visit(WriteInt64, m_writer);
            }

            void Visit(AZStd::string_view, AZStd::string_view valueName, AZ::SettingsRegistryInterface::Type, double value)
            {
                WriteName(valueName);
                auto WriteDouble = [value](auto&& writer)
                {
                    return writer.Double(value);
                };
                m_result = m_result && AZStd::visit(WriteDouble, m_writer);
            }

            void Visit(AZStd::string_view, AZStd::string_view valueName, AZ::SettingsRegistryInterface::Type, AZStd::string_view value)
            {
                WriteName(valueName);
                auto WriteString = [&value](auto&& writer)
                {
                    return writer.String(value.data(), aznumeric_caster(value.size()));
                };
                m_result = m_result && AZStd::visit(WriteString, m_writer);
            }

            bool Finalize()
            {
                if (!m_includeNameStack.empty())
                {
                    AZ_Assert(false, "m_includeNameStack is expected to be empty. This means that there was an object or array what wasn't closed.");
                    return false;
                }
                return m_result;
            }
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
