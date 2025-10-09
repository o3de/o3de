/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ParticleConstant.h"

#include <AzCore/Jobs/Algorithms.h>
#include <AzCore/Jobs/JobCompletion.h>
#include <AtomCore/Instance/InstanceDatabase.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/View.h>
#include <Atom/RHI/Factory.h>
#include <Atom/RHI/RHISystemInterface.h>
#include <Atom/RHI/RHIUtils.h>

#include <OpenParticleSystem/ParticleFeatureProcessor.h>
#include <particle/core/ParticleDriver.h>

namespace OpenParticle
{
    AZ::RHI::BufferBindFlags BindFlags(const SimuCore::ParticleCore::BufferCreate& info)
    {
        if (info.usage == SimuCore::ParticleCore::BufferUsage::VERTEX || info.usage == SimuCore::ParticleCore::BufferUsage::INDEX)
        {
            return info.memory == SimuCore::ParticleCore::MemoryType::STATIC ? AZ::RHI::BufferBindFlags::InputAssembly
                                                          : AZ::RHI::BufferBindFlags::DynamicInputAssembly;
        }
        return AZ::RHI::BufferBindFlags::None;
    }

    void CreateBuffer(DriverWrap* data, const SimuCore::ParticleCore::BufferCreate& info, SimuCore::ParticleCore::GpuInstance& instance)
    {
        AZ_PROFILE_SCOPE(AzCore, "FP CreateBuffer");
        if (!data->m_bufferPool)
        {
            return;
        }
        AZ::RHI::BufferInitRequest request;
        auto buffer = aznew AZ::RHI::Buffer;

        request.m_buffer = buffer;
        request.m_descriptor = AZ::RHI::BufferDescriptor{ BindFlags(info), info.size };
        request.m_initialData = info.data;
        auto result = data->m_bufferPool->InitBuffer(request);

        if (result != AZ::RHI::ResultCode::Success)
        {
            AZ_Error("ParticleSystem", false, "Failed to create buffer: %d", result);
        }
        instance.data.ptr = new PtrHolder<AZ::RHI::Buffer>{ buffer };
    }

    void UpdateBuffer(DriverWrap* data, const SimuCore::ParticleCore::BufferUpdate& info, const SimuCore::ParticleCore::GpuInstance& instance)
    {
        AZ_PROFILE_SCOPE(AzCore, "FP UpdateBuffer");
        if (!data->m_bufferPool || info.data == nullptr || instance.data.ptr == nullptr || !AZ::RHI::GetRHIDevice())
        {
            return;
        }
        auto buffer = reinterpret_cast<PtrHolder<AZ::RHI::Buffer>*>(instance.data.ptr);
        AZ::RHI::BufferMapRequest request = {};
        request.m_buffer = buffer->ptr.get();
        request.m_byteCount = info.size;
        request.m_byteOffset = info.offset;

        AZ::RHI::BufferMapResponse mapResponse;
        auto deviceIndex = AZ::RHI::GetRHIDevice()->GetDeviceIndex();
        auto result = data->m_bufferPool->MapBuffer(request, mapResponse);
        if (result == AZ::RHI::ResultCode::Success)
        {
            memcpy(mapResponse.m_data[deviceIndex], info.data, info.size);
        }
        data->m_bufferPool->UnmapBuffer(*buffer->ptr);
    }

    void DestroyBuffer(DriverWrap*, const SimuCore::ParticleCore::GpuInstance& instance)
    {
        if (instance.data.ptr == nullptr)
        {
            return;
        }
        auto buffer = reinterpret_cast<PtrHolder<AZ::RHI::Buffer>*>(instance.data.ptr);
        delete buffer;
    }

    ParticleDataInstance::~ParticleDataInstance()
    {
        AZ::Data::AssetBus::Handler::BusDisconnect();
    }

    void ParticleDataInstance::Init()
    {
        m_particleInstance = ParticleSystem::Create(m_particleAsset, m_rtConfig);
        m_particleInstance->SetEntityId(m_entityId);
        m_particleInstance->SetObjectId(m_objectId);
        m_particleInstance->SetFeatureProcessor(m_particleFp);
        m_particleInstance->SetScene(m_scene);
        m_particleInstance->SetTransform(m_transform);
        m_particleInstance->PostLoad();
    }

