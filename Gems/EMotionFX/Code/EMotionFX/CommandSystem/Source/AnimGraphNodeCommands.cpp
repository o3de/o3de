/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "AnimGraphNodeCommands.h"
#include "AnimGraphConnectionCommands.h"
#include "CommandManager.h"

#include <EMotionFX/CommandSystem/Source/AnimGraphTriggerActionCommands.h>
#include <EMotionFX/CommandSystem/Source/AnimGraphNodeGroupCommands.h>
#include <EMotionFX/Source/ActorInstance.h>
#include <EMotionFX/Source/ActorManager.h>
#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/Source/AnimGraphEntryNode.h>
#include <EMotionFX/Source/AnimGraphExitNode.h>
#include <EMotionFX/Source/AnimGraphInstance.h>
#include <EMotionFX/Source/AnimGraphManager.h>
#include <EMotionFX/Source/AnimGraphMotionCondition.h>
#include <EMotionFX/Source/AnimGraphMotionNode.h>
#include <EMotionFX/Source/AnimGraphNode.h>
#include <EMotionFX/Source/AnimGraphNodeGroup.h>
#include <EMotionFX/Source/AnimGraphObjectFactory.h>
#include <EMotionFX/Source/AnimGraphStateMachine.h>
#include <EMotionFX/Source/BlendTree.h>
#include <EMotionFX/Source/BlendTreeParameterNode.h>
#include <MCore/Source/LogManager.h>
#include <MCore/Source/ReflectionSerializer.h>

namespace CommandSystem
{
    AZStd::string GenerateUniqueNodeName(EMotionFX::AnimGraph* animGraph, const AZStd::string& namePrefix, const AZStd::string& typeString, const AZStd::unordered_set<AZStd::string>& nameReserveList = AZStd::unordered_set<AZStd::string>())
    {
        AZStd::string nameResult;

        if (namePrefix.empty())
        {
            nameResult = animGraph->GenerateNodeName(nameReserveList, typeString.c_str());
        }
        else
        {
            nameResult = animGraph->GenerateNodeName(nameReserveList, namePrefix.c_str());
        }

        // remove the AnimGraph prefix from the node names
        AzFramework::StringFunc::Replace(nameResult, "AnimGraph", "", true /* case sensitive */);

        // also remove the BlendTree prefix from all other nodes
        if (AzFramework::StringFunc::Equal(typeString.c_str(), "BlendTree", false /* no case */) == false)
        {
            AzFramework::StringFunc::Replace(nameResult, "BlendTree", "", true /* case sensitive */);
        }

        return nameResult;
    }

    AZStd::string GenerateUniqueNodeName(EMotionFX::AnimGraph* animGraph, EMotionFX::AnimGraphNode* node, const AZStd::unordered_set<AZStd::string>& nameReserveList)
    {
        if (node == nullptr)
        {
            return AZStd::string();
        }

        AZStd::string namePrefix = node->GetName();
        AZStd::string typeString = node->RTTI_GetTypeName();

        const char* nameReadPtr = namePrefix.c_str();
        size_t newLength = namePrefix.size();
        for (size_t i = newLength; i > 0; i--)
        {
            char currentChar = nameReadPtr[i - 1];
            if (isdigit(currentChar) != 0)
            {
                newLength--;
            }
            else
            {
                break;
            }
        }

        if (newLength > 0)
        {
            namePrefix.resize(newLength);
        }

        return GenerateUniqueNodeName(animGraph, namePrefix, typeString, nameReserveList);
    }

    //-------------------------------------------------------------------------------------
    // Create a anim graph node
    //-------------------------------------------------------------------------------------

    // constructor
    CommandAnimGraphCreateNode::CommandAnimGraphCreateNode(MCore::Command* orgCommand)
        : MCore::Command("AnimGraphCreateNode", orgCommand)
    {
    }

    // destructor
    CommandAnimGraphCreateNode::~CommandAnimGraphCreateNode()
    {
    }

    EMotionFX::AnimGraphNodeId CommandAnimGraphCreateNode::GetNodeId(const MCore::CommandLine& parameters)
    {
        if (parameters.CheckIfHasParameter("nodeId"))
        {
            const AZStd::string nodeIdString = parameters.GetValue("nodeId", this);
            return EMotionFX::AnimGraphNodeId::CreateFromString(nodeIdString);
        }

        return m_nodeId;
    }

    void CommandAnimGraphCreateNode::DeleteGraphNode(EMotionFX::AnimGraphNode* node)
    {
        EMotionFX::AnimGraph* animGraph = node->GetAnimGraph();
        if (animGraph)
        {
            animGraph->RemoveAllObjectData(node, true);
        }
        EMotionFX::AnimGraphNode* parentNode = node->GetParentNode();
        if (parentNode)
        {
            parentNode->RemoveChildNodeByPointer(node);
        }
        delete node;
    }

