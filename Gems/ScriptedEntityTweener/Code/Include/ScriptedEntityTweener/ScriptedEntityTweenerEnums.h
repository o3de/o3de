/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/any.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzFramework/Math/Easing.h>

namespace ScriptedEntityTweener
{
    struct AnimationParameterAddressData
    {
        AZStd::string m_componentName;
        AZStd::string m_virtualPropertyName;

        AnimationParameterAddressData() {}

        AnimationParameterAddressData(const AZStd::string& componentName, const AZStd::string virtualPropertyName)
            : m_componentName(componentName), m_virtualPropertyName(virtualPropertyName)
        {

        }

        bool operator==(const AnimationParameterAddressData& other) const
        {
            return m_componentName == other.m_componentName &&
                m_virtualPropertyName == other.m_virtualPropertyName;
        }
    };
}

//! Implementation required to store custom types in unordered_map
namespace AZStd
{
    template<>
    struct hash<ScriptedEntityTweener::AnimationParameterAddressData>
    {
        using argument_type = ScriptedEntityTweener::AnimationParameterAddressData;
        using result_type = AZStd::size_t;
        AZ_FORCE_INLINE result_type operator()(const argument_type& ref) const
        {
            return (AZStd::hash<AZStd::string>()(ref.m_componentName)
                ^ (AZStd::hash<AZStd::string>()(ref.m_virtualPropertyName) << 1) >> 1);
        }
    };
}

namespace ScriptedEntityTweener
{
    //Generally, animations created to animate n number of parameters a certain way, AnimationProperties defines what exactly is applied
    struct AnimationProperties
    {
        static const float UninitializedParamFloat;
        static const int InvalidCallbackId;
        static const unsigned int InvalidTimelineId;

        AzFramework::EasingMethod m_easeMethod;
        AzFramework::EasingType m_easeType;

        float m_timeIntoAnimation;
        
        float m_timeToDelayAnim;
        float m_timeDuration;

        float m_amplitudeOverride;

        bool m_isFrom;
        bool m_isPlayingBackward;

        int m_timesToPlay;

        float m_playbackSpeedMultiplier;

        AZ::Uuid m_animationId;
        int m_timelineId;
        int m_onCompleteCallbackId;
        int m_onUpdateCallbackId;
        int m_onLoopCallbackId;


        AnimationProperties()
        {
            Reset();
        }

        void Reset()
        {
            m_easeMethod = AzFramework::EasingMethod::None;
            m_easeType = AzFramework::EasingType::In;

            m_timeToDelayAnim = 0.0f;
            m_timeDuration = 0.0f;

            m_amplitudeOverride = 0.0f;
            
            m_isFrom = false;
            m_isPlayingBackward = false;

            m_timesToPlay = 1;

            m_playbackSpeedMultiplier = 1.0f;

            m_animationId = AZ::Uuid::CreateNull();
            m_timelineId = InvalidTimelineId;
            m_onCompleteCallbackId = InvalidCallbackId;
            m_onUpdateCallbackId = InvalidCallbackId;
            m_onLoopCallbackId = InvalidCallbackId;
        }
    };

    struct AnimationParameters
    {
        AZ_TYPE_INFO(AnimationParameters, "{7E375768-746E-48DC-BEF4-6F40FEB534F9}");
        AZ_CLASS_ALLOCATOR(AnimationParameters, AZ::SystemAllocator);

        AnimationParameters()
        {
            Reset();
        }

        void Reset()
        {
            m_animationProperties.Reset();
            m_animationParameters.clear();
        }

        AnimationProperties m_animationProperties;

        AZStd::unordered_map<AnimationParameterAddressData, AZStd::any> m_animationParameters;
    };

    enum class CallbackTypes
    {
        OnComplete,
        OnUpdate,
        OnLoop,
        RemoveCallback
    };

    struct CallbackData
    {
        CallbackData(CallbackTypes callbackType, int callbackId)
            : m_callbackType(callbackType)
            , m_callbackId(callbackId)
            , m_progressPercent(0)
        {

        }

        CallbackTypes m_callbackType = CallbackTypes::OnComplete;
        int m_callbackId = AnimationProperties::InvalidCallbackId;
        AZStd::any m_callbackData;
        float m_progressPercent;

        //! Comparison operators implemented to store this data type in a set
        bool operator<(const CallbackData& other) const
        {
            return m_callbackId < other.m_callbackId;
        }
        bool operator>(const CallbackData& other) const
        {
            return other.m_callbackId < m_callbackId;
        }
        bool operator==(const CallbackData& other) const
        {
            return m_callbackId == other.m_callbackId;
        }
    };
}

