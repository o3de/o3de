/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "AnimGraphConnectionCommands.h"
#include "AnimGraphNodeCommands.h"
#include "CommandManager.h"
#include <MCore/Source/LogManager.h>
#include <MCore/Source/ReflectionSerializer.h>
#include <EMotionFX/Source/ActorInstance.h>
#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/Source/BlendTreeParameterNode.h>
#include <EMotionFX/Source/AnimGraphInstance.h>
#include <EMotionFX/Source/AnimGraphManager.h>
#include <EMotionFX/Source/BlendTreeParameterNode.h>
#include <EMotionFX/Source/AnimGraphObjectFactory.h>
#include <EMotionFX/Source/AnimGraphMotionCondition.h>
#include <EMotionFX/Source/AnimGraphStateMachine.h>
#include <EMotionFX/Source/AnimGraphStateTransition.h>
#include <EMotionFX/Source/AnimGraphTransitionCondition.h>
#include <EMotionFX/Source/BlendTree.h>
#include <EMotionFX/Source/BlendTreeBlendNNode.h>
#include <EMotionFX/Source/ActorManager.h>
#include <EMotionFX/CommandSystem/Source/AnimGraphConditionCommands.h>
#include <EMotionFX/CommandSystem/Source/AnimGraphTriggerActionCommands.h>

namespace CommandSystem
{
    EMotionFX::AnimGraph* CommandsGetAnimGraph(const MCore::CommandLine& parameters, MCore::Command* command, AZStd::string& outResult)
    {
        if (parameters.CheckIfHasParameter("animGraphID"))
        {
            const uint32 animGraphID = parameters.GetValueAsInt("animGraphID", command);
            EMotionFX::AnimGraph* animGraph = EMotionFX::GetAnimGraphManager().FindAnimGraphByID(animGraphID);
            if (animGraph == nullptr)
            {
                outResult = AZStd::string::format("Anim graph with id '%i' cannot be found.", animGraphID);
                return nullptr;
            }
            return animGraph;
        }
        else
        {
            EMotionFX::AnimGraph* animGraph = GetCommandManager()->GetCurrentSelection().GetFirstAnimGraph();
            if (animGraph == nullptr)
            {
                outResult = AZStd::string::format("Anim graph id is not specified and no one anim graph is selected.");
                return nullptr;
            }
            return animGraph;
        }
    }

    //-------------------------------------------------------------------------------------
    // AnimGraphCreateConnection - create a connection between two nodes
    //-------------------------------------------------------------------------------------

    // constructor
    CommandAnimGraphCreateConnection::CommandAnimGraphCreateConnection(MCore::Command* orgCommand)
        : MCore::Command("AnimGraphCreateConnection", orgCommand)
    {
        m_transitionType = AZ::TypeId::CreateNull();
    }

    // destructor
    CommandAnimGraphCreateConnection::~CommandAnimGraphCreateConnection()
    {
    }


