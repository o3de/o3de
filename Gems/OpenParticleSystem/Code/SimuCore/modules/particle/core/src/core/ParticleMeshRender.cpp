/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ParticleMeshRender.h"
#include <algorithm>
#include "particle/core/ParticlePool.h"
#include "particle/core/ParticleDriver.h"
#include "particle/core/ParticleHelper.h"

namespace SimuCore::ParticleCore {
    ParticleMeshRender::~ParticleMeshRender()
    {
        for (auto& view : instanceBufferViews) {
            ParticleDriver::bufferDestroyFn(m_driver, view.second.buffer);
        }
        m_driver = nullptr;
    }

    AZ::u32 ParticleMeshRender::DataSize() const
    {
        return sizeof(MeshConfig);
    }

    static void UpdateParticle(const ParticlePool& pool, const WorldInfo& world,
            AZStd::vector<ParticleMeshInstanceData>& instanceData, AZStd::vector<AZ::Vector3>& positionBuffer)
    {
        const Particle* particle = pool.ParticleData().data();
        ParticleMeshInstanceData* instanceInfo = instanceData.data();
        for (AZ::u32 i = 0; i < pool.Alive(); ++i) {
            const Particle& curr = particle[i];
            if (i < positionBuffer.size() && curr.hasLightEffect) {
                positionBuffer[i] = curr.globalPosition;
            }
            const AZ::Vector3 offset = curr.globalPosition - world.emitterTransform.GetTranslation();
            AZ::Vector3 initAxis(curr.rotation.GetX(), curr.rotation.GetY(), curr.rotation.GetZ());
            const AZ::Vector4 initRotation = initAxis.IsClose(AZ::Vector3::CreateZero()) ? AZ::Vector4(initAxis, 0.f) : curr.rotation;
            AZ::Vector3 rotateAxis(curr.rotationVector.GetX(), curr.rotationVector.GetY(), curr.rotationVector.GetZ());
            const AZ::Quaternion rotationVector = rotateAxis.IsClose(AZ::Vector3::CreateZero())
                ? AZ::Quaternion(rotateAxis, 0.f)
                : AZ::Quaternion(rotateAxis, AZ::DegToRad(curr.rotationVector.GetW()));

            instanceInfo[i].offset = AZ::Vector4(offset, 0.0f);
            instanceInfo[i].color = curr.color;
            instanceInfo[i].scale = AZ::Vector4(curr.scale, 0.0f);
            instanceInfo[i].initRotation = initRotation;
            instanceInfo[i].rotationVector = rotationVector;
            instanceInfo[i].hasInstanceData = 1;
        }
    }

    void ParticleMeshRender::UpdateBuffer(const ParticlePool& pool, const WorldInfo& world, AZStd::vector<AZ::Vector3>& positionBuffer)
    {
        auto& instanceBufferView = instanceBufferViews[world.viewKey.v];
        auto& instanceVb = instanceData[world.viewKey.v];

        bool reCreate = false;
        if (pool.Alive() > instanceVb.size()) {
            particleSize = pool.Size();
            instanceVb.resize(particleSize);
            ParticleDriver::bufferDestroyFn(m_driver, instanceBufferView.buffer);
            reCreate = true;
        }
        UpdateParticle(pool, world, instanceVb, positionBuffer);

        if (reCreate || instanceBufferView.buffer.data.ptr == nullptr) {
            BufferCreate info = {};
            info.size = particleSize * static_cast<AZ::u32>(sizeof(ParticleMeshInstanceData));
            info.elementSize = static_cast<AZ::u32>(sizeof(ParticleMeshInstanceData));
            info.data = reinterpret_cast<const AZ::u8*>(instanceVb.data());
            info.usage = BufferUsage::STRUCTURED;
            info.memory = MemoryType::DYNAMIC;
            ParticleDriver::bufferCreateFn(m_driver, info, instanceBufferView.buffer);
            instanceBufferView.offset = 0;
            instanceBufferView.size = particleSize * static_cast<AZ::u32>(sizeof(ParticleMeshInstanceData));
            instanceBufferView.stride = sizeof(ParticleMeshInstanceData);
        } else {
            BufferUpdate info = {};
            info.usage = BufferUsage::STRUCTURED;
            info.memory = MemoryType::DYNAMIC;
            info.size = particleSize * static_cast<AZ::u32>(sizeof(ParticleMeshInstanceData));
            info.data = reinterpret_cast<const AZ::u8*>(instanceVb.data());
            ParticleDriver::bufferUpdateFn(m_driver, info, instanceBufferView.buffer);
        }
    }

    void ParticleMeshRender::Render(const AZ::u8* data, [[maybe_unused]] const BaseInfo& emitterInfo, AZ::u8* driver, const ParticlePool& pool,
        const WorldInfo& world, DrawItem& item)
    {
        if (data == nullptr || pool.Alive() == 0) {
            return;
        }
        const MeshConfig& config = *reinterpret_cast<const MeshConfig*>(data);
        m_driver = driver;
        UpdateBuffer(pool, world, item.positionBuffer);

        item.type = RenderType::MESH;
        item.drawArgs.type = DrawType::INDEXED;
        item.drawArgs.indexed.instanceCount = pool.Alive();
        item.instanceBuffer = instanceBufferViews[world.viewKey.v];
        SpriteVariantKeySetFacing(item.variantKey, config.facing);
    }
}
