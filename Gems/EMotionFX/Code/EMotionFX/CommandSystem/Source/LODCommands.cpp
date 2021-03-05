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
#include "LODCommands.h"
#include "CommandManager.h"
#include <EMotionFX/Source/ActorInstance.h>
#include <EMotionFX/Source/Actor.h>
#include <EMotionFX/Source/Importer/Importer.h>
#include <EMotionFX/Source/ActorManager.h>
#include <AzFramework/API/ApplicationAPI.h>
#include <MCore/Source/LogManager.h>
#include <MCore/Source/StringConversions.h>


namespace CommandSystem
{
    // 22 parameters
#define SYNTAX_LODCOMMANDS                                                                                                                                                                                                                                  \
    GetSyntax().AddRequiredParameter("actorID",        "The actor identification number of the actor to work on.", MCore::CommandSyntax::PARAMTYPE_INT);                                                                                                    \

    //--------------------------------------------------------------------------------
    // CommandAddLOD
    //--------------------------------------------------------------------------------

    // constructor
    CommandAddLOD::CommandAddLOD(MCore::Command* orgCommand)
        : MCore::Command("AddLOD", orgCommand)
    {
    }


    // destructor
    CommandAddLOD::~CommandAddLOD()
    {
    }


    // execute
    bool CommandAddLOD::Execute(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        // find the actor based on the given id
        const uint32 actorID = parameters.GetValueAsInt("actorID", this);
        EMotionFX::Actor* actor = EMotionFX::GetActorManager().FindActorByID(actorID);
        if (actor == nullptr)
        {
            outResult = AZStd::string::format("Cannot execute LOD command. Actor ID %i is not valid.", actorID);
            return false;
        }

        // get the LOD level to insert at
        const uint32 lodLevel = MCore::Clamp<uint32>(parameters.GetValueAsInt("lodLevel", this), 1, actor->GetNumLODLevels());

        // manual LOD mode?
        if (parameters.CheckIfHasParameter("actorFileName"))
        {
            // get the filename of the LOD actor
            AZStd::string lodFileName;
            parameters.GetValue("actorFileName", this, &lodFileName);

            // load the LOD actor
            AZStd::unique_ptr<EMotionFX::Actor> lodActor = EMotionFX::GetImporter().LoadActor(lodFileName.c_str());
            if (lodActor == nullptr)
            {
                outResult = AZStd::string::format("Cannot execute LOD command. Loading LOD actor from file '%s' failed.", lodFileName.c_str());
                return false;
            }

            // replace the given LOD level with the lod actor and remove the LOD actor object from memory afterwards
            actor->CopyLODLevel(lodActor.get(), 0, lodLevel, false);
        }
        // add a copy of the last LOD level to the end?
        else if (parameters.CheckIfHasParameter("addLastLODLevel"))
        {
            const bool copyAddLODLevel = parameters.GetValueAsBool("addLastLODLevel", this);
            if (copyAddLODLevel)
            {
                actor->AddLODLevel();
            }
        }
        // move/insert/copy a LOD level
        else if (parameters.CheckIfHasParameter("insertAt"))
        {
            int32 insertAt = parameters.GetValueAsInt("insertAt", this);
            int32 copyFrom = parameters.GetValueAsInt("copyFrom", this);

            actor->InsertLODLevel(insertAt);

            // in case we inserted our new LOD level
            if (insertAt < copyFrom)
            {
                copyFrom++;
            }

            actor->CopyLODLevel(actor, copyFrom, insertAt, true);

            // enable or disable nodes based on the skeletal LOD flags
            const uint32 numActorInstances = EMotionFX::GetActorManager().GetNumActorInstances();
            for (uint32 j = 0; j < numActorInstances; ++j)
            {
                EMotionFX::GetActorManager().GetActorInstance(j)->UpdateSkeletalLODFlags();
            }
        }

        // check if parametes skeletal LOD node names is set
        if (parameters.CheckIfHasParameter("skeletalLOD"))
        {
            const uint32 numNodes = actor->GetNumNodes();
            // get the node names of the currently enabled nodes for the skeletal LOD
            mOldSkeletalLOD.clear();
            mOldSkeletalLOD.reserve(16448);
            for (uint32 i = 0; i < numNodes; ++i)
            {
                EMotionFX::Node* node = actor->GetSkeleton()->GetNode(i);

                // check if the current node is enabled in the given skeletal LOD level and add it to the undo information
                if (node->GetSkeletalLODStatus(lodLevel))
                {
                    mOldSkeletalLOD += node->GetName();
                    mOldSkeletalLOD += ";";
                }
            }
            AzFramework::StringFunc::Strip(mOldSkeletalLOD, MCore::CharacterConstants::semiColon, true /* case sensitive */, false /* beginning */, true /* ending */);

            // get the node names for the skeletal LOD and split the string
            AZStd::string skeletalLODString;
            skeletalLODString.reserve(16448);
            parameters.GetValue("skeletalLOD", this, &skeletalLODString);

            // get the individual node names
            AZStd::vector<AZStd::string> nodeNames;
            AzFramework::StringFunc::Tokenize(skeletalLODString.c_str(), nodeNames, MCore::CharacterConstants::semiColon, true /* keep empty strings */, true /* keep space strings */);

            // iterate through the nodes and set the skeletal LOD flag based on the input parameter
            for (uint32 i = 0; i < numNodes; ++i)
            {
                EMotionFX::Node* node = actor->GetSkeleton()->GetNode(i);

                // check if this node is enabled in the parent skeletal LOD level and only allow the node to be enabled in the current skeletal LOD level in case it is also enabled in the parent LOD level
                /*bool parendLODEnabled = true;
                if (lodLevel > 0)
                    parendLODEnabled = node->GetSkeletalLODStatus(lodLevel-1);*/

                // check if the given node is in the parameter where the enabled nodes are listed, if yes enable the skeletal LOD flag, disable if not as well as
                // check if the node is enabled in the parent LOD level and only allow enabling
                if (/*parendLODEnabled && */ AZStd::find(nodeNames.begin(), nodeNames.end(), AZStd::string(node->GetName())) != nodeNames.end())
                {
                    node->SetSkeletalLODStatus(lodLevel, true);
                }
                else
                {
                    node->SetSkeletalLODStatus(lodLevel, false);
                }
            }

            // enable or disable nodes based on the skeletal LOD flags
            const uint32 numActorInstances = EMotionFX::GetActorManager().GetNumActorInstances();
            for (uint32 j = 0; j < numActorInstances; ++j)
            {
                EMotionFX::GetActorManager().GetActorInstance(j)->UpdateSkeletalLODFlags();
            }

            // adjust the skins and remove all weights to nodes
            actor->MakeGeomLODsCompatibleWithSkeletalLODs();
        }

        // reinit our render actors
        AZStd::string reinitRenderActorsResult;
        GetCommandManager()->ExecuteCommandInsideCommand("ReInitRenderActors -resetViewCloseup false", reinitRenderActorsResult);

        // save the current dirty flag and tell the actor that something got changed
        mOldDirtyFlag = actor->GetDirtyFlag();
        actor->SetDirtyFlag(true);

        return true;
    }


