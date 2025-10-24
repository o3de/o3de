/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ParticleConfig.h"

#include <Atom/RPI.Edit/Common/AssetUtils.h>
#include <AzCore/Serialization/EditContext.h>

namespace OpenParticle
{
    void ParticleConfig::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<ParticleConfig>()
                ->Version(0);

            serializeContext->Class<SimuCore::ParticleCore::ParticleSystem::PreWarm>()
                ->Version(0)
                ->Field("warmupTime", &SimuCore::ParticleCore::ParticleSystem::PreWarm::warmupTime)
                ->Field("tickCount", &SimuCore::ParticleCore::ParticleSystem::PreWarm::tickCount)
                ->Field("tickDelta", &SimuCore::ParticleCore::ParticleSystem::PreWarm::tickDelta);

            serializeContext->Class<SimuCore::ParticleCore::ParticleSystem::Config>()
                ->Version(0)
                ->Field("loop", &SimuCore::ParticleCore::ParticleSystem::Config::loop)
                ->Field("parallel", &SimuCore::ParticleCore::ParticleSystem::Config::parallel);

            serializeContext->Class<SimuCore::ParticleCore::ParticleEmitter::Config>()
                ->Version(0)
                ->Field("type", &SimuCore::ParticleCore::ParticleEmitter::Config::type)
                ->Field("maxSize", &SimuCore::ParticleCore::ParticleEmitter::Config::maxSize)
                ->Field("startTime", &SimuCore::ParticleCore::ParticleEmitter::Config::startTime)
                ->Field("localSpace", &SimuCore::ParticleCore::ParticleEmitter::Config::localSpace)
                ->Field("duration", &SimuCore::ParticleCore::ParticleEmitter::Config::duration)
                ->Field("loop", &SimuCore::ParticleCore::ParticleEmitter::Config::loop);

            serializeContext->Enum<SimuCore::ParticleCore::SimulateType>()
                ->Value("CPU", SimuCore::ParticleCore::SimulateType::CPU)
                ->Value("GPU", SimuCore::ParticleCore::SimulateType::GPU);

            serializeContext->Class<SimuCore::ParticleCore::SingleBurst>()
                ->Version(0)
                ->Field("time", &SimuCore::ParticleCore::SingleBurst::time)
                ->Field("count", &SimuCore::ParticleCore::SingleBurst::count)
                ->Field("minCount", &SimuCore::ParticleCore::SingleBurst::minCount);

            serializeContext->Class<SimuCore::ParticleCore::EmitBurstList>()
                ->Version(0)
                ->Field("burstList", &SimuCore::ParticleCore::EmitBurstList::burstList)
                ->Field("arrSize", &SimuCore::ParticleCore::EmitBurstList::arrSize)
                ->Field("isProcessBurstList", &SimuCore::ParticleCore::EmitBurstList::isProcessBurstList);

            serializeContext->Class<SimuCore::ParticleCore::EmitSpawn>()
                ->Version(0)
                ->Field("isProcessSpawnRate", &SimuCore::ParticleCore::EmitSpawn::isProcessSpawnRate)
                ->Field("spawnRate", &SimuCore::ParticleCore::EmitSpawn::spawnRate);

            serializeContext->Class<SimuCore::ParticleCore::EmitSpawnOverMoving>()
                ->Version(0)
                ->Field("spawnRatePerUnit", &SimuCore::ParticleCore::EmitSpawnOverMoving::spawnRatePerUnit)
                ->Field("isProcessSpawnRate", &SimuCore::ParticleCore::EmitSpawnOverMoving::isProcessSpawnRate)
                ->Field("isProcessBurstList", &SimuCore::ParticleCore::EmitSpawnOverMoving::isProcessBurstList)
                ->Field("isIgnoreSpawnRateWhenMoving", &SimuCore::ParticleCore::EmitSpawnOverMoving::isIgnoreSpawnRateWhenMoving);

            serializeContext->Class<SimuCore::ParticleCore::ParticleEventHandler>()
                ->Version(0)
                ->Field("emitterName", &SimuCore::ParticleCore::ParticleEventHandler::emitterIndex)
                ->Field("eventType", &SimuCore::ParticleCore::ParticleEventHandler::eventType)
                ->Field("maxEventNum", &SimuCore::ParticleCore::ParticleEventHandler::maxEventNum)
                ->Field("emitNum", &SimuCore::ParticleCore::ParticleEventHandler::emitNum)
                ->Field("useEventInfo", &SimuCore::ParticleCore::ParticleEventHandler::useEventInfo);

