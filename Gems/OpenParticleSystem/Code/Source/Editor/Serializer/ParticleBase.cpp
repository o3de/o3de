/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Serializer/ParticleBase.h>
#include <Serializer/ParticleSourceData.h>
#include "core/math/Constants.h"

namespace OpenParticle
{
    template<typename T>
    void Clamp(T& val, const T min, const T max)
    {
        val = val.GetMax(min).GetMin(max);
    }

    void Clamp(AZ::Color& val, const AZ::Color& min, const AZ::Color& max)
    {
        if (val.IsLessThan(min))
        {
            val = min;
        }

        if (val.IsGreaterThan(max))
        {
            val = max;
        }
    }

    void PreWarm::CheckParam()
    {
        warmupTime = AZStd::max(warmupTime, 0.f);
        tickCount = AZStd::max<AZ::u32>(tickCount, 0u);
        tickDelta = AZStd::max(tickDelta, 0.f);
        if (tickDelta > AZ::Constants::FloatEpsilon) {
            tickCount = static_cast<AZ::u32>(warmupTime / tickDelta);
            warmupTime = tickCount * tickDelta;
        }
    }

    void EmitterConfig::CheckParam()
    {
        maxSize = AZStd::clamp<AZ::u32>(maxSize, 1u, 1000000u);
        startTime = AZStd::max(startTime, 0.f);
        duration = AZStd::max(duration, 0.05f);
    }

    void SpriteConfig::CheckParam()
    {
        sortId = AZStd::max<AZ::u32>(sortId, 0u);
        subImageSize = subImageSize.GetMax(AZ::Vector2(1.0f, 1.0f));
    }

    void MeshConfig::CheckParam()
    {
        sortId = AZStd::max<AZ::u32>(sortId, 0u);
    }

    void RibbonConfig::CheckParam()
    {
        sortId = AZStd::max<AZ::u32>(sortId, 0u);
        minRibbonSegmentLength = AZStd::max(minRibbonSegmentLength, 1.f - SimuCore::ALMOST_ONE);
        ribbonWidthObject.dataValue = AZStd::max(ribbonWidthObject.dataValue, 0.f);
        tesselationFactor = AZStd::max(tesselationFactor, 1.f - SimuCore::ALMOST_ONE);
        curveTension = AZStd::clamp<float>(curveTension, 0.f, 1.f - SimuCore::ALMOST_ONE);
        trailParam.lifetime = AZStd::max(trailParam.lifetime, 0.f);
        trailParam.ratio = AZStd::clamp<float>(trailParam.ratio, 0.f, 1.f);
        ribbonParam.ribbonCount = AZStd::max<AZ::u32>(ribbonParam.ribbonCount, 0u);
    }

    void EmitSpawn::CheckParam()
    {
        spawnRateObject.dataValue = AZStd::max(spawnRateObject.dataValue, 0.f);
    }

    void EmitSpawnOverMoving::CheckParam()
    {
        spawnRatePerUnitObject.dataValue = AZStd::max(spawnRatePerUnitObject.dataValue, 0.f);
    }

    ParticleEventHandler::ParticleEventHandler()
    {
        EditorParticleDocumentBusRequestsBus::Handler::BusConnect();
    }

    ParticleEventHandler::~ParticleEventHandler()
    {
        EditorParticleDocumentBusRequestsBus::Handler::BusDisconnect();
    }

    void ParticleEventHandler::CheckParam()
    {
        maxEventNum = AZStd::max<AZ::u32>(maxEventNum, 0);
        emitNum = AZStd::max<AZ::u32>(emitNum, 0);
    }

    AZStd::vector<AZStd::string> ParticleEventHandler::GetEmitterNames() const
    {
        AZStd::vector<AZStd::string> emitterNames;
        EBUS_EVENT_RESULT(emitterNames, EditorParticleRequestsBus, GetEmitterNames);
        return emitterNames;
    }

