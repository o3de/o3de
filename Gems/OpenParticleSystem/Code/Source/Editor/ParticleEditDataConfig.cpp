/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "core/math/Constants.h"
#include <OpenParticleSystem/Asset/ParticleAsset.h>
#include <Atom/RPI.Edit/Common/AssetUtils.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzFramework/Translation/TranslationDef.h>
#include <OpenParticleSystem/ParticleEditDataConfig.h>

namespace OpenParticle
{
    template<typename T>
    void CheckParam(AZStd::any& any)
    {
        auto t = AZStd::any_cast<T>(any);
        t.CheckParam();
        AZStd::variant<AZStd::any> sourceVariant{ AZStd::make_any<T>(t) };
        any = AZStd::get<0>(sourceVariant);
    }

    template<typename T>
    void ConvertDistIndexVersion(AZStd::any& any, Distribution& distribution)
    {
        auto t = AZStd::any_cast<T>(any);
        t.ConvertDistIndexVersion(distribution);
        AZStd::variant<AZStd::any> sourceVariant{ AZStd::make_any<T>(t) };
        any = AZStd::get<0>(sourceVariant);
    }

    void ParticleEditDataConfig::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<ParticleEditDataConfig>()
                ->Version(0);

            serializeContext->Enum<OpenParticle::DistributionType>()
                ->Version(0)
                ->Value("Constant", OpenParticle::DistributionType::CONSTANT)
                ->Value("Random", OpenParticle::DistributionType::RANDOM)
                ->Value("Curve", OpenParticle::DistributionType::CURVE);

            serializeContext->Enum<OpenParticle::RandomTickMode>()
                ->Version(0)
                ->Value("Once", OpenParticle::RandomTickMode::ONCE)
                ->Value("PerFrame", OpenParticle::RandomTickMode::PER_FRAME)
                ->Value("PerSpawn", OpenParticle::RandomTickMode::PER_SPAWN);

            serializeContext->Enum<OpenParticle::CurveTickMode>()
                ->Version(0)
                ->Value("EmitDuration", OpenParticle::CurveTickMode::EMIT_DURATION)
                ->Value("ParticleLifetime", OpenParticle::CurveTickMode::PARTICLE_LIFETIME)
                ->Value("NormalizedAge", OpenParticle::CurveTickMode::NORMALIZED_AGE)
                ->Value("Custom", OpenParticle::CurveTickMode::CUSTOM);

            serializeContext->Enum<OpenParticle::CurveExtrapMode>()
                ->Version(0)
                ->Value("Cycle", OpenParticle::CurveExtrapMode::CYCLE)
                ->Value("CycleWithOffset", OpenParticle::CurveExtrapMode::CYCLE_WHIT_OFFSET)
                ->Value("Constant", OpenParticle::CurveExtrapMode::CONSTANT);

            serializeContext->Enum<OpenParticle::KeyPointInterpMode>()
                ->Version(0)
                ->Value("Linear", OpenParticle::KeyPointInterpMode::LINEAR)
                ->Value("Step", OpenParticle::KeyPointInterpMode::STEP)
                ->Value("CubicIn", OpenParticle::KeyPointInterpMode::CUBIC_IN)
                ->Value("CubicOut", OpenParticle::KeyPointInterpMode::CUBIC_OUT)
                ->Value("SineIn", OpenParticle::KeyPointInterpMode::SINE_IN)
                ->Value("SineOut", OpenParticle::KeyPointInterpMode::SINE_OUT)
                ->Value("CircleIn", OpenParticle::KeyPointInterpMode::CIRCLE_IN)
                ->Value("CircleOut", OpenParticle::KeyPointInterpMode::CIRCLE_OUT);

            serializeContext->Class<OpenParticle::ValueObjFloat>()
                ->Version(0)
                ->Attribute("Size", &OpenParticle::ValueObjFloat::Size)
                ->Field("dataValue", &OpenParticle::ValueObjFloat::dataValue)
                ->Field("isUniform", &OpenParticle::ValueObjFloat::isUniform)
                ->Field("distType", &OpenParticle::ValueObjFloat::distType)
                ->Field("distIndex", &OpenParticle::ValueObjFloat::distIndex);

            serializeContext->Class<OpenParticle::ValueObjVec2>()
                ->Version(0)
                ->Attribute("Size", &OpenParticle::ValueObjVec2::Size)
                ->Field("dataValue", &OpenParticle::ValueObjVec2::dataValue)
                ->Field("isUniform", &OpenParticle::ValueObjVec2::isUniform)
                ->Field("distType", &OpenParticle::ValueObjVec2::distType)
                ->Field("distIndex", &OpenParticle::ValueObjVec2::distIndex);

            serializeContext->Class<OpenParticle::ValueObjVec3>()
                ->Version(0)
                ->Attribute("Size", &OpenParticle::ValueObjVec3::Size)
                ->Field("dataValue", &OpenParticle::ValueObjVec3::dataValue)
                ->Field("isUniform", &OpenParticle::ValueObjVec3::isUniform)
                ->Field("distType", &OpenParticle::ValueObjVec3::distType)
                ->Field("distIndex", &OpenParticle::ValueObjVec3::distIndex);

            serializeContext->Class<OpenParticle::ValueObjVec4>()
                ->Version(0)
                ->Attribute("Size", &OpenParticle::ValueObjVec4::Size)
                ->Field("dataValue", &OpenParticle::ValueObjVec4::dataValue)
                ->Field("isUniform", &OpenParticle::ValueObjVec4::isUniform)
                ->Field("distType", &OpenParticle::ValueObjVec4::distType)
                ->Field("distIndex", &OpenParticle::ValueObjVec4::distIndex);

            serializeContext->Class<OpenParticle::ValueObjColor>()
                ->Version(0)
                ->Attribute("Size", &OpenParticle::ValueObjColor::Size)
                ->Field("dataValue", &OpenParticle::ValueObjColor::dataValue)
                ->Field("isUniform", &OpenParticle::ValueObjColor::isUniform)
                ->Field("distType", &OpenParticle::ValueObjColor::distType)
                ->Field("distIndex", &OpenParticle::ValueObjColor::distIndex);

            serializeContext->Class<OpenParticle::LinearValue>()
                ->Version(0)
                ->Field("value", &OpenParticle::LinearValue::value)
                ->Field("minValue", &OpenParticle::LinearValue::minValue)
                ->Field("maxValue", &OpenParticle::LinearValue::maxValue);

            serializeContext->Class<OpenParticle::ValueObjLinear>()
                ->Version(0)
                ->Attribute("Size", &OpenParticle::ValueObjLinear::Size)
                ->Field("dataValue", &OpenParticle::ValueObjLinear::dataValue)
                ->Field("isUniform", &OpenParticle::ValueObjLinear::isUniform)
                ->Field("distType", &OpenParticle::ValueObjLinear::distType)
                ->Field("distIndex", &OpenParticle::ValueObjLinear::distIndex);

            serializeContext->Class<OpenParticle::SystemConfig>()
                ->Version(0)
                ->Field("loop", &OpenParticle::SystemConfig::loop)
                ->Field("parallel", &OpenParticle::SystemConfig::parallel);

            serializeContext->Class<OpenParticle::PreWarm>()
                ->Version(0)
                ->Attribute("CHECK_PARAM", &CheckParam<OpenParticle::PreWarm>)
                ->Field("warmupTime", &OpenParticle::PreWarm::warmupTime)
                ->Field("tickCount", &OpenParticle::PreWarm::tickCount)
                ->Field("tickDelta", &OpenParticle::PreWarm::tickDelta);

            serializeContext->Class<OpenParticle::EmitterConfig>()
                ->Version(0)
                ->Attribute("CHECK_PARAM", &CheckParam<OpenParticle::EmitterConfig>)
                ->Field("type", &OpenParticle::EmitterConfig::type)
                ->Field("maxSize", &OpenParticle::EmitterConfig::maxSize)
                ->Field("startTime", &OpenParticle::EmitterConfig::startTime)
                ->Field("localSpace", &OpenParticle::EmitterConfig::localSpace)
                ->Field("duration", &OpenParticle::EmitterConfig::duration)
                ->Field("loop", &OpenParticle::EmitterConfig::loop);

            serializeContext->Class<OpenParticle::SingleBurst>()
                ->Version(0)
                ->Field("time", &OpenParticle::SingleBurst::time)
                ->Field("count", &OpenParticle::SingleBurst::count)
                ->Field("minCount", &OpenParticle::SingleBurst::minCount);

            serializeContext->Class<OpenParticle::EmitBurstList>()
                ->Version(0)
                ->Field("isProcessBurstList", &OpenParticle::EmitBurstList::isProcessBurstList)
                ->Field("burstList", &OpenParticle::EmitBurstList::burstList);

            serializeContext->Class<OpenParticle::EmitSpawn>()
                ->Version(0)
                ->Attribute("CHECK_PARAM", &CheckParam<OpenParticle::EmitSpawn>)
                ->Attribute("CONVERTOR", &ConvertDistIndexVersion<OpenParticle::EmitSpawn>)
                ->Field("isProcessSpawnRate", &OpenParticle::EmitSpawn::isProcessSpawnRate)
                ->Field("spawnRateObject", &OpenParticle::EmitSpawn::spawnRateObject);

            serializeContext->Class<OpenParticle::EmitSpawnOverMoving>()
                ->Version(0)
                ->Attribute("CHECK_PARAM", &CheckParam<OpenParticle::EmitSpawnOverMoving>)
                ->Attribute("CONVERTOR", &ConvertDistIndexVersion<OpenParticle::EmitSpawnOverMoving>)
                ->Field("isProcessSpawnRate", &OpenParticle::EmitSpawnOverMoving::isProcessSpawnRate)
                ->Field("isProcessBurstList", &OpenParticle::EmitSpawnOverMoving::isProcessBurstList)
                ->Field("isIgnoreSpawnRateWhenMoving", &OpenParticle::EmitSpawnOverMoving::isIgnoreSpawnRateWhenMoving)
                ->Field("spawnRatePerUnitObject", &OpenParticle::EmitSpawnOverMoving::spawnRatePerUnitObject);

            serializeContext->Class<OpenParticle::ParticleEventHandler>()
                ->Version(0)
                ->Attribute("CHECK_PARAM", &CheckParam<OpenParticle::ParticleEventHandler>)
                ->Field("emitterIndex", &OpenParticle::ParticleEventHandler::emitterIndex)
                ->Field("emitterName", &OpenParticle::ParticleEventHandler::emitterName)
                ->Field("eventType", &OpenParticle::ParticleEventHandler::eventType)
                ->Field("maxEventNum", &OpenParticle::ParticleEventHandler::maxEventNum)
                ->Field("emitNum", &OpenParticle::ParticleEventHandler::emitNum)
                ->Field("useEventInfo", &OpenParticle::ParticleEventHandler::useEventInfo);

