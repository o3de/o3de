/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once


#include <AzCore/Math/Transform.h>
#include <AzCore/Math/Color.h>
#include "core/math/RandomStream.h"
#include <AzCore/std/containers/array.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/vector.h>

namespace SimuCore::ParticleCore {
    struct Particle {
        AZ::Transform spawnTrans;    // transform when spawn
        AZ::Vector3 localPosition;     // local position in emitter
        AZ::Vector3 globalPosition;    // global position in world
        AZ::Vector3 velocity;          // velocity
        AZ::Vector3 baseScale = AZ::Vector3::CreateOne();
        AZ::Vector3 scale = AZ::Vector3::CreateOne();              // scale
        AZ::Vector4 rotation = AZ::Vector4::CreateAxisY();        // xyz: axis w: angle
        AZ::Vector4 rotationVector = AZ::Vector4::CreateAxisY();  // xyz: axis w: angle
        AZ::Vector4 rotateAroundPoint; // xyz: rotation axis w: angle
        AZ::Color color = AZ::Colors::White;             // rgba
        AZ::Color lightColor = AZ::Colors::White;        // light effect AZ::Color
        AZ::Vector3 collisionPosition;
        AZ::u64 id = UINT64_MAX;              // particle unique id
        AZ::u64 ribbonId = UINT64_MAX;        // in which ribbon
        AZ::u32 subUVFrame = 0u;              // subuv frame
        AZ::u32 locationEventCount = 0u;      // total location events generated
        AZ::u32 parentEventIdx = 0u;          // when spawn triggered by event, record parent event index  when spawn triggered by event
        float lifeTime = 0.f;                  // life time
        float currentLife = 0.f;               // current life time
        float angularVel = 0.f;                // angular velocity
        float radianScale  = 0.f;              // light color scale
        float collisionTimeBeforeTick = 0.0f;
        bool hasLightEffect = false;
        bool needKill = false;                 // whether kill particle in this tick
        bool isCollided = false;               // whether particle collide in this tick
        bool spawnSubUVDone{false};            // whether subUV has updated for spawnOnly scene
    };

    struct ParticleSpriteVertex {
        AZ::Vector4 position{ 0.f, 0.f, 0.f, 0.f};
        AZ::Color color{ 1.f, 1.f, 1.f, 1.f };
        AZ::Vector4 initRotation{ 0.f, 1.f, 0.f, 0.f }; // axis-angle
        AZ::Vector4 rotationVector{ 0.f, 1.f, 0.f, 0.f }; // axis-angle
        AZ::Vector4 scale{ 1.f, 1.f, 1.f, 1.f};
        AZ::Vector4 up{ 0.f, 1.f, 0.f, 0.f };
        AZ::Vector4 velocity{0.f, 0.f, 0.f, 0.f};
        AZ::Vector4 subuv{ 1.0f, 1.0f, 0.f, 0.f };
    };

    struct ParticleMeshVertex {
        AZ::Vector3 position{ 0.f, 0.f, 0.f };
        AZ::Color color{ 1.f, 1.f, 1.f, 1.f };
        AZ::Vector4 initRotation{ 0.f, 1.f, 0.f, 0.f }; // axis-angle
        AZ::Quaternion rotationVector{ 0.f, 0.f, 0.f, 1.f }; // quaternion
        AZ::Vector3 scale{ 1.f, 1.f, 1.f };
    };

    struct ParticleRibbonVertex {
        AZ::Vector4 position{ 0.f, 0.f, 0.f, 0.f};
        AZ::Color color{ 1.f, 1.f, 1.f, 1.f };
        AZ::Vector2 uv{ 0.5f, 0.5f };
    };

    enum class RenderType : AZ::u8 {
        SPRITE,
        MESH,
        RIBBON,
        UNDEFINED
    };

    enum class SimulateType : AZ::u8 {
        CPU,
        GPU
    };

    enum class Facing : AZ::u8 {
        CAMERA_POS,
        CAMERA_SQUARE,
        CAMERA_RECTANGLE,
        VELOCITY,
        CUSTOM
    };

    enum class RibbonFacing : AZ::u8 {
        SCREEN,
        CAMERA
    };

    enum class DrawType : AZ::u8 {
        LINEAR,
        INDEXED,
        INDIRECT
    };

    enum class Axis : AZ::u8 {
        X_POSITIVE,
        X_NEGATIVE,
        Y_POSITIVE,
        Y_NEGATIVE,
        Z_POSITIVE,
        Z_NEGATIVE,
        NO_AXIS
    };

    enum class MeshSampleType : AZ::u8 {
        BONE,
        VERTEX,
        AREA
    };
    
