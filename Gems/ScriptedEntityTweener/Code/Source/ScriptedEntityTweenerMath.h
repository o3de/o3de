/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Math/MathUtils.h>

namespace ScriptedEntityTweener
{
    ////////////////////////////////////////////////////////////////////////////////////////////////////
    //! A collection of common easing/tweening equations.
    class EasingEquations
    {
    public:
        template <typename T>
        static T GetEasingResult(EasingMethod easeMethod, EasingType easeType, float timeActive, float duration, T valueInitial, T valueTarget)
        {
            switch (easeMethod)
            {
            case EasingMethod::Linear:
                return valueInitial + (valueTarget - valueInitial) * (timeActive / duration);
            case EasingMethod::Quad:
                switch (easeType)
                {
                case EasingType::In:
                    return GetEasingResultCombinedIn(static_cast<float>(easeMethod), timeActive, duration, valueInitial, valueTarget);
                case EasingType::InOut:
                    return GetEasingResultInOutQuad(timeActive, duration, valueInitial, valueTarget);
                default: //EasingType::Out:
                    return GetEasingResultOutQuad(timeActive, duration, valueInitial, valueTarget);
                }
                break;
            case EasingMethod::Cubic:
                switch (easeType)
                {
                case EasingType::In:
                    return GetEasingResultCombinedIn(static_cast<float>(easeMethod), timeActive, duration, valueInitial, valueTarget);
                case EasingType::InOut:
                    return GetEasingResultInOutCubic(timeActive, duration, valueInitial, valueTarget);
                default: //EasingType::Out:
                    return GetEasingResultOutCubic(timeActive, duration, valueInitial, valueTarget);
                }
                break;
            case EasingMethod::Quart:
                switch (easeType)
                {
                case EasingType::In:
                    return GetEasingResultCombinedIn(static_cast<float>(easeMethod), timeActive, duration, valueInitial, valueTarget);
                case EasingType::InOut:
                    return GetEasingResultInOutQuart(timeActive, duration, valueInitial, valueTarget);
                default: //EasingType::Out:
                    return GetEasingResultOutQuart(timeActive, duration, valueInitial, valueTarget);
                }
                break;
            case EasingMethod::Quint:
                switch (easeType)
                {
                case EasingType::In:
                    return GetEasingResultCombinedIn(static_cast<float>(easeMethod), timeActive, duration, valueInitial, valueTarget);
                case EasingType::InOut:
                    return GetEasingResultInOutQuint(timeActive, duration, valueInitial, valueTarget);
                default: //EasingType::Out:
                    return GetEasingResultOutQuint(timeActive, duration, valueInitial, valueTarget);
                }
                break;
            case EasingMethod::Sine:
                switch (easeType)
                {
                case EasingType::In:
                    return GetEasingResultInSine(timeActive, duration, valueInitial, valueTarget);
                case EasingType::InOut:
                    return GetEasingResultInOutSine(timeActive, duration, valueInitial, valueTarget);
                default: //EasingType::Out:
                    return GetEasingResultOutSine(timeActive, duration, valueInitial, valueTarget);
                }
                break;
            case EasingMethod::Expo:
                switch (easeType)
                {
                case EasingType::In:
                    return GetEasingResultInExpo(timeActive, duration, valueInitial, valueTarget);
                case EasingType::InOut:
                    return GetEasingResultInOutExpo(timeActive, duration, valueInitial, valueTarget);
                default: //EasingType::Out:
                    return GetEasingResultOutExpo(timeActive, duration, valueInitial, valueTarget);
                }
                break;
            case EasingMethod::Circ:
                switch (easeType)
                {
                case EasingType::In:
                    return GetEasingResultInCirc(timeActive, duration, valueInitial, valueTarget);
                case EasingType::InOut:
                    return GetEasingResultInOutCirc(timeActive, duration, valueInitial, valueTarget);
                default: //EasingType::Out:
                    return GetEasingResultOutCirc(timeActive, duration, valueInitial, valueTarget);
                }
                break;
            case EasingMethod::Elastic:
                switch (easeType)
                {
                case EasingType::In:
                    return GetEasingResultInElastic(timeActive, duration, valueInitial, valueTarget);
                case EasingType::InOut:
                    return GetEasingResultInOutElastic(timeActive, duration, valueInitial, valueTarget);
                default: //EasingType::Out:
                    return GetEasingResultOutElastic(timeActive, duration, valueInitial, valueTarget);
                }
                break;
            case EasingMethod::Back:
                switch (easeType)
                {
                case EasingType::In:
                    return GetEasingResultInBack(timeActive, duration, valueInitial, valueTarget);
                case EasingType::InOut:
                    return GetEasingResultInOutBack(timeActive, duration, valueInitial, valueTarget);
                default: //EasingType::Out:
                    return GetEasingResultOutBack(timeActive, duration, valueInitial, valueTarget);
                }
                break;
            case EasingMethod::Bounce:
                switch (easeType)
                {
                case EasingType::In:
                    return GetEasingResultInBounce(timeActive, duration, valueInitial, valueTarget);
                case EasingType::InOut:
                    return GetEasingResultInOutBounce(timeActive, duration, valueInitial, valueTarget);
                default: //EasingType::Out:
                    return GetEasingResultOutBounce(timeActive, duration, valueInitial, valueTarget);
                }
                break;
            }
            AZ_Warning("ScriptedEntityTweener", false, "ScriptedEntityTweenerMath::GetEasingResult - Trying to animate with an invalid easing function [%i, %i]", static_cast<int>(easeMethod), static_cast<int>(easeType));
            return valueInitial;
        }