    void ParticleEventHandler::OnEmitterNameChangedNotify(size_t index)
    {
        EBUS_EVENT_RESULT(emitterIndex, EditorParticleRequestsBus, GetEmitterIndex, index);
    }

    void ParticleEventHandler::OnEmitterNameChanged(ParticleSourceData* p)
    {
        auto& emitters = p->m_emitters;
        if (emitterIndex < emitters.size())
        {
            emitterName = emitters[emitterIndex]->m_name;
        }
    }

    InheritanceHandler::InheritanceHandler()
    {
        EditorParticleDocumentBusRequestsBus::Handler::BusConnect();
    }

    InheritanceHandler::~InheritanceHandler()
    {
        EditorParticleDocumentBusRequestsBus::Handler::BusDisconnect();
    }

    void InheritanceHandler::CheckParam()
    {
        Clamp<AZ::Vector3>(positionOffset, AZ::Vector3(-100000.f, -100000.f, -100000.f), AZ::Vector3(100000.f, 100000.f, 100000.f));
        Clamp<AZ::Vector3>(velocityRatio, AZ::Vector3(0.f, 0.f, 0.f), AZ::Vector3(1000.f, 1000.f, 1000.f));
        Clamp<AZ::Vector4>(colorRatio, AZ::Vector4(0.f), AZ::Vector4(1000.f));
        spawnRate = AZStd::max(spawnRate, 0.f);
    }

    AZStd::vector<AZStd::string> InheritanceHandler::GetEmitterNames() const
    {
        AZStd::vector<AZStd::string> emitterNames;
        EBUS_EVENT_RESULT(emitterNames, EditorParticleRequestsBus, GetEmitterNames);
        return emitterNames;
    }

    void InheritanceHandler::OnEmitterNameChangedNotify(size_t index)
    {
        EBUS_EVENT_RESULT(emitterIndex, EditorParticleRequestsBus, GetEmitterIndex, index);
    }

    void InheritanceHandler::OnEmitterNameChanged(ParticleSourceData* p)
    {
        auto& emitters = p->m_emitters;
        if (emitterIndex < emitters.size())
        {
            emitterName = emitters[emitterIndex]->m_name;
        }
    }

    void SpawnColor::CheckParam()
    {
        Clamp(startColorObject.dataValue, AZ::Color(0.f, 0.f, 0.f, 0.f), AZ::Color(1.f, 1.f, 1.f, 1.f));
    }

    void SpawnLifetime::CheckParam()
    {
        lifeTimeObject.dataValue = AZStd::max(lifeTimeObject.dataValue, 0.f);
    }

    void SpawnLocBox::CheckParam()
    {
        size = size.GetMax(AZ::Vector3(0.f, 0.f, 0.f));
    }

    void SpawnLocSphere::CheckParam()
    {
        radius = AZStd::max(radius, 0.f);
        ratio = AZStd::clamp(ratio, 0.f, 1.f);
        angle = AZStd::clamp(angle, -360.f, 360.f);
        radiusThickness = AZStd::clamp(radiusThickness, 0.f, 1.f);
    }

    void SpawnLocCylinder::CheckParam()
    {
        radius = AZStd::max(radius, 0.f);
        height = AZStd::max(height, 0.f);
        angle = AZStd::clamp(angle, -360.f, 360.f);
        radiusThickness = AZStd::clamp(radiusThickness, 0.f, 1.f);
    }

    void SpawnLocTorus::CheckParam()
    {
        torusRadius = AZStd::max(torusRadius, 0.f);
        tubeRadius = AZStd::max(tubeRadius, 0.f);
    }

    void SpawnRotation::CheckParam()
    {
        initAngleObject.dataValue = AZStd::clamp(initAngleObject.dataValue, -360.f, 360.f);
        rotateSpeedObject.dataValue = AZStd::clamp(rotateSpeedObject.dataValue, -100000.f, 100000.f);
        Clamp<AZ::Vector3>(initAxis, AZ::Vector3(-100000.f, -100000.f, -100000.f), AZ::Vector3(100000.f, 100000.f, 100000.f));
        Clamp<AZ::Vector3>(rotateAxis, AZ::Vector3(-100000.f, -100000.f, -100000.f), AZ::Vector3(100000.f, 100000.f, 100000.f));
    }