    // execute
    bool CommandAnimGraphCreateConnection::Execute(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        // get the anim graph to work on
        EMotionFX::AnimGraph* animGraph = CommandsGetAnimGraph(parameters, this, outResult);
        if (animGraph == nullptr)
        {
            return false;
        }

        // store the anim graph id for undo
        m_animGraphId = animGraph->GetID();

        // get the transition type
        AZ::Outcome<AZStd::string> transitionTypeString = parameters.GetValueIfExists("transitionType", this);
        if (transitionTypeString.IsSuccess())
        {
            m_transitionType = AZ::TypeId::CreateString(transitionTypeString.GetValue().c_str());
        }
        
        // get the node names
        AZStd::string sourceNodeName;
        AZStd::string targetNodeName;
        parameters.GetValue("sourceNode", "", sourceNodeName);
        parameters.GetValue("targetNode", "", targetNodeName);

        // find the source node in the anim graph
        EMotionFX::AnimGraphNode* sourceNode = animGraph->RecursiveFindNodeByName(sourceNodeName.c_str());

        // find the target node in the anim graph
        EMotionFX::AnimGraphNode* targetNode = animGraph->RecursiveFindNodeByName(targetNodeName.c_str());
        if (!targetNode)
        {
            outResult = AZStd::string::format("Cannot find target node with name '%s' in anim graph '%s'", targetNodeName.c_str(), animGraph->GetFileName());
            return false;
        }

        if (targetNode == sourceNode)
        {
            outResult = AZStd::string::format("Unable to create connection: source node and target node are the same. Node name = %s", targetNode->GetName());
            return false;
        }

        // get the ports
        m_sourcePort = parameters.GetValueAsInt("sourcePort", 0);
        m_targetPort = parameters.GetValueAsInt("targetPort", 0);
        parameters.GetValue("sourcePortName", this, m_sourcePortName);
        parameters.GetValue("targetPortName", this, m_targetPortName);

        // in case the source port got specified by name, overwrite the source port number
        if (!m_sourcePortName.empty())
        {
            m_sourcePort = sourceNode->FindOutputPortIndex(m_sourcePortName);

            // in case we want to add this connection to a parameter node while the parameter name doesn't exist, still return true so that copy paste doesn't fail
            if (azrtti_typeid(sourceNode) == azrtti_typeid<EMotionFX::BlendTreeParameterNode>() && m_sourcePort == InvalidIndex)
            {
                m_connectionId.SetInvalid();
                return true;
            }
        }

        // in case the target port got specified by name, overwrite the target port number
        if (!m_targetPortName.empty())
        {
            m_targetPort = targetNode->FindInputPortIndex(m_targetPortName.c_str());
        }

        // get the parent of the source node
        if (targetNode->GetParentNode() == nullptr)
        {
            outResult = AZStd::string::format("Cannot create connections between root state machines.");
            return false;
        }

        // if the parent is state machine, we don't need to check the port ranges
        if (azrtti_typeid(targetNode->GetParentNode()) != azrtti_typeid<EMotionFX::AnimGraphStateMachine>())
        {
            if (sourceNode == nullptr)
            {
                outResult = AZStd::string::format("Cannot create blend tree connection in anim graph '%s' as the source node is not valid. ", animGraph->GetFileName());
                return false;
            }

            // verify port ranges
            if (m_sourcePort >= sourceNode->GetOutputPorts().size())
            {
                outResult = AZStd::string::format("The output port number is not valid for the given node. Node '%s' only has %zu output ports.", sourceNode->GetName(), sourceNode->GetOutputPorts().size());
                return false;
            }

            if (m_targetPort >= targetNode->GetInputPorts().size())
            {
                outResult = AZStd::string::format("The input port number is not valid for the given node. Node '%s' only has %zu input ports.", targetNode->GetName(), targetNode->GetInputPorts().size());
                return false;
            }

            // check if connection already exists
            if (targetNode->GetHasConnection(sourceNode, static_cast<uint16>(m_sourcePort), static_cast<uint16>(m_targetPort)))
            {
                outResult = AZStd::string::format("The connection you are trying to create already exists!");
                return false;
            }

            // create the connection and auto assign an id first of all
            EMotionFX::BlendTreeConnection* connection = targetNode->AddConnection(sourceNode, static_cast<uint16>(m_sourcePort), static_cast<uint16>(m_targetPort));

            // Overwrite the connection id if specified by a command parameter.
            if (parameters.CheckIfHasParameter("id"))
            {
                m_connectionId = EMotionFX::AnimGraphConnectionId::CreateFromString(parameters.GetValue("id", this));
                connection->SetId(m_connectionId);
            }
            // In case of redo, use the connection id from the previous call.
            else if (m_connectionId.IsValid())
            {
                connection->SetId(m_connectionId);
            }
            // Store the id for redo in case the connection got created with a new id.
            else
            {
                m_connectionId = connection->GetId();
            }

            connection->Reinit();

            if (azrtti_istypeof<EMotionFX::BlendTreeBlendNNode>(targetNode))
            {
                EMotionFX::BlendTreeBlendNNode* blendTreeBlendNNode = static_cast<EMotionFX::BlendTreeBlendNNode*>(targetNode);
                m_updateParamFlag = parameters.GetValueAsBool("updateParam", true);
                if (m_updateParamFlag)
                {
                    blendTreeBlendNNode->UpdateParamWeights();
                }
            }
        }
        else // create a state transition
        {
            // get the state machine
            EMotionFX::AnimGraphStateMachine* machine = (EMotionFX::AnimGraphStateMachine*)targetNode->GetParentNode();

            // try to create the anim graph node
            EMotionFX::AnimGraphObject* object = EMotionFX::AnimGraphObjectFactory::Create(m_transitionType, animGraph);
            if (!object)
            {
                outResult = AZStd::string::format("Cannot create transition of type %s", m_transitionType.ToString<AZStd::string>().c_str());
                return false;
            }

            // check if this is really a transition
            if (!azrtti_istypeof<EMotionFX::AnimGraphStateTransition>(object))
            {
                outResult = AZStd::string::format("Cannot create state transition of type %s, because this object type is not inherited from AnimGraphStateTransition.", m_transitionType.ToString<AZStd::string>().c_str());
                return false;
            }

            // typecast the anim graph node into a state transition node
            EMotionFX::AnimGraphStateTransition* transition = static_cast<EMotionFX::AnimGraphStateTransition*>(object);
            const EMotionFX::AnimGraphConnectionId idAfterCreation = transition->GetId();

            // Deserialize first, manually specified parameters have higher priority and can overwrite the contents.
            if (parameters.CheckIfHasParameter("contents"))
            {
                AZStd::string contents;
                parameters.GetValue("contents", this, &contents);
                MCore::ReflectionSerializer::Deserialize(transition, contents);

                transition->RemoveAllConditions();
                transition->GetTriggerActionSetup().RemoveAllActions();
            }

            // check if we are dealing with a wildcard transition
            bool isWildcardTransition = false;
            if (sourceNode == nullptr)
            {
                isWildcardTransition = true;
            }

            // setup the transition properties
            transition->SetSourceNode(sourceNode);
            transition->SetTargetNode(targetNode);

            // get the offsets
            m_startOffsetX = parameters.GetValueAsInt("startOffsetX", 0);
            m_startOffsetY = parameters.GetValueAsInt("startOffsetY", 0);
            m_endOffsetX = parameters.GetValueAsInt("endOffsetX", 0);
            m_endOffsetY = parameters.GetValueAsInt("endOffsetY", 0);
            if (parameters.CheckIfHasValue("startOffsetX") || parameters.CheckIfHasValue("startOffsetY") || parameters.CheckIfHasValue("endOffsetX") || parameters.CheckIfHasValue("endOffsetY"))
            {
                transition->SetVisualOffsets(m_startOffsetX, m_startOffsetY, m_endOffsetX, m_endOffsetY);
            }
            transition->SetIsWildcardTransition(isWildcardTransition);

            // Overwrite the transition id if specified by a command parameter.
            if (parameters.CheckIfHasParameter("id"))
            {
                m_connectionId = EMotionFX::AnimGraphConnectionId::CreateFromString(parameters.GetValue("id", this));
                transition->SetId(m_connectionId);
            }
            // In case of redo, use the transition id from the previous call.
            else if (m_connectionId.IsValid())
            {
                transition->SetId(m_connectionId);
            }
            // Store the id for redo in case the transition got created with a new id.
            else
            {
                // Reassign the id after creation as the contents parameter might have overwritten the id.
                m_connectionId = idAfterCreation;
                transition->SetId(m_connectionId);
            }

            // add the transition to the state machine
            machine->AddTransition(transition);

            transition->Reinit();
        }

        m_targetNodeId.SetInvalid();
        m_sourceNodeId.SetInvalid();
        if (targetNode)
        {
            m_targetNodeId = targetNode->GetId();
        }
        if (sourceNode)
        {
            m_sourceNodeId = sourceNode->GetId();
        }

        // save the current dirty flag and tell the anim graph that something got changed
        m_oldDirtyFlag = animGraph->GetDirtyFlag();
        animGraph->SetDirtyFlag(true);

        // set the command result to the connection id
        outResult = m_connectionId.ToString().c_str();

        animGraph->RecursiveInvalidateUniqueDatas();
        return true;
    }


    // undo the command
    bool CommandAnimGraphCreateConnection::Undo(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        MCORE_UNUSED(parameters);

        // get the anim graph
        EMotionFX::AnimGraph* animGraph = EMotionFX::GetAnimGraphManager().FindAnimGraphByID(m_animGraphId);
        if (animGraph == nullptr)
        {
            outResult = AZStd::string::format("The anim graph with id '%i' does not exist anymore.", m_animGraphId);
            return false;
        }

        // in case of a wildcard transition the source node is the invalid index, so that's all fine
        EMotionFX::AnimGraphNode* sourceNode = animGraph->RecursiveFindNodeById(m_sourceNodeId);
        EMotionFX::AnimGraphNode* targetNode = animGraph->RecursiveFindNodeById(m_targetNodeId);

        // NOTE: When source node is a nullptr, we are dealing with a wildcard transition, so there a nullptr is allowed.
        if (!targetNode)
        {
            outResult = AZStd::string::format("Target node does not exist!");
            return false;
        }

        // get the source node name, special path here as wildcard transitions have a nullptr source node
        AZStd::string sourceNodeName;
        if (sourceNode)
        {
            sourceNodeName = sourceNode->GetNameString();
        }

        // delete the connection
        const AZStd::string commandString = AZStd::string::format("AnimGraphRemoveConnection -animGraphID %i -targetNode \"%s\" -targetPort %zu -sourceNode \"%s\" -sourcePort %zu -id %s",
            animGraph->GetID(),
            targetNode->GetName(),
            m_targetPort,
            sourceNodeName.c_str(),
            m_sourcePort,
            m_connectionId.ToString().c_str());

        // execute the command without putting it in the history
        if (!GetCommandManager()->ExecuteCommandInsideCommand(commandString, outResult))
        {
            if (!outResult.empty())
            {
                MCore::LogError(outResult.c_str());
            }

            return false;
        }

        // reset the data used for undo and redo
        m_sourceNodeId.SetInvalid();
        m_targetNodeId.SetInvalid();

        // set the dirty flag back to the old value
        animGraph->SetDirtyFlag(m_oldDirtyFlag);
        return true;
    }


