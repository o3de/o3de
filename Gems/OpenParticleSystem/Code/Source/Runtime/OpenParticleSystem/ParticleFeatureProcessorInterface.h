/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
 
#pragma once

#include <Atom/RPI.Public/FeatureProcessor.h>
#include <Atom/RPI.Reflect/Image/Image.h>
#include <Atom/Utils/StableDynamicArray.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Math/Vector4.h>

#include <OpenParticleSystem/Asset/ParticleAsset.h>

namespace OpenParticle
{
    class ParticleDataInstance;
    class ParticleComponentConfig;

    class ParticleFeatureProcessorInterface
        : public AZ::RPI::FeatureProcessor
    {
    public:
        AZ_RTTI(OpenParticle::ParticleFeatureProcessorInterface, "{4ec72e75-10be-469e-8210-3a949c7a183a}",
            AZ::RPI::FeatureProcessor);

        using ParticleHandle = AZ::StableDynamicArrayHandle<ParticleDataInstance>;

        virtual void Init() = 0;

        virtual void ShutDown() = 0;

        virtual ParticleHandle AcquireParticle(const AZ::EntityId& id, const ParticleComponentConfig& rtConfig, const AZ::Transform& transform) = 0;

        virtual void ReleaseParticle(ParticleHandle& handle) = 0;

        virtual void SetTransform(
            const ParticleHandle& handle,
            const AZ::Transform& transform,
            const AZ::Vector3& nonUniformScale = AZ::Vector3::CreateOne()) = 0;

        virtual void SetMaterialDiffuseMap(const ParticleHandle& handle, AZ::u32 emitterIndex, AZStd::string mapPath) = 0;
    };

} // namespace OpenParticle
