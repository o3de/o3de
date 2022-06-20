/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <ScriptedEntityTweener/ScriptedEntityTweenerBus.h>

#include "ScriptedEntityTweenerTask.h"

namespace ScriptedEntityTweener
{
    const AZStd::any ScriptedEntityTweenerTask::QueuedSubtaskInfo::m_emptyInitialValue;

    const float AnimationProperties::UninitializedParamFloat = FLT_MIN;
    const int AnimationProperties::InvalidCallbackId = 0;
    const unsigned int AnimationProperties::InvalidTimelineId = 0;

    ScriptedEntityTweenerTask::ScriptedEntityTweenerTask(AZ::EntityId id)
        : m_entityId(id)
    {
    }

    ScriptedEntityTweenerTask::~ScriptedEntityTweenerTask()
    {
    }

    void ScriptedEntityTweenerTask::AddAnimation(const AnimationParameters& params, bool overwriteQueued)
    {
        float timeToDelay = params.m_animationProperties.m_timeToDelayAnim;
        if (timeToDelay > .0f)
        {
            m_queuedSubtasks.emplace_back(QueuedSubtaskInfo(params, params.m_animationProperties.m_timeToDelayAnim));
            return;
        }

        for (const auto& it : params.m_animationParameters)
        {
            const AnimationParameterAddressData& addressData = it.first;
            if (IsTimelineIdValid(params.m_animationProperties.m_timelineId))
            {

                ScriptedEntityTweenerNotificationsBus::Broadcast(&ScriptedEntityTweenerNotificationsBus::Events::OnTimelineAnimationStart,
                    params.m_animationProperties.m_timelineId,
                    params.m_animationProperties.m_animationId,
                    addressData.m_componentName,
                    addressData.m_virtualPropertyName);
            }

            auto subtask = m_subtasks.find(addressData);
            if (subtask == m_subtasks.end())
            {
                //For this property on this entity, an animation isn't already running.
                ScriptedEntityTweenerSubtask subtaskToAdd(m_entityId);
                if (params.m_animationProperties.m_timeDuration == 0.0f)
                {
                    //If the animation is a set animation, immediately initialize, execute, and call callbacks related to it.
                    if (InitializeSubtask(subtaskToAdd, it, params))
                    {
                        AZStd::set<CallbackData> callbacks;
                        subtaskToAdd.Update(0, callbacks);
                        ExecuteCallbacks(callbacks);
                    }
                    return;
                }
                else
                {
                    //Animation will play over some time, enqueue it to play as part of the update loop.
                    subtask = m_subtasks.emplace(addressData, subtaskToAdd).first;
                }
            }
            else
            {
                //An animation already exists for this virtual property
                //Cleanup any callbacks it may have registered.
                ClearCallbacks(subtask->second.GetAnimationProperties());

                //Overwrite any queued animations on this subtask if the animation wasn't started from the queue, as it was user specified.
                if (overwriteQueued)
                {
                    auto queuedSubtaskIter = m_queuedSubtasks.begin();
                    while (queuedSubtaskIter != m_queuedSubtasks.end())
                    {
                        auto& queuedAnimParams = queuedSubtaskIter->GetParameters();

                        //Remove each queued animation relating to this virtual address.
                        auto queuedAnimParamIter = queuedAnimParams.m_animationParameters.begin();
                        while (queuedAnimParamIter != queuedAnimParams.m_animationParameters.end())
                        {
                            const AnimationParameterAddressData& queuedAddressData = queuedAnimParamIter->first;
                            if (addressData == queuedAddressData)
                            {
                                queuedAnimParamIter = queuedAnimParams.m_animationParameters.erase(queuedAnimParamIter);
                            }
                            else
                            {
                                ++queuedAnimParamIter;
                            }
                        }
                        
                        //If queued anim subtask no longer contains any parameters to update, remove it completely.
                        if (queuedSubtaskIter->GetParameters().m_animationParameters.size() == 0)
                        {
                            queuedSubtaskIter = m_queuedSubtasks.erase(queuedSubtaskIter);
                        }
                        else
                        {
                            ++queuedSubtaskIter;
                        }
                    }
                }
            }

            if (!InitializeSubtask(subtask->second, it, params))
            {
                m_subtasks.erase(subtask);
            }
        }
    }

