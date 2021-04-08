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

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/Module/Module.h>

#include <CommonFeaturesSystemComponent.h>
#include <CoreLights/AreaLightComponent.h>
#include <CoreLights/DirectionalLightComponent.h>
#include <CoreLights/PointLightComponent.h>
#include <CoreLights/SpotLightComponent.h>
#include <Decals/DecalComponent.h>
#include <DiffuseProbeGrid/DiffuseProbeGridComponent.h>
#include <Grid/GridComponent.h>
#include <ImageBasedLights/ImageBasedLightComponent.h>
#include <Material/MaterialComponent.h>
#include <Mesh/MeshComponent.h>
#include <ReflectionProbe/ReflectionProbeComponent.h>
#include <PostProcess/PostFxLayerComponent.h>
#include <PostProcess/Bloom/BloomComponent.h>
#include <PostProcess/DepthOfField/DepthOfFieldComponent.h>
#include <PostProcess/DisplayMapper/DisplayMapperComponent.h>
#include <PostProcess/ExposureControl/ExposureControlComponent.h>
#include <PostProcess/Ssao/SsaoComponent.h>
#include <PostProcess/LookModification/LookModificationComponent.h>
#include <PostProcess/RadiusWeightModifier/RadiusWeightModifierComponent.h>
#include <PostProcess/ShapeWeightModifier/ShapeWeightModifierComponent.h>
#include <PostProcess/GradientWeightModifier/GradientWeightModifierComponent.h>
#include <ScreenSpace/DeferredFogComponent.h>
#include <SkyBox/HDRiSkyboxComponent.h>
#include <SkyBox/PhysicalSkyComponent.h>
#include <Scripting/EntityReferenceComponent.h>

#ifdef ATOMLYINTEGRATION_FEATURE_COMMON_EDITOR
#include <EditorCommonFeaturesSystemComponent.h>
#include <PostProcess/EditorPostFxSystemComponent.h>
#include <CoreLights/EditorAreaLightComponent.h>
#include <CoreLights/EditorDirectionalLightComponent.h>
#include <CoreLights/EditorPointLightComponent.h>
#include <CoreLights/EditorSpotLightComponent.h>
#include <Decals/EditorDecalComponent.h>
#include <DiffuseProbeGrid/EditorDiffuseProbeGridComponent.h>
#include <Grid/EditorGridComponent.h>
#include <ImageBasedLights/EditorImageBasedLightComponent.h>
#include <Material/EditorMaterialComponent.h>
#include <Material/EditorMaterialSystemComponent.h>
#include <Mesh/EditorMeshComponent.h>
#include <Mesh/EditorMeshSystemComponent.h>
#include <ReflectionProbe/EditorReflectionProbeComponent.h>
#include <PostProcess/EditorPostFxLayerComponent.h>
#include <PostProcess/Bloom/EditorBloomComponent.h>
#include <PostProcess/DepthOfField/EditorDepthOfFieldComponent.h>
#include <PostProcess/DisplayMapper/EditorDisplayMapperComponent.h>
#include <PostProcess/ExposureControl/EditorExposureControlComponent.h>
#include <PostProcess/Ssao/EditorSsaoComponent.h>
#include <PostProcess/LookModification/EditorLookModificationComponent.h>
#include <PostProcess/RadiusWeightModifier/EditorRadiusWeightModifierComponent.h>
#include <PostProcess/ShapeWeightModifier/EditorShapeWeightModifierComponent.h>
#include <PostProcess/GradientWeightModifier/EditorGradientWeightModifierComponent.h>
#include <ScreenSpace/EditorDeferredFogComponent.h>
#include <SkyBox/EditorHDRiSkyboxComponent.h>
#include <SkyBox/EditorPhysicalSkyComponent.h>
#include <Scripting/EditorEntityReferenceComponent.h>
#endif