    // undo the command
    bool CommandAddLOD::Undo(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        MCORE_UNUSED(parameters);
        MCORE_UNUSED(outResult);

        /*String commandString;
        //commandString = AZStd::string::format("RemoveLOD -actorID %i -lodLevel %i", );
        if (GetCommandManager()->ExecuteCommandInsideCommand(commandString.AsChar(), outResult, false) == false)
        {
            if (outResult.GetIsEmpty() == false)
                LogError( outResult.AsChar() );

            return false;
        }

        // set the dirty flag back to the old value
        actor->SetDirtyFlag(mOldDirtyFlag);*/
        return true;
    }


    // init the syntax of the command
    void CommandAddLOD::InitSyntax()
    {
        GetSyntax().ReserveParameters(22 + 4);
        SYNTAX_LODCOMMANDS
        GetSyntax().AddParameter("actorFileName",   "",                                                                 MCore::CommandSyntax::PARAMTYPE_STRING, "");
        GetSyntax().AddParameter("lodLevel",        "",                                                                 MCore::CommandSyntax::PARAMTYPE_INT, "-1");
        GetSyntax().AddParameter("insertAt",        "",                                                                 MCore::CommandSyntax::PARAMTYPE_INT, "-1");
        GetSyntax().AddParameter("copyFrom",        "",                                                                 MCore::CommandSyntax::PARAMTYPE_INT, "-1");
        GetSyntax().AddParameter("addLastLODLevel", "",                                                                 MCore::CommandSyntax::PARAMTYPE_BOOLEAN, "false");
        GetSyntax().AddParameter("skeletalLOD",     "A list of nodes that will be used to adjust the skeletal LOD.",    MCore::CommandSyntax::PARAMTYPE_STRING, "");
    }


