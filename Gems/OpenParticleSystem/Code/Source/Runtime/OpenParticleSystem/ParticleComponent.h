/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
 
#pragma once

#include <AzFramework/Components/ComponentAdapter.h>
#include <OpenParticleSystem/ParticleComponentConfig.h>
#include <OpenParticleSystem/ParticleComponentController.h>

namespace OpenParticle
{
    class ParticleComponent final
        : public AzFramework::Components::ComponentAdapter<ParticleComponentController
        , ParticleComponentConfig>
    {
    public:
        using BaseClass = AzFramework::Components::ComponentAdapter<ParticleComponentController, ParticleComponentConfig>;
        AZ_COMPONENT(ParticleComponent, "{250342FE-9592-4194-BBE9-FBF5CF8FD9E8}", BaseClass);

        static void Reflect(AZ::ReflectContext* context);

        ParticleComponent() = default;
        explicit ParticleComponent(const ParticleComponentConfig& config);
        ~ParticleComponent() = default;
    };
} // namespace OpenParticle