    // execute
    bool CommandAnimGraphCreateNode::Execute(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        // get the anim graph to work on
        EMotionFX::AnimGraph* animGraph = CommandsGetAnimGraph(parameters, this, outResult);
        if (animGraph == nullptr)
        {
            return false;
        }

        // store the anim graph id for undo
        m_animGraphId = animGraph->GetID();

        // find the graph
        EMotionFX::AnimGraphNode* parentNode = nullptr;
        AZStd::string parentNodeName;
        parameters.GetValue("parentName", "", parentNodeName);
        if (!parentNodeName.empty())
        {
            parentNode = animGraph->RecursiveFindNodeByName(parentNodeName.c_str());
            if (!parentNode)
            {
                outResult = AZStd::string::format("There is no anim graph node with name '%s' inside the selected/active anim graph.", parentNodeName.c_str());
                return false;
            }
        }

        // get the type of the node to be created
        const AZ::Outcome<AZStd::string> typeString = parameters.GetValueIfExists("type", this);
        const AZ::TypeId nodeType = typeString.IsSuccess() ? AZ::TypeId::CreateString(typeString.GetValue().c_str(), typeString.GetValue().size()) : AZ::TypeId::CreateNull();
        if (nodeType.IsNull())
        {
            outResult = AZStd::string::format("Cannot create node of type %s", nodeType.ToString<AZStd::string>().c_str());
            return false;
        }
        if (parentNodeName.empty() && nodeType != azrtti_typeid<EMotionFX::AnimGraphStateMachine>())
        {
            outResult = "Cannot create node. Root nodes can only be of type AnimGraphStateMachine.";
            return false;
        }

        // get the name check if the name is unique or not
        AZStd::string name;
        parameters.GetValue("name", "", name);
        if (!name.empty())
        {
            // if there is already a node with the same name
            if (animGraph->RecursiveFindNodeByName(name.c_str()))
            {
                outResult = AZStd::string::format("Failed to create node, as there is already a node with name '%s'", name.c_str());
                return false;
            }
        }

        // try to create the node of the given type
        EMotionFX::AnimGraphObject* object = EMotionFX::AnimGraphObjectFactory::Create(nodeType, animGraph);
        if (!object)
        {
            outResult = AZStd::string::format("Failed to create node of type '%s'", typeString.GetValue().c_str());
            return false;
        }

        // check if it is really a node
        if (!azrtti_istypeof<EMotionFX::AnimGraphNode>(object))
        {
            outResult = AZStd::string::format("Failed to create node of type '%s' as it is no AnimGraphNode inherited object.", typeString.GetValue().c_str());
            return false;
        }

        // convert the object into a node
        EMotionFX::AnimGraphNode* node = static_cast<EMotionFX::AnimGraphNode*>(object);

        // store the node id for the callbacks
        m_nodeId = node->GetId();

        if (parameters.CheckIfHasParameter("contents"))
        {
            AZStd::string contents;
            parameters.GetValue("contents", this, &contents);
            MCore::ReflectionSerializer::DeserializeMembers(node, contents);

            // The deserialize method will deserialize back the old id
            node->SetId(m_nodeId);

            // Verify we have not serialized connections, child nodes and transitions
            AZ_Assert(node->GetNumConnections() == 0, "Unexpected serialized connections");
            AZ_Assert(node->GetNumChildNodes() == 0, "Unexpected serialized child nodes");
            AZ_Assert(azrtti_typeid(node) != azrtti_typeid<EMotionFX::AnimGraphStateMachine>() || static_cast<EMotionFX::AnimGraphStateMachine*>(node)->GetNumTransitions() == 0, "Unexpected serialized transitions");

            // After deserialization the trigger actions also need to be initialized.
            node->InitTriggerActions();
        }

        // Force set the node id. Undo of the remove node command calls a create node command which has to force set the node id when reconstructing it.
        // Else wise all linked objects like transition conditions will be linked to an invalid node.
        if (parameters.CheckIfHasParameter("nodeId"))
        {
            const AZStd::string nodeIdString = parameters.GetValue("nodeId", this);

            const EMotionFX::AnimGraphNodeId nodeId = EMotionFX::AnimGraphNodeId::CreateFromString(nodeIdString);
            node->SetId(nodeId);
            m_nodeId = nodeId;
        }

        // if the name is not empty, set it
        if (!name.empty())
        {
            if (AzFramework::StringFunc::Equal(name.c_str(), "GENERATE", false /* no case */))
            {
                AZStd::string namePrefix;
                parameters.GetValue("namePrefix", this, &namePrefix);
                name = GenerateUniqueNodeName(animGraph, namePrefix, node->RTTI_GetTypeName());
            }

            node->SetName(name.c_str());
        }

        // Avoid adding a node with an id that another node in the anim graph already has.
        if (animGraph->RecursiveFindNodeById(node->GetId()) != nullptr)
        {
            outResult = AZStd::string::format("Node with id '%s' and type '%s' cannot be added to anim graph. Node with the given id already exists.", node->GetId().ToString().c_str(), typeString.GetValue().c_str());
            DeleteGraphNode(node);
            return false;
        }

        // if its a root node it has to be a state machine
        if (parentNode == nullptr)
        {
            if (azrtti_typeid(node) != azrtti_typeid<EMotionFX::AnimGraphStateMachine>())
            {
                outResult = AZStd::string::format("Nodes without parents are only allowed to be state machines, cancelling creation!");
                DeleteGraphNode(node);
                return false;
            }
        }
        else
        {
            if (azrtti_typeid(parentNode) == azrtti_typeid<EMotionFX::AnimGraphStateMachine>())
            {
                if (!node->GetCanActAsState())
                {
                    outResult = AZStd::string::format("Node with name '%s' cannot be added to state machine as the node with type '%s' can not act as a state.", name.c_str(), typeString.GetValue().c_str());
                    DeleteGraphNode(node);
                    return false;
                }

                // Handle node types that are only allowed once as a child.
                if (node->GetCanHaveOnlyOneInsideParent() && parentNode->HasChildNodeOfType(azrtti_typeid(node)))
                {
                    outResult = AZStd::string::format("Node with name '%s' and type '%s' cannot be added to state machine as a node with the given type already exists. Multiple nodes of this type per state machine are not allowed.", name.c_str(), typeString.GetValue().c_str());
                    DeleteGraphNode(node);
                    return false;
                }
            }
        }

        // now that the node is created, adjust its position
        const int32 xPos = parameters.GetValueAsInt("xPos", this);
        const int32 yPos = parameters.GetValueAsInt("yPos", this);
        node->SetVisualPos(xPos, yPos);

        // set the new value to the enabled flag
        if (parameters.CheckIfHasParameter("enabled"))
        {
            node->SetIsEnabled(parameters.GetValueAsBool("enabled", this));
        }

        // set the new value to the visualization flag
        if (parameters.CheckIfHasParameter("visualize"))
        {
            node->SetVisualization(parameters.GetValueAsBool("visualize", this));
        }

        // set the attributes from a string
        if (parameters.CheckIfHasParameter("attributesString"))
        {
            AZStd::string attributesString;
            parameters.GetValue("attributesString", this, &attributesString);
            MCore::ReflectionSerializer::Deserialize(node, MCore::CommandLine(attributesString));
        }

        // collapse the node if expected
        const bool collapsed = parameters.GetValueAsBool("collapsed", this);
        node->SetIsCollapsed(collapsed);

        // store the anim graph id for undo
        m_animGraphId = animGraph->GetID();

        // check if the parent is valid
        if (parentNode)
        {
            // add the node in the parent
            node->SetParentNode(parentNode);
            parentNode->AddChildNode(node);

            // in case the parent node is a state machine
            if (azrtti_typeid(parentNode) == azrtti_typeid<EMotionFX::AnimGraphStateMachine>())
            {
                // type cast the parent node to a state machine
                EMotionFX::AnimGraphStateMachine* stateMachine = static_cast<EMotionFX::AnimGraphStateMachine*>(parentNode);

                // in case this is the first state we add to the state machine, default it to the entry state
                if (stateMachine->GetNumChildNodes() == 1)
                {
                    stateMachine->SetEntryState(node);
                }
            }
        }
        else
        {
            //animGraph->AddStateMachine( (EMotionFX::AnimGraphStateMachine*)node );
            MCORE_ASSERT(false);
            MCore::LogError("Cannot add node at root level.");
        }

        // handle blend tree final node separately
        if (parentNode && azrtti_typeid(parentNode) == azrtti_typeid<EMotionFX::BlendTree>())
        {
            if (node && azrtti_typeid(node) == azrtti_typeid<EMotionFX::BlendTreeFinalNode>())
            {
                EMotionFX::BlendTree* blendTree = static_cast<EMotionFX::BlendTree*>(parentNode);
                blendTree->SetFinalNodeId(node->GetId());
            }
        }

        // save the current dirty flag and tell the anim graph that something got changed
        m_oldDirtyFlag = animGraph->GetDirtyFlag();
        animGraph->SetDirtyFlag(true);

        // return the node name
        outResult = node->GetName();

        // call the post create node event
        EMotionFX::GetEventManager().OnCreatedNode(animGraph, node);

        node->Reinit();
        animGraph->RecursiveInvalidateUniqueDatas();

        // init new node for all anim graph instances belonging to it
        const size_t numActorInstances = EMotionFX::GetActorManager().GetNumActorInstances();
        for (size_t i = 0; i < numActorInstances; ++i)
        {
            EMotionFX::AnimGraphInstance* animGraphInstance = EMotionFX::GetActorManager().GetActorInstance(i)->GetAnimGraphInstance();
            if (animGraphInstance && animGraphInstance->GetAnimGraph() == animGraph)
            {
                // activate the state automatically in all animgraph instances
                if (azrtti_typeid(parentNode) == azrtti_typeid<EMotionFX::AnimGraphStateMachine>())
                {
                    if (parentNode->GetNumChildNodes() == 1)
                    {
                        EMotionFX::AnimGraphStateMachine* stateMachine = static_cast<EMotionFX::AnimGraphStateMachine*>(parentNode);
                        stateMachine->SwitchToState(animGraphInstance, node);
                    }
                }
            }
        }

        return true;
    }

    // undo the command
    bool CommandAnimGraphCreateNode::Undo(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        MCORE_UNUSED(parameters);

        // get the anim graph
        EMotionFX::AnimGraph* animGraph = EMotionFX::GetAnimGraphManager().FindAnimGraphByID(m_animGraphId);
        if (animGraph == nullptr)
        {
            outResult = AZStd::string::format("The anim graph with id '%i' does not exist anymore.", m_animGraphId);
            return false;
        }

        // locate the node
        EMotionFX::AnimGraphNode* node = animGraph->RecursiveFindNodeById(m_nodeId);
        if (node == nullptr)
        {
            return false;
        }

        m_nodeId.SetInvalid();

        const AZStd::string commandString = AZStd::string::format("AnimGraphRemoveNode -animGraphID %i -name \"%s\"", animGraph->GetID(), node->GetName());
        if (GetCommandManager()->ExecuteCommandInsideCommand(commandString, outResult) == false)
        {
            if (!outResult.empty())
            {
                MCore::LogError(outResult.c_str());
            }
            return false;
        }

        // set the dirty flag back to the old value
        animGraph->SetDirtyFlag(m_oldDirtyFlag);
        return true;
    }

