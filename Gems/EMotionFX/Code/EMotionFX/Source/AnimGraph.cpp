/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Debug/Timer.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/Utils.h>
#include <EMotionFX/Source/ActorInstance.h>
#include <EMotionFX/Source/Allocators.h>
#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/Source/AnimGraphGameControllerSettings.h>
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
    AZ_CLASS_ALLOCATOR_IMPL(AnimGraph, AnimGraphAllocator, 0)

    AnimGraph::AnimGraph()
        : mGameControllerSettings(aznew AnimGraphGameControllerSettings())
    {
        mNodes.SetMemoryCategory(EMFX_MEMCATEGORY_ANIMGRAPH);

        mID = MCore::GetIDGenerator().GenerateID();
        mDirtyFlag = false;
        mAutoUnregister = true;
        mRetarget = false;
        mRootStateMachine = nullptr;

#if defined(EMFX_DEVELOPMENT_BUILD)
        mIsOwnedByRuntime = false;
        m_isOwnedByAsset = false;
#endif // EMFX_DEVELOPMENT_BUILD

        // reserve some memory
        mNodes.Reserve(1024);

        // automatically register the anim graph
        GetAnimGraphManager().AddAnimGraph(this);

        GetEventManager().OnCreateAnimGraph(this);
    }


    AnimGraph::~AnimGraph()
    {
        GetEventManager().OnDeleteAnimGraph(this);
        GetRecorder().RemoveAnimGraphFromRecording(this);

        RemoveAllNodeGroups();

        if (mRootStateMachine)
        {
            delete mRootStateMachine;
        }

        // automatically unregister the anim graph
        if (mAutoUnregister)
        {
            GetAnimGraphManager().RemoveAnimGraph(this, false);
        }

        delete mGameControllerSettings;
    }


    void AnimGraph::RecursiveReinit()
    {
        if (!mRootStateMachine)
        {
            return;
        }

        mRootStateMachine->RecursiveReinit();
    }


    bool AnimGraph::InitAfterLoading()
    {
        if (!mRootStateMachine)
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

        return mRootStateMachine->InitAfterLoading(this);
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
        return mRootStateMachine->RecursiveFindNodeByName(nodeName);
    }


    bool AnimGraph::IsNodeNameUnique(const AZStd::string& newNameCandidate, const AnimGraphNode* forNode) const
    {
        return mRootStateMachine->RecursiveIsNodeNameUnique(newNameCandidate, forNode);
    }


    AnimGraphNode* AnimGraph::RecursiveFindNodeById(AnimGraphNodeId nodeId) const
    {
        return mRootStateMachine->RecursiveFindNodeById(nodeId);
    }


    AnimGraphStateTransition* AnimGraph::RecursiveFindTransitionById(AnimGraphConnectionId transitionId) const
    {
        for (AnimGraphObject* object : mObjects)
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
        uint32 number = 0;
        bool found = false;
        while (found == false)
        {
            // build the string
            result = AZStd::string::format("%s%d", prefix, number++);

            // if there is no such state machine yet
            if (!RecursiveFindNodeByName(result.c_str()) && nameReserveList.find(result) == nameReserveList.end())
            {
                found = true;
            }
        }

        return result;
    }


    uint32 AnimGraph::RecursiveCalcNumNodes() const
    {
        return mRootStateMachine->RecursiveCalcNumNodes();
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
        RecursiveCalcStatistics(outStatistics, mRootStateMachine);
    }


    void AnimGraph::RecursiveCalcStatistics(Statistics& outStatistics, AnimGraphNode* animGraphNode, uint32 currentHierarchyDepth) const
    {
        outStatistics.m_maxHierarchyDepth = MCore::Max<uint32>(currentHierarchyDepth, outStatistics.m_maxHierarchyDepth);

        // Are we dealing with a state machine? If yes, increase the number of transitions, states etc. in the statistics.
        if (azrtti_typeid(animGraphNode) == azrtti_typeid<AnimGraphStateMachine>())
        {
            AnimGraphStateMachine* stateMachine = static_cast<AnimGraphStateMachine*>(animGraphNode);
            outStatistics.m_numStateMachines++;

            const AZ::u32 numTransitions = static_cast<AZ::u32>(stateMachine->GetNumTransitions());
            outStatistics.m_numTransitions += numTransitions;

            outStatistics.m_numStates += stateMachine->GetNumChildNodes();

            for (uint32 i = 0; i < numTransitions; ++i)
            {
                AnimGraphStateTransition* transition = stateMachine->GetTransition(i);

                if (transition->GetIsWildcardTransition())
                {
                    outStatistics.m_numWildcardTransitions++;
                }

                outStatistics.m_numTransitionConditions += static_cast<uint32>(transition->GetNumConditions());
            }
        }

        const uint32 numChildNodes = animGraphNode->GetNumChildNodes();
        for (uint32 i = 0; i < numChildNodes; ++i)
        {
            RecursiveCalcStatistics(outStatistics, animGraphNode->GetChildNode(i), currentHierarchyDepth + 1);
        }
    }


    // recursively calculate the number of node connections
    uint32 AnimGraph::RecursiveCalcNumNodeConnections() const
    {
        return mRootStateMachine->RecursiveCalcNumNodeConnections();
    }


    // adjust the dirty flag
    void AnimGraph::SetDirtyFlag(bool dirty)
    {
        mDirtyFlag = dirty;
    }


    // adjust the auto unregistering from the anim graph manager on delete
    void AnimGraph::SetAutoUnregister(bool enabled)
    {
        mAutoUnregister = enabled;
    }


    // do we auto unregister from the anim graph manager on delete?
    bool AnimGraph::GetAutoUnregister() const
    {
        return mAutoUnregister;
    }


    void AnimGraph::SetIsOwnedByRuntime(bool isOwnedByRuntime)
    {
#if defined(EMFX_DEVELOPMENT_BUILD)
        mIsOwnedByRuntime = isOwnedByRuntime;
#else
        AZ_UNUSED(isOwnedByRuntime);
#endif
    }


    bool AnimGraph::GetIsOwnedByRuntime() const
    {
#if defined(EMFX_DEVELOPMENT_BUILD)
        return mIsOwnedByRuntime;
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
    AnimGraphNodeGroup* AnimGraph::GetNodeGroup(uint32 index) const
    {
        return mNodeGroups[index];
    }


    // find the node group by name
    AnimGraphNodeGroup* AnimGraph::FindNodeGroupByName(const char* groupName) const
    {
        for (AnimGraphNodeGroup* nodeGroup : mNodeGroups)
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
    uint32 AnimGraph::FindNodeGroupIndexByName(const char* groupName) const
    {
        const size_t numNodeGroups = mNodeGroups.size();
        for (size_t i = 0; i < numNodeGroups; ++i)
        {
            // compare the node names and return the index in case they are equal
            if (mNodeGroups[i]->GetNameString() == groupName)
            {
                return static_cast<uint32>(i);
            }
        }

        return MCORE_INVALIDINDEX32;
    }


    // add the given node group to the anim graph
    void AnimGraph::AddNodeGroup(AnimGraphNodeGroup* nodeGroup)
    {
        mNodeGroups.push_back(nodeGroup);
    }


    // remove the node group at the given index from the anim graph
    void AnimGraph::RemoveNodeGroup(uint32 index, bool delFromMem)
    {
        // destroy the object
        if (delFromMem)
        {
            delete mNodeGroups[index];
        }

        // remove the node group from the array
        mNodeGroups.erase(mNodeGroups.begin() + index);
    }


    // remove all node groups
    void AnimGraph::RemoveAllNodeGroups(bool delFromMem)
    {
        // destroy the node groups
        if (delFromMem)
        {
            for (AnimGraphNodeGroup* nodeGroup : mNodeGroups)
            {
                delete nodeGroup;
            }
        }

        // remove all node groups
        mNodeGroups.clear();
    }


    // get the number of node groups
    uint32 AnimGraph::GetNumNodeGroups() const
    {
        return static_cast<uint32>(mNodeGroups.size());
    }


    // find the node group in which the given anim graph node is in and return a pointer to it
    AnimGraphNodeGroup* AnimGraph::FindNodeGroupForNode(AnimGraphNode* animGraphNode) const
    {
        for (AnimGraphNodeGroup* nodeGroup : mNodeGroups)
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
        mRootStateMachine->RecursiveCollectNodesOfType(nodeType, outNodes);
    }

    void AnimGraph::RecursiveCollectTransitionConditionsOfType(const AZ::TypeId& conditionType, MCore::Array<AnimGraphTransitionCondition*>* outConditions) const
    {
        mRootStateMachine->RecursiveCollectTransitionConditionsOfType(conditionType, outConditions);
    }


    void AnimGraph::RecursiveCollectObjectsOfType(const AZ::TypeId& objectType, AZStd::vector<AnimGraphObject*>& outObjects)
    {
        mRootStateMachine->RecursiveCollectObjectsOfType(objectType, outObjects);
    }


    void AnimGraph::RecursiveCollectObjectsAffectedBy(AnimGraph* animGraph, AZStd::vector<AnimGraphObject*>& outObjects)
    {
        mRootStateMachine->RecursiveCollectObjectsAffectedBy(animGraph, outObjects);
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
        MCore::LockGuard lock(mLock);

        for (AnimGraphInstance* animGraphInstance : m_animGraphInstances)
        {
            animGraphInstance->RemoveUniqueObjectData(object->GetObjectIndex(), delFromMem);  // remove all unique datas that belong to the given object
        }
    }


    // set the root state machine
    void AnimGraph::SetRootStateMachine(AnimGraphStateMachine* stateMachine)
    {
        mRootStateMachine = stateMachine;
        if (mRootStateMachine)
        {
            // make sure the name is always the same for the root state machine
            mRootStateMachine->SetName("Root");
        }
    }


    // add an object
    void AnimGraph::AddObject(AnimGraphObject* object)
    {
        MCore::LockGuard lock(mLock);

        // assign the index and add it to the objects array
        object->SetObjectIndex(static_cast<uint32>(mObjects.size()));
        mObjects.push_back(object);

        // if it's a node, add it to the nodes array as well
        if (azrtti_istypeof<AnimGraphNode>(object))
        {
            AnimGraphNode* node = static_cast<AnimGraphNode*>(object);
            node->SetNodeIndex(mNodes.GetLength());
            mNodes.Add(node);
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
        MCore::LockGuard lock(mLock);

        const size_t objectIndex = object->GetObjectIndex();

        // remove all internal attributes for this object
        object->RemoveInternalAttributesForAllInstances();

        // decrease the indices of all objects that have an index after this node
        const size_t numObjects = mObjects.size();
        for (size_t i = objectIndex + 1; i < numObjects; ++i)
        {
            AnimGraphObject* curObject = mObjects[i];
            MCORE_ASSERT(i == curObject->GetObjectIndex());
            curObject->SetObjectIndex(i - 1);
        }

        // remove the object from the array
        mObjects.erase(mObjects.begin() + objectIndex);

        // remove it from the nodes array if it is a node
        if (azrtti_istypeof<AnimGraphNode>(object))
        {
            AnimGraphNode* node = static_cast<AnimGraphNode*>(object);
            const uint32 nodeIndex = node->GetNodeIndex();

            const uint32 numNodes = mNodes.GetLength();
            for (uint32 i = nodeIndex + 1; i < numNodes; ++i)
            {
                AnimGraphNode* curNode = mNodes[i];
                MCORE_ASSERT(i == curNode->GetNodeIndex());
                curNode->SetNodeIndex(i - 1);
            }

            // remove the object from the array
            mNodes.Remove(nodeIndex);
        }
    }


    // reserve space for a given amount of objects
    void AnimGraph::ReserveNumObjects(uint32 numObjects)
    {
        mObjects.reserve(numObjects);
    }


    // reserve space for a given amount of nodes
    void AnimGraph::ReserveNumNodes(uint32 numNodes)
    {
        mNodes.Reserve(numNodes);
    }


    // Calculate number of motion nodes in the graph
    uint32 AnimGraph::CalcNumMotionNodes() const
    {
        const uint32 numNodes = mNodes.GetLength();
        uint32 numMotionNodes = 0;
        for (uint32 i = 0; i < numNodes; ++i)
        {
            if (azrtti_istypeof<AnimGraphMotionNode>(mNodes[i]))
            {
                numMotionNodes++;
            }
        }
        return numMotionNodes;
    }


    // reserve memory for the animgraph instance array
    void AnimGraph::ReserveNumAnimGraphInstances(size_t numInstances)
    {
        m_animGraphInstances.reserve(numInstances);
    }


    // register an animgraph instance
    void AnimGraph::AddAnimGraphInstance(AnimGraphInstance* animGraphInstance)
    {
        MCore::LockGuard lock(mLock);
        m_animGraphInstances.emplace_back(animGraphInstance);
    }


    // remove an animgraph instance
    void AnimGraph::RemoveAnimGraphInstance(AnimGraphInstance* animGraphInstance)
    {
        MCore::LockGuard lock(mLock);
        m_animGraphInstances.erase(AZStd::remove(m_animGraphInstances.begin(), m_animGraphInstances.end(), animGraphInstance));
    }


    // decrease internal attribute indices by one, for values higher than the given parameter
    void AnimGraph::DecreaseInternalAttributeIndices(uint32 decreaseEverythingHigherThan)
    {
        for (AnimGraphObject* object : mObjects)
        {
            object->DecreaseInternalAttributeIndices(decreaseEverythingHigherThan);
        }
    }


    const char* AnimGraph::GetFileName() const
    {
        return mFileName.c_str();
    }


    const AZStd::string& AnimGraph::GetFileNameString() const
    {
        return mFileName;
    }


    void AnimGraph::SetFileName(const char* fileName)
    {
        mFileName = fileName;
    }


    AnimGraphStateMachine* AnimGraph::GetRootStateMachine() const
    {
        return mRootStateMachine;
    }


    uint32 AnimGraph::GetID() const
    {
        return mID;
    }


    void AnimGraph::SetID(uint32 id)
    {
        mID = id;
    }


    bool AnimGraph::GetDirtyFlag() const
    {
        return mDirtyFlag;
    }


    AnimGraphGameControllerSettings& AnimGraph::GetGameControllerSettings()
    {
        return *mGameControllerSettings;
    }


    bool AnimGraph::GetRetargetingEnabled() const
    {
        return mRetarget;
    }


    void AnimGraph::SetRetargetingEnabled(bool enabled)
    {
        mRetarget = enabled;
    }

    void AnimGraph::Lock()
    {
        mLock.Lock();
    }


    void AnimGraph::Unlock()
    {
        mLock.Unlock();
    }


    void AnimGraph::OnRetargetingEnabledChanged()
    {
        for (AnimGraphInstance* animGraphInstance : m_animGraphInstances)
        {
            animGraphInstance->SetRetargetingEnabled(mRetarget);
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
            ->Version(1)
            ->Field("rootGroupParameter", &AnimGraph::m_rootParameter)
            ->Field("rootStateMachine", &AnimGraph::mRootStateMachine)
            ->Field("nodeGroups", &AnimGraph::mNodeGroups)
            ->Field("gameControllerSettings", &AnimGraph::mGameControllerSettings)
            ->Field("retarget", &AnimGraph::mRetarget)
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
            ->DataElement(AZ::Edit::UIHandlers::Default, &AnimGraph::mRetarget, "Retarget", "")
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
        const AZ::u32 numNodes = mNodes.GetLength();
        for (AZ::u32 i = 0; i < numNodes; ++i)
        {
            AnimGraphNode* node = mNodes[i];
            for (AZ::u32 c = 0; c < node->GetNumConnections();)
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
