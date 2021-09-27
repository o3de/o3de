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
    inline static constexpr char OrganizationRootKey[] = "/Amazon";
    inline static constexpr char BuildTargetNameKey[] = "/Amazon/AzCore/Settings/BuildTargetName";
    inline static constexpr char SpecializationsRootKey[] = "/Amazon/AzCore/Settings/Specializations";
    inline static constexpr char BootstrapSettingsRootKey[] = "/Amazon/AzCore/Bootstrap";
    inline static constexpr char GemListRootKey[] = "/Amazon/AzCore/Gems";
    inline static constexpr char FilePathsRootKey[] = "/Amazon/AzCore/Runtime/FilePaths";
    inline static constexpr char FilePathKey_BinaryFolder[] = "/Amazon/AzCore/Runtime/FilePaths/BinaryFolder";
    inline static constexpr char FilePathKey_EngineRootFolder[] = "/Amazon/AzCore/Runtime/FilePaths/EngineRootFolder";
    inline static constexpr char FilePathKey_InstalledBinaryFolder[] = "/Amazon/AzCore/Runtime/FilePaths/InstalledBinariesFolder";

    //! Stores the absolute path to root of a project's cache.  No asset platform in this path, this is where the asset database file lives.
    //! i.e. <ProjectPath>/Cache
    inline static constexpr char FilePathKey_CacheProjectRootFolder[] = "/Amazon/AzCore/Runtime/FilePaths/CacheProjectRootFolder";

    //! Stores the absolute path to the cache root for an asset platform.  This is the @root@ alias.
    //! i.e. <ProjectPath>/Cache/<assetplatform>
    inline static constexpr char FilePathKey_CacheRootFolder[] = "/Amazon/AzCore/Runtime/FilePaths/CacheRootFolder";

    //! Stores the absolute path of the Game Project Directory
    inline static constexpr char FilePathKey_ProjectPath[] = "/Amazon/AzCore/Runtime/FilePaths/SourceProjectPath";

    //! Store the absolute path to the Projects "user" directory, which is a transient directory where per user
    //! project settings can be stored
    inline static constexpr char FilePathKey_ProjectUserPath[] = "/Amazon/AzCore/Runtime/FilePaths/SourceProjectUserPath";

    //! Store the absolute path to the Projects "log" directory, which is a transient directory where per user
    //! logs can be stored. By default this would be on "{FilePathKey_ProjectUserPath}/log"
    inline static constexpr char FilePathKey_ProjectLogPath[] = "/Amazon/AzCore/Runtime/FilePaths/SourceProjectLogPath";

    //! User facing key which represents the root of a project cmake build tree. i.e the ${CMAKE_BINARY_DIR}
    //! A relative path is taking relative to the *project* root, NOT *engine* root.
    inline constexpr AZStd::string_view ProjectBuildPath = "/Amazon/Project/Settings/Build/project_build_path";
    //! In-Memory only key which stores an absolute path to the project build directory
    inline constexpr AZStd::string_view FilePathKey_ProjectBuildPath = "/Amazon/AzCore/Runtime/FilePaths/ProjectBuildPath";
    //! In-Memory only key which stores the configuration directory containing the built binaries
    inline constexpr AZStd::string_view FilePathKey_ProjectConfigurationBinPath = "/Amazon/AzCore/Runtime/FilePaths/ProjectConfigurationBinPath";

    //! Development write storage path may be considered temporary or cache storage on some platforms
    inline static constexpr char FilePathKey_DevWriteStorage[] = "/Amazon/AzCore/Runtime/FilePaths/DevWriteStorage";

    //! Stores error text regarding engine boot sequence when engine and project roots cannot be determined
    inline static constexpr char FilePathKey_ErrorText[] = "/Amazon/AzCore/Runtime/FilePaths/ErrorText";

    //! Root key for where command line are stored at within the settings registry
    inline static constexpr char CommandLineRootKey[] = "/Amazon/AzCore/Runtime/CommandLine";
    //! Key set to trigger a notification that the CommandLine has been stored within the settings registry
    //! The value of the key has no meaning. Notification Handlers only need to check if the key was supplied
    inline static constexpr char CommandLineValueChangedKey[] = "/Amazon/AzCore/Runtime/CommandLineChanged";

    //! Root key where raw project settings (project.json) file is merged to settings registry
    inline static constexpr char ProjectSettingsRootKey[] = "/Amazon/Project/Settings";

    //! Root key where raw engine manifest (o3de_manifest.json) file is merged to settings registry
    inline static constexpr char EngineManifestRootKey[] = "/Amazon/Engine/Manifest";

    //! Root key where raw engine settings (engine.json) file is merged to settings registry
    inline static constexpr char EngineSettingsRootKey[] = "/Amazon/Engine/Settings";

    //! Examines the Settings Registry for a "${BootstrapSettingsRootKey}/engine_path" key
    //! to use as an override for the Engine Root.
    //! Otherwise a directory walk upwards from the executable directory is performed
    //! to find the engine.json file which will be used as the engine root
    //! If it's still not found, attempt to find the project (by similar means) then reconcile the
    //! engine root by inspecting project.json and the engine manifest file.
    AZ::IO::FixedMaxPath FindEngineRoot(SettingsRegistryInterface& settingsRegistry);

    //! The algorithm that is used to find the project root is as follows
    //! 1. The first time this function is it performs a upward scan for a project.json file from
    //! the executable directory and if found stores that path to an internal key.
    //! In the same step it injects the path into the front of list of command line parameters
    //! using the --regset="{BootstrapSettingsRootKey}/project_path=<path>" value
    //! 2. Next the "{BootstrapSettingsRootKey}/project_path" is checked to see if it has a project path set
    //!
    //! The order in which the project path settings are overridden proceeds in the following order
    //! 1. project_path set in the <engine-root>/bootstrap.cfg file
    //! 2. project_path set in a *.setreg/*.setregpatch file
    //! 3. project_path found by scanning upwards from the executable directory to the project.json path
    //! 4. project_path set on the Command line via either --regset="{BootstrapSettingsRootKey}/project_path=<path>"
    //!    or --project_path=<path>
    AZ::IO::FixedMaxPath FindProjectRoot(SettingsRegistryInterface& settingsRegistry);

    //! Query the specializations that will be used when loading the Settings Registry.
    //! The SpecializationsRootKey is visited to retrieve any specializations stored within that section of that registry
    void QuerySpecializationsFromRegistry(SettingsRegistryInterface& registry, SettingsRegistryInterface::Specializations& specializations);

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

    //! Extracts file path information from the environment and bootstrap to calculate the various file paths and adds those
    //! to the Settings Registry under the FilePathsRootKey.
    void MergeSettingsToRegistry_AddRuntimeFilePaths(SettingsRegistryInterface& registry);

    //! Merges the registry folder which contains the build targets that the active project depends on for loading
    //! In most cases these build targets are the gem modules and are generated automatically by the build system(CMake in this case)
    //! The files are normally generated in a Registry next to the executable
    void MergeSettingsToRegistry_TargetBuildDependencyRegistry(SettingsRegistryInterface& registry, const AZStd::string_view platform,
        const SettingsRegistryInterface::Specializations& specializations, AZStd::vector<char>* scratchBuffer = nullptr);

    //! Adds the engine settings to the Settings Registry.
    void MergeSettingsToRegistry_EngineRegistry(SettingsRegistryInterface& registry, const AZStd::string_view platform,
        const SettingsRegistryInterface::Specializations& specializations, AZStd::vector<char>* scratchBuffer = nullptr);

    //! Merges all the registry folders found in the listed gems.
    void MergeSettingsToRegistry_GemRegistries(SettingsRegistryInterface& registry, const AZStd::string_view platform,
        const SettingsRegistryInterface::Specializations& specializations, AZStd::vector<char>* scratchBuffer = nullptr);

    //! Merges all the registry folders found in the listed gems.
    void MergeSettingsToRegistry_ProjectRegistry(SettingsRegistryInterface& registry, const AZStd::string_view platform,
        const SettingsRegistryInterface::Specializations& specializations, AZStd::vector<char>* scratchBuffer = nullptr);

    //! Adds the development settings added by individual users of the project to the Settings Registry.
    //! Note that this function is only called in development builds and is compiled out in release builds.
    void MergeSettingsToRegistry_ProjectUserRegistry(SettingsRegistryInterface& registry, const AZStd::string_view platform,
        const SettingsRegistryInterface::Specializations& specializations, AZStd::vector<char>* scratchBuffer = nullptr);

    //! Adds the user settings from the users home directory of "~/.o3de/Registry"
    //! '~' corresponds to %USERPROFILE% on Windows and $HOME on Unix-like platforms(Linux, Mac)
    //! Note that this function is only called in development builds and is compiled out in release builds.
    //! It is merged before the command line settings are merged so that the command line always takes precedence
    void MergeSettingsToRegistry_O3deUserRegistry(SettingsRegistryInterface& registry, const AZStd::string_view platform,
        const SettingsRegistryInterface::Specializations& specializations, AZStd::vector<char>* scratchBuffer = nullptr);

    //! Adds the settings set through the command line to the Settings Registry. This will also execute any Settings
    //! Registry related arguments. Note that --regset and -regremove will run in the order in which they are parsed
    //! --regset <arg> Sets a value in the registry. See MergeCommandLineArgument for options for <arg>
    //!     example: --regset "/My/String/Value=String value set"
    //! --regremove <arg> Removes a value in the registry
    //!    example: --regremove "/My/String/Value"
    //! only when executeCommands is true are the following options supported:
    //! --regdump <path> Dumps the content of the key at path and all it's content/children to output.
    //!     example: --regdump /My/Array/With/Objects
    //! --regdumpall Dumps the entire settings registry to output.
    //! Note that this function is only called in development builds and is compiled out in release builds.
    void MergeSettingsToRegistry_CommandLine(SettingsRegistryInterface& registry, AZ::CommandLine commandLine, bool executeCommands);

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
    //! //! Example: input path is unrelated to candidate path
    //! IsPathAncestorDescendantOrEqual("/Amazon/AzCore/Bootstrap", "/Amazon/Project/Settings/project_name") = false
    //! //! Example: The path "" is the root JSON pointer therefore that is the ancestor of all paths
    //! IsPathAncestorDescendantOrEqual("/Amazon/AzCore/Bootstrap", "") = true
    bool IsPathAncestorDescendantOrEqual(AZStd::string_view candidatePath, AZStd::string_view inputPath);
}
