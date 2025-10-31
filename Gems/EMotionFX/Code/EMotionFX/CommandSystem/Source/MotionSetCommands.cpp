/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "MotionSetCommands.h"
#include "CommandManager.h"
#include <MCore/Source/Compare.h>
#include <MCore/Source/LogManager.h>
#include <MCore/Source/FileSystem.h>
#include <EMotionFX/Source/MotionSystem.h>
#include <EMotionFX/Source/MotionManager.h>
#include <EMotionFX/Source/Motion.h>
#include <EMotionFX/Source/MotionSet.h>
#include <EMotionFX/Source/ActorManager.h>
#include <EMotionFX/Source/AnimGraphManager.h>
#include <EMotionFX/Source/EventManager.h>
#include <EMotionFX/Source/Importer/Importer.h>
#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/Source/AnimGraphNode.h>
#include <EMotionFX/Source/AnimGraphMotionNode.h>
#include <AzFramework/API/ApplicationAPI.h>

namespace CommandSystem
{
    AZ_CLASS_ALLOCATOR_IMPL(CommandSystemMotionSetCallback, EMotionFX::MotionAllocator)

    // Custom motion set motion loading.
    EMotionFX::Motion* CommandSystemMotionSetCallback::LoadMotion(EMotionFX::MotionSet::MotionEntry* entry)
    {
        AZ_Assert(m_motionSet, "Motion set is nullptr.");

        // Get the full filename and file extension.
        const AZStd::string filename = m_motionSet->ConstructMotionFilename(entry);
        AZStd::string extension;
        AzFramework::StringFunc::Path::GetExtension(filename.c_str(), extension, false /* include dot */);

        // Are we dealing with a valid motion file?
        EMotionFX::Motion* motion = nullptr;
        if (AzFramework::StringFunc::Equal(extension.c_str(), "motion"))
        {
            motion = (EMotionFX::Motion*)EMotionFX::GetImporter().LoadMotion(filename.c_str(), nullptr);
        }
        else
        {
            MCore::LogWarning("MotionSet - Loading motion '%s' (id=%s) failed as the file extension isn't known.", filename.c_str(), entry->GetId().c_str());
        }

        // Create and set the motion name.
        if (motion)
        {
            AZStd::string motionName;
            AzFramework::StringFunc::Path::GetFileName(filename.c_str(), motionName);
            motion->SetName(motionName.c_str());
        }

        return motion;
    }


    //--------------------------------------------------------------------------------
    // CommandCreateMotionSet
    //--------------------------------------------------------------------------------
    CommandCreateMotionSet::CommandCreateMotionSet(MCore::Command* orgCommand)
        : MCore::Command("CreateMotionSet", orgCommand)
    {
        m_previouslyUsedId = MCORE_INVALIDINDEX32;
    }


    CommandCreateMotionSet::~CommandCreateMotionSet()
    {
    }


    bool CommandCreateMotionSet::Execute(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        AZStd::string motionSetName;
        parameters.GetValue("name", this, motionSetName);

        // Does a motion set with the given name already exist?
        if (EMotionFX::GetMotionManager().FindMotionSetByName(motionSetName.c_str()))
        {
            outResult = AZStd::string::format("Cannot create motion set. A motion set with name '%s' already exists.", motionSetName.c_str());
            return false;
        }

        // Get the parent motion set.
        EMotionFX::MotionSet* parentSet = nullptr;
        if (parameters.CheckIfHasParameter("parentSetID"))
        {
            const uint32 parentSetID = parameters.GetValueAsInt("parentSetID", this);

            // Find the parent set by id.
            parentSet = EMotionFX::GetMotionManager().FindMotionSetByID(parentSetID);
            if (!parentSet)
            {
                outResult = AZStd::string::format("Cannot create motion set. The parent motion set with id %i does not exist.", parentSetID);
                return false;
            }
        }

        // Get the motion set id.
        uint32 motionSetID = MCORE_INVALIDINDEX32;
        if (parameters.CheckIfHasParameter("motionSetID"))
        {
            motionSetID = parameters.GetValueAsInt("motionSetID", this);
            if (EMotionFX::GetMotionManager().FindMotionSetByID(motionSetID))
            {
                outResult = AZStd::string::format("Cannot create motion set. A motion set with given ID '%i' already exists.", motionSetID);
                return false;
            }
        }

        // Create the new motion set.
        EMotionFX::MotionSet* motionSet = aznew EMotionFX::MotionSet(motionSetName.c_str(), parentSet);
        if (parentSet)
        {
            parentSet->AddChildSet(motionSet);
        }

        // Set the motion set id in case the parameter is specified.
        if (motionSetID != MCORE_INVALIDINDEX32)
        {
            motionSet->SetID(motionSetID);
        }

        // In case of redoing the command set the previously used id.
        if (m_previouslyUsedId != MCORE_INVALIDINDEX32)
        {
            motionSet->SetID(m_previouslyUsedId);
        }

        // Set the filename.
        if (parameters.CheckIfHasParameter("fileName"))
        {
            AZStd::string filename;
            parameters.GetValue("fileName", this, filename);
            AzFramework::ApplicationRequests::Bus::Broadcast(
                &AzFramework::ApplicationRequests::Bus::Events::NormalizePathKeepCase, filename);
            motionSet->SetFilename(filename.c_str());
        }

        // Store info for undo.
        m_previouslyUsedId   = motionSet->GetID();
        AZStd::to_string(outResult, m_previouslyUsedId);

        // Set the motion set callback for custom motion loading.
        motionSet->SetCallback(aznew CommandSystemMotionSetCallback(motionSet), true);

        // Set the dirty flag.
        const AZStd::string commandString = AZStd::string::format("AdjustMotionSet -motionSetID %i -dirtyFlag true", m_previouslyUsedId);
        GetCommandManager()->ExecuteCommandInsideCommand(commandString, outResult);

        // Seturn the id of the newly created motion set.
        AZStd::to_string(outResult, motionSet->GetID());

        // Recursively update attributes of all nodes.
        const size_t numAnimGraphs = EMotionFX::GetAnimGraphManager().GetNumAnimGraphs();
        for (size_t i = 0; i < numAnimGraphs; ++i)
        {
            EMotionFX::AnimGraph* animGraph = EMotionFX::GetAnimGraphManager().GetAnimGraph(i);

            if (animGraph->GetIsOwnedByRuntime())
            {
                continue;
            }

            animGraph->RecursiveReinit();
        }

        // Update unique datas for all anim graph instances using the given motion set.
        EMotionFX::GetAnimGraphManager().InvalidateInstanceUniqueDataUsingMotionSet(motionSet);

        // Mark the workspace as dirty
        m_oldWorkspaceDirtyFlag = GetCommandManager()->GetWorkspaceDirtyFlag();
        GetCommandManager()->SetWorkspaceDirtyFlag(true);
        return true;
    }


