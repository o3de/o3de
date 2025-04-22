/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// include required headers
#include <MCore/Source/RefCounted.h>
#include <EMotionFX/Source/ActorInstance.h>
#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/Source/AnimGraphInstance.h>
#include <EMotionFX/Source/AnimGraphManager.h>
#include <EMotionFX/Source/AnimGraphPosePool.h>
#include <EMotionFX/Source/AnimGraphStateMachine.h>
#include <EMotionFX/Source/Attachment.h>
#include <EMotionFX/Source/BlendTreeMotionFrameNode.h>
#include <EMotionFX/Source/EMotionFXConfig.h>
#include <EMotionFX/Source/EMotionFXManager.h>
#include <EMotionFX/Source/EventHandler.h>
#include <EMotionFX/Source/EventManager.h>
#include <EMotionFX/Source/MotionSet.h>
#include <EMotionFX/Source/Parameter/ValueParameter.h>
#include <EMotionFX/Source/ThreadData.h>
#include <EMotionFX/Source/TransformData.h>
#include <AzCore/std/smart_ptr/make_shared.h>


namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(AnimGraphInstance, AnimGraphInstanceAllocator)

    AnimGraphInstance::AnimGraphInstance(AnimGraph* animGraph, ActorInstance* actorInstance, MotionSet* motionSet, const InitSettings* initSettings)
        : MCore::RefCounted()
    {
        // register at the animgraph
        animGraph->AddAnimGraphInstance(this);
        animGraph->Lock();

        m_animGraph              = animGraph;
        m_actorInstance          = actorInstance;
        m_parentAnimGraphInstance = nullptr;
        m_motionSet              = motionSet;
        m_autoUnregister         = true;
        m_enableVisualization    = true;
        m_retarget               = animGraph->GetRetargetingEnabled();
        m_visualizeScale         = 1.0f;
        m_autoReleaseAllPoses   = true;
        m_autoReleaseAllRefDatas= true;

#if defined(EMFX_DEVELOPMENT_BUILD)
        m_isOwnedByRuntime       = false;
#endif // EMFX_DEVELOPMENT_BUILD

        if (initSettings)
        {
            m_initSettings = *initSettings;
        }

        m_eventHandlersByEventType.resize(EVENT_TYPE_ANIM_GRAPH_INSTANCE_LAST_EVENT - EVENT_TYPE_ANIM_GRAPH_INSTANCE_FIRST_EVENT + 1);

        // init the internal attributes (create them)
        InitInternalAttributes();

        // prealloc the unique data array (doesn't create the actual unique data objects yet though)
        InitUniqueDatas();

        // automatically register the anim graph instance
        GetAnimGraphManager().AddAnimGraphInstance(this);

        // create the parameter value objects
        CreateParameterValues();

        // Assign a unique seed for the lcg random number. Here we use the actorInstanceId because it guaranteed to be unique and available on actor instances.
        m_lcgRandom.SetSeed(actorInstance->GetID());

        m_animGraph->Unlock();
        GetEventManager().OnCreateAnimGraphInstance(this);
    }

    AnimGraphInstance::~AnimGraphInstance()
    {
        for (AnimGraphInstance* childInstance : m_childAnimGraphInstances)
        {
            childInstance->m_parentAnimGraphInstance = nullptr;
        }
        m_childAnimGraphInstances.clear();
        SetParentAnimGraphInstance(nullptr);

        if (m_parentAnimGraphInstance)
        {
            m_parentAnimGraphInstance->RemoveChildAnimGraphInstance(this);
        }

        GetEventManager().OnDeleteAnimGraphInstance(this);

        // automatically unregister the anim graph instance
        if (m_autoUnregister)
        {
            GetAnimGraphManager().RemoveAnimGraphInstance(this, false);
        }

        // Get rid of the unique data for all anim graph objects.
        for (AnimGraphObjectData* uniqueData : m_uniqueDatas)
        {
            if (uniqueData)
            {
                uniqueData->Destroy();
            }
        }
        m_uniqueDatas.clear();

        RemoveAllParameters(true);
        RemoveAllEventHandlers();

        // remove all the internal attributes (from node ports etc)
        RemoveAllInternalAttributes();

        // Remove all leader graph
        for (AnimGraphInstance* follower : m_followerGraphs)
        {
            follower->RemoveLeaderGraph(this);
        }
        m_followerGraphs.clear();

        for (AnimGraphInstance* leader : m_leaderGraphs)
        {
            leader->RemoveFollowerGraph(this, false);
        }
        m_leaderGraphs.clear();

        // unregister from the animgraph
        m_animGraph->RemoveAnimGraphInstance(this);
    }


    // create an animgraph instance
    AnimGraphInstance* AnimGraphInstance::Create(AnimGraph* animGraph, ActorInstance* actorInstance, MotionSet* motionSet, const InitSettings* initSettings)
    {
        return aznew AnimGraphInstance(animGraph, actorInstance, motionSet, initSettings);
    }


    // remove all parameter values
    void AnimGraphInstance::RemoveAllParameters(bool delFromMem)
    {
        if (delFromMem)
        {
            for (MCore::Attribute* paramValue : m_paramValues)
            {
                if (paramValue)
                {
                    delete paramValue;
                }
            }
        }

        m_paramValues.clear();
    }


    // remove all internal attributes
    void AnimGraphInstance::RemoveAllInternalAttributes()
    {
        MCore::LockGuard lock(m_mutex);

        for (MCore::Attribute* internalAttribute : m_internalAttributes)
        {
            if (internalAttribute)
            {
                delete internalAttribute;
            }
        }
    }


    size_t AnimGraphInstance::AddInternalAttribute(MCore::Attribute* attribute)
    {
        MCore::LockGuard lock(m_mutex);

        m_internalAttributes.emplace_back(attribute);
        return m_internalAttributes.size() - 1;
    }


    size_t AnimGraphInstance::GetNumInternalAttributes() const
    {
        return m_internalAttributes.size();
    }


    MCore::Attribute* AnimGraphInstance::GetInternalAttribute(size_t attribIndex) const
    {
        return m_internalAttributes[attribIndex];
    }


    void AnimGraphInstance::ReserveInternalAttributes(size_t totalNumInternalAttributes)
    {
        MCore::LockGuard lock(m_mutex);
        m_internalAttributes.reserve(totalNumInternalAttributes);
    }


    void AnimGraphInstance::RemoveInternalAttribute(size_t index, bool delFromMem)
    {
        MCore::LockGuard lock(m_mutex);
        if (delFromMem)
        {
            MCore::Attribute* internalAttribute = m_internalAttributes[index];
            if (internalAttribute)
            {
                delete internalAttribute;
            }
        }

        m_internalAttributes.erase(m_internalAttributes.begin() + index);
    }


    // output the results into the internal pose object
    void AnimGraphInstance::Output(Pose* outputPose)
    {
        AZ_PROFILE_SCOPE(Animation, "AnimGraphInstance::Output");

        // reset max used
        const uint32 threadIndex = m_actorInstance->GetThreadIndex();
        AnimGraphPosePool& posePool = GetEMotionFX().GetThreadData(threadIndex)->GetPosePool();
        posePool.ResetMaxUsedPoses();

        // calculate the anim graph output
        AnimGraphNode* rootNode = GetRootNode();

        // calculate the output of the state machine
        rootNode->PerformOutput(this);

        // update the output pose
        if (outputPose)
        {
            *outputPose = rootNode->GetMainOutputPose(this)->GetPose();
        }

        // decrease pose ref count for the root
        rootNode->DecreaseRef(this);

        //MCore::LogInfo("------poses   used = %d/%d (max=%d)----------", GetEMotionFX().GetThreadData(0)->GetPosePool().GetNumUsedPoses(), GetEMotionFX().GetThreadData(0)->GetPosePool().GetNumPoses(), GetEMotionFX().GetThreadData(0)->GetPosePool().GetNumMaxUsedPoses());
        //MCore::LogInfo("------refData used = %d/%d (max=%d)----------", GetEMotionFX().GetThreadData(0)->GetRefCountedDataPool().GetNumUsedItems(), GetEMotionFX().GetThreadData(0)->GetRefCountedDataPool().GetNumItems(), GetEMotionFX().GetThreadData(0)->GetRefCountedDataPool().GetNumMaxUsedItems());
        //MCORE_ASSERT(GetEMotionFX().GetThreadData(0).GetPosePool().GetNumUsedPoses() == 0);

        // Release only for root anim graphs and when we want to auto release.
        if (m_autoReleaseAllPoses && !m_parentAnimGraphInstance)
        {
            ReleasePoses();
            posePool.FreeAllPoses();
        }

        // Gather active state. Must be done in output function.
        if (m_snapshot && m_snapshot->IsNetworkAuthoritative())
        {
            m_snapshot->CollectActiveNodes(*this);
            m_snapshot->CollectMotionNodePlaytimes(*this);
        }
    }



    // resize the number of parameters
    void AnimGraphInstance::CreateParameterValues()
    {
        RemoveAllParameters(true);

        const ValueParameterVector& valueParameters = m_animGraph->RecursivelyGetValueParameters();
        m_paramValues.resize(valueParameters.size());

        // init the values
        const size_t numParams = m_paramValues.size();
        for (size_t i = 0; i < numParams; ++i)
        {
            m_paramValues[i] = valueParameters[i]->ConstructDefaultValueAsAttribute();
        }
    }


    // add the missing parameters that the anim graph has to this anim graph instance
    void AnimGraphInstance::AddMissingParameterValues()
    {
        // check how many parameters we need to add
        const ValueParameterVector& valueParameters = m_animGraph->RecursivelyGetValueParameters();
        const ptrdiff_t numToAdd = aznumeric_cast<ptrdiff_t>(valueParameters.size()) - m_paramValues.size();
        if (numToAdd <= 0)
        {
            return;
        }

        // make sure we have the right space pre-allocated
        m_paramValues.reserve(valueParameters.size());

        // add the remaining parameters
        const size_t startIndex = m_paramValues.size();
        for (ptrdiff_t i = 0; i < numToAdd; ++i)
        {
            const size_t index = startIndex + i;
            m_paramValues.emplace_back(valueParameters[index]->ConstructDefaultValueAsAttribute());
        }
    }


    // remove a parameter value
    void AnimGraphInstance::RemoveParameterValue(size_t index, bool delFromMem)
    {
        if (delFromMem)
        {
            if (m_paramValues[index])
            {
                delete m_paramValues[index];
            }
        }

        m_paramValues.erase(AZStd::next(begin(m_paramValues), index));
    }


    // reinitialize the parameter
    void AnimGraphInstance::ReInitParameterValue(size_t index)
    {
        if (m_paramValues[index])
        {
            delete m_paramValues[index];
        }

        m_paramValues[index] = m_animGraph->FindValueParameter(index)->ConstructDefaultValueAsAttribute();
    }


    void AnimGraphInstance::ReInitParameterValues()
    {
        const size_t parameterValueCount = m_paramValues.size();
        for (size_t i = 0; i < parameterValueCount; ++i)
        {
            ReInitParameterValue(i);
        }
    }


    // switch to another state using a state name
    bool AnimGraphInstance::SwitchToState(const char* stateName)
    {
        // now try to find the state
        AnimGraphNode* state = m_animGraph->RecursiveFindNodeByName(stateName);
        if (state == nullptr)
        {
            return false;
        }

        // check if the parent node is a state machine or not
        AnimGraphNode* parentNode = state->GetParentNode();
        if (parentNode == nullptr) // in this case the stateName node is a state machine itself
        {
            return false;
        }

        // if it's not a state machine, then our node is not a state we can switch to
        if (azrtti_typeid(parentNode) != azrtti_typeid<AnimGraphStateMachine>())
        {
            return false;
        }

        AnimGraphStateMachine* stateMachine = static_cast<AnimGraphStateMachine*>(parentNode);

        // TODO: Double check if this still holds true
        // only allow switching to a new state when we are currently not transitioning
        if (stateMachine->IsTransitioning(this))
        {
            return false;
        }

        // recurisvely make sure the parent state machines are currently active as well
        SwitchToState(parentNode->GetName());

        // now switch to the new state
        stateMachine->SwitchToState(this, state);
        return true;
    }


    // checks if there is a transition from the current to the target node and starts a transition towards it, in case there is no transition between them the target node just gets activated
    bool AnimGraphInstance::TransitionToState(const char* stateName)
    {
        // now try to find the state
        AnimGraphNode* state = m_animGraph->RecursiveFindNodeByName(stateName);
        if (state == nullptr)
        {
            return false;
        }

        // check if the parent node is a state machine or not
        AnimGraphNode* parentNode = state->GetParentNode();
        if (parentNode == nullptr) // in this case the stateName node is a state machine itself
        {
            return false;
        }

        // if it's not a state machine, then our node is not a state we can switch to
        if (azrtti_typeid(parentNode) != azrtti_typeid<AnimGraphStateMachine>())
        {
            return false;
        }

        AnimGraphStateMachine* machine = static_cast<AnimGraphStateMachine*>(parentNode);

        // TODO: Double check if this still holds true
        // only allow switching to a new state when we are currently not transitioning
        if (machine->IsTransitioning(this))
        {
            return false;
        }

        // recurisvely make sure the parent state machines are currently active as well
        TransitionToState(parentNode->GetName());

        // now transit to the new state
        machine->TransitionToState(this, state);
        return true;
    }


    void AnimGraphInstance::RecursiveSwitchToEntryState(AnimGraphNode* node)
    {
        // check if the given node is a state machine
        if (azrtti_typeid(node) == azrtti_typeid<AnimGraphStateMachine>())
        {
            // type cast the node to a state machine
            AnimGraphStateMachine* stateMachine = static_cast<AnimGraphStateMachine*>(node);

            // switch to the entry state
            AnimGraphNode* entryState = stateMachine->GetEntryState();
            if (entryState)
            {
                stateMachine->SwitchToState(this, entryState);
                RecursiveSwitchToEntryState(entryState);
            }
        }
        else
        {
            // get the number of child nodes, iterate through them and call the function recursively in case we are dealing with a blend tree or another node
            const size_t numChildNodes = node->GetNumChildNodes();
            for (size_t i = 0; i < numChildNodes; ++i)
            {
                RecursiveSwitchToEntryState(node->GetChildNode(i));
            }
        }
    }


    // start the state machines at the entry state
    void AnimGraphInstance::Start()
    {
        RecursiveSwitchToEntryState(GetRootNode());
    }


    // reset all current states of all state machines recursively
    void AnimGraphInstance::RecursiveResetCurrentState(AnimGraphNode* node)
    {
        // check if the given node is a state machine
        if (azrtti_typeid(node) == azrtti_typeid<AnimGraphStateMachine>())
        {
            // type cast the node to a state machine
            AnimGraphStateMachine* stateMachine = static_cast<AnimGraphStateMachine*>(node);

            // reset the current state
            stateMachine->SwitchToState(this, nullptr);
        }

        // get the number of child nodes, iterate through them and call the function recursively
        const size_t numChildNodes = node->GetNumChildNodes();
        for (size_t i = 0; i < numChildNodes; ++i)
        {
            RecursiveResetCurrentState(node->GetChildNode(i));
        }
    }


    // stop the state machines and reset the current state to nullptr
    void AnimGraphInstance::Stop()
    {
        RecursiveResetCurrentState(GetRootNode());
    }


    // find the parameter value for a parameter with a given name
    MCore::Attribute* AnimGraphInstance::FindParameter(const AZStd::string& name) const
    {
        const AZ::Outcome<size_t> paramIndex = m_animGraph->FindValueParameterIndexByName(name);
        if (!paramIndex.IsSuccess())
        {
            return nullptr;
        }

        return m_paramValues[paramIndex.GetValue()];
    }


    // add the last anim graph parameter to this instance
    void AnimGraphInstance::AddParameterValue()
    {
        m_paramValues.emplace_back(nullptr);
        ReInitParameterValue(m_paramValues.size() - 1);
    }


    // add the parameter of the animgraph, at a given index
    void AnimGraphInstance::InsertParameterValue(size_t index)
    {
        m_paramValues.emplace(AZStd::next(begin(m_paramValues), index), nullptr);
        ReInitParameterValue(index);
    }


    // move the parameter from old index to new index
    void AnimGraphInstance::MoveParameterValue(size_t oldIndex, size_t newIndex)
    {
        MCore::Attribute* oldAttribute = m_paramValues[oldIndex];

        // if old index is greater than new index, move elements between oldIndex and newIndex to the right of new index
        // otherwise, move to the left of new index
        if (oldIndex > newIndex)
        {
            for (size_t paramIndex = oldIndex; paramIndex > newIndex; paramIndex--)
            {
                const size_t prevIndex = paramIndex - 1;
                m_paramValues[paramIndex] = m_paramValues[prevIndex];
            }
            m_paramValues[newIndex] = oldAttribute;
        }
        else
        {
            for (size_t paramIndex = oldIndex; paramIndex < newIndex; paramIndex++)
            {
                const size_t nexIndex = paramIndex + 1;
                m_paramValues[paramIndex] = m_paramValues[nexIndex];
            }
            m_paramValues[newIndex] = oldAttribute;
        }
    }

    void AnimGraphInstance::ResetUniqueDatas()
    {
        GetRootNode()->RecursiveResetUniqueDatas(this);
    }

    void AnimGraphInstance::RecursiveInvalidateUniqueDatas()
    {
        GetRootNode()->RecursiveInvalidateUniqueDatas(this);
    }

    size_t AnimGraphInstance::CalcNumAllocatedUniqueDatas() const
    {
        size_t result = 0;
        for (const AnimGraphObjectData* uniqueData : m_uniqueDatas)
        {
            if (uniqueData)
            {
                result++;
            }
        }
        return result;
    }

    // set a new motion set to the anim graph instance
    void AnimGraphInstance::SetMotionSet(MotionSet* motionSet)
    {
        // update the local motion set pointer
        m_motionSet = motionSet;

        // get the number of state machines, iterate through them and recursively call the callback
        GetRootNode()->RecursiveOnChangeMotionSet(this, motionSet);
    }


    // adjust the auto unregistering from the anim graph manager on delete
    void AnimGraphInstance::SetAutoUnregisterEnabled(bool enabled)
    {
        m_autoUnregister = enabled;
    }


    // do we auto unregister from the anim graph manager on delete?
    bool AnimGraphInstance::GetAutoUnregisterEnabled() const
    {
        return m_autoUnregister;
    }

    void AnimGraphInstance::SetIsOwnedByRuntime(bool isOwnedByRuntime)
    {
#if defined(EMFX_DEVELOPMENT_BUILD)
        m_isOwnedByRuntime = isOwnedByRuntime;
#else
        AZ_UNUSED(isOwnedByRuntime);
#endif
    }


    bool AnimGraphInstance::GetIsOwnedByRuntime() const
    {
#if defined(EMFX_DEVELOPMENT_BUILD)
        return m_isOwnedByRuntime;
#else
        return true;
#endif
    }


    // find an actor instance based on a parent depth value
    ActorInstance* AnimGraphInstance::FindActorInstanceFromParentDepth(size_t parentDepth) const
    {
        // start with the actor instance this anim graph instance is working on
        ActorInstance* curInstance = m_actorInstance;
        if (parentDepth == 0)
        {
            return curInstance;
        }

        // repeat until we are at the root
        size_t depth = 1;
        while (curInstance)
        {
            // get the attachment object
            Attachment* attachment = curInstance->GetSelfAttachment();

            // if this is the depth we are looking for
            if (depth == parentDepth)
            {
                if (attachment)
                {
                    return attachment->GetAttachToActorInstance();
                }
                else
                {
                    return nullptr;
                }
            }

            // traverse up the hierarchy
            if (attachment)
            {
                depth++;
                curInstance = attachment->GetAttachToActorInstance();
            }
            else
            {
                return nullptr;
            }
        }

        return nullptr;
    }

    void AnimGraphInstance::AddUniqueObjectData()
    {
        m_uniqueDatas.emplace_back(nullptr);
        m_objectFlags.emplace_back(0);
    }

    // remove the given unique data object
    void AnimGraphInstance::RemoveUniqueObjectData(AnimGraphObjectData* uniqueData, bool delFromMem)
    {
        if (uniqueData == nullptr)
        {
            return;
        }

        const size_t index = uniqueData->GetObject()->GetObjectIndex();
        if (delFromMem && m_uniqueDatas[index])
        {
            m_uniqueDatas[index]->Destroy();
        }

        m_uniqueDatas.erase(m_uniqueDatas.begin() + index);
        m_objectFlags.erase(AZStd::next(begin(m_objectFlags), index));
    }


    void AnimGraphInstance::RemoveUniqueObjectData(size_t index, bool delFromMem)
    {
        AnimGraphObjectData* data = m_uniqueDatas[index];
        m_uniqueDatas.erase(m_uniqueDatas.begin() + index);
        m_objectFlags.erase(AZStd::next(begin(m_objectFlags), index));
        if (delFromMem && data)
        {
            data->Destroy();
        }
    }


    // remove all object data
    void AnimGraphInstance::RemoveAllObjectData(bool delFromMem)
    {
        if (delFromMem)
        {
            for (AnimGraphObjectData* uniqueData : m_uniqueDatas)
            {
                if (uniqueData)
                {
                    uniqueData->Destroy();
                }
            }
        }

        m_uniqueDatas.clear();
        m_objectFlags.clear();
    }


    // register event handler
    void AnimGraphInstance::AddEventHandler(AnimGraphInstanceEventHandler* eventHandler)
    {
        AZ_Assert(eventHandler, "Expected non-null event handler");
        for (const EventTypes eventType : eventHandler->GetHandledEventTypes())
        {
            AZ_Assert(AZStd::find(m_eventHandlersByEventType[eventType - EVENT_TYPE_ANIM_GRAPH_INSTANCE_FIRST_EVENT].begin(), m_eventHandlersByEventType[eventType - EVENT_TYPE_ANIM_GRAPH_INSTANCE_FIRST_EVENT].end(), eventHandler) == m_eventHandlersByEventType[eventType - EVENT_TYPE_ANIM_GRAPH_INSTANCE_FIRST_EVENT].end(),
                "Event handler already added to manager");
            m_eventHandlersByEventType[eventType - EVENT_TYPE_ANIM_GRAPH_INSTANCE_FIRST_EVENT].emplace_back(eventHandler);
        }
    }


    // unregister event handler
    void AnimGraphInstance::RemoveEventHandler(AnimGraphInstanceEventHandler* eventHandler)
    {
        for (const EventTypes eventType : eventHandler->GetHandledEventTypes())
        {
            EventHandlerVector& eventHandlers = m_eventHandlersByEventType[eventType - EVENT_TYPE_ANIM_GRAPH_INSTANCE_FIRST_EVENT];
            eventHandlers.erase(AZStd::remove(eventHandlers.begin(), eventHandlers.end(), eventHandler), eventHandlers.end());
        }
    }


    // remove all event handlers
    void AnimGraphInstance::RemoveAllEventHandlers()
    {
#ifdef AZ_DEBUG_BUILD
        for (const EventHandlerVector& eventHandlers : m_eventHandlersByEventType)
        {
            AZ_Assert(eventHandlers.empty(), "Expected all event handlers to be removed");
        }
#endif
        m_eventHandlersByEventType.clear();
    }


    void AnimGraphInstance::OnStateEnter(AnimGraphNode* state)
    {
        const EventHandlerVector& eventHandlers = m_eventHandlersByEventType[EVENT_TYPE_ON_STATE_ENTER - EVENT_TYPE_ANIM_GRAPH_INSTANCE_FIRST_EVENT];
        for (AnimGraphInstanceEventHandler* eventHandler : eventHandlers)
        {
            eventHandler->OnStateEnter(this, state);
        }
    }


    void AnimGraphInstance::OnStateEntering(AnimGraphNode* state)
    {
        const EventHandlerVector& eventHandlers = m_eventHandlersByEventType[EVENT_TYPE_ON_STATE_ENTERING - EVENT_TYPE_ANIM_GRAPH_INSTANCE_FIRST_EVENT];
        for (AnimGraphInstanceEventHandler* eventHandler : eventHandlers)
        {
            eventHandler->OnStateEntering(this, state);
        }
    }


    void AnimGraphInstance::OnStateExit(AnimGraphNode* state)
    {
        const EventHandlerVector& eventHandlers = m_eventHandlersByEventType[EVENT_TYPE_ON_STATE_EXIT - EVENT_TYPE_ANIM_GRAPH_INSTANCE_FIRST_EVENT];
        for (AnimGraphInstanceEventHandler* eventHandler : eventHandlers)
        {
            eventHandler->OnStateExit(this, state);
        }
    }


    void AnimGraphInstance::OnStateEnd(AnimGraphNode* state)
    {
        const EventHandlerVector& eventHandlers = m_eventHandlersByEventType[EVENT_TYPE_ON_STATE_END - EVENT_TYPE_ANIM_GRAPH_INSTANCE_FIRST_EVENT];
        for (AnimGraphInstanceEventHandler* eventHandler : eventHandlers)
        {
            eventHandler->OnStateEnd(this, state);
        }
    }


    void AnimGraphInstance::OnStartTransition(AnimGraphStateTransition* transition)
    {
        const EventHandlerVector& eventHandlers = m_eventHandlersByEventType[EVENT_TYPE_ON_START_TRANSITION - EVENT_TYPE_ANIM_GRAPH_INSTANCE_FIRST_EVENT];
        for (AnimGraphInstanceEventHandler* eventHandler : eventHandlers)
        {
            eventHandler->OnStartTransition(this, transition);
        }
    }


    void AnimGraphInstance::OnEndTransition(AnimGraphStateTransition* transition)
    {
        const EventHandlerVector& eventHandlers = m_eventHandlersByEventType[EVENT_TYPE_ON_END_TRANSITION - EVENT_TYPE_ANIM_GRAPH_INSTANCE_FIRST_EVENT];
        for (AnimGraphInstanceEventHandler* eventHandler : eventHandlers)
        {
            eventHandler->OnEndTransition(this, transition);
        }
    }


    // init the hashmap
    void AnimGraphInstance::InitUniqueDatas()
    {
        const size_t numObjects = m_animGraph->GetNumObjects();
        m_uniqueDatas.resize(numObjects);
        m_objectFlags.resize(numObjects);
        for (size_t i = 0; i < numObjects; ++i)
        {
            m_uniqueDatas[i] = nullptr;
            m_objectFlags[i] = 0;
        }
    }


    // get the root node
    AnimGraphNode* AnimGraphInstance::GetRootNode() const
    {
        return m_animGraph->GetRootStateMachine();
    }


    // apply motion extraction
    void AnimGraphInstance::ApplyMotionExtraction()
    {
        // perform motion extraction
        Transform trajectoryDelta;

        // get the motion extraction node, and if it hasn't been set, we can already quit
        Node* motionExtractNode = m_actorInstance->GetActor()->GetMotionExtractionNode();
        if (motionExtractNode == nullptr)
        {
            trajectoryDelta.IdentityWithZeroScale();
            m_actorInstance->SetTrajectoryDeltaTransform(trajectoryDelta);
            return;
        }

        // get the root node's trajectory delta
        AnimGraphRefCountedData* rootData = m_animGraph->GetRootStateMachine()->FindOrCreateUniqueNodeData(this)->GetRefCountedData();
        trajectoryDelta = rootData->GetTrajectoryDelta();
        trajectoryDelta.m_rotation.Normalize();

        // update the actor instance with the delta movement already
        m_actorInstance->SetTrajectoryDeltaTransform(trajectoryDelta);
        m_actorInstance->ApplyMotionExtractionDelta();
    }


    // synchronize all nodes, based on sync tracks etc
    void AnimGraphInstance::Update(float timePassedInSeconds)
    {
        AZ_PROFILE_SCOPE(Animation, "AnimGraphInstance::Update");

        // pass 0: (Optional, networking only) When this instance is shared between network, restore the instance using an animgraph snapshot.
        if (m_snapshot)
        {
            m_snapshot->Restore(*this);
        }

        // pass 1: update (bottom up), update motion timers etc
        // pass 2: topdown update (top down), syncing happens here (adjusts motion/node timers again)
        // pass 3: postupdate (bottom up), processing the motion events events and update motion extraction deltas
        // pass 4: output (bottom up), calculate all new bone transforms (the heavy thing to process) <--- not performed by this function but in AnimGraphInstance::Output()

        // reset the output is ready flags, so we return cached copies of the outputs, but refresh/recalculate them
        AnimGraphNode* rootNode = GetRootNode();

        ResetFlagsForAllObjects();

        if (GetEMotionFX().GetIsInEditorMode())
        {
            rootNode->RecursiveResetFlags(this, 0xffffffff);    // the 0xffffffff clears all flags
        }

        // reset all node pose ref counts
        const uint32 threadIndex = m_actorInstance->GetThreadIndex();
        ResetPoseRefCountsForAllNodes();
        ResetRefDataRefCountsForAllNodes();
        GetEMotionFX().GetThreadData(threadIndex)->GetRefCountedDataPool().ResetMaxUsedItems();

        // perform a bottom-up update, which updates the nodes, and sets their sync tracks, and play time etc
        rootNode->IncreasePoseRefCount(this);
        rootNode->IncreaseRefDataRefCount(this);
        rootNode->PerformUpdate(this, timePassedInSeconds);

        // perform a top-down update, starting from the root and going downwards to the leaf nodes
        AnimGraphNodeData* rootNodeUniqueData = rootNode->FindOrCreateUniqueNodeData(this);
        rootNodeUniqueData->SetGlobalWeight(1.0f); // start with a global weight of 1 at the root
        rootNodeUniqueData->SetLocalWeight(1.0f); // start with a local weight of 1 at the root
        rootNode->PerformTopDownUpdate(this, timePassedInSeconds);

        // bottom up pass event buffers and update motion extraction deltas
        rootNode->PerformPostUpdate(this, timePassedInSeconds);

        //-------------------------------------

        // apply motion extraction
        ApplyMotionExtraction();

        // store a copy of the root's event buffer
        m_eventBuffer = rootNodeUniqueData->GetRefCountedData()->GetEventBuffer();

        // trigger the events inside the root node's buffer
        OutputEvents();

        rootNode->DecreaseRefDataRef(this);

        //MCore::LogInfo("------bef refData used = %d/%d (max=%d)----------", GetEMotionFX().GetThreadData(threadIndex)->GetRefCountedDataPool().GetNumUsedItems(), GetEMotionFX().GetThreadData(threadIndex)->GetRefCountedDataPool().GetNumItems(), GetEMotionFX().GetThreadData(threadIndex)->GetRefCountedDataPool().GetNumMaxUsedItems());

        // Only release for root/main anim graphs and when we want to.
        if (m_autoReleaseAllRefDatas && !m_parentAnimGraphInstance)
        {
            ReleaseRefDatas();
        }

        //MCore::LogInfo("------refData used = %d/%d (max=%d)----------", GetEMotionFX().GetThreadData(threadIndex)->GetRefCountedDataPool().GetNumUsedItems(), GetEMotionFX().GetThreadData(threadIndex)->GetRefCountedDataPool().GetNumItems(), GetEMotionFX().GetThreadData(threadIndex)->GetRefCountedDataPool().GetNumMaxUsedItems());
    }


    // recursively reset flags
    void AnimGraphInstance::RecursiveResetFlags(uint32 flagsToDisable)
    {
        m_animGraph->GetRootStateMachine()->RecursiveResetFlags(this, flagsToDisable);
    }


    // reset all node flags
    void AnimGraphInstance::ResetFlagsForAllObjects(uint32 flagsToDisable)
    {
        for (uint32& objectFlag : m_objectFlags)
        {
            objectFlag &= ~flagsToDisable;
        }
    }


    // reset all node pose ref counts
    void AnimGraphInstance::ResetPoseRefCountsForAllNodes()
    {
        AZ_PROFILE_SCOPE(Animation, "AnimGraphInstance::ResetPoseRefCountsForAllNodes");

        const size_t numNodes = m_animGraph->GetNumNodes();
        for (size_t i = 0; i < numNodes; ++i)
        {
            m_animGraph->GetNode(i)->ResetPoseRefCount(this);
        }
    }


    // reset all node pose ref counts
    void AnimGraphInstance::ResetRefDataRefCountsForAllNodes()
    {
        AZ_PROFILE_SCOPE(Animation, "AnimGraphInstance::ResetRefDataRefCountsForAllNodes");

        const size_t numNodes = m_animGraph->GetNumNodes();
        for (size_t i = 0; i < numNodes; ++i)
        {
            m_animGraph->GetNode(i)->ResetRefDataRefCount(this);
        }
    }


    // reset all node flags
    void AnimGraphInstance::ResetFlagsForAllObjects()
    {
        AZ_PROFILE_SCOPE(Animation, "AnimGraphInstance::ResetFlagsForAllObjects");

        MCore::MemSet(m_objectFlags.data(), 0, sizeof(uint32) * m_objectFlags.size());

        for (AnimGraphInstance* childInstance : m_childAnimGraphInstances)
        {
            childInstance->ResetFlagsForAllObjects();
        }
    }


    // reset flags for all nodes
    void AnimGraphInstance::ResetFlagsForAllNodes(uint32 flagsToDisable)
    {
        const size_t numNodes = m_animGraph->GetNumNodes();
        for (size_t i = 0; i < numNodes; ++i)
        {
            AnimGraphNode* node = m_animGraph->GetNode(i);
            m_objectFlags[node->GetObjectIndex()] &= ~flagsToDisable;

            if (GetEMotionFX().GetIsInEditorMode())
            {
                // reset all connections
                const size_t numConnections = node->GetNumConnections();
                for (size_t c = 0; c < numConnections; ++c)
                {
                    node->GetConnection(c)->SetIsVisited(false);
                }
            }
        }
    }


    // output the events
    void AnimGraphInstance::OutputEvents()
    {
        AnimGraphNode* rootNode = GetRootNode();
        AnimGraphRefCountedData* rootData = rootNode->FindOrCreateUniqueNodeData(this)->GetRefCountedData();
        AnimGraphEventBuffer& eventBuffer = rootData->GetEventBuffer();
        eventBuffer.UpdateWeights(this);
        eventBuffer.TriggerEvents();
        //eventBuffer.Log();
    }


    // recursively collect all active anim graph nodes
    void AnimGraphInstance::CollectActiveAnimGraphNodes(AZStd::vector<AnimGraphNode*>* outNodes, const AZ::TypeId& nodeType)
    {
        outNodes->clear();
        m_animGraph->GetRootStateMachine()->RecursiveCollectActiveNodes(this, outNodes, nodeType);
    }


    void AnimGraphInstance::CollectActiveNetTimeSyncNodes(AZStd::vector<AnimGraphNode*>* outNodes)
    {
        outNodes->clear();
        m_animGraph->GetRootStateMachine()->RecursiveCollectActiveNetTimeSyncNodes(this, outNodes);
    }

    AnimGraphObjectData* AnimGraphInstance::FindOrCreateUniqueObjectData(const AnimGraphObject* object)
    {
        const size_t objectIndex = object->GetObjectIndex();
        AnimGraphObjectData* uniqueData = m_uniqueDatas[objectIndex];
        if (uniqueData)
        {
            // The unique data already existed, it might be invalidated.
            if (uniqueData->IsInvalidated())
            {
                uniqueData->Update();
                uniqueData->Validate();
            }
            return uniqueData;
        }

        // Create the unique data and register it at the array. This needs to happen before calling Update() on it.
        uniqueData = const_cast<AnimGraphObject*>(object)->CreateUniqueData(const_cast<AnimGraphInstance*>(this));
        AZ_Assert(uniqueData, "CreateUniqueData() not implemented for %s. nullptr was returned.", object->GetPaletteName());
        m_uniqueDatas[objectIndex] = uniqueData;

        uniqueData->Update();
        uniqueData->Validate();
        return uniqueData;
    }

    AnimGraphNodeData* AnimGraphInstance::FindOrCreateUniqueNodeData(const AnimGraphNode* node)
    {
        return reinterpret_cast<AnimGraphNodeData*>(FindOrCreateUniqueObjectData(node));
    }

    // find the parameter index
    AZ::Outcome<size_t> AnimGraphInstance::FindParameterIndex(const AZStd::string& name) const
    {
        return m_animGraph->FindValueParameterIndexByName(name);
    }


    // init all internal attributes
    void AnimGraphInstance::InitInternalAttributes()
    {
        const size_t numNodes = m_animGraph->GetNumNodes();
        for (size_t i = 0; i < numNodes; ++i)
        {
            m_animGraph->GetNode(i)->InitInternalAttributes(this);
        }
    }


    void AnimGraphInstance::SetVisualizeScale(float scale)
    {
        m_visualizeScale = scale;
    }


    float AnimGraphInstance::GetVisualizeScale() const
    {
        return m_visualizeScale;
    }


    void AnimGraphInstance::SetVisualizationEnabled(bool enabled)
    {
        m_enableVisualization = enabled;
    }


    bool AnimGraphInstance::GetVisualizationEnabled() const
    {
        return m_enableVisualization;
    }


    bool AnimGraphInstance::GetRetargetingEnabled() const
    {
        return m_retarget;
    }


    void AnimGraphInstance::SetRetargetingEnabled(bool enabled)
    {
        m_retarget = enabled;
    }

    const AnimGraphInstance::InitSettings& AnimGraphInstance::GetInitSettings() const
    {
        return m_initSettings;
    }


    const AnimGraphEventBuffer& AnimGraphInstance::GetEventBuffer() const
    {
        return m_eventBuffer;
    }


    void AnimGraphInstance::AddFollowerGraph(AnimGraphInstance* follower, bool registerLeaderInsideFollower)
    {
        const auto& it = AZStd::find(m_followerGraphs.begin(), m_followerGraphs.end(), follower);
        if (it == m_followerGraphs.end())
        {
            m_followerGraphs.emplace_back(follower);
        }

        if (registerLeaderInsideFollower)
        {
            follower->AddLeaderGraph(this);
        }
    }

    void AnimGraphInstance::RemoveFollowerGraph(AnimGraphInstance* follower, bool removeLeaderFromFollower)
    {
        const auto& it = AZStd::find(m_followerGraphs.begin(), m_followerGraphs.end(), follower);
        if (it != m_followerGraphs.end())
        {
            m_followerGraphs.erase(it);
        }

        if (removeLeaderFromFollower)
        {
            follower->RemoveLeaderGraph(this);
        }
    }

    AZStd::vector<AnimGraphInstance*>& AnimGraphInstance::GetFollowerGraphs()
    {
        return m_followerGraphs;
    }

    void AnimGraphInstance::AddLeaderGraph(AnimGraphInstance* leader)
    {
        const auto& it = AZStd::find(m_leaderGraphs.begin(), m_leaderGraphs.end(), leader);
        if (it == m_leaderGraphs.end())
        {
            m_leaderGraphs.emplace_back(leader);
        }
    }

    void AnimGraphInstance::RemoveLeaderGraph(AnimGraphInstance* leader)
    {
        const auto& it = AZStd::find(m_leaderGraphs.begin(), m_leaderGraphs.end(), leader);
        if (it != m_leaderGraphs.end())
        {
            m_leaderGraphs.erase(it);
        }
    }

    AZStd::vector<AnimGraphInstance*>& AnimGraphInstance::GetLeaderGraphs()
    {
        return m_leaderGraphs;
    }

    void AnimGraphInstance::CreateSnapshot(bool authoritative)
    {
        if (m_snapshot)
        {
            AZ_Error("EMotionFX", false, "Snapshot already created for this animgraph instance.");
            return;
        }
        m_snapshot = AZStd::make_shared<AnimGraphSnapshot>(*this, authoritative);
    }

    void AnimGraphInstance::SetSnapshotSerializer(AZStd::shared_ptr<Network::AnimGraphSnapshotSerializer> serializer)
    {
        if (!m_snapshot)
        {
            AZ_Error("EMotionFX", false, "Snapshot should be created first.");
            return;
        }
        m_snapshot->SetSnapshotSerializer(serializer);
    }

    void AnimGraphInstance::SetSnapshotChunkSerializer(AZStd::shared_ptr<Network::AnimGraphSnapshotChunkSerializer> serializer)
    {
        if (!m_snapshot)
        {
            AZ_Error("EMotionFX", false, "Snapshot should be created first.");
            return;
        }
        m_snapshot->SetSnapshotChunkSerializer(serializer);
    }

    void AnimGraphInstance::OnNetworkConnected()
    {
        if (!m_snapshot)
        {
            AZ_Error("EMotionFX", false, "Snapshot should be created first.");
            return;
        }
         m_snapshot->OnNetworkConnected(*this);
    }

    void AnimGraphInstance::OnNetworkParamUpdate(const AttributeContainer& parameters)
    {
        if (!m_snapshot)
        {
            AZ_Error("EMotionFX", false, "Snapshot should be created first.");
            return;
        }
        m_snapshot->SetParameters(parameters);
    }

    void AnimGraphInstance::OnNetworkActiveNodesUpdate(const AZStd::vector<AZ::u32>& activeNodes)
    {
        if (!m_snapshot)
        {
            AZ_Error("EMotionFX", false, "Snapshot should be created first.");
            return;
        }
        m_snapshot->SetActiveNodes(activeNodes);
    }

    void AnimGraphInstance::OnNetworkMotionNodePlaytimesUpdate(const MotionNodePlaytimeContainer& motionNodePlaytimes)
    {
        if (!m_snapshot)
        {
            AZ_Error("EMotionFX", false, "Snapshot should be created first.");
            return;
        }
        m_snapshot->SetMotionNodePlaytimes(motionNodePlaytimes);
    }

    void AnimGraphInstance::SetAutoReleaseRefDatas(bool automaticallyFreeRefDatas)
    {
        m_autoReleaseAllRefDatas = automaticallyFreeRefDatas;
    }

    void AnimGraphInstance::SetAutoReleasePoses(bool automaticallyFreePoses)
    {
        m_autoReleaseAllPoses = automaticallyFreePoses;
    }

    void AnimGraphInstance::ReleaseRefDatas()
    {
        const uint32 threadIndex = m_actorInstance->GetThreadIndex();
        AnimGraphRefCountedDataPool& refDataPool = GetEMotionFX().GetThreadData(threadIndex)->GetRefCountedDataPool();

        const size_t numNodes = m_animGraph->GetNumNodes();
        for (size_t i = 0; i < numNodes; ++i)
        {
            const AnimGraphNode* node = m_animGraph->GetNode(i);
            AnimGraphNodeData* nodeData = static_cast<AnimGraphNodeData*>(m_uniqueDatas[node->GetObjectIndex()]);
            if (nodeData)
            {
                AnimGraphRefCountedData* refData = nodeData->GetRefCountedData();
                if (refData)
                {
                    refDataPool.Free(refData);
                    nodeData->SetRefCountedData(nullptr);
                }
            }
        }
    }

    void AnimGraphInstance::ReleasePoses()
    {
        for (AnimGraphInstance* childAnimGraphInstance : m_childAnimGraphInstances)
        {
            childAnimGraphInstance->ReleasePoses();
        }

        for (MCore::Attribute* attribute : m_internalAttributes)
        {
            if (attribute->GetType() == AttributePose::TYPE_ID)
            {
                AttributePose* attributePose = static_cast<AttributePose*>(attribute);
                attributePose->SetValue(nullptr);
            }
        }
    }

    bool AnimGraphInstance::GetParameterValueAsFloat(size_t paramIndex, float* outValue)
    {
        MCore::AttributeFloat* floatAttribute = GetParameterValueChecked<MCore::AttributeFloat>(paramIndex);
        if (floatAttribute)
        {
            *outValue = floatAttribute->GetValue();
            return true;
        }

        MCore::AttributeInt32* intAttribute = GetParameterValueChecked<MCore::AttributeInt32>(paramIndex);
        if (intAttribute)
        {
            *outValue = static_cast<float>(intAttribute->GetValue());
            return true;
        }

        MCore::AttributeBool* boolAttribute = GetParameterValueChecked<MCore::AttributeBool>(paramIndex);
        if (boolAttribute)
        {
            *outValue = static_cast<float>(boolAttribute->GetValue());
            return true;
        }

        return false;
    }

    bool AnimGraphInstance::GetParameterValueAsBool(size_t paramIndex, bool* outValue)
    {
        float floatValue;
        if (GetParameterValueAsFloat(paramIndex, &floatValue))
        {
            *outValue = (MCore::Math::IsFloatZero(floatValue) == false);
            return true;
        }

        return false;
    }


    bool AnimGraphInstance::GetParameterValueAsInt(size_t paramIndex, int32* outValue)
    {
        float floatValue;
        if (GetParameterValueAsFloat(paramIndex, &floatValue))
        {
            *outValue = static_cast<int32>(floatValue);
            return true;
        }

        return false;
    }


    bool AnimGraphInstance::GetVector2ParameterValue(size_t paramIndex, AZ::Vector2* outValue)
    {
        MCore::AttributeVector2* param = GetParameterValueChecked<MCore::AttributeVector2>(paramIndex);
        if (param)
        {
            *outValue = param->GetValue();
            return true;
        }

        return false;
    }


    bool AnimGraphInstance::GetVector3ParameterValue(size_t paramIndex, AZ::Vector3* outValue)
    {
        MCore::AttributeVector3* param = GetParameterValueChecked<MCore::AttributeVector3>(paramIndex);
        if (param)
        {
            *outValue = param->GetValue();
            return true;
        }

        return false;
    }


    bool AnimGraphInstance::GetVector4ParameterValue(size_t paramIndex, AZ::Vector4* outValue)
    {
        MCore::AttributeVector4* param = GetParameterValueChecked<MCore::AttributeVector4>(paramIndex);
        if (param)
        {
            *outValue = param->GetValue();
            return true;
        }

        return false;
    }


    bool AnimGraphInstance::GetRotationParameterValue(size_t paramIndex, AZ::Quaternion* outRotation)
    {
        MCore::AttributeQuaternion* param = GetParameterValueChecked<MCore::AttributeQuaternion>(paramIndex);
        if (param)
        {
            *outRotation = param->GetValue();
            return true;
        }

        return false;
    }

    void AnimGraphInstance::SetParentAnimGraphInstance(AnimGraphInstance* parentAnimGraphInstance)
    {
        if (m_parentAnimGraphInstance)
        {
            m_parentAnimGraphInstance->m_childAnimGraphInstances.erase(
                AZStd::remove(m_parentAnimGraphInstance->m_childAnimGraphInstances.begin(), m_parentAnimGraphInstance->m_childAnimGraphInstances.end(), this),
                m_parentAnimGraphInstance->m_childAnimGraphInstances.end());
        }

        m_parentAnimGraphInstance = parentAnimGraphInstance;

        // Add myself to the parent
        if (parentAnimGraphInstance)
        {
            parentAnimGraphInstance->m_childAnimGraphInstances.emplace_back(this);
        }
    }


    void AnimGraphInstance::RemoveChildAnimGraphInstance(AnimGraphInstance* childAnimGraphInstance)
    {
        m_childAnimGraphInstances.erase(AZStd::remove(m_childAnimGraphInstances.begin(), m_childAnimGraphInstances.end(), childAnimGraphInstance), m_childAnimGraphInstances.end());
    }


    bool AnimGraphInstance::GetParameterValueAsFloat(const char* paramName, float* outValue)
    {
        const AZ::Outcome<size_t> index = FindParameterIndex(paramName);
        if (!index.IsSuccess())
        {
            return false;
        }

        return GetParameterValueAsFloat(index.GetValue(), outValue);
    }


    bool AnimGraphInstance::GetParameterValueAsBool(const char* paramName, bool* outValue)
    {
        const AZ::Outcome<size_t> index = FindParameterIndex(paramName);
        if (!index.IsSuccess())
        {
            return false;
        }

        return GetParameterValueAsBool(index.GetValue(), outValue);
    }


    bool AnimGraphInstance::GetParameterValueAsInt(const char* paramName, int32* outValue)
    {
        const AZ::Outcome<size_t> index = FindParameterIndex(paramName);
        if (!index.IsSuccess())
        {
            return false;
        }

        return GetParameterValueAsInt(index.GetValue(), outValue);
    }


    bool AnimGraphInstance::GetVector2ParameterValue(const char* paramName, AZ::Vector2* outValue)
    {
        const AZ::Outcome<size_t> index = FindParameterIndex(paramName);
        if (!index.IsSuccess())
        {
            return false;
        }

        return GetVector2ParameterValue(index.GetValue(), outValue);
    }


    bool AnimGraphInstance::GetVector3ParameterValue(const char* paramName, AZ::Vector3* outValue)
    {
        const AZ::Outcome<size_t> index = FindParameterIndex(paramName);
        if (!index.IsSuccess())
        {
            return false;
        }

        return GetVector3ParameterValue(index.GetValue(), outValue);
    }


    bool AnimGraphInstance::GetVector4ParameterValue(const char* paramName, AZ::Vector4* outValue)
    {
        const AZ::Outcome<size_t> index = FindParameterIndex(paramName);
        if (!index.IsSuccess())
        {
            return false;
        }

        return GetVector4ParameterValue(index.GetValue(), outValue);
    }


    bool AnimGraphInstance::GetRotationParameterValue(const char* paramName, AZ::Quaternion* outRotation)
    {
        const AZ::Outcome<size_t> index = FindParameterIndex(paramName);
        if (!index.IsSuccess())
        {
            return false;
        }

        return GetRotationParameterValue(index.GetValue(), outRotation);
    }
} // namespace EMotionFX