    // init the syntax of the command
    void CommandAnimGraphCreateConnection::InitSyntax()
    {
        GetSyntax().ReserveParameters(14);
        GetSyntax().AddRequiredParameter("sourceNode", "The name of the source node, where the connection starts (output port).", MCore::CommandSyntax::PARAMTYPE_STRING);
        GetSyntax().AddRequiredParameter("targetNode", "The name of the target node to connect to (input port).", MCore::CommandSyntax::PARAMTYPE_STRING);
        GetSyntax().AddParameter("animGraphID", "The id of the anim graph to work on.", MCore::CommandSyntax::PARAMTYPE_INT, "-1");
        GetSyntax().AddParameter("sourcePort", "The source port number where the connection starts inside the source node.", MCore::CommandSyntax::PARAMTYPE_INT, "-1");
        GetSyntax().AddParameter("targetPort", "The target port number where the connection connects into, in the target node.", MCore::CommandSyntax::PARAMTYPE_INT, "-1");
        GetSyntax().AddParameter("sourcePortName", "The source port name where the connection starts inside the source node.", MCore::CommandSyntax::PARAMTYPE_STRING, "");
        GetSyntax().AddParameter("targetPortName", "The target port name where the connection connects into, in the target node.", MCore::CommandSyntax::PARAMTYPE_STRING, "");
        GetSyntax().AddParameter("startOffsetX", "The start offset x position, which is the offset to from the upper left corner of the node where the connection starts from.", MCore::CommandSyntax::PARAMTYPE_INT, "0");
        GetSyntax().AddParameter("startOffsetY", "The start offset y position, which is the offset to from the upper left corner of the node where the connection starts from.", MCore::CommandSyntax::PARAMTYPE_INT, "0");
        GetSyntax().AddParameter("endOffsetX", "The end offset x position, which is the offset to from the upper left corner of the node where the connection ends.", MCore::CommandSyntax::PARAMTYPE_INT, "0");
        GetSyntax().AddParameter("endOffsetY", "The end offset y position, which is the offset to from the upper left corner of the node where the connection ends.", MCore::CommandSyntax::PARAMTYPE_INT, "0");
        GetSyntax().AddParameter("id", "The id of the connection.", MCore::CommandSyntax::PARAMTYPE_STRING, "");
        GetSyntax().AddParameter("transitionType", "The transition type ID. This is the type ID (UUID) of the AnimGraphStateTransition inherited node types.", MCore::CommandSyntax::PARAMTYPE_STRING, "");
        GetSyntax().AddParameter("contents", "The serialized contents of the parameter (in reflected XML).", MCore::CommandSyntax::PARAMTYPE_STRING, "");
        GetSyntax().AddParameter("updateParam", "The parameter of the connection, flag whether it needs to be updated.", MCore::CommandSyntax::PARAMTYPE_BOOLEAN, "true");
        GetSyntax().AddParameter("updateUniqueData", "Setting this to true will trigger update on anim graph unique data.", MCore::CommandSyntax::PARAMTYPE_BOOLEAN, "true");
    }


    // get the description
    const char* CommandAnimGraphCreateConnection::GetDescription() const
    {
        return "This command creates a connection between two anim graph nodes.";
    }




    //-------------------------------------------------------------------------------------
    // AnimGraphRemoveConnection - create a connection between two nodes
    //-------------------------------------------------------------------------------------

    // constructor
    CommandAnimGraphRemoveConnection::CommandAnimGraphRemoveConnection(MCore::Command* orgCommand)
        : MCore::Command("AnimGraphRemoveConnection", orgCommand)
    {
        m_sourcePort     = InvalidIndex;
        m_targetPort     = InvalidIndex;
        m_transitionType = AZ::TypeId::CreateNull();
        m_startOffsetX   = 0;
        m_startOffsetY   = 0;
        m_endOffsetX     = 0;
        m_endOffsetY     = 0;
    }


    // destructor
    CommandAnimGraphRemoveConnection::~CommandAnimGraphRemoveConnection()
    {
    }


    // execute
    bool CommandAnimGraphRemoveConnection::Execute(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        // get the anim graph to work on
        EMotionFX::AnimGraph* animGraph = CommandsGetAnimGraph(parameters, this, outResult);
        if (animGraph == nullptr)
        {
            return false;
        }

        // store the anim graph id for undo
        m_animGraphId = animGraph->GetID();

        // get the node names
        AZStd::string sourceNodeName;
        AZStd::string targetNodeName;
        parameters.GetValue("sourceNode", "", sourceNodeName);
        parameters.GetValue("targetNode", "", targetNodeName);

        // find the source node in the anim graph
        EMotionFX::AnimGraphNode* sourceNode = animGraph->RecursiveFindNodeByName(sourceNodeName.c_str());
        //if (sourceNode == nullptr)
        //{
        //  outResult = AZStd::string::format("Cannot find source node with name '%s' in anim graph '%s'", sourceName.AsChar(), animGraph->GetName());
        //  return false;
        //}

        // find the target node in the anim graph
        EMotionFX::AnimGraphNode* targetNode = animGraph->RecursiveFindNodeByName(targetNodeName.c_str());
        if (targetNode == nullptr)
        {
            outResult = AZStd::string::format("Cannot find target node with name '%s' in anim graph '%s'", targetNodeName.c_str(), animGraph->GetFileName());
            return false;
        }

        // get the ids from the source and destination nodes
        m_sourceNodeId.SetInvalid();
        m_sourceNodeName.clear();
        if (sourceNode)
        {
            m_sourceNodeId = sourceNode->GetId();
            m_sourceNodeName = sourceNode->GetName();
        }
        m_targetNodeId = targetNode->GetId();
        m_targetNodeName = targetNode->GetName();

        // get the ports
        m_sourcePort = parameters.GetValueAsInt("sourcePort", 0);
        m_targetPort = parameters.GetValueAsInt("targetPort", 0);

        // get the parent of the source node
        if (targetNode->GetParentNode() == nullptr)
        {
            outResult = AZStd::string::format("Cannot remove connections between root state machines.");
            return false;
        }

        // if the parent is state machine, we don't need to check the port ranges
        if (azrtti_typeid(targetNode->GetParentNode()) != azrtti_typeid<EMotionFX::AnimGraphStateMachine>())
        {
            if (sourceNode == nullptr)
            {
                outResult = AZStd::string::format("Cannot remove blend tree connection in anim graph '%s' as the source node is not valid.", animGraph->GetFileName());
                return false;
            }

            // verify port ranges
            if (m_sourcePort >= static_cast<int32>(sourceNode->GetOutputPorts().size()))
            {
                outResult = AZStd::string::format("The output port number is not valid for the given node. Node '%s' only has %zu output ports.", sourceNode->GetName(), sourceNode->GetOutputPorts().size());
                return false;
            }

            if (m_targetPort >= static_cast<int32>(targetNode->GetInputPorts().size()))
            {
                outResult = AZStd::string::format("The input port number is not valid for the given node. Node '%s' only has %zu input ports.", targetNode->GetName(), targetNode->GetInputPorts().size());
                return false;
            }

            // check if connection already exists
            if (!targetNode->GetHasConnection(sourceNode, static_cast<uint16>(m_sourcePort), static_cast<uint16>(m_targetPort)))
            {
                outResult = AZStd::string::format("The connection you are trying to remove doesn't exist!");
                return false;
            }

            // get the connection ID and store it
            EMotionFX::BlendTreeConnection* connection = targetNode->FindConnection(sourceNode, static_cast<uint16>(m_sourcePort), static_cast<uint16>(m_targetPort));
            if (connection)
            {
                m_connectionId = connection->GetId();
            }

            // create the connection
            targetNode->RemoveConnection(sourceNode, static_cast<uint16>(m_sourcePort), static_cast<uint16>(m_targetPort));

            if (azrtti_istypeof<EMotionFX::BlendTreeBlendNNode>(targetNode))
            {
                EMotionFX::BlendTreeBlendNNode* blendTreeBlendNNode = static_cast<EMotionFX::BlendTreeBlendNNode*>(targetNode);
                blendTreeBlendNNode->UpdateParamWeights();
            }
        }
        else // remove a state transition
        {
            // get the transition id
            EMotionFX::AnimGraphNodeId transitionId;
            if (parameters.CheckIfHasParameter("id"))
            {
                transitionId = EMotionFX::AnimGraphConnectionId::CreateFromString(parameters.GetValue("id", this));
            }
            else
            {
                outResult = AZStd::string::format("You cannot remove a state transition with an invalid id. (Did you specify the id parameter?)");
                return false;
            }

            // get the state machine
            EMotionFX::AnimGraphStateMachine* stateMachine = static_cast<EMotionFX::AnimGraphStateMachine*>(targetNode->GetParentNode());

            const AZ::Outcome<size_t> transitionIndex = stateMachine->FindTransitionIndexById(transitionId);
            if (!transitionIndex.IsSuccess())
            {
                outResult = AZStd::string::format("The state transition you are trying to remove cannot be found.");
                return false;
            }

            // save the transition information for undo
            EMotionFX::AnimGraphStateTransition* transition = stateMachine->GetTransition(transitionIndex.GetValue());
            m_startOffsetX       = transition->GetVisualStartOffsetX();
            m_startOffsetY       = transition->GetVisualStartOffsetY();
            m_endOffsetX         = transition->GetVisualEndOffsetX();
            m_endOffsetY         = transition->GetVisualEndOffsetY();
            m_transitionType     = azrtti_typeid(transition);
            m_connectionId      = transition->GetId();
            m_oldContents        = MCore::ReflectionSerializer::Serialize(transition).GetValue();

            // remove all unique datas for the transition itself
            animGraph->RemoveAllObjectData(transition, true);

            // remove the transition
            stateMachine->RemoveTransition(transitionIndex.GetValue());
        }

        // save the current dirty flag and tell the anim graph that something got changed
        m_oldDirtyFlag = animGraph->GetDirtyFlag();
        animGraph->SetDirtyFlag(true);

        if(parameters.GetValueAsBool("updateUniqueData", true))
        {
            animGraph->RecursiveInvalidateUniqueDatas();
        }
        return true;
    }


