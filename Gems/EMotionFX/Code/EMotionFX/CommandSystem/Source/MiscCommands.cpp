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

// include the required headers
#include "MiscCommands.h"
#include <MCore/Source/Command.h>
#include <MCore/Source/CommandGroup.h>
#include "CommandManager.h"
#include <EMotionFX/Source/Recorder.h>


namespace CommandSystem
{
    //--------------------------------------------------------------------------------
    // CommandRecorderClear
    //--------------------------------------------------------------------------------

    const char* CommandRecorderClear::s_RecorderClearCmdName = "RecorderClear";

    CommandRecorderClear::CommandRecorderClear(MCore::Command* orgCommand)
        : MCore::Command(s_RecorderClearCmdName, orgCommand)
    {
    }

    CommandRecorderClear::~CommandRecorderClear()
    {
    }

    bool CommandRecorderClear::Execute(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        MCORE_UNUSED(parameters);
        MCORE_UNUSED(outResult);

        const bool forceClear = parameters.GetValueAsBool("force", this);
        m_wasRecording = EMotionFX::GetRecorder().GetIsRecording();
        m_wasInPlayMode = EMotionFX::GetRecorder().GetIsInPlayMode();
        if (m_wasRecording || m_wasInPlayMode || forceClear)
        {
            EMotionFX::GetRecorder().Clear();
        }

        return true;
    }

    bool CommandRecorderClear::Undo(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        MCORE_UNUSED(parameters);
        MCORE_UNUSED(outResult);
        return true;
    }

    void CommandRecorderClear::InitSyntax()
    {
        GetSyntax().AddParameter("force", "Force clear? If set to false it will only clear while we are recording.", MCore::CommandSyntax::PARAMTYPE_BOOLEAN, "false");
    }

    const char* CommandRecorderClear::GetDescription() const
    {
        return "This command clears any existing recording inside the recorder.";
    }
} // namespace CommandSystem