            serializeContext->Class<OpenParticle::InheritanceHandler>()
                ->Version(0)
                ->Attribute("CHECK_PARAM", &CheckParam<OpenParticle::InheritanceHandler>)
                ->Field("emitterIndex", &OpenParticle::InheritanceHandler::emitterIndex)
                ->Field("emitterName", &OpenParticle::InheritanceHandler::emitterName)
                ->Field("calculateSpawnRate", &OpenParticle::InheritanceHandler::calculateSpawnRate)
                ->Field("spawnRate", &OpenParticle::InheritanceHandler::spawnRate)
                ->Field("spawnEnable", &OpenParticle::InheritanceHandler::spawnEnable)
                ->Field("applyPosition", &OpenParticle::InheritanceHandler::applyPosition)
                ->Field("positionOffset", &OpenParticle::InheritanceHandler::positionOffset)
                ->Field("applyVelocity", &OpenParticle::InheritanceHandler::applyVelocity)
                ->Field("overwriteVelocity", &OpenParticle::InheritanceHandler::overwriteVelocity)
                ->Field("velocityRatio", &OpenParticle::InheritanceHandler::velocityRatio)
                ->Field("applyColorRGB", &OpenParticle::InheritanceHandler::applyColorRGB)
                ->Field("applyColorAlpha", &OpenParticle::InheritanceHandler::applyColorAlpha)
                ->Field("colorRatio", &OpenParticle::InheritanceHandler::colorRatio)
                ->Field("applySize", &OpenParticle::InheritanceHandler::applySize)
                ->Field("applyAge", &OpenParticle::InheritanceHandler::applyAge)
                ->Field("applyLifetime", &OpenParticle::InheritanceHandler::applyLifetime);

            serializeContext->Class<OpenParticle::SpawnColor>()
                ->Version(0)
                ->Attribute("CHECK_PARAM", &CheckParam<OpenParticle::SpawnColor>)
                ->Attribute("CONVERTOR", &ConvertDistIndexVersion<OpenParticle::SpawnColor>)
                ->Field("startColorObject", &OpenParticle::SpawnColor::startColorObject);

            serializeContext->Class<OpenParticle::SpawnLifetime>()
                ->Version(0)
                ->Attribute("CHECK_PARAM", &CheckParam<OpenParticle::SpawnLifetime>)
                ->Attribute("CONVERTOR", &ConvertDistIndexVersion<OpenParticle::SpawnLifetime>)
                ->Field("lifeTimeObject", &OpenParticle::SpawnLifetime::lifeTimeObject);

            serializeContext->Class<OpenParticle::SpawnLocBox>()
                ->Version(0)
                ->Attribute("CHECK_PARAM", &CheckParam<OpenParticle::SpawnLocBox>)
                ->Field("center", &OpenParticle::SpawnLocBox::center)
                ->Field("size", &OpenParticle::SpawnLocBox::size);

            serializeContext->Class<OpenParticle::SpawnLocPoint>()
                ->Version(0)
                ->Attribute("CONVERTOR", &ConvertDistIndexVersion<OpenParticle::SpawnLocPoint>)
                ->Field("positionObject", &OpenParticle::SpawnLocPoint::positionObject);

            serializeContext->Class<OpenParticle::SpawnLocSphere>()
                ->Version(0)
                ->Attribute("CHECK_PARAM", &CheckParam<OpenParticle::SpawnLocSphere>)
                ->Field("axis", &OpenParticle::SpawnLocSphere::axis)
                ->Field("center", &OpenParticle::SpawnLocSphere::center)
                ->Field("radius", &OpenParticle::SpawnLocSphere::radius)
                ->Field("ratio", &OpenParticle::SpawnLocSphere::ratio)
                ->Field("angle", &OpenParticle::SpawnLocSphere::angle)
                ->Field("radiusThickness", &OpenParticle::SpawnLocSphere::radiusThickness);

            serializeContext->Class<OpenParticle::SpawnLocCylinder>()
                ->Version(0)
                ->Attribute("CHECK_PARAM", &CheckParam<OpenParticle::SpawnLocCylinder>)
                ->Field("axis", &OpenParticle::SpawnLocCylinder::axis)
                ->Field("center", &OpenParticle::SpawnLocCylinder::center)
                ->Field("radius", &OpenParticle::SpawnLocCylinder::radius)
                ->Field("height", &OpenParticle::SpawnLocCylinder::height)
                ->Field("angle", &OpenParticle::SpawnLocCylinder::angle)
                ->Field("radiusThickness", &OpenParticle::SpawnLocCylinder::radiusThickness);

            serializeContext->Class<OpenParticle::SpawnLocSkeleton>()
                ->Version(0)
                ->Field("sampleType", &OpenParticle::SpawnLocSkeleton::sampleType)
                ->Field("scale", &OpenParticle::SpawnLocSkeleton::scale);

            serializeContext->Class<OpenParticle::SpawnLocTorus>()
                ->Version(0)
                ->Attribute("CHECK_PARAM", &CheckParam<OpenParticle::SpawnLocTorus>)
                ->Field("center", &OpenParticle::SpawnLocTorus::center)
                ->Field("torusRadius", &OpenParticle::SpawnLocTorus::torusRadius)
                ->Field("tubeRadius", &OpenParticle::SpawnLocTorus::tubeRadius)
                ->Field("torusAxis", &OpenParticle::SpawnLocTorus::torusAxis);

            serializeContext->Class<OpenParticle::SpawnSize>()
                ->Version(0)
                ->Attribute("CHECK_PARAM", &CheckParam<OpenParticle::SpawnSize>)
                ->Attribute("CONVERTOR", &ConvertDistIndexVersion<OpenParticle::SpawnSize>)
                ->Field("sizeObject", &OpenParticle::SpawnSize::sizeObject);

            serializeContext->Class<OpenParticle::SpawnVelDirection>()
                ->Version(0)
                ->Attribute("CHECK_PARAM", &CheckParam<OpenParticle::SpawnVelDirection>)
                ->Attribute("CONVERTOR", &ConvertDistIndexVersion<OpenParticle::SpawnVelDirection>)
                ->Field("strengthObject", &OpenParticle::SpawnVelDirection::strengthObject)
                ->Field("directionObject", &OpenParticle::SpawnVelDirection::directionObject);

            serializeContext->Class<OpenParticle::SpawnVelSphere>()
                ->Version(0)
                ->Attribute("CHECK_PARAM", &CheckParam<OpenParticle::SpawnVelSphere>)
                ->Attribute("CONVERTOR", &ConvertDistIndexVersion<OpenParticle::SpawnVelSphere>)
                ->Field("strengthObject", &OpenParticle::SpawnVelSphere::strengthObject);

            serializeContext->Class<OpenParticle::SpawnVelConcentrate>()
                ->Version(0)
                ->Attribute("CHECK_PARAM", &CheckParam<OpenParticle::SpawnVelConcentrate>)
                ->Attribute("CONVERTOR", &ConvertDistIndexVersion<OpenParticle::SpawnVelConcentrate>)
                ->Field("centre", &OpenParticle::SpawnVelConcentrate::centre)
                ->Field("rateObject", &OpenParticle::SpawnVelConcentrate::rateObject);

            serializeContext->Class<OpenParticle::SpawnVelSector>()
                ->Version(0)
                ->Attribute("CHECK_PARAM", &CheckParam<OpenParticle::SpawnVelSector>)
                ->Attribute("CONVERTOR", &ConvertDistIndexVersion<OpenParticle::SpawnVelSector>)
                ->Field("centralAngle", &OpenParticle::SpawnVelSector::centralAngle)
                ->Field("rotateAngle", &OpenParticle::SpawnVelSector::rotateAngle)
                ->Field("strengthObject", &OpenParticle::SpawnVelSector::strengthObject)
                ->Field("direction", &OpenParticle::SpawnVelSector::direction);

            serializeContext->Class<OpenParticle::SpawnVelCone>()
                ->Version(0)
                ->Attribute("CHECK_PARAM", &CheckParam<OpenParticle::SpawnVelCone>)
                ->Attribute("CONVERTOR", &ConvertDistIndexVersion<OpenParticle::SpawnVelCone>)
                ->Field("angle", &OpenParticle::SpawnVelCone::angle)
                ->Field("strengthObject", &OpenParticle::SpawnVelCone::strengthObject)
                ->Field("direction", &OpenParticle::SpawnVelCone::direction);

            serializeContext->Class<OpenParticle::SpawnRotation>()
                ->Version(0)
                ->Attribute("CHECK_PARAM", &CheckParam<OpenParticle::SpawnRotation>)
                ->Attribute("CONVERTOR", &ConvertDistIndexVersion<OpenParticle::SpawnRotation>)
                ->Field("initAxis", &OpenParticle::SpawnRotation::initAxis)
                ->Field("rotateAxis", &OpenParticle::SpawnRotation::rotateAxis)
                ->Field("initAngleObject", &OpenParticle::SpawnRotation::initAngleObject)
                ->Field("rotateSpeedObject", &OpenParticle::SpawnRotation::rotateSpeedObject);

            serializeContext->Class<OpenParticle::SpawnLightEffect>()
                ->Version(0)
                ->Attribute("CONVERTOR", &ConvertDistIndexVersion<OpenParticle::SpawnLightEffect>)
                ->Field("lightColorObject", &OpenParticle::SpawnLightEffect::lightColorObject)
                ->Field("intensityObject", &OpenParticle::SpawnLightEffect::intensityObject)
                ->Field("radianScaleObject", &OpenParticle::SpawnLightEffect::radianScaleObject);

            serializeContext->Class<OpenParticle::SpawnLocationEvent>()
                ->Version(0)
                ->Field("whetherSendEvent", &OpenParticle::SpawnLocationEvent::whetherSendEvent);

            serializeContext->Class<OpenParticle::UpdateConstForce>()
                ->Version(0)
                ->Attribute("CONVERTOR", &ConvertDistIndexVersion<OpenParticle::UpdateConstForce>)
                ->Field("forceObject", &OpenParticle::UpdateConstForce::forceObject);

            serializeContext->Class<OpenParticle::UpdateDragForce>()
                ->Version(0)
                ->Attribute("CHECK_PARAM", &CheckParam<OpenParticle::UpdateDragForce>)
                ->Attribute("CONVERTOR", &ConvertDistIndexVersion<OpenParticle::UpdateDragForce>)
                ->Field("dragCoefficientObject", &OpenParticle::UpdateDragForce::dragCoefficientObject);

            serializeContext->Class<OpenParticle::UpdateVortexForce>()
                ->Version(0)
                ->Attribute("CHECK_PARAM", &CheckParam<OpenParticle::UpdateVortexForce>)
                ->Field("vortexAxis", &OpenParticle::UpdateVortexForce::vortexAxis)
                ->Field("origin", &OpenParticle::UpdateVortexForce::origin)
                ->Field("originPullObject", &OpenParticle::UpdateVortexForce::originPullObject)
                ->Field("vortexRateObject", &OpenParticle::UpdateVortexForce::vortexRateObject)
                ->Field("vortexRadiusObject", &OpenParticle::UpdateVortexForce::vortexRadiusObject);

