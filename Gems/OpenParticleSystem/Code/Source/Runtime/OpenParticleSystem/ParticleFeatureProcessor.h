/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
 
#pragma once

#include <Atom/RHI/PipelineState.h>

#include <Atom/RPI.Public/Buffer/Buffer.h>
#include <Atom/RPI.Public/PipelineState.h>
#include <Atom/RPI.Public/Shader/Shader.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>

#include <Atom/Feature/TransformService/TransformServiceFeatureProcessorInterface.h>
#include <Atom/Feature/Utils/GpuBufferHandler.h>

#include <AzCore/Math/Transform.h>
#include <AzCore/std/containers/array.h>
#include <AzCore/std/chrono/chrono.h>
#include <AzCore/Component/TickBus.h>

#include <OpenParticleSystem/Asset/ParticleAsset.h>
#include <OpenParticleSystem/ParticleFeatureProcessorInterface.h>
#include <OpenParticleSystem/ParticleSystem.h>

namespace OpenParticle
{
    class ParticleFeatureProcessor;

    enum class ParticleStatus {
        EMPTY,
        LOADED,
        PLAYING,
        PAUSED
    };

    class ParticleDataInstance
        : private AZ::Data::AssetBus::Handler
    {
    public:
        ParticleDataInstance() = default;
        ~ParticleDataInstance();

        void OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
        void OnAssetError(AZ::Data::Asset<AZ::Data::AssetData> asset) override;

        void Simulate(float delta);
        void Render(const AZ::RPI::FeatureProcessor::RenderPacket& packet);

        void Init();

        void LoadParticle(const AZ::Data::Asset<ParticleAsset>& asset, ParticleFeatureProcessor& fp);

        void SetMaterialDiffuseMap(AZ::u32 emitterIndex, AZStd::string mapPath);
        void SetRuntimeConfig(ParticleComponentConfig config) { m_rtConfig = config; };

        ParticleFeatureProcessor* m_particleFp = nullptr;
        AZ::RPI::Scene* m_scene = nullptr;
        AZ::Data::Asset<ParticleAsset> m_particleAsset;
        AZ::Data::Instance<ParticleSystem> m_particleInstance;
        AZ::EntityId m_entityId;
        AZ::Render::TransformServiceFeatureProcessorInterface::ObjectId m_objectId;
        AZ::Transform m_transform;
        DriverWrap m_driver;
        bool m_visible = true;
        bool m_enable = true;
        ParticleStatus m_status{ParticleStatus::EMPTY};

    private:
        ParticleComponentConfig m_rtConfig;
    };

    class ParticleFeatureProcessor final
        : public ParticleFeatureProcessorInterface
        , AZ::TickBus::Handler
    {
    public:
        AZ_RTTI(OpenParticle::ParticleFeatureProcessor, "{3757fbb5-7697-4c11-a3a0-203bbad452db}",
            OpenParticle::ParticleFeatureProcessorInterface);
        AZ_CLASS_ALLOCATOR(OpenParticle::ParticleFeatureProcessor, AZ::SystemAllocator, 0);

        static void Reflect(AZ::ReflectContext* context);

        void Activate() override;
        void Deactivate() override;
        void Simulate(const AZ::RPI::FeatureProcessor::SimulatePacket& packet) override;
        void Render(const AZ::RPI::FeatureProcessor::RenderPacket& packet) override;

        void OnRenderPipelineAdded(AZ::RPI::RenderPipelinePtr pipeline) override;

        void OnRenderPipelineRemoved(AZ::RPI::RenderPipeline* pipeline) override;

        ParticleHandle AcquireParticle(const AZ::EntityId& id, const ParticleComponentConfig& rtConfig, const AZ::Transform& transform) override;

        void ReleaseParticle(ParticleHandle& handle) override;

        void SetTransform(const ParticleHandle& handle, const AZ::Transform& transform, const AZ::Vector3& nonUniformScale) override;

        void SetMaterialDiffuseMap(const ParticleHandle& handle, AZ::u32 emitterIndex, AZStd::string mapPath) override;

        ParticlePipelineState* FetchOrCreate(AZ::u32 key);

        ParticlePipelineState* Fetch(AZ::u32 key);

        // AZ::TickBus::Handler overrides
        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;
        int GetTickOrder() override;

    private:
        void Init() override;

        void ShutDown() override;

        void InitFeature();

        void InitDriver();

        void InitBufferPool();

        AZ::RHI::Ptr<AZ::RHI::BufferPool> m_bufferPool;
        AZ::Render::TransformServiceFeatureProcessorInterface* m_transformService = nullptr;
        AZ::StableDynamicArray<ParticleDataInstance> m_particleInstances;
        AZStd::unordered_map<AZ::u32, ParticlePipelineState> m_pipelineStates;
        AZStd::optional<AZStd::chrono::time_point<AZStd::chrono::high_resolution_clock>> time;
        float m_timeToSim{0};
    };
} // namespace OpenParticle
