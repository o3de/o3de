/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <OpenParticleSystem/ParticleComponentConfig.h>

#include <Atom/Feature/CoreLights/SimplePointLightFeatureProcessorInterface.h>
#include <Atom/Feature/TransformService/TransformServiceFeatureProcessorInterface.h>
#include <Atom/RHI/BufferPool.h>
#include <Atom/RPI.Public/FeatureProcessor.h>

#include <AtomCore/Instance/InstanceData.h>

#include <AzCore/Asset/AssetCommon.h>

#include <OpenParticleSystem/Asset/ParticleAsset.h>
#include <EMotionFX/Source/ActorInstance.h>
#include <OpenParticleSystem/ParticlePipelineState.h>
#include <particle/core/ParticleSystem.h>

namespace OpenParticle
{
    class ParticleFeatureProcessor;

    struct DriverWrap
    {
        AZ::RHI::Ptr<AZ::RHI::BufferPool> m_bufferPool;
    };

    class ParticleSystem
        : public AZ::Data::InstanceData
    {
    public:
        AZ_INSTANCE_DATA(ParticleSystem, "{65680095-7b56-4d5f-91b5-d1beb26c84f7}");
        AZ_CLASS_ALLOCATOR(ParticleSystem, AZ::SystemAllocator, 0);

        explicit ParticleSystem(const ParticleComponentConfig& config) : m_rtConfig(config) {};
        ~ParticleSystem();
        static AZ::Data::Instance<ParticleSystem> Create(const AZ::Data::Asset<ParticleAsset>& asset, const ParticleComponentConfig& config);

        static AZ::Data::Instance<ParticleSystem> CreateInternal(ParticleAsset& asset, const ParticleComponentConfig& config);

        void SetFeatureProcessor(ParticleFeatureProcessor* fp);

        void SetScene(AZ::RPI::Scene* scene);

        void SetEntityId(const AZ::EntityId& id);

        void SetObjectId(const AZ::Render::TransformServiceFeatureProcessorInterface::ObjectId& id);

        void SetTransform(AZ::Transform trans);

        void PostLoad();

        bool Init(ParticleAsset& asset);

        void Tick(float delta);

        void Render(DriverWrap& driverWrap, const AZ::RPI::FeatureProcessor::RenderPacket& packet);

        void ClearAllLightEffects();

        void SetMaterialDiffuseMap(AZ::u32 emitterIndex, AZStd::string mapPath);

    private:
        struct DrawParam {
            AZ::Data::Instance<AZ::RPI::Shader> shader;
            AZ::RPI::ShaderVariant variant;
            SimuCore::ParticleCore::DrawItem item;
        };

        AZ::u32 GetPipelineKey(SimuCore::ParticleCore::RenderType) const;

        void SetBoneAndVertexBuffer(SimuCore::ParticleCore::ParticleEmitter& emitter);
        void GetSkeletonBoneBuffer(const AZ::u32 emitterId, const EMotionFX::Actor& actor, const EMotionFX::TransformData& transData);
        void GetMeshVertexBuffer(const AZ::u32 emitterId, const EMotionFX::Actor& actor, const EMotionFX::TransformData& transData);
        void GetMeshAreaBuffer(AZ::u32 emitterId, const EMotionFX::Actor& actor, const EMotionFX::TransformData& transData);
        void UpdateArea(AZ::u32 emitterId);
        void HandleSkeletonModel(SimuCore::ParticleCore::ParticleEmitter& emitter);
        void SetParticleLight(SimuCore::ParticleCore::ParticleEmitter& emitter, const SimuCore::ParticleCore::DrawItem& item);

        void RenderParticle(const AZ::RHI::DrawListTag drawListTag, const DrawParam& drawParam,
            EmitterInstance& instance, AZ::RPI::View& view, int meshIndex = -1);

        void ClearLightEffects(AZ::u32 emitterId);

        void Rebuild();

        ParticleFeatureProcessor* m_particleFp = nullptr;
        AZ::RPI::Scene* m_scene = nullptr;
        AZ::Data::Asset<ParticleAsset> m_asset;
        AZ::EntityId m_entityId;
        EMotionFX::ActorInstance* m_actorInstance = nullptr;
        size_t m_lastLodLevel = 0;
        AZ::Render::TransformServiceFeatureProcessorInterface::ObjectId m_objectId;
        AZStd::unique_ptr<SimuCore::ParticleCore::ParticleSystem> m_particleSystem;
        AZStd::unordered_map<const SimuCore::ParticleCore::ParticleEmitter*, EmitterInstance> m_emitterInstances;
        AZ::Transform m_transform;
        AZStd::unordered_map <AZ::u32,
            AZStd::vector<AZ::Render::SimplePointLightFeatureProcessorInterface::LightHandle>> lightHandles;
        AZStd::vector<AZ::RHI::ConstPtr<AZ::RHI::DrawPacket>> m_drawPacket;

        bool m_changedDiffuseMap;
        AZStd::unordered_map<AZ::u32, AZ::RPI::MaterialPropertyValue> m_diffuseMapInstances;
        AZStd::unordered_map<AZ::u32, AZStd::vector<AZ::Vector3>> m_boneStreams;
        AZStd::unordered_map<AZ::u32, AZStd::vector<AZ::Vector3>> m_vertexStreams;
        AZStd::unordered_map<AZ::u32, AZStd::vector<AZ::Vector3>> m_vertexCoordSteams;
        AZStd::unordered_map<AZ::u32, AZStd::vector<AZ::u32>> m_indiceStreams;
        AZStd::unordered_map<AZ::u32, AZStd::vector<double>> m_areaStreams;

        // realtime position of particle system, used when particle follow active camera enabled.
        AZ::Transform m_rtSysTransform;
        // runtime config of particle system which can be changed via particle component panel in edit mode.
        const ParticleComponentConfig& m_rtConfig;
    };

} // namespace OpenParticle
