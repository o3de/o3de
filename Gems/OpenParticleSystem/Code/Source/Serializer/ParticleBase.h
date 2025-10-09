/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/any.h>
#include <AzCore/std/containers/vector.h>

#include <AzCore/Math/Color.h>
#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Vector4.h>

#include <Runtime/OpenParticleSystem/ParticleConfig.h>
#include <Editor/EditorParticleSystemComponentRequestBus.h>

namespace OpenParticle
{
    enum class DistributionType
    {
        CONSTANT = 0,
        RANDOM,
        CURVE
    };

    enum class RandomTickMode
    {
        ONCE = 0,
        PER_FRAME,
        PER_SPAWN
    };

    enum class CurveTickMode
    {
        EMIT_DURATION = 0,
        PARTICLE_LIFETIME,
        NORMALIZED_AGE,
        CUSTOM
    };

    enum class CurveExtrapMode
    {
        CYCLE = 0,
        CYCLE_WHIT_OFFSET,
        CONSTANT
    };

    enum class KeyPointInterpMode
    {
        LINEAR = 0,
        STEP,
        CUBIC_IN,
        CUBIC_OUT,
        SINE_IN,
        SINE_OUT,
        CIRCLE_IN,
        CIRCLE_OUT
    };

    constexpr size_t DISTRIBUTION_COUNT_ONE = 1;
    constexpr size_t DISTRIBUTION_COUNT_TWO = 2;
    constexpr size_t DISTRIBUTION_COUNT_THREE = 3;
    constexpr size_t DISTRIBUTION_COUNT_FOUR = 4;

    template<typename T, size_t count>
    struct ValueObject
    {
        AZ_CLASS_ALLOCATOR(ValueObject, AZ::SystemAllocator, 0);

        ValueObject() = default;
        ValueObject(const AZStd::string& name, const T& value)
        {
            paramName = AZ::TypeId::CreateName(name.c_str());
            dataValue = value;
        }

        size_t Size() const
        {
            return count;
        };

        T dataValue;
        bool isUniform = false;
        DistributionType distType;
        AZ::TypeId paramName;
        AZStd::array<size_t, count> distIndex;
    };

    struct LinearValue
    {
        AZ_CLASS_ALLOCATOR(LinearValue, AZ::SystemAllocator, 0);

        AZ::Vector3 value;
        AZ::Vector3 minValue;
        AZ::Vector3 maxValue;
    };

    using ValueObjFloat = ValueObject<float, DISTRIBUTION_COUNT_ONE>;
    using ValueObjVec2 = ValueObject<AZ::Vector2, DISTRIBUTION_COUNT_TWO>;
    using ValueObjVec3 = ValueObject<AZ::Vector3, DISTRIBUTION_COUNT_THREE>;
    using ValueObjVec4 = ValueObject<AZ::Vector4, DISTRIBUTION_COUNT_FOUR>;
    using ValueObjColor = ValueObject<AZ::Color, DISTRIBUTION_COUNT_FOUR>;
    using ValueObjLinear = ValueObject<LinearValue, DISTRIBUTION_COUNT_THREE>;

    struct Distribution;

    struct SystemConfig
    {
        AZ_CLASS_ALLOCATOR(SystemConfig, AZ::SystemAllocator, 0);

        bool loop = true;
        bool parallel = true;
    };

    struct PreWarm
    {
        AZ_CLASS_ALLOCATOR(PreWarm, AZ::SystemAllocator, 0);
        void CheckParam();

        float warmupTime = 0.f;
        AZ::u32 tickCount = 0;
        float tickDelta = 0.f;
    };

    struct EmitterConfig
    {
        AZ_CLASS_ALLOCATOR(EmitterConfig, AZ::SystemAllocator, 0);
        void CheckParam();

        AZ::u32 maxSize = 500;
        bool localSpace = false;
        float startTime = 0.f;
        float duration = 2.f;
        SimuCore::ParticleCore::SimulateType type = SimuCore::ParticleCore::SimulateType::CPU;
        bool loop = true;
    };

    struct SingleBurst
    {
        AZ_CLASS_ALLOCATOR(SingleBurst, AZ::SystemAllocator, 0);
        float time = 0.f;
        AZ::u32 count = 0;
        int minCount = -1;
    };

    struct EmitBurstList
    {
        AZ_CLASS_ALLOCATOR(EmitBurstList, AZ::SystemAllocator, 0);
        bool isProcessBurstList = true;
        AZStd::vector<SingleBurst> burstList;
    };

    struct EmitSpawn
    {
        AZ_CLASS_ALLOCATOR(EmitSpawn, AZ::SystemAllocator, 0);
        void CheckParam();
        void ConvertDistIndexVersion(Distribution& distribution);

        ValueObjFloat spawnRateObject = { "spawnRateObject", 10.f };
        bool isProcessSpawnRate = true;
        AZ::u32 version = 1;
    };

    struct EmitSpawnOverMoving
    {
        AZ_CLASS_ALLOCATOR(EmitSpawnOverMoving, AZ::SystemAllocator, 0);
        void CheckParam();
        void ConvertDistIndexVersion(Distribution& distribution);

