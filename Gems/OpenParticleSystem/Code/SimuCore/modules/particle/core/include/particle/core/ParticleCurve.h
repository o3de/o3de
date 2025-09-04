/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <algorithm>
#include <functional>
#include <AzCore/std/containers/vector.h>
#include "particle/core/ParticleDistribution.h"

namespace SimuCore::ParticleCore {

    class ParticleCurve : public ParticleDistribution {
    public:
        ParticleCurve() = default;
        ~ParticleCurve() = default;

        static std::function<float(const float)> GetInterpFunc(const KeyPointInterpMode& mode);
        static float GetInterpVal(const KeyPointInterpMode& mode, float interTime);

        void SetTickMode(const CurveTickMode& mode);
        const CurveTickMode& GetTickMode() const;
        void SetLeftExtrapMode(const CurveExtrapMode& mode);
        void SetRightExtrapMode(const CurveExtrapMode& mode);
        const CurveExtrapMode& GetExtrapMode() const;
        void SetValueFactor(float factor);
        float GetValueFactor() const;
        void SetTimeFactor(float factor);
        float GetTimeFactor() const;
        void AddKeyPoint(float time, float value, const KeyPointInterpMode& mode);
        void NormalizeKeyPoints();
        void UpdateKeyPointTime(AZ::u32 index, float time);
        void SetKeyPointValue(AZ::u32 index, float value);
        void SetKeyPointInterpMode(AZ::u32 index, const KeyPointInterpMode& mode);
        void DeleteKeyPoint(AZ::u32 index);
        const AZStd::vector<KeyPoint>& GetKeyPoints() const;

        float Tick(const BaseInfo& info) override;
        float Tick(const BaseInfo& info, const Particle& particle) override;

    private:
        float ClampTime(float time) const;
        int FindLeftKeyPoint(float time);
        float CalcCurveValue(float currentTime);
        float CurveTick(float point, float range);

        CurveExtrapMode extrapModeLeft = CurveExtrapMode::CYCLE;
        CurveExtrapMode extrapModeRight = CurveExtrapMode::CYCLE;
        float valueFactor = 1.0f;
        float timeFactor = 1.0f;
        CurveTickMode tickMode = CurveTickMode::EMIT_DURATION;
        AZStd::vector<KeyPoint> keyPoints;
    };
} // SimuCore::ParticleCore