    // init the syntax of the command
    void CommandAnimGraphCreateNode::InitSyntax()
    {
        // required
        GetSyntax().ReserveParameters(12);
        GetSyntax().AddRequiredParameter("type", "The type of the node (UUID).", MCore::CommandSyntax::PARAMTYPE_STRING);
        GetSyntax().AddParameter("animGraphID", "The id of the anim graph to work on.", MCore::CommandSyntax::PARAMTYPE_INT, "-1");
        GetSyntax().AddParameter("parentName", "The name of the parent node to add it to.", MCore::CommandSyntax::PARAMTYPE_STRING, "");
        GetSyntax().AddParameter("name", "The name of the node, set to GENERATE to automatically generate a unique name.", MCore::CommandSyntax::PARAMTYPE_STRING, "");
        GetSyntax().AddParameter("nodeId", "The unique node id of the new node.", MCore::CommandSyntax::PARAMTYPE_STRING, "");
        GetSyntax().AddParameter("xPos", "The x position of the upper left corner in the visual graph.", MCore::CommandSyntax::PARAMTYPE_INT, "0");
        GetSyntax().AddParameter("yPos", "The y position of the upper left corner in the visual graph.", MCore::CommandSyntax::PARAMTYPE_INT, "0");
        GetSyntax().AddParameter("collapsed", "The node collapse flag. This is only for the visual representation and does not affect the functionality.", MCore::CommandSyntax::PARAMTYPE_BOOLEAN, "false");
        GetSyntax().AddParameter("center", "Center the created node around the mouse cursor or not.", MCore::CommandSyntax::PARAMTYPE_BOOLEAN, "true");
        GetSyntax().AddParameter("namePrefix", "The prefix of the name, when the name is set to GENERATE.", MCore::CommandSyntax::PARAMTYPE_STRING, "");
        GetSyntax().AddParameter("attributesString", "The node attributes as string.", MCore::CommandSyntax::PARAMTYPE_STRING, "");
        GetSyntax().AddParameter("contents", "The serialized contents of the parameter (in reflected XML).", MCore::CommandSyntax::PARAMTYPE_STRING, "");
        GetSyntax().AddParameter("enabled", "Is the node enabled?", MCore::CommandSyntax::PARAMTYPE_BOOLEAN, "true");
        GetSyntax().AddParameter("visualize", "Is the node visualized?", MCore::CommandSyntax::PARAMTYPE_BOOLEAN, "false");
    }

    // get the description
    const char* CommandAnimGraphCreateNode::GetDescription() const
    {
        return "This command creates a anim graph node of a given type. It returns the node name if successful.";
    }

    //-------------------------------------------------------------------------------------
    // AnimGraphAdjustNode - adjust node settings
    //-------------------------------------------------------------------------------------

    // constructor
    CommandAnimGraphAdjustNode::CommandAnimGraphAdjustNode(MCore::Command* orgCommand)
        : MCore::Command("AnimGraphAdjustNode", orgCommand)
    {
        m_oldPosX = 0;
        m_oldPosY = 0;
    }

    // destructor
    CommandAnimGraphAdjustNode::~CommandAnimGraphAdjustNode()
    {
    }

    // execute
    bool CommandAnimGraphAdjustNode::Execute(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        // get the anim graph to work on
        EMotionFX::AnimGraph* animGraph = CommandsGetAnimGraph(parameters, this, outResult);
        if (animGraph == nullptr)
        {
            return false;
        }

        // store the anim graph id for undo
        m_animGraphId = animGraph->GetID();

        // get the name of the node
        AZStd::string name;
        parameters.GetValue("name", "", name);

        // find the node in the anim graph
        EMotionFX::AnimGraphNode* node = animGraph->RecursiveFindNodeByName(name.c_str());
        if (!node)
        {
            outResult = AZStd::string::format("Cannot find node with name '%s' in anim graph '%s'", name.c_str(), animGraph->GetFileName());
            return false;
        }

        if (parameters.CheckIfHasParameter("attributesString"))
        {
            const AZStd::string attributesString = parameters.GetValue("attributesString", this);
            MCore::ReflectionSerializer::Deserialize(node, MCore::CommandLine(attributesString));
        }

        // get the x and y pos
        int32 xPos = node->GetVisualPosX();
        int32 yPos = node->GetVisualPosY();
        m_oldPosX = xPos;
        m_oldPosY = yPos;

        // get the new position values
        if (parameters.CheckIfHasParameter("xPos"))
        {
            xPos = parameters.GetValueAsInt("xPos", this);
        }

        if (parameters.CheckIfHasParameter("yPos"))
        {
            yPos = parameters.GetValueAsInt("yPos", this);
        }

        node->SetVisualPos(xPos, yPos);

        // set the new name
        AZStd::string newName;
        parameters.GetValue("newName", this, newName);
        if (!newName.empty())
        {
            // find the node group the node was in before the name change
            EMotionFX::AnimGraphNodeGroup* nodeGroup = animGraph->FindNodeGroupForNode(node);
            m_nodeGroupName.clear();
            if (nodeGroup)
            {
                // remember the node group name for undo
                m_nodeGroupName = nodeGroup->GetName();

                // remove the node from the node group as its id is going to change
                nodeGroup->RemoveNodeById(node->GetId());
            }

            m_oldName = node->GetName();
            node->SetName(newName.c_str());

            // as the id of the node changed after renaming it, we have to readd the node with the new id
            if (nodeGroup)
            {
                nodeGroup->AddNode(node->GetId());
            }

            // call the post rename node event
            EMotionFX::GetEventManager().OnRenamedNode(animGraph, node, m_oldName.c_str());
        }

        // remember and set the new value to the enabled flag
        m_oldEnabled = node->GetIsEnabled();
        if (parameters.CheckIfHasParameter("enabled"))
        {
            node->SetIsEnabled(parameters.GetValueAsBool("enabled", this));
        }

        // remember and set the new value to the visualization flag
        m_oldVisualized = node->GetIsVisualizationEnabled();
        if (parameters.CheckIfHasParameter("visualize"))
        {
            node->SetVisualization(parameters.GetValueAsBool("visualize", this));
        }

        m_nodeId = node->GetId();

        // save the current dirty flag and tell the anim graph that something got changed
        m_oldDirtyFlag = animGraph->GetDirtyFlag();
        animGraph->SetDirtyFlag(true);

        // only update attributes in case it is wanted
        if (parameters.GetValueAsBool("updateAttributes", this))
        {
            node->Reinit();
            animGraph->RecursiveInvalidateUniqueDatas();
        }

        return true;
    }

    // undo the command
    bool CommandAnimGraphAdjustNode::Undo(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        // get the anim graph
        EMotionFX::AnimGraph* animGraph = EMotionFX::GetAnimGraphManager().FindAnimGraphByID(m_animGraphId);
        if (animGraph == nullptr)
        {
            outResult = AZStd::string::format("The anim graph with id '%i' does not exist anymore.", m_animGraphId);
            return false;
        }

        EMotionFX::AnimGraphNode* node = animGraph->RecursiveFindNodeById(m_nodeId);
        if (node == nullptr)
        {
            outResult = AZStd::string::format("Cannot find node with ID %s.", m_nodeId.ToString().c_str());
            return false;
        }

        // restore the name
        if (!m_oldName.empty())
        {
            AZStd::string currentName = node->GetName();

            // find the node group the node was in before the name change
            EMotionFX::AnimGraphNodeGroup* nodeGroup = animGraph->FindNodeGroupForNode(node);

            // remove the node from the node group as its id is going to change
            if (nodeGroup)
            {
                nodeGroup->RemoveNodeById(node->GetId());
            }

            node->SetName(m_oldName.c_str());

            // as the id of the node changed after renaming it, we have to readd the node with the new id
            if (nodeGroup)
            {
                nodeGroup->AddNode(node->GetId());
            }

            // call the post rename node event
            EMotionFX::GetEventManager().OnRenamedNode(animGraph, node, node->GetName());
        }

        m_nodeId = node->GetId();
        node->SetVisualPos(m_oldPosX, m_oldPosY);

        // set the old values to the enabled flag and the visualization flag
        node->SetIsEnabled(m_oldEnabled);
        node->SetVisualization(m_oldVisualized);

        // do only for parameter nodes
        if (azrtti_typeid(node) == azrtti_typeid<EMotionFX::BlendTreeParameterNode>() && parameters.CheckIfHasParameter("parameterMask"))
        {
            // type cast to a parameter node
            EMotionFX::BlendTreeParameterNode* parameterNode = static_cast<EMotionFX::BlendTreeParameterNode*>(node);

            // get the parameter mask attribute and update the mask
            parameterNode->SetParameters(m_oldParameterMask);
        }

        // set the dirty flag back to the old value
        animGraph->SetDirtyFlag(m_oldDirtyFlag);

        node->Reinit();
        animGraph->RecursiveInvalidateUniqueDatas();

        return true;
    }