        ValueObjFloat spawnRatePerUnitObject = { "spawnRatePerUnitObject", 10.f };
        bool isIgnoreSpawnRateWhenMoving = false;
        bool isProcessSpawnRate = true;
        bool isProcessBurstList = true;
        AZ::u32 version = 1;
    };

    struct ParticleEventHandler : public EditorParticleDocumentBusRequestsBus::Handler
    {
        AZ_CLASS_ALLOCATOR(ParticleEventHandler, AZ::SystemAllocator, 0);
        ParticleEventHandler();
        ~ParticleEventHandler();

        void CheckParam();
        AZStd::vector<AZStd::string> GetEmitterNames() const;
        void OnEmitterNameChangedNotify(size_t index);

        // EditorParticleDocumentBusRequestsBus
        void OnEmitterNameChanged(ParticleSourceData* p) override;

        AZ::u32 emitterIndex = 0;
        AZStd::string emitterName;
        SimuCore::ParticleCore::ParticleEventType eventType = SimuCore::ParticleCore::ParticleEventType::SPAWN_LOCATION;
        AZ::u32 maxEventNum = 0;
        AZ::u32 emitNum = 1;
        bool useEventInfo = false;
    };

    struct InheritanceHandler : public EditorParticleDocumentBusRequestsBus::Handler
    {
        AZ_CLASS_ALLOCATOR(InheritanceHandler, AZ::SystemAllocator, 0);
        InheritanceHandler();
        ~InheritanceHandler();

        void CheckParam();
        AZStd::vector<AZStd::string> GetEmitterNames() const;
        void OnEmitterNameChangedNotify(size_t index);

        // EditorParticleDocumentBusRequestsBus
        void OnEmitterNameChanged(ParticleSourceData* p) override;

        AZ::Vector3 positionOffset = AZ::Vector3::CreateZero();
        AZ::Vector3 velocityRatio = AZ::Vector3::CreateZero();
        AZ::Vector4 colorRatio = AZ::Vector4( 1.f, 1.f, 1.f, 1.f );
        AZ::u32 emitterIndex = 0;
        AZStd::string emitterName;
        float spawnRate = 0.0f;
        bool calculateSpawnRate = false;
        bool spawnEnable = true;
        bool applyPosition = false;
        bool applyVelocity = false;
        bool overwriteVelocity = false;
        bool applySize = false;
        bool applyColorRGB = false;
        bool applyColorAlpha = false;
        bool applyAge = false;
        bool applyLifetime = false;

        inline bool PositionApplyed() const
        {
            return !applyPosition;
        }
        inline bool VelocityApplyed() const
        {
            return !applyVelocity;
        }
        inline bool ColorApplyed() const
        {
            return !applyColorRGB;
        }
        inline bool CalculateSpawnRate() const
        {
            return !calculateSpawnRate;
        }
    };

    struct SpawnColor
    {
        AZ_CLASS_ALLOCATOR(SpawnColor, AZ::SystemAllocator, 0);
        void CheckParam();
        void ConvertDistIndexVersion(Distribution& distribution);

        ValueObjColor startColorObject = { "startColorObject", { 1.f, 1.f, 1.f, 1.f } };
        AZ::u32 version = 1;
    };

    struct SpawnLifetime
    {
        AZ_CLASS_ALLOCATOR(SpawnLifetime, AZ::SystemAllocator, 0);
        void CheckParam();
        void ConvertDistIndexVersion(Distribution& distribution);

        ValueObjFloat lifeTimeObject = { "lifeTimeObject", 1.f };
        AZ::u32 version = 1;
    };

    struct SpawnLocBox
    {
        AZ_CLASS_ALLOCATOR(SpawnLocBox, AZ::SystemAllocator, 0);
        void CheckParam();

        AZ::Vector3 center = {};
        AZ::Vector3 size = AZ::Vector3(1.f, 1.f, 1.f);
    };

    struct SpawnLocPoint
    {
        AZ_CLASS_ALLOCATOR(SpawnLocPoint, AZ::SystemAllocator, 0);
        void ConvertDistIndexVersion(Distribution& distribution);

        ValueObjVec3 positionObject = { "positionObject", { 0.f, 0.f, 0.f } };
        AZ::u32 version = 1;
    };

    struct SpawnLocSphere
    {
        AZ_CLASS_ALLOCATOR(SpawnLocSphere, AZ::SystemAllocator, 0);
        void CheckParam();

        AZ::Vector3 center = { 0.f, 0.f, 0.f };
        float radius = 1.f;
        float ratio = 1.f;
        float angle = 360.f;
        float radiusThickness = 1.f;
        SimuCore::ParticleCore::Axis axis = SimuCore::ParticleCore::Axis::NO_AXIS;
    };

    struct SpawnLocCylinder
    {
        AZ_CLASS_ALLOCATOR(SpawnLocCylinder, AZ::SystemAllocator, 0);
        void CheckParam();

        AZ::Vector3 center = { 0.f, 0.f, 0.f };
        float radius = 1.f;
        float height = 1.f;
        float angle = 360.f;
        float radiusThickness = 0.f;
        SimuCore::ParticleCore::Axis axis = SimuCore::ParticleCore::Axis::Z_POSITIVE;
    };

