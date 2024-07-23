/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Component/Entity.h>
#include <AzCore/Math/MathUtils.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <EMotionFX/Source/AnimGraphBus.h>
#include "EMotionFXConfig.h"
#include "AnimGraphNode.h"
#include "AnimGraphInstance.h"
#include "ActorInstance.h"
#include "EventManager.h"
#include "AnimGraphStateMachine.h"
#include "AnimGraphNodeGroup.h"
#include "AnimGraphTransitionCondition.h"
#include "AnimGraphTriggerAction.h"
#include "AnimGraphStateTransition.h"
#include "AnimGraphObjectData.h"
#include "AnimGraphEventBuffer.h"
#include "AnimGraphManager.h"
#include "AnimGraphSyncTrack.h"
#include "AnimGraph.h"
#include "Recorder.h"

#include "AnimGraphMotionNode.h"
#include "ActorManager.h"
#include "EMotionFXManager.h"

#include <MCore/Source/StringIdPool.h>
#include <MCore/Source/IDGenerator.h>
#include <MCore/Source/Attribute.h>
#include <MCore/Source/AttributeFactory.h>
#include <MCore/Source/LogManager.h>
#include <MCore/Source/Stream.h>
#include <MCore/Source/Compare.h>
#include <MCore/Source/Random.h>


namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(AnimGraphNode, AnimGraphAllocator)
    AZ_CLASS_ALLOCATOR_IMPL(AnimGraphNode::Port, AnimGraphAllocator)

    AnimGraphNode::AnimGraphNode()
        : AnimGraphObject(nullptr)
        , m_id(AnimGraphNodeId::Create())
        , m_nodeIndex(InvalidIndex)
        , m_disabled(false)
        , m_parentNode(nullptr)
        , m_customData(nullptr)
        , m_visualizeColor(EMotionFX::AnimGraph::RandomGraphColor())
        , m_visEnabled(false)
        , m_isCollapsed(false)
        , m_posX(0)
        , m_posY(0)
    {
    }


    AnimGraphNode::AnimGraphNode(AnimGraph* animGraph, const char* name)
        : AnimGraphNode()
    {
        SetName(name);
        InitAfterLoading(animGraph);
    }


    AnimGraphNode::~AnimGraphNode()
    {
        RemoveAllConnections();
        RemoveAllChildNodes();

        if (m_animGraph)
        {
            m_animGraph->RemoveObject(this);
        }
    }


    void AnimGraphNode::RecursiveReinit()
    {
        for (BlendTreeConnection* connection : m_connections)
        {
            connection->Reinit();
        }

        for (AnimGraphNode* childNode : m_childNodes)
        {
            childNode->RecursiveReinit();
        }
    }


    bool AnimGraphNode::InitAfterLoading(AnimGraph* animGraph)
    {
        bool result = true;
        SetAnimGraph(animGraph);

        if (animGraph)
        {
            animGraph->AddObject(this);
        }

        // Initialize connections.
        for (BlendTreeConnection* connection : m_connections)
        {
            connection->InitAfterLoading(animGraph);
        }

        // Initialize child nodes.
        for (AnimGraphNode* childNode : m_childNodes)
        {
            // Sync the child node's parent.
            childNode->SetParentNode(this);

            if (!childNode->InitAfterLoading(animGraph))
            {
                result = false;
            }
        }

        InitTriggerActions();

        return result;
    }


    void AnimGraphNode::InitTriggerActions()
    {
        for (AnimGraphTriggerAction* action : m_actionSetup.GetActions())
        {
            action->InitAfterLoading(m_animGraph);
        }
    }


    // copy base settings to the other node
    void AnimGraphNode::CopyBaseNodeTo(AnimGraphNode* node) const
    {
        node->m_name            = m_name;
        node->m_id              = m_id;
        node->m_nodeInfo         = m_nodeInfo;
        node->m_customData       = m_customData;
        node->m_disabled         = m_disabled;
        node->m_posX             = m_posX;
        node->m_posY             = m_posY;
        node->m_visualizeColor   = m_visualizeColor;
        node->m_visEnabled       = m_visEnabled;
        node->m_isCollapsed      = m_isCollapsed;
    }


    void AnimGraphNode::RemoveAllConnections()
    {
        for (BlendTreeConnection* connection : m_connections)
        {
            delete connection;
        }

        m_connections.clear();
    }


    // add a connection
    BlendTreeConnection* AnimGraphNode::AddConnection(AnimGraphNode* sourceNode, uint16 sourcePort, uint16 targetPort)
    {
        // make sure the source and target ports are in range
        if (targetPort < m_inputPorts.size() && sourcePort < sourceNode->m_outputPorts.size())
        {
            BlendTreeConnection* connection = aznew BlendTreeConnection(sourceNode, sourcePort, targetPort);
            m_connections.push_back(connection);
            m_inputPorts[targetPort].m_connection = connection;
            sourceNode->m_outputPorts[sourcePort].m_connection = connection;
            return connection;
        }
        return nullptr;
    }


    BlendTreeConnection* AnimGraphNode::AddUnitializedConnection(AnimGraphNode* sourceNode, uint16 sourcePort, uint16 targetPort)
    {
        BlendTreeConnection* connection = aznew BlendTreeConnection(sourceNode, sourcePort, targetPort);
        m_connections.push_back(connection);
        return connection;
    }


    // validate the connections
    bool AnimGraphNode::ValidateConnections() const
    {
        for (const BlendTreeConnection* connection : m_connections)
        {
            if (!connection->GetIsValid())
            {
                return false;
            }
        }

        return true;
    }


    // check if the given input port is connected
    bool AnimGraphNode::CheckIfIsInputPortConnected(uint16 inputPort) const
    {
        for (const BlendTreeConnection* connection : m_connections)
        {
            if (connection->GetTargetPort() == inputPort)
            {
                return true;
            }
        }

        // failure, no connected plugged into the given port
        return false;
    }


    // remove all child nodes
    void AnimGraphNode::RemoveAllChildNodes(bool delFromMem)
    {
        if (delFromMem)
        {
            for (AnimGraphNode* childNode : m_childNodes)
            {
                delete childNode;
            }
        }

        m_childNodes.clear();

        // trigger that we removed nodes
        GetEventManager().OnRemovedChildNode(m_animGraph, this);

        // TODO: remove the nodes from the node groups of the anim graph as well here
    }


    // remove a given node
    void AnimGraphNode::RemoveChildNode(size_t index, bool delFromMem)
    {
        // remove the node from its node group
        AnimGraphNodeGroup* nodeGroup = m_animGraph->FindNodeGroupForNode(m_childNodes[index]);
        if (nodeGroup)
        {
            nodeGroup->RemoveNodeById(m_childNodes[index]->GetId());
        }

        // delete the node from memory
        if (delFromMem)
        {
            delete m_childNodes[index];
        }

        // delete the node from the child array
        m_childNodes.erase(m_childNodes.begin() + index);

        // trigger callbacks
        GetEventManager().OnRemovedChildNode(m_animGraph, this);
    }


    // remove a node by pointer
    void AnimGraphNode::RemoveChildNodeByPointer(AnimGraphNode* node, bool delFromMem)
    {
        // find the index of the given node in the child node array and remove it in case the index is valid
        const auto iterator = AZStd::find(m_childNodes.begin(), m_childNodes.end(), node);

        if (iterator != m_childNodes.end())
        {
            const size_t index = AZStd::distance(m_childNodes.begin(), iterator);
            RemoveChildNode(index, delFromMem);
        }
    }


    AnimGraphNode* AnimGraphNode::RecursiveFindNodeByName(const char* nodeName) const
    {
        if (AzFramework::StringFunc::Equal(m_name.c_str(), nodeName, true /* case sensitive */))
        {
            return const_cast<AnimGraphNode*>(this);
        }

        for (const AnimGraphNode* childNode : m_childNodes)
        {
            AnimGraphNode* result = childNode->RecursiveFindNodeByName(nodeName);
            if (result)
            {
                return result;
            }
        }

        return nullptr;
    }


    bool AnimGraphNode::RecursiveIsNodeNameUnique(const AZStd::string& newNameCandidate, const AnimGraphNode* forNode) const
    {
        if (forNode != this && m_name == newNameCandidate)
        {
            return false;
        }

        for (const AnimGraphNode* childNode : m_childNodes)
        {
            if (!childNode->RecursiveIsNodeNameUnique(newNameCandidate, forNode))
            {
                return false;
            }
        }

        return true;
    }


    AnimGraphNode* AnimGraphNode::RecursiveFindNodeById(AnimGraphNodeId nodeId) const
    {
        if (m_id == nodeId)
        {
            return const_cast<AnimGraphNode*>(this);
        }

        for (const AnimGraphNode* childNode : m_childNodes)
        {
            AnimGraphNode* result = childNode->RecursiveFindNodeById(nodeId);
            if (result)
            {
                return result;
            }
        }

        return nullptr;
    }


    // find a child node by name
    AnimGraphNode* AnimGraphNode::FindChildNode(const char* name) const
    {
        for (AnimGraphNode* childNode : m_childNodes)
        {
            // compare the node name with the parameter and return a pointer to the node in case they are equal
            if (AzFramework::StringFunc::Equal(childNode->GetName(), name, true /* case sensitive */))
            {
                return childNode;
            }
        }

        // failure, return nullptr pointer
        return nullptr;
    }


    AnimGraphNode* AnimGraphNode::FindChildNodeById(AnimGraphNodeId childId) const
    {
        for (AnimGraphNode* childNode : m_childNodes)
        {
            if (childNode->GetId() == childId)
            {
                return childNode;
            }
        }

        return nullptr;
    }


    // find a child node index by name
    size_t AnimGraphNode::FindChildNodeIndex(const char* name) const
    {
        const auto foundChildNode = AZStd::find_if(begin(m_childNodes), end(m_childNodes), [name](const AnimGraphNode* childNode)
        {
            return childNode->GetNameString() == name;
        });
        return foundChildNode != end(m_childNodes) ? AZStd::distance(begin(m_childNodes), foundChildNode) : InvalidIndex;
    }


    // find a child node index
    size_t AnimGraphNode::FindChildNodeIndex(AnimGraphNode* node) const
    {
        const auto foundChildNode = AZStd::find(m_childNodes.begin(), m_childNodes.end(), node);
        return foundChildNode != end(m_childNodes) ? AZStd::distance(begin(m_childNodes), foundChildNode) : InvalidIndex;
    }


    AnimGraphNode* AnimGraphNode::FindFirstChildNodeOfType(const AZ::TypeId& nodeType) const
    {
        const auto foundChild = AZStd::find_if(begin(m_childNodes), end(m_childNodes), [nodeType](const AnimGraphNode* childNode)
        {
            return azrtti_typeid(childNode) == nodeType;
        });
        return foundChild != end(m_childNodes) ? *foundChild : nullptr;
    }


    bool AnimGraphNode::HasChildNodeOfType(const AZ::TypeId& nodeType) const
    {
        return AZStd::any_of(begin(m_childNodes), end(m_childNodes), [nodeType](const AnimGraphNode* childNode)
        {
            return azrtti_typeid(childNode) == nodeType;
        });
    }


    // does this node has a specific incoming connection?
    bool AnimGraphNode::GetHasConnection(AnimGraphNode* sourceNode, uint16 sourcePort, uint16 targetPort) const
    {
        return AZStd::any_of(begin(m_connections), end(m_connections), [sourceNode, sourcePort, targetPort](const BlendTreeConnection* connection)
        {
            return connection->GetSourceNode() == sourceNode && connection->GetSourcePort() == sourcePort && connection->GetTargetPort() == targetPort;
        });
    }

    // remove a given connection
    void AnimGraphNode::RemoveConnection(BlendTreeConnection* connection, bool delFromMem)
    {
        m_inputPorts[connection->GetTargetPort()].m_connection = nullptr;

        AnimGraphNode* sourceNode = connection->GetSourceNode();
        if (sourceNode)
        {
            sourceNode->m_outputPorts[connection->GetSourcePort()].m_connection = nullptr;
        }

        // Remove object by value.
        m_connections.erase(AZStd::remove(m_connections.begin(), m_connections.end(), connection), m_connections.end());
        if (delFromMem)
        {
            delete connection;
        }
    }


    // remove a given connection
    void AnimGraphNode::RemoveConnection(AnimGraphNode* sourceNode, uint16 sourcePort, uint16 targetPort)
    {
        // for all input connections
        for (BlendTreeConnection* connection : m_connections)
        {
            if (connection->GetSourceNode() == sourceNode && connection->GetSourcePort() == sourcePort && connection->GetTargetPort() == targetPort)
            {
                RemoveConnection(connection, true);
                return;
            }
        }
    }


    bool AnimGraphNode::RemoveConnectionById(AnimGraphConnectionId connectionId, bool delFromMem)
    {
        const size_t numConnections = m_connections.size();
        for (size_t i = 0; i < numConnections; ++i)
        {
            if (m_connections[i]->GetId() == connectionId)
            {
                m_inputPorts[m_connections[i]->GetTargetPort()].m_connection = nullptr;

                AnimGraphNode* sourceNode = m_connections[i]->GetSourceNode();
                if (sourceNode)
                {
                    sourceNode->m_outputPorts[m_connections[i]->GetSourcePort()].m_connection = nullptr;
                }

                if (delFromMem)
                {
                    delete m_connections[i];
                }

                m_connections.erase(m_connections.begin() + i);
            }
        }

        return false;
    }


    // find a given connection
    BlendTreeConnection* AnimGraphNode::FindConnection(const AnimGraphNode* sourceNode, uint16 sourcePort, uint16 targetPort) const
    {
        for (BlendTreeConnection* connection : m_connections)
        {
            if (connection->GetSourceNode() == sourceNode && connection->GetSourcePort() == sourcePort && connection->GetTargetPort() == targetPort)
            {
                return connection;
            }
        }

        return nullptr;
    }



    // initialize the input ports
    void AnimGraphNode::InitInputPorts(size_t numPorts)
    {
        m_inputPorts.resize(numPorts);
    }


    // initialize the output ports
    void AnimGraphNode::InitOutputPorts(size_t numPorts)
    {
        m_outputPorts.resize(numPorts);
    }


    // find a given output port number
    size_t AnimGraphNode::FindOutputPortIndex(const AZStd::string& name) const
    {
        const auto foundPort = AZStd::find_if(begin(m_outputPorts), end(m_outputPorts), [&name](const Port& port)
        {
            return port.GetNameString() == name;
        });
        return foundPort != end(m_outputPorts) ? AZStd::distance(begin(m_outputPorts), foundPort) : InvalidIndex;
    }


    // find a given input port number
    size_t AnimGraphNode::FindInputPortIndex(const AZStd::string& name) const
    {
        const auto foundPort = AZStd::find_if(begin(m_inputPorts), end(m_inputPorts), [&name](const Port& port)
        {
            return port.GetNameString() == name;
        });
        return foundPort != end(m_inputPorts) ? AZStd::distance(begin(m_inputPorts), foundPort) : InvalidIndex;
    }


    // add an output port and return its index
    size_t AnimGraphNode::AddOutputPort()
    {
        const size_t currentSize = m_outputPorts.size();
        m_outputPorts.emplace_back();
        return currentSize;
    }


    // add an input port, and return its index
    size_t AnimGraphNode::AddInputPort()
    {
        const size_t currentSize = m_inputPorts.size();
        m_inputPorts.emplace_back();
        return static_cast<uint32>(currentSize);
    }


    // setup a port name
    void AnimGraphNode::SetInputPortName(size_t portIndex, const char* name)
    {
        MCORE_ASSERT(portIndex < m_inputPorts.size());
        m_inputPorts[portIndex].m_nameId = MCore::GetStringIdPool().GenerateIdForString(name);
    }


    // setup a port name
    void AnimGraphNode::SetOutputPortName(size_t portIndex, const char* name)
    {
        MCORE_ASSERT(portIndex < m_outputPorts.size());
        m_outputPorts[portIndex].m_nameId = MCore::GetStringIdPool().GenerateIdForString(name);
    }


    // get the total number of children
    size_t AnimGraphNode::RecursiveCalcNumNodes() const
    {
        size_t result = 0;
        for (const AnimGraphNode* childNode : m_childNodes)
        {
            childNode->RecursiveCountChildNodes(result);
        }

        return result;
    }


    // recursively count the number of nodes down the hierarchy
    void AnimGraphNode::RecursiveCountChildNodes(size_t& numNodes) const
    {
        // increase the counter
        numNodes++;

        for (const AnimGraphNode* childNode : m_childNodes)
        {
            childNode->RecursiveCountChildNodes(numNodes);
        }
    }


    // recursively calculate the number of node connections
    size_t AnimGraphNode::RecursiveCalcNumNodeConnections() const
    {
        size_t result = 0;
        RecursiveCountNodeConnections(result);
        return result;
    }


    // recursively calculate the number of node connections
    void AnimGraphNode::RecursiveCountNodeConnections(size_t& numConnections) const
    {
        // add the connections to our counter
        numConnections += GetNumConnections();

        for (const AnimGraphNode* childNode : m_childNodes)
        {
            childNode->RecursiveCountNodeConnections(numConnections);
        }
    }


    // setup an output port to output a given local pose
    void AnimGraphNode::SetupOutputPortAsPose(const char* name, size_t outputPortNr, uint32 portID)
    {
        // check if we already registered this port ID
        const size_t duplicatePort = FindOutputPortByID(portID);
        if (duplicatePort != InvalidIndex)
        {
            MCore::LogError("EMotionFX::AnimGraphNode::SetOutputPortAsPose() - There is already a port with the same ID (portID=%d existingPort='%s' newPort='%s' node='%s')", portID, m_outputPorts[duplicatePort].GetName(), name, RTTI_GetTypeName());
        }

        SetOutputPortName(outputPortNr, name);
        m_outputPorts[outputPortNr].Clear();
        m_outputPorts[outputPortNr].m_compatibleTypes[0] = AttributePose::TYPE_ID;    // setup the compatible types of this port
        m_outputPorts[outputPortNr].m_portId = portID;
    }


    // setup an output port to output a given motion instance
    void AnimGraphNode::SetupOutputPortAsMotionInstance(const char* name, size_t outputPortNr, uint32 portID)
    {
        // check if we already registered this port ID
        const size_t duplicatePort = FindOutputPortByID(portID);
        if (duplicatePort != InvalidIndex)
        {
            MCore::LogError("EMotionFX::AnimGraphNode::SetOutputPortAsMotionInstance() - There is already a port with the same ID (portID=%d existingPort='%s' newPort='%s' node='%s')", portID, m_outputPorts[duplicatePort].GetName(), name, RTTI_GetTypeName());
        }

        SetOutputPortName(outputPortNr, name);
        m_outputPorts[outputPortNr].Clear();
        m_outputPorts[outputPortNr].m_compatibleTypes[0] = AttributeMotionInstance::TYPE_ID;  // setup the compatible types of this port
        m_outputPorts[outputPortNr].m_portId = portID;
    }


    // setup an output port
    void AnimGraphNode::SetupOutputPort(const char* name, size_t outputPortNr, uint32 attributeTypeID, uint32 portID)
    {
        // check if we already registered this port ID
        const size_t duplicatePort = FindOutputPortByID(portID);
        if (duplicatePort != InvalidIndex)
        {
            MCore::LogError("EMotionFX::AnimGraphNode::SetOutputPort() - There is already a port with the same ID (portID=%d existingPort='%s' newPort='%s' name='%s')", portID, m_outputPorts[duplicatePort].GetName(), name, RTTI_GetTypeName());
        }

        SetOutputPortName(outputPortNr, name);
        m_outputPorts[outputPortNr].Clear();
        m_outputPorts[outputPortNr].m_compatibleTypes[0] = attributeTypeID;
        m_outputPorts[outputPortNr].m_portId = portID;
    }

    void AnimGraphNode::SetupInputPortAsVector3(const char* name, size_t inputPortNr, uint32 portID)
    {
        SetupInputPort(name, inputPortNr, AZStd::vector<uint32>{MCore::AttributeVector3::TYPE_ID, MCore::AttributeVector2::TYPE_ID, MCore::AttributeVector4::TYPE_ID}, portID);
    }

    void AnimGraphNode::SetupInputPortAsVector2(const char* name, size_t inputPortNr, uint32 portID)
    {
        SetupInputPort(name, inputPortNr, AZStd::vector<uint32>{MCore::AttributeVector2::TYPE_ID, MCore::AttributeVector3::TYPE_ID}, portID);
    }

    void AnimGraphNode::SetupInputPortAsVector4(const char* name, size_t inputPortNr, uint32 portID)
    {
        SetupInputPort(name, inputPortNr, AZStd::vector<uint32>{MCore::AttributeVector4::TYPE_ID, MCore::AttributeVector3::TYPE_ID}, portID);
    }

    void AnimGraphNode::SetupInputPort(const char* name, size_t inputPortNr, const AZStd::vector<uint32>& attributeTypeIDs, uint32 portID)
    {
        // Check if we already registered this port ID
        const size_t duplicatePort = FindInputPortByID(portID);
        if (duplicatePort != InvalidIndex)
        {
            MCore::LogError("EMotionFX::AnimGraphNode::SetInputPortAsNumber() - There is already a port with the same ID (portID=%d existingPort='%s' newPort='%s' node='%s')", portID, MCore::GetStringIdPool().GetName(m_inputPorts[duplicatePort].m_nameId).c_str(), name, RTTI_GetTypeName());
        }

        SetInputPortName(inputPortNr, name);
        m_inputPorts[inputPortNr].Clear();
        m_inputPorts[inputPortNr].m_portId = portID;
        m_inputPorts[inputPortNr].SetCompatibleTypes(attributeTypeIDs);
    }

    // setup an input port as a number (float/int/bool)
    void AnimGraphNode::SetupInputPortAsNumber(const char* name, size_t inputPortNr, uint32 portID)
    {
        // check if we already registered this port ID
        const size_t duplicatePort = FindInputPortByID(portID);
        if (duplicatePort != InvalidIndex)
        {
            MCore::LogError("EMotionFX::AnimGraphNode::SetInputPortAsNumber() - There is already a port with the same ID (portID=%d existingPort='%s' newPort='%s' node='%s')", portID, m_inputPorts[duplicatePort].GetName(), name, RTTI_GetTypeName());
        }

        SetInputPortName(inputPortNr, name);
        m_inputPorts[inputPortNr].Clear();
        m_inputPorts[inputPortNr].m_compatibleTypes[0] = MCore::AttributeFloat::TYPE_ID;
        m_inputPorts[inputPortNr].m_portId = portID;
    }

    void AnimGraphNode::SetupInputPortAsBool(const char* name, size_t inputPortNr, uint32 portID)
    {
        // check if we already registered this port ID
        const size_t duplicatePort = FindInputPortByID(portID);
        if (duplicatePort != InvalidIndex)
        {
            MCore::LogError("EMotionFX::AnimGraphNode::SetInputPortAsBool() - There is already a port with the same ID (portID=%d existingPort='%s' newPort='%s' node='%s')", portID, m_inputPorts[duplicatePort].GetName(), name, RTTI_GetTypeName());
        }

        SetInputPortName(inputPortNr, name);
        m_inputPorts[inputPortNr].Clear();
        m_inputPorts[inputPortNr].m_compatibleTypes[0] = MCore::AttributeBool::TYPE_ID;
        m_inputPorts[inputPortNr].m_compatibleTypes[1] = MCore::AttributeFloat::TYPE_ID;;
        m_inputPorts[inputPortNr].m_compatibleTypes[2] = MCore::AttributeInt32::TYPE_ID;
        m_inputPorts[inputPortNr].m_portId = portID;
    }

    // setup a given input port in a generic way
    void AnimGraphNode::SetupInputPort(const char* name, size_t inputPortNr, uint32 attributeTypeID, uint32 portID)
    {
        // check if we already registered this port ID
        const size_t duplicatePort = FindInputPortByID(portID);
        if (duplicatePort != InvalidIndex)
        {
            MCore::LogError("EMotionFX::AnimGraphNode::SetInputPort() - There is already a port with the same ID (portID=%d existingPort='%s' newPort='%s' node='%s')", portID, m_inputPorts[duplicatePort].GetName(), name, RTTI_GetTypeName());
        }

        SetInputPortName(inputPortNr, name);
        m_inputPorts[inputPortNr].Clear();
        m_inputPorts[inputPortNr].m_compatibleTypes[0] = attributeTypeID;
        m_inputPorts[inputPortNr].m_portId = portID;
    }


    void AnimGraphNode::RecursiveResetUniqueDatas(AnimGraphInstance* animGraphInstance)
    {
        ResetUniqueData(animGraphInstance);

        for (AnimGraphNode* childNode : m_childNodes)
        {
            childNode->RecursiveResetUniqueDatas(animGraphInstance);
        }
    }

    void AnimGraphNode::InvalidateUniqueData(AnimGraphInstance* animGraphInstance)
    {
        AnimGraphObject::InvalidateUniqueData(animGraphInstance);

        const size_t numActions = m_actionSetup.GetNumActions();
        for (size_t a = 0; a < numActions; ++a)
        {
            AnimGraphTriggerAction* action = m_actionSetup.GetAction(a);
            action->InvalidateUniqueData(animGraphInstance);
        }
    }

    void AnimGraphNode::RecursiveInvalidateUniqueDatas(AnimGraphInstance* animGraphInstance)
    {
        InvalidateUniqueData(animGraphInstance);

        for (AnimGraphNode* childNode : m_childNodes)
        {
            childNode->RecursiveInvalidateUniqueDatas(animGraphInstance);
        }
    }

    // get the input value for a given port
    const MCore::Attribute* AnimGraphNode::GetInputValue(AnimGraphInstance* animGraphInstance, size_t inputPort) const
    {
        MCORE_UNUSED(animGraphInstance);

        // get the connection that is plugged into the port
        BlendTreeConnection* connection = m_inputPorts[inputPort].m_connection;
        MCORE_ASSERT(connection); // make sure there is a connection plugged in, otherwise we can't read the value

        // get the value from the output port of the source node
        return connection->GetSourceNode()->GetOutputValue(animGraphInstance, connection->GetSourcePort());
    }


    // recursively reset the output is ready flag
    void AnimGraphNode::RecursiveResetFlags(AnimGraphInstance* animGraphInstance, uint32 flagsToReset)
    {
        // reset the flag in this node
        animGraphInstance->DisableObjectFlags(m_objectIndex, flagsToReset);

        if (GetEMotionFX().GetIsInEditorMode())
        {
            // reset all connections
            for (BlendTreeConnection* connection : m_connections)
            {
                connection->SetIsVisited(false);
            }
        }
        for (AnimGraphNode* childNode : m_childNodes)
        {
            childNode->RecursiveResetFlags(animGraphInstance, flagsToReset);
        }
    }


    // sync the current time with another node
    void AnimGraphNode::SyncPlayTime(AnimGraphInstance* animGraphInstance, AnimGraphNode* leaderNode)
    {
        const float leaderDuration = leaderNode->GetDuration(animGraphInstance);
        const float normalizedTime = (leaderDuration > MCore::Math::epsilon) ? leaderNode->GetCurrentPlayTime(animGraphInstance) / leaderDuration : 0.0f;
        SetCurrentPlayTimeNormalized(animGraphInstance, normalizedTime);
    }

    void AnimGraphNode::AutoSync(AnimGraphInstance* animGraphInstance, AnimGraphNode* leaderNode, float weight, ESyncMode syncMode, bool resync)
    {
        // exit if we don't want to sync or we have no leader node to sync to
        if (syncMode == SYNCMODE_DISABLED || leaderNode == nullptr)
        {
            return;
        }

        // if one of the tracks is empty, sync the full clip
        if (syncMode == SYNCMODE_TRACKBASED)
        {
            // get the sync tracks
            const AnimGraphSyncTrack* syncTrackA = leaderNode->FindOrCreateUniqueNodeData(animGraphInstance)->GetSyncTrack();
            const AnimGraphSyncTrack* syncTrackB = FindOrCreateUniqueNodeData(animGraphInstance)->GetSyncTrack();

            // if we have sync keys in both nodes, do the track based sync
            if (syncTrackA && syncTrackB && syncTrackA->GetNumEvents() > 0 && syncTrackB->GetNumEvents() > 0)
            {
                SyncUsingSyncTracks(animGraphInstance, leaderNode, syncTrackA, syncTrackB, weight, resync, /*modifyLeaderSpeed*/false);
                return;
            }
        }

        // we either have no evens inside the sync tracks in both nodes, or we just want to sync based on full clips
        SyncFullNode(animGraphInstance, leaderNode, weight, /*modifyLeaderSpeed*/false);
    }


    void AnimGraphNode::SyncFullNode(AnimGraphInstance* animGraphInstance, AnimGraphNode* leaderNode, float weight, bool modifyLeaderSpeed)
    {
        SyncPlaySpeeds(animGraphInstance, leaderNode, weight, modifyLeaderSpeed);
        SyncPlayTime(animGraphInstance, leaderNode);
    }


    // set the normalized time
    void AnimGraphNode::SetCurrentPlayTimeNormalized(AnimGraphInstance* animGraphInstance, float normalizedTime)
    {
        const float duration = GetDuration(animGraphInstance);
        SetCurrentPlayTime(animGraphInstance, normalizedTime * duration);
    }

    AZStd::tuple<float, float, float> AnimGraphNode::SyncPlaySpeeds(float playSpeedA, float durationA, float playSpeedB, float durationB, float weight)
    {
        const float timeRatio = (durationB > MCore::Math::epsilon) ? durationA / durationB : 0.0f;
        const float timeRatio2 = (durationA > MCore::Math::epsilon) ? durationB / durationA : 0.0f;
        const float factorA = AZ::Lerp(1.0f, timeRatio, weight);
        const float factorB = AZ::Lerp(timeRatio2, 1.0f, weight);
        const float interpolatedSpeed = AZ::Lerp(playSpeedA, playSpeedB, weight);

        return AZStd::make_tuple(interpolatedSpeed, factorA, factorB);
    }

    // sync blend the play speed of two nodes
    void AnimGraphNode::SyncPlaySpeeds(AnimGraphInstance* animGraphInstance, AnimGraphNode* leaderNode, float weight, bool modifyLeaderSpeed)
    {
        AnimGraphNodeData* uniqueDataA = leaderNode->FindOrCreateUniqueNodeData(animGraphInstance);
        AnimGraphNodeData* uniqueDataB = FindOrCreateUniqueNodeData(animGraphInstance);

        float factorA;
        float factorB;
        float interpolatedSpeed;
        AZStd::tie(interpolatedSpeed, factorA, factorB) = SyncPlaySpeeds(
            uniqueDataA->GetPlaySpeed(), uniqueDataA->GetDuration(),
            uniqueDataB->GetPlaySpeed(), uniqueDataB->GetDuration(),
            weight);

        if (modifyLeaderSpeed)
        {
            uniqueDataA->SetPlaySpeed(interpolatedSpeed * factorA);
        }

        uniqueDataB->SetPlaySpeed(interpolatedSpeed * factorB);
    }

    void AnimGraphNode::CalcSyncFactors(AnimGraphInstance* animGraphInstance, const AnimGraphNode* leaderNode, const AnimGraphNode* followerNode, ESyncMode syncMode, float weight, float* outLeaderFactor, float* outFollowerFactor, float* outPlaySpeed)
    {
        const AnimGraphNodeData* leaderUniqueData = leaderNode->FindOrCreateUniqueNodeData(animGraphInstance);
        const AnimGraphNodeData* followerUniqueData = followerNode->FindOrCreateUniqueNodeData(animGraphInstance);

        CalcSyncFactors(leaderUniqueData->GetPlaySpeed(), leaderUniqueData->GetSyncTrack(), leaderUniqueData->GetSyncIndex(), leaderUniqueData->GetDuration(),
            followerUniqueData->GetPlaySpeed(), followerUniqueData->GetSyncTrack(), followerUniqueData->GetSyncIndex(), followerUniqueData->GetDuration(),
            syncMode, weight, outLeaderFactor, outFollowerFactor, outPlaySpeed);
    }

    void AnimGraphNode::CalcSyncFactors(float leaderPlaySpeed, const AnimGraphSyncTrack* leaderSyncTrack, size_t leaderSyncTrackIndex, float leaderDuration,
        float followerPlaySpeed, const AnimGraphSyncTrack* followerSyncTrack, size_t followerSyncTrackIndex, float followerDuration,
        ESyncMode syncMode, float weight, float* outLeaderFactor, float* outFollowerFactor, float* outPlaySpeed)
    {
        // exit if we don't want to sync or we have no leader node to sync to
        if (syncMode == SYNCMODE_DISABLED)
        {
            *outLeaderFactor = 1.0f;
            *outFollowerFactor = 1.0f;

            // Use the leader/source state playspeed when transitioning, do not blend playspeeds if syncing is disabled.
            *outPlaySpeed = leaderPlaySpeed;
            return;
        }

        // Blend playspeeds only if syncing is enabled.
        *outPlaySpeed = AZ::Lerp(leaderPlaySpeed, followerPlaySpeed, weight);

        // if one of the tracks is empty, sync the full clip
        if (syncMode == SYNCMODE_TRACKBASED)
        {
            // if we have sync keys in both nodes, do the track based sync
            if (leaderSyncTrack && followerSyncTrack && leaderSyncTrack->GetNumEvents() > 0 && followerSyncTrack->GetNumEvents() > 0)
            {
                // if the sync indices are invalid, act like no syncing
                if (leaderSyncTrackIndex == InvalidIndex || followerSyncTrackIndex == InvalidIndex)
                {
                    *outLeaderFactor = 1.0f;
                    *outFollowerFactor = 1.0f;
                    return;
                }

                // get the segment lengths
                // TODO: handle motion clip start and end
                size_t leaderSyncIndexNext = leaderSyncTrackIndex + 1;
                if (leaderSyncIndexNext >= leaderSyncTrack->GetNumEvents())
                {
                    leaderSyncIndexNext = 0;
                }

                size_t followerSyncIndexNext = followerSyncTrackIndex + 1;
                if (followerSyncIndexNext >= followerSyncTrack->GetNumEvents())
                {
                    followerSyncIndexNext = 0;
                }

                const float durationA = leaderSyncTrack->CalcSegmentLength(leaderSyncTrackIndex, leaderSyncIndexNext);
                const float durationB = followerSyncTrack->CalcSegmentLength(followerSyncTrackIndex, followerSyncIndexNext);
                const float timeRatio = (durationB > MCore::Math::epsilon) ? durationA / durationB : 0.0f;
                const float timeRatio2 = (durationA > MCore::Math::epsilon) ? durationB / durationA : 0.0f;
                *outLeaderFactor = AZ::Lerp(1.0f, timeRatio, weight);
                *outFollowerFactor = AZ::Lerp(timeRatio2, 1.0f, weight);
                return;
            }
        }

        // calculate the factor based on full clip sync
        const float timeRatio = (followerDuration > MCore::Math::epsilon) ? leaderDuration / followerDuration : 0.0f;
        const float timeRatio2 = (leaderDuration > MCore::Math::epsilon) ? followerDuration / leaderDuration : 0.0f;
        *outLeaderFactor = AZ::Lerp(1.0f, timeRatio, weight);
        *outFollowerFactor = AZ::Lerp(timeRatio2, 1.0f, weight);
    }


    // recursively call the on change motion set callback function
    void AnimGraphNode::RecursiveOnChangeMotionSet(AnimGraphInstance* animGraphInstance, MotionSet* newMotionSet)
    {
        // callback call
        OnChangeMotionSet(animGraphInstance, newMotionSet);

        // get the number of child nodes, iterate through them and recursively call this function
        const size_t numChildNodes = GetNumChildNodes();
        for (size_t i = 0; i < numChildNodes; ++i)
        {
            m_childNodes[i]->RecursiveOnChangeMotionSet(animGraphInstance, newMotionSet);
        }
    }


    // perform syncing using the sync tracks
    void AnimGraphNode::SyncUsingSyncTracks(AnimGraphInstance* animGraphInstance, AnimGraphNode* syncWithNode, const AnimGraphSyncTrack* syncTrackA, const AnimGraphSyncTrack* syncTrackB, float weight, bool resync, bool modifyLeaderSpeed)
    {
        AnimGraphNode* nodeA = syncWithNode;
        AnimGraphNode* nodeB = this;

        AnimGraphNodeData* uniqueDataA = nodeA->FindOrCreateUniqueNodeData(animGraphInstance);
        AnimGraphNodeData* uniqueDataB = nodeB->FindOrCreateUniqueNodeData(animGraphInstance);

        // get the time of motion A
        const float currentTime = uniqueDataA->GetCurrentPlayTime();
        const bool forward = !uniqueDataA->GetIsBackwardPlaying();

        // get the event indices
        size_t firstIndexA;
        size_t firstIndexB;
        if (syncTrackA->FindEventIndices(currentTime, &firstIndexA, &firstIndexB) == false)
        {
            //MCORE_ASSERT(false);
            //MCore::LogInfo("FAILED FindEventIndices at time %f", currentTime);
            return;
        }

        // if the main motion changed event, we must make sure we also change it
        if (uniqueDataA->GetSyncIndex() != firstIndexA)
        {
            animGraphInstance->EnableObjectFlags(nodeA->GetObjectIndex(), AnimGraphInstance::OBJECTFLAGS_SYNCINDEX_CHANGED);
        }

        size_t startEventIndex = uniqueDataB->GetSyncIndex();
        if (animGraphInstance->GetIsObjectFlagEnabled(nodeA->GetObjectIndex(), AnimGraphInstance::OBJECTFLAGS_SYNCINDEX_CHANGED))
        {
            if (forward)
            {
                startEventIndex++;
            }
            else
            {
                startEventIndex--;
            }

            if (startEventIndex >= syncTrackB->GetNumEvents())
            {
                startEventIndex = 0;
            }

            if (startEventIndex == InvalidIndex)
            {
                startEventIndex = syncTrackB->GetNumEvents() - 1;
            }

            animGraphInstance->EnableObjectFlags(nodeB->GetObjectIndex(), AnimGraphInstance::OBJECTFLAGS_SYNCINDEX_CHANGED);
        }

        // find the matching indices in the second track
        size_t secondIndexA;
        size_t secondIndexB;
        if (resync == false)
        {
            if (syncTrackB->FindMatchingEvents(startEventIndex, syncTrackA->GetEvent(firstIndexA).HashForSyncing(uniqueDataA->GetIsMirrorMotion()), syncTrackA->GetEvent(firstIndexB).HashForSyncing(uniqueDataA->GetIsMirrorMotion()), &secondIndexA, &secondIndexB, forward, uniqueDataB->GetIsMirrorMotion()) == false)
            {
                //MCORE_ASSERT(false);
                //MCore::LogInfo("FAILED FindMatchingEvents");
                return;
            }
        }
        else // resyncing is required
        {
            const size_t occurrence = syncTrackA->CalcOccurrence(firstIndexA, firstIndexB, uniqueDataA->GetIsMirrorMotion());
            if (syncTrackB->ExtractOccurrence(occurrence, syncTrackA->GetEvent(firstIndexA).HashForSyncing(uniqueDataA->GetIsMirrorMotion()), syncTrackA->GetEvent(firstIndexB).HashForSyncing(uniqueDataA->GetIsMirrorMotion()), &secondIndexA, &secondIndexB, uniqueDataB->GetIsMirrorMotion()) == false)
            {
                //MCORE_ASSERT(false);
                //MCore::LogInfo("FAILED ExtractOccurrence");
                return;
            }
        }

        // update the sync indices
        uniqueDataA->SetSyncIndex(firstIndexA);
        uniqueDataB->SetSyncIndex(secondIndexA);

        // calculate the segment lengths
        const float firstSegmentLength  = syncTrackA->CalcSegmentLength(firstIndexA, firstIndexB);
        const float secondSegmentLength = syncTrackB->CalcSegmentLength(secondIndexA, secondIndexB);

        //MCore::LogInfo("t=%f firstA=%d firstB=%d occ=%d secA=%d secB=%d", currentTime, firstIndexA, firstIndexB, occurrence, secondIndexA, secondIndexB);

        // calculate the normalized offset inside the segment
        float normalizedOffset;
        if (firstIndexA < firstIndexB) // normal case
        {
            normalizedOffset = (firstSegmentLength > MCore::Math::epsilon) ? (currentTime - syncTrackA->GetEvent(firstIndexA).GetStartTime()) / firstSegmentLength : 0.0f;
        }
        else // looping case
        {
            float timeOffset;
            if (currentTime > syncTrackA->GetEvent(0).GetStartTime())
            {
                timeOffset = currentTime - syncTrackA->GetEvent(firstIndexA).GetStartTime();
            }
            else
            {
                timeOffset = (uniqueDataA->GetDuration() - syncTrackA->GetEvent(firstIndexA).GetStartTime()) + currentTime;
            }

            normalizedOffset = (firstSegmentLength > MCore::Math::epsilon) ? timeOffset / firstSegmentLength : 0.0f;
        }

        // get the durations of both nodes for later on
        const float durationA   = firstSegmentLength;
        const float durationB   = secondSegmentLength;

        // calculate the new time in the motion
        //  bool looped = false;
        float newTimeB;
        if (secondIndexA < secondIndexB) // if the second segment is a non-wrapping one, so a regular non-looping case
        {
            newTimeB = syncTrackB->GetEvent(secondIndexA).GetStartTime() + secondSegmentLength * normalizedOffset;
        }
        else // looping case
        {
            // calculate the new play time
            const float unwrappedTime = syncTrackB->GetEvent(secondIndexA).GetStartTime() + secondSegmentLength * normalizedOffset;

            // if it is past the motion duration, we need to wrap around
            if (unwrappedTime > uniqueDataB->GetDuration())
            {
                // the new wrapped time
                newTimeB = MCore::Math::SafeFMod(unwrappedTime, uniqueDataB->GetDuration()); // TODO: doesn't take into account the motion start and end clip times
            }
            else
            {
                newTimeB = unwrappedTime;
            }
        }

        // adjust the play speeds
        nodeB->SetCurrentPlayTime(animGraphInstance, newTimeB);
        const float timeRatio       = (durationB > MCore::Math::epsilon) ? durationA / durationB : 0.0f;
        const float timeRatio2      = (durationA > MCore::Math::epsilon) ? durationB / durationA : 0.0f;
        const float factorA         = AZ::Lerp(1.0f, timeRatio, weight);
        const float factorB         = AZ::Lerp(timeRatio2, 1.0f, weight);
        const float interpolatedSpeed         = AZ::Lerp(uniqueDataA->GetPlaySpeed(), uniqueDataB->GetPlaySpeed(), weight);

        if (modifyLeaderSpeed)
        {
            uniqueDataA->SetPlaySpeed(interpolatedSpeed * factorA);
        }

        uniqueDataB->SetPlaySpeed(interpolatedSpeed * factorB);
    }


    // check if the given node is the parent or the parent of the parent etc. of the node
    bool AnimGraphNode::RecursiveIsParentNode(const AnimGraphNode* node) const
    {
        // if we're dealing with a root node we can directly return failure
        if (!m_parentNode)
        {
            return false;
        }

        // check if the parent is the node and return success in that case
        if (m_parentNode == node)
        {
            return true;
        }

        // check if the parent's parent is the node we're searching for
        return m_parentNode->RecursiveIsParentNode(node);
    }


    // check if the given node is a child or a child of a chid etc. of the node
    bool AnimGraphNode::RecursiveIsChildNode(AnimGraphNode* node) const
    {
        // check if the given node is a child node of the current node
        if (FindChildNodeIndex(node) != InvalidIndex)
        {
            return true;
        }

        return AZStd::any_of(begin(m_childNodes), end(m_childNodes), [node](const AnimGraphNode* childNode)
        {
            return childNode->RecursiveIsChildNode(node);
        });
    }


    void AnimGraphNode::SetHasError(AnimGraphObjectData* uniqueData, bool hasError)
    {
        // nothing to change, return directly, only update when something changed
        if (uniqueData->GetHasError() == hasError)
        {
            return;
        }

        uniqueData->SetHasError(hasError);

        // sync the current node
        SyncVisualObject();

        // in case the parent node is valid check the error status of the parent by checking all children recursively and set that value
        if (m_parentNode)
        {
            AnimGraphObjectData* parentUniqueData = m_parentNode->FindOrCreateUniqueNodeData(uniqueData->GetAnimGraphInstance());
            if (hasError)
            {
                m_parentNode->SetHasError(parentUniqueData, true);
            }
            else if (!m_parentNode->HierarchicalHasError(parentUniqueData, true))
            {
                // In case we are clearing this error, we need to check if this node siblings have errors to clear the parent.
                m_parentNode->SetHasError(parentUniqueData, false);
            }
        }
    }

    bool AnimGraphNode::HierarchicalHasError(AnimGraphObjectData* uniqueData, bool onlyCheckChildNodes) const
    {
        if (!onlyCheckChildNodes && uniqueData->GetHasError())
        {
            return true;
        }

        for (const AnimGraphNode* childNode : m_childNodes)
        {
            AnimGraphObjectData* childUniqueData = childNode->FindOrCreateUniqueNodeData(uniqueData->GetAnimGraphInstance());
            if (childUniqueData->GetHasError())
            {
                return true;
            }
        }

        // no erroneous node found
        return false;
    }


    // collect child nodes of the given type
    void AnimGraphNode::CollectChildNodesOfType(const AZ::TypeId& nodeType, AZStd::vector<AnimGraphNode*>* outNodes) const
    {
        for (AnimGraphNode* childNode : m_childNodes)
        {
            // check the current node type and add it to the output array in case they are the same
            if (azrtti_typeid(childNode) == nodeType)
            {
                outNodes->emplace_back(childNode);
            }
        }
    }

    void AnimGraphNode::CollectChildNodesOfType(const AZ::TypeId& nodeType, AZStd::vector<AnimGraphNode*>& outNodes) const
    {
        for (AnimGraphNode* childNode : m_childNodes)
        {
            if (azrtti_typeid(childNode) == nodeType)
            {
                outNodes.push_back(childNode);
            }
        }
    }

    void AnimGraphNode::RecursiveCollectNodesOfType(const AZ::TypeId& nodeType, AZStd::vector<AnimGraphNode*>* outNodes) const
    {
        // check the current node type
        if (nodeType == azrtti_typeid(this))
        {
            outNodes->emplace_back(const_cast<AnimGraphNode*>(this));
        }

        for (const AnimGraphNode* childNode : m_childNodes)
        {
            childNode->RecursiveCollectNodesOfType(nodeType, outNodes);
        }
    }

    void AnimGraphNode::RecursiveCollectTransitionConditionsOfType(const AZ::TypeId& conditionType, AZStd::vector<AnimGraphTransitionCondition*>* outConditions) const
    {
        // check if the current node is a state machine
        if (azrtti_typeid(this) == azrtti_typeid<AnimGraphStateMachine>())
        {
            // type cast the node to a state machine
            const AnimGraphStateMachine* stateMachine = static_cast<AnimGraphStateMachine*>(const_cast<AnimGraphNode*>(this));

            // get the number of transitions and iterate through them
            const size_t numTransitions = stateMachine->GetNumTransitions();
            for (size_t i = 0; i < numTransitions; ++i)
            {
                const AnimGraphStateTransition* transition = stateMachine->GetTransition(i);

                // get the number of conditions and iterate through them
                const size_t numConditions = transition->GetNumConditions();
                for (size_t j = 0; j < numConditions; ++j)
                {
                    // check if the given condition is of the given type and add it to the output array in this case
                    AnimGraphTransitionCondition* condition = transition->GetCondition(j);
                    if (azrtti_typeid(condition) == conditionType)
                    {
                        outConditions->emplace_back(condition);
                    }
                }
            }
        }

        for (const AnimGraphNode* childNode : m_childNodes)
        {
            childNode->RecursiveCollectTransitionConditionsOfType(conditionType, outConditions);
        }
    }


    void AnimGraphNode::RecursiveCollectObjectsOfType(const AZ::TypeId& objectType, AZStd::vector<AnimGraphObject*>& outObjects) const
    {
        if (azrtti_istypeof(objectType, this))
        {
            outObjects.emplace_back(const_cast<AnimGraphNode*>(this));
        }

        for (const AnimGraphNode* childNode : m_childNodes)
        {
            childNode->RecursiveCollectObjectsOfType(objectType, outObjects);
        }
    }
    
    void AnimGraphNode::RecursiveCollectObjectsAffectedBy(AnimGraph* animGraph, AZStd::vector<AnimGraphObject*>& outObjects) const 
    {
        for (const AnimGraphNode* childNode : m_childNodes)
        {
            childNode->RecursiveCollectObjectsAffectedBy(animGraph, outObjects);
        }
    }

    void AnimGraphNode::OnStateEnter(AnimGraphInstance* animGraphInstance, AnimGraphNode* previousState, AnimGraphStateTransition* usedTransition)
    {
        MCORE_UNUSED(animGraphInstance);
        MCORE_UNUSED(previousState);
        MCORE_UNUSED(usedTransition);

        // Note: The enter action will only trigger when NOT entering from same state (because then you are not actually entering the state).
        if (this != previousState)
        {
            // Trigger the action at the enter of state.
            for (AnimGraphTriggerAction* action : m_actionSetup.GetActions())
            {
                if (action->GetTriggerMode() == AnimGraphTriggerAction::MODE_TRIGGERONENTER)
                {
                    action->TriggerAction(animGraphInstance);
                }
            }
        }
    }


    void AnimGraphNode::OnStateEnd(AnimGraphInstance* animGraphInstance, AnimGraphNode* newState, AnimGraphStateTransition* usedTransition)
    {
        MCORE_UNUSED(animGraphInstance);
        MCORE_UNUSED(newState);
        MCORE_UNUSED(usedTransition);

        // Note: The end of state action will only trigger when NOT entering the same state (because then you are not actually exiting the state).
        if (this != newState)
        {
            // Trigger the action when entering the state.
            for (AnimGraphTriggerAction* action : m_actionSetup.GetActions())
            {
                if (action->GetTriggerMode() == AnimGraphTriggerAction::MODE_TRIGGERONEXIT)
                {
                    action->TriggerAction(animGraphInstance);
                }
            }
        }
    }


    // find the input port, based on the port name
    AnimGraphNode::Port* AnimGraphNode::FindInputPortByName(const AZStd::string& portName)
    {
        const auto foundPort = AZStd::find_if(begin(m_inputPorts), end(m_inputPorts), [&portName](const Port& port)
        {
            return port.GetNameString() == portName;
        });
        return foundPort != end(m_inputPorts) ? foundPort : nullptr;
    }


    // find the output port, based on the port name
    AnimGraphNode::Port* AnimGraphNode::FindOutputPortByName(const AZStd::string& portName)
    {
        const auto foundPort = AZStd::find_if(begin(m_outputPorts), end(m_outputPorts), [&portName](const Port& port)
        {
            return port.GetNameString() == portName;
        });
        return foundPort != end(m_outputPorts) ? foundPort : nullptr;
    }


    // find the input port index, based on the port id
    size_t AnimGraphNode::FindInputPortByID(uint32 portID) const
    {
        const auto foundPort = AZStd::find_if(begin(m_inputPorts), end(m_inputPorts), [portID](const Port& port)
        {
            return port.m_portId == portID;
        });
        return foundPort != end(m_inputPorts) ? AZStd::distance(begin(m_inputPorts), foundPort) : InvalidIndex;
    }


    // find the output port index, based on the port id
    size_t AnimGraphNode::FindOutputPortByID(uint32 portID) const
    {
        const auto foundPort = AZStd::find_if(begin(m_outputPorts), end(m_outputPorts), [portID](const Port& port)
        {
            return port.m_portId == portID;
        });
        return foundPort != end(m_outputPorts) ? AZStd::distance(begin(m_outputPorts), foundPort) : InvalidIndex;
    }


    // this function will get called to rewind motion nodes as well as states etc. to reset several settings when a state gets exited
    void AnimGraphNode::Rewind(AnimGraphInstance* animGraphInstance)
    {
        // on default only reset the current time back to the start
        SetCurrentPlayTimeNormalized(animGraphInstance, 0.0f);
    }


    // collect all outgoing connections
    void AnimGraphNode::CollectOutgoingConnections(AZStd::vector<AZStd::pair<BlendTreeConnection*, AnimGraphNode*> >& outConnections) const
    {
        outConnections.clear();

        // if we don't have a parent node we cannot proceed
        if (!m_parentNode)
        {
            return;
        }

        for (AnimGraphNode* childNode : m_parentNode->GetChildNodes())
        {
            // Skip this child if the child is this node
            if (childNode == this)
            {
                continue;
            }

            for (BlendTreeConnection* connection : childNode->GetConnections())
            {
                // check if the connection comes from this node, if so add it to our output array
                if (connection->GetSourceNode() == this)
                {
                    outConnections.emplace_back(connection, childNode);
                }
            }
        }
    }


    void AnimGraphNode::CollectOutgoingConnections(AZStd::vector<AZStd::pair<BlendTreeConnection*, AnimGraphNode*> >& outConnections, const size_t portIndex) const
    {
        outConnections.clear();

        if (!m_parentNode)
        {
            return;
        }

        for (AnimGraphNode* childNode : m_parentNode->GetChildNodes())
        {
            // Skip this child if the child is this node
            if (childNode == this)
            {
                continue;
            }

            for (BlendTreeConnection* connection : childNode->GetConnections())
            {
                if (connection->GetSourceNode() == this && connection->GetSourcePort() == portIndex)
                {
                    outConnections.emplace_back(connection, childNode);
                }
            }
        }
    }


    // find and return the connection connected to the given port
    BlendTreeConnection* AnimGraphNode::FindConnection(uint16 port) const
    {
        // get the number of connections and iterate through them
        const size_t numConnections = GetNumConnections();
        for (size_t i = 0; i < numConnections; ++i)
        {
            // get the current connection and check if the connection is connected to the given port
            BlendTreeConnection* connection = GetConnection(i);
            if (connection->GetTargetPort() == port)
            {
                return connection;
            }
        }

        // failure, there is no connection connected to the given port
        return nullptr;
    }


    BlendTreeConnection* AnimGraphNode::FindConnectionById(AnimGraphConnectionId connectionId) const
    {
        for (BlendTreeConnection* connection : m_connections)
        {
            if (connection->GetId() == connectionId)
            {
                return connection;
            }
        }

        return nullptr;
    }


    bool AnimGraphNode::HasConnectionAtInputPort(AZ::u32 inputPortNr) const
    {
        const Port& inputPort = m_inputPorts[inputPortNr];
        return inputPort.m_connection != nullptr;
    }


    // callback that gets called before a node gets removed
    void AnimGraphNode::OnRemoveNode(AnimGraph* animGraph, AnimGraphNode* nodeToRemove)
    {
        for (AnimGraphNode* childNode : m_childNodes)
        {
            childNode->OnRemoveNode(animGraph, nodeToRemove);
        }
    }


    // collect internal objects
    void AnimGraphNode::RecursiveCollectObjects(AZStd::vector<AnimGraphObject*>& outObjects) const
    {
        outObjects.emplace_back(const_cast<AnimGraphNode*>(this));

        for (const AnimGraphNode* childNode : m_childNodes)
        {
            childNode->RecursiveCollectObjects(outObjects);
        }
    }


    // topdown update
    void AnimGraphNode::TopDownUpdate(AnimGraphInstance* animGraphInstance, float timePassedInSeconds)
    {
        // get the unique data
        AnimGraphNodeData* uniqueData = FindOrCreateUniqueNodeData(animGraphInstance);
        HierarchicalSyncAllInputNodes(animGraphInstance, uniqueData);

        // top down update all incoming connections
        for (BlendTreeConnection* connection : m_connections)
        {
            TopDownUpdateIncomingNode(animGraphInstance, connection->GetSourceNode(), timePassedInSeconds);
        }
    }

    // default update implementation
    void AnimGraphNode::Output(AnimGraphInstance* animGraphInstance)
    {
        OutputAllIncomingNodes(animGraphInstance);
    }


    // default update implementation
    void AnimGraphNode::Update(AnimGraphInstance* animGraphInstance, float timePassedInSeconds)
    {
        // get the unique data
        AnimGraphNodeData* uniqueData = FindOrCreateUniqueNodeData(animGraphInstance);

        // iterate over all incoming connections
        bool syncTrackFound = false;
        size_t connectionIndex = InvalidIndex;
        const size_t numConnections = m_connections.size();
        for (size_t i = 0; i < numConnections; ++i)
        {
            const BlendTreeConnection* connection = m_connections[i];
            AnimGraphNode* sourceNode = connection->GetSourceNode();

            // update the node
            UpdateIncomingNode(animGraphInstance, sourceNode, timePassedInSeconds);

            // use the sync track of the first input port of this node
            if (connection->GetTargetPort() == 0 && sourceNode->GetHasOutputPose())
            {
                syncTrackFound = true;
                connectionIndex = i;
            }
        }

        if (connectionIndex != InvalidIndex)
        {
            uniqueData->Init(animGraphInstance, m_connections[connectionIndex]->GetSourceNode());
        }

        // set the current sync track to the first input connection
        if (!syncTrackFound && numConnections > 0 && m_connections[0]->GetSourceNode()->GetHasOutputPose()) // just pick the first connection's sync track
        {
            uniqueData->Init(animGraphInstance, m_connections[0]->GetSourceNode());
        }
    }


    // output all incoming nodes
    void AnimGraphNode::OutputAllIncomingNodes(AnimGraphInstance* animGraphInstance)
    {
        for (const BlendTreeConnection* connection : m_connections)
        {
            OutputIncomingNode(animGraphInstance, connection->GetSourceNode());
        }
    }


    // update a specific node
    void AnimGraphNode::UpdateIncomingNode(AnimGraphInstance* animGraphInstance, AnimGraphNode* node, float timePassedInSeconds)
    {
        EMFX_ANIMGRAPH_PROFILE_INCOMING_NODE(animGraphInstance, ProfileMode::Update);
        if (!node)
        {
            return;
        }
        node->PerformUpdate(animGraphInstance, timePassedInSeconds);
    }


    // update all incoming nodes
    void AnimGraphNode::UpdateAllIncomingNodes(AnimGraphInstance* animGraphInstance, float timePassedInSeconds)
    {
        EMFX_ANIMGRAPH_PROFILE_INCOMING_NODE(animGraphInstance, ProfileMode::Update);
        for (const BlendTreeConnection* connection : m_connections)
        {
            AnimGraphNode* sourceNode = connection->GetSourceNode();
            sourceNode->PerformUpdate(animGraphInstance, timePassedInSeconds);
        }
    }


    // mark the connections going to a given node as visited
    void AnimGraphNode::MarkConnectionVisited(AnimGraphNode* sourceNode)
    {
        if (GetEMotionFX().GetIsInEditorMode())
        {
            for (BlendTreeConnection* connection : m_connections)
            {
                if (connection->GetSourceNode() == sourceNode)
                {
                    connection->SetIsVisited(true);
                }
            }
        }
    }


    // output a node
    void AnimGraphNode::OutputIncomingNode(AnimGraphInstance* animGraphInstance, AnimGraphNode* nodeToOutput)
    {
        EMFX_ANIMGRAPH_PROFILE_INCOMING_NODE(animGraphInstance, ProfileMode::Output);

        if (nodeToOutput == nullptr)
        {
            return;
        }

        // output the node
        nodeToOutput->PerformOutput(animGraphInstance);

        // mark any connection originating from this node as visited
        if (GetEMotionFX().GetIsInEditorMode())
        {
            for (BlendTreeConnection* connection : m_connections)
            {
                if (connection->GetSourceNode() == nodeToOutput)
                {
                    connection->SetIsVisited(true);
                }
            }
        }
    }

    // Process events and motion extraction delta.
    void AnimGraphNode::PostUpdate(AnimGraphInstance* animGraphInstance, float timePassedInSeconds)
    {
        // Post process all incoming nodes.
        bool poseFound = false;
        size_t connectionIndex = InvalidIndex;
        AZ::u16 minTargetPortIndex = MCORE_INVALIDINDEX16;
        const size_t numConnections = m_connections.size();
        for (size_t i = 0; i < numConnections; ++i)
        {
            const BlendTreeConnection* connection = m_connections[i];
            AnimGraphNode* sourceNode = connection->GetSourceNode();

            // update the node
            PostUpdateIncomingNode(animGraphInstance, sourceNode, timePassedInSeconds);

            // If the input node has no pose, we can skip to the next connection.
            if (!sourceNode->GetHasOutputPose())
            {
                continue;
            }

            // Find the first connection that plugs into a port that can take a pose.
            const AZ::u16 targetPortIndex = connection->GetTargetPort();
            if (m_inputPorts[targetPortIndex].m_compatibleTypes[0] == AttributePose::TYPE_ID)
            {
                poseFound = true;
                if (targetPortIndex < minTargetPortIndex)
                {
                    connectionIndex = i;
                    minTargetPortIndex = targetPortIndex;
                }
            }
        }

        // request the anim graph reference counted data objects
        RequestRefDatas(animGraphInstance);

        AnimGraphNodeData* uniqueData = FindOrCreateUniqueNodeData(animGraphInstance);
        if (poseFound && connectionIndex != InvalidIndex)
        {
            const BlendTreeConnection* connection = m_connections[connectionIndex];
            AnimGraphNode* sourceNode = connection->GetSourceNode();

            AnimGraphRefCountedData* data = uniqueData->GetRefCountedData();
            AnimGraphRefCountedData* sourceData = sourceNode->FindOrCreateUniqueNodeData(animGraphInstance)->GetRefCountedData();

            if (sourceData)
            {
                data->SetEventBuffer(sourceData->GetEventBuffer());
                data->SetTrajectoryDelta(sourceData->GetTrajectoryDelta());
                data->SetTrajectoryDeltaMirrored(sourceData->GetTrajectoryDeltaMirrored());
            }
        }
        else
        if (poseFound == false && numConnections > 0 && m_connections[0]->GetSourceNode()->GetHasOutputPose())
        {
            AnimGraphRefCountedData* data = uniqueData->GetRefCountedData();
            AnimGraphNode* sourceNode = m_connections[0]->GetSourceNode();
            AnimGraphRefCountedData* sourceData = sourceNode->FindOrCreateUniqueNodeData(animGraphInstance)->GetRefCountedData();
            data->SetEventBuffer(sourceData->GetEventBuffer());
            data->SetTrajectoryDelta(sourceData->GetTrajectoryDelta());
            data->SetTrajectoryDeltaMirrored(sourceData->GetTrajectoryDeltaMirrored());
        }
        else
        {
            if (poseFound == false)
            {
                AnimGraphRefCountedData* data = uniqueData->GetRefCountedData();
                data->ClearEventBuffer();
                data->ZeroTrajectoryDelta();
            }
        }
    }

    void AnimGraphNode::PostUpdateIncomingNode(AnimGraphInstance* animGraphInstance, AnimGraphNode* node, float timePassedInSeconds)
    {
        EMFX_ANIMGRAPH_PROFILE_INCOMING_NODE(animGraphInstance, ProfileMode::PostUpdate);
        if (!node)
        {
            return;
        }
        node->PerformPostUpdate(animGraphInstance, timePassedInSeconds);
        MarkConnectionVisited(node);
    }

    void AnimGraphNode::TopDownUpdateIncomingNode(AnimGraphInstance* animGraphInstance, AnimGraphNode* node, float timePassedInSeconds)
    {
        EMFX_ANIMGRAPH_PROFILE_INCOMING_NODE(animGraphInstance, ProfileMode::TopDown);
        if (!node)
        {
            return;
        }
        node->PerformTopDownUpdate(animGraphInstance, timePassedInSeconds);
        MarkConnectionVisited(node);
    }

    // recursively set object data flag
    void AnimGraphNode::RecursiveSetUniqueDataFlag(AnimGraphInstance* animGraphInstance, uint32 flag, bool enabled)
    {
        // set the flag
        animGraphInstance->SetObjectFlags(m_objectIndex, flag, enabled);

        // recurse downwards
        for (BlendTreeConnection* connection : m_connections)
        {
            connection->GetSourceNode()->RecursiveSetUniqueDataFlag(animGraphInstance, flag, enabled);
        }
    }

    void AnimGraphNode::FilterEvents(AnimGraphInstance* animGraphInstance, EEventMode eventMode, AnimGraphNode* nodeA, AnimGraphNode* nodeB, float localWeight, AnimGraphRefCountedData* refData)
    {
        AnimGraphRefCountedData* refDataA = nullptr;
        if (nodeA)
        {
            refDataA = nodeA->FindOrCreateUniqueNodeData(animGraphInstance)->GetRefCountedData();
        }

        FilterEvents(animGraphInstance, eventMode, refDataA, nodeB, localWeight, refData);
    }

    // filter the event based on a given event mode
    void AnimGraphNode::FilterEvents(AnimGraphInstance* animGraphInstance, EEventMode eventMode, AnimGraphRefCountedData* refDataNodeA, AnimGraphNode* nodeB, float localWeight, AnimGraphRefCountedData* refData)
    {
        switch (eventMode)
        {
        // Output nothing, so clear the output buffer.
        case EVENTMODE_NONE:
        {
            if (refData)
            {
                refData->GetEventBuffer().Clear();
            }
        }
        break;

        // only the leader
        case EVENTMODE_LEADERONLY:
        {
            if (refDataNodeA)
            {
                refData->SetEventBuffer(refDataNodeA->GetEventBuffer());
            }
        }
        break;

        // only the follower
        case EVENTMODE_FOLLOWERONLY:
        {
            if (nodeB)
            {
                AnimGraphRefCountedData* refDataNodeB = nodeB->FindOrCreateUniqueNodeData(animGraphInstance)->GetRefCountedData();
                if (refDataNodeB)
                {
                    refData->SetEventBuffer(refDataNodeB->GetEventBuffer());
                }
            }
            else if (refDataNodeA)
            {
                refData->SetEventBuffer(refDataNodeA->GetEventBuffer()); // leader is also follower
            }
        }
        break;

        // both nodes
        case EVENTMODE_BOTHNODES:
        {
            AnimGraphRefCountedData* refDataNodeB = nodeB ? nodeB->FindOrCreateUniqueNodeData(animGraphInstance)->GetRefCountedData() : nullptr;

            const size_t numEventsA = refDataNodeA ? refDataNodeA->GetEventBuffer().GetNumEvents() : 0;
            const size_t numEventsB = refDataNodeB ? refDataNodeB->GetEventBuffer().GetNumEvents() : 0;

            // resize to the right number of events already
            AnimGraphEventBuffer& eventBuffer = refData->GetEventBuffer();
            eventBuffer.Resize(numEventsA + numEventsB);

            // add the first node's events
            if (refDataNodeA)
            {
                const AnimGraphEventBuffer& eventBufferA = refDataNodeA->GetEventBuffer();
                for (size_t i = 0; i < numEventsA; ++i)
                {
                    eventBuffer.SetEvent(i, eventBufferA.GetEvent(i));
                }
            }

            if (refDataNodeB)
            {
                const AnimGraphEventBuffer& eventBufferB = refDataNodeB->GetEventBuffer();
                for (size_t i = 0; i < numEventsB; ++i)
                {
                    eventBuffer.SetEvent(numEventsA + i, eventBufferB.GetEvent(i));
                }
            }
        }
        break;

        // most active node's events
        case EVENTMODE_MOSTACTIVE:
        {
            // if the weight is lower than half, use the first node's events
            if (localWeight <= 0.5f)
            {
                if (refDataNodeA)
                {
                    refData->SetEventBuffer(refDataNodeA->GetEventBuffer());
                }
            }
            else // try to use the second node's events
            {
                if (nodeB)
                {
                    AnimGraphRefCountedData* refDataNodeB = nodeB->FindOrCreateUniqueNodeData(animGraphInstance)->GetRefCountedData();
                    if (refDataNodeB)
                    {
                        refData->SetEventBuffer(refDataNodeB->GetEventBuffer());
                    }
                }
                else if (refDataNodeA)
                {
                       refData->SetEventBuffer(refDataNodeA->GetEventBuffer()); // leader is also follower
                }
            }
        }
        break;

        default:
            AZ_Assert(false, "Unknown event filter mode used!");
        };
    }


    // hierarchically sync input a given input node
    void AnimGraphNode::HierarchicalSyncInputNode(AnimGraphInstance* animGraphInstance, AnimGraphNode* inputNode, AnimGraphNodeData* uniqueDataOfThisNode)
    {
        AnimGraphNodeData* inputUniqueData = inputNode->FindOrCreateUniqueNodeData(animGraphInstance);

        if (animGraphInstance->GetIsSynced(inputNode->GetObjectIndex()))
        {
            inputNode->AutoSync(animGraphInstance, this, 0.0f, SYNCMODE_TRACKBASED, false);
        }
        else
        {
            inputUniqueData->SetPlaySpeed(uniqueDataOfThisNode->GetPlaySpeed()); // default child node speed propagation in case it is not synced
        }
        // pass the global weight along to the child nodes
        inputUniqueData->SetGlobalWeight(uniqueDataOfThisNode->GetGlobalWeight());
        inputUniqueData->SetLocalWeight(1.0f);
    }


    // hierarchically sync all input nodes
    void AnimGraphNode::HierarchicalSyncAllInputNodes(AnimGraphInstance* animGraphInstance, AnimGraphNodeData* uniqueDataOfThisNode)
    {
        // for all connections
        for (const BlendTreeConnection* connection : m_connections)
        {
            AnimGraphNode* inputNode = connection->GetSourceNode();
            HierarchicalSyncInputNode(animGraphInstance, inputNode, uniqueDataOfThisNode);
        }
    }

    // recursively collect active animgraph nodes
    void AnimGraphNode::RecursiveCollectActiveNodes(AnimGraphInstance* animGraphInstance, AZStd::vector<AnimGraphNode*>* outNodes, const AZ::TypeId& nodeType) const
    {
        // check and add this node
        if (azrtti_typeid(this) == nodeType || nodeType.IsNull())
        {
            if (animGraphInstance->GetIsOutputReady(m_objectIndex)) // if we processed this node
            {
                outNodes->emplace_back(const_cast<AnimGraphNode*>(this));
            }
        }

        // process all child nodes (but only active ones)
        for (const AnimGraphNode* childNode : m_childNodes)
        {
            if (animGraphInstance->GetIsOutputReady(childNode->GetObjectIndex()))
            {
                childNode->RecursiveCollectActiveNodes(animGraphInstance, outNodes, nodeType);
            }
        }
    }


    void AnimGraphNode::RecursiveCollectActiveNetTimeSyncNodes(AnimGraphInstance * animGraphInstance, AZStd::vector<AnimGraphNode*>* outNodes) const
    {
        // Check and add this node
        if (GetNeedsNetTimeSync())
        {
            if (animGraphInstance->GetIsOutputReady(m_objectIndex)) // if we processed this node
            {
                outNodes->emplace_back(const_cast<AnimGraphNode*>(this));
            }
        }

        // Process all child nodes (but only active ones)
        for (const AnimGraphNode* childNode : m_childNodes)
        {
            if (animGraphInstance->GetIsOutputReady(childNode->GetObjectIndex()))
            {
                childNode->RecursiveCollectActiveNetTimeSyncNodes(animGraphInstance, outNodes);
            }
        }
    }


    bool AnimGraphNode::RecursiveDetectCycles(AZStd::unordered_set<const AnimGraphNode*>& nodes) const
    {
        for (const AnimGraphNode* childNode : m_childNodes)
        {
            if (nodes.find(childNode) != nodes.end())
            {
                return true;
            }

            if (childNode->RecursiveDetectCycles(nodes))
            {
                return true;
            }

            nodes.emplace(childNode);
        }

        return false;
    }


    // decrease the reference count
    void AnimGraphNode::DecreaseRef(AnimGraphInstance* animGraphInstance)
    {
        AnimGraphNodeData* uniqueData = FindOrCreateUniqueNodeData(animGraphInstance);
        if (uniqueData->GetPoseRefCount() == 0)
        {
            return;
        }

        uniqueData->DecreasePoseRefCount();
        if (uniqueData->GetPoseRefCount() > 0 || !GetHasOutputPose())
        {
            return;
        }

        const uint32 threadIndex = animGraphInstance->GetActorInstance()->GetThreadIndex();
        AnimGraphPosePool& posePool = GetEMotionFX().GetThreadData(threadIndex)->GetPosePool();
        const size_t numOutputs = m_outputPorts.size();
        for (size_t i = 0; i < numOutputs; ++i)
        {
            if (m_outputPorts[i].m_compatibleTypes[0] == AttributePose::TYPE_ID)
            {
                MCore::Attribute* attribute = GetOutputAttribute(animGraphInstance, i);
                MCORE_ASSERT(attribute->GetType() == AttributePose::TYPE_ID);

                AttributePose* poseAttribute = static_cast<AttributePose*>(attribute);
                AnimGraphPose* pose = poseAttribute->GetValue();
                if (pose)
                {
                    posePool.FreePose(pose);
                }
                poseAttribute->SetValue(nullptr);
            }
        }
    }


    // request poses from the pose cache for all output poses
    void AnimGraphNode::RequestPoses(AnimGraphInstance* animGraphInstance)
    {
        ActorInstance* actorInstance = animGraphInstance->GetActorInstance();
        const uint32 threadIndex = actorInstance->GetThreadIndex();

        AnimGraphPosePool& posePool = GetEMotionFX().GetThreadData(threadIndex)->GetPosePool();

        const size_t numOutputs = m_outputPorts.size();
        for (size_t i = 0; i < numOutputs; ++i)
        {
            if (m_outputPorts[i].m_compatibleTypes[0] == AttributePose::TYPE_ID)
            {
                MCore::Attribute* attribute = GetOutputAttribute(animGraphInstance, i);
                MCORE_ASSERT(attribute->GetType() == AttributePose::TYPE_ID);

                AnimGraphPose* pose = posePool.RequestPose(actorInstance);
                AttributePose* poseAttribute = static_cast<AttributePose*>(attribute);
                poseAttribute->SetValue(pose);
            }
        }
    }


    // free all poses from all incoming nodes
    void AnimGraphNode::FreeIncomingPoses(AnimGraphInstance* animGraphInstance)
    {
        for (const Port& inputPort : m_inputPorts)
        {
            const BlendTreeConnection* connection = inputPort.m_connection;
            if (connection)
            {
                AnimGraphNode* sourceNode = connection->GetSourceNode();
                sourceNode->DecreaseRef(animGraphInstance);
            }
        }
    }


    // free all poses from all incoming nodes
    void AnimGraphNode::FreeIncomingRefDatas(AnimGraphInstance* animGraphInstance)
    {
        for (const Port& port : m_inputPorts)
        {
            const BlendTreeConnection* connection = port.m_connection;
            if (connection)
            {
                AnimGraphNode* sourceNode = connection->GetSourceNode();
                sourceNode->DecreaseRefDataRef(animGraphInstance);
            }
        }
    }


    // request poses from the pose cache for all output poses
    void AnimGraphNode::RequestRefDatas(AnimGraphInstance* animGraphInstance)
    {
        ActorInstance* actorInstance = animGraphInstance->GetActorInstance();
        const uint32 threadIndex = actorInstance->GetThreadIndex();

        AnimGraphRefCountedDataPool& pool = GetEMotionFX().GetThreadData(threadIndex)->GetRefCountedDataPool();
        AnimGraphRefCountedData* newData = pool.RequestNew();

        FindOrCreateUniqueNodeData(animGraphInstance)->SetRefCountedData(newData);
    }


    // decrease the reference count
    void AnimGraphNode::DecreaseRefDataRef(AnimGraphInstance* animGraphInstance)
    {
        AnimGraphNodeData* uniqueData = FindOrCreateUniqueNodeData(animGraphInstance);
        if (uniqueData->GetRefDataRefCount() == 0)
        {
            return;
        }

        uniqueData->DecreaseRefDataRefCount();
        if (uniqueData->GetRefDataRefCount() > 0)
        {
            return;
        }

        // free it
        if (uniqueData->GetRefCountedData())
        {
            const uint32 threadIndex = animGraphInstance->GetActorInstance()->GetThreadIndex();
            AnimGraphRefCountedDataPool& pool = GetEMotionFX().GetThreadData(threadIndex)->GetRefCountedDataPool();
            pool.Free(uniqueData->GetRefCountedData());
            uniqueData->SetRefCountedData(nullptr);
        }
    }


    // perform top down update
    void AnimGraphNode::PerformTopDownUpdate(AnimGraphInstance* animGraphInstance, float timePassedInSeconds)
    {
        EMFX_ANIMGRAPH_PROFILE_NODE(animGraphInstance, ProfileMode::TopDown);

        // check if we already did update
        if (animGraphInstance->GetIsTopDownUpdateReady(m_objectIndex))
        {
            return;
        }

        // mark as done
        animGraphInstance->EnableObjectFlags(m_objectIndex, AnimGraphInstance::OBJECTFLAGS_TOPDOWNUPDATE_READY);

        TopDownUpdate(animGraphInstance, timePassedInSeconds);
    }


    // perform post update
    void AnimGraphNode::PerformPostUpdate(AnimGraphInstance* animGraphInstance, float timePassedInSeconds)
    {
        EMFX_ANIMGRAPH_PROFILE_NODE(animGraphInstance, ProfileMode::PostUpdate);

        // check if we already did update
        if (animGraphInstance->GetIsPostUpdateReady(m_objectIndex))
        {
            return;
        }

        // mark as done
        animGraphInstance->EnableObjectFlags(m_objectIndex, AnimGraphInstance::OBJECTFLAGS_POSTUPDATE_READY);

        // perform the actual post update
        PostUpdate(animGraphInstance, timePassedInSeconds);

        // free the incoming refs
        FreeIncomingRefDatas(animGraphInstance);
    }


    // perform an update
    void AnimGraphNode::PerformUpdate(AnimGraphInstance* animGraphInstance, float timePassedInSeconds)
    {
        EMFX_ANIMGRAPH_PROFILE_NODE(animGraphInstance, ProfileMode::Update);

        // check if we already did update
        if (animGraphInstance->GetIsUpdateReady(m_objectIndex))
        {
            return;
        }

        // mark as done
        animGraphInstance->EnableObjectFlags(m_objectIndex, AnimGraphInstance::OBJECTFLAGS_UPDATE_READY);

        // increase ref count for incoming nodes
        IncreaseInputRefCounts(animGraphInstance);
        IncreaseInputRefDataRefCounts(animGraphInstance);

        // perform the actual node update
        Update(animGraphInstance, timePassedInSeconds);
    }


    //
    void AnimGraphNode::PerformOutput(AnimGraphInstance* animGraphInstance)
    {
        EMFX_ANIMGRAPH_PROFILE_NODE(animGraphInstance, ProfileMode::Output);

        // check if we already did output
        if (animGraphInstance->GetIsOutputReady(m_objectIndex))
        {
            return;
        }

        // mark as done
        animGraphInstance->EnableObjectFlags(m_objectIndex, AnimGraphInstance::OBJECTFLAGS_OUTPUT_READY);

        // perform the output
        Output(animGraphInstance);

        // now decrease ref counts of all input nodes as we do not need the poses of this input node anymore for this node
        // once the pose ref count of a node reaches zero it will automatically release the poses back to the pool so they can be reused again by others
        FreeIncomingPoses(animGraphInstance);
    }


    // increase input ref counts
    void AnimGraphNode::IncreaseInputRefDataRefCounts(AnimGraphInstance* animGraphInstance)
    {
        for (const Port& port : m_inputPorts)
        {
            const BlendTreeConnection* connection = port.m_connection;
            if (connection)
            {
                AnimGraphNode* sourceNode = connection->GetSourceNode();
                sourceNode->IncreaseRefDataRefCount(animGraphInstance);
            }
        }
    }


    // increase input ref counts
    void AnimGraphNode::IncreaseInputRefCounts(AnimGraphInstance* animGraphInstance)
    {
        for (const Port& port : m_inputPorts)
        {
            const BlendTreeConnection* connection = port.m_connection;
            if (connection)
            {
                AnimGraphNode* sourceNode = connection->GetSourceNode();
                sourceNode->IncreasePoseRefCount(animGraphInstance);
            }
        }
    }


    void AnimGraphNode::RelinkPortConnections()
    {
        // After deserializing, nodes hold an array of incoming connections. Each node port caches a pointer to its connection object which we need to link.
        for (BlendTreeConnection* connection : m_connections)
        {
            AnimGraphNode* sourceNode = connection->GetSourceNode();
            const AZ::u16 targetPortNr = connection->GetTargetPort();
            const AZ::u16 sourcePortNr = connection->GetSourcePort();

            if (sourceNode)
            {
                if (sourcePortNr < sourceNode->m_outputPorts.size())
                {
                    sourceNode->GetOutputPort(sourcePortNr).m_connection = connection;
                }
                else
                {
                    AZ_Error("EMotionFX", false, "Can't link output port %i of '%s' with the connection going to %s at port %i.", sourcePortNr, sourceNode->GetName(), GetName(), targetPortNr);
                }
            }

            if (targetPortNr < m_inputPorts.size())
            {
                m_inputPorts[targetPortNr].m_connection = connection;
            }
            else
            {
                AZ_Error("EMotionFX", false, "Can't link input port %i of '%s' with the connection coming from %s at port %i.", targetPortNr, GetName(), sourceNode->GetName(), sourcePortNr);
            }
        }
    }


    // do we have a child of a given type?
    bool AnimGraphNode::CheckIfHasChildOfType(const AZ::TypeId& nodeType) const
    {
        for (const AnimGraphNode* childNode : m_childNodes)
        {
            if (azrtti_typeid(childNode) == nodeType)
            {
                return true;
            }
        }

        return false;
    }


    // check if we can visualize
    bool AnimGraphNode::GetCanVisualize(AnimGraphInstance* animGraphInstance) const
    {
        return (m_visEnabled && animGraphInstance->GetVisualizationEnabled() && EMotionFX::GetRecorder().GetIsInPlayMode() == false);
    }


    // remove internal attributes in all anim graph instances
    void AnimGraphNode::RemoveInternalAttributesForAllInstances()
    {
        // for all output ports
        for (Port& port : m_outputPorts)
        {
            const size_t internalAttributeIndex = port.m_attributeIndex;
            if (internalAttributeIndex != InvalidIndex)
            {
                const size_t numInstances = m_animGraph->GetNumAnimGraphInstances();
                for (size_t i = 0; i < numInstances; ++i)
                {
                    AnimGraphInstance* animGraphInstance = m_animGraph->GetAnimGraphInstance(i);
                    animGraphInstance->RemoveInternalAttribute(internalAttributeIndex);
                }

                m_animGraph->DecreaseInternalAttributeIndices(internalAttributeIndex);
                port.m_attributeIndex = InvalidIndex;
            }
        }
    }


    // decrease values higher than a given param value
    void AnimGraphNode::DecreaseInternalAttributeIndices(size_t decreaseEverythingHigherThan)
    {
        for (Port& port : m_outputPorts)
        {
            if (port.m_attributeIndex > decreaseEverythingHigherThan && port.m_attributeIndex != InvalidIndex)
            {
                port.m_attributeIndex--;
            }
        }
    }


    // init the internal attributes
    void AnimGraphNode::InitInternalAttributes(AnimGraphInstance* animGraphInstance)
    {
        // for all output ports of this node
        for (Port& port : m_outputPorts)
        {
            MCore::Attribute* newAttribute = MCore::GetAttributeFactory().CreateAttributeByType(port.m_compatibleTypes[0]); // assume compatibility type 0 to be the attribute type ID
            port.m_attributeIndex = animGraphInstance->AddInternalAttribute(newAttribute);
        }
    }


    void* AnimGraphNode::GetCustomData() const
    {
        return m_customData;
    }


    void AnimGraphNode::SetCustomData(void* dataPointer)
    {
        m_customData = dataPointer;
    }


    void AnimGraphNode::SetNodeInfo(const AZStd::string& info)
    {
        if (m_nodeInfo != info)
        {
            m_nodeInfo = info;

            SyncVisualObject();
        }
    }


    const AZStd::string& AnimGraphNode::GetNodeInfo() const
    {
        return m_nodeInfo;
    }


    bool AnimGraphNode::GetIsEnabled() const
    {
        return (m_disabled == false);
    }


    void AnimGraphNode::SetIsEnabled(bool enabled)
    {
        m_disabled = !enabled;
    }


    bool AnimGraphNode::GetIsCollapsed() const
    {
        return m_isCollapsed;
    }


    void AnimGraphNode::SetIsCollapsed(bool collapsed)
    {
        m_isCollapsed = collapsed;
    }


    void AnimGraphNode::SetVisualizeColor(const AZ::Color& color)
    {
        m_visualizeColor = color;
        SyncVisualObject();
    }


    const AZ::Color& AnimGraphNode::GetVisualizeColor() const
    {
        return m_visualizeColor;
    }


    void AnimGraphNode::SetVisualPos(int32 x, int32 y)
    {
        m_posX = x;
        m_posY = y;
    }


    int32 AnimGraphNode::GetVisualPosX() const
    {
        return m_posX;
    }


    int32 AnimGraphNode::GetVisualPosY() const
    {
        return m_posY;
    }


    bool AnimGraphNode::GetIsVisualizationEnabled() const
    {
        return m_visEnabled;
    }


    void AnimGraphNode::SetVisualization(bool enabled)
    {
        m_visEnabled = enabled;
    }


    void AnimGraphNode::AddChildNode(AnimGraphNode* node)
    {
        m_childNodes.push_back(node);
        node->SetParentNode(this);
    }


    void AnimGraphNode::ReserveChildNodes(size_t numChildNodes)
    {
        m_childNodes.reserve(numChildNodes);
    }


    const char* AnimGraphNode::GetName() const
    {
        return m_name.c_str();
    }


    const AZStd::string& AnimGraphNode::GetNameString() const
    {
        return m_name;
    }


    void AnimGraphNode::SetName(const char* name)
    {
        m_name = name;
    }

    void AnimGraphNode::ResetPoseRefCount(AnimGraphInstance* animGraphInstance)
    {
        AnimGraphNodeData* uniqueData = reinterpret_cast<AnimGraphNodeData*>(animGraphInstance->GetUniqueObjectData(m_objectIndex));
        if (uniqueData)
        {
            uniqueData->SetPoseRefCount(0);
        }
    }

    void AnimGraphNode::ResetRefDataRefCount(AnimGraphInstance* animGraphInstance)
    {
        AnimGraphNodeData* uniqueData = reinterpret_cast<AnimGraphNodeData*>(animGraphInstance->GetUniqueObjectData(m_objectIndex));
        if (uniqueData)
        {
            uniqueData->SetRefDataRefCount(0);
        }
    }

    void AnimGraphNode::GetAttributeStringForAffectedNodeIds(const AZStd::unordered_map<AZ::u64, AZ::u64>& convertedIds, AZStd::string& attributesString) const
    {
        // Default implementation is that the transition is not affected, therefore we don't do anything
        AZ_UNUSED(convertedIds);
        AZ_UNUSED(attributesString);
    }


    bool AnimGraphNode::VersionConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
    {
        const unsigned int version = classElement.GetVersion();
        if (version < 2)
        {
            int vizColorIndex = classElement.FindElement(AZ_CRC_CE("visualizeColor"));
            if (vizColorIndex > 0)
            {
                AZ::u32 oldColor;
                AZ::SerializeContext::DataElementNode& dataElementNode = classElement.GetSubElement(vizColorIndex);
                const bool result = dataElementNode.GetData<AZ::u32>(oldColor);
                if (!result)
                {
                    return false;
                }

                const AZ::Color convertedColor(
                    ((oldColor >> 16) & 0xff)/255.0f,
                    ((oldColor >> 8)  & 0xff)/255.0f,
                    (oldColor & 0xff)/255.0f,
                    1.0f
                );
                classElement.RemoveElement(vizColorIndex);
                classElement.AddElementWithData(context, "visualizeColor", convertedColor);
            }
        }
        return true;
    }


    void AnimGraphNode::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<AnimGraphNode, AnimGraphObject>()
            ->Version(2, VersionConverter)
            ->PersistentId([](const void* instance) -> AZ::u64 { return static_cast<const AnimGraphNode*>(instance)->GetId(); })
            ->Field("id", &AnimGraphNode::m_id)
            ->Field("name", &AnimGraphNode::m_name)
            ->Field("posX", &AnimGraphNode::m_posX)
            ->Field("posY", &AnimGraphNode::m_posY)
            ->Field("visualizeColor", &AnimGraphNode::m_visualizeColor)
            ->Field("isDisabled", &AnimGraphNode::m_disabled)
            ->Field("isCollapsed", &AnimGraphNode::m_isCollapsed)
            ->Field("isVisEnabled", &AnimGraphNode::m_visEnabled)
            ->Field("childNodes", &AnimGraphNode::m_childNodes)
            ->Field("connections", &AnimGraphNode::m_connections)
            ->Field("actionSetup", &AnimGraphNode::m_actionSetup);
            ;

        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (!editContext)
        {
            return;
        }

        editContext->Class<AnimGraphNode>("AnimGraphNode", "Base anim graph node")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
            ->Attribute(AZ::Edit::Attributes::AutoExpand, "")
            ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
            ->DataElement(AZ_CRC_CE("AnimGraphNodeName"), &AnimGraphNode::m_name, "Name", "Name of the node")
            ->Attribute(AZ_CRC_CE("AnimGraph"), &AnimGraphNode::GetAnimGraph)
        ;
    }