    // init the syntax of the command
    void CommandAnimGraphAdjustNode::InitSyntax()
    {
        // required
        GetSyntax().ReserveParameters(8);
        GetSyntax().AddRequiredParameter("animGraphID", "The id of the anim graph to work on.", MCore::CommandSyntax::PARAMTYPE_INT);
        GetSyntax().AddRequiredParameter("name", "The name of the node to modify.", MCore::CommandSyntax::PARAMTYPE_STRING);
        GetSyntax().AddParameter("newName", "The new name of the node.", MCore::CommandSyntax::PARAMTYPE_STRING, "");
        GetSyntax().AddParameter("xPos", "The new x position of the upper left corner in the visual graph.", MCore::CommandSyntax::PARAMTYPE_INT, "0");
        GetSyntax().AddParameter("yPos", "The new y position of the upper left corner in the visual graph.", MCore::CommandSyntax::PARAMTYPE_INT, "0");
        GetSyntax().AddParameter("enabled", "Is the node enabled?", MCore::CommandSyntax::PARAMTYPE_BOOLEAN, "true");
        GetSyntax().AddParameter("visualize", "Is the node visualized?", MCore::CommandSyntax::PARAMTYPE_BOOLEAN, "false");
        GetSyntax().AddParameter("updateAttributes", "Update attributes afterwards?", MCore::CommandSyntax::PARAMTYPE_BOOLEAN, "true");
        GetSyntax().AddParameter("attributesString", "The node attributes as string.", MCore::CommandSyntax::PARAMTYPE_STRING, "");
    }

    // get the description
    const char* CommandAnimGraphAdjustNode::GetDescription() const
    {
        return "This command adjust properties of a given anim graph node.";
    }

    //-------------------------------------------------------------------------------------
    // Remove a anim graph node
    //-------------------------------------------------------------------------------------
    // constructor
    CommandAnimGraphRemoveNode::CommandAnimGraphRemoveNode(MCore::Command* orgCommand)
        : MCore::Command("AnimGraphRemoveNode", orgCommand)
    {
        m_isEntryNode = false;
    }

    // destructor
    CommandAnimGraphRemoveNode::~CommandAnimGraphRemoveNode()
    {
    }

    // execute
    bool CommandAnimGraphRemoveNode::Execute(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        // get the anim graph to work on
        EMotionFX::AnimGraph* animGraph = CommandsGetAnimGraph(parameters, this, outResult);
        if (animGraph == nullptr)
        {
            return false;
        }

        // store the anim graph id for undo
        m_animGraphId = animGraph->GetID();

        // find the emfx node
        AZStd::string name;
        parameters.GetValue("name", "", name);
        EMotionFX::AnimGraphNode* emfxNode = animGraph->RecursiveFindNodeByName(name.c_str());
        if (emfxNode == nullptr)
        {
            outResult = AZStd::string::format("There is no node with the name '%s'", name.c_str());
            return false;
        }

        m_type = azrtti_typeid(emfxNode);
        m_name = emfxNode->GetName();
        m_posX = emfxNode->GetVisualPosX();
        m_posY = emfxNode->GetVisualPosY();
        m_collapsed = emfxNode->GetIsCollapsed();
        m_oldContents = MCore::ReflectionSerializer::SerializeMembersExcept(emfxNode, { "childNodes", "connections", "transitions" }).GetValue();
        m_nodeId = emfxNode->GetId();

        // remember the node group for the node for undo
        m_nodeGroupName.clear();
        EMotionFX::AnimGraphNodeGroup* nodeGroup = animGraph->FindNodeGroupForNode(emfxNode);
        if (nodeGroup)
        {
            m_nodeGroupName = nodeGroup->GetName();
        }

        // get the parent node
        EMotionFX::AnimGraphNode* parentNode = emfxNode->GetParentNode();
        if (parentNode)
        {
            if (azrtti_typeid(parentNode) == azrtti_typeid<EMotionFX::AnimGraphStateMachine>())
            {
                EMotionFX::AnimGraphStateMachine* stateMachine = static_cast<EMotionFX::AnimGraphStateMachine*>(parentNode);
                if (stateMachine->GetEntryState() == emfxNode)
                {
                    m_isEntryNode = true;

                    // Find a new entry node if we can
                    //--------------------------
                    // Find alternative entry state.
                    EMotionFX::AnimGraphNode* newEntryState = nullptr;
                    size_t numStates = stateMachine->GetNumChildNodes();
                    for (size_t s = 0; s < numStates; ++s)
                    {
                        EMotionFX::AnimGraphNode* childNode = stateMachine->GetChildNode(s);
                        if (childNode != emfxNode)
                        {
                            newEntryState = childNode;
                            break;
                        }
                    }

                    // Check if we've found a new possible entry state.
                    if (newEntryState)
                    {
                        AZStd::string commandString = AZStd::string::format("AnimGraphSetEntryState -animGraphID %i -entryNodeName \"%s\"", animGraph->GetID(), newEntryState->GetName());
                        if (!GetCommandManager()->ExecuteCommandInsideCommand(commandString.c_str(), outResult))
                        {
                            AZ_Error("EMotionFX", false, outResult.c_str());
                        }
                    }
                }
            }

            m_parentName = parentNode->GetName();
            m_parentNodeId = parentNode->GetId();

            // call the pre remove node event
            EMotionFX::GetEventManager().OnRemoveNode(animGraph, emfxNode);

            // remove all unique datas for the node
            animGraph->RemoveAllObjectData(emfxNode, true);

            // remove the actual node
            parentNode->RemoveChildNodeByPointer(emfxNode);
        }
        else
        {
            m_parentNodeId.SetInvalid();
            m_parentName.clear();
            MCore::LogError("Cannot remove root state machine.");
            MCORE_ASSERT(false);
            return false;
        }

        // save the current dirty flag and tell the anim graph that something got changed
        m_oldDirtyFlag = animGraph->GetDirtyFlag();
        animGraph->SetDirtyFlag(true);

        animGraph->RecursiveInvalidateUniqueDatas();

        return true;
    }