    struct SpawnLocSkeleton
    {
        AZ_CLASS_ALLOCATOR(SpawnLocSkeleton, AZ::SystemAllocator, 0);
        SimuCore::ParticleCore::MeshSampleType sampleType = SimuCore::ParticleCore::MeshSampleType::BONE;
        AZ::Vector3 scale{ 1.f, 1.f, 1.f };
    };

    struct SpawnLocTorus
    {
        AZ_CLASS_ALLOCATOR(SpawnLocTorus, AZ::SystemAllocator, 0);
        void CheckParam();

        AZ::Vector3 center = { 0.f, 0.f, 0.f };
        float torusRadius = 5.f;  // the distance from the center of the tube to the center of the torus
        float tubeRadius = 1.f;   // the radius of the tube
        AZ::Vector3 torusAxis = { 0.f, 0.f, 1.f };
    };

    struct SpawnRotation
    {
        AZ_CLASS_ALLOCATOR(SpawnRotation, AZ::SystemAllocator, 0);
        void CheckParam();
        void ConvertDistIndexVersion(Distribution& distribution);

        ValueObjFloat initAngleObject = { "initAngleObject", 0.f };
        ValueObjFloat rotateSpeedObject = { "rotateSpeedObject", 0.f };
        AZ::Vector3 initAxis = { 0, 1, 0 };
        AZ::Vector3 rotateAxis = { 0, 1, 0 };
        AZ::u32 version = 1;
    };

    struct SpawnSize
    {
        AZ_CLASS_ALLOCATOR(SpawnSize, AZ::SystemAllocator, 0);
        void CheckParam();
        void ConvertDistIndexVersion(Distribution& distribution);

        ValueObjVec3 sizeObject = { "sizeObject", { 1.f, 1.f, 1.f } };
        AZ::u32 version = 1;
    };

    struct SpawnVelDirection
    {
        AZ_CLASS_ALLOCATOR(SpawnVelDirection, AZ::SystemAllocator, 0);
        void CheckParam();
        void ConvertDistIndexVersion(Distribution& distribution);

        ValueObjFloat strengthObject = { "strengthObject", 1.f };
        ValueObjVec3 directionObject = { "directionObject", { 0.f, 0.f, 1.f } };
        AZ::u32 version = 1;
    };

    struct SpawnVelSector
    {
        AZ_CLASS_ALLOCATOR(SpawnVelSector, AZ::SystemAllocator, 0);
        void CheckParam();
        void ConvertDistIndexVersion(Distribution& distribution);

        ValueObjFloat strengthObject = { "strengthObject", 1.f };
        AZ::Vector3 direction = { 0.f, 0.f, 1.f };
        float centralAngle = 60.f;
        float rotateAngle = 60.f;
        AZ::u32 version = 1;
    };

    struct SpawnVelCone
    {
        AZ_CLASS_ALLOCATOR(SpawnVelCone, AZ::SystemAllocator, 0);
        void CheckParam();
        void ConvertDistIndexVersion(Distribution& distribution);

        float angle = 60.f;
        ValueObjFloat strengthObject = { "strengthObject", 1.f };
        AZ::Vector3 direction = { 0.f, 0.f, 1.f };
        AZ::u32 version = 1;
    };

    struct SpawnVelSphere
    {
        AZ_CLASS_ALLOCATOR(SpawnVelSphere, AZ::SystemAllocator, 0);
        void CheckParam();
        void ConvertDistIndexVersion(Distribution& distribution);

        ValueObjVec3 strengthObject = { "strengthObject", { 1.f, 1.f, 1.f } };
        AZ::u32 version = 1;
    };

    struct SpawnVelConcentrate
    {
        AZ_CLASS_ALLOCATOR(SpawnVelConcentrate, AZ::SystemAllocator, 0);
        void CheckParam();
        void ConvertDistIndexVersion(Distribution& distribution);

        ValueObjFloat rateObject = { "rateObject", 1.f };
        AZ::Vector3 centre = { 0.f, 0.f, 0.f };
        AZ::u32 version = 1;
    };

    struct SpawnLightEffect
    {
        AZ_CLASS_ALLOCATOR(SpawnLightEffect, AZ::SystemAllocator, 0);
        void ConvertDistIndexVersion(Distribution& distribution);

        ValueObjColor lightColorObject = { "lightColorObject", { 1.f, 1.f, 1.f, 1.f } };
        ValueObjFloat intensityObject = { "intensityObject", 1.f };
        ValueObjFloat radianScaleObject = { "radianScaleObject", 1.f };
        AZ::u32 version = 1;
    };

    struct SpawnLocationEvent
    {
        AZ_CLASS_ALLOCATOR(SpawnLocationEvent, AZ::SystemAllocator, 0);

        bool whetherSendEvent = true;
    };

    struct UpdateColor
    {
        AZ_CLASS_ALLOCATOR(UpdateColor, AZ::SystemAllocator, 0);
        void CheckParam();
        void ConvertDistIndexVersion(Distribution& distribution);

        ValueObjColor currentColorObject = { "currentColorObject", { 1.f, 1.f, 1.f, 1.f } };
        AZ::u32 version = 1;
    };