    void ScriptedEntityTweenerTask::Update(float deltaTime)
    {
        auto queuedIter = m_queuedSubtasks.begin();
        while (queuedIter != m_queuedSubtasks.end())
        {
            if (queuedIter->UpdateUntilReady(deltaTime))
            {
                AddAnimation(queuedIter->GetParameters(), false);
                if (queuedIter->HasInitialValue())
                {
                    auto& queuedAnimParams = queuedIter->GetParameters();
                    for (const auto& it : queuedAnimParams.m_animationParameters)
                    {
                        const AnimationParameterAddressData& addressData = it.first;
                        
                        const AZStd::any& initialValue = queuedIter->GetInitialValue(addressData);
                        if (!initialValue.empty())
                        {
                            auto subtaskPair = m_subtasks.find(addressData);
                            if (subtaskPair != m_subtasks.end())
                            {
                                ScriptedEntityTweenerSubtask& subtask = subtaskPair->second;
                                subtask.SetInitialValue(queuedIter->GetAnimationId(), initialValue);
                            }
                        }
                    }
                }
                queuedIter = m_queuedSubtasks.erase(queuedIter);
            }
            else
            {
                ++queuedIter;
            }
        }

        m_callbacks.clear();
        for (auto& subtaskPair : m_subtasks)
        {
            ScriptedEntityTweenerSubtask& subtask = subtaskPair.second;
            if (subtask.IsActive())
            {
                subtask.Update(deltaTime, m_callbacks);
            }
        }

        // Aggregate all callbacks from the subtasks to execute them all at once, as multiple subtasks may reference the same callback
        ExecuteCallbacks(m_callbacks);

        //TODO: Possible optimization: Logic to remove "stale" animation subtasks, use a "garbage collection" method to defer set removal operations (which is O(n))
        for (auto it = m_subtasks.begin(); it != m_subtasks.end(); )
        {
            ScriptedEntityTweenerSubtask& subtask = it->second;
            if (!subtask.IsActive())
            {
                it = m_subtasks.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }

    bool ScriptedEntityTweenerTask::GetIsActive()
    {
        for (auto& subtaskPair : m_subtasks)
        {
            ScriptedEntityTweenerSubtask& subtask = subtaskPair.second;
            if (subtask.IsActive())
            {
                return true;
            }
        }

        return m_queuedSubtasks.size() > 0;
    }

    void ScriptedEntityTweenerTask::Stop(int timelineId)
    {
        for (auto it = m_queuedSubtasks.begin(); it != m_queuedSubtasks.end(); )
        {
            auto& queuedSubtask = (*it);
            if (timelineId == 0 || queuedSubtask.GetTimelineId() == timelineId)
            {
                ClearCallbacks(queuedSubtask.GetParameters().m_animationProperties);
                it = m_queuedSubtasks.erase(it);
            }
            else
            {
                ++it;
            }
        }

        for (auto it = m_subtasks.begin(); it != m_subtasks.end(); )
        {
            ScriptedEntityTweenerSubtask& subtask = it->second;
            if (timelineId == 0 || subtask.GetTimelineId() == timelineId)
            {
                ClearCallbacks(subtask.GetAnimationProperties());
                it = m_subtasks.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }

    void ScriptedEntityTweenerTask::SetPaused(const AnimationParameterAddressData& addressData, int timelineId, bool isPaused)
    {
        auto subtaskPair = m_subtasks.find(addressData);
        if (subtaskPair != m_subtasks.end())
        {
            ScriptedEntityTweenerSubtask& subtask = subtaskPair->second;
            subtask.SetPaused(timelineId, isPaused);
        }

        if (IsTimelineIdValid(timelineId))
        {
            for (auto& queuedSubtask : m_queuedSubtasks)
            {
                if (queuedSubtask.GetTimelineId() == timelineId)
                {
                    queuedSubtask.SetPaused(isPaused);
                }
            }
        }
    }

    void ScriptedEntityTweenerTask::SetPlayDirectionReversed(const AnimationParameterAddressData& addressData, int timelineId, bool isPlayingBackward)
    {
        auto subtaskPair = m_subtasks.find(addressData);
        if (subtaskPair != m_subtasks.end())
        {
            ScriptedEntityTweenerSubtask& subtask = subtaskPair->second;
            subtask.SetPlayDirectionReversed(timelineId, isPlayingBackward);
        }

        //Remove any subtask queued for this timeline id, as now that we're rewinding, they should not play.
        if (IsTimelineIdValid(timelineId))
        {
            auto queuedIter = m_queuedSubtasks.begin();
            while (queuedIter != m_queuedSubtasks.end())
            {
                auto& queuedSubtask = (*queuedIter);
                if (queuedSubtask.GetTimelineId() == timelineId)
                {
                    queuedIter = m_queuedSubtasks.erase(queuedIter);
                }
                else
                {
                    ++queuedIter;
                }
            }
        }
    }

    void ScriptedEntityTweenerTask::SetSpeed(const AnimationParameterAddressData& addressData, int timelineId, float speed)
    {
        auto subtaskPair = m_subtasks.find(addressData);
        if (subtaskPair != m_subtasks.end())
        {
            ScriptedEntityTweenerSubtask& subtask = subtaskPair->second;
            subtask.SetSpeed(timelineId, speed);
        }

        if (IsTimelineIdValid(timelineId))
        {
            for (auto& queuedSubtask : m_queuedSubtasks)
            {
                if (queuedSubtask.GetTimelineId() == timelineId)
                {
                    queuedSubtask.SetPlaybackSpeed(speed);
                }
            }
        }
    }

    void ScriptedEntityTweenerTask::SetInitialValue(const AnimationParameterAddressData& addressData, const AZ::Uuid& animationId, const AZStd::any& initialValue)
    {
        auto subtaskPair = m_subtasks.find(addressData);
        if (subtaskPair != m_subtasks.end())
        {
            ScriptedEntityTweenerSubtask& subtask = subtaskPair->second;
            subtask.SetInitialValue(animationId, initialValue);
        }

        if (!animationId.IsNull())
        {
            for (auto& queuedSubtask : m_queuedSubtasks)
            {
                if (queuedSubtask.GetAnimationId() == animationId)
                {
                    queuedSubtask.SetInitialValue(addressData, initialValue);
                }
            }
        }
    }

    void ScriptedEntityTweenerTask::GetVirtualPropertyValue(AZStd::any& returnVal, const AnimationParameterAddressData& addressData)
    {
        auto subtaskPair = m_subtasks.find(addressData);
        if (subtaskPair != m_subtasks.end())
        {
            ScriptedEntityTweenerSubtask& subtask = subtaskPair->second;
            subtask.GetVirtualPropertyValue(returnVal, addressData);
        }
        else
        {
            ScriptedEntityTweenerSubtask tempSubtask(m_entityId);
            tempSubtask.GetVirtualPropertyValue(returnVal, addressData);
        }
    }

    bool ScriptedEntityTweenerTask::InitializeSubtask(ScriptedEntityTweenerSubtask& subtask, const AZStd::pair<AnimationParameterAddressData, AZStd::any> initData, AnimationParameters params)
    {
        if (!subtask.Initialize(initData.first, initData.second, params.m_animationProperties))
        {
            AZStd::string entityName;
            if (m_entityId.IsValid())
            {
                AZ::ComponentApplicationBus::BroadcastResult(entityName, &AZ::ComponentApplicationRequests::GetEntityName, m_entityId);
            }
            else
            {
                entityName = "InvalidEntity";
            }
            //AZ_Warning("ScriptedEntityTweenerTask", false, "ScriptedEntityTweenerTask::AddAnimation - Error starting set animation entity name [%s], [%s, %s]", entityName.c_str(), initData.first.m_componentName.c_str(), initData.first.m_virtualPropertyName.c_str());
            return false;
        }
        return true;
    }

    void ScriptedEntityTweenerTask::ExecuteCallbacks(const AZStd::set<CallbackData>& callbacks)
    {
        for (const auto& callback : callbacks)
        {
            switch (callback.m_callbackType)
            {
            case CallbackTypes::OnComplete:
                ScriptedEntityTweenerNotificationsBus::Broadcast(&ScriptedEntityTweenerNotificationsBus::Events::OnComplete, callback.m_callbackId);
                break;
            case CallbackTypes::OnUpdate:
                ScriptedEntityTweenerNotificationsBus::Broadcast(&ScriptedEntityTweenerNotificationsBus::Events::OnUpdate, callback.m_callbackId, callback.m_callbackData, callback.m_progressPercent);
                break;
            case CallbackTypes::OnLoop:
                ScriptedEntityTweenerNotificationsBus::Broadcast(&ScriptedEntityTweenerNotificationsBus::Events::OnLoop, callback.m_callbackId);
                break;
            case CallbackTypes::RemoveCallback:
                ScriptedEntityTweenerNotificationsBus::Broadcast(&ScriptedEntityTweenerNotificationsBus::Events::RemoveCallback, callback.m_callbackId);
                break;
            }
        }
    }

    void ScriptedEntityTweenerTask::ClearCallbacks(const AnimationProperties& animationProperties)
    {
        AZStd::vector<int> callbackIds = { animationProperties.m_onCompleteCallbackId, animationProperties.m_onLoopCallbackId, animationProperties.m_onUpdateCallbackId };

        for (const auto& callbackId : callbackIds)
        {
            if (callbackId != AnimationProperties::InvalidCallbackId)
            {
                ScriptedEntityTweenerNotificationsBus::Broadcast(&ScriptedEntityTweenerNotificationsBus::Events::RemoveCallback, callbackId);
            }
        }
    }
}
