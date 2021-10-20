/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

//#include <Atom/RHI/CommandList.h>
#include <Atom/RHI/RHISystemInterface.h>
#include <Atom/RHI/DrawPacketBuilder.h>
#include <Atom/RHI/PipelineState.h>

#include <Atom/RPI.Public/View.h>
#include <Atom/RPI.Public/RPIUtils.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/RPISystemInterface.h>
#include <Atom/RPI.Public/Pass/PassUtils.h>
#include <Atom/RPI.Public/Scene.h>

#include <Atom/RPI.Reflect/Asset/AssetUtils.h>
#include <Atom/RPI.Reflect/Pass/RasterPassData.h>
#include <Atom/RPI.Reflect/Pass/PassTemplate.h>
#include <Atom/RPI.Reflect/Shader/ShaderAsset.h>

#include <Passes/HairShortCutGeometryShadingPass.h>
#include <Rendering/HairRenderObject.h>
#include <Rendering/HairFeatureProcessor.h>

namespace AZ
{
    namespace Render
    {
        namespace Hair
        {

            HairShortCutGeometryShadingPass::HairShortCutGeometryShadingPass(const RPI::PassDescriptor& descriptor)
                : HairGeometryRasterPass(descriptor)
            {
                SetShaderPath("Shaders/hairshortcutgeometryshading.azshader");  
            }

            RPI::Ptr<HairShortCutGeometryShadingPass> HairShortCutGeometryShadingPass::Create(const RPI::PassDescriptor& descriptor)
            {
                RPI::Ptr<HairShortCutGeometryShadingPass> pass = aznew HairShortCutGeometryShadingPass(descriptor);
                return pass;
            }

            void HairShortCutGeometryShadingPass::UpdateGlobalShaderOptions()
            {
                RPI::ShaderOptionGroup shaderOption = m_shader->CreateShaderOptionGroup();

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
                shaderOption.SetValue(AZ::Name("o_enableLongtitudeCoeff"), AZ::RPI::ShaderOptionValue{ m_hairGlobalSettings.m_enableLongtitudeCoeff });
                shaderOption.SetValue(AZ::Name("o_enableAzimuthCoeff"), AZ::RPI::ShaderOptionValue{ m_hairGlobalSettings.m_enableAzimuthCoeff });

                m_shaderOptions = shaderOption.GetShaderVariantKeyFallbackValue();
            }

            void HairShortCutGeometryShadingPass::CompileResources(const RHI::FrameGraphCompileContext& context)
            {
                if (!m_shaderResourceGroup || !AcquireFeatureProcessor())
                {
                    AZ_Error("Hair Gem", m_shaderResourceGroup, "HairShortCutGeometryShadingPass: missing Srg or no feature processor yet");
                    return;     // no error message due to FP - initialization not complete yet, wait for the next frame
                }

                UpdateGlobalShaderOptions();

                if (m_shaderResourceGroup->HasShaderVariantKeyFallbackEntry())
                {
                    m_shaderResourceGroup->SetShaderVariantKeyFallbackValue(m_shaderOptions);
                }

                // Update the material array constant buffer within the per pass srg
                SrgBufferDescriptor descriptor = SrgBufferDescriptor(
                    RPI::CommonBufferPoolType::Constant, RHI::Format::Unknown,
                    sizeof(AMD::TressFXShadeParams), 1,
                    Name{ "HairMaterialsArray" }, Name{ "m_hairParams" }, 0, 0
                );

                m_featureProcessor->GetMaterialsArray().UpdateGPUData(m_shaderResourceGroup, descriptor);

                // Compilation of remaining srgs will be done by the parent class 
                RPI::RasterPass::CompileResources(context);
            }

            void HairShortCutGeometryShadingPass::BuildInternal()
            {
                RasterPass::BuildInternal();    // change this to call parent if the method exists

                if (!AcquireFeatureProcessor())
                {
                    return;
                }

                LoadShaderAndPipelineState();
            }

        } // namespace Hair
    }   // namespace Render
}   // namespace AZ
