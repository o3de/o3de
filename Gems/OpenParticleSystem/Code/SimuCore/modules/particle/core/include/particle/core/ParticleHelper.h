/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "particle/core/ParticleCurve.h"
#include "particle/core/ParticleRandom.h"

namespace SimuCore::ParticleCore {
    template<typename Func>
    inline void ForEach(Particle* particleBuffer, AZ::u32 begin, AZ::u32 alive, Func&& fn)
    {
        for (AZ::u32 i = begin; i < alive; ++i) {
            Particle& particle = particleBuffer[i];
            fn(particle);
        }
    }

    inline Facing SpriteVariantKeyGetFacing(const VariantKey& key)
    {
        AZ::u64 value = key.value & 0xff;
        return static_cast<Facing>(value);
    }

    inline void SpriteVariantKeySetFacing(VariantKey& key, Facing facing)
    {
        key.value = static_cast<AZ::u64>(facing) & 0xff;
    }

    template<typename T, AZ::u32 size>
    inline void UpdateDistributionPtr(ValueObject<T, size>& valueObject, const Distribution& distribution)
    {
        for (AZ::u32 index = 0; index < size; ++index) {
            if (valueObject.distType != DistributionType::CONSTANT && valueObject.distIndex[index] != 0 &&
                valueObject.distIndex[index] <= distribution.at(valueObject.distType).size()) {
                valueObject.distributions[index] =
                    distribution.at(valueObject.distType)[valueObject.distIndex[index] - 1];
            }
        }
    }

    inline float CalcDistributionTickValue(const ValueObjFloat& valueObject, const BaseInfo& info)
    {
        if (valueObject.distType != DistributionType::CONSTANT) {
            return valueObject.distributions.front()->Tick(info);
        }
        return valueObject.dataValue;
    }

    inline float CalcDistributionTickValue(const ValueObjFloat& valueObject,
        const BaseInfo& info, const Particle& particle)
    {
        if (valueObject.distType != DistributionType::CONSTANT) {
            return valueObject.distributions.front()->Tick(info, particle);
        }
        return valueObject.dataValue;
    }

    template<typename T, AZ::u32 size>
    inline T CalcDistributionTickValue(const ValueObject<T, size>& valueObject, const BaseInfo& info)
    {
        T updateValue = valueObject.dataValue;
        if (valueObject.isUniform) {
            updateValue = T(valueObject.dataValue[0]);
        }
        if (valueObject.distType != DistributionType::CONSTANT) {
            for (AZ::u32 index = 0; index < size; ++index) {
                if (valueObject.isUniform) {
                    updateValue[index] = valueObject.distributions.at(0)->Tick(info);
                    continue;
                }
                updateValue[index] = valueObject.distributions.at(index)->Tick(info);
            }
        }
        return updateValue;
    }

    template<typename T, AZ::u32 size>
    inline T CalcDistributionTickValue(const ValueObject<T, size>& valueObject,
        const BaseInfo& info, const Particle& particle)
    {
        T updateValue = valueObject.dataValue;
        if (valueObject.isUniform) {
            updateValue = T(valueObject.dataValue.GetElement(0));
        }
        if (valueObject.distType != DistributionType::CONSTANT) {
            auto tickValue = valueObject.distributions.at(0)->Tick(info, particle);
            for (AZ::u32 index = 0; index < size; ++index) {
                if (valueObject.isUniform) {
                    updateValue.SetElement(index, tickValue);
                    continue;
                }
                updateValue.SetElement(index, valueObject.distributions.at(index)->Tick(info, particle));
            }
        }
        return updateValue;
    }

    template<>
    inline LinearValue CalcDistributionTickValue(
        const ValueObjLinear& valueObject, const BaseInfo& info, const Particle& particle)
    {
        LinearValue updateValue = valueObject.dataValue;
        if (valueObject.isUniform) {
            updateValue.value = AZ::Vector3(valueObject.dataValue.value.GetX());
            updateValue.minValue = AZ::Vector3(valueObject.dataValue.minValue.GetX());
            updateValue.maxValue = AZ::Vector3(valueObject.dataValue.maxValue.GetX());
        }
        if (valueObject.distType == DistributionType::CURVE) {
            for (AZ::u32 index = 0; index < DISTRIBUTION_COUNT_THREE; ++index) 
            {
                if (valueObject.isUniform) 
                {
                    updateValue.value.SetElement(index, valueObject.distributions.at(0)->Tick(info, particle));
                    continue;
                }
                updateValue.value.SetElement(index, valueObject.distributions.at(index)->Tick(info, particle));
            }
        }
        return updateValue;
    }
}