    // undo the command
    bool CommandAnimGraphRemoveNode::Undo(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        MCORE_UNUSED(parameters);

        EMotionFX::AnimGraph* animGraph = EMotionFX::GetAnimGraphManager().FindAnimGraphByID(m_animGraphId);
        if (!animGraph)
        {
            outResult = AZStd::string::format("The anim graph with id '%i' does not exist anymore.", m_animGraphId);
            return false;
        }

        // create the node again
        MCore::CommandGroup group("Recreating node");
        AZStd::string commandString;
        if (!m_parentName.empty())
        {
            commandString = AZStd::string::format("AnimGraphCreateNode -animGraphID %i -type \"%s\" -parentName \"%s\" -name \"%s\" -nodeId \"%s\" -xPos %d -yPos %d -collapsed %s -center false -contents {%s}",
                animGraph->GetID(),
                m_type.ToString<AZStd::string>().c_str(),
                m_parentName.c_str(),
                m_name.c_str(),
                m_nodeId.ToString().c_str(),
                m_posX,
                m_posY,
                AZStd::to_string(m_collapsed).c_str(),
                m_oldContents.c_str());

            group.AddCommandString(commandString);

            if (m_isEntryNode)
            {
                commandString = AZStd::string::format("AnimGraphSetEntryState -animGraphID %i -entryNodeName \"%s\"", animGraph->GetID(), m_name.c_str());
                group.AddCommandString(commandString);
            }
        }
        else
        {
            commandString = AZStd::string::format("AnimGraphCreateNode -animGraphID %i -type \"%s\" -name \"%s\" -nodeId \"%s\" -xPos %d -yPos %d -collapsed %s -center false -contents {%s}",
                animGraph->GetID(),
                m_type.ToString<AZStd::string>().c_str(),
                m_name.c_str(),
                m_nodeId.ToString().c_str(),
                m_posX,
                m_posY,
                AZStd::to_string(m_collapsed).c_str(),
                m_oldContents.c_str());
            group.AddCommandString(commandString);
        }

        if (!GetCommandManager()->ExecuteCommandGroupInsideCommand(group, outResult))
        {
            if (!outResult.empty())
            {
                MCore::LogError(outResult.c_str());
            }

            return false;
        }

        // add it to the old node group if it was assigned to one before
        if (!m_nodeGroupName.empty())
        {
            auto* command = aznew CommandSystem::CommandAnimGraphAdjustNodeGroup(
                GetCommandManager()->FindCommand(CommandSystem::CommandAnimGraphAdjustNodeGroup::s_commandName),
                /*animGraphId = */ animGraph->GetID(),
                /*name = */ m_nodeGroupName,
                /*visible = */ AZStd::nullopt,
                /*newName = */ AZStd::nullopt,
                /*nodeNames = */ {{m_name}},
                /*nodeAction = */ CommandSystem::CommandAnimGraphAdjustNodeGroup::NodeAction::Add
            );
            if (GetCommandManager()->ExecuteCommandInsideCommand(command, outResult) == false)
            {
                if (!outResult.empty())
                {
                    MCore::LogError(outResult.c_str());
                }

                return false;
            }
        }

        // set the dirty flag back to the old value
        animGraph->SetDirtyFlag(m_oldDirtyFlag);
        return true;
    }

    // init the syntax of the command
    void CommandAnimGraphRemoveNode::InitSyntax()
    {
        // required
        GetSyntax().ReserveParameters(2);
        GetSyntax().AddRequiredParameter("animGraphID", "The id of the anim graph to work on.", MCore::CommandSyntax::PARAMTYPE_INT);
        GetSyntax().AddRequiredParameter("name", "The name of the node to remove.", MCore::CommandSyntax::PARAMTYPE_STRING);
    }

    // get the description
    const char* CommandAnimGraphRemoveNode::GetDescription() const
    {
        return "This command removes a anim graph nodewith a given name.";
    }

    //-------------------------------------------------------------------------------------
    // Set the entry state of a state machine
    //-------------------------------------------------------------------------------------

    // constructor
    CommandAnimGraphSetEntryState::CommandAnimGraphSetEntryState(MCore::Command* orgCommand)
        : MCore::Command("AnimGraphSetEntryState", orgCommand)
    {
    }

    // destructor
    CommandAnimGraphSetEntryState::~CommandAnimGraphSetEntryState()
    {
    }

    // execute
    bool CommandAnimGraphSetEntryState::Execute(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        // get the anim graph to work on
        EMotionFX::AnimGraph* animGraph = CommandsGetAnimGraph(parameters, this, outResult);
        if (animGraph == nullptr)
        {
            return false;
        }

        // store the anim graph id for undo
        m_animGraphId = animGraph->GetID();

        AZStd::string entryNodeName;
        parameters.GetValue("entryNodeName", this, entryNodeName);

        // find the entry anim graph node
        EMotionFX::AnimGraphNode* entryNode = animGraph->RecursiveFindNodeByName(entryNodeName.c_str());
        if (!entryNode)
        {
            outResult = AZStd::string::format("There is no entry node with the name '%s'", entryNodeName.c_str());
            return false;
        }

        // check if the parent node is a state machine
        EMotionFX::AnimGraphNode* stateMachineNode = entryNode->GetParentNode();
        if (stateMachineNode == nullptr || azrtti_typeid(stateMachineNode) != azrtti_typeid<EMotionFX::AnimGraphStateMachine>())
        {
            outResult = AZStd::string::format("Cannot set entry node '%s'. Parent node is not a state machine or not valid at all.", entryNodeName.c_str());
            return false;
        }

        // Check if the node can be set as the entry node
        if (!entryNode->GetCanBeEntryNode())
        {
            outResult = AZStd::string::format("Cannot set entry node '%s'. This type of node cannot be set as an entry node.", entryNodeName.c_str());
            return false;
        }

        // get the parent state machine
        EMotionFX::AnimGraphStateMachine* stateMachine = (EMotionFX::AnimGraphStateMachine*)stateMachineNode;

        // store the id of the old entry node
        EMotionFX::AnimGraphNode* oldEntryNode = stateMachine->GetEntryState();
        if (oldEntryNode)
        {
            m_oldEntryStateNodeId = oldEntryNode->GetId();
        }
        else
        {
            m_oldEntryStateNodeId.SetInvalid();
        }

        // store the id of the state machine
        m_oldStateMachineNodeId = stateMachineNode->GetId();

        // set the new entry state for the state machine
        stateMachine->SetEntryState(entryNode);

        // save the current dirty flag and tell the anim graph that something got changed
        m_oldDirtyFlag = animGraph->GetDirtyFlag();
        animGraph->SetDirtyFlag(true);

        stateMachine->Reinit();
        animGraph->RecursiveInvalidateUniqueDatas();

        return true;
    }

    // undo the command
    bool CommandAnimGraphSetEntryState::Undo(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        MCORE_UNUSED(parameters);

        // get the anim graph
        EMotionFX::AnimGraph* animGraph = EMotionFX::GetAnimGraphManager().FindAnimGraphByID(m_animGraphId);
        if (animGraph == nullptr)
        {
            outResult = AZStd::string::format("The anim graph with id '%i' does not exist anymore.", m_animGraphId);
            return false;
        }

        // get the state machine
        EMotionFX::AnimGraphNode* stateMachineNode = animGraph->RecursiveFindNodeById(m_oldStateMachineNodeId);
        if (stateMachineNode == nullptr || azrtti_typeid(stateMachineNode) != azrtti_typeid<EMotionFX::AnimGraphStateMachine>())
        {
            outResult = "Cannot undo set entry node. Parent node is not a state machine or not valid at all.";
            return false;
        }

        // get the parent state machine
        EMotionFX::AnimGraphStateMachine* stateMachine = (EMotionFX::AnimGraphStateMachine*)stateMachineNode;

        // find the entry anim graph node
        if (m_oldEntryStateNodeId.IsValid())
        {
            EMotionFX::AnimGraphNode* entryNode = animGraph->RecursiveFindNodeById(m_oldEntryStateNodeId);
            if (!entryNode)
            {
                outResult = "Cannot undo set entry node. Old entry node cannot be found.";
                return false;
            }

            // set the old entry state for the state machine
            stateMachine->SetEntryState(entryNode);
        }
        else
        {
            // set the old entry state for the state machine
            stateMachine->SetEntryState(nullptr);
        }

        // set the dirty flag back to the old value
        animGraph->SetDirtyFlag(m_oldDirtyFlag);

        stateMachine->Reinit();
        animGraph->RecursiveInvalidateUniqueDatas();

        return true;
    }