    struct UpdateLocationEvent
    {
        AZ_CLASS_ALLOCATOR(UpdateLocationEvent, AZ::SystemAllocator, 0);

        bool whetherSendEvent = true;
    };

    struct UpdateDeathEvent
    {
        AZ_CLASS_ALLOCATOR(UpdateDeathEvent, AZ::SystemAllocator, 0);

        bool whetherSendEvent = true;
    };

    struct UpdateCollisionEvent
    {
        AZ_CLASS_ALLOCATOR(UpdateCollisionEvent, AZ::SystemAllocator, 0);

        bool whetherSendEvent = true;
    };

    struct UpdateInheritanceEvent
    {
        AZ_CLASS_ALLOCATOR(UpdateInheritanceEvent, AZ::SystemAllocator, 0);

        bool whetherSendEvent = true;
    };

    struct UpdateConstForce
    {
        AZ_CLASS_ALLOCATOR(UpdateConstForce, AZ::SystemAllocator, 0);
        void ConvertDistIndexVersion(Distribution& distribution);

        ValueObjVec3 forceObject = { "forceObject", { 0.f, 0.f, 0.f } };
        AZ::u32 version = 1;
    };

    struct UpdateDragForce
    {
        AZ_CLASS_ALLOCATOR(UpdateDragForce, AZ::SystemAllocator, 0);
        void CheckParam();
        void ConvertDistIndexVersion(Distribution& distribution);

        ValueObjFloat dragCoefficientObject = { "dragCoefficientObject", 1.f };
        AZ::u32 version = 1;
    };

    struct UpdateVortexForce
    {
        AZ_CLASS_ALLOCATOR(UpdateVortexForce, AZ::SystemAllocator, 0);
        void CheckParam();

        ValueObjFloat originPullObject = { "originPullObject", 1.f };
        ValueObjFloat vortexRateObject = { "vortexRateObject", 1.f };
        ValueObjFloat vortexRadiusObject = { "vortexRadiusObject", 0.f };
        AZ::Vector3 vortexAxis = { 0.f, 0.f, 1.f };
        AZ::Vector3 origin = { 0.f, 0.f, 0.f };
        AZ::u32 version = 1;
    };

    struct UpdateCurlNoiseForce
    {
        AZ_CLASS_ALLOCATOR(UpdateCurlNoiseForce, AZ::SystemAllocator, 0);
        void CheckParam();

        float noiseStrength = 10.0f;
        float noiseFrequency = 5.0f;
        bool panNoise = false;
        AZ::Vector3 panNoiseField;
        AZ::u32 randomSeed = 0;
        AZ::Vector3 randomizationVector;
    };

    struct UpdateSizeLinear
    {
        AZ_CLASS_ALLOCATOR(UpdateSizeLinear, AZ::SystemAllocator, 0);
        void CheckParam();
        void ConvertDistIndexVersion(Distribution& distribution);

        ValueObjVec3 sizeObject = { "sizeObject", { 1.f, 1.f, 1.f } };
        AZ::u32 version = 1;
    };

    struct UpdateSizeByVelocity
    {
        AZ_CLASS_ALLOCATOR(UpdateSizeByVelocity, AZ::SystemAllocator, 0);
        void CheckParam();
        void ConvertDistIndexVersion(Distribution& distribution);

        ValueObjLinear velScaleObject = { "velScaleObject",
            {
                { 1.f, 1.f, 1.f },
                { 0.1f, 0.1f, 0.1f },
                { 1.f, 1.f, 1.f }
            } };
        float velocityRange = 1.f;
        AZ::u32 version = 1;
    };

    struct SizeScale
    {
        AZ_CLASS_ALLOCATOR(SizeScale, AZ::SystemAllocator, 0);
        void CheckParam();
        void ConvertDistIndexVersion(Distribution& distribution);

        ValueObjVec3 scaleFactorObject = { "scaleFactorObject", { 1.f, 1.f, 1.f } };
        AZ::u32 version = 1;
    };

    struct UpdateSubUv
    {
        AZ_CLASS_ALLOCATOR(UpdateSubUv, AZ::SystemAllocator, 0);
        void CheckParam();

        AZ::u32 framePerSecond = 30;
        AZ::u32 frameNum = 1;
        bool spawnOnly = false;
        bool IndexByEventOrder = false;
    };

    struct UpdateRotateAroundPoint
    {
        AZ_CLASS_ALLOCATOR(UpdateRotateAroundPoint, AZ::SystemAllocator, 0);
        void CheckParam();

        float rotateRate = 1.0f;
        float radius = 1.0f;
        AZ::Vector3 xAxis = { 1.0f, 0.f, 0.f };
        AZ::Vector3 yAxis = { 0.f, 1.0f, 0.f };
        AZ::Vector3 center = { 0.f, 0.f, 0.f };
    };

    struct UpdateVelocity
    {
        AZ_CLASS_ALLOCATOR(UpdateVelocity, AZ::SystemAllocator, 0);
        void ConvertDistIndexVersion(Distribution& distribution);

        ValueObjFloat strengthObject = { "strengthObject", 1.f };
        ValueObjVec3 directionObject = { "directionObject", { 0.f, 0.f, 1.f } };
        AZ::u32 version = 1;
    };