    void ParticleDataInstance::LoadParticle(const AZ::Data::Asset<ParticleAsset>& asset, ParticleFeatureProcessor& fp)
    {
        m_particleFp = &fp;
        m_particleAsset = asset;

        if (!m_particleAsset.GetId().IsValid())
        {
            return;
        }
        // find from global database
        auto particle =
            AZ::Data::InstanceDatabase<ParticleSystem>::Instance().Find(AZ::Data::InstanceId::CreateFromAssetId(m_particleAsset.GetId()));
        if (particle)
        {
            Init();
            return;
        }

        m_particleAsset.QueueLoad();
        AZ::Data::AssetBus::Handler::BusConnect(m_particleAsset.GetId());
    }

    void ParticleDataInstance::OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        m_particleAsset = asset;
        Init();
    }

    void ParticleDataInstance::OnAssetError([[maybe_unused]] AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        AZ_Error("ParticleDataInstance", false, "Failed to load asset %s.", asset.GetHint().c_str());
    }

    void ParticleDataInstance::Simulate(float delta)
    {
        if (m_particleInstance)
        {
            m_particleInstance->Tick(delta);
        }
    }

    void ParticleDataInstance::Render(const AZ::RPI::FeatureProcessor::RenderPacket& packet)
    {
        if (m_particleInstance)
        {
            m_particleInstance->Render(m_driver, packet);
        }
    }

    void ParticleDataInstance::SetMaterialDiffuseMap(AZ::u32 emitterIndex, AZStd::string mapPath)
    {
        if (m_particleInstance)
        {
            m_particleInstance->SetMaterialDiffuseMap(emitterIndex, mapPath);
        }
    }

    void ParticleFeatureProcessor::Init()
    {
        InitFeature();
        InitBufferPool();
        InitDriver();
    }

    void ParticleFeatureProcessor::ShutDown()
    {
        if (m_bufferPool)
        {
            m_bufferPool.reset();
        }
    }

    void ParticleFeatureProcessor::InitDriver()
    {
        SimuCore::ParticleCore::ParticleDriver::BindBufferCreate<CreateBuffer, DriverWrap>();
        SimuCore::ParticleCore::ParticleDriver::BindBufferUpdate<UpdateBuffer, DriverWrap>();
        SimuCore::ParticleCore::ParticleDriver::BindBufferDestroy<DestroyBuffer, DriverWrap>();
    }

    void ParticleFeatureProcessor::InitBufferPool()
    {
        AZ::RHI::BufferPoolDescriptor desc;
        desc.m_heapMemoryLevel = AZ::RHI::HeapMemoryLevel::Device;
        desc.m_bindFlags = AZ::RHI::BufferBindFlags::DynamicInputAssembly;

        m_bufferPool = aznew AZ::RHI::BufferPool;
        m_bufferPool->SetName(AZ::Name("ParticleBufferPool"));
        auto resultCode = m_bufferPool->Init(desc);
        if (resultCode != AZ::RHI::ResultCode::Success)
        {
            AZ_Error("ParticleFeatureProcessor", false, "Failed to initialize buffer pool");
        }
    }

    void ParticleFeatureProcessor::InitFeature()
    {
        m_transformService = GetParentScene()->GetFeatureProcessor<AZ::Render::TransformServiceFeatureProcessorInterface>();
        AZ_Assert(m_transformService != nullptr, "Failed to load transform service");
    }

    void ParticleFeatureProcessor::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<ParticleFeatureProcessor, FeatureProcessor>()->Version(0);
        }
    }

    void ParticleFeatureProcessor::Activate()
    {
        Init();
        AZ::TickBus::Handler::BusConnect();
    }

    void ParticleFeatureProcessor::Deactivate()
    {
        ShutDown();
        AZ::TickBus::Handler::BusDisconnect();
    }

    void ParticleFeatureProcessor::Simulate([[maybe_unused]] const AZ::RPI::FeatureProcessor::SimulatePacket& packet)
    {
        if (m_timeToSim <= 0)
        {
            return;
        }

        AZ_PROFILE_SCOPE(AzRender, "ParticleFeatureProcessor::Simulate");

        auto deltaTime = AZStd::min(m_timeToSim, MAX_UPDATE_INTERVAL);
        m_timeToSim = 0;

        AZ::Job* parentJob = packet.m_parentJob;
        const auto iteratorRanges = m_particleInstances.GetParallelRanges();
        AZ::JobCompletion jobCompletion;
        for (const auto& iteratorRange : iteratorRanges)
        {
            const auto jobLambda = [&]() -> void
            {
                AZ_PROFILE_SCOPE(AzRender, "Particle::Simulate() Lambda");
                for (auto iter = iteratorRange.m_begin; iter != iteratorRange.m_end; ++iter)
                {
                    if (!iter->m_visible ||
                        !iter->m_enable ||
                        iter->m_status != ParticleStatus::PLAYING)
                    {
                        continue;
                    }
                    iter->Simulate(deltaTime);
                }
            };
            AZ::Job* executeGroupJob = aznew AZ::JobFunction<decltype(jobLambda)>(jobLambda, true, nullptr); // Auto-deletes
            if (parentJob)
            {
                parentJob->StartAsChild(executeGroupJob);
            }
            else
            {
                executeGroupJob->SetDependent(&jobCompletion);
                executeGroupJob->Start();
            }
        }
        {
            AZ_PROFILE_SCOPE(AzRender, "ParticleFeatureProcessor Simulate: WaitForChildren");
            if (parentJob)
            {
                parentJob->WaitForChildren();
            }
            else
            {
                jobCompletion.StartAndWaitForCompletion();
            }
        }
    }

    void ParticleFeatureProcessor::Render(const AZ::RPI::FeatureProcessor::RenderPacket& packet)
    {
        const auto iteratorRanges = m_particleInstances.GetParallelRanges();
        for (const auto& iteratorRange : iteratorRanges)
        {
            for (auto iter = iteratorRange.m_begin; iter != iteratorRange.m_end; ++iter)
            {
                if (!iter->m_visible ||
                    !iter->m_enable ||
                    (iter->m_status != ParticleStatus::PLAYING && iter->m_status != ParticleStatus::PAUSED))
                {
                    if (iter->m_particleInstance)
                    {
                        iter->m_particleInstance.get()->ClearAllLightEffects();
                    }

                    continue;
                }
                iter->Render(packet);
            }
        }
    }

    void ParticleFeatureProcessor::SetMaterialDiffuseMap(const ParticleHandle& handle, AZ::u32 emitterIndex, AZStd::string mapPath)
    {
        if (handle.IsValid())
        {
            handle->SetMaterialDiffuseMap(emitterIndex, mapPath);
        }
    }

    void ParticleFeatureProcessor::OnRenderPipelineAdded([[maybe_unused]] AZ::RPI::RenderPipelinePtr pipeline)
    {
    }

    void ParticleFeatureProcessor::OnRenderPipelineRemoved([[maybe_unused]] AZ::RPI::RenderPipeline* pipeline)
    {
    }

    // Called by component controller when editor or running is loading
    ParticleFeatureProcessor::ParticleHandle ParticleFeatureProcessor::AcquireParticle(const AZ::EntityId& id,
            const ParticleComponentConfig& rtConfig, const AZ::Transform& transform)
    {
        auto handle = m_particleInstances.emplace();
        handle->m_scene = GetParentScene();
        handle->m_particleAsset = rtConfig.m_particleAsset;
        handle->m_entityId = id;
        handle->m_objectId = m_transformService->ReserveObjectId();
        handle->m_driver.m_bufferPool = m_bufferPool;
        handle->m_transform = transform;
        handle->SetRuntimeConfig(rtConfig);
        handle->LoadParticle(rtConfig.m_particleAsset, *this);
        return handle;
    }

    void ParticleFeatureProcessor::ReleaseParticle(ParticleHandle& handle)
    {
        if (handle.IsValid())
        {
            handle->m_enable = false;
            m_transformService->ReleaseObjectId(handle->m_objectId);
            m_particleInstances.erase(handle);
        }
    }

    void ParticleFeatureProcessor::SetTransform(
        const ParticleHandle& handle, const AZ::Transform& transform, const AZ::Vector3& nonUniformScale)
    {
        if (handle.IsValid())
        {
            m_transformService->SetTransformForId(handle->m_objectId, transform, nonUniformScale);
            if (handle->m_particleInstance != nullptr)
            {
                handle->m_particleInstance->SetTransform(transform);
            }
        }
    }

    ParticlePipelineState* ParticleFeatureProcessor::FetchOrCreate(AZ::u32 key)
    {
        auto iter = m_pipelineStates.find(key);
        if (iter == m_pipelineStates.end())
        {
            auto& state = m_pipelineStates[key];
            if (!state.Setup(key))
            {
                return nullptr;
            }
        }
        return &m_pipelineStates[key];
    }

    ParticlePipelineState* ParticleFeatureProcessor::Fetch(AZ::u32 key)
    {
        auto iter = m_pipelineStates.find(key);
        return iter == m_pipelineStates.end() ? nullptr : &iter->second;
    }

    void ParticleFeatureProcessor::OnTick(float deltaTime, AZ::ScriptTimePoint scriptTime)
    {
        m_timeToSim = deltaTime;
        AZ_UNUSED(scriptTime);
    }

    int ParticleFeatureProcessor::GetTickOrder()
    {
        return AZ::TICK_PHYSICS;
    }

} // namespace OpenParticle
