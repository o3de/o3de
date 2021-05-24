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

#include <CommonSystemComponent.h>
#include <Source/Material/UseTextureFunctor.h>
#include <Source/Material/SubsurfaceTransmissionParameterFunctor.h>
#include <Source/Material/Transform2DFunctor.h>
#include <Source/Material/ShaderEnableFunctor.h>
#include <Source/Material/PropertyVisibilityFunctor.h>
#include <Source/Material/DrawListFunctor.h>
#include <Source/Material/ConvertEmissiveUnitFunctor.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/EditContextConstants.inl>

#include <Atom/Feature/Material/MaterialAssignment.h>
#include <Atom/Feature/Material/MaterialAssignmentId.h>

#include <Atom/Feature/TransformService/TransformServiceFeatureProcessor.h>
#include <Atom/Feature/Mesh/MeshFeatureProcessor.h>
#include <Atom/Feature/LookupTable/LookupTableAsset.h>
#include <Atom/Feature/ReflectionProbe/ReflectionProbeFeatureProcessor.h>
#include <RayTracing/RayTracingFeatureProcessor.h>

#include <Atom/Feature/ImageBasedLights/ImageBasedLightFeatureProcessor.h>
#include <Atom/Feature/DisplayMapper/AcesOutputTransformLutPass.h>
#include <Atom/Feature/DisplayMapper/AcesOutputTransformPass.h>
#include <Atom/Feature/DisplayMapper/ApplyShaperLookupTablePass.h>
#include <Atom/Feature/DisplayMapper/BakeAcesOutputTransformLutPass.h>
#include <Atom/Feature/DisplayMapper/DisplayMapperPass.h>
#include <Atom/Feature/DisplayMapper/DisplayMapperFullScreenPass.h>
#include <Atom/Feature/DisplayMapper/DisplayMapperConfigurationDescriptor.h>
#include <Atom/Feature/DisplayMapper/OutputTransformPass.h>
#include <Atom/Feature/ACES/AcesDisplayMapperFeatureProcessor.h>
#include <Atom/Feature/AuxGeom/AuxGeomFeatureProcessor.h>
#include <Atom/Feature/Utils/LightingPreset.h>
#include <Atom/Feature/Utils/ModelPreset.h>
#include <PostProcess/PostProcessFeatureProcessor.h>
#include <PostProcessing/BlendColorGradingLutsPass.h>
#include <PostProcessing/BloomParentPass.h>
#include <PostProcessing/DepthUpsamplePass.h>
#include <PostProcessing/DepthOfFieldCompositePass.h>
#include <PostProcessing/DepthOfFieldBokehBlurPass.h>
#include <PostProcessing/DepthOfFieldMaskPass.h>
#include <PostProcessing/DepthOfFieldParentPass.h>
#include <PostProcessing/DepthOfFieldReadBackFocusDepthPass.h>
#include <PostProcessing/DepthOfFieldWriteFocusDepthFromGpuPass.h>
#include <PostProcessing/EyeAdaptationPass.h>
#include <PostProcessing/LuminanceHistogramGeneratorPass.h>
#include <PostProcessing/FastDepthAwareBlurPasses.h>
#include <PostProcessing/LookModificationCompositePass.h>
#include <PostProcessing/LookModificationTransformPass.h>
#include <PostProcessing/SMAAFeatureProcessor.h>
#include <PostProcessing/SMAAEdgeDetectionPass.h>
#include <PostProcessing/SMAABlendingWeightCalculationPass.h>
#include <PostProcessing/SMAANeighborhoodBlendingPass.h>
#include <PostProcessing/SsaoPasses.h>
#include <PostProcessing/SubsurfaceScatteringPass.h>
#include <PostProcessing/BloomDownsamplePass.h>
#include <PostProcessing/BloomBlurPass.h>
#include <PostProcessing/BloomCompositePass.h>
#include <ScreenSpace/DeferredFogPass.h>
#include <Shadows/ProjectedShadowFeatureProcessor.h>

#include <SkyBox/SkyBoxFeatureProcessor.h>

#include <Atom/RPI.Public/Pass/PassSystemInterface.h>