    // get the description
    const char* CommandAddLOD::GetDescription() const
    {
        return "This command can be used to add a new LOD level.";
    }


    //--------------------------------------------------------------------------------
    // CommandRemoveGeometryLOD
    //--------------------------------------------------------------------------------

    // constructor
    CommandRemoveLOD::CommandRemoveLOD(MCore::Command* orgCommand)
        : MCore::Command("RemoveLOD", orgCommand)
    {
    }


    // destructor
    CommandRemoveLOD::~CommandRemoveLOD()
    {
    }


    // execute
    bool CommandRemoveLOD::Execute(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        AZStd::string commandResult;

        // find the actor based on the given id
        const uint32 actorID = parameters.GetValueAsInt("actorID", this);
        EMotionFX::Actor* actor = EMotionFX::GetActorManager().FindActorByID(actorID);
        if (actor == nullptr)
        {
            outResult = AZStd::string::format("Cannot execute LOD command. Actor ID %i is not valid.", actorID);
            return false;
        }

        // get the LOD level to work on
        const uint32 lodLevel = parameters.GetValueAsInt("lodLevel", this);
        if (lodLevel >= actor->GetNumLODLevels())
        {
            outResult = AZStd::string::format("Cannot execute LOD command. Actor only has %d LOD levels (valid indices are [0, %d]) while the operation wanted to work on LOD level %d.", actor->GetNumLODLevels(), actor->GetNumLODLevels() - 1, lodLevel);
            return false;
        }

        // check if there is a LOD level to remove
        if (actor->GetNumLODLevels() <= 1)
        {
            outResult = "Cannot remove LOD level 0.";
            return false;
        }

        // remove the LOD level from the actor
        actor->RemoveLODLevel(lodLevel);

        // iterate over all actor instances from the given actor and make sure they have a valid LOD level set
        const uint32 numActorInstances = EMotionFX::GetActorManager().GetNumActorInstances();
        for (uint32 i = 0; i < numActorInstances; ++i)
        {
            // get the actor instance and check if it belongs to the given actor
            EMotionFX::ActorInstance* actorInstance = EMotionFX::GetActorManager().GetActorInstance(i);
            if (actorInstance->GetActor() != actor)
            {
                continue;
            }

            // make sure the LOD level is valid
            if (actorInstance->GetLODLevel() >= actor->GetNumLODLevels())
            {
                actorInstance->SetLODLevel(actor->GetNumLODLevels() - 1);
            }
        }

        // reinit our render actors
        GetCommandManager()->ExecuteCommandInsideCommand("ReInitRenderActors -resetViewCloseup false", commandResult);

        // save the current dirty flag and tell the actor that something got changed
        mOldDirtyFlag = actor->GetDirtyFlag();
        actor->SetDirtyFlag(true);
        return true;
    }


    // undo the command
    bool CommandRemoveLOD::Undo(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        MCORE_UNUSED(parameters);
        MCORE_UNUSED(outResult);
        return true;
    }


