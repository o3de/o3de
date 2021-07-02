/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/string/string.h>
#include <AzCore/std/functional.h>
#include "CommandSystemConfig.h"
#include <MCore/Source/Command.h>
#include "CommandManager.h"
#include <EMotionFX/Source/AnimGraphInstance.h>


namespace CommandSystem
{
    // load the given anim graph
    MCORE_DEFINECOMMAND_START(CommandLoadAnimGraph, "Load a anim graph", true)
    public:
        using RelocateFilenameFunction = AZStd::function<void(AZStd::string&)>;
        RelocateFilenameFunction m_relocateFilenameFunction;
        uint32 mOldAnimGraphID;
        bool mOldWorkspaceDirtyFlag;
    MCORE_DEFINECOMMAND_END


    // create a new anim graph
    MCORE_DEFINECOMMAND_START(CommandCreateAnimGraph, "Create a anim graph", true)
public:
    uint32              mPreviouslyUsedID;
    bool                mOldWorkspaceDirtyFlag;
    MCORE_DEFINECOMMAND_END


    // delete the given anim graph
    MCORE_DEFINECOMMAND_START(CommandRemoveAnimGraph, "Remove a anim graph", true)
public:
    AZStd::vector<AZStd::pair<AZStd::string, uint32>> m_oldFileNamesAndIds;
    bool                             mOldWorkspaceDirtyFlag;
    MCORE_DEFINECOMMAND_END


    // Activate the given anim graph.
    MCORE_DEFINECOMMAND_START(CommandActivateAnimGraph, "Activate a anim graph", true)
public:
    uint32                          mActorInstanceID;
    uint32                          mOldAnimGraphUsed;
    uint32                          mOldMotionSetUsed;
    float                           mOldVisualizeScaleUsed;
    bool                            mOldWorkspaceDirtyFlag;
    static const char*              s_activateAnimGraphCmdName;
    MCORE_DEFINECOMMAND_END


    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Helper functions
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    COMMANDSYSTEM_API void ClearAnimGraphsCommand(MCore::CommandGroup* commandGroup = nullptr);
    COMMANDSYSTEM_API void LoadAnimGraphsCommand(const AZStd::vector<AZStd::string>& filenames, bool reload = false);
} // namespace CommandSystem