    bool CommandCreateMotionSet::Undo(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        MCORE_UNUSED(parameters);

        const AZStd::string commandString = AZStd::string::format("RemoveMotionSet -motionSetID %i", m_previouslyUsedId);
        bool result = GetCommandManager()->ExecuteCommandInsideCommand(commandString, outResult);

        // Restore the workspace dirty flag.
        GetCommandManager()->SetWorkspaceDirtyFlag(m_oldWorkspaceDirtyFlag);

        return result;
    }


    void CommandCreateMotionSet::InitSyntax()
    {
        GetSyntax().ReserveParameters(4);
        GetSyntax().AddRequiredParameter("name", "The name of the motion set.",                          MCore::CommandSyntax::PARAMTYPE_STRING);
        GetSyntax().AddParameter("parentSetID",  "The name of the parent motion set.",                   MCore::CommandSyntax::PARAMTYPE_INT,        "-1");
        GetSyntax().AddParameter("motionSetID",  "The unique identification number of the motion set.",  MCore::CommandSyntax::PARAMTYPE_INT,        "-1");
        GetSyntax().AddParameter("fileName",     "The absolute filename of the motion set.",             MCore::CommandSyntax::PARAMTYPE_STRING,     "");
    }


    const char* CommandCreateMotionSet::GetDescription() const
    {
        return "Create a motion set with the given parameters.";
    }


    //--------------------------------------------------------------------------------
    // CommandRemoveMotionSet
    //--------------------------------------------------------------------------------
    CommandRemoveMotionSet::CommandRemoveMotionSet(MCore::Command* orgCommand)
        : MCore::Command("RemoveMotionSet", orgCommand)
    {
        m_previouslyUsedId = MCORE_INVALIDINDEX32;
    }


    CommandRemoveMotionSet::~CommandRemoveMotionSet()
    {
    }


    bool CommandRemoveMotionSet::Execute(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        const int32 motionSetID = parameters.GetValueAsInt("motionSetID", this);

        // Does a motion set with the given id exist?
        EMotionFX::MotionSet* motionSet = EMotionFX::GetMotionManager().FindMotionSetByID(motionSetID);
        if (!motionSet)
        {
            outResult = AZStd::string::format("Cannot remove motion set. Motion set with id '%i' does not exist.", motionSetID);
            return false;
        }

        // Store information used by undo.
        m_previouslyUsedId   = motionSet->GetID();
        m_oldName            = motionSet->GetName();
        m_oldFileName        = motionSet->GetFilename();

        if (!motionSet->GetParentSet())
        {
            m_oldParentSetId = MCORE_INVALIDINDEX32;
        }
        else
        {
            m_oldParentSetId = motionSet->GetParentSet()->GetID();

            // Set the dirty flag on the parent.
            const AZStd::string commandString = AZStd::string::format("AdjustMotionSet -motionSetID %i -dirtyFlag true", m_oldParentSetId);
            GetCommandManager()->ExecuteCommandInsideCommand(commandString, outResult);
        }

        // Update unique datas for all anim graph instances using the given motion set.
        // After removing a motion set, the used motion set from an anim graph instance will be reset. If we call this function after
        // RemoveMotionSet, the anim graph instance would hold a nullptr for motion set, and wouldn't be invalidated.
        EMotionFX::GetAnimGraphManager().InvalidateInstanceUniqueDataUsingMotionSet(motionSet);

        // Destroy the motion set.
        EMotionFX::GetMotionManager().RemoveMotionSet(motionSet, true);

        // Recursively update attributes of all nodes.
        const size_t numAnimGraphs = EMotionFX::GetAnimGraphManager().GetNumAnimGraphs();
        for (size_t i = 0; i < numAnimGraphs; ++i)
        {
            EMotionFX::AnimGraph* animGraph = EMotionFX::GetAnimGraphManager().GetAnimGraph(i);

            if (animGraph->GetIsOwnedByRuntime())
            {
                continue;
            }

            animGraph->RecursiveReinit();
        }

        // Mark the workspace as dirty.
        m_oldWorkspaceDirtyFlag = GetCommandManager()->GetWorkspaceDirtyFlag();
        GetCommandManager()->SetWorkspaceDirtyFlag(true);

        return true;
    }


    bool CommandRemoveMotionSet::Undo(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        MCORE_UNUSED(parameters);

        AZStd::string commandString = AZStd::string::format("CreateMotionSet -name \"%s\" -motionSetID %i", m_oldName.c_str(), m_previouslyUsedId);

        if (!m_oldFileName.empty())
        {
            commandString += AZStd::string::format(" -fileName \"%s\"", m_oldFileName.c_str());
        }

        if (m_oldParentSetId != MCORE_INVALIDINDEX32)
        {
            commandString += AZStd::string::format(" -parentSetID %i", m_oldParentSetId);
        }

        const bool result = GetCommandManager()->ExecuteCommandInsideCommand(commandString, outResult);

        // Restore the workspace dirty flag.
        GetCommandManager()->SetWorkspaceDirtyFlag(m_oldWorkspaceDirtyFlag);

        return result;
    }


    void CommandRemoveMotionSet::InitSyntax()
    {
        GetSyntax().ReserveParameters(1);
        GetSyntax().AddRequiredParameter("motionSetID", "The unique identification number of the motion set.", MCore::CommandSyntax::PARAMTYPE_INT);
    }


