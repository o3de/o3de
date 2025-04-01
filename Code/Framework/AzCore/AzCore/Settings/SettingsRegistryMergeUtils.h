/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/IO/Path/Path_fwd.h>
#include <AzCore/Memory/OSAllocator.h>
#include <AzCore/Settings/SettingsRegistry.h>
#include <AzCore/Settings/CommandLine.h>

namespace AZ::IO
{
    class GenericStream;
}

namespace AZ::SettingsRegistryMergeUtils
{
    inline constexpr const char* OrganizationRootKey = "/Amazon";
    inline constexpr const char* BuildTargetNameKey = "/O3DE/Settings/BuildTargetName";
    inline constexpr const char* SpecializationsRootKey = "/O3DE/Settings/Specializations";
    inline constexpr const char* BootstrapSettingsRootKey = "/Amazon/AzCore/Bootstrap";
    inline constexpr const char* FilePathsRootKey = "/O3DE/Runtime/FilePaths";
    inline constexpr const char* FilePathKey_BinaryFolder = "/O3DE/Runtime/FilePaths/BinaryFolder";
    inline constexpr const char* FilePathKey_EngineRootFolder = "/O3DE/Runtime/FilePaths/EngineRootFolder";
    inline constexpr const char* FilePathKey_InstalledBinaryFolder = "/O3DE/Runtime/FilePaths/InstalledBinariesFolder";

    //! Stores path to the O3DE Manifest root directory
    //! Defaults to ~/.o3de, where ~ represents the user's home directory
    inline constexpr const char* FilePathKey_O3deManifestRootFolder = "/O3DE/Runtime/FilePaths/O3deManifestRootFolder";

    //! Stores the absolute path to root of a project's cache.  No asset platform in this path, this is where the asset database file lives.
    //! i.e. <ProjectPath>/Cache
    inline constexpr const char* FilePathKey_CacheProjectRootFolder = "/O3DE/Runtime/FilePaths/CacheProjectRootFolder";

    //! Stores the absolute path to the cache root for an asset platform.  This is the @root@ alias.
    //! i.e. <ProjectPath>/Cache/<assetplatform>
    inline constexpr const char* FilePathKey_CacheRootFolder = "/O3DE/Runtime/FilePaths/CacheRootFolder";

    //! Stores the absolute path of the Game Project Directory
    inline constexpr const char* FilePathKey_ProjectPath = "/O3DE/Runtime/FilePaths/SourceProjectPath";

    //! Store the absolute path to the Projects "user" directory, which is a transient directory where per user
    //! project settings can be stored
    inline constexpr const char* FilePathKey_ProjectUserPath = "/O3DE/Runtime/FilePaths/SourceProjectUserPath";

    //! Store the absolute path to the Projects "log" directory, which is a transient directory where per user
    //! logs can be stored. By default this would be on "{FilePathKey_ProjectUserPath}/log"
    inline constexpr const char* FilePathKey_ProjectLogPath = "/O3DE/Runtime/FilePaths/SourceProjectLogPath";

    //! User facing key which represents the root of a project cmake build tree. i.e the ${CMAKE_BINARY_DIR}
    //! A relative path is taking relative to the *project* root, NOT *engine* root.
    inline constexpr AZStd::string_view ProjectBuildPath = "/Amazon/Project/Settings/Build/project_build_path";
    //! In-Memory only key which stores an absolute path to the project build directory
    inline constexpr AZStd::string_view FilePathKey_ProjectBuildPath = "/O3DE/Runtime/FilePaths/ProjectBuildPath";
    //! In-Memory only key which stores the configuration directory containing the built binaries
    inline constexpr AZStd::string_view FilePathKey_ProjectConfigurationBinPath = "/O3DE/Runtime/FilePaths/ProjectConfigurationBinPath";

    //! Development write storage path may be considered temporary or cache storage on some platforms
    inline constexpr const char* FilePathKey_DevWriteStorage = "/O3DE/Runtime/FilePaths/DevWriteStorage";

