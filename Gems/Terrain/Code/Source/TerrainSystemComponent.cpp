/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of
 * this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <TerrainSystemComponent.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/EditContextConstants.inl>

#include <Atom/RPI.Public/FeatureProcessorFactory.h>
#include <TerrainFeatureProcessor.h>

namespace Terrain
{
    void TerrainSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<TerrainSystemComponent, AZ::Component>()
                ->Version(0)
                ;

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<TerrainSystemComponent>("Terrain", "[Description of functionality provided by this System Component]")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System"))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }

            Terrain::TerrainFeatureProcessor::Reflect(context);
        }
    }

    void TerrainSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("TerrainService"));
    }

    void TerrainSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("TerrainService"));
    }

    void TerrainSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC_CE("RPISystem"));
    }

    void TerrainSystemComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        AZ_UNUSED(dependent);
    }

    void TerrainSystemComponent::Init()
    {
    }

    void TerrainSystemComponent::Activate()
    {
        AZ::RPI::FeatureProcessorFactory::Get()->RegisterFeatureProcessor<Terrain::TerrainFeatureProcessor>();

        TerrainRequestBus::Handler::BusConnect();
        AZ::TickBus::Handler::BusConnect();
    }

    void TerrainSystemComponent::Deactivate()
    {
        AZ::TickBus::Handler::BusDisconnect();
        TerrainRequestBus::Handler::BusDisconnect();

        AZ::RPI::FeatureProcessorFactory::Get()->UnregisterFeatureProcessor<Terrain::TerrainFeatureProcessor>();
    }

    void TerrainSystemComponent::OnTick(float /*deltaTime*/, AZ::ScriptTimePoint /*time*/)
    {

    }

} // namespace Terrain