    // undo the command
    bool CommandAnimGraphRemoveConnection::Undo(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        const AZStd::string updateUniqueData = parameters.GetValue("updateUniqueData", this);

        // get the anim graph
        EMotionFX::AnimGraph* animGraph = EMotionFX::GetAnimGraphManager().FindAnimGraphByID(m_animGraphId);
        if (animGraph == nullptr)
        {
            outResult = AZStd::string::format("The anim graph with id '%i' does not exist anymore.", m_animGraphId);
            return false;
        }

        if (!m_targetNodeId.IsValid())
        {
            return false;
        }

        AZStd::string commandString = AZStd::string::format("AnimGraphCreateConnection -animGraphID %i -sourceNode \"%s\" -targetNode \"%s\" -sourcePort %zu -targetPort %zu -startOffsetX %d -startOffsetY %d -endOffsetX %d -endOffsetY %d -id %s -transitionType \"%s\" -updateUniqueData %s",
                animGraph->GetID(),
                m_sourceNodeName.c_str(),
                m_targetNodeName.c_str(),
                m_sourcePort,
                m_targetPort,
                m_startOffsetX, m_startOffsetY,
                m_endOffsetX, m_endOffsetY,
                m_connectionId.ToString().c_str(),
                m_transitionType.ToString<AZStd::string>().c_str(),
                updateUniqueData.c_str());

        // add the old attributes
        if (m_oldContents.empty() == false)
        {
            commandString += AZStd::string::format(" -contents {%s}", m_oldContents.c_str());
        }

        if (!GetCommandManager()->ExecuteCommandInsideCommand(commandString, outResult))
        {
            if (!outResult.empty())
            {
                MCore::LogError(outResult.c_str());
            }

            return false;
        }

        m_targetNodeId.SetInvalid();
        m_sourceNodeId.SetInvalid();
        m_connectionId.SetInvalid();
        m_sourcePort     = InvalidIndex;
        m_targetPort     = InvalidIndex;
        m_startOffsetX   = 0;
        m_startOffsetY   = 0;
        m_endOffsetX     = 0;
        m_endOffsetY     = 0;

        // set the dirty flag back to the old value
        animGraph->SetDirtyFlag(m_oldDirtyFlag);
        return true;
    }


    // init the syntax of the command
    void CommandAnimGraphRemoveConnection::InitSyntax()
    {
        // required
        GetSyntax().ReserveParameters(7);
        GetSyntax().AddRequiredParameter("animGraphID", "The id of the anim graph to work on.", MCore::CommandSyntax::PARAMTYPE_INT);
        GetSyntax().AddRequiredParameter("sourceNode", "The name of the source node, where the connection starts (output port).", MCore::CommandSyntax::PARAMTYPE_STRING);
        GetSyntax().AddRequiredParameter("targetNode", "The name of the target node where it connects to (input port).", MCore::CommandSyntax::PARAMTYPE_STRING);
        GetSyntax().AddRequiredParameter("sourcePort", "The source port number where the connection starts inside the source node.", MCore::CommandSyntax::PARAMTYPE_INT);
        GetSyntax().AddRequiredParameter("targetPort", "The target port number where the connection connects into, in the target node.", MCore::CommandSyntax::PARAMTYPE_INT);
        GetSyntax().AddParameter("id", "The id of the connection.", MCore::CommandSyntax::PARAMTYPE_STRING, "");
        GetSyntax().AddParameter("updateUniqueData", "Setting this to true will trigger update on anim graph unique data.", MCore::CommandSyntax::PARAMTYPE_BOOLEAN, "true");
    }


    // get the description
    const char* CommandAnimGraphRemoveConnection::GetDescription() const
    {
        return "This command removes a connection between two anim graph nodes.";
    }



    //-------------------------------------------------------------------------------------
    // CommandAnimGraphAdjustTransition
    //-------------------------------------------------------------------------------------
    void AdjustTransition(const EMotionFX::AnimGraphStateTransition* transition,
        const AZStd::optional<bool>& isDisabled,
        const AZStd::optional<AZStd::string>& sourceNode, const AZStd::optional<AZStd::string>& targetNode,
        const AZStd::optional<AZ::s32>& startOffsetX, const AZStd::optional<AZ::s32>& startOffsetY,
        const AZStd::optional<AZ::s32>& endOffsetX, const AZStd::optional<AZ::s32>& endOffsetY,
        const AZStd::optional<AZStd::string>& attributesString, const AZStd::optional<AZStd::string>& serializedMembers,
        MCore::CommandGroup* commandGroup, bool executeInsideCommand)
    {
        AZStd::string command = AZStd::string::format("%s -%s %i -%s %s",
            CommandAnimGraphAdjustTransition::s_commandName,
            EMotionFX::ParameterMixinAnimGraphId::s_parameterName,
            transition->GetAnimGraph()->GetID(),
            EMotionFX::ParameterMixinTransitionId::s_parameterName,
            transition->GetId().ToString().c_str());

        if (isDisabled)
        {
            command += AZStd::string::format(" -isDisabled %s", AZStd::to_string(isDisabled.value()).c_str());
        }

        if (startOffsetX)
        {
            command += AZStd::string::format(" -startOffsetX %i", startOffsetX.value());
        }
        if (startOffsetY)
        {
            command += AZStd::string::format(" -startOffsetY %i", startOffsetY.value());
        }
        if (endOffsetX)
        {
            command += AZStd::string::format(" -endOffsetX %i", endOffsetX.value());
        }
        if (endOffsetY)
        {
            command += AZStd::string::format(" -endOffsetY %i", endOffsetY.value());
        }

        if (sourceNode)
        {
            command += AZStd::string::format(" -sourceNode \"%s\"", sourceNode.value().c_str());
        }
        if (targetNode)
        {
            command += AZStd::string::format(" -targetNode \"%s\"", targetNode.value().c_str());
        }

        if (attributesString)
        {
            command += AZStd::string::format(" -%s {", EMotionFX::ParameterMixinAttributesString::s_parameterName);
            command += attributesString.value();
            command += "}";
        }

        if (serializedMembers)
        {
            command += AZStd::string::format(" -%s {", EMotionFX::ParameterMixinSerializedMembers::s_parameterName);
            command += serializedMembers.value();
            command += "}";
        }

        CommandSystem::GetCommandManager()->ExecuteCommandOrAddToGroup(command, commandGroup, executeInsideCommand);
    }