    enum class DistributionType : AZ::u8 {
        CONSTANT = 0,
        RANDOM,
        CURVE
    };

    enum class RandomTickMode : AZ::u8 {
        ONCE = 0,
        PER_FRAME,
        PER_SPAWN
    };

    enum class CurveTickMode : AZ::u8 {
        EMIT_DURATION = 0,
        PARTICLE_LIFETIME,
        NORMALIZED_AGE,
        CUSTOM
    };

    enum class KeyPointInterpMode : AZ::u8 {
        LINEAR = 0,
        STEP,
        CUBIC_IN,
        CUBIC_OUT,
        SINE_IN,
        SINE_OUT,
        CIRCLE_IN,
        CIRCLE_OUT
    };

    enum class CurveExtrapMode : AZ::u8 {
        CYCLE = 0,
        CYCLE_WITH_OFFSET,
        CONSTANT
    };

    struct KeyPoint {
        KeyPoint() = default;
        KeyPoint(float val, float time, const KeyPointInterpMode& mode) : value(val),
            time(time), interpMode(mode) {};
        ~KeyPoint() = default;

        float value;
        float time;
        KeyPointInterpMode interpMode;
    };

    struct SamplerInfo {
        SamplerInfo() = default;
        ~SamplerInfo() = default;
    };

    struct CurveSamplerInfo : public SamplerInfo {
        CurveSamplerInfo() = default;
        CurveSamplerInfo(float point, float range): samplePoint(point), sampleRange(range) {};
        ~CurveSamplerInfo() = default;
        float samplePoint = 0.0f;
        float sampleRange = 1.0f;
    };

    struct RandomSamplerInfo : public SamplerInfo {
        RandomSamplerInfo() = default;
        explicit RandomSamplerInfo(bool value): needRegenerate(value) {};
        ~RandomSamplerInfo() = default;
        bool needRegenerate = true;
    };


    template<typename T>
    struct TypeInfo {
        static constexpr AZStd::string_view Name() noexcept
        {
            AZStd::string_view id(AZ_FUNCTION_SIGNATURE);
            auto first = id.find_first_of(' ', id.find_last_of('<') + 1) + 1;
            auto last = id.find_last_of('>');
            return id.substr(first, last - first);
        }
    };

    class ParticleDistribution;

    using Distribution = AZStd::unordered_map<DistributionType, AZStd::vector<ParticleDistribution*>>;

    template<typename T, AZ::u32 size>
    struct ValueObject {
        ValueObject() = default;
        explicit ValueObject(const T& t) : dataValue(t) {}

        AZStd::array<ParticleDistribution*, size> distributions = { nullptr };
        AZStd::array<AZ::u64, size> distIndex;
        T dataValue;
        DistributionType distType = DistributionType::CONSTANT;
        bool isUniform = false;
        AZ::u16 padding0 = 0;
        AZ::u32 padding1 = 0;
        AZ::u64 padding2 = 0;
    };

    template<>
    struct ValueObject<float, 1> {
        ValueObject() = default;
        explicit ValueObject(const float& t) : dataValue(t) {}

        AZStd::array<ParticleDistribution*, 1> distributions = { nullptr };
        AZStd::array<AZ::u64, 1> distIndex;
        float dataValue;
        DistributionType distType = DistributionType::CONSTANT;
        bool isUniform = false;
        AZ::u16 padding0 = 0;
    };

    struct LinearValue {
        AZ::Vector3 value;
        AZ::Vector3 minValue;
        AZ::Vector3 maxValue;
    };

    constexpr AZ::u32 DISTRIBUTION_COUNT_ONE = 1;
    constexpr AZ::u32 DISTRIBUTION_COUNT_TWO = 2;
    constexpr AZ::u32 DISTRIBUTION_COUNT_THREE = 3;
    constexpr AZ::u32 DISTRIBUTION_COUNT_FOUR = 4;

    using ValueObjFloat = ValueObject<float, DISTRIBUTION_COUNT_ONE>;
    using ValueObjVec2 = ValueObject<AZ::Vector2, DISTRIBUTION_COUNT_TWO>;
    using ValueObjVec3 = ValueObject<AZ::Vector3, DISTRIBUTION_COUNT_THREE>;
    using ValueObjVec4 = ValueObject<AZ::Vector4, DISTRIBUTION_COUNT_FOUR>;
    using ValueObjColor = ValueObject<AZ::Color, DISTRIBUTION_COUNT_FOUR>;
    using ValueObjLinear = ValueObject<LinearValue, DISTRIBUTION_COUNT_THREE>;