    struct CollisionPlane
    {
        AZ_CLASS_ALLOCATOR(CollisionPlane, AZ::SystemAllocator, 0);

        AZ::Vector3 normal = { 0.f, 0.f, 1.f };
        AZ::Vector3 position = { 0.f, 0.f, 0.f };
    };

    struct CollisionRadius
    {
        AZ_CLASS_ALLOCATOR(CollisionRadius, AZ::SystemAllocator, 0);

        SimuCore::ParticleCore::RadiusCalculationType type = SimuCore::ParticleCore::RadiusCalculationType::SPRITE;
        SimuCore::ParticleCore::RadiusCalculationMethod method = SimuCore::ParticleCore::RadiusCalculationMethod::BOUNDS;
        float radius = 1.f;
        float radiusScale = 1.f;

        inline AZ::Crc32 RadiusVisibility() const
        {
            return (type == SimuCore::ParticleCore::RadiusCalculationType::CUSTOM) ? AZ_CRC("PropertyVisibility_Show", 0xa43c82dd)
                                                                              : AZ_CRC("PropertyVisibility_Hide", 0x32ab90f7);
        }
    };

    struct Bounce
    {
        AZ_CLASS_ALLOCATOR(Bounce, AZ::SystemAllocator, 0);

        float restitution = 1.f;
        float randomizeNormal = 0.f;
    };

    struct ParticleCollision
    {
        AZ_CLASS_ALLOCATOR(ParticleCollision, AZ::SystemAllocator, 0);
        void CheckParam();
        SimuCore::ParticleCore::CpuCollisionType collisionType = SimuCore::ParticleCore::CpuCollisionType::PLANE;
        CollisionRadius collisionRadius;
        Bounce bounce;
        SimuCore::ParticleCore::Friction friction;
        bool useTwoPlane = false;
        CollisionPlane collisionPlane1;
        CollisionPlane collisionPlane2;

        AZ::Crc32 UseTwoPlaneIsSelected() const {
            return useTwoPlane ? AZ_CRC("PropertyVisibility_Show", 0xa43c82dd) : AZ_CRC("PropertyVisibility_Hide", 0x32ab90f7);
        }
    };

    struct SpriteConfig
    {
        AZ_CLASS_ALLOCATOR(SpriteConfig, AZ::SystemAllocator, 0);
        void CheckParam();

        static constexpr SimuCore::ParticleCore::RenderType RENDER = SimuCore::ParticleCore::RenderType::SPRITE;
        SimuCore::ParticleCore::Facing facing = SimuCore::ParticleCore::Facing::CAMERA_RECTANGLE;
        AZ::u32 sortId = 0;
        AZ::Vector2 subImageSize{ 1.0f, 1.0f };
    };

    struct MeshConfig
    {
        AZ_CLASS_ALLOCATOR(MeshConfig, AZ::SystemAllocator, 0);
        void CheckParam();

        static constexpr SimuCore::ParticleCore::RenderType RENDER = SimuCore::ParticleCore::RenderType::MESH;
        SimuCore::ParticleCore::Facing facing = SimuCore::ParticleCore::Facing::CAMERA_RECTANGLE;
        AZ::u32 sortId = 0;
    };

    struct TrailParam
    {
        AZ_CLASS_ALLOCATOR(TrailParam, AZ::SystemAllocator, 0);

        float ratio = 1.0;
        float lifetime = 0.0;
        bool inheritLifetime = true;
        bool dieWithParticles = false;
    };

    struct RibbonParam
    {
        AZ_CLASS_ALLOCATOR(RibbonParam, AZ::SystemAllocator, 0);

        AZ::u32 ribbonCount = 1u;
    };

    struct RibbonConfig
    {
        AZ_CLASS_ALLOCATOR(RibbonConfig, AZ::SystemAllocator, 0);
        void CheckParam();
        void ConvertDistIndexVersion(Distribution& distribution);

        static constexpr SimuCore::ParticleCore::RenderType RENDER = SimuCore::ParticleCore::RenderType::RIBBON;

        TrailParam trailParam;
        RibbonParam ribbonParam;
        AZ::u32 sortId = 0;
        float minRibbonSegmentLength = 0.01f;
        ValueObjFloat ribbonWidthObject = { "ribbonWidthObject", 0.1f };
        bool inheritSize = false;
        float tesselationFactor = 0.01f;
        float curveTension = 0.f;
        float tilingDistance = 0.f;
        SimuCore::ParticleCore::RibbonFacing facing = SimuCore::ParticleCore::RibbonFacing::SCREEN;
        SimuCore::ParticleCore::TrailMode mode = SimuCore::ParticleCore::TrailMode::RIBBON;
        AZ::u32 version = 1;

        AZ::Crc32 ModeChangToRibbon() const
        {
            return mode == SimuCore::ParticleCore::TrailMode::RIBBON ?
                AZ_CRC("PropertyVisibility_Show", 0xa43c82dd) : AZ_CRC("PropertyVisibility_Hide", 0x32ab90f7);
        }

        AZ::Crc32 ModeChangToTrail() const
        {
            return mode == SimuCore::ParticleCore::TrailMode::TRAIL ?
                AZ_CRC("PropertyVisibility_Show", 0xa43c82dd) : AZ_CRC("PropertyVisibility_Hide", 0x32ab90f7);
        }
    };