    //! Stores error text regarding engine boot sequence when engine and project roots cannot be determined
    inline constexpr const char* FilePathKey_ErrorText = "/O3DE/Runtime/FilePaths/ErrorText";

    //! Root key for where command line are stored at within the settings registry
    inline constexpr const char* CommandLineRootKey = "/O3DE/Runtime/CommandLine";
    //! Key set to trigger a notification that the CommandLine has been stored within the settings registry
    //! The value of the key has no meaning. Notification Handlers only need to check if the key was supplied
    inline constexpr const char* CommandLineValueChangedKey = "/O3DE/Runtime/CommandLineChanged";

    //! Root key where raw project manifest (project.json) file is merged to settings registry
    inline constexpr const char* ProjectSettingsRootKey = "/O3DE/Runtime/Manifest/Project";

    //! Root key where raw o3de manifest (o3de_manifest.json) file is merged to settings registry
    inline constexpr const char* O3deManifestSettingsRootKey = "/O3DE/Runtime/Manifest/O3de_Manifest";

    //! Root key where raw engine manifest (engine.json) file is merged to settings registry
    inline constexpr const char* EngineSettingsRootKey = "/O3DE/Runtime/Manifest/Engine";

    //! Root Key used to store arrays of gem name identifier to gem path objects as read from the o3de manifest files
    //! Those files are the o3de_manifest.json, engine.json, project.json and gem.json for each registered gem
    //! This key is not persistent to the disk due to be under the path of /O3DE/Runtime
    inline constexpr const char* ManifestGemsRootKey = "/O3DE/Runtime/Manifest/Gems";


    //! Root json pointer path keys representing the identifier field for each of the o3de manifest objects
    inline constexpr AZStd::string_view EngineNameKey = "/engine_name";
    inline constexpr AZStd::string_view GemNameKey = "/gem_name";
    inline constexpr AZStd::string_view ProjectNameKey = "/project_name";
    inline constexpr AZStd::string_view O3DEManifestNameKey = "/o3de_manifest_name";

    //! Json pointer path to the "external_subdirectories" entry field with a manifest.json
    inline constexpr AZStd::string_view ExternalSubdirectoriesKey = "/external_subdirectories";

    //! Root key where settings pertaining to active gems are stored
    //! If a gem contains multiple targets module it will be stored underneath this key
    inline constexpr const char* ActiveGemsRootKey = "/O3DE/Gems";

    //! Examines the Settings Registry for a "${BootstrapSettingsRootKey}/engine_path" key
    //! to use as an override for the Engine Root.
    //! Otherwise a directory walk upwards from the executable directory is performed
    //! to find the engine.json file which will be used as the engine root
    //! If it's still not found, attempt to find the project (by similar means) then reconcile the
    //! engine root by inspecting project.json and the engine manifest file.
    AZ::IO::FixedMaxPath FindEngineRoot(SettingsRegistryInterface& settingsRegistry);

    //! The algorithm that is used to find the project root is as follows
    //! 1. The first time this function runs it performs an upward scan for a "project.json" file from
    //! the executable directory and stores that path into an internal key.
    //! In the same step it injects the path into the front of the command line parameters
    //! using the --regset="{BootstrapSettingsRootKey}/project_path=<path>" value
    //! 2. Next the "{BootstrapSettingsRootKey}/project_path" is checked to see if it has a project path set
    //!
    //! The order in which the project path settings are overridden proceeds in the following order
    //! 1. project_path set in a *.setreg/*.setregpatch file
    //! 2. project_path found by scanning upwards from the executable directory to the project.json path
    //! 3. project_path set on the Command line via either --regset="{BootstrapSettingsRootKey}/project_path=<path>"
    //!    or --project_path=<path>
    AZ::IO::FixedMaxPath FindProjectRoot(SettingsRegistryInterface& settingsRegistry);

    //! Query the specializations that will be used when loading the Settings Registry.
    //! The SpecializationsRootKey is visited to retrieve any specializations stored within that section of that registry
    void QuerySpecializationsFromRegistry(SettingsRegistryInterface& registry, SettingsRegistryInterface::Specializations& specializations);

