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

// include the required headers
#include "CommandSystemConfig.h"
#include <MCore/Source/Command.h>
#include <MCore/Source/CommandGroup.h>
#include <EMotionFX/Source/MorphSetup.h>


namespace CommandSystem
{
    // add a LOD level to the actor
    MCORE_DEFINECOMMAND_START(CommandAddLOD, "Add LOD", false)
    bool                            mOldDirtyFlag;
    AZStd::string                   mOldSkeletalLOD;
    MCORE_DEFINECOMMAND_END


    // remove a LOD level from the actor
    MCORE_DEFINECOMMAND_START(CommandRemoveLOD, "Remove LOD", false)
    bool                            mOldDirtyFlag;
    MCORE_DEFINECOMMAND_END

    // helper functions
    void COMMANDSYSTEM_API ClearLODLevels(EMotionFX::Actor* actor, MCore::CommandGroup* commandGroup = nullptr);
    void COMMANDSYSTEM_API ConstructReplaceManualLODCommand(EMotionFX::Actor* actor, uint32 lodLevel, const char* lodActorFileName, const MCore::Array<uint32>& enabledNodeIDs, AZStd::string* outString, bool useForMetaData = false);
} // namespace CommandSystem