    const char* CommandRemoveMotionSet::GetDescription() const
    {
        return "Remove the given motion set.";
    }

    //--------------------------------------------------------------------------------
    // CommandAdjustMotionSet
    //--------------------------------------------------------------------------------
    CommandAdjustMotionSet::CommandAdjustMotionSet(MCore::Command* orgCommand)
        : MCore::Command("AdjustMotionSet", orgCommand)
    {
    }


    CommandAdjustMotionSet::~CommandAdjustMotionSet()
    {
    }


    bool CommandAdjustMotionSet::Execute(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        const int32 motionSetID = parameters.GetValueAsInt("motionSetID", this);

        // Does a motion set with the given id exist?
        EMotionFX::MotionSet* motionSet = EMotionFX::GetMotionManager().FindMotionSetByID(motionSetID);
        if (!motionSet)
        {
            outResult = AZStd::string::format("Cannot adjust motion set. Motion set with id '%i' does not exist.", motionSetID);
            return false;
        }

        // Adjust the dirty flag.
        if (parameters.CheckIfHasParameter("dirtyFlag"))
        {
            m_oldDirtyFlag           = motionSet->GetDirtyFlag();
            const bool dirtyFlag    = parameters.GetValueAsBool("dirtyFlag", this);
            motionSet->SetDirtyFlag(dirtyFlag);
        }

        // Set the new name in case the name parameter is specified.
        if (parameters.CheckIfHasParameter("newName"))
        {
            m_oldSetName = motionSet->GetName();
            AZStd::string name;
            parameters.GetValue("newName", this, name);
            motionSet->SetName(name.c_str());

            m_oldDirtyFlag = motionSet->GetDirtyFlag();
            motionSet->SetDirtyFlag(true);
        }

        return true;
    }


    bool CommandAdjustMotionSet::Undo(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        const int32 motionSetID = parameters.GetValueAsInt("motionSetID", this);

        // Does a motion set with the given id exist?
        EMotionFX::MotionSet* motionSet = EMotionFX::GetMotionManager().FindMotionSetByID(motionSetID);
        if (!motionSet)
        {
            outResult = AZStd::string::format("Cannot adjust motion set. Motion set with id '%i' does not exist.", motionSetID);
            return false;
        }

        // Adjust the dirty flag
        if (parameters.CheckIfHasParameter("dirtyFlag"))
        {
            motionSet->SetDirtyFlag(m_oldDirtyFlag);
        }

        // Adjust the name.
        if (parameters.CheckIfHasParameter("newName"))
        {
            motionSet->SetName(m_oldSetName.c_str());
        }

        return true;
    }


    void CommandAdjustMotionSet::InitSyntax()
    {
        GetSyntax().ReserveParameters(3);
        GetSyntax().AddRequiredParameter("motionSetID", "The unique identification number of the motion set.",                                  MCore::CommandSyntax::PARAMTYPE_INT);
        GetSyntax().AddParameter("newName",             "The new name of the motion set.",                                                      MCore::CommandSyntax::PARAMTYPE_STRING,     "");
        GetSyntax().AddParameter("dirtyFlag",           "The dirty flag indicates whether the user has made changes to the motion set or not.", MCore::CommandSyntax::PARAMTYPE_BOOLEAN,    "false");
    }


    const char* CommandAdjustMotionSet::GetDescription() const
    {
        return "Adjust the attributes of a given motion set.";
    }


    //--------------------------------------------------------------------------------
    // CommandMotionSetAddMotion
    //--------------------------------------------------------------------------------
    CommandMotionSetAddMotion::CommandMotionSetAddMotion(MCore::Command* orgCommand)
        : MCore::Command("MotionSetAddMotion", orgCommand)
    {
    }


    CommandMotionSetAddMotion::~CommandMotionSetAddMotion()
    {
    }


    bool CommandMotionSetAddMotion::Execute(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        const int32 motionSetID = parameters.GetValueAsInt("motionSetID", this);
        EMotionFX::MotionSet* motionSet = EMotionFX::GetMotionManager().FindMotionSetByID(motionSetID);
        if (!motionSet)
        {
            outResult = AZStd::string::format("Cannot add motion entries to motion set. Motion set with id '%i' does not exist.", motionSetID);
            return false;
        }

        m_oldDirtyFlag = motionSet->GetDirtyFlag();
        m_oldMotionIds.clear();

        const AZStd::string motionFilenamesAndIds = parameters.GetValue("motionFilenamesAndIds", this);

        AZStd::vector<AZStd::string> tokens;
        AzFramework::StringFunc::Tokenize(motionFilenamesAndIds.c_str(), tokens, ";", true /* keep empty strings */, true);

        const size_t tokenCount = tokens.size();
        AZ_Assert(tokenCount % 2 == 0, "There should be a motion id for each motion.");

        for (size_t i = 0; i < tokenCount; i+=2)
        {
            const AZStd::string& motionFilename = tokens[i+0];
            const AZStd::string& motionId = tokens[i+1];

            EMotionFX::MotionSet::MotionEntry* newMotionEntry = aznew EMotionFX::MotionSet::MotionEntry(motionFilename.c_str(), motionId.c_str(), nullptr);
            motionSet->AddMotionEntry(newMotionEntry);

            // Store added motion ids for undo.
            if (!m_oldMotionIds.empty())
            {
                m_oldMotionIds += ';';
            }
            m_oldMotionIds += motionId;
        }

        motionSet->SetDirtyFlag(true);

        // Recursively update attributes of all nodes.
        const size_t numAnimGraphs = EMotionFX::GetAnimGraphManager().GetNumAnimGraphs();
        for (size_t i = 0; i < numAnimGraphs; ++i)
        {
            EMotionFX::AnimGraph* animGraph = EMotionFX::GetAnimGraphManager().GetAnimGraph(i);

            if (animGraph->GetIsOwnedByRuntime())
            {
                continue;
            }

            animGraph->RecursiveReinit();
        }

        // Update unique datas for all anim graph instances using the given motion set.
        EMotionFX::GetAnimGraphManager().InvalidateInstanceUniqueDataUsingMotionSet(motionSet);

        // Return the id of the newly created motion set.
        AZStd::to_string(outResult, motionSet->GetID());
        return true;
    }


