/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/containers/fixed_vector.h>
#include <AzCore/Console/ConsoleFunctor.h>
#include <AzCore/Settings/SettingsRegistry.h>

namespace AZ::SettingsRegistryConsoleUtils
{
    //! The following console command are registered for the settings registry
    //! "regset", "regremove", "regdump", "regdumpall", "regset-file"
    //! The value should be increased if more commands are needed
    inline constexpr size_t MaxSettingsRegistryConsoleFunctors = 5;

    inline constexpr const char* SettingsRegistrySet = "sr_regset";
    inline constexpr const char* SettingsRegistryRemove = "sr_regremove";
    inline constexpr const char* SettingsRegistryDump = "sr_regdump";
    inline constexpr const char* SettingsRegistryDumpAll = "sr_regdumpall";
    inline constexpr const char* SettingsRegistryMergeFile = "sr_regset_file";

    // RAII structure which owns the instances of the Settings Registry Console commands
    // registered with an AZ Console
    // On destruction of the handle, the Settings Registry Console commands are unregistered
    struct ConsoleFunctorHandle
    {
        ConsoleFunctorHandle() = default;
        using SettingsRegistryConsoleFunctor = AZ::ConsoleFunctor<SettingsRegistryInterface, false>;
        using SettingsRegistryFunctorsArray = AZStd::fixed_vector<SettingsRegistryConsoleFunctor, MaxSettingsRegistryConsoleFunctors>;

        SettingsRegistryFunctorsArray m_consoleFunctors;
    };

    //! Registers the following console commands
    //! "sr_regset" accepts 2 or more arguments - <json path> <json value>*
    //!  Sets the supplied <json value> at the input <json path>
    //!
    //! "sr_regremove" accepts 1 or more argument - <json path>*
    //!  Remove the json values at each of the input <json path>*
    //!  Removal occurs in the order of the json paths specified to the command
    //!
    //! "sr_regdump" accepts 1 or more arguments <json path>*
    //!  Outputs each json value at the supplied json paths to stdout.
    //!  The values are dumped in the order of the json paths specified to the command
    //!  If multiple json paths are dumped, the values will be separated by a newline
    //!
    //! "sr_regdumpall" accepts 0 arguments and dumps the entire settings registry
    //!  NOTE: this might result in a large amount of output to the console
    //!
    //! "sr_regset_file" accepts 1 or 2 arguments - <file-path> [<anchor json path>]
    //!  Merges the json formatted file <file path> into the settings registry underneath the root anchor ""
    //!  or <anchor json path> if supplied
    [[nodiscard]] ConsoleFunctorHandle RegisterAzConsoleCommands(SettingsRegistryInterface& registry, AZ::IConsole& azConsole);
    
}