    enum class ParticleEventType : AZ::u32 {
        SPAWN_LOCATION,
        UPDATE_LOCATION,
        UPDATE_DEATH,
        UPDATE_COLLISION
    };

    struct ParticleEventInfo {
        AZ::Vector3 eventPosition;
        AZ::Color color;
        AZ::Vector3 size;
        AZ::u64 particleId = UINT64_MAX;
        AZ::u32 emitNum = 0u;
        AZ::u32 locationEventIdx = 0u;
        float eventTimeBeforeTick = 0.0f;
        float lifeTime = 0.f;
        float currentLife = 0.f;
        bool useEventInfo = false;
    };

    struct InheritanceInfo {
        AZ::Vector3 position;
        AZ::Vector3 velocity;
        AZ::Vector3 size;
        AZ::Color color;
        float currentLife;
        float lifetime;
    };

    struct InheritanceSpawn {
        AZ::Vector3 position;
        AZ::Vector3 velocity;
        AZ::Vector3 size;
        AZ::Color color;
        AZ::u64 ribbonId = UINT64_MAX;
        float currentLife = 0.0f;
        float lifetime = 0.0f;

        float currentNum = 0.0f;
        AZ::u32 emitNum = 0u;
        float emitTime = 0.0f;
        bool applyPosition = false;
        bool applyVelocity = false;
        bool overwriteVelocity = false;
        bool applySize = false;
        bool applyColorRGB = false;
        bool applyColorAlpha = false;
        bool applyAge = false;
        bool applyLifetime = false;
    };

    struct ParticleEventPool {
        // key = emitterID + eventType, value = vector<info>
        AZStd::unordered_map<AZ::u64, AZStd::vector<ParticleEventInfo>> events;
        // emitterID, value = map<particleID, info>
        AZStd::vector<AZStd::unordered_map<AZ::u64, InheritanceInfo>> inheritances;
    };

    struct CollisionInfo {
        float collisionTimeBeforeTick = 0.0f;
        AZ::Vector3 collisionPosition;
    };

    struct EmitSpawnParam {
        // Spawn or Burst Info
        bool isProcessBurstList = true;
        bool isProcessSpawnRate = true;
        bool isIgnoreSpawnRate = false;
        AZ::u32 burstNum = 0;
        AZ::u32 spawnNum = 0;
        AZ::u32 numOverMoving = 0;
        AZ::u32 newParticleNum = 0;
        float realEmitTime = 0.0f;
        // Event Info
        AZStd::vector<ParticleEventInfo> eventSpawn;
        AZStd::vector<InheritanceSpawn*> inheritanceSpawn;
    };
    
    enum class InfoType : AZ::u32 {
        EMIT = 0,
        SPAWN,
        UPDATE
    };

    struct BaseInfo {
        // Emitter currentTime && duration
        float currentTime = 0.f;
        float duration = 0.f;
        InfoType type;
        float padding = 0.f;
    };

    struct EmitInfo {
        RandomStream* randomStream = nullptr;
        ParticleEventPool* systemEventPool;
        AZStd::unordered_map<AZ::u32, InheritanceSpawn>* emitterInheritances;
        BaseInfo baseInfo;
        float tickTime = 0.f;
        float moveDistance = 0.f;
        bool started = false;
    };

    struct SpawnInfo {
        AZ::Transform emitterTrans;
        AZ::Vector3 front = AZ::Vector3::CreateAxisZ();
        const AZ::Vector3* vertexStream = nullptr;
        const AZ::u32* indiceStream = nullptr;
        const double* areaStream = nullptr;
        const AZ::Vector3* boneStream = nullptr;
        RandomStream* randomStream = nullptr;
        ParticleEventPool* systemEventPool;
        BaseInfo baseInfo;
        AZ::u32 vertexCount = 0;
        AZ::u32 indiceCount = 0;
        AZ::u32 boneCount = 0;
        AZ::u32 currentEmitter = 0;
    };

    struct UpdateInfo {
        AZ::Transform emitterTrans;
        AZ::Vector3 front = AZ::Vector3::CreateAxisZ();
        AZ::Vector3 maxExtend = AZ::Vector3::CreateZero();
        AZ::Vector3 minExtend = AZ::Vector3::CreateZero();
        RandomStream* randomStream = nullptr;
        BaseInfo baseInfo;
        float tickTime = 0.0f;
        bool localSpace = false;
        bool padding0 = false;
        bool padding1 = false;
        bool padding2 = false;
    };

    struct EventInfo {
        AZ::Transform emitterTrans;
        Particle* particleBuffer = nullptr;
        ParticleEventPool* systemEventPool;
        AZ::u32 begin = 0;
        AZ::u32 alive = 0;
        AZ::u32 currentEmitter = 0;
        float tickTime = 0.0f;
        bool localSpace = false;
    };