    void SpawnSize::CheckParam()
    {
        sizeObject.dataValue = sizeObject.dataValue.GetMax(AZ::Vector3(0.f, 0.f, 0.f));
    }

    void SpawnVelDirection::CheckParam()
    {
        Clamp<AZ::Vector3>(
            directionObject.dataValue, AZ::Vector3(-100000.f, -100000.f, -100000.f), AZ::Vector3(100000.f, 100000.f, 100000.f));
    }

    void SpawnVelSector::CheckParam()
    {
        centralAngle = AZStd::clamp(centralAngle, -360.f, 360.f);
        strengthObject.dataValue = AZStd::clamp(strengthObject.dataValue, -100000.f, 100000.f);
        rotateAngle = AZStd::clamp(rotateAngle, -360.f, 360.f);
        Clamp<AZ::Vector3>(direction, AZ::Vector3(-10000.f, -10000.f, -10000.f), AZ::Vector3(10000.f, 10000.f, 10000.f));
    }

    void SpawnVelCone::CheckParam()
    {
        angle = AZStd::clamp(angle, -360.f, 360.f);
        strengthObject.dataValue = AZStd::clamp(strengthObject.dataValue, -100000.f, 100000.f);
        Clamp<AZ::Vector3>(direction, AZ::Vector3(-10000.f, -10000.f, -10000.f), AZ::Vector3(10000.f, 10000.f, 10000.f));
    }

    void SpawnVelSphere::CheckParam()
    {
        Clamp<AZ::Vector3>(strengthObject.dataValue,
            AZ::Vector3(-100000.f, -100000.f, -100000.f), AZ::Vector3(100000.f, 100000.f, 100000.f));
    }

    void SpawnVelConcentrate::CheckParam()
    {
        rateObject.dataValue = AZStd::clamp(rateObject.dataValue, 0.f, 100000.f);
    }

    void UpdateColor::CheckParam()
    {
        Clamp(currentColorObject.dataValue, AZ::Color(0.f, 0.f, 0.f, 0.f), AZ::Color(1.f, 1.f, 1.f, 1.f));
    }

    void UpdateDragForce::CheckParam()
    {
        dragCoefficientObject.dataValue = AZStd::max(dragCoefficientObject.dataValue, 0.f);
    }

    void UpdateVortexForce::CheckParam()
    {
        originPullObject.dataValue = AZStd::max(originPullObject.dataValue, 0.f);
        vortexRateObject.dataValue = AZStd::clamp(vortexRateObject.dataValue, -100000.f, 100000.f);
        vortexRadiusObject.dataValue = AZStd::max(vortexRadiusObject.dataValue, 0.f);
    }

    void UpdateCurlNoiseForce::CheckParam()
    {
        noiseStrength = AZStd::clamp(noiseStrength, -10000.f, 10000.f);
        noiseFrequency = AZStd::clamp(noiseStrength, 0.f, 10000.f);
        Clamp<AZ::Vector3>(panNoiseField,
            AZ::Vector3(-100000.f, -100000.f, -100000.f), AZ::Vector3(100000.f, 100000.f, 100000.f));
        Clamp<AZ::Vector3>(randomizationVector,
            AZ::Vector3(-100000.f, -100000.f, -100000.f), AZ::Vector3(100000.f, 100000.f, 100000.f));
    }

    void UpdateSizeLinear::CheckParam()
    {
        sizeObject.dataValue = sizeObject.dataValue.GetMax(AZ::Vector3(0.f, 0.f, 0.f));
    }

