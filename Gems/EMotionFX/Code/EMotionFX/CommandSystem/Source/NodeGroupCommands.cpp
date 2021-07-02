/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// include the required headers
#include "NodeGroupCommands.h"
#include "CommandManager.h"
#include <EMotionFX/Source/NodeGroup.h>
#include <EMotionFX/Source/ActorManager.h>
#include <MCore/Source/LogManager.h>
#include <MCore/Source/StringConversions.h>


namespace CommandSystem
{
    //--------------------------------------------------------------------------------
    // CommandAdjustNodeGroup
    //--------------------------------------------------------------------------------

    // constructor
    CommandAdjustNodeGroup::CommandAdjustNodeGroup(MCore::Command* orgCommand)
        : MCore::Command("AdjustNodeGroup", orgCommand)
        , mOldNodeGroup(nullptr)
    {
    }


    // destructor
    CommandAdjustNodeGroup::~CommandAdjustNodeGroup()
    {
        if (mOldNodeGroup)
        {
            mOldNodeGroup->Destroy();
        }
    }


    // execute
    bool CommandAdjustNodeGroup::Execute(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        AZStd::string valueString;

        // get the motion id and the corresponding motion pointer
        const int32 actorID = parameters.GetValueAsInt("actorID", this);
        parameters.GetValue("name", this, &valueString);

        // get the actor
        EMotionFX::Actor* actor = EMotionFX::GetActorManager().FindActorByID(actorID);
        if (actor == nullptr)
        {
            outResult = AZStd::string::format("Cannot adjust node group. Actor with id='%i' does not exist.", actorID);
            return false;
        }

        // get the node group
        EMotionFX::NodeGroup* nodeGroup = actor->FindNodeGroupByNameNoCase(valueString.c_str());
        if (nodeGroup == nullptr)
        {
            outResult = AZStd::string::format("Cannot adjust node group. Node group with name='%s' does not exist.", valueString.c_str());
            return false;
        }

        // copy the old node group for undo
        if (mOldNodeGroup)
        {
            mOldNodeGroup->Destroy();
        }

        mOldNodeGroup = aznew EMotionFX::NodeGroup(*nodeGroup);

        // check if newName is set and apply new name
        if (parameters.CheckIfHasParameter("newName"))
        {
            parameters.GetValue("newName", this, &valueString);
            nodeGroup->SetName(valueString.c_str());
        }

        // check if parameter disabledOnDefault is set and adjust it
        if (parameters.CheckIfHasParameter("enabledOnDefault"))
        {
            const bool enabledOnDefault = parameters.GetValueAsBool("enabledOnDefault", this);
            nodeGroup->SetIsEnabledOnDefault(enabledOnDefault);
        }

        // check if parametes nodeNames is set
        if (parameters.CheckIfHasParameter("nodeNames"))
        {
            // get the node action
            AZStd::string nodeAction;
            parameters.GetValue("nodeAction", this, &valueString);

            // get the node names and split the string
            AZStd::string nodeNameString;
            parameters.GetValue("nodeNames", this, &nodeNameString);

            // get the individual node names
            AZStd::vector<AZStd::string> nodeNames;
            AzFramework::StringFunc::Tokenize(nodeNameString.c_str(), nodeNames, MCore::CharacterConstants::semiColon, true /* keep empty strings */, true /* keep space strings */);

            // get the number of nodes
            const size_t numNodes = nodeNames.size();

            // remove the selected nodes from the node group
            if (AzFramework::StringFunc::Equal(valueString.c_str(), "remove", false /* no case */))
            {
                for (size_t i = 0; i < numNodes; ++i)
                {
                    // get the node
                    EMotionFX::Node* node = actor->GetSkeleton()->FindNodeByName(nodeNames[i].c_str());
                    if (node == nullptr)
                    {
                        continue;
                    }

                    // remove the node
                    nodeGroup->RemoveNodeByNodeIndex((uint16)node->GetNodeIndex());
                }
            }
            else if (AzFramework::StringFunc::Equal(valueString.c_str(), "add", false /* no case */)) // add the selected nodes to the node group
            {
                for (size_t i = 0; i < numNodes; ++i)
                {
                    // get the node
                    EMotionFX::Node* node = actor->GetSkeleton()->FindNodeByName(nodeNames[i].c_str());
                    if (node == nullptr)
                    {
                        continue;
                    }

                    // add the node
                    uint16 nodeIndex = (uint16)node->GetNodeIndex();
                    nodeGroup->RemoveNodeByNodeIndex(nodeIndex);
                    nodeGroup->AddNode(nodeIndex);
                }
            }
            else // selected nodes form the new node group
            {
                // clear previous nodes
                nodeGroup->GetNodeArray().Clear();

                // add all nodes to the group
                for (size_t i = 0; i < numNodes; ++i)
                {
                    // get the node
                    EMotionFX::Node* node = actor->GetSkeleton()->FindNodeByName(nodeNames[i].c_str());

                    // check if node exists
                    if (node == nullptr)
                    {
                        continue;
                    }

                    // add the node
                    nodeGroup->AddNode((uint16)node->GetNodeIndex());
                }
            }
        }

        // save the current dirty flag and tell the actor that something got changed
        mOldDirtyFlag = actor->GetDirtyFlag();
        actor->SetDirtyFlag(true);
        return true;
    }