    bool CommandMotionSetAddMotion::Undo(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        const int32 motionSetID = parameters.GetValueAsInt("motionSetID", this);
        EMotionFX::MotionSet* motionSet = EMotionFX::GetMotionManager().FindMotionSetByID(motionSetID);
        if (!motionSet)
        {
            outResult = AZStd::string::format("Cannot undo add motion entries. Motion set with id '%i' does not exist.", motionSetID);
            return false;
        }

        AZStd::string commandString = AZStd::string::format("MotionSetRemoveMotion -motionSetID %i -motionIds \"", motionSetID);
        commandString += m_oldMotionIds;
        commandString += '\"';

        const bool result = GetCommandManager()->ExecuteCommandInsideCommand(commandString, outResult);
        motionSet->SetDirtyFlag(m_oldDirtyFlag);
        return result;
    }


    void CommandMotionSetAddMotion::InitSyntax()
    {
        GetSyntax().ReserveParameters(2);
        GetSyntax().AddRequiredParameter("motionSetID",             "The unique identification number of the motion set.",                                                                                  MCore::CommandSyntax::PARAMTYPE_INT);
        GetSyntax().AddRequiredParameter("motionFilenamesAndIds",   "Pairs of filenames and motion ids (everything separated by semicolons). An example would be \"Walk.motion,walk;Run.motion,run\".",     MCore::CommandSyntax::PARAMTYPE_STRING);
    }


    const char* CommandMotionSetAddMotion::GetDescription() const
    {
        return "Add motions to the given motion set.";
    }

    //--------------------------------------------------------------------------------
    // CommandMotionSetRemoveMotion
    //--------------------------------------------------------------------------------
    CommandMotionSetRemoveMotion::CommandMotionSetRemoveMotion(MCore::Command* orgCommand)
        : MCore::Command("MotionSetRemoveMotion", orgCommand)
    {
    }


    CommandMotionSetRemoveMotion::~CommandMotionSetRemoveMotion()
    {
    }


    bool CommandMotionSetRemoveMotion::Execute(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        const int32 motionSetID = parameters.GetValueAsInt("motionSetID", this);
        EMotionFX::MotionSet* motionSet = EMotionFX::GetMotionManager().FindMotionSetByID(motionSetID);
        if (!motionSet)
        {
            outResult = AZStd::string::format("Cannot remove motion entry from motion set. Motion set with id '%i' does not exist.", motionSetID);
            return false;
        }

        m_oldDirtyFlag = motionSet->GetDirtyFlag();
        m_oldMotionFilenamesAndIds.clear();

        // Get the motion ids from the parameter.
        const AZStd::string& motionIdsString = parameters.GetValue("motionIds", this);
        AZStd::vector<AZStd::string> tokens;
        AzFramework::StringFunc::Tokenize(motionIdsString.c_str(), tokens, ";", false, true);

        // Iterate over all motion ids and remove the corresponding motion entries.
        AZStd::string failedToRemoveMotionIdsString;
        for (const AZStd::string& motionId : tokens)
        {
             // Get the motion entry by id string.
            EMotionFX::MotionSet::MotionEntry* motionEntry = motionSet->FindMotionEntryById(motionId);
            if (!motionEntry)
            {
                if (!failedToRemoveMotionIdsString.empty())
                {
                    failedToRemoveMotionIdsString += ", ";
                }

                failedToRemoveMotionIdsString += motionId;
                continue;
            }

            // Store the filename and motion id for undo.
            if (!m_oldMotionFilenamesAndIds.empty())
            {
                m_oldMotionFilenamesAndIds += ';';
            }
            m_oldMotionFilenamesAndIds += motionEntry->GetFilenameString();
            m_oldMotionFilenamesAndIds += ';';
            m_oldMotionFilenamesAndIds += motionEntry->GetId();

            // Remove the motion entry from the motion set.
            motionSet->RemoveMotionEntry(motionEntry);
        }

        motionSet->SetDirtyFlag(true);

        // Recursively update attributes of all nodes.
        const size_t numAnimGraphs = EMotionFX::GetAnimGraphManager().GetNumAnimGraphs();
        for (size_t i = 0; i < numAnimGraphs; ++i)
        {
            EMotionFX::AnimGraph* animGraph = EMotionFX::GetAnimGraphManager().GetAnimGraph(i);

            if (animGraph->GetIsOwnedByRuntime())
            {
                continue;
            }

            animGraph->RecursiveReinit();
        }

        // Update unique datas for all anim graph instances using the given motion set.
        EMotionFX::GetAnimGraphManager().InvalidateInstanceUniqueDataUsingMotionSet(motionSet);

        // Check if we were able to remove all requested motion entries.
        if (!failedToRemoveMotionIdsString.empty())
        {
            outResult = AZStd::string::format("One or more motion entries could not be removed from motion set. (%s)", failedToRemoveMotionIdsString.c_str());
            return false;
        }

        return true;
    }


    bool CommandMotionSetRemoveMotion::Undo(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        const int32 motionSetID = parameters.GetValueAsInt("motionSetID", this);
        EMotionFX::MotionSet* motionSet = EMotionFX::GetMotionManager().FindMotionSetByID(motionSetID);
        if (!motionSet)
        {
            outResult = AZStd::string::format("Cannot undo remove motion entries. Motion set with id '%i' does not exist.", motionSetID);
            return false;
        }

        AZStd::string commandString = AZStd::string::format("MotionSetAddMotion -motionSetID %i -motionFilenamesAndIds \"", motionSetID);
        commandString += m_oldMotionFilenamesAndIds;
        commandString += '\"';

        const bool result = GetCommandManager()->ExecuteCommandInsideCommand(commandString, outResult);
        motionSet->SetDirtyFlag(m_oldDirtyFlag);

        return result;
    }


    void CommandMotionSetRemoveMotion::InitSyntax()
    {
        GetSyntax().ReserveParameters(2);
        GetSyntax().AddRequiredParameter("motionSetID", "The unique identification number of the motion set.",          MCore::CommandSyntax::PARAMTYPE_INT);
        GetSyntax().AddRequiredParameter("motionIds", "The identification strings that are assigned to the motions separated with semicolons.",    MCore::CommandSyntax::PARAMTYPE_STRING);
    }