    void UpdateSizeByVelocity::CheckParam()
    {
        velScaleObject.dataValue.minValue = velScaleObject.dataValue.minValue.GetMax(AZ::Vector3(0.0f, 0.0f, 0.0f));
        velScaleObject.dataValue.maxValue = velScaleObject.dataValue.maxValue.GetMax(AZ::Vector3(0.0f, 0.0f, 0.0f));
        velocityRange = AZStd::max(velocityRange, 0.f);
    }

    void SizeScale::CheckParam()
    {
        scaleFactorObject.dataValue = scaleFactorObject.dataValue.GetMax(AZ::Vector3(0.f, 0.f, 0.f));
    }

    void UpdateSubUv::CheckParam()
    {
        frameNum = AZStd::clamp<AZ::u32>(frameNum, 1u, 1000000u);
        framePerSecond = AZStd::clamp<AZ::u32>(framePerSecond, 0u, 1000000u);
    }

    void UpdateRotateAroundPoint::CheckParam()
    {
        radius = AZStd::max(radius, 0.f);
        rotateRate = AZStd::clamp(rotateRate, -100000.f, 100000.f);
    }

    void ParticleCollision::CheckParam()
    {
        bounce.restitution = AZStd::clamp<float>(bounce.restitution, 0.f, 1.f);
        bounce.randomizeNormal = AZStd::clamp<float>(bounce.randomizeNormal, 0.f, 1.f);
        collisionRadius.radius = AZStd::max(collisionRadius.radius, 0.f);
        collisionRadius.radiusScale = AZStd::max(collisionRadius.radiusScale, 0.0f);
    }

    template<typename T, size_t size>
    void ConvertDistIndexImpl(ValueObject<T, size>& valueObj, AZ::u32 version, Distribution& dists,
        [[maybe_unused]] const RandomTickMode& randomTickMode, [[maybe_unused]] const CurveTickMode& curveTickMode)
    {
        if (version == 1)
        {
            if (valueObj.distType == DistributionType::CONSTANT)
            {
                for (size_t index = 0; index < size; ++index)
                {
                    valueObj.distIndex[index] = 0;
                }
            }
        }

        if (version == 0)
        {
            for (size_t index = 0; index < size; ++index)
            {
                if (valueObj.distIndex[index] == 0)
                {
                    continue;
                }
                if (valueObj.distIndex[index] > dists.randoms.size())
                {
                    valueObj.distType = DistributionType::CURVE;
                    valueObj.distIndex[index] -= dists.randoms.size();
                    dists.curves[valueObj.distIndex[index] - 1]->tickMode = curveTickMode;
                }
                else
                {
                    if (dists.randoms.size() != 0)
                    {
                        valueObj.distType = DistributionType::RANDOM;
                        dists.randoms[valueObj.distIndex[index] - 1]->tickMode = randomTickMode;
                    }
                }
            }
        }
    }

    void RibbonConfig::ConvertDistIndexVersion(Distribution& distribution)
    {
        ConvertDistIndexImpl(ribbonWidthObject, version, distribution, RandomTickMode::ONCE, CurveTickMode::PARTICLE_LIFETIME);
        version = 1;
    }

    void EmitSpawn::ConvertDistIndexVersion(Distribution& distribution)
    {
        ConvertDistIndexImpl(spawnRateObject, version, distribution, RandomTickMode::PER_FRAME, CurveTickMode::EMIT_DURATION);
        version = 1;
    }

    void EmitSpawnOverMoving::ConvertDistIndexVersion(Distribution& distribution)
    {
        ConvertDistIndexImpl(spawnRatePerUnitObject, version, distribution, RandomTickMode::PER_FRAME, CurveTickMode::EMIT_DURATION);
        version = 1;
    }

    void SpawnColor::ConvertDistIndexVersion(Distribution& distribution)
    {
        ConvertDistIndexImpl(startColorObject, version, distribution, RandomTickMode::PER_FRAME, CurveTickMode::EMIT_DURATION);
        version = 1;
    }

