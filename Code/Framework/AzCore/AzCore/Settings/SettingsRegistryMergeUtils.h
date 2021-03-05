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

#pragma once

#include <AzCore/IO/Path/Path_fwd.h>
#include <AzCore/Memory/OSAllocator.h>
#include <AzCore/Settings/SettingsRegistry.h>

namespace AZ
{
    class CommandLine;
}

namespace AZ::IO
{
    class GenericStream;
}

namespace AZ::SettingsRegistryMergeUtils
{
    inline static constexpr char OrganizationRootKey[] = "/Amazon";
    inline static constexpr char SpecializationsRootKey[] = "/Amazon/AzCore/Settings/Specializations";
    inline static constexpr char BootstrapSettingsRootKey[] = "/Amazon/AzCore/Bootstrap";
    inline static constexpr char GemListRootKey[] = "/Amazon/AzCore/Gems";
    inline static constexpr char FilePathsRootKey[] = "/Amazon/AzCore/Runtime/FilePaths";
    inline static constexpr char FilePathKey_BinaryFolder[] = "/Amazon/AzCore/Runtime/FilePaths/BinaryFolder";
    inline static constexpr char FilePathKey_EngineRootFolder[] = "/Amazon/AzCore/Runtime/FilePaths/EngineRootFolder";
    inline static constexpr char FilePathKey_CacheRootFolder[] = "/Amazon/AzCore/Runtime/FilePaths/CacheRootFolder";
    inline static constexpr char FilePathKey_CacheGameFolder[] = "/Amazon/AzCore/Runtime/FilePaths/CacheGameFolder";
    inline static constexpr char FilePathKey_SourceGameFolder[] = "/Amazon/AzCore/Runtime/FilePaths/SourceGameFolder";
    //! Stores the filename of the Game Project Directory which is equivalent to the project name
    inline static constexpr char FilePathKey_SourceGameName[] = "/Amazon/AzCore/Runtime/FilePaths/SourceGameName";

    //! Root key for where command line are stored at witin the settings registry
    inline static constexpr char CommandLineRootKey[] = "/Amazon/AzCore/Runtime/CommandLine";
    //! Root key for command line switches(arguments that start with "-" or "--")
    inline static constexpr char CommandLineSwitchRootKey[] = "/Amazon/AzCore/Runtime/CommandLine/Switches";
    //! Root key for command line positional arguments 
    inline static constexpr char CommandLineMiscValuesRootKey[] = "/Amazon/AzCore/Runtime/CommandLine/MiscValues";

    //! Examines the Settings Registry for a "/Amazon/CommandLine/Switches/app-root" key
    //! to use as an override for the Application Root.
    //! If that key is not found, it then checks the AZ::Utils::GetDefaultAppRootPath and returns that if it is value
    //! Otherwise a directory walk upwards from the executable directory is performed
    //! to find the boostrap.cfg file which will be used as the app root
    AZ::IO::FixedMaxPath GetAppRoot(SettingsRegistryInterface* settingsRegistry = nullptr);

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
        //! Otherwise an empty view is returend
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

    //! Loads bootstrap.cfg into the Settings Registry. This file does not support specializations.
    void MergeSettingsToRegistry_Bootstrap(SettingsRegistryInterface& registry);

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

    //! Adds the development settings added by individual users to the Settings Registry.
    //! Note that this function is only called in development builds and is compiled out in release builds.
    void MergeSettingsToRegistry_DevRegistry(SettingsRegistryInterface& registry, const AZStd::string_view platform,
        const SettingsRegistryInterface::Specializations& specializations, AZStd::vector<char>* scratchBuffer = nullptr);

    //! Adds the settings set through the command line to the Settings Registry. This will also execute any Settings
    //! Registry related arguments. Note that --set will be run first and all other commands are run afterwards and only
    //! if executeCommands is true. The following options are supported:
    //! --regset <arg> Sets a value in the registry. See MergeCommandLineArgument for options for <arg>
    //!     example: --regset "/My/String/Value=String value set"
    //! --regdump <path> Dumps the content of the key at path and all it's content/children to output.
    //!     example: --regdump /My/Array/With/Objects
    //! --regdumpall Dumps the entire settings registry to output.
    //! Note that this function is only called in development builds and is compiled out in release builds.
    void MergeSettingsToRegistry_CommandLine(SettingsRegistryInterface& registry, const AZ::CommandLine& commandLine, bool executeCommands);

    //! Stores the command line settings into the Setting Registry
    //! The arguments can be used later anywhere the command line is needed
    void MergeSettingsToRegistry_StoreCommandLine(SettingsRegistryInterface& registry, const AZ::CommandLine& commandLine);

    //! Structure for configuring how values should be dumped from the Settings Registry
    struct DumperSettings
    {
        //! Determines if a PrettyWriter should be used when dumping the Settings Registry
        bool m_prettifyOutput{};
        //! Include filter which is used to indicate which paths of the Settings Registry
        //! should be traversed. 
        //! If the include filter is empty then all paths underneath the JSON pointer path are included
        //! otherwise the include filter invoked and if it returns true does it proceed with traversal continues down the path
        AZStd::function<bool(AZStd::string_view path)> m_includeFilter;
    };

    //! Dumps supplied settings registry from the path specified by key if it exist the the AZ::IO::GenericStream
    //! key is a JSON pointer path to dumping settings recursively from
    //! stream is an AZ::IO::GenericStream that supports writing
    //! dumperSettings are used to determine how to format the dumped output
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
}