    const char* CommandMotionSetRemoveMotion::GetDescription() const
    {
        return "Remove the given motions from the motion set.";
    }

    //--------------------------------------------------------------------------------
    // CommandMotionSetAdjustMotion
    //--------------------------------------------------------------------------------

    CommandMotionSetAdjustMotion::CommandMotionSetAdjustMotion(MCore::Command* orgCommand)
        : MCore::Command("MotionSetAdjustMotion", orgCommand)
    {
    }


    CommandMotionSetAdjustMotion::~CommandMotionSetAdjustMotion()
    {
    }


    void CommandMotionSetAdjustMotion::UpdateMotionNodes(const char* oldID, const char* newID)
    {
        // iterate through the anim graphs and update all motion nodes
        const size_t numAnimGraphs = EMotionFX::GetAnimGraphManager().GetNumAnimGraphs();
        for (size_t i = 0; i < numAnimGraphs; ++i)
        {
            // get the current anim graph
            EMotionFX::AnimGraph* animGraph = EMotionFX::GetAnimGraphManager().GetAnimGraph(i);

            if (animGraph->GetIsOwnedByRuntime())
            {
                continue;
            }

            // collect all motion nodes inside that anim graph
            AZStd::vector<EMotionFX::AnimGraphNode*> motionNodes;
            animGraph->RecursiveCollectNodesOfType(azrtti_typeid<EMotionFX::AnimGraphMotionNode>(), &motionNodes);

            // iterate through all motion nodes and update their id as well
            for (EMotionFX::AnimGraphNode* node : motionNodes)
            {
                EMotionFX::AnimGraphMotionNode* motionNode = static_cast<EMotionFX::AnimGraphMotionNode*>(node);
                motionNode->ReplaceMotionId(oldID, newID);
            }
        }
    }


    bool CommandMotionSetAdjustMotion::Execute(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        const int32 motionSetID = parameters.GetValueAsInt("motionSetID", this);

        // Find the motion set with the given id.
        EMotionFX::MotionSet* motionSet = EMotionFX::GetMotionManager().FindMotionSetByID(motionSetID);
        if (!motionSet)
        {
            outResult = AZStd::string::format("Cannot remove motion entry from motion set. Motion set with id '%i' does not exist.", motionSetID);
            return false;
        }

        AZStd::string idString;
        parameters.GetValue("idString", this, idString);

        // Get the motion entry.
        EMotionFX::MotionSet::MotionEntry* motionEntry = motionSet->FindMotionEntryById(idString);
        if (!motionEntry)
        {
            outResult = AZStd::string::format("Cannot adjust motion entry. Motion entry '%s' does not exist.", idString.c_str());
            return false;
        }

        // Save the old infos for undo.
        m_oldIdString        = motionEntry->GetId();
        m_oldMotionFilename  = motionEntry->GetFilename();

        if (parameters.CheckIfHasParameter("motionFileName"))
        {
            // Get the new motion filename from the parameters and set it to the entry.
            AZStd::string motionFilename;
            parameters.GetValue("motionFileName", this, motionFilename);
            motionEntry->SetFilename(motionFilename.c_str());

            // Reset the motion entry so that it automatically reloads the new motion next time.
            motionEntry->Reset();

            // Reset all motion node unique datas recursively for all anim graph instances.
            const size_t numAnimGraphInstances = EMotionFX::GetAnimGraphManager().GetNumAnimGraphInstances();
            for (size_t i = 0; i < numAnimGraphInstances; ++i)
            {
                EMotionFX::AnimGraphInstance* animGraphInstance = EMotionFX::GetAnimGraphManager().GetAnimGraphInstance(i);

                // Only continue in case the anim graph instance is using the given motion set.
                if (animGraphInstance->GetMotionSet() != motionSet)
                {
                    continue;
                }

                EMotionFX::AnimGraph* animGraph = animGraphInstance->GetAnimGraph();

                // Recursively get all motion nodes inside the anim graph.
                AZStd::vector<EMotionFX::AnimGraphNode*> motionNodes;
                animGraph->RecursiveCollectNodesOfType(azrtti_typeid<EMotionFX::AnimGraphMotionNode>(), &motionNodes);

                // Get the number of motion nodes in the anim graph and iterate through them.
                for (EMotionFX::AnimGraphNode* node : motionNodes)
                {
                    // Type cast the node to a motion node and reset its unique data so that the motion instance gets recreated.
                    EMotionFX::AnimGraphMotionNode* motionNode = static_cast<EMotionFX::AnimGraphMotionNode*>(node);
                    motionNode->ResetUniqueData(animGraphInstance);
                }
            }
        }

        if (parameters.CheckIfHasParameter("newIDString"))
        {
            // Get the new id string.
            AZStd::string newId;
            parameters.GetValue("newIDString", this, newId);

            // Build a list of unique string id values from all motion set entries.
            AZStd::vector<AZStd::string> idStrings;
            motionSet->BuildIdStringList(idStrings);

            // Check if the id string is already in the list and return false if yes. The ids have to be unique.
            if (AZStd::find(idStrings.begin(), idStrings.end(), newId) != idStrings.end())
            {
                outResult = AZStd::string::format("Cannot set id '%s' to the motion entry '%s'. The id already exists.", idString.c_str(), motionEntry->GetId().c_str());
                return false;
            }

            motionSet->SetMotionEntryId(motionEntry, newId.c_str());

            // Update all motion nodes and link them to the new motion id.
            if (parameters.GetValueAsBool("updateMotionNodeStringIDs", this))
            {
                UpdateMotionNodes(m_oldIdString.c_str(), newId.c_str());
            }
        }

        // Recursively update attributes of all nodes.
        const size_t numAnimGraphs = EMotionFX::GetAnimGraphManager().GetNumAnimGraphs();
        for (size_t i = 0; i < numAnimGraphs; ++i)
        {
            EMotionFX::AnimGraph* animGraph = EMotionFX::GetAnimGraphManager().GetAnimGraph(i);

            if (animGraph->GetIsOwnedByRuntime())
            {
                continue;
            }

            animGraph->RecursiveReinit();
        }

        // Update unique datas for all anim graph instances using the given motion set.
        EMotionFX::GetAnimGraphManager().InvalidateInstanceUniqueDataUsingMotionSet(motionSet);

        // Set the dirty flag.
        const AZStd::string command = AZStd::string::format("AdjustMotionSet -motionSetID %i -dirtyFlag true", motionSetID);
        return GetCommandManager()->ExecuteCommandInsideCommand(command, outResult);
    }


