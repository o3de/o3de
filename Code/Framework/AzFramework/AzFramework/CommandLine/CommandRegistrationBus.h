/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/EBus/EBus.h>

namespace AzFramework
{
    /** Standard results from a command callback function
     */
    enum class CommandResult
    {
        Success,
        Error,
        ErrorWrongNumberOfArguments,
        ErrorCommandNotFound,
    };

    /** The command callback signature
     */
    using CommandFunction = AZStd::function<CommandResult(const AZStd::vector<AZStd::string_view>& args)>;

    /** Flags to combine for a command to describe its behavior in the command callback system
     *  Note: the command flag values should match "enum EVarFlags" values inside IConsole.h
     */      
    enum CommandFlags : AZ::u32
    {
        NoValue = 0x00000000,       // the default no flags value
        Cheat = 0x00000002,         // a command that is a game cheat
        Development = 0x00000004,   // registered only for non release builds
        Restricted = 0x00080000,    // a visible command and usable in restricted mode
        Invisible = 0x00100000,     // invisible to the user when listing commands
        BlockFrame = 0x00400000,    // blocks the execution of console commands for one frame
    };

    /** A command registration system to register console commands
     */
    class CommandRegistration
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        //////////////////////////////////////////////////////////////////////////

        /** Adds a command callback function to be executed with a string identifier
         */
        virtual bool RegisterCommand(AZStd::string_view identifier, AZStd::string_view helpText, AZ::u32 commandFlags, CommandFunction callback) = 0;

        /** Removes a command (using its identifier)
         */
        virtual bool UnregisterCommand(AZStd::string_view identifier) = 0;
    };

    using CommandRegistrationBus = AZ::EBus<CommandRegistration>;

} // namespace AzFramework