            serializeContext->Class<SimuCore::ParticleCore::InheritanceHandler>()
                ->Version(0)
                ->Field("positionOffset", &SimuCore::ParticleCore::InheritanceHandler::positionOffset)
                ->Field("velocityRatio", &SimuCore::ParticleCore::InheritanceHandler::velocityRatio)
                ->Field("colorRatio", &SimuCore::ParticleCore::InheritanceHandler::colorRatio)
                ->Field("emitterIndex", &SimuCore::ParticleCore::InheritanceHandler::emitterIndex)
                ->Field("spawnRate", &SimuCore::ParticleCore::InheritanceHandler::spawnRate)
                ->Field("calculateSpawnRate", &SimuCore::ParticleCore::InheritanceHandler::calculateSpawnRate)
                ->Field("spawnEnable", &SimuCore::ParticleCore::InheritanceHandler::spawnEnable)
                ->Field("applyPosition", &SimuCore::ParticleCore::InheritanceHandler::applyPosition)
                ->Field("applyVelocity", &SimuCore::ParticleCore::InheritanceHandler::applyVelocity)
                ->Field("overwriteVelocity", &SimuCore::ParticleCore::InheritanceHandler::overwriteVelocity)
                ->Field("applySize", &SimuCore::ParticleCore::InheritanceHandler::applySize)
                ->Field("applyColorRGB", &SimuCore::ParticleCore::InheritanceHandler::applyColorRGB)
                ->Field("applyColorAlpha", &SimuCore::ParticleCore::InheritanceHandler::applyColorAlpha)
                ->Field("applyAge", &SimuCore::ParticleCore::InheritanceHandler::applyAge)
                ->Field("applyLifetime", &SimuCore::ParticleCore::InheritanceHandler::applyLifetime);

            serializeContext->Class<SimuCore::ParticleCore::SpawnColor>()
                ->Version(0)
                ->Field("startColor", &SimuCore::ParticleCore::SpawnColor::startColor);

            serializeContext->Class<SimuCore::ParticleCore::SpawnLifetime>()
                ->Version(0)
                ->Field("lifeTime", &SimuCore::ParticleCore::SpawnLifetime::lifeTime);

            serializeContext->Class<SimuCore::ParticleCore::SpawnLocBox>()
                ->Version(0)
                ->Field("center", &SimuCore::ParticleCore::SpawnLocBox::center)
                ->Field("size", &SimuCore::ParticleCore::SpawnLocBox::size);

            serializeContext->Class<SimuCore::ParticleCore::SpawnLocPoint>()
                ->Version(0)
                ->Field("position", &SimuCore::ParticleCore::SpawnLocPoint::pos);

            serializeContext->Enum<SimuCore::ParticleCore::Axis>()
                ->Value("xPositive", SimuCore::ParticleCore::Axis::X_POSITIVE)
                ->Value("xNegative", SimuCore::ParticleCore::Axis::X_NEGATIVE)
                ->Value("yPositive", SimuCore::ParticleCore::Axis::Y_POSITIVE)
                ->Value("yNegative", SimuCore::ParticleCore::Axis::Y_NEGATIVE)
                ->Value("zPositive", SimuCore::ParticleCore::Axis::Z_POSITIVE)
                ->Value("zNegative", SimuCore::ParticleCore::Axis::Z_NEGATIVE)
                ->Value("noAxis", SimuCore::ParticleCore::Axis::NO_AXIS);

            serializeContext->Enum<SimuCore::ParticleCore::MeshSampleType>()
                ->Value("Bone", SimuCore::ParticleCore::MeshSampleType::BONE)
                ->Value("Vertex", SimuCore::ParticleCore::MeshSampleType::VERTEX)
                ->Value("Area", SimuCore::ParticleCore::MeshSampleType::AREA);

            serializeContext->Class<SimuCore::ParticleCore::SpawnLocSphere>()
                ->Version(0)
                ->Field("axis", &SimuCore::ParticleCore::SpawnLocSphere::axis)
                ->Field("center", &SimuCore::ParticleCore::SpawnLocSphere::center)
                ->Field("radius", &SimuCore::ParticleCore::SpawnLocSphere::radius)
                ->Field("ratio", &SimuCore::ParticleCore::SpawnLocSphere::ratio)
                ->Field("angle", &SimuCore::ParticleCore::SpawnLocSphere::angle)
                ->Field("radiusThickness", &SimuCore::ParticleCore::SpawnLocSphere::radiusThickness);