        //!Helper method to get EaseIn results for Quad, Cubic, Quart, and Quint since they all follow the same equation besides from how many times to multiply progressPercent.
        template <typename T>
        static T GetEasingResultCombinedIn(float expo, float timeActive, float duration, T valueInitial, T valueTarget)
        {
            float progressPercent = timeActive / duration;
            return valueInitial + ((valueTarget - valueInitial) * pow(progressPercent, 1.0f + expo));
        }

        template <typename T>
        static T GetEasingResultInSine(float timeActive, float duration, T valueInitial, T valueTarget)
        {
            return -(valueTarget - valueInitial) * cos(timeActive / duration * AZ::Constants::HalfPi) + (valueTarget - valueInitial) + valueInitial;
        }

        template <typename T>
        static T GetEasingResultInExpo(float timeActive, float duration, T valueInitial, T valueTarget)
        {
            return (valueTarget - valueInitial) * pow(2.0f, 10.0f * (timeActive / duration - 1.0f)) + valueInitial;
        }

        template <typename T>
        static T GetEasingResultInCirc(float timeActive, float duration, T valueInitial, T valueTarget)
        {
            timeActive = timeActive / duration;
            return -(valueTarget - valueInitial) * (sqrt(1.0f - timeActive * timeActive) - 1.0f) + valueInitial;
        }

        template <typename T>
        static T GetEasingResultInElastic(float timeActive, float duration, T valueInitial, T valueTarget)
        {
            float progressPercent = timeActive / duration;
            if (progressPercent == 0.0f)
            {
                return valueInitial;
            }
            if (progressPercent == 1.0f)
            {
                return valueTarget;
            }
            T position = (valueTarget - valueInitial) * pow(2.0f, 10.0f * (progressPercent -= 1.0f));
            float elasticAmplitude = 0.3f / 4.0f;
            /*
            if (amplitudeOverride != 0.0f)
            {
                elasticAmplitude = amplitudeOverride * elasticAmplitude;
            }
            */
            return valueInitial - (position * sin((progressPercent - elasticAmplitude) * 2.0f * AZ::Constants::Pi / 0.3f));
        }