    void SpawnLifetime::ConvertDistIndexVersion(Distribution& distribution)
    {
        ConvertDistIndexImpl(lifeTimeObject, version, distribution, RandomTickMode::PER_FRAME, CurveTickMode::EMIT_DURATION);
        version = 1;
    }

    void SpawnLocPoint::ConvertDistIndexVersion(Distribution& distribution)
    {
        ConvertDistIndexImpl(positionObject, version, distribution, RandomTickMode::PER_FRAME, CurveTickMode::EMIT_DURATION);
        version = 1;
    }

    void SpawnRotation::ConvertDistIndexVersion(Distribution& distribution)
    {
        ConvertDistIndexImpl(initAngleObject, version, distribution, RandomTickMode::PER_FRAME, CurveTickMode::EMIT_DURATION);
        ConvertDistIndexImpl(rotateSpeedObject, version, distribution, RandomTickMode::PER_FRAME, CurveTickMode::EMIT_DURATION);
        version = 1;
    }

    void SpawnSize::ConvertDistIndexVersion(Distribution& distribution)
    {
        ConvertDistIndexImpl(sizeObject, version, distribution, RandomTickMode::PER_FRAME, CurveTickMode::EMIT_DURATION);
        version = 1;
    }

    void SpawnVelDirection::ConvertDistIndexVersion(Distribution& distribution)
    {
        ConvertDistIndexImpl(strengthObject, version, distribution, RandomTickMode::PER_FRAME, CurveTickMode::EMIT_DURATION);
        ConvertDistIndexImpl(directionObject, version, distribution, RandomTickMode::PER_FRAME, CurveTickMode::EMIT_DURATION);
        version = 1;
    }

    void SpawnVelSector::ConvertDistIndexVersion(Distribution& distribution)
    {
        ConvertDistIndexImpl(strengthObject, version, distribution, RandomTickMode::PER_FRAME, CurveTickMode::EMIT_DURATION);
        version = 1;
    }

    void SpawnVelCone::ConvertDistIndexVersion(Distribution& distribution)
    {
        ConvertDistIndexImpl(strengthObject, version, distribution, RandomTickMode::PER_FRAME, CurveTickMode::EMIT_DURATION);
        version = 1;
    }

    void SpawnVelSphere::ConvertDistIndexVersion(Distribution& distribution)
    {
        ConvertDistIndexImpl(strengthObject, version, distribution, RandomTickMode::PER_FRAME, CurveTickMode::EMIT_DURATION);
        version = 1;
    }

    void SpawnVelConcentrate::ConvertDistIndexVersion(Distribution& distribution)
    {
        ConvertDistIndexImpl(rateObject, version, distribution, RandomTickMode::PER_FRAME, CurveTickMode::EMIT_DURATION);
        version = 1;
    }

    void SpawnLightEffect::ConvertDistIndexVersion(Distribution& distribution)
    {
        ConvertDistIndexImpl(lightColorObject, version, distribution, RandomTickMode::PER_FRAME, CurveTickMode::EMIT_DURATION);
        ConvertDistIndexImpl(intensityObject, version, distribution, RandomTickMode::PER_FRAME, CurveTickMode::EMIT_DURATION);
        ConvertDistIndexImpl(radianScaleObject, version, distribution, RandomTickMode::PER_FRAME, CurveTickMode::EMIT_DURATION);
        version = 1;
    }

    void UpdateColor::ConvertDistIndexVersion(Distribution& distribution)
    {
        ConvertDistIndexImpl(currentColorObject, version, distribution, RandomTickMode::ONCE, CurveTickMode::PARTICLE_LIFETIME);
        version = 1;
    }

    void UpdateConstForce::ConvertDistIndexVersion(Distribution& distribution)
    {
        ConvertDistIndexImpl(forceObject, version, distribution, RandomTickMode::ONCE, CurveTickMode::PARTICLE_LIFETIME);
        version = 1;
    }