            serializeContext->Class<SimuCore::ParticleCore::SpawnLocCylinder>()
                ->Version(0)
                ->Field("axis", &SimuCore::ParticleCore::SpawnLocCylinder::axis)
                ->Field("center", &SimuCore::ParticleCore::SpawnLocCylinder::center)
                ->Field("radius", &SimuCore::ParticleCore::SpawnLocCylinder::radius)
                ->Field("height", &SimuCore::ParticleCore::SpawnLocCylinder::height)
                ->Field("angle", &SimuCore::ParticleCore::SpawnLocCylinder::angle)
                ->Field("radiusThickness", &SimuCore::ParticleCore::SpawnLocCylinder::radiusThickness);

            serializeContext->Class<SimuCore::ParticleCore::SpawnLocSkeleton>()
                ->Version(0)
                ->Field("sampleType", &SimuCore::ParticleCore::SpawnLocSkeleton::sampleType)
                ->Field("scale", &SimuCore::ParticleCore::SpawnLocSkeleton::scale);

            serializeContext->Class<SimuCore::ParticleCore::SpawnLocTorus>()
                ->Version(0)
                ->Field("torusAxis", &SimuCore::ParticleCore::SpawnLocTorus::torusAxis)
                ->Field("xAxis", &SimuCore::ParticleCore::SpawnLocTorus::xAxis)
                ->Field("yAxis", &SimuCore::ParticleCore::SpawnLocTorus::yAxis)
                ->Field("center", &SimuCore::ParticleCore::SpawnLocTorus::center)
                ->Field("torusRadius", &SimuCore::ParticleCore::SpawnLocTorus::torusRadius)
                ->Field("tubeRadius", &SimuCore::ParticleCore::SpawnLocTorus::tubeRadius);

            serializeContext->Class<SimuCore::ParticleCore::SpawnSize>()
                ->Version(0)
                ->Field("size", &SimuCore::ParticleCore::SpawnSize::size);

            serializeContext->Class<SimuCore::ParticleCore::SpawnVelDirection>()
                ->Version(0)
                ->Field("strength", &SimuCore::ParticleCore::SpawnVelDirection::strength)
                ->Field("velocity", &SimuCore::ParticleCore::SpawnVelDirection::direction);

            serializeContext->Class<SimuCore::ParticleCore::SpawnVelSector>()
                ->Version(0)
                ->Field("strength", &SimuCore::ParticleCore::SpawnVelSector::strength)
                ->Field("centralAngle", &SimuCore::ParticleCore::SpawnVelSector::centralAngle)
                ->Field("rotateAngle", &SimuCore::ParticleCore::SpawnVelSector::rotateAngle)
                ->Field("direction", &SimuCore::ParticleCore::SpawnVelSector::direction);

            serializeContext->Class<SimuCore::ParticleCore::SpawnVelSphere>()
                ->Version(0)
                ->Field("strength", &SimuCore::ParticleCore::SpawnVelSphere::strength);

            serializeContext->Class<SimuCore::ParticleCore::SpawnVelConcentrate>()
                ->Version(0)
                ->Field("rate", &SimuCore::ParticleCore::SpawnVelConcentrate::rate)
                ->Field("centre", &SimuCore::ParticleCore::SpawnVelConcentrate::centre);

            serializeContext->Class<SimuCore::ParticleCore::SpawnVelCone>()
                ->Version(0)
                ->Field("angle", &SimuCore::ParticleCore::SpawnVelCone::angle)
                ->Field("strength", &SimuCore::ParticleCore::SpawnVelCone::strength)
                ->Field("direction", &SimuCore::ParticleCore::SpawnVelCone::direction);

            serializeContext->Class<SimuCore::ParticleCore::SpawnRotation>()
                ->Version(0)
                ->Field("intAngle", &SimuCore::ParticleCore::SpawnRotation::initAngle)
                ->Field("rotateSpeed", &SimuCore::ParticleCore::SpawnRotation::rotateSpeed)
                ->Field("intAxis", &SimuCore::ParticleCore::SpawnRotation::initAxis)
                ->Field("rotateAxis", &SimuCore::ParticleCore::SpawnRotation::rotateAxis);

