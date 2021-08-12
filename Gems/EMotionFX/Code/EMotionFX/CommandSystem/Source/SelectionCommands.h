/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef __EMFX_SELECTIONCOMMANDS_H
#define __EMFX_SELECTIONCOMMANDS_H

// include the required headers
#include "CommandSystemConfig.h"
#include "SelectionList.h"
#include "CommandManager.h"
#include <EMotionFX/Source/ActorInstance.h>


namespace CommandSystem
{
    MCORE_DEFINECOMMAND_START(CommandSelect, "Select object", true)
    SelectionList m_data;
public:
    static const char* s_SelectCmdName;
    static bool Select(MCore::Command* command, const MCore::CommandLine& parameters, AZStd::string& outResult, bool unselect);
    MCORE_DEFINECOMMAND_END


    MCORE_DEFINECOMMAND_1(CommandUnselect, "Unselect", "Unselect object", true, SelectionList)
public:
    static const char* s_unselectCmdName;
    MCORE_DEFINECOMMAND_1_END


    MCORE_DEFINECOMMAND_1(CommandClearSelection, "ClearSelection", "Unselect all", true, SelectionList)
public:
    static const char* s_clearSelectionCmdName;
    MCORE_DEFINECOMMAND_1_END


    MCORE_DEFINECOMMAND_1(CommandToggleLockSelection, "ToggleLockSelection", "Selection (un)locked.", true, bool)
public:
    static const char* s_toggleLockSelectionCmdName;
    MCORE_DEFINECOMMAND_1_END

    // helper functions
    void COMMANDSYSTEM_API SelectActorInstancesUsingCommands(const AZStd::vector<EMotionFX::ActorInstance*>& selectedActorInstances);
    bool COMMANDSYSTEM_API CheckIfHasMotionSelectionParameter(const MCore::CommandLine& parameters);
    bool COMMANDSYSTEM_API CheckIfHasAnimGraphSelectionParameter(const MCore::CommandLine& parameters);
    bool COMMANDSYSTEM_API CheckIfHasActorSelectionParameter(const MCore::CommandLine& parameters, bool ignoreInstanceParameters = false);
} // namespace CommandSystem


#endif
