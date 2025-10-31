/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// include the required headers
#include "NodeGroupCommands.h"
#include "CommandManager.h"
#include <EMotionFX/Source/Allocators.h>
#include <EMotionFX/Source/ActorManager.h>
#include <MCore/Source/LogManager.h>
#include <MCore/Source/StringConversions.h>


namespace CommandSystem
{
    //--------------------------------------------------------------------------------
    // CommandAdjustNodeGroup
    //--------------------------------------------------------------------------------
    AZ_CLASS_ALLOCATOR_IMPL(CommandAdjustNodeGroup, EMotionFX::CommandAllocator)

    CommandAdjustNodeGroup::CommandAdjustNodeGroup(
        MCore::Command* orgCommand,
        uint32 actorId,
        const AZStd::string& name,
        AZStd::optional<AZStd::string> newName,
        AZStd::optional<bool> enabledOnDefault,
        AZStd::optional<AZStd::vector<AZStd::string>> nodeNames,
        AZStd::optional<NodeAction> nodeAction
    )
        : MCore::Command(s_commandName.data(), orgCommand)
        , EMotionFX::ParameterMixinActorId(actorId)
        , m_name(name)
        , m_newName(AZStd::move(newName))
        , m_enabledOnDefault(enabledOnDefault)
        , m_nodeNames(AZStd::move(nodeNames))
        , m_nodeAction(nodeAction)
    {
    }


    // execute
    bool CommandAdjustNodeGroup::Execute(const MCore::CommandLine&, AZStd::string& outResult)
    {
        // get the actor
        EMotionFX::Actor* actor = EMotionFX::GetActorManager().FindActorByID(m_actorId);
        if (actor == nullptr)
        {
            outResult = AZStd::string::format("Cannot adjust node group. Actor with id='%i' does not exist.", m_actorId);
            return false;
        }

        // get the node group
        EMotionFX::NodeGroup* nodeGroup = actor->FindNodeGroupByNameNoCase(m_name.c_str());
        if (nodeGroup == nullptr)
        {
            outResult = AZStd::string::format("Cannot adjust node group. Node group with name='%s' does not exist.", m_name.c_str());
            return false;
        }

        m_oldNodeGroup = AZStd::make_unique<EMotionFX::NodeGroup>(*nodeGroup);

        // check if newName is set and apply new name
        if (m_newName.has_value())
        {
            nodeGroup->SetName(*m_newName);
        }

        // check if parameter disabledOnDefault is set and adjust it
        if (m_enabledOnDefault.has_value())
        {
            nodeGroup->SetIsEnabledOnDefault(*m_enabledOnDefault);
        }

        // check if parametes nodeNames is set
        if (m_nodeNames.has_value())
        {
            if (*m_nodeAction == NodeAction::Replace)
            {
                nodeGroup->GetNodeArray().clear();
            }
            for (const AZStd::string& nodeName : *m_nodeNames)
            {
                EMotionFX::Node* node = actor->GetSkeleton()->FindNodeByName(nodeName);
                if (!node)
                {
                    continue;
                }

                uint16 nodeIndex = (uint16)node->GetNodeIndex();
                nodeGroup->RemoveNodeByNodeIndex(nodeIndex);
                if (*m_nodeAction == NodeAction::Add || *m_nodeAction == NodeAction::Replace)
                {
                    nodeGroup->AddNode(nodeIndex);
                }
            }
        }

        // save the current dirty flag and tell the actor that something got changed
        m_oldDirtyFlag = actor->GetDirtyFlag();
        actor->SetDirtyFlag(true);
        return true;
    }


    // undo the command
    bool CommandAdjustNodeGroup::Undo(const MCore::CommandLine&, AZStd::string& outResult)
    {
        // return if no information about the previous node group was stored
        if (!m_oldNodeGroup)
        {
            return false;
        }

        EMotionFX::Actor* actor = EMotionFX::GetActorManager().FindActorByID(m_actorId);

        // return error if actor was not found
        if (actor == nullptr)
        {
            outResult = AZStd::string::format("Cannot adjust node group. Actor with id='%i' does not exist.", m_actorId);
            return false;
        }

        EMotionFX::NodeGroup* nodeGroup = actor->FindNodeGroupByNameNoCase(m_newName.has_value() ? m_newName->c_str() : m_name.c_str());

        if (!nodeGroup)
        {
            outResult = AZStd::string::format("Cannot adjust node group. Node group with name='%s' does not exist.", m_newName.has_value() ? m_newName->c_str() : m_name.c_str());
            return false;
        }

        // reset the old values
        if (m_enabledOnDefault.has_value())
        {
            nodeGroup->SetIsEnabledOnDefault(m_oldNodeGroup->GetIsEnabledOnDefault());
        }

        if (m_newName.has_value())
        {
            nodeGroup->SetName(m_oldNodeGroup->GetName());
        }

        if (m_nodeNames.has_value())
        {
            // clear previous nodes
            nodeGroup->GetNodeArray().clear();
            const size_t numNodes = m_oldNodeGroup->GetNumNodes();
            nodeGroup->SetNumNodes(numNodes);

            // add all nodes to the group
            for (size_t i = 0; i < numNodes; ++i)
            {
                nodeGroup->SetNode(i, m_oldNodeGroup->GetNode(i));
            }
        }

        m_oldNodeGroup = nullptr;

        // set the dirty flag back to the old value
        actor->SetDirtyFlag(m_oldDirtyFlag);
        return true;
    }