            serializeContext->Class<SimuCore::ParticleCore::SpawnLightEffect>()
                ->Version(0)
                ->Field("lightColor", &SimuCore::ParticleCore::SpawnLightEffect::lightColor)
                ->Field("intensity", &SimuCore::ParticleCore::SpawnLightEffect::intensity)
                ->Field("radianScale", &SimuCore::ParticleCore::SpawnLightEffect::radianScale);

            serializeContext->Class<SimuCore::ParticleCore::SpawnLocationEvent>()
                ->Version(0)
                ->Field("whetherSendEvent", &SimuCore::ParticleCore::SpawnLocationEvent::whetherSendEvent);

            serializeContext->Class<SimuCore::ParticleCore::UpdateConstForce>()
                ->Version(0)
                ->Field("force", &SimuCore::ParticleCore::UpdateConstForce::force);

            serializeContext->Class<SimuCore::ParticleCore::UpdateDragForce>()
                ->Version(0)
                ->Field("dragCoefficient", &SimuCore::ParticleCore::UpdateDragForce::dragCoefficient);

           serializeContext->Class<SimuCore::ParticleCore::UpdateVortexForce>()
               ->Version(0)
               ->Field("vortexRate", &SimuCore::ParticleCore::UpdateVortexForce::vortexRate)
               ->Field("vortexRadius", &SimuCore::ParticleCore::UpdateVortexForce::vortexRadius)
               ->Field("vortexAxis", &SimuCore::ParticleCore::UpdateVortexForce::vortexAxis)
               ->Field("originPull", &SimuCore::ParticleCore::UpdateVortexForce::originPull)
               ->Field("origin", &SimuCore::ParticleCore::UpdateVortexForce::origin);

            serializeContext->Class<SimuCore::ParticleCore::UpdateCurlNoiseForce>()
                ->Version(0)
                ->Field("noiseStrength", &SimuCore::ParticleCore::UpdateCurlNoiseForce::noiseStrength)
                ->Field("noiseFrequency", &SimuCore::ParticleCore::UpdateCurlNoiseForce::noiseFrequency)
                ->Field("panNoise", &SimuCore::ParticleCore::UpdateCurlNoiseForce::panNoise)
                ->Field("panNoiseField", &SimuCore::ParticleCore::UpdateCurlNoiseForce::panNoiseField)
                ->Field("randomSeed", &SimuCore::ParticleCore::UpdateCurlNoiseForce::randomSeed)
                ->Field("randomizationVector", &SimuCore::ParticleCore::UpdateCurlNoiseForce::randomizationVector);

            serializeContext->Class<SimuCore::ParticleCore::UpdateSizeLinear>()
                ->Version(0)
                ->Field("size", &SimuCore::ParticleCore::UpdateSizeLinear::size);

            serializeContext->Class<SimuCore::ParticleCore::UpdateSizeByVelocity>()
                ->Version(0)
                ->Field("velScale", &SimuCore::ParticleCore::UpdateSizeByVelocity::velScale)
                ->Field("velocityRange", &SimuCore::ParticleCore::UpdateSizeByVelocity::velocityRange);

            serializeContext->Class<SimuCore::ParticleCore::SizeScale>()
                ->Version(0)
                ->Field("scaleFactor", &SimuCore::ParticleCore::SizeScale::scaleFactor);

            serializeContext->Class<SimuCore::ParticleCore::UpdateColor>()
                ->Version(0)
                ->Field("currentColor", &SimuCore::ParticleCore::UpdateColor::currentColor);

            serializeContext->Class<SimuCore::ParticleCore::UpdateLocationEvent>()
                ->Version(0)
                ->Field("whetherSendEvent", &SimuCore::ParticleCore::UpdateLocationEvent::whetherSendEvent);

            serializeContext->Class<SimuCore::ParticleCore::UpdateDeathEvent>()
                ->Version(0)
                ->Field("whetherSendEvent", &SimuCore::ParticleCore::UpdateDeathEvent::whetherSendEvent);

            serializeContext->Class<SimuCore::ParticleCore::UpdateCollisionEvent>()
                ->Version(0)
                ->Field("whetherSendEvent", &SimuCore::ParticleCore::UpdateCollisionEvent::whetherSendEvent);

            serializeContext->Class<SimuCore::ParticleCore::UpdateInheritanceEvent>()
                ->Version(0)
                ->Field("whetherSendEvent", &SimuCore::ParticleCore::UpdateInheritanceEvent::whetherSendEvent);

