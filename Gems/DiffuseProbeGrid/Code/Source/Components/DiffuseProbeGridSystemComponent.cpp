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
#include <Components/DiffuseProbeGridSystemComponent.h>
#include <Components/DiffuseProbeGridComponentController.h>
#include <Render/DiffuseProbeGridFeatureProcessor.h>
#include <Render/DiffuseGlobalIlluminationFeatureProcessor.h>
#include <Render/DiffuseProbeGridBlendDistancePass.h>
#include <Render/DiffuseProbeGridBlendIrradiancePass.h>
#include <Render/DiffuseProbeGridBorderUpdatePass.h>
#include <Render/DiffuseProbeGridClassificationPass.h>
#include <Render/DiffuseProbeGridDownsamplePass.h>
#include <Render/DiffuseProbeGridPreparePass.h>
#include <Render/DiffuseProbeGridQueryPass.h>
#include <Render/DiffuseProbeGridQueryFullscreenPass.h>
#include <Render/DiffuseProbeGridQueryFullscreenPassData.h>
#include <Render/DiffuseProbeGridRayTracingPass.h>
#include <Render/DiffuseProbeGridRelocationPass.h>
#include <Render/DiffuseProbeGridRenderPass.h>
#include <Render/DiffuseProbeGridVisualizationAccelerationStructurePass.h>
#include <Render/DiffuseProbeGridVisualizationCompositePass.h>
#include <Render/DiffuseProbeGridVisualizationPreparePass.h>
#include <Render/DiffuseProbeGridVisualizationRayTracingPass.h>

namespace AZ
{
    namespace Render
    {
        void DiffuseProbeGridSystemComponent::Reflect(ReflectContext* context)
        {
            if (SerializeContext* serialize = azrtti_cast<SerializeContext*>(context))
            {
                serialize->Class<DiffuseProbeGridSystemComponent, Component>()
                    ->Version(0)
                    ;
            }

            DiffuseProbeGridFeatureProcessor::Reflect(context);
            DiffuseGlobalIlluminationFeatureProcessor::Reflect(context);
            DiffuseProbeGridQueryFullscreenPassData::Reflect(context);
        }

        void DiffuseProbeGridSystemComponent::GetProvidedServices(ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC_CE("DiffuseProbeGrid"));
            provided.push_back(AZ_CRC_CE("DiffuseGlobalIllumination"));
        }

        void DiffuseProbeGridSystemComponent::GetIncompatibleServices(ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC_CE("DiffuseProbeGrid"));
            incompatible.push_back(AZ_CRC_CE("DiffuseGlobalIllumination"));
        }

        void DiffuseProbeGridSystemComponent::GetRequiredServices(ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ_CRC_CE("RPISystem"));
        }

        void DiffuseProbeGridSystemComponent::LoadPassTemplateMappings()
        {
            auto* passSystem = RPI::PassSystemInterface::Get();
            AZ_Assert(passSystem, "PassSystemInterface is null");

            const char* passTemplatesFile = "Passes/DiffuseProbeGridTemplates.azasset";
            passSystem->LoadPassTemplateMappings(passTemplatesFile);
        }

        void DiffuseProbeGridSystemComponent::Activate()
        {
            // register feature processors
            RPI::FeatureProcessorFactory::Get()->RegisterFeatureProcessor<DiffuseProbeGridFeatureProcessor>();
            RPI::FeatureProcessorFactory::Get()->RegisterFeatureProcessor<DiffuseGlobalIlluminationFeatureProcessor>();

            auto* passSystem = RPI::PassSystemInterface::Get();
            AZ_Assert(passSystem, "PassSystemInterface is null");

            // setup handler for pass templates mappings
            m_loadTemplatesHandler = RPI::PassSystemInterface::OnReadyLoadTemplatesEvent::Handler([this]() { this->LoadPassTemplateMappings(); });
            passSystem->ConnectEvent(m_loadTemplatesHandler);

            // load the DiffuseProbeGrid pass classes
            passSystem->AddPassCreator(Name("DiffuseProbeGridPreparePass"), &DiffuseProbeGridPreparePass::Create);
            passSystem->AddPassCreator(Name("DiffuseProbeGridRayTracingPass"), &DiffuseProbeGridRayTracingPass::Create);
            passSystem->AddPassCreator(Name("DiffuseProbeGridBlendIrradiancePass"), &DiffuseProbeGridBlendIrradiancePass::Create);
            passSystem->AddPassCreator(Name("DiffuseProbeGridBlendDistancePass"), &DiffuseProbeGridBlendDistancePass::Create);
            passSystem->AddPassCreator(Name("DiffuseProbeGridBorderUpdatePass"), &DiffuseProbeGridBorderUpdatePass::Create);
            passSystem->AddPassCreator(Name("DiffuseProbeGridRelocationPass"), &DiffuseProbeGridRelocationPass::Create);
            passSystem->AddPassCreator(Name("DiffuseProbeGridClassificationPass"), &DiffuseProbeGridClassificationPass::Create);
            passSystem->AddPassCreator(Name("DiffuseProbeGridDownsamplePass"), &DiffuseProbeGridDownsamplePass::Create);
            passSystem->AddPassCreator(Name("DiffuseProbeGridRenderPass"), &DiffuseProbeGridRenderPass::Create);
            passSystem->AddPassCreator(Name("DiffuseProbeGridVisualizationPreparePass"), &DiffuseProbeGridVisualizationPreparePass::Create);
            passSystem->AddPassCreator(Name("DiffuseProbeGridVisualizationAccelerationStructurePass"), &DiffuseProbeGridVisualizationAccelerationStructurePass::Create);
            passSystem->AddPassCreator(Name("DiffuseProbeGridVisualizationRayTracingPass"), &DiffuseProbeGridVisualizationRayTracingPass::Create);
            passSystem->AddPassCreator(Name("DiffuseProbeGridVisualizationCompositePass"), &DiffuseProbeGridVisualizationCompositePass::Create);
            passSystem->AddPassCreator(Name("DiffuseProbeGridQueryPass"), &DiffuseProbeGridQueryPass::Create);
            passSystem->AddPassCreator(Name("DiffuseProbeGridQueryFullscreenPass"), &DiffuseProbeGridQueryFullscreenPass::Create);
        }

        void DiffuseProbeGridSystemComponent::Deactivate()
        {
            RPI::FeatureProcessorFactory::Get()->UnregisterFeatureProcessor<DiffuseProbeGridFeatureProcessor>();
            RPI::FeatureProcessorFactory::Get()->UnregisterFeatureProcessor<DiffuseGlobalIlluminationFeatureProcessor>();

            m_loadTemplatesHandler.Disconnect();
        }
    }
}