    // undo the command
    bool CommandAdjustNodeGroup::Undo(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        // return if no information about the previous node group was stored
        if (!mOldNodeGroup)
        {
            return false;
        }

        // get the motion id and the corresponding motion pointer
        int32   actorID = parameters.GetValueAsInt("actorID", this);

        // get the name
        AZStd::string name;
        if (parameters.CheckIfHasParameter("newName"))
        {
            parameters.GetValue("newName", this, &name);
        }
        else
        {
            parameters.GetValue("name", this, &name);
        }

        // get the actor
        EMotionFX::Actor* actor = EMotionFX::GetActorManager().FindActorByID(actorID);

        // return error if actor was not found
        if (actor == nullptr)
        {
            outResult = AZStd::string::format("Cannot adjust node group. Actor with id='%i' does not exist.", actorID);
            return false;
        }

        // get the node group
        EMotionFX::NodeGroup* nodeGroup = actor->FindNodeGroupByNameNoCase(name.c_str());

        // return error if node group name is not set
        if (nodeGroup == nullptr)
        {
            outResult = AZStd::string::format("Cannot adjust node group. Node group with name='%s' does not exist.", name.c_str());
            return false;
        }

        // reset the old values
        if (parameters.CheckIfHasParameter("enabledOnDefault"))
        {
            nodeGroup->SetIsEnabledOnDefault(mOldNodeGroup->GetIsEnabledOnDefault());
        }

        if (parameters.CheckIfHasParameter("newName"))
        {
            nodeGroup->SetName(mOldNodeGroup->GetName());
        }

        if (parameters.CheckIfHasParameter("nodeNames"))
        {
            // clear previous nodes
            nodeGroup->GetNodeArray().Clear();
            const uint32 numNodes = mOldNodeGroup->GetNumNodes();
            nodeGroup->SetNumNodes(static_cast<uint16>(numNodes));

            // add all nodes to the group
            for (uint32 i = 0; i < numNodes; ++i)
            {
                nodeGroup->SetNode(static_cast<uint16>(i), mOldNodeGroup->GetNode(static_cast<uint16>(i)));
            }
        }

        // delete the old node group
        if (mOldNodeGroup)
        {
            mOldNodeGroup->Destroy();
        }
        mOldNodeGroup = nullptr;

        // set the dirty flag back to the old value
        actor->SetDirtyFlag(mOldDirtyFlag);
        return true;
    }


    // init the syntax of the command
    void CommandAdjustNodeGroup::InitSyntax()
    {
        GetSyntax().ReserveParameters(6);
        GetSyntax().AddRequiredParameter("actorID",          "The id of the actor the node group belongs to.",               MCore::CommandSyntax::PARAMTYPE_INT);
        GetSyntax().AddRequiredParameter("name",             "The name of the node group to adjust.",                        MCore::CommandSyntax::PARAMTYPE_STRING);
        GetSyntax().AddParameter("newName",          "The new name of the node group.",                              MCore::CommandSyntax::PARAMTYPE_STRING,     "");
        GetSyntax().AddParameter("enabledOnDefault", "The enabled on default flag.",                                 MCore::CommandSyntax::PARAMTYPE_BOOLEAN,    "false");
        GetSyntax().AddParameter("nodeNames",        "A list of nodes that should be added to the node group.",      MCore::CommandSyntax::PARAMTYPE_STRING,     "");
        GetSyntax().AddParameter("nodeAction",       "The action to perform with the nodes passed to the command.",  MCore::CommandSyntax::PARAMTYPE_STRING,     "select");
    }