        template <typename T>
        static T GetEasingResultInBack(float timeActive, float duration, T valueInitial, T valueTarget)
        {
            float backAmplitude = 1.7337f;
            float progressPercent = timeActive / duration;
            /*if (amplitudeOverride != 0.0f)
            {
                backAmplitude = amplitudeOverride;
            }
            */
            return (valueTarget - valueInitial) * progressPercent * progressPercent * ((backAmplitude + 1.0f) * progressPercent - backAmplitude) + valueInitial;
        }

        template <typename T>
        static T GetEasingResultInBounce(float timeActive, float duration, T valueInitial, T valueTarget)
        {
            const float bounceFullAmplitude = 7.1337f;
            float progressPercent = timeActive / duration;
            float bounceAmplitudeModifier = 0.0f;
            if (progressPercent < (1.0f / 2.75f))
            {
                bounceAmplitudeModifier = 0.0f;
            }
            else if (progressPercent < (2.0f / 2.75f))
            {
                progressPercent -= (1.5f / 2.75f);
                bounceAmplitudeModifier = 0.75f;
            }
            else if (progressPercent < (2.5f / 2.75f))
            {
                progressPercent -= (2.25f / 2.75f);
                bounceAmplitudeModifier = 0.9375f;
            }
            else
            {
                progressPercent -= (2.625f / 2.75f);
                bounceAmplitudeModifier = 0.984375f;
            }
            return valueInitial + (valueTarget - valueInitial) * (bounceFullAmplitude * progressPercent * progressPercent + bounceAmplitudeModifier);
        }

        //EASE OUT VARIANTS
        template <typename T>
        static T GetEasingResultOutQuad(float timeActive, float duration, T valueInitial, T valueTarget)
        {
            timeActive = timeActive / duration;
            return -(valueTarget - valueInitial) * timeActive * (timeActive - 2.0f) + valueInitial;
        }

        template <typename T>
        static T GetEasingResultOutCubic(float timeActive, float duration, T valueInitial, T valueTarget)
        {
            timeActive = timeActive / duration;
            timeActive--;
            return (valueTarget - valueInitial) * (timeActive * timeActive * timeActive + 1.0f) + valueInitial;
        }

        template <typename T>
        static T GetEasingResultOutQuart(float timeActive, float duration, T valueInitial, T valueTarget)
        {
            timeActive = timeActive / duration;
            timeActive--;
            return -(valueTarget - valueInitial) * (pow(timeActive, 4) - 1.0f) + valueInitial;
        }

        template <typename T>
        static T GetEasingResultOutQuint(float timeActive, float duration, T valueInitial, T valueTarget)
        {
            timeActive = timeActive / duration;
            timeActive--;
            return (valueTarget - valueInitial) * (pow(timeActive, 5) + 1.0f) + valueInitial;
        }

        template <typename T>
        static T GetEasingResultOutSine(float timeActive, float duration, T valueInitial, T valueTarget)
        {
            return (valueTarget - valueInitial) * sin(timeActive / duration * AZ::Constants::HalfPi) + valueInitial;
        }

        template <typename T>
        static T GetEasingResultOutExpo(float timeActive, float duration, T valueInitial, T valueTarget)
        {
            return (valueTarget - valueInitial) * (-pow(2.0f, -10.0f * timeActive / duration) + 1.0f) + valueInitial;
        }

        template <typename T>
        static T GetEasingResultOutCirc(float timeActive, float duration, T valueInitial, T valueTarget)
        {
            timeActive = timeActive / duration;
            timeActive--;
            return (valueTarget - valueInitial) * sqrt(1.0f - timeActive * timeActive) + valueInitial;
        }

        template <typename T>
        static T GetEasingResultOutElastic(float timeActive, float duration, T valueInitial, T valueTarget)
        {
            float progressPercent = timeActive / duration;
            if (progressPercent == 0.0f)
            {
                return valueInitial;
            }
            if (progressPercent == 1.0f)
            {
                return valueTarget;
            }
            T distance = valueTarget - valueInitial;
            T positionFix = distance * pow(2.0f, -10.0f * progressPercent);
            float constant = 0.3f / 4.0f;
            /*
            if (amplitudeOverride != -1)
                constant = amplitudeOverride*constant;
            */
            return positionFix * sin((progressPercent - constant) * 2.0f * AZ::Constants::Pi / 0.3f) + valueTarget;
        }