    bool CommandMotionSetAdjustMotion::Undo(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        const int32 motionSetID = parameters.GetValueAsInt("motionSetID", this);

        // In case we changed the id string, our current id string is the new id string parameter from the execute call.
        AZStd::string idString;
        const bool idStringChanged = parameters.CheckIfHasParameter("newIDString");
        if (idStringChanged)
        {
            parameters.GetValue("newIDString", this, idString);
        }
        else
        {
            // In case we didn't change the id string, just use normal id string parameter.
            parameters.GetValue("idString", this, idString);
        }

        bool updateMotionNodeStringIDs = parameters.GetValueAsBool("updateMotionNodeStringIDs", this);

        // Construct the undo command.
        AZStd::string command = AZStd::string::format("MotionSetAdjustMotion -motionSetID %i -idString \"%s\" -updateMotionNodeStringIDs %i", motionSetID, idString.c_str(), updateMotionNodeStringIDs/* ? "true" : "false"*/);

        if (idStringChanged)
        {
            command += AZStd::string::format(" -newIDString \"%s\"", m_oldIdString.c_str());
        }

        if (parameters.CheckIfHasParameter("motionFileName"))
        {
            command += AZStd::string::format(" -motionFileName \"%s\"", m_oldMotionFilename.c_str());
        }

        return GetCommandManager()->ExecuteCommandInsideCommand(command, outResult);
    }


    void CommandMotionSetAdjustMotion::InitSyntax()
    {
        GetSyntax().ReserveParameters(5);
        GetSyntax().AddRequiredParameter("motionSetID",          "The unique identification number of the motion set.",                                                 MCore::CommandSyntax::PARAMTYPE_INT);
        GetSyntax().AddRequiredParameter("idString",             "The identification string that is assigned to the motion.",                                           MCore::CommandSyntax::PARAMTYPE_STRING);
        GetSyntax().AddParameter("motionFileName",               "The local filename of the motion. An example would be \"Walk.motion\" without any path information.", MCore::CommandSyntax::PARAMTYPE_STRING, "");
        GetSyntax().AddParameter("newIDString",                  "The identification string that is assigned to the motion.",                                           MCore::CommandSyntax::PARAMTYPE_STRING, "");
        GetSyntax().AddParameter("updateMotionNodeStringIDs",    "Update references to the motion ids.",                                                                MCore::CommandSyntax::PARAMTYPE_STRING, "false");
    }


    const char* CommandMotionSetAdjustMotion::GetDescription() const
    {
        return "Adjust the given motion from the motion set.";
    }



    //--------------------------------------------------------------------------------
    // CommandLoadMotionSet
    //--------------------------------------------------------------------------------
    CommandLoadMotionSet::CommandLoadMotionSet(MCore::Command* orgCommand)
        : MCore::Command("LoadMotionSet", orgCommand)
    {
        m_oldMotionSetId = MCORE_INVALIDINDEX32;
    }


    CommandLoadMotionSet::~CommandLoadMotionSet()
    {
    }


    bool CommandLoadMotionSet::Execute(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        // Get the filename of the motion set asset.
        AZStd::string filename = parameters.GetValue("filename", this);
        if (m_relocateFilenameFunction)
        {
            m_relocateFilenameFunction(filename);
        }
        AzFramework::ApplicationRequests::Bus::Broadcast(
            &AzFramework::ApplicationRequests::Bus::Events::NormalizePathKeepCase, filename);

        // Get the old log levels.
        MCore::LogCallback::ELogLevel oldLogLevels = MCore::GetLogManager().GetLogLevels();

        // Log warnings and errors only for now.
        MCore::GetLogManager().SetLogLevels((MCore::LogCallback::ELogLevel)(MCore::LogCallback::LOGLEVEL_ERROR | MCore::LogCallback::LOGLEVEL_WARNING));

        // Is the given motion set already loaded?
        if (EMotionFX::GetMotionManager().FindMotionSetByFileName(filename.c_str()))
        {
            outResult = AZStd::string::format("Motion set '%s' has already been loaded. Skipping.", filename.c_str());
            return true;
        }

        // Load the motion set.
        EMotionFX::MotionSet* motionSet = EMotionFX::GetImporter().LoadMotionSet(filename.c_str());
        if (!motionSet)
        {
            outResult = AZStd::string::format("Could not load motion set from file '%s'.", filename.c_str());
            return false;
        }

        // In case we are in a redo call assign the previously used id.
        if (m_oldMotionSetId != MCORE_INVALIDINDEX32)
        {
            motionSet->SetID(m_oldMotionSetId);
        }
        m_oldMotionSetId = motionSet->GetID();

        // Set the custom loading callback and preload all motions.
        motionSet->SetCallback(aznew CommandSystemMotionSetCallback(motionSet), true);
        motionSet->Preload();

        // return the id of the newly created motionset
        AZStd::to_string(outResult, motionSet->GetID());

        // Mark the workspace as dirty.
        m_oldWorkspaceDirtyFlag = GetCommandManager()->GetWorkspaceDirtyFlag();
        GetCommandManager()->SetWorkspaceDirtyFlag(true);

        // Restore original log levels.
        MCore::GetLogManager().SetLogLevels(oldLogLevels);
        return true;
    }