    // init the syntax of the command
    void CommandAdjustNodeGroup::InitSyntax()
    {
        GetSyntax().ReserveParameters(6);
        EMotionFX::ParameterMixinActorId::InitSyntax(GetSyntax(), /*isParameterRequired=*/ true);
        GetSyntax().AddRequiredParameter("name",             "The name of the node group to adjust.",                        MCore::CommandSyntax::PARAMTYPE_STRING);
        GetSyntax().AddParameter("newName",          "The new name of the node group.",                              MCore::CommandSyntax::PARAMTYPE_STRING,     "");
        GetSyntax().AddParameter("enabledOnDefault", "The enabled on default flag.",                                 MCore::CommandSyntax::PARAMTYPE_BOOLEAN,    "false");
        GetSyntax().AddParameter("nodeNames",        "A list of nodes that should be added to the node group.",      MCore::CommandSyntax::PARAMTYPE_STRING,     "");
        GetSyntax().AddParameter("nodeAction",       "The action to perform with the nodes passed to the command.",  MCore::CommandSyntax::PARAMTYPE_STRING,     "select");
    }


    bool CommandAdjustNodeGroup::SetCommandParameters(const MCore::CommandLine& parameters)
    {
        EMotionFX::ParameterMixinActorId::SetCommandParameters(parameters);

        m_name = parameters.GetValue("name", this);
        if (parameters.CheckIfHasParameter("newName"))
        {
            m_newName = parameters.GetValue("newName", this);
        }
        if (parameters.CheckIfHasParameter("enabledOnDefault"))
        {
            m_enabledOnDefault = parameters.GetValueAsBool("enabledOnDefault", this);
        }
        if (parameters.CheckIfHasParameter("nodeNames"))
        {
            m_nodeNames.emplace();
            AzFramework::StringFunc::Tokenize(parameters.GetValue("nodeNames", this), m_nodeNames.value(), ";", false, true);
        }
        if (parameters.CheckIfHasParameter("nodeAction"))
        {
            const AZStd::string& nodeActionStr = parameters.GetValue("nodeAction", this);
            if (nodeActionStr == "add")
            {
                m_nodeAction = NodeAction::Add;
            }
            else if (nodeActionStr == "remove")
            {
                m_nodeAction = NodeAction::Remove;
            }
            else if (nodeActionStr == "replace")
            {
                m_nodeAction = NodeAction::Replace;
            }
        }

        return true;
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
        EMotionFX::NodeGroup* nodeGroup = aznew EMotionFX::NodeGroup(name);
        actor->AddNodeGroup(nodeGroup);

        // save the current dirty flag and tell the actor that something got changed
        m_oldDirtyFlag = actor->GetDirtyFlag();
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
        actor->SetDirtyFlag(m_oldDirtyFlag);
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
        , m_oldNodeGroup(nullptr)
    {
    }


    // destructor
    CommandRemoveNodeGroup::~CommandRemoveNodeGroup()
    {
        delete m_oldNodeGroup;
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
        delete m_oldNodeGroup;
        m_oldNodeGroup = aznew EMotionFX::NodeGroup(*nodeGroup);

        // remove the node group
        actor->RemoveNodeGroup(nodeGroup);

        // save the current dirty flag and tell the actor that something got changed
        m_oldDirtyFlag = actor->GetDirtyFlag();
        actor->SetDirtyFlag(true);
        return true;
    }


    // undo the command
    bool CommandRemoveNodeGroup::Undo(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        // check if old node group exists
        if (!m_oldNodeGroup)
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
        if (name == m_oldNodeGroup->GetName())
        {
            actor->AddNodeGroup(aznew EMotionFX::NodeGroup(*m_oldNodeGroup));
        }

        // set the dirty flag back to the old value
        actor->SetDirtyFlag(m_oldDirtyFlag);
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