    //! Adds value as a string under the Settings Registry `SpecializationsRootKey`
    //! The specializations can be queried using the QuerySpecializationsFromRegistry function above
    void MergeSettingsToRegistry_AddSpecialization(SettingsRegistryInterface& registry, AZStd::string_view value);

    //! Adds name of current build system target to the Settings Registry specialization section
    //! A build system target is the name used by the build system to build a particular executable or library
    void MergeSettingsToRegistry_AddBuildSystemTargetSpecialization(SettingsRegistryInterface& registry, AZStd::string_view targetName);

    //! Settings structure which is used to determine how to parse Windows INI style config file(.cfg, .ini, etc...)
    //! It supports being able to supply a custom comment filter and section header filter
    //! The names of section headers are appended to the root Json pointer path member to form new root paths
    struct ConfigParserSettings
    {
        // Filters out line which start with a prefix of ';' or '#'
        // If a line matches the comment filter and empty view is returned
        // Otherwise the line is returned as is
        static AZStd::string_view DefaultCommentPrefixFilter(AZStd::string_view line);

        /* Matches a section header in the regular expression form of
        * \[(?P<header>[^]]+)\] # '[' followed by 1 or more non-']' characters followed by a ']'
        * If the line matches section header format, the section name portion of that line
        * is returned in the output parameter otherwise an empty view is returned
        */
        static AZStd::string_view DefaultSectionHeaderFilter(AZStd::string_view line);

        //! Callback function that is invoked when a line is read.
        //! returns a substr of the line which contains the non-commented portion of the line
        using CommentPrefixFunc = AZStd::function<AZStd::string_view(AZStd::string_view line)>;

        //! Callback function that is after a has been filtered through the CommentPrefixFunc
        //! to determine if the text matches a section header
        //! returns a view of the section name if the line contains a section
        //! Otherwise an empty view is returned
        using SectionHeaderFunc = AZStd::function<AZStd::string_view(AZStd::string_view line)>;

        //! Root JSON pointer path to place all key=values pairs of configuration data within
        //! Any section headers found within the config file is appended to the root pointer path
        //! when merging the key=value pairs until another section header is found
        AZStd::string_view m_registryRootPointerPath;

        //! Function which is invoked to retrieve the non-commented section of a line
        //! The input line will never start with leading whitespace
        CommentPrefixFunc m_commentPrefixFunc = &DefaultCommentPrefixFilter;

        //! Invoked on the non-commented section of a line that has been read from the config file
        //! This function should examine the supplied line to determine if it matches a section header
        //! If so the section name should be returned
        //! Any sections names returned will be used to root any new <key, value> entries into
        //! the settings registry
        SectionHeaderFunc m_sectionHeaderFunc = &DefaultSectionHeaderFilter;

        //! structure which is forwarded to the SettingsRegistryInterface MergeCommandLineArgument function
        //! The structure contains a functor which returns true if a character is a valid delimiter
        SettingsRegistryInterface::CommandLineArgumentSettings m_commandLineSettings;

        //! enumeration to indicate if AZ::IO::FileIOBase should be used to open the config file over AZ::IO::SystemFile
        enum class FileReaderClass
        {
            UseFileIOIfAvailableFallbackToSystemFile,
            UseSystemFileOnly,
            UseFileIOOnly
        };
        FileReaderClass m_fileReaderClass = FileReaderClass::UseFileIOIfAvailableFallbackToSystemFile;
    };
    //! Loads basic configuration files which have structures similar to Windows INI files
    //! It is inspired by the Python configparser module: https://docs.python.org/3.10/library/configparser.html
    //! NOTE: There is a max line length for any one configuration entry of 4096
    //!       If a line greater than that is found parsing of the configuration file fails
    //! @param registry Settings Registry to populate with contents of config file
    //! @param filePath path to the config file. If relative the path is appended to the application root
    //!        otherwise the absolute path is used(This works transparently via the AZ::IO::Path::operator/)
    //! @param structure for determining how the configuration file should be parsed and into which json pointer path
    //! any found keys should be rooted under
    //! @return true if the configuration file was able to be merged into the Settings Registry
    bool MergeSettingsToRegistry_ConfigFile(SettingsRegistryInterface& registry, AZStd::string_view filePath,
        const ConfigParserSettings& configParserSettings);