    struct KeyPoint
    {
        AZ_CLASS_ALLOCATOR(KeyPoint, AZ::SystemAllocator, 0);

        float time = 0.0f;
        float value = 0.0f;
        KeyPointInterpMode interpMode = KeyPointInterpMode::LINEAR;
    };

    struct Curve
    {
        AZ_CLASS_ALLOCATOR(Curve, AZ::SystemAllocator, 0);

        static Curve* InitCurve();

        CurveExtrapMode leftExtrapMode = CurveExtrapMode::CYCLE;
        CurveExtrapMode rightExtrapMode = CurveExtrapMode::CYCLE;
        float valueFactor = 1.0f;
        float timeFactor = 1.0f;
        CurveTickMode tickMode = CurveTickMode::EMIT_DURATION;
        AZStd::vector<KeyPoint> keyPoints;
    };

    struct Random
    {
        AZ_CLASS_ALLOCATOR(Random, AZ::SystemAllocator, 0);

        float min = 0.0f;
        float max = 0.0f;
        RandomTickMode tickMode = RandomTickMode::ONCE;
    };

    struct ParamDistInfo
    {
        AZ_CLASS_ALLOCATOR(ParamDistInfo, AZ::SystemAllocator, 0);
        static void Reflect(AZ::ReflectContext* context);
        void* valueObjPtr = nullptr;
        AZ::TypeId valueTypeId;
        bool isUniform = false;
        AZ::TypeId paramName;
        size_t paramIndex = 0;
        size_t distIndex = 0;
    };

    struct DistInfos
    {
        AZ_CLASS_ALLOCATOR(DistInfos, AZ::SystemAllocator, 0);
        static void Reflect(AZ::ReflectContext* context);

        void Clear();
        AZStd::vector<ParamDistInfo>* operator()(const OpenParticle::DistributionType& type);

        AZStd::vector<ParamDistInfo> randomIndexInfos;
        AZStd::vector<ParamDistInfo> curveIndexInfos;
    };

    struct Distribution
    {
        AZ_CLASS_ALLOCATOR(Distribution, AZ::SystemAllocator, 0);

        static void Reflect(AZ::ReflectContext* context);

        struct CurveCache
        {
            int activeAxis;
            AZStd::array<Curve*, 4> curves;
        };

        void Clear()
        {
            for (auto random : randoms)
            {
                delete random;
                random = nullptr;
            }
            for (auto curve : curves)
            {
                delete curve;
                curve = nullptr;
            }
            randoms.clear();
            curves.clear();
            distInfos.Clear();
        }

        void ClearCaches()
        {
            randomCaches.clear();
            curveCaches.clear();
        }

        void GetDistributionIndex(AZStd::list<AZStd::any>& dist);
        void GetDistributionIndex(AZStd::any& dist);
        void ClearDistributionIndex();
        void SortDistributionIndex();
        void RebuildDistribution();
        void RebuildDistributionIndex();
        bool CheckDistributionIndex();

        AZStd::vector<Random*> randoms;
        AZStd::vector<Curve*> curves;

        // sourceData module key
        AZStd::unordered_map<AZStd::string, AZStd::vector<Random*>> randomCaches;
        AZStd::unordered_map<AZStd::string, CurveCache> curveCaches;

        DistInfos distInfos;

        AZStd::vector<size_t> stashedRandomIndexes;
        AZStd::vector<size_t> stashedCurveIndexes;
    };
} // namespace OpenParticle

namespace AZ
{
    AZ_TYPE_INFO_SPECIALIZE(OpenParticle::DistributionType, "{C54E9E8F-3674-4422-89A3-67CBA3D823C7}");
    AZ_TYPE_INFO_SPECIALIZE(OpenParticle::RandomTickMode, "{0C2A017F-796E-476C-8A25-F41D406682EB}");
    AZ_TYPE_INFO_SPECIALIZE(OpenParticle::CurveTickMode, "{E78EA98C-1071-4126-982E-D5702E677856}");
    AZ_TYPE_INFO_SPECIALIZE(OpenParticle::CurveExtrapMode, "{B57C617D-DFCF-4ED3-8F74-EEA77DD846A7}");
    AZ_TYPE_INFO_SPECIALIZE(OpenParticle::KeyPointInterpMode, "{FC7881B3-DDAA-49F2-B488-F3C139B9592C}");

    AZ_TYPE_INFO_SPECIALIZE(OpenParticle::ValueObjFloat, "{B39048D9-C568-4546-B6FD-C24304256C84}");
    AZ_TYPE_INFO_SPECIALIZE(OpenParticle::ValueObjVec2, "{AD8CDC0E-C389-4E0B-AF6E-EB25BDCC5282}");
    AZ_TYPE_INFO_SPECIALIZE(OpenParticle::ValueObjVec3, "{D3652918-D8AE-469D-A176-0193D819DEE2}");
    AZ_TYPE_INFO_SPECIALIZE(OpenParticle::ValueObjVec4, "{15E82A32-C118-4E7C-BB57-30BB3CCE0F2F}");
    AZ_TYPE_INFO_SPECIALIZE(OpenParticle::ValueObjColor, "{D806DE01-6907-4F67-AFC2-5D0084184DF9}");
    AZ_TYPE_INFO_SPECIALIZE(OpenParticle::LinearValue, "{39282FD5-967D-448B-893B-035F9E1105D8}");
    AZ_TYPE_INFO_SPECIALIZE(OpenParticle::ValueObjLinear, "{C37AB3E1-2046-47CB-B24B-B2690ACF5FD8}");

