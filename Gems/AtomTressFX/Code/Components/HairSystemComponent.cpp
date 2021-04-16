/*
 * All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
 * its licensors.
 *
 * For complete copyright and license terms please see the LICENSE at the root of this
 * distribution (the "License"). All use of this software is governed by the License,
 * or, if provided, by the license below or the license accompanying this file. Do not
 * remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *
 */


#include <Atom/RPI.Public/Pass/PassSystemInterface.h>
//#include <Atom/RPI.Public/RPIUtils.h>

#include <AzCore/Serialization/SerializeContext.h>

#include <Atom/RPI.Public/FeatureProcessorFactory.h>
//#include <Atom/RPI.Public/Pass/PassSystemInterface.h>

#include <Components/HairSystemComponent.h>

#include <Rendering/HairFeatureProcessor.h>
#include <Components/HairComponentController.h>
//#include <Rendering/HairDispatchItem.h>
//#include <Rendering/SharedBuffer.h>
//#include <Rendering/HairCommon.h>

//#include <Passes/HairSkinningComputePass.h>

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
//                Hair::HairComponentController::Reflect(context);
            }

            void HairSystemComponent::GetProvidedServices(ComponentDescriptor::DependencyArrayType& provided)
            {
//                provided.push_back(AZ_CRC("HairService", 0xf1c5a6c3));
                provided.push_back(AZ_CRC_CE("HairService"));
            }

            void HairSystemComponent::GetIncompatibleServices(ComponentDescriptor::DependencyArrayType& incompatible)
            {
//                incompatible.push_back(AZ_CRC("HairService", 0xf1c5a6c3));
                incompatible.push_back(AZ_CRC_CE("HairService"));

            }

            void HairSystemComponent::GetRequiredServices([[maybe_unused]] ComponentDescriptor::DependencyArrayType& required)
            {
                // [To Do] Adi: add dependency in Actor
                required.push_back(AZ_CRC("ActorSystemService", 0x5e493d6c));
                required.push_back(AZ_CRC("EMotionFXAnimationService", 0x3f8a6369));
            }

            void HairSystemComponent::Init()
            {
            }

            void HairSystemComponent::Activate()
            {
                // Feature processor
                AZ::RPI::FeatureProcessorFactory::Get()->RegisterFeatureProcessor<Hair::HairFeatureProcessor>();

                // Adding all Hair passes
                auto* passSystem = RPI::PassSystemInterface::Get();

                AZ_Assert(passSystem, "Cannot get the pass system.");
                passSystem->AddPassCreator(Name("HairSkinningComputePass"), &HairSkinningComputePass::Create);
                passSystem->AddPassCreator(Name("HairPPLLRasterPass"), &HairPPLLRasterPass::Create);
                passSystem->AddPassCreator(Name("HairPPLLResolvePass"), &HairPPLLResolvePass::Create);
            }

            void HairSystemComponent::Deactivate()
            {
                AZ::RPI::FeatureProcessorFactory::Get()->UnregisterFeatureProcessor<Hair::HairFeatureProcessor>();
            }
        } // namespace Hair
    } // End Render namespace
} // End AZ namespace

