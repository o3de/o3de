/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Serializer/DataConvertor.h>
#include <Serializer/ParticleSourceData.h>

namespace OpenParticle
{
    void DataConvertor::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->RegisterGenericType<ValueType>();
            serializeContext->Class<DataConvertor>()->Version(0);
        }
    }

    bool DataConvertor::IsCustomType(AZ::TypeId id)
    {
        return id == azrtti_typeid<ValueObjFloat>() ||
                id == azrtti_typeid<ValueObjVec2>() ||
                id == azrtti_typeid<ValueObjVec3>() ||
                id == azrtti_typeid<ValueObjVec4>() ||
                id == azrtti_typeid<ValueObjLinear>();
    }

    SimuCore::ParticleCore::KeyPointInterpMode DataConvertor::InterpModeToRuntime(const KeyPointInterpMode& value)
    {
        return static_cast<SimuCore::ParticleCore::KeyPointInterpMode>(ECast(value));
    }

    KeyPointInterpMode DataConvertor::InterpModeToEditor(const SimuCore::ParticleCore::KeyPointInterpMode& value)
    {
        return static_cast<KeyPointInterpMode>(ECast(value));
    }

    SimuCore::ParticleCore::CurveExtrapMode DataConvertor::ExtrapModeToRuntime(const CurveExtrapMode& value)
    {
        return static_cast<SimuCore::ParticleCore::CurveExtrapMode>(ECast(value));
    }

    CurveExtrapMode DataConvertor::ExtrapModeToEditor(const SimuCore::ParticleCore::CurveExtrapMode& value)
    {
        return static_cast<CurveExtrapMode>(ECast(value));
    }

    SimuCore::ParticleCore::CurveTickMode DataConvertor::CurveTickModeToRuntime(const CurveTickMode& value)
    {
        return static_cast<SimuCore::ParticleCore::CurveTickMode>(ECast(value));
    }

    CurveTickMode DataConvertor::CurveTickModeToEditor(const SimuCore::ParticleCore::CurveTickMode& value)
    {
        return static_cast<CurveTickMode>(ECast(value));
    }

    SimuCore::ParticleCore::RandomTickMode DataConvertor::RandomTickModeToRuntime(const RandomTickMode& value)
    {
        return static_cast<SimuCore::ParticleCore::RandomTickMode>(ECast(value));
    }

    RandomTickMode DataConvertor::RandomTickModeToEditor(const SimuCore::ParticleCore::RandomTickMode& value)
    {
        return static_cast<RandomTickMode>(ECast(value));
    }

    SimuCore::ParticleCore::DistributionType DataConvertor::DistributionTypeToRuntime(const DistributionType& value)
    {
        return static_cast<SimuCore::ParticleCore::DistributionType>(ECast(value));
    }

    DistributionType DataConvertor::DistributionTypeToEditor(const SimuCore::ParticleCore::DistributionType& value)
    {
        return static_cast<DistributionType>(ECast(value));
    }

    template<typename ET, typename RT, size_t count>
    static void ConvertValueObjectToRuntime(
        const ValueObject<ET, count>& editor, SimuCore::ParticleCore::ValueObject<RT, static_cast<AZ::u32>(count)>& runtime)
    {
        runtime.isUniform = editor.isUniform;
        runtime.distType = DataConvertor::DistributionTypeToRuntime(editor.distType);
        for (auto index = 0; index < count; ++index)
        {
            runtime.dataValue.SetElement(index, editor.dataValue.GetElement(index));
            runtime.distIndex[index] = static_cast<AZ::u32>(editor.distIndex[index]);
        }
    }

    template<>
    void ConvertValueObjectToRuntime(const ValueObjFloat& editor, SimuCore::ParticleCore::ValueObjFloat& runtime)
    {
        runtime.dataValue = editor.dataValue;
        runtime.isUniform = editor.isUniform;
        runtime.distType = DataConvertor::DistributionTypeToRuntime(editor.distType);
        runtime.distIndex[0] = static_cast<AZ::u32>(editor.distIndex[0]);
    }

    template<>
    void ConvertValueObjectToRuntime(const ValueObjLinear& editor, SimuCore::ParticleCore::ValueObjLinear& runtime)
    {
        runtime.isUniform = editor.isUniform;
        runtime.distType = DataConvertor::DistributionTypeToRuntime(editor.distType);

        runtime.dataValue.value = editor.dataValue.value;
        runtime.dataValue.minValue = editor.dataValue.minValue;
        runtime.dataValue.maxValue = editor.dataValue.maxValue;
        for (AZ::u32 index = 0; index < editor.Size(); ++index)
        {
            runtime.distIndex[index] = static_cast<AZ::u32>(editor.distIndex[index]);
        }
    }

    void DataConvertor::ToRuntime(OpenParticle::SystemConfig& editData, SimuCore::ParticleCore::ParticleSystem::Config& runtimeData)
    {
        runtimeData.loop = editData.loop;
        runtimeData.parallel = editData.parallel;
    }

    void DataConvertor::ToRuntime(OpenParticle::PreWarm& editData, SimuCore::ParticleCore::ParticleSystem::PreWarm& runtimeData)
    {
        runtimeData.warmupTime = editData.warmupTime;
        runtimeData.tickCount = editData.tickCount;
        runtimeData.tickDelta = editData.tickDelta;
    }

    void DataConvertor::ToRuntime(OpenParticle::EmitterConfig& editData, SimuCore::ParticleCore::ParticleEmitter::Config& runtimeData)
    {
        runtimeData.type = editData.type;
        runtimeData.startTime = editData.startTime;
        runtimeData.duration = editData.duration;
        runtimeData.localSpace = editData.localSpace;
        runtimeData.maxSize = editData.maxSize;
        runtimeData.loop = editData.loop;
    }

    void DataConvertor::ToRuntime(OpenParticle::EmitBurstList& editData, SimuCore::ParticleCore::EmitBurstList& runtimeData)
    {
        runtimeData.isProcessBurstList = editData.isProcessBurstList;
        runtimeData.arrSize = editData.burstList.size();
        for (size_t i = 0; i < runtimeData.arrSize; i++)
        {
            runtimeData.burstList[i].time = editData.burstList[i].time;
            runtimeData.burstList[i].count = editData.burstList[i].count;
            runtimeData.burstList[i].minCount = editData.burstList[i].minCount;
        }
    }

    void DataConvertor::ToRuntime(OpenParticle::EmitSpawn& editData, SimuCore::ParticleCore::EmitSpawn& runtimeData)
    {
        runtimeData.isProcessSpawnRate = editData.isProcessSpawnRate;
        ConvertValueObjectToRuntime(editData.spawnRateObject, runtimeData.spawnRate);
    }

    void DataConvertor::ToRuntime(OpenParticle::EmitSpawnOverMoving& editData, SimuCore::ParticleCore::EmitSpawnOverMoving& runtimeData)
    {
        runtimeData.isProcessSpawnRate = editData.isProcessSpawnRate;
        runtimeData.isProcessBurstList = editData.isProcessBurstList;
        runtimeData.isIgnoreSpawnRateWhenMoving = editData.isIgnoreSpawnRateWhenMoving;
        ConvertValueObjectToRuntime(editData.spawnRatePerUnitObject, runtimeData.spawnRatePerUnit);
    }

    void DataConvertor::ToRuntime(OpenParticle::ParticleEventHandler& editData, SimuCore::ParticleCore::ParticleEventHandler& runtimeData)
    {
        runtimeData.emitterIndex = editData.emitterIndex;
        runtimeData.eventType = static_cast<AZ::u32>(editData.eventType);
        runtimeData.maxEventNum = editData.maxEventNum;
        runtimeData.emitNum = editData.emitNum;
        runtimeData.useEventInfo = editData.useEventInfo;
    }

    void DataConvertor::ToRuntime(OpenParticle::InheritanceHandler& editData, SimuCore::ParticleCore::InheritanceHandler& runtimeData)
    {
        runtimeData.positionOffset = editData.positionOffset;
        runtimeData.velocityRatio = editData.velocityRatio;
        runtimeData.colorRatio = editData.colorRatio;
        runtimeData.emitterIndex = editData.emitterIndex;
        runtimeData.spawnRate = editData.spawnRate;
        runtimeData.calculateSpawnRate = editData.calculateSpawnRate;
        runtimeData.spawnEnable = editData.spawnEnable;
        runtimeData.applyPosition = editData.applyPosition;
        runtimeData.applyVelocity = editData.applyVelocity;
        runtimeData.overwriteVelocity = editData.overwriteVelocity;
        runtimeData.applySize = editData.applySize;
        runtimeData.applyColorRGB = editData.applyColorRGB;
        runtimeData.applyColorAlpha = editData.applyColorAlpha;
        runtimeData.applyAge = editData.applyAge;
        runtimeData.applyLifetime = editData.applyLifetime;
    }

    void DataConvertor::ToRuntime(OpenParticle::SpawnColor& editData, SimuCore::ParticleCore::SpawnColor& runtimeData)
    {
        ConvertValueObjectToRuntime(editData.startColorObject, runtimeData.startColor);
    }

    void DataConvertor::ToRuntime(OpenParticle::SpawnLifetime& editData, SimuCore::ParticleCore::SpawnLifetime& runtimeData)
    {
        ConvertValueObjectToRuntime(editData.lifeTimeObject, runtimeData.lifeTime);
    }

    void DataConvertor::ToRuntime(OpenParticle::SpawnLocBox& editData, SimuCore::ParticleCore::SpawnLocBox& runtimeData)
    {
        runtimeData.size = editData.size;
        runtimeData.center = editData.center;
    }

    void DataConvertor::ToRuntime(OpenParticle::SpawnLocPoint& editData, SimuCore::ParticleCore::SpawnLocPoint& runtimeData)
    {
        ConvertValueObjectToRuntime(editData.positionObject, runtimeData.pos);
    }

    void DataConvertor::ToRuntime(OpenParticle::SpawnLocSphere& editData, SimuCore::ParticleCore::SpawnLocSphere& runtimeData)
    {
        runtimeData.axis = editData.axis;
        runtimeData.angle = editData.angle;
        runtimeData.radius = editData.radius;
        runtimeData.ratio = editData.ratio;
        runtimeData.radiusThickness = editData.radiusThickness;
        runtimeData.center = editData.center;
    }

    void DataConvertor::ToRuntime(OpenParticle::SpawnLocCylinder& editData, SimuCore::ParticleCore::SpawnLocCylinder& runtimeData)
    {
        runtimeData.axis = editData.axis;
        runtimeData.angle = editData.angle;
        runtimeData.radius = editData.radius;
        runtimeData.height = editData.height;
        runtimeData.radiusThickness = editData.radiusThickness;
        runtimeData.center = editData.center;
    }

    void DataConvertor::ToRuntime(OpenParticle::SpawnLocSkeleton& editData, SimuCore::ParticleCore::SpawnLocSkeleton& runtimeData)
    {
        runtimeData.sampleType = editData.sampleType;
        runtimeData.scale = editData.scale;
    }

    void DataConvertor::ToRuntime(OpenParticle::SpawnLocTorus& editData, SimuCore::ParticleCore::SpawnLocTorus& runtimeData)
    {
        runtimeData.torusRadius = editData.torusRadius;
        runtimeData.tubeRadius = editData.tubeRadius;
        runtimeData.center = editData.center;
        runtimeData.torusAxis = editData.torusAxis;
        runtimeData.torusAxis.Normalize();
        if (runtimeData.torusAxis.GetZ()) {
            runtimeData.xAxis = {
                1.f, 1.f, -(runtimeData.torusAxis.GetX() + runtimeData.torusAxis.GetY()) / runtimeData.torusAxis.GetZ()
            };
        }
        else if (runtimeData.torusAxis.GetY())
        {
            runtimeData.xAxis = {
                1.f, 1.f, -(runtimeData.torusAxis.GetX() + runtimeData.torusAxis.GetZ()) / runtimeData.torusAxis.GetY()

            };
        }
        else if (runtimeData.torusAxis.GetZ())
        {
            runtimeData.xAxis = {
                1.f,  1.f, -(runtimeData.torusAxis.GetY() + runtimeData.torusAxis.GetZ()) / runtimeData.torusAxis.GetX()
            };
        }
        runtimeData.xAxis.Normalize();
        runtimeData.yAxis = runtimeData.torusAxis.Cross(runtimeData.xAxis).GetNormalized();
    }

    void DataConvertor::ToRuntime(OpenParticle::SpawnRotation& editData, SimuCore::ParticleCore::SpawnRotation& runtimeData)
    {
        ConvertValueObjectToRuntime(editData.initAngleObject, runtimeData.initAngle);
        ConvertValueObjectToRuntime(editData.rotateSpeedObject, runtimeData.rotateSpeed);
        runtimeData.initAxis = editData.initAxis;
        runtimeData.rotateAxis = editData.rotateAxis;
    }

    void DataConvertor::ToRuntime(OpenParticle::SpawnSize& editData, SimuCore::ParticleCore::SpawnSize& runtimeData)
    {
        ConvertValueObjectToRuntime(editData.sizeObject, runtimeData.size);
    }

    void DataConvertor::ToRuntime(OpenParticle::SpawnVelSector& editData, SimuCore::ParticleCore::SpawnVelSector& runtimeData)
    {
        ConvertValueObjectToRuntime(editData.strengthObject, runtimeData.strength);
        runtimeData.centralAngle = editData.centralAngle;
        runtimeData.rotateAngle = editData.rotateAngle;
        runtimeData.direction = editData.direction;
        runtimeData.direction.Normalize();
    }

    void DataConvertor::ToRuntime(OpenParticle::SpawnVelCone& editData, SimuCore::ParticleCore::SpawnVelCone& runtimeData)
    {
        runtimeData.angle = editData.angle;
        ConvertValueObjectToRuntime(editData.strengthObject, runtimeData.strength);
        runtimeData.direction = editData.direction;
    }

    void DataConvertor::ToRuntime(OpenParticle::SpawnVelDirection& editData, SimuCore::ParticleCore::SpawnVelDirection& runtimeData)
    {
        ConvertValueObjectToRuntime(editData.strengthObject, runtimeData.strength);
        ConvertValueObjectToRuntime(editData.directionObject, runtimeData.direction);
    }

    void DataConvertor::ToRuntime(OpenParticle::SpawnVelSphere& editData, SimuCore::ParticleCore::SpawnVelSphere& runtimeData)
    {
        ConvertValueObjectToRuntime(editData.strengthObject, runtimeData.strength);
    }

    void DataConvertor::ToRuntime(OpenParticle::SpawnVelConcentrate& editData, SimuCore::ParticleCore::SpawnVelConcentrate& runtimeData)
    {
        ConvertValueObjectToRuntime(editData.rateObject, runtimeData.rate);
        runtimeData.centre = editData.centre;
    }

    void DataConvertor::ToRuntime(OpenParticle::SpawnLightEffect& editData, SimuCore::ParticleCore::SpawnLightEffect& runtimeData)
    {
        ConvertValueObjectToRuntime(editData.lightColorObject, runtimeData.lightColor);
        ConvertValueObjectToRuntime(editData.radianScaleObject, runtimeData.radianScale);
        ConvertValueObjectToRuntime(editData.intensityObject, runtimeData.intensity);
    }

    void DataConvertor::ToRuntime(OpenParticle::SpawnLocationEvent& editData, SimuCore::ParticleCore::SpawnLocationEvent& runtimeData)
    {
        runtimeData.whetherSendEvent = editData.whetherSendEvent;
    }

    void DataConvertor::ToRuntime(OpenParticle::UpdateColor& editData, SimuCore::ParticleCore::UpdateColor& runtimeData)
    {
        ConvertValueObjectToRuntime(editData.currentColorObject, runtimeData.currentColor);
    }

    void DataConvertor::ToRuntime(OpenParticle::UpdateLocationEvent& editData, SimuCore::ParticleCore::UpdateLocationEvent& runtimeData)
    {
        runtimeData.whetherSendEvent = editData.whetherSendEvent;
    }

    void DataConvertor::ToRuntime(OpenParticle::UpdateDeathEvent& editData, SimuCore::ParticleCore::UpdateDeathEvent& runtimeData)
    {
        runtimeData.whetherSendEvent = editData.whetherSendEvent;
    }

    void DataConvertor::ToRuntime(OpenParticle::UpdateCollisionEvent& editData, SimuCore::ParticleCore::UpdateCollisionEvent& runtimeData)
    {
        runtimeData.whetherSendEvent = editData.whetherSendEvent;
    }

    void DataConvertor::ToRuntime(OpenParticle::UpdateInheritanceEvent& editData, SimuCore::ParticleCore::UpdateInheritanceEvent& runtimeData)
    {
        runtimeData.whetherSendEvent = editData.whetherSendEvent;
    }

    void DataConvertor::ToRuntime(OpenParticle::UpdateConstForce& editData, SimuCore::ParticleCore::UpdateConstForce& runtimeData)
    {
        ConvertValueObjectToRuntime(editData.forceObject, runtimeData.force);
    }

    void DataConvertor::ToRuntime(OpenParticle::UpdateDragForce& editData, SimuCore::ParticleCore::UpdateDragForce& runtimeData)
    {
        ConvertValueObjectToRuntime(editData.dragCoefficientObject, runtimeData.dragCoefficient);
    }

    void DataConvertor::ToRuntime(OpenParticle::UpdateVortexForce& editData, SimuCore::ParticleCore::UpdateVortexForce& runtimeData)
    {
        runtimeData.origin = editData.origin;
        runtimeData.vortexAxis = editData.vortexAxis;
        if (runtimeData.vortexAxis != AZ::Vector3::CreateZero())
        {
            runtimeData.vortexAxis.Normalize();
        }
        ConvertValueObjectToRuntime(editData.originPullObject, runtimeData.originPull);
        ConvertValueObjectToRuntime(editData.vortexRateObject, runtimeData.vortexRate);
        ConvertValueObjectToRuntime(editData.vortexRadiusObject, runtimeData.vortexRadius);
    }

    void DataConvertor::ToRuntime(OpenParticle::UpdateCurlNoiseForce& editData, SimuCore::ParticleCore::UpdateCurlNoiseForce& runtimeData)
    {
        runtimeData.noiseStrength = editData.noiseStrength;
        runtimeData.noiseFrequency = editData.noiseFrequency;
        runtimeData.panNoise = editData.panNoise;
        runtimeData.panNoiseField = editData.panNoiseField;
        runtimeData.randomSeed = editData.randomSeed;
        runtimeData.randomizationVector = editData.randomizationVector;
    }

    void DataConvertor::ToRuntime(OpenParticle::UpdateSizeLinear& editData, SimuCore::ParticleCore::UpdateSizeLinear& runtimeData)
    {
        ConvertValueObjectToRuntime(editData.sizeObject, runtimeData.size);
    }

    void DataConvertor::ToRuntime(OpenParticle::UpdateSizeByVelocity& editData, SimuCore::ParticleCore::UpdateSizeByVelocity& runtimeData)
    {
        ConvertValueObjectToRuntime(editData.velScaleObject, runtimeData.velScale);
        runtimeData.velocityRange = editData.velocityRange;
    }

    void DataConvertor::ToRuntime(OpenParticle::SizeScale& editData, SimuCore::ParticleCore::SizeScale& runtimeData)
    {
        ConvertValueObjectToRuntime(editData.scaleFactorObject, runtimeData.scaleFactor);
    }

    void DataConvertor::ToRuntime(OpenParticle::UpdateSubUv& editData, SimuCore::ParticleCore::UpdateSubUv& runtimeData)
    {
        runtimeData.frameNum = editData.frameNum;
        runtimeData.framePerSecond = editData.framePerSecond;
        runtimeData.spawnOnly = editData.spawnOnly;
        runtimeData.IndexByEventOrder = editData.IndexByEventOrder;
    }

    void DataConvertor::ToRuntime(OpenParticle::UpdateRotateAroundPoint& editData, SimuCore::ParticleCore::UpdateRotateAroundPoint& runtimeData)
    {
        runtimeData.rotateRate = editData.rotateRate;
        runtimeData.radius = editData.radius;
        runtimeData.center = editData.center;
        runtimeData.xAxis = editData.xAxis;
        runtimeData.yAxis = editData.yAxis;
        runtimeData.xAxis.Normalize();
        runtimeData.yAxis = (runtimeData.xAxis.Cross(runtimeData.yAxis)).Cross(runtimeData.xAxis).GetNormalized();
    }

    void DataConvertor::ToRuntime(OpenParticle::UpdateVelocity& editData, SimuCore::ParticleCore::UpdateVelocity& runtimeData)
    {
        ConvertValueObjectToRuntime(editData.strengthObject, runtimeData.strength);
        ConvertValueObjectToRuntime(editData.directionObject, runtimeData.direction);
    }

    void DataConvertor::ToRuntime(OpenParticle::ParticleCollision& editData, SimuCore::ParticleCore::ParticleCollision& runtimeData)
    {
        runtimeData.collisionType = editData.collisionType;
        runtimeData.collisionRadius.type = editData.collisionRadius.type;
        runtimeData.collisionRadius.method = editData.collisionRadius.method;
        runtimeData.collisionRadius.radius = editData.collisionRadius.radius;
        runtimeData.collisionRadius.radiusScale = editData.collisionRadius.radiusScale;
        runtimeData.bounce.restitution = editData.bounce.restitution;
        runtimeData.bounce.randomizeNormal = editData.bounce.randomizeNormal;
        runtimeData.friction = editData.friction;
        runtimeData.useTwoPlane = editData.useTwoPlane;
        runtimeData.collisionPlane1.normal = {
            editData.collisionPlane1.normal.GetX(),
            editData.collisionPlane1.normal.GetY(),
            editData.collisionPlane1.normal.GetZ()
        };
        runtimeData.collisionPlane1.normal.Normalize();
        runtimeData.collisionPlane1.position = {
            editData.collisionPlane1.position.GetX(),
            editData.collisionPlane1.position.GetY(),
            editData.collisionPlane1.position.GetZ()
        };
        runtimeData.collisionPlane2.normal = {
            editData.collisionPlane2.normal.GetX(),
            editData.collisionPlane2.normal.GetY(),
            editData.collisionPlane2.normal.GetZ()
        };
        runtimeData.collisionPlane2.normal.Normalize();
        runtimeData.collisionPlane2.position = {
            editData.collisionPlane2.position.GetX(),
            editData.collisionPlane2.position.GetY(),
            editData.collisionPlane2.position.GetZ()
        };
    }

    void DataConvertor::ToRuntime(OpenParticle::SpriteConfig& editData, SimuCore::ParticleCore::SpriteConfig& runtimeData)
    {
        runtimeData.facing = editData.facing;
        runtimeData.sortId = editData.sortId;
        runtimeData.subImageSize.SetX(editData.subImageSize.GetX());
        runtimeData.subImageSize.SetY(editData.subImageSize.GetY());
    }

    void DataConvertor::ToRuntime(OpenParticle::MeshConfig& editData, SimuCore::ParticleCore::MeshConfig& runtimeData)
    {
        runtimeData.facing = editData.facing;
        runtimeData.sortId = editData.sortId;
    }

    void DataConvertor::ToRuntime(OpenParticle::RibbonConfig& editData, SimuCore::ParticleCore::RibbonConfig& runtimeData)
    {
        runtimeData.facing = editData.facing;
        runtimeData.mode = editData.mode;
        runtimeData.sortId = editData.sortId;
        runtimeData.minRibbonSegmentLength = editData.minRibbonSegmentLength;
        ConvertValueObjectToRuntime(editData.ribbonWidthObject, runtimeData.ribbonWidth);
        runtimeData.inheritSize = editData.inheritSize;
        runtimeData.tesselationFactor = editData.tesselationFactor;
        runtimeData.curveTension = editData.curveTension;
        runtimeData.tilingDistance = editData.tilingDistance;
        runtimeData.trailParam.ratio = editData.trailParam.ratio;
        runtimeData.trailParam.lifetime = editData.trailParam.lifetime;
        runtimeData.trailParam.inheritLifetime = editData.trailParam.inheritLifetime;
        runtimeData.trailParam.dieWithParticles = editData.trailParam.dieWithParticles;
        runtimeData.ribbonParam.ribbonCount = editData.ribbonParam.ribbonCount;
    }

    template<typename ET, typename RT>
    AZStd::any ConvertToRuntime(const AZStd::any& editValue)
    {
        AZStd::any result;
        auto editData = AZStd::any_cast<ET>(editValue);
        RT runtimeData;
        DataConvertor::ToRuntime(editData, runtimeData);
        DataConvertor::ValueType sourceVariant{ AZStd::make_any<RT>(runtimeData) };
        result = AZStd::get<0>(sourceVariant);
        return result;
    }

    template<typename ET, typename RT>
    AZStd::any ConvertToRuntime(const AZStd::any& editValue,
        const AZStd::vector<SimuCore::ParticleCore::ParticleDistribution*>& distrobutions)
    {
        AZStd::any result;
        auto editData = AZStd::any_cast<ET>(editValue);
        RT runtimeData;
        DataConvertor::ToRuntime(editData, runtimeData, distrobutions);
        DataConvertor::ValueType sourceVariant{ AZStd::make_any<RT>(runtimeData) };
        result = AZStd::get<0>(sourceVariant);
        return result;
    }

    void DataConvertor::GetDistIndex(AZStd::any& editValue, DistInfos& distInfos)
    {
        if (editValue.is<RibbonConfig>())
        {
            GetDistIndex(AZStd::any_cast<RibbonConfig&>(editValue), distInfos);
            return;
        }
        if (editValue.is<EmitSpawn>())
        {
            GetDistIndex(AZStd::any_cast<EmitSpawn&>(editValue), distInfos);
            return;
        }
        if (editValue.is<EmitSpawnOverMoving>())
        {
            GetDistIndex(AZStd::any_cast<EmitSpawnOverMoving&>(editValue), distInfos);
            return;
        }
        if (editValue.is<SpawnVelSector>())
        {
            GetDistIndex(AZStd::any_cast<SpawnVelSector&>(editValue), distInfos);
            return;
        }
        if (editValue.is<SpawnColor>())
        {
            GetDistIndex(AZStd::any_cast<SpawnColor&>(editValue), distInfos);
            return;
        }
        if (editValue.is<SpawnLifetime>())
        {
            GetDistIndex(AZStd::any_cast<SpawnLifetime&>(editValue), distInfos);
            return;
        }
        if (editValue.is<SpawnVelCone>())
        {
            GetDistIndex(AZStd::any_cast<SpawnVelCone&>(editValue), distInfos);
            return;
        }
        if (editValue.is<SpawnVelSphere>())
        {
            GetDistIndex(AZStd::any_cast<SpawnVelSphere&>(editValue), distInfos);
            return;
        }
        if (editValue.is<SpawnVelConcentrate>())
        {
            GetDistIndex(AZStd::any_cast<SpawnVelConcentrate&>(editValue), distInfos);
            return;
        }
        if (editValue.is<SpawnLocPoint>())
        {
            GetDistIndex(AZStd::any_cast<SpawnLocPoint&>(editValue), distInfos);
            return;
        }
        if (editValue.is<SpawnRotation>())
        {
            GetDistIndex(AZStd::any_cast<SpawnRotation&>(editValue), distInfos);
            return;
        }
        if (editValue.is<SpawnSize>())
        {
            GetDistIndex(AZStd::any_cast<SpawnSize&>(editValue), distInfos);
            return;
        }
        if (editValue.is<SpawnVelDirection>())
        {
            GetDistIndex(AZStd::any_cast<SpawnVelDirection&>(editValue), distInfos);
            return;
        }
        if (editValue.is<SpawnLightEffect>())
        {
            GetDistIndex(AZStd::any_cast<SpawnLightEffect&>(editValue), distInfos);
            return;
        }
        if (editValue.is<UpdateColor>())
        {
            GetDistIndex(AZStd::any_cast<UpdateColor&>(editValue), distInfos);
            return;
        }
        if (editValue.is<UpdateConstForce>())
        {
            GetDistIndex(AZStd::any_cast<UpdateConstForce&>(editValue), distInfos);
            return;
        }
        if (editValue.is<UpdateDragForce>())
        {
            GetDistIndex(AZStd::any_cast<UpdateDragForce&>(editValue), distInfos);
            return;
        }
        if (editValue.is<UpdateVortexForce>())
        {
            GetDistIndex(AZStd::any_cast<UpdateVortexForce&>(editValue), distInfos);
            return;
        }
        if (editValue.is<SizeScale>())
        {
            GetDistIndex(AZStd::any_cast<SizeScale&>(editValue), distInfos);
            return;
        }
        if (editValue.is<UpdateSizeLinear>())
        {
            GetDistIndex(AZStd::any_cast<UpdateSizeLinear&>(editValue), distInfos);
            return;
        }
        if (editValue.is<UpdateSizeByVelocity>())
        {
            GetDistIndex(AZStd::any_cast<UpdateSizeByVelocity&>(editValue), distInfos);
            return;
        }
        if (editValue.is<UpdateVelocity>())
        {
            GetDistIndex(AZStd::any_cast<UpdateVelocity&>(editValue), distInfos);
            return;
        }
    }

    void DataConvertor::UpdateDistIndex(AZStd::any& editValue, DistInfos& distInfos)
    {
        if (editValue.is<RibbonConfig>())
        {
            UpdateDistIndex(AZStd::any_cast<RibbonConfig&>(editValue), distInfos);
            return;
        }
        if (editValue.is<EmitSpawn>())
        {
            UpdateDistIndex(AZStd::any_cast<EmitSpawn&>(editValue), distInfos);
            return;
        }
        if (editValue.is<EmitSpawnOverMoving>())
        {
            UpdateDistIndex(AZStd::any_cast<EmitSpawnOverMoving&>(editValue), distInfos);
            return;
        }
        if (editValue.is<SpawnColor>())
        {
            UpdateDistIndex(AZStd::any_cast<SpawnColor&>(editValue), distInfos);
            return;
        }
        if (editValue.is<SpawnVelSector>())
        {
            UpdateDistIndex(AZStd::any_cast<SpawnVelSector&>(editValue), distInfos);
            return;
        }
        if (editValue.is<SpawnVelCone>())
        {
            UpdateDistIndex(AZStd::any_cast<SpawnVelCone&>(editValue), distInfos);
            return;
        }
        if (editValue.is<SpawnVelSphere>())
        {
            UpdateDistIndex(AZStd::any_cast<SpawnVelSphere&>(editValue), distInfos);
            return;
        }
        if (editValue.is<SpawnVelConcentrate>())
        {
            UpdateDistIndex(AZStd::any_cast<SpawnVelConcentrate&>(editValue), distInfos);
            return;
        }
        if (editValue.is<SpawnLifetime>())
        {
            UpdateDistIndex(AZStd::any_cast<SpawnLifetime&>(editValue), distInfos);
            return;
        }
        if (editValue.is<SpawnLocPoint>())
        {
            UpdateDistIndex(AZStd::any_cast<SpawnLocPoint&>(editValue), distInfos);
            return;
        }
        if (editValue.is<SpawnRotation>())
        {
            UpdateDistIndex(AZStd::any_cast<SpawnRotation&>(editValue), distInfos);
            return;
        }
        if (editValue.is<SpawnSize>())
        {
            UpdateDistIndex(AZStd::any_cast<SpawnSize&>(editValue), distInfos);
            return;
        }
        if (editValue.is<SpawnVelDirection>())
        {
            UpdateDistIndex(AZStd::any_cast<SpawnVelDirection&>(editValue), distInfos);
            return;
        }
        if (editValue.is<SpawnLightEffect>())
        {
            UpdateDistIndex(AZStd::any_cast<SpawnLightEffect&>(editValue), distInfos);
            return;
        }
        if (editValue.is<UpdateColor>())
        {
            UpdateDistIndex(AZStd::any_cast<UpdateColor&>(editValue), distInfos);
            return;
        }
        if (editValue.is<UpdateConstForce>())
        {
            UpdateDistIndex(AZStd::any_cast<UpdateConstForce&>(editValue), distInfos);
            return;
        }
        if (editValue.is<UpdateDragForce>())
        {
            UpdateDistIndex(AZStd::any_cast<UpdateDragForce&>(editValue), distInfos);
            return;
        }
        if (editValue.is<UpdateVortexForce>())
        {
            UpdateDistIndex(AZStd::any_cast<UpdateVortexForce&>(editValue), distInfos);
            return;
        }
        if (editValue.is<SizeScale>())
        {
            UpdateDistIndex(AZStd::any_cast<SizeScale&>(editValue), distInfos);
            return;
        }
        if (editValue.is<UpdateSizeLinear>())
        {
            UpdateDistIndex(AZStd::any_cast<UpdateSizeLinear&>(editValue), distInfos);
            return;
        }
        if (editValue.is<UpdateSizeByVelocity>())
        {
            UpdateDistIndex(AZStd::any_cast<UpdateSizeByVelocity&>(editValue), distInfos);
            return;
        }
        if (editValue.is<UpdateVelocity>())
        {
            UpdateDistIndex(AZStd::any_cast<UpdateVelocity&>(editValue), distInfos);
            return;
        }
    }

    void DataConvertor::GetParamId(AZStd::any& editValue, AZStd::vector<AZ::TypeId>& id)
    {
        if (editValue.is<RibbonConfig>())
        {
            GetParamId(AZStd::any_cast<RibbonConfig&>(editValue), id);
            return;
        }
        if (editValue.is<EmitSpawn>())
        {
            GetParamId(AZStd::any_cast<EmitSpawn&>(editValue), id);
            return;
        }
        if (editValue.is<EmitSpawnOverMoving>())
        {
            GetParamId(AZStd::any_cast<EmitSpawnOverMoving&>(editValue), id);
            return;
        }
        if (editValue.is<SpawnColor>())
        {
            GetParamId(AZStd::any_cast<SpawnColor&>(editValue), id);
            return;
        }
        if (editValue.is<SpawnVelSector>())
        {
            GetParamId(AZStd::any_cast<SpawnVelSector&>(editValue), id);
            return;
        }
        if (editValue.is<SpawnVelCone>())
        {
            GetParamId(AZStd::any_cast<SpawnVelCone&>(editValue), id);
            return;
        }
        if (editValue.is<SpawnVelSphere>())
        {
            GetParamId(AZStd::any_cast<SpawnVelSphere&>(editValue), id);
            return;
        }
        if (editValue.is<SpawnVelConcentrate>())
        {
            GetParamId(AZStd::any_cast<SpawnVelConcentrate&>(editValue), id);
            return;
        }
        if (editValue.is<SpawnLifetime>())
        {
            GetParamId(AZStd::any_cast<SpawnLifetime&>(editValue), id);
            return;
        }
        if (editValue.is<SpawnLocPoint>())
        {
            GetParamId(AZStd::any_cast<SpawnLocPoint&>(editValue), id);
            return;
        }
        if (editValue.is<SpawnRotation>())
        {
            GetParamId(AZStd::any_cast<SpawnRotation&>(editValue), id);
            return;
        }
        if (editValue.is<SpawnSize>())
        {
            GetParamId(AZStd::any_cast<SpawnSize&>(editValue), id);
            return;
        }
        if (editValue.is<SpawnVelDirection>())
        {
            GetParamId(AZStd::any_cast<SpawnVelDirection&>(editValue), id);
            return;
        }
        if (editValue.is<SpawnLightEffect>())
        {
            GetParamId(AZStd::any_cast<SpawnLightEffect&>(editValue), id);
            return;
        }
        if (editValue.is<UpdateColor>())
        {
            GetParamId(AZStd::any_cast<UpdateColor&>(editValue), id);
            return;
        }
        if (editValue.is<UpdateConstForce>())
        {
            GetParamId(AZStd::any_cast<UpdateConstForce&>(editValue), id);
            return;
        }
        if (editValue.is<UpdateDragForce>())
        {
            GetParamId(AZStd::any_cast<UpdateDragForce&>(editValue), id);
            return;
        }
        if (editValue.is<UpdateVortexForce>())
        {
            GetParamId(AZStd::any_cast<UpdateVortexForce&>(editValue), id);
            return;
        }
        if (editValue.is<SizeScale>())
        {
            GetParamId(AZStd::any_cast<SizeScale&>(editValue), id);
            return;
        }
        if (editValue.is<UpdateSizeLinear>())
        {
            GetParamId(AZStd::any_cast<UpdateSizeLinear&>(editValue), id);
            return;
        }
        if (editValue.is<UpdateSizeByVelocity>())
        {
            GetParamId(AZStd::any_cast<UpdateSizeByVelocity&>(editValue), id);
            return;
        }
        if (editValue.is<UpdateVelocity>())
        {
            GetParamId(AZStd::any_cast<UpdateVelocity&>(editValue), id);
            return;
        }
    }

    AZStd::any DataConvertor::ToRuntime(const AZStd::any& editValue)
    {
        AZStd::any result; 
        if (editValue.is<SystemConfig>())
        {
            result = ConvertToRuntime<OpenParticle::SystemConfig, SimuCore::ParticleCore::ParticleSystem::Config>(editValue);
            return result;
        }
        if (editValue.is<PreWarm>())
        {
            result = ConvertToRuntime<OpenParticle::PreWarm, SimuCore::ParticleCore::ParticleSystem::PreWarm>(editValue);
            return result;
        }
        if (editValue.is<EmitterConfig>())
        {
            result = ConvertToRuntime<OpenParticle::EmitterConfig, SimuCore::ParticleCore::ParticleEmitter::Config>(editValue);
            return result;
        }
        if (editValue.is<SpriteConfig>())
        {
            result = ConvertToRuntime<OpenParticle::SpriteConfig, SimuCore::ParticleCore::SpriteConfig>(editValue);
            return result;
        }
        if (editValue.is<MeshConfig>())
        {
            result = ConvertToRuntime<OpenParticle::MeshConfig, SimuCore::ParticleCore::MeshConfig>(editValue);
            return result;
        }
        if (editValue.is<RibbonConfig>())
        {
            result = ConvertToRuntime<OpenParticle::RibbonConfig, SimuCore::ParticleCore::RibbonConfig>(editValue);
            return result;
        }
        if (editValue.is<EmitBurstList>())
        {
            result = ConvertToRuntime<OpenParticle::EmitBurstList, SimuCore::ParticleCore::EmitBurstList>(editValue);
            return result;
        }
        if (editValue.is<EmitSpawn>())
        {
            result = ConvertToRuntime<OpenParticle::EmitSpawn, SimuCore::ParticleCore::EmitSpawn>(editValue);
            return result;
        }
        if (editValue.is<EmitSpawnOverMoving>())
        {
            result = ConvertToRuntime<OpenParticle::EmitSpawnOverMoving, SimuCore::ParticleCore::EmitSpawnOverMoving>(editValue);
            return result;
        }
        if (editValue.is<ParticleEventHandler>())
        {
            result = ConvertToRuntime<OpenParticle::ParticleEventHandler, SimuCore::ParticleCore::ParticleEventHandler>(editValue);
            return result;
        }
        if (editValue.is<InheritanceHandler>())
        {
            result = ConvertToRuntime<OpenParticle::InheritanceHandler, SimuCore::ParticleCore::InheritanceHandler>(editValue);
            return result;
        }
        if (editValue.is<SpawnColor>())
        {
            result = ConvertToRuntime<OpenParticle::SpawnColor, SimuCore::ParticleCore::SpawnColor>(editValue);
            return result;
        }
        if (editValue.is<SpawnLifetime>())
        {
            result = ConvertToRuntime<OpenParticle::SpawnLifetime, SimuCore::ParticleCore::SpawnLifetime>(editValue);
            return result;
        }
        if (editValue.is<SpawnLocBox>())
        {
            result = ConvertToRuntime<OpenParticle::SpawnLocBox, SimuCore::ParticleCore::SpawnLocBox>(editValue);
            return result;
        }
        if (editValue.is<SpawnLocPoint>())
        {
            result = ConvertToRuntime<OpenParticle::SpawnLocPoint, SimuCore::ParticleCore::SpawnLocPoint>(editValue);
            return result;
        }
        if (editValue.is<SpawnLocSphere>())
        {
            result = ConvertToRuntime<OpenParticle::SpawnLocSphere, SimuCore::ParticleCore::SpawnLocSphere>(editValue);
            return result;
        }
        if (editValue.is<SpawnLocCylinder>())
        {
            result = ConvertToRuntime<OpenParticle::SpawnLocCylinder, SimuCore::ParticleCore::SpawnLocCylinder>(editValue);
            return result;
        }
        if (editValue.is<SpawnLocSkeleton>())
        {
            result = ConvertToRuntime<OpenParticle::SpawnLocSkeleton, SimuCore::ParticleCore::SpawnLocSkeleton>(editValue);
            return result;
        }
        if (editValue.is<SpawnLocTorus>())
        {
            result = ConvertToRuntime<OpenParticle::SpawnLocTorus, SimuCore::ParticleCore::SpawnLocTorus>(editValue);
            return result;
        }
        if (editValue.is<SpawnRotation>())
        {
            result = ConvertToRuntime<OpenParticle::SpawnRotation, SimuCore::ParticleCore::SpawnRotation>(editValue);
            return result;
        }
        if (editValue.is<SpawnSize>())
        {
            result = ConvertToRuntime<OpenParticle::SpawnSize, SimuCore::ParticleCore::SpawnSize>(editValue);
            return result;
        }
        if (editValue.is<SpawnVelSector>())
        {
            result = ConvertToRuntime<OpenParticle::SpawnVelSector, SimuCore::ParticleCore::SpawnVelSector>(editValue);
            return result;
        }
        if (editValue.is<SpawnVelCone>())
        {
            result = ConvertToRuntime<OpenParticle::SpawnVelCone, SimuCore::ParticleCore::SpawnVelCone>(editValue);
            return result;
        }
        if (editValue.is<SpawnVelDirection>())
        {
            result = ConvertToRuntime<OpenParticle::SpawnVelDirection, SimuCore::ParticleCore::SpawnVelDirection>(editValue);
            return result;
        }
        if (editValue.is<SpawnVelSphere>())
        {
            result = ConvertToRuntime<OpenParticle::SpawnVelSphere, SimuCore::ParticleCore::SpawnVelSphere>(editValue);
            return result;
        }
        if (editValue.is<SpawnVelConcentrate>())
        {
            result = ConvertToRuntime<OpenParticle::SpawnVelConcentrate, SimuCore::ParticleCore::SpawnVelConcentrate>(editValue);
            return result;
        }
        if (editValue.is<SpawnLightEffect>())
        {
            result = ConvertToRuntime<OpenParticle::SpawnLightEffect, SimuCore::ParticleCore::SpawnLightEffect>(editValue);
            return result;
        }
        if (editValue.is<SpawnLocationEvent>())
        {
            result = ConvertToRuntime<OpenParticle::SpawnLocationEvent, SimuCore::ParticleCore::SpawnLocationEvent>(editValue);
            return result;
        }
        if (editValue.is<UpdateColor>())
        {
            result = ConvertToRuntime<OpenParticle::UpdateColor, SimuCore::ParticleCore::UpdateColor>(editValue);
            return result;
        }
        if (editValue.is<UpdateLocationEvent>())
        {
            result = ConvertToRuntime<OpenParticle::UpdateLocationEvent, SimuCore::ParticleCore::UpdateLocationEvent>(editValue);
            return result;
        }
        if (editValue.is<UpdateDeathEvent>())
        {
            result = ConvertToRuntime<OpenParticle::UpdateDeathEvent, SimuCore::ParticleCore::UpdateDeathEvent>(editValue);
            return result;
        }
        if (editValue.is<UpdateCollisionEvent>())
        {
            result = ConvertToRuntime<OpenParticle::UpdateCollisionEvent, SimuCore::ParticleCore::UpdateCollisionEvent>(editValue);
            return result;
        }
        if (editValue.is<UpdateInheritanceEvent>())
        {
            result = ConvertToRuntime<OpenParticle::UpdateInheritanceEvent, SimuCore::ParticleCore::UpdateInheritanceEvent>(editValue);
            return result;
        }
        if (editValue.is<UpdateConstForce>())
        {
            result = ConvertToRuntime<OpenParticle::UpdateConstForce, SimuCore::ParticleCore::UpdateConstForce>(editValue);
            return result;
        }
        if (editValue.is<UpdateDragForce>())
        {
            result = ConvertToRuntime<OpenParticle::UpdateDragForce, SimuCore::ParticleCore::UpdateDragForce>(editValue);
            return result;
        }
        if (editValue.is<UpdateVortexForce>())
        {
            result = ConvertToRuntime<OpenParticle::UpdateVortexForce, SimuCore::ParticleCore::UpdateVortexForce>(editValue);
            return result;
        }
        if (editValue.is<UpdateCurlNoiseForce>())
        {
            result = ConvertToRuntime<OpenParticle::UpdateCurlNoiseForce, SimuCore::ParticleCore::UpdateCurlNoiseForce>(editValue);
            return result;
        }
        if (editValue.is<UpdateSizeLinear>())
        {
            result = ConvertToRuntime<OpenParticle::UpdateSizeLinear, SimuCore::ParticleCore::UpdateSizeLinear>(editValue);
            return result;
        }
        if (editValue.is<UpdateSizeByVelocity>())
        {
            result = ConvertToRuntime<OpenParticle::UpdateSizeByVelocity, SimuCore::ParticleCore::UpdateSizeByVelocity>(editValue);
            return result;
        }
        if (editValue.is<SizeScale>())
        {
            result = ConvertToRuntime<OpenParticle::SizeScale, SimuCore::ParticleCore::SizeScale>(editValue);
            return result;
        }
        if (editValue.is<UpdateSubUv>())
        {
            result = ConvertToRuntime<OpenParticle::UpdateSubUv, SimuCore::ParticleCore::UpdateSubUv>(editValue);
            return result;
        }
        if (editValue.is<UpdateRotateAroundPoint>())
        {
            result = ConvertToRuntime<OpenParticle::UpdateRotateAroundPoint, SimuCore::ParticleCore::UpdateRotateAroundPoint>(editValue);
            return result;
        }
        if (editValue.is<UpdateVelocity>())
        {
            result = ConvertToRuntime<OpenParticle::UpdateVelocity, SimuCore::ParticleCore::UpdateVelocity>(editValue);
            return result;
        }
        if (editValue.is<ParticleCollision>())
        {
            result = ConvertToRuntime<OpenParticle::ParticleCollision, SimuCore::ParticleCore::ParticleCollision>(editValue);
            return result;
        }
        return result;
    }

    template<typename T, size_t count>
    void GetDistIndexImpl(ValueObject<T, count>& valueObj, DistInfos& distInfos)
    {
        for (size_t index = 0; index < count; ++index)
        {
            if (valueObj.distType == DistributionType::RANDOM && valueObj.distIndex[index] != 0)
            {
                distInfos.randomIndexInfos.emplace_back(ParamDistInfo {
                    &valueObj, azrtti_typeid<ValueObject<T, count>>(), valueObj.isUniform,
                    valueObj.paramName, index, static_cast<AZ::u32>(valueObj.distIndex[index]) });
            }
            if (valueObj.distType == DistributionType::CURVE && valueObj.distIndex[index] != 0)
            {
                distInfos.curveIndexInfos.emplace_back(ParamDistInfo {
                    &valueObj, azrtti_typeid<ValueObject<T, count>>(), valueObj.isUniform,
                    valueObj.paramName, index, static_cast<AZ::u32>(valueObj.distIndex[index]) });
            }
        }
    }

    void DataConvertor::GetDistIndex(RibbonConfig& editData, DistInfos& distInfos)
    {
        GetDistIndexImpl(editData.ribbonWidthObject, distInfos);
    }

    void DataConvertor::GetDistIndex(EmitSpawn& editData, DistInfos& distInfos)
    {
        GetDistIndexImpl(editData.spawnRateObject, distInfos);
    }

    void DataConvertor::GetDistIndex(EmitSpawnOverMoving& editData, DistInfos& distInfos)
    {
        GetDistIndexImpl(editData.spawnRatePerUnitObject, distInfos);
    }

    void DataConvertor::GetDistIndex(SpawnColor& editData, DistInfos& distInfos)
    {
        GetDistIndexImpl(editData.startColorObject, distInfos);
    }

    void DataConvertor::GetDistIndex(SpawnLifetime& editData, DistInfos& distInfos)
    {
        GetDistIndexImpl(editData.lifeTimeObject, distInfos);
    }

    void DataConvertor::GetDistIndex(SpawnLocPoint& editData, DistInfos& distInfos)
    {
        GetDistIndexImpl(editData.positionObject, distInfos);
    }

    void DataConvertor::GetDistIndex(SpawnRotation& editData, DistInfos& distInfos)
    {
        GetDistIndexImpl(editData.initAngleObject, distInfos);
        GetDistIndexImpl(editData.rotateSpeedObject, distInfos);
    }

    void DataConvertor::GetDistIndex(SpawnSize& editData, DistInfos& distInfos)
    {
        GetDistIndexImpl(editData.sizeObject, distInfos);
    }

    void DataConvertor::GetDistIndex(SpawnVelDirection& editData, DistInfos& distInfos)
    {
        GetDistIndexImpl(editData.strengthObject, distInfos);
        GetDistIndexImpl(editData.directionObject, distInfos);
    }

    void DataConvertor::GetDistIndex(SpawnVelSector& editData, DistInfos& distInfos)
    {
        GetDistIndexImpl(editData.strengthObject, distInfos);
    }

    void DataConvertor::GetDistIndex(SpawnVelCone& editData, DistInfos& distInfos)
    {
        GetDistIndexImpl(editData.strengthObject, distInfos);
    }

    void DataConvertor::GetDistIndex(SpawnVelSphere& editData, DistInfos& distInfos)
    {
        GetDistIndexImpl(editData.strengthObject, distInfos);
    }

    void DataConvertor::GetDistIndex(SpawnVelConcentrate& editData, DistInfos& distInfos)
    {
        GetDistIndexImpl(editData.rateObject, distInfos);
    }

    void DataConvertor::GetDistIndex(SpawnLightEffect& editData, DistInfos& distInfos)
    {
        GetDistIndexImpl(editData.lightColorObject, distInfos);
        GetDistIndexImpl(editData.intensityObject, distInfos);
        GetDistIndexImpl(editData.radianScaleObject, distInfos);
    }

    void DataConvertor::GetDistIndex(UpdateColor& editData, DistInfos& distInfos)
    {
        GetDistIndexImpl(editData.currentColorObject, distInfos);
    }

    void DataConvertor::GetDistIndex(UpdateConstForce& editData, DistInfos& distInfos)
    {
        GetDistIndexImpl(editData.forceObject, distInfos);
    }

    void DataConvertor::GetDistIndex(UpdateDragForce& editData, DistInfos& distInfos)
    {
        GetDistIndexImpl(editData.dragCoefficientObject, distInfos);
    }

    void DataConvertor::GetDistIndex(UpdateSizeLinear& editData, DistInfos& distInfos)
    {
        GetDistIndexImpl(editData.sizeObject, distInfos);
    }

    void DataConvertor::GetDistIndex(UpdateSizeByVelocity& editData, DistInfos& distInfos)
    {
        GetDistIndexImpl(editData.velScaleObject, distInfos);
    }

    void DataConvertor::GetDistIndex(SizeScale& editData, DistInfos& distInfos)
    {
        GetDistIndexImpl(editData.scaleFactorObject, distInfos);
    }

    void DataConvertor::GetDistIndex(UpdateVortexForce& editData, DistInfos& distInfos)
    {
        GetDistIndexImpl(editData.originPullObject, distInfos);
        GetDistIndexImpl(editData.vortexRateObject, distInfos);
        GetDistIndexImpl(editData.vortexRadiusObject, distInfos);
    }

    void DataConvertor::GetDistIndex(UpdateVelocity& editData, DistInfos& distInfos)
    {
        GetDistIndexImpl(editData.strengthObject, distInfos);
        GetDistIndexImpl(editData.directionObject, distInfos);
    }

    template<typename T, size_t count>
    void SetDistIndex(ValueObject<T, count>& valueObject, const AZStd::vector<ParamDistInfo>& dists, const DistributionType& type)
    {
        for (auto& dist : dists)
        {
            if (dist.paramName == valueObject.paramName)
            {
                valueObject.distType = type;
                valueObject.distIndex[dist.paramIndex] = dist.distIndex;
                valueObject.isUniform = dist.isUniform;
                if (dist.distIndex == 0)
                {
                    valueObject.distType = DistributionType::CONSTANT;
                }
            }
        }
    }

    template<typename T, size_t count>
    void UpdateDistIndexImpl(ValueObject<T, count>& valueObject, DistInfos& distInfos)
    {
        if (!distInfos.randomIndexInfos.empty())
        {
            SetDistIndex(valueObject, *distInfos(DistributionType::RANDOM), DistributionType::RANDOM);
        }
        if (!distInfos.curveIndexInfos.empty())
        {
            SetDistIndex(valueObject, *distInfos(DistributionType::CURVE), DistributionType::CURVE);
        }
    }

    void DataConvertor::UpdateDistIndex(RibbonConfig& editData, DistInfos& distInfos)
    {
        UpdateDistIndexImpl(editData.ribbonWidthObject, distInfos);
    }

    void DataConvertor::UpdateDistIndex(EmitSpawn& editData, DistInfos& distInfos)
    {
        UpdateDistIndexImpl(editData.spawnRateObject, distInfos);
    }

    void DataConvertor::UpdateDistIndex(EmitSpawnOverMoving& editData, DistInfos& distInfos)
    {
        UpdateDistIndexImpl(editData.spawnRatePerUnitObject, distInfos);
    }

    void DataConvertor::UpdateDistIndex(SpawnColor& editData, DistInfos& distInfos)
    {
        UpdateDistIndexImpl(editData.startColorObject, distInfos);
    }

    void DataConvertor::UpdateDistIndex(SpawnLifetime& editData, DistInfos& distInfos)
    {
        UpdateDistIndexImpl(editData.lifeTimeObject, distInfos);
    }

    void DataConvertor::UpdateDistIndex(SpawnLocPoint& editData, DistInfos& distInfos)
    {
        UpdateDistIndexImpl(editData.positionObject, distInfos);
    }

    void DataConvertor::UpdateDistIndex(SpawnRotation& editData, DistInfos& distInfos)
    {
        UpdateDistIndexImpl(editData.initAngleObject, distInfos);
        UpdateDistIndexImpl(editData.rotateSpeedObject, distInfos);
    }

    void DataConvertor::UpdateDistIndex(SpawnSize& editData, DistInfos& distInfos)
    {
        UpdateDistIndexImpl(editData.sizeObject, distInfos);
    }

    void DataConvertor::UpdateDistIndex(SpawnVelDirection& editData, DistInfos& distInfos)
    {
        UpdateDistIndexImpl(editData.strengthObject, distInfos);
        UpdateDistIndexImpl(editData.directionObject, distInfos);
    }

    void DataConvertor::UpdateDistIndex(SpawnVelSector& editData, DistInfos& distInfos)
    {
        UpdateDistIndexImpl(editData.strengthObject, distInfos);
    }

    void DataConvertor::UpdateDistIndex(SpawnVelCone& editData, DistInfos& distInfos)
    {
        UpdateDistIndexImpl(editData.strengthObject, distInfos);
    }

    void DataConvertor::UpdateDistIndex(SpawnVelSphere& editData, DistInfos& distInfos)
    {
        UpdateDistIndexImpl(editData.strengthObject, distInfos);
    }

    void DataConvertor::UpdateDistIndex(SpawnVelConcentrate& editData, DistInfos& distInfos)
    {
        UpdateDistIndexImpl(editData.rateObject, distInfos);
    }

    void DataConvertor::UpdateDistIndex(SpawnLightEffect& editData, DistInfos& distInfos)
    {
        UpdateDistIndexImpl(editData.lightColorObject, distInfos);
        UpdateDistIndexImpl(editData.intensityObject, distInfos);
        UpdateDistIndexImpl(editData.radianScaleObject, distInfos);
    }

    void DataConvertor::UpdateDistIndex(UpdateColor& editData, DistInfos& distInfos)
    {
        UpdateDistIndexImpl(editData.currentColorObject, distInfos);
    }

    void DataConvertor::UpdateDistIndex(UpdateConstForce& editData, DistInfos& distInfos)
    {
        UpdateDistIndexImpl(editData.forceObject, distInfos);
    }

    void DataConvertor::UpdateDistIndex(UpdateDragForce& editData, DistInfos& distInfos)
    {
        UpdateDistIndexImpl(editData.dragCoefficientObject, distInfos);
    }

    void DataConvertor::UpdateDistIndex(UpdateSizeLinear& editData, DistInfos& distInfos)
    {
        UpdateDistIndexImpl(editData.sizeObject, distInfos);
    }

    void DataConvertor::UpdateDistIndex(UpdateSizeByVelocity& editData, DistInfos& distInfos)
    {
        UpdateDistIndexImpl(editData.velScaleObject, distInfos);
    }

    void DataConvertor::UpdateDistIndex(SizeScale& editData, DistInfos& distInfos)
    {
        UpdateDistIndexImpl(editData.scaleFactorObject, distInfos);
    }

    void DataConvertor::UpdateDistIndex(UpdateVortexForce& editData, DistInfos& distInfos)
    {
        UpdateDistIndexImpl(editData.originPullObject, distInfos);
        UpdateDistIndexImpl(editData.vortexRateObject, distInfos);
        UpdateDistIndexImpl(editData.vortexRadiusObject, distInfos);
    }

    void DataConvertor::UpdateDistIndex(UpdateVelocity& editData, DistInfos& distInfos)
    {
        UpdateDistIndexImpl(editData.strengthObject, distInfos);
        UpdateDistIndexImpl(editData.directionObject, distInfos);
    }

    void DataConvertor::GetParamId(RibbonConfig& editData, AZStd::vector<AZ::TypeId>& paramIds)
    {
        paramIds.emplace_back(editData.ribbonWidthObject.paramName);
    }

    void DataConvertor::GetParamId(EmitSpawn& editData, AZStd::vector<AZ::TypeId>& paramIds)
    {
        paramIds.emplace_back(editData.spawnRateObject.paramName);
    }

    void DataConvertor::GetParamId(EmitSpawnOverMoving& editData, AZStd::vector<AZ::TypeId>& paramIds)
    {
        paramIds.emplace_back(editData.spawnRatePerUnitObject.paramName);
    }

    void DataConvertor::GetParamId(SpawnColor& editData, AZStd::vector<AZ::TypeId>& paramIds)
    {
        paramIds.emplace_back(editData.startColorObject.paramName);
    }

    void DataConvertor::GetParamId(SpawnLifetime& editData, AZStd::vector<AZ::TypeId>& paramIds)
    {
        paramIds.emplace_back(editData.lifeTimeObject.paramName);
    }

    void DataConvertor::GetParamId(SpawnLocPoint& editData, AZStd::vector<AZ::TypeId>& paramIds)
    {
        paramIds.emplace_back(editData.positionObject.paramName);
    }

    void DataConvertor::GetParamId(SpawnRotation& editData, AZStd::vector<AZ::TypeId>& paramIds)
    {
        paramIds.emplace_back(editData.initAngleObject.paramName);
        paramIds.emplace_back(editData.rotateSpeedObject.paramName);
    }

    void DataConvertor::GetParamId(SpawnSize& editData, AZStd::vector<AZ::TypeId>& paramIds)
    {
        paramIds.emplace_back(editData.sizeObject.paramName);
    }

    void DataConvertor::GetParamId(SpawnVelDirection& editData, AZStd::vector<AZ::TypeId>& paramIds)
    {
        paramIds.emplace_back(editData.strengthObject.paramName);
        paramIds.emplace_back(editData.directionObject.paramName);
    }

    void DataConvertor::GetParamId(SpawnVelSector& editData, AZStd::vector<AZ::TypeId>& paramIds)
    {
        paramIds.emplace_back(editData.strengthObject.paramName);
    }

    void DataConvertor::GetParamId(SpawnVelCone& editData, AZStd::vector<AZ::TypeId>& paramIds)
    {
        paramIds.emplace_back(editData.strengthObject.paramName);
    }

    void DataConvertor::GetParamId(SpawnVelSphere& editData, AZStd::vector<AZ::TypeId>& paramIds)
    {
        paramIds.emplace_back(editData.strengthObject.paramName);
    }

    void DataConvertor::GetParamId(SpawnVelConcentrate& editData, AZStd::vector<AZ::TypeId>& paramIds)
    {
        paramIds.emplace_back(editData.rateObject.paramName);
    }

    void DataConvertor::GetParamId(SpawnLightEffect& editData, AZStd::vector<AZ::TypeId>& paramIds)
    {
        paramIds.emplace_back(editData.lightColorObject.paramName);
        paramIds.emplace_back(editData.intensityObject.paramName);
        paramIds.emplace_back(editData.radianScaleObject.paramName);
    }

    void DataConvertor::GetParamId(UpdateColor& editData, AZStd::vector<AZ::TypeId>& paramIds)
    {
        paramIds.emplace_back(editData.currentColorObject.paramName);
    }

    void DataConvertor::GetParamId(UpdateConstForce& editData, AZStd::vector<AZ::TypeId>& paramIds)
    {
        paramIds.emplace_back(editData.forceObject.paramName);
    }

    void DataConvertor::GetParamId(UpdateDragForce& editData, AZStd::vector<AZ::TypeId>& paramIds)
    {
        paramIds.emplace_back(editData.dragCoefficientObject.paramName);
    }

    void DataConvertor::GetParamId(UpdateSizeLinear& editData, AZStd::vector<AZ::TypeId>& paramIds)
    {
        paramIds.emplace_back(editData.sizeObject.paramName);
    }

    void DataConvertor::GetParamId(UpdateSizeByVelocity& editData, AZStd::vector<AZ::TypeId>& paramIds)
    {
        paramIds.emplace_back(editData.velScaleObject.paramName);
    }

    void DataConvertor::GetParamId(SizeScale& editData, AZStd::vector<AZ::TypeId>& paramIds)
    {
        paramIds.emplace_back(editData.scaleFactorObject.paramName);
    }

    void DataConvertor::GetParamId(UpdateVortexForce& editData, AZStd::vector<AZ::TypeId>& paramIds)
    {
        paramIds.emplace_back(editData.originPullObject.paramName);
        paramIds.emplace_back(editData.vortexRateObject.paramName);
        paramIds.emplace_back(editData.vortexRadiusObject.paramName);
    }

    void DataConvertor::GetParamId(UpdateVelocity& editData, AZStd::vector<AZ::TypeId>& paramIds)
    {
        paramIds.emplace_back(editData.strengthObject.paramName);
        paramIds.emplace_back(editData.directionObject.paramName);
    }
} // namespace OpenParticle
