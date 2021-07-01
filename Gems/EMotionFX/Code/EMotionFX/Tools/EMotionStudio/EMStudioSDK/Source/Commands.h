/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "EMStudioConfig.h"
#include <AzCore/std/containers/vector.h>
#include <MCore/Source/Command.h>
#include <MCore/Source/CommandGroup.h>
#include <EMotionFX/Source/MotionSet.h>
#include <EMotionFX/CommandSystem/Source/AnimGraphCommands.h>
#include <EMotionFX/CommandSystem/Source/MotionSetCommands.h>

namespace EMStudio
{
    class SourceControlCommand
        : public MCore::Command
    {
    public:
        SourceControlCommand(const char* commandName, Command* originalCommand)
            : MCore::Command(commandName, originalCommand)
        {
        }

        void InitSyntax();

        static bool CheckOutFile(const char* filename, bool& inOutFileExistedBefore, AZStd::string& outResult, bool useSourceControl, bool add);
        bool CheckOutFile(const MCore::CommandLine& parameters, const char* filepath, AZStd::string& outResult, bool add);

    private:
        bool m_fileExistsBeforehand = false;
    };

    MCORE_DEFINECOMMAND_START(CommandSaveActorAssetInfo, "Save actor assetinfo", false)
    MCORE_DEFINECOMMAND_END

    MCORE_DEFINECOMMAND_START(CommandSaveMotionAssetInfo, "Save motion assetinfo", false)
    MCORE_DEFINECOMMAND_END

    class CommandEditorLoadAnimGraph
        : public CommandSystem::CommandLoadAnimGraph
    {
    public:
        CommandEditorLoadAnimGraph(MCore::Command* orgCommand = nullptr);
        MCore::Command* Create() override { return new CommandEditorLoadAnimGraph(this); }
        static void RelocateFilename(AZStd::string& filename);
    };

    class CommandEditorLoadMotionSet
        : public CommandSystem::CommandLoadMotionSet
    {
    public:
        CommandEditorLoadMotionSet(MCore::Command* orgCommand = nullptr);
        MCore::Command* Create() override { return new CommandEditorLoadMotionSet(this); }
    };

    MCORE_DEFINECOMMAND_START_BASE(CommandSaveMotionSet, "Save motion set", false, SourceControlCommand)
    void RecursiveSetDirtyFlag(EMotionFX::MotionSet* motionSet, bool dirtyFlag);
    MCORE_DEFINECOMMAND_END

    MCORE_DEFINECOMMAND_START_BASE(CommandSaveAnimGraph, "Save an anim graph", false, SourceControlCommand)
    MCORE_DEFINECOMMAND_END

    MCORE_DEFINECOMMAND_START(CommandSaveWorkspace, "Save Workspace", false)
    MCORE_DEFINECOMMAND_END
} // namespace EMStudio
