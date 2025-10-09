/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <OpenParticleSystem/Asset/ParticleAssetHandler.h>
#include <AzCore/Component/Component.h>
#include <AzCore/Component/TickBus.h>

namespace OpenParticle
{
    class SystemComponent
        : public AZ::Component
        , private AZ::TickBus::Handler
    {
    public:
        AZ_COMPONENT(SystemComponent, "{D521B11D-AC30-4CC5-B9AD-41ECB816EA48}");

        SystemComponent() = default;
        ~SystemComponent() = default;

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);

        static void Reflect(AZ::ReflectContext* context);

    protected:
        // AZ::TickBus::Handler
        void OnTick(float delta, AZ::ScriptTimePoint timePoint) override;
        int GetTickOrder() override;

        void Activate() override;
        void Deactivate() override;

        void Initialize();

        void ShutDown();

    private:
        AZStd::unique_ptr<ParticleAssetHandler> m_assetHandler;
    };
} // namespace OpenParticle