namespace AZ
{
    namespace Render
    {
        class AtomLyIntegrationCommonFeaturesModule
            : public AZ::Module
        {
        public:
            AZ_RTTI(AtomLyIntegrationCommonFeaturesModule, "{E6FF4862-9355-4B23-AE00-B936F0C6E6C9}", AZ::Module);
            AZ_CLASS_ALLOCATOR(AtomLyIntegrationCommonFeaturesModule, AZ::SystemAllocator, 0);

            AtomLyIntegrationCommonFeaturesModule()
                : AZ::Module()
            {
                m_descriptors.insert(m_descriptors.end(), {
                        AtomLyIntegrationCommonFeaturesSystemComponent::CreateDescriptor(),
                        AreaLightComponent::CreateDescriptor(),
                        DecalComponent::CreateDescriptor(),
                        DirectionalLightComponent::CreateDescriptor(),
                        BloomComponent::CreateDescriptor(),
                        DisplayMapperComponent::CreateDescriptor(),
                        DepthOfFieldComponent::CreateDescriptor(),
                        ExposureControlComponent::CreateDescriptor(),
                        SsaoComponent::CreateDescriptor(),
                        LookModificationComponent::CreateDescriptor(),
                        GridComponent::CreateDescriptor(),
                        HDRiSkyboxComponent::CreateDescriptor(),
                        ImageBasedLightComponent::CreateDescriptor(),
                        MaterialComponent::CreateDescriptor(),
                        MeshComponent::CreateDescriptor(),
                        PhysicalSkyComponent::CreateDescriptor(),
                        PointLightComponent::CreateDescriptor(),
                        PostFxLayerComponent::CreateDescriptor(),
                        ReflectionProbeComponent::CreateDescriptor(),
                        SpotLightComponent::CreateDescriptor(),
                        RadiusWeightModifierComponent::CreateDescriptor(),
                        ShapeWeightModifierComponent::CreateDescriptor(),
                        EntityReferenceComponent::CreateDescriptor(),
                        GradientWeightModifierComponent::CreateDescriptor(),
                        DiffuseProbeGridComponent::CreateDescriptor(),
                        DeferredFogComponent::CreateDescriptor(),

#ifdef ATOMLYINTEGRATION_FEATURE_COMMON_EDITOR
                        EditorAreaLightComponent::CreateDescriptor(),
                        EditorCommonFeaturesSystemComponent::CreateDescriptor(),
                        EditorPostFxSystemComponent::CreateDescriptor(),
                        EditorDecalComponent::CreateDescriptor(),
                        EditorDirectionalLightComponent::CreateDescriptor(),
                        EditorBloomComponent::CreateDescriptor(),
                        EditorDepthOfFieldComponent::CreateDescriptor(),
                        EditorDisplayMapperComponent::CreateDescriptor(),
                        EditorExposureControlComponent::CreateDescriptor(),
                        EditorSsaoComponent::CreateDescriptor(),
                        EditorLookModificationComponent::CreateDescriptor(),
                        EditorGridComponent::CreateDescriptor(),
                        EditorHDRiSkyboxComponent::CreateDescriptor(),
                        EditorImageBasedLightComponent::CreateDescriptor(),
                        EditorMaterialComponent::CreateDescriptor(),
                        EditorMaterialSystemComponent::CreateDescriptor(),
                        EditorMeshSystemComponent::CreateDescriptor(),
                        EditorMeshComponent::CreateDescriptor(),
                        EditorPhysicalSkyComponent::CreateDescriptor(),
                        EditorPointLightComponent::CreateDescriptor(),
                        EditorPostFxLayerComponent::CreateDescriptor(),
                        EditorReflectionProbeComponent::CreateDescriptor(),
                        EditorSpotLightComponent::CreateDescriptor(),
                        EditorRadiusWeightModifierComponent::CreateDescriptor(),
                        EditorShapeWeightModifierComponent::CreateDescriptor(),
                        EditorEntityReferenceComponent::CreateDescriptor(),
                        EditorGradientWeightModifierComponent::CreateDescriptor(),
                        EditorDiffuseProbeGridComponent::CreateDescriptor(),
                        EditorDeferredFogComponent::CreateDescriptor(),
#endif
                    });
            }

            AZ::ComponentTypeList GetRequiredSystemComponents() const override
            {
                return AZ::ComponentTypeList{
                    azrtti_typeid<AtomLyIntegrationCommonFeaturesSystemComponent>(),
#ifdef ATOMLYINTEGRATION_FEATURE_COMMON_EDITOR
                    azrtti_typeid<EditorMaterialSystemComponent>(),
                    azrtti_typeid<EditorMeshSystemComponent>(),
                    azrtti_typeid<EditorCommonFeaturesSystemComponent>(),
                    azrtti_typeid<EditorPostFxSystemComponent>(),
#endif
                };
            }
        };
    } // namespace Render
} // namespace AZ

// DO NOT MODIFY THIS LINE UNLESS YOU RENAME THE GEM
// The first parameter should be GemName_GemIdLower
// The second should be the fully qualified name of the class above
AZ_DECLARE_MODULE_CLASS(Gem_AtomLyIntegration_CommonFeatures, AZ::Render::AtomLyIntegrationCommonFeaturesModule)