    // init the syntax of the command
    void CommandAnimGraphSetEntryState::InitSyntax()
    {
        GetSyntax().ReserveParameters(2);
        GetSyntax().AddRequiredParameter("entryNodeName", "The name of the new entry node.", MCore::CommandSyntax::PARAMTYPE_STRING);
        GetSyntax().AddParameter("animGraphID", "The id of the anim graph to work on.", MCore::CommandSyntax::PARAMTYPE_INT, "-1");
    }

    // get the description
    const char* CommandAnimGraphSetEntryState::GetDescription() const
    {
        return "This command sets the entry state of a state machine.";
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    void CreateAnimGraphNode(MCore::CommandGroup* commandGroup, EMotionFX::AnimGraph* animGraph, const AZ::TypeId& type, const AZStd::string& namePrefix, EMotionFX::AnimGraphNode* parentNode, int32 offsetX, int32 offsetY, const AZStd::string& serializedContents)
    {
        AZStd::string commandString;
        MCore::CommandGroup internalGroup;

        // Add the newly generated commands to the commandGroup parameter in case it is valid, add it to the internal group and execute it otherwise.
        bool executeGroup = false;
        if (!commandGroup)
        {
            executeGroup = true;
            commandGroup = &internalGroup;
        }

        // if we want to add a blendtree, we also should automatically add a final node
        if (type == azrtti_typeid<EMotionFX::BlendTree>())
        {
            MCORE_ASSERT(parentNode);
            commandGroup->SetGroupName("Create blend tree");

            commandString = AZStd::string::format("AnimGraphCreateNode -animGraphID %i -type \"%s\" -parentName \"%s\" -xPos %d -yPos %d -name GENERATE -namePrefix \"%s\"",
                animGraph->GetID(),
                type.ToString<AZStd::string>().c_str(),
                parentNode->GetName(),
                offsetX,
                offsetY,
                namePrefix.c_str());

            if (!serializedContents.empty())
            {
                commandString += AZStd::string::format(" -contents {%s} ", serializedContents.c_str());
            }

            commandGroup->AddCommandString(commandString);

            // auto create the final node
            commandString = AZStd::string::format("AnimGraphCreateNode -animGraphID %i -type \"%s\" -parentName \"%%LASTRESULT%%\" -xPos %d -yPos %d -name GENERATE -namePrefix \"FinalNode\"",
                animGraph->GetID(),
                azrtti_typeid<EMotionFX::BlendTreeFinalNode>().ToString<AZStd::string>().c_str(),
                0,
                0);
            commandGroup->AddCommandString(commandString);
        }
        // if we want to add a state machine, we also should automatically add an exit node
        else if (type == azrtti_typeid<EMotionFX::AnimGraphStateMachine>())
        {
            MCORE_ASSERT(parentNode);
            commandGroup->SetGroupName("Create child state machine");

            commandString = AZStd::string::format("AnimGraphCreateNode -animGraphID %i -type \"%s\" -parentName \"%s\" -xPos %d -yPos %d -name GENERATE -namePrefix \"%s\"",
                animGraph->GetID(),
                type.ToString<AZStd::string>().c_str(),
                parentNode->GetName(),
                offsetX,
                offsetY,
                namePrefix.c_str());

            if (!serializedContents.empty())
            {
                commandString += AZStd::string::format(" -contents {%s} ", serializedContents.c_str());
            }

            commandGroup->AddCommandString(commandString);

            // auto create an exit node in case we're not creating a state machine inside a blend tree
            if (azrtti_typeid(parentNode) != azrtti_typeid<EMotionFX::BlendTree>())
            {
                commandString = AZStd::string::format("AnimGraphCreateNode -animGraphID %i -type \"%s\" -parentName \"%%LASTRESULT%%\" -xPos %d -yPos %d -name GENERATE -namePrefix \"EntryNode\"",
                    animGraph->GetID(),
                    azrtti_typeid<EMotionFX::AnimGraphEntryNode>().ToString<AZStd::string>().c_str(),
                    -200,
                    0);
                commandGroup->AddCommandString(commandString);

                commandString = AZStd::string::format("AnimGraphCreateNode -animGraphID %i -type \"%s\" -parentName \"%%LASTRESULT2%%\" -xPos %d -yPos %d -name GENERATE -namePrefix \"ExitNode\"",
                    animGraph->GetID(),
                    azrtti_typeid<EMotionFX::AnimGraphExitNode>().ToString<AZStd::string>().c_str(),
                    200,
                    0);
                commandGroup->AddCommandString(commandString);
            }
        }
        else
        {
            commandGroup->SetGroupName(AZStd::string::format("Create %s node", namePrefix.c_str()));
            if (parentNode)
            {
                commandString = AZStd::string::format("AnimGraphCreateNode -animGraphID %i -type \"%s\" -parentName \"%s\" -xPos %d -yPos %d -name GENERATE -namePrefix \"%s\"",
                    animGraph->GetID(),
                    type.ToString<AZStd::string>().c_str(),
                    parentNode->GetName(),
                    offsetX,
                    offsetY,
                    namePrefix.c_str());
            }
            else
            {
                commandString = AZStd::string::format("AnimGraphCreateNode -animGraphID %i -type \"%s\" -xPos %d -yPos %d -name GENERATE -namePrefix \"%s\"",
                    animGraph->GetID(),
                    type.ToString<AZStd::string>().c_str(),
                    offsetX,
                    offsetY,
                    namePrefix.c_str());
            }

            if (!serializedContents.empty())
            {
                commandString += AZStd::string::format(" -contents {%s} ", serializedContents.c_str());
            }

            commandGroup->AddCommandString(commandString);
        }

        // Execute the command group in case the command group parameter was not set.
        if (executeGroup &&
            !commandGroup->IsEmpty())
        {
            AZStd::string result;
            if (!GetCommandManager()->ExecuteCommandGroup(*commandGroup, result))
            {
                AZ_Error("EMotionFX", false, result.c_str());
            }
        }
    }

    void DeleteNode(MCore::CommandGroup* commandGroup, EMotionFX::AnimGraph* animGraph, EMotionFX::AnimGraphNode* node, AZStd::vector<EMotionFX::AnimGraphNode*>& nodeList, AZStd::vector<EMotionFX::BlendTreeConnection*>& connectionList, AZStd::vector<EMotionFX::AnimGraphStateTransition*>& transitionList, bool recursive, bool firstRootIteration = true, bool autoChangeEntryStates = true)
    {
        MCORE_UNUSED(recursive);
        MCORE_UNUSED(autoChangeEntryStates);

        if (!node)
        {
            return;
        }

        // Skip directly if the node is already in the list.
        if (AZStd::find(nodeList.begin(), nodeList.end(), node) != nodeList.end())
        {
            return;
        }

        // Only delete nodes that are also deletable, final nodes e.g. can't be cut and deleted.
        if (firstRootIteration && !node->GetIsDeletable())
        {
            return;
        }

        // Check if the last instance is not deletable while all others are.
        if (firstRootIteration && !node->GetIsLastInstanceDeletable())
        {
            EMotionFX::AnimGraphNode* parentNode = node->GetParentNode();
            if (parentNode)
            {
                // Gather the number of nodes with the same type as the one we're trying to remove.
                AZStd::vector<EMotionFX::AnimGraphNode*> outNodes;
                const AZ::TypeId nodeType = azrtti_typeid(node);
                parentNode->CollectChildNodesOfType(nodeType, &outNodes);
                const size_t numTypeNodes = outNodes.size();

                // Gather the number of already removed nodes with the same type as the one we're trying to remove.
                size_t numTypeDeletedNodes = 0;
                for (const EMotionFX::AnimGraphNode* i : nodeList)
                {
                    // Check if the nodes have the same parent, meaning they are in the same graph plus check if they have the same type
                    // if that both is the same we can increase the number of deleted nodes for the graph where the current node is in.
                    if (i->GetParentNode() == parentNode && azrtti_typeid(i) == nodeType)
                    {
                        numTypeDeletedNodes++;
                    }
                }

                // In case there the number of nodes with the same type as the given node is bigger than the number of already removed nodes + 1 means there is
                // only one node left with the given type return directly without deleting the node as we're not allowed to remove the last instance of the node.
                if (numTypeDeletedNodes + 1 >= numTypeNodes)
                {
                    return;
                }
            }
        }

        /////////////////////////
        // 1. Delete all connections and transitions that are connected to the node.

        // Delete all incoming and outgoing connections from current node.
        DeleteNodeConnections(commandGroup, node, node->GetParentNode(), connectionList, true);
        DeleteStateTransitions(commandGroup, node, node->GetParentNode(), transitionList, true);

        /////////////////////////
        // 2. Delete all child nodes recursively before deleting the node.

        // Get the number of child nodes, iterate through them and recursively call the function.
        const size_t numChildNodes = node->GetNumChildNodes();
        for (size_t i = 0; i < numChildNodes; ++i)
        {
            EMotionFX::AnimGraphNode* childNode = node->GetChildNode(i);
            DeleteNode(commandGroup, animGraph, childNode, nodeList, connectionList, transitionList, true, false, false);
        }

        /////////////////////////
        // 3. Delete the node.

        // Remove state actions back to front.
        const size_t numActions = node->GetTriggerActionSetup().GetNumActions();
        for (size_t i = 0; i < numActions; ++i)
        {
            const size_t actionIndex = numActions - i - 1;
            RemoveStateAction(node, actionIndex, commandGroup);
        }

        const AZStd::string commandString = AZStd::string::format("AnimGraphRemoveNode -animGraphID %i -name \"%s\"", node->GetAnimGraph()->GetID(), node->GetName());
        commandGroup->AddCommandString(commandString);

        nodeList.push_back(node);
    }

    void DeleteNodes(MCore::CommandGroup* commandGroup, EMotionFX::AnimGraph* animGraph, const AZStd::vector<AZStd::string>& nodeNames, AZStd::vector<EMotionFX::AnimGraphNode*>& nodeList, AZStd::vector<EMotionFX::BlendTreeConnection*>& connectionList, AZStd::vector<EMotionFX::AnimGraphStateTransition*>& transitionList, bool autoChangeEntryStates)
    {
        for (const AZStd::string& nodeName : nodeNames)
        {
            EMotionFX::AnimGraphNode* node = animGraph->RecursiveFindNodeByName(nodeName.c_str());

            // Add the delete node commands to the command group.
            DeleteNode(commandGroup, animGraph, node, nodeList, connectionList, transitionList, true, true, autoChangeEntryStates);
        }
    }

    void DeleteNodes(EMotionFX::AnimGraph* animGraph, const AZStd::vector<AZStd::string>& nodeNames)
    {
        MCore::CommandGroup commandGroup("Delete anim graph nodes");

        AZStd::vector<EMotionFX::BlendTreeConnection*> connectionList;
        AZStd::vector<EMotionFX::AnimGraphStateTransition*> transitionList;
        AZStd::vector<EMotionFX::AnimGraphNode*> nodeList;
        DeleteNodes(&commandGroup, animGraph, nodeNames, nodeList, connectionList, transitionList);

        AZStd::string result;
        if (!GetCommandManager()->ExecuteCommandGroup(commandGroup, result))
        {
            AZ_Error("EMotionFX", false, result.c_str());
        }
    }

    void DeleteNodes(MCore::CommandGroup* commandGroup, EMotionFX::AnimGraph* animGraph, const AZStd::vector<EMotionFX::AnimGraphNode*>& nodes, bool autoChangeEntryStates)
    {
        AZStd::vector<EMotionFX::BlendTreeConnection*> connectionList;
        AZStd::vector<EMotionFX::AnimGraphStateTransition*> transitionList;
        AZStd::vector<EMotionFX::AnimGraphNode*> nodeList;

        for (EMotionFX::AnimGraphNode* node : nodes)
        {
            // Add the delete node commands to the command group.
            DeleteNode(commandGroup, animGraph, node, nodeList, connectionList, transitionList, true, true, autoChangeEntryStates);
        }
    }

    void CopyAnimGraphNodeCommand(MCore::CommandGroup* commandGroup, EMotionFX::AnimGraph* targetAnimGraph, EMotionFX::AnimGraphNode* targetNode, EMotionFX::AnimGraphNode* node,
        bool cutMode, AZStd::unordered_map<AZ::u64, AZ::u64>& convertedIds, AnimGraphCopyPasteData& copyPasteData, AZStd::unordered_set<AZStd::string>& generatedNames)
    {
        if (!node)
        {
            return;
        }

        // Construct the parent name
        const AZStd::string parentName = copyPasteData.GetNewNodeName(targetNode, cutMode);
        EMotionFX::AnimGraphNodeId nodeId;
        AZStd::string nodeName = node->GetNameString();
        if (cutMode)
        {
            nodeId = node->GetId();
        }
        else
        {
            // Create new node id and name
            nodeId = EMotionFX::AnimGraphNodeId::Create();
            convertedIds.emplace(node->GetId(), nodeId);

            nodeName = GenerateUniqueNodeName(targetAnimGraph, node, generatedNames);
            generatedNames.emplace(nodeName);
        }
        copyPasteData.m_newNamesByCopiedNodes.emplace(node, nodeName);

        AZStd::string commandString = AZStd::string::format("AnimGraphCreateNode -animGraphID %d -type \"%s\" -parentName \"%s\" -xPos %i -yPos %i -name \"%s\" -collapsed %s -enabled %s -visualize %s -nodeId %s",
            targetAnimGraph->GetID(),
            azrtti_typeid(node).ToString<AZStd::string>().c_str(),
            parentName.c_str(),
            node->GetVisualPosX(),
            node->GetVisualPosY(),
            nodeName.c_str(),
            AZStd::to_string(node->GetIsCollapsed()).c_str(),
            AZStd::to_string(node->GetIsEnabled()).c_str(),
            AZStd::to_string(node->GetIsVisualizationEnabled()).c_str(),
            nodeId.ToString().c_str());

        // Don't put that into the format as the attribute string can become pretty big strings.
        commandString += " -contents {";
        commandString += MCore::ReflectionSerializer::SerializeMembersExcept(node, { "childNodes", "connections", "transitions" }).GetValue();
        commandString += "}";

        commandGroup->AddCommandString(commandString);

        if (!cutMode)
        {
            AZStd::string attributesString;
            node->GetAttributeStringForAffectedNodeIds(convertedIds, attributesString);
            if (!attributesString.empty())
            {
                // need to convert
                commandString = AZStd::string::format("AnimGraphAdjustNode -animGraphID %d -name \"%s\" -attributesString {%s}",
                    targetAnimGraph->GetID(),
                    nodeName.c_str(),
                    attributesString.c_str());
                commandGroup->AddCommandString(commandString);
            }
        }

        // Check if the given node is part of a node group.
        EMotionFX::AnimGraphNodeGroup* nodeGroup = node->GetAnimGraph()->FindNodeGroupForNode(node);
        if (nodeGroup && !cutMode)
        {
            auto* command = aznew CommandSystem::CommandAnimGraphAdjustNodeGroup(
                GetCommandManager()->FindCommand(CommandSystem::CommandAnimGraphAdjustNodeGroup::s_commandName),
                /*animGraphId = */ targetAnimGraph->GetID(),
                /*name = */ nodeGroup->GetNameString(),
                /*visible = */ AZStd::nullopt,
                /*newName = */ AZStd::nullopt,
                /*nodeNames = */ {{nodeName}},
                /*nodeAction = */ CommandSystem::CommandAnimGraphAdjustNodeGroup::NodeAction::Add
            );
            commandGroup->AddCommand(command);
        }

        // Recurse through the child nodes.
        const size_t numChildNodes = node->GetNumChildNodes();
        for (size_t i = 0; i < numChildNodes; ++i)
        {
            EMotionFX::AnimGraphNode* childNode = node->GetChildNode(i);
            CopyAnimGraphNodeCommand(commandGroup, targetAnimGraph, node, childNode,
                cutMode, convertedIds, copyPasteData, generatedNames);
        }
    }

    void CopyAnimGraphConnectionsCommand(MCore::CommandGroup* commandGroup, EMotionFX::AnimGraph* targetAnimGraph, EMotionFX::AnimGraphNode* node,
        bool cutMode, AZStd::unordered_map<AZ::u64, AZ::u64>& convertedIds, AnimGraphCopyPasteData& copyPasteData, AZStd::unordered_set<AZStd::string>& generatedNames,
        bool ignoreTopLevelConnections)
    {
        if (!node)
        {
            return;
        }

        // Recurse through the child nodes.
        const size_t numChildNodes = node->GetNumChildNodes();
        for (size_t i = 0; i < numChildNodes; ++i)
        {
            EMotionFX::AnimGraphNode* childNode = node->GetChildNode(i);
            CopyAnimGraphConnectionsCommand(commandGroup, targetAnimGraph, childNode,
                cutMode, convertedIds, copyPasteData, generatedNames,
                false);
        }

        if (!ignoreTopLevelConnections)
        {
            if (azrtti_typeid(node) == azrtti_typeid<EMotionFX::AnimGraphStateMachine>())
            {
                EMotionFX::AnimGraphStateMachine* stateMachine = static_cast<EMotionFX::AnimGraphStateMachine*>(node);

                EMotionFX::AnimGraphNode* entryState = stateMachine->GetEntryState();
                if (entryState)
                {
                    const AZStd::string entryStateName = copyPasteData.GetNewNodeName(entryState, cutMode);
                    const AZStd::string commandString = AZStd::string::format("AnimGraphSetEntryState -animGraphID %d -entryNodeName \"%s\"", targetAnimGraph->GetID(), entryStateName.c_str());
                    commandGroup->AddCommandString(commandString);
                }

                const size_t numTransitions = stateMachine->GetNumTransitions();
                for (size_t i = 0; i < numTransitions; ++i)
                {
                    CopyStateTransition(commandGroup, targetAnimGraph, stateMachine->GetTransition(i),
                        cutMode, convertedIds, copyPasteData);
                }
            }
            else
            {
                const size_t numConnections = node->GetNumConnections();
                for (size_t i = 0; i < numConnections; ++i)
                {
                    EMotionFX::BlendTreeConnection* connection = node->GetConnection(i);
                    CopyBlendTreeConnection(commandGroup, targetAnimGraph, node, connection,
                        cutMode, convertedIds, copyPasteData);
                }
            }
        }
    }

    void ConstructCopyAnimGraphNodesCommandGroup(MCore::CommandGroup* commandGroup, EMotionFX::AnimGraphNode* targetNode, AZStd::vector<EMotionFX::AnimGraphNode*>& nodesToCopy, int32 posX, int32 posY, bool cutMode, AnimGraphCopyPasteData& copyPasteData, bool ignoreTopLevelConnections)
    {
        if (nodesToCopy.empty())
        {
            return;
        }

        // Remove all nodes that are child nodes of other selected nodes.
        AZStd::erase_if(nodesToCopy, [&nodesToCopy](const EMotionFX::AnimGraphNode* node)
        {
            const auto found = AZStd::find_if(begin(nodesToCopy), end(nodesToCopy), [node](const EMotionFX::AnimGraphNode* parent)
            {
                return node != parent && node->RecursiveIsParentNode(parent);
            });
            return found != end(nodesToCopy);
        });

        // In case we are in cut and paste mode and delete the cut nodes.
        if (cutMode)
        {
            DeleteNodes(commandGroup, nodesToCopy.front()->GetAnimGraph(), nodesToCopy, false);
        }

        AZStd::unordered_map<AZ::u64, AZ::u64> convertedIds;
        AZStd::unordered_set<AZStd::string> generatedNames;

        for (EMotionFX::AnimGraphNode* node : nodesToCopy)
        {
            if (cutMode || !node->GetCanHaveOnlyOneInsideParent())
            {
                CopyAnimGraphNodeCommand(commandGroup, targetNode->GetAnimGraph(), targetNode, node,
                    cutMode, convertedIds, copyPasteData, generatedNames);
            }
        }

        // Collect transitions between the nodes to copy (wildcards to a target transition that is being copied or transitions
        // where both the source and destination are being copied)
        AZStd::unordered_set<EMotionFX::AnimGraphStateMachine*> parentStateMachines;

        for (EMotionFX::AnimGraphNode* node : nodesToCopy)
        {
            CopyAnimGraphConnectionsCommand(commandGroup, targetNode->GetAnimGraph(), node,
                cutMode, convertedIds, copyPasteData, generatedNames,
                ignoreTopLevelConnections);

            // Collect parent state machines for cut/copied nodes to avoid copying transitions multiple times.
            // The ownership for blend tree connections is defined differently than for state machines. State machines own the transitions
            // while the blend tree does not own any connections. The nodes within the blend tree hold their incoming connections.
            EMotionFX::AnimGraphNode* parentNode = node->GetParentNode();
            if (parentNode &&
                azrtti_typeid(parentNode) == azrtti_typeid<EMotionFX::AnimGraphStateMachine>() &&
                AZStd::find(nodesToCopy.begin(), nodesToCopy.end(), parentNode) == nodesToCopy.end())
            {
                EMotionFX::AnimGraphStateMachine* parentSM = static_cast<EMotionFX::AnimGraphStateMachine*>(parentNode);
                if (parentStateMachines.find(parentSM) == parentStateMachines.end())
                {
                    parentStateMachines.emplace(parentSM);
                }
            }
        }

        // Copy state transitions
        if (!ignoreTopLevelConnections)
        {
            EMotionFX::AnimGraphStateMachine* targetStateMachine = azdynamic_cast<EMotionFX::AnimGraphStateMachine*>(targetNode);
            if (targetStateMachine)
            {
                for (EMotionFX::AnimGraphStateMachine* stateMachine : parentStateMachines)
                {
                    const size_t numTransitions = stateMachine->GetNumTransitions();
                    for (size_t t = 0; t < numTransitions; ++t)
                    {
                        CopyStateTransition(commandGroup, targetStateMachine->GetAnimGraph(), stateMachine->GetTransition(t), cutMode, convertedIds, copyPasteData);
                    }
                }
            }
        }

        ///////////////////////////////////////////////////////////////////////////////////////////////////////
        // PHASE 1: Iterate over the top level copy&paste nodes and calculate the mid point of them.
        ///////////////////////////////////////////////////////////////////////////////////////////////////////

        // Variables to sum up the node positions to later calculate the middle point of the copied nodes.
        // We only need to fix the top-level nodes
        int32 middlePosX = 0;
        int32 middlePosY = 0;

        for (EMotionFX::AnimGraphNode* node : nodesToCopy)
        {
            middlePosX += node->GetVisualPosX();
            middlePosY += node->GetVisualPosY();
        }

        middlePosX = static_cast<int32>(MCore::Math::SignOfFloat((float)middlePosX) * (MCore::Math::Abs((float)middlePosX) / (float)nodesToCopy.size()));
        middlePosY = static_cast<int32>(MCore::Math::SignOfFloat((float)middlePosY) * (MCore::Math::Abs((float)middlePosY) / (float)nodesToCopy.size()));

        ///////////////////////////////////////////////////////////////////////////////////////////////////////
        // PHASE 2: Adjust attributes to new position
        ///////////////////////////////////////////////////////////////////////////////////////////////////////
        for (EMotionFX::AnimGraphNode* node : nodesToCopy)
        {
            const AZStd::string nodeName = copyPasteData.GetNewNodeName(node, cutMode);
            const int newNodeX = node->GetVisualPosX() + (posX - middlePosX);
            const int newNodeY = node->GetVisualPosY() + (posY - middlePosY);

            const AZStd::string commandString = AZStd::string::format("AnimGraphAdjustNode -animGraphID %d -name \"%s\" -xPos %d -yPos %d",
                targetNode->GetAnimGraph()->GetID(),
                nodeName.c_str(),
                newNodeX,
                newNodeY);
            commandGroup->AddCommandString(commandString);
        }
    }
} // namespace CommandSystem
