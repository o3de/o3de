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
                o_enableShadows = AZ::Name("o_enableShadows");
                o_enableDirectionalLights = AZ::Name("o_enableDirectionalLights");
                o_enablePunctualLights = AZ::Name("o_enablePunctualLights");
                o_enableAreaLights = AZ::Name("o_enableAreaLights");
                o_enableIBL = AZ::Name("o_enableIBL");
                o_hairLightingModel = AZ::Name("o_hairLightingModel");
                o_enableMarschner_R = AZ::Name("o_enableMarschner_R");
                o_enableMarschner_TRT = AZ::Name("o_enableMarschner_TRT");
                o_enableMarschner_TT = AZ::Name("o_enableMarschner_TT");
                o_enableLongtitudeCoeff = AZ::Name("o_enableLongtitudeCoeff");
                o_enableAzimuthCoeff = AZ::Name("o_enableAzimuthCoeff");
            }

            void HairPPLLResolvePass::UpdateGlobalShaderOptions()
            {
                RPI::ShaderOptionGroup shaderOption = m_shader->CreateShaderOptionGroup();

                m_featureProcessor->GetHairGlobalSettings(m_hairGlobalSettings);

                shaderOption.SetValue(o_enableShadows, AZ::RPI::ShaderOptionValue{ m_hairGlobalSettings.m_enableShadows });
                shaderOption.SetValue(o_enableDirectionalLights, AZ::RPI::ShaderOptionValue{ m_hairGlobalSettings.m_enableDirectionalLights });
                shaderOption.SetValue(o_enablePunctualLights, AZ::RPI::ShaderOptionValue{ m_hairGlobalSettings.m_enablePunctualLights });
                shaderOption.SetValue(o_enableAreaLights, AZ::RPI::ShaderOptionValue{ m_hairGlobalSettings.m_enableAreaLights });
                shaderOption.SetValue(o_enableIBL, AZ::RPI::ShaderOptionValue{ m_hairGlobalSettings.m_enableIBL });
                shaderOption.SetValue(o_hairLightingModel, AZ::Name{ "HairLightingModel::" + AZStd::string(HairLightingModelNamespace::ToString(m_hairGlobalSettings.m_hairLightingModel)) });
                shaderOption.SetValue(o_enableMarschner_R, AZ::RPI::ShaderOptionValue{ m_hairGlobalSettings.m_enableMarschner_R });
                shaderOption.SetValue(o_enableMarschner_TRT, AZ::RPI::ShaderOptionValue{ m_hairGlobalSettings.m_enableMarschner_TRT });
                shaderOption.SetValue(o_enableMarschner_TT, AZ::RPI::ShaderOptionValue{ m_hairGlobalSettings.m_enableMarschner_TT });
                shaderOption.SetValue(o_enableLongtitudeCoeff, AZ::RPI::ShaderOptionValue{ m_hairGlobalSettings.m_enableLongtitudeCoeff });
                shaderOption.SetValue(o_enableAzimuthCoeff, AZ::RPI::ShaderOptionValue{ m_hairGlobalSettings.m_enableAzimuthCoeff });

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

                if (!m_featureProcessor || !m_featureProcessor->IsInitialized())
                {
                    AZ_Warning("Hair Gem", m_featureProcessor,
                        "HairPPLLResolvePass [%s] - Failed to retrieve Hair feature processor from the scene",
                        GetName().GetCStr());
                    m_featureProcessor = nullptr;   // set it as null if not initialized to repeat this check.
                    return false;
                }
                return true;
            }

            void HairPPLLResolvePass::CompileResources(const RHI::FrameGraphCompileContext& context)
            {
                if (!m_shaderResourceGroup || !AcquireFeatureProcessor())
                {
                    AZ_Error("Hair Gem", m_shaderResourceGroup, "HairPPLLResolvePass: PPLL list data was not bound - missing Srg");
                    return;     // no error message due to FP - initialization not complete yet, wait for the next frame
                }

                UpdateGlobalShaderOptions();

                if (m_shaderResourceGroup->HasShaderVariantKeyFallbackEntry())
                {
                    m_shaderResourceGroup->SetShaderVariantKeyFallbackValue(m_shaderOptions);
                }


                SrgBufferDescriptor descriptor = SrgBufferDescriptor(
                    RPI::CommonBufferPoolType::ReadWrite, RHI::Format::Unknown,
                    PPLL_NODE_SIZE, RESERVED_PIXELS_FOR_OIT,
                    Name{ "LinkedListNodesPPLL" }, Name{ "m_linkedListNodes" }, 0, 0
                );
                if (!UtilityClass::BindBufferToSrg("Hair Gem", m_featureProcessor->GetPerPixelListBuffer(), descriptor, m_shaderResourceGroup))
                {
                    AZ_Error("Hair Gem", false, "HairPPLLResolvePass: PPLL list data could not be bound.");
                }

                // Update the material array constant buffer within the per pass srg
                descriptor = SrgBufferDescriptor(
                    RPI::CommonBufferPoolType::Constant, RHI::Format::Unknown,
                    sizeof(AMD::TressFXShadeParams), 1,
                    Name{ "HairMaterialsArray" }, Name{ "m_hairParams" }, 0, 0
                );

                m_featureProcessor->GetMaterialsArray().UpdateGPUData(m_shaderResourceGroup, descriptor);

                // All remaining srgs should compile here
                FullscreenTrianglePass::CompileResources(context);
            }

        } // namespace Hair
     }   // namespace Render
}   // namespace AZ