    AZ_CLASS_ALLOCATOR_IMPL(CommandAnimGraphAdjustTransition, EMotionFX::CommandAllocator)
    const char* CommandAnimGraphAdjustTransition::s_commandName = "AnimGraphAdjustTransition";

    CommandAnimGraphAdjustTransition::CommandAnimGraphAdjustTransition(MCore::Command* orgCommand)
        : MCore::Command(s_commandName, orgCommand)
    {
    }

    void CommandAnimGraphAdjustTransition::RewindTransitionIfActive(EMotionFX::AnimGraphStateTransition* transition)
    {
        const EMotionFX::AnimGraph* animGraph = transition->GetAnimGraph();
        EMotionFX::AnimGraphStateMachine* stateMachine = transition->GetStateMachine();

        const size_t numAnimGraphInstances = animGraph->GetNumAnimGraphInstances();
        for (size_t i = 0; i < numAnimGraphInstances; ++i)
        {
            EMotionFX::AnimGraphInstance* animGraphInstance = animGraph->GetAnimGraphInstance(i);
            if (stateMachine->IsTransitionActive(transition, animGraphInstance))
            {
                stateMachine->Rewind(animGraphInstance);
            }
        }
    }


    bool CommandAnimGraphAdjustTransition::Execute(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        EMotionFX::AnimGraph* animGraph = GetAnimGraph(outResult);
        if (!animGraph)
        {
            return false;
        }

        EMotionFX::AnimGraphStateTransition* transition = GetTransition(animGraph, outResult);
        if (!transition)
        {
            return false;
        }

        m_oldSerializedMembers = MCore::ReflectionSerializer::SerializeMembersExcept(transition, {"conditions", "actionSetup"});

        // set the new source node
        if (parameters.CheckIfHasParameter("sourceNode"))
        {
            const AZStd::string newSourceName = parameters.GetValue("sourceNode", this);
            EMotionFX::AnimGraphNode* newSourceNode = animGraph->RecursiveFindNodeByName(newSourceName.c_str());
            if (!newSourceNode)
            {
                outResult = AZStd::string::format("Cannot find new source node with name '%s' in anim graph '%s'", newSourceName.c_str(), animGraph->GetFileName());
                return false;
            }

            RewindTransitionIfActive(transition);
            transition->SetSourceNode(newSourceNode);
        }

        // set the new target node
        if (parameters.CheckIfHasParameter("targetNode"))
        {
            const AZStd::string newTargetName = parameters.GetValue("targetNode", this);
            EMotionFX::AnimGraphNode* newTargetNode = animGraph->RecursiveFindNodeByName(newTargetName.c_str());
            if (!newTargetNode)
            {
                outResult = AZStd::string::format("Cannot find new target node with name '%s' in anim graph '%s'", newTargetName.c_str(), animGraph->GetFileName());
                return false;
            }

            RewindTransitionIfActive(transition);
            transition->SetTargetNode(newTargetNode);
        }

        // set the new visual offsets
        if (parameters.CheckIfHasParameter("startOffsetX") && parameters.CheckIfHasParameter("startOffsetY") &&
            parameters.CheckIfHasParameter("endOffsetX") && parameters.CheckIfHasParameter("endOffsetY"))
        {
            const int32 newStartOffsetX = parameters.GetValueAsInt("startOffsetX", this);
            const int32 newStartOffsetY = parameters.GetValueAsInt("startOffsetY", this);
            const int32 newEndOffsetX = parameters.GetValueAsInt("endOffsetX", this);
            const int32 newEndOffsetY = parameters.GetValueAsInt("endOffsetY", this);

            transition->SetVisualOffsets(newStartOffsetX, newStartOffsetY, newEndOffsetX, newEndOffsetY);
        }

        // set the disabled flag
        if (parameters.CheckIfHasParameter("isDisabled"))
        {
            const bool isDisabled = parameters.GetValueAsBool("isDisabled", this);
            transition->SetIsDisabled(isDisabled);
        }

        const AZStd::optional<AZStd::string>& attributesString = ParameterMixinAttributesString::GetAttributesString();
        if (attributesString)
        {
            MCore::ReflectionSerializer::Deserialize(transition, MCore::CommandLine(attributesString.value()));
        }

        const AZStd::optional<AZStd::string>& serializedMembers = ParameterMixinSerializedMembers::GetSerializedMembers();
        if (serializedMembers)
        {
            MCore::ReflectionSerializer::DeserializeMembers(transition, serializedMembers.value());
        }

        // save the current dirty flag and tell the anim graph that something got changed
        m_oldDirtyFlag = animGraph->GetDirtyFlag();
        animGraph->SetDirtyFlag(true);

        transition->Reinit();
        animGraph->RecursiveInvalidateUniqueDatas();

        outResult = m_transitionId.ToString().c_str();
        return true;
    }

    bool CommandAnimGraphAdjustTransition::Undo(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        AZ_UNUSED(parameters);
        EMotionFX::AnimGraph* animGraph = GetAnimGraph(outResult);
        if (!animGraph)
        {
            return false;
        }

        EMotionFX::AnimGraphStateTransition* transition = GetTransition(animGraph, outResult);
        if (!transition)
        {
            return false;
        }

        AdjustTransition(transition,
            /*isDisabled=*/AZStd::nullopt,
            /*sourceNode=*/AZStd::nullopt, /*targetNode=*/AZStd::nullopt,
            /*startOffsetX=*/AZStd::nullopt, /*startOffsetY=*/AZStd::nullopt,
            /*endOffsetX=*/AZStd::nullopt, /*endOffsetY=*/AZStd::nullopt,
            /*attributesString=*/AZStd::nullopt, /*serializedMembers=*/m_oldSerializedMembers.GetValue(),
            /*commandGroup*/nullptr, /*executeInsideCommand*/true);

        animGraph->SetDirtyFlag(m_oldDirtyFlag);
        return true;
    }