    void UpdateDragForce::ConvertDistIndexVersion(Distribution& distribution)
    {
        ConvertDistIndexImpl(dragCoefficientObject, version, distribution, RandomTickMode::ONCE, CurveTickMode::PARTICLE_LIFETIME);
        version = 1;
    }

    void UpdateSizeLinear::ConvertDistIndexVersion(Distribution& distribution)
    {
        ConvertDistIndexImpl(sizeObject, version, distribution, RandomTickMode::ONCE, CurveTickMode::PARTICLE_LIFETIME);
        version = 1;
    }

    void UpdateSizeByVelocity::ConvertDistIndexVersion(Distribution& distribution)
    {
        ConvertDistIndexImpl(velScaleObject, version, distribution, RandomTickMode::ONCE, CurveTickMode::PARTICLE_LIFETIME);
        version = 1;
    }

    void SizeScale::ConvertDistIndexVersion(Distribution& distribution)
    {
        ConvertDistIndexImpl(scaleFactorObject, version, distribution, RandomTickMode::ONCE, CurveTickMode::PARTICLE_LIFETIME);
        version = 1;
    }

    void UpdateVelocity::ConvertDistIndexVersion(Distribution& distribution)
    {
        ConvertDistIndexImpl(strengthObject, version, distribution, RandomTickMode::ONCE, CurveTickMode::PARTICLE_LIFETIME);
        ConvertDistIndexImpl(directionObject, version, distribution, RandomTickMode::ONCE, CurveTickMode::PARTICLE_LIFETIME);
        version = 1;
    }

    Curve* Curve::InitCurve()
    {
        auto res = new Curve();
        KeyPoint first;
        KeyPoint last;
        first.time = 0.0f;
        first.value = 1.0f;
        last.time = 1.0f;
        last.value = 1.0f;
        res->keyPoints.emplace_back(first);
        res->keyPoints.emplace_back(last);
        return res;
    }

