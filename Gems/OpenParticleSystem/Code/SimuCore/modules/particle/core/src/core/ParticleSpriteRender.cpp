/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ParticleSpriteRender.h"
#include <algorithm>
#include "particle/core/ParticlePool.h"
#include "particle/core/ParticleDriver.h"
#include "particle/core/ParticleHelper.h"

namespace SimuCore::ParticleCore {
    ParticleSpriteRender::~ParticleSpriteRender()
    {
        for (auto& view : bufferViews) {
            ParticleDriver::bufferDestroyFn(gDriver, view.second.buffer);
        }
        gDriver = nullptr;
    }

    uint32_t ParticleSpriteRender::DataSize() const
    {
        return sizeof(SpriteConfig);
    }

    static void UpdateParticle(const ParticlePool& pool, const SpriteConfig& config, const WorldInfo& world, std::vector<ParticleSpriteVertex>& vb,
            std::vector<Vector3>& positionBuffer, const GpuInstance& buffer, uint8_t* gDriver)
    {
        AZ_PROFILE_SCOPE(AzCore, "UpdateParticle");
        pool.RenderAll([&config, &world, &vb, &positionBuffer, &buffer, gDriver](const Particle* particleData, uint32_t begin, uint32_t end) {
            const auto trans = world.emitterTransform.Inverse();
            for (uint32_t index = begin; index < end; ++index) {
                const auto& particle = particleData[index];
                ParticleSpriteVertex& particleVertex = vb[index];
                if (particle.hasLightEffect) {
                    positionBuffer[index] = particle.globalPosition;
                }
                particleVertex.position = Vector4(trans.TransformPoint(particle.globalPosition), 0.f);
                particleVertex.color = particle.color;
                particleVertex.scale = Vector4(particle.scale, 1.0f);
                particleVertex.up = Vector4(world.cameraUp, 0.f);

                Vector3 vel = particle.velocity;
                if (vel.IsEqual(VEC3_ZERO)) {
                    particleVertex.velocity = Vector4(vel, 0.f);
                } else {
                    particleVertex.velocity = Vector4(vel.Normalize(), 0.f);
                }

                uint32_t width = std::max(static_cast<uint32_t>(std::floor(config.subImageSize.GetX())), 1u);
                particleVertex.subuv = Vector4(config.subImageSize.GetX(), config.subImageSize.GetY(),
                        static_cast<float>((particle.subUVFrame % width) / config.subImageSize.GetX()),
                        static_cast<float>((particle.subUVFrame / width) / config.subImageSize.GetY()));
                Vector3 initAxis(particle.rotation.x, particle.rotation.y, particle.rotation.z);
                if (config.facing == Facing::CUSTOM && initAxis.IsEqual(VEC3_ZERO)) {
                    particleVertex.initRotation = Vector4(initAxis, 0.f);
                } else {
                    particleVertex.initRotation = particle.rotation;
                }
                Vector3 rotateAxis(particle.rotationVector.x, particle.rotationVector.y, particle.rotationVector.z);
                particleVertex.rotationVector = rotateAxis.IsEqual(VEC3_ZERO) ? Vector4(rotateAxis, 0.f) :
                                                Vector4(rotateAxis, Math::AngleToRadians(particle.rotationVector.w));
            }

            BufferUpdate info = {};
            info.usage = BufferUsage::VERTEX;
            info.memory = MemoryType::DYNAMIC;
            info.size = (end - begin) * static_cast<uint32_t>(sizeof(ParticleSpriteVertex));
            info.offset = sizeof(ParticleSpriteVertex) * begin;
            info.data = reinterpret_cast<const uint8_t*>(vb.data()) + info.offset;
            ParticleDriver::bufferUpdateFn(gDriver, info, buffer);
        });
    }

    void ParticleSpriteRender::UpdateBuffer(const ParticlePool& pool, const SpriteConfig& config,
        const WorldInfo& world, DrawItem& item)
    {
        AZ_PROFILE_SCOPE(AzCore, "ParticleSpriteRender::UpdateBuffer");
        auto& bufferView = bufferViews[world.viewKey.v];
        auto& vb = vbs[world.viewKey.v];
        if (pool.Alive() > vb.size()) {
            particleSize = pool.Size();
            vb.resize(particleSize);
            ParticleDriver::bufferDestroyFn(gDriver, bufferView.buffer);

            BufferCreate info = {};
            info.size = particleSize * static_cast<uint32_t>(sizeof(ParticleSpriteVertex));
            info.data = nullptr; //must be nullptr here to avoid data copy
            info.usage = BufferUsage::VERTEX;
            info.memory = MemoryType::DYNAMIC;
            ParticleDriver::bufferCreateFn(gDriver, info, bufferView.buffer);
            bufferView.offset = 0;
            bufferView.size = particleSize * static_cast<uint32_t>(sizeof(ParticleSpriteVertex));
            bufferView.stride = sizeof(ParticleSpriteVertex);
        }
        UpdateParticle(pool, config, world, vb, item.positionBuffer, bufferView.buffer, gDriver);
    }

    void ParticleSpriteRender::Render(const uint8_t* data, [[maybe_unused]] const BaseInfo& emitterInfo, uint8_t* driver, const ParticlePool& pool,
        const WorldInfo& world, DrawItem& item)
    {
        AZ_PROFILE_SCOPE(AzCore, "ParticleSpriteRender::Render");
        if (data == nullptr || pool.Alive() == 0) {
            return;
        }
        const SpriteConfig& config = *reinterpret_cast<const SpriteConfig*>(data);
        gDriver = driver;

        UpdateBuffer(pool, config, world, item);

        item.type = RenderType::SPRITE;
        item.drawArgs.type = DrawType::LINEAR;
        item.drawArgs.linear.vertexCount = VERTEX_COUNT;
        item.drawArgs.linear.instanceOffset = 0;
        item.drawArgs.linear.instanceCount = pool.Alive();
        item.drawArgs.linear.vertexOffset = 0;
        item.vertexBuffer = bufferViews[world.viewKey.v];
        SpriteVariantKeySetFacing(item.variantKey, config.facing);
    }
}