            serializeContext->Class<SimuCore::ParticleCore::UpdateSubUv>()
                ->Version(0)
                ->Field("framePerSecond", &SimuCore::ParticleCore::UpdateSubUv::framePerSecond)
                ->Field("frameNum", &SimuCore::ParticleCore::UpdateSubUv::frameNum)
                ->Field("spawnOnly", &SimuCore::ParticleCore::UpdateSubUv::spawnOnly)
                ->Field("IndexByEventOrder", &SimuCore::ParticleCore::UpdateSubUv::IndexByEventOrder);

            serializeContext->Class<SimuCore::ParticleCore::UpdateRotateAroundPoint>()
                ->Version(0)
                ->Field("rotateRate", &SimuCore::ParticleCore::UpdateRotateAroundPoint::rotateRate)
                ->Field("radius", &SimuCore::ParticleCore::UpdateRotateAroundPoint::radius)
                ->Field("xAxis", &SimuCore::ParticleCore::UpdateRotateAroundPoint::xAxis)
                ->Field("yAxis", &SimuCore::ParticleCore::UpdateRotateAroundPoint::yAxis)
                ->Field("center", &SimuCore::ParticleCore::UpdateRotateAroundPoint::center);

            serializeContext->Class<SimuCore::ParticleCore::UpdateVelocity>()
                ->Version(0)
                ->Field("strength", &SimuCore::ParticleCore::UpdateVelocity::strength)
                ->Field("direction", &SimuCore::ParticleCore::UpdateVelocity::direction);

            serializeContext->Class<SimuCore::ParticleCore::ParticleCollision>()
                ->Version(0)
                ->Field("collisionType", &SimuCore::ParticleCore::ParticleCollision::collisionType)
                ->Field("collisionRadius", &SimuCore::ParticleCore::ParticleCollision::collisionRadius)
                ->Field("bounce", &SimuCore::ParticleCore::ParticleCollision::bounce)
                ->Field("friction", &SimuCore::ParticleCore::ParticleCollision::friction);

            serializeContext->Enum<SimuCore::ParticleCore::CpuCollisionType>()
                ->Value("PLANE", SimuCore::ParticleCore::CpuCollisionType::PLANE);

            serializeContext->Enum<SimuCore::ParticleCore::RadiusCalculationType>()
                ->Value("SPRITE", SimuCore::ParticleCore::RadiusCalculationType::SPRITE)
                ->Value("MESH", SimuCore::ParticleCore::RadiusCalculationType::MESH)
                ->Value("CUSTOM", SimuCore::ParticleCore::RadiusCalculationType::CUSTOM);

            serializeContext->Enum<SimuCore::ParticleCore::RadiusCalculationMethod>()
                ->Value("BOUNDS", SimuCore::ParticleCore::RadiusCalculationMethod::BOUNDS)
                ->Value("MAXIMUM_AXIS", SimuCore::ParticleCore::RadiusCalculationMethod::MAXIMUM_AXIS)
                ->Value("MINIMUM_AXIS", SimuCore::ParticleCore::RadiusCalculationMethod::MINIMUM_AXIS);

            serializeContext->Class<SimuCore::ParticleCore::Bounce>()
                ->Version(0)
                ->Field("restitution", &SimuCore::ParticleCore::Bounce::restitution)
                ->Field("randomizeNormal", &SimuCore::ParticleCore::Bounce::randomizeNormal);

            serializeContext->Class<SimuCore::ParticleCore::Friction>()
                ->Version(0)
                ->Field("friCoefficient", &SimuCore::ParticleCore::Friction::friCoefficient);

            serializeContext->Class<SimuCore::ParticleCore::CollisionPlane>()
                ->Version(0)
                ->Field("normal", &SimuCore::ParticleCore::CollisionPlane::normal)
                ->Field("position", &SimuCore::ParticleCore::CollisionPlane::position);

            serializeContext->Class<SimuCore::ParticleCore::SpriteConfig>()
                ->Version(0)
                ->Field("facing", &SimuCore::ParticleCore::SpriteConfig::facing)
                ->Field("sortId", &SimuCore::ParticleCore::SpriteConfig::sortId)
                ->Field("subImage", &SimuCore::ParticleCore::SpriteConfig::subImageSize);

            serializeContext->Class<SimuCore::ParticleCore::MeshConfig>()
                ->Version(0)
                ->Field("facing", &SimuCore::ParticleCore::MeshConfig::facing)
                ->Field("sortId", &SimuCore::ParticleCore::MeshConfig::sortId);