    void ParamDistInfo::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<ParamDistInfo>()
                ->Version(0)
                ->Field("valueObjPtr", &ParamDistInfo::valueObjPtr)
                ->Field("valueTypeId", &ParamDistInfo::valueTypeId)
                ->Field("isUniform", &ParamDistInfo::isUniform)
                ->Field("paramName", &ParamDistInfo::paramName)
                ->Field("paramIndex", &ParamDistInfo::paramIndex)
                ->Field("distIndex", &ParamDistInfo::distIndex);
        }
    }

    void DistInfos::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<DistInfos>()
                ->Version(0)
                ->Field("randomIndexInfos", &DistInfos::randomIndexInfos)
                ->Field("curveIndexInfos", &DistInfos::curveIndexInfos);
        }
    }

    void Distribution::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<Distribution>()
                ->Version(0)
                ->Field("randoms", &Distribution::randoms)
                ->Field("curves", &Distribution::curves)
                ->Field("distInfos", &Distribution::distInfos);
        }
    }

    void DistInfos::Clear()
    {
        randomIndexInfos.clear();
        curveIndexInfos.clear();
    }

    AZStd::vector<ParamDistInfo>* DistInfos::operator()(const OpenParticle::DistributionType& mode)
    {
        return mode == DistributionType::RANDOM ? &randomIndexInfos : (mode == DistributionType::CURVE ? &curveIndexInfos : nullptr);
    }

    void Distribution::GetDistributionIndex(AZStd::list<AZStd::any>& dist)
    {
        for (auto& val : dist)
        {
            DataConvertor::GetDistIndex(val, distInfos);
        }
    }

    void Distribution::GetDistributionIndex(AZStd::any& dist)
    {
        DataConvertor::GetDistIndex(dist, distInfos);
    }

    void Distribution::ClearDistributionIndex()
    {
        distInfos.Clear();
    }

    void SortIndex(AZStd::vector<ParamDistInfo>& indexInfo)
    {
        AZStd::sort(
            indexInfo.begin(), indexInfo.end(),
            [](ParamDistInfo& left, ParamDistInfo& right)
            {
                return left.distIndex < right.distIndex;
            });
    }

    void Distribution::SortDistributionIndex()
    {
        SortIndex(distInfos.randomIndexInfos);
        SortIndex(distInfos.curveIndexInfos);
    }

    template<typename DistType>
    void RebuildDist(AZStd::vector<DistType*>& dist, AZStd::vector<DistType*>& cache, AZStd::vector<size_t>& stashedIndexes)
    {
        for (auto stashedIndex = stashedIndexes.begin(); stashedIndex != stashedIndexes.end(); ++stashedIndex)
        {
            auto iter = AZStd::find_if(
                cache.begin(), cache.end(),
                [&dist, &stashedIndex](DistType* ptr)
                {
                    return dist[*stashedIndex - 1] == ptr;
                });
            if (iter == cache.end())
            {
                delete dist[*stashedIndex - 1];
            }
            dist.erase(dist.begin() + *stashedIndex - 1);
        }

        stashedIndexes.clear();
    }

    void Distribution::RebuildDistribution()
    {
        AZStd::vector<Random*> randomCache;
        for (auto& cache : randomCaches)
        {
            for (auto random : cache.second)
            {
                randomCache.emplace_back(random);
            }
        }
        RebuildDist(randoms, randomCache, stashedRandomIndexes);
        AZStd::vector<Curve*> curveCache;
        for (auto& cache : curveCaches)
        {
            for (auto curve : cache.second.curves)
            {
                curveCache.emplace_back(curve);
            }
        }
        RebuildDist(curves, curveCache, stashedCurveIndexes);
    }

    void RebuildIndex(AZStd::vector<ParamDistInfo>& indexInfo)
    {
        for (AZ::u32 index = 0; index < indexInfo.size(); index++)
        {
            if (indexInfo[index].valueTypeId == azrtti_typeid<ValueObjFloat>())
            {
                static_cast<ValueObjFloat*>(indexInfo[index].valueObjPtr)->distIndex[indexInfo[index].paramIndex] = index + 1;
            }
            if (indexInfo[index].valueTypeId == azrtti_typeid<ValueObjVec2>())
            {
                static_cast<ValueObjVec2*>(indexInfo[index].valueObjPtr)->distIndex[indexInfo[index].paramIndex] = index + 1;
            }
            if (indexInfo[index].valueTypeId == azrtti_typeid<ValueObjVec3>())
            {
                static_cast<ValueObjVec3*>(indexInfo[index].valueObjPtr)->distIndex[indexInfo[index].paramIndex] = index + 1;
            }
            if (indexInfo[index].valueTypeId == azrtti_typeid<ValueObjVec4>())
            {
                static_cast<ValueObjVec4*>(indexInfo[index].valueObjPtr)->distIndex[indexInfo[index].paramIndex] = index + 1;
            }
            if (indexInfo[index].valueTypeId == azrtti_typeid<ValueObjLinear>())
            {
                static_cast<ValueObjLinear*>(indexInfo[index].valueObjPtr)->distIndex[indexInfo[index].paramIndex] = index + 1;
            }
        }
    }

    void Distribution::RebuildDistributionIndex()
    {
        RebuildIndex(distInfos.randomIndexInfos);
        RebuildIndex(distInfos.curveIndexInfos);
    }

    template<typename DistType>
    bool CheckIndex(const AZStd::vector<DistType*>& dist, AZStd::vector<ParamDistInfo>& infos)
    {
        AZ::u32 index = 0;
        auto iter = AZStd::find_if(
            infos.begin(), infos.end(),
            [&index, &dist](ParamDistInfo& info)
            {
                return (info.distIndex > dist.size() || info.distIndex != ++index) && info.distIndex != 0;
            });
        return iter == infos.end();
    }

    bool Distribution::CheckDistributionIndex()
    {
        return CheckIndex(randoms, distInfos.randomIndexInfos) && CheckIndex(curves, distInfos.curveIndexInfos);
    }
} // namespace OpenParticle
