/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>

#include <Atom/RPI.Public/Pass/PassSystemInterface.h>
#include <Atom/RPI.Public/FeatureProcessorFactory.h>

#include <Components/HairSystemComponent.h>
#include <Components/HairComponentController.h>

#include <Rendering/HairFeatureProcessor.h>

namespace AZ
{
    namespace Render
    {
        namespace Hair
        {
            HairSystemComponent::HairSystemComponent() = default;
            HairSystemComponent::~HairSystemComponent() = default;

            void HairSystemComponent::Reflect(ReflectContext* context)
            {
                if (SerializeContext* serialize = azrtti_cast<SerializeContext*>(context))
                {
                    serialize->Class<HairSystemComponent, Component>()
                        ->Version(0)
                        ;
                }

                Hair::HairFeatureProcessor::Reflect(context);
            }

            void HairSystemComponent::GetProvidedServices(ComponentDescriptor::DependencyArrayType& provided)
            {
                provided.push_back(AZ_CRC_CE("HairService"));
            }

            void HairSystemComponent::GetIncompatibleServices(ComponentDescriptor::DependencyArrayType& incompatible)
            {
                incompatible.push_back(AZ_CRC_CE("HairService"));
            }

            void HairSystemComponent::GetRequiredServices([[maybe_unused]] ComponentDescriptor::DependencyArrayType& required)
            {
                required.push_back(AZ_CRC("ActorSystemService", 0x5e493d6c));
                required.push_back(AZ_CRC("EMotionFXAnimationService", 0x3f8a6369));
            }

            void HairSystemComponent::LoadPassTemplateMappings()
            {
                auto* passSystem = RPI::PassSystemInterface::Get();
                AZ_Assert(passSystem, "Cannot get the pass system.");

                const char* passTemplatesFile = "Passes/AtomTressFX_PassTemplates.azasset";
                passSystem->LoadPassTemplateMappings(passTemplatesFile);
            }

            void HairSystemComponent::Init()
            {
            }

            void HairSystemComponent::Activate()
            {
                // Feature processor
                AZ::RPI::FeatureProcessorFactory::Get()->RegisterFeatureProcessor<Hair::HairFeatureProcessor>();

                auto* passSystem = RPI::PassSystemInterface::Get();
                AZ_Assert(passSystem, "Cannot get the pass system.");

                // Setup handler for load pass templates mappings
                m_loadTemplatesHandler = RPI::PassSystemInterface::OnReadyLoadTemplatesEvent::Handler([this]() { this->LoadPassTemplateMappings(); });
                passSystem->ConnectEvent(m_loadTemplatesHandler);

                // Load the AtomTressFX pass classes
                passSystem->AddPassCreator(Name("HairSkinningComputePass"), &HairSkinningComputePass::Create);

                // Load the PPLL render method passes
                passSystem->AddPassCreator(Name("HairPPLLRasterPass"), &HairPPLLRasterPass::Create);
                passSystem->AddPassCreator(Name("HairPPLLResolvePass"), &HairPPLLResolvePass::Create);

                // Load the ShortCut render method passes
                passSystem->AddPassCreator(Name("HairShortCutGeometryDepthAlphaPass"), &HairShortCutGeometryDepthAlphaPass::Create);
                passSystem->AddPassCreator(Name("HairShortCutGeometryShadingPass"), &HairShortCutGeometryShadingPass::Create);
            }

            void HairSystemComponent::Deactivate()
            {
                AZ::RPI::FeatureProcessorFactory::Get()->UnregisterFeatureProcessor<Hair::HairFeatureProcessor>();
                m_loadTemplatesHandler.Disconnect();
            }
        } // namespace Hair
    } // End Render namespace
} // End AZ namespace