        template <typename T>
        static T GetEasingResultOutBack(float timeActive, float duration, T valueInitial, T valueTarget)
        {
            float progressPercent = timeActive / duration;
            float constant = 1.7337f;
            /*
            if (amplitudeOverride != -1)
                constant = amplitudeOverride;
            */
            progressPercent--;
            return (valueTarget - valueInitial) * (progressPercent * progressPercent * ((constant + 1.0f) * progressPercent + constant) + 1.0f) + valueInitial;
        }

        template <typename T>
        static T GetEasingResultOutBounce(float timeActive, float duration, T valueInitial, T valueTarget)
        {
            float progressPercent = timeActive / duration;
            if (progressPercent < (1.0f / 2.75f))
            {
                return (valueTarget - valueInitial) * (7.5625f * progressPercent * progressPercent) + valueInitial;
            }
            else if (progressPercent < (2.0f / 2.75f))
            {
                progressPercent -= (1.5f / 2.75f);
                return (valueTarget - valueInitial) * (7.5625f * progressPercent * progressPercent + 0.75f) + valueInitial;
            }
            else if (progressPercent < (2.5f / 2.75f))
            {
                progressPercent -= (2.25f / 2.75f);
                return (valueTarget - valueInitial) * (7.5625f * progressPercent * progressPercent + 0.9375f) + valueInitial;
            }
            else
            {
                progressPercent -= (2.625f / 2.75f);
                return (valueTarget - valueInitial) * (7.5625f * progressPercent * progressPercent + 0.984375f) + valueInitial;
            }
        }

        //EASE IN OUT VARIANTS
        template <typename T>
        static T GetEasingResultInOutQuad(float timeActive, float duration, T valueInitial, T valueTarget)
        {
            timeActive /= duration / 2.0f;
            if (timeActive < 1.0f)
            {
                return (valueTarget - valueInitial) / 2.0f * timeActive * timeActive + valueInitial;
            }
            timeActive--;
            return -(valueTarget - valueInitial) / 2.0f * (timeActive * (timeActive - 2.0f) - 1) + valueInitial;
        }

        template <typename T>
        static T GetEasingResultInOutCubic(float timeActive, float duration, T valueInitial, T valueTarget)
        {
            timeActive /= duration / 2.0f;
            if (timeActive < 1.0f)
            {
                return (valueTarget - valueInitial) / 2.0f * timeActive * timeActive * timeActive + valueInitial;
            }
            timeActive -= 2.0f;
            return (valueTarget - valueInitial) / 2.0f * (timeActive * timeActive * timeActive + 2.0f) + valueInitial;
        }

        template <typename T>
        static T GetEasingResultInOutQuart(float timeActive, float duration, T valueInitial, T valueTarget)
        {
            timeActive /= duration / 2.0f;
            if (timeActive < 1.0f)
            {
                return (valueTarget - valueInitial) / 2.0f * timeActive * timeActive * timeActive * timeActive + valueInitial;
            }
            timeActive -= 2.0f;
            return -(valueTarget - valueInitial) / 2.0f * (timeActive * timeActive * timeActive * timeActive - 2.0f) + valueInitial;
        }

        template <typename T>
        static T GetEasingResultInOutQuint(float timeActive, float duration, T valueInitial, T valueTarget)
        {
            timeActive /= duration / 2.0f;
            if (timeActive < 1.0f)
            {
                return (valueTarget - valueInitial) / 2.0f * timeActive * timeActive * timeActive * timeActive * timeActive + valueInitial;
            }
            timeActive -= 2.0f;
            return (valueTarget - valueInitial) / 2.0f * (timeActive * timeActive * timeActive * timeActive * timeActive + 2.0f) + valueInitial;
        }