            serializeContext->Enum<SimuCore::ParticleCore::TrailMode>()
                ->Value("RIBBON", SimuCore::ParticleCore::TrailMode::RIBBON)
                ->Value("TRAIL", SimuCore::ParticleCore::TrailMode::TRAIL);

            serializeContext->Class<SimuCore::ParticleCore::TrailParam>()
                ->Version(0)
                ->Field("ratio", &SimuCore::ParticleCore::TrailParam::ratio)
                ->Field("lifetime", &SimuCore::ParticleCore::TrailParam::lifetime)
                ->Field("inheritLifetime", &SimuCore::ParticleCore::TrailParam::inheritLifetime)
                ->Field("dieWithParticles", &SimuCore::ParticleCore::TrailParam::dieWithParticles);

            serializeContext->Class<SimuCore::ParticleCore::RibbonParam>()
                ->Version(0)
                ->Field("ribbonCount", &SimuCore::ParticleCore::RibbonParam::ribbonCount);

            serializeContext->Class<SimuCore::ParticleCore::RibbonConfig>()
                ->Version(0)
                ->Field("ribbonWidth", &SimuCore::ParticleCore::RibbonConfig::ribbonWidth)
                ->Field("trailParam", &SimuCore::ParticleCore::RibbonConfig::trailParam)
                ->Field("ribbonParam", &SimuCore::ParticleCore::RibbonConfig::ribbonParam)
                ->Field("sortId", &SimuCore::ParticleCore::RibbonConfig::sortId)
                ->Field("minRibbonSegmentLength", &SimuCore::ParticleCore::RibbonConfig::minRibbonSegmentLength)
                ->Field("tesselationFactor", &SimuCore::ParticleCore::RibbonConfig::tesselationFactor)
                ->Field("curveTension", &SimuCore::ParticleCore::RibbonConfig::curveTension)
                ->Field("tilingDistance", &SimuCore::ParticleCore::RibbonConfig::tilingDistance)
                ->Field("facing", &SimuCore::ParticleCore::RibbonConfig::facing)
                ->Field("mode", &SimuCore::ParticleCore::RibbonConfig::mode)
                ->Field("inheritSize", &SimuCore::ParticleCore::RibbonConfig::inheritSize);

            serializeContext->Class<SimuCore::ParticleCore::KeyPoint>()
                ->Version(0)
                ->Field("m_value", &SimuCore::ParticleCore::KeyPoint::value)
                ->Field("m_time", &SimuCore::ParticleCore::KeyPoint::time)
                ->Field("m_interpMode", &SimuCore::ParticleCore::KeyPoint::interpMode);

            serializeContext->Class<SimuCore::ParticleCore::ParticleDistribution>()
                ->Version(0);

            serializeContext->Class<SimuCore::ParticleCore::ParticleCurve>()
                ->Version(0);

            serializeContext->Class<SimuCore::ParticleCore::ParticleRandom>()
                ->Version(0);

            serializeContext->Enum<SimuCore::ParticleCore::Facing>()
                ->Value("CAMERA_SQUARE", SimuCore::ParticleCore::Facing::CAMERA_SQUARE)
                ->Value("CAMERA_RECTANGLE", SimuCore::ParticleCore::Facing::CAMERA_RECTANGLE)
                ->Value("CAMERA_POS", SimuCore::ParticleCore::Facing::CAMERA_POS)
                ->Value("VELOCITY", SimuCore::ParticleCore::Facing::VELOCITY)
                ->Value("CUSTOM", SimuCore::ParticleCore::Facing::CUSTOM);

            serializeContext->Enum<SimuCore::ParticleCore::RibbonFacing>()
                ->Value("SCREEN", SimuCore::ParticleCore::RibbonFacing::SCREEN)
                ->Value("CAMERA", SimuCore::ParticleCore::RibbonFacing::CAMERA);

            serializeContext->Enum<SimuCore::ParticleCore::RenderType>()
                ->Value("SPRITE", SimuCore::ParticleCore::RenderType::SPRITE)
                ->Value("MESH", SimuCore::ParticleCore::RenderType::MESH)
                ->Value("RIBBON", SimuCore::ParticleCore::RenderType::RIBBON);

            serializeContext->Enum<SimuCore::ParticleCore::ParticleEventType>()
                ->Value("SPAWN_LOCATION", SimuCore::ParticleCore::ParticleEventType::SPAWN_LOCATION)
                ->Value("UPDATE_LOCATION", SimuCore::ParticleCore::ParticleEventType::UPDATE_LOCATION)
                ->Value("UPDATE_DEATH", SimuCore::ParticleCore::ParticleEventType::UPDATE_DEATH)
                ->Value("UPDATE_COLLISION", SimuCore::ParticleCore::ParticleEventType::UPDATE_COLLISION);