    //! Gathers the paths of every gem registered in the ~/.o3de/o3de_manifest.json, <project-root>/project.json
    //! <engine-root>/engine.json files and merges them to the SettingsRegistry underneath the key of
    //! `ManifestGemsRootKey`
    //! This will recurse through "external_subdirectories" specified in gem.json files to gather sub gems as well
    //! @prereq - To merge gem paths under the <engine-root>, the `FilePathKey_EngineRootFolder` key must be set in the registry
    //! @prereq - To merge gem paths under the <project-root>, the `FilePathKey_ProjectPath` key must be set in the registry
    //!
    //! @param registry Settings registry where registered paths gem paths are merge to
    void MergeSettingsToRegistry_ManifestGemsPaths(SettingsRegistryInterface& registry);

    //! Extracts file path information from the environment and bootstrap to calculate the various file paths and adds those
    //! to the Settings Registry under the FilePathsRootKey.
    void MergeSettingsToRegistry_AddRuntimeFilePaths(SettingsRegistryInterface& registry);

    //! Merges the registry folder which contains the build targets that the active project depends on for loading
    //! In most cases these build targets are the gem modules and are generated automatically by the build system(CMake in this case)
    //! The files are normally generated in a Registry next to the executable
    auto MergeSettingsToRegistry_TargetBuildDependencyRegistry(SettingsRegistryInterface& registry, const AZStd::string_view platform,
        const SettingsRegistryInterface::Specializations& specializations, AZStd::vector<char>* scratchBuffer = nullptr)
        -> SettingsRegistryInterface::MergeSettingsResult;

    //! Adds the engine settings to the Settings Registry.
    auto MergeSettingsToRegistry_EngineRegistry(SettingsRegistryInterface& registry, const AZStd::string_view platform,
        const SettingsRegistryInterface::Specializations& specializations, AZStd::vector<char>* scratchBuffer = nullptr)
        -> SettingsRegistryInterface::MergeSettingsResult;

    //! Merges all the registry folders found in the listed gems.
    auto MergeSettingsToRegistry_GemRegistries(SettingsRegistryInterface& registry, const AZStd::string_view platform,
        const SettingsRegistryInterface::Specializations& specializations, AZStd::vector<char>* scratchBuffer = nullptr)
        -> SettingsRegistryInterface::MergeSettingsResult;

    //! Merges all the registry folders found in the listed gems.
    auto MergeSettingsToRegistry_ProjectRegistry(SettingsRegistryInterface& registry, const AZStd::string_view platform,
        const SettingsRegistryInterface::Specializations& specializations, AZStd::vector<char>* scratchBuffer = nullptr)
        -> SettingsRegistryInterface::MergeSettingsResult;

    //! Adds the development settings added by individual users of the project to the Settings Registry.
    //! Note that this function is only called in development builds and is compiled out in release builds.
    auto MergeSettingsToRegistry_ProjectUserRegistry(SettingsRegistryInterface& registry, const AZStd::string_view platform,
        const SettingsRegistryInterface::Specializations& specializations, AZStd::vector<char>* scratchBuffer = nullptr)
        -> SettingsRegistryInterface::MergeSettingsResult;

    //! Adds the user settings from the users home directory of "~/.o3de/Registry"
    //! '~' corresponds to %USERPROFILE% on Windows and $HOME on Unix-like platforms(Linux, Mac)
    //! Note that this function is only called in development builds and is compiled out in release builds.
    //! It is merged before the command line settings are merged so that the command line always takes precedence
    auto MergeSettingsToRegistry_O3deUserRegistry(SettingsRegistryInterface& registry, const AZStd::string_view platform,
        const SettingsRegistryInterface::Specializations& specializations, AZStd::vector<char>* scratchBuffer = nullptr)
        -> SettingsRegistryInterface::MergeSettingsResult;

