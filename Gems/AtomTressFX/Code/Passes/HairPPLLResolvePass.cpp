/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Public/Pass/PassUtils.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RHI/RHISystemInterface.h>
#include <Atom/RPI.Public/Shader/ShaderSystem.h>
#include <Atom/RPI.Public/RPIUtils.h>
#include <Atom/RPI.Public/Scene.h>

#include <Atom/RPI.Reflect/Pass/PassTemplate.h>
#include <Atom/RPI.Reflect/Shader/ShaderAsset.h>

#include <Passes/HairPPLLResolvePass.h>
#include <Rendering/HairFeatureProcessor.h>
#include <Rendering/HairLightingModels.h>

namespace AZ
{
    namespace Render
    {
        namespace Hair
        {

            HairPPLLResolvePass::HairPPLLResolvePass(const RPI::PassDescriptor& descriptor)
                : RPI::FullscreenTrianglePass(descriptor)
            {
            }

            void HairPPLLResolvePass::UpdateGlobalShaderOptions()
            {
                RPI::ShaderOptionGroup shaderOption = m_shader->CreateShaderOptionGroup();

                // Shader Options
                m_featureProcessor->GetHairGlobalSettings(m_hairGlobalSettings);
                shaderOption.SetValue(AZ::Name("o_enableShadows"), AZ::RPI::ShaderOptionValue{ m_hairGlobalSettings.m_enableShadows });
                shaderOption.SetValue(AZ::Name("o_enableDirectionalLights"), AZ::RPI::ShaderOptionValue{ m_hairGlobalSettings.m_enableDirectionalLights });
                shaderOption.SetValue(AZ::Name("o_enablePunctualLights"), AZ::RPI::ShaderOptionValue{ m_hairGlobalSettings.m_enablePunctualLights });
                shaderOption.SetValue(AZ::Name("o_enableAreaLights"), AZ::RPI::ShaderOptionValue{ m_hairGlobalSettings.m_enableAreaLights });
                shaderOption.SetValue(AZ::Name("o_enableIBL"), AZ::RPI::ShaderOptionValue{ m_hairGlobalSettings.m_enableIBL });
                shaderOption.SetValue(AZ::Name("o_hairLightingModel"), AZ::Name{ "HairLightingModel::" + AZStd::string(HairLightingModelNamespace::ToString(m_hairGlobalSettings.m_hairLightingModel)) });
                shaderOption.SetValue(AZ::Name("o_enableMarschner_R"), AZ::RPI::ShaderOptionValue{ m_hairGlobalSettings.m_enableMarschner_R });
                shaderOption.SetValue(AZ::Name("o_enableMarschner_TRT"), AZ::RPI::ShaderOptionValue{ m_hairGlobalSettings.m_enableMarschner_TRT });
                shaderOption.SetValue(AZ::Name("o_enableMarschner_TT"), AZ::RPI::ShaderOptionValue{ m_hairGlobalSettings.m_enableMarschner_TT });
                shaderOption.SetValue(AZ::Name("o_enableDiffuseLobe"), AZ::RPI::ShaderOptionValue{ m_hairGlobalSettings.m_enableDiffuseLobe });
                shaderOption.SetValue(AZ::Name("o_enableSpecularLobe"), AZ::RPI::ShaderOptionValue{ m_hairGlobalSettings.m_enableSpecularLobe });
                shaderOption.SetValue(AZ::Name("o_enableTransmittanceLobe"), AZ::RPI::ShaderOptionValue{ m_hairGlobalSettings.m_enableTransmittanceLobe });

                m_shaderOptions = shaderOption.GetShaderVariantKeyFallbackValue();
            }

            RPI::Ptr<HairPPLLResolvePass> HairPPLLResolvePass::Create(const RPI::PassDescriptor& descriptor)
            {
                RPI::Ptr<HairPPLLResolvePass> pass = aznew HairPPLLResolvePass(descriptor);

                return AZStd::move(pass);
            }

            void HairPPLLResolvePass::InitializeInternal()
            {
                if (GetScene())
                {
                    FullscreenTrianglePass::InitializeInternal();
                }
            }

            bool HairPPLLResolvePass::AcquireFeatureProcessor()
            {
                if (m_featureProcessor)
                {
                    return true;
                }

                RPI::Scene* scene = GetScene();
                if (scene)
                {
                    m_featureProcessor = scene->GetFeatureProcessor<HairFeatureProcessor>();
                }
                else
                {
                    return false;
                }

                if (!m_featureProcessor)
                {
                    AZ_Warning("Hair Gem", false,
                        "HairPPLLResolvePass [%s] - Failed to retrieve Hair feature processor from the scene",
                        GetName().GetCStr());
                    return false;
                }
                return true;
            }

            void HairPPLLResolvePass::BuildInternal()
            {
                // No need to attach any buffer / image - it is done in the fill pass
                FullscreenTrianglePass::BuildInternal();
            }

            void HairPPLLResolvePass::SetupFrameGraphDependencies(RHI::FrameGraphInterface frameGraph)
            {
                FullscreenTrianglePass::SetupFrameGraphDependencies(frameGraph);

                if (!m_shaderResourceGroup || !AcquireFeatureProcessor())
                {
                    AZ_Error("Hair Gem", false, "HairPPLLResolvePass: PPLL list data was not bound - missing Srg or Feature Processor");
                    return;
                }

                UpdateGlobalShaderOptions();
                
                {
                    SrgBufferDescriptor descriptor = SrgBufferDescriptor(
                        RPI::CommonBufferPoolType::ReadWrite, RHI::Format::Unknown,
                        PPLL_NODE_SIZE, RESERVED_PIXELS_FOR_OIT,
                        Name{ "LinkedListNodesPPLL" }, Name{ "m_linkedListNodes" }, 0, 0
                    );
                    if (!UtilityClass::BindBufferToSrg("Hair Gem", m_featureProcessor->GetPerPixelListBuffer(), descriptor, m_shaderResourceGroup))
                    {
                        AZ_Error("Hair Gem", false, "HairPPLLResolvePass: PPLL list data could not be bound.");
                    }
                }
            }

            void HairPPLLResolvePass::CompileResources(const RHI::FrameGraphCompileContext& context)
            {
                if (!m_shaderResourceGroup || !m_featureProcessor)
                {
                    return;
                }

                // Update the material array constant buffer within the per pass srg
                SrgBufferDescriptor descriptor = SrgBufferDescriptor(
                    RPI::CommonBufferPoolType::Constant, RHI::Format::Unknown,
                    sizeof(AMD::TressFXShadeParams), 1,
                    Name{ "HairMaterialsArray" }, Name{ "m_hairParams" }, 0, 0
                );

                if (m_shaderResourceGroup->HasShaderVariantKeyFallbackEntry())
                {
                    m_shaderResourceGroup->SetShaderVariantKeyFallbackValue(m_shaderOptions);
                }

                m_featureProcessor->GetMaterialsArray().UpdateGPUData(m_shaderResourceGroup, descriptor);

                // All remaining srgs should compile here
                FullscreenTrianglePass::CompileResources(context);
            }

        } // namespace Hair
     }   // namespace Render
}   // namespace AZ