            serializeContext->Class<OpenParticle::UpdateCurlNoiseForce>()
                ->Version(0)
                ->Field("noiseStrength", &OpenParticle::UpdateCurlNoiseForce::noiseStrength)
                ->Field("noiseFrequency", &OpenParticle::UpdateCurlNoiseForce::noiseFrequency)
                ->Field("panNoise", &OpenParticle::UpdateCurlNoiseForce::panNoise)
                ->Field("panNoiseField", &OpenParticle::UpdateCurlNoiseForce::panNoiseField)
                ->Field("randomSeed", &OpenParticle::UpdateCurlNoiseForce::randomSeed)
                ->Field("randomizationVector", &OpenParticle::UpdateCurlNoiseForce::randomizationVector);

            serializeContext->Class<OpenParticle::UpdateSizeLinear>()
                ->Version(0)
                ->Attribute("CHECK_PARAM", &CheckParam<OpenParticle::UpdateSizeLinear>)
                ->Attribute("CONVERTOR", &ConvertDistIndexVersion<OpenParticle::UpdateSizeLinear>)
                ->Field("sizeObject", &OpenParticle::UpdateSizeLinear::sizeObject);

            serializeContext->Class<OpenParticle::UpdateSizeByVelocity>()
                ->Version(0)
                ->Attribute("CHECK_PARAM", &CheckParam<OpenParticle::UpdateSizeByVelocity>)
                ->Attribute("CONVERTOR", &ConvertDistIndexVersion<OpenParticle::UpdateSizeByVelocity>)
                ->Field("velocityRange", &OpenParticle::UpdateSizeByVelocity::velocityRange)
                ->Field("velScaleObject", &OpenParticle::UpdateSizeByVelocity::velScaleObject);

            serializeContext->Class<OpenParticle::SizeScale>()
                ->Version(0)
                ->Attribute("CHECK_PARAM", &CheckParam<OpenParticle::SizeScale>)
                ->Attribute("CONVERTOR", &ConvertDistIndexVersion<OpenParticle::SizeScale>)
                ->Field("scaleFactorObject", &OpenParticle::SizeScale::scaleFactorObject);

            serializeContext->Class<OpenParticle::UpdateColor>()
                ->Version(0)
                ->Attribute("CHECK_PARAM", &CheckParam<OpenParticle::UpdateColor>)
                ->Attribute("CONVERTOR", &ConvertDistIndexVersion<OpenParticle::UpdateColor>)
                ->Field("currentColorObject", &OpenParticle::UpdateColor::currentColorObject);

            serializeContext->Class<OpenParticle::UpdateLocationEvent>()
                ->Version(0)
                ->Field("whetherSendEvent", &OpenParticle::UpdateLocationEvent::whetherSendEvent);

            serializeContext->Class<OpenParticle::UpdateDeathEvent>()
                ->Version(0)
                ->Field("whetherSendEvent", &OpenParticle::UpdateDeathEvent::whetherSendEvent);

            serializeContext->Class<OpenParticle::UpdateCollisionEvent>()
                ->Version(0)
                ->Field("whetherSendEvent", &OpenParticle::UpdateCollisionEvent::whetherSendEvent);

            serializeContext->Class<OpenParticle::UpdateInheritanceEvent>()
                ->Version(0)
                ->Field("whetherSendEvent", &OpenParticle::UpdateInheritanceEvent::whetherSendEvent);

            serializeContext->Class<OpenParticle::UpdateSubUv>()
                ->Version(0)
                ->Attribute("CHECK_PARAM", &CheckParam<OpenParticle::UpdateSubUv>)
                ->Field("framePerSecond", &OpenParticle::UpdateSubUv::framePerSecond)
                ->Field("frameNum", &OpenParticle::UpdateSubUv::frameNum)
                ->Field("spawnOnly", &OpenParticle::UpdateSubUv::spawnOnly)
                ->Field("IndexByEventOrder", &OpenParticle::UpdateSubUv::IndexByEventOrder);

            serializeContext->Class<OpenParticle::UpdateRotateAroundPoint>()
                ->Version(0)
                ->Attribute("CHECK_PARAM", &CheckParam<OpenParticle::UpdateRotateAroundPoint>)
                ->Field("rotateRate", &OpenParticle::UpdateRotateAroundPoint::rotateRate)
                ->Field("radius", &OpenParticle::UpdateRotateAroundPoint::radius)
                ->Field("xAxis", &OpenParticle::UpdateRotateAroundPoint::xAxis)
                ->Field("yAxis", &OpenParticle::UpdateRotateAroundPoint::yAxis)
                ->Field("center", &OpenParticle::UpdateRotateAroundPoint::center);

            serializeContext->Class<OpenParticle::UpdateVelocity>()
                ->Version(0)
                ->Attribute("CONVERTOR", &ConvertDistIndexVersion<OpenParticle::UpdateVelocity>)
                ->Field("strengthObject", &OpenParticle::UpdateVelocity::strengthObject)
                ->Field("directionObject", &OpenParticle::UpdateVelocity::directionObject);

            serializeContext->Class<OpenParticle::CollisionPlane>()
                ->Version(0)
                ->Field("normal", &OpenParticle::CollisionPlane::normal)
                ->Field("position", &OpenParticle::CollisionPlane::position);

            serializeContext->Class<OpenParticle::CollisionRadius>()
                ->Version(0)
                ->Field("type", &OpenParticle::CollisionRadius::type)
                ->Field("method", &OpenParticle::CollisionRadius::method)
                ->Field("radius", &OpenParticle::CollisionRadius::radius)
                ->Field("radiusScale", &OpenParticle::CollisionRadius::radiusScale);

            serializeContext->Class<OpenParticle::Bounce>()
                ->Version(0)
                ->Field("restitution", &OpenParticle::Bounce::restitution)
                ->Field("randomizeNormal", &OpenParticle::Bounce::randomizeNormal);

            serializeContext->Class<OpenParticle::ParticleCollision>()
                ->Version(0)
                ->Attribute("CHECK_PARAM", &CheckParam<OpenParticle::ParticleCollision>)
                ->Field("collisionType", &OpenParticle::ParticleCollision::collisionType)
                ->Field("collisionRadius", &OpenParticle::ParticleCollision::collisionRadius)
                ->Field("bounce", &OpenParticle::ParticleCollision::bounce)
                ->Field("friction", &OpenParticle::ParticleCollision::friction)
                ->Field("useTwoPlane", &OpenParticle::ParticleCollision::useTwoPlane)
                ->Field("collisionPlane1", &OpenParticle::ParticleCollision::collisionPlane1)
                ->Field("collisionPlane2", &OpenParticle::ParticleCollision::collisionPlane2);

            serializeContext->Class<OpenParticle::SpriteConfig>()
                ->Version(0)
                ->Attribute("CHECK_PARAM", &CheckParam<OpenParticle::SpriteConfig>)
                ->Field("facing", &OpenParticle::SpriteConfig::facing)
                ->Field("sortId", &OpenParticle::SpriteConfig::sortId)
                ->Field("subImage", &OpenParticle::SpriteConfig::subImageSize);

            serializeContext->Class<OpenParticle::Random>()
                ->Version(0)
                ->Field("min", &OpenParticle::Random::min)
                ->Field("max", &OpenParticle::Random::max)
                ->Field("tickMode", &OpenParticle::Random::tickMode);

            serializeContext->Class<OpenParticle::KeyPoint>()
                ->Version(0)
                ->Field("time", &OpenParticle::KeyPoint::time)
                ->Field("value", &OpenParticle::KeyPoint::value)
                ->Field("interpMode", &OpenParticle::KeyPoint::interpMode);

            serializeContext->Class<OpenParticle::Curve>()
                ->Version(0)
                ->Attribute("InitCurve", &OpenParticle::Curve::InitCurve)
                ->Field("leftExtrapMode", &OpenParticle::Curve::leftExtrapMode)
                ->Field("rightExtrapMode", &OpenParticle::Curve::rightExtrapMode)
                ->Field("valueFactor", &OpenParticle::Curve::valueFactor)
                ->Field("timeFactor", &OpenParticle::Curve::timeFactor)
                ->Field("tickMode", &OpenParticle::Curve::tickMode)
                ->Field("keyPoint", &OpenParticle::Curve::keyPoints);

            serializeContext->Class<OpenParticle::MeshConfig>()
                ->Version(0)
                ->Attribute("CHECK_PARAM", &CheckParam<OpenParticle::MeshConfig>)
                ->Field("facing", &OpenParticle::MeshConfig::facing)
                ->Field("sortId", &OpenParticle::MeshConfig::sortId);

            serializeContext->Class<OpenParticle::TrailParam>()
                ->Version(0)
                ->Field("ratio", &OpenParticle::TrailParam::ratio)
                ->Field("lifetime", &OpenParticle::TrailParam::lifetime)
                ->Field("inheritLiftime", &OpenParticle::TrailParam::inheritLifetime)
                ->Field("dieWithParticles", &OpenParticle::TrailParam::dieWithParticles);

            serializeContext->Class<OpenParticle::RibbonParam>()
                ->Version(0)
                ->Field("ribbonCount", &OpenParticle::RibbonParam::ribbonCount);

            serializeContext->Class<OpenParticle::RibbonConfig>()
                ->Version(0)
                ->Attribute("CHECK_PARAM", &CheckParam<OpenParticle::RibbonConfig>)
                ->Attribute("CONVERTOR", &ConvertDistIndexVersion<OpenParticle::RibbonConfig>)
                ->Field("trailParam", &OpenParticle::RibbonConfig::trailParam)
                ->Field("ribbonParam", &OpenParticle::RibbonConfig::ribbonParam)
                ->Field("sortId", &OpenParticle::RibbonConfig::sortId)
                ->Field("minRibbonSegmentLength", &OpenParticle::RibbonConfig::minRibbonSegmentLength)
                ->Field("ribbonWidthObject", &OpenParticle::RibbonConfig::ribbonWidthObject)
                ->Field("inheritSize", &OpenParticle::RibbonConfig::inheritSize)
                ->Field("tesselation", &OpenParticle::RibbonConfig::tesselationFactor)
                ->Field("curveTension", &OpenParticle::RibbonConfig::curveTension)
                ->Field("tilingDistance", &OpenParticle::RibbonConfig::tilingDistance)
                ->Field("facing", &OpenParticle::RibbonConfig::facing)
                ->Field("mode", &OpenParticle::RibbonConfig::mode);