    bool CommandLoadMotionSet::Undo(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        MCORE_UNUSED(parameters);

        // Get the motion set the command created earlier by id.
        EMotionFX::MotionSet* motionSet = EMotionFX::GetMotionManager().FindMotionSetByID(m_oldMotionSetId);
        if (motionSet == nullptr)
        {
            outResult = AZStd::string::format("Cannot undo load motion set command. Previously used motion set id '%i' is not valid.", m_oldMotionSetId);
            return false;
        }

        // Remove the motion set including all child sets.
        MCore::CommandGroup commandGroup("Remove motion sets");
        AZStd::set<AZ::u32> toBeRemoved;
        RecursivelyRemoveMotionSets(motionSet, commandGroup, toBeRemoved);
        const bool result = GetCommandManager()->ExecuteCommandGroupInsideCommand(commandGroup, outResult);

        // Restore the workspace dirty flag.
        GetCommandManager()->SetWorkspaceDirtyFlag(m_oldWorkspaceDirtyFlag);

        return result;
    }


    void CommandLoadMotionSet::InitSyntax()
    {
        GetSyntax().ReserveParameters(1);
        GetSyntax().AddRequiredParameter("filename", "The filename of the motion file.", MCore::CommandSyntax::PARAMTYPE_STRING);
    }


    const char* CommandLoadMotionSet::GetDescription() const
    {
        return "Load the given motion set from disk.";
    }


    void ClearMotionSetMotions(EMotionFX::MotionSet* motionSet, MCore::CommandGroup* commandGroup)
    {
        const EMotionFX::Motion* selectedMotion = GetCommandManager()->GetCurrentSelection().GetSingleMotion();

        const EMotionFX::MotionSet::MotionEntries& motionEntries = motionSet->GetMotionEntries();
        if (motionEntries.empty())
        {
            return;
        }

        AZStd::string commandString = AZStd::string::format("MotionSetRemoveMotion -motionSetID %i -motionIds \"", motionSet->GetID());

        AZStd::string motionIdsString;
        for (const auto& item : motionEntries)
        {
            const EMotionFX::MotionSet::MotionEntry* motionEntry = item.second;

            if (selectedMotion && motionEntry->GetMotion() == selectedMotion)
            {
                if (commandGroup)
                {
                    commandGroup->AddCommandString(AZStd::string::format("Unselect -motionName %s", selectedMotion->GetName()));
                }
                else
                {
                    AZStd::string result;
                    [[maybe_unused]] const bool success = GetCommandManager()->ExecuteCommand(AZStd::string::format("Unselect -motionName %s", selectedMotion->GetName()), result);
                    AZ_Error("EMotionFX", success, result.c_str());
                }
            }

            if (!motionIdsString.empty())
            {
                motionIdsString += ';';
            }

            motionIdsString += motionEntry->GetId();
        }

        commandString += motionIdsString;
        commandString += '\"';

        if (commandGroup)
        {
            commandGroup->AddCommandString(commandString);
        }
        else
        {
            AZStd::string result;
            if (!GetCommandManager()->ExecuteCommand(commandString, result))
            {
                AZ_Error("EMotionFX", false, result.c_str());
            }
        }
    }


    void RemoveMotionSet(EMotionFX::MotionSet* motionSet, MCore::CommandGroup* commandGroup, AZStd::set<AZ::u32>& toBeRemoved)
    {
        // Make sure we don't remove the motionset multiple times.
        if (toBeRemoved.find(motionSet->GetID()) != toBeRemoved.end())
        {
            return;
        }
        toBeRemoved.insert(motionSet->GetID());

        // Remove all motions from the motion set.
        ClearMotionSetMotions(motionSet, commandGroup);

        // Remove the motion set itself.
        const AZStd::string commandString = AZStd::string::format("RemoveMotionSet -motionSetID %i", motionSet->GetID());

        commandGroup->AddCommandString(commandString);
    }


    void RecursivelyRemoveMotionSets(EMotionFX::MotionSet* motionSet, MCore::CommandGroup& commandGroup, AZStd::set<AZ::u32>& toBeRemoved)
    {
        if (!motionSet)
        {
            return;
        }

        // Iterate through the child motion sets and recursively remove them.
        const size_t numChildSets = motionSet->GetNumChildSets();
        for (size_t i = 0; i < numChildSets; ++i)
        {
            EMotionFX::MotionSet* childSet = motionSet->GetChildSet(i);
            RecursivelyRemoveMotionSets(childSet, commandGroup, toBeRemoved);
        }

        // Remove the current motion set with all its motions.
        RemoveMotionSet(motionSet, &commandGroup, toBeRemoved);
    }


    void ClearMotionSetsCommand(MCore::CommandGroup* commandGroup)
    {
        // Create our command group.
        AZStd::string outResult;
        MCore::CommandGroup internalCommandGroup("Clear motion sets");

        // Iterate through all root motion sets and remove them.
        const size_t numMotionSets = EMotionFX::GetMotionManager().GetNumMotionSets();
        AZStd::set<AZ::u32> toBeRemoved;
        for (size_t i = 0; i < numMotionSets; ++i)
        {
            // Is the given motion set a root one? Only process root motion sets in the loop and remove all others recursively.
            EMotionFX::MotionSet* motionSet = EMotionFX::GetMotionManager().GetMotionSet(i);
            if (motionSet->GetParentSet())
            {
                continue;
            }

            if (motionSet->GetIsOwnedByRuntime() || motionSet->GetIsOwnedByAsset())
            {
                continue;
            }

            // Recursively remove motion sets.
            if (commandGroup == nullptr)
            {
                RecursivelyRemoveMotionSets(motionSet, internalCommandGroup, toBeRemoved);
            }
            else
            {
                RecursivelyRemoveMotionSets(motionSet, *commandGroup, toBeRemoved);
            }
        }

        // Execute the internal command group in case the command group parameter is not specified.
        if (!commandGroup)
        {
            AZStd::string result;
            if (!GetCommandManager()->ExecuteCommandGroup(internalCommandGroup, result))
            {
                AZ_Error("EMotionFX", false, result.c_str());
            }
        }
    }