        template <typename T>
        static T GetEasingResultInOutSine(float timeActive, float duration, T valueInitial, T valueTarget)
        {
            return -(valueTarget - valueInitial) / 2.0f * (cos(AZ::Constants::Pi * timeActive / duration) - 1.0f) + valueInitial;
        }

        template <typename T>
        static T GetEasingResultInOutExpo(float timeActive, float duration, T valueInitial, T valueTarget)
        {
            timeActive /= duration / 2.0f;
            if (timeActive < 1.0f)
            {
                return (valueTarget - valueInitial) / 2.0f * pow(2.0f, 10.0f * (timeActive - 1.0f)) + valueInitial;
            }
            timeActive--;
            return (valueTarget - valueInitial) / 2.0f * (-pow(2.0f, -10.0f * timeActive) + 2.0f) + valueInitial;
        }

        template <typename T>
        static T GetEasingResultInOutCirc(float timeActive, float duration, T valueInitial, T valueTarget)
        {
            timeActive /= duration / 2.0f;
            if (timeActive < 1.0f)
            {
                return -(valueTarget - valueInitial) / 2.0f * (sqrt(1.0f - timeActive * timeActive) - 1.0f) + valueInitial;
            }
            timeActive -= 2.0f;
            return (valueTarget - valueInitial) / 2.0f * (sqrt(1.0f - timeActive * timeActive) - 1.0f) + valueInitial;
        }

        template <typename T>
        static T GetEasingResultInOutElastic(float timeActive, float duration, T valueInitial, T valueTarget)
        {
            float progressPercent = timeActive / duration;
            progressPercent *= 2.0f;
            if (progressPercent == 0.0f)
            {
                return valueInitial;
            }
            if (progressPercent == 2.0f)
            {
                return valueTarget;
            }
            T distance = valueTarget - valueInitial;
            float constant = .3f * 1.5f;
            /*
            if (amplitudeOverride != -1)
                constant = amplitudeOverride*constant;
            */
            if (progressPercent < 1.0f)
            {
                T positionFix = distance * pow(2.0f, 10.0f * (progressPercent -= 1.0f));
                return (positionFix * sin((progressPercent - (constant / 4.0f)) * 2.0f * AZ::Constants::Pi / constant)) * (-0.5f) + valueInitial;
            }
            else
            {
                T positionFix = distance * pow(2.0f, -10.0f * (progressPercent -= 1.0f));
                return positionFix * sin((progressPercent - constant / 4.0f) * 2.0f * AZ::Constants::Pi / constant) * 0.5f + valueTarget;
            }
        }

        template <typename T>
        static T GetEasingResultInOutBack(float timeActive, float duration, T valueInitial, T valueTarget)
        {
            float progressPercent = timeActive / duration;
            float constant = 1.7337f;
            /*
            if (amplitudeOverride != -1)
                constant = amplitudeOverride;
                */
            progressPercent *= 2.0f;
            if (progressPercent < 1.0f)
            {
                constant = constant * 1.525f;
                return (valueTarget - valueInitial) / 2.0f * (progressPercent * progressPercent * ((constant + 1.0f) * progressPercent - constant)) + valueInitial;
            }
            progressPercent -= 2.0f;
            constant = constant * 1.525f;
            return (valueTarget - valueInitial) / 2.0f * (progressPercent * progressPercent * ((constant + 1.0f) * progressPercent + constant) + 2.0f) + valueInitial;
        }

        template <typename T>
        static T GetEasingResultInOutBounce(float timeActive, float duration, T valueInitial, T valueTarget)
        {
            float progressPercent = timeActive / duration;
            T zero = valueInitial - valueInitial;
            if (progressPercent < 0.5f)
            {
                return GetEasingResultInBounce(timeActive, duration / 2.0f, zero, (valueTarget - valueInitial)) * 0.5f + valueInitial;
            }
            else
            {
                return GetEasingResultOutBounce(timeActive - (duration / 2.0f), duration / 2.0f, zero, (valueTarget - valueInitial)) * 0.5f + (valueTarget - valueInitial) * 0.5f + valueInitial;
            }
        }
    };
}