            serializeContext->Enum<SimuCore::ParticleCore::DistributionType>()
                ->Value("DistributionType_CONSTANT", SimuCore::ParticleCore::DistributionType::CONSTANT)
                ->Value("DistributionType_RANDOM", SimuCore::ParticleCore::DistributionType::RANDOM)
                ->Value("DistributionType_CURVE", SimuCore::ParticleCore::DistributionType::CURVE);

            serializeContext->Enum<SimuCore::ParticleCore::RandomTickMode>()
                ->Value("RandomTickMode_ONCE", SimuCore::ParticleCore::RandomTickMode::ONCE)
                ->Value("RandomTickMode_PER_FRAME", SimuCore::ParticleCore::RandomTickMode::PER_FRAME)
                ->Value("RandomTickMode_PER_SPAWN", SimuCore::ParticleCore::RandomTickMode::PER_SPAWN);

            serializeContext->Enum<SimuCore::ParticleCore::KeyPointInterpMode>()
                ->Value("InterpMode_Linear", SimuCore::ParticleCore::KeyPointInterpMode::LINEAR)
                ->Value("InterpMode_Step", SimuCore::ParticleCore::KeyPointInterpMode::STEP)
                ->Value("InterpMode_CubicIn", SimuCore::ParticleCore::KeyPointInterpMode::CUBIC_IN)
                ->Value("InterpMode_CubicOut", SimuCore::ParticleCore::KeyPointInterpMode::CUBIC_OUT)
                ->Value("InterpMode_SineIn", SimuCore::ParticleCore::KeyPointInterpMode::SINE_IN)
                ->Value("InterpMode_SineOut", SimuCore::ParticleCore::KeyPointInterpMode::SINE_OUT)
                ->Value("InterpMode_CircleIn", SimuCore::ParticleCore::KeyPointInterpMode::CIRCLE_IN)
                ->Value("InterpMode_CircleOut", SimuCore::ParticleCore::KeyPointInterpMode::CIRCLE_OUT);

            serializeContext->Enum<SimuCore::ParticleCore::CurveExtrapMode>()
                ->Value("ExtrapMode_Cycle", SimuCore::ParticleCore::CurveExtrapMode::CYCLE)
                ->Value("ExtrapMode_CycleWithOffset", SimuCore::ParticleCore::CurveExtrapMode::CYCLE_WITH_OFFSET)
                ->Value("ExtrapMode_Constant", SimuCore::ParticleCore::CurveExtrapMode::CONSTANT);

            serializeContext->Enum<SimuCore::ParticleCore::CurveTickMode>()
                ->Value("CurveTickMode_NORMALIZED_AGE", SimuCore::ParticleCore::CurveTickMode::NORMALIZED_AGE)
                ->Value("CurveTickMode_CUSTOM", SimuCore::ParticleCore::CurveTickMode::CUSTOM)
                ->Value("CurveTickMode_EMIT_DURATION", SimuCore::ParticleCore::CurveTickMode::EMIT_DURATION)
                ->Value("CurveTickMode_PARTICLE_LIFETIME", SimuCore::ParticleCore::CurveTickMode::PARTICLE_LIFETIME);

