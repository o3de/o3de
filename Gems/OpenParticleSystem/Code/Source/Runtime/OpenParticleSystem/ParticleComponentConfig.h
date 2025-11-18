/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
 
#pragma once

#include <AzCore/Component/ComponentBus.h>
#include <OpenParticleSystem/Asset/ParticleAsset.h>

namespace OpenParticle
{
    class ReflectContext;

    class ParticleComponentConfig final
        : public AZ::ComponentConfig
    {
    public:
        AZ_RTTI(OpenParticle::ParticleComponentConfig, "{8a0f1f42-a04c-4fd1-a7a9-9221078b5410}", AZ::ComponentConfig);

        static void Reflect(AZ::ReflectContext* context);

        AZ::Data::Asset<ParticleAsset> m_particleAsset = { AZ::Data::AssetLoadBehavior::QueueLoad };
        bool m_enable = true;
        bool m_followActiveCamera = false;
        bool m_autoPlay = true;
        bool m_inParticleEditor = false;
    };
} // namespace OpenParticle