    void CommandAnimGraphAdjustTransition::InitSyntax()
    {
        GetSyntax().ReserveParameters(13);

        MCore::CommandSyntax& syntax = GetSyntax();
        ParameterMixinTransitionId::InitSyntax(syntax);
        ParameterMixinAttributesString::InitSyntax(syntax, false);
        ParameterMixinSerializedMembers::InitSyntax(syntax, false);

        GetSyntax().AddParameter("sourceNode", "The new source node of the transition.", MCore::CommandSyntax::PARAMTYPE_STRING, "");
        GetSyntax().AddParameter("targetNode", "The new target node of the transition.", MCore::CommandSyntax::PARAMTYPE_STRING, "");
        GetSyntax().AddParameter("startOffsetX", ".", MCore::CommandSyntax::PARAMTYPE_INT, "0");
        GetSyntax().AddParameter("startOffsetY", ".", MCore::CommandSyntax::PARAMTYPE_INT, "0");
        GetSyntax().AddParameter("endOffsetX", ".", MCore::CommandSyntax::PARAMTYPE_INT, "0");
        GetSyntax().AddParameter("endOffsetY", ".", MCore::CommandSyntax::PARAMTYPE_INT, "0");
        GetSyntax().AddParameter("isDisabled", "False in case the transition shall be active and working, true in case it should be disabled and act like it does not exist.", MCore::CommandSyntax::PARAMTYPE_BOOLEAN, "true");
        GetSyntax().AddParameter("attributesString", "The connection attributes as string.", MCore::CommandSyntax::PARAMTYPE_STRING, "");
        GetSyntax().AddParameter("updateUniqueData", "Setting this to true will trigger update on anim graph unique data.", MCore::CommandSyntax::PARAMTYPE_BOOLEAN, "true");
    }

    bool CommandAnimGraphAdjustTransition::SetCommandParameters(const MCore::CommandLine& parameters)
    {
        ParameterMixinTransitionId::SetCommandParameters(parameters);
        ParameterMixinAttributesString::SetCommandParameters(parameters);
        ParameterMixinSerializedMembers::SetCommandParameters(parameters);
        return true;
    }

