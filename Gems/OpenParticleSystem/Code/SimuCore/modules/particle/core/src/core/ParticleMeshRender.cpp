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

    uint32_t ParticleMeshRender::DataSize() const
    {
        return sizeof(MeshConfig);
    }

    static void UpdateParticle(const ParticlePool& pool, const WorldInfo& world, std::vector<ParticleMeshVertex>& vb,
            std::vector<Vector3>& positionBuffer)
    {
        const Particle* particle = pool.ParticleData().data();
        ParticleMeshVertex* meshInfo = vb.data();
        for (uint32_t i = 0; i < pool.Alive(); ++i) {
            const Particle& curr = particle[i];
            if (curr.hasLightEffect) {
                positionBuffer[i] = curr.globalPosition;
            }
            meshInfo[i].position = curr.globalPosition - world.emitterTransform.GetTranslation();
            meshInfo[i].color = curr.color;
            meshInfo[i].scale = curr.scale;
            Vector3 initAxis(curr.rotation.x, curr.rotation.y, curr.rotation.z);
            meshInfo[i].initRotation = initAxis.IsEqual(VEC3_ZERO) ? Vector4(initAxis, 0.f) : curr.rotation;
            Vector3 rotateAxis(curr.rotationVector.x, curr.rotationVector.y, curr.rotationVector.z);
            meshInfo[i].rotationVector = rotateAxis.IsEqual(VEC3_ZERO) ? Quaternion(rotateAxis, 0.f) :
                Quaternion(rotateAxis, Math::AngleToRadians(curr.rotationVector.w));
        }
    }

    void ParticleMeshRender::UpdateBuffer(const ParticlePool& pool, const WorldInfo& world, std::vector<Vector3>& positionBuffer)
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
            info.size = particleSize * static_cast<uint32_t>(sizeof(ParticleMeshVertex));
            info.data = reinterpret_cast<const uint8_t*>(vb.data());
            info.usage = BufferUsage::VERTEX;
            info.memory = MemoryType::DYNAMIC;
            ParticleDriver::bufferCreateFn(gDriver, info, bufferView.buffer);
            bufferView.offset = 0;
            bufferView.size = particleSize * static_cast<uint32_t>(sizeof(ParticleMeshVertex));
            bufferView.stride = sizeof(ParticleMeshVertex);
        } else {
            BufferUpdate info = {};
            info.usage = BufferUsage::VERTEX;
            info.memory = MemoryType::DYNAMIC;
            info.size = particleSize * static_cast<uint32_t>(sizeof(ParticleMeshVertex));
            info.data = reinterpret_cast<const uint8_t*>(vb.data());
            ParticleDriver::bufferUpdateFn(gDriver, info, bufferView.buffer);
        }
    }

    void ParticleMeshRender::Render(const uint8_t* data, [[maybe_unused]] const BaseInfo& emitterInfo, uint8_t* driver, const ParticlePool& pool,
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
