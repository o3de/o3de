/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "CommandSystemConfig.h"
#include <MCore/Source/Command.h>
#include <MCore/Source/CommandGroup.h>
#include <EMotionFX/Source/Motion.h>
#include <EMotionFX/Source/MotionInstance.h>
#include <EMotionFX/Source/PlayBackInfo.h>
#include <EMotionFX/Source/MotionSet.h>


namespace CommandSystem
{
    // load motions inside the motion set
    class CommandSystemMotionSetCallback
        : public EMotionFX::MotionSetCallback
    {
    public:
        AZ_CLASS_ALLOCATOR_DECL

        CommandSystemMotionSetCallback()
            : EMotionFX::MotionSetCallback() {}
        CommandSystemMotionSetCallback(EMotionFX::MotionSet* motionSet)
            : EMotionFX::MotionSetCallback(motionSet) {}
        ~CommandSystemMotionSetCallback() {}

        EMotionFX::Motion* LoadMotion(EMotionFX::MotionSet::MotionEntry* entry) override;
    };

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Main motion set commands
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    MCORE_DEFINECOMMAND_START(CommandCreateMotionSet, "Create motion set", true)
    public:
        uint32  mPreviouslyUsedID;
        bool    mOldWorkspaceDirtyFlag;
    MCORE_DEFINECOMMAND_END

    MCORE_DEFINECOMMAND_START(CommandRemoveMotionSet, "Remove motion set", true)
    public:
        AZStd::string   mOldName;
        AZStd::string   mOldFileName;
        uint32          mOldParentSetID;
        uint32          mPreviouslyUsedID;
        bool            mOldWorkspaceDirtyFlag;
    MCORE_DEFINECOMMAND_END

    MCORE_DEFINECOMMAND_START(CommandAdjustMotionSet, "Adjust motion set", true)
        AZStd::string   mOldSetName;
        bool            mOldDirtyFlag;
    MCORE_DEFINECOMMAND_END

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    // EMotionFX::Motion* entries
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    MCORE_DEFINECOMMAND_START(CommandMotionSetAddMotion, "Add motion to set", true)
        bool                m_oldDirtyFlag;
        AZStd::string       m_oldMotionIds;
    MCORE_DEFINECOMMAND_END

    MCORE_DEFINECOMMAND_START(CommandMotionSetRemoveMotion, "Remove motion from set", true)
        AZStd::string       m_oldMotionFilenamesAndIds;
        bool                m_oldDirtyFlag;
    MCORE_DEFINECOMMAND_END

    MCORE_DEFINECOMMAND_START(CommandMotionSetAdjustMotion, "Adjust motion set", true)
    public:
        AZStd::string   mOldIdString;
        AZStd::string   mOldMotionFilename;
        void UpdateMotionNodes(const char* oldID, const char* newID);
    MCORE_DEFINECOMMAND_END

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Loading/Saving
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    MCORE_DEFINECOMMAND_START(CommandLoadMotionSet, "Load motion set", true)
    public:
        using RelocateFilenameFunction = AZStd::function<void(AZStd::string&)>;
        RelocateFilenameFunction m_relocateFilenameFunction;
        uint32 mOldMotionSetID;
        bool mOldWorkspaceDirtyFlag;
    MCORE_DEFINECOMMAND_END

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Helper functions
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    void COMMANDSYSTEM_API ClearMotionSetMotions(EMotionFX::MotionSet* motionSet, MCore::CommandGroup* commandGroup = nullptr);
    void COMMANDSYSTEM_API RecursivelyRemoveMotionSets(EMotionFX::MotionSet* motionSet, MCore::CommandGroup& commandGroup, AZStd::set<AZ::u32>& toBeRemoved);
    void COMMANDSYSTEM_API ClearMotionSetsCommand(MCore::CommandGroup* commandGroup = nullptr);
    void COMMANDSYSTEM_API LoadMotionSetsCommand(const AZStd::vector<AZStd::string>& filenames, bool reload, bool clearUpfront);
    AZStd::string COMMANDSYSTEM_API GenerateMotionId(const AZStd::string& motionFilenameToAdd, const AZStd::string& defaultIdString, const AZStd::vector<AZStd::string>& idStrings);

    /**
     * Construct the command to add a new motion set entry.
     * @param[in] motionSetId The runtime id for the motion set where we want to add the new motion entry.
     * @param[in] defaultIdString The entry string id for the new motion entry. In case the string is empty and the motionFilename is valid, we'll use the name of the file without extension as string id.
     * @param[in] idStrings A list of already existing string ids for the motion set. This is needed because we need to make sure each id string is unique. In case the defaultIdString is already being used
     *                      we're adding numbers as postfix.
     * @param[in] motionFilename In case the new motion entry is already linked to a motion asset, specify the filename here.
     * @param[in] commandGroup In case a command group is specified, the newly constructed command will be added to the group but is not executed. Elsewise the command is directly executed as a single command.
     */
    AZStd::string COMMANDSYSTEM_API AddMotionSetEntry(uint32 motionSetId, const AZStd::string& defaultIdString, const AZStd::vector<AZStd::string>& idStrings, const AZStd::string& motionFilename, MCore::CommandGroup* commandGroup = nullptr);
} // namespace CommandSystem
