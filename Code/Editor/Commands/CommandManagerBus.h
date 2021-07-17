/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>

class CommandManagerRequests : public AZ::EBusTraits
{
public:

    struct CommandDetails
    {
        AZStd::string m_name;
        AZStd::vector<AZStd::string> m_arguments;
    };

    virtual AZStd::vector<AZStd::string> GetCommands() const = 0;
    virtual void ExecuteCommand(const AZStd::string& commandLine) {}

    virtual void GetCommandDetails(AZStd::string commandName, CommandDetails& outArguments) const = 0;

};

using CommandManagerRequestBus = AZ::EBus<CommandManagerRequests>;