    AZ_TYPE_INFO_SPECIALIZE(OpenParticle::SystemConfig, "{6937d5b3-8d3d-47dd-a006-13cb1651e932}");
    AZ_TYPE_INFO_SPECIALIZE(OpenParticle::PreWarm, "{421922dc-1132-4c86-9bb7-2498c2d0f8b8}");
    AZ_TYPE_INFO_SPECIALIZE(OpenParticle::EmitterConfig, "{c8b49ff4-3f8a-4079-ad5d-204d6031c7e6}");

    AZ_TYPE_INFO_SPECIALIZE(OpenParticle::SingleBurst, "{6137406C-8348-435D-9D15-0AA4148CA76B}");
    AZ_TYPE_INFO_SPECIALIZE(OpenParticle::EmitBurstList, "{436ecfc8-6579-4604-90b5-0ca53509b0c8}");
    AZ_TYPE_INFO_SPECIALIZE(OpenParticle::EmitSpawn, "{07c4d53c-e7bf-4e30-a3c7-c1cc5ddd3552}");
    AZ_TYPE_INFO_SPECIALIZE(OpenParticle::EmitSpawnOverMoving, "{750bbbf8-c5ec-44da-9203-fea65fc18991}");
    AZ_TYPE_INFO_SPECIALIZE(OpenParticle::ParticleEventHandler, "{8CEA39F1-EC02-4082-8B49-3184AA4A9777}");
    AZ_TYPE_INFO_SPECIALIZE(OpenParticle::InheritanceHandler, "{321EACDE-60B8-4B99-8EE5-E62F0617452D}");

