/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <CommonSystemComponent.h>
#include <Material/UseTextureFunctor.h>
#include <Material/SubsurfaceTransmissionParameterFunctor.h>
#include <Material/Transform2DFunctor.h>
#include <Atom/Feature/Material/ConvertEmissiveUnitFunctor.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/EditContextConstants.inl>

#include <LookupTable/LookupTableAsset.h>
#include <ReflectionProbe/ReflectionProbeFeatureProcessor.h>
#include <SpecularReflections/SpecularReflectionsFeatureProcessor.h>
#include <TransformService/TransformServiceFeatureProcessor.h>
#include <CubeMapCapture/CubeMapCaptureFeatureProcessor.h>

#include <ImageBasedLights/ImageBasedLightFeatureProcessor.h>
#include <DisplayMapper/AcesOutputTransformLutPass.h>
#include <DisplayMapper/AcesOutputTransformPass.h>
#include <DisplayMapper/ApplyShaperLookupTablePass.h>
#include <DisplayMapper/BakeAcesOutputTransformLutPass.h>
#include <DisplayMapper/DisplayMapperPass.h>
#include <DisplayMapper/DisplayMapperFullScreenPass.h>
#include <DisplayMapper/OutputTransformPass.h>
#include <Atom/Feature/DisplayMapper/DisplayMapperConfigurationDescriptor.h>
#include <Atom/Feature/ACES/AcesDisplayMapperFeatureProcessor.h>
#include <Atom/Feature/LightingChannel/LightingChannelConfiguration.h>
#include <Atom/Feature/RayTracing/RayTracingPass.h>
#include <Atom/Feature/RayTracing/RayTracingPassData.h>
#include <Atom/Feature/SplashScreen/SplashScreenSettings.h>
#include <Atom/Feature/Utils/LightingPreset.h>
#include <Atom/Feature/Utils/ModelPreset.h>
#include <Atom/Feature/SkyBox/SkyBoxFogSettings.h>
#include <AuxGeom/AuxGeomFeatureProcessor.h>
#include <ColorGrading/LutGenerationPass.h>
#include <Debug/RayTracingDebugFeatureProcessor.h>
#include <Debug/RenderDebugFeatureProcessor.h>
#include <Mesh/MeshFeatureProcessor.h>
#include <Silhouette/SilhouetteFeatureProcessor.h>
#include <Silhouette/SilhouetteCompositePass.h>
#include <PostProcess/PostProcessFeatureProcessor.h>
#include <PostProcessing/BlendColorGradingLutsPass.h>
#include <PostProcessing/BloomParentPass.h>
#include <PostProcessing/HDRColorGradingPass.h>
#include <PostProcessing/DepthUpsamplePass.h>
#include <PostProcessing/DepthOfFieldCompositePass.h>
#include <PostProcessing/DepthOfFieldBokehBlurPass.h>
#include <PostProcessing/DepthOfFieldMaskPass.h>
#include <PostProcessing/DepthOfFieldParentPass.h>
#include <PostProcessing/DepthOfFieldReadBackFocusDepthPass.h>
#include <PostProcessing/DepthOfFieldWriteFocusDepthFromGpuPass.h>
#include <PostProcessing/NewDepthOfFieldPasses.h>
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
#include <PostProcessing/TaaPass.h>
#include <PostProcessing/BloomDownsamplePass.h>
#include <PostProcessing/BloomBlurPass.h>
#include <PostProcessing/BloomCompositePass.h>
#include <PostProcessing/ChromaticAberrationPass.h>
#include <PostProcessing/PaniniProjectionPass.h>
#include <PostProcessing/FilmGrainPass.h>
#include <PostProcessing/WhiteBalancePass.h>
#include <PostProcessing/VignettePass.h>
#include <RayTracing/RayTracingFeatureProcessor.h>
#include <ScreenSpace/DeferredFogPass.h>
#include <Shadows/ProjectedShadowFeatureProcessor.h>
#include <SkyAtmosphere/SkyAtmosphereFeatureProcessor.h>
#include <SkyAtmosphere/SkyAtmosphereParentPass.h>
#include <SkyBox/SkyBoxFeatureProcessor.h>
#include <SplashScreen/SplashScreenFeatureProcessor.h>
#include <SplashScreen/SplashScreenPass.h>

