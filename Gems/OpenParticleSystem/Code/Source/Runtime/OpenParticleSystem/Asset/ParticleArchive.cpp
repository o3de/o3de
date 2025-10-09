/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ParticleArchive.h"

#include <OpenParticleSystem/ParticleConfig.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Asset/AssetSerializer.h>

namespace OpenParticle
{
    template<typename T>
    inline void ConstructEmit(SimuCore::ParticleCore::ParticleEmitter* emitter, AZ::u32 data)
    {
        emitter->AddEmitterEffectorData<T>(data);
    }

    template<typename T>
    inline void ConstructSpawn(SimuCore::ParticleCore::ParticleEmitter* emitter, AZ::u32 data)
    {
        emitter->AddSpawnEffectorData<T>(data);
    }

    template<typename T>
    inline void ConstructUpdate(SimuCore::ParticleCore::ParticleEmitter* emitter, AZ::u32 data)
    {
        emitter->AddUpdateEffectorData<T>(data);
    }

    template<typename T>
    inline void ConstructEvent(SimuCore::ParticleCore::ParticleEmitter* emitter, AZ::u32 data)
    {
        emitter->AddEventEffectorData<T>(data);
    }

    using AddEffectorFunc = void (*)(SimuCore::ParticleCore::ParticleEmitter*, AZ::u32);