    // get the description
    const char* CommandAdjustNodeGroup::GetDescription() const
    {
        return "This command can be used to adjust the node groups of the given actor.";
    }


    //--------------------------------------------------------------------------------
    // CommandAddNodeGroup
    //--------------------------------------------------------------------------------


    // constructor
    CommandAddNodeGroup::CommandAddNodeGroup(MCore::Command* orgCommand)
        : MCore::Command("AddNodeGroup", orgCommand)
    {
    }


    // destructor
    CommandAddNodeGroup::~CommandAddNodeGroup()
    {
    }


    // execute
    bool CommandAddNodeGroup::Execute(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        // get parameters
        const int32 actorID = parameters.GetValueAsInt("actorID", this);

        AZStd::string name;
        parameters.GetValue("name", this, &name);

        // get the actor
        EMotionFX::Actor* actor = EMotionFX::GetActorManager().FindActorByID(actorID);
        if (actor == nullptr)
        {
            outResult = AZStd::string::format("Cannot add node group. Actor with id='%i' does not exist.", actorID);
            return false;
        }

        // add new node group to the actor
        EMotionFX::NodeGroup* nodeGroup = EMotionFX::NodeGroup::Create(name.c_str());
        actor->AddNodeGroup(nodeGroup);

        // save the current dirty flag and tell the actor that something got changed
        mOldDirtyFlag = actor->GetDirtyFlag();
        actor->SetDirtyFlag(true);
        return true;
    }


    // undo the command
    bool CommandAddNodeGroup::Undo(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        // get actor id and the actor
        const int32 actorID = parameters.GetValueAsInt("actorID", this);
        EMotionFX::Actor* actor = EMotionFX::GetActorManager().FindActorByID(actorID);
        if (actor == nullptr)
        {
            outResult = AZStd::string::format("Cannot undo add node group. Actor with id='%i' does not exist.", actorID);
            return false;
        }

        AZStd::string name;
        parameters.GetValue("name", this, &name);

        // call remove command
        AZStd::string command;
        command = AZStd::string::format("RemoveNodeGroup -actorID %i -name %s", actorID, name.c_str());
        if (GetCommandManager()->ExecuteCommandInsideCommand(command.c_str(), outResult) == false)
        {
            MCore::LogInfo(outResult.c_str());
            return false;
        }

        // set the dirty flag back to the old value
        actor->SetDirtyFlag(mOldDirtyFlag);
        return true;
    }


    // init the syntax of the command
    void CommandAddNodeGroup::InitSyntax()
    {
        GetSyntax().ReserveParameters(2);
        GetSyntax().AddRequiredParameter("actorID",  "The id of the actor to add the node group.",   MCore::CommandSyntax::PARAMTYPE_INT);
        GetSyntax().AddParameter("name",     "The name of the node group.",                  MCore::CommandSyntax::PARAMTYPE_STRING, "Unnamed Node Group");
    }


    // get the description
    const char* CommandAddNodeGroup::GetDescription() const
    {
        return "This command can be used to add a new node group to the selected actor.";
    }

    //--------------------------------------------------------------------------------
    // CommandRemoveNodeGroup
    //--------------------------------------------------------------------------------

    // constructor
    CommandRemoveNodeGroup::CommandRemoveNodeGroup(MCore::Command* orgCommand)
        : MCore::Command("RemoveNodeGroup", orgCommand)
        , mOldNodeGroup(nullptr)
    {
    }


    // destructor
    CommandRemoveNodeGroup::~CommandRemoveNodeGroup()
    {
        if (mOldNodeGroup)
        {
            mOldNodeGroup->Destroy();
        }
    }


