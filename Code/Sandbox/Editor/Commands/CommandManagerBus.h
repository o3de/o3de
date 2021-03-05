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