/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/EntityId.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/list.h>
#include <ScriptedEntityTweener/ScriptedEntityTweenerEnums.h>
#include "ScriptedEntityTweenerSubtask.h"


namespace ScriptedEntityTweener
{
    //! One task per entity id, contains a collection of subtasks that are unique per virtual property.
    class ScriptedEntityTweenerTask
    {
    public: // member functions
        ScriptedEntityTweenerTask(AZ::EntityId id);
        ~ScriptedEntityTweenerTask();

        void AddAnimation(const AnimationParameters& params, bool overwriteQueued = true);

        void Update(float deltaTime);

        bool GetIsActive();

        void Stop(int timelineId);

        void SetPaused(const AnimationParameterAddressData& addressData, int timelineId, bool isPaused);

        void SetPlayDirectionReversed(const AnimationParameterAddressData& addressData, int timelineId, bool isPlayingBackward);

        void SetSpeed(const AnimationParameterAddressData& addressData, int timelineId, float speed);

        void SetInitialValue(const AnimationParameterAddressData& addressData, const AZ::Uuid& timelineId, const AZStd::any& initialValue);

        void GetVirtualPropertyValue(AZStd::any& returnVal, const AnimationParameterAddressData& addressData);

        bool operator<(const ScriptedEntityTweenerTask& other) const
        {
            return m_entityId < other.m_entityId;
        }
        bool operator>(const ScriptedEntityTweenerTask& other) const
        {
            return other.m_entityId < m_entityId;
        }
        bool operator==(const ScriptedEntityTweenerTask& other) const
        {
            return m_entityId == other.m_entityId;
        }

    private: // member functions
        AZ::EntityId m_entityId;

        //! Unique(per address data) active subtasks being updated
        AZStd::unordered_map<AnimationParameterAddressData, ScriptedEntityTweenerSubtask> m_subtasks;

        class QueuedSubtaskInfo
        {
        public:
            QueuedSubtaskInfo(AnimationParameters params, float delayTime)
                : m_params(params)
                , m_currentDelayTime(delayTime)
                , m_isPaused(false)
            {
            }

            bool UpdateUntilReady(float deltaTime)
            {
                if (!m_isPaused)
                {
                    m_currentDelayTime -= deltaTime * m_params.m_animationProperties.m_playbackSpeedMultiplier;
                    if (m_currentDelayTime <= .0f)
                    {
                        m_params.m_animationProperties.m_timeToDelayAnim = .0f;
                        return true;
                    }
                }
                return false;
            }

            int GetTimelineId()
            {
                return m_params.m_animationProperties.m_timelineId;
            }

            const AZ::Uuid& GetAnimationId()
            {
                return m_params.m_animationProperties.m_animationId;
            }

            void SetPlaybackSpeed(float speed)
            {
                m_params.m_animationProperties.m_playbackSpeedMultiplier = speed;
            }

            AnimationParameters& GetParameters()
            {
                return m_params;
            }

            void SetPaused(bool isPaused)
            {
                m_isPaused = isPaused;
            }

            void SetInitialValue(const AnimationParameterAddressData& addressData, const AZStd::any initialValue)
            {
                m_initialValues[addressData] = initialValue;
            }

            bool HasInitialValue()
            {
                return !m_initialValues.empty();
            }

            const AZStd::any& GetInitialValue(const AnimationParameterAddressData& addressData)
            {
                auto initialValueEntry = m_initialValues.find(addressData);
                if (initialValueEntry != m_initialValues.end())
                {
                    return initialValueEntry->second;
                }
                return m_emptyInitialValue;
            }

        private:
            QueuedSubtaskInfo();

            float m_currentDelayTime;
            bool m_isPaused;
            
            AZStd::unordered_map<AnimationParameterAddressData, AZStd::any> m_initialValues;

            AnimationParameters m_params;

            static const AZStd::any m_emptyInitialValue;
        };

        //! List of AnimationsParameters that need to be delayed before being added to m_subtasks, possibly overriding an animation.
        AZStd::list<QueuedSubtaskInfo> m_queuedSubtasks;

        AZStd::set<CallbackData> m_callbacks;

        bool IsTimelineIdValid(int timelineId)
        {
            return timelineId != static_cast<int>(AnimationProperties::InvalidTimelineId);
        }

        bool InitializeSubtask(ScriptedEntityTweenerSubtask& subtask, const AZStd::pair<AnimationParameterAddressData, AZStd::any> initData, AnimationParameters params);
        void ExecuteCallbacks(const AZStd::set<CallbackData>& callbacks);
        void ClearCallbacks(const AnimationProperties& animationProperties);
    };
}