    AZ_TYPE_INFO_SPECIALIZE(OpenParticle::SpawnColor, "{5463bfac-d716-4609-8aa1-2d4d3b5e5794}");
    AZ_TYPE_INFO_SPECIALIZE(OpenParticle::SpawnLifetime, "{bc528310-19e5-4c3c-9982-bf342dbc9229}");
    AZ_TYPE_INFO_SPECIALIZE(OpenParticle::SpawnLocBox, "{9154a595-d110-4aca-a143-bd748b70916c}");
    AZ_TYPE_INFO_SPECIALIZE(OpenParticle::SpawnLocPoint, "{99c5c1df-c45e-4ab6-ad3d-4cdcbefecfba}");
    AZ_TYPE_INFO_SPECIALIZE(OpenParticle::SpawnLocSphere, "{e91883a2-488f-4082-85ca-fdbd9391e275}");
    AZ_TYPE_INFO_SPECIALIZE(OpenParticle::SpawnLocCylinder, "{556c2576-083c-4c99-be23-405eb65c6d89}");
    AZ_TYPE_INFO_SPECIALIZE(OpenParticle::SpawnLocSkeleton, "{928A3C7C-3663-E3D2-7EAA-532F9DF5A2C9}");
    AZ_TYPE_INFO_SPECIALIZE(OpenParticle::SpawnLocTorus, "{e4fd1f3d-96bb-4f7e-aa78-86f4d3812280}");
    AZ_TYPE_INFO_SPECIALIZE(OpenParticle::SpawnLightEffect, "{3ec37ebb-6095-4a71-a430-6de11bf361ca}");
    AZ_TYPE_INFO_SPECIALIZE(OpenParticle::SpawnSize, "{056d77be-9520-43c0-a4f8-d186eb42bda8}");
    AZ_TYPE_INFO_SPECIALIZE(OpenParticle::SpawnVelDirection, "{0cc5af4d-56ee-481f-ba88-23f03c198a6d}");
    AZ_TYPE_INFO_SPECIALIZE(OpenParticle::SpawnVelSector, "{f8fcfea4-2449-45f3-a8c1-5c8087ee71a2}");
    AZ_TYPE_INFO_SPECIALIZE(OpenParticle::SpawnVelCone, "{ab2f60a6-82cc-433f-9ca1-28d99ccc0628}");
    AZ_TYPE_INFO_SPECIALIZE(OpenParticle::SpawnVelSphere, "{79960c0a-0148-487b-bcc5-cbfb9bd7c127}");
    AZ_TYPE_INFO_SPECIALIZE(OpenParticle::SpawnVelConcentrate, "{3464bcd5-e006-4d2a-b8a8-4b2c98c413a8}");
    AZ_TYPE_INFO_SPECIALIZE(OpenParticle::SpawnRotation, "{111a4002-83c1-4e85-a500-2a123a7a8379}");
    AZ_TYPE_INFO_SPECIALIZE(OpenParticle::SpawnLocationEvent, "{EAA499D2-BB18-44B3-B2CC-95997469C41A}");
    AZ_TYPE_INFO_SPECIALIZE(OpenParticle::UpdateConstForce, "{c64373f1-057f-4d2a-af87-e0dec7518856}");
    AZ_TYPE_INFO_SPECIALIZE(OpenParticle::UpdateDragForce, "{a55784e9-f0e0-460f-b090-01668969c39f}");
    AZ_TYPE_INFO_SPECIALIZE(OpenParticle::UpdateCurlNoiseForce, "{5F699F47-98DF-4C1A-A074-E6115BAA4DF4}");
    AZ_TYPE_INFO_SPECIALIZE(OpenParticle::UpdateVortexForce, "{48e41935-944c-4353-acc0-7e087c20eb6b}");
    AZ_TYPE_INFO_SPECIALIZE(OpenParticle::UpdateColor, "{aebf2070-8024-4492-9dc7-90f5efdf8cb0}");
    AZ_TYPE_INFO_SPECIALIZE(OpenParticle::UpdateLocationEvent, "{C7D01C57-D71D-43D8-AA4E-F2A1981F36F1}");
    AZ_TYPE_INFO_SPECIALIZE(OpenParticle::UpdateDeathEvent, "{97052A37-8019-4F60-A8D9-97C36B63C00B}");
    AZ_TYPE_INFO_SPECIALIZE(OpenParticle::UpdateCollisionEvent, "{79AA81AC-7CDE-4D08-9169-9CD8F360C407}");
    AZ_TYPE_INFO_SPECIALIZE(OpenParticle::UpdateInheritanceEvent, "{6780C79E-7C9A-497C-A713-26200805DC41}");
    AZ_TYPE_INFO_SPECIALIZE(OpenParticle::UpdateSizeLinear, "{e6f6975f-e956-453a-9037-5476ec8986d6}");
    AZ_TYPE_INFO_SPECIALIZE(OpenParticle::UpdateSizeByVelocity, "{5656227b-646b-41f1-9e82-7db39590b995}");
    AZ_TYPE_INFO_SPECIALIZE(OpenParticle::SizeScale, "{49ee5754-6a5c-4eff-9688-8bf54d292bee}");
    AZ_TYPE_INFO_SPECIALIZE(OpenParticle::UpdateSubUv, "{35cdb3cc-260e-41b1-adc8-7830f0a7579d}");
    AZ_TYPE_INFO_SPECIALIZE(OpenParticle::UpdateRotateAroundPoint, "{14085f26-0a2b-42e3-8791-91e976f4cf3a}");
    AZ_TYPE_INFO_SPECIALIZE(OpenParticle::UpdateVelocity, "{639a0289-65cc-4cae-9a29-40be6b2df5b9}");
    AZ_TYPE_INFO_SPECIALIZE(OpenParticle::CollisionPlane, "{D52A67D6-1017-4D77-B08C-4E25133CA828}");
    AZ_TYPE_INFO_SPECIALIZE(OpenParticle::ParticleCollision, "{7B7B0CDF-2263-4C09-87ED-93E389C5C8C3}");
    AZ_TYPE_INFO_SPECIALIZE(OpenParticle::CollisionRadius, "{E28F15E9-7D08-4D02-86D0-A6DCF9034473}");
    AZ_TYPE_INFO_SPECIALIZE(OpenParticle::Bounce, "{7F080E79-DE6B-4050-80C8-C478EA9B959E}");

    AZ_TYPE_INFO_SPECIALIZE(OpenParticle::KeyPoint, "{7e499d19-6092-4434-942b-d20c5bbcdc54}");
    AZ_TYPE_INFO_SPECIALIZE(OpenParticle::Curve, "{623647D9-7089-42A3-9DB3-78985D3F7704}");
    AZ_TYPE_INFO_SPECIALIZE(OpenParticle::Random, "{caf04ca4-8bfc-4320-82d5-5939dc297dc3}");
    AZ_TYPE_INFO_SPECIALIZE(OpenParticle::ParamDistInfo, "{0D801392-DD44-4716-BB91-636942EC54F5}");
    AZ_TYPE_INFO_SPECIALIZE(OpenParticle::DistInfos, "{6C3FD8F3-FF05-43AF-AC6B-C386AFA040D8}");
    AZ_TYPE_INFO_SPECIALIZE(OpenParticle::Distribution, "{C0B97C93-4F51-466F-A9DB-D63417C973CB}");

    AZ_TYPE_INFO_SPECIALIZE(OpenParticle::SpriteConfig, "{17bfa14b-5e19-46f9-a2a9-7712d3f69e52}");
    AZ_TYPE_INFO_SPECIALIZE(OpenParticle::MeshConfig, "{95e33d7f-c60a-4572-841b-522651f777ee}");
    AZ_TYPE_INFO_SPECIALIZE(OpenParticle::TrailParam, "{391EBB0C-4BF8-4092-BC41-21879E83F8F5}");
    AZ_TYPE_INFO_SPECIALIZE(OpenParticle::RibbonParam, "{142B7183-6376-405D-9836-832EE759D2D2}");
    AZ_TYPE_INFO_SPECIALIZE(OpenParticle::RibbonConfig, "{5ef04cdb-4acb-481b-893f-a341d238396d}");
} // namespace AZ