    //! Filter for determining whether the current invocation of MergeSettingsToRegistry_CommandLine
    //! should parse run the regdump and regset-file commands
    struct CommandsToParse
    {
        bool m_parseRegdumpCommands{};
        bool m_parseRegsetCommands{ true };
        bool m_parseRegremoveCommands{ true };
        bool m_parseRegsetFileCommands{};
    };
    //! Adds the settings set through the command line to the Settings Registry. This will also execute any Settings
    //! Registry related arguments. Note that --regset, --regset-file and -regremove will run in the order in which they are parsed
    //! --regset <arg> Sets a value in the registry. See MergeCommandLineArgument for options for <arg>
    //!     example: --regset "/My/String/Value=String value set"
    //! --regset-file <path>[::anchor] Merges the specified file into the Settings registry
    //!     If the extension is .setregpatch, then JSON Patch  will be used to merge the file otherwise JSON Merge Patch will be used
    //!     `anchor` is a JSON path used to optionally select where to merge the settings underneath, otherwise settings are merged
    //!     under the root.
    //!     example: --regset-file="C:/Users/testuser/custom.setreg"
    //!     example: --regset-file="relative/path/other.setregpatch::/O3DE/settings"
    //! --regremove <arg> Removes a value in the registry
    //!    example: --regremove "/My/String/Value"
    //! --regdump <path> Dumps the content of the key at path and all it's content/children to output.
    //!     example: --regdump /My/Array/With/Objects
    //! --regdumpall Dumps the entire settings registry to output.
    //!
    //! The CommandsToParse structure determines which options should be processed from the command line
    //! `CommandsToParse::m_parseRegdumpCommands=true` allows the --regdump and --regdumpall commands to be processed
    //! `CommandsToParse::m_parseRegsetCommands=true` allows the --regset command to be processed
    //! `CommandsToParse::m_parseRegremveCommands=true` allows the --regremove command to be processed
    //! `CommandsToParse::m_parseRegsetFileCommands=true` allows the --regset-file command to be processed
    //!
    //! Note that this function is only called in development builds and is compiled out in release builds.
    void MergeSettingsToRegistry_CommandLine(SettingsRegistryInterface& registry, AZ::CommandLine commandLine,
        const CommandsToParse& commandsToParse = {});

    //! Stores the command line settings into the Setting Registry
    //! The arguments can be used later anywhere the command line is needed
    void StoreCommandLineToRegistry(SettingsRegistryInterface& registry, const AZ::CommandLine& commandLine);

    //! Query the command line settings from the Setting Registry and stores them
    //! into the AZ::CommandLine instance
    bool GetCommandLineFromRegistry(SettingsRegistryInterface& registry, AZ::CommandLine& commandLine);

    //! Parse a CommandLine and transform certain options into formal "regset" options
    void ParseCommandLine(AZ::CommandLine& commandLine);

    //! Structure for configuring how values should be dumped from the Settings Registry
    struct DumperSettings
    {
        //! Determines if a PrettyWriter should be used when dumping the Settings Registry
        bool m_prettifyOutput{};
        //! Include filter which is used to indicate which paths of the Settings Registry
        //! should be traversed.
        //! If the include filter is empty then all paths underneath the JSON pointer path are included
        //! otherwise the include filter invoked and if it returns true does it proceed with traversal down the path
        //! The supplied JSON pointer will be a complete path from the root of the registry
        AZStd::function<bool(AZStd::string_view path)> m_includeFilter;
        //! JSON pointer prefix to dump all settings underneath
        //! For example if the prefix is "/Amazon/Settings", then the dumped settings will be placed underneath
        //! an object at that path
        //! """
        //! {
        //!   "Amazon":{
        //!     "Settings":{ <Dumped values> }
        //!   }
        //! }
        AZStd::string_view m_jsonPointerPrefix;
    };

