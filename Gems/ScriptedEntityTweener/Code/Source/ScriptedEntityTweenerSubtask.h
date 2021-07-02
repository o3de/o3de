/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Quaternion.h>
#include <AzCore/std/any.h>
#include <AzCore/std/containers/set.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <ScriptedEntityTweener/ScriptedEntityTweenerEnums.h>

namespace ScriptedEntityTweener
{
    ////////////////////////////////////////////////////////////////////////////////////////////////////
    //! Each subtask performs operations on a virtual address.
    class ScriptedEntityTweenerSubtask
    {
    public:
        ScriptedEntityTweenerSubtask(const AZ::EntityId& entityId)
            : m_entityId(entityId)
        {
            Reset();
        }

        bool Initialize(const AnimationParameterAddressData& animParamData, const AZStd::any& targetValue, const AnimationProperties& properties);
        
        //! Update virtual property based on animation properties, fill out callbacks vector with any callback information that needs to be called this update.
        void Update(float deltaTime, AZStd::set<CallbackData>& callbacks);

        //! True if active and animating a virtual property
        bool IsActive() { return m_isActive; }

        void SetPaused(int timelineId, bool isPaused)
        {
            if (m_animationProperties.m_timelineId == timelineId)
            {
                m_isPaused = isPaused;
            }
        }
        void SetPlayDirectionReversed(int timelineId, bool isPlayingBackward)
        {
            if (m_animationProperties.m_timelineId == timelineId)
            {
                m_animationProperties.m_isPlayingBackward = isPlayingBackward;
            }
        }
        void SetSpeed(int timelineId, float speed)
        {
            if (m_animationProperties.m_timelineId == timelineId)
            {
                m_animationProperties.m_playbackSpeedMultiplier = speed;
            }
        }

        void SetInitialValue(const AZ::Uuid& animationId, const AZStd::any& initialValue)
        {
            if (m_animationProperties.m_animationId == animationId)
            {
                GetValueFromAny(m_valueInitial, initialValue);
            }
        }

        const int GetTimelineId() const
        {
            return m_animationProperties.m_timelineId;
        }

        const AnimationProperties& GetAnimationProperties() const
        {
            return m_animationProperties;
        }

        bool GetVirtualPropertyValue(AZStd::any& returnVal, const AnimationParameterAddressData& animParamData);

    private:
        //! Animated value class that abstracts the supported types
        struct EntityAnimatedValue
        {
        public:
            EntityAnimatedValue()
                : floatVal(AnimationProperties::UninitializedParamFloat), vectorVal(AZ::Vector3::CreateZero()), quatVal(AZ::Quaternion::CreateIdentity())
            {

            }
            void GetValue(float& outVal) const
            {
                outVal = floatVal;
            }
            void GetValue(AZ::Vector3& outVal) const
            {
                outVal = vectorVal;
            }
            void GetValue(AZ::Quaternion& outVal) const
            {
                outVal = quatVal;
            }

            void SetValue(float val)
            {
                floatVal = val;
            }
            void SetValue(AZ::Vector3 val)
            {
                vectorVal = val;
            }
            void SetValue(AZ::Quaternion val)
            {
                quatVal = val;
            }
        private:
            float floatVal;
            AZ::Vector3 vectorVal;
            AZ::Quaternion quatVal;
        };

        AnimationProperties m_animationProperties;

        // The entity being modified
        AZ::EntityId m_entityId;

        // The component and property name to be modified. This is only used for displaying warning messages
        AnimationParameterAddressData m_animParamData;

        // Cached virtual property
        AZ::BehaviorEBus::VirtualProperty* m_virtualProperty;

        // Type of the virtual property
        AZ::Uuid m_virtualPropertyTypeId;

        bool m_isActive;
        bool m_isPaused;
        float m_timeSinceStart;
        int m_timesPlayed;

        EntityAnimatedValue m_valueInitial;
        EntityAnimatedValue m_valueTarget;

        void Reset()
        {
            m_isActive = false;
            m_isPaused = false;
            m_timeSinceStart = 0.0f;

            m_valueInitial = EntityAnimatedValue();
            m_valueTarget = EntityAnimatedValue();

            m_timesPlayed = 0;

            m_animationProperties.Reset();
            m_animParamData = AnimationParameterAddressData();
            m_virtualPropertyTypeId = AZ::Uuid::CreateNull();
            m_virtualProperty = nullptr;
        }

        //! Cache the virtual property to be animated
        bool CacheVirtualProperty(const AnimationParameterAddressData& animParamData);

        //! Return whether the virtual property has been cached
        bool IsVirtualPropertyCached();

        //! Set value from an AZStd::any object
        bool GetValueFromAny(EntityAnimatedValue& value, const AZStd::any& anyValue);

        //! Set value to an AZStd::any object
        bool GetValueAsAny(AZStd::any& anyValue, const EntityAnimatedValue& value);

        //! Get value from the virtual address's value
        bool GetVirtualValue(EntityAnimatedValue& animatedValue);

        //! Set the virtual address's value
        bool SetVirtualValue(const EntityAnimatedValue& value);
    };
}