            AZ::EditContext* editContext = serializeContext->GetEditContext();
            if (editContext != nullptr)
            {
                editContext->Enum<SimuCore::ParticleCore::SimulateType>("Simulate Type", "Particle Simulate Type CPU or GPU")
                    ->Value("CPU", SimuCore::ParticleCore::SimulateType::CPU)
                    ->Value("GPU", SimuCore::ParticleCore::SimulateType::GPU);

                editContext->Enum<SimuCore::ParticleCore::Axis>("Axis", "The useful axis of the emitter shape.")
                    ->Value("xPositive", SimuCore::ParticleCore::Axis::X_POSITIVE)
                    ->Value("xNegative", SimuCore::ParticleCore::Axis::X_NEGATIVE)
                    ->Value("yPositive", SimuCore::ParticleCore::Axis::Y_POSITIVE)
                    ->Value("yNegative", SimuCore::ParticleCore::Axis::Y_NEGATIVE)
                    ->Value("zPositive", SimuCore::ParticleCore::Axis::Z_POSITIVE)
                    ->Value("zNegative", SimuCore::ParticleCore::Axis::Z_NEGATIVE)
                    ->Value("noAxis", SimuCore::ParticleCore::Axis::NO_AXIS);

                editContext->Enum<SimuCore::ParticleCore::MeshSampleType>("MeshSampleType","The sample type of mesh.")
                    ->Value("Bone", SimuCore::ParticleCore::MeshSampleType::BONE)
                    ->Value("Vertex", SimuCore::ParticleCore::MeshSampleType::VERTEX)
                    ->Value("Area", SimuCore::ParticleCore::MeshSampleType::AREA);

                editContext->Enum<SimuCore::ParticleCore::RenderType>("Renderer","The render type of particles.")
                    ->Value("SPRITE", SimuCore::ParticleCore::RenderType::SPRITE)
                    ->Value("MESH", SimuCore::ParticleCore::RenderType::MESH)
                    ->Value("RIBBON", SimuCore::ParticleCore::RenderType::RIBBON);

                editContext->Enum<SimuCore::ParticleCore::Facing>("Render Alignment", "The facing mode of sprite particles.")
                    ->Value("CAMERA_POS", SimuCore::ParticleCore::Facing::CAMERA_POS)
                    ->Value("SQUARE", SimuCore::ParticleCore::Facing::CAMERA_SQUARE)
                    ->Value("RECTANGLE", SimuCore::ParticleCore::Facing::CAMERA_RECTANGLE)
                    ->Value("VELOCITY", SimuCore::ParticleCore::Facing::VELOCITY)
                    ->Value("CUSTOM", SimuCore::ParticleCore::Facing::CUSTOM);

                editContext->Enum<SimuCore::ParticleCore::RibbonFacing>("Render Alignment", "The facing Mode of ribbon particles.")
                    ->Value("SCREEN", SimuCore::ParticleCore::RibbonFacing::SCREEN)
                    ->Value("CAMERA", SimuCore::ParticleCore::RibbonFacing::CAMERA);

                editContext->Enum<SimuCore::ParticleCore::TrailMode>("Trail Mode",
                    "The type of trail.")
                    ->Value("RIBBON", SimuCore::ParticleCore::TrailMode::RIBBON)
                    ->Value("TRAIL", SimuCore::ParticleCore::TrailMode::TRAIL);

                editContext->Enum<SimuCore::ParticleCore::ParticleEventType>("ParticleEventType","The event type of particles.")
                    ->Value("SPAWN_LOCATION", SimuCore::ParticleCore::ParticleEventType::SPAWN_LOCATION)
                    ->Value("UPDATE_LOCATION", SimuCore::ParticleCore::ParticleEventType::UPDATE_LOCATION)
                    ->Value("UPDATE_DEATH", SimuCore::ParticleCore::ParticleEventType::UPDATE_DEATH)
                    ->Value("UPDATE_COLLISION", SimuCore::ParticleCore::ParticleEventType::UPDATE_COLLISION);

                editContext->Enum<SimuCore::ParticleCore::CpuCollisionType>("CpuCollisionType", "The collision type of particles.")
                    ->Value("PLANE", SimuCore::ParticleCore::CpuCollisionType::PLANE);


                editContext->Enum<SimuCore::ParticleCore::RadiusCalculationType>("RadiusCalculationType", "The calculation type of particles' radius.")
                    ->Value("SPRITE", SimuCore::ParticleCore::RadiusCalculationType::SPRITE)
                    ->Value("MESH", SimuCore::ParticleCore::RadiusCalculationType::MESH)
                    ->Value("CUSTOM", SimuCore::ParticleCore::RadiusCalculationType::CUSTOM);

                editContext->Enum<SimuCore::ParticleCore::RadiusCalculationMethod>("RadiusCalculationMethod", "The calculation method of particles' radius.")
                    ->Value("BOUNDS", SimuCore::ParticleCore::RadiusCalculationMethod::BOUNDS)
                    ->Value("MAXIMUM_AXIS", SimuCore::ParticleCore::RadiusCalculationMethod::MAXIMUM_AXIS)
                    ->Value("MINIMUM_AXIS", SimuCore::ParticleCore::RadiusCalculationMethod::MINIMUM_AXIS);

                editContext->Class<SimuCore::ParticleCore::Friction>("Friction", "The friction of plane.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::Show)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &SimuCore::ParticleCore::Friction::friCoefficient,
                        "FriCoefficient", "The friCoefficient of plane.");
            }
        }
    }
} // namespace OpenParticle