    // init the syntax of the command
    void CommandRemoveLOD::InitSyntax()
    {
        GetSyntax().ReserveParameters(2);
        GetSyntax().AddRequiredParameter("actorID",     "The actor identification number of the actor to work on.", MCore::CommandSyntax::PARAMTYPE_INT);
        GetSyntax().AddRequiredParameter("lodLevel",    "",                                                         MCore::CommandSyntax::PARAMTYPE_INT);
    }


    // get the description
    const char* CommandRemoveLOD::GetDescription() const
    {
        return "This command can be used to remove the given LOD level.";
    }


    //--------------------------------------------------------------------------------
    // Helper functions
    //--------------------------------------------------------------------------------

    // remove all LOD levels from an actor
    void ClearLODLevels(EMotionFX::Actor* actor, MCore::CommandGroup* commandGroup)
    {
        // get the number of LOD levels and return directly in case there only is the original
        const uint32 numLODLevels = actor->GetNumLODLevels();
        if (numLODLevels == 1)
        {
            return;
        }

        // create the command group
        AZStd::string command;
        MCore::CommandGroup internalCommandGroup("Clear LOD levels");

        // iterate through the LOD levels and remove them back to front
        for (uint32 lodLevel = numLODLevels - 1; lodLevel > 0; --lodLevel)
        {
            // construct the command to remove the given LOD level
            command = AZStd::string::format("RemoveLOD -actorID %d -lodLevel %d", actor->GetID(), lodLevel);

            // add the command to the command group
            if (commandGroup == nullptr)
            {
                internalCommandGroup.AddCommandString(command.c_str());
            }
            else
            {
                commandGroup->AddCommandString(command.c_str());
            }
        }

        // execute the command or add it to the given command group
        if (commandGroup == nullptr)
        {
            AZStd::string resultString;
            if (GetCommandManager()->ExecuteCommandGroup(internalCommandGroup, resultString) == false)
            {
                MCore::LogError(resultString.c_str());
            }
        }
    }


    void PrepareSkeletalLODParameter(EMotionFX::Actor* actor, uint32 lodLevel, const MCore::Array<uint32>& enabledNodeIDs, AZStd::string* outString)
    {
        MCORE_UNUSED(lodLevel);

        // skeletal LOD
        *outString += " -skeletalLOD \"";
        const uint32 numEnabledNodes = enabledNodeIDs.GetLength();
        for (uint32 n = 0; n < numEnabledNodes; ++n)
        {
            // find the node by id
            EMotionFX::Node* node = actor->GetSkeleton()->FindNodeByID(enabledNodeIDs[n]);
            if (node == nullptr)
            {
                continue;
            }

            *outString += AZStd::string::format("%s;", node->GetName());
        }
        *outString += "\"";
    }


    // construct the replace LOD command using the manual LOD method
    void ConstructReplaceManualLODCommand(EMotionFX::Actor* actor, uint32 lodLevel, const char* lodActorFileName, const MCore::Array<uint32>& enabledNodeIDs, AZStd::string* outString, bool useForMetaData)
    {
        // clear the command string, reserve space and start filling it
        outString->clear();
        outString->reserve(16384);

        AZStd::string nativeFileName = lodActorFileName;
        EBUS_EVENT(AzFramework::ApplicationRequests::Bus, NormalizePathKeepCase, nativeFileName);

        if (useForMetaData == false)
        {
            *outString = AZStd::string::format("AddLOD -actorID %i -lodLevel %i -actorFileName \"%s\"", actor->GetID(), lodLevel, nativeFileName.c_str());
        }
        else
        {
            *outString = AZStd::string::format("AddLOD -actorID $(ACTORID) -lodLevel %i -actorFileName \"%s\"", lodLevel, nativeFileName.c_str());
        }

        // skeletal LOD
        PrepareSkeletalLODParameter(actor, lodLevel, enabledNodeIDs, outString);

        // add a new line for meta data usage
        if (useForMetaData)
        {
            *outString += "\n";
        }
    }
} // namespace CommandSystem