    const char* CommandAnimGraphAdjustTransition::GetDescription() const
    {
        return "";
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    void DeleteNodeConnection(MCore::CommandGroup* commandGroup, const EMotionFX::AnimGraphNode* node, const EMotionFX::BlendTreeConnection* connection, bool updateUniqueData)
    {
        const AZStd::string commandString = AZStd::string::format("AnimGraphRemoveConnection -animGraphID %d -targetNode \"%s\" -targetPort %d -sourceNode \"%s\" -sourcePort %d -updateUniqueData %s",
            node->GetAnimGraph()->GetID(),
            node->GetName(),
            connection->GetTargetPort(),
            connection->GetSourceNode()->GetName(),
            connection->GetSourcePort(),
            updateUniqueData ? "true" : "false");
        commandGroup->AddCommandString(commandString);
    }


    void CreateNodeConnection(MCore::CommandGroup* commandGroup, EMotionFX::AnimGraphNode* targetNode, EMotionFX::BlendTreeConnection* connection)
    {
        const AZStd::string commandString = AZStd::string::format("AnimGraphCreateConnection -animGraphID %i -sourceNode \"%s\" -targetNode \"%s\" -sourcePort %d -targetPort %d",
            targetNode->GetAnimGraph()->GetID(),
            connection->GetSourceNode()->GetName(),
            targetNode->GetName(),
            connection->GetSourcePort(),
            connection->GetTargetPort());
        commandGroup->AddCommandString(commandString);
    }


    // Delete a given connection.
    void DeleteConnection(MCore::CommandGroup* commandGroup, EMotionFX::AnimGraphNode* node, EMotionFX::BlendTreeConnection* connection, AZStd::vector<EMotionFX::BlendTreeConnection*>& connectionList)
    {
        // Skip directly if the connection is already in the list.
        if (AZStd::find(connectionList.begin(), connectionList.end(), connection) != connectionList.end())
        {
            return;
        }

        // In case the source node is specified, get the node name from the connection.
        AZStd::string sourceNodeName;
        if (connection->GetSourceNode())
        {
            sourceNodeName = connection->GetSourceNode()->GetName();
        }

        const AZStd::string commandString = AZStd::string::format("AnimGraphRemoveConnection -animGraphID %i -targetNode \"%s\" -targetPort %d -sourceNode \"%s\" -sourcePort %d",
                node->GetAnimGraph()->GetID(),
                node->GetName(),
                connection->GetTargetPort(),
                sourceNodeName.c_str(),
                connection->GetSourcePort());

        connectionList.push_back(connection);
        commandGroup->AddCommandString(commandString);
    }


    // Delete all incoming and outgoing connections for the given node.
    void DeleteNodeConnections(MCore::CommandGroup* commandGroup, EMotionFX::AnimGraphNode* node, EMotionFX::AnimGraphNode* parentNode, AZStd::vector<EMotionFX::BlendTreeConnection*>& connectionList, bool recursive)
    {
        // Delete the connections that start from the given node.
        if (parentNode)
        {
            const size_t numChildNodes = parentNode->GetNumChildNodes();
            for (size_t i = 0; i < numChildNodes; ++i)
            {
                EMotionFX::AnimGraphNode* childNode = parentNode->GetChildNode(i);
                if (childNode == node)
                {
                    continue;
                }

                const size_t numChildConnections = childNode->GetNumConnections();
                for (size_t j = 0; j < numChildConnections; ++j)
                {
                    EMotionFX::BlendTreeConnection* childConnection = childNode->GetConnection(j);

                    // If the connection starts at the given node, delete it.
                    if (childConnection->GetSourceNode() == node)
                    {
                        DeleteConnection(commandGroup, childNode, childConnection, connectionList);
                    }
                }
            }
        }

        // Delete the connections that end in the given node.
        const size_t numConnections = node->GetNumConnections();
        for (size_t i = 0; i < numConnections; ++i)
        {
            EMotionFX::BlendTreeConnection* connection = node->GetConnection(i);
            DeleteConnection(commandGroup, node, connection, connectionList);
        }

        // Recursively delete all connections.
        if (recursive)
        {
            const size_t numChildNodes = node->GetNumChildNodes();
            for (size_t i = 0; i < numChildNodes; ++i)
            {
                EMotionFX::AnimGraphNode* childNode = node->GetChildNode(i);
                DeleteNodeConnections(commandGroup, childNode, node, connectionList, recursive);
            }
        }
    }


    // Relink the given connection from the given node to a new target node.
    void RelinkConnectionTarget(MCore::CommandGroup* commandGroup, const uint32 animGraphID, const char* sourceNodeName, uint32 sourcePortNr, const char* oldTargetNodeName, uint32 oldTargetPortNr, const char* newTargetNodeName, uint32 newTargetPortNr)
    {
        AZStd::string commandString;

        // Delete the old connection first.
        commandString = AZStd::string::format("AnimGraphRemoveConnection -animGraphID %i -targetNode \"%s\" -targetPort %d -sourceNode \"%s\" -sourcePort %d",
                animGraphID,
                oldTargetNodeName,
                oldTargetPortNr,
                sourceNodeName,
                sourcePortNr);

        commandGroup->AddCommandString(commandString);

        // Create the new connection.
        commandString = AZStd::string::format("AnimGraphCreateConnection -animGraphID %i -sourceNode \"%s\" -targetNode \"%s\" -sourcePort %d -targetPort %d",
                animGraphID,
                sourceNodeName,
                newTargetNodeName,
                sourcePortNr,
                newTargetPortNr);

        commandGroup->AddCommandString(commandString);
    }


    // Relink connection to a new source node and or port.
    void RelinkConnectionSource(MCore::CommandGroup* commandGroup, const uint32 animGraphID, const char* oldSourceNodeName, uint32 oldSourcePortNr, const char* newSourceNodeName, uint32 newSourcePortNr, const char* targetNodeName, uint32 targetPortNr)
    {
        AZStd::string commandString;

        // Delete the old connection first.
        commandString = AZStd::string::format("AnimGraphRemoveConnection -animGraphID %i -targetNode \"%s\" -targetPort %d -sourceNode \"%s\" -sourcePort %d",
                animGraphID,
                targetNodeName,
                targetPortNr,
                oldSourceNodeName,
                oldSourcePortNr);

        commandGroup->AddCommandString(commandString);

        // Create the new connection.
        commandString = AZStd::string::format("AnimGraphCreateConnection -animGraphID %i -sourceNode \"%s\" -targetNode \"%s\" -sourcePort %d -targetPort %d",
                animGraphID,
                newSourceNodeName,
                targetNodeName,
                newSourcePortNr,
                targetPortNr);

        commandGroup->AddCommandString(commandString);
    }


    void DeleteStateTransition(MCore::CommandGroup* commandGroup, EMotionFX::AnimGraphStateTransition* transition, AZStd::vector<EMotionFX::AnimGraphStateTransition*>& transitionList)
    {
        // Skip directly if the transition is already in the list.
        if (AZStd::find(transitionList.begin(), transitionList.end(), transition) != transitionList.end())
        {
            return;
        }

        const EMotionFX::AnimGraphNode* sourceNode = transition->GetSourceNode();
        const EMotionFX::AnimGraphNode* targetNode = transition->GetTargetNode();
        const EMotionFX::AnimGraphStateMachine* stateMachine = azdynamic_cast<const EMotionFX::AnimGraphStateMachine*>(targetNode->GetParentNode());

        // Safety check, we need to be working with states, not blend tree nodes.
        if (!stateMachine)
        {
            AZ_Error("EMotionFX", false, "Cannot delete state transition. The anim graph node named '%s' is not a state.", targetNode->GetName());
            return;
        }

        // Remove the transition that is about to be removed from all other transition's can be interrupted by transition id masks.
        AZStd::string attributesString;
        const size_t numTransitions = stateMachine->GetNumTransitions();
        for (size_t i = 0; i < numTransitions; ++i)
        {
            EMotionFX::AnimGraphStateTransition* checkTransition = stateMachine->GetTransition(i);

            AZStd::vector<AZ::u64> canBeInterruptedByTransitionIds = checkTransition->GetCanBeInterruptedByTransitionIds();
            if (checkTransition != transition &&
                AZStd::find(canBeInterruptedByTransitionIds.begin(), canBeInterruptedByTransitionIds.end(), transition->GetId()) != canBeInterruptedByTransitionIds.end() &&
                AZStd::find(transitionList.begin(), transitionList.end(), checkTransition) == transitionList.end()) // Skip in case the transition already got added to the command group to be removed.
            {
                // Get the can be interrupted by transition ids vector and remove the transition about to be
                // removed as well as all the already removed transtions within the same command group from it.
                canBeInterruptedByTransitionIds.erase(AZStd::remove(canBeInterruptedByTransitionIds.begin(),
                    canBeInterruptedByTransitionIds.end(),
                    transition->GetId()),
                    canBeInterruptedByTransitionIds.end());

                for (EMotionFX::AnimGraphStateTransition* alreadyRemovedTranstion : transitionList)
                {
                    canBeInterruptedByTransitionIds.erase(AZStd::remove(canBeInterruptedByTransitionIds.begin(),
                        canBeInterruptedByTransitionIds.end(),
                        alreadyRemovedTranstion->GetId()),
                        canBeInterruptedByTransitionIds.end());
                }

                // Serialize the attribute into a string so we can pass it as a command parameter.
                attributesString = AZStd::string::format("-canBeInterruptedByTransitionIds {%s}",
                    MCore::ReflectionSerializer::Serialize(&canBeInterruptedByTransitionIds).GetValue().c_str());

                // Construct the command and let it adjust the can be interrupted by transition id mask.
                AdjustTransition(checkTransition,
                    /*isDisabled*/AZStd::nullopt, /*sourceNode*/AZStd::nullopt, /*targetNode*/AZStd::nullopt,
                    /*startOffsetX*/AZStd::nullopt, /*startOffsetY*/AZStd::nullopt, /*endOffsetX*/AZStd::nullopt, /*endOffsetY*/AZStd::nullopt,
                    attributesString, /*serializedMembers=*/AZStd::nullopt,
                    commandGroup);
            }
        }

        // Remove transition actions back to front.
        const size_t numActions = transition->GetTriggerActionSetup().GetNumActions();
        for (size_t i = 0; i < numActions; ++i)
        {
            const size_t actionIndex = numActions - i - 1;
            RemoveTransitionAction(transition, actionIndex, commandGroup);
        }

        // Remove transition conditions back to front.
        const size_t numConditions = transition->GetNumConditions();
        for (size_t i = 0; i < numConditions; ++i)
        {
            const size_t conditionIndex = numConditions - i - 1;
            CommandRemoveTransitionCondition* removeConditionCommand = aznew CommandRemoveTransitionCondition(transition->GetAnimGraph()->GetID(), transition->GetId(), conditionIndex);
            commandGroup->AddCommand(removeConditionCommand);
        }

        // If we are dealing with a wildcard transition, reset the source node so that we use the empty name for that.
        if (transition->GetIsWildcardTransition())
        {
            sourceNode = nullptr;
        }

        // If the source node is specified, get the node name.
        AZStd::string sourceNodeName;
        if (sourceNode)
        {
            sourceNodeName = sourceNode->GetName();
        }

        const AZStd::string commandString = AZStd::string::format("AnimGraphRemoveConnection -animGraphID %i -sourceNode \"%s\" -targetNode \"%s\" -targetPort 0 -sourcePort 0 -id %s",
            targetNode->GetAnimGraph()->GetID(),
            sourceNodeName.c_str(),
            targetNode->GetName(),
            transition->GetId().ToString().c_str());
        commandGroup->AddCommandString(commandString);

        transitionList.push_back(transition);
    }


    // Delete all incoming and outgoing transitions for the given node.
    void DeleteStateTransitions(MCore::CommandGroup* commandGroup, EMotionFX::AnimGraphNode* state, EMotionFX::AnimGraphNode* parentNode, AZStd::vector<EMotionFX::AnimGraphStateTransition*>& transitionList, bool recursive)
    {
        // Only do for state machines.
        if (parentNode && azrtti_typeid(parentNode) == azrtti_typeid<EMotionFX::AnimGraphStateMachine>())
        {
            EMotionFX::AnimGraphStateMachine* stateMachine = static_cast<EMotionFX::AnimGraphStateMachine*>(parentNode);

            const size_t numTransitions = stateMachine->GetNumTransitions();
            for (size_t j = 0; j < numTransitions; ++j)
            {
                EMotionFX::AnimGraphStateTransition* transition = stateMachine->GetTransition(j);
                const EMotionFX::AnimGraphNode* sourceNode = transition->GetSourceNode();
                const EMotionFX::AnimGraphNode* targetNode = transition->GetTargetNode();

                // If the connection starts at the given node, delete it.
                if (targetNode == state || (!transition->GetIsWildcardTransition() && sourceNode == state))
                {
                    DeleteStateTransition(commandGroup, transition, transitionList);
                }
            }
        }

        // Recursively delete all transitions.
        if (recursive)
        {
            const size_t numChildNodes = state->GetNumChildNodes();
            for (size_t i = 0; i < numChildNodes; ++i)
            {
                EMotionFX::AnimGraphNode* childNode = state->GetChildNode(i);
                DeleteStateTransitions(commandGroup, childNode, state, transitionList, recursive);
            }
        }
    }


    void CopyStateTransition(MCore::CommandGroup* commandGroup, EMotionFX::AnimGraph* targetAnimGraph, EMotionFX::AnimGraphStateTransition* transition,
        bool cutMode, AZStd::unordered_map<AZ::u64, AZ::u64>& convertedIds, AnimGraphCopyPasteData& copyPasteData)
    {
        EMotionFX::AnimGraphNode* sourceNode = transition->GetSourceNode();
        EMotionFX::AnimGraphNode* targetNode = transition->GetTargetNode();

        // We only copy transitions that are between nodes that are copied. Otherwise, the transition doesn't have a valid origin/target. If the transition is a wildcard
        // we only need the target.
        if (copyPasteData.m_newNamesByCopiedNodes.find(targetNode) == copyPasteData.m_newNamesByCopiedNodes.end() ||
            (!transition->GetIsWildcardTransition() && copyPasteData.m_newNamesByCopiedNodes.find(sourceNode) == copyPasteData.m_newNamesByCopiedNodes.end()))
        {
            return;
        }

        AZStd::string sourceName;
        if (!transition->GetIsWildcardTransition() && sourceNode)
        {
            // In case the source node is going to get copied too get the new name, if not just use name of the source node of the connection.
            sourceName = copyPasteData.GetNewNodeName(sourceNode, cutMode);
        }

        const AZStd::string targetName = copyPasteData.GetNewNodeName(targetNode, cutMode);
        const EMotionFX::AnimGraphConnectionId newTransitionId = copyPasteData.GetNewConnectionId(transition->GetId(), cutMode);
        
        // Relink the interruption candidates, serialize the transition contents and set it back to its original state.
        const AZStd::vector<AZ::u64> oldCanBeInterruptedByTransitionIds = transition->GetCanBeInterruptedByTransitionIds();
        AZStd::vector<AZ::u64> canBeInterruptedByTransitionIds = transition->GetCanBeInterruptedByTransitionIds();
        for (AZ::u64& id : canBeInterruptedByTransitionIds)
        {
            id = copyPasteData.GetNewConnectionId(id, cutMode);
        }
        transition->SetCanBeInterruptedBy(canBeInterruptedByTransitionIds);

        const AZStd::string serializedTransition = MCore::ReflectionSerializer::Serialize(transition).GetValue();
        transition->SetCanBeInterruptedBy(oldCanBeInterruptedByTransitionIds);


        AZStd::string commandString = AZStd::string::format("AnimGraphCreateConnection -animGraphID %d -sourceNode \"%s\" -targetNode \"%s\" -sourcePort %d -targetPort %d -transitionType \"%s\" -id %s -contents {%s}",
                targetAnimGraph->GetID(),
                sourceName.c_str(),
                targetName.c_str(),
                0, // sourcePort
                0, // targetPort
                azrtti_typeid(transition).ToString<AZStd::string>().c_str(),
                newTransitionId.ToString().c_str(),
                serializedTransition.c_str());
        commandGroup->AddCommandString(commandString);

        // Find the name of the state machine
        EMotionFX::AnimGraphStateMachine* stateMachine = static_cast<EMotionFX::AnimGraphStateMachine*>(targetNode->GetParentNode());
        const AZStd::string stateMachineName = copyPasteData.GetNewNodeName(stateMachine, cutMode);

        if (!cutMode)
        {
            AZStd::string attributesString;
            transition->GetAttributeStringForAffectedNodeIds(convertedIds, attributesString);
            if (!attributesString.empty())
            {
                commandString = AZStd::string::format("%s -%s %d -%s %s -attributesString {%s}",
                    CommandAnimGraphAdjustTransition::s_commandName,
                    EMotionFX::ParameterMixinAnimGraphId::s_parameterName, targetAnimGraph->GetID(),
                    EMotionFX::ParameterMixinTransitionId::s_parameterName, newTransitionId.ToString().c_str(),
                    attributesString.c_str());
                commandGroup->AddCommandString(commandString);
            }
        }

        const size_t numConditions = transition->GetNumConditions();
        for (size_t i = 0; i < numConditions; ++i)
        {
            EMotionFX::AnimGraphTransitionCondition* condition = transition->GetCondition(i);
            const AZ::TypeId conditionType = azrtti_typeid(condition);

            commandString = AZStd::string::format("%s -%s %d -%s %s -conditionType \"%s\" -contents {%s}",
                CommandSystem::CommandAddTransitionCondition::s_commandName,
                EMotionFX::ParameterMixinAnimGraphId::s_parameterName, targetAnimGraph->GetID(),
                EMotionFX::ParameterMixinTransitionId::s_parameterName, newTransitionId.ToString().c_str(),
                conditionType.ToString<AZStd::string>().c_str(),
                MCore::ReflectionSerializer::Serialize(condition).GetValue().c_str());
            commandGroup->AddCommandString(commandString);

            if (!cutMode)
            {
                AZStd::string attributesString;
                condition->GetAttributeStringForAffectedNodeIds(convertedIds, attributesString);

                if (!attributesString.empty())
                {
                    CommandSystem::CommandAdjustTransitionCondition* adjustConditionCommand = aznew CommandSystem::CommandAdjustTransitionCondition(
                        targetAnimGraph->GetID(),
                        newTransitionId,
                        /*conditionIndex=*/i,
                        attributesString);
                    commandGroup->AddCommand(adjustConditionCommand);
                }
            }
        }

        const AZStd::vector<EMotionFX::AnimGraphTriggerAction*> actions = transition->GetTriggerActionSetup().GetActions();
        for (const EMotionFX::AnimGraphTriggerAction* action : actions)
        {
            CommandSystem::AddTransitionAction(targetAnimGraph->GetID(),
                newTransitionId.ToString().c_str(),
                azrtti_typeid(action),
                MCore::ReflectionSerializer::Serialize(action).GetValue(),
                /*insertAt=*/AZStd::nullopt,
                commandGroup);
        }
    }


    void CopyBlendTreeConnection(MCore::CommandGroup* commandGroup, EMotionFX::AnimGraph* targetAnimGraph, EMotionFX::AnimGraphNode* targetNode, EMotionFX::BlendTreeConnection* connection,
        bool cutMode, AZStd::unordered_map<AZ::u64, AZ::u64>& convertedIds, AnimGraphCopyPasteData& copyPasteData)
    {
        AZ_UNUSED(convertedIds);
        EMotionFX::AnimGraphNode* sourceNode = connection->GetSourceNode();
        if (!sourceNode)
        {
            return;
        }

        // Only copy connections that are between nodes that are copied.
        if (copyPasteData.m_newNamesByCopiedNodes.find(sourceNode) == copyPasteData.m_newNamesByCopiedNodes.end() ||
            copyPasteData.m_newNamesByCopiedNodes.find(targetNode) == copyPasteData.m_newNamesByCopiedNodes.end())
        {
            return;
        }

        const AZStd::string sourceNodeName = copyPasteData.GetNewNodeName(sourceNode, cutMode);
        const AZStd::string targetNodeName = copyPasteData.GetNewNodeName(targetNode, cutMode);
        const uint16 sourcePort = connection->GetSourcePort();
        const uint16 targetPort = connection->GetTargetPort();
        
        const AZStd::string commandString = AZStd::string::format("AnimGraphCreateConnection -animGraphID %u -sourceNode \"%s\" -targetNode \"%s\" -sourcePortName \"%s\" -targetPortName \"%s\" -updateParam \"false\"",
                targetAnimGraph->GetID(),
                sourceNodeName.c_str(),
                targetNodeName.c_str(),
                sourceNode->GetOutputPort(sourcePort).GetName(),
                targetNode->GetInputPort(targetPort).GetName());
        commandGroup->AddCommandString(commandString);
    }
} // namesapce EMotionFX
