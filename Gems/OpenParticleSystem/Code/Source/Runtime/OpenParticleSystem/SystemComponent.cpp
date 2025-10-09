/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <OpenParticleSystem/Asset/ParticleAsset.h>
#include <OpenParticleSystem/Asset/ParticleAssetHandler.h>
#include <Atom/RPI.Public/FeatureProcessorFactory.h>
#include <AtomCore/Instance/InstanceDatabase.h>
#include <OpenParticleSystem/ParticleComponent.h>
#include <OpenParticleSystem/ParticleFeatureProcessor.h>
#include <OpenParticleSystem/SystemComponent.h>

namespace OpenParticle
{
    void SystemComponent::Activate()
    {
        Initialize();
        AZ::RPI::FeatureProcessorFactory::Get()
            ->RegisterFeatureProcessorWithInterface<ParticleFeatureProcessor, ParticleFeatureProcessorInterface>();
        AZ::TickBus::Handler::BusConnect();
    }

    void SystemComponent::Deactivate()
    {
        AZ::RPI::FeatureProcessorFactory::Get()->UnregisterFeatureProcessor<ParticleFeatureProcessor>();
        AZ::TickBus::Handler::BusDisconnect();
        ShutDown();
    }

    void SystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<SystemComponent, AZ::Component>()->Version(0);
        }
        ParticleFeatureProcessor::Reflect(context);
        ParticleConfig::Reflect(context);
        ParticleAsset::Reflect(context);
    }

    void SystemComponent::OnTick(float delta, AZ::ScriptTimePoint timePoint)
    {
        AZ_UNUSED(delta);
        AZ_UNUSED(timePoint);
    }

    int SystemComponent::GetTickOrder()
    {
        return AZ::TICK_DEFAULT;
    }

    void SystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("OpenParticleService"));
    }

    void SystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("OpenParticleService"));
    }

    void SystemComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC_CE("RPISystem"));
    }

    void SystemComponent::Initialize()
    {
        m_assetHandler = MakeAssetHandler<ParticleAssetHandler>();

        AZ::Data::InstanceHandler<ParticleSystem> handler;
        handler.m_createFunctionWithParam = [](AZ::Data::AssetData* particleAsset, const AZStd::any* rtConfig)
        {
            return ParticleSystem::CreateInternal(*(azrtti_cast<ParticleAsset*>(particleAsset)),
                                                  *(reinterpret_cast<const ParticleComponentConfig*>(rtConfig)));
        };
        AZ::Data::InstanceDatabase<ParticleSystem>::Create(azrtti_typeid<ParticleAsset>(), handler);
    }

    void SystemComponent::ShutDown()
    {
        m_assetHandler->Unregister();
        AZ::Data::InstanceDatabase<ParticleSystem>::Destroy();
    }
} // namespace OpenParticle