    struct GpuInstance {
        union {
            void* ptr;
            AZ::u64 idx;
        } data;
    };

    struct SpriteConfig {
        static constexpr RenderType RENDER_TYPE = RenderType::SPRITE;

        AZ::Vector2 subImageSize{ 1.0f, 1.0f };
        AZ::u32 sortId = 0;
        Facing facing = Facing::CAMERA_RECTANGLE;
    };

    struct MeshConfig {
        static constexpr RenderType RENDER_TYPE = RenderType::MESH;

        AZ::u32 sortId = 0;
        Facing facing = Facing::CUSTOM;
    };

    enum class TrailMode : AZ::u8 {
        RIBBON = 0,
        TRAIL
    };

    struct RibbonParam {
        AZ::u32 ribbonCount = 1;
    };

    struct TrailParam {
        float ratio = 1.0;
        float lifetime = 0.f;
        bool inheritLifetime = true;
        bool dieWithParticles = false;
        AZ::u16 padding0 = 0;
    };

    struct RibbonConfig {
        static constexpr RenderType RENDER_TYPE = RenderType::RIBBON;

        ValueObjFloat ribbonWidth {1.f};
        TrailParam trailParam;
        RibbonParam ribbonParam;
        AZ::u32 sortId = 0;
        float minRibbonSegmentLength = AZ::Constants::FloatEpsilon;
        float tesselationFactor = 1.f;
        float curveTension = 0.f;
        float tilingDistance = 0.f;
        RibbonFacing facing = RibbonFacing::SCREEN;
        TrailMode mode = TrailMode::RIBBON;
        bool inheritSize = false;
    };

    struct DrawLinear {
        AZ::u32 instanceCount;
        AZ::u32 instanceOffset;
        AZ::u32 vertexCount;
        AZ::u32 vertexOffset;
    };

    struct DrawIndexed {
        AZ::u32 instanceCount;
        AZ::u32 instanceOffset;
        AZ::u32 vertexOffset;
        AZ::u32 indexCount;
        AZ::u32 indexOffset;
    };

    struct DrawIndirect {
    };

    struct DrawArgsInitializer {
        // max size of DrawLinear, DrawIndexed, DrawIndirect
        AZ::u8 value[sizeof(DrawIndexed)];
    };

    struct DrawArgs {
        DrawArgs() : type(DrawType::LINEAR), initializer{ 0 }
        {
        }

        bool Empty() const
        {
            switch (type) {
                case DrawType::LINEAR:
                    return linear.instanceCount == 0;
                case DrawType::INDEXED:
                    return indexed.instanceCount == 0;
                case DrawType::INDIRECT:
                default:
                    return true;
            }
        }

        DrawType type;
        union {
            DrawLinear linear;
            DrawIndexed indexed;
            DrawIndirect indirect;
            DrawArgsInitializer initializer;
        };
    };

    struct BufferView {
        GpuInstance buffer;
        AZ::u32 offset;
        AZ::u32 size;
        AZ::u32 stride;
    };

    struct VariantKey {
        AZ::u64 value;
    };

    struct LightParticle {
        AZ::Color lightColor{ 1.f, 1.f, 1.f, 1.f };
        float radianScale = 1.f;
    };

    struct DrawItem {
        RenderType type = RenderType::UNDEFINED;
        VariantKey variantKey;
        BufferView vertexBuffer;
        BufferView indexBuffer;
        DrawArgs drawArgs;
        AZStd::vector<AZ::Vector3> positionBuffer;
    };

    struct WorldInfo {
        union ViewKey {
            AZ::u64 v;
            uintptr_t p;
        } viewKey;
        AZ::Vector3 cameraPosition;
        AZ::Vector3 cameraUp;
        AZ::Vector3 cameraRight;
        AZ::Vector3 axisZ;
        AZ::Transform emitterTransform;
    };

    enum class BufferUsage : AZ::u8 {
        VERTEX,
        INDEX
    };

    enum class MemoryType : AZ::u8 {
        DYNAMIC,
        STATIC
    };

    enum class InputRate : AZ::u8 {
        VERTEX,
        INSTANCE
    };

    struct BufferCreate {
        const AZ::u8* data = nullptr;
        AZ::u32 size = 0;
        BufferUsage usage;
        MemoryType memory;
    };

    struct BufferUpdate {
        const AZ::u8* data = nullptr;
        AZ::u32 size = 0;
        AZ::u32 offset = 0;
        BufferUsage usage;
        MemoryType memory;
    };
}
