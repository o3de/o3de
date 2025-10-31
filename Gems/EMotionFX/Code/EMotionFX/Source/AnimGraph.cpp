/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Debug/Timer.h>
#include <AzCore/std/numeric.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/Utils.h>
#include <EMotionFX/Source/ActorInstance.h>
#include <EMotionFX/Source/Allocators.h>
#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/Source/AnimGraphInstance.h>
#include <EMotionFX/Source/AnimGraphManager.h>
#include <EMotionFX/Source/AnimGraphMotionNode.h>
#include <EMotionFX/Source/AnimGraphNodeGroup.h>
#include <EMotionFX/Source/AnimGraphObject.h>
#include <EMotionFX/Source/AnimGraphStateMachine.h>
#include <EMotionFX/Source/BlendTree.h>
#include <EMotionFX/Source/EMotionFXConfig.h>
#include <EMotionFX/Source/EventManager.h>
#include <EMotionFX/Source/Pose.h>
#include <EMotionFX/Source/Recorder.h>
#include <MCore/Source/IDGenerator.h>
#include <MCore/Source/LogManager.h>


namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(AnimGraph, AnimGraphAllocator)


    namespace internal
    {
        static constexpr AZStd::array GraphNodeColors = { AZ::u32{ 0xFF000080 },
                                                          0xFF00008B,
                                                          0xFF2A2AA5,
                                                          0xFF2222B2,
                                                          0xFF3C14DC,
                                                          0xFF0000FF,
                                                          0xFF4763FF,
                                                          0xFF507FFF,
                                                          0xFF5C5CCD,
                                                          0xFF8080F0,
                                                          0xFF7A96E9,
                                                          0xFF7280FA,
                                                          0xFF7AA0FF,
                                                          0xFF0045FF,
                                                          0xFF008CFF,
                                                          0xFF00A5FF,
                                                          0xFF00D7FF,
                                                          0xFF0B86B8,
                                                          0xFF20A5DA,
                                                          0xFFAAE8EE,
                                                          0xFF6BB7BD,
                                                          0xFF8CE6F0,
                                                          0xFF008080,
                                                          0xFF00FFFF,
                                                          0xFF32CD9A,
                                                          0xFF2F6B55,
                                                          0xFF238E6B,
                                                          0xFF00FC7C,
                                                          0xFF00FF7F,
                                                          0xFF2FFFAD,
                                                          0xFF006400,
                                                          0xFF008000,
                                                          0xFF228B22,
                                                          0xFF00FF00,
                                                          0xFF32CD32,
                                                          0xFF90EE90,
                                                          0xFF98FB98,
                                                          0xFF8FBC8F,
                                                          0xFF9AFA00,
                                                          0xFF7FFF00,
                                                          0xFF578B2E,
                                                          0xFFAACD66,
                                                          0xFF71B33C,
                                                          0xFFAAB220,
                                                          0xFF4F4F2F,
                                                          0xFF808000,
                                                          0xFF8B8B00,
                                                          0xFFFFFF00,
                                                          0xFFFFFF00,
                                                          0xFFFFFFE0,
                                                          0xFFD1CE00,
                                                          0xFFD0E040,
                                                          0xFFCCD148,
                                                          0xFFEEEEAF,
                                                          0xFFD4FF7F,
                                                          0xFFE6E0B0,
                                                          0xFFA09E5F,
                                                          0xFFB48246,
                                                          0xFFED9564,
                                                          0xFFFFBF00,
                                                          0xFFFF901E,
                                                          0xFFE6D8AD,
                                                          0xFFEBCE87,
                                                          0xFFFACE87,
                                                          0xFF701919,
                                                          0xFF800000,
                                                          0xFF8B0000,
                                                          0xFFCD0000,
                                                          0xFFFF0000,
                                                          0xFFE16941,
                                                          0xFFE22B8A,
                                                          0xFF82004B,
                                                          0xFF8B3D48,
                                                          0xFFCD5A6A,
                                                          0xFFEE687B,
                                                          0xFFDB7093,
                                                          0xFF8B008B,
                                                          0xFFD30094,
                                                          0xFFCC3299,
                                                          0xFFD355BA,
                                                          0xFF800080,
                                                          0xFFD8BFD8,
                                                          0xFFDDA0DD,
                                                          0xFFEE82EE,
                                                          0xFFFF00FF,
                                                          0xFFD670DA,
                                                          0xFF8515C7,
                                                          0xFF9370DB,
                                                          0xFF9314FF,
                                                          0xFFB469FF,
                                                          0xFFC1B6FF,
                                                          0xFFCBC0FF,
                                                          0xFFD7EBFA,
                                                          0xFFDCF5F5,
                                                          0xFFC4E4FF,
                                                          0xFFCDEBFF,
                                                          0xFFB3DEF5,
                                                          0xFFDCF8FF,
                                                          0xFFCDFAFF,
                                                          0xFFD2FAFA,
                                                          0xFFE0FFFF,
                                                          0xFF13458B,
                                                          0xFF2D52A0,
                                                          0xFF1E69D2,
                                                          0xFF3F85CD,
                                                          0xFF60A4F4,
                                                          0xFF87B8DE,
                                                          0xFF8CB4D2,
                                                          0xFF8F8FBC,
                                                          0xFFB5E4FF,
                                                          0xFFADDEFF,
                                                          0xFFB9DAFF,
                                                          0xFFE1E4FF,
                                                          0xFFF5F0FF,
                                                          0xFFE6F0FA,
                                                          0xFFE6F5FD,
                                                          0xFFD5EFFF,
                                                          0xFFEEF5FF,
                                                          0xFFFAFFF5,
                                                          0xFF908070,
                                                          0xFF998877,
                                                          0xFFDEC4B0,
                                                          0xFFFAE6E6,
                                                          0xFF696969,
                                                          0xFF808080,
                                                          0xFFA9A9A9,
                                                          0xFFC0C0C0,
                                                          0xFFD3D3D3 };
    }

    AZ::Color AnimGraph::RandomGraphColor() 
    {
        AZ::Color color;
        color.FromU32(internal::GraphNodeColors[rand() % internal::GraphNodeColors.size()]);
        return color;
    }

    AnimGraph::AnimGraph()
    {
        m_id = aznumeric_caster(MCore::GetIDGenerator().GenerateID());
        m_dirtyFlag = false;
        m_autoUnregister = true;
        m_retarget = false;
        m_rootStateMachine = nullptr;

#if defined(EMFX_DEVELOPMENT_BUILD)
        m_isOwnedByRuntime = false;
        m_isOwnedByAsset = false;
#endif // EMFX_DEVELOPMENT_BUILD

        // reserve some memory
        m_nodes.reserve(1024);

        // automatically register the anim graph
        GetAnimGraphManager().AddAnimGraph(this);

        GetEventManager().OnCreateAnimGraph(this);
    }


    AnimGraph::~AnimGraph()
    {
        GetEventManager().OnDeleteAnimGraph(this);
        GetRecorder().RemoveAnimGraphFromRecording(this);

        RemoveAllNodeGroups();

        if (m_rootStateMachine)
        {
            delete m_rootStateMachine;
        }

        // automatically unregister the anim graph
        if (m_autoUnregister)
        {
            GetAnimGraphManager().RemoveAnimGraph(this, false);
        }
    }


    void AnimGraph::RecursiveReinit()
    {
        if (!m_rootStateMachine)
        {
            return;
        }

        m_rootStateMachine->RecursiveReinit();
    }


    bool AnimGraph::InitAfterLoading()
    {
        if (!m_rootStateMachine)
        {
            return false;
        }

        // Cache the value parameters
        m_valueParameters = m_rootParameter.RecursivelyGetChildValueParameters();
        const size_t valueParameterSize = m_valueParameters.size();
        for (size_t i = 0; i < valueParameterSize; ++i)
        {
            m_valueParameterIndexByName.emplace(m_valueParameters[i]->GetName(), i);
        }

        return m_rootStateMachine->InitAfterLoading(this);
    }

    void AnimGraph::RecursiveInvalidateUniqueDatas()
    {
        for (AnimGraphInstance* animGraphInstance : m_animGraphInstances)
        {
            animGraphInstance->RecursiveInvalidateUniqueDatas();
        }
    }

    bool AnimGraph::AddParameter(Parameter* parameter, const GroupParameter* parentGroup)
    {
        if (!parentGroup)
        {
            parentGroup = &m_rootParameter;
        }
        if (m_rootParameter.AddParameter(parameter, parentGroup))
        {
            if (azrtti_typeid(parameter) != azrtti_typeid<GroupParameter>())
            {
                const AZ::Outcome<size_t> valueParameterIndex = m_rootParameter.FindValueParameterIndex(parameter);
                AZ_Assert(valueParameterIndex.IsSuccess(), "Expected to have a value value parameter index");
                m_valueParameters.insert(m_valueParameters.begin() + valueParameterIndex.GetValue(), static_cast<ValueParameter*>(parameter));
                AddValueParameterToIndexByNameCache(valueParameterIndex.GetValue(), parameter->GetName());
            }
            return true;
        }
        return false;
    }

    bool AnimGraph::InsertParameter(size_t index, Parameter* parameter, const GroupParameter* parent)
    {
        if (parent == nullptr)
        {
            parent = &m_rootParameter;
        }
        if (m_rootParameter.InsertParameter(index, parameter, parent))
        {
            if (azrtti_typeid(parameter) != azrtti_typeid<GroupParameter>())
            {
                const AZ::Outcome<size_t> valueParameterIndex = m_rootParameter.FindValueParameterIndex(parameter);
                AZ_Assert(valueParameterIndex.IsSuccess(), "Expected to have a value value parameter index");
                m_valueParameters.insert(m_valueParameters.begin() + valueParameterIndex.GetValue(), static_cast<ValueParameter*>(parameter));
                AddValueParameterToIndexByNameCache(valueParameterIndex.GetValue(), parameter->GetName());
            }
            return true;
        }
        return false;
    }

    bool AnimGraph::RenameParameter(Parameter* parameter, const AZStd::string& newName)
    {
        if (m_rootParameter.FindParameterByName(newName))
        {
            return false; // already a parameter named like that
        }

        const bool isValueParameter = azrtti_istypeof<EMotionFX::ValueParameter>(parameter);
        size_t oldIndex;
        if (isValueParameter)
        {
            AZStd::unordered_map<AZStd::string_view, size_t>::iterator cachedIndexIterator = m_valueParameterIndexByName.find(parameter->GetName());
            AZ_Assert(cachedIndexIterator != m_valueParameterIndexByName.end(), "Cached parameter indices are out of sync with the actual parameters.");

            // Store the value parameter index so that we can re-add it without looking it up after renaming and remove it from the cached indices.
            oldIndex = cachedIndexIterator->second;
            m_valueParameterIndexByName.erase(cachedIndexIterator);
        }

        parameter->SetName(newName);

        if (isValueParameter)
        {
            // Re-add the parameter index after renaming the parameter.
            m_valueParameterIndexByName.emplace(parameter->GetName(), oldIndex);
        }
        return true;
    }

    bool AnimGraph::RemoveParameter(Parameter* parameter)
    {
        if (azrtti_typeid(parameter) != azrtti_typeid<GroupParameter>())
        {
            const auto it = AZStd::find(m_valueParameters.begin(), m_valueParameters.end(), parameter);
            AZ_Assert(it != m_valueParameters.end(), "Expected to have the parameter in this anim graph");
            const size_t valueParameterIndex = AZStd::distance(m_valueParameters.begin(), it);
            m_valueParameters.erase(it);
            RemoveValueParameterToIndexByNameCache(valueParameterIndex, parameter->GetName());
        }

        return m_rootParameter.RemoveParameter(parameter);
    }

    size_t AnimGraph::GetNumValueParameters() const
    {
        return m_valueParameters.size();
    }

    size_t AnimGraph::GetNumParameters() const
    {
        return m_rootParameter.GetNumParameters();
    }


    // get a given parameter
    const ValueParameter* AnimGraph::FindValueParameter(size_t index) const
    {
        if (index < m_valueParameters.size())
        {
            return m_valueParameters[index];
        }
        return nullptr;
    }

    const Parameter* AnimGraph::FindParameter(size_t index) const
    {
        return m_rootParameter.FindParameter(index);
    }

    GroupParameterVector AnimGraph::RecursivelyGetGroupParameters() const
    {
        return m_rootParameter.RecursivelyGetChildGroupParameters();
    }

    const ValueParameterVector& AnimGraph::RecursivelyGetValueParameters() const
    {
        return m_valueParameters;
    }

    ValueParameterVector AnimGraph::GetChildValueParameters() const
    {
        return m_rootParameter.GetChildValueParameters();
    }

    const ParameterVector& AnimGraph::GetChildParameters() const
    {
        return m_rootParameter.GetChildParameters();
    }

    const Parameter* AnimGraph::FindParameterByName(const AZStd::string& paramName) const
    {
        return m_rootParameter.FindParameterByName(paramName);
    }

    const ValueParameter* AnimGraph::FindValueParameterByName(const AZStd::string& paramName) const
    {
        const AZStd::unordered_map<AZStd::string_view, size_t>::const_iterator it = m_valueParameterIndexByName.find(paramName);
        if (it != m_valueParameterIndexByName.end())
        {
            return m_valueParameters[it->second];
        }
        return nullptr;
    }

    AZ::Outcome<size_t> AnimGraph::FindParameterIndexByName(const AZStd::string& paramName) const
    {
        return m_rootParameter.FindParameterIndexByName(paramName);
    }

    AZ::Outcome<size_t> AnimGraph::FindValueParameterIndexByName(const AZStd::string& paramName) const
    {
        const AZStd::unordered_map<AZStd::string_view, size_t>::const_iterator it = m_valueParameterIndexByName.find(paramName);
        if (it != m_valueParameterIndexByName.end())
        {
            return AZ::Success(it->second);
        }
        return AZ::Failure();
    }

    AZ::Outcome<size_t> AnimGraph::FindParameterIndex(Parameter* parameter) const
    {
        return m_rootParameter.FindParameterIndex(parameter);
    }

    AZ::Outcome<size_t> AnimGraph::FindParameterIndex(const Parameter* parameter) const
    {
        return m_rootParameter.FindParameterIndex(parameter);
    }

    AZ::Outcome<size_t> AnimGraph::FindRelativeParameterIndex(const Parameter* parameter) const
    {
        return m_rootParameter.FindRelativeParameterIndex(parameter);
    }

    AZ::Outcome<size_t> AnimGraph::FindValueParameterIndex(const ValueParameter* parameter) const
    {
        const size_t parameterCount = m_valueParameters.size();
        for (size_t i = 0; i < parameterCount; ++i)
        {
            if (m_valueParameters[i] == parameter)
            {
                return AZ::Success(i);
            }
        }
        return AZ::Failure();
    }

    // recursively find a given node
    AnimGraphNode* AnimGraph::RecursiveFindNodeByName(const char* nodeName) const
    {
        return m_rootStateMachine->RecursiveFindNodeByName(nodeName);
    }


    bool AnimGraph::IsNodeNameUnique(const AZStd::string& newNameCandidate, const AnimGraphNode* forNode) const
    {
        return m_rootStateMachine->RecursiveIsNodeNameUnique(newNameCandidate, forNode);
    }


    AnimGraphNode* AnimGraph::RecursiveFindNodeById(AnimGraphNodeId nodeId) const
    {
        return m_rootStateMachine->RecursiveFindNodeById(nodeId);
    }


    AnimGraphStateTransition* AnimGraph::RecursiveFindTransitionById(AnimGraphConnectionId transitionId) const
    {
        for (AnimGraphObject* object : m_objects)
        {
            if (azrtti_typeid(object) == azrtti_typeid<AnimGraphStateTransition>())
            {
                AnimGraphStateTransition* transition = static_cast<AnimGraphStateTransition*>(object);
                if (transition->GetId() == transitionId)
                {
                    return transition;
                }
            }
        }

        return nullptr;
    }


    // generate a state name that isn't in use yet
    AZStd::string AnimGraph::GenerateNodeName(const AZStd::unordered_set<AZStd::string>& nameReserveList, const char* prefix) const
    {
        AZStd::string result;
        size_t number = 0;
        bool found = false;
        while (found == false)
        {
            // build the string
            result = AZStd::string::format("%s%zu", prefix, number++);

            // if there is no such state machine yet
            if (!RecursiveFindNodeByName(result.c_str()) && nameReserveList.find(result) == nameReserveList.end())
            {
                found = true;
            }
        }

        return result;
    }


    size_t AnimGraph::RecursiveCalcNumNodes() const
    {
        return m_rootStateMachine->RecursiveCalcNumNodes();
    }


    AnimGraph::Statistics::Statistics()
        : m_maxHierarchyDepth(0)
        , m_numStateMachines(0)
        , m_numStates(1)
        , m_numTransitions(0)
        , m_numWildcardTransitions(0)
        , m_numTransitionConditions(0)
    {
    }


    void AnimGraph::RecursiveCalcStatistics(Statistics& outStatistics) const
    {
        RecursiveCalcStatistics(outStatistics, m_rootStateMachine);
    }


    void AnimGraph::RecursiveCalcStatistics(Statistics& outStatistics, AnimGraphNode* animGraphNode, size_t currentHierarchyDepth) const
    {
        outStatistics.m_maxHierarchyDepth = AZStd::max(currentHierarchyDepth, outStatistics.m_maxHierarchyDepth);

        // Are we dealing with a state machine? If yes, increase the number of transitions, states etc. in the statistics.
        if (azrtti_typeid(animGraphNode) == azrtti_typeid<AnimGraphStateMachine>())
        {
            AnimGraphStateMachine* stateMachine = static_cast<AnimGraphStateMachine*>(animGraphNode);
            outStatistics.m_numStateMachines++;

            const size_t numTransitions = stateMachine->GetNumTransitions();
            outStatistics.m_numTransitions += numTransitions;

            outStatistics.m_numStates += stateMachine->GetNumChildNodes();

            for (size_t i = 0; i < numTransitions; ++i)
            {
                AnimGraphStateTransition* transition = stateMachine->GetTransition(i);

                if (transition->GetIsWildcardTransition())
                {
                    outStatistics.m_numWildcardTransitions++;
                }

                outStatistics.m_numTransitionConditions += transition->GetNumConditions();
            }
        }

        const size_t numChildNodes = animGraphNode->GetNumChildNodes();
        for (size_t i = 0; i < numChildNodes; ++i)
        {
            RecursiveCalcStatistics(outStatistics, animGraphNode->GetChildNode(i), currentHierarchyDepth + 1);
        }
    }


    // recursively calculate the number of node connections
    size_t AnimGraph::RecursiveCalcNumNodeConnections() const
    {
        return m_rootStateMachine->RecursiveCalcNumNodeConnections();
    }


    // adjust the dirty flag
    void AnimGraph::SetDirtyFlag(bool dirty)
    {
        m_dirtyFlag = dirty;
    }


    // adjust the auto unregistering from the anim graph manager on delete
    void AnimGraph::SetAutoUnregister(bool enabled)
    {
        m_autoUnregister = enabled;
    }


    // do we auto unregister from the anim graph manager on delete?
    bool AnimGraph::GetAutoUnregister() const
    {
        return m_autoUnregister;
    }


    void AnimGraph::SetIsOwnedByRuntime(bool isOwnedByRuntime)
    {
#if defined(EMFX_DEVELOPMENT_BUILD)
        m_isOwnedByRuntime = isOwnedByRuntime;
#else
        AZ_UNUSED(isOwnedByRuntime);
#endif
    }


    bool AnimGraph::GetIsOwnedByRuntime() const
    {
#if defined(EMFX_DEVELOPMENT_BUILD)
        return m_isOwnedByRuntime;
#else
        return true;
#endif
    }


    void AnimGraph::SetIsOwnedByAsset(bool isOwnedByAsset)
    {
#if defined(EMFX_DEVELOPMENT_BUILD)
        m_isOwnedByAsset = isOwnedByAsset;
#else
        AZ_UNUSED(isOwnedByAsset);
#endif
    }


    bool AnimGraph::GetIsOwnedByAsset() const
    {
#if defined(EMFX_DEVELOPMENT_BUILD)
        return m_isOwnedByAsset;
#else
        return true;
#endif
    }

    

    // get a pointer to the given node group
    AnimGraphNodeGroup* AnimGraph::GetNodeGroup(size_t index) const
    {
        return m_nodeGroups[index];
    }


    // find the node group by name
    AnimGraphNodeGroup* AnimGraph::FindNodeGroupByName(const char* groupName) const
    {
        for (AnimGraphNodeGroup* nodeGroup : m_nodeGroups)
        {
            // Compare the node names and return a pointer in case they are equal.
            if (nodeGroup->GetNameString() == groupName)
            {
                return nodeGroup;
            }
        }

        return nullptr;
    }


    // find the node group index by name
    size_t AnimGraph::FindNodeGroupIndexByName(const char* groupName) const
    {
        const auto foundNodeGroup = AZStd::find_if(begin(m_nodeGroups), end(m_nodeGroups), [groupName](const AnimGraphNodeGroup* nodeGroup)
        {
            return nodeGroup->GetNameString() == groupName;
        });
        return foundNodeGroup != end(m_nodeGroups) ? AZStd::distance(begin(m_nodeGroups), foundNodeGroup) : InvalidIndex;
    }


    // add the given node group to the anim graph
    void AnimGraph::AddNodeGroup(AnimGraphNodeGroup* nodeGroup)
    {
        m_nodeGroups.push_back(nodeGroup);
    }


    // remove the node group at the given index from the anim graph
    void AnimGraph::RemoveNodeGroup(size_t index, bool delFromMem)
    {
        // destroy the object
        if (delFromMem)
        {
            delete m_nodeGroups[index];
        }

        // remove the node group from the array
        m_nodeGroups.erase(m_nodeGroups.begin() + index);
    }


    // remove all node groups
    void AnimGraph::RemoveAllNodeGroups(bool delFromMem)
    {
        // destroy the node groups
        if (delFromMem)
        {
            for (AnimGraphNodeGroup* nodeGroup : m_nodeGroups)
            {
                delete nodeGroup;
            }
        }

        // remove all node groups
        m_nodeGroups.clear();
    }


    // get the number of node groups
    size_t AnimGraph::GetNumNodeGroups() const
    {
        return m_nodeGroups.size();
    }


    // find the node group in which the given anim graph node is in and return a pointer to it
    AnimGraphNodeGroup* AnimGraph::FindNodeGroupForNode(AnimGraphNode* animGraphNode) const
    {
        for (AnimGraphNodeGroup* nodeGroup : m_nodeGroups)
        {
            // check if the given node is part of the currently iterated node group
            if (nodeGroup->Contains(animGraphNode->GetId()))
            {
                return nodeGroup;
            }
        }

        // failure, the node is not part of a node group
        return nullptr;
    }

    void AnimGraph::FindAndRemoveCycles(AZStd::string* outRemovedConnectionsMessage)
    {
        AZStd::vector<AnimGraphNode*> blendTreeNodes;
        RecursiveCollectNodesOfType(azrtti_typeid<BlendTree>(), &blendTreeNodes);

        const size_t blendTreeNodesCount = blendTreeNodes.size();
        for (size_t i = 0; i < blendTreeNodesCount; ++i)
        {
            BlendTree* blendTree = static_cast<BlendTree*>(blendTreeNodes[i]);
            const AZStd::unordered_set<AZStd::pair<BlendTreeConnection*, AnimGraphNode*> > cycleConnections = blendTree->FindCycles();
            if (!cycleConnections.empty())
            {
                for (const auto& cycleConnection : cycleConnections)
                {
                    if (outRemovedConnectionsMessage)
                    {
                        AnimGraphNode* sourceNode = cycleConnection.first->GetSourceNode();
                        AnimGraphNode* targetNode = cycleConnection.second;
                        *outRemovedConnectionsMessage += sourceNode->GetNameString()
                            + "[" + sourceNode->GetOutputPort(cycleConnection.first->GetSourcePort()).GetNameString() + "]"
                            + "->"
                            + targetNode->GetNameString()
                            + "[" + targetNode->GetInputPort(cycleConnection.first->GetTargetPort()).GetNameString() + "] ";
                    }
                    cycleConnection.second->RemoveConnection(cycleConnection.first);
                }
            }
        }
    }

    void AnimGraph::RecursiveCollectNodesOfType(const AZ::TypeId& nodeType, AZStd::vector<AnimGraphNode*>* outNodes) const
    {
        m_rootStateMachine->RecursiveCollectNodesOfType(nodeType, outNodes);
    }

    void AnimGraph::RecursiveCollectTransitionConditionsOfType(const AZ::TypeId& conditionType, AZStd::vector<AnimGraphTransitionCondition*>* outConditions) const
    {
        m_rootStateMachine->RecursiveCollectTransitionConditionsOfType(conditionType, outConditions);
    }


    void AnimGraph::RecursiveCollectObjectsOfType(const AZ::TypeId& objectType, AZStd::vector<AnimGraphObject*>& outObjects)
    {
        m_rootStateMachine->RecursiveCollectObjectsOfType(objectType, outObjects);
    }


    void AnimGraph::RecursiveCollectObjectsAffectedBy(AnimGraph* animGraph, AZStd::vector<AnimGraphObject*>& outObjects)
    {
        m_rootStateMachine->RecursiveCollectObjectsAffectedBy(animGraph, outObjects);
    }

    GroupParameter* AnimGraph::FindGroupParameterByName(const AZStd::string& groupName) const
    {
        if (groupName.empty())
        {
            return const_cast<GroupParameter*>(&m_rootParameter);
        }
        else
        {
            Parameter* parameter = const_cast<Parameter*>(FindParameterByName(groupName));
            if (azrtti_typeid(parameter) == azrtti_typeid<GroupParameter>())
            {
                return static_cast<GroupParameter*>(parameter);
            }
        }
        return nullptr;
    }

    const GroupParameter* AnimGraph::FindParentGroupParameter(const Parameter* parameter) const
    {
        const GroupParameter* groupParameter = m_rootParameter.FindParentGroupParameter(parameter);
        if (groupParameter == &m_rootParameter)
        {
            return nullptr;
        }
        return groupParameter;
    }


    bool AnimGraph::TakeParameterFromParent(const Parameter* parameter)
    {
        // if it is a value parameter, remove it from the list of values
        if (azrtti_typeid(parameter) != azrtti_typeid<GroupParameter>())
        {
            const auto it = AZStd::find(m_valueParameters.begin(), m_valueParameters.end(), parameter);
            AZ_Assert(it != m_valueParameters.end(), "Expected to have the parameter in this anim graph");
            const size_t valueParameterIndex = AZStd::distance(m_valueParameters.begin(), it);
            m_valueParameters.erase(it);
            RemoveValueParameterToIndexByNameCache(valueParameterIndex, parameter->GetName());
        }

        return m_rootParameter.TakeParameterFromParent(parameter);
    }

    // delete all unique datas for a given object
    void AnimGraph::RemoveAllObjectData(AnimGraphObject* object, bool delFromMem)
    {
        MCore::LockGuard lock(m_lock);

        for (AnimGraphInstance* animGraphInstance : m_animGraphInstances)
        {
            animGraphInstance->RemoveUniqueObjectData(object->GetObjectIndex(), delFromMem);  // remove all unique datas that belong to the given object
        }
    }


    // set the root state machine
    void AnimGraph::SetRootStateMachine(AnimGraphStateMachine* stateMachine)
    {
        m_rootStateMachine = stateMachine;
        if (m_rootStateMachine)
        {
            // make sure the name is always the same for the root state machine
            m_rootStateMachine->SetName("Root");
        }
    }


    // add an object
    void AnimGraph::AddObject(AnimGraphObject* object)
    {
        MCore::LockGuard lock(m_lock);

        // assign the index and add it to the objects array
        object->SetObjectIndex(m_objects.size());
        m_objects.push_back(object);

        // if it's a node, add it to the nodes array as well
        if (azrtti_istypeof<AnimGraphNode>(object))
        {
            AnimGraphNode* node = static_cast<AnimGraphNode*>(object);
            node->SetNodeIndex(m_nodes.size());
            m_nodes.emplace_back(node);
        }

        // create a unique data for this added object in the animgraph instances as well
        for (AnimGraphInstance* animGraphInstance : m_animGraphInstances)
        {
            animGraphInstance->AddUniqueObjectData();
        }
    }


    // remove an object
    void AnimGraph::RemoveObject(AnimGraphObject* object)
    {
        MCore::LockGuard lock(m_lock);

        const size_t objectIndex = object->GetObjectIndex();

        // remove all internal attributes for this object
        object->RemoveInternalAttributesForAllInstances();

        // decrease the indices of all objects that have an index after this node
        const size_t numObjects = m_objects.size();
        for (size_t i = objectIndex + 1; i < numObjects; ++i)
        {
            AnimGraphObject* curObject = m_objects[i];
            MCORE_ASSERT(i == curObject->GetObjectIndex());
            curObject->SetObjectIndex(i - 1);
        }

        // remove the object from the array
        m_objects.erase(m_objects.begin() + objectIndex);

        // remove it from the nodes array if it is a node
        if (azrtti_istypeof<AnimGraphNode>(object))
        {
            AnimGraphNode* node = static_cast<AnimGraphNode*>(object);
            const size_t nodeIndex = node->GetNodeIndex();

            const size_t numNodes = m_nodes.size();
            for (size_t i = nodeIndex + 1; i < numNodes; ++i)
            {
                AnimGraphNode* curNode = m_nodes[i];
                MCORE_ASSERT(i == curNode->GetNodeIndex());
                curNode->SetNodeIndex(i - 1);
            }

            // remove the object from the array
            m_nodes.erase(AZStd::next(begin(m_nodes), nodeIndex));
        }
    }


    // reserve space for a given amount of objects
    void AnimGraph::ReserveNumObjects(size_t numObjects)
    {
        m_objects.reserve(numObjects);
    }


    // reserve space for a given amount of nodes
    void AnimGraph::ReserveNumNodes(size_t numNodes)
    {
        m_nodes.reserve(numNodes);
    }


    // Calculate number of motion nodes in the graph
    size_t AnimGraph::CalcNumMotionNodes() const
    {
        return AZStd::accumulate(begin(m_nodes), end(m_nodes), size_t{0}, [](size_t total, const AnimGraphNode* node)
        {
            return total + azrtti_istypeof<AnimGraphMotionNode>(node);
        });
    }


    // reserve memory for the animgraph instance array
    void AnimGraph::ReserveNumAnimGraphInstances(size_t numInstances)
    {
        m_animGraphInstances.reserve(numInstances);
    }


    // register an animgraph instance
    void AnimGraph::AddAnimGraphInstance(AnimGraphInstance* animGraphInstance)
    {
        MCore::LockGuard lock(m_lock);
        m_animGraphInstances.emplace_back(animGraphInstance);
    }


    // remove an animgraph instance
    void AnimGraph::RemoveAnimGraphInstance(AnimGraphInstance* animGraphInstance)
    {
        MCore::LockGuard lock(m_lock);
        m_animGraphInstances.erase(AZStd::remove(m_animGraphInstances.begin(), m_animGraphInstances.end(), animGraphInstance));
    }


    // decrease internal attribute indices by one, for values higher than the given parameter
    void AnimGraph::DecreaseInternalAttributeIndices(size_t decreaseEverythingHigherThan)
    {
        for (AnimGraphObject* object : m_objects)
        {
            object->DecreaseInternalAttributeIndices(decreaseEverythingHigherThan);
        }
    }


    const char* AnimGraph::GetFileName() const
    {
        return m_fileName.c_str();
    }


    const AZStd::string& AnimGraph::GetFileNameString() const
    {
        return m_fileName;
    }


    void AnimGraph::SetFileName(const char* fileName)
    {
        m_fileName = fileName;
    }


    AnimGraphStateMachine* AnimGraph::GetRootStateMachine() const
    {
        return m_rootStateMachine;
    }


    uint32 AnimGraph::GetID() const
    {
        return m_id;
    }


    void AnimGraph::SetID(uint32 id)
    {
        m_id = id;
    }


    bool AnimGraph::GetDirtyFlag() const
    {
        return m_dirtyFlag;
    }


    bool AnimGraph::GetRetargetingEnabled() const
    {
        return m_retarget;
    }


    void AnimGraph::SetRetargetingEnabled(bool enabled)
    {
        m_retarget = enabled;
    }

    void AnimGraph::Lock()
    {
        m_lock.Lock();
    }


    void AnimGraph::Unlock()
    {
        m_lock.Unlock();
    }


    void AnimGraph::OnRetargetingEnabledChanged()
    {
        for (AnimGraphInstance* animGraphInstance : m_animGraphInstances)
        {
            animGraphInstance->SetRetargetingEnabled(m_retarget);
        }
    }


    void AnimGraph::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<AnimGraph>()
            ->Version(2)
            ->Field("rootGroupParameter", &AnimGraph::m_rootParameter)
            ->Field("rootStateMachine", &AnimGraph::m_rootStateMachine)
            ->Field("nodeGroups", &AnimGraph::m_nodeGroups)
            ->Field("retarget", &AnimGraph::m_retarget)
        ;

        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (!editContext)
        {
            return;
        }

        editContext->Class<AnimGraph>("Anim Graph", "Anim graph attributes")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
            ->Attribute(AZ::Edit::Attributes::AutoExpand, "")
            ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
            ->DataElement(AZ::Edit::UIHandlers::Default, &AnimGraph::m_retarget, "Retarget", "")
            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &AnimGraph::OnRetargetingEnabledChanged)
        ;
    }


    AnimGraph* AnimGraph::LoadFromFile(const AZStd::string& filename, AZ::SerializeContext* context, const AZ::ObjectStream::FilterDescriptor& loadFilter)
    {
        AZ::Debug::Timer loadTimer;
        loadTimer.Stamp();

        AnimGraph* animGraph =  AZ::Utils::LoadObjectFromFile<AnimGraph>(filename, context, loadFilter);
        if (animGraph)
        {
            if (!animGraph->GetRootStateMachine())
            {
                AZ_Error("EMotionFX", false, "Loaded anim graph does not have a root state machine");
                delete animGraph;
                return nullptr;
            }
            const float deserializeTimeInMs = loadTimer.StampAndGetDeltaTimeInSeconds() * 1000.0f;

            animGraph->InitAfterLoading();
            animGraph->RemoveInvalidConnections(true);  // Remove connections that have nullptr source node's, which happens when connections point to unknown nodes.
            const float initTimeInMs = loadTimer.GetDeltaTimeInSeconds() * 1000.0f;

            AZ_Printf("EMotionFX", "Loaded anim graph from %s in %.1f ms (Deserialization %.1f ms, Initialization %.1f ms).", filename.c_str(), deserializeTimeInMs + initTimeInMs, deserializeTimeInMs, initTimeInMs);
        }

        return animGraph;
    }


    AnimGraph* AnimGraph::LoadFromBuffer(const void* buffer, const AZStd::size_t length, AZ::SerializeContext* context)
    {
        AZ::Debug::Timer loadTimer;
        loadTimer.Stamp();

        AZ::ObjectStream::FilterDescriptor loadFilter(nullptr, AZ::ObjectStream::FILTERFLAG_IGNORE_UNKNOWN_CLASSES);
        AnimGraph* animGraph = AZ::Utils::LoadObjectFromBuffer<AnimGraph>(buffer, length, context, loadFilter);
        if (animGraph)
        {
            if (!animGraph->GetRootStateMachine())
            {
                AZ_Error("EMotionFX", false, "Loaded anim graph does not have a root state machine");
                delete animGraph;
                return nullptr;
            }
            const float deserializeTimeInMs = loadTimer.StampAndGetDeltaTimeInSeconds() * 1000.0f;
            animGraph->InitAfterLoading();
            animGraph->RemoveInvalidConnections(true);  // Remove connections that have nullptr source node's, which happens when connections point to unknown nodes.
            const float initTimeInMs = loadTimer.GetDeltaTimeInSeconds() * 1000.0f;

            AZ_Printf("EMotionFX", "Loaded anim graph from buffer in %.1f ms (Deserialization %.1f ms, Initialization %.1f ms).", deserializeTimeInMs + initTimeInMs, deserializeTimeInMs, initTimeInMs);
        }

        return animGraph;
    }


    bool AnimGraph::SaveToFile(const AZStd::string& filename, AZ::SerializeContext* context) const
    {
        AZ::Debug::Timer saveTimer;
        saveTimer.Stamp();

        const bool result = AZ::Utils::SaveObjectToFile<AnimGraph>(filename, AZ::ObjectStream::ST_XML, this, context);
        if (result)
        {
            const float saveTimeInMs = saveTimer.GetDeltaTimeInSeconds() * 1000.0f;
            AZ_Printf("EMotionFX", "Saved anim graph to %s in %.1f ms.", filename.c_str(), saveTimeInMs);
        }

        return result;
    }


    void AnimGraph::RemoveInvalidConnections(bool logWarnings)
    {
        // Iterate over all nodes
        for (AnimGraphNode* node : m_nodes)
        {
            for (size_t c = 0; c < node->GetNumConnections();)
            {
                BlendTreeConnection* connection = node->GetConnection(c);
                if (!connection->GetSourceNode())   // Invalid source node.
                {
                    if (logWarnings)
                    {
                        AZ_Warning("EMotionFX", false, "Removing a connection plugged into input port index %d of node '%s' (parent='%s'), because the source node most likely has been removed as it was an unknown (probably custom) node.", 
                            connection->GetTargetPort(), node->GetName(), node->GetParentNode() ? node->GetParentNode()->GetName() : "<Root>");
                    }
                    node->RemoveConnection(connection);
                }
                else
                {
                    c++;
                }
            }
        }
    }


    void AnimGraph::AddValueParameterToIndexByNameCache(const size_t index, const AZStd::string& parameterName)
    {
        for (auto& nameAndIndex : m_valueParameterIndexByName)
        {
            if (nameAndIndex.second >= index)
            {
                ++nameAndIndex.second;
            }
        }
        m_valueParameterIndexByName.emplace(parameterName, index);
    }

    void AnimGraph::RemoveValueParameterToIndexByNameCache(const size_t index, const AZStd::string& parameterName)
    {
        m_valueParameterIndexByName.erase(parameterName);
        for (auto& nameAndIndex : m_valueParameterIndexByName)
        {
            if (nameAndIndex.second > index)
            {
                --nameAndIndex.second;
            }
        }
    }

} // namespace EMotionFX