    void ParticleArchive::EmitterInfo::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<ParticleArchive::EmitterInfo>()
                ->Version(0)
                ->Field("config", &ParticleArchive::EmitterInfo::m_config)
                ->Field("render", &ParticleArchive::EmitterInfo::m_render)
                ->Field("material", &ParticleArchive::EmitterInfo::m_material)
                ->Field("model", &ParticleArchive::EmitterInfo::m_model)
                ->Field("skeleton model", &ParticleArchive::EmitterInfo::m_skeletonModel)
                ->Field("effectors", &ParticleArchive::EmitterInfo::m_effectors)
                ->Field("meshSampleType", &ParticleArchive::EmitterInfo::m_meshSampleType);
        }
    }

    void ParticleArchive::Reflect(AZ::ReflectContext* context)
    {
        EmitterInfo::Reflect(context);

        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<ParticleArchive>()
                ->Version(0)
                ->Field("systemConfig", &ParticleArchive::m_systemConfig)
                ->Field("preWarm", &ParticleArchive::m_preWarm)
                ->Field("emitters", &ParticleArchive::m_emitterInfos)
                ->Field("buffers", &ParticleArchive::m_buffers);
        }
    }

    AZ::u32 BufferEmplace(
        AZStd::unordered_map<AZ::u32, AZStd::vector<AZ::u8>>& buffers,
        const SimuCore::ParticleCore::ParticleSystem& system, const AZ::u32& index, const AZ::u32 stride)
    {
        auto alignSize = SimuCore::ParticleCore::ParticleDataPool::AlignSize(stride);
        auto& block = buffers[alignSize];
        AZ::u32 offset = static_cast<AZ::u32>(block.size());
        block.resize(block.size() + alignSize);
        AZ::u8* dst = &block[offset];
        const AZ::u8* src = system.Data(stride, index);
        memcpy(dst, src, alignSize);
        
        return offset;
    }

    AZ::u32 BufferEmplace(AZStd::unordered_map<AZ::u32, AZStd::vector<AZ::u8>>& buffers, const void* src, AZ::u32 stride)
    {
        auto alignSize = SimuCore::ParticleCore::ParticleDataPool::AlignSize(stride);
        auto& block = buffers[alignSize];
        AZ::u32 offset = static_cast<AZ::u32>(block.size());
        block.resize(block.size() + alignSize);
        AZ::u8* dst = &block[offset];
        memcpy(dst, src, alignSize);
        
        return offset;
    }

    template<typename T>
    void Convert(
        AZ::SerializeContext* context,
        AZStd::unordered_map<AZ::u32, AZStd::vector<AZ::u8>>& buffers,
        const SimuCore::ParticleCore::ParticleSystem& system,
        const AZStd::vector<SimuCore::ParticleCore::ParticleEffectorInfo<T>>& lists,
        AZStd::vector<AZStd::tuple<AZ::Uuid, AZ::u32>>& out)
    {
        for (auto& t : lists)
        {
            AZ::u32 offset = BufferEmplace(buffers, system, t.offset, t.effector->DataSize());
            AZStd::vector<AZ::Uuid> ids = context->FindClassId(AZ::Crc32(t.effector->Name().c_str()));
            if (ids.empty())
            {
                continue;
            }
            out.emplace_back(ids[0], offset);
        }
    }

    void ParticleArchive::Reset()
    {
        m_systemConfig = 0;
        m_emitterInfos.clear();
        m_buffers.clear();
    }

    void ParticleArchive::operator>>(const SimuCore::ParticleCore::ParticleSystem& system)
    {
        Reset();

        m_systemConfig = BufferEmplace(m_buffers, system, system.GetConfig(), sizeof(SimuCore::ParticleCore::ParticleSystem::Config));
        m_preWarm = BufferEmplace(m_buffers, system, system.GetPreWarm(), sizeof(SimuCore::ParticleCore::ParticleSystem::PreWarm));

        auto emitters = system.GetAllEmitters();
        m_emitterInfos.resize(emitters.size());
        for (auto& emitter : emitters)
        {
            auto& info = m_emitterInfos[emitter.first];
            info.m_config = BufferEmplace(m_buffers, system, emitter.second->GetConfig(), sizeof(SimuCore::ParticleCore::ParticleEmitter::Config));
            auto render = emitter.second->GetRender();
            auto renderType = render.first->GetType();
            if (renderType == SimuCore::ParticleCore::RenderType::SPRITE)
            {
                info.m_render.first = azrtti_typeid<SimuCore::ParticleCore::SpriteConfig>();
            }
            info.m_render.second = BufferEmplace(m_buffers, system, render.second, render.first->DataSize());
            Convert(m_serializeContext, m_buffers, system, emitter.second->GetEmitEffectors(), info.m_effectors);
            Convert(m_serializeContext, m_buffers, system, emitter.second->GetSpawnEffectors(), info.m_effectors);
            Convert(m_serializeContext, m_buffers, system, emitter.second->GetUpdateEffectors(), info.m_effectors);
            Convert(m_serializeContext, m_buffers, system, emitter.second->GetEventEffectors(), info.m_effectors);
        }
    }

    AZStd::unordered_map<AZ::Uuid, AddEffectorFunc> GetConstructMap()
    {
        AZStd::unordered_map<AZ::Uuid, AddEffectorFunc> constructMap = {
            { azrtti_typeid<SimuCore::ParticleCore::EmitBurstList>(), { &ConstructEmit<SimuCore::ParticleCore::EmitBurstList> } },
            { azrtti_typeid<SimuCore::ParticleCore::EmitSpawn>(), { &ConstructEmit<SimuCore::ParticleCore::EmitSpawn> } },
            { azrtti_typeid<SimuCore::ParticleCore::EmitSpawnOverMoving>(), { &ConstructEmit<SimuCore::ParticleCore::EmitSpawnOverMoving> } },
            { azrtti_typeid<SimuCore::ParticleCore::ParticleEventHandler>(), { &ConstructEmit<SimuCore::ParticleCore::ParticleEventHandler> } },
            { azrtti_typeid<SimuCore::ParticleCore::InheritanceHandler>(), { &ConstructEmit<SimuCore::ParticleCore::InheritanceHandler> } },
            { azrtti_typeid<SimuCore::ParticleCore::SpawnColor>(), { &ConstructSpawn<SimuCore::ParticleCore::SpawnColor> } },
            { azrtti_typeid<SimuCore::ParticleCore::SpawnLifetime>(), { &ConstructSpawn<SimuCore::ParticleCore::SpawnLifetime> } },
            { azrtti_typeid<SimuCore::ParticleCore::SpawnLocBox>(), { &ConstructSpawn<SimuCore::ParticleCore::SpawnLocBox> } },
            { azrtti_typeid<SimuCore::ParticleCore::SpawnLocPoint>(), { &ConstructSpawn<SimuCore::ParticleCore::SpawnLocPoint> } },
            { azrtti_typeid<SimuCore::ParticleCore::SpawnLocSphere>(), { &ConstructSpawn<SimuCore::ParticleCore::SpawnLocSphere> } },
            { azrtti_typeid<SimuCore::ParticleCore::SpawnLocCylinder>(), { &ConstructSpawn<SimuCore::ParticleCore::SpawnLocCylinder> } },
            { azrtti_typeid<SimuCore::ParticleCore::SpawnLocSkeleton>(), { &ConstructSpawn<SimuCore::ParticleCore::SpawnLocSkeleton> } },
            { azrtti_typeid<SimuCore::ParticleCore::SpawnLocTorus>(), { &ConstructSpawn<SimuCore::ParticleCore::SpawnLocTorus> } },
            { azrtti_typeid<SimuCore::ParticleCore::SpawnSize>(), { &ConstructSpawn<SimuCore::ParticleCore::SpawnSize> } },
            { azrtti_typeid<SimuCore::ParticleCore::SpawnVelDirection>(), { &ConstructSpawn<SimuCore::ParticleCore::SpawnVelDirection> } },
            { azrtti_typeid<SimuCore::ParticleCore::SpawnVelSector>(), {&ConstructSpawn<SimuCore::ParticleCore::SpawnVelSector> } },
            { azrtti_typeid<SimuCore::ParticleCore::SpawnVelCone>(), { &ConstructSpawn<SimuCore::ParticleCore::SpawnVelCone> } },
            { azrtti_typeid<SimuCore::ParticleCore::SpawnVelSphere>(), { &ConstructSpawn<SimuCore::ParticleCore::SpawnVelSphere> } },
            { azrtti_typeid<SimuCore::ParticleCore::SpawnVelConcentrate>(), { &ConstructSpawn<SimuCore::ParticleCore::SpawnVelConcentrate> } },
            { azrtti_typeid<SimuCore::ParticleCore::SpawnRotation>(), { &ConstructSpawn<SimuCore::ParticleCore::SpawnRotation> } },
            { azrtti_typeid<SimuCore::ParticleCore::SpawnLightEffect>(), { &ConstructSpawn<SimuCore::ParticleCore::SpawnLightEffect> } },
            { azrtti_typeid<SimuCore::ParticleCore::SpawnLocationEvent>(), { &ConstructSpawn<SimuCore::ParticleCore::SpawnLocationEvent> } },
            { azrtti_typeid<SimuCore::ParticleCore::UpdateConstForce>(), { &ConstructUpdate<SimuCore::ParticleCore::UpdateConstForce> } },
            { azrtti_typeid<SimuCore::ParticleCore::UpdateDragForce>(), { &ConstructUpdate<SimuCore::ParticleCore::UpdateDragForce> } },
            { azrtti_typeid<SimuCore::ParticleCore::UpdateVortexForce>(), { &ConstructUpdate<SimuCore::ParticleCore::UpdateVortexForce> } },
            { azrtti_typeid<SimuCore::ParticleCore::UpdateCurlNoiseForce>(), { &ConstructUpdate<SimuCore::ParticleCore::UpdateCurlNoiseForce> } },
            { azrtti_typeid<SimuCore::ParticleCore::UpdateColor>(), { &ConstructUpdate<SimuCore::ParticleCore::UpdateColor> } },
            { azrtti_typeid<SimuCore::ParticleCore::UpdateSizeLinear>(), { &ConstructUpdate<SimuCore::ParticleCore::UpdateSizeLinear> } },
            { azrtti_typeid<SimuCore::ParticleCore::UpdateSizeByVelocity>(), { &ConstructUpdate<SimuCore::ParticleCore::UpdateSizeByVelocity> } },
            { azrtti_typeid<SimuCore::ParticleCore::SizeScale>(), { &ConstructUpdate<SimuCore::ParticleCore::SizeScale> } },
            { azrtti_typeid<SimuCore::ParticleCore::UpdateSubUv>(), { &ConstructUpdate<SimuCore::ParticleCore::UpdateSubUv> } },
            { azrtti_typeid<SimuCore::ParticleCore::UpdateRotateAroundPoint>(), { &ConstructUpdate<SimuCore::ParticleCore::UpdateRotateAroundPoint> } },
            { azrtti_typeid<SimuCore::ParticleCore::UpdateVelocity>(), { &ConstructUpdate<SimuCore::ParticleCore::UpdateVelocity> } },
            { azrtti_typeid<SimuCore::ParticleCore::ParticleCollision>(), { &ConstructUpdate<SimuCore::ParticleCore::ParticleCollision> } },
            { azrtti_typeid<SimuCore::ParticleCore::UpdateLocationEvent>(), { &ConstructEvent<SimuCore::ParticleCore::UpdateLocationEvent> } },
            { azrtti_typeid<SimuCore::ParticleCore::UpdateDeathEvent>(), { &ConstructEvent<SimuCore::ParticleCore::UpdateDeathEvent> } },
            { azrtti_typeid<SimuCore::ParticleCore::UpdateCollisionEvent>(), { &ConstructEvent<SimuCore::ParticleCore::UpdateCollisionEvent> } },
            { azrtti_typeid<SimuCore::ParticleCore::UpdateInheritanceEvent>(), { &ConstructEvent<SimuCore::ParticleCore::UpdateInheritanceEvent> } },
        };
        return constructMap;
    }

    void ParticleArchive::operator<<(SimuCore::ParticleCore::ParticleSystem& system)
    {
        system.Reset();
        for (auto& buffer : m_buffers)
        {
            auto& rawData = buffer.second;
            system.EmplaceData(buffer.first, rawData.data(), static_cast<AZ::u32>(rawData.size()));
        }

        system.SetConfig(m_systemConfig);
        system.SetPreWarm(m_preWarm);

        AZStd::unordered_map<AZ::Uuid, AddEffectorFunc> constructMap = GetConstructMap();

        for (auto& emitterInfo : m_emitterInfos)
        {
            auto emitter = system.AddEmitter(emitterInfo.m_config);

            auto renderType = emitterInfo.m_render.first;
            if (renderType == azrtti_typeid<SimuCore::ParticleCore::SpriteConfig>())
            {
                emitter->SetParticleRender(emitterInfo.m_render.second, SimuCore::ParticleCore::RenderType::SPRITE);
            }
            else if (renderType == azrtti_typeid<SimuCore::ParticleCore::MeshConfig>())
            {
                emitter->SetParticleRender(emitterInfo.m_render.second, SimuCore::ParticleCore::RenderType::MESH);
            }
            else if (renderType == azrtti_typeid<SimuCore::ParticleCore::RibbonConfig>())
            {
                emitter->SetParticleRender(emitterInfo.m_render.second, SimuCore::ParticleCore::RenderType::RIBBON);
            }

            for (auto& effector : emitterInfo.m_effectors)
            {
                auto iter = constructMap.find(AZStd::get<0>(effector));
                if (iter == constructMap.end())
                {
                    continue;
                }

                iter->second(emitter, AZStd::get<1>(effector));
            }
        }
    }

    ParticleArchive& ParticleArchive::Begin(AZ::SerializeContext* context)
    {
        m_serializeContext = context;
        Reset();
        return *this;
    }

    static inline AZStd::pair<AZ::u32, bool> AnySize(AZ::SerializeContext* context, const AZStd::any& val)
    {
        AZ_Assert(context != nullptr, "invalid serialize context");
        auto classData = context->FindClassData(val.type());
        if (classData == nullptr)
        {
            return { 0, false };
        }

        return { (AZ::u32)classData->m_azRtti->GetTypeSize(), true };
    }

    ParticleArchive& ParticleArchive::SystemConfig(const AZStd::any& val)
    {
        auto pair = AnySize(m_serializeContext, val);
        if (!pair.second)
        {
            return *this;
        }
        m_systemConfig = BufferEmplace(m_buffers, AZStd::any_cast<void>(&val), pair.first);
        return *this;
    }

    ParticleArchive& ParticleArchive::PreWarm(const AZStd::any& val)
    {
        auto pair = AnySize(m_serializeContext, val);
        if (!pair.second)
        {
            return *this;
        }
        m_preWarm = BufferEmplace(m_buffers, AZStd::any_cast<void>(&val), pair.first);
        return *this;
    }

    ParticleArchive& ParticleArchive::EmitterBegin(const AZStd::any& val)
    {
        auto pair = AnySize(m_serializeContext, val);
        if (!pair.second)
        {
            return *this;
        }

        m_emitterInfos.emplace_back();
        auto& emitter = m_emitterInfos.back();
        emitter.m_config = BufferEmplace(m_buffers, AZStd::any_cast<void>(&val), pair.first);
        return *this;
    }

    ParticleArchive& ParticleArchive::RenderConfig(const AZStd::any& val)
    {
        auto pair = AnySize(m_serializeContext, val);
        if (!pair.second)
        {
            return *this;
        }

        auto& emitter = m_emitterInfos.back();
        emitter.m_render.first = val.type();
        emitter.m_render.second = BufferEmplace(m_buffers, AZStd::any_cast<void>(&val), pair.first);
        return *this;
    }

    ParticleArchive& ParticleArchive::Material(const AZ::Data::Asset<AZ::RPI::MaterialAsset>& asset)
    {
        auto& emitter = m_emitterInfos.back();
        emitter.m_material = asset;
        return *this;
    }

    ParticleArchive& ParticleArchive::Model(const AZ::Data::Asset<AZ::RPI::ModelAsset>& asset)
    {
        auto& emitter = m_emitterInfos.back();
        emitter.m_model = asset;
        return *this;
    }

    ParticleArchive& ParticleArchive::SkeletonModel(const AZ::Data::Asset<AZ::RPI::ModelAsset>& asset)
    {
        auto& emitter = m_emitterInfos.back();
        emitter.m_skeletonModel = asset;
        return *this;
    }

    ParticleArchive& ParticleArchive::AddEffector(const AZStd::any& val)
    {
        auto pair = AnySize(m_serializeContext, val);
        if (!pair.second)
        {
            return *this;
        }

        auto& emitter = m_emitterInfos.back();
        AZ::u32 buffIndex = BufferEmplace(m_buffers, AZStd::any_cast<void>(&val), pair.first);
        emitter.m_effectors.emplace_back(val.type(), buffIndex);
        return *this;
    }

    ParticleArchive& ParticleArchive::EmitterEnd()
    {
        return *this;
    }

    ParticleArchive& ParticleArchive::MeshType(SimuCore::ParticleCore::MeshSampleType& type)
    {
        auto& emitter = m_emitterInfos.back();
        emitter.m_meshSampleType = type;
        return *this;
    }
} // namespace OpenParticle