#if defined(EMFX_ANIMGRAPH_PROFILER_ENABLED)
    void AnimGraphNode::ClearProfileTimers(AnimGraphInstance* animGraphInstance)
    {
        AnimGraphNodeData* uniqueData = FindOrCreateUniqueNodeData(animGraphInstance);
        uniqueData->ClearUpdateTimes();
    }

    AZStd::chrono::microseconds AnimGraphNode::GetTotalUpdateTime(AnimGraphInstance* animGraphInstance) const
    {
        AnimGraphNodeData* uniqueData = FindOrCreateUniqueNodeData(animGraphInstance);
        return AZStd::chrono::duration_cast<AZStd::chrono::microseconds>(uniqueData->GetTotalUpdateTime());
    }

    AZStd::chrono::microseconds AnimGraphNode::GetUpdateTime(AnimGraphInstance* animGraphInstance) const
    {
        AnimGraphNodeData* uniqueData = FindOrCreateUniqueNodeData(animGraphInstance);
        return AZStd::chrono::duration_cast<AZStd::chrono::microseconds>(uniqueData->GetTotalUpdateTime() - uniqueData->GetInputNodesUpdateTime());
    }

    AnimGraphNode::ProfileSection::ProfileSection(AnimGraphNode* node, AnimGraphInstance* animGraphInstance, ProfileMode mode, bool incomingNode)
        : m_node(node)
        , m_animGraphInstance(animGraphInstance)
        , m_profileMode(mode)
        , m_isIncomingNode(incomingNode)
        , m_startPoint(AZStd::chrono::system_clock::now())
    {
    }

    AnimGraphNode::ProfileSection::~ProfileSection()
    {
        if (m_node->GetProfileMode() & static_cast<AZ::u8>(m_profileMode))
        {
            AZStd::chrono::nanoseconds duration = AZStd::chrono::system_clock::now() - m_startPoint;
            if (AnimGraphNodeData* uniqueData = m_node->FindOrCreateUniqueNodeData(m_animGraphInstance))
            {
                if (m_isIncomingNode)
                {
                    uniqueData->mInputNodesUpdateTime += duration;
                }
                else
                {
                    uniqueData->mTotalUpdateTime += duration;
                }
            }
        }
    }
#endif
} // namespace EMotionFX