#if AZ_TRAIT_LUXCORE_SUPPORTED
#include <Atom/Feature/LuxCore/RenderTexturePass.h>
#include <Atom/Feature/LuxCore/LuxCoreTexturePass.h>
#endif

#include <Checkerboard/CheckerboardPass.h>
#include <Checkerboard/CheckerboardColorResolvePass.h>

#include <CoreLights/LightCullingTilePreparePass.h>
#include <CoreLights/LightCullingPass.h>
#include <CoreLights/LightCullingRemap.h>
#include <Decals/DecalTextureArrayFeatureProcessor.h>
#include <ImGui/ImGuiPass.h>

#include <RayTracing/RayTracingAccelerationStructurePass.h>
#include <DiffuseProbeGrid/DiffuseProbeGridRayTracingPass.h>
#include <DiffuseProbeGrid/DiffuseProbeGridBlendIrradiancePass.h>
#include <DiffuseProbeGrid/DiffuseProbeGridBlendDistancePass.h>
#include <DiffuseProbeGrid/DiffuseProbeGridBorderUpdatePass.h>
#include <DiffuseProbeGrid/DiffuseProbeGridRelocationPass.h>
#include <DiffuseProbeGrid/DiffuseProbeGridClassificationPass.h>
#include <DiffuseProbeGrid/DiffuseProbeGridRenderPass.h>
#include <DiffuseProbeGrid/DiffuseProbeGridFeatureProcessor.h>
#include <ReflectionScreenSpace/ReflectionScreenSpaceBlurPass.h>
#include <ReflectionScreenSpace/ReflectionScreenSpaceBlurChildPass.h>
#include <ReflectionScreenSpace/ReflectionCopyFrameBufferPass.h>

namespace AZ
{
    namespace Render
    {
        void CommonSystemComponent::Reflect(ReflectContext* context)
        {
            AuxGeomFeatureProcessor::Reflect(context);
            TransformServiceFeatureProcessor::Reflect(context);
            ProjectedShadowFeatureProcessor::Reflect(context);
            SkyBoxFeatureProcessor::Reflect(context);
            UseTextureFunctor::Reflect(context);
            PropertyVisibilityFunctor::Reflect(context);
            DrawListFunctor::Reflect(context);
            SubsurfaceTransmissionParameterFunctor::Reflect(context);
            Transform2DFunctor::Reflect(context);
            MaterialAssignment::Reflect(context);
            MeshFeatureProcessor::Reflect(context);
            ImageBasedLightFeatureProcessor::Reflect(context);
            AcesDisplayMapperFeatureProcessor::Reflect(context);
            DisplayMapperConfigurationDescriptor::Reflect(context);
            DisplayMapperPassData::Reflect(context);
            ConvertEmissiveUnitFunctor::Reflect(context);
            LookupTableAsset::Reflect(context);
            ShaderEnableFunctor::Reflect(context);
            ReflectionProbeFeatureProcessor::Reflect(context);
            DecalTextureArrayFeatureProcessor::Reflect(context);
            SMAAFeatureProcessor::Reflect(context);
            PostProcessFeatureProcessor::Reflect(context);
            ImGuiPassData::Reflect(context);

            LightingPreset::Reflect(context);
            ModelPreset::Reflect(context);
            DiffuseProbeGridFeatureProcessor::Reflect(context);
            RayTracingFeatureProcessor::Reflect(context);

            if (SerializeContext* serialize = azrtti_cast<SerializeContext*>(context))
            {
                serialize->Class<CommonSystemComponent, Component>()
                    ->Version(0);

                if (AZ::EditContext* ec = serialize->GetEditContext())
                {
                    ec->Class<CommonSystemComponent>("CommonSystemComponent", "System Component for common render features")
                        ->ClassElement(Edit::ClassElements::EditorData, "")
                        ->Attribute(Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System", 0xc94d118b))
                        ->Attribute(Edit::Attributes::AutoExpand, true)
                    ;
                }
            }
        }

        void CommonSystemComponent::GetProvidedServices(ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC("CommonService", 0x6398eec4));
        }

        void CommonSystemComponent::GetIncompatibleServices(ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC("CommonService", 0x6398eec4));
        }

