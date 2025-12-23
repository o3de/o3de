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
        for (auto& view : bufferViews) {
            ParticleDriver::bufferDestroyFn(gDriver, view.second.buffer);
        }
        gDriver = nullptr;
    }

    AZ::u32 ParticleMeshRender::DataSize() const
    {
        return sizeof(MeshConfig);
    }

    static void UpdateParticle(const ParticlePool& pool, const WorldInfo& world, AZStd::vector<ParticleMeshVertex>& vb,
            AZStd::vector<AZ::Vector3>& positionBuffer)
    {
        const Particle* particle = pool.ParticleData().data();
        ParticleMeshVertex* meshInfo = vb.data();
        for (AZ::u32 i = 0; i < pool.Alive(); ++i) {
            const Particle& curr = particle[i];
            if (curr.hasLightEffect) {
                positionBuffer[i] = curr.globalPosition;
            }
            meshInfo[i].position = curr.globalPosition - world.emitterTransform.GetTranslation();
            meshInfo[i].color = curr.color;
            meshInfo[i].scale = curr.scale;
            AZ::Vector3 initAxis(curr.rotation.GetX(), curr.rotation.GetY(), curr.rotation.GetZ());
            meshInfo[i].initRotation = initAxis.IsClose(AZ::Vector3::CreateZero()) ? AZ::Vector4(initAxis, 0.f) : curr.rotation;
            AZ::Vector3 rotateAxis(curr.rotationVector.GetX(), curr.rotationVector.GetY(), curr.rotationVector.GetZ());
            meshInfo[i].rotationVector = rotateAxis.IsClose(AZ::Vector3::CreateZero())
                ? AZ::Quaternion(rotateAxis, 0.f)
                : AZ::Quaternion(rotateAxis, AZ::DegToRad(curr.rotationVector.GetW()));
        }
    }

    void ParticleMeshRender::UpdateBuffer(const ParticlePool& pool, const WorldInfo& world, AZStd::vector<AZ::Vector3>& positionBuffer)
    {
        auto& bufferView = bufferViews[world.viewKey.v];
        auto& vb = vbs[world.viewKey.v];

        bool reCreate = false;
        if (pool.Alive() > vb.size()) {
            particleSize = pool.Size();
            vb.resize(particleSize);
            ParticleDriver::bufferDestroyFn(gDriver, bufferView.buffer);
            reCreate = true;
        }
        UpdateParticle(pool, world, vb, positionBuffer);

        if (reCreate || bufferView.buffer.data.ptr == nullptr) {
            BufferCreate info = {};
            info.size = particleSize * static_cast<AZ::u32>(sizeof(ParticleMeshVertex));
            info.data = reinterpret_cast<const AZ::u8*>(vb.data());
            info.usage = BufferUsage::VERTEX;
            info.memory = MemoryType::DYNAMIC;
            ParticleDriver::bufferCreateFn(gDriver, info, bufferView.buffer);
            bufferView.offset = 0;
            bufferView.size = particleSize * static_cast<AZ::u32>(sizeof(ParticleMeshVertex));
            bufferView.stride = sizeof(ParticleMeshVertex);
        } else {
            BufferUpdate info = {};
            info.usage = BufferUsage::VERTEX;
            info.memory = MemoryType::DYNAMIC;
            info.size = particleSize * static_cast<AZ::u32>(sizeof(ParticleMeshVertex));
            info.data = reinterpret_cast<const AZ::u8*>(vb.data());
            ParticleDriver::bufferUpdateFn(gDriver, info, bufferView.buffer);
        }
    }

    void ParticleMeshRender::Render(const AZ::u8* data, [[maybe_unused]] const BaseInfo& emitterInfo, AZ::u8* driver, const ParticlePool& pool,
        const WorldInfo& world, DrawItem& item)
    {
        if (data == nullptr || pool.Alive() == 0) {
            return;
        }
        const MeshConfig& config = *reinterpret_cast<const MeshConfig*>(data);
        gDriver = driver;
        UpdateBuffer(pool, world, item.positionBuffer);

        item.type = RenderType::MESH;
        item.drawArgs.type = DrawType::INDEXED;
        item.drawArgs.indexed.instanceCount = pool.Alive();
        item.vertexBuffer = bufferViews[world.viewKey.v];
        SpriteVariantKeySetFacing(item.variantKey, config.facing);
    }
}