    void LoadMotionSetsCommand(const AZStd::vector<AZStd::string>& filenames, bool reload, bool clearUpfront)
    {
        if (filenames.empty())
        {
            return;
        }

        // Get the number of motion sets to load.
        const size_t numFilenames = filenames.size();

        // Construct the command.
        const AZStd::string commandGroupName = AZStd::string::format("%s %zu motion set%s", reload ? "Reload" : "Load", numFilenames, (numFilenames > 1) ? "s" : "");
        MCore::CommandGroup commandGroup(commandGroupName);

        // Clear all other motion sets first.
        if (clearUpfront)
        {
            ClearMotionSetsCommand(&commandGroup);
        }

        // Iterate over all filenames and load the motion sets.
        AZStd::string commandString;
        AZStd::set<AZ::u32> toBeRemoved;
        for (const AZStd::string& filename : filenames)
        {
            // In case we want to reload the same motion set remove the old version first.
            EMotionFX::MotionSet* motionSet = EMotionFX::GetMotionManager().FindMotionSetByFileName(filename.c_str());

            if (reload && !clearUpfront && motionSet)
            {
                RecursivelyRemoveMotionSets(motionSet, commandGroup, toBeRemoved);
            }

            // Construct the load motion set command and add it to the group.
            commandString = AZStd::string::format("LoadMotionSet -filename \"%s\"", filename.c_str());
            commandGroup.AddCommandString(commandString);

            // iterate over each actor instance and re-active the motion set
            if (motionSet)
            {
                size_t commandIndex = 1;
                const size_t numActorInstances = EMotionFX::GetActorManager().GetNumActorInstances();
                for (size_t j = 0; j < numActorInstances; ++j)
                {
                    EMotionFX::ActorInstance* actorInstance = EMotionFX::GetActorManager().GetActorInstance(j);
                    if (!actorInstance)
                    {
                        continue;
                    }
                    EMotionFX::AnimGraphInstance* animGraphInstance = actorInstance->GetAnimGraphInstance();
                    if (!animGraphInstance)
                    {
                        continue;
                    }

                    EMotionFX::MotionSet* currentActiveMotionSet = animGraphInstance->GetMotionSet();
                    if (currentActiveMotionSet == motionSet)
                    {
                        commandString = AZStd::string::format("ActivateAnimGraph -actorInstanceID %d -animGraphID %d -motionSetID %%LASTRESULT%zu%%",
                            actorInstance->GetID(),
                            animGraphInstance->GetAnimGraph()->GetID(),
                            commandIndex);
                        commandGroup.AddCommandString(commandString);
                        ++commandIndex;
                    }
                }
            }
        }

        // Execute the group command.
        AZStd::string result;
        if (!GetCommandManager()->ExecuteCommandGroup(commandGroup, result))
        {
            AZ_Error("EMotionFX", false, result.c_str());
        }
    }
    

    AZStd::string GenerateMotionId(const AZStd::string& motionFilenameToAdd, const AZStd::string& defaultIdString, const AZStd::vector<AZStd::string>& idStrings)
    {
        // Use the filename without extension as id string in case there is no default id string specified.
        AZStd::string idString = defaultIdString;
        if (idString.empty())
        {
            AzFramework::StringFunc::Path::GetFileName(motionFilenameToAdd.c_str(), idString);
        }

        // As each entry in the motion set needs its' unique id, add a number as post-fix and increase it until we find a non-existing one that we can use.
        const AZStd::string orgIdString = idString;
        AZ::u32 iterationNr = 1;
        while (AZStd::find(idStrings.begin(), idStrings.end(), idString) != idStrings.end())
        {
            idString = AZStd::string::format("%s%i", orgIdString.c_str(), iterationNr);
            iterationNr++;
        }

        return idString;
    }


    AZStd::string AddMotionSetEntry(uint32 motionSetId, const AZStd::string& defaultIdString, const AZStd::vector<AZStd::string>& idStrings, const AZStd::string& motionFilename, MCore::CommandGroup* commandGroup)
    {
        // Internal command group is used in case the given command group parameter is nullptr.
        MCore::CommandGroup internalCommandGroup("Add motion to motion set");

        const AZStd::string motionId = GenerateMotionId(motionFilename, defaultIdString, idStrings);

        // Construct the command and add it to the command group.
        const AZStd::string command = AZStd::string::format("MotionSetAddMotion -motionSetID %i -motionFilenamesAndIds \"%s;%s\"", motionSetId, motionFilename.c_str(), motionId.c_str());

        if (!commandGroup)
        {
            internalCommandGroup.AddCommandString(command);
        }
        else
        {
            commandGroup->AddCommandString(command);
        }

        // Execute the group command.
        if (!commandGroup)
        {
            AZStd::string result;
            if (!GetCommandManager()->ExecuteCommandGroup(internalCommandGroup, result))
            {
                AZ_Error("EMotionFX", false, result.c_str());
            }
        }

        return motionId;
    }

    void CreateDefaultMotionSet(bool forceCreate, MCore::CommandGroup* commandGroup)
    {
        if (!forceCreate)
        {
            // Only add the default motion set in case there is no other present.
            const size_t numMotionSets = EMotionFX::GetMotionManager().GetNumMotionSets();
            for (size_t i = 0; i < numMotionSets; ++i)
            {
                EMotionFX::MotionSet* motionSet = EMotionFX::GetMotionManager().GetMotionSet(i);
                if (!motionSet->GetParentSet() && !motionSet->GetIsOwnedByRuntime())
                {
                    return;
                }
            }
        }

        const bool oldWorkspaceDirtyFlag = GetCommandManager()->GetWorkspaceDirtyFlag();

        const AZStd::string command = AZStd::string::format("CreateMotionSet -name \"%s\"", s_defaultMotionSetName);

        if (!commandGroup)
        {
            AZStd::string result;
            if (!GetCommandManager()->ExecuteCommand(command, result))
            {
                AZ_Error("EMotionFX", false, result.c_str());
            }
        }
        else
        {
            commandGroup->AddCommandString(command);
        }

        if (EMotionFX::MotionSet* defaultMotionSet = EMotionFX::GetMotionManager().FindMotionSetByName(s_defaultMotionSetName))
        {
            // Unset the dirty flag as an empty default motion set should not ask users to save when closing.
            defaultMotionSet->SetDirtyFlag(false);
            GetCommandManager()->SetWorkspaceDirtyFlag(oldWorkspaceDirtyFlag);
        }
    }
} // namespace CommandSystem