        void CommonSystemComponent::GetRequiredServices(ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ_CRC("RPISystem", 0xf2add773));
        }

        void CommonSystemComponent::GetDependentServices(ComponentDescriptor::DependencyArrayType& dependent)
        {
            AZ_UNUSED(dependent);
        }

        void CommonSystemComponent::Init()
        {
        }

        void CommonSystemComponent::Activate()
        {
            AZ::RPI::FeatureProcessorFactory::Get()->RegisterFeatureProcessorWithInterface<AuxGeomFeatureProcessor, RPI::AuxGeomFeatureProcessorInterface>();
            AZ::RPI::FeatureProcessorFactory::Get()->RegisterFeatureProcessorWithInterface<TransformServiceFeatureProcessor, TransformServiceFeatureProcessorInterface>();
            AZ::RPI::FeatureProcessorFactory::Get()->RegisterFeatureProcessorWithInterface<SkyBoxFeatureProcessor, SkyBoxFeatureProcessorInterface>();
            AZ::RPI::FeatureProcessorFactory::Get()->RegisterFeatureProcessorWithInterface<MeshFeatureProcessor, MeshFeatureProcessorInterface>();
            AZ::RPI::FeatureProcessorFactory::Get()->RegisterFeatureProcessorWithInterface<ImageBasedLightFeatureProcessor, ImageBasedLightFeatureProcessorInterface>();
            AZ::RPI::FeatureProcessorFactory::Get()->RegisterFeatureProcessorWithInterface<DecalTextureArrayFeatureProcessor, DecalFeatureProcessorInterface>();
            AZ::RPI::FeatureProcessorFactory::Get()->RegisterFeatureProcessorWithInterface<PostProcessFeatureProcessor, PostProcessFeatureProcessorInterface>();
            AZ::RPI::FeatureProcessorFactory::Get()->RegisterFeatureProcessorWithInterface<AcesDisplayMapperFeatureProcessor, DisplayMapperFeatureProcessorInterface>();
            AZ::RPI::FeatureProcessorFactory::Get()->RegisterFeatureProcessorWithInterface<ProjectedShadowFeatureProcessor, ProjectedShadowFeatureProcessorInterface>();
            AZ::RPI::FeatureProcessorFactory::Get()->RegisterFeatureProcessor<ReflectionProbeFeatureProcessor>();
            AZ::RPI::FeatureProcessorFactory::Get()->RegisterFeatureProcessor<SMAAFeatureProcessor>();
            AZ::RPI::FeatureProcessorFactory::Get()->RegisterFeatureProcessor<DiffuseProbeGridFeatureProcessor>();
            AZ::RPI::FeatureProcessorFactory::Get()->RegisterFeatureProcessor<RayTracingFeatureProcessor>();

            // Add SkyBox pass
            auto* passSystem = RPI::PassSystemInterface::Get();
            AZ_Assert(passSystem, "Cannot get the pass system.");

            // Add DisplayMapper pass
            passSystem->AddPassCreator(Name("AcesOutputTransformLutPass"), &AcesOutputTransformLutPass::Create);
            passSystem->AddPassCreator(Name("AcesOutputTransformPass"), &AcesOutputTransformPass::Create);
            passSystem->AddPassCreator(Name("ApplyShaperLookupTablePass"), &ApplyShaperLookupTablePass::Create);
            passSystem->AddPassCreator(Name("BakeAcesOutputTransformLutPass"), &BakeAcesOutputTransformLutPass::Create);
            passSystem->AddPassCreator(Name("DisplayMapperPass"), &DisplayMapperPass::Create);
            passSystem->AddPassCreator(Name("DisplayMapperFullScreenPass"), &DisplayMapperFullScreenPass::Create);
            passSystem->AddPassCreator(Name("OutputTransformPass"), &OutputTransformPass::Create);
            passSystem->AddPassCreator(Name("EyeAdaptationPass"), &EyeAdaptationPass::Create);
            // Add RenderTexture and LuxCoreTexture pass
#if AZ_TRAIT_LUXCORE_SUPPORTED
            passSystem->AddPassCreator(Name("RenderTexturePass"), &RenderTexturePass::Create);
            passSystem->AddPassCreator(Name("LuxCoreTexturePass"), &LuxCoreTexturePass::Create);
#endif
            passSystem->AddPassCreator(Name("ImGuiPass"), &ImGuiPass::Create);
            passSystem->AddPassCreator(Name("LightCullingPass"), &LightCullingPass::Create);
            passSystem->AddPassCreator(Name("LightCullingRemapPass"), &LightCullingRemap::Create);
            passSystem->AddPassCreator(Name("LightCullingTilePreparePass"), &LightCullingTilePreparePass::Create);
            passSystem->AddPassCreator(Name("BlendColorGradingLutsPass"), &BlendColorGradingLutsPass::Create);
            passSystem->AddPassCreator(Name("LookModificationCompositePass"), &LookModificationCompositePass::Create);
            passSystem->AddPassCreator(Name("LookModificationTransformPass"), &LookModificationPass::Create);
            passSystem->AddPassCreator(Name("SMAAEdgeDetectionPass"), &SMAAEdgeDetectionPass::Create);
            passSystem->AddPassCreator(Name("SMAABlendingWeightCalculationPass"), &SMAABlendingWeightCalculationPass::Create);
            passSystem->AddPassCreator(Name("SMAANeighborhoodBlendingPass"), &SMAANeighborhoodBlendingPass::Create);

            // Add Depth Downsample/Upsample passes
            passSystem->AddPassCreator(Name("DepthUpsamplePass"), &DepthUpsamplePass::Create);

            // Add DepthOfField pass
            passSystem->AddPassCreator(Name("DepthOfFieldCompositePass"), &DepthOfFieldCompositePass::Create);
            passSystem->AddPassCreator(Name("DepthOfFieldBokehBlurPass"), &DepthOfFieldBokehBlurPass::Create);
            passSystem->AddPassCreator(Name("DepthOfFieldMaskPass"), &DepthOfFieldMaskPass::Create);
            passSystem->AddPassCreator(Name("DepthOfFieldParentPass"), &DepthOfFieldParentPass::Create);
            passSystem->AddPassCreator(Name("DepthOfFieldReadBackFocusDepthPass"), &DepthOfFieldReadBackFocusDepthPass::Create);
            passSystem->AddPassCreator(Name("DepthOfFieldWriteFocusDepthFromGpuPass"), &DepthOfFieldWriteFocusDepthFromGpuPass::Create);

            // Add FastDepthAwareBlur passes
            passSystem->AddPassCreator(Name("FastDepthAwareBlurHorPass"), &FastDepthAwareBlurHorPass::Create);
            passSystem->AddPassCreator(Name("FastDepthAwareBlurVerPass"), &FastDepthAwareBlurVerPass::Create);

            // Add SSAO passes
            passSystem->AddPassCreator(Name("SsaoParentPass"), &SsaoParentPass::Create);
            passSystem->AddPassCreator(Name("SsaoComputePass"), &SsaoComputePass::Create);

            // Add Subsurface Scattering pass
            passSystem->AddPassCreator(Name("SubsurfaceScatteringPass"), &RPI::SubsurfaceScatteringPass::Create);

            // Add checkerboard rendering passes
            passSystem->AddPassCreator(Name("CheckerboardPass"), &Render::CheckerboardPass::Create);
            passSystem->AddPassCreator(Name("CheckerboardColorResolvePass"), &Render::CheckerboardColorResolvePass::Create);

            // Add bloom passes
            passSystem->AddPassCreator(Name("BloomParentPass"), &BloomParentPass::Create);
            passSystem->AddPassCreator(Name("BloomDownsamplePass"), &BloomDownsamplePass::Create);
            passSystem->AddPassCreator(Name("BloomBlurPass"), &BloomBlurPass::Create);
            passSystem->AddPassCreator(Name("BloomCompositePass"), &BloomCompositePass::Create);

            // Add Diffuse Global Illumination passes
            passSystem->AddPassCreator(Name("RayTracingAccelerationStructurePass"), &Render::RayTracingAccelerationStructurePass::Create);
            passSystem->AddPassCreator(Name("DiffuseProbeGridRayTracingPass"), &Render::DiffuseProbeGridRayTracingPass::Create);
            passSystem->AddPassCreator(Name("DiffuseProbeGridBlendIrradiancePass"), &Render::DiffuseProbeGridBlendIrradiancePass::Create);
            passSystem->AddPassCreator(Name("DiffuseProbeGridBlendDistancePass"), &Render::DiffuseProbeGridBlendDistancePass::Create);
            passSystem->AddPassCreator(Name("DiffuseProbeGridBorderUpdatePass"), &Render::DiffuseProbeGridBorderUpdatePass::Create);
            passSystem->AddPassCreator(Name("DiffuseProbeGridRelocationPass"), &Render::DiffuseProbeGridRelocationPass::Create);
            passSystem->AddPassCreator(Name("DiffuseProbeGridClassificationPass"), &Render::DiffuseProbeGridClassificationPass::Create);
            passSystem->AddPassCreator(Name("DiffuseProbeGridRenderPass"), &Render::DiffuseProbeGridRenderPass::Create);

            passSystem->AddPassCreator(Name("LuminanceHistogramGeneratorPass"), &LuminanceHistogramGeneratorPass::Create);

            // Deferred Fog
            passSystem->AddPassCreator(Name("DeferredFogPass"), &DeferredFogPass::Create);

            // Add Reflection passes
            passSystem->AddPassCreator(Name("ReflectionScreenSpaceBlurPass"), &Render::ReflectionScreenSpaceBlurPass::Create);
            passSystem->AddPassCreator(Name("ReflectionScreenSpaceBlurChildPass"), &Render::ReflectionScreenSpaceBlurChildPass::Create);
            passSystem->AddPassCreator(Name("ReflectionCopyFrameBufferPass"), &Render::ReflectionCopyFrameBufferPass::Create);

            // setup handler for load pass template mappings
            m_loadTemplatesHandler = RPI::PassSystemInterface::OnReadyLoadTemplatesEvent::Handler([this]() { this->LoadPassTemplateMappings(); });
            RPI::PassSystemInterface::Get()->ConnectEvent(m_loadTemplatesHandler);
        }

        void CommonSystemComponent::Deactivate()
        {
            AZ::RPI::FeatureProcessorFactory::Get()->UnregisterFeatureProcessor<RayTracingFeatureProcessor>();
            AZ::RPI::FeatureProcessorFactory::Get()->UnregisterFeatureProcessor<DiffuseProbeGridFeatureProcessor>();
            AZ::RPI::FeatureProcessorFactory::Get()->UnregisterFeatureProcessor<SMAAFeatureProcessor>();
            AZ::RPI::FeatureProcessorFactory::Get()->UnregisterFeatureProcessor<ReflectionProbeFeatureProcessor>();
            AZ::RPI::FeatureProcessorFactory::Get()->UnregisterFeatureProcessor<ProjectedShadowFeatureProcessor>();
            AZ::RPI::FeatureProcessorFactory::Get()->UnregisterFeatureProcessor<AcesDisplayMapperFeatureProcessor>();
            AZ::RPI::FeatureProcessorFactory::Get()->UnregisterFeatureProcessor<PostProcessFeatureProcessor>();
            AZ::RPI::FeatureProcessorFactory::Get()->UnregisterFeatureProcessor<DecalTextureArrayFeatureProcessor>();
            AZ::RPI::FeatureProcessorFactory::Get()->UnregisterFeatureProcessor<ImageBasedLightFeatureProcessor>();
            AZ::RPI::FeatureProcessorFactory::Get()->UnregisterFeatureProcessor<MeshFeatureProcessor>();
            AZ::RPI::FeatureProcessorFactory::Get()->UnregisterFeatureProcessor<SkyBoxFeatureProcessor>();
            AZ::RPI::FeatureProcessorFactory::Get()->UnregisterFeatureProcessor<TransformServiceFeatureProcessor>();
            AZ::RPI::FeatureProcessorFactory::Get()->UnregisterFeatureProcessor<AuxGeomFeatureProcessor>();
        }

        void CommonSystemComponent::LoadPassTemplateMappings()
        {
            const char* passTemplatesFile = "Passes/PassTemplates.azasset";
            RPI::PassSystemInterface::Get()->LoadPassTemplateMappings(passTemplatesFile);
        }

    } // namespace Render
} // namespace AZ
