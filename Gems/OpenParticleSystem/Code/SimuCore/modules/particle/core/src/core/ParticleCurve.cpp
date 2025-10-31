/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "particle/core/ParticleCurve.h"

namespace SimuCore::ParticleCore {
    std::function<float(const float)> ParticleCurve::GetInterpFunc(const KeyPointInterpMode& mode)
    {
        static const AZStd::unordered_map<KeyPointInterpMode, std::function<float(float)>> interpFuncs = {
            {
                KeyPointInterpMode::LINEAR,     [](float t) -> float { return t; }
            },
            {
                KeyPointInterpMode::STEP,       [](float) -> float { return 0; }
            },
            {
                KeyPointInterpMode::CUBIC_IN,   [](float t) -> float { return t * t * t; }
            },
            {
                KeyPointInterpMode::CUBIC_OUT,
                [](float t) -> float { return (t - 1.0f) * (t - 1.0f) * (t - 1.0f) + 1.0f; }
            },
            {
                KeyPointInterpMode::SINE_IN,    [](float t) -> float { return sin((t - 1.0f) * AZ::Constants::HalfPi) + 1.0f; }
            },
            {
                KeyPointInterpMode::SINE_OUT,   [](float t) -> float { return sin(t * AZ::Constants::HalfPi); }
            },
            {
                KeyPointInterpMode::CIRCLE_IN,  [](float t) -> float { return 1.0f - sqrt(1.0f - (t * t)); }
            },
            {
                KeyPointInterpMode::CIRCLE_OUT, [](float t) -> float { return sqrt((2.0f - t) * t); }
            }
        };
        if (interpFuncs.find(mode) == interpFuncs.end()) {
            return interpFuncs.at(KeyPointInterpMode::LINEAR);
        }
        return interpFuncs.at(mode);
    }

    float ParticleCurve::GetInterpVal(const KeyPointInterpMode& mode, float interTime)
    {
        switch (mode) {
            case KeyPointInterpMode::LINEAR: {
                return interTime;
            }
            case KeyPointInterpMode::STEP: {
                return 0;
            }
            case KeyPointInterpMode::CUBIC_IN: {
                return interTime * interTime * interTime;
            }
            case KeyPointInterpMode::CUBIC_OUT: {
                return (interTime - 1.0f) * (interTime - 1.0f) * (interTime - 1.0f) + 1.0f;;
            }
            case KeyPointInterpMode::SINE_IN: {
                return sin((interTime - 1.0f) * AZ::Constants::HalfPi) + 1.0f;;
            }
            case KeyPointInterpMode::SINE_OUT: {
                return sin(interTime * AZ::Constants::HalfPi);;
            }
            case KeyPointInterpMode::CIRCLE_IN: {
                return 1.0f - sqrt(1.0f - (interTime * interTime));
            }
            case KeyPointInterpMode::CIRCLE_OUT: {
                return sqrt((2.0f - interTime) * interTime);
            }
            default: {
                return interTime;
            }
        }
    }

    void ParticleCurve::SetTickMode(const CurveTickMode& mode)
    {
        tickMode = mode;
    }

    const CurveTickMode& ParticleCurve::GetTickMode() const
    {
        return tickMode;
    }

    void ParticleCurve::SetLeftExtrapMode(const CurveExtrapMode& mode)
    {
        extrapModeLeft = mode;
    }

    void ParticleCurve::SetRightExtrapMode(const CurveExtrapMode& mode)
    {
        extrapModeRight = mode;
    }

    const CurveExtrapMode& ParticleCurve::GetExtrapMode() const
    {
        return extrapModeLeft;
    }

    void ParticleCurve::SetValueFactor(float factor)
    {
        valueFactor = factor;
    }

    float ParticleCurve::GetValueFactor() const
    {
        return valueFactor;
    }

    void ParticleCurve::SetTimeFactor(float factor)
    {
        timeFactor = factor;
    }

    float ParticleCurve::GetTimeFactor() const
    {
        return timeFactor;
    }

    void ParticleCurve::AddKeyPoint(float time, float value, const KeyPointInterpMode& mode)
    {
        float clampTime = ClampTime(time);
        float clampValue = AZ::GetClamp(value, 0.0f, 1.0f);
        int leftPoint = FindLeftKeyPoint(clampTime);
        if (leftPoint == -1) {
            keyPoints.insert(keyPoints.begin(), KeyPoint(clampTime, clampValue, mode));
            return;
        }
        if (leftPoint == keyPoints.size() - 1) {
            keyPoints.emplace_back(KeyPoint(clampTime, clampValue, mode));
            return;
        }
        keyPoints.insert(keyPoints.begin() + static_cast<decltype(keyPoints)::difference_type>(leftPoint),
            KeyPoint(clampTime, clampValue, mode));
    }

