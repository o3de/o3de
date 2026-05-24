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
#include "core/math/Constants.h"

namespace SimuCore::ParticleCore {
    ParticleSpriteRender::~ParticleSpriteRender()
    {
        for (auto& view : bufferViews) {
            ParticleDriver::bufferDestroyFn(gDriver, view.second.buffer);
        }
        gDriver = nullptr;
    }

    AZ::u32 ParticleSpriteRender::DataSize() const
    {
        return sizeof(SpriteConfig);
    }

    static void UpdateParticle(const ParticlePool& pool, const SpriteConfig& config, const WorldInfo& world, AZStd::vector<ParticleSpriteVertex>& vb,
            AZStd::vector<AZ::Vector3>& positionBuffer, const GpuInstance& buffer, AZ::u8* gDriver)
    {
        AZ_PROFILE_SCOPE(AzCore, "UpdateParticle");
        pool.RenderAll([&config, &world, &vb, &positionBuffer, &buffer, gDriver](const Particle* particleData, AZ::u32 begin, AZ::u32 end) {
            const auto trans = world.emitterTransform.GetInverse();
            for (AZ::u32 index = begin; index < end; ++index) {
                const auto& particle = particleData[index];
                ParticleSpriteVertex& particleVertex = vb[index];
                if (particle.hasLightEffect) {
                    positionBuffer[index] = particle.globalPosition;
                }
                particleVertex.position = AZ::Vector4(trans.TransformPoint(particle.globalPosition), 0.f);
                particleVertex.color = particle.color;
                particleVertex.scale = AZ::Vector4(particle.scale, 1.0f);
                particleVertex.up = AZ::Vector4(world.cameraUp, 0.f);

                AZ::Vector3 vel = particle.velocity;
                if (vel.IsClose(AZ::Vector3::CreateZero())) {
                    particleVertex.velocity = AZ::Vector4(vel, 0.f);
                } else {
                    particleVertex.velocity = AZ::Vector4(vel.GetNormalized(), 0.f);
                }

                AZ::u32 width = std::max(static_cast<AZ::u32>(std::floor(config.subImageSize.GetX())), 1u);
                particleVertex.subuv = AZ::Vector4(config.subImageSize.GetX(), config.subImageSize.GetY(),
                        static_cast<float>((particle.subUVFrame % width) / config.subImageSize.GetX()),
                        static_cast<float>((particle.subUVFrame / width) / config.subImageSize.GetY()));
                AZ::Vector3 initAxis(particle.rotation.GetX(), particle.rotation.GetY(), particle.rotation.GetZ());
                if (config.facing == Facing::CUSTOM && initAxis.IsClose(AZ::Vector3::CreateZero()))
                {
                    particleVertex.initRotation = AZ::Vector4(initAxis, 0.f);
                }
                else {
                    particleVertex.initRotation = particle.rotation;
                }
                AZ::Vector3 rotateAxis(particle.rotationVector.GetX(), particle.rotationVector.GetY(), particle.rotationVector.GetZ());
                particleVertex.rotationVector = rotateAxis.IsClose(AZ::Vector3::CreateZero())
                    ? AZ::Vector4(rotateAxis, 0.f)
                    : AZ::Vector4(rotateAxis, AZ::DegToRad(particle.rotationVector.GetW()));
            }

            BufferUpdate info = {};
            info.usage = BufferUsage::VERTEX;
            info.memory = MemoryType::DYNAMIC;
            info.size = (end - begin) * static_cast<AZ::u32>(sizeof(ParticleSpriteVertex));
            info.offset = sizeof(ParticleSpriteVertex) * begin;
            info.data = reinterpret_cast<const AZ::u8*>(vb.data()) + info.offset;
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
            info.size = particleSize * static_cast<AZ::u32>(sizeof(ParticleSpriteVertex));
            info.data = nullptr; //must be nullptr here to avoid data copy
            info.usage = BufferUsage::VERTEX;
            info.memory = MemoryType::DYNAMIC;
            ParticleDriver::bufferCreateFn(gDriver, info, bufferView.buffer);
            bufferView.offset = 0;
            bufferView.size = particleSize * static_cast<AZ::u32>(sizeof(ParticleSpriteVertex));
            bufferView.stride = sizeof(ParticleSpriteVertex);
        }
        UpdateParticle(pool, config, world, vb, item.positionBuffer, bufferView.buffer, gDriver);
    }

    void ParticleSpriteRender::Render(const AZ::u8* data, [[maybe_unused]] const BaseInfo& emitterInfo, AZ::u8* driver, const ParticlePool& pool,
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