#include <Atom/RPI.Public/Pass/PassSystemInterface.h>

#include <Checkerboard/CheckerboardPass.h>
#include <Checkerboard/CheckerboardColorResolvePass.h>

#include <CoreLights/LightCullingTilePreparePass.h>
#include <CoreLights/LightCullingPass.h>
#include <Shadows/FullscreenShadowPass.h>
#include <CoreLights/LightCullingRemap.h>
#include <Decals/DecalTextureArrayFeatureProcessor.h>
#include <ImGui/ImGuiPass.h>

#include <RayTracing/RayTracingAccelerationStructurePass.h>
#include <ReflectionScreenSpace/ReflectionScreenSpacePass.h>
#include <ReflectionScreenSpace/ReflectionScreenSpaceTracePass.h>
#include <ReflectionScreenSpace/ReflectionScreenSpaceDownsampleDepthLinearPass.h>
#include <ReflectionScreenSpace/ReflectionScreenSpaceDownsampleDepthLinearChildPass.h>
#include <ReflectionScreenSpace/ReflectionScreenSpaceBlurPass.h>
#include <ReflectionScreenSpace/ReflectionScreenSpaceBlurChildPass.h>
#include <ReflectionScreenSpace/ReflectionScreenSpaceFilterPass.h>
#include <ReflectionScreenSpace/ReflectionScreenSpaceCompositePass.h>
#include <ReflectionScreenSpace/ReflectionCopyFrameBufferPass.h>
#include <OcclusionCullingPlane/OcclusionCullingPlaneFeatureProcessor.h>
#include <Mesh/ModelReloaderSystem.h>

namespace AZ
{
    namespace Render
    {
        CommonSystemComponent::CommonSystemComponent() = default;
        CommonSystemComponent::~CommonSystemComponent() = default;

        void CommonSystemComponent::Reflect(ReflectContext* context)
        {
            AuxGeomFeatureProcessor::Reflect(context);
            TransformServiceFeatureProcessor::Reflect(context);
            ProjectedShadowFeatureProcessor::Reflect(context);
            SkyAtmosphereFeatureProcessor::Reflect(context);
            SkyBoxFeatureProcessor::Reflect(context);
            SkyBoxFogSettings::Reflect(context);
            UseTextureFunctor::Reflect(context);
            SubsurfaceTransmissionParameterFunctor::Reflect(context);
            Transform2DFunctor::Reflect(context);
            MeshFeatureProcessor::Reflect(context);
            ImageBasedLightFeatureProcessor::Reflect(context);
            AcesDisplayMapperFeatureProcessor::Reflect(context);
            DisplayMapperConfigurationDescriptor::Reflect(context);
            DisplayMapperPassData::Reflect(context);
            ConvertEmissiveUnitFunctor::Reflect(context);
            LookupTableAsset::Reflect(context);
            ReflectionProbeFeatureProcessor::Reflect(context);
            SpecularReflectionsFeatureProcessor::Reflect(context);
            CubeMapCaptureFeatureProcessor::Reflect(context);
            DecalTextureArrayFeatureProcessor::Reflect(context);
            SMAAFeatureProcessor::Reflect(context);
            SilhouetteFeatureProcessor::Reflect(context);
            PostProcessFeatureProcessor::Reflect(context);
            ImGuiPassData::Reflect(context);
            RayTracingPassData::Reflect(context);
            TaaPassData::Reflect(context);
            RayTracingDebugFeatureProcessor::Reflect(context);
            RenderDebugFeatureProcessor::Reflect(context);
            SplashScreenFeatureProcessor::Reflect(context);
            SplashScreenSettings::Reflect(context);

            LightingPreset::Reflect(context);
            ModelPreset::Reflect(context);
            RayTracingFeatureProcessor::Reflect(context);
            OcclusionCullingPlaneFeatureProcessor::Reflect(context);
            LightingChannelConfiguration::Reflect(context);

            if (SerializeContext* serialize = azrtti_cast<SerializeContext*>(context))
            {
                serialize->Class<CommonSystemComponent, Component>()
                    ->Version(0);

                if (AZ::EditContext* ec = serialize->GetEditContext())
                {
                    ec->Class<CommonSystemComponent>("CommonSystemComponent", "System Component for common render features")
                        ->ClassElement(Edit::ClassElements::EditorData, "")
                        ->Attribute(Edit::Attributes::AutoExpand, true)
                    ;
                }
            }
        }