            AZ::EditContext* editContext = serializeContext->GetEditContext();
            if (editContext != nullptr)
            {
                editContext->Class<OpenParticle::SystemConfig>(QT_TRANSLATE_NOOP("OpenParticleSystem", "Particle System Config"), "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &OpenParticle::SystemConfig::loop, QT_TRANSLATE_NOOP("OpenParticleSystem", "Looping"),
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "Particle system loop playback."));

                editContext->Class<OpenParticle::PreWarm>(QT_TRANSLATE_NOOP("OpenParticleSystem", "PreWarm"), QT_TRANSLATE_NOOP("OpenParticleSystem", "Particle Simulate Type CPU or GPU."))
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &OpenParticle::PreWarm::warmupTime,
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "Warmup Time"),
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "Warm up time in seconds"))
                    ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &OpenParticle::PreWarm::tickCount,
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "Tick Count"),
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "Number of ticks to process for warmup"))
                    ->Attribute(AZ::Edit::Attributes::Min, 0)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &OpenParticle::PreWarm::tickDelta,
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "Tick Delta"),
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "Delta time to use for warmup ticks."))
                    ->Attribute(AZ::Edit::Attributes::Min, 0.f);

                editContext->Class<OpenParticle::EmitterConfig>(QT_TRANSLATE_NOOP("OpenParticleSystem", "Particle Emitter Conifg"), "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &OpenParticle::EmitterConfig::type, QT_TRANSLATE_NOOP("OpenParticleSystem", "Simulate Type"),
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "The simulation type of the emitter."))
                        ->Attribute(AZ::Edit::Attributes::ReadOnly, true)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &OpenParticle::EmitterConfig::duration,
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "Emitter Duration"), QT_TRANSLATE_NOOP("OpenParticleSystem", "The duration of the emitter emits particles."))
                        ->Attribute(AZ::Edit::Attributes::Min, .05f)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &OpenParticle::EmitterConfig::localSpace,
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "Use Local Space"), QT_TRANSLATE_NOOP("OpenParticleSystem", "Whether the emitter uses local space."))
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &OpenParticle::EmitterConfig::maxSize, QT_TRANSLATE_NOOP("OpenParticleSystem", "Max Particles"),
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "Maximum number of particles emitted by the emitter."))
                        ->Attribute(AZ::Edit::Attributes::Min, 1)
                        ->Attribute(AZ::Edit::Attributes::Max, 1000000)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &OpenParticle::EmitterConfig::startTime, QT_TRANSLATE_NOOP("OpenParticleSystem", "Spawn Delay"),
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "How long the emitter delays spawning particles."))
                        ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &OpenParticle::EmitterConfig::loop, QT_TRANSLATE_NOOP("OpenParticleSystem", "Looping"),
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "Particle system loop playback."));

                editContext->Class<OpenParticle::SingleBurst>(QT_TRANSLATE_NOOP("OpenParticleSystem", "SingleBurst"), QT_TRANSLATE_NOOP("OpenParticleSystem", "The burst list of particles."))
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::Show)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &OpenParticle::SingleBurst::time, QT_TRANSLATE_NOOP("OpenParticleSystem", "Time"),
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "The burst time of particles."))
                    ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &OpenParticle::SingleBurst::count,
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "Particle Number"), QT_TRANSLATE_NOOP("OpenParticleSystem", "The burst number of particles."))
                    ->Attribute(AZ::Edit::Attributes::Min, 0u)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &OpenParticle::SingleBurst::minCount,
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "Min Particle Number"),
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "The minimum burst number of particles."));

                editContext->Class<OpenParticle::EmitBurstList>(QT_TRANSLATE_NOOP("OpenParticleSystem", "Burst List"), "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::Show)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &OpenParticle::EmitBurstList::isProcessBurstList,
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "Process Burst List"),
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "Whether to process burst."))
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &OpenParticle::EmitBurstList::burstList, QT_TRANSLATE_NOOP("OpenParticleSystem", "Burst List"),
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "The list of the burst element."))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true);

                editContext->Class<OpenParticle::EmitSpawn>(QT_TRANSLATE_NOOP("OpenParticleSystem", "Spawn Rate"), "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::Show)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &OpenParticle::EmitSpawn::isProcessSpawnRate,
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "Process Spawn Rate"), QT_TRANSLATE_NOOP("OpenParticleSystem", "Whether to process spawn rate."))
                    ->DataElement(
                        AZ_CRC("DistCtrlHandler"), &OpenParticle::EmitSpawn::spawnRateObject, QT_TRANSLATE_NOOP("OpenParticleSystem", "Particle Rate"),
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "The rate of spawning particles."))
                        ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                        ->Attribute(AZ::Edit::Attributes::Max, 100000.f)
                        ->Attribute(AZ_CRC("Id"), azrtti_typeid<OpenParticle::EmitSpawn>())
                        ->Attribute(AZ_CRC("ParamId"), AZ::TypeId::CreateName("spawnRateObject"));

                editContext->Class<OpenParticle::EmitSpawnOverMoving>(QT_TRANSLATE_NOOP("OpenParticleSystem", "Spawn Per Unit"), "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::Show)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &OpenParticle::EmitSpawnOverMoving::isProcessBurstList,
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "Process Burst List"), QT_TRANSLATE_NOOP("OpenParticleSystem", "Whether to process burst."))
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &OpenParticle::EmitSpawnOverMoving::isProcessSpawnRate,
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "Process Spawn Rate"), QT_TRANSLATE_NOOP("OpenParticleSystem", "Whether to process spawn rate."))
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &OpenParticle::EmitSpawnOverMoving::isIgnoreSpawnRateWhenMoving,
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "Ignore Spawn Rate When Moving"),
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "Whether to ignore spawn rate when moving."))
                    ->DataElement(
                        AZ_CRC("DistCtrlHandler"), &OpenParticle::EmitSpawnOverMoving::spawnRatePerUnitObject,
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "Spawn Rate Per Unit"), QT_TRANSLATE_NOOP("OpenParticleSystem", "The rate per unit distance of spawning particles."))
                        ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                        ->Attribute(AZ::Edit::Attributes::Max, 100000.f)
                        ->Attribute(AZ_CRC("Id"), azrtti_typeid<OpenParticle::EmitSpawnOverMoving>())
                        ->Attribute(AZ_CRC("ParamId"), AZ::TypeId::CreateName("spawnRatePerUnitObject"));

                editContext->Class<OpenParticle::ParticleEventHandler>(QT_TRANSLATE_NOOP("OpenParticleSystem", "Event Handler"), "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::Show)
                    ->DataElement(
                        AZ::Edit::UIHandlers::ComboBox, &OpenParticle::ParticleEventHandler::emitterName,
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "Emitter Name"),
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "The name of emitter whose event will be handled."))
                        ->Attribute(AZ::Edit::Attributes::StringList, &OpenParticle::ParticleEventHandler::GetEmitterNames)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &OpenParticle::ParticleEventHandler::OnEmitterNameChangedNotify)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &OpenParticle::ParticleEventHandler::eventType,
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "Event Type"), QT_TRANSLATE_NOOP("OpenParticleSystem", "The type of event which will be handled."))
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &OpenParticle::ParticleEventHandler::maxEventNum,
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "Max Event Number Per Frame"),
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "The max number of event to be handled."))
                        ->Attribute(AZ::Edit::Attributes::Min, 0)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &OpenParticle::ParticleEventHandler::emitNum,
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "Particle Number"), QT_TRANSLATE_NOOP("OpenParticleSystem", "The number of particles to be emitted."))
                        ->Attribute(AZ::Edit::Attributes::Min, 0)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &OpenParticle::ParticleEventHandler::useEventInfo,
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "Use Event Info"),
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "Write cover particle attributes with event info."));

                editContext->Class<OpenParticle::InheritanceHandler>(QT_TRANSLATE_NOOP("OpenParticleSystem", "Inheritance Handler"), "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::Show)
                    ->DataElement(
                        AZ::Edit::UIHandlers::ComboBox, &OpenParticle::InheritanceHandler::emitterName,
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "Emitter Name"),
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "The name of emitter which to be inherited."))
                        ->Attribute(AZ::Edit::Attributes::StringList, &OpenParticle::InheritanceHandler::GetEmitterNames)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &OpenParticle::InheritanceHandler::OnEmitterNameChangedNotify)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &OpenParticle::InheritanceHandler::spawnEnable,
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "Spawn Enable"),
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "Whether to enable spawn."))
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &OpenParticle::InheritanceHandler::calculateSpawnRate,
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "Calculate Spawn Rate"),
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "Whether to calculate spawn rate or spawn once."))
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::AttributesAndValues)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &OpenParticle::InheritanceHandler::spawnRate,
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "Spawn Rate"),
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "The spawn rate to calculate with source emitter."))
                        ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                        ->Attribute(AZ::Edit::Attributes::ReadOnly, &OpenParticle::InheritanceHandler::CalculateSpawnRate)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &OpenParticle::InheritanceHandler::applyPosition,
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "Apply Position"),
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "Whether to apply position."))
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::AttributesAndValues)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &OpenParticle::InheritanceHandler::positionOffset,
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "Position Offset"),
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "The offset of source emitter's position."))
                        ->Attribute(AZ::Edit::Attributes::ReadOnly, &OpenParticle::InheritanceHandler::PositionApplyed)
                        ->Attribute(AZ::Edit::Attributes::Min, -100000.f)
                        ->Attribute(AZ::Edit::Attributes::Max, 100000.f)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &OpenParticle::InheritanceHandler::applyVelocity,
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "Apply Velocity"),
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "Whether to apply velocity."))
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::AttributesAndValues)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &OpenParticle::InheritanceHandler::overwriteVelocity,
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "Overwrite Velocity"),
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "Whether to overwrite velocity."))
                        ->Attribute(AZ::Edit::Attributes::ReadOnly, &OpenParticle::InheritanceHandler::VelocityApplyed)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &OpenParticle::InheritanceHandler::velocityRatio,
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "Velocity Ratio"),
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "The ratio of source emitter's velocity."))
                        ->Attribute(AZ::Edit::Attributes::ReadOnly, &OpenParticle::InheritanceHandler::VelocityApplyed)
                        ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                        ->Attribute(AZ::Edit::Attributes::Max, 1000.f)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &OpenParticle::InheritanceHandler::applySize,
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "Apply Size"),
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "Whether to apply size."))
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &OpenParticle::InheritanceHandler::applyColorRGB,
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "Apply Color RGB"),
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "Whether to apply RGB of color."))
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::AttributesAndValues)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &OpenParticle::InheritanceHandler::applyColorAlpha,
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "Apply Color Alpha"),
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "Whether to apply alpha of color."))
                        ->Attribute(AZ::Edit::Attributes::ReadOnly, &OpenParticle::InheritanceHandler::ColorApplyed)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &OpenParticle::InheritanceHandler::colorRatio,
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "Color Ratio"),
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "The ratio of source emitter's color."))
                        ->Attribute(AZ::Edit::Attributes::ReadOnly, &OpenParticle::InheritanceHandler::ColorApplyed)
                        ->Attribute(AZ::Edit::Attributes::LabelForX, "R")
                        ->Attribute(AZ::Edit::Attributes::LabelForY, "G")
                        ->Attribute(AZ::Edit::Attributes::LabelForZ, "B")
                        ->Attribute(AZ::Edit::Attributes::LabelForW, "A")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                        ->Attribute(AZ::Edit::Attributes::Max, 1000.f)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &OpenParticle::InheritanceHandler::applyAge,
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "Apply Particle Age"),
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "Whether to apply age of particle."))
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &OpenParticle::InheritanceHandler::applyLifetime,
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "Apply Particle Lifetime"),
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "Whether to apply lifetime of particle."));

                editContext->Class<OpenParticle::SpawnColor>(QT_TRANSLATE_NOOP("OpenParticleSystem", "Start Color"), "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::Show)
                    ->DataElement(
                        AZ_CRC("DistCtrlHandler"), &OpenParticle::SpawnColor::startColorObject, QT_TRANSLATE_NOOP("OpenParticleSystem", "Color"),
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "The start color of partiles."))
                        ->Attribute(AZ_CRC("Id"), azrtti_typeid<OpenParticle::SpawnColor>())
                        ->Attribute(AZ_CRC("ParamId"), AZ::TypeId::CreateName("startColorObject"));

                editContext->Class<OpenParticle::SpawnLifetime>(QT_TRANSLATE_NOOP("OpenParticleSystem", "Particle Lifetime"), "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::Show)
                    ->DataElement(
                        AZ_CRC("DistCtrlHandler"), &OpenParticle::SpawnLifetime::lifeTimeObject, QT_TRANSLATE_NOOP("OpenParticleSystem", "Lifetime"),
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "The life time of particles."))
                        ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                        ->Attribute(AZ::Edit::Attributes::Max, 100000.f)
                        ->Attribute(AZ_CRC("Id"), azrtti_typeid<OpenParticle::SpawnLifetime>())
                        ->Attribute(AZ_CRC("ParamId"), AZ::TypeId::CreateName("lifeTimeObject"));

                editContext->Class<OpenParticle::SpawnLocBox>(QT_TRANSLATE_NOOP("OpenParticleSystem", "Box"), "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::Show)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Vector3, &OpenParticle::SpawnLocBox::center, QT_TRANSLATE_NOOP("OpenParticleSystem", "Position"),
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "The position of the box center."))
                    ->DataElement(
                        AZ::Edit::UIHandlers::Vector3, &OpenParticle::SpawnLocBox::size, QT_TRANSLATE_NOOP("OpenParticleSystem", "Size"),
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "The size of the box."))
                        ->Attribute(AZ::Edit::Attributes::Min, 0.f);

                editContext->Class<OpenParticle::SpawnLocPoint>(QT_TRANSLATE_NOOP("OpenParticleSystem", "Point"), "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::Show)
                    ->DataElement(
                        AZ_CRC("DistCtrlHandler"), &OpenParticle::SpawnLocPoint::positionObject, QT_TRANSLATE_NOOP("OpenParticleSystem", "Position"),
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "The position of the point center."))
                        ->Attribute(AZ::Edit::Attributes::Min, -100000.f)
                        ->Attribute(AZ::Edit::Attributes::Max, 100000.f)
                        ->Attribute(AZ_CRC("Id"), azrtti_typeid<OpenParticle::SpawnLocPoint>())
                        ->Attribute(AZ_CRC("ParamId"), AZ::TypeId::CreateName("positionObject"));

                editContext->Class<OpenParticle::SpawnLocSphere>(QT_TRANSLATE_NOOP("OpenParticleSystem", "Sphere"), "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::Show)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Vector3, &OpenParticle::SpawnLocSphere::center, QT_TRANSLATE_NOOP("OpenParticleSystem", "Position"),
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "The position of the sphere center."))
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &OpenParticle::SpawnLocSphere::axis, QT_TRANSLATE_NOOP("OpenParticleSystem", "Axis"),
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "The direction of the sphere."))
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &OpenParticle::SpawnLocSphere::angle, QT_TRANSLATE_NOOP("OpenParticleSystem", "Angle"),
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "The emit angle of the sphere."))
                        ->Attribute(AZ::Edit::Attributes::Suffix, QT_TRANSLATE_NOOP("OpenParticleSystem", "deg"))
                        ->Attribute(AZ::Edit::Attributes::Min, -360.f)
                        ->Attribute(AZ::Edit::Attributes::Max, 360.f)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &OpenParticle::SpawnLocSphere::radius, QT_TRANSLATE_NOOP("OpenParticleSystem", "Radius"),
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "The radius of the sphere."))
                        ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &OpenParticle::SpawnLocSphere::radiusThickness,
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "Radius Thickness"), QT_TRANSLATE_NOOP("OpenParticleSystem", "The radius thickness of the sphere."))
                        ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                        ->Attribute(AZ::Edit::Attributes::Max, 1.f)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &OpenParticle::SpawnLocSphere::ratio, QT_TRANSLATE_NOOP("OpenParticleSystem", "Ratio"),
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "The ratio of the sphere radius."))
                        ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                        ->Attribute(AZ::Edit::Attributes::Max, 1.f);

                editContext->Class<OpenParticle::SpawnLocSkeleton>(QT_TRANSLATE_NOOP("OpenParticleSystem", "Mesh"), "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::Show)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &OpenParticle::SpawnLocSkeleton::sampleType,
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "Mesh Sample Type"), QT_TRANSLATE_NOOP("OpenParticleSystem", "To use the Bone or Vertex of the mesh."))
                    ->DataElement(
                        AZ::Edit::UIHandlers::Vector3, &OpenParticle::SpawnLocSkeleton::scale, QT_TRANSLATE_NOOP("OpenParticleSystem", "Scale"),
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "Scale model size."));

                editContext->Class<OpenParticle::SpawnLocCylinder>(QT_TRANSLATE_NOOP("OpenParticleSystem", "Cylinder"), "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::Show)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &OpenParticle::SpawnLocCylinder::axis, QT_TRANSLATE_NOOP("OpenParticleSystem", "Axis"),
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "The direction of the cylinder."))
                    ->DataElement(
                        AZ::Edit::UIHandlers::Vector3, &OpenParticle::SpawnLocCylinder::center, QT_TRANSLATE_NOOP("OpenParticleSystem", "Position"),
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "The position of the cylinder center."))
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &OpenParticle::SpawnLocCylinder::radius, QT_TRANSLATE_NOOP("OpenParticleSystem", "Radius"),
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "The radius of the cylinder."))
                        ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &OpenParticle::SpawnLocCylinder::height, QT_TRANSLATE_NOOP("OpenParticleSystem", "Height"),
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "The height of the cylinder."))
                        ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &OpenParticle::SpawnLocCylinder::angle, QT_TRANSLATE_NOOP("OpenParticleSystem", "Angle"),
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "The emit angle of the cylinder."))
                        ->Attribute(AZ::Edit::Attributes::Suffix, QT_TRANSLATE_NOOP("OpenParticleSystem", "deg"))
                        ->Attribute(AZ::Edit::Attributes::Min, -360.f)
                        ->Attribute(AZ::Edit::Attributes::Max, 360.f)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &OpenParticle::SpawnLocCylinder::radiusThickness,
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "Radius Thickness"), QT_TRANSLATE_NOOP("OpenParticleSystem", "The radius thickness of the cylinder."))
                        ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                        ->Attribute(AZ::Edit::Attributes::Max, 1.f);

                editContext->Class<OpenParticle::SpawnLocTorus>(QT_TRANSLATE_NOOP("OpenParticleSystem", "Torus"), "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::Show)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Vector3, &OpenParticle::SpawnLocTorus::center, QT_TRANSLATE_NOOP("OpenParticleSystem", "Center"),
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "The position of the torus center."))
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &OpenParticle::SpawnLocTorus::torusAxis, QT_TRANSLATE_NOOP("OpenParticleSystem", "Torus Axis"),
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "The direction of the torus."))
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &OpenParticle::SpawnLocTorus::torusRadius, QT_TRANSLATE_NOOP("OpenParticleSystem", "Torus Radius"),
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "The distance from the center of the tube to the center of the torus."))
                        ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &OpenParticle::SpawnLocTorus::tubeRadius, QT_TRANSLATE_NOOP("OpenParticleSystem", "Tube Radius"),
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "The radius of the tube."))
                        ->Attribute(AZ::Edit::Attributes::Min, 0.f);

                editContext->Class<OpenParticle::SpawnSize>(QT_TRANSLATE_NOOP("OpenParticleSystem", "Start Size"), "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::Show)
                    ->DataElement(
                        AZ_CRC("DistCtrlHandler"), &OpenParticle::SpawnSize::sizeObject, QT_TRANSLATE_NOOP("OpenParticleSystem", "Size"),
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "The start size of particles."))
                        ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                        ->Attribute(AZ::Edit::Attributes::Max, 100000.f)
                        ->Attribute(AZ_CRC("Id"), azrtti_typeid<OpenParticle::SpawnSize>())
                        ->Attribute(AZ_CRC("ParamId"), AZ::TypeId::CreateName("sizeObject"));

                editContext->Class<OpenParticle::SpawnVelDirection>(QT_TRANSLATE_NOOP("OpenParticleSystem", "Start Velocity"), "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::Show)
                    ->DataElement(
                        AZ_CRC("DistCtrlHandler"), &OpenParticle::SpawnVelDirection::strengthObject,
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "Velocity strength"), QT_TRANSLATE_NOOP("OpenParticleSystem", "The start velocity strength of particles."))
                        ->Attribute(AZ::Edit::Attributes::Min, -100000.f)
                        ->Attribute(AZ::Edit::Attributes::Max, 100000.f)
                        ->Attribute(AZ_CRC("Id"), azrtti_typeid<OpenParticle::SpawnVelDirection>())
                        ->Attribute(AZ_CRC("ParamId"), AZ::TypeId::CreateName("strengthObject"))
                    ->DataElement(
                        AZ_CRC("DistCtrlHandler"), &OpenParticle::SpawnVelDirection::directionObject,
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "Velocity direction"), QT_TRANSLATE_NOOP("OpenParticleSystem", "The start velocity direction of particles."))
                        ->Attribute(AZ::Edit::Attributes::Min, -100000.f)
                        ->Attribute(AZ::Edit::Attributes::Max, 100000.f)
                        ->Attribute(AZ_CRC("Id"), azrtti_typeid<OpenParticle::SpawnVelDirection>())
                        ->Attribute(AZ_CRC("ParamId"), AZ::TypeId::CreateName("directionObject"));

                editContext->Class<OpenParticle::SpawnVelSphere>(QT_TRANSLATE_NOOP("OpenParticleSystem", "Velocity Sphere"), "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::Show)
                    ->DataElement(
                        AZ_CRC("DistCtrlHandler"), &OpenParticle::SpawnVelSphere::strengthObject,
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "Velocity strength"), QT_TRANSLATE_NOOP("OpenParticleSystem", "The velocity strength of particles."))
                        ->Attribute(AZ::Edit::Attributes::Min, -100000.f)
                        ->Attribute(AZ::Edit::Attributes::Max, 100000.f)
                        ->Attribute(AZ_CRC("Id"), azrtti_typeid<OpenParticle::SpawnVelSphere>())
                        ->Attribute(AZ_CRC("ParamId"), AZ::TypeId::CreateName("strengthObject"));

                editContext->Class<OpenParticle::SpawnVelConcentrate>(QT_TRANSLATE_NOOP("OpenParticleSystem", "Velocity Concentrate"), "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::Show)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default,
                        &OpenParticle::SpawnVelConcentrate::centre,
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "Center"),
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "The center of the Concentrate."))
                    ->DataElement(
                        AZ_CRC("DistCtrlHandler"), &OpenParticle::SpawnVelConcentrate::rateObject, QT_TRANSLATE_NOOP("OpenParticleSystem", "Rate"),
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "The rate of Concentrate."))
                        ->Attribute(AZ::Edit::Attributes::Min, -100000.f)
                        ->Attribute(AZ::Edit::Attributes::Max, 100000.f)
                        ->Attribute(AZ_CRC("Id"), azrtti_typeid<OpenParticle::SpawnVelConcentrate>())
                        ->Attribute(AZ_CRC("ParamId"), AZ::TypeId::CreateName("rateObject"));

                editContext->Class<OpenParticle::SpawnVelSector>(QT_TRANSLATE_NOOP("OpenParticleSystem", "Velocity Sector"), "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::Show)
                    ->DataElement(
                        AZ_CRC("DistCtrlHandler"), &OpenParticle::SpawnVelSector::strengthObject,
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "Velocity strength"), QT_TRANSLATE_NOOP("OpenParticleSystem", "The velocity strength of particles."))
                        ->Attribute(AZ::Edit::Attributes::Min, -100000.f)
                        ->Attribute(AZ::Edit::Attributes::Max, 100000.f)
                        ->Attribute(AZ_CRC("Id"), azrtti_typeid<OpenParticle::SpawnVelSector>())
                        ->Attribute(AZ_CRC("ParamId"), AZ::TypeId::CreateName("strengthObject"))
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &OpenParticle::SpawnVelSector::centralAngle, QT_TRANSLATE_NOOP("OpenParticleSystem", "CentralAngle"),
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "The central angle of the sector."))
                        ->Attribute(AZ::Edit::Attributes::Suffix, QT_TRANSLATE_NOOP("OpenParticleSystem", "deg"))
                        ->Attribute(AZ::Edit::Attributes::Min, -360.f)
                        ->Attribute(AZ::Edit::Attributes::Max, 360.f)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &OpenParticle::SpawnVelSector::rotateAngle,
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "RotateAngle"), QT_TRANSLATE_NOOP("OpenParticleSystem", "rotate the sector with this around sector central axis."))
                        ->Attribute(AZ::Edit::Attributes::Suffix, QT_TRANSLATE_NOOP("OpenParticleSystem", "deg"))
                        ->Attribute(AZ::Edit::Attributes::Min, -360.f)
                        ->Attribute(AZ::Edit::Attributes::Max, 360.f)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Vector3, &OpenParticle::SpawnVelSector::direction, QT_TRANSLATE_NOOP("OpenParticleSystem", "Axis"),
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "The direction of the sector plane."))
                        ->Attribute(AZ::Edit::Attributes::Min, -100000.f)
                        ->Attribute(AZ::Edit::Attributes::Max, 100000.f);

                editContext->Class<OpenParticle::SpawnVelCone>(QT_TRANSLATE_NOOP("OpenParticleSystem", "Velocity Cone"), "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::Show)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &OpenParticle::SpawnVelCone::angle, QT_TRANSLATE_NOOP("OpenParticleSystem", "Angle"),
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "The emit angle of the cone."))
                        ->Attribute(AZ::Edit::Attributes::Suffix, QT_TRANSLATE_NOOP("OpenParticleSystem", "deg"))
                        ->Attribute(AZ::Edit::Attributes::Min, -360.f)
                        ->Attribute(AZ::Edit::Attributes::Max, 360.f)
                    ->DataElement(
                        AZ_CRC("DistCtrlHandler"), &OpenParticle::SpawnVelCone::strengthObject,
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "Velocity strength"), QT_TRANSLATE_NOOP("OpenParticleSystem", "The velocity strength of particles."))
                        ->Attribute(AZ::Edit::Attributes::Min, -100000.f)
                        ->Attribute(AZ::Edit::Attributes::Max, 100000.f)
                        ->Attribute(AZ_CRC("Id"), azrtti_typeid<OpenParticle::SpawnVelCone>())
                        ->Attribute(AZ_CRC("ParamId"), AZ::TypeId::CreateName("strengthObject"))
                    ->DataElement(
                        AZ::Edit::UIHandlers::Vector3, &OpenParticle::SpawnVelCone::direction, QT_TRANSLATE_NOOP("OpenParticleSystem", "Axis"),
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "The direction of the cone."))
                        ->Attribute(AZ::Edit::Attributes::Min, -100000.f)
                        ->Attribute(AZ::Edit::Attributes::Max, 100000.f);

                editContext->Class<OpenParticle::SpawnRotation>(QT_TRANSLATE_NOOP("OpenParticleSystem", "Start Rotation"), "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::Show)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Vector3, &OpenParticle::SpawnRotation::initAxis, QT_TRANSLATE_NOOP("OpenParticleSystem", "Init Axis"),
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "The initial rotate axis of particles."))
                        ->Attribute(AZ::Edit::Attributes::Min, -100000.f)
                        ->Attribute(AZ::Edit::Attributes::Max, 100000.f)
                    ->DataElement(
                        AZ_CRC("DistCtrlHandler"), &OpenParticle::SpawnRotation::initAngleObject, QT_TRANSLATE_NOOP("OpenParticleSystem", "Init Angle"),
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "The initial angle of particles."))
                        ->Attribute(AZ::Edit::Attributes::Suffix, QT_TRANSLATE_NOOP("OpenParticleSystem", "deg"))
                        ->Attribute(AZ::Edit::Attributes::Min, -360.f)
                        ->Attribute(AZ::Edit::Attributes::Max, 360.f)
                        ->Attribute(AZ_CRC("Id"), azrtti_typeid<OpenParticle::SpawnRotation>())
                        ->Attribute(AZ_CRC("ParamId"), AZ::TypeId::CreateName("initAngleObject"))
                    ->DataElement(
                            AZ::Edit::UIHandlers::Vector3, &OpenParticle::SpawnRotation::rotateAxis, QT_TRANSLATE_NOOP("OpenParticleSystem", "Rotate Axis"),
                            QT_TRANSLATE_NOOP("OpenParticleSystem", "The rotate axis of particles."))
                            ->Attribute(AZ::Edit::Attributes::Min, -100000.f)
                            ->Attribute(AZ::Edit::Attributes::Max, 100000.f)
                    ->DataElement(AZ_CRC("DistCtrlHandler"), &OpenParticle::SpawnRotation::rotateSpeedObject,
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "Rotate Speed"), QT_TRANSLATE_NOOP("OpenParticleSystem", "The rotation rate of particles."))
                        ->Attribute(AZ::Edit::Attributes::Min, -100000.f)
                        ->Attribute(AZ::Edit::Attributes::Max, 100000.f)
                        ->Attribute(AZ_CRC("Id"), azrtti_typeid<OpenParticle::SpawnRotation>())
                        ->Attribute(AZ_CRC("ParamId"), AZ::TypeId::CreateName("rotateSpeedObject"));

                editContext->Class<OpenParticle::SpawnLightEffect>(QT_TRANSLATE_NOOP("OpenParticleSystem", "Light"), "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::Show)
                    ->DataElement(
                        AZ_CRC("DistCtrlHandler"), &OpenParticle::SpawnLightEffect::lightColorObject, QT_TRANSLATE_NOOP("OpenParticleSystem", "Color"),
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "The color of light."))
                        ->Attribute(AZ_CRC("Id"), azrtti_typeid<OpenParticle::SpawnLightEffect>())
                        ->Attribute(AZ_CRC("ParamId"), AZ::TypeId::CreateName("lightColorObject"))
                    ->DataElement(
                        AZ_CRC("DistCtrlHandler"), &OpenParticle::SpawnLightEffect::intensityObject,
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "Intensity"),
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "The intensity of light."))
                        ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                        ->Attribute(AZ::Edit::Attributes::Max, 1000.f)
                        ->Attribute(AZ_CRC("Id"), azrtti_typeid<OpenParticle::SpawnLightEffect>())
                        ->Attribute(AZ_CRC("ParamId"), AZ::TypeId::CreateName("intensityObject"))
                    ->DataElement(
                        AZ_CRC("DistCtrlHandler"), &OpenParticle::SpawnLightEffect::radianScaleObject,
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "Radius"),
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "The radius of light."))
                        ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                        ->Attribute(AZ::Edit::Attributes::Max, 100000.f)
                        ->Attribute(AZ_CRC("Id"), azrtti_typeid<OpenParticle::SpawnLightEffect>())
                        ->Attribute(AZ_CRC("ParamId"), AZ::TypeId::CreateName("radianScaleObject"));

                editContext->Class<OpenParticle::SpawnLocationEvent>(QT_TRANSLATE_NOOP("OpenParticleSystem", "Spawn Event"), "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::Show)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &OpenParticle::SpawnLocationEvent::whetherSendEvent,
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "Send Spawn Event"), QT_TRANSLATE_NOOP("OpenParticleSystem", "Whether to send spawn event."));

                editContext->Class<OpenParticle::UpdateConstForce>(QT_TRANSLATE_NOOP("OpenParticleSystem", "Acceleration"), "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::Show)
                    ->DataElement(
                        AZ_CRC("DistCtrlHandler"), &OpenParticle::UpdateConstForce::forceObject,
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "Acceleration"),
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "The acceleration of particles."))
                        ->Attribute(AZ::Edit::Attributes::Min, -100000.f)
                        ->Attribute(AZ::Edit::Attributes::Max, 100000.f)
                        ->Attribute(AZ_CRC("Id"), azrtti_typeid<OpenParticle::UpdateConstForce>())
                        ->Attribute(AZ_CRC("ParamId"), AZ::TypeId::CreateName("forceObject"));

                editContext->Class<OpenParticle::UpdateDragForce>(QT_TRANSLATE_NOOP("OpenParticleSystem", "Drag"), "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::Show)
                    ->DataElement(
                        AZ_CRC("DistCtrlHandler"), &OpenParticle::UpdateDragForce::dragCoefficientObject,
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "Drag Coefficient"), QT_TRANSLATE_NOOP("OpenParticleSystem", "The drag force of particles."))
                        ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                        ->Attribute(AZ::Edit::Attributes::Max, 100000.f)
                        ->Attribute(AZ_CRC("Id"), azrtti_typeid<OpenParticle::UpdateDragForce>())
                        ->Attribute(AZ_CRC("ParamId"), AZ::TypeId::CreateName("dragCoefficientObject"));

                editContext->Class<OpenParticle::UpdateVortexForce>(QT_TRANSLATE_NOOP("OpenParticleSystem", "Vortex Force"), "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::Show)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &OpenParticle::UpdateVortexForce::origin,
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "Origin"),
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "The centre of the Vortex."))
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &OpenParticle::UpdateVortexForce::vortexAxis,
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "Vortex Axis"),
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "Vortex Axis."))
                    ->DataElement(
                        AZ_CRC("DistCtrlHandler"), &OpenParticle::UpdateVortexForce::originPullObject,
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "Origin Pull"),
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "The tensile force of the origin."))
                        ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                        ->Attribute(AZ::Edit::Attributes::Max, 100000.f)
                        ->Attribute(AZ_CRC("Id"), azrtti_typeid<OpenParticle::UpdateVortexForce>())
                        ->Attribute(AZ_CRC("ParamId"), AZ::TypeId::CreateName("originPullObject"))
                    ->DataElement(
                        AZ_CRC("DistCtrlHandler"), &OpenParticle::UpdateVortexForce::vortexRateObject,
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "Vortex Rate"),
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "The rate of vortex."))
                        ->Attribute(AZ::Edit::Attributes::Min, -100000.f)
                        ->Attribute(AZ::Edit::Attributes::Max, 100000.f)
                        ->Attribute(AZ_CRC("Id"), azrtti_typeid<OpenParticle::UpdateVortexForce>())
                        ->Attribute(AZ_CRC("ParamId"), AZ::TypeId::CreateName("vortexRateObject"))
                    ->DataElement(
                        AZ_CRC("DistCtrlHandler"), &OpenParticle::UpdateVortexForce::vortexRadiusObject,
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "Vortex Radius"),
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "The radius of a particle in the final steady state of rotation."))
                        ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                        ->Attribute(AZ::Edit::Attributes::Max, 100000.f)
                        ->Attribute(AZ_CRC("Id"), azrtti_typeid<OpenParticle::UpdateVortexForce>())
                        ->Attribute(AZ_CRC("ParamId"), AZ::TypeId::CreateName("vortexRadiusObject"));

                editContext->Class<OpenParticle::UpdateCurlNoiseForce>(QT_TRANSLATE_NOOP("OpenParticleSystem", "Noise"), "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::Show)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &OpenParticle::UpdateCurlNoiseForce::noiseStrength,
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "Noise Strength"), QT_TRANSLATE_NOOP("OpenParticleSystem", "The noise strength of particles."))
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &OpenParticle::UpdateCurlNoiseForce::noiseFrequency,
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "Noise Frequency"), QT_TRANSLATE_NOOP("OpenParticleSystem", "The noise frequency of particles."))
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &OpenParticle::UpdateCurlNoiseForce::panNoise,
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "Pan Noise"), QT_TRANSLATE_NOOP("OpenParticleSystem", "The pan noise of particles."))
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &OpenParticle::UpdateCurlNoiseForce::panNoiseField,
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "Pan Noise Field"), QT_TRANSLATE_NOOP("OpenParticleSystem", "The pan noise field of particles."))
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &OpenParticle::UpdateCurlNoiseForce::randomSeed,
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "Random Seed"), QT_TRANSLATE_NOOP("OpenParticleSystem", "The random seed of particles."))
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &OpenParticle::UpdateCurlNoiseForce::randomizationVector,
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "Randomization Vector"), QT_TRANSLATE_NOOP("OpenParticleSystem", "The randomization vector of particles."));

                editContext->Class<OpenParticle::UpdateColor>(QT_TRANSLATE_NOOP("OpenParticleSystem", "Color Over Lifetime"), "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::Show)
                    ->DataElement(AZ_CRC("GradientColor"), &OpenParticle::UpdateColor::currentColorObject,
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "Color"), QT_TRANSLATE_NOOP("OpenParticleSystem", "The color of particles changes with particle life time."))
                        ->Attribute(AZ_CRC("Id"), azrtti_typeid<OpenParticle::UpdateColor>())
                        ->Attribute(AZ_CRC("ParamId"), AZ::TypeId::CreateName("currentColorObject"));

                editContext->Class<OpenParticle::UpdateLocationEvent>(QT_TRANSLATE_NOOP("OpenParticleSystem", "Location Event"), "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::Show)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &OpenParticle::UpdateLocationEvent::whetherSendEvent,
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "Send Location Event"),
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "Whether to send location event."));

                editContext->Class<OpenParticle::UpdateDeathEvent>(QT_TRANSLATE_NOOP("OpenParticleSystem", "Death Event"), "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::Show)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &OpenParticle::UpdateDeathEvent::whetherSendEvent,
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "Send Death Event"), QT_TRANSLATE_NOOP("OpenParticleSystem", "Whether to send death event."));

                editContext->Class<OpenParticle::UpdateCollisionEvent>(QT_TRANSLATE_NOOP("OpenParticleSystem", "Collision Event"), "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::Show)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &OpenParticle::UpdateCollisionEvent::whetherSendEvent,
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "Send Collision Event"), QT_TRANSLATE_NOOP("OpenParticleSystem", "Whether to send collision event."));

                editContext->Class<OpenParticle::UpdateInheritanceEvent>(QT_TRANSLATE_NOOP("OpenParticleSystem", "Inheritance Event"), "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::Show)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &OpenParticle::UpdateInheritanceEvent::whetherSendEvent,
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "Send Inheritance Event"), QT_TRANSLATE_NOOP("OpenParticleSystem", "Whether to send inheritance event."));

                editContext->Class<OpenParticle::UpdateSizeLinear>(QT_TRANSLATE_NOOP("OpenParticleSystem", "Size Over Lifetime"), "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::Show)
                    ->DataElement(
                        AZ_CRC("DistCtrlHandler"), &OpenParticle::UpdateSizeLinear::sizeObject, QT_TRANSLATE_NOOP("OpenParticleSystem", "Size"),
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "The size of particles changes with particle life time."))
                        ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                        ->Attribute(AZ::Edit::Attributes::Max, 100000.f)
                        ->Attribute(AZ_CRC("Id"), azrtti_typeid<OpenParticle::UpdateSizeLinear>())
                        ->Attribute(AZ_CRC("ParamId"), AZ::TypeId::CreateName("sizeObject"));

                editContext->Class<OpenParticle::UpdateSizeByVelocity>(QT_TRANSLATE_NOOP("OpenParticleSystem", "Size By Speed"), "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::Show)
                    ->DataElement(
                        AZ_CRC("DistCtrlHandler"), &OpenParticle::UpdateSizeByVelocity::velScaleObject,
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "Scale Factor"), QT_TRANSLATE_NOOP("OpenParticleSystem", "The size scale factor of particles."))
                        ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                        ->Attribute(AZ_CRC("Id"), azrtti_typeid<OpenParticle::UpdateSizeByVelocity>())
                        ->Attribute(AZ_CRC("ParamId"), AZ::TypeId::CreateName("velScaleObject"))
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &OpenParticle::UpdateSizeByVelocity::velocityRange,
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "Velocity Range"), QT_TRANSLATE_NOOP("OpenParticleSystem", "The maximum value of the velocity size."))
                        ->Attribute(AZ::Edit::Attributes::Min, 0.f);

                editContext->Class<OpenParticle::SizeScale>(QT_TRANSLATE_NOOP("OpenParticleSystem", "Size Scale"), "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::Show)
                    ->DataElement(
                        AZ_CRC("DistCtrlHandler"), &OpenParticle::SizeScale::scaleFactorObject,
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "Scale Factor"), QT_TRANSLATE_NOOP("OpenParticleSystem", "The size scale factor of particles."))
                        ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                        ->Attribute(AZ::Edit::Attributes::Max, 100000.f)
                        ->Attribute(AZ_CRC("Id"), azrtti_typeid<OpenParticle::SizeScale>())
                        ->Attribute(AZ_CRC("ParamId"), AZ::TypeId::CreateName("scaleFactorObject"));

                editContext->Class<OpenParticle::UpdateSubUv>(QT_TRANSLATE_NOOP("OpenParticleSystem", "SubUV"), "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::Show)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &OpenParticle::UpdateSubUv::frameNum, QT_TRANSLATE_NOOP("OpenParticleSystem", "Frame Number"),
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "The number of frames in the texture."))
                        ->Attribute(AZ::Edit::Attributes::Min, 1)
                        ->Attribute(AZ::Edit::Attributes::Max, 1000000)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &OpenParticle::UpdateSubUv::framePerSecond,
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "Frame Per Second"), QT_TRANSLATE_NOOP("OpenParticleSystem", "The frames per second."))
                        ->Attribute(AZ::Edit::Attributes::Min, 0)
                        ->Attribute(AZ::Edit::Attributes::Max, 1000000)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &OpenParticle::UpdateSubUv::spawnOnly,
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "Spawn Only"), QT_TRANSLATE_NOOP("OpenParticleSystem", "Only update subUV once when particle spawn, FramePerSecond option would be ignored"))
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &OpenParticle::UpdateSubUv::IndexByEventOrder,
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "Index By Event Order"),
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "Only update subUV once when particle spawn, Index subUV by the order of event generation which trigger particle spawn."))
                        ->Attribute(AZ::Edit::Attributes::Visibility, &OpenParticle::UpdateSubUv::spawnOnly);

                editContext->Class<OpenParticle::UpdateRotateAroundPoint>(QT_TRANSLATE_NOOP("OpenParticleSystem", "Velocity Rotate Around Point"), "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::Show)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &OpenParticle::UpdateRotateAroundPoint::rotateRate,
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "Rotate Rate"),
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "Rotate Rate."))
                        ->Attribute(AZ::Edit::Attributes::Min, -100000.f)
                        ->Attribute(AZ::Edit::Attributes::Max, 100000.f)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &OpenParticle::UpdateRotateAroundPoint::radius,
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "Radius"), QT_TRANSLATE_NOOP("OpenParticleSystem", "The radius of the circle."))
                        ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &OpenParticle::UpdateRotateAroundPoint::xAxis,
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "First Axis"),
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "The First Axis."))
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &OpenParticle::UpdateRotateAroundPoint::yAxis,
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "Second Axis"),
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "The second Axis."))
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &OpenParticle::UpdateRotateAroundPoint::center,
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "Center"),
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "The center of the circle."));

                editContext->Class<OpenParticle::UpdateVelocity>(QT_TRANSLATE_NOOP("OpenParticleSystem", "Velocity Over Lifetime"), "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::Show)
                    ->DataElement(AZ_CRC("DistCtrlHandler"), &OpenParticle::UpdateVelocity::strengthObject,
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "Velocity strength"), QT_TRANSLATE_NOOP("OpenParticleSystem", "The velocity strength of particles changes with particle life time."))
                        ->Attribute(AZ::Edit::Attributes::Min, -100000.f)
                        ->Attribute(AZ::Edit::Attributes::Max, 100000.f)
                        ->Attribute(AZ_CRC("Id"), azrtti_typeid<OpenParticle::UpdateVelocity>())
                        ->Attribute(AZ_CRC("ParamId"), AZ::TypeId::CreateName("strengthObject"))
                    ->DataElement(AZ_CRC("DistCtrlHandler"), &OpenParticle::UpdateVelocity::directionObject,
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "Velocity direction"), QT_TRANSLATE_NOOP("OpenParticleSystem", "The velocity direction of particles changes with particle life time."))
                        ->Attribute(AZ::Edit::Attributes::Min, -100000.f)
                        ->Attribute(AZ::Edit::Attributes::Max, 100000.f)
                        ->Attribute(AZ_CRC("Id"), azrtti_typeid<OpenParticle::UpdateVelocity>())
                        ->Attribute(AZ_CRC("ParamId"), AZ::TypeId::CreateName("directionObject"));

                editContext->Class<OpenParticle::CollisionPlane>(QT_TRANSLATE_NOOP("OpenParticleSystem", "Particle Collision"), "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::Show)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &OpenParticle::CollisionPlane::normal, QT_TRANSLATE_NOOP("OpenParticleSystem", "Plane Normal"),
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "The normal of plane."))
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &OpenParticle::CollisionPlane::position, QT_TRANSLATE_NOOP("OpenParticleSystem", "Plane Position"),
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "The position of plane."));

                editContext->Class<OpenParticle::CollisionRadius>(QT_TRANSLATE_NOOP("OpenParticleSystem", "CollisionRadius"), QT_TRANSLATE_NOOP("OpenParticleSystem", "The collision radius of particles."))
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::Show)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &OpenParticle::CollisionRadius::type, QT_TRANSLATE_NOOP("OpenParticleSystem", "Calculation Type"),
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "The radius calculation type of particles."))
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &OpenParticle::CollisionRadius::method, QT_TRANSLATE_NOOP("OpenParticleSystem", "Calculation Method"),
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "The radius calculation method of particles."))
                    ->DataElement(AZ::Edit::UIHandlers::Default, &OpenParticle::CollisionRadius::radius, QT_TRANSLATE_NOOP("OpenParticleSystem", "Radius"),
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "The radius of particles."))
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Visibility, &OpenParticle::CollisionRadius::RadiusVisibility)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &OpenParticle::CollisionRadius::radiusScale, QT_TRANSLATE_NOOP("OpenParticleSystem", "Radius Scale"),
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "The radius scale of particles."))
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f);

                editContext->Class<OpenParticle::Bounce>(QT_TRANSLATE_NOOP("OpenParticleSystem", "Bounce"), QT_TRANSLATE_NOOP("OpenParticleSystem", "The bounce of plane."))
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::Show)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &OpenParticle::Bounce::restitution,
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "Restitution"), QT_TRANSLATE_NOOP("OpenParticleSystem", "The restitution of plane."))
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &OpenParticle::Bounce::randomizeNormal,
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "Randomize Normal"), QT_TRANSLATE_NOOP("OpenParticleSystem", "The randomize normal of plane."))
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 1.0f);

                editContext->Class<OpenParticle::ParticleCollision>(QT_TRANSLATE_NOOP("OpenParticleSystem", "Particle Collision"), "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                         ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                         ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::Show)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &OpenParticle::ParticleCollision::collisionType, QT_TRANSLATE_NOOP("OpenParticleSystem", "Collision Type"),
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "The collision type of particles."))
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &OpenParticle::ParticleCollision::collisionRadius, QT_TRANSLATE_NOOP("OpenParticleSystem", "Collision Radius"),
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "The collision radius of particles."))
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &OpenParticle::ParticleCollision::bounce, QT_TRANSLATE_NOOP("OpenParticleSystem", "Bounce"),
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "The bounce of plane."))
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &OpenParticle::ParticleCollision::useTwoPlane, QT_TRANSLATE_NOOP("OpenParticleSystem", "UseTwoPlane"),
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "whether to use the plane 2."))
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &OpenParticle::ParticleCollision::collisionPlane1, QT_TRANSLATE_NOOP("OpenParticleSystem", "Collision Plane1"),
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "The collision plane 1."))
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &OpenParticle::ParticleCollision::collisionPlane2, QT_TRANSLATE_NOOP("OpenParticleSystem", "Collision Plane2"),
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "The collision plane 2."))
                        ->Attribute(AZ::Edit::Attributes::Visibility, &ParticleCollision::UseTwoPlaneIsSelected);

                editContext->Class<OpenParticle::SpriteConfig>(QT_TRANSLATE_NOOP("OpenParticleSystem", "Sprite Renderer"), "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::Show)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &OpenParticle::SpriteConfig::facing, QT_TRANSLATE_NOOP("OpenParticleSystem", "Render Alignment"),
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "The alignment type of particles."))
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &OpenParticle::SpriteConfig::sortId, QT_TRANSLATE_NOOP("OpenParticleSystem", "Render Sort"),
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "According to this this parameter to render the material."))
                        ->Attribute(AZ::Edit::Attributes::Min, 0u)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Vector2, &OpenParticle::SpriteConfig::subImageSize,
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "Sub Image Size"), QT_TRANSLATE_NOOP("OpenParticleSystem", "The size of the image."))
                        ->Attribute(AZ::Edit::Attributes::Min, 1.f);

                editContext->Class<OpenParticle::MeshConfig>(QT_TRANSLATE_NOOP("OpenParticleSystem", "Mesh Renderer"), "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::Show)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &OpenParticle::MeshConfig::facing, QT_TRANSLATE_NOOP("OpenParticleSystem", "Render Alignment"),
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "The alignment type of particles."))
                        ->Attribute(AZ::Edit::Attributes::ReadOnly, true)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &OpenParticle::MeshConfig::sortId, QT_TRANSLATE_NOOP("OpenParticleSystem", "Render Sort"),
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "According to this this parameter to render the material."));

                editContext->Class<OpenParticle::TrailParam>(QT_TRANSLATE_NOOP("OpenParticleSystem", "TrailParam"), QT_TRANSLATE_NOOP("OpenParticleSystem", "Trail Parameter"))
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::Show)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &OpenParticle::TrailParam::ratio,
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "Ratio"),
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "Proportion of particles with tail"))
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &OpenParticle::TrailParam::lifetime,
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "Lifetime"), QT_TRANSLATE_NOOP("OpenParticleSystem", "Life time of trails"))
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &OpenParticle::TrailParam::inheritLifetime,
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "Inherit Lifetime"), QT_TRANSLATE_NOOP("OpenParticleSystem", "Inherit parent particle's lifetime."))
                    ->DataElement(AZ::Edit::UIHandlers::Default, &OpenParticle::TrailParam::dieWithParticles,
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "Die With Particles"),
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "Trials die with particles die"));

                editContext->Class<OpenParticle::RibbonParam>(QT_TRANSLATE_NOOP("OpenParticleSystem", "RibbonParam"), QT_TRANSLATE_NOOP("OpenParticleSystem", "Ribbon Parameter"))
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::Show)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &OpenParticle::RibbonParam::ribbonCount,
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "Ribbon Count"), QT_TRANSLATE_NOOP("OpenParticleSystem", "the count of ribbons"))
                        ->Attribute(AZ::Edit::Attributes::Min, 1u);

                editContext->Class<OpenParticle::RibbonConfig>(QT_TRANSLATE_NOOP("OpenParticleSystem", "Ribbon Renderer"), "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::Show)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &OpenParticle::RibbonConfig::trailParam,
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "Trail Parameter"),
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "The alignment type of particles."))
                         ->Attribute(AZ::Edit::Attributes::Visibility, &RibbonConfig::ModeChangToTrail)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &OpenParticle::RibbonConfig::ribbonParam,
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "Ribbon Parameter"),
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "The alignment type of particles."))
                        ->Attribute(AZ::Edit::Attributes::Visibility, &RibbonConfig::ModeChangToRibbon)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &OpenParticle::RibbonConfig::sortId,
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "Render Sort"),
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "According to this this parameter to render the material."))
                        ->Attribute(AZ::Edit::Attributes::Min, 0u)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &OpenParticle::RibbonConfig::minRibbonSegmentLength,
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "Min Segment Length"),
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "The minimum length between two particles in ribbon."))
                        ->Attribute(AZ::Edit::Attributes::Min, 1.f - SimuCore::ALMOST_ONE)
                    ->DataElement(
                        AZ_CRC("DistCtrlHandler"), &OpenParticle::RibbonConfig::ribbonWidthObject,
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "Ribbon Width"),
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "The width of the ribbon."))
                        ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                        ->Attribute(AZ_CRC("Id"), azrtti_typeid<OpenParticle::RibbonConfig>())
                        ->Attribute(AZ_CRC("ParamId"), AZ::TypeId::CreateName("ribbonWidthObject"))
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &OpenParticle::RibbonConfig::inheritSize,
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "Inherit Size"),
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "Inherit parent particle's size."))
                        ->Attribute(AZ::Edit::Attributes::Visibility, &RibbonConfig::ModeChangToTrail)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &OpenParticle::RibbonConfig::tesselationFactor,
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "Tesselation Factor"),
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "Each time a ribbon segment grows longer than this value, it will be subdivided.  Lower values "
                                          "result in more (smoother) subdivisions, more resource use."))
                        ->Attribute(AZ::Edit::Attributes::Min, 1.f - SimuCore::ALMOST_ONE) 
                    ->DataElement(AZ::Edit::UIHandlers::Default, &OpenParticle::RibbonConfig::curveTension,
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "Curve Tension"),
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "The curve tension of the ribbon."))
                        ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                        ->Attribute(AZ::Edit::Attributes::Max, SimuCore::ALMOST_ONE)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &OpenParticle::RibbonConfig::tilingDistance,
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "Tile Distance"),
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "The tile distance of the ribbon."))
                        ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &OpenParticle::RibbonConfig::facing,
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "Render Alignment"),
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "The alignment type of particles."))
                    ->DataElement(AZ::Edit::UIHandlers::Default, &OpenParticle::RibbonConfig::mode,
                        QT_TRANSLATE_NOOP("OpenParticleSystem", "Trail Mode"), QT_TRANSLATE_NOOP("OpenParticleSystem", "Type of trail."))
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree);
            }
        }
    }
} // namespace OpenParticle