    //! Dumps supplied settings registry from the path specified by key if it exist the the AZ::IO::GenericStream
    //! @param key is a JSON pointer to recursively dump settings from
    //! @param stream is an AZ::IO::GenericStream that supports writing
    //! @param dumperSettings are used to determine how to format the dumped output
    bool DumpSettingsRegistryToStream(SettingsRegistryInterface& registry, AZStd::string_view key,
        AZ::IO::GenericStream& stream, const DumperSettings& dumperSettings);

    //! Do not use this function for anything other than bootstrap settings. It is only here to provide compatibility
    //! with current functionality. Proper settings per platform should use the MergeSettingsFolder functionality.
    //! Gets the value using the provided rootPath + platform + keyName, if the platform key does not exist
    //! it then tries to get the rootPath + key.
    //! @param result The target to write the result to.
    //! @param rootPath The root path of a key to the value.
    //! @param the keyName to append to the rootPath.
    //! @return Whether or not the value was retrieved. An invalid path or type-mismatch will return false;
    template<typename T>
    bool PlatformGet(SettingsRegistryInterface& registry, T& result, AZStd::string_view rootPath, AZStd::string_view keyName)
    {
        AZ::SettingsRegistryInterface::FixedValueString key = AZ::SettingsRegistryInterface::FixedValueString::format("%.*s/%s_%.*s",
            aznumeric_cast<int>(rootPath.size()), rootPath.data(), AZ_TRAIT_OS_PLATFORM_CODENAME_LOWER,
            aznumeric_cast<int>(keyName.size()), keyName.data());
        if (registry.Get(result, key))
        {
            return true;
        }

        key = AZ::SettingsRegistryInterface::FixedValueString::format("%.*s/%.*s",
            aznumeric_cast<int>(rootPath.size()), rootPath.data(),
            aznumeric_cast<int>(keyName.size()), keyName.data());
        return registry.Get(result, key);
    }

    //! Check if the supplied input path is an ancestor, a descendant or exactly equal to the candidate path
    //! The can be used to check if a JSON pointer to a settings registry entry has potentially
    //! "modified" the object at candidate path or its children in notifications
    //! @param candidatePath Path which is being checked for the ancestor/descendant relationship
    //! @param inputPath Path which is checked to determine if it is an ancestor or descendant of the candidate path
    //! @return true if the input path is an ancestor, descendant or equal to the candidate path
    //! Example: input path is ancestor path of candidate path
    //! IsPathAncestorDescendantOrEqual("/Amazon/AzCore/Bootstrap", "/Amazon/AzCore") = true
    //! Example: input path is equal to candidate path
    //! IsPathAncestorDescendantOrEqual("/Amazon/AzCore/Bootstrap", "/Amazon/AzCore/Bootstrap") = true
    //! Example: input path is descendant of candidate path
    //! IsPathAncestorDescendantOrEqual("/Amazon/AzCore/Bootstrap", "/Amazon/AzCore/Bootstrap/project_path") = true
    //! Example: input path is unrelated to candidate path
    //! IsPathAncestorDescendantOrEqual("/Amazon/AzCore/Bootstrap", "/Amazon/Project/Settings/project_name") = false
    //! Example: The path "" is the root JSON pointer therefore that is the ancestor of all paths
    //! IsPathAncestorDescendantOrEqual("/Amazon/AzCore/Bootstrap", "") = true
    bool IsPathAncestorDescendantOrEqual(AZStd::string_view candidatePath, AZStd::string_view inputPath);

    //! Check if the supplied input path is a descendant or exactly equal to the candidate path
    //! @param candidatePath Path which is being checked for the descendant relationship
    //! @param inputPath Path which is checked to determine if it is equal or a descendant of the candidate path
    //! @return true if the input path is an descendant or equal to the candidate path
    bool IsPathDescendantOrEqual(AZStd::string_view candidatePath, AZStd::string_view inputPath);

    //! Callback signature which VisitActiveGems invokes for each Gem found underneath the ActiveGems root key
    //! @param gem_name value of "gem_name" field
    //! @param gemPath absolute path to the directory containing the gem.json
    using GemCallback = AZStd::function<void(AZStd::string_view gemName, AZStd::string_view gemPath)>;