        void CommonSystemComponent::GetProvidedServices(ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC_CE("CommonService"));
        }

        void CommonSystemComponent::GetIncompatibleServices(ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC_CE("CommonService"));
        }

        void CommonSystemComponent::GetRequiredServices(ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ_CRC_CE("RPISystem"));
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
            AZ::ApplicationTypeQuery appType;
            ComponentApplicationBus::Broadcast(&AZ::ComponentApplicationBus::Events::QueryApplicationType, appType);
            if (!appType.IsHeadless())
            {
                AZ::RPI::FeatureProcessorFactory::Get()->RegisterFeatureProcessorWithInterface<SkyAtmosphereFeatureProcessor, SkyAtmosphereFeatureProcessorInterface>();
                AZ::RPI::FeatureProcessorFactory::Get()->RegisterFeatureProcessorWithInterface<SkyBoxFeatureProcessor, SkyBoxFeatureProcessorInterface>();
                AZ::RPI::FeatureProcessorFactory::Get()->RegisterFeatureProcessorWithInterface<ImageBasedLightFeatureProcessor, ImageBasedLightFeatureProcessorInterface>();
                AZ::RPI::FeatureProcessorFactory::Get()->RegisterFeatureProcessorWithInterface<DecalTextureArrayFeatureProcessor, DecalFeatureProcessorInterface>();
                AZ::RPI::FeatureProcessorFactory::Get()->RegisterFeatureProcessorWithInterface<PostProcessFeatureProcessor, PostProcessFeatureProcessorInterface>();
                AZ::RPI::FeatureProcessorFactory::Get()->RegisterFeatureProcessorWithInterface<AcesDisplayMapperFeatureProcessor, DisplayMapperFeatureProcessorInterface>();
                AZ::RPI::FeatureProcessorFactory::Get()->RegisterFeatureProcessorWithInterface<ProjectedShadowFeatureProcessor, ProjectedShadowFeatureProcessorInterface>();
                AZ::RPI::FeatureProcessorFactory::Get()->RegisterFeatureProcessorWithInterface<RayTracingDebugFeatureProcessor, RayTracingDebugFeatureProcessorInterface>();
                AZ::RPI::FeatureProcessorFactory::Get()->RegisterFeatureProcessorWithInterface<RenderDebugFeatureProcessor, RenderDebugFeatureProcessorInterface>();
                AZ::RPI::FeatureProcessorFactory::Get()->RegisterFeatureProcessorWithInterface<ReflectionProbeFeatureProcessor, ReflectionProbeFeatureProcessorInterface>();
                AZ::RPI::FeatureProcessorFactory::Get()->RegisterFeatureProcessorWithInterface<SpecularReflectionsFeatureProcessor, SpecularReflectionsFeatureProcessorInterface>();
                AZ::RPI::FeatureProcessorFactory::Get()->RegisterFeatureProcessorWithInterface<CubeMapCaptureFeatureProcessor, CubeMapCaptureFeatureProcessorInterface>();
                AZ::RPI::FeatureProcessorFactory::Get()->RegisterFeatureProcessor<SMAAFeatureProcessor>();
                AZ::RPI::FeatureProcessorFactory::Get()->RegisterFeatureProcessorWithInterface<RayTracingFeatureProcessor, RayTracingFeatureProcessorInterface>();
                AZ::RPI::FeatureProcessorFactory::Get()->RegisterFeatureProcessorWithInterface<OcclusionCullingPlaneFeatureProcessor, OcclusionCullingPlaneFeatureProcessorInterface>();
                AZ::RPI::FeatureProcessorFactory::Get()->RegisterFeatureProcessor<SplashScreenFeatureProcessor>();
                AZ::RPI::FeatureProcessorFactory::Get()->RegisterFeatureProcessor<SilhouetteFeatureProcessor>();
            }

            AZ::RPI::FeatureProcessorFactory::Get()->RegisterFeatureProcessorWithInterface<AuxGeomFeatureProcessor, RPI::AuxGeomFeatureProcessorInterface>();
            AZ::RPI::FeatureProcessorFactory::Get()->RegisterFeatureProcessorWithInterface<TransformServiceFeatureProcessor, TransformServiceFeatureProcessorInterface>();
            AZ::RPI::FeatureProcessorFactory::Get()->RegisterFeatureProcessorWithInterface<MeshFeatureProcessor, MeshFeatureProcessorInterface>();

            auto* passSystem = RPI::PassSystemInterface::Get();
            AZ_Assert(passSystem, "Cannot get the pass system.");

            // Add Sky Atmosphere Parent pass
            passSystem->AddPassCreator(Name("SkyAtmosphereParentPass"), &SkyAtmosphereParentPass::Create);

            // Add DisplayMapper pass
            passSystem->AddPassCreator(Name("AcesOutputTransformLutPass"), &AcesOutputTransformLutPass::Create);
            passSystem->AddPassCreator(Name("AcesOutputTransformPass"), &AcesOutputTransformPass::Create);
            passSystem->AddPassCreator(Name("ApplyShaperLookupTablePass"), &ApplyShaperLookupTablePass::Create);
            passSystem->AddPassCreator(Name("BakeAcesOutputTransformLutPass"), &BakeAcesOutputTransformLutPass::Create);
            passSystem->AddPassCreator(Name("DisplayMapperPass"), &DisplayMapperPass::Create);
            passSystem->AddPassCreator(Name("DisplayMapperFullScreenPass"), &DisplayMapperFullScreenPass::Create);
            passSystem->AddPassCreator(Name("OutputTransformPass"), &OutputTransformPass::Create);
            passSystem->AddPassCreator(Name("EyeAdaptationPass"), &EyeAdaptationPass::Create);
            passSystem->AddPassCreator(Name("ImGuiPass"), &ImGuiPass::Create);
            passSystem->AddPassCreator(Name("LightCullingPass"), &LightCullingPass::Create);
            passSystem->AddPassCreator(Name("LightCullingRemapPass"), &LightCullingRemap::Create);
            passSystem->AddPassCreator(Name("LightCullingTilePreparePass"), &LightCullingTilePreparePass::Create);
            passSystem->AddPassCreator(Name("BlendColorGradingLutsPass"), &BlendColorGradingLutsPass::Create);
            passSystem->AddPassCreator(Name("HDRColorGradingPass"), &HDRColorGradingPass::Create);
            passSystem->AddPassCreator(Name("FullscreenShadowPass"), &FullscreenShadowPass::Create);
            passSystem->AddPassCreator(Name("LookModificationCompositePass"), &LookModificationCompositePass::Create);
            passSystem->AddPassCreator(Name("LookModificationTransformPass"), &LookModificationPass::Create);
            passSystem->AddPassCreator(Name("LutGenerationPass"), &LutGenerationPass::Create);
            passSystem->AddPassCreator(Name("SMAAEdgeDetectionPass"), &SMAAEdgeDetectionPass::Create);
            passSystem->AddPassCreator(Name("SMAABlendingWeightCalculationPass"), &SMAABlendingWeightCalculationPass::Create);
            passSystem->AddPassCreator(Name("SMAANeighborhoodBlendingPass"), &SMAANeighborhoodBlendingPass::Create);

            // Add Depth Downsample/Upsample passes
            passSystem->AddPassCreator(Name("DepthUpsamplePass"), &DepthUpsamplePass::Create);
            
            // Add Taa Pass
            passSystem->AddPassCreator(Name("TaaPass"), &TaaPass::Create);

            // Add DepthOfField pass
            passSystem->AddPassCreator(Name("DepthOfFieldCompositePass"), &DepthOfFieldCompositePass::Create);
            passSystem->AddPassCreator(Name("DepthOfFieldBokehBlurPass"), &DepthOfFieldBokehBlurPass::Create);
            passSystem->AddPassCreator(Name("DepthOfFieldMaskPass"), &DepthOfFieldMaskPass::Create);
            passSystem->AddPassCreator(Name("DepthOfFieldParentPass"), &DepthOfFieldParentPass::Create);
            passSystem->AddPassCreator(Name("DepthOfFieldReadBackFocusDepthPass"), &DepthOfFieldReadBackFocusDepthPass::Create);
            passSystem->AddPassCreator(Name("DepthOfFieldWriteFocusDepthFromGpuPass"), &DepthOfFieldWriteFocusDepthFromGpuPass::Create);

            passSystem->AddPassCreator(Name("NewDepthOfFieldParentPass"), &NewDepthOfFieldParentPass::Create);
            passSystem->AddPassCreator(Name("NewDepthOfFieldTileReducePass"), &NewDepthOfFieldTileReducePass::Create);
            passSystem->AddPassCreator(Name("NewDepthOfFieldFilterPass"), &NewDepthOfFieldFilterPass::Create);

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

            // Add Chromatic Aberration
            passSystem->AddPassCreator(Name("ChromaticAberrationPass"), &ChromaticAberrationPass::Create);

            // Add Panini Projection
            passSystem->AddPassCreator(Name("PaniniProjectionPass"), &PaniniProjectionPass::Create);

            // Add Film Grain
            passSystem->AddPassCreator(Name("FilmGrainPass"), &FilmGrainPass::Create);

            // Add White Balance pass
            passSystem->AddPassCreator(Name("WhiteBalancePass"), &WhiteBalancePass::Create);

            // Add Vignette
            passSystem->AddPassCreator(Name("VignettePass"), &VignettePass::Create);

            // Add Luminance Histogram pass
            passSystem->AddPassCreator(Name("LuminanceHistogramGeneratorPass"), &LuminanceHistogramGeneratorPass::Create);

            // Deferred Fog
            passSystem->AddPassCreator(Name("DeferredFogPass"), &DeferredFogPass::Create);

            // Add SilhouetteComposite pass
            passSystem->AddPassCreator(Name("SilhouetteCompositePass"), &SilhouetteCompositePass::Create);

            // Add Reflection passes
            passSystem->AddPassCreator(Name("ReflectionScreenSpacePass"), &Render::ReflectionScreenSpacePass::Create);
            passSystem->AddPassCreator(Name("ReflectionScreenSpaceTracePass"), &Render::ReflectionScreenSpaceTracePass::Create);
            passSystem->AddPassCreator(Name("ReflectionScreenSpaceDownsampleDepthLinearPass"), &Render::ReflectionScreenSpaceDownsampleDepthLinearPass::Create);
            passSystem->AddPassCreator(Name("ReflectionScreenSpaceDownsampleDepthLinearChildPass"), &Render::ReflectionScreenSpaceDownsampleDepthLinearChildPass::Create);
            passSystem->AddPassCreator(Name("ReflectionScreenSpaceBlurPass"), &Render::ReflectionScreenSpaceBlurPass::Create);
            passSystem->AddPassCreator(Name("ReflectionScreenSpaceBlurChildPass"), &Render::ReflectionScreenSpaceBlurChildPass::Create);
            passSystem->AddPassCreator(Name("ReflectionScreenSpaceFilterPass"), &Render::ReflectionScreenSpaceFilterPass::Create);
            passSystem->AddPassCreator(Name("ReflectionScreenSpaceCompositePass"), &Render::ReflectionScreenSpaceCompositePass::Create);
            passSystem->AddPassCreator(Name("ReflectionCopyFrameBufferPass"), &Render::ReflectionCopyFrameBufferPass::Create);

            // Add RayTracing passes
            passSystem->AddPassCreator(Name("RayTracingAccelerationStructurePass"), &Render::RayTracingAccelerationStructurePass::Create);
            passSystem->AddPassCreator(Name("RayTracingPass"), &Render::RayTracingPass::Create);

            // Add splash screen pass
            passSystem->AddPassCreator(Name("SplashScreenPass"), &Render::SplashScreenPass::Create);

            // setup handler for load pass template mappings
            m_loadTemplatesHandler = RPI::PassSystemInterface::OnReadyLoadTemplatesEvent::Handler([this]() { this->LoadPassTemplateMappings(); });
            RPI::PassSystemInterface::Get()->ConnectEvent(m_loadTemplatesHandler);
            
            m_modelReloaderSystem = AZStd::make_unique<ModelReloaderSystem>();
        }

        void CommonSystemComponent::Deactivate()
        {
            m_modelReloaderSystem.reset();
            m_loadTemplatesHandler.Disconnect();

            AZ::ApplicationTypeQuery appType;
            ComponentApplicationBus::Broadcast(&AZ::ComponentApplicationBus::Events::QueryApplicationType, appType);
            if (!appType.IsHeadless())
            {
                AZ::RPI::FeatureProcessorFactory::Get()->UnregisterFeatureProcessor<RayTracingFeatureProcessor>();
                AZ::RPI::FeatureProcessorFactory::Get()->UnregisterFeatureProcessor<SMAAFeatureProcessor>();
                AZ::RPI::FeatureProcessorFactory::Get()->UnregisterFeatureProcessor<ReflectionProbeFeatureProcessor>();
                AZ::RPI::FeatureProcessorFactory::Get()->UnregisterFeatureProcessor<SpecularReflectionsFeatureProcessor>();
                AZ::RPI::FeatureProcessorFactory::Get()->UnregisterFeatureProcessor<CubeMapCaptureFeatureProcessor>();
                AZ::RPI::FeatureProcessorFactory::Get()->UnregisterFeatureProcessor<ProjectedShadowFeatureProcessor>();
                AZ::RPI::FeatureProcessorFactory::Get()->UnregisterFeatureProcessor<AcesDisplayMapperFeatureProcessor>();
                AZ::RPI::FeatureProcessorFactory::Get()->UnregisterFeatureProcessor<PostProcessFeatureProcessor>();
                AZ::RPI::FeatureProcessorFactory::Get()->UnregisterFeatureProcessor<DecalTextureArrayFeatureProcessor>();
                AZ::RPI::FeatureProcessorFactory::Get()->UnregisterFeatureProcessor<ImageBasedLightFeatureProcessor>();
                AZ::RPI::FeatureProcessorFactory::Get()->UnregisterFeatureProcessor<SkyBoxFeatureProcessor>();
                AZ::RPI::FeatureProcessorFactory::Get()->UnregisterFeatureProcessor<SkyAtmosphereFeatureProcessor>();
                AZ::RPI::FeatureProcessorFactory::Get()->UnregisterFeatureProcessor<OcclusionCullingPlaneFeatureProcessor>();
                AZ::RPI::FeatureProcessorFactory::Get()->UnregisterFeatureProcessor<RayTracingDebugFeatureProcessor>();
                AZ::RPI::FeatureProcessorFactory::Get()->UnregisterFeatureProcessor<RenderDebugFeatureProcessor>();
                AZ::RPI::FeatureProcessorFactory::Get()->UnregisterFeatureProcessor<SplashScreenFeatureProcessor>();
                AZ::RPI::FeatureProcessorFactory::Get()->UnregisterFeatureProcessor<SilhouetteFeatureProcessor>();
            }

            AZ::RPI::FeatureProcessorFactory::Get()->UnregisterFeatureProcessor<MeshFeatureProcessor>();
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