    void ParticleCurve::NormalizeKeyPoints()
    {
        for (auto& point : keyPoints) {
            point.time = ClampTime(point.time);
            point.value = AZ::GetClamp(point.value, 0.0f, 1.0f);
        }
        std::sort(keyPoints.begin(), keyPoints.end(),
            [](const KeyPoint& left, const KeyPoint& right) {
                return left.time < right.time;
            });
        auto iter = std::unique(keyPoints.begin(), keyPoints.end(),
            [](const KeyPoint& left, const KeyPoint& right) {
                return abs(right.time - left.time) <= FLT_EPSILON;
            });
        keyPoints.erase(iter, keyPoints.end());
    }

    void ParticleCurve::UpdateKeyPointTime(AZ::u32 index, float time)
    {
        if (index < keyPoints.size()) {
            keyPoints[index].time = time;
            std::sort(keyPoints.begin(), keyPoints.end(), [](const KeyPoint& left, const KeyPoint& right) {
                return left.time < right.time;
            });
        }
    }

    void ParticleCurve::SetKeyPointValue(AZ::u32 index, float value)
    {
        if (index >= keyPoints.size()) {
            return;
        }
        keyPoints[index].value = value;
    }

    void ParticleCurve::SetKeyPointInterpMode(AZ::u32 index, const KeyPointInterpMode& mode)
    {
        if (index >= keyPoints.size()) {
            return;
        }
        keyPoints[index].interpMode = mode;
    }

    void ParticleCurve::DeleteKeyPoint(AZ::u32 index)
    {
        if (index >= keyPoints.size()) {
            return;
        }
        keyPoints.erase(keyPoints.begin() + index);
    }

    const AZStd::vector<KeyPoint>& ParticleCurve::GetKeyPoints() const
    {
        return keyPoints;
    }

    float ParticleCurve::Tick(const BaseInfo& info)
    {
        if (tickMode == CurveTickMode::EMIT_DURATION) {
            return CurveTick(info.currentTime, info.duration);
        }
        return CurveTick(0.0f, 1.0f);
    }

    float ParticleCurve::Tick(const BaseInfo& info, const Particle& particle)
    {
        if (tickMode == CurveTickMode::PARTICLE_LIFETIME) {
            return CurveTick(particle.currentLife, particle.lifeTime);
        }
        if (tickMode == CurveTickMode::EMIT_DURATION) {
            return CurveTick(info.currentTime, info.duration);
        }
        return CurveTick(0.0f, 1.0f);
    }

    float ParticleCurve::ClampTime(float time) const
    {
        if (time > 1.0f) {
            if (extrapModeRight == CurveExtrapMode::CONSTANT) {
                return 1.0f;
            }
            if (extrapModeRight == CurveExtrapMode::CYCLE ||
                extrapModeRight == CurveExtrapMode::CYCLE_WITH_OFFSET) {
                return 0.0f;
            }
        }

        if (time < 0.0f) {
            if (extrapModeRight == CurveExtrapMode::CONSTANT) {
                return 0.0f;
            }
            if (extrapModeRight == CurveExtrapMode::CYCLE ||
                extrapModeRight == CurveExtrapMode::CYCLE_WITH_OFFSET) {
                return 1.0f - fmodf(-time, 1.0f);
            }
        }
        return time;
    }

    int ParticleCurve::FindLeftKeyPoint(float time)
    {
        if (keyPoints.empty() || time < keyPoints.front().time) {
            return -1;
        }
        for (size_t iter = 0; iter < keyPoints.size() - 1; ++iter) {
            if (time >= keyPoints[iter].time && time < keyPoints[iter + 1].time) {
                return static_cast<int>(iter);
            }
        }
        return static_cast<int>(keyPoints.size() - 1);
    }

    float ParticleCurve::CalcCurveValue(float currentTime)
    {
        if (keyPoints.empty()) {
            return 0.0f;
        }
        int left = FindLeftKeyPoint(currentTime);
        if (left == keyPoints.size() - 1) {
            return keyPoints.back().value;
        }
        if (left <= -1) {
            return keyPoints.front().value;
        }
        float interTime = (currentTime - keyPoints[left].time) /
            (keyPoints[left + 1].time - keyPoints[left].time);
        float valueRange = keyPoints[left + 1].value - keyPoints[left].value;
        return GetInterpVal(keyPoints[left + 1].interpMode, interTime) * valueRange + keyPoints[left].value;
    }

    float ParticleCurve::CurveTick(float point, float range)
    {
        if (range > AZ::Constants::FloatEpsilon) {
            float clampTime = ClampTime(point / range);
            auto value = CalcCurveValue(clampTime);
            return value * valueFactor;
        } else {
            return keyPoints.empty() ? valueFactor : keyPoints.front().value * valueFactor;
        }
    }
} // SimuCore::ParticleCore