    //! Invokes a callback for each gem found underneath the ActiveGemRootKey field
    //! The paths are are unmodified from the SettingsRegistry
    //! i.e They can be relative paths if they are relative with the registry
    //! @param registry SettingsRegistry instance to search for the ActiveGemsRootKey entries
    //! @param activeGemCallback Callback to invoke when a gem is visited that contains a non-empty path entry
    void VisitActiveGems(SettingsRegistryInterface& registry, const GemCallback& activeGemCallback);

    //! Callback signature which is invoked for each manifest json with the specified manifest object key
    //! @param manifestObjectKey "<object>_name" (i.e gem_name, project_name, engine_name, o3de_manifest_name)
    //! @param manifestObjectName value of "<object>_name" field
    //! @param manifestRootPath absolute path to the directory containing the manifest file
    using ManifestCallback = AZStd::function<void(AZStd::string_view manifestObjectKey,
        AZStd::string_view manifestObjectName, AZStd::string_view manifestRootPath)>;

    //! Looks up all "external_subdirectories" fields registered in a manifest json file at the supplied path
    //! This will recurse through "gem.json" files as well
    //! @param gemManifestCallback Invoked for each manifest json file by visiting each through the "external_subdirectories" path
    //! @param manifestPath absolute path to the manifest json file to open
    //! @param manifestNameKey the key within the manifest json file that uniquely identifies the manifest
    //! @return true if manifest is a valid json file with the specified manifestNameKey field
    bool VisitManifestJson(const ManifestCallback& gemManifestCallback, AZStd::string_view manifestPath,
        AZStd::string_view manifestNameKey = GemNameKey);

    //! Looks up all "external_subdirectories" fields registered in the "~/.o3de/o3de_manifest.json"
    //! This will recurse through "gem.json" files for the "external_subdirectories" fields
    //! @param registry SettingsRegistry instance to locate the o3de manifest folder via the FilePathKey_O3deManifestRootFolder
    //! @param callback Invoked for each gem.json file by visiting each through the "external_subdirectories" path
    //! @return true if all visited paths contained a valid manifest
    bool VisitO3deManifestGems(AZ::SettingsRegistryInterface& registry, const ManifestCallback& gemManifestCallback);

    //! Looks up all "external_subdirectories" fields registered in the "<engine-root>/engine.json"
    //! This will recurse through "gem.json" files as well
    //! @param registry SettingsRegistry instance to locate the engine root via the FilePathKey_EngineRootFolder
    //! @param gemManifestCallback Invoked for each gem.json file by visiting each through the "external_subdirectories" path
    //! @return true if all visited paths contained a valid manifest
    bool VisitEngineGems(AZ::SettingsRegistryInterface& registry, const ManifestCallback& gemManifestCallback);

    //! Looks up all "external_subdirectories" fields registered in the "<project-root>/project.json"
    //! This will recurse through "gem.json" files as well
    //! @param registry SettingsRegistry instance which will be used to locate the project root via the FilePathKey_ProjectPath
    //! @param gemManifestCallback Invoked for each gem.json file by visiting each through the "external_subdirectories" path
    //! @return true if all visited paths contained a valid manifest
    bool VisitProjectGems(AZ::SettingsRegistryInterface& registry, const ManifestCallback& gemManifestCallback);

    //! Looks up all "external_subdirectories" fields registered in the "~/.o3de/o3de_manifest.json"
    //! "<engine-root>/engine.json", "<project-root>/project.json" files
    //! This will recurse through "gem.json" files for the "external_subdirectories" fields
    //! @param registry SettingsRegistry instance which will be used to locate the o3de_manifest, project and engine roots
    //! @param gemManifestCallback Invoked for each gem.json file by visiting each through the "external_subdirectories" path
    //! @return true if all visited paths contained a valid manifest
    bool VisitAllManifestGems(AZ::SettingsRegistryInterface& registry, const ManifestCallback& gemManifestCallback);
}