    // execute
    bool CommandRemoveNodeGroup::Execute(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        // get parameters
        const int32 actorID = parameters.GetValueAsInt("actorID", this);

        AZStd::string name;
        parameters.GetValue("name", this, &name);

        // get the actor
        EMotionFX::Actor* actor = EMotionFX::GetActorManager().FindActorByID(actorID);
        if (actor == nullptr)
        {
            outResult = AZStd::string::format("Cannot remove node group. Actor with id='%i' does not exist.", actorID);
            return false;
        }

        // get the node group
        EMotionFX::NodeGroup* nodeGroup = actor->FindNodeGroupByNameNoCase(name.c_str());
        if (nodeGroup == nullptr)
        {
            outResult = AZStd::string::format("Cannot remove node group. Node group with name='%s' does not exist.", name.c_str());
            return false;
        }

        // copy the old node group for undo
        if (mOldNodeGroup)
        {
            mOldNodeGroup->Destroy();
        }

        mOldNodeGroup = aznew EMotionFX::NodeGroup(*nodeGroup);

        // remove the node group
        actor->RemoveNodeGroup(nodeGroup);

        // save the current dirty flag and tell the actor that something got changed
        mOldDirtyFlag = actor->GetDirtyFlag();
        actor->SetDirtyFlag(true);
        return true;
    }


    // undo the command
    bool CommandRemoveNodeGroup::Undo(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        // check if old node group exists
        if (!mOldNodeGroup)
        {
            return false;
        }

        // get parameters
        const int32 actorID = parameters.GetValueAsInt("actorID", this);

        AZStd::string name;
        parameters.GetValue("name", this, &name);

        // get the actor
        EMotionFX::Actor* actor = EMotionFX::GetActorManager().FindActorByID(actorID);
        if (actor == nullptr)
        {
            outResult = AZStd::string::format("Cannot remove node group. Actor with id='%i' does not exist.", actorID);
            return false;
        }

        // add the node to the group again
        if (name == mOldNodeGroup->GetName())
        {
            actor->AddNodeGroup(aznew EMotionFX::NodeGroup(*mOldNodeGroup));
        }

        // set the dirty flag back to the old value
        actor->SetDirtyFlag(mOldDirtyFlag);
        return true;
    }


    // init the syntax of the command
    void CommandRemoveNodeGroup::InitSyntax()
    {
        GetSyntax().ReserveParameters(2);
        GetSyntax().AddRequiredParameter("actorID", "The id of the actor to add the node group.",   MCore::CommandSyntax::PARAMTYPE_INT);
        GetSyntax().AddRequiredParameter("name",    "The name of the node group to remove.",        MCore::CommandSyntax::PARAMTYPE_STRING);
    }


    // get the description
    const char* CommandRemoveNodeGroup::GetDescription() const
    {
        return "This command can be used to remove a node group from the selected actor.";
    }

    //--------------------------------------------------------------------------------
    // Helper Functions
    //--------------------------------------------------------------------------------

    // remove all node groups
    void COMMANDSYSTEM_API ClearNodeGroupsCommand(EMotionFX::Actor* actor, MCore::CommandGroup* commandGroup)
    {
        // if actor does not exists return directly
        if (actor == nullptr)
        {
            return;
        }

        // get number of nodes
        const uint16 numNodeGroups = static_cast<uint16>(actor->GetNumNodeGroups());
        if (numNodeGroups == 0)
        {
            return;
        }

        // create our command group
        AZStd::string outResult;
        MCore::CommandGroup internalCommandGroup("Clear node groups");

        // iterate trough all node groups and add remove command to the command group
        AZStd::string command;
        for (uint16 i = 0; i < numNodeGroups; ++i)
        {
            // get the nodegroup and check if it is valid
            EMotionFX::NodeGroup* nodeGroup = actor->GetNodeGroup(i);
            if (nodeGroup == nullptr)
            {
                continue;
            }

            // add the command to the group
            command = AZStd::string::format("RemoveNodeGroup -actorID %i -name \"%s\"", actor->GetID(), nodeGroup->GetName());
            if (commandGroup == nullptr)
            {
                internalCommandGroup.AddCommandString(command.c_str());
            }
            else
            {
                commandGroup->AddCommandString(command.c_str());
            }
        }

        // execute the group command
        if (commandGroup == nullptr)
        {
            if (GetCommandManager()->ExecuteCommandGroup(internalCommandGroup, outResult) == false)
            {
                MCore::LogError(outResult.c_str());
            }
        }
    }
} // namespace CommandSystem
