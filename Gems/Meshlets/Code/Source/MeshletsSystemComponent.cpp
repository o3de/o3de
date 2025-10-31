
/*
* Modifications Copyright (c) Contributors to the Open 3D Engine Project. 
* For complete copyright and license terms please see the LICENSE at the root of this distribution.
* 
* SPDX-License-Identifier: Apache-2.0 OR MIT
*
*/

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/EditContextConstants.inl>

#include <Atom/RHI/Factory.h>
#include <Atom/RPI.Public/RPISystemInterface.h>

#include <Atom/RPI.Public/FeatureProcessorFactory.h>

#include <MeshletsSystemComponent.h>
#include <MeshletsFeatureProcessor.h>
#include <MultiDispatchComputePass.h>

namespace AZ
{
    namespace Meshlets
    {
        void MeshletsSystemComponent::Reflect(AZ::ReflectContext* context)
        {
            if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serialize->Class<MeshletsSystemComponent, AZ::Component>()
                    ->Version(0)
                    ;

                if (AZ::EditContext* ec = serialize->GetEditContext())
                {
                    ec->Class<MeshletsSystemComponent>("Meshlets", "[Description of functionality provided by this System Component]")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ;
                }
            }

            Meshlets::MeshletsFeatureProcessor::Reflect(context);
        }

        void MeshletsSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC_CE("MeshletsService"));
        }

        void MeshletsSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC_CE("MeshletsService"));
        }

        void MeshletsSystemComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ::RHI::Factory::GetComponentService());
            required.push_back(AZ_CRC_CE("AssetDatabaseService"));
            required.push_back(AZ_CRC_CE("RPISystem"));
        }

        void MeshletsSystemComponent::GetDependentServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& dependent)
        {
        }

        MeshletsSystemComponent::MeshletsSystemComponent()
        {
            if (MeshletsInterface::Get() == nullptr)
            {
                MeshletsInterface::Register(this);
            }
        }

        MeshletsSystemComponent::~MeshletsSystemComponent()
        {
            if (MeshletsInterface::Get() == this)
            {
                MeshletsInterface::Unregister(this);
            }
        }

        void MeshletsSystemComponent::Init()
        {
        }

        void MeshletsSystemComponent::LoadPassTemplateMappings()
        {
            auto* passSystem = AZ::RPI::PassSystemInterface::Get();
            AZ_Assert(passSystem, "Meshlets Gem - cannot get the pass system.");

            const char* passTemplatesFile = "Passes/MeshletsPassTemplates.azasset";
            passSystem->LoadPassTemplateMappings(passTemplatesFile);
        }

        void MeshletsSystemComponent::Activate()
        {
            // Feature processor
            AZ::RPI::FeatureProcessorFactory::Get()->RegisterFeatureProcessor<Meshlets::MeshletsFeatureProcessor>();

            auto* passSystem = AZ::RPI::PassSystemInterface::Get();
            AZ_Assert(passSystem, "Cannot get the pass system.");

            // Setup handler for load pass templates mappings
            m_loadTemplatesHandler = AZ::RPI::PassSystemInterface::OnReadyLoadTemplatesEvent::Handler([this]() { this->LoadPassTemplateMappings(); });
            passSystem->ConnectEvent(m_loadTemplatesHandler);

            passSystem->AddPassCreator(AZ::Name("MultiDispatchComputePass"), &MultiDispatchComputePass::Create);
            passSystem->AddPassCreator(AZ::Name("MeshletsRenderPass"), &MeshletsRenderPass::Create);

            MeshletsRequestBus::Handler::BusConnect();
            AZ::TickBus::Handler::BusConnect();
        }

        void MeshletsSystemComponent::Deactivate()
        {
            AZ::TickBus::Handler::BusDisconnect();
            MeshletsRequestBus::Handler::BusDisconnect();

            AZ::RPI::FeatureProcessorFactory::Get()->UnregisterFeatureProcessor<Meshlets::MeshletsFeatureProcessor>();
        }

        void MeshletsSystemComponent::OnTick([[maybe_unused]] float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint time)
        {
        }

    } // namespace Meshlets
} // namespace AZ

